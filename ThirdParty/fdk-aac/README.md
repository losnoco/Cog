This package was built with version 2.0.2, with the included patches applied.

https://downloads.sourceforge.net/project/opencore-amr/fdk-aac/fdk-aac-2.0.2.tar.gz

Using the following commandline:

env CFLAGS="-Os -arch x86_64 -arch arm64 -fPIC -isysroot $(xcode-select -p)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -mmacosx-version-min=10.12" CXXFLAGS="-Os -arch x86_64 -arch arm64 -fPIC -isysroot $(xcode-select -p)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -mmacosx-version-min=10.12" LDFLAGS="-arch x86_64 -arch arm64 -mmacosx-version-min=10.12" ./configure --prefix=$(COG_REPO_DIR)/ThirdParty/fdk-aac
make -j8
make install

Then I update the id path in the resulting dylib:

install_name_tool -id @rpath/libfdk-aac.2.dylib libfdk-aac.2.dylib
