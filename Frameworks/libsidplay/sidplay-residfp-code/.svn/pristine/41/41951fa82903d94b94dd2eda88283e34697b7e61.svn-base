/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2012-2015 Leandro Nini
 * Copyright 2000 Simon White
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

#ifdef _WIN32
#  include <conio.h>
#else
    int _kbhit (void);
#endif

enum
{
    A_NONE = 0,

    // Standard Commands
    A_PREFIX,
    A_SKIP,
    A_END_LIST,
    A_INVALID,

    // Custom Commands
    A_LEFT_ARROW,
    A_RIGHT_ARROW,
    A_UP_ARROW,
    A_DOWN_ARROW,
    A_HOME,
    A_END,
    A_PAUSED,
    A_QUIT,

    /* Debug */
    A_TOGGLE_VOICE1,
    A_TOGGLE_VOICE2,
    A_TOGGLE_VOICE3,
    A_TOGGLE_VOICE4,
    A_TOGGLE_VOICE5,
    A_TOGGLE_VOICE6,
    A_TOGGLE_VOICE7,
    A_TOGGLE_VOICE8,
    A_TOGGLE_VOICE9,
    A_TOGGLE_FILTER
};

int  keyboard_decode      ();
void keyboard_enable_raw  ();
void keyboard_disable_raw ();
