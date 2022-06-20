Built with the Homebrew defaults, sort of. Plus a patch to fix weird LAME/Xing
headers:

```
patch -Np1 -i libmad_ancdata.patch
touch NEWS
touch AUTHORS
touch ChangeLog
autoreconf -fiv
./configure --disable-debugging --enable-fpm=64bit
make -j8 CFLAGS="-Os -arch x86_64 -arch arm64 -mmacosx-version-min=10.12" LDFLAGS="-arch x86_64 -arch arm64 -mmacosx-version-min=10.12"
```

Version 0.15.1b was used, with the minor patch listed above.
