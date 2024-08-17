
README
======


OpenMPT and libopenmpt
======================

This repository contains OpenMPT, a free Windows/Wine-based
[tracker](https://en.wikipedia.org/wiki/Music_tracker) and libopenmpt,
a library to render tracker music (MOD, XM, S3M, IT MPTM and dozens of other
legacy formats) to a PCM audio stream. libopenmpt is directly based on OpenMPT,
offering the same playback quality and format support, and development of the
two happens in parallel.


License
-------

The OpenMPT/libopenmpt project is distributed under the *BSD-3-Clause* License.
See [LICENSE](LICENSE) for the full license text.

Files below the `include/` (external projects) and `contrib/` (related assets
not directly considered integral part of the project) folders may be subject to
other licenses. See the respective subfolders for license information. These
folders are not distributed in all source packages, and in particular they are
not distributed in the Autotools packages.


How to compile
--------------


### OpenMPT

 -  Supported Visual Studio versions:

     -  Visual Studio 2019, and 2022 Community/Professional/Enterprise

        To compile the project, open `build/vsVERSIONwin7/OpenMPT.sln` (VERSION
        being 2019, or 2022) and hit the compile button. Other target systems
        can be found in the `vs2019*`, and `vs2022*` sibling folders.

        Note that you have to build the `PluginBridge` and `PluginBridgeLegacy`
        projects manually for architectures other than the one you are building
        OpenMPT for, as Visual Studio only builds one architecture configuration
        at a time.

        Please note that we do not support building with a later Visual Studio
        installation with an earlier compiler version. This is because, while
        later Visual Studio versions allow installing earlier compilers to be
        available via the later version's environment, in this configuration,
        the earlier compiler will still use the later C and C++ runtime's
        headers and implementation, which significantly increases the matrix of
        possible configurations to test.

     -  Visual Studio 2017 XP targeting toolset

 -  OpenMPT requires the compile host system to be Windows 8.1 (or later) on
    amd64 for VS2019 and VS2017, Windows 10 (or later) on amd64 for VS2022, or
    Windows 11 (or later) ARM64.

 -  In order to build OpenMPT for Windows XP, the Visual Studio 2017 XP 
    targeting toolset as well as the Windows 8.1 SDK need to be installed. The
    SDK is optionally included with Visual Studio 2017, but must be separately
    installed with later Visual Studio versions.

    The Windows 8.1 SDK is available from
    <https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/> or
    directly from
    <https://download.microsoft.com/download/B/0/C/B0C80BA3-8AD6-4958-810B-6882485230B5/standalonesdk/sdksetup.exe>
    .

 -  Microsoft Foundation Classes (MFC) are required to build OpenMPT.


### libopenmpt and openmpt123

See [Dependencies](doc/libopenmpt/dependencies.md) and
[Getting Started](doc/libopenmpt/gettingstarted.md).


Contributing to OpenMPT/libopenmpt
----------------------------------


See [contributing](doc/contributing.md).

