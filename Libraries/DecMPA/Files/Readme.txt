*** DecMPA v0.4.1 - Simple MPEG Audio Decoder ***

DecMPA is a small library for decoding MPEG Audio (layer 1,2 and 3). You may
use it under the terms of the GNU Lesser General Public License
(see License.txt).

DecMPA uses some parts of project MPEGLib v0.4.1 by Martin Vogt
(see http://mpeglib.sourceforge.net/) which itself uses modified SPlay
decoding routines (see http://splay.sourceforge.net).
DecMPA v0.3.0 and higher also uses code from Lame's HIP decoding
engine (see http://lame.sourceforge.net/).

Essentially DecMPA is a mixture of "raw" decoding routines from other projects
(with a few bug fixes and extensions), with a new interface coded on top of
it. The goal is to provide a powerful MPEG Audio decoding library that can
be easily integrated into applications.

The project homepage can be found at
http://decmpa.sourceforge.net


************************************************************
** IMPORTANT NOTE for users of versions older than v0.4.0 **
************************************************************

In version 0.4.0 the API has changed a little, in particular the parameters
you pass to the DecMPA_CreateXXX functions. The output type is not passed
directly to Create anymore, but can now be set using DecMPA_SetParam.

Also note that the structure DecMPA_Callbacks now contains an additional
pointer to a "GetPosition" function that HAS to be provided. Be sure to
initialize it!

Please read the documentation for additional information.

These changes allow a greater flexibility when adding new features and will
hopefully enable future releases to be fully downward compatible to earlier
versions.


** Documentation

Documentation is available in the subdirectory 'doc'.


** Compiling

If you want to compile DecMPA yourself, project files and make files for
some target systems and compilers are located in build/SYSTEM, where
SYSTEM is:

vc6:         Windows, Visual C++ 6.0
gcc-mingw32: Windows, GCC Mingw32
gcc-cygwin:  Windows, GCC Cygwin
gcc-linux:   Linux, GCC
gcc-digux:   Digital Unix, GCC

For gcc systems, just open a shell/command prompt, got to the appropriate
build/SYSTEM directory and run "make" (or possibly "gmake" if the default
make for your system is not GNU make).

DecMPA can use two different decoding engines: HIP and SPlay. HIP is the
default, but if you want to use SPlay instead you can pass "ENGINE=splay"
as a parameter to you call to make.

More information about the make files can be found in build/MakeGoals.txt

Generated libraries, shared objects and import libraries are placed in
lib/SYSTEM.
Generated DLLs and example executables are placed in bin/SYSTEM.


** Usage

Include include/decmpa.h in your code files and link with the
appropriate library or import library for your target system from lib/SYSTEM.

All library files except those for Windows/Visual C++ have to be compiled
first (see Compiling).

Note that the LGPL might require you to use the dll/shared object instead
of the static library if your application is not released under the GPL
(see License.txt).


** Examples

Example applications are located in the "examples" subdirectory.
The makefiles and project files for the examples are found exactly where
the library make files are (see "Compiling").

For GNU make files, the example make files are integrated into the
normal make files. They can be executed by calling GNU Make with the goal
"example_NAME" where NAME is the directory name of the example. For example
"make example_decodefile" builds the decodefile example application.
More information about the make files can be found in build/MakeGoals.txt


** Portability

DecMPA has been compiled under a few systems, including Windows and Linux
(see "Compiling"). It will probably compile well on other UNIXes with the
Linux or Digital Unix makefiles, but that has not been tested yet.

If you successfully compiled DecMPA on other systems, I would appreciate if
you could drop me a note (and maybe contribute your makefiles or project files
- see below).



** Suggestions, fixes, extensions, feedback

If you have any suggestions, small extension or bug fixes that you would like to
be incorporated into this library, send them to hazard_hd@users.sourceforge.net

If you send me any of your code, you must agree that it will be released under
LGPL.

If you want to continuously help with the development, please send a short resume
describing your qualifications and experience to hazard_hd@users.sourceforge.net.
If I think you will "fit in" I'll add you to the developer list, thus allowing
you direct access to the project CVS.

I would also like to hear from you if you find this library useful and decide
to use it in one of your projects.



Copyright 2002 Hauke Duden
hazard_hd@users.sourceforge.net

