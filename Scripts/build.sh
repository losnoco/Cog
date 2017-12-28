#!/bin/sh

pushd $(dirname $0)
BASE=`pwd -P`
popd

BUILDPRODUCTS="$BASE"/build/Build/Products/Release

xcodebuild -quiet -workspace "$BASE"/../Cog.xcodeproj/project.xcworkspace -scheme Cog -configuration Release -derivedDataPath "$BASE"/build

