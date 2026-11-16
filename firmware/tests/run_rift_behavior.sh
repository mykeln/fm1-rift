#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. tools/hostflags.sh
host_gen_ok rift
mkdir -p build/host build/audio-behavior
${CC:-cc} -O2 -w -fsanitize=address,undefined -fno-sanitize-recover=all \
    $HOST_INC tests/rift_behavior_probe.c -lm -o build/host/rift_behavior_probe
ASAN_OPTIONS=detect_leaks=0 build/host/rift_behavior_probe build/audio-behavior
"${PYTHON:-python3}" tests/rift_behavior_response.py build/audio-behavior
