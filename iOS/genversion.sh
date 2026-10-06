#!/bin/sh
#
# Versions the iOS app as Scripts/genversion.sh versions the macOS one: by
# how many commits it is past where it started. The macOS app counts from
# the k54 tag; the iOS app counts from its first commit, "iOS: The Cog app
# for iPhone and iPad", which is version 1. Writes the version, and the
# same GitHash, GitVersion and BuildTime keys, into the built Info.plist.
#

set -eu

FIRST_COMMIT=700560d4f3b97c82ede0acd8ddee0b13efb4d344

PlistBuddy="/usr/libexec/PlistBuddy"
plist="${TARGET_BUILD_DIR}/${INFOPLIST_PATH}"
repo=$(git -C "${SRCROOT}" rev-parse --show-toplevel)

GIT_HASH=$(git -C "$repo" show -s --format=%H)

if GIT_RELEASE_NUMBER=$(git -C "$repo" rev-list --count "${FIRST_COMMIT}^..HEAD" 2>/dev/null); then
	:
else
	# A shallow clone, as CI's, has no history to count.
	echo "warning: ${FIRST_COMMIT} is not in this clone; the app is version 0"
	GIT_RELEASE_NUMBER=0
fi

# As `git describe` spells it for the macOS app: "3785-gc3fb35c96".
GIT_RELEASE_VERSION="${GIT_RELEASE_NUMBER}-g$(git -C "$repo" rev-parse --short HEAD)"

# Local time, in the one spelling PlistBuddy reads as a date.
BUILD_TIME=$(date '+%a %b %d %H:%M:%S %Y')

echo "RELEASE_VERSION: $GIT_RELEASE_VERSION"

# Set, or add where the plist does not have the key yet.
put() {
	"$PlistBuddy" -c "Set :$1 $3" "$plist" 2>/dev/null || "$PlistBuddy" -c "Add :$1 $2 $3" "$plist"
}

put CFBundleVersion string "$GIT_RELEASE_NUMBER"
put CFBundleShortVersionString string "$GIT_RELEASE_NUMBER"
put GitHash string "$GIT_HASH"
put GitVersion string "$GIT_RELEASE_VERSION"
put BuildTime date "$BUILD_TIME"
