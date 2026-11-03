# VHIKK-X behavior reference for an FM1 implementation

Researched 2026-11-03 from the manufacturer's documentation. This specifies observable behavior to approximate with original DSP; it is not source code or a claim of sonic equivalence. No VHIKK-X firmware binary was downloaded or examined.

## Current scope and interface

Firmware 0006 implements **18 algorithms across two banks**, explicitly enumerated in the [current algorithm reference](https://forge-tme.com/vhikk-x/algos_06.html). The overview's four-bank/36-algorithm language describes the larger architecture: its October 2026 news says further algorithms, including effects-only modes, remain in development. Do not advertise 36 implemented algorithms. [Manufacturer overview](https://forge-tme.com/vhikk-x/)

Two three-position switches choose one of nine algorithms. Two encoders alternate between BASIS/TIME and SEED/SCAN; volume mode controls input/output gains. A button combination randomizes SEED/SCAN. Newer firmware offers bank-selection behavior options. Encoder parameters persist: SEED/SCAN per algorithm, volumes globally, BASIS/TIME either globally or per algorithm. Saving follows roughly 30 seconds without changes. The hardware provides stereo 24-bit audio, a VCA CV input, and parameter CV connections including five attenuverters. Its USB connection is for updates rather than USB audio/MIDI performance. These physical facilities cannot be recreated solely through firmware on hardware lacking the corresponding connections. [Manufacturer operating guide](https://forge-tme.com/vhikk-x/)

## Core synthesis and macro controls

The launch architecture runs oscillators through tone shaping and a VCA before a delay processor. Stereo external audio joins before the filter. The processor can move from audible echoes toward dense, modulated reverberation. Feedback incorporates frequency shifting; mixing shifted and unshifted feedback produces moving notches. Cluster scanning crossfades through oscillator-specific pitch/waveform pairs. [Launch reference](https://forge-tme.com/vhikk-x/algos_launch.html)

| Macro | Behavioral target |
| --- | --- |
| BASIS | Common oscillator root pitch |
| SPAN | Relative tuning or pitch spread; sometimes stereo spread |
| WARP | Bipolar choice/depth of cross-modulation |
| MORPH | Waveform or pitch/wave cluster interpolation |
| SEED | Continuous variation of internal pitch, waveform, tap, or notch offsets |
| SCAN | Additional variation of waveform/timbre choices |
| FIELD | Filter frequency |
| FUSE | Mode-dependent coloration: chorus, resonance, saturation, folding, or modulation |
| TIME | Overall delay-length scale |
| FORM | Processor structure/diffusion |
| FEED | Feedback amount and routing/mix character |
| CELL | Frequency shift |
| MIX | Blend of generator and processed signal |

These labels summarize the [launch reference](https://forge-tme.com/vhikk-x/algos_launch.html); parameter meanings vary by algorithm. The original nine combine analog-style oscillator pairs, dynamic waveshaping, sine clusters, wavetable banks, and quantized/unquantized clusters with metallic or diffuse delay structures. Several use four oscillators. Modulation includes phase modulation, exponential FM, ring modulation, and bit-depth reduction. [Launch algorithms](https://forge-tme.com/vhikk-x/algos_launch.html)

## Newer algorithm families

The additional bank comprises filterbank/comb feedback, granular clusters with grain-delay pitch shifting, a zero-frequency wavetable carrier driven by modulators, phase-coupled sines with a dispersor, a driven notch stack, harmonic/inharmonic partials, quantized chord scanning, a sine swarm, and a revised cluster/diffuse processor. CELL becomes algorithm-dependent: frequency shift, pitch shift, comb filtering, or dispersor frequency. FEED can select whether transformation occurs inside the feedback loop. [Firmware 0006 algorithm reference](https://forge-tme.com/vhikk-x/algos_06.html)

The same reference retains the generator/filter/VCA/processor ordering and defines BASIS CV tracking at one volt per octave with the attenuverter fully clockwise. Its exact electrical and performance behavior should not be assumed for another synth. [Current reference](https://forge-tme.com/vhikk-x/algos_06.html)

The [release notes](https://forge-tme.com/vhikk-x/firmware.html) confirm the new bank in October 2026, longer original delay times, low-frequency noise suppression, quantization changes, and optional BASIS/TIME randomization. The VHIKK-X uses its own mass-storage update bootloader; those update files/procedures are not applicable to an FM1.

## Implementation judgment, pending FM1 hardware findings

These are engineering proposals, not statements about the manufacturer's implementation:

1. Prioritize continuous drone generation, oscillator cross-modulation, cluster scanning, and a resonant delay/diffusion loop. These interact to create the instrument's behavior; relabeling ordinary FM parameters would cover only part of it.
2. Start with one original four-oscillator voice and one bounded stereo feedback processor. Share a stable macro interface across later modes.
3. If memory is tight, reduce delay length, tap count, or sample rate before promising large algorithm coverage. If CPU is tight, measure simpler sine/lookup oscillators and allpass diffusion first.
4. Give modulation and delay-length changes smooth transitions. Keep feedback finite under extreme settings and verify silence/DC, saturation, clipping, and mode changes with rendered audio and on-device timing measurements.
5. Map unavailable CV facilities to MIDI/controllers only when the FM1 transport and controls are known; document the resulting differences.

The public references do not specify exact DSP equations, wavetable content, tap coefficients, randomization maps, interpolation curves, or processor resource requirements. Those must be independently designed and tuned by ear. Exact sound matching cannot be established from parameter descriptions alone, and FM1 performance remains unproven until its audio path, memory, processing budget, and boot/recovery mechanism are verified.
