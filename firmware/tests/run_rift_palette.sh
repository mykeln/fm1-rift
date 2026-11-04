#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. tools/hostflags.sh
host_gen_ok rift
dest="${1:-build/audio-palette}"
mkdir -p build/host "$dest/controls"
${CC:-cc} -O2 -w -fsanitize=address,undefined -fno-sanitize-recover=all \
    $HOST_INC tests/rift_palette_probe.c -lm -o build/host/rift_palette_probe
ASAN_OPTIONS=detect_leaks=0 build/host/rift_palette_probe "$dest"
"${PYTHON:-python3}" tests/rift_palette_response.py "$dest"
