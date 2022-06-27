#!/bin/sh

BASEDIR=$(dirname "$0")

until [ ! -f "${BASEDIR}/../ThirdParty/libraries.extracting" ]
do
	sleep 5
done

if [ \( ! -f "${BASEDIR}/../ThirdParty/libraries.updated" \) -o \( "${BASEDIR}/../ThirdParty/libraries.updated" -ot "${BASEDIR}/../ThirdParty/libraries.tar.xz" \) ]; then
	touch "${BASEDIR}/../ThirdParty/libraries.extracting"
	tar -C "${BASEDIR}/../ThirdParty" -xvf "${BASEDIR}/../ThirdParty/libraries.tar.xz"
	touch "${BASEDIR}/../ThirdParty/libraries.updated"
	rm -f "${BASEDIR}/../ThirdParty/libraries.extracting"
fi

