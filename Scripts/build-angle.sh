#!/bin/sh
# Builds ANGLE, Metal backend only, as universal libEGL.dylib and
# libGLESv2.dylib for macOS 12 and later, into ThirdParty/angle. The
# projectM pane draws through it (see ThirdParty/angle/README.md).
#
# Like the FFmpeg scripts, this runs by hand when ANGLE is updated, and its
# output goes into libraries.tar.xz. It needs Google's depot_tools and a
# full ANGLE checkout (several GB, with Chromium's build files and clang),
# which it fetches into $ANGLE_WORK (default ~/Projects/angle-build).
#
# Usage: Scripts/build-angle.sh [work dir]

set -e

ANGLE_COMMIT=c053bf85793bbb83016b1196d04e5df3594b9bcc
MACOS_DEPLOYMENT_TARGET=12.0

info() {
	printf '\033[1;32m==>\033[0m %s\n' "$*"
}

BASEDIR=$(cd "$(dirname "$0")/.." && pwd)
OUT_DIR="$BASEDIR/ThirdParty/angle"
WORK=${1:-${ANGLE_WORK:-$HOME/Projects/angle-build}}
mkdir -p "$WORK"
WORK=$(cd "$WORK" && pwd)

if [ ! -d "$WORK/depot_tools" ]; then
	info "Fetching depot_tools"
	git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git "$WORK/depot_tools"
fi
export PATH="$WORK/depot_tools:$PATH"
# Its first-run setup, which the self-update turned off below would skip.
"$WORK/depot_tools/ensure_bootstrap"
export DEPOT_TOOLS_UPDATE=0

if [ ! -d "$WORK/angle/.git" ]; then
	info "Cloning ANGLE"
	git clone https://chromium.googlesource.com/angle/angle "$WORK/angle"
fi
cd "$WORK/angle"
info "Checking out ANGLE $ANGLE_COMMIT"
git fetch origin "$ANGLE_COMMIT"
git checkout --quiet "$ANGLE_COMMIT"
if [ ! -f .gclient ]; then
	python3 scripts/bootstrap.py
fi
info "Syncing ANGLE's dependencies"
gclient sync --no-history --shallow

for cpu in arm64 x64; do
	out="out/cog-mac-$cpu"
	info "Building ANGLE for $cpu"
	mkdir -p "$out"
	cat > "$out/args.gn" << EOF
target_os = "mac"
target_cpu = "$cpu"
mac_deployment_target = "$MACOS_DEPLOYMENT_TARGET"
is_debug = false
is_component_build = false
symbol_level = 0
treat_warnings_as_errors = false
# Xcode's linker: Chromium's lld at this commit can't read the macOS 27 SDK's
# .tbd files (they list an arm64e.x1-macos target it doesn't know).
use_lld = false
angle_enable_metal = true
angle_enable_gl = false
angle_enable_gl_desktop_backend = false
angle_enable_vulkan = false
angle_enable_swiftshader = false
angle_enable_null = false
angle_enable_wgpu = false
angle_build_tests = false
angle_has_frame_capture = false
EOF
	gn gen "$out"
	autoninja -C "$out" libEGL libGLESv2
done

info "Joining the architectures"
mkdir -p "$OUT_DIR/lib"
for lib in libEGL libGLESv2; do
	lipo -create "out/cog-mac-arm64/$lib.dylib" "out/cog-mac-x64/$lib.dylib" -output "$OUT_DIR/lib/$lib.dylib"
	install_name_tool -id "@rpath/$lib.dylib" "$OUT_DIR/lib/$lib.dylib" 2> /dev/null
	codesign -f -s - "$OUT_DIR/lib/$lib.dylib"
done

info "Copying the public headers and license"
rm -rf "$OUT_DIR/include"
mkdir -p "$OUT_DIR/include"
for dir in EGL GLES2 GLES3 KHR; do
	mkdir -p "$OUT_DIR/include/$dir"
	cp include/$dir/*.h "$OUT_DIR/include/$dir/"
done
cp LICENSE "$OUT_DIR/LICENSE"

for lib in libEGL libGLESv2; do
	lipo -info "$OUT_DIR/lib/$lib.dylib"
	for arch in arm64 x86_64; do
		echo "  $arch: macOS $(vtool -arch $arch -show-build "$OUT_DIR/lib/$lib.dylib" | awk '/minos/ { print $2 }')"
	done
done
info "Done. Repack libraries.tar.xz with angle/lib."
