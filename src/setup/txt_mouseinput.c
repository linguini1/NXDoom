/****************************************************************************
 * apps/games/NXDoom/src/setup/txt_mouseinput.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomkeys.h"
#include "m_misc.h"

#include "txt_gui.h"
#include "txt_io.h"
#include "txt_label.h"
#include "txt_mouseinput.h"
#include "txt_utf8.h"
#include "txt_window.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* eg. "BUTTON #10" */

#define MOUSE_INPUT_WIDTH 10

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int mouse_press_callback(txt_window_t *window, int x, int y, int b,
                                TXT_UNCAST_ARG(mouse_input));
static void open_prompt_window(txt_mouse_input_t *mouse_input);
static void txt_mouse_input_size_calc(TXT_UNCAST_ARG(mouse_input));
static void get_mouse_button_description(int button, char *buf,
                                         size_t buf_len);
static void txt_mouse_input_drawer(TXT_UNCAST_ARG(mouse_input));
static void txt_mouse_input_destructor(TXT_UNCAST_ARG(mouse_input));
static int txt_mouse_input_keypress(TXT_UNCAST_ARG(mouse_input), int key);
static void txt_mouse_input_mousepress(TXT_UNCAST_ARG(widget), int x, int y,
                                       int b);

/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_mouse_input_class =
{
  txt_always_selectable,
  txt_mouse_input_size_calc,
  txt_mouse_input_drawer,
  txt_mouse_input_keypress,
  txt_mouse_input_destructor,
  txt_mouse_input_mousepress,
  NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int mouse_press_callback(txt_window_t *window, int x, int y, int b,
                              TXT_UNCAST_ARG(mouse_input))
{
  TXT_CAST_ARG(txt_mouse_input_t, mouse_input);

  /* Got the mouse press.  Save to the variable and close the window. */

  *mouse_input->variable = b - TXT_MOUSE_BASE;

  if (mouse_input->check_conflicts)
    {
      txt_emit_signal(mouse_input, "set");
    }

  txt_close_window(window);

  return 1;
}

static void open_prompt_window(txt_mouse_input_t *mouse_input)
{
  txt_window_t *window;

  /* Silently update when the shift key is held down. */

  mouse_input->check_conflicts = !txt_get_modifier_state(TXT_MOD_SHIFT);
  window = txt_message_box(NULL, "Press the new mouse button...");
  txt_set_mouse_listener(window, mouse_press_callback, mouse_input);
}

static void txt_mouse_input_size_calc(TXT_UNCAST_ARG(mouse_input))
{
  TXT_CAST_ARG(txt_mouse_input_t, mouse_input);

  /* All mouseinputs are the same size. */

  mouse_input->widget.w = MOUSE_INPUT_WIDTH;
  mouse_input->widget.h = 1;
}

static void get_mouse_button_description(int button, char *buf,
                                         size_t buf_len)
{
  switch (button)
    {
    case 0:
      m_str_copy(buf, "LEFT", buf_len);
      break;
    case 1:
      m_str_copy(buf, "RIGHT", buf_len);
      break;
    case 2:
      m_str_copy(buf, "MID", buf_len);
      break;
    case 3:
      m_str_copy(buf, "WHEEL UP", buf_len);
      break;
    case 4:
      m_str_copy(buf, "WHEEL DOWN", buf_len);
      break;
    default:
      snprintf(buf, buf_len, "BUTTON #%i", button - 1);
      break;
    }
}

static void txt_mouse_input_drawer(TXT_UNCAST_ARG(mouse_input))
{
  TXT_CAST_ARG(txt_mouse_input_t, mouse_input);
  char buf[20];
  int i;

  if (*mouse_input->variable < 0)
    {
      m_str_copy(buf, "(none)", sizeof(buf));
    }
  else
    {
      get_mouse_button_description(*mouse_input->variable, buf, sizeof(buf));
    }

  txt_set_widget_bg(mouse_input);
  txt_fgcolour(TXT_COLOR_BRIGHT_WHITE);

  txt_draw_string(buf);

  for (i = txt_utf8_strlen(buf); i < MOUSE_INPUT_WIDTH; ++i)
    {
      txt_draw_string(" ");
    }
}

static void txt_mouse_input_destructor(TXT_UNCAST_ARG(mouse_input))
{
}

static int txt_mouse_input_keypress(TXT_UNCAST_ARG(mouse_input), int key)
{
  TXT_CAST_ARG(txt_mouse_input_t, mouse_input);

  if (key == KEY_ENTER)
    {
      /* Open a window to prompt for the new mouse press */

      open_prompt_window(mouse_input);

      return 1;
    }

  if (key == KEY_BACKSPACE || key == KEY_DEL)
    {
      *mouse_input->variable = -1;
    }

  return 0;
}

static void txt_mouse_input_mousepress(TXT_UNCAST_ARG(widget), int x, int y,
                                     int b)
{
  TXT_CAST_ARG(txt_mouse_input_t, widget);

  /* Clicking is like pressing enter */

  if (b == TXT_MOUSE_LEFT)
    {
      txt_mouse_input_keypress(widget, KEY_ENTER);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

txt_mouse_input_t *txt_new_mouse_input(int *variable)
{
  txt_mouse_input_t *mouse_input;

  mouse_input = malloc(sizeof(txt_mouse_input_t));

  txt_init_widget(mouse_input, &txt_mouse_input_class);
  mouse_input->variable = variable;

  return mouse_input;
}
