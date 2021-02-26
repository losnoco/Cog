#!/bin/sh

set -e

rm -rf generated
cd sidplayfp

autoreconf -vfi
./configure --without-gcrypt
make -s -j$(sysctl -n hw.ncpu)

{ git status --porcelain --ignored | awk '{ print $2 }';\
    git submodule foreach\
      'git status --ignored --porcelain |\awk "{ print \"$sm_path/\"\$2 }"'; } |\
        grep -E 'bin|h$' |\
        xargs -I % rsync -R % ../generated

git clean -ffdx > /dev/null
git submodule foreach git clean -ffdx > /dev/null

cd -
