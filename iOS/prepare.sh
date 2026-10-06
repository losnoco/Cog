#!/bin/sh
#
# Readies a checkout for the iOS build: the iOS libraries
# (Scripts/build-ios-libraries.sh, unless already built) and the changes the
# iOS build needs in submodules, which live here as patches, applied in
# place (submodules point at upstream commits).

set -eu

BASEDIR=$(cd "$(dirname "$0")/.." && pwd)

# Submodule path, patch
PATCHES="Plugins/MIDI/MIDI/spessasynth_core iOS/patches/spessasynth_core.patch"

set -- ${PATCHES}
while [ $# -ge 2 ]; do
	submodule=$1
	patch=$2
	shift 2
	if git -C "${BASEDIR}/${submodule}" apply --reverse --check "${BASEDIR}/${patch}" 2>/dev/null; then
		echo "Already applied: ${patch}"
	else
		git -C "${BASEDIR}/${submodule}" apply "${BASEDIR}/${patch}"
		echo "Applied: ${patch}"
	fi
done

if [ ! -d "${BASEDIR}/ThirdParty/ffmpeg/ios/libavcodec.xcframework" ]; then
	"${BASEDIR}/Scripts/build-ios-libraries.sh"
fi
