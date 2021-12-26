#!/usr/bin/env bash
cd "${0%/*}"
. ./fuzz-settings.sh

#export AFL_PRELOAD=$AFL_DIR/libdislocator.so
LD_LIBRARY_PATH=$FUZZING_TEMPDIR/bin $AFL_DIR/afl-fuzz -p coe -f $FUZZING_TEMPDIR/infile02 -x all_formats.dict -t $FUZZING_TIMEOUT $FUZZING_INPUT -o $FUZZING_FINDINGS_DIR -S fuzzer02 $FUZZING_TEMPDIR/bin/fuzz $FUZZING_TEMPDIR/infile02
