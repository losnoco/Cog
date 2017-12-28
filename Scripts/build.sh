#!/bin/sh

pushd $(dirname $0)
BASE=`pwd -P`
popd

sign=true

while getopts ":hn" option; do
  case $option in
    h) echo "usage: $0 [-h] [-n]"; exit ;;
    n) sign=false ;;
    ?) echo "error: option -$OPTARG is not implemented"; exit ;;
  esac
done

SIGNARGS=""

if [ "$sign" = true ] ; then
  SIGNARGS=('CODE_SIGN_IDENTITY=""' 'CODE_SIGNING_REQUIRED=NO')
fi

BUILDPRODUCTS="$BASE"/build/Build/Products/Release

xcodebuild -quiet -workspace "$BASE"/../Cog.xcodeproj/project.xcworkspace -scheme Cog -configuration Release -derivedDataPath "$BASE"/build ${SIGNARGS[*]}

