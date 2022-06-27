#!/bin/sh

BASEDIR=$(dirname "$0")

"${BASEDIR}/../Scripts/extract_libraries.sh"

"${BASEDIR}/../Scripts/genversion.sh"
