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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <new>

using std::cout;
using std::cerr;
using std::endl;

#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "utils.h"
#include "keyboard.h"
#include "audio/AudioDrv.h"
#include "audio/wav/WavFile.h"
#include "ini/types.h"

#include "sidcxx11.h"

#include <sidplayfp/sidbuilder.h>
#include <sidplayfp/SidInfo.h>
#include <sidplayfp/SidTuneInfo.h>

// Previous song select timeout (3 secs)
#define SID2_PREV_SONG_TIMEOUT 4

#ifdef HAVE_SIDPLAYFP_BUILDERS_RESIDFP_H
#  include <sidplayfp/builders/residfp.h>
const char ConsolePlayer::RESIDFP_ID[] = "ReSIDfp";
#endif

#ifdef HAVE_SIDPLAYFP_BUILDERS_RESID_H
#  include <sidplayfp/builders/resid.h>
const char ConsolePlayer::RESID_ID[] = "ReSID";
#endif

#ifdef HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
#   include <sidplayfp/builders/hardsid.h>
const char ConsolePlayer::HARDSID_ID[] = "HardSID";
#endif

#ifdef HAVE_SIDPLAYFP_BUILDERS_EXSID_H
#   include <sidplayfp/builders/exsid.h>
const char ConsolePlayer::EXSID_ID[] = "exSID";
#endif


uint8_t* loadRom(const SID_STRING &romPath, const int size)
{
    SID_IFSTREAM is(romPath.c_str(), std::ios::binary);

    if (is.is_open())
    {
        try
        {
            uint8_t *buffer = new uint8_t[size];

            is.read((char*)buffer, size);
            if (!is.fail())
            {
                is.close();
                return buffer;
            }
            delete [] buffer;
        }
        catch (std::bad_alloc const &ba) {}
    }

    return nullptr;
}


uint8_t* loadRom(const SID_STRING &romPath, const int size, const TCHAR defaultRom[])
{
    // Try to load given rom
    if (!romPath.empty())
    {
        uint8_t* buffer = loadRom(romPath, size);
        if (buffer)
            return buffer;
    }

    // Fallback to default rom path
    try
    {
        SID_STRING dataPath(utils::getDataPath());

        dataPath.append(SEPARATOR).append(TEXT("sidplayfp")).append(SEPARATOR).append(defaultRom);

#if !defined _WIN32 && defined HAVE_UNISTD_H
        if (::access(dataPath.c_str(), R_OK) != 0)
        {
            dataPath = PKGDATADIR;
            dataPath.append(defaultRom);
        }
#endif

        return loadRom(dataPath, size);
    }
    catch (utils::error const &e)
    {
        return nullptr;
    }
}


ConsolePlayer::ConsolePlayer (const char * const name) :
    m_name(name),
    m_tune(nullptr),
    m_state(playerStopped),
    m_outfile(NULL),
    m_filename(""),
    m_quietLevel(0),
    m_verboseLevel(0),
    m_cpudebug(false)
{   // Other defaults
    m_filter.enabled = true;
    m_driver.device  = NULL;
    m_driver.sid     = EMU_RESIDFP;
    m_timer.start    = 0;
    m_timer.length   = 0; // FOREVER
    m_timer.valid    = false;
    m_track.first    = 0;
    m_track.selected = 0;
    m_track.loop     = false;
    m_track.single   = false;
    m_speed.current  = 1;
    m_speed.max      = 32;

    // Read default configuration
    m_iniCfg.read ();
    m_engCfg = m_engine.config ();

    {   // Load ini settings
        IniConfig::audio_section     audio     = m_iniCfg.audio();
        IniConfig::emulation_section emulation = m_iniCfg.emulation();

        // INI Configuration Settings
        m_engCfg.forceC64Model   = emulation.modelForced;
        m_engCfg.defaultC64Model = emulation.modelDefault;
        m_engCfg.defaultSidModel = emulation.sidModel;
        m_engCfg.forceSidModel   = emulation.forceModel;
        m_engCfg.frequency    = audio.frequency;
        m_engCfg.playback     = audio.playback;
        m_precision           = audio.precision;
        m_filter.enabled      = emulation.filter;
        m_filter.bias         = emulation.bias;
        m_filter.filterCurve6581 = emulation.filterCurve6581;
        m_filter.filterCurve8580 = emulation.filterCurve8580;

        if (!emulation.engine.empty())
        {
            if (emulation.engine.compare(TEXT("RESIDFP")) == 0)
            {
                m_driver.sid    = EMU_RESIDFP;
            }
            else if (emulation.engine.compare(TEXT("RESID")) == 0)
            {
                m_driver.sid    = EMU_RESID;
            }
#ifdef HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
            else if (emulation.engine.compare(TEXT("HARDSID")) == 0)
            {
                m_driver.sid    = EMU_HARDSID;
                m_driver.output = OUT_NULL;
            }
#endif
#ifdef HAVE_SIDPLAYFP_BUILDERS_EXSID_H
            else if (emulation.engine.compare(TEXT("EXSID")) == 0)
            {
                m_driver.sid    = EMU_EXSID;
                m_driver.output = OUT_NULL;
            }
#endif
            else if (emulation.engine.compare(TEXT("NONE")) == 0)
            {
                m_driver.sid    = EMU_NONE;
            }
        }
    }

    createOutput (OUT_NULL, nullptr);
    createSidEmu (EMU_NONE);

    uint8_t *kernalRom = loadRom((m_iniCfg.sidplay2()).kernalRom, 8192, TEXT("kernal"));
    uint8_t *basicRom = loadRom((m_iniCfg.sidplay2()).basicRom, 8192, TEXT("basic"));
    uint8_t *chargenRom = loadRom((m_iniCfg.sidplay2()).chargenRom, 4096, TEXT("chargen"));
    m_engine.setRoms(kernalRom, basicRom, chargenRom);
    delete [] kernalRom;
    delete [] basicRom;
    delete [] chargenRom;
}

IAudio* ConsolePlayer::getWavFile(const SidTuneInfo *tuneInfo)
{
    std::string title;

    if (m_outfile != NULL)
    {
        title = m_outfile;
    }
    else
    {
        // Generate a name for the wav file
        title = tuneInfo->dataFileName();

        title.erase(title.find_last_of('.'));

        // Change name based on subtune
        if (tuneInfo->songs() > 1)
        {
            std::ostringstream sstream;
            sstream << "[" << tuneInfo->currentSong() << "]";
            title.append(sstream.str());
        }
        title.append(WavFile::extension());
    }

    return new WavFile(title);
}

// Create the output object to process sound buffer
bool ConsolePlayer::createOutput (OUTPUTS driver, const SidTuneInfo *tuneInfo)
{
    // Remove old audio driver
    m_driver.null.close ();
    m_driver.selected = &m_driver.null;
    if (m_driver.device != nullptr)
    {
        if (m_driver.device != &m_driver.null)
            delete m_driver.device;
        m_driver.device = nullptr;
    }

    // Create audio driver
    switch (driver)
    {
    case OUT_NULL:
        m_driver.device = &m_driver.null;
    break;

    case OUT_SOUNDCARD:
        try
        {
            m_driver.device = new audioDrv();
        }
        catch (std::bad_alloc const &ba)
        {
            m_driver.device = nullptr;
        }
    break;

    case OUT_WAV:
        try
        {
            m_driver.device = getWavFile(tuneInfo);
        }
        catch (std::bad_alloc const &ba)
        {
            m_driver.device = nullptr;
        }
    break;

    default:
        break;
    }

    // Audio driver failed
    if (!m_driver.device)
    {
        m_driver.device = &m_driver.null;
        displayError (ERR_NOT_ENOUGH_MEMORY);
        return false;
    }

    // Configure with user settings
    m_driver.cfg.frequency = m_engCfg.frequency;
    m_driver.cfg.precision = m_precision;
    m_driver.cfg.channels  = 1; // Mono
    m_driver.cfg.bufSize   = 0; // Recalculate
    if (m_engCfg.playback == SidConfig::STEREO)
        m_driver.cfg.channels = 2;

    {   // Open the hardware
        bool err = false;
        if (!m_driver.device->open (m_driver.cfg))
            err = true;

        // Can't open the same driver twice
        if (driver != OUT_NULL)
        {
            if (!m_driver.null.open (m_driver.cfg))
                err = true;
        }

        if (err)
        {
            displayError(m_driver.device->getErrorString());
            return false;
        }
    }

    // See what we got
    m_engCfg.frequency = m_driver.cfg.frequency;
    m_precision = m_driver.cfg.precision;
    switch (m_driver.cfg.channels)
    {
    case 1:
        if (m_engCfg.playback == SidConfig::STEREO)
            m_engCfg.playback  = SidConfig::MONO;
        break;
    case 2:
        if (m_engCfg.playback != SidConfig::STEREO)
            m_engCfg.playback  = SidConfig::STEREO;
        break;
    default:
        cerr << m_name << ": " << "ERROR: " << m_driver.cfg.channels
             << " audio channels not supported" << endl;
        return false;
    }
    return true;
}


// Create the sid emulation
bool ConsolePlayer::createSidEmu (SIDEMUS emu)
{
    // Remove old driver and emulation
    if (m_engCfg.sidEmulation)
    {
        sidbuilder *builder   = m_engCfg.sidEmulation;
        m_engCfg.sidEmulation = nullptr;
        m_engine.config (m_engCfg);
        delete builder;
    }

    // Now setup the sid emulation
    switch (emu)
    {
#ifdef HAVE_SIDPLAYFP_BUILDERS_RESIDFP_H
    case EMU_RESIDFP:
    {
        try
        {
            ReSIDfpBuilder *rs = new ReSIDfpBuilder( RESIDFP_ID );

            m_engCfg.sidEmulation = rs;
            if (!rs->getStatus()) goto createSidEmu_error;
            rs->create ((m_engine.info ()).maxsids());
            if (!rs->getStatus()) goto createSidEmu_error;

            if (m_filter.filterCurve6581)
                rs->filter6581Curve(m_filter.filterCurve6581);
            if (m_filter.filterCurve8580)
                rs->filter8580Curve((double)m_filter.filterCurve8580);
        }
        catch (std::bad_alloc const &ba) {}
        break;
    }
#endif // HAVE_SIDPLAYFP_BUILDERS_RESIDFP_H

#ifdef HAVE_SIDPLAYFP_BUILDERS_RESID_H
    case EMU_RESID:
    {
        try
        {
            ReSIDBuilder *rs = new ReSIDBuilder( RESID_ID );

            m_engCfg.sidEmulation = rs;
            if (!rs->getStatus()) goto createSidEmu_error;
            rs->create ((m_engine.info ()).maxsids());
            if (!rs->getStatus()) goto createSidEmu_error;

            rs->bias(m_filter.bias);
        }
        catch (std::bad_alloc const &ba) {}
        break;
    }
#endif // HAVE_SIDPLAYFP_BUILDERS_RESID_H

#ifdef HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
    case EMU_HARDSID:
    {
        try
        {
            HardSIDBuilder *hs = new HardSIDBuilder( HARDSID_ID );

            m_engCfg.sidEmulation = hs;
            if (!hs->getStatus()) goto createSidEmu_error;
            hs->create ((m_engine.info ()).maxsids());
            if (!hs->getStatus()) goto createSidEmu_error;
        }
        catch (std::bad_alloc const &ba) {}
        break;
    }
#endif // HAVE_SIDPLAYFP_BUILDERS_HARDSID_H

#ifdef HAVE_SIDPLAYFP_BUILDERS_EXSID_H
    case EMU_EXSID:
    {
        try
        {
            exSIDBuilder *hs = new exSIDBuilder( EXSID_ID );

            m_engCfg.sidEmulation = hs;
            if (!hs->getStatus()) goto createSidEmu_error;
            hs->create ((m_engine.info ()).maxsids());
            if (!hs->getStatus()) goto createSidEmu_error;
        }
        catch (std::bad_alloc const &ba) {}
        break;
    }
#endif // HAVE_SIDPLAYFP_BUILDERS_EXSID_H

    default:
        // Emulation Not yet handled
        // This default case results in the default
        // emulation
        break;
    }

    if (!m_engCfg.sidEmulation)
    {
        if (emu > EMU_DEFAULT)
        {   // No sid emulation?
            displayError (ERR_NOT_ENOUGH_MEMORY);
            return false;
        }
    }

    if (m_engCfg.sidEmulation) {
        /* set up SID filter. HardSID just ignores call with def. */
        m_engCfg.sidEmulation->filter(m_filter.enabled);
    }

    return true;

createSidEmu_error:
    displayError (m_engCfg.sidEmulation->error ());
    delete m_engCfg.sidEmulation;
    m_engCfg.sidEmulation = nullptr;
    return false;
}


bool ConsolePlayer::open (void)
{
    if ((m_state & ~playerFast) == playerRestart)
    {
        if (m_quietLevel < 2)
            cerr << endl;
        if (m_state & playerFast)
            m_driver.selected->reset ();
        m_state = playerStopped;
    }

    // Select the required song
    m_track.selected = m_tune.selectSong (m_track.selected);
    if (!m_engine.load (&m_tune))
    {
        displayError (m_engine.error ());
        return false;
    }

    // Get tune details
    const SidTuneInfo *tuneInfo = m_tune.getInfo ();
    if (!m_track.single)
        m_track.songs = tuneInfo->songs();
    if (!createOutput (m_driver.output, tuneInfo))
        return false;
    if (!createSidEmu (m_driver.sid))
        return false;

    // Configure engine with settings
    if (!m_engine.config (m_engCfg))
    {   // Config failed
        displayError (m_engine.error ());
        return false;
    }

    // Start the player.  Do this by fast
    // forwarding to the start position
    m_driver.selected = &m_driver.null;
    m_speed.current   = m_speed.max;
    m_engine.fastForward (100 * m_speed.current);

    m_engine.mute(0, 0, vMute[0]);
    m_engine.mute(0, 1, vMute[1]);
    m_engine.mute(0, 2, vMute[2]);
    m_engine.mute(1, 0, vMute[3]);
    m_engine.mute(1, 1, vMute[4]);
    m_engine.mute(1, 2, vMute[5]);
    m_engine.mute(2, 0, vMute[6]);
    m_engine.mute(2, 1, vMute[7]);
    m_engine.mute(2, 2, vMute[8]);

    // As yet we don't have a required songlength
    // so try the songlength database or keep the default
    if (!m_timer.valid)
    {
        const int_least32_t length = m_database.length(m_tune);
        if (length > 0)
            m_timer.length = length;
    }

    // Set up the play timer
    m_timer.stop = m_timer.length;

    if (m_timer.valid)
    {   // Length relative to start
        if (m_timer.stop > 0)
            m_timer.stop += m_timer.start;
    }
    else
    {   // Check to make start time dosen't exceed end
        if (m_timer.stop & (m_timer.start >= m_timer.stop))
        {
            displayError ("ERROR: Start time exceeds song length!");
            return false;
        }
    }

    m_timer.current = ~0;
    m_state = playerRunning;

    // Update display
    menu();
    updateDisplay();
    return true;
}

void ConsolePlayer::close ()
{
    m_engine.stop();
    if (m_state == playerExit)
    {   // Natural finish
        emuflush ();
        if (m_driver.file)
            cerr << (char) 7; // Bell
    }
    else // Destroy buffers
        m_driver.selected->reset ();

    // Shutdown drivers, etc
    createOutput    (OUT_NULL, nullptr);
    createSidEmu    (EMU_NONE);
    m_engine.load   (nullptr);
    m_engine.config (m_engCfg);

    if (m_quietLevel < 2)
    {   // Correctly leave ansi mode and get prompt to
        // end up in a suitable location
        if ((m_iniCfg.console ()).ansi)
            cerr << '\x1b' << "[0m";
#ifndef _WIN32
        cerr << endl;
#endif
    }
}

// Flush any hardware sid fifos so all music is played
void ConsolePlayer::emuflush ()
{
    switch (m_driver.sid)
    {
#ifdef HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
    case EMU_HARDSID:
        ((HardSIDBuilder *)m_engCfg.sidEmulation)->flush ();
        break;
#endif // HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
#ifdef HAVE_SIDPLAYFP_BUILDERS_EXSID_H
    case EMU_EXSID:
        ((exSIDBuilder *)m_engCfg.sidEmulation)->flush ();
        break;
#endif // HAVE_SIDPLAYFP_BUILDERS_EXSID_H
    default:
        break;
    }
}


// Out play loop to be externally called
bool ConsolePlayer::play ()
{
    if (m_state == playerRunning)
    {
        updateDisplay();

        // Fill buffer
        short *buffer = m_driver.selected->buffer ();
        const uint_least32_t length = m_driver.cfg.bufSize;
        const uint_least32_t ret = m_engine.play (buffer, length);
        if (ret < length)
        {
            if (m_engine.isPlaying ())
            {
                m_state = playerError;
            }
            return false;
        }
    }

    switch (m_state)
    {
    case playerRunning:
        m_driver.selected->write ();
        // Deliberate run on
    case playerPaused:
        // Check for a keypress (approx 250ms rate, but really depends
        // on music buffer sizes).  Don't do this for high quiet levels
        // as chances are we are under remote control.
        if ((m_quietLevel < 2) && _kbhit ())
            decodeKeys ();
        return true;
    default:
        if (m_quietLevel < 2)
            cerr << endl;
        m_engine.stop ();
#if HAVE_TSID == 1
        if (m_tsid)
        {
            m_tsid.addTime ((int) m_timer.current, m_track.selected,
                            m_filename);
        }
#elif HAVE_TSID == 2
        if (m_tsid)
        {
            char md5[SidTune::MD5_LENGTH + 1];
            m_tune.createMD5 (md5);
            int_least32_t length = m_database.length (md5, m_track.selected);
            // ignore errors
            if (length < 0)
                length = 0;
            m_tsid.addTime (md5, m_filename, (uint) m_timer.current,
                            m_track.selected, (uint) length);
        }
#endif
        break;
    }
    return false;
}


void ConsolePlayer::stop ()
{
    m_state = playerStopped;
    m_engine.stop ();
}


// External Timer Event
void ConsolePlayer::updateDisplay()
{
    const uint_least32_t seconds = m_engine.time();
    if (seconds == m_timer.current)
        return;

    if (!m_quietLevel)
    {
        cerr << "\b\b\b\b\b" << std::setw(2) << std::setfill('0')
             << ((seconds / 60) % 100) << ':' << std::setw(2)
             << std::setfill('0') << (seconds % 60) << std::flush;
    }

    m_timer.current = seconds;

    if (seconds == m_timer.start)
    {   // Switch audio drivers.
        m_driver.selected = m_driver.device;
        memset(m_driver.selected->buffer (), 0, m_driver.cfg.bufSize);
        m_speed.current = 1;
        m_engine.fastForward(100);
        if (m_cpudebug)
            m_engine.debug (true, nullptr);
    }
    else if (m_timer.stop && (seconds >= m_timer.stop))
    {
        m_state = playerExit;
        for (;;)
        {
            if (m_track.single)
                return;
            // Move to next track
            m_track.selected++;
            if (m_track.selected > m_track.songs)
                m_track.selected = 1;
            if (m_track.selected == m_track.first)
                return;
            m_state = playerRestart;
            break;
        }
        if (m_track.loop)
            m_state = playerRestart;
    }
}


void ConsolePlayer::displayError (const char *error)
{
    cerr << m_name << ": " << error << endl;
}


// Keyboard handling
void ConsolePlayer::decodeKeys ()
{
    do
    {
        const int action = keyboard_decode ();
        if (action == A_INVALID)
            continue;

        switch (action)
        {
        case A_RIGHT_ARROW:
            m_state = playerFastRestart;
            if (!m_track.single)
            {
                m_track.selected++;
                if (m_track.selected > m_track.songs)
                    m_track.selected = 1;
            }
        break;

        case A_LEFT_ARROW:
            m_state = playerFastRestart;
            if (!m_track.single)
            {   // Only select previous song if less than timeout
                // else restart current song
                if ((m_engine.time()) < SID2_PREV_SONG_TIMEOUT)
                {
                    m_track.selected--;
                    if (m_track.selected < 1)
                        m_track.selected = m_track.songs;
                }
            }
        break;

        case A_UP_ARROW:
            m_speed.current *= 2;
            if (m_speed.current > m_speed.max)
                m_speed.current = m_speed.max;
            m_engine.fastForward (100 * m_speed.current);
        break;

        case A_DOWN_ARROW:
            m_speed.current = 1;
            m_engine.fastForward (100);
        break;

        case A_HOME:
            m_state = playerFastRestart;
            m_track.selected = 1;
        break;

        case A_END:
            m_state = playerFastRestart;
            m_track.selected = m_track.songs;
        break;

        case A_PAUSED:
            if (m_state == playerPaused)
            {
                cerr << "\b\b\b\b\b\b\b\b\b";
                // Just to make sure PAUSED is removed from screen
                cerr << "         ";
                cerr << "\b\b\b\b\b\b\b\b\b";
                m_state  = playerRunning;
            }
            else
            {
                cerr << " [PAUSED]";
                m_state = playerPaused;
                m_driver.selected->pause ();
            }
        break;

        case A_TOGGLE_VOICE1:
            vMute[0] = !vMute[0];
            m_engine.mute(0, 0, vMute[0]);
        break;

        case A_TOGGLE_VOICE2:
            vMute[1] = !vMute[1];
            m_engine.mute(0, 1, vMute[1]);
        break;

        case A_TOGGLE_VOICE3:
            vMute[2] = !vMute[2];
            m_engine.mute(0, 2, vMute[2]);
        break;

        case A_TOGGLE_VOICE4:
            vMute[3] = !vMute[3];
            m_engine.mute(1, 0, vMute[3]);
        break;

        case A_TOGGLE_VOICE5:
            vMute[4] = !vMute[4];
            m_engine.mute(1, 1, vMute[4]);
        break;

        case A_TOGGLE_VOICE6:
            vMute[5] = !vMute[5];
            m_engine.mute(1, 2, vMute[5]);
        break;

        case A_TOGGLE_VOICE7:
            vMute[6] = !vMute[6];
            m_engine.mute(2, 0, vMute[6]);
        break;

        case A_TOGGLE_VOICE8:
            vMute[7] = !vMute[7];
            m_engine.mute(2, 1, vMute[7]);
        break;

        case A_TOGGLE_VOICE9:
            vMute[8] = !vMute[8];
            m_engine.mute(2, 2, vMute[8]);
        break;

        case A_TOGGLE_FILTER:
            m_filter.enabled = !m_filter.enabled;
            m_engCfg.sidEmulation->filter(m_filter.enabled);
        break;

        case A_QUIT:
            m_state = playerFastExit;
            return;
        break;
        }
    } while (_kbhit ());
}
