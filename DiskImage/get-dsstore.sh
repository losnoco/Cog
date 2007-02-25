#!/bin/sh

#This script creates a .ds_store from a template dmg.

TEMPLATE_DMG=$1
DSSTORE=$2

TEMP_DIR=temp

bunzip2 -k ${TEMPLATE_DMG}.bz2

mkdir -p ${TEMP_DIR}

hdiutil attach "${TEMPLATE_DMG}" -noautoopen -quiet -mountpoint "${TEMP_DIR}"

cp ${TEMP_DIR}/.DS_Store ${DSSTORE}

DMG_DEV=`hdiutil info | grep "${TEMP_DIR}" | grep "Apple_HFS" | awk '{print $1}'`
hdiutil detach ${DMG_DEV} -quiet -force

rm -r ${TEMP_DIR}

rm ${TEMPLATE_DMG}

