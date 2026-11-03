# Contributing to RIFT

Report RIFT bugs and propose changes in [this repository](https://github.com/mykeln/fm1-rift/issues). Include the commit, host OS or FM-1 hardware, reproduction steps, and whether the result was observed in simulation or on a real device.

Clone with `--recurse-submodules`, install the dependencies in the README, and run `sh rift.sh test`. Changes to device behavior also need real FM-1 testing before an installable release. Describe the checks you ran in your pull request; do not claim hardware verification from host tests.

Keep changes focused. New firmware, tooling and test files use an `SPDX-License-Identifier: GPL-3.0-only` header. Existing components retain their own licenses; see [LICENSING.md](LICENSING.md). Contributions to RIFT use GPL-3.0-only.

RIFT is derived from [Felucca](https://github.com/hugelton/Felucca). Report RIFT-specific problems here. For changes intended for Felucca itself, follow its [contribution guide](firmware/CONTRIBUTING.md). The pinned engine and board-support dependencies have their own upstream repositories and contribution policies.
