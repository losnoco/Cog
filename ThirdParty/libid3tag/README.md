Built with the Arch Linux defaults, sort of:

```
patch -Np1 -i 10_utf16.diff
patch -Np1 -i 11_unknown_encoding.diff
patch -Np0 -i CVE-2008-2109.patch
patch -Np1 -i libid3tag-gperf.patch
rm compat.c frametype.c

touch NEWS
touch AUTHORS
touch ChangeLog

autoreconf -fiv
./configure
make -j8 CFLAGS="-Os -arch x86_64 -arch arm64 -mmacosx-version-min=10.12" LDFLAGS="-arch x86_64 -arch arm64 -mmacosx-version-min=10.12"
```

Version 0.15.1b was used, with Arch Linux patches. I also had to tweak
the compat.c and frametype.c to change the function definitions to match
the gperf patch used above.
