/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 1998, 2002 by LaLa <LaLa@C64.org>
 * Copyright 2012-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef STIL_H
#define STIL_H

#include <string>
#include <algorithm>
#include <map>
#include <iosfwd>

#include "stildefs.h"

/**
 * STIL class
 *
 * @author LaLa <LaLa@C64.org>
 * @copyright 1998, 2002 by LaLa
 *
 *
 * Given the location of HVSC this class can extract STIL information for a
 * given tune of a given SID file. (Sounds simple, huh?)
 *
 * PLEASE, READ THE ACCOMPANYING README.TXT FILE BEFORE PROCEEDING!!!!
 */
class STIL_EXTERN STIL
{
public:

    /// Enum to use for asking for specific fields.
    enum STILField
    {
        all,
        name,
        author,
        title,
        artist,
        comment
    };

    /// Enum that describes the possible errors this class may encounter.
    enum STILerror
    {
        NO_STIL_ERROR = 0,
        BUG_OPEN,           ///< INFO ONLY: failed to open BUGlist.txt.
        WRONG_DIR,          ///< INFO ONLY: path was not within HVSC base dir.
        NOT_IN_STIL,        ///< INFO ONLY: requested entry was not found in STIL.txt.
        NOT_IN_BUG,         ///< INFO ONLY: requested entry was not found in BUGlist.txt.
        WRONG_ENTRY,        ///< INFO ONLY: section-global comment was asked for with get*Entry().
        CRITICAL_STIL_ERROR = 10,
        BASE_DIR_LENGTH,    ///< The length of the HVSC base dir was wrong (empty string?)
        STIL_OPEN,          ///< Failed to open STIL.txt.
        NO_EOL,             ///< Failed to determine EOL char(s).
        NO_STIL_DIRS,       ///< Failed to get sections (subdirs) when parsing STIL.txt.
        NO_BUG_DIRS         ///< Failed to get sections (subdirs) when parsing BUGlist.txt.
    };

    /// To turn debug output on
    bool STIL_DEBUG;

    //----//

    /**
     * Allocates necessary memory.
     *
     * @param stilPath relative path to STIL file
     * @param bugsPath relative path to BUG file
     */
    STIL(const char *stilPath = DEFAULT_PATH_TO_STIL, const char *bugsPath = DEFAULT_PATH_TO_BUGLIST);

    /**
     * Returns a formatted string telling what the version
     * number is for the STIL class and other info.
     * If it is called after setBaseDir(), the string also
     * has the STIL.txt file's version number in it.
     *
     * @return
     *     printable formatted string with version and copyright
     *     info
     *     (It's kinda dangerous to return a pointer that points
     *     to an internal structure, but I trust you. :)
     */
    const char *getVersion();

    /**
     * Returns a floating number telling what the version
     * number is of this STIL class.
     *
     * @return
     *     version number
     */
    float getVersionNo();

    /**
     * Tell the object where the HVSC base directory is - it
     * figures that the STIL should be in /DOCUMENTS/STIL.txt
     * and that the BUGlist should be in /DOCUMENTS/BUGlist.txt.
     * It should not matter whether the path is given in UNIX,
     * WinDOS, or Mac format (ie. '\' vs. '/' vs. ':')
     *
     * @param  pathToHVSC = HVSC base directory in your machine's format
     * @return
     *      - false - Problem opening or parsing STIL/BUGlist
     *      - true  - All okay
     */
    bool setBaseDir(const char *pathToHVSC);

    /**
     * Returns a floating number telling what the version
     * number is of the STIL.txt file.
     * To be called only after setBaseDir()!
     *
     * @return
     *     version number (0.0 if setBaseDir() was not called, yet)
     */
    float getSTILVersionNo();

    /**
     * Given an HVSC pathname, a tune number and a
     * field designation, it returns a formatted string that
     * contains the STIL field for the tune number (if exists).
     * If it doesn't exist, returns a NULL.
     *
     * @param relPathToEntry = relative to the HVSC base dir, starting with
     *                         a slash
     * @param tuneNo         = song number within the song (default=0).
     * @param field          = which field to retrieve (default=all).
     *
     * What the possible combinations of tuneNo and field represent:
     *
     * - tuneNo = 0, field = all : all of the STIL entry is returned.
     * - tuneNo = 0, field = comment : the file-global comment is returned.
     *   (For single-tune entries, this returns nothing!)
     * - tuneNo = 0, field = <other> : INVALID! (NULL is returned)
     * - tuneNo != 0, field = all : all fields of the STIL entry for the
     *   given tune number are returned. (For single-tune entries, this is
     *   equivalent to saying tuneNo = 0, field = all.)
     *   However, the file-global comment is *NOT* returned with it any
     *   more! (Unlike in versions before v2.00.) It led to confusions:
     *   eg. when a comment was asked for tune #3, it returned the
     *   file-global comment even if there was no specific entry for tune #3!
     * - tuneNo != 0, field = <other> : the specific field of the specific
     *   tune number is returned. If the tune number doesn't exist (eg. if
     *   tuneNo=2 for single-tune entries, or if tuneNo=2 when there's no
     *   STIL entry for tune #2 in a multitune entry), returns NULL.
     *
     * NOTE: For older versions of STIL (older than v2.59) the tuneNo and
     * field parameters are ignored and are assumed to be tuneNo=0 and
     * field=all to maintain backwards compatibility.
     *
     * @return
     *      - pointer to a printable formatted string containing
     *        the STIL entry
     *        (It's kinda dangerous to return a pointer that points
     *        to an internal structure, but I trust you. :)
     *      - NULL if there's absolutely no STIL entry for the tune
     */
    const char *getEntry(const char *relPathToEntry, int tuneNo = 0, STILField field = all);

    /**
     * Same as #getEntry, but with an absolute path given
     * given in your machine's format.
     */
    const char *getAbsEntry(const char *absPathToEntry, int tuneNo = 0, STILField field = all);

    /**
     * Given an HVSC pathname and tune number it returns a
     * formatted string that contains the section-global
     * comment for the tune number (if it exists). If it
     * doesn't exist, returns a NULL.
     *
     * @param relPathToEntry = relative to the HVSC base dir starting with
     *                       a slash
     * @return
     *      - pointer to a printable formatted string containing
     *        the section-global comment
     *        (It's kinda dangerous to return a pointer that points
     *        to an internal structure, but I trust you. :)
     *      - NULL if there's absolutely no section-global comment
     *        for the tune
     */
    const char *getGlobalComment(const char *relPathToEntry);

    /**
     * Same as #getGlobalComment, but with an absolute path
     * given in your machine's format.
     */
    const char *getAbsGlobalComment(const char *absPathToEntry);

    /**
     * Given an HVSC pathname and tune number it returns a
     * formatted string that contains the BUG entry for the
     * tune number (if exists). If it doesn't exist, returns
     * a NULL.
     *
     * @param relPathToEntry = relative to the HVSC base dir starting with
     *                         a slash
     * @param tuneNo         = song number within the song (default=0)
     *                         If tuneNo=0, returns all of the BUG entry.
     *
     *      NOTE: For older versions of STIL (older than v2.59) tuneNo is
     *      ignored and is assumed to be 0 to maintain backwards
     *      compatibility.
     *
     * @return
     *      - pointer to a printable formatted string containing
     *        the BUG entry
     *        (It's kinda dangerous to return a pointer that points
     *        to an internal structure, but I trust you. :)
     *      - NULL if there's absolutely no BUG entry for the tune
     */
    const char *getBug(const char *relPathToEntry, int tuneNo = 0);

    /**
     * Same as #getBug, but with an absolute path
     * given in your machine's format.
     */
    const char *getAbsBug(const char *absPathToEntry, int tuneNo = 0);

    /**
     * Returns a specific error number identifying the problem
     * that happened at the last invoked public method.
     *
     * @return
     *      STILerror - an enumerated error value
     */
    inline STILerror getError() const {return (lastError);}

    /**
     * Returns true if the last error encountered was critical
     * (ie. not one that the STIL class can recover from).
     *
     * @return
     *      true if the last error encountered was critical
     */
    inline bool hasCriticalError() const
    {
        return ((lastError >= CRITICAL_STIL_ERROR) ? true : false);
    }

    /**
     * Returns an ASCII error string containing the
     * description of the error that happened at the last
     * invoked public method.
     *
     * @return
     *      pointer to string with the error description
     */
    inline const char *getErrorStr() const {return (STIL_ERROR_STR[lastError]);}

private:
    typedef std::map<std::string, std::streampos> dirList;

    /// Path to STIL.
    const char *PATH_TO_STIL;

    /// Path to BUGlist.
    const char *PATH_TO_BUGLIST;

    /// Version number/copyright string
    std::string versionString;

    /// STIL.txt's version number
    float STILVersion;

    /// Base dir
    std::string baseDir;

    /// Maps of sections (subdirs) for easier positioning.
    //@{
    dirList stilDirs;
    dirList bugDirs;
    //@}

    /**
     * This tells us what the line delimiter is in STIL.txt.
     * (It may be two chars!)
     */
    char STIL_EOL;
    char STIL_EOL2;

    /// Error number of the last error that happened.
    STILerror lastError;

    /// Error strings containing the description of the possible errors in STIL.
    static const char *STIL_ERROR_STR[];

    ////////////////

    /// The last retrieved entry
    std::string entrybuf;

    /// The last retrieved section-global comment
    std::string globalbuf;

    /// The last retrieved BUGentry
    std::string bugbuf;

    /// Buffers to hold the resulting strings
    std::string resultEntry;
    std::string resultBug;

    ////////////////

    void setVersionString();

    /**
     * Determines what the EOL char is (or are) from STIL.txt.
     * It is assumed that BUGlist.txt will use the same EOL.
     *
     * @return
     *      - false - something went wrong
     *      - true  - everything is okay
     */
    bool determineEOL(std::ifstream &stilFile);

    /**
     * Populates the given dirList array with the directories
     * obtained from 'inFile' for faster positioning within
     * 'inFile'.
     *
     * @param inFile - where to read the directories from
     * @param dirs   - the dirList array that should be populated with the
     *                 directory list
     * @param isSTILFile - is this the STIL or the BUGlist we are parsing
     * @return
     *      - false - No entries were found or otherwise failed to process
     *                inFile
     *      - true  - everything is okay
     */
    bool getDirs(std::ifstream &inFile, dirList &dirs, bool isSTILFile);

    /**
     * Positions the file pointer to the given entry in 'inFile'
     * using the 'dirs' dirList for faster positioning.
     *
     * @param entryStr - the entry to position to
     * @param inFile   - position the file pointer in this file
     * @param dirs     - the list of dirs in inFile for easier positioning
     * @return
     *      - true - if successful
     *      - false - otherwise
     */
    bool positionToEntry(const char *entryStr, std::ifstream &inFile, dirList &dirs);

    /**
     * Reads the entry from 'inFile' into 'buffer'. 'inFile' should
     * already be positioned to the entry to be read.
     *
     * @param inFile   - filehandle of file to read from
     * @param entryStr - the entry needed to be read
     * @param buffer   - where to put the result to
     */
    void readEntry(std::ifstream &inFile, std::string &buffer);

    /**
     * Given a STIL formatted entry in 'buffer', a tune number,
     * and a field designation, it returns the requested
     * STIL field into 'result'.
     * If field=all, it also puts the file-global comment (if it exists)
     * as the first field into 'result'.
     *
     * @param result - where to put the resulting string to (if any)
     * @param buffer - pointer to the first char of what to search for
     *                 the field. Should be a buffer in standard STIL
     *                 format.
     * @param tuneNo - song number within the song (default=0)
     * @param field  - which field to retrieve (default=all).
     * @return
     *      - false - if nothing was put into 'result'
     *      - true  - 'result' has the resulting field
     */
    bool getField(std::string &result, const char *buffer, int tuneNo = 0, STILField field = all);

    /**
     * @param result - where to put the resulting string to (if any)
     * @param start  - pointer to the first char of what to search for
     *                 the field. Should be a buffer in standard STIL
     *                 format.
     * @param end    - pointer to the last+1 char of what to search for
     *                 the field. ('end-1' should be a '\n'!)
     * @param field  - which specific field to retrieve
     * @return
     *      - false - if nothing was put into 'result'
     *      - true  - 'result' has the resulting field
     */
    bool getOneField(std::string &result, const char *start, const char *end, STILField field);

    /**
     * Extracts one line from 'infile' to 'line[]'. The end of
     * the line is marked by endOfLineChar. Also eats up
     * additional EOL-like chars.
     *
     * @param infile - filehandle (streampos should already be positioned
     *                 to the start of the desired line)
     * @param line   - char array to put the line into
     */
    void getStilLine(std::ifstream &infile, std::string &line);
};

#endif // STIL_H
