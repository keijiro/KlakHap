#!/bin/sh
set -x
make ARCH=arm64 clean
make ARCH=x86_64 clean
rm KlakHap.bundle
