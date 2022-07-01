#!/usr/bin/env bash

set -e

./autogen.sh

./configure
make
make distcheck
make distclean
