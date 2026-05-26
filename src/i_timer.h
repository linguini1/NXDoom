/*
 * Copyright(C) 1993-1996 Id Software, Inc.
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * DESCRIPTION:
 *      System-specific timer interface
 */

#ifndef __I_TIMER__
#define __I_TIMER__

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define TICRATE 35

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: I_GetTime
 *
 * Description:
 *  Called by D_DoomLoop.
 *
 * Returns:
 *  The current time in tics.
 ****************************************************************************/

int I_GetTime(void);

/****************************************************************************
 * Name: I_GetTimeMS
 *
 * Returns:
 *  The current time in ms.
 ****************************************************************************/

int I_GetTimeMS(void);

/****************************************************************************
 * Name: I_Sleep
 *
 * Returns:
 *  Pause for a specified number of ms.
 ****************************************************************************/

void I_Sleep(int ms);

/****************************************************************************
 * Name: I_InitTimer
 *
 * Description:
 *  Initialize timer.
 ****************************************************************************/

void I_InitTimer(void);

/****************************************************************************
 * Name: I_WaitVBL
 *
 * Description:
 *   Wait for vertical retrace or pause a bit.
 ****************************************************************************/

void I_WaitVBL(int count);

#endif /* __I_TIMER__ */
