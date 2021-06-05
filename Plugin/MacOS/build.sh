#!/bin/sh
set -x

mkdir -p arm64
make ARCH=arm64

mkdir -p x86_64
make ARCH=x86_64

lipo -create -output KlakHap.bundle x86_64/KlakHap.so arm64/KlakHap.so
