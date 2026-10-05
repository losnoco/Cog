#!/bin/sh
#
# Builds the third-party libraries Cog uses as static xcframeworks for iOS
# devices and the iOS Simulator (arm64, and x86_64 for Intel Macs), into
# ThirdParty/<library>/ios/. The macOS libraries in libraries.tar.xz are
# untouched; this only adds the iOS builds, which are not committed.
#
# Sources are the releases the macOS builds use (see each library's
# README.md), checked against their hashes. Headers stay those in
# ThirdParty/<library>/include, shared with macOS.
#
# Usage: build-ios-libraries.sh [library ...]   (default: all of them)
#
# Needs cmake (brew install cmake).

set -eu

BASEDIR=$(cd "$(dirname "$0")/.." && pwd)
THIRDPARTY="${BASEDIR}/ThirdParty"
IOS_MIN=18.0
WORK=$(mktemp -d -t cog-ios-libraries)
trap 'rm -rf "${WORK}"' EXIT
JOBS=$(sysctl -n hw.ncpu)

# Platforms, as "sdk:architectures"
PLATFORMS="iphoneos:arm64 iphonesimulator:arm64,x86_64"

# Each platform's install prefix, shared by the libraries built for it, so
# later ones find earlier ones (FLAC finds ogg).
prefix() {
	echo "${WORK}/prefix-$1"
}

fetch() {
	url=$1
	file=$2
	sha=$3
	curl -sSL -o "${WORK}/${file}" "${url}"
	echo "${sha}  ${WORK}/${file}" | shasum -a 256 -c - >/dev/null || {
		echo "Checksum mismatch for ${file}" >&2
		exit 1
	}
	tar -C "${WORK}" -xf "${WORK}/${file}"
}

# cmake_build <source directory> [cmake options ...]: builds and installs into
# each platform's prefix.
cmake_build() {
	source=$1
	shift
	for platform in ${PLATFORMS}; do
		sdk=${platform%%:*}
		archs=$(echo "${platform#*:}" | tr , ';')
		build="${WORK}/build-$(basename "${source}")-${sdk}"
		cmake -Wno-author -S "${source}" -B "${build}" -G Ninja \
			-DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
			-DCMAKE_SYSTEM_NAME=iOS \
			-DCMAKE_OSX_SYSROOT="${sdk}" \
			-DCMAKE_OSX_ARCHITECTURES="${archs}" \
			-DCMAKE_OSX_DEPLOYMENT_TARGET="${IOS_MIN}" \
			-DCMAKE_BUILD_TYPE=Release \
			-DBUILD_SHARED_LIBS=OFF \
			-DCMAKE_PREFIX_PATH="$(prefix "${sdk}")" \
			-DCMAKE_FIND_ROOT_PATH="$(prefix "${sdk}")" \
			-DCMAKE_INSTALL_PREFIX="$(prefix "${sdk}")" \
			"$@" >/dev/null
		cmake --build "${build}" --target install -j "${JOBS}" >/dev/null
	done
}

# xcframework <ThirdParty directory> <library file name> [headers]: packs
# the library from each platform's prefix into ThirdParty/<directory>/ios/,
# with a headers directory if given (otherwise the headers are those in
# ThirdParty/<directory>/include).
xcframework() {
	directory=$1
	library=$2
	headers=${3:-}
	args=""
	for platform in ${PLATFORMS}; do
		args="${args} -library $(prefix "${platform%%:*}")/lib/${library}"
		[ -n "${headers}" ] && args="${args} -headers ${headers}"
	done
	rm -rf "${THIRDPARTY}/${directory}/ios/${library%.a}.xcframework"
	mkdir -p "${THIRDPARTY}/${directory}/ios"
	xcodebuild -create-xcframework ${args} -output "${THIRDPARTY}/${directory}/ios/${library%.a}.xcframework" >/dev/null
	echo "Built ThirdParty/${directory}/ios/${library%.a}.xcframework"
}

# MARK: - Libraries

build_soxr() {
	fetch https://downloads.sourceforge.net/project/soxr/soxr-0.1.3-Source.tar.xz soxr.tar.xz \
		b111c15fdc8c029989330ff559184198c161100a59312f5dc19ddeb9b5a15889
	curl -sSL -o "${WORK}/soxr-arm64.patch" https://raw.githubusercontent.com/Homebrew/formula-patches/76868b36263be42440501d3692fd3a258f507d82/libsoxr/arm64_defines.patch
	echo "9df5737a21b9ce70cc136c302e195fad9f9f6c14418566ad021f14bb34bb022c  ${WORK}/soxr-arm64.patch" | shasum -a 256 -c - >/dev/null
	(cd "${WORK}/soxr-0.1.3-Source" && patch -p1 -s < "${WORK}/soxr-arm64.patch")
	cmake_build "${WORK}/soxr-0.1.3-Source" -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF -DWITH_OPENMP=OFF -DWITH_LSR_BINDINGS=OFF
	xcframework soxr libsoxr.a
}

# The single-file build: the vDSP FFT and the built-in resampler, as on
# macOS, without Rubber Band's own threads (Cog runs it in real-time mode).
build_rubberband() {
	fetch https://breakfastquay.com/files/releases/rubberband-4.0.0.tar.bz2 rubberband.tar.bz2 \
		af050313ee63bc18b35b2e064e5dce05b276aaf6d1aa2b8a82ced1fe2f8028e9
	for platform in ${PLATFORMS}; do
		sdk=${platform%%:*}
		build="${WORK}/build-rubberband-${sdk}"
		mkdir -p "${build}" "$(prefix "${sdk}")/lib"
		suffix=""
		[ "${sdk}" = iphonesimulator ] && suffix=-simulator
		slices=""
		for arch in $(echo "${platform#*:}" | tr , ' '); do
			xcrun --sdk "${sdk}" clang++ -target "${arch}-apple-ios${IOS_MIN}${suffix}" -std=c++14 -O3 -fvisibility-inlines-hidden \
				-c "${WORK}/rubberband-4.0.0/single/RubberBandSingle.cpp" -o "${build}/RubberBandSingle-${arch}.o"
			xcrun --sdk "${sdk}" libtool -static -o "${build}/librubberband-${arch}.a" "${build}/RubberBandSingle-${arch}.o"
			slices="${slices} ${build}/librubberband-${arch}.a"
		done
		lipo -create ${slices} -output "$(prefix "${sdk}")/lib/librubberband.a"
	done
	xcframework rubberband librubberband.a
}

build_ogg() {
	fetch https://downloads.xiph.org/releases/ogg/libogg-1.3.6.tar.xz libogg.tar.xz \
		5c8253428e181840cd20d41f3ca16557a9cc04bad4a3d04cce84808677fa1061
	cmake_build "${WORK}/libogg-1.3.6" -DBUILD_TESTING=OFF -DINSTALL_DOCS=OFF
	xcframework ogg libogg.a
}

build_flac() {
	[ -f "$(prefix iphoneos)/lib/libogg.a" ] || build_ogg
	fetch https://downloads.xiph.org/releases/flac/flac-1.5.0.tar.xz flac.tar.xz \
		f2c1c76592a82ffff8413ba3c4a1299b6c7ab06c734dee03fd88630485c2b920
	cmake_build "${WORK}/flac-1.5.0" -DBUILD_PROGRAMS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF -DBUILD_DOCS=OFF \
		-DINSTALL_MANPAGES=OFF -DBUILD_CXXLIBS=OFF -DWITH_OGG=ON -DWITH_STACK_PROTECTOR=OFF
	xcframework flac libFLAC.a
}

# TagLib's headers go in the xcframework as tag/, so the plugins' framework
# style includes (<tag/fileref.h>) work as they do with macOS's tag.framework.
build_taglib() {
	fetch https://taglib.org/releases/taglib-2.2.1.tar.gz taglib.tar.gz \
		7e76b5299dcef427c486bffe455098470c8da91cf3ccb9ea804893df57389b5e
	cmake_build "${WORK}/taglib-2.2.1" -DBUILD_BINDINGS=OFF -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF
	mkdir -p "${WORK}/taglib-headers/tag"
	cp "$(prefix iphoneos)"/include/taglib/* "${WORK}/taglib-headers/tag/"
	xcframework taglib libtag.a "${WORK}/taglib-headers"
}

# With Cog's fixed-point patch, as for macOS (fdk-aac/README.md).
build_fdkaac() {
	fetch https://downloads.sourceforge.net/project/opencore-amr/fdk-aac/fdk-aac-2.0.2.tar.gz fdk-aac.tar.gz \
		c9e8630cf9d433f3cead74906a1520d2223f89bcd3fa9254861017440b8eb22f
	(cd "${WORK}/fdk-aac-2.0.2" && patch -p1 -s < "${THIRDPARTY}/fdk-aac/patches/fdk_fixedpoint.patch")
	cmake_build "${WORK}/fdk-aac-2.0.2" -DBUILD_PROGRAMS=OFF
	xcframework fdk-aac libfdk-aac.a
}

# FFmpeg 8.1.2 with Cog's patches and the components of
# Scripts/ffmpeg-build-arm64.sh, less hardware video decoding, which an
# audio player has no use for (and which would link VideoToolbox).
FFMPEG_LIBRARIES="libavcodec libavformat libavutil libswresample"

build_ffmpeg() {
	[ -f "$(prefix iphoneos)/lib/libfdk-aac.a" ] || build_fdkaac
	fetch https://ffmpeg.org/releases/ffmpeg-8.1.2.tar.xz ffmpeg.tar.xz \
		464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c
	for patch in "${THIRDPARTY}"/ffmpeg/patches/*.patch; do
		(cd "${WORK}/ffmpeg-8.1.2" && patch -p1 -s < "${patch}")
	done

	PCM_CODECS=pcm_alaw,pcm_bluray,pcm_dvd,pcm_f16le,pcm_f24le,pcm_f32be,pcm_f32le,pcm_f64be,pcm_f64le,pcm_lxf,pcm_mulaw,pcm_s8,pcm_s8_planar,pcm_s16be,pcm_s16be_planar,pcm_s16le,pcm_s16le_planar,pcm_s24be,pcm_s24daud,pcm_s24le,pcm_s24le_planar,pcm_s32be,pcm_s32le,pcm_s32le_planar,pcm_s64be,pcm_s64le,pcm_sga,pcm_u8pcm_u16be,pcm_u16le,pcm_u24be,pcm_u24le,pcm_u32be,pcm_u32le,pcm_vidc
	ADPCM_CODECS=adpcm_4xm,adpcm_adx,adpcm_afx,adpcm_agm,adpcm_aica,adpcm_argo,adpcm_ct,adpcm_dtk,adpcm_ea,adpcm_ea_maxis_xa,adpcm_ea_r1,adpcm_ea_r2,adpcm_ea_r3,adpcm_ea_xa,adpcm_g722,adpcm_g726,adpcm_g726le,adpcm_ima_amv,adpcm_ima_alp,adpcm_ima_apc,adpcm_ima_apm,adpcm_ima_cunning,adpcm_ima_dat4,adppcm_ima_dk3,adpcm_ima_dk4,adpcm_ima_ea_eacs,adpcm_ima_ea_sead,adpcm_ima_iss,adpcm_ima_moflex,adpcm_ima_mtf,adpcm_ima_oki,adpcm_ima_qt,adpcm_ima_rad,adpcm_ima_ssi,adpcm_ima_smjpeg,adpcm_ima_wav,adpcm_ima_ws,adpcm_ms,adpcm_mtaf,adpcm_psx,adpcm_sbpro_2,adpcm_sbpro_3,adpcm_sbpro_4,adpcm_swf,adpcm_thp,adpcm_thp_le,adpcm_vima,adpcm_xa,adpcm_yamaha,adpcm_zork

	for platform in ${PLATFORMS}; do
		sdk=${platform%%:*}
		sysroot=$(xcrun --sdk "${sdk}" --show-sdk-path)
		suffix=""
		[ "${sdk}" = iphonesimulator ] && suffix=-simulator
		installs=""
		for arch in $(echo "${platform#*:}" | tr , ' '); do
			build="${WORK}/build-ffmpeg-${sdk}-${arch}"
			mkdir -p "${build}"
			target="${arch}-apple-ios${IOS_MIN}${suffix}"
			if [ "${arch}" = arm64 ]; then
				archflags="--arch=aarch64 --enable-neon"
			else
				# No nasm for a simulator slice only Intel Macs run.
				archflags="--arch=x86_64 --disable-x86asm"
			fi
			(cd "${build}" && PKG_CONFIG_LIBDIR="$(prefix "${sdk}")/lib/pkgconfig" "${WORK}/ffmpeg-8.1.2/configure" \
				--enable-cross-compile --target-os=darwin ${archflags} \
				--cc="$(xcrun --sdk "${sdk}" -f clang)" --cxx="$(xcrun --sdk "${sdk}" -f clang++)" --sysroot="${sysroot}" \
				--extra-cflags="-target ${target} -I$(prefix "${sdk}")/include" \
				--extra-ldflags="-target ${target} -L$(prefix "${sdk}")/lib" \
				--extra-libs="-lc++" --pkg-config-flags=--static \
				--enable-static --disable-shared --prefix="${build}/install" \
				--enable-nonfree --enable-libfdk-aac \
				--enable-pic --enable-gpl --disable-doc --disable-programs \
				--disable-avdevice --disable-avfilter \
				--disable-swscale --enable-network --disable-swscale-alpha --disable-vdpau \
				--disable-dxva2 --disable-everything --disable-videotoolbox \
				--enable-swresample \
				--enable-parser=ac3,mpegaudio,xma,vorbis,opus \
				--enable-demuxer=mpegts,mpegtsraw,ac3,asf,xwma,mov,oma,ogg,tak,dsf,wav,w64,aac,dts,dtshd,eac3,mp3,bink,flac,msf,xmv,caf,ape,smacker,spdif,mpc,mpc8,rm,matroska,tta,dff,wsd,iff,aiff,truehd,${PCM_CODECS},${ADPCM_CODECS} \
				--enable-decoder=ac3,ac3_t,eac3,wmapro,wmav1,wmav2,wmavoice,wmalossless,xma1,xma2,dca,tak,dsd_lsbf,dsd_lsbf_planar,dsd_mbf,dsd_msbf_planar,aac,libfdk_aac,atrac3,atrac3p,mp3float,mp2float,mp1float,bink,binkaudio_dct,binkaudio_rdft,flac,vorbis,ape,smackaud,opus,mpc7,mpc8,alac,cook,tta,truehd,${PCM_CODECS},${ADPCM_CODECS} \
				--disable-parser=mpeg4video,h263 \
				--disable-decoder=mpeg2video,h263,h264,mpeg1video,mpeg2video,mpeg4,hevc,vp9 \
				--disable-version3 \
				--disable-xlib >"${build}/configure.log" 2>&1) || {
				tail -20 "${build}/configure.log" >&2
				exit 1
			}
			make -C "${build}" -j "${JOBS}" install >"${build}/make.log" 2>&1 || {
				tail -30 "${build}/make.log" >&2
				exit 1
			}
			installs="${installs} ${build}/install"
		done
		mkdir -p "$(prefix "${sdk}")/lib"
		for library in ${FFMPEG_LIBRARIES}; do
			lipo -create $(for install in ${installs}; do echo "${install}/lib/${library}.a"; done) -output "$(prefix "${sdk}")/lib/${library}.a"
		done
	done
	for library in ${FFMPEG_LIBRARIES}; do
		xcframework ffmpeg "${library}.a"
	done
}

LIBRARIES=${*:-"soxr rubberband ogg flac taglib fdkaac ffmpeg"}
for library in ${LIBRARIES}; do
	"build_${library}"
done
