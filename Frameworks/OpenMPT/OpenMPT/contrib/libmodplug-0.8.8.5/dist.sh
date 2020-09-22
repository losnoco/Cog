#!/usr/bin/env bash

set -e

./test.sh

./autogen.sh
./configure
make dist
