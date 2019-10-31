#!/usr/bin/env bash
cd "${0%/*}"

AFL_VERSION="$(wget --quiet -O - "https://api.github.com/repos/google/AFL/releases/latest" | grep -Po '"tag_name": "\K.*?(?=")')"
AFL_FILENAME="$AFL_VERSION.tar.gz"
AFL_URL="https://github.com/google/AFL/archive/$AFL_FILENAME"

rm $AFL_FILENAME
wget $AFL_URL || exit
tar -xzvf $AFL_FILENAME
rm $AFL_FILENAME
cd AFL-*
make || exit
cd llvm_mode
# may need to prepend LLVM_CONFIG=/usr/bin/llvm-config-3.8 or similar, depending on the system
make || exit
cd ../libdislocator
make || exit
cd ../..
rm -rf afl
mv AFL-* afl