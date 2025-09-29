Build with CMake, using the following options:

```
cmake -B build.x86 -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" -DBUILD_SHARED_LIBS=ON
cmake -B build.arm -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" -DBUILD_SHARED_LIBS=ON
```

Then using lipo to merge them, then using install_name_tool to clean up the overly verbose version number SONAMEs.

Version v1.3.7-20-g43bbff01 was used from the following repository:

https://github.com/xiph/vorbis.git
