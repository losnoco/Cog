#!/bin/sh

FUCKING_THING_TO_SIGN=$1

FUCKING_SIGNED=0

while [ $FUCKING_SIGNED -eq 0 ]; do
    codesign -s 'Developer ID Application' --deep --force "$FUCKING_THING_TO_SIGN"
    spctl -a "$FUCKING_THING_TO_SIGN" && FUCKING_SIGNED=1
done
