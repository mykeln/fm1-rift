#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. tools/hostflags.sh
host_gen_ok rift
mkdir -p build/host build/audio-space
${CC:-cc} -O2 -w -fsanitize=address,undefined -fno-sanitize-recover=all \
    $HOST_INC tests/rift_space_probe.c -lm -o build/host/rift_space_probe
ASAN_OPTIONS=detect_leaks=0 build/host/rift_space_probe build/audio-space
"${PYTHON:-python3}" tests/rift_space_response.py build/audio-space
