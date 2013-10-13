#!/bin/sh

hg_version=$(/usr/local/bin/hg log -r . --template '{latesttag}-{latesttagdistance}-{node|short}')

build_time=$(date)

echo "hg_version=${hg_version}"
echo "build_time=${build_time}"

info_plist="${BUILT_PRODUCTS_DIR}/${INFOPLIST_PATH}"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion '${hg_version}'" "${info_plist}"
/usr/libexec/PlistBuddy -c "Add :BuildTime date '${build_time}'" "${info_plist}"

exit 0
