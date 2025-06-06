#!/usr/bin/env bash
cd "${0%/*}"
. ./fuzz-settings.sh

LD_LIBRARY_PATH=$FUZZING_TEMPDIR/bin $FUZZING_AFL_DIR/afl-fuzz -c0 -l2 -x all_formats.dict -t $FUZZING_TIMEOUT $FUZZING_INPUT -o $FUZZING_FINDINGS_DIR -S fuzzer02 $FUZZING_TEMPDIR/bin/fuzz
