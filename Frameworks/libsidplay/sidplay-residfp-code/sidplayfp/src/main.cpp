/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2012 Leandro Nini
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

#include "player.h"

#include <stdlib.h>
#include <signal.h>

#include <iostream>

using std::cerr;
using std::endl;

#include "keyboard.h"


// Function prototypes
static void sighandler (int signum);
static ConsolePlayer *g_player;

int main(int argc, char *argv[])
{
    ConsolePlayer player(argv[0]);
    g_player = &player;

    {// Decode the command line args
        const int ret = player.args (argc - 1, const_cast<const char**>(argv + 1));
        if (ret < 0)
            goto main_error;
        else if (!ret)
            goto main_exit;
    }

main_restart:
    if (!player.open ())
        goto main_error;

    // Install signal error handlers
    if ((signal (SIGINT,  &sighandler) == SIG_ERR)
     || (signal (SIGABRT, &sighandler) == SIG_ERR)
     || (signal (SIGTERM, &sighandler) == SIG_ERR))
    {
        displayError(argv[0], ERR_SIGHANDLER);
        goto main_error;
    }

#ifndef _WIN32
    // Configure terminal to allow direct access to key events
    keyboard_enable_raw ();
#endif

    // Play loop
    for (;;)
    {
        if (!player.play ())
            break;
    }

#ifndef _WIN32
    keyboard_disable_raw ();
#endif

    // Restore default signal error handlers
    if ((signal (SIGINT,  SIG_DFL) == SIG_ERR)
     || (signal (SIGABRT, SIG_DFL) == SIG_ERR)
     || (signal (SIGTERM, SIG_DFL) == SIG_ERR))
    {
        displayError(argv[0], ERR_SIGHANDLER);
        goto main_error;
    }

    if ((player.state() & ~playerFast) == playerRestart)
        goto main_restart;
main_exit:
    player.close ();
    return EXIT_SUCCESS;

main_error:
    player.close ();
    return EXIT_FAILURE;
}


void sighandler (int signum)
{
    switch (signum)
    {
    case SIGINT:
    case SIGABRT:
    case SIGTERM:
        // Exit now!
        g_player->stop ();
        break;
    default: break;
    }
}


void displayError (const char *arg0, unsigned int num)
{
    cerr << arg0 << ": ";

    switch (num)
    {
    case ERR_SYNTAX:
        cerr << "command line syntax error" << endl
             << "Try `" << arg0 << " --help' for more information." << endl;
        break;

    case ERR_NOT_ENOUGH_MEMORY:
        cerr << "ERROR: Not enough memory." << endl;
        break;

    case ERR_SIGHANDLER:
        cerr << "ERROR: Could not install signal handler." << endl;
        break;

    case ERR_FILE_OPEN:
        cerr << "ERROR: Could not open file for binary input." << endl;
        break;

    default: break;
    }
}
