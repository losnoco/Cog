#!/bin/sh

BASEDIR=$(dirname "$0")
SRCROOT="$BASEDIR/.."

xcodebuild -workspace "$SRCROOT/Cog.xcodeproj/project.xcworkspace" -scheme Cog -configuration Release -derivedDataPath "$BASEDIR/build"
