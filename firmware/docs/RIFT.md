# RIFT 0.8 — experimental FM1 drone firmware

RIFT is an original implementation informed by public VHIKK-X behavior. The [public-source investigation](../../docs/vhikk-x-public-research.md) combines the designer's interview, instructional demos, hands-on reviews and owner reports with the manufacturer's references. It distinguishes observed behavior from independent DSP choices. It is built on Felucca commit `743551712c36b3984392eea1dbe13a309907e17a`, with its pinned engines and board-support submodules. The supplied FM1-to-SMK37 repository helped identify this firmware family; no SMK37 board mappings or flash map were applied to the FM1.

## What is implemented

Nine original modes combine oscillator cross-modulation, waveform/pitch cluster crossfades, bipolar tone filtering, a generator gate, stereo delay feedback and diffusion, signed frequency shifting, and dry/wet mixing. SINES, CHORD, CLUSTER and CAVERN run two independently pitched oscillators for each of four logical voices; MORPH crossfades adjacent outputs rather than gliding one oscillator. The oscillators run continuously in drone mode. Keys or MIDI select a new root pitch. Keyed mode closes the generator when the last key is released, leaving the feedback tail.

RIFT occupies part 1 and exclusively owns the stereo output while that part requests its engine. Other Felucca parts are not mixed during RIFT operation. The normal Felucca engines remain available by changing part 1's engine in the standard editor. RIFT's engine number is 15; existing engine numbers were preserved, including the disabled ADPCM slot at 14.

| Mode | Implementation |
| --- | --- |
| PAIR | Audible carrier plus modulator; WARP selects exponential cross-FM CCW or ring modulation CW; MORPH scans carrier wave, SCAN scans modulator wave |
| FOLD | Dynamic shaping pair; WARP selects modulator/carrier feedback; FUSE selects audio-rate delay modulation CCW or stronger shaping CW |
| SINES | Four sine clusters, eight running oscillators; phase feedback CCW, exponential cross-FM CW; SCAN controls inter-oscillator ring mix |
| WAVES | Four original sine/triangle/saw/pulse scans; continuous bit-depth reduction CCW, exponential cross-FM CW; negative FUSE blends bandpass and a parallel sine |
| CHORD | Four diatonic cluster scans with eight running oscillators; bipolar pitch spread, PM CCW and exponential cross-FM CW |
| CLUSTER | Four waveform/pitch cluster scans with soft additive harmonics; FUSE selects chorus CCW or individual oscillator folding CW |
| PARTIAL | A 1:2:3:5 sine bank; MORPH balances upper partials, SPAN adds inharmonicity, SCAN scans partial amplitudes |
| CROSS | A 1:2:3:7 bank with baseline phase coupling, additional PM CCW and exponential cross-FM CW |
| CAVERN | Root/fifth/octave scaffold plus four waveform/pitch cluster scans; FUSE selects audio-rate delay modulation CCW or saturation CW; a modulated diffuse tank provides motion |

These names identify the new implementation. They do not claim exact equivalence to the reference algorithms. The modes differ in pitch/wave rules and modulation and share a processor foundation rather than the reference instrument's separate processor families.

Version 0.8 addresses a real-device audio overload in 0.7. With the controls untouched, the three CHORD sounds reported 96–99% audio CPU and missed 155–158 buffer deadlines in roughly 0.9 seconds each. Preset values were reaching the device correctly while audio rendering exceeded its time budget. This failure was not detected by host sound renders.

The sample loop now uses explicit signed power-of-two scaling, fixed-size oscillator-state loads, masked voice indexing, and block-rate chorus/filter coefficient calculations. Neutral WARP avoids a frequency-modulation call. All 27 factory WAVs are sample-identical to the 0.7 host reference; oscillator and processor sample rates remain 44.1/22.05 kHz. Target assembly confirms the per-sample memory-copy call has been removed. Hardware deadline verification on 0.8 is pending installation, so this build is not yet a confirmed device fix.

`tests/run_rift_live.sh` exercises CHORD recalls through the UI, mixer, audio ISR and simulated DAC. It checks that each sound has multiple tones, that preset recalls change the spectrum with old tails retained, and that the physical master reaches silence with USB fixed-level mode both off and on. `scripts/check_rift_hardware.py` is the device acceptance check: it briefly recalls the three CHORD sounds (or all 27 with `--all-presets`), requires zero missed deadlines and at most 80% audio CPU, then restores and verifies the original patch. Leave the physical controls untouched while it runs. It snapshots the original parameters to a local `.before.json` file before making changes and sends no flash/save commands.

Version 0.7 changes the DSP mappings using the broader public-source investigation. FIELD now has LP/open/HP regions. FEED is neutral at 64, with feedback depth increasing toward either endpoint; CCW uses the shifted signal, CW progressively mixes it with a phase-aligned unshifted reference. This produces a different moving-notch behavior from simply increasing decay. Small nonzero CELL values on the CW side can shorten a tail substantially through interference, so long-space presets use neutral CELL.

SEED and SCAN are reversible deterministic coordinates, not motion-speed controls. They map oscillator pitch, waveform or amplitude choices and processor tap spacing according to the mode. Slow chorus/tank modulation remains separate. WARP now uses exponential FM, ring modulation, phase feedback, PM, continuous bit-depth reduction or shaper feedback according to the mode. The eight physical oscillators used by a four-voice cluster preserve the shared anchor's phase when crossing a scan boundary. SPAN is bipolar, with unison at 64; positive values also increase stereo spread except in PAIR, where SPAN tunes the modulator.

FUSE CCW adds chorus in PAIR, SINES, CHORD, CLUSTER, PARTIAL and CROSS; WAVES uses bandpass/parallel-sine blending; FOLD/CAVERN couple the oscillator audio into the delay read positions. FUSE CW adds filter resonance in PAIR/WAVES/CHORD, noise-driven phase modulation in SINES, folding in FOLD/CLUSTER, saturation in PARTIAL/CROSS/CAVERN, and chorus alongside resonance in WAVES/CHORD. These are RIFT's explicit mappings, not a claim that each named mode corresponds to a manufacturer's switch position. Some reference destinations, including modulated allpass coefficients and exact filter-shape mappings, remain approximations.

Factory sounds have been moved into the new control ranges and rebalanced. Existing project/preset storage and sound indices are retained, but saved patches intentionally sound different; there is no automatic parameter migration. The old 0.5/0.6 sample-preservation hashes no longer apply. Version 0.7 adds controlled spectral tests for LP/HP direction, simultaneous cluster pitches, reversible SEED/SCAN targets, both feedback directions and matched phase at neutral CELL. Full device timing and listening comparisons against reference hardware remain pending.

### Earlier versions

Version 0.6 revoices the weaker factory sounds toward warm, pitched, spacious drones. Large spaces now keep a tonal anchor instead of combining extreme WARP, broad SPAN, large CELL shifts and heavy feedback. The second bank emphasizes halos/veils; the third uses richer ember/prism/opal colors in a long space instead of the old IRON bank. CROSS 2, which had no numeric rating, also receives this gentler treatment. FOLD's source level is balanced against the other generators. CLUSTER becomes a close unison ensemble with additive shimmer, distinct from WAVES; CAVERN keeps a stable root/fifth/octave scaffold with breathing harmonics. Their SPAN tuning uses fine frequency increments rather than coarse pitch steps.

PAIR 1, SINES 1, WAVES 1–2, CHORD 1, PARTIAL 1 and CROSS 1 are sample-identical to 0.5 at the factory root/SEED/SCAN used by the host probe. CLUSTER 1 changes despite its 7/10 rating to address its similarity to WAVES. All original preset indices, macro ranges and saved-project formats remain valid. Existing saved patches keep their parameter values; the CLUSTER/CAVERN voices and the upper FUSE/SCAN/FEED ranges intentionally sound different.

FEED above 96 reduces new excitation as the tank decay lengthens, giving sustained drones more headroom. FUSE above its midpoint adds soft harmonic drive so it remains audible on low fundamentals; its lower range retains the existing resonance behavior. SCAN's upper range reaches faster timbral travel and gentle breathing swells. The seven preserved factory sounds stay below these revised ranges. These controls still permit abrasive extremes when pushed together; the factory sounds no longer start there.

The new palette probe renders every sound for sixteen seconds plus four seconds of release at actual output gain. It checks tonal peak concentration, subsonic energy, treble, sustained level, buildup and bank balance, plus all thirteen dials in every mode and matched-macro CLUSTER/WAVES separation. These are regression proxies for the reported rumble and control problems, not a listening verdict. The 0.6 on-device timing and subjective listening checks remain pending.

Version 0.5 fixes automatic flash saves during the independent drone or a long tail. RIFT now reports output activity to the idle checks; autosave and queued settings writes wait for at least one second below an eight-count output threshold. Explicit project saves retain their existing behavior. This means that an always-running drone deliberately defers autosave: mute it and let the tail finish, or use an explicit project save before switching off.

A real FM1 capture of 0.4 measured 73% CPU with USB audio active and zero shed voices, but accumulated nine late blocks and fourteen USB underruns over the long take. Host reproduction then confirmed that the generic autosave idle test incorrectly permitted flash erases during both the RIFT drone and tail; those erases pause audio. The 0.5 regression test checks that neither automatic write occurs while sounding and that pending state saves after silence. Its hardware confirmation and the listening decision remain pending. The hardware recorder also now assigns timestamps from sample count so its release marker and file duration agree.

Version 0.4 expands the processor to eight interconnected, slowly modulated delay lines at 22.05 kHz, four input allpasses per side and an allpass in each feedback path. A cubic FEED curve and a limiter with unity gain below its knee preserve quiet tails; a 7 Hz cut prevents subsonic accumulation. FORM changes line-length scale, diffusion, scattering, damping and wet compensation. The generator remains at 44.1 kHz. SCAN also moves a smooth, periodic timbre field over time. PARTIAL's harmonic levels were rebalanced, and the default CAVERN patch is brighter, more modulated and much more spacious. Changing algorithms fades only the generator and preserves the tank. The output has a soft peak limiter rather than a hard ceiling.

A host render of the earlier CAVERN patch measured about −82 dBFS nine seconds after release. The revised engine, using that earlier patch, measured about −51 dBFS twenty seconds after release. The revised factory CAVERN patch holds an audible tail longer still. These are measurements of software renders, not a claim that it matches a physical VHIKK-X. On-device performance and a listening assessment of 0.4 remain pending.

Version 0.3 addresses the similar-sounding final three modes and weak processor controls. PARTIAL, CROSS and CAVERN have the distinct voicings above. FORM now controls real stereo allpass diffusion, continuous lossless scattering rotations, and damping. The old crossfade between identity and Hadamard matrices cancelled feedback energy at intermediate settings; the replacement preserves it. CELL uses four wideband allpass frequency shifters, reaches approximately -258 to +250 Hz with fine control near zero, and transforms the wet sound as well as its feedback. It no longer crossfades filtered audio with an unmatched raw path. FEED has a stronger middle range while remaining below unity. High-precision filter state keeps the new shifters quiet after the sound stops. Existing parameter ranges, factory indices and patch storage are retained, but the revised DSP intentionally changes saved sounds.

Version 0.2 fixes the full-screen refresh on each knob turn: a value edit redraws just its number. PRESETS now recalls three distinct sounds per algorithm (the original patch, an airy variation, and a metallic variation), rather than switching control pages. ALGORITHM recalls the corresponding variation for the new mode. These recalls preserve root pitch and drone/mute state while loading the sound's macros. The top line shows SOUND 1/3, 2/3 or 3/3. The original nine factory preset indices remain valid for existing saved projects.

## Controls

On the RIFT home screen:

| FM1 control | Action |
| --- | --- |
| ALGORITHM | Select one of nine modes and recall its current sound variation |
| SELECT | BASIS: root pitch in MIDI semitones, 0–108 on the panel |
| PRESETS | Recall one of three sounds within the selected algorithm; keep the active control page |
| K1–K4, GEN | MORPH, SPAN, WARP, FIELD |
| K1–K4, FX | TIME, FORM, FEED, CELL |
| K1–K4, VAR | SEED, SCAN, FUSE, MIX |
| ENV or EDIT | Open GEN page |
| FX | Open FX page |
| LFO | Open VAR page |
| ARP | Toggle continuous drone / keyed generator |
| PLAY | Toggle generator mute; existing tails continue to decay |
| REC | Randomize SEED and SCAN, keeping other controls |
| OCT− / OCT+ | Move root and keyboard octave down/up |
| HOME | Open Felucca's system menu |
| SAVE | Open Felucca's existing save pages and overwrite-confirmation flow |
| GLO | Open the standard engine edit pages; HOME returns to RIFT |
| Master pot | Output volume, including the first audio block after boot |

SCL and SEQ have no special home-screen action in this version. ENV/EDIT, FX and LFO select the GEN, FX and VAR macro pages. PRESETS recalls actual sounds in place. The standard editor/preset browser after GLO also lists all 27 factory sounds.

SPAN, WARP, FIELD, FEED, FUSE and CELL show −64…+63, with zero at stored value 64. WARP/FUSE meanings vary by mode as described above. FIELD is open at zero, low-pass CCW and high-pass CW; its final few CCW steps close the source, leaving processor tails. FEED has no recirculation at zero and longer feedback toward either side. CW mixes shifted and reference signals inside the tank and at its output; this is an interference effect, so decay also depends on CELL. CELL translates the wet signal and its feedback, reaching approximately −258 to +250 Hz; zero translation retains the allpass reference phase response.

TIME slews delay lengths. FORM changes line-length scale, input/tank allpass diffusion, orthogonal scattering, damping and wet level. MIX crossfades generator and processor output. SEED/SCAN smoothly scan deterministic random fields: changing them and returning restores the same target landscape, while oscillator phases and an existing feedback tail continue. SCAN is not a free-running LFO rate.

SAVE > PROJECT stores all macro values, root pitch and drone state using Felucca's existing project storage. Its ordinary autosave also includes this state. A USER sound preset stores sound parameters according to Felucca's existing rules, which keep some keyboard/arp parameters outside a sound preset; use PROJECT for a complete RIFT snapshot. No custom flash-writing path was added.

RIFT repurposes several common part-1 parameters for extra macros. In the generic Felucca editor their labels remain: DIST = FUSE, CHO = BASIS, DLY = CELL, REV = MIX, LFO phase = SEED, LFO rate = SCAN, ARP HOLD = DRONE. The RIFT home screen uses their instrument labels. Do not expect the generic LFO/effects controls to perform their usual Felucca functions while this engine owns the output.

## MIDI

Notes on MIDI channel 1 control the root (with the default routing). Overlapping notes use last-note legato priority and return to the remaining held note. USB and TRS MIDI use Felucca's existing parsers. The RIFT-specific CC map follows the channel routed to part 1:

| CC | Macro | CC | Macro |
| --- | --- | --- | --- |
| 20 | BASIS | 27 | FORM |
| 21 | SPAN | 28 | FEED |
| 22 | WARP | 29 | CELL |
| 23 | MORPH | 30 | MIX |
| 24 | FIELD | 31 | SEED |
| 25 | FUSE | 32 | SCAN |
| 26 | TIME | 33 | ALGORITHM (0–127 scaled to 0–8) |
| 69 | DRONE (0–63 off, 64–127 on) | 120 | Stop generator, release notes |

All macros except ALGORITHM accept the 0–127 range directly; SPAN/WARP/FIELD/FEED/FUSE/CELL center at 64. CC32 overrides Felucca's usual bank-LSB handling for the RIFT part. Standard Felucca CCs remain present, so controllers should use the table above. Pitch bend, mod wheel, velocity-to-VCA, aftertouch and the generic modulation matrix are not yet applied to RIFT's independent DSP. This version does not emulate the reference module's CV inputs or their normalization.

## Deliberate limits and unverified behavior

- This is an approximation from observable behavior. No original VHIKK-X firmware or wave assets were obtained or examined, and there has been no comparison against a physical reference unit.
- The current reference instrument has 18 documented algorithms. RIFT's nine original modes are an initial scope, with no newer granular, comb, dispersor or effects-only bank.
- The processor uses eight 8,192-sample 16-bit delay lines at 22.05 kHz, reusing Felucca's 128 KiB delay allocation, plus 24 KiB of allpass diffusion memory and 1 KiB of short-delay chorus memory. The longest individual line is about 371 ms; actual lengths depend on TIME, FORM, SEED, modulation and the line. This is considerably shorter than the reference's long-delay range. Other DSP state and mixing use 32-bit fixed point.
- The processor and its allpass frequency shifter are original fixed-point implementations based on published DSP mathematics. They are not the reference instrument's implementations. The shifter test covers 65, 130 and 1000 Hz in both directions at both 44.1 and 22.05 kHz; performance outside that tested range is not established by those measurements alone.
- No stereo external audio processing is implemented. Audio-rate CV, CV attenuverters, hardware normalization, analog levels and USB mass-storage behavior are not reproduced. Felucca supplies USB audio output capture instead.
- BASIS currently follows equal-tempered MIDI pitches with smoothing. Sub-audio tuning below MIDI 0, continuous large-range tuning, exact scale quantizers, per-algorithm parameter memories and reference-style delayed saves are not implemented.
- Mode changes briefly fade the generator and retain the old tail. Switching away from the RIFT engine clears its shared delay ownership. Extreme cross-modulation can alias; only the basic saw/pulse edges use polyBLEP correction.
- RIFT 0.1 booted on the connected unit. A later RIFT 0.2 CDC query reported 39% CPU, 1 late audio block and 0 shed voices. Controlled USB audio captures of 0.2 confirmed weak CROSS FORM and weak CELL responses. The original live patch was restored and all parameter values verified after capture. Version 0.8 passes host macro-behavior, live controls/DAC, sound-response, long-decay, persistence, palette and sanitizer tests. Version 0.7 has a reproduced device deadline regression; verification of the optimized 0.8 build remains pending installation.
- Version 0.8 uses local update identity `FM-1_908`, following 0.7's `FM-1_907`, 0.6's `FM-1_906`, 0.5's `FM-1_905`, 0.4's `FM-1_904`, 0.3's `FM-1_903` and 0.2's `FM-1_902`. The first 0.2 package incorrectly reused 0.1's `FM-1_900`, causing M-UPGRADE's same-version refusal. The package and running firmware share the identity in `firmware/src/rift_update_id.h`. Treat it as a development package, not a registered public firmware release.

## Build and verification

From the workspace above this checkout:

```sh
sh rift.sh test
sh rift.sh preview
sh rift.sh build
```

The native simulator has an independent flash file at `build/rift-preview-flash.bin` and does not need the FM1. It starts with MIDI disabled to avoid inadvertently receiving the hardware's controls. Its mouse/keyboard panel is documented in [platforms/sdl/README.md](../platforms/sdl/README.md).

`tests/run_rift.sh` compiles the full firmware host with AddressSanitizer and UndefinedBehaviorSanitizer. Tests cover boot defaults, page/encoder mapping, MIDI macros, overlapping notes, project save/restore, all modes' levels/stereo/DC, deterministic reset, frequency-shift direction/sideband suppression at a 1 kHz test tone, random macro sweeps, and mute decay. It generates four-second 44.1 kHz stereo WAV files and screenshots under `build/rift-demo/`.

`sh tests/run_rift_controls.sh` reproduces dial edits through the complete host input/render path. It measures LCD writes, checks that PRESETS changes actual rendered audio while keeping the algorithm/page, verifies all 27 sound variations, compares all nine algorithms with identical macros, and checks that TIME, FORM, FEED and CELL reach the DSP. Before the fixes it reported 14 LCD writes for one detent, unchanged audio from PRESETS, and 3 pairs of identical algorithms.

`sh tests/run_rift_behavior.sh` checks a two-tone filter input, cluster anchor/midpoint spectra, repeatable procedural targets after returning SEED/SCAN, center-versus-endpoint feedback tails, phase-aligned routing at neutral CELL and spectral differences between FEED sides when CELL is active. It writes controlled WAVs and `behavior-response.json` to `build/audio-behavior/`.

`sh tests/run_rift_audio.sh` adds bass-frequency sideband rejection tests and sustained-drone/decay renders for PARTIAL, CROSS and CAVERN. NumPy analysis checks broad spectral response to FORM/CELL, decay/resonance changes from TIME and longer tails from FEED, and upper harmonic energy in PARTIAL. These are measurable response proxies, not a guarantee of subjective sound quality. The former FIR measured only 0.8 dB image rejection at 65 Hz; the replacement measures about 45 dB there with unity gain. The hardware probe in `scripts/probe_rift_hardware.py` records only an exact Felucca USB input, temporarily changes the live patch, and restores/verifies all original parameters in `finally`; it does not send update or flash-write commands. The `--space` option requires a matching CDC version (default `RIFT 0.8`) before changing any parameter, captures a 48-second CAVERN drone and 32-second tail, and records CPU/deadline counters. It restores the original live parameters and selected track afterward.

`sh tests/run_rift_space.sh` renders a six-second factory CAVERN drone followed by thirty seconds of decay. It checks audible energy twenty seconds after release, continued decay, stereo correlation, bounded peaks and preservation of the tail when the algorithm changes. `build/audio-space/CAVERN-base.wav` is the complete render for listening. A second 60-second render, `CAVERN-evolving.wav`, holds the same note with unchanged controls for 48 seconds; the report quantifies spectral motion and checks for bounded long-hold buildup. Version 0.7 intentionally removes the requirement for a large spectrum change at fixed SCAN because SCAN is a static landscape coordinate. These thresholds protect measurable properties; they do not replace a listening decision about the desired atmosphere.

`sh tests/run_rift_palette.sh` renders all 27 factory sounds under ASan/UBSan to `build/audio-palette/`, then checks the measured palette and all 117 mode/dial combinations. Each factory WAV contains sixteen seconds held and four seconds of release, without loudness normalization. Files use the screen's sound number: `PAIR-1.wav` through `CAVERN-3.wav`. The build bundle copies these to `dist/audio/palette/` and includes `palette-response.json`. Listen through the held section to assess buildup and the final four seconds to assess the space. All 27 sounds use property checks in 0.7. PAIR modulator SPAN/SCAN and SEED response is measured with cross-modulation engaged; PAIR FEED/SEED use wet output to expose processor changes; FIELD compares its LP/HP regions because both endpoints can silence a low-pitched source, and FEED compares center against CW maximum.

`sh tests/run_rift_idle.sh` exercises the real host audio/main/storage loop for a drone, a released tail and silence. It checks that autosave and pending settings writes defer while sounding, then save once after silence. The upstream persistence test also passes with RIFT disabled, including rollback, retry and autosave wear checks.

The target build also checks image sizes, flash/RAM regions, flash-critical RAM code and that application source does not directly access hardware registers. The existing simulated installer and project-format tests were run. The entire upstream test suite has not been run; this fork intentionally changes its boot sound and home UI.

## Hardware trial and rollback

The device initially reported stock `FM-1_015`; after the user's M-UPGRADE installation, USB CDC reports RIFT 0.1. The agent has not sent an update or bootloader command or changed calibration. Hardware probes temporarily changed the live patch and restored/verified it; 0.4's unintended background autosave during sound was reproduced. A full-device backup has not been completed.

Before installing, obtain the unmodified official V15 package and preserve any stock patches you need. Felucca's installer recognizes its expected SHA-256: `db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a`. A stock package is not a backup of unit-specific flash. Follow the FM1-specific backup/recovery documentation linked from [Felucca's build guide](../BUILDING.md); do not use the SMK37 flash map from the linked porting kit. Do not erase or rewrite protected boot/calibration/key ranges.

After a backup/recovery route is established and installation is authorized, the existing installer accepts the custom package:

```sh
../.venv/bin/python tools/fm1_install.py ../dist/rift-0.8.fwsc
```

That command **writes firmware**; it has not been run. It may replace incompatible saved data. Keep power and USB stable through the update. A firmware update can fail, and some recovery cases require the hardware rescue tools linked by upstream.

The first hardware trial should start at low master volume, confirm the home controls and MIDI, inspect the CPU meter through HOME > system information while sounding the heaviest modes, then check noise/dropouts, mode changes, maximum feedback, mute tails, a project save, and a power cycle. Record the actual behavior and iterate before treating the firmware as a reliable instrument.

## Source and licenses

New DSP and UI are in `firmware/src/eng_rift.c`, `rift_dsp.c`, `rift_ui_draw.c` and `rift_ui_input.c`. Changes to engine registration, default boot, the master mixer, MIDI and preset loading integrate them. Defining `FELUCCA_RIFT=0` at compile time disables the new instrument; the standard host DSP tests default to that setting. Full FM1/desktop entry points enable RIFT by default.

The custom sources use GPL-3.0-only, retaining the upstream firmware's licensing. Engines (MIT), board support (MPL-2.0), fonts, ported DSP and the packaged SDK retain their respective notices in `LICENSE`, `LICENSING.md` and `LICENSES/`. The frequency shifter follows the published equations and coefficients in Olli Niemitalo's [90 degree phase difference IIR allpass pair](https://www.mikrocontroller.net/attachment/33904/hilbert_Olli_Niemitalo.pdf) (July 2003), converted to original fixed-point code. Frequency translation follows the analytic-signal construction described in [Miller Puckette's synthesis text](https://msp.ucsd.edu/techniques/latest/book-html/node159.html). `tools/rift_hilbert.py` preserves the previous FIR generator for historical comparison; it is not the current shifter.
