#!/bin/sh

BASEDIR=$(dirname "$0")

PREVIOUS_CONFIGURATION=$(cat "${BASEDIR}/../ThirdParty/libraries.updated")

if ( [[ "${CONFIGURATION}" = "Debug" ]] && [[ "${PREVIOUS_CONFIGURATION}" != "Debug" ]] ) || ( [[ "${CONFIGURATION}" != "Debug" ]] && [[ "${PREVIOUS_CONFIGURATION}" = "Debug" ]] ); then
	rm -f "${BASEDIR}/../ThirdParty/libraries.updated"
fi

until [ ! -f "${BASEDIR}/../ThirdParty/libraries.extracting" ]
do
	sleep 5
done

if [ \( ! -f "${BASEDIR}/../ThirdParty/libraries.updated" \) -o \( "${BASEDIR}/../ThirdParty/libraries.updated" -ot "${BASEDIR}/../ThirdParty/libraries.tar.xz" \) ]; then
	touch "${BASEDIR}/../ThirdParty/libraries.extracting"
	tar -C "${BASEDIR}/../ThirdParty" -xvf "${BASEDIR}/../ThirdParty/libraries.tar.xz"
	if [[ "${CONFIGURATION}" = "Debug" ]]; then
		tar -C "${BASEDIR}/../ThirdParty" -xvf "${BASEDIR}/../ThirdParty/libraries-debug-overlay.tar.xz"
	fi
	echo "${CONFIGURATION}" > "${BASEDIR}/../ThirdParty/libraries.updated"
	rm -f "${BASEDIR}/../ThirdParty/libraries.extracting"
fi

