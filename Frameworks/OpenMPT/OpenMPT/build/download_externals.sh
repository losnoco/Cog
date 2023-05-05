#!/usr/bin/env bash

# stop on error
set -e

# normalize current directory to project root
cd build 2>&1 > /dev/null || true
cd ..

function download_and_unpack_tar () {
 set -e
 MPT_GET_DESTDIR="$1"
 MPT_GET_URL="$2"
 MPT_GET_FILE="$3"
 MPT_GET_SUBDIR="$4"
 if [ ! -f "$3" ]; then
  wget "$2" -O "$3"
 fi
 cd include
  if [ -d "$1" ]; then
   rm -rf "$1"
  fi
  if [ "$4" = "." ]; then
   mkdir "$1"
   cd "$1"
    tar xvaf "../../$3"
   cd ..
  else
   tar xvaf "../$3"
   if [ ! "$4" = "$1" ]; then
    mv "$4" "$1"
   fi
  fi
 cd ..
 return 0
}

function download_and_unpack_zip () {
 set -e
 MPT_GET_DESTDIR="$1"
 MPT_GET_URL="$2"
 MPT_GET_FILE="$3"
 MPT_GET_SUBDIR="$4"
 if [ ! -f "$3" ]; then
  wget "$2" -O "$3"
 fi
 cd include
  if [ -d "$1" ]; then
   rm -rf "$1"
  fi
  if [ "$4" = "." ]; then
   mkdir "$1"
   cd "$1"
    unzip "../../$3"
   cd ..
  else
   unzip "../$3"
   if [ ! "$4" = "$1" ]; then
    mv "$4" "$1"
   fi
  fi
 cd ..
 return 0
}

function download_and_unpack_7z () {
 set -e
 MPT_GET_DESTDIR="$1"
 MPT_GET_URL="$2"
 MPT_GET_FILE="$3"
 MPT_GET_SUBDIR="$4"
 if [ ! -f "$3" ]; then
  wget "$2" -O "$3"
 fi
 cd include
  if [ -d "$1" ]; then
   rm -rf "$1"
  fi
  if [ "$4" = "." ]; then
   mkdir "$1"
   cd "$1"
    7z x "../../$3"
   cd ..
  else
   7z x "../$3"
   if [ ! "$4" = "$1" ]; then
    mv "$4" "$1"
   fi
  fi
 cd ..
 return 0
}

function download () {
 set -e
 MPT_GET_URL="$1"
 MPT_GET_FILE="$2"
 if [ ! -f "$2" ]; then
  wget "$1" -O "$2"
 fi
 return 0
}

if [ ! -d "build/externals" ]; then
 mkdir build/externals
fi
if [ ! -d "build/tools" ]; then
 mkdir build/tools
fi



download_and_unpack_zip "allegro42" "https://lib.openmpt.org/files/libopenmpt/contrib/allegro/allegro-4.2.3.1-hg.8+r8500.zip" "build/externals/allegro-4.2.3.1-hg.8+r8500.zip" "."
download_and_unpack_zip "cwsdpmi"   "https://lib.openmpt.org/files/libopenmpt/contrib/djgpp/cwsdpmi/csdpmi7b.zip" "build/externals/csdpmi7b.zip" "."
download                            "https://lib.openmpt.org/files/libopenmpt/contrib/djgpp/cwsdpmi/csdpmi7s.zip" "build/externals/csdpmi7s.zip"
#download_and_unpack_zip "cwsdpmi"   "https://djgpp.mirror.garr.it/current/v2misc/csdpmi7b.zip" "build/externals/csdpmi7b.zip" "."
#download                            "https://djgpp.mirror.garr.it/current/v2misc/csdpmi7s.zip" "build/externals/csdpmi7s.zip"
download_and_unpack_7z  "winamp" "https://web.archive.org/web/20131217072017if_/http://download.nullsoft.com/winamp/plugin-dev/WA5.55_SDK.exe" "build/externals/WA5.55_SDK.exe" "."
ln -s OUT.H include/winamp/Winamp/out.h
download_and_unpack_zip "xmplay" "https://www.un4seen.com/files/xmp-sdk.zip"                     "build/externals/xmp-sdk.zip"    "."
