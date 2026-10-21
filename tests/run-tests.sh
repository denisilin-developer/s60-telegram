#!/bin/sh
set -e
cd "$(dirname "$0")/.."
sh tests/build-host-crypto.sh
M=../common/mbedtls
INC="-Icore -I../common/net"
for d in $(cat ../common/group/mbedtls-includes.txt); do INC="$INC -I$M/$d"; done
FLAGS="-std=gnu99 -O1 -g -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -fsanitize=address,undefined -fno-sanitize-recover=undefined"
${CC:-cc} $FLAGS $INC core/*.c ../common/host/rsym_tcp_posix.c tests/tg_cli.c tests/srp_check.c \
    build/host/libcrypto.a -lz -o build/host/tg_cli
build/host/tg_cli "${@:-selftest}"
