/****************************************************************************
 * apps/games/NXDoom/src/setup/txt_joyaxis.c
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 2014 Simon Howard
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

#include "SDL.h"

#include "i_joystick.h"
#include "i_system.h"
#include "joystick.h"
#include "m_controls.h"
#include "m_misc.h"

#include "textscreen.h"
#include "txt_gui.h"
#include "txt_io.h"
#include "txt_joyaxis.h"
#include "txt_utf8.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define JOYSTICK_AXIS_WIDTH (20)

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void txt_joystick_axis_size_calc(TXT_UNCAST_ARG(joystick_axis));
static void txt_joystick_axis_drawer(TXT_UNCAST_ARG(joystick_axis));
static void txt_gamepad_axis_drawer(TXT_UNCAST_ARG(joystick_axis));
static void txt_joystick_axis_destructor(TXT_UNCAST_ARG(joystick_axis));
static int txt_joystick_axis_keypress(TXT_UNCAST_ARG(joystick_axis),
                                      int key);
static int txt_gamepad_axis_keypress(TXT_UNCAST_ARG(joystick_axis), int key);
static void txt_joystick_axis_mousepress(TXT_UNCAST_ARG(widget), int x,
                                         int y, int b);
static void txt_gamepad_axis_mousepress(TXT_UNCAST_ARG(widget), int x,
                                        int y, int b);

/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_joystick_axis_class =
{
  txt_always_selectable,
  txt_joystick_axis_size_calc,
  txt_joystick_axis_drawer,
  txt_joystick_axis_keypress,
  txt_joystick_axis_destructor,
  txt_joystick_axis_mousepress,
  NULL,
};

txt_widget_class_t txt_gamepad_axis_class =
{
  txt_always_selectable,
  txt_joystick_axis_size_calc,
  txt_gamepad_axis_drawer,
  txt_gamepad_axis_keypress,
  txt_joystick_axis_destructor,
  txt_gamepad_axis_mousepress,
  NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static const char *calibration_label(txt_joystick_axis_t *joystick_axis)
{
  switch (joystick_axis->config_stage)
    {
    case CONFIG_CENTER:
      return "Center the D-pad or joystick,\n"
             "and press a button.";

    case CONFIG_STAGE1:
      if (joystick_axis->dir == JOYSTICK_AXIS_VERTICAL)
        {
          return "Push the D-pad or joystick up,\n"
                 "and press the button.";
        }
      else
        {
          return "Push the D-pad or joystick to the\n"
                 "left, and press the button.";
        }

    case CONFIG_STAGE2:
      if (joystick_axis->dir == JOYSTICK_AXIS_VERTICAL)
        {
          return "Push the D-pad or joystick down,\n"
                 "and press the button.";
        }
      else
        {
          return "Push the D-pad or joystick to the\n"
                 "right, and press the button.";
        }
    }

  return NULL;
}

static void set_calibration_lable(txt_joystick_axis_t *joystick_axis)
{
  txt_set_label(joystick_axis->config_label,
                calibration_label(joystick_axis));
}

/* Search all axes on joystick being configured; find a button that is
 * pressed (other than the calibrate button). Returns the button number.
 */

static int find_pressed_axis_button(txt_joystick_axis_t *joystick_axis)
{
  int i;

  for (i = 0; i < SDL_JoystickNumButtons(joystick_axis->joystick); ++i)
    {
      if (i == joystick_axis->config_button)
        {
          continue;
        }

      if (SDL_JoystickGetButton(joystick_axis->joystick, i))
        {
          return i;
        }
    }

  return -1;
}

/* Look for a hat that isn't centered. Returns the encoded hat axis. */

static int find_uncentered_hat(SDL_Joystick *joystick, int *axis_invert)
{
  int i;
  int hatval;

  for (i = 0; i < SDL_JoystickNumHats(joystick); ++i)
    {
      hatval = SDL_JoystickGetHat(joystick, i);

      switch (hatval)
        {
        case SDL_HAT_LEFT:
        case SDL_HAT_RIGHT:
          *axis_invert = hatval != SDL_HAT_LEFT;
          return CREATE_HAT_AXIS(i, HAT_AXIS_HORIZONTAL);

        case SDL_HAT_UP:
        case SDL_HAT_DOWN:
          *axis_invert = hatval != SDL_HAT_UP;
          return CREATE_HAT_AXIS(i, HAT_AXIS_VERTICAL);

          /* If the hat is centered, or is not pointing in a
           * definite direction, then ignore it. We don't accept
           * the hat being pointed to the upper-left for example,
           * because it's ambiguous.
           */

        case SDL_HAT_CENTERED:
        default:
          break;
        }
    }

  return -1; /* None found. */
}

static boolean calibrate_axis(txt_joystick_axis_t *joystick_axis)
{
  int best_axis;
  int best_value;
  int best_invert;
  Sint16 axis_value;
  int i;

  /* Check all axes to find which axis has the largest value.  We test
   * for one axis at a time, so eg. when we prompt to push the joystick
   * left, whichever axis has the largest value is the left axis.
   */

  best_axis = 0;
  best_value = 0;
  best_invert = 0;

  for (i = 0; i < SDL_JoystickNumAxes(joystick_axis->joystick); ++i)
    {
      axis_value = SDL_JoystickGetAxis(joystick_axis->joystick, i);

      if (joystick_axis->bad_axis[i])
        {
          continue;
        }

      if (abs(axis_value) > best_value)
        {
          best_value = abs(axis_value);
          best_invert = axis_value > 0;
          best_axis = i;
        }
    }

  /* Did we find one axis that had a significant value? */

  if (best_value > 32768 / 4)
    {
      /* Save the best values we have found */

      *joystick_axis->axis = best_axis;
      *joystick_axis->invert = best_invert;
      return true;
    }

  /* Otherwise, maybe this is a "button axis", like the PS3 SIXAXIS
   * controller that exposes the D-pad as four individual buttons.
   * Search for a button.
   */

  i = find_pressed_axis_button(joystick_axis);

  if (i >= 0)
    {
      *joystick_axis->axis = CREATE_BUTTON_AXIS(i, 0);
      *joystick_axis->invert = 0;
      return true;
    }

  /* Maybe it's a D-pad that is presented as a hat. This sounds weird
   * but gamepads like this really do exist; an example is the
   * Nyko AIRFLO Ex.
   */

  i = FindUncenteredHat(joystick_axis->joystick, joystick_axis->invert);

  if (i >= 0)
    {
      *joystick_axis->axis = i;
      return true;
    }

  /* User pressed the button without pushing the joystick anywhere. */

  return false;
}

static boolean set_button_axis(txt_joystick_axis_t *joystick_axis)
{
  int button;

  button = find_pressed_axis_button(joystick_axis);

  if (button >= 0)
    {
      *joystick_axis->axis |= CREATE_BUTTON_AXIS(0, button);
      return true;
    }

  return false;
}

static void identifiy_bad_axes(txt_joystick_axis_t *joystick_axis)
{
  int i;
  int val;

  free(joystick_axis->bad_axis);

  joystick_axis->bad_axis =
      calloc(SDL_JoystickNumAxes(joystick_axis->joystick), sizeof(boolean));

  /* Look for uncentered axes. */

  for (i = 0; i < SDL_JoystickNumAxes(joystick_axis->joystick); ++i)
    {
      val = SDL_JoystickGetAxis(joystick_axis->joystick, i);

      joystick_axis->bad_axis[i] = abs(val) > (32768 / 5);

      if (joystick_axis->bad_axis[i])
        {
          printf("Ignoring uncentered joystick axis #%i: %i\n", i, val);
        }
    }
}

static int next_calibrate_stage(txt_joystick_axis_t *joystick_axis)
{
  switch (joystick_axis->config_stage)
    {
    case CONFIG_CENTER:
      return CONFIG_STAGE1;

      /* After pushing to the left, there are two possibilities:
       * either it is a button axis, in which case we need to find
       * the other button, or we can just move on to the next axis.
       */

    case CONFIG_STAGE1:
      if (IS_BUTTON_AXIS(*joystick_axis->axis))
        {
          return CONFIG_STAGE2;
        }
      else
        {
          return CONFIG_CENTER;
        }

    case CONFIG_STAGE2:
      return CONFIG_CENTER;
    }

  return -1;
}

static int event_callback(SDL_Event *event, TXT_UNCAST_ARG(joystick_axis))
{
  TXT_CAST_ARG(txt_joystick_axis_t, joystick_axis);
  boolean advance;

  if (event->type != SDL_JOYBUTTONDOWN)
    {
      return 0;
    }

  /* At this point, we have a button press.
   * In the first "center" stage, we're just trying to work out which
   * joystick is being configured and which button the user is pressing.
   */

  if (joystick_axis->config_stage == CONFIG_CENTER)
    {
      joystick_axis->config_button = event->jbutton.button;
      identifiy_bad_axes(joystick_axis);

      /* Advance to next stage. */

      joystick_axis->config_stage = CONFIG_STAGE1;
      set_calibration_lable(joystick_axis);

      return 1;
    }

  /* In subsequent stages, the user is asked to push in a specific
   * direction and press the button. They must push the same button
   * as they did before; this is necessary to support button axes.
   */

  if (event->jbutton.which ==
          SDL_JoystickInstanceID(joystick_axis->joystick) &&
      event->jbutton.button == joystick_axis->config_button)
    {
      switch (joystick_axis->config_stage)
        {
        default:
        case CONFIG_STAGE1:
          advance = CalibrateAxis(joystick_axis);
          break;

        case CONFIG_STAGE2:
          advance = SetButtonAxisPositive(joystick_axis);
          break;
        }

      /* Advance to the next calibration stage? */

      if (advance)
        {
          joystick_axis->config_stage = NextCalibrateStage(joystick_axis);
          set_calibration_lable(joystick_axis);

          /* Finished? */

          if (joystick_axis->config_stage == CONFIG_CENTER)
            {
              txt_close_window(joystick_axis->config_window);

              if (joystick_axis->callback != NULL)
                {
                  joystick_axis->callback();
                }
            }

          return 1;
        }
    }

  return 0;
}

static void calibrate_window_closed(TXT_UNCAST_ARG(widget),
                                  TXT_UNCAST_ARG(joystick_axis))
{
  TXT_CAST_ARG(txt_joystick_axis_t, joystick_axis);

  free(joystick_axis->bad_axis);
  joystick_axis->bad_axis = NULL;

  SDL_JoystickClose(joystick_axis->joystick);
  SDL_JoystickEventState(SDL_DISABLE);
  SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
  txt_sdl_set_event_callback(NULL, NULL);
}

static void txt_joystick_axis_size_calc(TXT_UNCAST_ARG(joystick_axis))
{
  TXT_CAST_ARG(txt_joystick_axis_t, joystick_axis);

  /* All joystickinputs are the same size. */

  joystick_axis->widget.w = JOYSTICK_AXIS_WIDTH;
  joystick_axis->widget.h = 1;
}

static void txt_joystick_axis_drawer(TXT_UNCAST_ARG(joystick_axis))
{
  TXT_CAST_ARG(txt_joystick_axis_t, joystick_axis);
  char buf[JOYSTICK_AXIS_WIDTH + 1];
  int i;

  if (*joystick_axis->axis < 0)
    {
      m_str_copy(buf, "(none)", sizeof(buf));
    }
  else if (IS_BUTTON_AXIS(*joystick_axis->axis))
    {
      int neg;
      int pos;

      neg = BUTTON_AXIS_NEG(*joystick_axis->axis);
      pos = BUTTON_AXIS_POS(*joystick_axis->axis);
      snprintf(buf, sizeof(buf), "BUTTONS #%i+#%i", neg, pos);
    }
  else if (IS_HAT_AXIS(*joystick_axis->axis))
    {
      int hat;
      int dir;

      hat = HAT_AXIS_HAT(*joystick_axis->axis);
      dir = HAT_AXIS_DIRECTION(*joystick_axis->axis);

      snprintf(buf, sizeof(buf), "HAT #%i (%s)", hat,
               dir == HAT_AXIS_HORIZONTAL ? "horizontal" : "vertical");
    }
  else
    {
      snprintf(buf, sizeof(buf), "AXIS #%i", *joystick_axis->axis);
    }

  txt_set_widget_bg(joystick_axis);
  txt_fgcolour(TXT_COLOR_BRIGHT_WHITE);

  txt_draw_string(buf);

  for (i = txt_utf8_strlen(buf); i < joystick_axis->widget.w; ++i)
    {
      txt_draw_string(" ");
    }
}

static void get_axis_description(int axis, char *buf, size_t buf_len)
{
  switch (axis)
    {
    case SDL_CONTROLLER_AXIS_INVALID:
      m_str_copy(buf, "(none)", sizeof(buf));
      break;

    case SDL_CONTROLLER_AXIS_LEFTX:
      m_str_copy(buf, "Left X", sizeof(buf));
      break;

    case SDL_CONTROLLER_AXIS_LEFTY:
      m_str_copy(buf, "Left Y", sizeof(buf));
      break;

    case SDL_CONTROLLER_AXIS_RIGHTX:
      m_str_copy(buf, "Right X", sizeof(buf));
      break;

    case SDL_CONTROLLER_AXIS_RIGHTY:
      m_str_copy(buf, "Right Y", sizeof(buf));
      break;

    default:
      m_str_copy(buf, "(unknown)", sizeof(buf));
      break;
    }
}

static void txt_gamepad_axis_drawer(TXT_UNCAST_ARG(joystick_axis))
{
  TXT_CAST_ARG(txt_joystick_axis_t, joystick_axis);
  char buf[JOYSTICK_AXIS_WIDTH + 1];
  int i;

  get_axis_description(*joystick_axis->axis, buf, sizeof(buf));

  txt_set_widget_bg(joystick_axis);
  txt_fgcolour(TXT_COLOR_BRIGHT_WHITE);

  txt_draw_string(buf);

  for (i = txt_utf8_strlen(buf); i < joystick_axis->widget.w; ++i)
    {
      txt_draw_string(" ");
    }
}

static void txt_joystick_axis_destructor(TXT_UNCAST_ARG(joystick_axis))
{
}

static int txt_joystick_axis_keypress(TXT_UNCAST_ARG(joystick_axis), int key)
{
  TXT_CAST_ARG(txt_joystick_axis_t, joystick_axis);

  if (key == KEY_ENTER)
    {
      txt_configure_joystick_axis(joystick_axis, -1, NULL);
      return 1;
    }

  if (key == KEY_BACKSPACE || key == KEY_DEL)
    {
      *joystick_axis->axis = -1;
    }

  return 0;
}

static int txt_gamepad_axis_keypress(TXT_UNCAST_ARG(joystick_axis), int key)
{
  TXT_CAST_ARG(txt_joystick_axis_t, joystick_axis);

  if (key == KEY_ENTER)
    {
      TXT_ConfigureGamepadAxis(joystick_axis, -1, NULL);
      return 1;
    }

  if (key == KEY_BACKSPACE || key == KEY_DEL)
    {
      *joystick_axis->axis = -1;
    }

  return 0;
}

static void txt_joystick_axis_mousepress(TXT_UNCAST_ARG(widget), int x,
                                         int y, int b)
{
  TXT_CAST_ARG(txt_joystick_axis_t, widget);

  /* Clicking is like pressing enter */

  if (b == TXT_MOUSE_LEFT)
    {
      txt_joystick_axis_keypress(widget, KEY_ENTER);
    }
}

static void txt_gamepad_axis_mousepress(TXT_UNCAST_ARG(widget), int x,
                                        int y, int b)
{
  TXT_CAST_ARG(txt_joystick_axis_t, widget);

  /* Clicking is like pressing enter */

  if (b == TXT_MOUSE_LEFT)
    {
      txt_gamepad_axis_keypress(widget, KEY_ENTER);
    }
}

static void txt_configure_gamepad_axis(txt_joystick_axis_t *joystick_axis,
                              int using_button,
                              txt_joystick_axis_callback_t callback)
{
  /* Build the prompt window. */

  joystick_axis->config_window = txt_new_window("Configure axis");
  txt_set_table_columns(joystick_axis->config_window, 2);
  txt_set_column_widths(joystick_axis->config_window, 10, 5);
  txt_add_widgets(joystick_axis->config_window,
                  txt_new_check_box("Invert", joystick_axis->invert),
                  TXT_TABLE_EMPTY, txt_new_label("Dead zone"),
                  txt_newspin_control(joystick_axis->dead_zone, 10, 90),
                  NULL);

  txt_set_window_action(joystick_axis->config_window, TXT_HORIZ_LEFT, NULL);
  txt_set_window_action(
      joystick_axis->config_window, TXT_HORIZ_CENTER,
      txt_new_window_escape_action(joystick_axis->config_window));
  txt_set_window_action(joystick_axis->config_window, TXT_HORIZ_RIGHT, NULL);
  txt_set_widget_align(joystick_axis->config_window, TXT_HORIZ_CENTER);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void txt_configure_joystick_axis(txt_joystick_axis_t *joystick_axis,
                                 int using_button,
                                 txt_joystick_axis_callback_t callback)
{
  /* Open the joystick first. */

  if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
    {
      return;
    }

  joystick_axis->joystick = SDL_JoystickOpen(joystick_index);
  if (joystick_axis->joystick == NULL)
    {
      txt_message_box(NULL, "Please configure a controller first!");
      return;
    }

  SDL_JoystickEventState(SDL_ENABLE);

  /* Build the prompt window. */

  joystick_axis->config_window =
      txt_new_window("Gamepad/Joystick calibration");
  txt_add_widgets(joystick_axis->config_window, txt_new_strut(0, 1),
                  joystick_axis->config_label = txt_new_label(""),
                  txt_new_strut(0, 1), NULL);

  txt_set_window_action(joystick_axis->config_window, TXT_HORIZ_LEFT, NULL);
  txt_set_window_action(
      joystick_axis->config_window, TXT_HORIZ_CENTER,
      txt_new_window_abort_action(joystick_axis->config_window));
  txt_set_window_action(joystick_axis->config_window, TXT_HORIZ_RIGHT, NULL);
  txt_set_widget_align(joystick_axis->config_window, TXT_HORIZ_CENTER);

  if (using_button >= 0)
    {
      joystick_axis->config_stage = CONFIG_STAGE1;
      joystick_axis->config_button = using_button;
      identifiy_bad_axes(joystick_axis);
    }
  else
    {
      joystick_axis->config_stage = CONFIG_CENTER;
    }

  set_calibration_lable(joystick_axis);

  /* Close the joystick and shut down joystick subsystem when the window
   * is closed.
   */

  txt_signal_connect(joystick_axis->config_window, "closed",
                     CalibrateWindowClosed, joystick_axis);

  txt_sdl_set_event_callback(EventCallback, joystick_axis);

  /* When successfully calibrated, invoke this callback: */

  joystick_axis->callback = callback;
}

txt_joystick_axis_t *txt_newjoystick_axis(int *axis, int *invert,
                                          int *dead_zone,
                                          txt_joystick_axis_direction_t dir)
{
  txt_joystick_axis_t *joystick_axis;

  joystick_axis = malloc(sizeof(txt_joystick_axis_t));

  if (use_gamepad)
    {
      txt_init_widget(joystick_axis, &txt_gamepad_axis_class);
    }
  else
    {
      txt_init_widget(joystick_axis, &txt_joystick_axis_class);
    }

  joystick_axis->axis = axis;
  joystick_axis->invert = invert;
  joystick_axis->dead_zone = dead_zone;
  joystick_axis->dir = dir;
  joystick_axis->bad_axis = NULL;

  return joystick_axis;
}
