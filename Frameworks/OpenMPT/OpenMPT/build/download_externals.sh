#!/usr/bin/env bash

# stop on error
set -e

# normalize current directory to project root
cd build 2>&1 > /dev/null || true
cd ..

function download () {
 set -e
 MPT_GET_FILE_NAME="$1"
 MPT_GET_FILE_SIZE="$2"
 MPT_GET_FILE_CHECKSUM="$3"
 MPT_GET_URLS="$4"
 echo "Checking '$MPT_GET_FILE_NAME' ..."
 if [ -f "$MPT_GET_FILE_NAME" ]; then
  FILE_SIZE=$(find "$MPT_GET_FILE_NAME" -printf '%s')
  if [ ! "x$FILE_SIZE" = "x$MPT_GET_FILE_SIZE" ]; then
   echo "$FILE_SIZE does not match expected file size $MPT_GET_FILE_SIZE. Redownloading."
   rm -f "$MPT_GET_FILE_NAME"
  fi
 fi
 if [ -f "$MPT_GET_FILE_NAME" ]; then
  FILE_CHECKSUM=$(sha512sum "$MPT_GET_FILE_NAME" | awk '{print $1;}')
  if [ ! "x$FILE_CHECKSUM" = "x$MPT_GET_FILE_CHECKSUM" ]; then
   echo "$FILE_CHECKSUM does not match expected file checksum $MPT_GET_FILE_CHECKSUM. Redownloading."
   rm -f "$MPT_GET_FILE_NAME"
  fi
 fi
 for URL in $MPT_GET_URLS; do
  if [ ! -f "$MPT_GET_FILE_NAME" ]; then
   echo "Downloading '$MPT_GET_FILE_NAME' from '$URL' ..."
   curl -o "$MPT_GET_FILE_NAME" "$URL"
   echo "Verifying '$URL' ..."
   if [ -f "$MPT_GET_FILE_NAME" ]; then
    FILE_SIZE=$(find "$MPT_GET_FILE_NAME" -printf '%s')
    if [ ! "x$FILE_SIZE" = "x$MPT_GET_FILE_SIZE" ]; then
     echo "$FILE_SIZE does not match expected file size $MPT_GET_FILE_SIZE."
     rm -f "$MPT_GET_FILE_NAME"
    fi
   fi
   if [ -f "$MPT_GET_FILE_NAME" ]; then
    FILE_CHECKSUM=$(sha512sum "$MPT_GET_FILE_NAME" | awk '{print $1;}')
    if [ ! "x$FILE_CHECKSUM" = "x$MPT_GET_FILE_CHECKSUM" ]; then
     echo "$FILE_CHECKSUM does not match expected file checksum $MPT_GET_FILE_CHECKSUM."  
     rm -f "$MPT_GET_FILE_NAME"
    fi
   fi
  fi
 done
 if [ ! -f "$MPT_GET_FILE_NAME" ]; then
  echo "Failed to download '$MPT_GET_FILE_NAME'."
  return 1
 fi
 return 0
}

function unpack () {
 set -e
 MPT_GET_DESTDIR="$1"
 MPT_GET_FILE="$2"
 MPT_GET_SUBDIR="$3"
 echo "Extracting '$MPT_GET_DESTDIR' from '$MPT_GET_FILE:$MPT_GET_SUBDIR' ..."
 EXTENSION="${MPT_GET_FILE##*.}"
 if [ -d "$MPT_GET_DESTDIR" ]; then
  rm -rf "$MPT_GET_DESTDIR"
 fi
 mkdir "$MPT_GET_DESTDIR"
 case "$EXTENSION" in
  tar)
   tar -xvaf "$MPT_GET_FILE" -C "$MPT_GET_DESTDIR"
   ;;
  zip)
   unzip -d "$MPT_GET_DESTDIR" "$MPT_GET_FILE"
   ;;
  7z)
   7z x -o"$MPT_GET_DESTDIR" "$MPT_GET_FILE"
   ;;
  exe)
   7z x -o"$MPT_GET_DESTDIR" "$MPT_GET_FILE"
   ;;
 esac
 if [ ! "$MPT_GET_SUBDIR" = "." ]; then
  mv "$MPT_GET_DESTDIR" "$MPT_GET_DESTDIR.tmp"
  mv "$MPT_GET_DESTDIR.tmp/$MPT_GET_SUBDIR" "$MPT_GET_DESTDIR"
 fi
 return 0
}

if [ ! -d "build/externals" ]; then
 mkdir build/externals
fi
if [ ! -d "build/tools" ]; then
 mkdir build/tools
fi

download "build/externals/allegro-4.2.3.1-hg.8+r8500.zip" 3872466 46cd8d4d7138b795dbc66994e953d0abc578c6d3c00615e3580237458529d33d7ad9d269a9778918d4b3719d75750d5cca74ff6bf38ad357a766472799ee9e7b "https://lib.openmpt.org/files/libopenmpt/contrib/allegro/allegro-4.2.3.1-hg.8+r8500.zip"
download "build/externals/csdpmi7b.zip"                     71339 58c24691d27cead1cec92d334af551f37a3ba31de25a687d99399c28d822ec9f6ffccc9332bfce35e65dae4dd1210b54e54b223a4de17f5adcb11e2da004b834 "https://lib.openmpt.org/files/libopenmpt/contrib/djgpp/cwsdpmi/csdpmi7b.zip https://djgpp.mirror.garr.it/current/v2misc/csdpmi7b.zip"
download "build/externals/csdpmi7s.zip"                     89872 ea5652d31850d8eb0d15a919de0b51849f58efea0d16ad2aa4687fac4abd223d0ca34a2d1b616b02fafe84651dbef3e506df9262cfb399eb6d9909bffc89bfd3 "https://lib.openmpt.org/files/libopenmpt/contrib/djgpp/cwsdpmi/csdpmi7s.zip https://djgpp.mirror.garr.it/current/v2misc/csdpmi7s.zip"
download "build/externals/WA5.55_SDK.exe"                  336166 394375db8a16bf155b5de9376f6290488ab339e503dbdfdc4e2f5bede967799e625c559cca363bc988324f1a8e86e5fd28a9f697422abd7bb3dcde4a766607b5 "http://download.nullsoft.com/winamp/plugin-dev/WA5.55_SDK.exe https://web.archive.org/web/20131217072017id_/http://download.nullsoft.com/winamp/plugin-dev/WA5.55_SDK.exe"
download "build/externals/xmp-sdk.zip"                     322744 62c442d656d4bb380360368a0f5f01da11b4ed54333d7f54f875a9a5ec390b08921e00bd08e62cd7a0a5fe642e3377023f20a950cc2a42898ff4cda9ab88fc91 "https://www.un4seen.com/files/xmp-sdk.zip"

unpack "include/allegro42" "build/externals/allegro-4.2.3.1-hg.8+r8500.zip" "."
unpack "include/cwsdpmi"   "build/externals/csdpmi7b.zip"                   "."
unpack "include/winamp"    "build/externals/WA5.55_SDK.exe"                 "."
unpack "include/xmplay"    "build/externals/xmp-sdk.zip"                    "."

ln -s OUT.H include/winamp/Winamp/out.h

