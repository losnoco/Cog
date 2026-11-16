ANGLE, Google's OpenGL ES implementation, built with only its Metal backend. On Apple Silicon the projectM pane draws
through it, so it uses Metal and no deprecated OpenGL. arm64 only: Intel Macs keep desktop OpenGL.

The binaries come from the EdgeFirstAI angle-package release v2.1.28252, ANGLE commit
c053bf85793bbb83016b1196d04e5df3594b9bcc (position 28252, 2026-07-03), which reports
`OpenGL ES 3.0 (ANGLE 2.1.28252 git hash: c053bf85793b)`:

https://github.com/EdgeFirstAI/angle-package/releases/download/v2.1.28252/angle-xcframeworks-v2.1.28252.zip

With the following sha256 hash:

59e4b1f68956c92441cde4dca0e9eb1a835bbccd107cefdd1d3d3d60e27410be

The package's macos-arm64 slices are flat iOS-style frameworks, and its libEGL loads libGLESv2 from its own folder
(`libGLESv2.dylib`), which the two separate frameworks don't satisfy. Cog ships them as plain dylibs side by side in
`Contents/Frameworks` instead:

```
cp EGL.xcframework/macos-arm64/libEGL.framework/libEGL lib/libEGL.dylib
cp GLESv2.xcframework/macos-arm64/libGLESv2.framework/libGLESv2 lib/libGLESv2.dylib
install_name_tool -id @rpath/libEGL.dylib lib/libEGL.dylib
install_name_tool -id @rpath/libGLESv2.dylib lib/libGLESv2.dylib
codesign -f -s - lib/*.dylib
```

Xcode signs them again when it copies them into the app. Both need macOS 13: Cog opens libEGL with `dlopen`
(`Visualization/ProjectMSurfaceANGLE.m`) instead of linking it, and libprojectM, which links libGLESv2, is
weak-linked. The dylibs live in `libraries.tar.xz`.

`include/` holds ANGLE's public headers (EGL, GLES2, GLES3, KHR) from the same commit, which the package leaves out:

```
https://chromium.googlesource.com/angle/angle/+/c053bf85793bbb83016b1196d04e5df3594b9bcc/include/<dir>/<file>?format=TEXT
```

ANGLE is licensed under the BSD 3-Clause license (see `LICENSE`); the Khronos headers carry their own notices.
