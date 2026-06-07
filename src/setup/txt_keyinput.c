/****************************************************************************
 * apps/games/NXDoom/src/setup/txt_keyinput.c
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
#include <string.h>

#include "doomkeys.h"
#include "m_misc.h"

#include "txt_gui.h"
#include "txt_io.h"
#include "txt_keyinput.h"
#include "txt_label.h"
#include "txt_utf8.h"
#include "txt_window.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define KEY_INPUT_WIDTH 8

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

static void txt_key_input_size_calc(TXT_UNCAST_ARG(key_input));
static void txt_key_input_drawer(TXT_UNCAST_ARG(key_input));
static void txt_key_input_destructor(TXT_UNCAST_ARG(key_input));
static int txt_key_input_key_press(TXT_UNCAST_ARG(key_input), int key);
static void txt_key_input_mousepress(TXT_UNCAST_ARG(widget), int x, int y,
                                     int b);

/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_key_input_class =
{
  txt_always_selectable,
  txt_key_input_size_calc,
  txt_key_input_drawer,
  txt_key_input_key_press,
  txt_key_input_destructor,
  txt_key_input_mousepress,
  NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int key_press_callback(txt_window_t *window, int key,
                            TXT_UNCAST_ARG(key_input))
{
  TXT_CAST_ARG(txt_key_input_t, key_input);

  if (key != KEY_ESCAPE)
    {
      /* Got the key press. Save to the variable and close the window. */

      *key_input->variable = key;

      if (key_input->check_conflicts)
        {
          txt_emit_signal(key_input, "set");
        }

      txt_close_window(window);

      /* Return to normal input mode now that we have the key. */

      txt_set_input_mode(TXT_INPUT_NORMAL);

      return 1;
    }
  else
    {
      return 0;
    }
}

static void release_grab(TXT_UNCAST_ARG(window), TXT_UNCAST_ARG(unused))
{
  /* SDL2-TODO: Needed?
   * SDL_WM_GrabInput(SDL_GRAB_OFF);
   */
}

static void open_prompt_window(txt_key_input_t *key_input)
{
  txt_window_t *window;

  /* Silently update when the shift button is held down. */

  key_input->check_conflicts = !txt_get_modifier_state(TXT_MOD_SHIFT);

  window = txt_message_box(NULL, "Press the new key...");

  txt_set_key_listener(window, key_press_callback, key_input);

  /* Switch to raw input mode while we're grabbing the key. */

  txt_set_input_mode(TXT_INPUT_RAW);

  /* Grab input while reading the key.  On Windows Mobile
   * handheld devices, the hardware keypresses are only
   * detected when input is grabbed.
   * SDL2-TODO: Needed?
   * SDL_WM_GrabInput(SDL_GRAB_ON);
   */

  txt_signal_connect(window, "closed", release_grab, NULL);
}

static void txt_key_input_size_calc(TXT_UNCAST_ARG(key_input))
{
  TXT_CAST_ARG(txt_key_input_t, key_input);

  /* All keyinputs are the same size. */

  key_input->widget.w = KEY_INPUT_WIDTH;
  key_input->widget.h = 1;
}

static void txt_key_input_drawer(TXT_UNCAST_ARG(key_input))
{
  TXT_CAST_ARG(txt_key_input_t, key_input);
  char buf[20];
  int i;

  if (*key_input->variable == 0)
    {
      m_str_copy(buf, "(none)", sizeof(buf));
    }
  else
    {
      txt_get_key_description(*key_input->variable, buf, sizeof(buf));
    }

  txt_set_widget_bg(key_input);
  txt_fgcolour(TXT_COLOR_BRIGHT_WHITE);

  txt_draw_string(buf);

  for (i = txt_utf8_strlen(buf); i < KEY_INPUT_WIDTH; ++i)
    {
      txt_draw_string(" ");
    }
}

static void txt_key_input_destructor(TXT_UNCAST_ARG(key_input))
{
}

static int txt_key_input_key_press(TXT_UNCAST_ARG(key_input), int key)
{
  TXT_CAST_ARG(txt_key_input_t, key_input);

  if (key == KEY_ENTER)
    {
      /* Open a window to prompt for the new key press */

      open_prompt_window(key_input);

      return 1;
    }

  if (key == KEY_BACKSPACE || key == KEY_DEL)
    {
      *key_input->variable = 0;
    }

  return 0;
}

static void txt_key_input_mousepress(TXT_UNCAST_ARG(widget), int x, int y,
                                   int b)
{
  TXT_CAST_ARG(txt_key_input_t, widget);

  /* Clicking is like pressing enter */

  if (b == TXT_MOUSE_LEFT)
    {
      txt_key_input_key_press(widget, KEY_ENTER);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

txt_key_input_t *txt_new_key_input(int *variable)
{
  txt_key_input_t *key_input;

  key_input = malloc(sizeof(txt_key_input_t));

  txt_init_widget(key_input, &txt_key_input_class);
  key_input->variable = variable;

  return key_input;
}
