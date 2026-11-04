#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. tools/hostflags.sh
host_gen_ok rift
mkdir -p build/host build/audio-diagnosis/host
for test in rift_audio_probe rift_shift_test; do
    ${CC:-cc} -O2 -w -fsanitize=address,undefined -fno-sanitize-recover=all \
        $HOST_INC "tests/$test.c" -lm -o "build/host/$test"
done
ASAN_OPTIONS=detect_leaks=0 build/host/rift_shift_test
ASAN_OPTIONS=detect_leaks=0 build/host/rift_audio_probe build/audio-diagnosis/host
"${PYTHON:-python3}" tests/rift_audio_response.py build/audio-diagnosis/host
