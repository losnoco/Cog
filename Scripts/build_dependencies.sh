#!/bin/sh

../Scripts/build_frameworks.sh
../Scripts/build_preferences.sh
../Scripts/build_plugins.sh

cd ../Audio
xcodebuild -alltargets -configuration Release
cd ..

