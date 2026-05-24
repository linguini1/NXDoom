//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//      Timer functions.
//

#include <nuttx/clock.h>
#include <unistd.h>

#include "i_timer.h"
#include "doomtype.h"

//
// I_GetTime
// returns time in 1/35th second tics
//

static struct timespec basetime =
{
  .tv_nsec = 0,
  .tv_sec = 0
};

int  I_GetTime (void)
{
    return (I_GetTimeMS() * TICRATE) / 1000;    
}

//
// Same as I_GetTime, but returns time in milliseconds
//

int I_GetTimeMS(void)
{
    struct timespec curtime;

    /* NOTE: we ignore any possible error here */

    clock_gettime(CLOCK_MONOTONIC, &curtime);
    if (basetime.tv_sec == 0 && basetime.tv_nsec == 0)
    {
      clock_gettime(CLOCK_MONOTONIC, &basetime);
    }

    return (clock_time2usec(&curtime) - clock_time2usec(&basetime)) / 1000;
}

// Sleep for a specified number of ms

void I_Sleep(int ms)
{
    usleep(ms / 1000);
}

void I_WaitVBL(int count)
{
    I_Sleep((count * 1000) / 70);
}


void I_InitTimer(void)
{
    // initialize timer
#if 0
    SDL_SetHint(SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING, "1");

    SDL_Init(SDL_INIT_TIMER);
#endif
}

