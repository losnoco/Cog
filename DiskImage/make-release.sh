#!/bin/sh

# Create a read-only disk image of the contents of a folder
#
# Usage: make-release <version>

set -e;

VERSION=0.06

APP_NAME=Cog
DEST_FOLDER=Release
APPLESCRIPT=dmg-cog.scpt

TEMPLATE_DMG=template.dmg
DSSTORE=cog.dsstore

APP_PATH=../build/Release/Cog.app
LICENSE_PATH=../COPYING
CHANGES_PATH=../ChangeLog
README_PATH=../README

#Make dest
echo "Creating destination..."
mkdir ${DEST_FOLDER}

echo "Copying app..."
cp -R ${APP_PATH} ${DEST_FOLDER}

echo "Copying misc files..."
cp ${LICENSE_PATH} ${DEST_FOLDER}/License.txt
cp ${CHANGES_PATH} ${DEST_FOLDER}/Changes.txt
cp ${README_PATH} ${DEST_FOLDER}/Readme.txt

echo "Creating Applications alias..."
ln -s /Applications ${DEST_FOLDER}/Applications


echo "Mounting template..."
TEMP_DIR=temp
BACKGROUND=${TEMP_DIR}/.background/background.png

bunzip2 -k ${TEMPLATE_DMG}.bz2

mkdir -p ${TEMP_DIR}

hdiutil attach "${TEMPLATE_DMG}" -noautoopen -quiet -mountpoint "${TEMP_DIR}"

echo "Copying .DS_Store"
cp ${TEMP_DIR}/.DS_Store ${DSSTORE}

echo "Launching applescript..."
osascript ${APPLESCRIPT} ${DEST_FOLDER} ${TEMP_DIR}

echo "Creating image..."
./make-diskimage.sh ${APP_NAME}-${VERSION}.dmg ${DEST_FOLDER} "${APP_NAME} ${VERSION}" -null- ${DSSTORE} ${BACKGROUND}

echo "Unmounting template..."
DMG_DEV=`hdiutil info | grep "${TEMP_DIR}" | grep "Apple_HFS" | awk '{print $1}'`
hdiutil detach ${DMG_DEV} -quiet -force

rm -r ${TEMP_DIR}

rm ${TEMPLATE_DMG}
rm ${DSSTORE}

rm -r ${DEST_FOLDER}
