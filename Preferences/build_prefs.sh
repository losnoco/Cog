#!/bin/sh

prefs=( General )

for pref in "${prefs[@]}"
do
	cd $pref
	xcodebuild -alltargets -configuration Release
	cd ..
done
