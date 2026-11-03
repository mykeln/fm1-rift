# FM-1 publication and directory research

Checked 2026-11-03 against the maintainers' own sites and repositories. This note records requirements and recommendations; it does not mean a listing has been accepted.

RIFT 0.1 booted on an FM-1; the current 0.2 update has host tests but awaits device confirmation. Full installation/exit and persistence validation are still pending. Preserve that distinction in every submission. Another unrelated [RIFT firmware by Vaayaa](https://github.com/Vaayaa/rift-fw) already exists; use the author and repository link to distinguish this project.

## Where to submit

| Venue | Submission path | Suitable now? |
| --- | --- | --- |
| [Awesome M-VAVE FM-1](https://github.com/cicloid/awesome-fm-1) | One-entry README pull request. | Yes, accurately labeled experimental with current-version device validation pending. |
| [aroum's FM1 resource hub](https://github.com/aroum/fm1-custom-fw) | README pull request under the Felucca ecosystem; follow the existing table. | Yes as a source project, subject to maintainer review. |
| [FM1 Editor firmware directory](https://fm1-editor.com/firmware/) | Its footer directs missing-project reports to [GitHub issues](https://github.com/benny-sparra/fm1-dx7-patch-importer/issues). | Request the **watch list**, which already includes hardware-untested projects. |
| [FM-1 Firmware Collection](https://github.com/2bbb/fm1-firmware-collection) | Automatic discovery; manual `catalog/overrides.json` inclusion is also supported. | Catalog inclusion is possible; identify the experimental status. |
| [FM-1 Firmware Atlas](https://github.com/wero1414/atlas-fm-1) | Add `data/firmwares.json` entry and regenerate with `python3 build.py`. | Yes with explicit development status. |
| [The Firmware Hub](https://thefirmwarehub.com/submit/) | Email submission after all requirements below are satisfied. | **No: real hardware testing is required first.** |

The first two suitability judgments are recommendations based on their published scope and current entries, not promises of acceptance. aroum has accepted [firmware listing PRs](https://github.com/aroum/fm1-custom-fw/pull/21); no separate contribution policy was found in its root.

## Requirements and useful details

[Awesome's contribution guidelines](https://github.com/cicloid/awesome-fm-1/blob/main/CONTRIBUTING.md) request one line in the format `- [Name](url) - Description.`, with author, function and software license. Use an appropriate section, a working link, and meaningful changes beyond a bare fork. Submit one entry per PR. A truthful description would identify this as mykeln's experimental Felucca-based drone firmware with nine original modes, GPL-3.0-only, full hardware validation pending.

The [FM1 Editor directory's watch list](https://fm1-editor.com/firmware/#watch-list) explicitly covers projects that cannot yet be installed and includes projects whose packages have not been hardware tested. Report the public repository, status, build instructions and available previews; do not imply normal installer availability. The missing-project issue link is published in the directory footer.

The [collection README](https://github.com/2bbb/fm1-firmware-collection#how-it-works) describes nightly discovery from the Hub, aroum's list and GitHub. Its [discovery code](https://github.com/2bbb/fm1-firmware-collection/blob/main/scripts/catalog.py) filters GitHub results by descriptions containing both `fm-1`/`fm1` and `firmware`, so include those terms in the public repository description. It recognizes `firmware/build.sh`. A root license helps license detection. Without an upstream package, a recognized build recipe and redistributable license can trigger its unofficial CI build; those builds are explicitly untested. The [overrides file](https://github.com/2bbb/fm1-firmware-collection/blob/main/catalog/overrides.json) supports manual inclusion and recipe overrides.

The [Atlas README](https://github.com/wero1414/atlas-fm-1#contributing) lists its JSON fields and build command. Do not supply invented hardware behavior, polyphony, MIDI or installation claims merely to populate the schema.

### The Firmware Hub gate

The Hub requires public complete source, a GitHub release with `.fwsc`, a unique `FM-1_<number>` identity, the update loader, and real-device tests. Root `fmhub.json` is optional card metadata. Test installation from stock V15 and another firmware, then installation away again; emulator success does not qualify. Its submission page also links the identity checker. [Requirements](https://thefirmwarehub.com/submit/)

`fmhub.json` accepts only `what`, `author`, `kinds`, `play`, `editor`, `cheat`, `image`. Use plain English `what` of 20–300 characters, author of 2–60 characters, and a category such as `synth`. Optional links use HTTPS. The image is a repository path to a square PNG/WebP/JPEG, 240–1024 pixels, at most 300 KB; a 240-pixel screen capture is preferred. [Maker metadata specification](https://thefirmwarehub.com/fmhub.md)

When eligible, send the firmware and author names, repository, identity, license, and confirmation of device/exit tests to `zven@thefirmwarehub.com`. New `.fwsc` releases and metadata updates are subsequently detected automatically. [Submission procedure](https://thefirmwarehub.com/submit/)

## Release preparation

Recommendations for this project: publish the complete modified firmware source and retained component licenses, make a clean clone reproducible, keep packages out of ordinary Git history, and use a tagged experimental release only when its exact corresponding source and build inputs are available. Include checksums, provenance, controls, backup/recovery instructions and a recorded hardware test result before offering routine installation. A simulator screen capture and host-generated audio previews can support source-project listings now, provided their origin is explicit.
