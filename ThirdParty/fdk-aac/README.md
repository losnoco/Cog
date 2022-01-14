This was built with my modified FDK-AAC from:

https://gitlab.com/kode54/fdk-aac.git

Which was only slightly modified from upstream from here:

https://github.com/mstorsjo/fdk-aac.git

Using the following commandline:

env CFLAGS="-arch x86_64 -arch arm64 -fPIC -isysroot $(xcode-select -p)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -mmacosx-version-min=10.12" CXXFLAGS="-arch x86_64 -arch arm64 -fPIC -isysroot $(xcode-select -p)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -mmacosx-version-min=10.12" LDFLAGS="-arch x86_64 -arch arm64 -mmacosx-version-min=10.12" ./configure --prefix=$(COG_REPO_DIR)/ThirdParty/fdk-aac
make -j8
make install

Then I update the id path in the resulting dylib:

install_name_tool -id @rpath/libfdk-aac.2.dylib libfdk-aac.2.dylib
