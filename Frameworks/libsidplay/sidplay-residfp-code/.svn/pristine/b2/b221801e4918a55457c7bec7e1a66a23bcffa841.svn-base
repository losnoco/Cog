/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 1998, 2002 by LaLa <LaLa@C64.org>
 * Copyright 2012-2015 Leandro Nini <drfiemost@users.sourceforge.net>
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

//
// STIL class - Implementation file
//
// AUTHOR: LaLa
// Email : LaLa@C64.org
// Copyright (C) 1998, 2002 by LaLa
//

#include "stil.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>      // For snprintf() and NULL
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <utility>

#include "stringutils.h"

using namespace std;

const ios_base::openmode STILopenFlags = ios::in | ios::binary;

const float VERSION_NO = 3.0f;

#define CERR_STIL_DEBUG if (STIL_DEBUG) cerr << "Line #" << __LINE__ << " STIL::"

// These are the hardcoded STIL/BUG field names.
const char    _NAME_STR[] = "   NAME: ";
const char  _AUTHOR_STR[] = " AUTHOR: ";
const char   _TITLE_STR[] = "  TITLE: ";
const char  _ARTIST_STR[] = " ARTIST: ";
const char _COMMENT_STR[] = "COMMENT: ";
const char     _BUG_STR[] = "BUG: ";

const char *STIL::STIL_ERROR_STR[] =
{
    "No error.",
    "Failed to open BUGlist.txt.",
    "Base dir path is not the HVSC base dir path.",
    "The entry was not found in STIL.txt.",
    "The entry was not found in BUGlist.txt.",
    "A section-global comment was asked for in the wrong way.",
    "",
    "",
    "",
    "",
    "CRITICAL ERROR",
    "Incorrect HVSC base dir length!",
    "Failed to open STIL.txt!",
    "Failed to determine EOL from STIL.txt!",
    "No STIL sections were found in STIL.txt!",
    "No STIL sections were found in BUGlist.txt!"
};

/**
 * Converts slashes to the one the OS uses to access files.
 *
 * @param
 *      str - what to convert
 */
void convertSlashes(std::string &str) { std::replace(str.begin(), str.end(), '/', SLASH); }

/**
 * Converts OS specific dir separators to slashes.
 *
 * @param
 *      str - what to convert
 */
void convertToSlashes(std::string &str) { std::replace(str.begin(), str.end(), SLASH, '/'); }


// CONSTRUCTOR
STIL::STIL(const char *stilPath, const char *bugsPath) :
    STIL_DEBUG(false),
    PATH_TO_STIL(stilPath),
    PATH_TO_BUGLIST(bugsPath),
    STILVersion(0.0f),
    STIL_EOL('\n'),
    STIL_EOL2('\0'),
    lastError(NO_STIL_ERROR)
{
    setVersionString();
}

void STIL::setVersionString()
{
    ostringstream ss;
    ss << fixed << setw(4) << setprecision(2);
    ss << "STILView v" << VERSION_NO << endl;
    ss << "\tCopyright (C) 1998, 2002 by LaLa (LaLa@C64.org)" << endl;
    ss << "\tCopyright (C) 2012-2015 by Leandro Nini <drfiemost@users.sourceforge.net>" << endl;
    versionString = ss.str();
}

const char *
STIL::getVersion()
{
    lastError = NO_STIL_ERROR;
    return versionString.c_str();
}

float
STIL::getVersionNo()
{
    lastError = NO_STIL_ERROR;
    return VERSION_NO;
}

float
STIL::getSTILVersionNo()
{
    lastError = NO_STIL_ERROR;
    return STILVersion;
}

bool
STIL::setBaseDir(const char *pathToHVSC)
{
    // Temporary placeholder for STIL.txt's version number.
    const float tempSTILVersion = STILVersion;

    // Temporary placeholders for lists of sections.
    dirList tempStilDirs;
    dirList tempBugDirs;

    lastError = NO_STIL_ERROR;

    CERR_STIL_DEBUG << "setBaseDir() called, pathToHVSC=" << pathToHVSC << endl;

    string tempBaseDir(pathToHVSC);

    // Sanity check the length.
    if (tempBaseDir.empty())
    {
        CERR_STIL_DEBUG << "setBaseDir() has problem with the size of pathToHVSC" << endl;
        lastError = BASE_DIR_LENGTH;
        return false;
    }

    // Chop the trailing slash
    const string::iterator lastChar = tempBaseDir.end() - 1;

    if (*lastChar == SLASH)
    {
        tempBaseDir.erase(lastChar);
    }

    // Attempt to open STIL

    // Create the full path+filename
    string tempName = tempBaseDir;
    tempName.append(PATH_TO_STIL);
    convertSlashes(tempName);

    ifstream stilFile(tempName.c_str(), STILopenFlags);

    if (stilFile.fail())
    {
        CERR_STIL_DEBUG << "setBaseDir() open failed for " << tempName << endl;
        lastError = STIL_OPEN;
        return false;
    }

    CERR_STIL_DEBUG << "setBaseDir(): open succeeded for " << tempName << endl;

    // Attempt to open BUGlist

    // Create the full path+filename
    tempName = tempBaseDir;
    tempName.append(PATH_TO_BUGLIST);
    convertSlashes(tempName);

    ifstream bugFile(tempName.c_str(), STILopenFlags);

    if (bugFile.fail())
    {
        // This is not a critical error - some earlier versions of HVSC did
        // not have a BUGlist.txt file at all.

        CERR_STIL_DEBUG << "setBaseDir() open failed for " << tempName << endl;
        lastError = BUG_OPEN;
    }
    else
    {
        CERR_STIL_DEBUG << "setBaseDir(): open succeeded for " << tempName << endl;
    }

    // Find out what the EOL really is
    if (determineEOL(stilFile) != true)
    {
        CERR_STIL_DEBUG << "determinEOL() failed" << endl;
        lastError = NO_EOL;
        return false;
    }

    // Save away the current string so we can restore it if needed.
    const string tempVersionString(versionString);

    setVersionString();

    // This is necessary so the version number gets scanned in from the new
    // file, too.
    STILVersion = 0.0;

    // These will populate the tempStilDirs and tempBugDirs maps (or not :)

    if (getDirs(stilFile, tempStilDirs, true) != true)
    {
        CERR_STIL_DEBUG << "getDirs() failed for stilFile" << endl;
        lastError = NO_STIL_DIRS;

        // Clean up and restore things.
        STILVersion = tempSTILVersion;
        versionString = tempVersionString;
        return false;
    }

    if (bugFile.good())
    {
        if (getDirs(bugFile, tempBugDirs, false) != true)
        {
            // This is not a critical error - it is possible that the
            // BUGlist.txt file has no entries in it at all (in fact, that's
            // good!).

            CERR_STIL_DEBUG << "getDirs() failed for bugFile" << endl;
            lastError = BUG_OPEN;
        }
    }

    // Now we can copy the stuff into private data.
    // NOTE: At this point, STILVersion and the versionString should contain
    // the new info!

    // Copy.
    baseDir = tempBaseDir;
    stilDirs = tempStilDirs;
    bugDirs = tempBugDirs;

    // Clear the buffers (caches).
    entrybuf.clear();
    globalbuf.clear();
    bugbuf.clear();

    CERR_STIL_DEBUG << "setBaseDir() succeeded" << endl;

    return true;
}

const char *
STIL::getAbsEntry(const char *absPathToEntry, int tuneNo, STILField field)
{
    lastError = NO_STIL_ERROR;

    CERR_STIL_DEBUG << "getAbsEntry() called, absPathToEntry=" << absPathToEntry << endl;

    if (baseDir.empty())
    {
        CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
        lastError = STIL_OPEN;
        return NULL;
    }

    // Determine if the baseDir is in the given pathname.

    if (!stringutils::equal(absPathToEntry, baseDir.data(), baseDir.size()))
    {
        CERR_STIL_DEBUG << "getAbsEntry() failed: baseDir=" << baseDir << ", absPath=" << absPathToEntry << endl;
        lastError = WRONG_DIR;
        return NULL;
    }


    string tempDir(absPathToEntry + baseDir.size());
    convertToSlashes(tempDir);

    return getEntry(tempDir.c_str(), tuneNo, field);
}

const char *
STIL::getEntry(const char *relPathToEntry, int tuneNo, STILField field)
{
    lastError = NO_STIL_ERROR;

    CERR_STIL_DEBUG << "getEntry() called, relPath=" << relPathToEntry << ", rest=" << tuneNo << "," << field << endl;

    if (baseDir.empty())
    {
        CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
        lastError = STIL_OPEN;
        return NULL;
    }

    const size_t relPathToEntryLen = strlen(relPathToEntry);

    // Fail if a section-global comment was asked for.

    if (*(relPathToEntry + relPathToEntryLen - 1) == '/')
    {
        CERR_STIL_DEBUG << "getEntry() section-global comment was asked for - failed" << endl;
        lastError = WRONG_ENTRY;
        return NULL;
    }

    if (STILVersion < 2.59f)
    {
        // Older version of STIL is detected.

        tuneNo = 0;
        field = all;
    }

    // Find out whether we have this entry in the buffer.

    if ((!stringutils::equal(entrybuf.data(), relPathToEntry, relPathToEntryLen))
        || ((entrybuf.find_first_of('\n') != relPathToEntryLen)
        && (STILVersion > 2.59f)))
    {
        // The relative pathnames don't match or they're not the same length:
        // we don't have it in the buffer, so pull it in.

        CERR_STIL_DEBUG << "getEntry(): entry not in buffer" << endl;

        // Create the full path+filename
        string tempName(baseDir);
        tempName.append(PATH_TO_STIL);
        convertSlashes(tempName);

        ifstream stilFile(tempName.c_str(), STILopenFlags);

        if (stilFile.fail())
        {
            CERR_STIL_DEBUG << "getEntry() open failed for stilFile" << endl;
            lastError = STIL_OPEN;
            return NULL;
        }

        CERR_STIL_DEBUG << "getEntry() open succeeded for stilFile" << endl;

        if (positionToEntry(relPathToEntry, stilFile, stilDirs) == false)
        {
            // Copy the entry's name to the buffer.
            entrybuf.assign(relPathToEntry).append("\n");
            CERR_STIL_DEBUG << "getEntry() posToEntry() failed" << endl;
            lastError = NOT_IN_STIL;
        }
        else
        {
            entrybuf.clear();
            readEntry(stilFile, entrybuf);
            CERR_STIL_DEBUG << "getEntry() entry read" << endl;
        }
    }

    // Put the requested field into the result string.
    return getField(resultEntry, entrybuf.c_str(), tuneNo, field) ? resultEntry.c_str() : NULL;
}

const char *
STIL::getAbsBug(const char *absPathToEntry, int tuneNo)
{
    lastError = NO_STIL_ERROR;

    CERR_STIL_DEBUG << "getAbsBug() called, absPathToEntry=" << absPathToEntry << endl;

    if (baseDir.empty())
    {
        CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
        lastError = BUG_OPEN;
        return NULL;
    }

    // Determine if the baseDir is in the given pathname.

    if (!stringutils::equal(absPathToEntry, baseDir.data(), baseDir.size()))
    {
        CERR_STIL_DEBUG << "getAbsBug() failed: baseDir=" << baseDir << ", absPath=" << absPathToEntry << endl;
        lastError = WRONG_DIR;
        return NULL;
    }

    string tempDir(absPathToEntry + baseDir.size());
    convertToSlashes(tempDir);

    return getBug(tempDir.c_str(), tuneNo);
}

const char *
STIL::getBug(const char *relPathToEntry, int tuneNo)
{
    lastError = NO_STIL_ERROR;

    CERR_STIL_DEBUG << "getBug() called, relPath=" << relPathToEntry << ", rest=" << tuneNo << endl;

    if (baseDir.empty())
    {
        CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
        lastError = BUG_OPEN;
        return NULL;
    }

    // Older version of STIL is detected.

    if (STILVersion < 2.59f)
    {
        tuneNo = 0;
    }

    // Find out whether we have this bug entry in the buffer.
    // If the baseDir was changed, we'll have to read it in again,
    // even if it might be in the buffer already.

    const size_t relPathToEntryLen = strlen(relPathToEntry);

    if ((!stringutils::equal(bugbuf.data(), relPathToEntry, relPathToEntryLen)) ||
        ((bugbuf.find_first_of('\n') != relPathToEntryLen) &&
         (STILVersion > 2.59f)))
    {
        // The relative pathnames don't match or they're not the same length:
        // we don't have it in the buffer, so pull it in.

        CERR_STIL_DEBUG << "getBug(): entry not in buffer" << endl;

        // Create the full path+filename
        string tempName(baseDir);
        tempName.append(PATH_TO_BUGLIST);
        convertSlashes(tempName);

        ifstream bugFile(tempName.c_str(), STILopenFlags);

        if (bugFile.fail())
        {
            CERR_STIL_DEBUG << "getBug() open failed for bugFile" << endl;
            lastError = BUG_OPEN;
            return NULL;
        }

        CERR_STIL_DEBUG << "getBug() open succeeded for bugFile" << endl;

        if (positionToEntry(relPathToEntry, bugFile, bugDirs) == false)
        {
            // Copy the entry's name to the buffer.
            bugbuf.assign(relPathToEntry).append("\n");
            CERR_STIL_DEBUG << "getBug() posToEntry() failed" << endl;
            lastError = NOT_IN_BUG;
        }
        else
        {
            bugbuf.clear();
            readEntry(bugFile, bugbuf);
            CERR_STIL_DEBUG << "getBug() entry read" << endl;
        }
    }

    // Put the requested field into the result string.
    return getField(resultBug, bugbuf.c_str(), tuneNo) ? resultBug.c_str() : NULL;
}

const char *
STIL::getAbsGlobalComment(const char *absPathToEntry)
{
    lastError = NO_STIL_ERROR;

    CERR_STIL_DEBUG << "getAbsGC() called, absPathToEntry=" << absPathToEntry << endl;

    if (baseDir.empty())
    {
        CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
        lastError = STIL_OPEN;
        return NULL;
    }

    // Determine if the baseDir is in the given pathname.

    if (!stringutils::equal(absPathToEntry, baseDir.data(), baseDir.size()))
    {
        CERR_STIL_DEBUG << "getAbsGC() failed: baseDir=" << baseDir << ", absPath=" << absPathToEntry << endl;
        lastError = WRONG_DIR;
        return NULL;
    }

    string tempDir(absPathToEntry + baseDir.size());
    convertToSlashes(tempDir);

    return getGlobalComment(tempDir.c_str());
}

const char *
STIL::getGlobalComment(const char *relPathToEntry)
{
    lastError = NO_STIL_ERROR;

    CERR_STIL_DEBUG << "getGC() called, relPath=" << relPathToEntry << endl;

    if (baseDir.empty())
    {
        CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
        lastError = STIL_OPEN;
        return NULL;
    }

    // Save the dirpath.

    const char *lastSlash = strrchr(relPathToEntry, '/');

    if (lastSlash == NULL)
    {
        lastError = WRONG_DIR;
        return NULL;
    }

    const size_t pathLen = lastSlash - relPathToEntry + 1;
    const string dir(relPathToEntry, pathLen);

    // Find out whether we have this global comment in the buffer.
    // If the baseDir was changed, we'll have to read it in again,
    // even if it might be in the buffer already.

    if ((!stringutils::equal(globalbuf.data(), dir.data(), pathLen)) ||
        ((globalbuf.find_first_of('\n') != pathLen) &&
         (STILVersion > 2.59f)))
    {
        // The relative pathnames don't match or they're not the same length:
        // we don't have it in the buffer, so pull it in.

        CERR_STIL_DEBUG << "getGC(): entry not in buffer" << endl;

        // Create the full path+filename
        string tempName(baseDir);
        tempName.append(PATH_TO_STIL);
        convertSlashes(tempName);

        ifstream stilFile(tempName.c_str(), STILopenFlags);

        if (stilFile.fail())
        {
            CERR_STIL_DEBUG << "getGC() open failed for stilFile" << endl;
            lastError = STIL_OPEN;
            return NULL;
        }

        if (positionToEntry(dir.c_str(), stilFile, stilDirs) == false)
        {
            // Copy the dirname to the buffer.
            globalbuf.assign(dir).append("\n");
            CERR_STIL_DEBUG << "getGC() posToEntry() failed" << endl;
            lastError = NOT_IN_STIL;
        }
        else
        {
            globalbuf.clear();
            readEntry(stilFile, globalbuf);
            CERR_STIL_DEBUG << "getGC() entry read" << endl;
        }
    }

    CERR_STIL_DEBUG << "getGC() globalbuf=" << globalbuf << endl;
    CERR_STIL_DEBUG << "-=END=-" << endl;

    // Position pointer to the global comment field.

    const size_t temp = globalbuf.find_first_of('\n') + 1;

    // Check whether this is a NULL entry or not.
    return temp != globalbuf.size() ? globalbuf.c_str() + temp : NULL;
}

//////// PRIVATE

bool
STIL::determineEOL(ifstream &stilFile)
{
    CERR_STIL_DEBUG << "detEOL() called" << endl;

    if (stilFile.fail())
    {
        CERR_STIL_DEBUG << "detEOL() open failed" << endl;
        return false;
    }

    stilFile.seekg(0);

    STIL_EOL = '\0';
    STIL_EOL2 = '\0';

    // Determine what the EOL character is
    // (it can be different from OS to OS).
    istream::sentry se(stilFile, true);
    if (se)
    {
        streambuf *sb = stilFile.rdbuf();

        const int eof = char_traits<char>::eof();

        while (sb->sgetc() != eof)
        {
            const int c = sb->sbumpc();
            if ((c == '\n') || (c == '\r'))
            {
                STIL_EOL = c;

                if (c == '\r')
                {
                    if (sb->sgetc() == '\n')
                        STIL_EOL2 = '\n';
                }
                break;
            }
        }
    }

    if (STIL_EOL == '\0')
    {
        // Something is wrong - no EOL-like char was found.
        CERR_STIL_DEBUG << "detEOL() no EOL found" << endl;
        return false;
    }

    CERR_STIL_DEBUG << "detEOL() EOL1=0x" << hex << static_cast<int>(STIL_EOL) << " EOL2=0x" << hex << static_cast<int>(STIL_EOL2) << dec << endl;

    return true;
}

bool
STIL::getDirs(ifstream &inFile, dirList &dirs, bool isSTILFile)
{
    bool newDir = !isSTILFile;

    CERR_STIL_DEBUG << "getDirs() called" << endl;

    inFile.seekg(0);

    while (inFile.good())
    {
        string line;

        getStilLine(inFile, line);

        if (!isSTILFile) { CERR_STIL_DEBUG << line << '\n'; }

        // Try to extract STIL's version number if it's not done, yet.

        if (isSTILFile && (STILVersion == 0.0f))
        {
            if (strncmp(line.data(), "#  STIL v", 9) == 0)
            {
                // Get the version number
                STILVersion = atof(line.c_str() + 9);

                // Put it into the string, too.
                ostringstream ss;
                ss << fixed << setw(4) << setprecision(2);
                ss << "SID Tune Information List (STIL) v" << STILVersion << endl;
                versionString.append(ss.str());

                CERR_STIL_DEBUG << "getDirs() STILVersion=" << STILVersion << endl;

                continue;
            }
        }

        // Search for the start of a dir separator first.

        if (isSTILFile && !newDir && stringutils::equal(line.data(), "### ", 4))
        {
            newDir = true;
            continue;
        }

        // Is this the start of an entry immediately following a dir separator?

        if (newDir && (line[0] == '/'))
        {
            // Get the directory only
            const string dirName(line, 0, line.find_last_of('/') + 1);

            if (!isSTILFile)
            {
                // Compare it to the stored dirnames
                newDir = (dirs.count(dirName) == 0);
            }

            // Store the info
            if (newDir)
            {
                const streampos position = inFile.tellg() - (streampos)line.size() - 1L;

                CERR_STIL_DEBUG << "getDirs() dirName=" << dirName << ", pos=" << position <<  endl;

                dirs.insert(make_pair(dirName, position));
            }

            newDir = !isSTILFile;
        }
    }

    if (dirs.empty())
    {
        // No entries found - something is wrong.
        // NOTE: It's perfectly valid to have a BUGlist.txt file with no
        // entries in it!
        CERR_STIL_DEBUG << "getDirs() no dirs found" << endl;
        return false;
    }

    CERR_STIL_DEBUG << "getDirs() successful" << endl;

    return true;
}

bool
STIL::positionToEntry(const char *entryStr, ifstream &inFile, dirList &dirs)
{
    CERR_STIL_DEBUG << "pos2Entry() called, entryStr=" << entryStr << endl;

    inFile.seekg(0);

    // Get the dirpath.

    char *chrptr = strrchr((char *)entryStr, '/');

    // If no slash was found, something is screwed up in the entryStr.

    if (chrptr == NULL)
    {
        return false;
    }

    const size_t pathLen = chrptr - entryStr + 1;

    // Determine whether a section-global comment is asked for.

    const size_t entryStrLen = strlen(entryStr);
    const bool globComm = (pathLen == entryStrLen);

    // Find it in the table.
    const string entry(entryStr, pathLen);
    dirList::iterator elem = dirs.find(entry);

    if (elem == dirs.end())
    {
        // The directory was not found.
        CERR_STIL_DEBUG << "pos2Entry() did not find the dir" << endl;
        return false;
    }

    // Jump to the first entry of this section.
    inFile.seekg(elem->second);
    bool foundIt = false;

    // Now find the desired entry

    string line;

    do
    {
        getStilLine(inFile, line);

        if (inFile.eof())
        {
            break;
        }

        // Check if it is the start of an entry

        if (line[0] == '/')
        {
            if (!stringutils::equal(elem->first.data(), line.data(), pathLen))
            {
                // We are outside the section - get out of the loop,
                // which will fail the search.
                break;
            }

            // Check whether we need to find a section-global comment or
            // a specific entry.

            if (globComm || (STILVersion > 2.59f))
            {
                foundIt = stringutils::equal(line.c_str(), entryStr);
            }
            else
            {
                // To be compatible with older versions of STIL, which may have
                // the tune designation on the first line of a STIL entry
                // together with the pathname.
                foundIt = stringutils::equal(line.data(), entryStr, entryStrLen);
            }

            CERR_STIL_DEBUG << "pos2Entry() line=" << line << endl;
        }
    }
    while (!foundIt);

    if (foundIt)
    {
        // Reposition the file pointer back to the start of the entry.
        inFile.seekg(inFile.tellg() - (streampos)line.size() - 1L);
        CERR_STIL_DEBUG << "pos2Entry() entry found" << endl;
        return true;
    }
    else
    {
        CERR_STIL_DEBUG << "pos2Entry() entry not found" << endl;
        return false;
    }
}

void
STIL::readEntry(ifstream &inFile, string &buffer)
{
    string line;

    for (;;)
    {
        getStilLine(inFile, line);

        if (line.size() == 0)
            break;

        buffer.append(line);
        buffer.append("\n");
    }
}

bool
STIL::getField(string &result, const char *buffer, int tuneNo, STILField field)
{
    CERR_STIL_DEBUG << "getField() called, buffer=" << buffer << ", rest=" << tuneNo << "," << field << endl;

    // Clean out the result buffer first.
    result.clear();

    // Position pointer to the first char beyond the file designation.

    const char *start = strchr(buffer, '\n') + 1;

    // Check whether this is a NULL entry or not.

    if (*start == '\0')
    {
        CERR_STIL_DEBUG << "getField() null entry" << endl;
        return false;
    }

    // Is this a multitune entry?
    const char *firstTuneNo = strstr(start, "(#");

    // This is a tune designation only if the previous char was
    // a newline (ie. if the "(#" is on the beginning of a line).
    if ((firstTuneNo != NULL) && (*(firstTuneNo - 1) != '\n'))
    {
        firstTuneNo = NULL;
    }

    if (firstTuneNo == NULL)
    {
        //-------------------//
        // SINGLE TUNE ENTRY //
        //-------------------//

        // Is the first thing in this STIL entry the COMMENT?

        const char *temp = strstr(start, _COMMENT_STR);
        const char *temp2 = NULL;

        // Search for other potential fields beyond the COMMENT.
        if (temp == start)
        {
            temp2 = strstr(start, _NAME_STR);

            if (temp2 == NULL)
            {
                temp2 = strstr(start, _AUTHOR_STR);

                if (temp2 == NULL)
                {
                    temp2 = strstr(start, _TITLE_STR);

                    if (temp2 == NULL)
                    {
                        temp2 = strstr(start, _ARTIST_STR);
                    }
                }
            }
        }

        if (temp == start)
        {
            // Yes. So it's assumed to be a file-global comment.

            CERR_STIL_DEBUG << "getField() single-tune entry, COMMENT only" << endl;

            if ((tuneNo == 0) && ((field == all) || ((field == comment) && (temp2 == NULL))))
            {
                // Simply copy the stuff in.
                result.append(start);
                CERR_STIL_DEBUG << "getField() copied to resultbuf" << endl;
                return true;
            }

            else if ((tuneNo == 0) && (field == comment))
            {
                // Copy just the comment.
                result.append(start, temp2 - start);
                CERR_STIL_DEBUG << "getField() copied to just the COMMENT to resultbuf" << endl;
                return true;
            }

            else if ((tuneNo == 1) && (temp2 != NULL))
            {
                // A specific field was asked for.

                CERR_STIL_DEBUG << "getField() copying COMMENT to resultbuf" << endl;
                return getOneField(result, temp2, temp2 + strlen(temp2), field);
            }

            else
            {
                // Anything else is invalid as of v2.00.

                CERR_STIL_DEBUG << "getField() invalid parameter combo: single tune, tuneNo=" << tuneNo << ", field=" << field << endl;
                return false;
            }
        }
        else
        {
            // No. Handle it as a regular entry.

            CERR_STIL_DEBUG << "getField() single-tune regular entry" << endl;

            if ((field == all) && ((tuneNo == 0) || (tuneNo == 1)))
            {
                // The complete entry was asked for. Simply copy the stuff in.
                result.append(start);
                CERR_STIL_DEBUG << "getField() copied to resultbuf" << endl;
                return true;
            }

            else if (tuneNo == 1)
            {
                // A specific field was asked for.

                CERR_STIL_DEBUG << "getField() copying COMMENT to resultbuf" << endl;
                return getOneField(result, start, start + strlen(start), field);
            }

            else
            {
                // Anything else is invalid as of v2.00.

                CERR_STIL_DEBUG << "getField() invalid parameter combo: single tune, tuneNo=" << tuneNo << ", field=" << field << endl;
                return false;
            }
        }
    }
    else
    {
        //-------------------//
        // MULTITUNE ENTRY
        //-------------------//

        CERR_STIL_DEBUG << "getField() multitune entry" << endl;

        // Was the complete entry asked for?

        if (tuneNo == 0)
        {
            switch (field)
            {
            case all:
                // Yes. Simply copy the stuff in.
                result.append(start);
                CERR_STIL_DEBUG << "getField() copied all to resultbuf" << endl;
                return true;

            case comment:
                // Only the file-global comment field was asked for.

                if (firstTuneNo != start)
                {
                    CERR_STIL_DEBUG << "getField() copying file-global comment to resultbuf" << endl;
                    return getOneField(result, start, firstTuneNo, comment);
                }
                else
                {
                    CERR_STIL_DEBUG << "getField() no file-global comment" << endl;
                    return false;
                }

                break;

            default:
                // If a specific field other than a comment is
                // asked for tuneNo=0, this is illegal.

                CERR_STIL_DEBUG << "getField() invalid parameter combo: multitune, tuneNo=" << tuneNo << ", field=" << field << endl;
                return false;
            }
        }

        char tuneNoStr[8];

        // Search for the requested tune number.

        snprintf(tuneNoStr, 7, "(#%d)", tuneNo);
        tuneNoStr[7] = '\0';
        const char *myTuneNo = strstr(start, tuneNoStr);

        if (myTuneNo != NULL)
        {
            // We found the requested tune number.
            // Set the pointer beyond it.
            myTuneNo = strchr(myTuneNo, '\n') + 1;

            // Where is the next one?

            const char *nextTuneNo = strstr(myTuneNo, "\n(#");

            if (nextTuneNo == NULL)
            {
                // There is no next one - set pointer to end of entry.
                nextTuneNo = start + strlen(start);
            }
            else
            {
                // The search included the \n - go beyond it.
                nextTuneNo++;
            }

            // Put the desired fields into the result (which may be 'all').

            CERR_STIL_DEBUG << "getField() myTuneNo=" << myTuneNo << ", nextTuneNo=" << nextTuneNo << endl;
            return getOneField(result, myTuneNo, nextTuneNo, field);
        }

        else
        {
            CERR_STIL_DEBUG << "getField() nothing found" << endl;
            return false;
        }
    }
}

bool
STIL::getOneField(string &result, const char *start, const char *end, STILField field)
{
    // Sanity checking

    if ((end < start) || (*(end - 1) != '\n'))
    {
        CERR_STIL_DEBUG << "getOneField() illegal parameters" << endl;
        return false;
    }

    CERR_STIL_DEBUG << "getOneField() called, start=" << start << ", rest=" << field << endl;

    const char *temp = NULL;

    switch (field)
    {
    case all:
        result.append(start, end - start);
        return true;

    case name:
        temp = strstr(start, _NAME_STR);
        break;

    case author:
        temp = strstr(start, _AUTHOR_STR);
        break;

    case title:
        temp = strstr(start, _TITLE_STR);
        break;

    case artist:
        temp = strstr(start, _ARTIST_STR);
        break;

    case comment:
        temp = strstr(start, _COMMENT_STR);
        break;

    default:
        break;
    }

    // If the field was not found or it is not in between 'start'
    // and 'end', it is declared a failure.

    if ((temp == NULL) || (temp < start) || (temp > end))
    {
        return false;
    }

    // Search for the end of this field. This is done by finding
    // where the next field starts.

    const char *nextName = strstr(temp + 1, _NAME_STR);
    const char *nextAuthor = strstr(temp + 1, _AUTHOR_STR);
    const char *nextTitle = strstr(temp + 1, _TITLE_STR);
    const char *nextArtist = strstr(temp + 1, _ARTIST_STR);
    const char *nextComment = strstr(temp + 1, _COMMENT_STR);

    // If any of these fields is beyond 'end', they are ignored.

    if ((nextName != NULL) && (nextName >= end))
    {
        nextName = NULL;
    }

    if ((nextAuthor != NULL) && (nextAuthor >= end))
    {
        nextAuthor = NULL;
    }

    if ((nextTitle != NULL) && (nextTitle >= end))
    {
        nextTitle = NULL;
    }

    if ((nextArtist != NULL) && (nextArtist >= end))
    {
        nextArtist = NULL;
    }

    if ((nextComment != NULL) && (nextComment >= end))
    {
        nextComment = NULL;
    }

    // Now determine which one is the closest to our field - that one
    // will mark the end of the required field.

    const char *nextField = nextName;

    if (nextField == NULL)
    {
        nextField = nextAuthor;
    }
    else if ((nextAuthor != NULL) && (nextAuthor < nextField))
    {
        nextField = nextAuthor;
    }

    if (nextField == NULL)
    {
        nextField = nextTitle;
    }
    else if ((nextTitle != NULL) && (nextTitle < nextField))
    {
        nextField = nextTitle;
    }

    if (nextField == NULL)
    {
        nextField = nextArtist;
    }
    else if ((nextArtist != NULL) && (nextArtist < nextField))
    {
        nextField = nextArtist;
    }

    if (nextField == NULL)
    {
        nextField = nextComment;
    }
    else if ((nextComment != NULL) && (nextComment < nextField))
    {
        nextField = nextComment;
    }

    if (nextField == NULL)
    {
        nextField = end;
    }

    // Now nextField points to the last+1 char that should be copied to
    // result. Do that.

    result.append(temp, nextField - temp);
    return true;
}

void
STIL::getStilLine(ifstream &infile, string &line)
{
    if (STIL_EOL2 != '\0')
    {
        // If there was a remaining EOL char from the previous read, eat it up.

        char temp = infile.peek();

        if ((temp == 0x0d) || (temp == 0x0a))
        {
            infile.get(temp);
        }
    }

    getline(infile, line, STIL_EOL);
}
