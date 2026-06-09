/****************************************************************************
 * apps/games/NXDoom/src/setup/mouse.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>

#include "doomtype.h"
#include "m_config.h"
#include "m_controls.h"
#include "textscreen.h"

#include "execute.h"
#include "txt_mouseinput.h"

#include "mode.h"
#include "mouse.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WINDOW_HELP_URL "https://www.chocolate-doom.org/setup-mouse"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int usemouse = 1;

static int g_mouse_sensitivity = 5;
static float mouse_acceleration = 2.0;
static int mouse_threshold = 10;
static int grabmouse = 1;

static int *all_mouse_buttons[] =
{
  &mousebfire,       &mousebstrafe,      &mousebforward,
  &mousebstrafeleft, &mousebstraferight, &mousebbackward,
  &mousebuse,        &mousebjump,        &mousebprevweapon,
  &mousebnextweapon, &mousebspeed,       &mousebinvleft,
  &mousebinvright,   &mousebuseartifact, &mousebturnleft,
  &mousebturnright,
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

int novert = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void mouse_set_callback(TXT_UNCAST_ARG(widget),
        TXT_UNCAST_ARG(variable))
{
  TXT_CAST_ARG(int, variable);
  unsigned int i;

  /* Check if the same mouse button is used for a different action
   * If so, set the other action(s) to -1 (unset)
   */

  for (i = 0; i < arrlen(all_mouse_buttons); ++i)
    {
      if (*all_mouse_buttons[i] == *variable &&
          all_mouse_buttons[i] != variable)
        {
          *all_mouse_buttons[i] = -1;
        }
    }
}

static void add_mouse_control(TXT_UNCAST_ARG(table), const char *label,
                            int *var)
{
  TXT_CAST_ARG(txt_table_t, table);
  txt_mouse_input_t *mouse_input;

  txt_add_widget(table, txt_new_label(label));

  mouse_input = txt_new_mouse_input(var);
  txt_add_widget(table, mouse_input);

  txt_signal_connect(mouse_input, "set", mouse_set_callback, var);
}

static void config_extra_buttons(TXT_UNCAST_ARG(widget),
        TXT_UNCAST_ARG(unused))
{
  txt_window_t *window;
  txt_table_t *buttons_table;

  window = txt_new_window("Additional mouse buttons");

  txt_set_window_help_url(window, WINDOW_HELP_URL);

  txt_add_widgets(window, buttons_table = txt_new_table(4), NULL);

  txt_set_column_widths(buttons_table, 16, 11, 16, 10);

  add_mouse_control(buttons_table, "Move forward", &mousebforward);
  add_mouse_control(buttons_table, "Strafe left", &mousebstrafeleft);
  add_mouse_control(buttons_table, "Move backward", &mousebbackward);
  add_mouse_control(buttons_table, "Strafe right", &mousebstraferight);
  add_mouse_control(buttons_table, "Previous weapon", &mousebprevweapon);
  add_mouse_control(buttons_table, "Strafe on", &mousebstrafe);
  add_mouse_control(buttons_table, "Next weapon", &mousebnextweapon);
  add_mouse_control(buttons_table, "Run", &mousebspeed);

  if (gamemission == heretic || gamemission == hexen)
    {
      add_mouse_control(buttons_table, "Inventory left", &mousebinvleft);
      add_mouse_control(buttons_table, "Inventory right", &mousebinvright);
      add_mouse_control(buttons_table, "Use artifact", &mousebuseartifact);
    }

  if (gamemission == hexen || gamemission == strife)
    {
      add_mouse_control(buttons_table, "Jump", &mousebjump);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void config_mouse(TXT_UNCAST_ARG(widget), void *user_data)
{
  txt_window_t *window;

  window = txt_new_window("Mouse configuration");

  txt_set_table_columns(window, 2);

  txt_set_window_action(window, TXT_HORIZ_CENTER, test_config_action());
  txt_set_window_help_url(window, WINDOW_HELP_URL);

  txt_add_widgets(
      window, txt_new_check_box("Enable mouse", &usemouse),
      TXT_TABLE_OVERFLOW_RIGHT,
      txt_new_inverted_checkbox("Allow vertical mouse movement", &novert),
      TXT_TABLE_OVERFLOW_RIGHT,
      txt_new_check_box("Grab mouse in windowed mode", &grabmouse),
      TXT_TABLE_OVERFLOW_RIGHT,
      txt_new_check_box("Double click acts as \"use\"", &dclick_use),
      TXT_TABLE_OVERFLOW_RIGHT,

      txt_new_separator("Mouse motion"), txt_new_label("Speed"),
      txt_newspin_control(&g_mouse_sensitivity, 1, 256),
      txt_new_label("Acceleration"),
      txt_new_float_spincontrol(&mouse_acceleration, 1.0, 5.0),
      txt_new_label("Acceleration threshold"),
      txt_newspin_control(&mouse_threshold, 0, 32),

      txt_new_separator("Buttons"), NULL);

  add_mouse_control(window, "Fire/Attack", &mousebfire);
  add_mouse_control(window, "Use", &mousebuse);

  txt_add_widget(window, txt_new_button2("More controls...",
                                         config_extra_buttons, NULL));
}

void bind_mouse_variables(void)
{
  m_bind_int_variable("use_mouse", &usemouse);
  m_bind_int_variable("novert", &novert);
  m_bind_int_variable("grabmouse", &grabmouse);
  m_bind_int_variable("mouse_sensitivity", &g_mouse_sensitivity);
  m_bind_int_variable("mouse_threshold", &mouse_threshold);
  m_bind_float_variable("mouse_acceleration", &mouse_acceleration);
}
