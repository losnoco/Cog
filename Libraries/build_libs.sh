libs=( DecMPA FAAD2 MAC MPCDec Ogg FLAC Shorten SndFile TagLib Vorbis WavPack )

for lib in "${libs[@]}"
do
	cd $lib
	xcodebuild -alltargets -configuration Release
	cd ..
done
