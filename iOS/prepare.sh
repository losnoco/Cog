#!/bin/sh
#
# Readies a checkout for the iOS build: builds the iOS libraries
# (Scripts/build-ios-libraries.sh) unless they are already in place.

set -eu

BASEDIR=$(cd "$(dirname "$0")/.." && pwd)

if [ ! -d "${BASEDIR}/ThirdParty/ffmpeg/ios/libavcodec.xcframework" ]; then
	"${BASEDIR}/Scripts/build-ios-libraries.sh"
fi
