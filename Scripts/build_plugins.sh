#!/bin/sh

plugins=( CoreAudio MonkeysAudio Musepack Flac Shorten TagLib Vorbis WavPack MAD FileSource HTTPSource)

for plugin in "${plugins[@]}"
do
	cd Plugins/$plugin
	xcodebuild -alltargets -configuration Release
	cd ../..
done
