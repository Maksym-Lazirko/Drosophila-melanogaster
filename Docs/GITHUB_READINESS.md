# GitHub readiness — Drosophila melanogaster

## Prepared locally

- Active plugin source, tests and extraction tooling are separated from generated builds and unused original app-template code.
- Research drafts/reference evidence live in [Docs/Research](Research/README.md); curated plugin media/measurements remain in `Reports/`.
- [Ignore rules](../.gitignore) exclude every `build-*` directory, original `build/`, Projucer `Builds/` / `JuceLibraryCode/`, download caches, virtual environments, agent state, logs, secrets and the local cleanup archive.
- [Attributes](../.gitattributes) normalize source text while preserving exact data bytes needed by hash tests.
- [README](../README.md) documents current CMake target/output paths, four modes, true Delta monitoring and compatibility. [Verification](../VERIFICATION.md) distinguishes current checks from historical evidence.
- No Git repository was initialized and no files were staged, committed, pushed, deployed or published.

## Before publication

1. **Choose the source-code licence.** There is currently no application licence. JUCE AGPL/commercial obligations must be reviewed independently; data [CC BY 4.0 attribution](../Data/ATTRIBUTION.md) does not license the application.
2. **Review research/data disclosures.** The CSV/provenance and reference evidence intentionally include scientific IDs, curator attribution and scholarly metadata. Author placeholders remain in research drafts. Decide what belongs in a public repository; no publication consent is inferred by organizing files.
3. **Initialize Git only when requested.** `.gitignore` does not remove already tracked files. This workspace has no `.git`; in a future repository, inspect tracked files before publication.
4. **Regenerate workstation exports.** `Fly.jucer` retains this machine's `G:/JUCE/modules` exporter setting; set your local JUCE path and resave. CMake is the portable primary build. Ignored old exporter binaries are not the current Delta build.
5. **Use a verified release artifact.** Current Windows x64 outputs are under `build-rel/Drosophila_artefacts/Release/`. Copy the whole VST3 bundle. No installation or REAPER rescan has been performed by this pass.

## Checks and boundaries

The plugin Release build, core/processor/actual-VST3/data tests and native Delta/editor controls are verified in [VERIFICATION](../VERIFICATION.md). Documentation links and the GitHub-facing file list are audited using Git's native ignore/attribute evaluation with temporary isolated metadata, without initializing the workspace. See the [audit](../Reports/github-audit.json). Local binaries/logs are not source dependencies. Files over GitHub's 100 MiB per-file limit must not enter the source set; downloaded archives remain ignored.

No secret scanner or licence audit certification is claimed. Commercial DAW/hardware routing, macOS/Linux/AU, subjective listening, deterministic offline bounce, physiological calibration, author declarations and official journal-template certification remain untested or unresolved.
