/****************************************************************************
 * apps/games/NXDoom/src/setup/joystick.h
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

#ifndef SETUP_JOYSTICK_H
#define SETUP_JOYSTICK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "i_joystick.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int joystick_index;
extern int joystick_physical_buttons[NUM_VIRTUAL_BUTTONS];
extern int use_gamepad;
extern int gamepad_type;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void config_joystick(void *widget, void *user_data);
void bind_joystick_variables(void);

#endif /* SETUP_JOYSTICK_H */
