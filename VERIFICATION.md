# Verification — Drosophila melanogaster

**Lazirko Records · verified October 4, 2026**, Windows x64, JUCE **8.0.12**, MSVC **19.51.36244**, Visual Studio 2026, Windows SDK 10.0.26100, CMake 4.4.3. CPU reports AMD64 Family 26 Model 68. Measurements are host-specific, not universal real-time guarantees.

## Final artifacts and build checks

- CMake Release **VST3 and Standalone built successfully** under the new name. Final build log contains no project warnings/errors.
- [Fly.jucer](Fly.jucer) resaved with Projucer 8.0.12; generated x64 Release solution built with **0 warnings / 0 errors**. The actual Projucer VST3 independently scanned, processed audio and recalled state through the host test.
- **4/4 CTest suites passed after final plugin/core edits and byte-identical data regeneration**, **38.56 seconds**: core, processor, actual VST3 bundle, data audit. A subsequent strengthened disabled-audio assertion rebuilt and reran the complete affected processor suite plus live editor checks successfully.
- Python source compilation and four data tests passed. Official-input regeneration produced **byte-identical binary, both CSVs and provenance JSON** for the expanded graph.
- Product metadata checked through the actual VST3 scan: **Drosophila melanogaster / Lazirko Records**. Compatibility identifiers were retained.
- On-disk outputs: [Drosophila melanogaster.vst3](build/Fly_artefacts/Release/VST3/Drosophila%20melanogaster.vst3) (copy the **entire directory**) and [Drosophila melanogaster.exe](build/Fly_artefacts/Release/Standalone/Drosophila%20melanogaster.exe). Historical old-name outputs may still exist locally; they are not the current delivery.

Logs: [final-confirm-build.log](build/final-confirm-build.log), [final-confirm-tests.log](build/final-confirm-tests.log), [final-editor.log](build/final-editor.log), [final-disabled-audio.log](build/final-disabled-audio.log), [final-vst3-editor.log](build/final-vst3-editor.log), [final-projucer-build.log](build/final-projucer-build.log), [final-projucer-host.log](build/final-projucer-host.log), [final-regeneration.log](build/final-regeneration.log). Logs/builds are local artifacts, not source dependencies.

## Behaviour actually exercised

### Graph and simulation

- **731 neurons / 7,056 directed pairs / 38,765 detected synapses**, exact artifact hashes, binary/CSV consistency, materialisation/tag, unique 64-bit root IDs and auditory identity boundary.
- Reachability audit matches the **seven retained central cells with no detected JO path**; no reachability pruning or fabricated input. Coverage gaps and excluded visual_centrifugal identities are documented in [Data/COVERAGE.md](Data/COVERAGE.md).
- Loader bounds, truncated/corrupt graph rejection and model signs/weights.
- Silence/zero gain produces no spikes; nonzero JO and downstream response; seeded repeatability; balance response; bounded voltages; no capped ticks at supported stress settings.
- Concurrent SPSC transfer of **200,000 values** without order corruption.
- Visual-event queue intentionally overflows at maximum drive. Disabling visualisation leaves **identical spike totals and membrane trajectory**, produces no queued events or additional dropped-spike count, and cannot change propagation.
- Input features remain frequency-selective; anti-phase stereo does not cancel sensory power.

### Audio and plugin interface

- New defaults asserted: sensory gain **1**, time-scale **1**, wet/influence **1**, mode **0 clean filter** (normalisation tolerance 1e-5). Visualisation preference survives APVTS recall.
- Clean-filter linearity tested with half/full amplitude, response at low downstream rates, and transparent carrier at zero neural control. No explicit waveshaping/compression is invoked in clean mode. This is an engineering carrier mapping, not a physiological transfer-function validation.
- All four modes produce stable finite output, and startup wet=0 is exact bypass.
- Actual processor repeatedly prepared, processed and released in **mono/stereo, 64/128/256/512 samples at 48 kHz**. Short repeated lifecycle/mode stress intentionally uses sensory gain **4** to establish downstream activity within its shortest run; it does not demonstrate physiological calibration.
- Self-tested callback C++ heap interposition: **0 allocations / 0 deallocations across 5,600 blocks**. Scalar/array/aligned C++ operators are intercepted; direct C/OS allocation APIs and separately loaded DLLs are outside the audit. No dropped feature frames in paced integration tests.
- Actual compiled renamed VST3, through JUCE's VST3 hosting API: one effect scanned, correct product/company, **untouched gain-1 clean defaults** modified paced 220 Hz anti-phase stereo with finite non-silent output, state saved/recalled. This checks the real wrapper, not only linked processor code.
- Native VST3 editor wrapper showed, resized to 1020×900 and closed under live processing.

### Live visualisation-disable button

The direct native editor was driven by a paced gain-4 test stimulus. Actual APVTS-attached coordinate/population/disable buttons were exercised; labels and resize were checked.

- Direct GL framebuffer read: **GL error 0**, **97,498 illuminated pixels**.
- **Disable visualisation:** attached button set the host parameter, renderer disabled, context detached, visualActive=false, event production disabled. Rendered-frame counter held at **196** across the one-second measured disabled interval. Input and downstream audio-control rates remained nonzero.
- **Audio while disabled:** **57,088 finite processed samples** were checked, with nonzero neural modification; disabling the view did not silently bypass or corrupt audio.
- **Re-enable:** host parameter returned to zero, context reattached, visualActive=true, rendered-frame counter advanced again.
- Captured live label examples: JO **62.8 Hz**, AMMC-B1 **1.7 Hz**, central auditory **1.0 Hz**; drops **0 / 0**. These are synthetic stimulus responses, not validated fly firing rates.
- Displayed rates come from the exact latest callback control-rate reads; nominal band centres and selected mode match audio settings. Spike flashes are actual model events; structural zero-fast-weight lines remain dim/non-transmitting.

Native Windows wrapper/editor snapshots can be black under composition and are **not** offered as visual evidence. The directly read framebuffer is [Reports/circuit.png](Reports/circuit.png).

## Measured optimisation

Fair before/after comparison uses the **same expanded graph, final carrier cutoff, inputs, Release compiler and benchmark harness**. Median of nine repetitions; every DSP sample and sampled intermediate/final neural trajectory is hashed. Hashes match exactly, including block-wise automated mode switches and 48/96 kHz feature changes.

| Two-second stereo features + DSP | Before | After | Compute-time reduction |
|---|---:|---:|---:|
| Clean, 48 kHz | 3.041 ms | 2.192 ms | **27.9%** |
| Clean, 96 kHz | 6.054 ms | 4.334 ms | **28.4%** |
| Artistic blend, 48 kHz | 3.154 ms | 2.731 ms | 13.4% |
| Automated modes, 48 kHz | 3.119 ms | 2.393 ms | 23.3% |

Band coefficients are cached when unchanged; stereo modulation is computed once per sample; inactive nonlinear modes are not evaluated; filter state remains continuous across mode switches. Sensory probabilities are calculated once per feature frame and visual flags once per tick. **LIF timing differences were only 0.1–0.7%; no meaningful LIF speedup is claimed.** Hashing is included in benchmark time, host/GL overhead is not. This is not a total DAW CPU reduction claim. Full table/method: [Reports/optimisation.md](Reports/optimisation.md); harness: [Tests/Benchmark.cpp](Tests/Benchmark.cpp).

Final core sustained frame: **two simulated seconds in 0.0221785 s**, **31,139 spikes**, ~**1.11%** one-core budget at 1× speed. Maximum gain **32** / **4×** speed with visual overflow: **one audio second / four simulated seconds in 0.060214 s**, **316,292 spikes**, ~**6.02%** one-core budget. These are representative/stress inputs, not exhaustive worst-case proofs.

Final paced callback averages (wet=0 features/DSP still run; C++ heap audit enabled):

| Buffer | Mono | Stereo | Audio deadline |
|---:|---:|---:|---:|
| 64 | 1.23 μs | 3.21 μs | 1,333 μs |
| 128 | 3.17 μs | 4.99 μs | 2,667 μs |
| 256 | 8.40 μs | 11.47 μs | 5,333 μs |
| 512 | 12.06 μs | 17.74 μs | 10,667 μs |

Averages are not worst-case latency and exclude worker/graphics compute. Driver, scheduler, background load and sample rate matter. Disabling GL was measured as **zero frame-count advancement and detached context**, not an invented CPU/GPU percentage saving.

## Review artifacts

- [Reports/preview.html](Reports/preview.html): reviewed in the running browser; graph image loaded and audio decoder reported readyState=4 with no error.
- [Reports/circuit.png](Reports/circuit.png): real GL framebuffer, 890×680 pixels.
- [Reports/editor-status.txt](Reports/editor-status.txt): live native labels/rates/settings.
- [Reports/host-output.wav](Reports/host-output.wav): actual VST3 clean-default output, **6.8667 s**, stereo 48 kHz PCM16; verified readable/non-silent (peak ≈0.5). Duration follows samples recorded, not wall-clock time under the Windows sleep scheduler. Browser decoded duration **6.866667 s**. No subjective listening evaluation is claimed.

## Limitations — not marked as passes

No commercial DAW, macOS/Linux build, hardware microphone loopback, listening judgement, C/OS allocation audit, exhaustive worst-case latency proof or GPU frame-time distribution was tested. Standalone was built; hardware device routing was not validated. The editor's 60 Hz requests are not a guaranteed presentation rate. Offline sample-deterministic bounce is not implemented; unpaced hosts can overflow asynchronous features.

No calibrated antennal mechanics or natural sensory gain, physiological-response fit, complete auditory census, subjective fly-hearing equivalence or waveform resynthesis is established. Missing JON synapses, uncertain transmitter signs, zero-fast-weight modulators and unresolved/multimodal identities remain explicit scientific limitations. See [README.md](README.md), [Data/COVERAGE.md](Data/COVERAGE.md) and [Data/ATTRIBUTION.md](Data/ATTRIBUTION.md).

No repository commit or GitHub publication was performed; this workspace has no Git metadata. Documentation is ready for repository review, not a claim of deployment.
