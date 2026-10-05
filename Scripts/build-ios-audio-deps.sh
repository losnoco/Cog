#!/bin/sh
#
# Builds the libraries CogAudio links (soxr, Rubber Band) as static
# xcframeworks for iOS devices and the iOS Simulator, into
# ThirdParty/<library>/ios/. The macOS dylibs in libraries.tar.xz are
# untouched; this only adds the iOS slices.
#
# Sources are the same releases the macOS builds use (see each library's
# README.md), checked against the same hashes.
#
# Needs cmake (brew install cmake).

set -eu

BASEDIR=$(cd "$(dirname "$0")/.." && pwd)
THIRDPARTY="${BASEDIR}/ThirdParty"
IOS_MIN=18.0
WORK=$(mktemp -d -t cog-ios-deps)
trap 'rm -rf "${WORK}"' EXIT
JOBS=$(sysctl -n hw.ncpu)

fetch() {
	url=$1
	file=$2
	sha=$3
	curl -sSL -o "${WORK}/${file}" "${url}"
	echo "${sha}  ${WORK}/${file}" | shasum -a 256 -c - >/dev/null || {
		echo "Checksum mismatch for ${file}" >&2
		exit 1
	}
}

# Platforms, as "sdk:architectures" (the simulator also for Intel Macs)
PLATFORMS="iphoneos:arm64 iphonesimulator:arm64,x86_64"

# MARK: soxr

fetch https://downloads.sourceforge.net/project/soxr/soxr-0.1.3-Source.tar.xz soxr.tar.xz \
	b111c15fdc8c029989330ff559184198c161100a59312f5dc19ddeb9b5a15889
fetch https://raw.githubusercontent.com/Homebrew/formula-patches/76868b36263be42440501d3692fd3a258f507d82/libsoxr/arm64_defines.patch soxr-arm64.patch \
	9df5737a21b9ce70cc136c302e195fad9f9f6c14418566ad021f14bb34bb022c
tar -C "${WORK}" -xf "${WORK}/soxr.tar.xz"
(cd "${WORK}/soxr-0.1.3-Source" && patch -p1 -s < "${WORK}/soxr-arm64.patch")

SOXR_ARGS=""
for platform in ${PLATFORMS}; do
	sdk=${platform%%:*}
	archs=$(echo "${platform#*:}" | tr , ';')
	build="${WORK}/soxr-${sdk}"
	cmake -Wno-author -S "${WORK}/soxr-0.1.3-Source" -B "${build}" -G Ninja \
		-DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
		-DCMAKE_SYSTEM_NAME=iOS \
		-DCMAKE_OSX_SYSROOT="${sdk}" \
		-DCMAKE_OSX_ARCHITECTURES="${archs}" \
		-DCMAKE_OSX_DEPLOYMENT_TARGET="${IOS_MIN}" \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SHARED_LIBS=OFF \
		-DBUILD_TESTS=OFF \
		-DBUILD_EXAMPLES=OFF \
		-DWITH_OPENMP=OFF \
		-DWITH_LSR_BINDINGS=OFF \
		-DCMAKE_INSTALL_PREFIX="${build}/install" >/dev/null
	cmake --build "${build}" --target install -j "${JOBS}" >/dev/null
	SOXR_ARGS="${SOXR_ARGS} -library ${build}/install/lib/libsoxr.a"
done
rm -rf "${THIRDPARTY}/soxr/ios"
mkdir -p "${THIRDPARTY}/soxr/ios"
# Headers come from ThirdParty/soxr/include, shared with macOS.
xcodebuild -create-xcframework ${SOXR_ARGS} -output "${THIRDPARTY}/soxr/ios/libsoxr.xcframework" >/dev/null

# MARK: Rubber Band

# The single-file build: the vDSP FFT and the built-in resampler, as on macOS,
# without Rubber Band's own threads (Cog runs it in real-time mode).
fetch https://breakfastquay.com/files/releases/rubberband-4.0.0.tar.bz2 rubberband.tar.bz2 \
	af050313ee63bc18b35b2e064e5dce05b276aaf6d1aa2b8a82ced1fe2f8028e9
tar -C "${WORK}" -xf "${WORK}/rubberband.tar.bz2"

RUBBERBAND_ARGS=""
for platform in ${PLATFORMS}; do
	sdk=${platform%%:*}
	build="${WORK}/rubberband-${sdk}"
	mkdir -p "${build}"
	objects=""
	for arch in $(echo "${platform#*:}" | tr , ' '); do
		suffix=""
		[ "${sdk}" = iphonesimulator ] && suffix=-simulator
		xcrun --sdk "${sdk}" clang++ -target "${arch}-apple-ios${IOS_MIN}${suffix}" -std=c++14 -O3 -fvisibility-inlines-hidden \
			-c "${WORK}/rubberband-4.0.0/single/RubberBandSingle.cpp" -o "${build}/RubberBandSingle-${arch}.o"
		objects="${objects} ${build}/RubberBandSingle-${arch}.o"
	done
	for object in ${objects}; do
		xcrun --sdk "${sdk}" libtool -static -o "${object%.o}.a" "${object}"
	done
	lipo -create $(for object in ${objects}; do echo "${object%.o}.a"; done) -output "${build}/librubberband.a"
	RUBBERBAND_ARGS="${RUBBERBAND_ARGS} -library ${build}/librubberband.a"
done
rm -rf "${THIRDPARTY}/rubberband/ios"
mkdir -p "${THIRDPARTY}/rubberband/ios"
xcodebuild -create-xcframework ${RUBBERBAND_ARGS} -output "${THIRDPARTY}/rubberband/ios/librubberband.xcframework" >/dev/null

echo "Built iOS xcframeworks for soxr and Rubber Band"
