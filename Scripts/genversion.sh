#!/bin/sh

git_version=$(/usr/bin/git describe --tags)

build_time=$(date)

echo "git_version=${git_version}"
echo "build_time=${build_time}"

info_plist="${BUILT_PRODUCTS_DIR}/${INFOPLIST_PATH}"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion '${git_version}'" "${info_plist}"
/usr/libexec/PlistBuddy -c "Add :BuildTime date '${build_time}'" "${info_plist}"

exit 0
