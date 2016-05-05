/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2011-2015 Leandro Nini
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

#ifndef PLAYER_H
#define PLAYER_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string>

#include <sidplayfp/SidTune.h>
#include <sidplayfp/sidplayfp.h>
#include <sidplayfp/SidConfig.h>
#include <sidplayfp/SidTuneInfo.h>
#include <sidplayfp/SidDatabase.h>

#include "audio/IAudio.h"
#include "audio/AudioConfig.h"
#include "audio/null/null.h"
#include "IniConfig.h"

#ifdef HAVE_TSID
#  if HAVE_TSID > 1
#    include <tsid2/tsid2.h>
#    define TSID TSID2
#  else
#    include <tsid/tsid.h>
#  endif
#endif

typedef enum { black, red, green, yellow, blue, magenta, cyan, white }
    player_colour_t;
typedef enum { tableStart, tableMiddle, tableSeparator, tableEnd }
    player_table_t;
typedef enum
{
    playerError = 0, playerRunning, playerPaused, playerStopped,
    playerRestart, playerExit, playerFast = 128,
    playerFastRestart = playerRestart | playerFast,
    playerFastExit = playerExit | playerFast
} player_state_t;

typedef enum
{
    /* Same as EMU_DEFAULT except no soundcard.
    Still allows wav generation */
    EMU_NONE = 0,
    /* The following require a soundcard */
    EMU_DEFAULT, EMU_RESIDFP, EMU_RESID,
    /* The following should disable the soundcard */
    EMU_HARDSID, EMU_EXSID, EMU_SIDSTATION, EMU_COMMODORE,
    EMU_SIDSYN, EMU_END} SIDEMUS;

typedef enum
{
    /* Define possible output sources */
    OUT_NULL = 0,
    /* Hardware */
    OUT_SOUNDCARD,
    /* File creation support */
    OUT_WAV, OUT_AU, OUT_END
} OUTPUTS;

// Error and status message numbers.
enum
{
    ERR_SYNTAX = 0,
    ERR_NOT_ENOUGH_MEMORY,
    ERR_SIGHANDLER,
    ERR_FILE_OPEN
};

void displayError (const char *arg0, unsigned int num);


// Grouped global variables
class ConsolePlayer
{
private:
#ifdef HAVE_SIDPLAYFP_BUILDERS_RESIDFP_H
    static const char  RESIDFP_ID[];
#endif
#ifdef HAVE_SIDPLAYFP_BUILDERS_RESID_H
    static const char  RESID_ID[];
#endif
#ifdef HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
    static const char  HARDSID_ID[];
#endif
#ifdef HAVE_SIDPLAYFP_BUILDERS_EXSID_H
    static const char  EXSID_ID[];
#endif
#ifdef HAVE_TSID
    TSID               m_tsid;
#endif

    const char* const  m_name;
    sidplayfp          m_engine;
    SidConfig          m_engCfg;
    SidTune            m_tune;
    player_state_t     m_state;
    const char*        m_outfile;
    std::string        m_filename;

    IniConfig          m_iniCfg;
    SidDatabase        m_database;

    // Display parameters
    uint_least8_t      m_quietLevel;
    uint_least8_t      m_verboseLevel;

    bool               m_cpudebug;

    bool vMute[9];

    int  m_precision;

    struct m_filter_t
    {
        // Filter parameter for reSID
        double         bias;
        // Filter parameters for reSIDfp
        double         filterCurve6581;
        int            filterCurve8580;

        bool           enabled;
    } m_filter;

    struct m_driver_t
    {
        OUTPUTS        output;   // Selected output type
        SIDEMUS        sid;      // Sid emulation
        bool           file;     // File based driver
        AudioConfig    cfg;
        IAudio*        selected; // Selected Output Driver
        IAudio*        device;   // HW/File Driver
        Audio_Null     null;     // Used for everything
    } m_driver;

    struct m_timer_t
    {   // secs
        uint_least32_t start;
        uint_least32_t current;
        uint_least32_t stop;
        uint_least32_t length;
        bool           valid;
    } m_timer;

    struct m_track_t
    {
        uint_least16_t first;
        uint_least16_t selected;
        uint_least16_t songs;
        bool           loop;
        bool           single;
    } m_track;

    struct m_speed_t
    {
        uint_least8_t current;
        uint_least8_t max;
    } m_speed;

private:
    // Console
    void consoleColour  (player_colour_t colour, bool bold);
    void consoleTable   (player_table_t table);
    void consoleRestore (void);

    // Command line args
    bool parseTime        (const char *str, uint_least32_t &time);
    bool parseAddress      (const char *str, uint_least16_t &address);
    void displayArgs      (const char *arg = NULL);
    void displayDebugArgs ();

    bool createOutput   (OUTPUTS driver, const SidTuneInfo *tuneInfo);
    bool createSidEmu   (SIDEMUS emu);
    void displayError   (const char *error);
    void displayError   (unsigned int num) { ::displayError (m_name, num); }
    void decodeKeys     (void);
    void updateDisplay();
    void emuflush       (void);
    void menu           (void);

    const char* getModel (SidTuneInfo::model_t model);

    IAudio* getWavFile(const SidTuneInfo *tuneInfo);

    inline bool tryOpenTune(const char *hvscBase);
    inline bool tryOpenDatabase(const char *hvscBase);

public:
    ConsolePlayer (const char * const name);
    virtual ~ConsolePlayer() {}

    int            args  (int argc, const char *argv[]);
    bool           open  (void);
    void           close (void);
    bool           play  (void);
    void           stop  (void);
    player_state_t state (void) { return m_state; }
};

#endif // PLAYER_H
