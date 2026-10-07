# Research documents

“Fruit Fly” names the research concept; the plugin is **Drosophila melanogaster** by **Lazirko Records**. These documents analyse the October 4, 2026 prototype snapshot. They do not claim current Delta functionality, later DSP colour changes, new participant results, or biological validation as evidence for the research propositions.

- [CAIS manuscript](Fruit_Fly_CAIS_Manuscript.md) · [Word draft](Fruit_Fly_CAIS_Manuscript.docx) · [HTML review](Fruit_Fly_CAIS_Review.html)
- [Cover letter](Fruit_Fly_CAIS_Cover_Letter.md) · [Word draft](Fruit_Fly_CAIS_Cover_Letter.docx)
- [Submission checklist](Fruit_Fly_CAIS_Submission_Checklist.md) · [current package checks](CAIS_Package_Verification.md)
- [Original research draft](Fruit_Fly_Research_Paper.md) · [reference audit](Fruit_Fly_Reference_Audit.md)
- [Journal shortlist](ABDC_Journal_Shortlist.md) · [citation alignment](CAIS_Citation_Alignment_Audit.md)

Current implemented controls/builds/tests: [README](../../README.md), [VERIFICATION](../../VERIFICATION.md). Historical media and benchmarking: [Reports](../../Reports/preview.html).

## Document tooling

```bash
python Docs/Research/check_manuscript_readiness.py
# Uses the already-installed python-docx package in the local virtual environment:
.venv/Scripts/python.exe Docs/Research/prepare_submission_documents.py
```

The generator requires `python-docx` (tested 1.2.0) and writes DOCX/HTML beside the Markdown. It is separate from the plugin/data runtime. The checker runs from any working directory and asserts ten numbered sections, three tables, twenty references, eighteen reference DOI URLs, and explicit evidence boundaries. Counts are approximate, not editorial certification.

Author details, disclosures, licence/immutable evidence packaging, scholarly review and official-template/pagination checks remain required. Nothing has been submitted or published.
