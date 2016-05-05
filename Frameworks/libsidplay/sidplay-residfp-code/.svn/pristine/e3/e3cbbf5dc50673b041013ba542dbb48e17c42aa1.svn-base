/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001 Simon White
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

#include "hardsid.h"

#include "sidcxx11.h"

#include <cstring>
#include <string>
#include <memory>
#include <sstream>
#include <algorithm>
#include <new>

#ifdef _WIN32
#  include <iomanip>
#endif

#include "hardsid-emu.h"


#ifdef _WIN32
//**************************************************************************
// Version 1 Interface
typedef BYTE (CALLBACK* HsidDLL1_InitMapper_t) ();

libsidplayfp::HsidDLL2 hsid2 = {0};
#endif

bool HardSIDBuilder::m_initialised = false;
#ifndef _WIN32
unsigned int HardSIDBuilder::m_count = 0;
#endif

HardSIDBuilder::HardSIDBuilder(const char * const name) :
    sidbuilder (name)
{
    if (!m_initialised)
    {
        if (init() < 0)
            return;
        m_initialised = true;
    }
}

HardSIDBuilder::~HardSIDBuilder()
{   // Remove all SID emulations
    remove();
}

// Create a new sid emulation.
unsigned int HardSIDBuilder::create(unsigned int sids)
{
    m_status = true;

    // Check available devices
    unsigned int count = availDevices();
    if (count == 0)
    {
        m_errorBuffer = "HARDSID ERROR: No devices found (run HardSIDConfig)";
        goto HardSIDBuilder_create_error;
    }

    if (count < sids)
        sids = count;

    for (count = 0; count < sids; count++)
    {
        try
        {
            std::unique_ptr<libsidplayfp::HardSID> sid(new libsidplayfp::HardSID(this));

            // SID init failed?
            if (!sid->getStatus())
            {
                m_errorBuffer = sid->error();
                goto HardSIDBuilder_create_error;
            }
            sidobjs.insert(sid.release());
        }
        // Memory alloc failed?
        catch (std::bad_alloc const &)
        {
            m_errorBuffer.assign(name()).append(" ERROR: Unable to create HardSID object");
            goto HardSIDBuilder_create_error;
        }


    }
    return count;

HardSIDBuilder_create_error:
    m_status = false;
    return count;
}

unsigned int HardSIDBuilder::availDevices() const
{
    // Available devices
    // @FIXME@ not yet supported on Linux
#ifdef _WIN32
    return hsid2.Instance ? hsid2.Devices() : 0;
#else
    return m_count;
#endif
}

const char *HardSIDBuilder::credits() const
{
    return libsidplayfp::HardSID::getCredits();
}

void HardSIDBuilder::flush()
{
    for (emuset_t::iterator it=sidobjs.begin(); it != sidobjs.end(); ++it)
        static_cast<libsidplayfp::HardSID*>(*it)->flush();
}

void HardSIDBuilder::filter(bool enable)
{
    std::for_each(sidobjs.begin(), sidobjs.end(), applyParameter<libsidplayfp::HardSID, bool>(&libsidplayfp::HardSID::filter, enable));
}

#ifdef _WIN32

// Load the library and initialise the hardsid
int HardSIDBuilder::init()
{
    HINSTANCE dll;

    if (hsid2.Instance)
        return 0;

    m_status = false;

    dll = LoadLibrary(TEXT("HARDSID.DLL"));

    if (!dll)
    {
        DWORD err = GetLastError();
        if (err == ERROR_DLL_INIT_FAILED)
            m_errorBuffer = "HARDSID ERROR: hardsid.dll init failed!";
        else
            m_errorBuffer = "HARDSID ERROR: hardsid.dll not found!";
        goto HardSID_init_error;
    }

    {   // Export Needed Version 1 Interface
        HsidDLL1_InitMapper_t mapper;
        mapper = (HsidDLL1_InitMapper_t) GetProcAddress(dll, "InitHardSID_Mapper");

        if (mapper)
            mapper();
        else
        {
            m_errorBuffer = "HARDSID ERROR: hardsid.dll is corrupt!";
            goto HardSID_init_error;
        }
    }

    {   // Check for the Version 2 interface
        HsidDLL2_Version_t version;
        version = (HsidDLL2_Version_t) GetProcAddress(dll, "HardSID_Version");
        if (!version)
        {
            m_errorBuffer = "HARDSID ERROR: hardsid.dll not V2";
            goto HardSID_init_error;
        }
        hsid2.Version = version ();
    }

    {
        WORD version = hsid2.Version;
        if ((version >> 8) != (HSID_VERSION_MIN >> 8))
        {
            std::ostringstream ss;
            ss << "HARDSID ERROR: hardsid.dll not V" << (HSID_VERSION_MIN >> 8) << std::endl;
            m_errorBuffer = ss.str();
            goto HardSID_init_error;
        }

        if (version < HSID_VERSION_MIN)
        {
            std::ostringstream ss;
            ss.fill('0');
            ss << "HARDSID ERROR: hardsid.dll hardsid.dll must be V";
            ss << std::setw(2) << (HSID_VERSION_MIN >> 8);
            ss << ".";
            ss << std::setw(2) << (HSID_VERSION_MIN & 0xff);
            ss <<  " or greater" << std::endl;
            m_errorBuffer = ss.str();
            goto HardSID_init_error;
        }
    }

    // Export Needed Version 2 Interface
    hsid2.Delay    = (HsidDLL2_Delay_t)   GetProcAddress(dll, "HardSID_Delay");
    hsid2.Devices  = (HsidDLL2_Devices_t) GetProcAddress(dll, "HardSID_Devices");
    hsid2.Filter   = (HsidDLL2_Filter_t)  GetProcAddress(dll, "HardSID_Filter");
    hsid2.Flush    = (HsidDLL2_Flush_t)   GetProcAddress(dll, "HardSID_Flush");
    hsid2.MuteAll  = (HsidDLL2_MuteAll_t) GetProcAddress(dll, "HardSID_MuteAll");
    hsid2.Read     = (HsidDLL2_Read_t)    GetProcAddress(dll, "HardSID_Read");
    hsid2.Sync     = (HsidDLL2_Sync_t)    GetProcAddress(dll, "HardSID_Sync");
    hsid2.Write    = (HsidDLL2_Write_t)   GetProcAddress(dll, "HardSID_Write");

    if (hsid2.Version < HSID_VERSION_204)
        hsid2.Reset  = (HsidDLL2_Reset_t)  GetProcAddress(dll, "HardSID_Reset");
    else
    {
        hsid2.Lock   = (HsidDLL2_Lock_t)   GetProcAddress(dll, "HardSID_Lock");
        hsid2.Unlock = (HsidDLL2_Unlock_t) GetProcAddress(dll, "HardSID_Unlock");
        hsid2.Reset2 = (HsidDLL2_Reset2_t) GetProcAddress(dll, "HardSID_Reset2");
    }

    if (hsid2.Version < HSID_VERSION_207)
        hsid2.Mute   = (HsidDLL2_Mute_t)   GetProcAddress(dll, "HardSID_Mute");
    else
        hsid2.Mute2  = (HsidDLL2_Mute2_t)  GetProcAddress(dll, "HardSID_Mute2");

    hsid2.Instance = dll;
    m_status       = true;
    return 0;

HardSID_init_error:
    if (dll)
        FreeLibrary(dll);
    return -1;
}

#else

#include <ctype.h>
#include <dirent.h>

// Find the number of sid devices.  We do not care about
// stuppid device numbering or drivers not loaded for the
// available nodes.
int HardSIDBuilder::init()
{
    DIR *dir = opendir("/dev");
    if (!dir)
        return -1;

    m_count = 0;

    while (dirent *entry=readdir(dir))
    {
        // SID device
        if (strncmp ("sid", entry->d_name, 3))
            continue;

        // if it is truely one of ours then it will be
        // followed by numerics only
        const char *p = entry->d_name+3;
        unsigned int index = 0;
        while (*p)
        {
            if (!isdigit(*p))
                continue;
            index = index * 10 + (*p++ - '0');
        }
        index++;
        if (m_count < index)
            m_count = index;
    }

    closedir(dir);
    return 0;
}

#endif // _WIN32
