# Drosophila melanogaster

### Lazirko Records · connectome-constrained auditory audio effect

A C++17 / JUCE 8 **mono/stereo VST3 effect and Standalone application** using a real FlyWire auditory subgraph. Incoming audio activates Johnston's organ (JO-A/JO-B) model neurons. Detected synapses propagate leaky integrate-and-fire (LIF) activity; downstream population rates control the audio filter. An OpenGL view displays that same circuit and its actual simulated spikes.

> **This is a computational model, not a living fruit fly or a calibrated reconstruction of fly hearing.** The waveform remains a conventional DSP carrier controlled by a biological-connectivity-constrained simulation. It is not reconstructed from spikes. Sensory frequency mapping, dBFS-to-activation conversion and the output filter are engineering assumptions. There is no validated subjective-hearing, song-discrimination or complete auditory-census claim.
> 
<img width="1807" height="1406" alt="image" src="https://github.com/user-attachments/assets/36cdd96e-833d-4053-bf3d-91fe49a9b600" />

## Quick start

1. Build using CMake or Projucer below.
2. Copy the **entire** VST3 bundle to your DAW's user VST3 scan directory and rescan.
3. Insert it on an audio track. Try a 150–350 Hz sine or pulse stimulus, then music.
4. Start with the clean defaults; increase sensory gain if your signal produces little downstream activity.
5. Select **Disable visualisation** to stop the OpenGL renderer while retaining audio and neural simulation.

Standalone requires an audio input/output device. Unmute input if JUCE's feedback-protection control has muted it. Use headphones to avoid microphone/speaker feedback. The plugin does not generate its own test tone.

## Clean defaults and controls

| Parameter | Default | Meaning |
|---|---:|---|
| Output mode | Auditory filter (clean) | Neural-controlled linear low-pass; no gate or waveshaper |
| Sensory gain | 1 | Uncalibrated input-to-JO activation scale, **not** input audio gain |
| JO-B / JO-A balance | 0.5 | Equal relative sensory weighting |
| JO band centre | 300 Hz | Logarithmic midpoint of the feature bands |
| Band separation | 1.5 octaves | Nominal JO-B / JO-A centres ≈178 / 505 Hz |
| Simulation time-scale | 1× | One simulated second per input-audio second |
| Wet / dry | 1 | Fully wet carrier mapping |
| Neural influence | 1 | Full downstream-rate influence |
| Disable visualisation | Off | Persistent host-state control |

These are **biologically inspired / engineering defaults**, not measured natural hearing parameters. Natural auditory stimuli are particle-velocity signals; arbitrary dBFS cannot establish antenna velocity. The nominal band centres are not fitted JO-A/JO-B tuning curves. Broad first-order difference bands overlap, and their nominal centres are not brick-wall cutoffs.

- **Clean filter:** AMMC-B1 and central-auditory rates determine filter amount and a nominal 1200→150 Hz cutoff; AMMC-A1/A2/B2 rates determine resonance. At zero downstream rates the carrier is transparent. Linear filtering avoids the explicit nonlinear distortion used by the optional modes; it may still attenuate/change phase or produce transients. Time-varying neural control is not a fixed linear time-invariant transfer function.
- **Auditory gating / compression**, **Auditory waveshaping** and **Artistic circuit blend** are optional creative mappings, not natural hearing claims.
- Either wet or influence zero is transparent **after parameter smoothing settles**; startup wet=0 is exact bypass. Parameters smooth over 20 ms.
- Brightness, decay, neuron colour, connection display, population visibility and FAFB anchor coordinates affect the view only. Hidden populations remain simulated.
- Drag to pan, wheel to zoom, double-click to reset. Default stages are schematic, not a claim of a feedforward circuit.

The UI displays the current nominal JO band centres and selected output mapping. Rate meters show the **latest population rates read by the audio callback**, rather than unrelated animation or synthetic activity. JO is a population-size-weighted mean; AMMC-B1 and central auditory are population means. Rates are Hz in simulated time. Input is RMS dBFS. Status reports worker compute ratio and dropped feature/visual-event counters.

## Real auditory data and coverage

The embedded [auditory.flygraph](Data/auditory.flygraph) contains **731 v783 neurons, 7,056 directed pairs and 38,765 detected synapses**. No runtime download, Python, authentication or random network generation is needed.

| Population | Neurons |
|---|---:|
| JO-A | 94 |
| JO-B | 299 |
| AMMC-A1 | 5 |
| AMMC-A2 | 2 |
| AMMC-B1 | 81 |
| AMMC-B2 | 6 |
| Central auditory: WED / AVLP / PVLP / IPS / SAD / GNG-associated classes | 244 |

Selection uses publication-matched Schlegel annotations (`v2.1.0`) and explicit original study-curator labels from **Christa Baker / Claire McKellar** in Codex v783. A neuron innervating an auditory neuropil is not automatically classified auditory. Ambiguous “-like” labels are rejected; central candidates must be intrinsic and in the annotation's central super-class.

The expanded allowlist includes confirmed **GNG_pr01, IPS_pr02, AVLP_pr23 (AMMC-VLP2), WV-WV and WV-WV-1** identities. All confirmed candidates are retained, including **seven central cells without a detected directed JO path**. Their IDs are audited in [provenance.json](Data/provenance.json); they receive no invented drive. Eight GNG_pr01 curator-labeled cells annotated visual_centrifugal are excluded by the central-only scope. AVLP_pr18/pr24 and SAD/AMMC_pr01 lack resolved accepted v783 identities. **The graph is not every relevant auditory neuron.** See the explicit coverage evidence and gaps in [COVERAGE.md](Data/COVERAGE.md).

All detected pairs within the selected whole-cell graph are included (≥1 synapse), including recurrent/self connections; counts across neuropils are summed. Cells may arborise outside AMMC/WED, so whole-cell identity selection is not a neuropil crop. No descending neurons, Giant Fiber, optic-lobe expansion, mushroom body, other JO groups or other sensory drives are included. Runtime/CSV population slot 6 retains the historical string `WED/IVLP auditory` for schema compatibility; the UI correctly calls it **Central auditory**.

### Missing JO synapses are not fabricated

Baker et al. report poor automatic JON synapse detection and use membrane contacts plus manual validation. This project uses only **released detected synapse counts**, never substitutes contacts or invents JO→AMMC edges. JO transmitter predictions are often missing/uncertain and are not forced cholinergic. Missing/zero-weight sensory links materially affect the response; low-drive stimuli may produce little central activity.

Audit 64-bit root IDs, class evidence, label IDs, predicted transmitter/confidence and anchor positions in [neurons.csv](Data/neurons.csv); pairs/counts/model signs/neuropils in [edges.csv](Data/edges.csv); hashes/source URLs in [provenance.json](Data/provenance.json). Data are **CC BY 4.0** with adaptations documented in [ATTRIBUTION.md](Data/ATTRIBUTION.md).

## Build

Requirements: CMake ≥3.22, local **JUCE 8** checkout (tested 8.0.12), C++17 compiler and OpenGL 3.2-capable drivers. Windows requires MSVC + Windows SDK; macOS requires Xcode command-line tools; Linux requires the standard JUCE ALSA/X11/OpenGL/Freetype development packages. No Eigen/Brian/Python runtime is needed by the plugin.

```bash
cmake -S . -B build -DJUCE_PATH=/path/to/JUCE
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

Windows example (match your installed generator):

```bash
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DJUCE_PATH=G:/JUCE
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

Multi-config Windows outputs:

```text
build/Fly_artefacts/Release/VST3/Drosophila melanogaster.vst3/
build/Fly_artefacts/Release/Standalone/Drosophila melanogaster.exe
```

Single-config generators should use `-DCMAKE_BUILD_TYPE=Release` and omit the configuration directory in output paths. macOS produces platform-specific bundles/apps. Builds do not install into system directories. CMake target `Fly`, manufacturer/plugin codes, bundle identifier and APVTS state type remain unchanged to preserve identity; old saved parameter values are not replaced with new defaults.

[Fly.jucer](Fly.jucer) also builds VST3 + Standalone. Open in Projucer, set your JUCE module path, save and build the exporter solution. The Windows exporter currently points to `G:/JUCE/modules`. JUCE 8.0.12 uses `juce_audio_processors_headless`; older versions may need a module-list adjustment. CMake is the primary portable build. Unused original application-template sources are outside both build lists.

JUCE is not vendored. Check **JUCE AGPL/commercial licensing** before distribution. CC-BY data licensing does not license JUCE or choose a licence for this application's source. This workspace has no Git metadata; nothing has been published or pushed to GitHub.

## Model parameters and numerical assumptions

[AuditoryPathwaySimulator.cpp](Source/AuditoryPathwaySimulator.cpp) implements fixed **0.2 ms** timesteps with outgoing CSR propagation. Shared constants are inspired by Shiu's published model code, not fitted to individual auditory cells:

| Quantity | Value |
|---|---:|
| Rest / reset | −52 mV |
| Spike threshold | −45 mV |
| Membrane time constant | 20 ms |
| Synaptic decay | 5 ms |
| Refractory interval | 2.2 ms |
| Transmission delay | 1.8 ms (nine ticks) |
| Per-edge impulse | 0.275 mV × count × model sign |
| Population rate smoother | 50 ms EWMA, simulated time |

Predicted ACh is excitatory; GABA/glutamate inhibitory. Dopamine/serotonin/octopamine/unknown edges are stored with **zero fast weight**, not mislabeled excitatory. Predicted transmitter does not establish receptor-specific sign. No modulator/receptor dynamics are inferred.

Sensory activation is Bernoulli sampling of Poisson-event probability `1 − exp(−rate × dt)`, capped at 350 Hz and one event per step, injecting 68.75 mV into JO only. Unlike Shiu's optogenetic targets, JO cells here share the model refractory period. No downstream tonic/background drive is added. RNG is seeded for repeatable sequential simulation, not physiological randomness validation.

The input extractor uses two first-order difference-band envelopes, full-band RMS and positive envelope change on fixed ~1 ms windows. Stereo powers are averaged so anti-phase signals do not cancel drive. There is no antennal mechanics, phase coding, FFT or pitch model. Voltage/conductance and per-frame tick caps ensure bounded work; operator splitting and shared cell constants are further approximations.

## Real-time and visual alignment

- **Audio callback:** bounded, preallocated feature extraction and stereo DSP; POD features go to a 2,048-frame SPSC ring and lock-free rates return. No LIF loop, allocation, file access, mutex, thread wake or GL call.
- **Worker:** high-priority JUCE thread owns LIF state and advances using input audio time. No input means no advancement. Host lifecycle stops/joins the worker before reset/reuse. Neural state/RNG is not saved in presets; APVTS controls are.
- **Asynchrony:** feature-window + worker scheduling + neural/rate smoothing determines response latency; no fixed sample delay is promised. Faster-than-real-time offline rendering can overflow; sample-deterministic offline bounce is **not implemented**. Overflow drops newest features and reports it without blocking audio.
- **Visuals:** actual simulated neuron spike IDs populate a 65,536-event queue. Flashes are recent activity with visual decay, not calibrated firing rates. Lines are real stored pairs; excitation/inhibition colours refer to **model** sign. Presynaptic highlights are not animations of exact delayed synaptic arrival. Zero-fast-weight edges are dim grey and never flash as transmissions. FAFB spatial view is an x/z anchor projection, not soma positions or reconstructed morphology.
- **Disable visualisation:** detaches the GL context, stops its 60 Hz repaint timer, stops spike-event production and clears queued flashes. The editor's lightweight 30 Hz meters still update; **audio and LIF simulation continue**. Re-enable reattaches with fresh activity. Closing the editor also stops visual work. In-flight worker events may briefly remain during transition, without affecting neural propagation.
- **Optimisation:** unchanged feature-band coefficients are cached, stereo modulation is shared, unused nonlinear mappings are not computed in clean mode, and sensory exponentials/visual flags are hoisted out of per-neuron work. Filter state continues in all modes to preserve automated switching. Measured clean feature/DSP time fell **27.9% at 48 kHz / 28.4% at 96 kHz**, with matching sample/trajectory hashes. This is not a whole-plugin/DAW speedup claim; no meaningful LIF speedup is claimed. See [optimisation.md](Reports/optimisation.md).
- Loading is bounded to 4,096 neurons, 1,000,000 edges and 32 MB. Invalid data reports an error and **dry bypass**, never a synthetic graph.

## Tests and review

[VERIFICATION.md](VERIFICATION.md) records build/interface checks, actual measurements and untested platforms. Core-only tests need no JUCE:

```bash
cmake -S . -B build-core -DFLY_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --config Release
ctest --test-dir build-core -C Release --output-on-failure
```

- [CoreTests.cpp](Tests/CoreTests.cpp): graph validation, queues, seeded LIF, silence/zero gain, 4× stress, visual-overflow trajectory invariance, anti-phase features, clean-filter linearity and all DSP modes.
- [PluginTests.cpp](Tests/PluginTests.cpp): APVTS/defaults/recall, threaded prepare/process/release, mono/stereo blocks, all modes and callback C++ heap audit. `--editor` feeds a native editor, tests actual attached buttons, GL detachment/frame stoppage and re-enable, meters and resize. Short repeated lifecycle stress uses gain 4; the VST3 test uses untouched gain-1 clean defaults.
- [Vst3HostTests.cpp](Tests/Vst3HostTests.cpp): scans the **actual bundle**, checks product/company, processes anti-phase stereo at defaults and recalls state. `--editor` opens its native wrapper and records output.
- [test_data.py](Tests/test_data.py): hashes, selected identities, binary/CSV consistency and audited unreachable cells.
- [Benchmark.cpp](Tests/Benchmark.cpp): repeated 48/96 kHz feature/DSP and actual-graph trajectory/timing measurements, including mode automation.

[Reports/preview.html](Reports/preview.html) presents a real GL framebuffer and recorded VST3 audio, not a web version of the plugin. Native Windows snapshots may be black under composition; only the directly read GL framebuffer is offered as visual evidence. Harness tones are test inputs, not a plugin feature.

## Regenerate data

Python 3.10+, ~1 GB cache disk and RAM for one Arrow record batch:

```bash
python -m venv .venv
# Activate the environment using your shell's normal command.
python -m pip install -r scripts/requirements.txt
python scripts/fetch_sources.py
python scripts/extract_auditory.py
python Tests/test_data.py
# Rebuild to embed the new binary.
```

[fetch_sources.py](scripts/fetch_sources.py) verifies Zenodo's MD5 and records SHA-256. [extract_auditory.py](scripts/extract_auditory.py) streams the ~852 MB proofread pair archive, preserves 64-bit IDs, selects identities, aggregates pairs, audits reachability without pruning confirmed cells, and writes binary/CSV/provenance. Annotation tag is pinned; Codex URLs may evolve independently, so compare source hashes. Increasing `--min-synapses` changes the graph and can disconnect cells. Ontology coverage evidence is documented separately; it does not generate edges.

**Binary schema:** little-endian header `FLYAUD1\0`, uint32 release=783/neuronCount/edgeCount. Neurons: uint64 root ID; uint8 population/transmitter; int8 model sign; pad byte; float32 x/y/z nm and NT confidence; uint16 UTF-8 label length; label. Edges: uint32 pre/post/count; float32 model weight mV. [DataLoader.cpp](Source/DataLoader.cpp) validates and creates outgoing CSR. Incoming connections from excluded cells are absent.

## Citations

1. Dorkenwald et al. (2024), *Neuronal wiring diagram of an adult brain*. Nature. https://doi.org/10.1038/s41586-024-07558-y
2. Schlegel et al. (2024), *Whole-brain annotation and multi-connectome cell typing of Drosophila*. Nature. https://doi.org/10.1038/s41586-024-07686-5
3. Baker et al. (2022), *Neural network organization for courtship-song feature detection in Drosophila*. Current Biology 32, 3317–3333.e7. https://doi.org/10.1016/j.cub.2022.06.019
4. Shiu et al. (2024), *A Drosophila computational brain model reveals sensorimotor processing*. Nature. https://doi.org/10.1038/s41586-024-07763-9 · [published parameter code](https://github.com/philshiu/Drosophila_brain_model/blob/main/model.py). Their whole-brain model used v630; this is not a reproduction of it.
5. Kim et al. (2020), *Wiring patterns from auditory sensory neurons to the escape and song-relay pathways in fruit flies*. J. Comp. Neurol. 528, 2068–2098. Background, not a source of fabricated/transferred edges.
6. FlyBase / Virtual Fly Brain, Drosophila anatomy ontology, auditory class definitions and Baker references; specific terms/release/hash in [COVERAGE.md](Data/COVERAGE.md).

## Scientific and delivery limitations

Single female FAFB specimen; not validated for male hearing or behaviour. Incomplete auditory coverage and missing JO synapses; coarse historic labels; weak-edge false positives; uncertain transmitter signs; no electrical synapses, calibrated mechanics/phase coding, receptor dynamics, cell-specific membrane fits, background inputs or plasticity. No neural waveform resynthesis, Giant-Fiber/descending or granular mode. No physiological-response fit, subjective listening evaluation or commercial-DAW/platform-wide performance guarantee. Scientific authors/data contributors do not endorse this plugin.
