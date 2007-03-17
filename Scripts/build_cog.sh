#!/bin/sh

./Scripts/build_dependencies.sh

xcodebuild -alltargets -configuration Release

