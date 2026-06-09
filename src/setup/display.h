/****************************************************************************
 * apps/games/NXDoom/src/setup/display.h
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
 *
 ****************************************************************************/

#ifndef SETUP_DISPLAY_H
#define SETUP_DISPLAY_H

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int show_endoom;
extern int graphical_startup;
extern int png_screenshots;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void config_display(void *widget, void *user_data);
void set_display_driver(void);
void bind_display_variables(void);

#endif /* SETUP_DISPLAY_H */
