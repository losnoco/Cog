#!/usr/bin/env bash
cd "${0%/*}"
. ./fuzz-settings.sh

# Create tmpfs for storing temporary fuzzing data
mkdir $FUZZING_TEMPDIR
sudo mount -t tmpfs -o size=300M none $FUZZING_TEMPDIR
rm -rf $FUZZING_TEMPDIR/bin
mkdir $FUZZING_TEMPDIR/bin
cp -d ../../bin/* $FUZZING_TEMPDIR/bin/

#export AFL_PRELOAD=$AFL_DIR/libdislocator.so
LD_LIBRARY_PATH=$FUZZING_TEMPDIR/bin $AFL_DIR/afl-fuzz -p exploit -f $FUZZING_TEMPDIR/infile01 -x all_formats.dict -t $FUZZING_TIMEOUT $FUZZING_INPUT -o $FUZZING_FINDINGS_DIR -D -M fuzzer01 $FUZZING_TEMPDIR/bin/fuzz $FUZZING_TEMPDIR/infile01
