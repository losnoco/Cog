These were built from libavif and libaom, from the following repositories:

https://github.com/link-u/libaom - at revision: v2.0.0-586-gdee179da7
https://github.com/AOMediaCodec/libavif - at revision: v0.9.3-43-g380d91a

libaom was built to two separate targets, with the following options:

```
arch -x86_64 /usr/local/bin/cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.12" -DCONFIG_AV1_ENCODER=0 -DCONFIG_WEBM_IO=0
make -j8
```

```
cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" -DCONFIG_AV1_ENCODER=0 -DCONFIG_WEBM_IO=0 -DCONFIG_RUNTIME_CPU_DETECT=0
make -j8
```

And then they were merged using lipo.

libavif was built with the following:

```
arch -x86_64 /usr/local/bin/cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.12" -DAVIF_CODEC_AOM=ON -DBUILD_SHARED_LIBS=OFF
make -j8
```

```
cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" -DAVIF_CODEC_AOM=ON -DBUILD_SHARED_LIBS=OFF
make -j8
```

And then they were merged with lipo.

The AVIF decoder contained in the source directory was based on code from
the following repository:

https://github.com/dreampiggy/AVIFQuickLook


I updated the code to work with newer libavif.
