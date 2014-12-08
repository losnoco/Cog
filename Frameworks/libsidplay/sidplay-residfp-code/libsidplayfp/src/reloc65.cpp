/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright (C) 2013-2014 Leandro Nini
 * Copyright (C) 2001 Dag Lem
 * Copyright (C) 1989-1997 André Fachat (a.fachat@physik.tu-chemnitz.de)
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "reloc65.h"

#include <cstring>

#include "sidplayfp/siddefs.h"

/// 16 bit header
const int HEADER_SIZE = (8 + 9 * 2);

/// Magic number
const unsigned char o65hdr[] = {1, 0, 'o', '6', '5'};

/**
 * Get the size of header options section.
 *
 * @param buf
 */
int read_options(unsigned char *buf)
{
    int l = 0;

    unsigned char c = buf[0];
    while (c)
    {
        l += c;
        c = buf[l];
    }
    return ++l;
}

/**
 * Get the size of undefined references list.
 *
 * @param buf
 */
int read_undef(unsigned char *buf)
{
    int l = 2;

    int n = buf[0] + 256 * buf[1];
    while (n)
    {
        n--;
        while (!buf[l++]) {}
    }
    return l;
}

reloc65::reloc65() :
    m_tbase(0),
    m_dbase(0),
    m_bbase(0),
    m_zbase(0),
    m_tflag(false),
    m_dflag(false),
    m_bflag(false),
    m_zflag(false),
    m_extract(WHOLE) {}

void reloc65::setReloc(segment_t type, int addr)
{
    switch (type)
    {
    case TEXT:
        m_tflag = true;
        m_tbase = addr;
        break;
    case DATA:
        m_dflag = true;
        m_dbase = addr;
        break;
    case BSS:
        m_bflag = true;
        m_bbase = addr;
        break;
    case ZEROPAGE:
        m_zflag = true;
        m_zbase = addr;
        break;
    default:
        break;
    }
}

void reloc65::setExtract(segment_t type)
{
    m_extract = type;
}

bool reloc65::reloc(unsigned char **buf, int *fsize)
{
    unsigned char *tmpBuf = *buf;

    if (memcmp(tmpBuf, o65hdr, 5) != 0)
    {
        return false;
    }

    const int mode = tmpBuf[6] + 256 * tmpBuf[7];
    if ((mode & 0x2000)     // 32 bit size not supported
        || (mode & 0x4000)) // pagewise relocation not supported
    {
        return false;
    }

    const int hlen = HEADER_SIZE + read_options(tmpBuf + HEADER_SIZE);

    const int tbase = tmpBuf[ 8] + 256 * tmpBuf[ 9];
    const int tlen  = tmpBuf[10] + 256 * tmpBuf[11];
    m_tdiff = m_tflag ? m_tbase - tbase : 0;
    const int dbase = tmpBuf[12] + 256 * tmpBuf[13];
    const int dlen  = tmpBuf[14] + 256 * tmpBuf[15];
    m_ddiff = m_dflag ? m_dbase - dbase : 0;
    const int bbase = tmpBuf[16] + 256 * tmpBuf[17];
    const int blen SID_UNUSED = tmpBuf[18] + 256 * tmpBuf[19];
    m_bdiff = m_bflag ? m_bbase - bbase : 0;
    const int zbase = tmpBuf[20] + 256 * tmpBuf[21];
    const int zlen SID_UNUSED = tmpBuf[21] + 256 * tmpBuf[23];
    m_zdiff = m_zflag ? m_zbase - zbase : 0;

    unsigned char *segt = tmpBuf + hlen;                    // Text segment
    unsigned char *segd = segt + tlen;                      // Data segment
    unsigned char *utab = segd + dlen;                      // Undefined references list

    unsigned char *rttab = utab + read_undef(utab);         // Text relocation table

    unsigned char *rdtab = reloc_seg(segt, tlen, rttab);    // Data relocation table
    unsigned char *extab = reloc_seg(segd, dlen, rdtab);    // Exported globals list

    reloc_globals(extab);

    if (m_tflag)
    {
        tmpBuf[ 8] = m_tbase & 255;
        tmpBuf[ 9] = (m_tbase >> 8) & 255;
    }
    if (m_dflag)
    {
        tmpBuf[12] = m_dbase & 255;
        tmpBuf[13] = (m_dbase >> 8) & 255;
    }
    if (m_bflag)
    {
        tmpBuf[16] = m_bbase & 255;
        tmpBuf[17] = (m_bbase >> 8) & 255;
    }
    if (m_zflag)
    {
        tmpBuf[20] = m_zbase & 255;
        tmpBuf[21] = (m_zbase >> 8) & 255;
    }

    switch (m_extract)
    {
    case WHOLE:
        return true;
    case TEXT:
        *buf = segt;
        *fsize = tlen;
        return true;
    case DATA:
        *buf = segd;
        *fsize = dlen;
        return true;
    default:
        return false;
    }
}

int reloc65::reldiff(unsigned char s)
{
    switch (s)
    {
    case 2: return m_tdiff;
    case 3: return m_ddiff;
    case 4: return m_bdiff;
    case 5: return m_zdiff;
    default: return 0;
    }
}

unsigned char* reloc65::reloc_seg(unsigned char *buf, int len, unsigned char *rtab)
{
    int adr = -1;
    while (*rtab)
    {
        if ((*rtab & 255) == 255)
        {
            adr += 254;
            rtab++;
        }
        else
        {
            adr += *rtab & 255;
            rtab++;
            const unsigned char type = *rtab & 0xe0;
            unsigned char seg = *rtab & 0x07;
            rtab++;
            switch(type)
            {
            case 0x80: {
                const int oldVal = buf[adr] + 256 * buf[adr+1];
                const int newVal = oldVal + reldiff(seg);
                buf[adr] = newVal & 255;
                buf[adr + 1] = (newVal >> 8) & 255;
                break; }
            case 0x40: {
                const int oldVal = buf[adr] * 256 + *rtab;
                const int newVal = oldVal + reldiff(seg);
                buf[adr] = (newVal >> 8) & 255;
                *rtab = newVal & 255;
                rtab++;
                break; }
            case 0x20: {
                const int oldVal = buf[adr];
                const int newVal = oldVal + reldiff(seg);
                buf[adr] = newVal & 255;
                break; }
            }
            if (seg == 0)
            {
                rtab += 2;
            }
        }

        if (adr > len)
        {
                // Warning: relocation table entries past segment end!
        }
    }
    return ++rtab;
}

unsigned char *reloc65::reloc_globals(unsigned char *buf)
{
    int n = buf[0] + 256 * buf[1];
    buf +=2;

    while (n)
    {
        while (*(buf++)) {}
        unsigned char seg = *buf;
        const int oldVal = buf[1] + 256 * buf[2];
        const int newVal = oldVal + reldiff(seg);
        buf[1] = newVal & 255;
        buf[2] = (newVal >> 8) & 255;
        buf +=3;
        n--;
    }
    return buf;
}
