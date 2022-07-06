#!/bin/sh

BASEDIR=$(dirname "$0")

git=$(which git)

"${BASEDIR}/../Scripts/extract_libraries.sh"

"$git" -C "${BASEDIR}/.." fetch --tags

"${BASEDIR}/../Scripts/genversion.sh"
