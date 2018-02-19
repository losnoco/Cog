#!/usr/bin/env bash
cd "${0%/*}"
cd ../..
AFL_HARDEN=1 CONFIG=afl make clean all EXAMPLES=0 TEST=0 OPENMPT123=0 NO_VORBIS=1 NO_VORBISFILE=1 USE_MINIMP3=1
