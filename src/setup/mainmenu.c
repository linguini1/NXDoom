/****************************************************************************
 * apps/games/NXDoom/src/setup/mainmenu.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "textscreen.h"

#include "execute.h"

#include "m_argv.h"
#include "m_config.h"
#include "m_controls.h"
#include "m_misc.h"
#include "z_zone.h"

#include "mode.h"
#include "setup_icon.c"

#include "compatibility.h"
#include "display.h"
#include "joystick.h"
#include "keyboard.h"
#include "mouse.h"
#include "multiplayer.h"
#include "sound.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WINDOW_HELP_URL "https://www.chocolate-doom.org/setup"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const int cheat_sequence[] =
{
  KEY_UPARROW,   KEY_UPARROW,    KEY_DOWNARROW,
  KEY_DOWNARROW, KEY_LEFTARROW,  KEY_RIGHTARROW,
  KEY_LEFTARROW, KEY_RIGHTARROW, 'b',
  'a',           KEY_ENTER,      0,
};

static unsigned int cheat_sequence_index = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* I think these are good "sensible" defaults: */

static void sensible_defaults(void)
{
  key_up = 'w';
  key_down = 's';
  key_strafeleft = 'a';
  key_straferight = 'd';
  key_jump = '/';
  key_lookup = KEY_PGUP;
  key_lookdown = KEY_PGDN;
  key_lookcenter = KEY_HOME;
  key_flyup = KEY_INS;
  key_flydown = KEY_DEL;
  key_flycenter = KEY_END;
  key_prevweapon = ',';
  key_nextweapon = '.';
  key_invleft = '[';
  key_invright = ']';
  key_message_refresh = '\'';
  key_mission = 'i'; /* Strife keys */
  key_invpop = 'o';
  key_invkey = 'p';
  key_multi_msgplayer[0] = 'g';
  key_multi_msgplayer[1] = 'h';
  key_multi_msgplayer[2] = 'j';
  key_multi_msgplayer[3] = 'k';
  key_multi_msgplayer[4] = 'v';
  key_multi_msgplayer[5] = 'b';
  key_multi_msgplayer[6] = 'n';
  key_multi_msgplayer[7] = 'm';
  mousebprevweapon = 4; /* Scroll wheel = weapon cycle */
  mousebnextweapon = 3;
  snd_musicdevice = 3;
  joybspeed = 29; /* Always run */
  vanilla_savegame_limit = 0;
  vanilla_keyboard_mapping = 0;
  vanilla_demo_limit = 0;
  graphical_startup = 0;
  show_endoom = 0;
  dclick_use = 0;
  novert = 1;
  png_screenshots = 1;
}

static int main_menu_key_press(txt_window_t *window, int key,
        void *user_data)
{
  if (key == cheat_sequence[cheat_sequence_index])
    {
      ++cheat_sequence_index;

      if (cheat_sequence[cheat_sequence_index] == 0)
        {
          sensible_defaults();
          cheat_sequence_index = 0;

          window = TXT_MessageBox(NULL, "    \x01    ");

          return 1;
        }
    }
  else
    {
      cheat_sequence_index = 0;
    }

  return 0;
}

static void do_quit(void *widget, void *dosave)
{
  if (dosave != NULL)
    {
      m_save_defaults();
    }

  TXT_Shutdown();

  exit(0);
}

static void quit_confirm(void *unused1, void *unused2)
{
  txt_window_t *window;
  txt_label_t *label;
  txt_button_t *yes_button;
  txt_button_t *no_button;

  window = TXT_NewWindow(NULL);

  TXT_AddWidgets(window,
                 label = TXT_NewLabel("Exiting setup.\nSave settings?"),
                 TXT_NewStrut(24, 0),
                 yes_button = TXT_NewButton2("  Yes  ", DoQuit, DoQuit),
                 no_button = TXT_NewButton2("  No   ", DoQuit, NULL), NULL);

  TXT_SetWidgetAlign(label, TXT_HORIZ_CENTER);
  TXT_SetWidgetAlign(yes_button, TXT_HORIZ_CENTER);
  TXT_SetWidgetAlign(no_button, TXT_HORIZ_CENTER);

  /* Only an "abort" button in the middle. */

  TXT_SetWindowAction(window, TXT_HORIZ_LEFT, NULL);
  TXT_SetWindowAction(window, TXT_HORIZ_CENTER,
                      TXT_NewWindowAbortAction(window));
  TXT_SetWindowAction(window, TXT_HORIZ_RIGHT, NULL);
}

static void launch_doom(void *unused1, void *unused2)
{
  execute_context_t *exec;

  /* Save configuration first */

  m_save_defaults();

  /* Shut down textscreen GUI */

  TXT_Shutdown();

  /* Launch Doom */

  exec = NewExecuteContext();
  PassThroughArguments(exec);
  ExecuteDoom(exec);

  exit(0);
}

static txt_button_t *get_launch_button(void)
{
  const char *label;

  switch (gamemission)
    {
    case doom:
      label = "Save parameters and launch DOOM";
      break;
    case heretic:
      label = "Save parameters and launch Heretic";
      break;
    case hexen:
      label = "Save parameters and launch Hexen";
      break;
    case strife:
      label = "Save parameters and launch STRIFE!";
      break;
    default:
      label = "Save parameters and launch game";
      break;
    }

  return TXT_NewButton2(label, launch_doom, NULL);
}

/* Initialize all configuration variables, load config file, etc */

static void init_config(void)
{
  m_set_config_dir(NULL);
  init_bindings();

  set_chat_macro_defaults();
  set_player_name_default();

  m_load_defaults();

  /* Create and configure the music pack directory if it does not
   * already exist.
   */

  m_set_music_pack_dir();
}

/* Application icon */

static void set_icon(void)
{
  SDL_Surface *surface;

  surface = SDL_CreateRGBSurfaceFrom(
      (void *)setup_icon_data, setup_icon_w, setup_icon_h, 32,
      setup_icon_w * 4, 0xffu << 24, 0xffu << 16, 0xffu << 8, 0xffu << 0);

  SDL_SetWindowIcon(TXT_SDLWindow, surface);
  SDL_FreeSurface(surface);
}

static void set_window_title(void)
{
  char *title;

  title = m_string_replace(PACKAGE_NAME " Setup ver " PACKAGE_VERSION,
          "Doom", get_game_title());

  TXT_SetDesktopTitle(title);

  free(title);
}

/* Initialize the textscreen library. */

static void init_text_screen(void)
{
  SetDisplayDriver();

  if (!TXT_Init())
    {
      fprintf(stderr, "Failed to initialize GUI\n");
      exit(-1);
    }

  /* Set Romero's "funky blue" color:
   * <https://doomwiki.org/wiki/Romero_Blue>
   */

  TXT_SetColor(TXT_COLOR_BLUE, 0x04, 0x14, 0x40);

  set_icon();
  set_window_title();
}

/* Initialize and run the textscreen GUI. */

static void run_gui(void)
{
  init_text_screen();
  TXT_GUIMainLoop();
}

static void mission_set(void)
{
  set_window_title();
  init_config();
  main_menu();
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void main_menu(void)
{
  txt_window_t *window;
  txt_window_action_t *quit_action;
  txt_window_action_t *warp_action;

  window = TXT_NewWindow("Main Menu");

  TXT_SetWindowHelpURL(window, WINDOW_HELP_URL);

  TXT_AddWidgets(
      window,
      TXT_NewButton2("Configure Display", (TxtWidgetSignalFunc)ConfigDisplay,
                     NULL),
      TXT_NewButton2("Configure Sound", (TxtWidgetSignalFunc)ConfigSound,
                     NULL),
      TXT_NewButton2("Configure Keyboard",
                     (TxtWidgetSignalFunc)ConfigKeyboard, NULL),
      TXT_NewButton2("Configure Mouse", (TxtWidgetSignalFunc)ConfigMouse,
                     NULL),
      TXT_NewButton2("Configure Gamepad/Joystick",
                     (TxtWidgetSignalFunc)ConfigJoystick, NULL),
      TXT_NewButton2("Compatibility",
                     (TxtWidgetSignalFunc)CompatibilitySettings, NULL),
      GetLaunchButton(), TXT_NewStrut(0, 1),
      TXT_NewButton2("Start a Network Game",
                     (TxtWidgetSignalFunc)start_multi_game, NULL),
      TXT_NewButton2("Join a Network Game",
                     (TxtWidgetSignalFunc)join_multi_game, NULL),
      TXT_NewButton2("Multiplayer Configuration",
                     (TxtWidgetSignalFunc)multiplayer_config, NULL),
      NULL);

  quit_action = TXT_NewWindowAction(KEY_ESCAPE, "Quit");
  warp_action = TXT_NewWindowAction(KEY_F2, "Warp");
  TXT_SignalConnect(quit_action, "pressed", quit_confirm, NULL);
  TXT_SignalConnect(warp_action, "pressed", (TxtWidgetSignalFunc)warp_menu,
                    NULL);
  TXT_SetWindowAction(window, TXT_HORIZ_LEFT, quit_action);
  TXT_SetWindowAction(window, TXT_HORIZ_CENTER, warp_action);

  TXT_SetKeyListener(window, main_menuKeyPress, NULL);
}

void d_doom_main(void)
{
  SetupMission(mission_set);

  run_gui();
}
