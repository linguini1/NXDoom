/****************************************************************************
 * apps/games/NXDoom/src/setup/txt_joybinput.h
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

#ifndef TXT_JOYB_INPUT_H
#define TXT_JOYB_INPUT_H

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct txt_joystick_input_s txt_joystick_input_t;

#include "txt_widget.h"
#include "txt_window.h"

/* A joystick input is like an input box.  When selected, a box pops up
 * allowing a joystick button to be pressed to select it.
 */

struct txt_joystick_input_s
{
  txt_widget_t widget;
  int *variable;
  txt_window_t *prompt_window;
  int check_conflicts;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

txt_joystick_input_t *txt_new_joystick_input(int *variable);

#endif /* TXT_JOYB_INPUT_H */
