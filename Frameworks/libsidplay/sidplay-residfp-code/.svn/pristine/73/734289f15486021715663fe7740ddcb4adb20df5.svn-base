/***************************************************************************
      exsid-builder.cpp - exSID builder class for creating/controlling
                               exSIDs.
                               -------------------
   Based on hardsid-builder.cpp (C) 2001 Simon White

    copyright            : (C) 2015 Thibaut VARENE
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <cstring>
#include <string>
#include <memory>
#include <sstream>
#include <algorithm>
#include <new>

#include "exsid.h"
#include "exsid-emu.h"


bool exSIDBuilder::m_initialised = false;
unsigned int exSIDBuilder::m_count = 0;

exSIDBuilder::exSIDBuilder(const char * const name) :
    sidbuilder(name)
{
    if (!m_initialised)
    {
        m_count = 1;
        m_initialised = true;
    }
}

exSIDBuilder::~exSIDBuilder()
{
    // Remove all SID emulations
    remove();
}

// Create a new sid emulation.  Called by libsidplay2 only
unsigned int exSIDBuilder::create(unsigned int sids)
{
    m_status = true;

    // Check available devices
    unsigned int count = availDevices();
    if (count == 0)
    {
        m_errorBuffer.assign(name()).append(" ERROR: No devices found");
        goto exSIDBuilder_create_error;
    }

    if (count < sids)
        sids = count;

    for (count = 0; count < sids; count++)
    {
        try
        {
            std::unique_ptr<libsidplayfp::exSID> sid(new libsidplayfp::exSID(this));

            // SID init failed?
            if (!sid->getStatus())
            {
                m_errorBuffer = sid->error();
                goto exSIDBuilder_create_error;
            }

            sidobjs.insert(sid.release());
        }
        // Memory alloc failed?
        catch (std::bad_alloc const &)
        {
            m_errorBuffer.assign(name()).append(" ERROR: Unable to create exSID object");
            goto exSIDBuilder_create_error;
        }
    }

exSIDBuilder_create_error:
    if (count == 0)
        m_status = false;
    return count;
}

unsigned int exSIDBuilder::availDevices() const
{
    return m_count;
}

const char *exSIDBuilder::credits() const
{
    return libsidplayfp::exSID::getCredits();
}

void exSIDBuilder::flush()
{
    for (emuset_t::iterator it=sidobjs.begin(); it != sidobjs.end(); ++it)
        static_cast<libsidplayfp::exSID*>(*it)->flush();
}

void exSIDBuilder::filter (bool enable)
{
    std::for_each(sidobjs.begin(), sidobjs.end(), applyParameter<libsidplayfp::exSID, bool>(&libsidplayfp::exSID::filter, enable));
}
