#!/bin/sh

libs=( MAC MPCDec Ogg FLAC Shorten TagLib Vorbis WavPack MAD ID3Tag )

for lib in "${libs[@]}"
do
	cd $lib
	xcodebuild -alltargets -configuration Release
	cd ..
done
