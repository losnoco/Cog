#!/usr/bin/env bash
cd "${0%/*}"
. ./fuzz-settings.sh

unset AFL_DISABLE_TRIM
LD_LIBRARY_PATH=$FUZZING_TEMPDIR/bin $FUZZING_AFL_DIR/afl-fuzz -p exploit -P 300 -a binary -x all_formats.dict -t $FUZZING_TIMEOUT $FUZZING_INPUT -o $FUZZING_FINDINGS_DIR -S fuzzer04 $FUZZING_TEMPDIR/bin/fuzz
