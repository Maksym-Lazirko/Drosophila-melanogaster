# Data attribution and reuse

The files in this directory are an adapted subset of the **FlyWire FAFB female adult Drosophila melanogaster connectome, materialization 783**. They are distributed under **Creative Commons Attribution 4.0 International (CC BY 4.0)**: https://creativecommons.org/licenses/by/4.0/ . Retain this attribution, the source links and the description of modifications when redistributing the data.

## Sources and credits

- **FlyWire Consortium**, *FlyWire Whole-brain Connectome Connectivity Data*, release 783.0, June 2, 2024: https://zenodo.org/records/10676866 . The official `proofread_connections_783.feather` archive supplies every edge and synapse count in this subset.
- **Dorkenwald et al. (2024)**, *Neuronal wiring diagram of an adult brain*. Nature. https://doi.org/10.1038/s41586-024-07558-y . Credit the FlyWire Consortium and the community proofreaders.
- **Schlegel et al. (2024)**, *Whole-brain annotation and multi-connectome cell typing of Drosophila*. Nature. https://doi.org/10.1038/s41586-024-07686-5 . Publication-matched annotations, tag `v2.1.0`: https://github.com/flyconnectome/flywire_annotations/tree/v2.1.0 . These supply v783 root IDs, JO identity, anchor coordinates and predicted transmitter/confidence.
- **Baker et al. (2022)**, *Neural network organization for courtship-song feature detection in Drosophila*. Current Biology 32, 3317–3333.e7. https://doi.org/10.1016/j.cub.2022.06.019 . Auditory identity is constrained by this work and original study-curator labels (**Christa Baker and Claire McKellar**) in the official Codex v783 export: https://storage.googleapis.com/flywire-data/codex/data/fafb/783/labels.csv.gz . Label IDs and curator attribution are retained per neuron. This is not the paper's original v11/v274 graph, and its historic root IDs are not reused as v783 IDs.
- Synapse-detection and neurotransmitter-prediction contributors described by the official dataset: **Buhmann et al. (2021)**, **Heinrich et al. (2018)**, and **Eckstein, Bates et al. (2024)**. Predicted transmitter is not a measured receptor-specific synaptic sign.

- **FlyBase / Virtual Fly Brain anatomy ontology contributors**, release `fbbt/releases/2026-10-01`, CC BY 4.0: https://github.com/FlyBase/drosophila-anatomy-developmental-ontology . Auditory definitions support the expanded class allowlist; terms, snapshot hash and deliberate omissions are recorded in [COVERAGE.md](COVERAGE.md). The ontology contributes identity evidence, not synapses.

## Modifications

We select only JO-A/JO-B and conservatively identified central auditory classes, exclude ambiguous “-like” labels and non-central intrinsic populations, and retain all confirmed allowlisted central cells, including seven without a detected directed JO path. Reachability is audited, not used to erase established identities. We sum per-neuropil counts for each directed pair, retain pairs with at least one detected synapse, keep recurrent and self connections present in v783, and derive an outgoing sparse adjacency matrix. No edges are synthesized. The distributed graph is a conservative subset, not a complete auditory census or a reproduction of Baker's manually assessed JON contact analysis.

Coordinates are annotation **anchor** positions converted from 4 × 4 × 40 nm voxel coordinates to nanometres. Runtime spatial display projects x/z; the default stage layout is schematic.

Modeled weights are `0.275 mV × synapse count × modeled sign`: ACh = +1, GABA/glutamate = −1, other/unknown = 0. This is explicitly a simplified LIF assumption; zero-weight edges remain stored. Low-confidence and missing JO transmitter predictions are not relabeled. Hashes, source URLs and selection statistics are in [provenance.json](provenance.json). The compact binary is [auditory.flygraph](auditory.flygraph); human-readable audits are [neurons.csv](neurons.csv) and [edges.csv](edges.csv).

Neither these authors nor the FlyWire Consortium endorse this audio plugin. JUCE's licensing is separate from the CC-BY data license.
