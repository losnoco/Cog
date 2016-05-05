/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2012-2016 Leandro Nini
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

#include "keyboard.h"

#include "sidcxx11.h"

#ifndef _WIN32
// Unix console headers
#  include <ctype.h>
// bzero requires memset on some platforms
#  include <string.h>
#  include <termios.h>
#  include <sys/time.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#  include <unistd.h>
int _getch (void);
#endif

#define MAX_CMDLEN 10
#define ESC '\033'

// Special Extended Key Definitions
enum
{
    PCK_HOME          = '\107',
    PCK_UP            = '\110',
    PCK_LEFT          = '\113',
    PCK_RIGHT         = '\115',
    PCK_END           = '\117',
    PCK_DOWN          = '\120',
    PCK_EXTENDED      = '\340'
};

static char keytable[] =
{
    // Windows Special Cursors
#ifdef _WIN32
    PCK_EXTENDED, PCK_RIGHT,0,    A_RIGHT_ARROW,
    PCK_EXTENDED, PCK_LEFT,0,     A_LEFT_ARROW,
    PCK_EXTENDED, PCK_UP,0,       A_UP_ARROW,
    PCK_EXTENDED, PCK_DOWN,0,     A_DOWN_ARROW,
    PCK_EXTENDED, PCK_HOME,0,     A_HOME,
    PCK_EXTENDED, PCK_END,0,      A_END,
#else
    // Linux Special Keys
    ESC,'[','C',0,          A_RIGHT_ARROW,
    ESC,'[','D',0,          A_LEFT_ARROW,
    ESC,'[','A',0,          A_UP_ARROW,
    ESC,'[','B',0,          A_DOWN_ARROW,
    // Hmm, in consile there:
    ESC,'[','1','~',0,      A_HOME,
    ESC,'[','4','~',0,      A_END,
    // But in X there:
    ESC,'[','H',0,          A_HOME,
    ESC,'[','F',0,          A_END,

    ESC,'[','1','0',0,      A_INVALID,
    ESC,'[','2','0',0,      A_INVALID,
#endif

    /* debug keys. Just use the cursor keys in linux to move in the song */
    '1',0,                  A_TOGGLE_VOICE1,
    '2',0,                  A_TOGGLE_VOICE2,
    '3',0,                  A_TOGGLE_VOICE3,
    '4',0,                  A_TOGGLE_VOICE4,
    '5',0,                  A_TOGGLE_VOICE5,
    '6',0,                  A_TOGGLE_VOICE6,
    '7',0,                  A_TOGGLE_VOICE7,
    '8',0,                  A_TOGGLE_VOICE8,
    '9',0,                  A_TOGGLE_VOICE9,
    'f',0,                  A_TOGGLE_FILTER,

    // General Keys
    'p',0,                  A_PAUSED,
    ESC,ESC,0,              A_QUIT,

    // Old Keys
    '>',0,                  A_RIGHT_ARROW,
    '<',0,                  A_LEFT_ARROW,
    '.',0,                  A_RIGHT_ARROW,
    ',',0,                  A_LEFT_ARROW,

    0,                      A_END_LIST
};


/*
 * Search a single command table for the command string in cmd.
 */
static int keyboard_search (char *cmd)
{
    char *p;
    char *q;
    int   a;

    for (p = keytable, q = cmd;;  p++, q++)
    {
        if (*p == *q)
        {
            /*
             * Current characters match.
             * If we're at the end of the string, we've found it.
             * Return the action code, which is the character
             * after the null at the end of the string
             * in the command table.
             */
            if (*p == '\0')
            {
                a = *++p & 0377;
                while (a == A_SKIP)
                    a = *++p & 0377;
                if (a == A_END_LIST)
                {
                    /*
                     * We get here only if the original
                     * cmd string passed in was empty ("").
                     * I don't think that can happen,
                     * but just in case ...
                     */
                    break;
                }
                return (a);
            }
        }
        else if (*q == '\0')
        {
            /*
             * Hit the end of the user's command,
             * but not the end of the string in the command table.
             * The user's command is incomplete.
             */
            return (A_PREFIX);
        }
        else
        {
            /*
             * Not a match.
             * Skip ahead to the next command in the
             * command table, and reset the pointer
             * to the beginning of the user's command.
             */
            if (*p == '\0' && p[1] == A_END_LIST)
            {
                /*
                 * A_END_LIST is a special marker that tells
                 * us to abort the cmd search.
                 */
                break;
            }
            while (*p++ != '\0')
                continue;
            while (*p == A_SKIP)
                p++;
            q = cmd-1;
        }
    }
    /*
     * No match found in the entire command table.
     */
    return (A_INVALID);
}

int keyboard_decode ()
{
    char cmd[MAX_CMDLEN+1];
    int  nch = 0;
    int  action = A_NONE;

    /*
     * Collect characters in a buffer.
     * Start with the one we have, and get more if we need them.
     */
    int c = _getch();
    if (c == '\0')
        c = '\340'; // 224
    else if (c == ESC)
    {
        cmd[nch++] = c;
        if (_kbhit ())
            c = _getch ();
    }

    while (c >= 0)
    {
        cmd[nch++] = c;
        cmd[nch]   = '\0';
        action     = keyboard_search (cmd);

        if (action != A_PREFIX)
            break;
        if (!_kbhit ())
            break;
        c = _getch ();
    }
    return action;
}

// Simulate Standard Microsoft Extensions under Unix
#ifndef _WIN32

static int infd = -1;

int _kbhit (void)
{
    if (infd >= 0)
    {   // Set no delay
        static struct timeval tv = {0, 0};
        fd_set rdfs;

        // See if key has been pressed
        FD_ZERO (&rdfs);
        FD_SET  (infd, &rdfs);
        if (select  (infd + 1, &rdfs, nullptr, nullptr, &tv) <= 0)
            return 0;
        if (FD_ISSET (infd, &rdfs))
            return 1;
    }
    return 0;
}

int _getch (void)
{
    char ch = -1;
    if (infd >= 0)
        read (infd, &ch, 1);
    return ch;
}

// Set keyboard to raw mode to getch will work
static termios term;
void keyboard_enable_raw ()
{
    // set to non canonical mode, echo off, ignore signals
    struct termios current;

    // Already open
    if (infd >= 0)
        return;

    // Determine if stdin/stderr has been redirected
    if (isatty (STDIN_FILENO))
        infd = STDIN_FILENO;
    else if (isatty (STDERR_FILENO))
        infd = STDERR_FILENO;
    else
    {   // Try opening a terminal directly
        infd = open("/dev/tty", O_RDONLY);
        if (infd < 0)
            return;
    }

    // save current terminal settings
    tcgetattr (infd, &current);

    // set to non canonical mode, echo off, ignore signals
    term = current;
    current.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    current.c_cc[VMIN] = 1;
    current.c_cc[VTIME] = 0;
    tcsetattr (infd, TCSAFLUSH, &current);
}

void keyboard_disable_raw ()
{
    if (infd >= 0)
    {   // Restore old terminal settings
        tcsetattr (infd, TCSAFLUSH, &term);
        switch (infd)
        {
        case STDIN_FILENO:
        case STDERR_FILENO:
            break;
        default:
            close (infd);
        }
        infd = -1;
    }
}

#endif // HAVE_LINUX
