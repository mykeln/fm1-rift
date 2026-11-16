# VHIKK-X public-source investigation

Researched 2026-11-03 for RIFT. This expands [the initial reference](vhikk-x-reference.md) with a designer interview, demonstration captions, hands-on reviews and owner reports. This is a bounded survey of accessible public evidence, not an exhaustive archive or reverse engineering of firmware. No VHIKK code, binaries or wavetable assets were used.

## Findings beyond the short manual

### Designer interview: architecture and parameter mapping

[Signal Flow: Fletcher from Forge-TME, June 10 2026](https://www.buzzsprout.com/2600777/episodes/19325690-talking-vhikk-x-with-fletcher-from-forge-tme) provides a full public transcript:

- **4:39:** describes a synth voice with effects, without a general modulation section; users supply external control signals. Effect modulation is still part of some algorithms.
- **6:04:** many knobs have a neutral noon position and perform different functions on either side.
- **25:57:** 96 kHz permits simpler oscillator/modulation implementations before aliasing becomes objectionable; 44.1/48 kHz reaches that limit sooner.
- **26:40:** an output mastering strip includes a mid scoop and dynamic compression.
- **45:29–46:37:** SEED/SCAN explore a reversible procedural parameter landscape across many internal settings.
- **57:52–59:30:** mapping curves are individually chosen and repeatedly tuned by listening; deep DAW macro systems informed the design.
- **1:00:14–1:00:30:** discussion estimates around 128 internal destinations for SEED/SCAN, including oscillator tuning, amplitudes, waveforms and delay settings. “Maybe 32” delay lines is an approximate spoken description, not a guaranteed count for every algorithm. Automatic speaker labels are imperfect here.

**Consequence:** mapping direction, coupled destinations and useful intermediate positions matter as much as oscillator type. These statements do not reveal exact curves or coefficients.

### Independent instructional demonstration

[Metamyther: Tame the Chaos: Vhikk X Explained](https://www.youtube.com/watch?v=Mx8tViTAAjw) was inspected through its public English auto-captions; timestamps are approximate:

- **3:31–4:24:** noon settings, dry MIX and UP/UP produce a relatively simple static starting waveform.
- **6:17–6:38:** attributes this explanation to Fletcher: cluster positions carry waveform **and** pitch offsets, determined by SPAN/SEED; crossfading requires eight simultaneous wavetable oscillators for four voices.
- **7:56–8:13:** FIELD is open at noon, low-pass CCW and high-pass CW; full CCW can silence a drone.
- **9:11–10:18:** effects have distinct metallic/diffuse families; TIME covers conventional delays through very short resonant behavior.
- **12:41:** CELL demonstrates frequency shifting.
- **15:21–15:53:** first-bank groups are classic oscillator, wavetable and cluster families. Wavetable/cluster SEED affects pitch/wave offsets and delay spacing; SCAN selects the waveform set.

**Interpretation:** cluster scanning should crossfade independently pitched signals, rather than glide one oscillator's frequency while morphing its waveform. The exact fade law remains unknown. Captions are sufficient for these structural observations, not sonic measurement.

### Hands-on reviews and owners

[AMAZONA hands-on test, May 20 2026](https://www.amazona.de/test-forge-tme-vhikk-x-eurorack/) independently identifies FIELD as LP-left/HP-right, places it before the effects, and demonstrates the effect tail surviving filter closure. The internal VCA follows the filter and precedes the processor. Its audio examples cover algorithms with/without effects, FIELD and high-feedback FORM/CELL changes. These are useful listening references; their author does not supply DSP equations.

[Robin Vincent's Perfect Circuit review, August 31 2026](https://www.perfectcircuit.com/signal/forge-tme-vhikk-x-review) reports direct use of a borrowed unit. It corroborates the DJ filter, algorithm-dependent FUSE (chorus versus resonance), and FORM's delay/reverb transition. It describes strong interactions between CELL, WARP and feedback rather than independent static effects. The complete article was available in search-index text; direct page fetch returned HTTP 403.

[Smapti's first-person owner report, VCV Community, May 27 2026](https://community.vcvrack.com/t/anyone-have-a-module-like-vhikk-x-drone/24131/8) stresses the wide waveform/pitch combinations exposed by cluster macros. This supports testing the breadth of the parameter landscape, but the owner's claim that replication cannot be done is opinion, not a technical limit.

[Elektronauts discussion, April 4 2026](https://www.elektronauts.com/t/elektronauts-do-dr-o-ne-day-2026/248707?page=2) distinguishes V1 rerandomization on mode selection from X's reversible SEED/SCAN and randomize gesture. Its proposed built-in random tables are explicitly the poster's inference; the designer interview supports reversibility without establishing that storage method.

## First-bank control distinctions

From the [launch reference](https://forge-tme.com/vhikk-x/algos_launch.html). Left/right switches; PM = phase modulation; expFM = exponential FM; D = Dungeon; R = Reflekta. Reference families differ from RIFT's names.

| Switches | Generator / processor | WARP CCW → CW | Distinguishing FUSE |
| --- | --- | --- | --- |
| U/U | Analog pair / D | expFM → ring | chorus → resonance/shape |
| U/M | Dynamic shaper / R | modulator-feedback → carrier-feedback | audio-rate delay modulation → saturation |
| U/D | Sine cluster / D | phase-feedback → expFM | chorus → noise modulation |
| M/U | Wavetable quartet / D | bit-depth reduction → expFM | bandpass/parallel sine → chorus/resonance |
| M/M | Wavetable quartet / D | PM → expFM | chorus → chorus/resonance |
| M/D | Wavetable quartet / R | PM → expFM | audio-rate allpass/delay modulation → folding |
| D/U | Quantized cluster / R | PM → expFM | chorus → saturation |
| D/M | Cluster / D | PM → expFM | chorus → saturation |
| D/D | Cluster / R | PM → expFM | chorus → saturation |

FEED selects shifted feedback CCW and shifted-plus-unshifted feedback CW, producing pronounced moving notches. Noon is neutral. SPAN is bipolar, with extra stereo spread CW in many modes; U/U tunes the modulator. MORPH traverses waveforms or pitched cluster positions. SEED also moves processor tap spacing.

## New-bank scope and extra public detail

The [firmware 0006 reference](https://forge-tme.com/vhikk-x/algos_06.html) enumerates a second bank. Its notable control differences are:

| Switches | Family | Distinct controls |
| --- | --- | --- |
| U/U | Filterbank/comb swarm | MORPH filter clusters; FORM comb spread/diffusion; FEED signed comb feedback; CELL filter cutoff |
| U/M | Granular cluster | WARP skew/crossmod; FORM grain period/diffusion; CELL pitch shift; FEED chooses shifter outside/inside loop |
| U/D | Zero-Hz wavetable carrier | MORPH wave shape; SCAN phase lookup; SPAN secondary frequency |
| M/U | Phase-coupled sines/dispersor | SCAN allpass randomization; FORM spread/diffusion; CELL allpass frequency |
| M/M | Driven sine/notches | MORPH notch scan; SCAN rectifier/waveshaper |
| M/D | Harmonic/inharmonic partials | SPAN inharmonic versus harmonic spacing; SCAN partial amplitudes; FEED shifter outside/inside loop |
| D/U | Quantized chord scanner | WARP waveform; SCAN phaser spacing |
| D/M | Sine swarm | SCAN oscillator amplitudes |
| D/D | Revised cluster/diffuse tank | Wave/pitch clusters and revised processing |

CELL is therefore not universally a frequency shifter, and SCAN is not universally waveform scanning.

The designer's [new-algorithm overview video](https://www.youtube.com/watch?v=aOJZM91yHOs), inspected through public auto-captions, adds details:

- **12:33–15:36:** eight parallel combs; FORM first spreads them, then adds diffusion above halfway; CELL controls a DJ filter in each comb's feedback; SCAN randomizes spread factors.
- **26:02–26:46:** granular oscillator waveform playback rate is decoupled from fundamental, producing pulsar-like constant-formant behavior; MORPH also affects grain skew.
- **29:56–32:10:** FORM controls grain period then diffusion; CELL changes pitch; FEED chooses a cascading in-loop or noncascading post-loop pitch shifter.
- **44:08–44:59:** zero-Hz carrier acts as a wavetable waveshaper driven by two sine oscillators.
- **69:27–72:10:** dispersor is a series allpass chain; mixing its output with dry signal creates phaser notches. The spoken estimate of 64 stages is uncertain.
- **101:38:** partial generator uses eight oscillators.
- **124:39:** new chord mode shares one waveform across oscillators, unlike first-bank wavetable clusters.

These are implementation targets for future new-bank work; RIFT's original nine should not be advertised as these eighteen.

## Public material assessed but not treated as specifications

- The [ModularGrid thread's user cheat sheet](https://modulargrid.net/c/forum/posts/index/13564/page%3A1) labels several first-bank modes as four-operator FM, eight-part additive, noise/glitch and external-input/subosc modes. Those assignments conflict with the actual reference and hands-on test. Do not implement that table as fact.
- [Siren's author describes an independent inspired implementation](https://mohoyt.com/blog/2026/siren-a-workshop-system-drone-oscillator/), with [public source](https://github.com/mohoyt/siren). It is evidence of that author's design, not a disclosure of VHIKK internals. Likewise [Downspout Fathom](https://danja.github.io/downspout/plugins/fathom/) calls itself modeled on the algorithm reference, not original firmware.
- [Richard Brewster's new-bank patch report](https://pugix.com/vhikk-x-new-algo-animated/) identifies external modulation of WARP, SPAN, CELL, TIME, MORPH and VCA. Motion in such a performance cannot be assumed to come from internal modulation.
- [State Azure's long performance](https://www.youtube.com/watch?v=bLg2wdW-G7Q) is a listening reference linked by the VCV discussion; it was discovered, not audited for individual knob positions or measured here.
- Searches included Reddit, ModularGrid, ModWiggler, Elektronauts, VCV Community, reviews, interviews and videos. No verified maker-authored forum disclosure of exact coefficients or wavetable data was found. The ModWiggler discussion was not retrievable through indexed results; that gap should remain explicit.

## Implementation proposals for RIFT

These are independent engineering choices, not claims about the original firmware:

1. Implement FIELD with a true open center and useful filtering on both sides. Verify bass survives at center, LP reduces upper harmonics, HP removes the fundamental, and released processor tails bypass subsequent FIELD changes.
2. Split FEED into direction-dependent routing and distance-from-center decay gain. At equal depth, compare shifted-only against normalized shifted-plus-reference feedback. Align the reference and shifted paths before mixing; arbitrary phase mismatch would create unrelated coloration.
3. Make SEED/SCAN deterministic coordinates. Keep slow tank modulation separate from SCAN's meaning. Verify moving a knob away and back restores its target coefficients; test audible output with allowance for persistent phase/tank state.
4. Apply mode-specific WARP functions rather than a universal ring/phase pair. Quantization distortion and exponential FM need distinct implementations. Exponential FM should modulate phase increment multiplicatively; PM should change phase lookup without being mislabeled FM.
5. Expand clusters by holding adjacent waveform/pitch members concurrently and crossfading their outputs. If resource constraints require pitch glides instead, document that approximation. Maintain unique offsets per voice.
6. Give FUSE meaningful bipolar destinations by family. Delay audio-rate modulation, chorus, oscillator noise modulation and filter resonance should not all collapse into generic gain.
7. Tune nonlinear curves by listening across the full dial, using fixed source tones and wet/dry renders to expose behavior. A test that merely detects changed samples does not establish the intended control direction.
8. Evaluate a gentle output mid scoop and dynamic compression after the structural fixes, with matched-level comparisons. Exact frequency/Q/threshold/attack/release are unknown; avoid pretending that a chosen mastering curve is recovered behavior.

Unknowns remain: wavetable content, random landscape distribution/resolution, pitch offset laws, scan fades, chorus topology, filter order/Q, output EQ/dynamics settings, exact delay structures and coefficients, and CPU cost. Public evidence supports closer behavioral replication; sonic equivalence still needs controlled reference-unit recordings and listening comparisons.


## RIFT 0.7 implementation and verification

Implemented in `firmware/firmware/src/rift_dsp.c`, with the exact RIFT mode mappings in [the instrument guide](../firmware/docs/RIFT.md):

- Paired waveform/pitch anchor oscillators in SINES, CHORD, CLUSTER and CAVERN; the four logical voices run eight phase accumulators and crossfade their outputs. A shared anchor keeps its phase when crossing a scan boundary.
- Bipolar FIELD with an open center, LP/HP regions and source closure at the CCW endpoint. SPAN is bipolar, including modulator tuning in PAIR and diatonic cluster spread in CHORD.
- Bipolar FEED depth, translated-only versus translated-plus-phase-aligned-reference routing, and a bounded cubic decay map. Small nonzero CELL can shorten the CW tail through interference; long-space factory sounds use neutral translation.
- Separate mode-dependent WARP/FUSE destinations, including multiplicative exponential FM, phase/ring modulation, continuous quantizer blending, shaper feedback, chorus, noise-driven phase modulation, audio-rate delay modulation, resonance, folding and saturation.
- Static reversible SEED/SCAN landscapes, with independent chorus/tank modulation. PAIR's modulator is not independently audible. Its tuning/waveform response requires active cross-modulation.

`sh rift.sh test` passes with ASan/UBSan: 27 sustained factory renders, all 117 mode/dial combinations, 291 palette checks, ten controlled spectral behavior checks, source closure through all nine complete dry paths, long-tail and idle/persistence checks. Cluster midpoint spectra retain four endpoint pitches from each adjacent anchor; both FEED extremes sustain longer tails than center. The device build passes with an image of 537,876 bytes and 8,612 bytes of pool headroom, above the repository's 8,192-byte minimum. These are software/build observations, not measurements of a VHIKK-X.

RIFT retains nine original modes and its eight-line processor; it does not implement all eighteen reference algorithms, the newer granular/comb/dispersor bank, the exact Dungeon/Reflekta topology, manufacturer wavetables or the output mastering strip. The new factory sounds and existing saved patches intentionally differ from 0.6. Real-device timing and reference-unit listening comparisons remain pending.
