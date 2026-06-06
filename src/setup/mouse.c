//
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

#include <stdlib.h>

#include "doomtype.h"
#include "m_config.h"
#include "m_controls.h"
#include "textscreen.h"

#include "execute.h"
#include "txt_mouseinput.h"

#include "mode.h"
#include "mouse.h"

#define WINDOW_HELP_URL "https://www.chocolate-doom.org/setup-mouse"

static int usemouse = 1;

static int mouseSensitivity = 5;
static float mouse_acceleration = 2.0;
static int mouse_threshold = 10;
static int grabmouse = 1;

int novert = 0;

static int *all_mouse_buttons[] = {
    &mousebfire,       &mousebstrafe,      &mousebforward,
    &mousebstrafeleft, &mousebstraferight, &mousebbackward,
    &mousebuse,        &mousebjump,        &mousebprevweapon,
    &mousebnextweapon, &mousebspeed,       &mousebinvleft,
    &mousebinvright,   &mousebuseartifact, &mousebturnleft,
    &mousebturnright,
};

static void MouseSetCallback(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(variable))
{
  TXT_CAST_ARG(int, variable);
  unsigned int i;

  // Check if the same mouse button is used for a different action
  // If so, set the other action(s) to -1 (unset)

  for (i = 0; i < arrlen(all_mouse_buttons); ++i)
    {
      if (*all_mouse_buttons[i] == *variable &&
          all_mouse_buttons[i] != variable)
        {
          *all_mouse_buttons[i] = -1;
        }
    }
}

static void AddMouseControl(TXT_UNCAST_ARG(table), const char *label,
                            int *var)
{
  TXT_CAST_ARG(txt_table_t, table);
  txt_mouse_input_t *mouse_input;

  TXT_AddWidget(table, txt_new_label(label));

  mouse_input = TXT_NewMouseInput(var);
  TXT_AddWidget(table, mouse_input);

  txt_signal_connect(mouse_input, "set", MouseSetCallback, var);
}

static void ConfigExtraButtons(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(unused))
{
  txt_window_t *window;
  txt_table_t *buttons_table;

  window = txt_new_window("Additional mouse buttons");

  txt_set_window_help_url(window, WINDOW_HELP_URL);

  txt_add_widgets(window, buttons_table = TXT_NewTable(4), NULL);

  txt_set_column_widths(buttons_table, 16, 11, 16, 10);

  AddMouseControl(buttons_table, "Move forward", &mousebforward);
  AddMouseControl(buttons_table, "Strafe left", &mousebstrafeleft);
  AddMouseControl(buttons_table, "Move backward", &mousebbackward);
  AddMouseControl(buttons_table, "Strafe right", &mousebstraferight);
  AddMouseControl(buttons_table, "Previous weapon", &mousebprevweapon);
  AddMouseControl(buttons_table, "Strafe on", &mousebstrafe);
  AddMouseControl(buttons_table, "Next weapon", &mousebnextweapon);
  AddMouseControl(buttons_table, "Run", &mousebspeed);

  if (gamemission == heretic || gamemission == hexen)
    {
      AddMouseControl(buttons_table, "Inventory left", &mousebinvleft);
      AddMouseControl(buttons_table, "Inventory right", &mousebinvright);
      AddMouseControl(buttons_table, "Use artifact", &mousebuseartifact);
    }

  if (gamemission == hexen || gamemission == strife)
    {
      AddMouseControl(buttons_table, "Jump", &mousebjump);
    }
}

void ConfigMouse(TXT_UNCAST_ARG(widget), void *user_data)
{
  txt_window_t *window;

  window = txt_new_window("Mouse configuration");

  TXT_SetTableColumns(window, 2);

  txt_set_window_action(window, TXT_HORIZ_CENTER, TestConfigAction());
  txt_set_window_help_url(window, WINDOW_HELP_URL);

  txt_add_widgets(
      window, txt_new_check_box("Enable mouse", &usemouse),
      TXT_TABLE_OVERFLOW_RIGHT,
      TXT_NewInvertedCheckBox("Allow vertical mouse movement", &novert),
      TXT_TABLE_OVERFLOW_RIGHT,
      txt_new_check_box("Grab mouse in windowed mode", &grabmouse),
      TXT_TABLE_OVERFLOW_RIGHT,
      txt_new_check_box("Double click acts as \"use\"", &dclick_use),
      TXT_TABLE_OVERFLOW_RIGHT,

      txt_new_separator("Mouse motion"), txt_new_label("Speed"),
      TXT_NewSpinControl(&mouseSensitivity, 1, 256),
      txt_new_label("Acceleration"),
      TXT_NewFloatSpinControl(&mouse_acceleration, 1.0, 5.0),
      txt_new_label("Acceleration threshold"),
      TXT_NewSpinControl(&mouse_threshold, 0, 32),

      txt_new_separator("Buttons"), NULL);

  AddMouseControl(window, "Fire/Attack", &mousebfire);
  AddMouseControl(window, "Use", &mousebuse);

  TXT_AddWidget(window,
                TXT_NewButton2("More controls...", ConfigExtraButtons, NULL));
}

void BindMouseVariables(void)
{
  m_bind_int_variable("use_mouse", &usemouse);
  m_bind_int_variable("novert", &novert);
  m_bind_int_variable("grabmouse", &grabmouse);
  m_bind_int_variable("mouse_sensitivity", &mouseSensitivity);
  m_bind_int_variable("mouse_threshold", &mouse_threshold);
  m_bind_float_variable("mouse_acceleration", &mouse_acceleration);
}
