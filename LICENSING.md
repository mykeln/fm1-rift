# RIFT licensing and provenance

RIFT's original DSP, UI, integration, tests and project tooling are licensed under **GPL-3.0-only**. The complete license is in [LICENSE](LICENSE).

RIFT is a modified version of [Felucca](https://github.com/hugelton/Felucca), based on commit `743551712c36b3984392eea1dbe13a309907e17a`. Its history and original notices are retained. The original project is by Leo Kuroshita (@kurogedelic), Hügelton Instruments; see [credits](firmware/CREDITS.md) and the [component license inventory](firmware/LICENSING.md).

The six engine dependency is MIT, the board-support dependency is MPL-2.0, and fonts, samples and ported DSP retain their individual licenses. Both dependencies are pinned Git submodules. Clone recursively to obtain the complete source. GitHub's automatic ZIP/tar archives omit submodule contents.

Keep the notices and [component license texts](firmware/LICENSES/) with redistributed builds. Installable packages also contain three Apache-2.0 JieLi SDK files, as documented in the component inventory. The SDK and toolchain are fetched separately and are excluded from this repository.

No VHIKK-X firmware, code or wavetable assets were used. RIFT's modes are original approximations inspired by public behavior documentation. M-VAVE/FM-1, FORGE TME/VHIKK-X, Felucca and Hügelton Instruments names belong to their respective owners. This project is independent and does not imply their endorsement.
