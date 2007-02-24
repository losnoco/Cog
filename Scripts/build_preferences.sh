#!/bin/sh

prefs=( General )

for pref in "${prefs[@]}"
do
	cd Preferences/$pref
	xcodebuild -alltargets -configuration Release
	cd ../..
done
