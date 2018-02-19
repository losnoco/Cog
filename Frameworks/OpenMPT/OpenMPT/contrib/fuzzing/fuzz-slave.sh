#!/usr/bin/env bash
cd "${0%/*}"
. ./fuzz-settings.sh

LD_LIBRARY_PATH=$FUZZING_TEMPDIR/bin $AFL_BIN -f $FUZZING_TEMPDIR/infile02 -x all_formats.dict -t $FUZZING_TIMEOUT $FUZZING_INPUT -o $FUZZING_FINDINGS_DIR -S fuzzer02 $FUZZING_TEMPDIR/bin/fuzz $FUZZING_TEMPDIR/infile02
