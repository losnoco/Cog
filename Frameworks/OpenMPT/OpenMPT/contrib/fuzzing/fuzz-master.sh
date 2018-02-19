#!/usr/bin/env bash
cd "${0%/*}"
. ./fuzz-settings.sh

# Create tmpfs for storing temporary fuzzing data
mkdir $FUZZING_TEMPDIR
sudo mount -t tmpfs -o size=200M none $FUZZING_TEMPDIR
rm -rf $FUZZING_TEMPDIR/bin
mkdir $FUZZING_TEMPDIR/bin
cp ../../bin/* $FUZZING_TEMPDIR/bin/
rm $FUZZING_TEMPDIR/bin/libopenmpt.so
cp ../../bin/libopenmpt.so.1 $FUZZING_TEMPDIR/bin/libopenmpt.so
cp ../../bin/libopenmpt.so.1 $FUZZING_TEMPDIR/bin/libopenmpt.so.1

LD_LIBRARY_PATH=$FUZZING_TEMPDIR/bin $AFL_BIN -f $FUZZING_TEMPDIR/infile01 -x all_formats.dict -t $FUZZING_TIMEOUT $FUZZING_INPUT -o $FUZZING_FINDINGS_DIR -M fuzzer01 $FUZZING_TEMPDIR/bin/fuzz $FUZZING_TEMPDIR/infile01
