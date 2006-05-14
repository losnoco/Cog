libs=( MAC MPCDec Ogg FLAC Shorten TagLib Vorbis WavPack )

for lib in "${libs[@]}"
do
	cd $lib
	xcodebuild -alltargets -configuration Release
	cd ..
done
