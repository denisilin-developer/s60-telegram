#!/bin/sh
# Build mbedTLS's crypto (TF-PSA-Crypto) natively for host tests, from the
# same source list the phone build uses, with the stock config so entropy
# comes from the OS. -> build/host/libcrypto.a
set -e
cd "$(dirname "$0")/.."
M=../common/mbedtls
OUT=build/host
[ -f $OUT/libcrypto.a ] && exit 0
mkdir -p $OUT/obj
INC=""
for d in $(cat ../common/group/mbedtls-includes.txt); do INC="$INC -I$M/$d"; done
for s in $(grep '^tf-psa-crypto/' ../common/group/mbedtls-sources.txt); do
    o=$OUT/obj/$(echo "$s" | tr / _ | sed 's/\.c$/.o/')
    ${CC:-cc} -std=gnu99 -O2 -w $INC -c "$M/$s" -o "$o" &
    pids="$pids $!"
done
fail=
for p in $pids; do wait "$p" || fail=1; done
[ -z "$fail" ] || { echo "mbedTLS host build failed" >&2; exit 1; }
ar rcs $OUT/libcrypto.a $OUT/obj/*.o
