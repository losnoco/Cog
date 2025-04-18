#!/bin/sh

BASEDIR=$(dirname "$0")

git=$(command -v git)

REPO_ROOT_PATH=$("$git" rev-parse --show-toplevel)

"$git" -C "$REPO_ROOT_PATH" fetch --unshallow --tags

"${BASEDIR}/../Scripts/genversion.sh"
