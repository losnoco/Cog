Build with CMake, using the following options for x86_64:

```
arch -x86_64 (x86_64 version of cmake) .. -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release|Debug
```

And these for ARM64:

```
cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release|Debug
```

(Release|Debug) means one or the other for the separate release/debug builds

And some minor tweaks with `install_name_tool -id` to make sure that the
resulting libsoxr.0.dylib imported libavutil properly, as it seems to want
to use that for CPU detection on ARM, even though the only feature being
detected is NEON, which is mandatory on the supported Apple Silicon platforms.

Version 0.1.3 was retrieved from:

https://downloads.sourceforge.net/project/soxr/soxr-0.1.3-Source.tar.xz

With the following sha256 hash:

b111c15fdc8c029989330ff559184198c161100a59312f5dc19ddeb9b5a15889


And a patch from Homebrew was applied to fix ARM64 building:

https://raw.githubusercontent.com/Homebrew/formula-patches/76868b36263be42440501d3692fd3a258f507d82/libsoxr/arm64_defines.patch

With the following sha256 hash:

9df5737a21b9ce70cc136c302e195fad9f9f6c14418566ad021f14bb34bb022c
