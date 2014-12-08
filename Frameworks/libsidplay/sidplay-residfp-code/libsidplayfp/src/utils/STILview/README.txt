STILVIEW v3.00
==============

Copyright (C) 1998, 2002 by LaLa <LaLa@C64.org>
Copyright (C) 2012 by Leandro Nini <drfiemost@users.sourceforge.net>

URL: http://sourceforge.net/projects/sidplay-residfp/


LICENSE
~~~~~~~~~~~
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


WHAT IS IT?
~~~~~~~~~~~

If you don't have a clue about what STIL is, read the STIL.faq file in the
DOCUMENTS subdirectory of your HVSC.

If you don't even know what HVSC is, head over to http://www.hvsc.c64.org.

STILView (or more precisely, the STIL class written in C++) is intended to be
compiled with the various SID emulators available on many platforms to provide
the capability of showing STIL and BUG information along with the given SID
that is currently being played in the emulator. It requires HVSC v2.6
(post-update #12) or later to function correctly, but it will work with
earlier versions to a limited extent.

Also included in the ZIP is 'stilview.cpp', which demonstrates the current
features of STILView and provides an example on usage for SID emulator
developers, but it also makes STILView a standalone application.


IMPORTANT NOTES
~~~~~~~~~~~~~~~

You should have an ISO C++ Standards compliant compiler to compile this code.

In addition in stildefs.h you should define:

- what character is used on your system as a pathname separator (an attempt is
made to determine the value for SLASH automatically, though),

- whether your system uses strcasecmp/strancasecmp or stricmp/strncimp for
case-insensitive string comparisons.


DEBUG OUTPUT
~~~~~~~~~~~~

If you run STILVIEW with the -d option, it will spew out debugging stuff to
STDERR. This provides helpful output for me to find problems with the code. If
you can include the output acquired in this way in your bug-report, please do
so. See the BUG REPORTS section below.


STILView_vX.XX.zip CONTAINS:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- *.cpp, *.h - The required source files. However, your application has to
  #include only "stil.h", as demonstrated by 'stilview.cpp'.

- Makefile - This should build the command-line version of STILView and all
  the required object files.

- README.txt - This text.

- USAGE.txt - How to use the standalone STILVIEW program (which is the result
  of compiling 'stilview.cpp', ie. the STILVIEW.EXE under WinDOS).

- TODO.txt - Ignore this file. ;)


STILVIEW FEATURES:
~~~~~~~~~~~~~~~~~~

- It's implemented as an object-oriented C++ class.

- Fast searching of both STIL.txt and BUGlist.txt with an internal directory
cache list.

- The last requested entry is internally buffered (cached), so that subsequent
requests on the same entry/directory are significantly faster (no disk I/O
involved at all).

- Strings returned contain newlines as '\n' is defined for the given OS the
code was compiled under. Yet, it doesn't matter what is used as an
end-of-line (EOL) char (or chars) in STIL.txt: (0x0d), (0x0a), (0x0d 0x0a) or
even (0x0a 0x0d) is all properly taken care of. One important note, though:
both STIL.txt and BUGlist.txt have to use the same EOL chars!!! (The actual
EOL used is determined only from STIL.txt!)

- Ability to get the section-global comment for an HVSC directory (even if you
specify the full path+filename!).

- Ability to get the BUG entry for a given tune number (0 = get it for all
subtunes).

- Ability to get the STIL entry for a given tune number.

- Ability to get a specific STIL field for a given tune number.

- Partially compatible with STIL v2.5 or earlier: section-global comments
and BUG entries (if HVSC:/DOCUMENTS/BUGlist.txt exists) are retrieved, and
full STIL entries are retrieved, too, but specific tune number and field
requests are ignored (ie. doesn't matter what you ask for, you'll receive the
full STIL entry as a result).

- If setBaseDir() fails, it will *not* have an impact on the private data
already there. In other words, if you did a previous setBaseDir() that
succeeded, then you do one again that fails, you should still be able to
safely use the STIL class, because it'll have the private data populated from
the first setBaseDir() operation intact.

- Also included is a standalone version of STILView that can be used as a
command-line tool to print STIL entries, but is also good for testing
STILView's capabilities. See USAGE.txt for details.


USAGE:
~~~~~~

I admit, the public interface of the STIL class can be somewhat confusing for
newcomers, so here's the bottom line on how to make good use of it.

In your program, #include "stil.h" and then instantiate a STIL object. Then
ask the user about the location of HVSC, and pass the pathname string entered
by the user to STIL::setBaseDir(). This initializes the internal structures of
the STIL object. (Of course, you should check with STIL::getError() to make
sure this step succeeded, i.e. that STILView found the STIL.txt document and
it was able to read it in.)

Then you have essentially two choices:

A) The procedure to extract everything from STIL relating to a specific tune
   in one specific SID file is:

    1) Retrieve the section-global comment with:
       getGlobalComment()
    2) Retrieve the file-global comment with:
       getEntry(..., tuneNo=0, field=comment)
    3) Retrieve all of the STIL entry for tune #x:
       getEntry(..., tuneNo=x)
       or retrieve one field from the STIL entry of tune #x:
       getEntry(..., tuneNo=x, field=<field>)
    4) Retrieve the BUG entry for tune #x with:
       getBug(..., tuneNo=x)

B) If you would like to extract everything from STIL relating to a specific
   SID file (i.e. including everything related to all of its subtunes), the
   procedure is:

    1) Retrieve the section-global comment with:
       getGlobalComment()
    2) Retrieve all of the STIL entry with:
       getEntry(..., tuneNo=0, field=all)
    3) Retrieve all of the BUG entry with:
       getBug(..., tuneNo=0)

These steps may or may not return something to you, but it is guaranteed that
there will be no duplicate info if they all return something.

See also 'stil.h' or 'USAGE.txt' for details and 'stilview.cpp' for more
practical usage ideas.


BUG REPORTS:
~~~~~~~~~~~~

If for whatever reason you cannot get STILView compiled under your OS,
or it compiles but gives you warnings, or it runs but crashes on you, or it
doesn't behave as expected (please, do read 'stil.h' before complaining,
though!), file a bug with  your *DETAILED* description of the encountered
problem along with the STIL.txt and BUGlist.txt files found in your
HVSC:/DOCUMENTS/ subdirectory at the project webpage:

http://sourceforge.net/projects/sidplay-residfp/


GREETZ TO:
~~~~~~~~~~

The Shark - for HVSC and for his relentless support of STILView.

Michael Schwendt - for giving me excellent feedback and great bug reports that
forced me to completely rewrite STILView v1.0... ;)

Andreas Varga - for giving me great feedback on Mac stuff and for giving me an
idea on how to speed up searching the STIL by indexing the sections (subdirs).

The rest of the HVSC Crew - for giving me many-many ideas!


HISTORY
~~~~~~~

v3.00:
STILview is now part of the libsidplayfp project and is installed
as a separate library. Changes are mostly internal refactorings.
- Replace some internal structures with STL maps and strings.
- The public get*() methods now return a const pointer.

v2.17:
- BUGFIX: The get*() member functions used to bomb when any of them was
  called after setBaseDir() failed the very first time. Frankly, people should
  check the return value of setBaseDir() to prevent this from happening, but
  what the heck - it was an easy fix.
- BUGFIX: The internal buffers (caches) were not flushed in setBaseDir() which
  might've resulted in incorrect entries being returned if the same entry was
  asked for right before and right after setting the baseDir to something
  different with setBaseDir().
- NEW: Minor fixes in I/O stream handling to make the code ISO C++ standards
  (and thus, GCC3) compliant. (Thanks, Michael!)
- NEW: Replaced insecure library functions with more secure ones. Namely,
  replaced strcpy() with strncpy(), strcat() with strncat(), sprintf() with
  snprintf(), and an sscanf() with an atof() call. (Thanks, Andreas!)
- NEW: A slightly different way of #define's is introduced in stildefs.h that
  will hopefully make them easier to define for developers.
- NEW: The Win32 executable version of STILView is no longer available.

v2.16:
- BUGFIX: getEntry(tune=0,field=comment) used to retrieve all of the entry for
  single-tune entries like /Hubbard_Rob/Chicken_Song.sid instead of just
  the file-global comment. This is now fixed, so the behavior of getEntry()
  should be consistent once again.
- NEW: Added a practical usage guideline to the README file.

v2.15:
- NEW: Added support for the new AUTHOR field.

v2.14:
- BUGFIX: If the BUGlist.txt file had no entries in it (which is a perfectly
  valid, normal, in fact, desired situation), STILView refused to continue.
  This is now fixed, and an empty BUGlist.txt will be accepted, too.
- NEW: Restored debug output to go to STDERR again. Screw Microsoft.

v2.13:
- BUGFIX OF BUGFIX: The previous "fix" prevented multitune entries from
  showing up if a specific tune number was asked for. Fixed it (fingers
  crossed).
- NEW: Debug output is now dumped to STDOUT, because I don't know how to dump
  STDERR to a file under MS-DOS/Windows (really stupid).

v2.12:
- BUGFIX: If a COMMENT had the string "(#" in it, STILView didn't always show
  it when a tune-specific entry was asked for. Fixed it.

v2.11:
- BUGFIX: Char arrays are now deleted by delete[] (thanks, Michael!).
- BUGFIX: Version info acquired from a new STIL.txt file is now also updated
  when setBaseDir() is called with a different HVSC base directory. The
  command-line STILView with the "-d -m" options will now test this, too.

v2.10:
All of the changes in this version are strictly internal enhancements.
- NEW: The directory list structures are replaced with dynamically allocated
  linked lists. This gets rid of a previous limitation, which allowed only a max
  number of dirs in STIL.txt before STILView crapped out.
- NEW: Also got rid of the STIL_MAX_DIRS #define, which is not needed any more:
  theoretically, STILView is now able to handle as many dirs present in STIL.txt
  as the available memory in your machine allows.
- NEW: Internal structures for holding absolute pathnames are now dynamically
  allocated, not statically.
- NEW: Also got rid of the STIL_MAX_PATH_SIZE #define, which is not needed any
  more: theoretically, STILView is now able to handle an arbitrary long
  pathname for the absolute path of the HVSC base directory.
- NEW: Increased the STIL_MAX_ENTRY_SIZE #define to 100 lines. Analysis of STIL
  v3.5 shows that the largest STIL entry is /DEMOS/Synth_Sample.sid, which is
  about 3087 bytes large, which is still well within the old max entry size
  limit, but better be safe than sorry. It was easier to do this than to change
  all internal buffers to dynamically allocated ones... (I'm lacking time to rip
  out the guts of the code to change this.)

v2.00:
- BUGFIX: Some for loops used inline declared integers. Moved the declarations
  to the beginning of the functions.
- Please, note that for the get*Entry() methods saying tuneNo != 0, field = all
  will return stuff for single-tune entries only if tuneNo = 1! For all other
  numbers, nothing is returned.
- BUGFIX: In the get*Entry() methods tuneNo = 0, field = comment returned
  nothing if the SID file had more than one tune, but the STIL entry had nothing
  else but a COMMENT in it (ie. there's no "(#x)" tune designation in it). This
  was because STILView assumed that the STIL entry was for a single-tune SID.
  (And asking for tuneNo=1, field=comment/all did return the COMMENT!) Fixed it
  so that if a STIL entry has nothing but a single COMMENT in it, tuneNo=0,
  field=comment will always return it, but tuneNo != 0, field=<anything> will
  not.
  Essentially, single-tune entries that have nothing but a COMMENT field in
  them, that COMMENT is assumed to be a file-global comment from now on.
  Here's a full and complete explanation of how the two parameters work now
  (it is exactly the same as in the beta, except that now this bug is fixed):
    - tuneNo = 0, field = all : all of the STIL entry is returned.
    - tuneNo = 0, field = comment : the file-global comment is returned.
      For single-tune entries that have nothing but a COMMENT field in them,
      this returns that COMMENT. For single-tune entries that have other fields
      in them, this returns nothing.
    - tuneNo = 0, field = <other> : INVALID! (NULL is returned)
    - tuneNo != 0, field = all : all fields of the STIL entry for the given
      tune number are returned. For single-tune entries that have nothing but a
      COMMENT in them, this returns nothing. For single-tune entries that have
      other fields in them, this returns the whole entry, but only if tuneNo=1
      (otherwise it returns nothing).
    - tuneNo != 0, field = <other> : the specific field of the specific
      tune number is returned. If the tune number doesn't exist (eg. if
      tuneNo=2 for single-tune entries, or if tuneNo=2 when there's no
      STIL entry for tune #2 in a multitune entry), returns NULL. For
      single-tune entries that have nothing but a COMMENT in them, this returns
      nothing.

    The procedure to extract everything from STIL relating to a specific tune
    in one specific SID file is unchanged:
       a) Retrieve the section-global comment with:
          get*GlobalComment()
       b) Retrieve the file-global comment with:
          get*Entry(..., tuneNo=0, field=comment)
       c) Retrieve all of the STIL entry for tune #x:
          get*Entry(..., tuneNo=x)
          or retrieve one field from the STIL entry of tune #x:
          get*Entry(..., tuneNo=x, field=<field>)
       d) Retrieve the BUG entry for tune #x with:
          get*Bug(..., tuneNo=x)

     If you would like to extract everything from STIL relating to a specific
     SID file (including all its subtunes), the procedure is:
       a) Retrieve the section-global comment with:
          get*GlobalComment()
       c) Retrieve all of the STIL entry with:
          get*Entry(..., tuneNo=0, field=all)
       d) Retrieve all of the BUG entry with:
          get*Bug(..., tuneNo=0)

     Any of these steps may or may not return something to you, but it is
     guaranteed that there will be no duplicate info if they all return
     something.
- BUGFIX: Increased the STIL_MAX_DIR #define to 500 (STIL v3.5 has entries for
  more than 300 dirs in it!).

v2.00beta:
- NEW: Added support for the new "NAME" field in STIL (see the new enum for
  STILField).
  Previous versions of STILView will *NOT* break because of the presence of
  this new field in STIL, but you won't be able to retrieve just this specific
  field with them with the get*Entry() methods.
- NEW: The Makefile is back again, and hopefully, this time it works on every
  platform (at least GCC likes it both under Solaris and Win98. :)
- NEW: Totally got rid of all references to timestamps. (I will just never have
  the time to implement it, plus nobody screamed to me that they want to have
  it implemented.) This also means that the public methods of get*Entry()
  have one less parameter now! You might have to change your code where these
  methods are called.
- NEW: Since a few users of the command line STILView complained about it (and
  rightly so), I have modified how the tuneNo and field parameters are
  interpreted by get*Entry(). Below is full and complete (!) explanation:
    - tuneNo = 0, field = all : all of the STIL entry is returned.
    - tuneNo = 0, field = comment : the file-global comment is returned.
      (For single-tune entries, this returns nothing! This is NEW!)
    - tuneNo = 0, field = <other> : INVALID! (NULL is returned) (This is NEW!)
    - tuneNo != 0, field = all : all fields of the STIL entry for the
      given tune number are returned. (For single-tune entries, this is
      equivalent to saying tuneNo = 0, field = all.)
      However, the file-global comment is *NOT* returned with it any
      more! (Unlike in versions before v2.00.) It led to confusions:
      eg. when a comment was asked for tune #3, it returned the
      file-global comment even if there was no specific entry for tune #3!
    - tuneNo != 0, field = <other> : the specific field of the specific
      tune number is returned. If the tune number doesn't exist (eg. if
      tuneNo=2 for single-tune entries, or if tuneNo=2 when there's no
      STIL entry for tune #2 in a multitune entry), returns NULL.
  What does all of this really mean to you? Two things:
    1) tuneNo != 0, field = all will no longer return the file-global comment
       (along with the actual entry) any more! In other words, the *ONLY* way
       that a file-global comment will show up in a result is if you explicitly
       ask for it (tuneNo=0, field=comment), or if you ask for the whole entry
       (tuneNo=0, field=all). (Provided it exists at all, of course.)
    2) Subsequently, tuneNo=0, field=name/title/artist will no longer return
       all of the STIL entry for single-tune entries! If you relied on this
       before, change your calling routine(s). The *ONLY* way to retrieve
       specific fields in a single-tune entry (other than a comment) is to say
       tuneNo=1, field=name/title/artist/comment.
    3) So, if you want to retrieve all pieces of info from STIL that are
       related to one specific tune in one specific SID file, you'll need to do
       the following steps:
       a) Retrieve the section-global comment with:
          get*GlobalComment()
       b) Retrieve the file-global comment with:
          get*Entry(..., tuneNo=0, field=comment)
       c) Retrieve all of the STIL entry for tune #x:
          get*Entry(..., tuneNo=x)
          or retrieve one field from the STIL entry of tune #x:
          get*Entry(..., tuneNo=x, field=<field>)
       d) Retrieve the BUG entry for tune #x with:
          get*Bug(..., tuneNo=x)
       Any of these steps may or may not return something to you, but it is
       guaranteed that there will be no duplicate info if they all return
       something (ie. step c. will *NOT* return the file-global comment, even
       if there exists a COMMENT field in a single-tune entry!).
  I hope this all sounds and looks more logical now. Strangely enough, my
  modifications for this resulted in a leaner and more logical code and
  algorithm in stil.cpp... :-)
- NEW: Also, in the get*Entry() and get*Bug() methods the default value of
  tuneNo is now 0, and the default value of field is 'all'.
- NEW: Minor enhancements in the formatted output of the command-line STILView.
- BUGFIX: Previous versions of STILView used to have a "hidden feature": they
  were able to retrieve STIL entries even if only a partial pathname was
  specified, eg. /Hubbard_Rob/C retrieved /Hubbard_Rob/Chain_Reaction.sid (the
  first entry in Hubbard's section starting with a C). This was all nice, but
  if you asked for /Hubbard_Rob/Comm (which retrieved the Commando entry) then
  for /Hubbard_Rob/C again, this still returned the Commando entry, since the
  internal buffer now had a Hubbard entry in it that was starting with a C!
  This behavior was inconsistent, so now the pathname have to be a fully
  qualified STIL entry in order for the search to be successful. In other
  words, asking for partial pathnames (eg. just /Hubbard_Rob/C) will now fail.
- BUGFIX: If a section-global comment was asked for with get*Entry() (eg.
  /Hubbard_Rob/, with the trailing slash!), it returned it. This is not
  desirable, as those entries should be asked for only with
  get*GlobalComment(). Fixed it. Also added a new non-critical error to note
  when get*Entry() is wrongly used to ask for section-global comments.
- NEW: Made STIL entry search under UNIX totally case-insensitive (strcasecmp()
  instead of strcmp() ).
- NEW: If setBaseDir() fails, it will *not* have an impact on the private data
  already there. In other words, if you did a previous setBaseDir() that
  succeeded, then you do one again that fails, you should still be able to
  safely use the STIL class, because it'll have the private data populated from
  the first setBaseDir() operation intact.
  This new functionality slightly increases the time it takes to do
  setBaseDir(), but it should not be too noticeable.
- NEW: Separated the demo mode from the interactive mode, and added a new
  option (-m) for the command-line STILView to ask for just demo mode.
  Specifying just -m will print some things demonstrating the capabilities of
  STILView (which also tests some of its most important features) then exits.
  Specifying just -i enters interactive mode. Specifying both -m and -i will
  work like previous versions: the demo is printed, then interactive mode is
  entered.

v1.30:
- Never-released internal version to test the new NAME field support.

v1.25:
- Version number is now defined as a float constant, not with a #define.
- ios::bad() calls were changed to the more compatible ios::fail().
- ios::clear() is now called every time before an ios::open() to clear the
  potential error bits left over from a previous open().
- the standalone STILView now returns a 0 after normal operation (it is still
  returning 0's and 1's with exit() in some cases, though).
- Added a sentence the (C) statements everywhere to reflect that I don't mind
  if other people modify and/or extend this code, as long as they still give
  credit to the original author (ie. me).
- Updated the USAGE.txt here and there, did a spell check on it and other
  small cosmetic changes.

v1.24:
- getGlobalComment() was not safeguarded: if an incorrectly formatted
  path+filename was passed to it, it crapped out. Fixed it.
- Previous versions are no longer available from the website. I am tight on
  space on the server (I have to pay for the extra space occupied), plus I see
  no need to keep earlier versions around. The Win32 executable is also gone
  for the same reason.

v1.23:
- Naturally, I screwed up the error strings with the reshuffled error codes.
  This could've caused having garbage printed out instead of the actual error
  description. This has been fixed.

v1.22:
- Added a new method called hasCriticalError() that tells whether the last
  encountered error was a critical show-stopper error or not. The STILerror
  enum values have also been shuffled around to support this, so if you did
  not use them by their symbolic names, take a look at them again.

v1.21:
- Changed the demo entry to /Galway_Martin/Green_Beret.sid

v1.20:
- Got rid of the Makefile in the source distribution - it was too specific.
- Added #defines to support the Amiga OS.
- Renamed everything concerning DEBUG to be more STILView specific.
- Added a new public method called getError(), which returns the specific
  error number for the error that happened during the last execution of a STIL
  method.
- Added a new public method called getErrorStr(), which returns a string
  containing the description of the error that happened during the last
  execution of a STIL method.
- Changed all "char *" to "const char *" in the public methods to get rid of
  the warnings they generated (the strings passed in do not get modified at
  all in the STIL class).
- The included stilview.cpp file now compiles a standalone command-line
  version of STILView, mainly for demonstration purposes, but it can be used
  as a full-fledged command-line tool. See USAGE.TXT for details about its
  usage.
- STILView now has a webpage from where all versions can be downloaded.

v1.12:
- Fixed a bug that happened with the latest STIL v2.6 and was due to the fact
  that the STIL got resorted case-insensitively. Made the positionToEntry()
  function independent of how STIL is sorted.
- At the same time, also made a small change that speeds up the entry search
  within a section in positionToEntry() significantly (a friendly advice: use
  tellg() only when really necessary, otherwise it slows things down
  considerably, like it did here...)

v1.11:
- Fixed a bug with the getAbs*() functions that was introduced with the
  previous bugfix. ;)
- STIL::setBaseDir() now will complain when an empty string is passed to it
  as a path.
- Fixed a minor compile-time warning (getDirs() was hiding stilFile).
- The source files in this package now have the .cpp extension instead of .C.

v1.10:
- Fixed bug when an entry was passed in with an empty string instead of the
  actual relative path (caused a crash in SIDPlay/Win when playing SIDs
  outside of the HVSC subdir).
- Fixed problem with doing a case-sensitive pathname match under Win instead
  of a case-insensitive one.
- Significantly sped up the initial parsing of STIL.txt when building up the
  list of directories.

v1.00:
- First public release.
