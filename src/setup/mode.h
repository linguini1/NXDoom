/****************************************************************************
 * apps/games/NXDoom/src/setup/mode.h
 *
 * SPDX-License-Identifer: GPLv2
 *
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
 ****************************************************************************/

#ifndef SETUP_MODE_H
#define SETUP_MODE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_iwad.h"
#include "d_mode.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef void (*game_select_callback)(void);

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern gamemission_t gamemission;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void setup_mission(game_select_callback callback);
void init_bindings(void);
const char *get_executable_name(void);
const char *get_game_title(void);
const iwad_t **get_iwads(void);

#endif /* SETUP_MODE_H */
