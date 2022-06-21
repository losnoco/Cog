#!/bin/sh

git_version=$(/usr/bin/git describe --tags | sed -e 's/k54-//')
short_version=${git_version%-*}

build_time=$(date)

echo "git_version=${git_version}"
echo "short_version=${short_version}"
echo "build_time=${build_time}"

info_plist="${BUILT_PRODUCTS_DIR}/${INFOPLIST_PATH}"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion '${short_version}'" "${info_plist}"
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString '${short_version}'" "${info_plist}"
/usr/libexec/PlistBuddy -c "Add :GitVersion string '${git_version}'" "${info_plist}"
/usr/libexec/PlistBuddy -c "Add :BuildTime date '${build_time}'" "${info_plist}"

exit 0
