#!/bin/sh

plugins=( CoreAudio MonkeysAudio Musepack Flac Shorten TagLib Vorbis WavPack MAD )

for plugin in "${plugins[@]}"
do
	cd Plugins/$plugin
	xcodebuild -alltargets -configuration Release
	cd ../..
done
