# RIFT 0.2 — experimental FM1 drone firmware

RIFT is an original implementation inspired by the [VHIKK-X's public operating guide](https://forge-tme.com/vhikk-x/) and [launch algorithm descriptions](https://forge-tme.com/vhikk-x/algos_launch.html). It is built on Felucca commit `743551712c36b3984392eea1dbe13a309907e17a`, with its pinned engines and board-support submodules. The supplied FM1-to-SMK37 repository helped identify this firmware family; no SMK37 board mappings or flash map were applied to the FM1.

## What is implemented

Nine original modes share four interacting oscillators (two in PAIR/FOLD), continuous waveform interpolation, bipolar phase/ring modulation, continuously varying pitch/timbre clusters, filtering, a generator gate, stereo delay feedback, a delay-to-scattering macro, signed frequency shifting, and dry/wet mixing. The oscillators run continuously in drone mode. Keys or MIDI select a new root pitch. Keyed mode closes the generator when the last key is released, leaving the feedback tail.

RIFT occupies part 1 and exclusively owns the stereo output while that part requests its engine. Other Felucca parts are not mixed during RIFT operation. The normal Felucca engines remain available by changing part 1's engine in the standard editor. RIFT's engine number is 15; existing engine numbers were preserved, including the disabled ADPCM slot at 14.

| Mode | Implementation |
| --- | --- |
| PAIR | Two continuously morphing oscillators with phase or ring modulation |
| FOLD | Two oscillators with a sinusoidal folding stage |
| SINES | Four sine oscillators with continuously varying relative tuning |
| WAVES | Four sine/triangle/saw/pulse waveform scans |
| CHORD | Four oscillators with semitone-quantized interval spread |
| CLUSTER | Four continuously pitched, waveform-scanned oscillators |
| PARTIAL | Four sine oscillators with progressively spaced tuning |
| CROSS | Four waveform-scanned oscillators with cross-modulation |
| CAVERN | Cluster generation with a diffuse feedback starting patch |

These names identify the new implementation. They do not claim exact equivalence to the reference algorithms. The modes differ in pitch/wave rules and modulation; CROSS adds its own coupled-oscillator route and CAVERN uses a more diffuse scattering range. They share the same processor foundation rather than the reference instrument's separate processor families.

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

WARP and CELL show −64…+63; zero is their neutral center. Positive WARP introduces phase modulation; negative WARP introduces ring modulation. SPAN spreads the oscillators' relative pitches. MORPH scans their waveforms and, in the cluster modes, pitch positions. FIELD controls the low-pass filter. FUSE adds resonance and controls folding in FOLD. TIME changes delay lengths with interpolation and slew; FORM mixes independent echoes into an orthogonal four-line scattering network. FEED controls feedback gain below unity, with saturation inside the loop. CELL introduces a translated feedback component; MIX crossfades dry generator and processor output. SEED/SCAN interpolate a deterministic random field rather than abruptly reseeding audio.

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

All macros except ALGORITHM accept the 0–127 range directly; WARP/CELL center at 64. CC32 overrides Felucca's usual bank-LSB handling for the RIFT part. Standard Felucca CCs remain present, so controllers should use the table above. Pitch bend, mod wheel, velocity-to-VCA, aftertouch and the generic modulation matrix are not yet applied to RIFT's independent DSP. This version does not emulate the reference module's CV inputs or their normalization.

## Deliberate limits and unverified behavior

- This is an approximation from observable behavior. No original VHIKK-X firmware or wave assets were obtained or examined, and there has been no comparison against a physical reference unit.
- The current reference instrument has 18 documented algorithms. RIFT's nine original modes are an initial scope, with no newer granular, comb, dispersor or effects-only bank.
- The processor uses four 16,384-sample 16-bit delay lines at 44.1 kHz, reusing Felucca's 128 KiB delay allocation. The longest individual line is about 371 ms; actual lengths depend on TIME, SEED and the line. This is considerably shorter than the reference's long-delay range. Other DSP state and mixing use 32-bit fixed point.
- FORM uses feedback scattering; a convincing reverb with independent diffusion/modulation stages still needs work. The finite 31-tap Hilbert FIR gives approximate single-sideband translation; image rejection worsens near DC and Nyquist. It is not the reference's frequency-shifter implementation.
- No stereo external audio processing is implemented. Audio-rate CV, CV attenuverters, hardware normalization, analog levels and USB mass-storage behavior are not reproduced. Felucca supplies USB audio output capture instead.
- BASIS currently follows equal-tempered MIDI pitches with smoothing. Sub-audio tuning below MIDI 0, continuous large-range tuning, exact scale quantizers, per-algorithm parameter memories and reference-style delayed saves are not implemented.
- Mode changes briefly fade down and reset the tank. Sweeping topology does not preserve a continuous old tail. Extreme cross-modulation can alias; only the basic saw/pulse edges use polyBLEP correction.
- RIFT 0.1 booted on the connected unit. USB CDC reported 36% CPU, 0 late audio blocks, and 0 shed voices while the instrument was running. This confirms basic operation, not a full hardware/audio validation. RIFT 0.2 has host coverage for controls, every sound variation and every pair of algorithms with identical macros; on-device confirmation of this update remains pending.
- The shipped package retains Felucca's updater identity `FM-1_900`; its displayed version is `RIFT 0.2`. Treat it as a development package.

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

The target build also checks image sizes, flash/RAM regions, flash-critical RAM code and that application source does not directly access hardware registers. The existing simulated installer and project-format tests were run. The entire upstream test suite has not been run; this fork intentionally changes its boot sound and home UI.

## Hardware trial and rollback

The device initially reported stock `FM-1_015`; after the user's M-UPGRADE installation, USB CDC reports RIFT 0.1. The agent has not sent an update or bootloader command, changed calibration, or performed flash writes. A full-device backup has not been completed.

Before installing, obtain the unmodified official V15 package and preserve any stock patches you need. Felucca's installer recognizes its expected SHA-256: `db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a`. A stock package is not a backup of unit-specific flash. Follow the FM1-specific backup/recovery documentation linked from [Felucca's build guide](../BUILDING.md); do not use the SMK37 flash map from the linked porting kit. Do not erase or rewrite protected boot/calibration/key ranges.

After a backup/recovery route is established and installation is authorized, the existing installer accepts the custom package:

```sh
../.venv/bin/python tools/fm1_install.py ../dist/rift-0.2.fwsc
```

That command **writes firmware**; it has not been run. It may replace incompatible saved data. Keep power and USB stable through the update. A firmware update can fail, and some recovery cases require the hardware rescue tools linked by upstream.

The first hardware trial should start at low master volume, confirm the home controls and MIDI, inspect the CPU meter through HOME > system information while sounding the heaviest modes, then check noise/dropouts, mode changes, maximum feedback, mute tails, a project save, and a power cycle. Record the actual behavior and iterate before treating the firmware as a reliable instrument.

## Source and licenses

New DSP and UI are in `firmware/src/eng_rift.c`, `rift_dsp.c`, `rift_ui_draw.c` and `rift_ui_input.c`. Changes to engine registration, default boot, the master mixer, MIDI and preset loading integrate them. Defining `FELUCCA_RIFT=0` at compile time disables the new instrument; the standard host DSP tests default to that setting. Full FM1/desktop entry points enable RIFT by default.

The custom sources use GPL-3.0-only, retaining the upstream firmware's licensing. Engines (MIT), board support (MPL-2.0), fonts, ported DSP and the packaged SDK retain their respective notices in `LICENSE`, `LICENSING.md` and `LICENSES/`. The Hilbert coefficients are calculated by `tools/rift_hilbert.py`; frequency translation follows the standard analytic-signal modulation construction described in [Miller Puckette's synthesis text](https://msp.ucsd.edu/techniques/latest/book-html/node159.html).
