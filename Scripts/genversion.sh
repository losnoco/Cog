#!/bin/sh

git=$(command -v git)

sed=$(command -v sed)

PlistBuddy="/usr/libexec/PlistBuddy"

REPO_ROOT_PATH=$("$git" rev-parse --show-toplevel)

GIT_HASH=$("$git" -C "$REPO_ROOT_PATH" show -s --format=%H)

GIT_NUMBER_OF_COMMITS=$("$git" -C "$REPO_ROOT_PATH" rev-list HEAD --count)

GIT_RELEASE_VERSION=$("$git" -C "$REPO_ROOT_PATH" describe --tags --always | "$sed" -e 's/k54-//')

GIT_RELEASE_NUMBER=${GIT_RELEASE_VERSION%-*}

MACOS_PLIST_PATH="$REPO_ROOT_PATH/Info.plist"

BUILD_TIME=$(date)

echo "GIT: $git"

echo "NUMBER_OF_COMMITS: $GIT_NUMBER_OF_COMMITS"

echo "RELEASE_VERSION: $GIT_RELEASE_VERSION"

for plist in "$MACOS_PLIST_PATH"; do

	plist_template=${plist}.template

	if [ -f "$plist_template" ]; then

		echo "COPY: $plist_template"
		echo "TO: $plist"

		cp -f "$plist_template" "$plist"

		"$PlistBuddy" -c "Set :CFBundleVersion $GIT_RELEASE_NUMBER" "$plist"

		"$PlistBuddy" -c "Set :CFBundleShortVersionString $GIT_RELEASE_NUMBER" "$plist"

		"$PlistBuddy" -c "Add :GitHash string $GIT_HASH" "$plist"

		"$PlistBuddy" -c "Add :GitVersion string $GIT_RELEASE_VERSION" "$plist"

		"$PlistBuddy" -c "Add :BuildTime date $BUILD_TIME" "$plist"

	fi

done
