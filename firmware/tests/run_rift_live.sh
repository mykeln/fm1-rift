#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. tools/hostflags.sh
host_gen_ok rift
mkdir -p build/host build/audio-live
${CC:-cc} -O1 -g -std=gnu99 -w -fsanitize=address,undefined -fno-sanitize-recover=all \
    $HOST_INC tests/rift_live_test.c -lm -o build/host/rift_live_test
ASAN_OPTIONS=detect_leaks=0 build/host/rift_live_test build/audio-live
"${PYTHON:-python3}" tests/rift_live_response.py build/audio-live
