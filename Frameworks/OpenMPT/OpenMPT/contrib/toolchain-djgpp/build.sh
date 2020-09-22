#!/usr/bin/env bash

# apt install \
# autoconf \
# autoconf-archive \
# autotools \
# bison \
# bzip2 \
# curl \
# dos2unix \
# flex \
# g++ \
# gcc \
# gzip \
# make \
# make \
# pkg-config \
# tar \
# texinfo \
# unzip \
# wget \
# xz-utils \
# zlib1g-dev \
#

set -e

MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd "${MY_DIR}"

rm -rf build
rm -rf prefix

mkdir build
mkdir prefix

cd build
git clone https://github.com/jwt27/build-gcc build-gcc
cd build-gcc
git checkout 5d5747cc6da983e493a4258ff2009c338b2d6f60   # 2020-05-09

CFLAGS_FOR_TARGET="-O2" CXXFLAGS_FOR_TARGET="-O2" ./build-djgpp.sh --batch --prefix="${MY_DIR}/prefix" all
