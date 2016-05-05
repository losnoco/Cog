/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2011-2016 Leandro Nini
 * Copyright 2000-2001 Simon White
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

#include "player.h"

#include <cstring>
#include <ctype.h>

#include <iostream>
#include <iomanip>

using std::cout;
using std::cerr;
using std::endl;
using std::dec;
using std::hex;
using std::flush;
using std::setw;
using std::setfill;

#include <sidplayfp/SidInfo.h>
#include <sidplayfp/SidTuneInfo.h>


// Display console menu
void ConsolePlayer::menu ()
{
    if (m_quietLevel > 1)
        return;

    const SidInfo &info         = m_engine.info ();
    const SidTuneInfo *tuneInfo = m_tune.getInfo();

    // cerr << (char) 12 << '\f'; // New Page
    if ((m_iniCfg.console ()).ansi)
    {
        cerr << '\x1b' << "[40m";  // Background black
        cerr << '\x1b' << "[2J";   // Clear screen
        cerr << '\x1b' << "[0;0H"; // Move cursor to 0,0
    }

    consoleTable (tableStart);
    consoleTable (tableMiddle);
    consoleColour (red, true);
    cerr << "  SID";
    consoleColour (blue, true);
    cerr << "PLAYFP";
    consoleColour (white, true);
    cerr << " - Music Player and C64 SID Chip Emulator" << endl;
    consoleTable  (tableMiddle);
    consoleColour (white, false);
    {
        cerr << setw(19) << "Sidplayfp" << " V" << VERSION << ", ";
        cerr << (char) toupper (*info.name());
        cerr << info.name() + 1 << " V" << info.version() << endl;
    }

    const unsigned int n = tuneInfo->numberOfInfoStrings();
    if (n)
    {
        consoleTable (tableSeparator);

        consoleTable  (tableMiddle);
        consoleColour (cyan, true);
        cerr << " Title        : ";
        consoleColour (magenta, true);
        cerr << tuneInfo->infoString(0) << endl;
        if (n>1)
        {
            consoleTable  (tableMiddle);
            consoleColour (cyan, true);
            cerr << " Author       : ";
            consoleColour (magenta, true);
            cerr << tuneInfo->infoString(1) << endl;
            consoleTable  (tableMiddle);
            consoleColour (cyan, true);
            cerr << " Released     : ";
            consoleColour (magenta, true);
            cerr << tuneInfo->infoString(2) << endl;
        }
    }

    for (unsigned int i = 0; i < tuneInfo->numberOfCommentStrings(); i++)
    {
        consoleTable  (tableMiddle);
        consoleColour (cyan, true);
        cerr << " Comment      : ";
        consoleColour (magenta, true);
        cerr << tuneInfo->commentString(i) << endl;
    }

    consoleTable (tableSeparator);

    if (m_verboseLevel)
    {
        consoleTable  (tableMiddle);
        consoleColour (green, true);
        cerr << " File format  : ";
        consoleColour (white, true);
        cerr << tuneInfo->formatString() << endl;
        consoleTable  (tableMiddle);
        consoleColour (green, true);
        cerr << " Filename(s)  : ";
        consoleColour (white, true);
        cerr << tuneInfo->dataFileName() << endl;
        // Second file is only sometimes present
        if (tuneInfo->infoFileName())
        {
            consoleTable  (tableMiddle);
            consoleColour (green, true);
            cerr << "              : ";
            consoleColour (white, true);
            cerr << tuneInfo->infoFileName() << endl;
        }
        consoleTable  (tableMiddle);
        consoleColour (green, true);
        cerr << " Condition    : ";
        consoleColour (white, true);
        cerr << m_tune.statusString() << endl;

#if HAVE_TSID == 1
        if (!m_tsid)
        {
            consoleTable  (tableMiddle);
            consoleColour (green, true);
            cerr << " TSID Error   : ";
            consoleColour (white, true);
            cerr << m_tsid.getError () << endl;
        }
#endif // HAVE_TSID
    }

    consoleTable  (tableMiddle);
    consoleColour (green, true);
    cerr << " Playlist     : ";
    consoleColour (white, true);

    {   // This will be the format used for playlists
        int i = 1;
        if (!m_track.single)
        {
            i  = m_track.selected;
            i -= (m_track.first - 1);
            if (i < 1)
                i += m_track.songs;
        }
        cerr << i << '/' << m_track.songs;
        cerr << " (tune " << tuneInfo->currentSong() << '/'
             << tuneInfo->songs() << '['
             << tuneInfo->startSong() << "])";
    }

    if (m_track.loop)
        cerr << " [LOOPING]";
    cerr << endl;

    if (m_verboseLevel)
    {
        consoleTable  (tableMiddle);
        consoleColour (green, true);
        cerr << " Song Speed   : ";
        consoleColour (white, true);
        cerr << info.speedString() << endl;
    }

    consoleTable  (tableMiddle);
    consoleColour (green, true);
    cerr << " Song Length  : ";
    consoleColour (white, true);
    if (m_timer.stop)
        cerr << setw(2) << setfill('0') << ((m_timer.stop / 60) % 100)
             << ':' << setw(2) << setfill('0') << (m_timer.stop % 60);
    else if (m_timer.valid)
        cerr << "FOREVER";
    else
        cerr << "UNKNOWN";
    if (m_timer.start)
    {   // Show offset
        cerr << " (+" << setw(2) << setfill('0') << ((m_timer.start / 60) % 100)
             << ':' << setw(2) << setfill('0') << (m_timer.start % 60) << ")";
    }
    cerr << endl;

    if (m_verboseLevel)
    {
        consoleTable  (tableSeparator);

        consoleTable  (tableMiddle);
        consoleColour (yellow, true);
        cerr << " Addresses    : " << hex;
        cerr.setf(std::ios::uppercase);
        consoleColour (white, false);
        // Display PSID Driver location
        cerr << "DRIVER = ";
        if (info.driverAddr() == 0)
            cerr << "NOT PRESENT";
        else
        {
            cerr << "$"  << setw(4) << setfill('0') << info.driverAddr();
            cerr << "-$" << setw(4) << setfill('0') << info.driverAddr() +
                (info.driverLength() - 1);
        }
        if (tuneInfo->playAddr() == 0xffff)
            cerr << ", SYS = $" << setw(4) << setfill('0') << tuneInfo->initAddr();
        else
            cerr << ", INIT = $" << setw(4) << setfill('0') << tuneInfo->initAddr();
        cerr << endl;
        consoleTable  (tableMiddle);
        consoleColour (yellow, true);
        cerr << "              : ";
        consoleColour (white, false);
        cerr << "LOAD   = $" << setw(4) << setfill('0') << tuneInfo->loadAddr();
        cerr << "-$"         << setw(4) << setfill('0') << tuneInfo->loadAddr() +
            (tuneInfo->c64dataLen() - 1);
        if (tuneInfo->playAddr() != 0xffff)
            cerr << ", PLAY = $" << setw(4) << setfill('0') << tuneInfo->playAddr();
        cerr << dec << endl;
        cerr.unsetf(std::ios::uppercase);

        consoleTable  (tableMiddle);
        consoleColour (yellow, true);
        cerr << " SID Details  : ";
        consoleColour (white, false);
        cerr << "Filter = "
             << ((m_filter.enabled == true) ? "Yes" : "No");
        cerr << ", Model = ";
#if LIBSIDPLAYFP_VERSION_MAJ > 1 || LIBSIDPLAYFP_VERSION_MIN >= 8
        cerr << getModel(tuneInfo->sidModel(0));
#else
        cerr << getModel(tuneInfo->sidModel1());
#endif
        cerr << endl;
#if LIBSIDPLAYFP_VERSION_MAJ > 1 || LIBSIDPLAYFP_VERSION_MIN >= 8
        if (tuneInfo->sidChips() > 1)
#else
        if (tuneInfo->isStereo())
#endif
        {
            consoleTable  (tableMiddle);
            consoleColour (yellow, true);
            cerr << "              : ";
            consoleColour (white, false);
#if LIBSIDPLAYFP_VERSION_MAJ > 1 || LIBSIDPLAYFP_VERSION_MIN >= 8
            cerr << "2nd SID = $" << hex << tuneInfo->sidChipBase(1) << dec;
            cerr << ", Model = " << getModel(tuneInfo->sidModel(1));
#else
            cerr << "2nd SID = $" << hex << tuneInfo->sidChipBase2() << dec;
            cerr << ", Model = " << getModel(tuneInfo->sidModel2());
#endif
            cerr << endl;
#if LIBSIDPLAYFP_VERSION_MAJ > 1 || LIBSIDPLAYFP_VERSION_MIN >= 8
            if (tuneInfo->sidChips() > 2)
            {
                consoleTable  (tableMiddle);
                consoleColour (yellow, true);
                cerr << "              : ";
                consoleColour (white, false);
                cerr << "3rd SID = $" << hex << tuneInfo->sidChipBase(2) << dec;
                cerr << ", Model = " << getModel(tuneInfo->sidModel(2));
                cerr << endl;
            }
#endif
        }

        if (m_verboseLevel > 1)
        {
            consoleTable  (tableMiddle);
            consoleColour (yellow, true);
            cerr << " Delay        : ";
            consoleColour (white, false);
            cerr << info.powerOnDelay() << " (cycles at poweron)" << endl;
        }
    }

    const char* romDesc = info.kernalDesc();

    consoleTable  (tableSeparator);

    consoleTable  (tableMiddle);
    consoleColour (magenta, true);
    cerr << " Kernal ROM   : ";
    if (strlen(romDesc) == 0)
    {
        consoleColour (red, false);
        cerr << "None - Some tunes may not play!";
    }
    else
    {
        consoleColour (white, false);
        cerr << romDesc;
    }
    cerr << endl;

    romDesc = info.basicDesc();

    consoleTable  (tableMiddle);
    consoleColour (magenta, true);
    cerr << " BASIC ROM    : ";
    if (strlen(romDesc) == 0)
    {
        consoleColour (red, false);
        cerr << "None - Basic tunes will not play!";
    }
    else
    {
        consoleColour (white, false);
        cerr << romDesc;
    }
    cerr << endl;

    romDesc = info.chargenDesc();

    consoleTable  (tableMiddle);
    consoleColour (magenta, true);
    cerr << " Chargen ROM  : ";
    if (strlen(romDesc) == 0)
    {
        consoleColour (red, false);
        cerr << "None";
    }
    else
    {
        consoleColour (white, false);
        cerr << romDesc;
    }
    cerr << endl;

    consoleTable (tableEnd);

    if (m_driver.file)
        cerr << "Creating audio file, please wait...";
    else
        cerr << "Playing, press ESC to stop...";

    // Get all the text to the screen so music playback
    // is not disturbed.
    if ( !m_quietLevel )
        cerr << "00:00";
    cerr << flush;
}

// Set colour of text on console
void ConsolePlayer::consoleColour (player_colour_t colour, bool bold)
{
    if ((m_iniCfg.console ()).ansi)
    {
        const char *mode = "";

        switch (colour)
        {
        case black:   mode = "30"; break;
        case red:     mode = "31"; break;
        case green:   mode = "32"; break;
        case yellow:  mode = "33"; break;
        case blue:    mode = "34"; break;
        case magenta: mode = "35"; break;
        case cyan:    mode = "36"; break;
        case white:   mode = "37"; break;
        }

        if (bold)
            cerr << '\x1b' << "[1;40;" << mode << 'm';
        else
            cerr << '\x1b' << "[0;40;" << mode << 'm';
    }
}

// Display menu outline
void ConsolePlayer::consoleTable (player_table_t table)
{
    const unsigned int tableWidth = 54;

    consoleColour (white, true);
    switch (table)
    {
    case tableStart:
        cerr << (m_iniCfg.console ()).topLeft << setw(tableWidth)
             << setfill ((m_iniCfg.console ()).horizontal) << ""
             << (m_iniCfg.console ()).topRight;
        break;

    case tableMiddle:
        cerr << setw(tableWidth + 1) << setfill(' ') << ""
             << (m_iniCfg.console ()).vertical << '\r'
             << (m_iniCfg.console ()).vertical;
        return;

    case tableSeparator:
        cerr << (m_iniCfg.console ()).junctionRight << setw(tableWidth)
             << setfill ((m_iniCfg.console ()).horizontal) << ""
             << (m_iniCfg.console ()).junctionLeft;
        break;

    case tableEnd:
        cerr << (m_iniCfg.console ()).bottomLeft << setw(tableWidth)
             << setfill ((m_iniCfg.console ()).horizontal) << ""
             << (m_iniCfg.console ()).bottomRight;
        break;
    }

    // Move back to begining of row and skip first char
    cerr << "\n";
}


// Restore Ansi Console to defaults
void ConsolePlayer::consoleRestore ()
{
    if ((m_iniCfg.console ()).ansi)
        cerr << '\x1b' << "[0m";
}

const char* ConsolePlayer::getModel (SidTuneInfo::model_t model)
{
    switch (model)
    {
    default:
    case SidTuneInfo::SIDMODEL_UNKNOWN:
        return "UNKNOWN";
    case SidTuneInfo::SIDMODEL_6581:
        return "6581";
    case SidTuneInfo::SIDMODEL_8580:
        return "8580";
    case SidTuneInfo::SIDMODEL_ANY:
        return "ANY";
    }
}
