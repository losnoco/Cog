ANGLE, Google's OpenGL ES implementation, built with only its Metal backend. The projectM pane draws through it, so
Cog uses no deprecated OpenGL.

Built by `Scripts/build-angle.sh` from ANGLE commit c053bf85793bbb83016b1196d04e5df3594b9bcc (position 28252,
2026-07-03), which reports `OpenGL ES 3.0 (ANGLE 2.1.28252 git hash: c053bf85793b)`. The script fetches Google's
depot_tools and ANGLE's checkout (several GB) into a work folder outside the repository, builds arm64 and x86_64 for
macOS 12, joins them with lipo, sets the install names to `@rpath/libEGL.dylib` and `@rpath/libGLESv2.dylib`, and
copies the public headers (`include/`: EGL, GLES2, GLES3, KHR) and `LICENSE`. Like the FFmpeg scripts, it runs by
hand when ANGLE is updated: change `ANGLE_COMMIT` there, rerun it, and repack `libraries.tar.xz`, where the two
dylibs live. Rebuild libprojectM against the new libGLESv2 too (see `../projectM/README.md`).

The build arguments are in the script. Two are worth knowing:

- `mac_deployment_target = "12.0"`: ANGLE's default at this commit is 13.0, which Cog doesn't require.
- `use_lld = false`: Chromium's lld at this commit can't read the macOS 27 SDK's `.tbd` files (they list an
  `arm64e.x1-macos` target it doesn't know), so Xcode's linker is used.

The two dylibs ship side by side in `Contents/Frameworks`; libEGL loads libGLESv2 from its own folder. Cog opens
libEGL with `dlopen` (`Visualization/ProjectMSurfaceANGLE.m`), and libprojectM links libGLESv2.

ANGLE is licensed under the BSD 3-Clause license (see `LICENSE`); the Khronos headers carry their own notices.
