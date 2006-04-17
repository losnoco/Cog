for directory in *
do
	if [ -d $directory ]; then
		cd $directory
		xcodebuild -alltargets -configuration Release
		cd ..
	fi
done
