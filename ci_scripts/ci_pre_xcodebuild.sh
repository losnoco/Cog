#!/bin/sh

BASEDIR=$(dirname "$0")

git=$(which git)

"$git" -C "${BASEDIR}/.." fetch --tags

"${BASEDIR}/../Scripts/genversion.sh"
