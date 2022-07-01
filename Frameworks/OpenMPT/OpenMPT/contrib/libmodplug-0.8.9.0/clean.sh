#!/usr/bin/env bash

set -e

./autogen.sh
./configure
make distclean
