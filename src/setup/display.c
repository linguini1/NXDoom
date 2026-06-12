/****************************************************************************
 * apps/games/NXDoom/src/setup/display.c
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

#include "m_config.h"
#include "m_misc.h"
#include "mode.h"
#include "textscreen.h"

#include "config.h"
#include "display.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WINDOW_HELP_URL "https://www.chocolate-doom.org/setup-display"

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  int w;
  int h;
} window_size_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* List of aspect ratio-uncorrected window sizes: */

static window_size_t g_window_sizes_unscaled[] =
{
    {
        .w = 320,
        .h = 200,
    },
    {
        .w = 640,
        .h = 400,
    },
    {
        .w = 960,
        .h = 600,
    },
    {
        .w = 1280,
        .h = 800,
    },
    {
        .w = 1600,
        .h = 1000,
    },
    {
        .w = 0,
        .h = 0,
    },
};

/* List of aspect ratio-corrected window sizes: */

static window_size_t g_window_sizes_scaled[] =
{
    {
        .w = 320,
        .h = 240,
    },
    {
        .w = 512,
        .h = 400,
    },
    {
        .w = 640,
        .h = 480,
    },
    {
        .w = 800,
        .h = 600,
    },
    {
        .w = 960,
        .h = 720,
    },
    {
        .w = 1024,
        .h = 800,
    },
    {
        .w = 1280,
        .h = 960,
    },
    {
        .w = 1600,
        .h = 1200,
    },
    {
        .w = 1920,
        .h = 1440,
    },
    {
        .w = 0,
        .h = 0,
    },
};

static char *video_driver = "";
static char *window_position = "";
static int video_display = 0;
static int aspect_ratio_correct = 1;
static int integer_scaling = 0;
static int smooth_pixel_scaling = 1;
static int vga_porch_flash = 0;
static int force_software_renderer = 0;
static int fullscreen = 1;
static int fullscreen_width = 0;
static int fullscreen_height = 0;
static int window_width = 800;
static int window_height = 600;
static int startup_delay = 1000;
static int max_scaling_buffer_pixels = 16000000;
static int usegamma = 0;
static int system_video_env_set;

/****************************************************************************
 * Public Data
 ****************************************************************************/

int graphical_startup = 1;
int show_endoom = 1;
int show_diskicon = 1;
int png_screenshots = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void window_size_selected(TXT_UNCAST_ARG(widget),
        TXT_UNCAST_ARG(size))
{
  TXT_CAST_ARG(window_size_t, size);

  window_width = size->w;
  window_height = size->h;
}

static txt_radiobutton_t *size_select_button(window_size_t *size)
{
  char buf[15];
  txt_radiobutton_t *result;

  snprintf(buf, sizeof(buf), "%ix%i", size->w, size->h);
  result = txt_new_radio_button(buf, &window_width, size->w);
  txt_signal_connect(result, "selected", window_size_selected, size);

  return result;
}

static void generate_sizes_table(TXT_UNCAST_ARG(widget),
                               TXT_UNCAST_ARG(sizes_table))
{
  TXT_CAST_ARG(txt_table_t, sizes_table);
  window_size_t *sizes;
  boolean have_size;
  int i;

  /* Pick which window sizes list to use */

  if (aspect_ratio_correct == 1)
    {
      sizes = g_window_sizes_scaled;
    }
  else
    {
      sizes = g_window_sizes_unscaled;
    }

  /* Build the table */

  txt_clear_table(sizes_table);
  txt_set_column_widths(sizes_table, 14, 14, 14);

  txt_add_widget(sizes_table, txt_new_separator("Window size"));

  have_size = false;

  for (i = 0; sizes[i].w != 0; ++i)
    {
      txt_add_widget(sizes_table, size_select_button(&sizes[i]));
      have_size = have_size || window_width == sizes[i].w;
    }

  /* Windows can be any arbitrary size. We key off the width of the
   * window in pixels. If the current size is not in the list of
   * standard (integer multiply) sizes, create a special button to
   * mean "the current window size".
   */

  if (!have_size)
    {
      static window_size_t current_size;
      current_size.w = window_width;
      current_size.h = window_height;
      txt_add_widget(sizes_table, size_select_button(&current_size));
    }
}

static void advanced_display_config(TXT_UNCAST_ARG(widget),
                                  TXT_UNCAST_ARG(sizes_table))
{
  TXT_CAST_ARG(txt_table_t, sizes_table);
  txt_window_t *window;
  txt_checkbox_t *ar_checkbox;

  window = txt_new_window("Advanced display options");

  txt_set_window_help_url(window, WINDOW_HELP_URL);

  txt_set_column_widths(window, 40);

  txt_add_widgets(
      window,
      ar_checkbox = txt_new_check_box("Force correct aspect ratio",
                                      &aspect_ratio_correct),
      txt_if(gamemission == heretic || gamemission == hexen ||
                 gamemission == strife,
             txt_new_check_box("Graphical startup", &graphical_startup)),
      txt_if(gamemission == doom || gamemission == heretic ||
                 gamemission == strife,
             txt_new_check_box("Show ENDOOM screen on exit", &show_endoom)),
      txt_new_check_box("Smooth pixel scaling",
          &smooth_pixel_scaling), NULL);

  txt_signal_connect(ar_checkbox, "changed",
                    generate_sizes_table, sizes_table);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Set the SDL_VIDEODRIVER environment variable */

void set_display_driver(void)
{
  static int first_time = 1;

  if (first_time)
    {
      system_video_env_set = getenv("SDL_VIDEODRIVER") != NULL;

      first_time = 0;
    }

  /* Don't override the command line environment, if it has been set. */

  if (system_video_env_set)
    {
      return;
    }

  /* Use the value from the configuration file, if it has been set. */

  if (strcmp(video_driver, "") != 0)
    {
      char *env_string;

      env_string = m_string_join("SDL_VIDEODRIVER=", video_driver, NULL);
      putenv(env_string);
      free(env_string);
    }
}

void config_display(TXT_UNCAST_ARG(widget), void *user_data)
{
  txt_window_t *window;
  txt_table_t *sizes_table;
  txt_window_action_t *advanced_button;

  /* Open the window */

  window = txt_new_window("Display Configuration");
  txt_set_window_help_url(window, WINDOW_HELP_URL);

  /* Build window: */

  txt_add_widgets(
      window, txt_new_check_box("Full screen", &fullscreen),
      txt_new_conidtional(&fullscreen, 0, sizes_table = txt_new_table(3)),
      NULL);

  txt_set_column_widths(window, 42);

  /* The window is set at a fixed vertical position.  This keeps
   * the top of the window stationary when switching between
   * fullscreen and windowed mode (which causes the window's
   * height to change).
   */

  txt_set_window_position(window, TXT_HORIZ_CENTER, TXT_VERT_TOP,
                          TXT_SCREEN_W / 2, 6);

  generate_sizes_table(NULL, sizes_table);

  /* Button to open "advanced" window.
   * Need to pass a pointer to the window sizes table, as some of the options
   * in there trigger a rebuild of it.
   */

  advanced_button = txt_new_window_action('a', "Advanced");
  txt_set_window_action(window, TXT_HORIZ_CENTER, advanced_button);
  txt_signal_connect(advanced_button, "pressed", advanced_display_config,
                     sizes_table);
}

void bind_display_variables(void)
{
  m_bind_int_variable("video_display", &video_display);
  m_bind_int_variable("aspect_ratio_correct", &aspect_ratio_correct);
  m_bind_int_variable("integer_scaling", &integer_scaling);
  m_bind_int_variable("smooth_pixel_scaling", &smooth_pixel_scaling);
  m_bind_int_variable("fullscreen", &fullscreen);
  m_bind_int_variable("fullscreen_width", &fullscreen_width);
  m_bind_int_variable("fullscreen_height", &fullscreen_height);
  m_bind_int_variable("window_width", &window_width);
  m_bind_int_variable("window_height", &window_height);
  m_bind_int_variable("startup_delay", &startup_delay);
  m_bind_string_variable("video_driver", &video_driver);
  m_bind_string_variable("window_position", &window_position);
  m_bind_int_variable("usegamma", &usegamma);
  m_bind_int_variable("png_screenshots", &png_screenshots);
  m_bind_int_variable("vga_porch_flash", &vga_porch_flash);
  m_bind_int_variable("force_software_renderer", &force_software_renderer);
  m_bind_int_variable("max_scaling_buffer_pixels",
                      &max_scaling_buffer_pixels);

  if (gamemission == doom || gamemission == heretic || gamemission == strife)
    {
      m_bind_int_variable("show_endoom", &show_endoom);
    }

  if (gamemission == doom || gamemission == strife)
    {
      m_bind_int_variable("show_diskicon", &show_diskicon);
    }

  if (gamemission == heretic || gamemission == hexen ||
      gamemission == strife)
    {
      m_bind_int_variable("graphical_startup", &graphical_startup);
    }
}
