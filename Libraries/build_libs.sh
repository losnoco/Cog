for directory in *
do
	if [ -d $directory ]; then
		cd $directory
		xcodebuild -configuration Release
		cd ..
	fi
done
