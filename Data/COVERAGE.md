# Auditory coverage audit

## Release and identity policy

Runtime graph: **FlyWire FAFB materialisation 783**, publication annotations **v2.1.0**. Counts: **731 neurons / 7,056 directed pairs / 38,765 detected synapses**. Exact per-class counts and seven unreachable central root IDs are in [provenance.json](provenance.json); original curator label IDs/text/authorship in [neurons.csv](neurons.csv).

Only explicit accepted identities are used. Central candidates must have `flow=intrinsic` and `super_class=central` in the pinned annotations. No auditory identity is inferred solely from an arbor in AVLP/WED/AMMC. “-like”, uncertain, negative and connectivity-note labels are rejected. No root-ID rounding, missing-edge fabrication, membrane-contact-to-synapse conversion or forced JO transmitter assignment is used.

## Expanded identities and evidence

Baker et al. (2022): https://doi.org/10.1016/j.cub.2022.06.019 . Original study-curator v783 labels from Christa Baker and Claire McKellar provide the identity-to-root mapping. Anatomical ontology definitions independently support auditory class interpretation; they do **not** establish measured physiology for each mapped root.

| Added class | Retained v783 cells | Evidence / caveat |
|---|---:|---|
| GNG_pr01 | 1 | FBbt:00052103; auditory, sine-preferring class, GABAergic definition |
| IPS_pr02 | 2 | FBbt:00052104; auditory responses to both modes, GABAergic definition |
| AVLP_pr23 | 2 | Exact synonym of AMMC-VLP2, FBbt:00111356; auditory both modes, cholinergic definition |
| WV-WV | 7 | Established early auditory class and unambiguous original curator labels |
| WV-WV-1 | 2 | Unambiguous original curator labels; longer slash-combination labels are not automatically accepted |

The ontology terms cite Baker under `FlyBase:FBrf0254226`. Evidence snapshot: **Drosophila anatomy ontology `fbbt/releases/2026-10-01`**, retrieved from https://raw.githubusercontent.com/FlyBase/drosophila-anatomy-developmental-ontology/master/fbbt.obo . SHA-256: `bc1eba4d1bb78eb9438ba3b284856d251566670b5e8128d0ff159ba81bd4f2e3`. The URL evolves; use the recorded release/hash when comparing evidence. Repository: https://github.com/FlyBase/drosophila-anatomy-developmental-ontology . Ontology credit: FlyBase / Virtual Fly Brain contributors, CC BY 4.0. This evidence is a selection rationale, not an additional connectivity input to extraction.

Other accepted classes are explicitly listed in [extract_auditory.py](../scripts/extract_auditory.py), including the original Baker auditory set, established AMMC-A1/A2/B1/B2, and JO-A/JO-B. Detailed original type names are preserved rather than renaming every cell to a generic WED population. Population slot 6 retains the old serialized name `WED/IVLP auditory` for compatibility; the view labels its broader coverage **Central auditory**.

## Cells retained despite missing detected input

The prior reachability pruning was removed. All confirmed allowlisted candidates remain, including **seven central cells with no directed path from JO in the detected sparse graph**. These are audited, not supplied artificial background/input edges. Their spontaneous activity is not guaranteed; lack of detected reachability is not evidence that the biological cell is non-auditory.

## Known omissions / scope conflicts

- **Eight of nine** GNG_pr01 curator-labeled roots have intrinsic **visual_centrifugal** annotations (cM19 / PVLP046). They fail the central-only boundary and are deliberately excluded rather than silently expanding the requested auditory-only scope into visual circuitry. The retained root is `720575940624666245` (central SAD017).
- **AVLP_pr18 / AVLP_pr24:** auditory classes in Baker, but no confidently resolved accepted v783 curator identity in this audit. Their identities/edges are not invented.
- **SAD/AMMC_pr01:** included in the accepted class vocabulary, but no exact accepted curator-labeled cell found in the release.
- Many additional AVLP anatomical terms alone do not demonstrate auditory responses; the entire neuropil is not swept into the graph.
- No descending/Giant-Fiber, other JO sensory groups, optic-lobe expansion, mushroom-body or other sensory inputs. Such pathways could be studied in a separately declared expanded scope.
- Automatic JON synapse detection is incomplete. Baker's manual/contact analysis cannot be replaced by these released detected counts. Weak detected edges may also be false positives.

**Conclusion:** this is a larger, auditable confirmed auditory subset—not a complete auditory connectome or validated natural hearing model. A full census requires manual identity resolution, synapse validation and a declared decision on multimodal/descending boundaries.
