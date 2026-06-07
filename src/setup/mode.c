/****************************************************************************
 * apps/games/NXDoom/src/setup/mode.c
 *
 * SPDX-License-Identifier: GPLv2
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

#include "doomtype.h"

#include "config.h"
#include "textscreen.h"

#include "d_iwad.h"
#include "d_mode.h"
#include "doomtype.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_controls.h"
#include "m_misc.h"

#include "compatibility.h"
#include "display.h"
#include "joystick.h"
#include "keyboard.h"
#include "mouse.h"
#include "multiplayer.h"
#include "sound.h"

#include "mode.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Default mission to fall back on, if no IWADs are found at all: */

#define DEFAULT_MISSION (&mission_configs[0])

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  const char *label;
  gamemission_t mission;
  int mask;
  const char *name;
  const char *config_file;
  const char *extra_config_file;
  const char *executable;
} mission_config_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const iwad_t **g_iwads;

static mission_config_t g_mission_configs[] =
{
    {
        "Doom",
        doom,
        IWAD_MASK_DOOM,
        "doom",
        "default.cfg",
        PROGRAM_PREFIX "doom.cfg",
        PROGRAM_PREFIX "doom",
    },
    {
        "Heretic",
        heretic,
        IWAD_MASK_HERETIC,
        "heretic",
        "heretic.cfg",
        PROGRAM_PREFIX "heretic.cfg",
        PROGRAM_PREFIX "heretic",
    },
    {
        "Hexen",
        hexen,
        IWAD_MASK_HEXEN,
        "hexen",
        "hexen.cfg",
        PROGRAM_PREFIX "hexen.cfg",
        PROGRAM_PREFIX "hexen",
    },
    {
        "Strife",
        strife,
        IWAD_MASK_STRIFE,
        "strife",
        "strife.cfg",
        PROGRAM_PREFIX "strife.cfg",
        PROGRAM_PREFIX "strife",
    },
};

static game_select_callback g_game_selected_callback;

/* Miscellaneous variables that aren't used in setup. */

static int g_show_messages = 1;
static int g_screenblocks = 9;
static int g_detail_level = 0;
static char *g_savedir = NULL;
static char *g_executable = NULL;
static const char *g_game_title = "Doom";
static char *g_back_flat = "F_PAVE01";
static int g_comport = 0;
static char *g_nickname = NULL;

/****************************************************************************
 * Public Data
 ****************************************************************************/

gamemission_t gamemission;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void bind_misc_variables(void)
{
  if (gamemission == doom)
    {
      m_bind_int_variable("detaillevel", &g_detail_level);
      m_bind_int_variable("show_messages", &g_show_messages);
    }

  if (gamemission == hexen)
    {
      m_bind_string_variable("savedir", &g_savedir);
      m_bind_int_variable("messageson", &g_show_messages);

      /* Hexen has a variable to control the savegame directory that is used.
       */

      g_savedir = m_get_save_game_dir("hexen.wad");

      /* On Windows, hexndata\ is the default. */

      if (!strcmp(g_savedir, ""))
        {
          free(g_savedir);
          g_savedir = "hexndata" DIR_SEPARATOR_S;
        }
    }

  if (gamemission == strife)
    {
      /* Strife has a different default value than the other games */

      g_screenblocks = 10;

      m_bind_string_variable("back_flat", &g_back_flat);
      m_bind_string_variable("nickname", &g_nickname);

      m_bind_int_variable("screensize", &g_screenblocks);
      m_bind_int_variable("comport", &g_comport);
    }
  else
    {
      m_bind_int_variable("screenblocks", &g_screenblocks);
    }
}

/****************************************************************************
 * Name: translate_key
 *
 * Description:
 *  Set the name of the executable program to run the game:
 *
 ****************************************************************************/

static void set_executable(mission_config_t *config)
{
  char *extension;

  free(g_executable);
  extension = "";
  g_executable = m_string_join(config->executable, extension, NULL);
}

static void set_mission(mission_config_t *config)
{
  g_iwads = d_find_all_iwads(config->mask);
  gamemission = config->mission;
  set_executable(config);
  g_game_title = config->label;
  m_set_config_filenames(config->config_file, config->extra_config_file);
}

static mission_config_t *get_mission_for_name(const char *name)
{
  int i;

  for (i = 0; i < arrlen(g_mission_configs); ++i)
    {
      if (!strcmp(g_mission_configs[i].name, name))
        {
          return &g_mission_configs[i];
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: check_executable_name
 *
 * Description:
 *  Check the name of the executable.  If it contains one of the game names
 * (eg. chocolate-hexen-setup.exe) then use that game.
 *
 ****************************************************************************/

static boolean check_executable_name(game_select_callback callback)
{
  mission_config_t *config;
  const char *exe_name;
  int i;

  exe_name = m_get_executable_name();

  for (i = 0; i < arrlen(g_mission_configs); ++i)
    {
      config = &g_mission_configs[i];

      if (strstr(exe_name, config->name) != NULL)
        {
          set_mission(config);
          callback();
          return true;
        }
    }

  return false;
}

static void game_selected(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(config))
{
  TXT_CAST_ARG(mission_config_t, config);

  set_mission(config);
  g_game_selected_callback();
}

static void open_game_select_dialog(game_select_callback callback)
{
  mission_config_t *mission = NULL;
  txt_window_t *window;
  const iwad_t **iwads;
  int num_games;
  int i;

  window = txt_new_window("Select game");

  txt_add_widget(window, txt_new_label("Select a game to configure:\n"));
  num_games = 0;

  /* Add a button for each game. */

  for (i = 0; i < arrlen(g_mission_configs); ++i)
    {
      /* Do we have any IWADs for this game installed?
       * If so, add a button.
       */

      iwads = d_find_all_iwads(g_mission_configs[i].mask);

      if (iwads[0] != NULL)
        {
          mission = &g_mission_configs[i];
          txt_add_widget(window, txt_new_button2(g_mission_configs[i].label,
                                                 game_selected,
                                                 &g_mission_configs[i]));
          ++num_games;
        }

      free(iwads);
    }

  txt_add_widget(window, txt_new_strut(0, 1));

  /* No IWADs found at all?  Fall back to doom, then. */

  if (num_games == 0)
    {
      txt_close_window(window);
      set_mission(DEFAULT_MISSION);
      callback();
      return;
    }

  /* Only one game? Use that game, and don't bother with a dialog. */

  if (num_games == 1)
    {
      txt_close_window(window);
      set_mission(mission);
      callback();
      return;
    }

  g_game_selected_callback = callback;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: init_bindings
 *
 * Description:
 *  Initialise all configuration file bindings.
 *
 ****************************************************************************/

void init_bindings(void)
{
  m_apply_platform_defaults();

  /* Keyboard, mouse, joystick controls */

  m_bind_base_controls();
  m_bind_weapon_controls();
  m_bind_map_controls();
  m_bind_menu_controls();

  if (gamemission == heretic || gamemission == hexen)
    {
      m_bind_heretic_controls();
    }

  if (gamemission == hexen)
    {
      m_bind_hexen_controls();
    }

  if (gamemission == strife)
    {
      m_bind_strife_controls();
    }

  /* All other variables */

  bind_compatibility_variables();
  bind_display_variables();
  bind_joystick_variables();
  bind_keyboard_variables();
  bind_mouse_variables();
  bind_sound_variables();
  bind_misc_variables();
  bind_multiple_variables();
}

void setup_mission(game_select_callback callback)
{
  mission_config_t *config;
  const char *mission_name;
  int p;

  /* Specify the game to configure the settings for. Valid values are
   * 'doom', 'heretic', 'hexen' and 'strife'.
   */

  p = m_check_parm("-game");

  if (p > 0)
    {
      mission_name = myargv[p + 1];

      config = get_mission_for_name(mission_name);

      if (config == NULL)
        {
          i_error("Invalid parameter - '%s'", mission_name);
        }

      set_mission(config);
      callback();
    }
  else if (!check_executable_name(callback))
    {
      open_game_select_dialog(callback);
    }
}

const char *get_executable_name(void)
{
  return g_executable;
}

const char *get_game_title(void)
{
  return g_game_title;
}

const iwad_t **get_iwads(void)
{
  return g_iwads;
}
