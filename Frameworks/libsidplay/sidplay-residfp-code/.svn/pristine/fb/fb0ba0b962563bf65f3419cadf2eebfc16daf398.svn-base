/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
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

#include "sidbuilder.h"

#include <algorithm>

#include "sidemu.h"

#include "sidcxx11.h"

libsidplayfp::sidemu *sidbuilder::lock(libsidplayfp::EventScheduler *env, SidConfig::sid_model_t model)
{
    m_status = true;

    for (emuset_t::iterator it=sidobjs.begin(); it != sidobjs.end(); ++it)
    {
        libsidplayfp::sidemu *sid = (*it);
        if (sid->lock(env))
        {
            sid->model(model);
            return sid;
        }
    }

    // Unable to locate free SID
    m_status = false;
    m_errorBuffer.assign(name()).append(" ERROR: No available SIDs to lock");
    return nullptr;
}

void sidbuilder::unlock(libsidplayfp::sidemu *device)
{
    emuset_t::iterator it = sidobjs.find(device);
    if (it != sidobjs.end())
    {
        (*it)->unlock();
    }
}

template<class T>
void Delete(T s) { delete s; }

void sidbuilder::remove()
{
    std::for_each(sidobjs.begin(), sidobjs.end(), Delete<emuset_t::value_type>);

    sidobjs.clear();
}
