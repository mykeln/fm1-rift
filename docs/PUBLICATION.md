# Publication tracking

RIFT by mykeln is an experimental source project. RIFT 0.1 booted on hardware; 0.2 has host coverage and awaits device confirmation. No complete install/exit validation or installable public release has been completed. Listing requests must keep that status explicit.

## Repository and source listings

- Repository: https://github.com/mykeln/fm1-rift
- Awesome FM-1: submission pending.
- aroum FM1 resources: submission pending.
- FM1 Editor: watch-list request pending.
- FM-1 Firmware Atlas: submission pending.
- FM-1 Firmware Collection: catalog request pending.
- The Firmware Hub: not submitted; hardware requirements below remain open.

See [primary-source research](publication-research.md) for submission routes and requirements. Maintainers control acceptance; a submitted request is not an accepted listing.

## Before an installable release and Hub submission

- [ ] Choose a unique updater identity after checking the Hub's current registry; use it consistently in app identity responses and package metadata. Current `FM-1_900` belongs to the upstream development build.
- [ ] Decide the long-term project name: another author already publishes an unrelated RIFT. Current listings must say **RIFT by mykeln**.
- [ ] Preserve needed stock patches and establish an FM-1-specific backup/recovery route.
- [ ] Install the exact release package on a real FM-1 from stock V15.
- [ ] Install it from another custom firmware.
- [ ] Confirm USB MIDI enumeration and the unique identity response.
- [ ] Install another firmware from RIFT, proving the update entry/loader allows exit.
- [ ] Validate panel controls, listening, worst-case CPU/dropouts, feedback stability, mute, project save/restore and power cycling; record device and source commit.
- [ ] Document actual cross-firmware project/sample compatibility and data loss behavior.
- [ ] Build from the tagged source and pinned dependencies; publish `.fwsc`, SHA-256 sums, complete corresponding source, licenses and provenance in an explicitly experimental GitHub release.
- [ ] Add a screen capture and verified release links to the Hub submission; send the required information through its published submission route.

The root `fmhub.json` prepares card text only. It does not register the firmware or make it eligible. No hardware writes are part of repository publication.
