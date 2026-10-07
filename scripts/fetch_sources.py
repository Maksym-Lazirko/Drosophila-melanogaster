#!/usr/bin/env python3
"""Download official v783 data; no credentials or third-party services required."""
import hashlib
import json
from pathlib import Path
import urllib.request

CACHE = Path(__file__).resolve().parents[1] / '.cache' / 'flywire'
SOURCES = {
    'annotations.tsv': ('https://raw.githubusercontent.com/flyconnectome/flywire_annotations/v2.1.0/supplemental_files/Supplemental_file1_neuron_annotations.tsv', None),
    'connections.feather': ('https://zenodo.org/records/10676866/files/proofread_connections_783.feather?download=1', 'f48f972d262323a102aed49af1396b8a'),
    'labels.csv.gz': ('https://storage.googleapis.com/flywire-data/codex/data/fafb/783/labels.csv.gz', None),
    'classification.csv.gz': ('https://storage.googleapis.com/flywire-data/codex/data/fafb/783/classification.csv.gz', None),
    'neurons.csv.gz': ('https://storage.googleapis.com/flywire-data/codex/data/fafb/783/neurons.csv.gz', None),
    'consolidated_cell_types.csv.gz': ('https://storage.googleapis.com/flywire-data/codex/data/fafb/783/consolidated_cell_types.csv.gz', None),
    'connections.csv.gz': ('https://storage.googleapis.com/flywire-data/codex/data/fafb/783/connections.csv.gz', None),
}

def main():
    CACHE.mkdir(parents=True, exist_ok=True)
    manifest = {}
    for name, (url, expected) in SOURCES.items():
        path = CACHE / name
        if not path.exists():
            temporary = path.with_suffix('.partial')
            print('Downloading', url, flush=True)
            with urllib.request.urlopen(url, timeout=120) as response, temporary.open('wb') as output:
                while chunk := response.read(4 * 1024 * 1024):
                    output.write(chunk)
            temporary.replace(path)
        md5 = hashlib.md5()
        sha = hashlib.sha256()
        with path.open('rb') as source:
            while chunk := source.read(4 * 1024 * 1024):
                md5.update(chunk)
                sha.update(chunk)
        if expected and md5.hexdigest() != expected:
            raise RuntimeError(f'Checksum mismatch: {path}; remove the corrupted download and retry')
        manifest[name] = {'url': url, 'bytes': path.stat().st_size, 'md5': md5.hexdigest(), 'sha256': sha.hexdigest()}
        print(name, manifest[name], flush=True)
    (CACHE / 'sources.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')

if __name__ == '__main__':
    main()
