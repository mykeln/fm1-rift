# RIFT for the M-VAVE FM-1

Experimental drone firmware, heavily inspired by the dystopian sounds of the VHIKK-X, by FORGE TME. This firwmare is a built on a modified version of  [Felucca](https://github.com/hugelton/Felucca).

Nine original modes combine oscillator pairs and clusters, phase/ring modulation, filtering, stereo delay/scattering feedback and frequency translation.

The result, some nasty, evil sounding stuff.

## Sound and controls

Modes: **PAIR, FOLD, SINES, WAVES, CHORD, CLUSTER, PARTIAL, CROSS and CAVERN**. RIFT occupies part 1 and owns the stereo output while active. Felucca supplies hardware support, USB MIDI/audio, storage, the updater and recovery entry.

SELECT sets root pitch, ALGORITHM chooses a mode, PRESETS recalls one of three sounds per mode. ENV/EDIT, FX and LFO select the three macro pages; K1–K4 edit the active page. ARP toggles drone/keyed operation; REC varies SEED/SCAN; the master volume pot keeps its usual function. See the [RIFT guide](firmware/docs/RIFT.md) for every control, MIDI CC, implementation detail and known limit, and the [reference research](docs/vhikk-x-reference.md) for the documented inspiration.

## Get the source and run host tests

```sh
git clone --recurse-submodules https://github.com/mykeln/fm1-rift.git
cd fm1-rift
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
sh rift.sh test
```

Host tests need a C compiler and Python 3. They generate headers, run RIFT integration tests under AddressSanitizer/UndefinedBehaviorSanitizer, and render nine short stereo WAV previews and screen captures into `firmware/build/rift-demo/`. CI also checks the installer against simulated hardware. Host tests do not establish real-device performance.

For an existing clone, run `git submodule update --init --recursive`. The engine and board-support dependencies are pinned. GitHub's automatic source ZIP/tar downloads omit those dependencies; use a recursive Git clone.

## Preview and build

```sh
sh rift.sh preview    # desktop simulator; requires SDL2
sh rift.sh build      # FM-1 build and local review bundle; requires JieLi tools/SDK
```

Install SDL2 with `brew install sdl2` on macOS or `libsdl2-dev` on Debian/Ubuntu. The simulator uses its own flash file and starts with MIDI disabled.

For the FM-1 build, follow [firmware/BUILDING.md](firmware/BUILDING.md) for the JieLi compiler, pinned AC79 SDK and Docker prerequisites. Run commands from `firmware/` when following that upstream guide. RIFT's wrapper respects `PYTHON`, `JIELI_TOOLCHAIN`, `AC79_SDK` and `DOCKER_CONTEXT`. Without local overrides, the underlying build uses `~/.jieli/toolchain` and `~/fw-AC79_AIoT_SDK`; Linux x86-64 runs the toolchain natively.

The original local setup keeps tools in `.tools/` and uses a Colima profile/context named `fm1-build` / `colima-fm1-build` on macOS. Those tools, the SDK, build products and `.venv/` are excluded from Git. They are not required for host tests. Start that VM with `colima start --profile fm1-build` if you use that setup.

The local bundle is written to `dist/`. Packages still use the upstream development identity `FM-1_900`. A unique identity and real-device validation are required before a public installable release. Do not use Felucca's `--release` version numbering to publish RIFT packages: it assigns Felucca identities.

## Hardware validation and publication

Read the [hardware trial and rollback guide](firmware/docs/RIFT.md#hardware-trial-and-rollback) before considering installation. Installing may replace incompatible stored data. RIFT reuses Felucca's project formats, but cross-firmware project/sample compatibility and persistence have not been verified on hardware. Preserve your data and establish a recovery route first.

[Publication tracking](docs/PUBLICATION.md) records directory submissions and the remaining release checks. [Directory research](docs/publication-research.md) links each maintainer's submission requirements. The Firmware Hub requires install/exit tests on a real device before listing; this project currently belongs in experimental source listings.

## Contributing and licenses

[Contributions and bug reports](CONTRIBUTING.md) are welcome. The modified Felucca source is under `firmware/`; upstream Git history is retained. RIFT is based on Felucca commit `743551712c36b3984392eea1dbe13a309907e17a`.

RIFT is **GPL-3.0-only**; dependencies retain their own licenses. See [LICENSE](LICENSE), [LICENSING.md](LICENSING.md), [upstream credits](firmware/CREDITS.md) and [component notices](firmware/LICENSES/). No VHIKK-X firmware, code or wavetable assets were used. No affiliation with M-VAVE or FORGE TME is implied.
