/****************************************************************************
 * apps/games/NXDoom/src/m_fixed.h
 *
 * SPDX-License-Identifer: GPLv2
 *
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
 *	Fixed point arithemtics, implementation.
 *
 ****************************************************************************/

#ifndef __M_FIXED__
#define __M_FIXED__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <fixedmath.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Fixed point, 32bit as 16.16. */

#define FRACBITS 16
#define FRACUNIT (b16ONE)

#define fixed_mul(a, b)   b16mulb16(a, b)
#define fixed_div(a, b)   b16divb16(a, b)
#define fixed_to_whole(a) b16toi(a)
#define whole_to_fixed(a) itob16(a)

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef b16_t fixed_t;

#endif /* __M_FIXED__ */
