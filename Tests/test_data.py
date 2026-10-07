#!/usr/bin/env python3
import csv
import hashlib
import json
from pathlib import Path
import struct
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from extract_auditory import AUDITORY_CLASSES, POPULATIONS, classify

class DataTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.provenance = json.loads((ROOT / 'Data/provenance.json').read_text(encoding='utf-8'))
        with (ROOT / 'Data/neurons.csv').open(encoding='utf-8') as source:
            cls.neurons = list(csv.DictReader(source))
        with (ROOT / 'Data/edges.csv').open(encoding='utf-8') as source:
            cls.edges = list(csv.DictReader(source))

    def test_integrity(self):
        for name, expected in self.provenance['artifacts'].items():
            self.assertEqual(hashlib.sha256((ROOT / 'Data' / name).read_bytes()).hexdigest(), expected)
        self.assertEqual(len(self.neurons), self.provenance['neuron_count'])
        self.assertEqual(len(self.edges), self.provenance['edge_count'])
        self.assertEqual(sum(int(e['synapse_count']) for e in self.edges), self.provenance['synapse_count'])
        self.assertEqual(self.provenance['materialization'], 783)
        self.assertEqual(self.provenance['annotations_tag'], 'v2.1.0')

    def test_auditory_boundary_and_reachability(self):
        self.assertEqual(len({r['root_id'] for r in self.neurons}), len(self.neurons))
        for r in self.neurons:
            self.assertIn(r['population'], POPULATIONS)
            self.assertTrue(r['cell_type'] in AUDITORY_CLASSES or r['cell_type'] in POPULATIONS[:6])
            self.assertNotIn('like', r['identity_evidence'].lower())
            self.assertGreater(int(r['root_id']), 2**53)
        reached = {int(r['index']) for r in self.neurons if r['population'].startswith('JO-')}
        while True:
            enlarged = reached | {int(e['post_index']) for e in self.edges if int(e['pre_index']) in reached}
            if reached == enlarged:
                break
            reached = enlarged
        unreachable = {r['root_id'] for r in self.neurons if int(r['index']) not in reached and not r['population'].startswith('JO-')}
        self.assertEqual(unreachable, set(self.provenance['unreachable_central_root_ids']))
        self.assertEqual(len(unreachable), 7)
        self.assertEqual(self.provenance['excluded_unreachable_central_neurons'], 0)
        self.assertIn('retain', self.provenance['coverage_policy'].lower())

    def test_binary_matches_csv(self):
        data = (ROOT / 'Data/auditory.flygraph').read_bytes()
        magic, version, n, m = struct.unpack_from('<8sIII', data)
        self.assertEqual((magic, version, n, m), (b'FLYAUD1\0', 783, len(self.neurons), len(self.edges)))
        offset = 20
        for row in self.neurons:
            values = struct.unpack_from('<QBBbxffffH', data, offset); offset += 30
            root, population, nt, sign, x, y, z, confidence, length = values
            self.assertEqual(str(root), row['root_id']); self.assertEqual(POPULATIONS[population], row['population'])
            self.assertEqual(sign, int(row['modeled_polarity'])); self.assertEqual(x, float(row['x_nm']))
            self.assertEqual(y, float(row['y_nm'])); self.assertEqual(z, float(row['z_nm']))
            self.assertEqual(data[offset:offset + length].decode('utf-8'), row['cell_type']); offset += length
            self.assertGreaterEqual(confidence, 0); self.assertLessEqual(confidence, 1)
        for edge in self.edges:
            pre, post, count, weight = struct.unpack_from('<IIIf', data, offset); offset += 16
            self.assertEqual((pre, post, count), tuple(int(edge[k]) for k in ('pre_index', 'post_index', 'synapse_count')))
            self.assertAlmostEqual(weight, count * int(edge['modeled_polarity']) * 0.275, delta=0.0001)
        self.assertEqual(offset, len(data))

    def test_no_unsafe_identity_inference(self):
        central = {'cell_type': '', 'hemibrain_type': 'WED001', 'flow': 'intrinsic', 'super_class': 'central', 'root_id': '123'}
        self.assertIsNone(classify(central, []))
        for text in ('AMMC-B1-like', 'possibly AMMC-B1', 'not a known auditory neuron', 'AVLP_pr15-like'):
            self.assertIsNone(classify(central, [{'label': text, 'label_id': '1', 'user_name': 'Christa Baker'}]))
        optic = dict(central, super_class='optic'); self.assertIsNone(classify(optic, [{'label': 'AMMC-B1', 'label_id': '1', 'user_name': 'Christa Baker'}]))

if __name__ == '__main__':
    unittest.main()
