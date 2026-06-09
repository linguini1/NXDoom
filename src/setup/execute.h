/****************************************************************************
 * apps/games/NXDoom/src/setup/execute.h
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

#ifndef TESTCONFIG_H
#define TESTCONFIG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomtype.h"
#include "textscreen.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IWAD_DOOM2 (1 << 0)    /* doom2.wad */
#define IWAD_PLUTONIA (1 << 1) /* plutonia.wad */
#define IWAD_TNT (1 << 2)      /* tnt.wad */
#define IWAD_DOOM (1 << 3)     /* doom.wad */
#define IWAD_DOOM1 (1 << 4)    /* doom1.wad */
#define IWAD_CHEX (1 << 5)     /* chex.wad */

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct execute_context_s execute_context_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

execute_context_t *new_execute_context(void);
void add_cmdline_parameter(execute_context_t *context, const char *s, ...)
    PRINTF_ATTR(2, 3);
void pass_through_arguments(execute_context_t *context);
int execute_doom(execute_context_t *context);
boolean open_folder(const char *path);

txt_window_action_t *test_config_action(void);

#endif /* TESTCONFIG_H */
