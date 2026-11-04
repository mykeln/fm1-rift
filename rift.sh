#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build/preview only. Deliberately contains no firmware install command.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -x "$ROOT/.venv/bin/python" ]; then
  export PATH="$ROOT/.venv/bin:$PATH"
fi
export PYTHON="${PYTHON:-python3}"
if [ -d "$ROOT/.tools/jieli/toolchain" ]; then
  export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$ROOT/.tools/jieli/toolchain}"
fi
if [ -d "$ROOT/.tools/ac79-sdk" ]; then
  export AC79_SDK="${AC79_SDK:-$ROOT/.tools/ac79-sdk}"
fi
# The original local macOS setup uses a dedicated Colima context.
if [ "$(uname -s)" = Darwin ] && command -v docker >/dev/null 2>&1 &&
   docker context inspect colima-fm1-build >/dev/null 2>&1; then
  export DOCKER_CONTEXT="${DOCKER_CONTEXT:-colima-fm1-build}"
fi
cd "$ROOT/firmware"
case "${1:-test}" in
  test) sh build.sh gen; sh tests/run_rift.sh; sh tests/run_rift_controls.sh; sh tests/run_rift_audio.sh; sh tests/run_rift_space.sh; sh tests/run_rift_idle.sh ;;
  build) sh build.sh; "$PYTHON" "$ROOT/scripts/package_rift.py" ;;
  preview)
    sh build.sh sdl
    exec build/sdl/felucca "$ROOT/firmware/build/rift-preview-flash.bin" --no-midi
    ;;
  *) echo "usage: sh rift.sh test|build|preview" >&2; exit 2 ;;
esac
