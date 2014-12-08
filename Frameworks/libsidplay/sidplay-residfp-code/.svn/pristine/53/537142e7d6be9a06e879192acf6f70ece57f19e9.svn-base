#!/bin/bash

set -e

MYDIR=$(dirname $0)
cd "$MYDIR/../libsidplayfp"

# compile
CXXFLAGS="-O2 -ffast-math -fPIC" \
DEB_BUILD_OPTIONS="debug nostrip" \
MAKEFLAGS=-j3 \
fakeroot debian/rules binary-arch

# install
cd ..
sudo dpkg -i *.deb
