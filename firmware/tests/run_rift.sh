#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. tools/hostflags.sh
host_gen_ok rift
mkdir -p build/rift-demo build/host
${CC:-cc} -O0 -g -std=gnu99 -w -fsanitize=address,undefined -fno-sanitize-recover=all \
    $HOST_INC tests/rift_test.c -lm -o build/host/rift_test
ASAN_OPTIONS=detect_leaks=0 build/host/rift_test build/rift-demo
