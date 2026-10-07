#!/usr/bin/env python3
"""Conservative auditory-only extraction. Never infer auditory identity from neuropil alone."""
import argparse
import collections
import csv
import gzip
import hashlib
import json
import math
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
# Conservative Baker 2022 auditory classes (main text plus VirtualFlyBrain
# auditory ontology entries for AVLP_pr11, AVLP_pr12, PVLP_pr03), plus the
# specified established early auditory classes. No '*-like' identities.
AUDITORY_CLASSES = {
    'AVLP_pr01', 'AVLP_pr02', 'AVLP_pr04', 'AVLP_pr05', 'AVLP_pr11',
    'AVLP_pr12', 'AVLP_pr22', 'AVLP_pr31', 'AVLP_pr32', 'AVLP_pr35',
    'AVLP_pr36', 'WED_pr01', 'WED_pr02', 'WED-VLP', 'IPS_pr01',
    'IPS/WED_pr01', 'SAD_pr01', 'SAD_pr02', 'SAD/AMMC_pr01',
    'AVLP/PVLP_pr01', 'PVLP_pr03', 'GNG_pr01', 'IPS_pr02',
    'AVLP_pr23', 'WV-WV', 'WV-WV-1',
}
POPULATIONS = ['JO-A', 'JO-B', 'AMMC-A1', 'AMMC-A2', 'AMMC-B1', 'AMMC-B2', 'WED/IVLP auditory']
NT_CODES = {'acetylcholine': 1, 'gaba': 2, 'glutamate': 3, 'dopamine': 4, 'serotonin': 5, 'octopamine': 6}

def read_labels(path):
    result = collections.defaultdict(list)
    with gzip.open(path, 'rt', encoding='utf-8') as f:
        for row in csv.DictReader(f):
            # Original auditory study curators; retain exact label and authorship.
            if row['user_name'] in ('Christa Baker', 'Claire McKellar'):
                result[row['root_id']].append(row)
    return result

def classify(row, labels):
    if row['cell_type'] in ('JO-A', 'JO-B'):
        return row['cell_type'], row['cell_type'], 'Schlegel v2.1.0 cell_type'
    if row['flow'] != 'intrinsic' or row['super_class'] != 'central':
        return None
    candidates = []
    if row['hemibrain_type'] == 'AMMC-A1':
        candidates.append(('AMMC-A1', 'AMMC-A1', 'Schlegel v2.1.0 hemibrain_type'))
    for label in labels:
        text = label['label'].strip()
        if any(x in text.lower() for x in ('-like', 'not ', 'possibly', '?', 'topinput', 'topoutput')):
            continue
        match = re.match(r'^AMMC-([AB][12])(?:$|,|/)', text)
        cell = 'AMMC-' + match[1] if match else None
        if cell is None:
            # Full label begins with an accepted type, followed by annotation suffix.
            cell = next((t for t in sorted(AUDITORY_CLASSES, key=len, reverse=True)
                         if text == t or text.startswith(t + ' ') or text.startswith(t + ';')), None)
        if cell:
            population = cell if cell.startswith('AMMC-') else 'WED/IVLP auditory'
            candidates.append((population, cell, f"Codex label {label['label_id']} by {label['user_name']}: {text}"))
    if not candidates:
        return None
    if len({c[0] for c in candidates}) > 1:
        raise ValueError(f"Conflicting population labels for {row['root_id']}: {candidates}")
    return sorted(candidates)[0]

def sha256(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        while block := f.read(4 * 1024 * 1024):
            h.update(block)
    return h.hexdigest()

def extract(cache, output, minimum):
    import pyarrow as pa
    import pyarrow.ipc as ipc
    labels = read_labels(cache / 'labels.csv.gz')
    candidates = {}
    with (cache / 'annotations.tsv').open(encoding='utf-8') as f:
        for row in csv.DictReader(f, delimiter='\t'):
            identity = classify(row, labels.get(row['root_id'], []))
            if identity:
                candidates[int(row['root_id'])] = (row, identity)
    counts = collections.Counter(item[1][0] for item in candidates.values())
    if any(counts[p] == 0 for p in POPULATIONS):
        raise ValueError(f'Missing required auditory population: {counts}')
    # Feather v2 = Arrow IPC file. Decode one record batch at a time.
    edges = collections.Counter()
    neuropils = collections.defaultdict(set)
    with pa.memory_map(str(cache / 'connections.feather'), 'r') as source:
        reader = ipc.open_file(source)
        for b in range(reader.num_record_batches):
            batch = reader.get_batch(b)
            pre = batch.column(batch.schema.get_field_index('pre_pt_root_id')).to_pylist()
            post = batch.column(batch.schema.get_field_index('post_pt_root_id')).to_pylist()
            number = batch.column(batch.schema.get_field_index('syn_count')).to_pylist()
            regions = batch.column(batch.schema.get_field_index('neuropil')).to_pylist()
            for a, z, n, region in zip(pre, post, number, regions):
                if a in candidates and z in candidates:
                    edges[a, z] += n
                    neuropils[a, z].add(region)
    edges = {e: n for e, n in edges.items() if n >= minimum}
    # Audit directed sensory reachability, but retain every confirmed candidate.
    # A missing detected JO path is a dataset limitation, not a reason to erase
    # an established auditory neuron or invent an input to it.
    reachable = {i for i, (_, c) in candidates.items() if c[0].startswith('JO-')}
    while True:
        enlarged = reachable | {post for pre, post in edges if pre in reachable}
        if enlarged == reachable:
            break
        reachable = enlarged
    ids = sorted(candidates, key=lambda i: (POPULATIONS.index(candidates[i][1][0]), i))
    if len(ids) > 4096:
        raise ValueError('Extraction exceeds the 4096-neuron real-time safety limit')
    index = {root: i for i, root in enumerate(ids)}
    output.mkdir(parents=True, exist_ok=True)
    rows = []
    for root in ids:
        r, (pop, cell, evidence) = candidates[root]
        nt = r['top_nt'].lower()
        # Polarity is a model assumption, not a receptor-specific observation.
        polarity = 1 if nt == 'acetylcholine' else -1 if nt in ('gaba', 'glutamate') else 0
        rows.append({'index': index[root], 'root_id': str(root), 'population': pop,
                     'cell_type': cell, 'flywire_cell_type': r['cell_type'], 'hemibrain_type': r['hemibrain_type'],
                     'nt_prediction': nt, 'nt_confidence': r['top_nt_conf'], 'modeled_polarity': polarity,
                     'side': r['side'], 'x_nm': int(r['pos_x']) * 4,
                     'y_nm': int(r['pos_y']) * 4, 'z_nm': int(r['pos_z']) * 40,
                     'identity_evidence': evidence})
    selected_edges = [(index[a], index[b], n, rows[index[a]]['modeled_polarity'], ';'.join(sorted(neuropils[a, b])))
                      for (a, b), n in sorted(edges.items()) if a in index and b in index]
    # Compact little-endian binary: magic/version/counts; fixed neuron records;
    # variable UTF-8 labels; edges stored ordered by presynaptic index (CSR load).
    with (output / 'auditory.flygraph').open('wb') as f:
        f.write(struct.pack('<8sIII', b'FLYAUD1\0', 783, len(rows), len(selected_edges)))
        for r in rows:
            label = r['cell_type'].encode('utf-8')
            f.write(struct.pack('<QBBbxffffH', int(r['root_id']), POPULATIONS.index(r['population']),
                                NT_CODES.get(r['nt_prediction'], 0), r['modeled_polarity'],
                                float(r['x_nm']), float(r['y_nm']), float(r['z_nm']),
                                float(r['nt_confidence'] or 0), len(label)))
            f.write(label)
        for a, b, n, polarity, _ in selected_edges:
            f.write(struct.pack('<IIIf', a, b, n, 0.275 * n * polarity))
    with (output / 'neurons.csv').open('w', newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0])); writer.writeheader(); writer.writerows(rows)
    with (output / 'edges.csv').open('w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f); writer.writerow(['pre_index', 'post_index', 'synapse_count', 'modeled_polarity', 'neuropils']); writer.writerows(selected_edges)
    actual = collections.Counter(r['population'] for r in rows)
    report = {
        'schema': 'FLYAUD1', 'materialization': 783, 'annotations_tag': 'v2.1.0',
        'license': 'CC-BY-4.0', 'neuron_count': len(rows), 'edge_count': len(selected_edges),
        'synapse_count': sum(e[2] for e in selected_edges), 'populations': dict(actual),
        'minimum_synapses_per_pair': minimum, 'candidate_populations': dict(counts),
        'excluded_unreachable_central_neurons': 0,
        'unreachable_central_root_ids': [str(i) for i in ids if i not in reachable],
        'cell_classes': dict(collections.Counter(r['cell_type'] for r in rows)),
        'coverage_policy': 'Retain all confirmed allowlisted cells, including cells lacking a detected JO path. No invented inputs.',
        'selection': 'Explicit auditory cell labels; central intrinsic only; no neuropil-only expansion; no descending, MB or optic neurons.',
        'coordinate_space': 'FAFB14.1 anchor coordinates in nm; not soma or morphology',
        'polarity_assumption': 'ACh +1; GABA and glutamate -1; modulators and unknown 0. Predicted NT does not establish receptor-specific sign.',
        'JO_detection_warning': 'Baker 2022 reports poor automated JON synapse detection. Missing links remain absent; membrane contact counts are NOT substituted.',
        'sources': json.loads((cache / 'sources.json').read_text(encoding='utf-8')),
        'artifacts': {n: sha256(output / n) for n in ('auditory.flygraph', 'neurons.csv', 'edges.csv')},
    }
    (output / 'provenance.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: v for k, v in report.items() if k not in ('sources', 'artifacts')}, indent=2))
    return report

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', type=Path, default=ROOT / '.cache' / 'flywire')
    parser.add_argument('--output', type=Path, default=ROOT / 'Data')
    parser.add_argument('--min-synapses', type=int, default=1, help='Keep all observed direct connections by default')
    args = parser.parse_args()
    if args.min_synapses < 1:
        parser.error('--min-synapses must be >= 1')
    extract(args.cache, args.output, args.min_synapses)

if __name__ == '__main__':
    main()
