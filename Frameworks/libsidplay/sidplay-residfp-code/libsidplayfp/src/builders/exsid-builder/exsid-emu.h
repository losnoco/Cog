/***************************************************************************
                   exsid-emu.h - exSID support interface.
                             -------------------
   Based on hardsid-emu.h (C) 2000-2002 Simon White, (C) 2001-2002 Jarno Paananen

    copyright            : (C) 2015 Thibaut VARENE
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/

#ifndef EXSID_EMU_H
#define EXSID_EMU_H

#include "sidemu.h"
#include "Event.h"
#include "EventScheduler.h"
#include "sidplayfp/siddefs.h"

#include "sidcxx11.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

namespace libsidplayfp
{

#define EXSID_VOICES 3

// Approx 30ms
#define EXSID_DELAY_CYCLES 50480

/***************************************************************************
 * exSID SID Specialisation
 ***************************************************************************/
class exSID final : public sidemu
{
private:
    //friend class HardSIDBuilder;

    // exSID specific data
    static unsigned int sid;

    bool m_status;

    bool readflag;

    uint8_t busValue;

    bool muted[3];

private:
    unsigned int delay();

public:
    static const char* getCredits();

public:
    exSID(sidbuilder *builder);
    ~exSID();

    bool getStatus() const { return m_status; }

    uint8_t read(uint_least8_t addr) override;
    void write(uint_least8_t addr, uint8_t data) override;

    // c64sid functions
    void reset(uint8_t volume) override;

    // Standard SID functions
    void clock() override;

    void model(SidConfig::sid_model_t model) override;

    void voice(unsigned int num, bool mute) override;

    void filter(bool enable) {}

    // exSID specific
    void flush();

    // Must lock the SID before using the standard functions.
    bool lock(EventScheduler *env) override;
    void unlock() override;
};

}

#endif // EXSID_EMU_H
