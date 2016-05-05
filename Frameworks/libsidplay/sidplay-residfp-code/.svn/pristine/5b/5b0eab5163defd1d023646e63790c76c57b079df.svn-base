/***************************************************************************
               exsid.h  -  exSID support interface.
                             -------------------
   Based on hardsid.h (C) 2000-2002 Simon White

    copyright            : (C) 2015 Thibaut VARENE
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/

#ifndef  EXSID_H
#define  EXSID_H

#include "sidplayfp/sidbuilder.h"
#include "sidplayfp/siddefs.h"

class SID_EXTERN exSIDBuilder : public sidbuilder
{
private:
    static bool m_initialised;

    static unsigned int m_count;

public:
    exSIDBuilder(const char * const name);
    ~exSIDBuilder();

    /**
     * Available sids.
     *
     * @return the number of available sids, 0 = endless.
     */
    unsigned int availDevices() const;

    const char *credits() const;
    void flush();

    /**
     * enable/disable filter.
     */
    void filter(bool enable);

    /**
     * Create the sid emu.
     *
     * @param sids the number of required sid emu
     */
    unsigned int create(unsigned int sids);
};

#endif // EXSID_H
