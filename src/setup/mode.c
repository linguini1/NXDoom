/*
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
 */

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
  GameMission_t mission;
  int mask;
  const char *name;
  const char *config_file;
  const char *extra_config_file;
  const char *executable;
} mission_config_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const iwad_t **iwads;

static mission_config_t mission_configs[] = {
    {"Doom", doom, IWAD_MASK_DOOM, "doom", "default.cfg",
     PROGRAM_PREFIX "doom.cfg", PROGRAM_PREFIX "doom"},
    {"Heretic", heretic, IWAD_MASK_HERETIC, "heretic", "heretic.cfg",
     PROGRAM_PREFIX "heretic.cfg", PROGRAM_PREFIX "heretic"},
    {"Hexen", hexen, IWAD_MASK_HEXEN, "hexen", "hexen.cfg",
     PROGRAM_PREFIX "hexen.cfg", PROGRAM_PREFIX "hexen"},
    {"Strife", strife, IWAD_MASK_STRIFE, "strife", "strife.cfg",
     PROGRAM_PREFIX "strife.cfg", PROGRAM_PREFIX "strife"}};

static GameSelectCallback game_selected_callback;

/* Miscellaneous variables that aren't used in setup. */

static int showMessages = 1;
static int screenblocks = 9;
static int detailLevel = 0;
static char *savedir = NULL;
static char *executable = NULL;
static const char *game_title = "Doom";
static char *back_flat = "F_PAVE01";
static int comport = 0;
static char *nickname = NULL;

/****************************************************************************
 * Public Data
 ****************************************************************************/

GameMission_t gamemission;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void bind_misc_variables(void)
{
  if (gamemission == doom)
    {
      m_bind_int_variable("detaillevel", &detailLevel);
      m_bind_int_variable("show_messages", &showMessages);
    }

  if (gamemission == hexen)
    {
      m_bind_string_variable("savedir", &savedir);
      m_bind_int_variable("messageson", &showMessages);

      /* Hexen has a variable to control the savegame directory that is used.
       */

      savedir = m_get_save_game_dir("hexen.wad");

      /* On Windows, hexndata\ is the default. */

      if (!strcmp(savedir, ""))
        {
          free(savedir);
          savedir = "hexndata" DIR_SEPARATOR_S;
        }
    }

  if (gamemission == strife)
    {
      /* Strife has a different default value than the other games */

      screenblocks = 10;

      m_bind_string_variable("back_flat", &back_flat);
      m_bind_string_variable("nickname", &nickname);

      m_bind_int_variable("screensize", &screenblocks);
      m_bind_int_variable("comport", &comport);
    }
  else
    {
      m_bind_int_variable("screenblocks", &screenblocks);
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

  free(executable);
  extension = "";
  executable = m_string_join(config->executable, extension, NULL);
}

static void set_mission(mission_config_t *config)
{
  iwads = D_FindAllIWADs(config->mask);
  gamemission = config->mission;
  set_executable(config);
  game_title = config->label;
  m_set_config_filenames(config->config_file, config->extra_config_file);
}

static mission_config_t *get_mission_for_name(const char *name)
{
  int i;

  for (i = 0; i < arrlen(mission_configs); ++i)
    {
      if (!strcmp(mission_configs[i].name, name))
        {
          return &mission_configs[i];
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

static boolean check_executable_name(GameSelectCallback callback)
{
  mission_config_t *config;
  const char *exe_name;
  int i;

  exe_name = m_get_executable_name();

  for (i = 0; i < arrlen(mission_configs); ++i)
    {
      config = &mission_configs[i];

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
  game_selected_callback();
}

static void open_game_select_dialog(GameSelectCallback callback)
{
  mission_config_t *mission = NULL;
  txt_window_t *window;
  const iwad_t **iwads;
  int num_games;
  int i;

  window = TXT_NewWindow("Select game");

  TXT_AddWidget(window, TXT_NewLabel("Select a game to configure:\n"));
  num_games = 0;

  /* Add a button for each game. */

  for (i = 0; i < arrlen(mission_configs); ++i)
    {
      /* Do we have any IWADs for this game installed? If so, add a button. */

      iwads = D_FindAllIWADs(mission_configs[i].mask);

      if (iwads[0] != NULL)
        {
          mission = &mission_configs[i];
          TXT_AddWidget(window,
                        TXT_NewButton2(mission_configs[i].label, GameSelected,
                                       &mission_configs[i]));
          ++num_games;
        }

      free(iwads);
    }

  TXT_AddWidget(window, TXT_NewStrut(0, 1));

  /* No IWADs found at all?  Fall back to doom, then. */

  if (num_games == 0)
    {
      TXT_CloseWindow(window);
      set_mission(DEFAULT_MISSION);
      callback();
      return;
    }

  /* Only one game? Use that game, and don't bother with a dialog. */

  if (num_games == 1)
    {
      TXT_CloseWindow(window);
      set_mission(mission);
      callback();
      return;
    }

  game_selected_callback = callback;
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
  M_ApplyPlatformDefaults();

  /* Keyboard, mouse, joystick controls */

  M_BindBaseControls();
  M_BindWeaponControls();
  M_BindMapControls();
  M_BindMenuControls();

  if (gamemission == heretic || gamemission == hexen)
    {
      M_BindHereticControls();
    }

  if (gamemission == hexen)
    {
      M_BindHexenControls();
    }

  if (gamemission == strife)
    {
      M_BindStrifeControls();
    }

  /* All other variables */

  BindCompatibilityVariables();
  BindDisplayVariables();
  BindJoystickVariables();
  BindKeyboardVariables();
  BindMouseVariables();
  BindSoundVariables();
  bind_misc_variables();
  BindMultiplayerVariables();
}

void setup_mission(GameSelectCallback callback)
{
  mission_config_t *config;
  const char *mission_name;
  int p;

  /* Specify the game to configure the settings for.  Valid values are 'doom',
   * 'heretic', 'hexen' and 'strife'.
   */

  p = m_check_parm("-game");

  if (p > 0)
    {
      mission_name = myargv[p + 1];

      config = get_mission_for_name(mission_name);

      if (config == NULL)
        {
          I_Error("Invalid parameter - '%s'", mission_name);
        }

      set_mission(config);
      callback();
    }
  else if (!CheckExecutableName(callback))
    {
      open_game_select_dialog(callback);
    }
}

const char *get_executable_name(void) { return executable; }

const char *get_game_title(void) { return game_title; }

const iwad_t **get_iwads(void) { return iwads; }
