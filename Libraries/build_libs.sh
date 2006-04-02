for directory in *
do
	if [ -d $directory ]; then
		cd $directory
		xcodebuild -configuration Deployment
		cd ..
	fi
done
