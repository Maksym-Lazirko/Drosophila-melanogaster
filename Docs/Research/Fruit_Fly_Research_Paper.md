# Fruit Fly: Translating Drosophila Auditory Processing into a Bio-Inspired Musical Effect Through Design Science

> **Historical draft:** this paper describes the October 4, 2026 software snapshot, before Delta listening and subsequent DSP colour changes. See the [current verification](../../VERIFICATION.md) for the latest implemented behaviour. Research proposals remain unvalidated.

## Abstract

**Problem:** Musical audio effects offer extensive signal transformations, but nonhuman auditory mechanisms remain an underexplored source of explicit, traceable design constraints. **Method:** This artifact study applies the six-stage Design Science Research Methodology to translate selected findings on *Drosophila melanogaster* audition into musical signal-processing abstractions. **Artifact:** “Fruit Fly” denotes the research-facing design; its implemented precursor, distributed locally as “Drosophila melanogaster,” combines frequency-selective features, a connectivity-constrained neural simulation, and neural-controlled filtering in a VST3 effect and Standalone application. Explicit inter-pulse-interval processing, adaptation, species presets, and Audio Unit support are proposed extensions. **Evaluation:** Documented developer tests establish bounded responses on tested inputs, host-state recall, continued audio processing with visualisation disabled, and reduced feature/DSP computation time. Comparative musical assessment and an expert usability study are specified prospectively, not reported as completed. **Contribution:** The study formulates reusable principles for separating biological evidence from artistic mappings, preserving temporal relationships, exposing interpretable controls, and evaluating control dynamics independently of timbral appeal. **Limitations:** The artifact neither reproduces complete fly audition nor supplies new validated neurobiological findings; ecological validity, perceptual usefulness, and broader host compatibility remain unresolved.

**Keywords:** design science; Drosophila audition; bio-inspired audio; digital audio effects; temporal processing; adaptive signal processing

## 1. Introduction

Conventional audio effects typically organise interaction around filters, dynamics, delay, modulation, and nonlinear processing (Zölzer, 2011). These categories encompass substantial technical diversity, yet they do not themselves provide a diverse catalogue of biological listening strategies. Human auditory models offer established engineering precedents (Lyon, 2017); alternative organisms may supply different organising principles. This is a design motivation, not a quantified market-wide claim.

*Drosophila melanogaster* audition is particularly instructive because acoustic communication involves both within-pulse oscillation and patterns extending across pulses and song bouts. Frequency selection, temporal filtering, inhibition, and adaptation can consequently be considered together rather than reduced to an insect-themed equaliser.

The research question is: **How can selected temporal and frequency-processing characteristics of the Drosophila auditory system be translated into a usable audio plug-in, and what design knowledge emerges from building and evaluating that artifact?**

“Fruit Fly” names the research concept; the existing Lazirko Records prototype retains its compiled name, “Drosophila melanogaster.” Practical contributions comprise a working connectivity-constrained effect and a specification for additional temporal modules. Scholarly contributions comprise an evidence-to-implementation mapping, provisional design principles, and an evaluation protocol separating software performance, musical utility, and biological correspondence. None establishes how a fly subjectively hears music.

## 2. Background

### 2.1. Biological evidence

Male courtship song includes sine and pulse modes. The arista and antenna form a mechanically selective receiver, and Johnston’s organ neurons transduce antennal movement (Göpfert & Robert, 2002). Importantly, this system responds to particle velocity in near-field communication; digital amplitude in dBFS is not a calibrated equivalent of antennal stimulation.

**Intra-pulse frequency** describes oscillations within a pulse. **Inter-pulse interval** (IPI) describes spacing between successive pulses, commonly operationalised between their onsets. These are distinct dimensions: changing the carrier frequency need not change pulse timing. Approximately 35 ms is characteristic of melanogaster pulse spacing, but it is not a universal preferred interval across neurons or behavioural assays. Yamada et al. (2018) demonstrated inhibitory shaping of AMMC-B1 responses, particularly suppression at short intervals. Conversely, Clemens et al. (2015) found that longer-timescale song structure, rather than variation within the observed conspecific IPI range, explained aspects of female locomotor responses.

**Transient encoding** concerns changes at sound onset, offset, or intensity transitions. Clemens et al. (2015) described biphasic temporal filters and adaptation in sampled central neurons; their predominantly graded responses caution against treating every auditory neuron as a spike-only unit. **Temporal filtering** describes weighting of stimulus history, not simply removing high audio frequencies. **Sensory adaptation** changes sensitivity with stimulus statistics: Clemens et al. (2018) demonstrated fast intensity adaptation and distinguished subtractive mean correction from divisive intensity correction. A conventional compressor is therefore an analogy, not an established equivalent.

Baker et al. (2022) additionally showed interconnected auditory populations with diverse song-mode preferences and response timescales. A serial module diagram should not be mistaken for this recurrent biological architecture.

**Table 1**  
*Biological mechanism, engineering abstraction, implementation, and expected sonic effect*

| Biological mechanism | Engineering abstraction | Plug-in implementation/status | Expected sonic effect—not a listening finding |
|---|---|---|---|
| Antennal frequency selection (Göpfert & Robert, 2002) | Tunable frequency weighting | Implemented overlapping feature bands; direct resonant band-pass proposed | Spectrally selective modulation or narrowed timbre |
| Transient responses (Clemens et al., 2018) | Envelope-change detection | Positive RMS change implemented; explicit pulse/envelope shaper proposed | Attack-sensitive modulation |
| Interval-dependent inhibition (Yamada et al., 2018) | Event-spacing selector | Explicit IPI detector/gate proposed; current network interval tuning unvalidated | Selective response to pulse spacing |
| Central temporal filtering (Clemens et al., 2015) | Low-pass/biphasic control kernels | 50 ms rate smoother implemented; fitted biphasic kernels proposed | Slower evolving control, reduced flutter |
| Intensity adaptation (Clemens et al., 2018) | Divisive gain regulation | Dedicated adaptation module proposed; optional compression is not equivalent | Reduced sustained-level sensitivity, onset contrast |
| Context-dependent song processing (Baker et al., 2022) | Parameter-profile selection | Species/behaviour presets proposed, without species-validation claims | Contrasting spectral/temporal profiles |

### 2.2. Engineering context and gap

Auditory engineering already uses filter banks, rectification, and gain regulation (Lyon, 2017). Adaptive effects also map extracted signal features to transformation parameters (Verfaille et al., 2006). Fruit Fly does not originate these techniques. Its proposed distinction is their organisation around explicitly sourced Drosophila temporal mechanisms, combined with an auditable connectivity-constrained precursor. The methodological gap concerns understandable control without conflating creative usefulness with physiological validity; comparable effects are not ruled out.

## 3. Design-Science Method

The study follows Peffers et al.’s (2007) six-stage methodology and treats the artifact as a **software instantiation accompanied by design principles**. Following Hevner et al. (2004), construction is necessary but insufficient: claims must also be evaluated against requirements.

Hevner’s (2007) **relevance cycle** connects the project to production needs, including controllability and real-time operation; direct practitioner validation remains prospective. The **rigor cycle** connects primary biological studies and DSP literature to explicit abstractions. The **design cycle** connects implementation, test evidence, and revision. This account is a retrospective structuring of documented development, not evidence that an experimental protocol was preregistered before construction.

**Table 2**  
*Design-science stages, project activities, and evidence produced*

| Stage | Project activity | Output and evidence status |
|---|---|---|
| 1. Problem identification | Identify translation and usability risks | Literature-grounded problem; practitioner confirmation proposed |
| 2. Solution objectives | Specify traceability, stability, and controllability | Explicit requirements below |
| 3. Design and development | Build feature, neural, carrier, and interface layers | Existing source, embedded graph, test harnesses |
| 4. Demonstration | Process synthetic tones/pulses; specify musical cases | Recorded host output; musical walkthrough proposed |
| 5. Evaluation | Test software and benchmark computation | Completed developer checks; comparative/user study proposed |
| 6. Communication | Document implementation, limits, and principles | Local reproducibility materials and this manuscript; publication pending |

## 4. Design Requirements

Seven requirements govern development and evaluation:

1. **Real-time operation:** mono/stereo processing must meet host deadlines without blocking the audio callback; commercial-DAW validation remains necessary.
2. **Stability:** tested operating ranges must produce finite output, with bounded internal states, explicit overload reporting, and documented output-level limits. Finite output is not a guarantee against clipping.
3. **Controllable transformation:** listeners must be able to obtain an audible effect and reduce it continuously, including a settled transparent mix setting. Perceptibility requires listening evidence.
4. **Biological traceability:** each module must identify its source mechanism, computational substitution, and unvalidated assumptions.
5. **Latency and resources:** distinguish carrier-path latency from feature-window, scheduling, and control-response delays; report timing distributions and hardware rather than a generic “low latency” claim.
6. **Automation and reproducibility:** persist parameter settings, test automation transitions, and distinguish preset reproducibility from identical stochastic audio renders.
7. **Interpretive honesty:** scientific labels and creative controls must remain distinguishable; neither presets nor visualisations should imply organism-specific perceptual validation.

Requirements are partly satisfied by developer checks, but perceptual usefulness, DAW coverage, and temporal-module performance remain open. Build success does not establish effectiveness.

## 5. Artifact Design

### 5.1. Implemented architecture

The C++17/JUCE prototype separates analysis and carrier processing. Approximately 1 ms feature windows measure two overlapping first-order difference-band envelopes, full-band RMS, and positive envelope change. Stereo powers are averaged rather than waveforms summed, preventing anti-phase cancellation of sensory drive. Features activate JO-A/JO-B model neurons; a worker advances a leaky integrate-and-fire (LIF) network using released connectivity (FlyWire Consortium, 2024).

The embedded subset contains **731 neurons, 7,056 directed pairs, and 38,765 detected synapses**. Seven confirmed central cells lacking detected JO reachability remain without fabricated inputs. Missing detected connections and uncertain transmitter predictions are retained as limitations. The LIF approach is informed by Shiu et al. (2024), but their validated sensorimotor results do not validate this auditory adaptation.

Simulation uses 0.2 ms steps, a 20 ms membrane time constant, 5 ms synaptic decay, and 1.8 ms transmission delay. Population activity is exponentially smoothed over 50 ms of simulated time. These shared constants are modelling assumptions, not fits to each cell. Downstream rates control a two-pole low-pass carrier; optional mappings provide gating/compression, waveshaping, or a blend. The input waveform is not regenerated from spikes.

**Figure 1. Conceptual signal flow.**

```text
Input ─────────────────────────────► carrier filter / optional mapping ─► mix ─► output
  └► frequency features + RMS/onset ─► JO drive ─► recurrent LIF graph
                                                    └► smoothed rates ─► control mapping
                                                    └► optional visualisation

Proposed control-path extensions: explicit pulse events → IPI selection;
level statistics → adaptation. Proposed interfaces: species profiles and AU.
```

*Note.* Solid-path operations are implemented. The carrier/control separation is architectural; the neural graph is recurrent, not the depicted linear block sequence.

### 5.2. Mechanism-to-module mappings

Table 1 distinguishes implemented mechanisms from proposed extensions.

**Antennal Tuning** currently weights the control signal rather than directly band-passing the audible output. Its default 300 Hz midpoint and 1.5-octave separation yield nominal centres near 178 and 505 Hz. These broad engineering bands are not fitted antennal tuning curves. A proposed resonant band-pass would expose frequency and resonance separately, with limits preventing uncontrolled amplification.

**Pulse Extraction** currently contributes positive envelope change to sensory activation. A proposed explicit detector would add hysteresis and a refractory interval, followed by adjustable envelope shaping. Those additions would distinguish pulses from noise and sustained oscillation; the existing onset feature alone does not establish reliable musical onset segmentation.

**IPI Processing** would calculate onset separation, \(\Delta_j=t_j-t_{j-1}\), and an illustrative selectivity weight,

\[
q_j=\exp[-(\Delta_j-T)^2/(2\sigma^2)].
\]

Here, \(T\) and \(\sigma\) are artistic target interval and tolerance, not fitted neuronal constants. This proposed Gaussian selector is easier to inspect than a network-only response, but less mechanistic than inhibitory circuitry. Beat subdivisions must not be silently equated with courtship IPIs.

**Neural Low-Pass** operates on control activity. A reusable smoother is

\[
u[n]=\alpha u[n-1]+(1-\alpha)v[n],\qquad \alpha=e^{-1/(f_s\tau)}.
\]

This converts a time constant into sample-rate-dependent coefficients. The implemented rate smoother instead advances at the simulation timestep; a prospective adjustable envelope smoother would use the audio-rate form. Simple smoothing cannot reproduce every biphasic or interval-selective response.

**Adaptation** would estimate recent intensity and divide sensitivity by a bounded positive denominator. Attack/recovery controls would be evaluated independently of a musical compressor. Neither the prototype’s fixed refractory period nor its optional compression implements the adaptation mechanisms demonstrated by Clemens et al. (2018).

**Species/Behavior Controls** would provide named parameter profiles, labelled “inspired” until supported by species-specific measurements. Implemented controls include sensory sensitivity, band settings, balance, simulation speed, neural influence, and wet/dry. Independent input/output gains, user-adjustable resonance, adaptation rate, and dedicated species presets are proposed—not existing controls.

### 5.3. Implementation choices and principles

**Figure 2. Iterative build–evaluate cycle.** Biological observation → evidence/assumption ledger → DSP abstraction → prototype → functional and technical tests → musical demonstration and expert feedback **[proposed]** → requirement revision → revised prototype. Literature constrains each iteration; musical feedback may change artistic mappings without rewriting biological evidence.

The worker avoids placing recurrent simulation inside the callback. Preallocated queues and lock-free rate exchange favour uninterrupted playback, although unpaced offline rendering can overflow and is not sample-deterministic. Parameters persist, but membrane/RNG trajectories do not. Audio control values use 20 ms smoothing; filter coefficients update at buffer boundaries, requiring separate automation-artifact measurements. Cutoffs are constrained relative to sample rate; numerical caps protect simulation work and states. VST3 and Standalone are implemented; AU is a target interface only.

Four provisional principles emerge: **(P1)** maintain an evidence–abstraction–implementation ledger; **(P2)** preserve meaningful time units before introducing musical rescaling; **(P3)** separate interpretable control dynamics from carrier coloration; and **(P4)** evaluate biological correspondence, software reliability, and musical utility independently. These propositions remain provisional.

## 6. Demonstration

A completed synthetic demonstration processed paced 220 Hz sine/pulse material through the actual VST3 at clean defaults; a 6.8667 s stereo recording is retained. It establishes output generation, not musical quality or IPI recognition.

The following musical-production walkthrough is **proposed**, without listening findings. A drum loop would test whether band weighting and onset-driven control differentiate bass-heavy hits from brighter attacks; isolated hits and dense rolls would expose detector errors. Sustained guitar or synthesiser would reveal frequency-dependent carrier changes and control smoothing; stepped levels would test adaptation only after implementation.

Synthetic pulse trains would vary carrier frequency and IPI independently. Regular and jittered spacing, missing pulses, and level-matched conditions would distinguish true interval selection from pulse-count or energy effects. A full mix would reveal masking, competing onsets, and whether low-frequency content dominates the control path. Adaptation might reduce persistent dominance, but that outcome remains hypothetical.

Demonstrations would retain recordings, settings, stimuli, and control trajectories. Visual activity cannot substitute for mechanism-specific measurements.

## 7. Evaluation

### 7.1. Completed technical evidence

The accompanying local verification report documents four passing suites covering graph/data integrity, core simulation/DSP, processor integration, and actual VST3 hosting. Mono/stereo processing was exercised at 48 kHz with 64–512-sample blocks. Startup dry bypass, state recall, finite outputs, seeded sequential simulation, and downstream modulation were checked. Short lifecycle stress used sensitivity 4; actual VST3 default-processing checks used sensitivity 1. C++ allocation interposition observed **zero allocations/deallocations across 5,600 callbacks**, excluding direct C/OS allocation and separately loaded libraries.

A Windows x64/JUCE 8.0.12/MSVC 19.51 benchmark on an AMD64 Family 26 Model 68 host, using identical graph and carrier mapping, reported median clean stereo feature/DSP time for two audio seconds decreasing from **3.041 to 2.192 ms at 48 kHz** and **6.054 to 4.334 ms at 96 kHz**, across nine repetitions. Sample and trajectory hashes matched. These are **27.9% and 28.4%** local compute-time reductions, not statistical significance or whole-DAW improvements; hashing overhead was included and uncertainty intervals were not supplied. No meaningful LIF speedup is claimed.

Disabling visualisation detached OpenGL and stopped frame advancement while **57,088 finite processed samples** retained neural modification. Tests also showed that visual-event overflow did not alter the compared neural trajectory. No commercial-DAW study, musical comparison, physiological calibration, or participant evaluation has been completed. Fixed-control filter linearity does not imply that the entire input-dependent effect is linear.

### 7.2. Proposed technical and comparative protocol

The remaining protocol is **proposed, not preregistered or completed**. Before collection, archive the version, hypotheses, endpoints, analysis decisions, stimuli, and failure criteria. Test implemented features first; test proposed modules only after their implementation is frozen.

Functional trials would cover parameter limits, rapid automation, preset recall, transport changes, repeated preparation, editor reopening, and mono/stereo layouts. Test selected commercial VST3 hosts with versions reported; AU/macOS testing becomes applicable only after a build exists. Preset equality and repeated-render equality must be separate endpoints.

At 44.1, 48, and 96 kHz and 64–512-sample buffers, measure callback median, upper percentiles, and maximum duration alongside deadline overruns, worker load, queue drops, and graphical state. Report CPU model, compiler, power configuration, and background activity. Carrier-path impulse alignment and onset-to-control delay should be measured separately; repeat timing trials to characterise scheduler variability rather than reporting one nominal latency.

Use impulses, level-stepped tones/noise, sweeps, and independent carrier/IPI grids. Measure frozen-control frequency response separately from input-dependent modulation, plus onset/offset response, output peaks, RMS, crest factor, and nonfinite samples. Test silence, overload, long runs, and extreme automation. Predetermine operating limits; a single unstable condition must be disclosed rather than omitted.

Compare against a conventional filter, transient shaper, and rhythmic gate individually and in a matched chain. Equalise monitoring level, comparable control ranges, and task instructions. Comparison should distinguish additional utility from biological branding. Ablations would fix neural rates, remove onset drive, or bypass one implemented stage at a time; later, bypass IPI/adaptation separately. Match output level and log control differences so loudness does not explain preference. No improvement is assumed.

### 7.3. Proposed expert usability study

Recruit a **target of eight** experienced audio engineers or musicians for an exploratory within-participant study; this is a planned sample, with no participants yet recruited. Obtain consent and applicable ethics approval, document production experience and hearing-related accommodations, and report actual enrolment, exclusions, and missing data.

After standardised training, participants would complete counterbalanced tasks: emphasise drum attacks, reshape sustained timbre, select a pulse-spacing profile where implemented, and produce a controlled full-mix transformation. Include neutral-label comparator listening to reduce branding effects, followed by disclosed-interface use. Record task completion, time, errors, settings, and interviews.

Use the System Usability Scale as a general usability measure (Brooke, 2013), plus explicitly exploratory seven-point items for novelty, controllability, usefulness, and correspondence between expected and heard behaviour. The custom items are not validated scales. Prediction-before-listening tasks would test whether controls communicate intended effects.

Report individual observations, paired contrasts, and uncertainty intervals with assumptions. Avoid population-level efficacy claims from eight experts or post hoc significance claims. Preserve negative cases, including subtle effects, confusing terminology, detector failures, and preference for conventional tools. Qualitative coding procedures and discrepant accounts should accompany summaries.

## 8. Discussion

The contribution beyond combining familiar effects is an auditable translation process and an architectural distinction between feature interpretation and sonic transformation. Biological constraints determine which histories and relationships are considered; DSP determines how those relationships become controllable musical behaviour. Whether this organisation supplies added musical utility remains an empirical question.

Construction generated concrete design knowledge. Retaining confirmed cells without detected input revealed the difference between identity evidence and functional completeness. Replacing overly subtle carrier mapping showed that valid neural activity need not produce a perceptible musical transformation. Preserving filter state during optimisation demonstrated that reduced computation must be checked against automated transitions, not just steady-state output. These lessons support P1–P4 but do not establish their generality.

Biological fidelity and productive inspiration serve different purposes. A direct interval selector may improve explanation and usability while departing from inhibitory circuit mechanisms. Conversely, realistic connectivity can complicate control without guaranteeing faithful physiology. Scientific and creative modes should disclose this tradeoff rather than hide it behind species names.

The artifact contributes no new validated fact about Drosophila auditory senses. It identifies testable computational questions—such as whether interval selectivity survives alternative input and transmitter assumptions—that would require neural data and biological validation. The design principles may transfer to other sensory-inspired audio systems where physical units, temporal scales, and perceptual goals differ, but such transfer remains prospective.

## 9. Limitations

The prototype simplifies a single female connectome with shared LIF parameters, uncertain fast-synaptic signs, missing connections, and absent receptor-specific adaptation. Predominantly graded biological responses are not faithfully represented by universal spiking dynamics. Selected studies sample different cells, sexes, and assays; their findings cannot be merged into one universal circuit claim.

Anthropomorphic descriptions such as “hearing like a fly” are inappropriate. Music, headphones, and DAW amplitudes differ from ecological near-field courtship stimuli. Musical-quality judgements are task- and listener-dependent. Proposed musical cases have not been assessed, and expert recruitment is pending.

Technical evidence is local and developer-generated. Timing averages do not establish worst-case safety; coefficient automation, offline determinism, commercial hosts, AU support, and cross-platform operation remain incomplete. Missing uncertainty estimates constrain interpretation of optimisation measurements.

## 10. Conclusion

Selected Drosophila frequency and temporal characteristics can be translated into audio software through explicit feature extraction, constrained neural/control dynamics, and separable carrier processing. The implemented precursor demonstrates technical feasibility on tested Windows configurations; proposed IPI and adaptation modules define a further development agenda.

The resulting design knowledge concerns traceability, preservation of time units, interpretable control/carrier separation, and independent evaluation of reliability, musical utility, and biological correspondence. It does not establish complete fly-hearing reproduction or biological discovery. The next empirical step is a frozen-version, mechanism-specific pulse/level-response comparison, followed by the proposed expert study to test whether these mappings are understandable and musically useful.

## References

Baker, C. A., McKellar, C., Pang, R., Nern, A., Dorkenwald, S., Pacheco, D. A., Eckstein, N., Funke, J., Dickson, B. J., & Murthy, M. (2022). Neural network organization for courtship-song feature detection in Drosophila. *Current Biology, 32*(15), 3317–3333.e7. https://doi.org/10.1016/j.cub.2022.06.019

Brooke, J. (2013). SUS: A retrospective. *Journal of Usability Studies, 8*(2), 29–40. https://dl.acm.org/citation.cfm?id=2817913

Clemens, J., Girardin, C. C., Coen, P., Guan, X.-J., Dickson, B. J., & Murthy, M. (2015). Connecting neural codes with behavior in the auditory system of Drosophila. *Neuron, 87*(6), 1332–1343. https://doi.org/10.1016/j.neuron.2015.08.014

Clemens, J., Ozeri-Engelhard, N., & Murthy, M. (2018). Fast intensity adaptation enhances the encoding of sound in Drosophila. *Nature Communications, 9*, Article 134. https://doi.org/10.1038/s41467-017-02453-9

FlyWire Consortium. (2024). *FlyWire whole-brain connectome connectivity data* (Version 783.0) [Data set]. Zenodo. https://doi.org/10.5281/zenodo.10676866

Göpfert, M. C., & Robert, D. (2002). The mechanical basis of Drosophila audition. *Journal of Experimental Biology, 205*(9), 1199–1208. https://doi.org/10.1242/jeb.205.9.1199

Hevner, A. R. (2007). A three cycle view of design science research. *Scandinavian Journal of Information Systems, 19*(2), 87–92. https://aisel.aisnet.org/sjis/vol19/iss2/4/

Hevner, A. R., March, S. T., Park, J., & Ram, S. (2004). Design science in information systems research. *MIS Quarterly, 28*(1), 75–105. https://doi.org/10.2307/25148625

Lyon, R. F. (2017). *Human and machine hearing: Extracting meaning from sound*. Cambridge University Press. https://doi.org/10.1017/9781139051699

Peffers, K., Tuunanen, T., Rothenberger, M. A., & Chatterjee, S. (2007). A design science research methodology for information systems research. *Journal of Management Information Systems, 24*(3), 45–77. https://doi.org/10.2753/MIS0742-1222240302

Shiu, P. K., Sterne, G. R., Spiller, N., Franconville, R., Sandoval, A., Zhou, J., Simha, N., Kang, C. H., Yu, S., Kim, J. S., Dorkenwald, S., Matsliah, A., Schlegel, P., Yu, S. C., McKellar, C. E., Sterling, A., Costa, M., Eichler, K., Bates, A. S., . . . Scott, K. (2024). A Drosophila computational brain model reveals sensorimotor processing. *Nature, 634*, 210–219. https://doi.org/10.1038/s41586-024-07763-9

Verfaille, V., Zölzer, U., & Arfib, D. (2006). Adaptive digital audio effects (A-DAFx): A new class of sound transformations. *IEEE Transactions on Audio, Speech, and Language Processing, 14*(5), 1817–1831. https://doi.org/10.1109/TSA.2005.858531

Yamada, D., Ishimoto, H., Li, X., Kohashi, T., Ishikawa, Y., & Kamikouchi, A. (2018). GABAergic local interneurons shape female fruit fly response to mating songs. *The Journal of Neuroscience, 38*(18), 4329–4347. https://doi.org/10.1523/JNEUROSCI.3644-17.2018

Zölzer, U. (Ed.). (2011). *DAFX: Digital audio effects* (2nd ed.). Wiley. https://doi.org/10.1002/9781119991298

## Artifact Availability

**Repository:** [public repository URL pending; no publication performed]. **Source-code licence:** [not yet specified]; JUCE licensing is separate. Embedded FlyWire-derived data are **CC BY 4.0**, with attribution and adaptations documented. **Formats:** implemented VST3 and Standalone; AU proposed, not built. **Operating systems:** Windows x64 build/host-test evidence; macOS/Linux targets unverified. **Source status:** local C++17/JUCE source is available in the project workspace, not an asserted public release. **Reproducibility:** [README](../../README.md), [verification report](../../VERIFICATION.md), [coverage audit](../../Data/COVERAGE.md), [data provenance](../../Data/provenance.json), extraction scripts, test harnesses, [optimisation measurements](../../Reports/optimisation.md), and [recorded VST3 output](../../Reports/host-output.wav) accompany the local artifact. [Archived release DOI/version identifier pending].
