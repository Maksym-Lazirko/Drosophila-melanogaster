# Historical optimisation measurements — October 4, 2026

These measurements predate Delta monitoring and the later waveshaping/AMMC colour changes. Exact-hash agreement applies to the archived comparison, not to current source. No current-version or total-DAW speedup is claimed.

Same 731-neuron / 7,056-edge graph, final 1200–150 Hz carrier mapping, inputs, compiler and Release settings; median of nine runs. Hashes cover intermediate/final neural trajectories and every DSP sample (including automated mode switches).

| Scenario | Before (ms) | After (ms) | Time reduction | Exact hashes match |
|---|---:|---:|---:|---|
| lif0 | 22.326 | 22.165 | 0.7% | yes |
| lif1 | 116.742 | 116.621 | 0.1% | yes |
| lif2 | 54.405 | 54.322 | 0.2% | yes |
| audio48000mode0 | 3.041 | 2.192 | 27.9% | yes |
| audio48000mode1 | 3.076 | 2.238 | 27.2% | yes |
| audio48000mode2 | 3.103 | 2.425 | 21.8% | yes |
| audio48000mode3 | 3.154 | 2.731 | 13.4% | yes |
| audio48000mode4 | 3.119 | 2.393 | 23.3% | yes |
| audio96000mode0 | 6.054 | 4.334 | 28.4% | yes |
| audio96000mode1 | 6.041 | 4.439 | 26.5% | yes |
| audio96000mode2 | 6.130 | 4.821 | 21.4% | yes |
| audio96000mode3 | 6.270 | 5.437 | 13.3% | yes |
| audio96000mode4 | 6.115 | 4.757 | 22.2% | yes |

Each scenario processes two audio seconds. lif0: default sustained frame; lif1: max gain/4× speed with visual overflow; lif2: gain/balance/speed automation. Audio rows: stereo feature extraction + DSP at 48/96 kHz; mode4 switches through all modes every block. Timing includes hashing overhead, excludes host/OS scheduling and GL. LIF changes are small/noisy; no LIF speedup is claimed. Baseline is local archived pre-optimisation code, with only the final carrier cutoff applied for a fair comparison.

Raw logs are local ignored artifacts: `build/final-before-benchmark.log` and `build/final-after-benchmark.log`; they are not part of the GitHub-facing evidence set. Harness: [Benchmark.cpp](../Tests/Benchmark.cpp).
