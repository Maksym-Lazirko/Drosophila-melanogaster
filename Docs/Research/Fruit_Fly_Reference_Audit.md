# Fruit Fly manuscript reference and evidence audit

Audit date: 4 October 2026. Manuscript: [Fruit_Fly_Research_Paper.md](Fruit_Fly_Research_Paper.md).

## Bibliographic verification

Twelve DOI-bearing references were checked against their registration-agency metadata. Exact author/title/volume/page details were cross-checked with accessible primary full text, publisher catalogues or institutional records. Raw registration metadata: [Fruit_Fly_Reference_Metadata.json](Fruit_Fly_Reference_Metadata.json).

| Reference | Verification source | Identifier/status |
|---|---|---|
| Baker et al. (2022) | Crossref; [PMC primary article](https://pmc.ncbi.nlm.nih.gov/articles/PMC9378594/) | 10.1016/j.cub.2022.06.019 verified; hyphenated title follows Crossref/published title |
| Brooke (2013) | [ACM catalogue record](https://dl.acm.org/citation.cfm?id=2817913), indexed author/title/year/pages; author's scholarly record | Journal of Usability Studies 8(2), 29–40 verified; catalogue URL used, not an asserted registered DOI |
| Clemens et al. (2015) | Crossref; [PMC primary article](https://pmc.ncbi.nlm.nih.gov/articles/PMC4629847/) | 10.1016/j.neuron.2015.08.014 verified |
| Clemens et al. (2018) | Crossref; [PMC primary article](https://pmc.ncbi.nlm.nih.gov/articles/PMC5760620/) | 10.1038/s41467-017-02453-9 verified; article number 134 confirmed in primary text |
| FlyWire Consortium (2024) | DataCite; [Zenodo release record](https://zenodo.org/records/10676866) | 10.5281/zenodo.10676866 verified; version 783.0; dataset, not an article |
| Göpfert and Robert (2002) | Crossref; publisher indexed record; [Bristol institutional record](https://research-information.bris.ac.uk/en/publications/the-mechanical-basis-of-idrosophilai-audition/) | 10.1242/jeb.205.9.1199 verified |
| Hevner (2007) | [AIS publisher archive](https://aisel.aisnet.org/sjis/vol19/iss2/4/); indexed archive PDF first-page metadata | Scandinavian Journal of Information Systems 19(2), 87–92 verified; no DOI asserted |
| Hevner et al. (2004) | Crossref; [Arizona institutional bibliography](https://experts.arizona.edu/en/publications/design-science-in-information-systems-research); indexed article first page and ACM catalogue | 10.2307/25148625 verified; 75–105 retained from article/institutional sources despite Crossref's 75–106 entry |
| Lyon (2017) | Crossref; [Cambridge publisher catalogue](https://www.cambridge.org/core/books/human-and-machine-hearing/3660166B40020EE587D94BB7A309FC12) | 10.1017/9781139051699 verified; full subtitle/publisher confirmed |
| Peffers et al. (2007) | Crossref; publisher/ACM indexed records | 10.2753/MIS0742-1222240302 verified; 24(3), 45–77; publication year 2007, not online-posting year 2014 |
| Shiu et al. (2024) | Crossref; [Princeton institutional bibliography](https://collaborate.princeton.edu/en/publications/a-drosophila-computational-brain-model-reveals-sensorimotor-proce/) | 10.1038/s41586-024-07763-9 verified; APA first 19 authors + ellipsis + final author for >20 authors |
| Verfaille et al. (2006) | Crossref; indexed IEEE/ACM/HAL records | 10.1109/TSA.2005.858531 verified; 14(5), 1817–1831 |
| Yamada et al. (2018) | Crossref; [PMC primary article](https://pmc.ncbi.nlm.nih.gov/articles/PMC6596007/) | 10.1523/JNEUROSCI.3644-17.2018 verified |
| Zölzer (2011) | Crossref; [Wiley publisher catalogue](https://www.wiley.com/en-fr/DAFX%3A+Digital+Audio+Effects%2C+2nd+Edition-p-9780470665992); DAFx book site | 10.1002/9781119991298 verified; edited book, second edition |

Several publisher pages refused full-text extraction (403/406), and HAL returned a bot challenge. These refusals were not bypassed. Bibliographic metadata were checked using the alternate sources above. The dataset DOI is registered at DataCite, not Crossref. A transient Crossref rate limit for the DAFX book was resolved by a later metadata request. ACM's 10.5555-style catalogue identifier for Brooke was not found in Crossref; the manuscript therefore cites the verified catalogue URL instead of treating it as a verified registered DOI. Hevner (2007) likewise uses its journal archive URL.

## Artifact evidence boundaries

Completed technical results are explicitly attributed to the local developer verification record, not to an independent publication or participant experiment. Supporting documents: [VERIFICATION.md](../../VERIFICATION.md), [README.md](../../README.md), [coverage audit](../../Data/COVERAGE.md), [optimisation report](../../Reports/optimisation.md), inspected implementation and test source.

- The research concept is **Fruit Fly**; actual compiled product remains **Drosophila melanogaster**, Lazirko Records.
- VST3 and Standalone are implemented; AU is proposed.
- Explicit IPI detection/gating, sensory adaptation, species profiles and several conventional controls are proposed, not claimed implemented.
- Musical drum/guitar/pulse/mix cases are conceptual/proposed. Existing tone/pulse host recording is identified separately.
- Eight experts is a proposed recruitment target, not a participant count. No listening findings, p-values or statistical significance are fabricated.
- The remaining protocol is proposed, not preregistered.
- Preset parameter recall is distinguished from neural-state persistence and deterministic offline rendering.
- Biological inspiration is not complete hearing replication, calibrated natural parameters or new validated neuroscience.

## Final format check

Final manuscript contains all ten requested sections, two numbered tables, two textual figures, six keywords, a structured 179-word abstract, 14 APA-style references, and an availability statement with unknown repository/licence/release information left as placeholders. Automated counts and relative links were checked after editing; references are excluded from the stated length. No plugin source was changed for the paper, so earlier passing plugin checks were not needlessly rerun.
