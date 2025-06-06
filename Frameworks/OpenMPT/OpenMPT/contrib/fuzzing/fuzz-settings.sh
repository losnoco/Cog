#!/usr/bin/env bash

# Input data for fuzzer
# If you run the fuzzer for the first time, specify a directory with some input
# files for the fuzzer, e.g.
#     FUZZING_INPUT="-i /home/foo/testcases/"
# If you want to continue fuzzing using the previous findings, use:
#     FUZZING_INPUT=-i-
FUZZING_INPUT=-i-

# Directory to place temporary fuzzing data into
FUZZING_TEMPDIR=~/libopenmpt-fuzzing-temp
# Directory to store permanent fuzzing data (e.g. found crashes) into
FUZZING_FINDINGS_DIR=~/libopenmpt-fuzzing
# Fuzzer timeout in ms, + = don't abort on timeout
FUZZING_TIMEOUT=5000+
# Path to afl-fuzz binary
FUZZING_AFL_DIR=afl

# AFL specific envs
AFL_TRY_AFFINITY=1
AFL_CMPLOG_ONLY_NEW=1
AFL_NO_WARN_INSTABILITY=1
AFL_FAST_CAL=1
AFL_IMPORT_FIRST=1
AFL_DISABLE_TRIM=1
AFL_IGNORE_SEED_PROBLEMS=1
