#!/bin/sh

hg_version=$(hg log -r . --template '{latesttag}-{latesttagdistance}-{node|short}')

build_time=$(date)

info_plist="${BUILT_PRODUCTS_DIR}/${EXECUTABLE_FOLDER_PATH}/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion '${hg_version}'" "${info_plist}"
/usr/libexec/PlistBuddy -c "Add :BuildTime date '${build_time}'" "${info_plist}"

