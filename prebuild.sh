#!/bin/sh
set -e
cd "$(dirname "$0")"
mkdir -p build/icon
mifconv build/icon/tgnative_aif.mif /c32,8 data/tgnative-icon.svg >/dev/null
