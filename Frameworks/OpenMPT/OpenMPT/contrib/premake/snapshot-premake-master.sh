#!/usr/bin/env bash

set -e

rm -rf premake.git
git clone --recursive https://github.com/premake/premake-core.git premake.git
cd premake.git
make -f Bootstrap.mak linux
echo | bin/release/premake5 package master source

echo "Done."
