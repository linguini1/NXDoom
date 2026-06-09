/****************************************************************************
 * apps/games/NXDoom/src/setup/multiplayer.c
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

#include "doomtype.h"

#include "textscreen.h"

#include "d_iwad.h"
#include "doom/d_englsh.h"
#include "m_config.h"
#include "m_controls.h"
#include "m_misc.h"

#include "execute.h"
#include "mode.h"
#include "multiplayer.h"

#include "net_io.h"
#include "net_query.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MULTI_START_HELP_URL                                                 \
  "https://www.chocolate-doom.org/setup-multi-start"
#define MULTI_JOIN_HELP_URL "https://www.chocolate-doom.org/setup-multi-join"
#define MULTI_CONFIG_HELP_URL                                                \
  "https://www.chocolate-doom.org/setup-multi-config"
#define LEVEL_WARP_HELP_URL "https://www.chocolate-doom.org/setup-level-warp"

#define NUM_WADS 10
#define NUM_EXTRA_PARAMS 10

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef enum
{
  WARP_EXMY,
  WARP_MAPXY,
} warptype_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Fallback IWADs to use if no IWADs are detected. */

static const iwad_t fallback_iwads[] =
{
  {"doom.wad", doom, registered, "Doom"},
  {"heretic.wad", heretic, retail, "Heretic"},
  {"hexen.wad", hexen, commercial, "Hexen"},
  {"strife1.wad", strife, commercial, "Strife"},
};

/* Array of IWADs found to be installed */

static const iwad_t **found_iwads;
static const char **iwad_labels;

/* Index of the currently selected IWAD */

static int found_iwad_selected = -1;

/* Filename to pass to '-iwad'. */

static const char *iwadfile;

static const char *wad_extensions[] =
{
  "wad",
  "lmp",
  "deh",
  NULL,
};

static const char *doom_skills[] =
{
  "I'm too young to die.", "Hey, not too rough.", "Hurt me plenty.",
  "Ultra-Violence.",       "NIGHTMARE!",
};

static const char *chex_skills[] =
{
  "Easy does it", "Not so sticky", "Gobs of goo",
  "Extreme ooze", "SUPER SLIMEY!",
};

static const char *heretic_skills[] =
{
  "Thou needeth a wet-nurse",    "Yellowbellies-R-us",
  "Bringest them oneth",         "Thou art a smite-meister",
  "Black plague possesses thee",
};

static const char *hexen_fighter_skills[] =
{
  "Squire", "Knight", "Warrior", "Berserker", "Titan",
};

static const char *hexen_cleric_skills[] =
{
  "Altar boy", "Acolyte", "Priest", "Cardinal", "Pope",
};

static const char *hexen_mage_skills[] =
{
  "Apprentice", "Enchanter", "Sorcerer", "Warlock", "Archimage",
};

static const char *strife_skills[] =
{
  "Training", "Rookie", "Veteran", "Elite", "Bloodbath",
};

static const char *character_classes[] =
{
  "Fighter",
  "Cleric",
  "Mage",
};

static const char *gamemodes[] =
{
  "Co-operative",
  "Deathmatch",
  "Deathmatch 2.0",
};

static const char *strife_gamemodes[] =
{
  "Normal deathmatch",
  "Items respawn", /* (altdeath) */
};

static char *net_player_name;
static char *chat_macros[10];

static char *wads[NUM_WADS];
static char *extra_params[NUM_EXTRA_PARAMS];
static int character_class = 0;
static int skill = 2;
static int nomonsters = 0;
static int deathmatch = 0;
static int strife_altdeath = 0;
static int fast = 0;
static int respawn = 0;
static int udpport = 2342;
static int timer = 0;
static int privateserver = 0;

static txt_dropdown_list_t *skillbutton;
static txt_button_t *warpbutton;
static warptype_t warptype = WARP_MAPXY;
static int warpepisode = 1;
static int warpmap = 1;

/* Address to connect to when joining a game */

static char *connect_address = NULL;

static txt_window_t *query_window;
static int query_servers_found;

static const char *const g_defaults[] =
{
  HUSTR_CHATMACRO0, HUSTR_CHATMACRO1, HUSTR_CHATMACRO2, HUSTR_CHATMACRO3,
  HUSTR_CHATMACRO4, HUSTR_CHATMACRO5, HUSTR_CHATMACRO6, HUSTR_CHATMACRO7,
  HUSTR_CHATMACRO8, HUSTR_CHATMACRO9,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Find an IWAD from its description */

static const iwad_t *get_current_iwad(void)
{
  return found_iwads[found_iwad_selected];
}

/* Is the currently selected IWAD the Chex Quest chex.wad? */

static boolean is_chex_quest(const iwad_t *iwad)
{
  return !strcmp(iwad->name, "chex.wad");
}

static void add_wads(execute_context_t *exec)
{
  int have_wads = 0;
  int i;

  for (i = 0; i < NUM_WADS; ++i)
    {
      if (wads[i] != NULL && strlen(wads[i]) > 0)
        {
          if (!have_wads)
            {
              add_cmdline_parameter(exec, "-merge");
              have_wads = 1;
            }

          add_cmdline_parameter(exec, "\"%s\"", wads[i]);
        }
    }
}

static void add_extra_parameters(execute_context_t *exec)
{
  int i;

  for (i = 0; i < NUM_EXTRA_PARAMS; ++i)
    {
      if (extra_params[i] != NULL && strlen(extra_params[i]) > 0)
        {
          add_cmdline_parameter(exec, "%s", extra_params[i]);
        }
    }
}

static void add_iwad_parameter(execute_context_t *exec)
{
  if (iwadfile != NULL)
    {
      add_cmdline_parameter(exec, "-iwad %s", iwadfile);
    }
}

/* Callback function invoked to launch the game.
 * This is used when starting a server and also when starting a
 * single player game via the "warp" menu.
 */

static void start_game(int multiplayer)
{
  execute_context_t *exec;

  exec = new_execute_context();

  /* Extra parameters come first, before all others; this way,
   * they can override any of the options set in the dialog.
   */

  add_extra_parameters(exec);

  add_iwad_parameter(exec);
  add_cmdline_parameter(exec, "-skill %i", skill + 1);

  if (gamemission == hexen)
    {
      add_cmdline_parameter(exec, "-class %i", character_class);
    }

  if (nomonsters)
    {
      add_cmdline_parameter(exec, "-nomonsters");
    }

  if (fast)
    {
      add_cmdline_parameter(exec, "-fast");
    }

  if (respawn)
    {
      add_cmdline_parameter(exec, "-respawn");
    }

  if (warptype == WARP_EXMY)
    {
      /* TODO: select IWAD based on warp type */

      add_cmdline_parameter(exec, "-warp %i %i", warpepisode, warpmap);
    }
  else if (warptype == WARP_MAPXY)
    {
      add_cmdline_parameter(exec, "-warp %i", warpmap);
    }

  /* Multiplayer-specific options: */

  if (multiplayer)
    {
      add_cmdline_parameter(exec, "-server");
      add_cmdline_parameter(exec, "-port %i", udpport);

      if (deathmatch == 1)
        {
          add_cmdline_parameter(exec, "-deathmatch");
        }
      else if (deathmatch == 2 || strife_altdeath != 0)
        {
          add_cmdline_parameter(exec, "-altdeath");
        }

      if (timer > 0)
        {
          add_cmdline_parameter(exec, "-timer %i", timer);
        }

      if (privateserver)
        {
          add_cmdline_parameter(exec, "-privateserver");
        }
    }

  add_wads(exec);

  txt_shutdown();

  m_save_defaults();
  pass_through_arguments(exec);

  execute_doom(exec);

  exit(0);
}

static void start_server_game(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(unused))
{
  start_game(1);
}

static void start_single_player_game(TXT_UNCAST_ARG(widget),
                                     TXT_UNCAST_ARG(unused))
{
  start_game(0);
}

static void update_warp_button(void)
{
  char buf[10];

  if (warptype == WARP_EXMY)
    {
      snprintf(buf, sizeof(buf), "E%iM%i", warpepisode, warpmap);
    }
  else if (warptype == WARP_MAPXY)
    {
      snprintf(buf, sizeof(buf), "MAP%02i", warpmap);
    }

  txt_set_button_label(warpbutton, buf);
}

static void update_skill_button(void)
{
  const iwad_t *iwad = get_current_iwad();

  if (is_chex_quest(iwad))
    {
      skillbutton->values = chex_skills;
    }
  else
    switch (gamemission)
      {
      default:
      case doom:
        skillbutton->values = doom_skills;
        break;

      case heretic:
        skillbutton->values = heretic_skills;
        break;

      case hexen:
        if (character_class == 0)
          {
            skillbutton->values = hexen_fighter_skills;
          }
        else if (character_class == 1)
          {
            skillbutton->values = hexen_cleric_skills;
          }
        else
          {
            skillbutton->values = hexen_mage_skills;
          }
        break;

      case strife:
        skillbutton->values = strife_skills;
        break;
      }
}

static void set_ex_my_warp(TXT_UNCAST_ARG(widget), void *val)
{
  int l;

  l = (intptr_t)val;

  warpepisode = l / 10;
  warpmap = l % 10;

  update_warp_button();
}

static void set_map_xy_warp(TXT_UNCAST_ARG(widget), void *val)
{
  int l;

  l = (intptr_t)val;

  warpmap = l;

  update_warp_button();
}

static void close_level_select_dialog(TXT_UNCAST_ARG(button),
                                      TXT_UNCAST_ARG(window))
{
  TXT_CAST_ARG(txt_window_t, window);

  txt_close_window(window);
}

static void level_select_dialog(TXT_UNCAST_ARG(widget),
                                TXT_UNCAST_ARG(user_data))
{
  txt_window_t *window;
  txt_button_t *button;
  const iwad_t *iwad;
  char buf[10];
  int episodes;
  int x;
  int y;
  int l;
  int i;

  window = txt_new_window("Select level");
  iwad = get_current_iwad();

  if (warptype == WARP_EXMY)
    {
      episodes = d_get_num_episodes(iwad->mission, iwad->mode);
      txt_set_table_columns(window, episodes);

      /* ExMy levels */

      for (y = 1; y < 10; ++y)
        {
          for (x = 1; x <= episodes; ++x)
            {
              if (is_chex_quest(iwad) && (x > 1 || y > 5))
                {
                  continue;
                }

              if (!d_valid_episode_map(iwad->mission, iwad->mode, x, y))
                {
                  txt_add_widget(window, NULL);
                  continue;
                }

              snprintf(buf, sizeof(buf), " E%dM%d ", x, y);
              button = txt_new_button(buf);
              txt_signal_connect(button, "pressed", set_ex_my_warp,
                                 (void *)(intptr_t)(x * 10 + y));
              txt_signal_connect(button, "pressed",
                      close_level_select_dialog, window);
              txt_add_widget(window, button);

              if (warpepisode == x && warpmap == y)
                {
                  txt_select_widget(window, button);
                }
            }
        }
    }
  else
    {
      txt_set_table_columns(window, 6);

      for (i = 0; i < 60; ++i)
        {
          x = i % 6;
          y = i / 6;

          l = x * 10 + y + 1;

          if (!d_valid_episode_map(iwad->mission, iwad->mode, 1, l))
            {
              txt_add_widget(window, NULL);
              continue;
            }

          snprintf(buf, sizeof(buf), " MAP%02d ", l);
          button = txt_new_button(buf);
          txt_signal_connect(button, "pressed", set_map_xy_warp,
                             (void *)(intptr_t)l);
          txt_signal_connect(button, "pressed",
                  close_level_select_dialog, window);
          txt_add_widget(window, button);

          if (warpmap == l)
            {
              txt_select_widget(window, button);
            }
        }
    }
}

static void iwad_selected(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(unused))
{
  const iwad_t *iwad;

  /* Find the iwad_t selected */

  iwad = get_current_iwad();

  /* Update iwadfile */

  iwadfile = iwad->name;
}

/* Called when the IWAD button is changed, to update warptype. */

static void update_warp_type(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(unused))
{
  warptype_t new_warptype;
  const iwad_t *iwad;

  /* Get the selected IWAD */

  iwad = get_current_iwad();

  /* Find the new warp type */

  if (d_is_episode_map(iwad->mission))
    {
      new_warptype = WARP_EXMY;
    }
  else
    {
      new_warptype = WARP_MAPXY;
    }

  /* Reset to E1M1 / MAP01 when the warp type is changed. */

  if (new_warptype != warptype)
    {
      warpepisode = 1;
      warpmap = 1;
    }

  warptype = new_warptype;

  update_warp_button();
  update_skill_button();
}

/* Get an IWAD list with a default fallback IWAD that is appropriate
 * for the game we are configuring (matches gamemission global variable).
 */

static const iwad_t **get_fallback_iwad_list(void)
{
  static const iwad_t *fallback_iwad_list[2];
  unsigned int i;

  /* Default to use if we don't find something better. */

  fallback_iwad_list[0] = &fallback_iwads[0];
  fallback_iwad_list[1] = NULL;

  for (i = 0; i < arrlen(fallback_iwads); ++i)
    {
      if (gamemission == fallback_iwads[i].mission)
        {
          fallback_iwad_list[0] = &fallback_iwads[i];
          break;
        }
    }

  return fallback_iwad_list;
}

static txt_widget_t *iwad_selector(void)
{
  txt_dropdown_list_t *dropdown;
  txt_widget_t *result;
  int num_iwads;
  unsigned int i;

  /* Find out what WADs are installed */

  found_iwads = get_iwads();

  /* Build a list of the descriptions for all installed IWADs */

  num_iwads = 0;

  for (i = 0; found_iwads[i] != NULL; ++i)
    {
      ++num_iwads;
    }

  iwad_labels = malloc(sizeof(*iwad_labels) * num_iwads);

  for (i = 0; i < num_iwads; ++i)
    {
      iwad_labels[i] = found_iwads[i]->description;
    }

  /* If no IWADs are found, provide Doom 2 as an option, but
   * we're probably screwed.
   */

  if (num_iwads == 0)
    {
      found_iwads = get_fallback_iwad_list();
      num_iwads = 1;
    }

  /* Build a dropdown list of IWADs */

  if (num_iwads < 2)
    {
      /* We have only one IWAD. Show as a label. */

      result = (txt_widget_t *)txt_new_label(found_iwads[0]->description);
    }
  else
    {
      /* Dropdown list allowing IWAD to be selected. */

      dropdown = txt_new_dropdown_list(&found_iwad_selected,
              iwad_labels, num_iwads);

      txt_signal_connect(dropdown, "changed", iwad_selected, NULL);

      result = (txt_widget_t *)dropdown;
    }

  /* The first time the dialog is opened, found_iwad_selected=-1,
   * so select the first IWAD in the list. Don't lose the setting
   * if we close and reopen the dialog.
   */

  if (found_iwad_selected < 0 || found_iwad_selected >= num_iwads)
    {
      found_iwad_selected = 0;
    }

  iwad_selected(NULL, NULL);

  return result;
}

/* Create the window action button to start the game.  This invokes
 * a different callback depending on whether to start a multiplayer
 * or single player game.
 */

static txt_window_action_t *start_game_action(int multiplayer)
{
  txt_window_action_t *action;
  txt_widget_signal_f callback;

  action = txt_new_window_action(KEY_F10, "Start");

  if (multiplayer)
    {
      callback = start_server_game;
    }
  else
    {
      callback = start_single_player_game;
    }

  txt_signal_connect(action, "pressed", callback, NULL);

  return action;
}

static void open_wads_window(TXT_UNCAST_ARG(widget),
                             TXT_UNCAST_ARG(user_data))
{
  txt_window_t *window;
  int i;

  window = txt_new_window("Add WADs");

  for (i = 0; i < NUM_WADS; ++i)
    {
      txt_add_widget(window,
                    txt_new_file_selector(&wads[i], 60, "Select a WAD file",
                                          wad_extensions));
    }
}

static void open_extra_params_window(TXT_UNCAST_ARG(widget),
                                     TXT_UNCAST_ARG(user_data))
{
  txt_window_t *window;
  int i;

  window = txt_new_window("Extra command line parameters");

  for (i = 0; i < NUM_EXTRA_PARAMS; ++i)
    {
      txt_add_widget(window, txt_new_input_box(&extra_params[i], 70));
    }
}

static txt_window_action_t *wad_window_action(void)
{
  txt_window_action_t *action;

  action = txt_new_window_action('w', "Add WADs");
  txt_signal_connect(action, "pressed", open_wads_window, NULL);

  return action;
}

static txt_dropdown_list_t *game_type_dropdown(void)
{
  switch (gamemission)
    {
    case doom:
    default:
      return txt_new_dropdown_list(&deathmatch, gamemodes, 3);

      /* Heretic and Hexen don't support Deathmatch II: */

    case heretic:
    case hexen:
      return txt_new_dropdown_list(&deathmatch, gamemodes, 2);

      /* Strife supports both deathmatch modes, but doesn't support
       * multiplayer co-op. Use a different variable to indicate whether
       * to use altdeath or not.
       */

    case strife:
      return txt_new_dropdown_list(&strife_altdeath, strife_gamemodes, 2);
    }
}

/* "Start game" menu.  This is used for the start server window
 * and the single player warp menu.  The parameters specify
 * the window title and whether to display multiplayer options.
 */

static void start_game_menu(const char *window_title, int multiplayer)
{
  txt_window_t *window;
  txt_widget_t *iwad_selector;

  window = txt_new_window(window_title);
  txt_set_table_columns(window, 2);
  txt_set_column_widths(window, 12, 6);

  if (multiplayer)
    {
      txt_set_window_help_url(window, MULTI_START_HELP_URL);
    }
  else
    {
      txt_set_window_help_url(window, LEVEL_WARP_HELP_URL);
    }

  txt_set_window_action(window, TXT_HORIZ_CENTER, wad_window_action());
  txt_set_window_action(window, TXT_HORIZ_RIGHT,
                        start_game_action(multiplayer));

  txt_add_widgets(window, txt_new_label("Game"),
                  iwad_selector = iwad_selector(), NULL);

  if (gamemission == hexen)
    {
      txt_dropdown_list_t *cc_dropdown;
      txt_add_widgets(window, txt_new_label("Character class "),
                      cc_dropdown = txt_new_dropdown_list(
                          &character_class, character_classes, 3),
                      NULL);

      /* Update skill level dropdown when the character class is changed: */

      txt_signal_connect(cc_dropdown, "changed", update_warp_type, NULL);
    }

  txt_add_widgets(
      window, txt_new_label("Skill"),
      skillbutton = txt_new_dropdown_list(&skill, doom_skills, 5),
      txt_new_label("Level warp"),
      warpbutton = txt_new_button2("?", level_select_dialog, NULL), NULL);

  if (multiplayer)
    {
      txt_add_widgets(window, txt_new_label("Game type"),
                      game_type_dropdown(), txt_new_label("Time limit"),
                      txt_new_horiz_box(txt_new_int_input_box(&timer, 2),
                                        txt_new_label("minutes"), NULL),
                      NULL);
    }

  txt_add_widgets(window, txt_new_separator("Monster options"),
                  txt_new_inverted_checkbox("Monsters enabled", &nomonsters),
                  TXT_TABLE_OVERFLOW_RIGHT,
                  txt_new_check_box("Fast monsters", &fast),
                  TXT_TABLE_OVERFLOW_RIGHT,
                  txt_new_check_box("Respawning monsters", &respawn),
                  TXT_TABLE_OVERFLOW_RIGHT, NULL);

  if (multiplayer)
    {
      txt_add_widgets(window, txt_new_separator("Advanced"),
        txt_new_label("UDP port"),
        txt_new_int_input_box(&udpport, 5),
        txt_new_inverted_checkbox("Register with master server",
                                &privateserver),
        TXT_TABLE_OVERFLOW_RIGHT, NULL
      );
    }

  txt_add_widgets(window,
                  txt_new_button2("Add extra parameters...",
                                  open_extra_params_window, NULL),
                  TXT_TABLE_OVERFLOW_RIGHT, NULL);

  txt_signal_connect(iwad_selector, "changed", update_warp_type, NULL);

  update_warp_type(NULL, NULL);
  update_warp_button();
}

static void do_join_game(void *unused1, void *unused2)
{
  execute_context_t *exec;

  if (connect_address == NULL || strlen(connect_address) <= 0)
    {
      txt_message_box(NULL, "Please enter a server address\n"
                            "to connect to.");
      return;
    }

  exec = new_execute_context();

  add_cmdline_parameter(exec, "-connect %s", connect_address);

  if (gamemission == hexen)
    {
      add_cmdline_parameter(exec, "-class %i", character_class);
    }

  /* Extra parameters come first, so that they can be used to override
   * the other parameters.
   */

  add_extra_parameters(exec);
  add_iwad_parameter(exec);
  add_wads(exec);

  txt_shutdown();

  m_save_defaults();

  pass_through_arguments(exec);

  execute_doom(exec);

  exit(0);
}

static txt_window_action_t *join_game_action(void)
{
  txt_window_action_t *action;

  action = txt_new_window_action(KEY_F10, "Connect");
  txt_signal_connect(action, "pressed", do_join_game, NULL);

  return action;
}

static void select_query_address(TXT_UNCAST_ARG(button),
                                 TXT_UNCAST_ARG(querydata))
{
  TXT_CAST_ARG(txt_button_t, button);
  TXT_CAST_ARG(net_querydata_t, querydata);
  int i;

  if (querydata->server_state != 0)
    {
      txt_message_box("Cannot connect to server",
                      "Gameplay is already in progress\n"
                      "on this server.");
      return;
    }

  /* Set address to connect to: */

  free(connect_address);
  connect_address = m_string_duplicate(button->label);

  /* Auto-choose IWAD if there is already a player connected. */

  if (querydata->num_players > 0)
    {
      for (i = 0; found_iwads[i] != NULL; ++i)
        {
          if (found_iwads[i]->mode == querydata->gamemode &&
              found_iwads[i]->mission == querydata->gamemission)
            {
              found_iwad_selected = i;
              iwadfile = found_iwads[i]->name;
              break;
            }
        }

      if (found_iwads[i] == NULL)
        {
          txt_message_box(NULL,
                          "The game on this server seems to be:\n"
                          "\n"
                          "   %s\n"
                          "\n"
                          "but the IWAD file %s is not found!\n"
                          "Without the required IWAD file, it may not be\n"
                          "possible to join this game.",
                          d_suggest_game_name(querydata->gamemission,
                                              querydata->gamemode),
                          d_suggest_iwad_name(querydata->gamemission,
                                              querydata->gamemode));
        }
    }

  /* Finished with search. */

  txt_close_window(query_window);
}

static void query_response_callback(net_addr_t *addr,
                                    net_querydata_t *querydata,
                                    unsigned int ping_time,
                                    TXT_UNCAST_ARG(results_table))
{
  TXT_CAST_ARG(txt_table_t, results_table);
  char ping_time_str[16];
  char description[47];

  /* When we connect we'll have to negotiate a common protocol that we
   * can agree upon between the client and server. If we can't then we
   * won't be able to connect, so it's pointless to include it in the
   * results list. If protocol==NET_PROTOCOL_UNKNOWN then this may be
   * an old, pre-3.0 Chocolate Doom server that doesn't support the new
   * protocol negotiation mechanism, or it may be an incompatible fork.
   */

  if (querydata->protocol == NET_PROTOCOL_UNKNOWN)
    {
      return;
    }

  snprintf(ping_time_str, sizeof(ping_time_str), "%ims", ping_time);

  /* Build description from server name field. Because there is limited
   * space, we only include the player count if there are already players
   * connected to the server.
   */

  if (querydata->num_players > 0)
    {
      snprintf(description, sizeof(description), "(%d/%d) ",
               querydata->num_players, querydata->max_players);
    }
  else
    {
      m_str_copy(description, "", sizeof(description));
    }

  m_string_concat(description, querydata->description, sizeof(description));

  txt_add_widgets(results_table, txt_new_label(ping_time_str),
                  txt_new_button2(net_addr_to_string(addr),
                                  select_query_address, querydata),
                  txt_new_label(description), NULL);

  ++query_servers_found;
}

static void query_periodic_callback(TXT_UNCAST_ARG(results_table))
{
  TXT_CAST_ARG(txt_table_t, results_table);

  if (!net_query_poll(query_response_callback, results_table))
    {
      txt_set_periodic_callback(NULL, NULL, 0);

      if (query_servers_found == 0)
        {
          txt_add_widgets(results_table, TXT_TABLE_EMPTY,
                          txt_new_label("No compatible servers found."),
                          NULL);
        }
    }
}

static void query_window_closed(TXT_UNCAST_ARG(window), void *unused)
{
  txt_set_periodic_callback(NULL, NULL, 0);
}

static void server_query_window(const char *title)
{
  txt_table_t *results_table;

  query_servers_found = 0;

  query_window = txt_new_window(title);

  txt_add_widget(query_window,
                txt_new_scrollpane(70, 10,
                    results_table = txt_new_table(3)));

  txt_set_column_widths(results_table, 7, 22, 40);
  txt_set_periodic_callback(query_periodic_callback, results_table, 1);

  txt_signal_connect(query_window, "closed", query_window_closed, NULL);
}

static void find_internet_server(TXT_UNCAST_ARG(widget),
                                 TXT_UNCAST_ARG(user_data))
{
  net_start_master_query();
  server_query_window("Find Internet server");
}

static void find_lan_server(TXT_UNCAST_ARG(widget),
        TXT_UNCAST_ARG(user_data))
{
  net_start_lan_query();
  server_query_window("Find LAN server");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void start_multi_game(TXT_UNCAST_ARG(widget), void *user_data)
{
  start_game_menu("Start multiplayer game", 1);
}

void warp_menu(TXT_UNCAST_ARG(widget), void *user_data)
{
  start_game_menu("Level Warp", 0);
}

void join_multi_game(TXT_UNCAST_ARG(widget), void *user_data)
{
  txt_window_t *window;
  txt_inputbox_t *address_box;

  window = txt_new_window("Join multiplayer game");
  txt_set_table_columns(window, 2);
  txt_set_column_widths(window, 12, 12);

  txt_set_window_help_url(window, MULTI_JOIN_HELP_URL);

  txt_add_widgets(window, txt_new_label("Game"), iwad_selector(), NULL);

  if (gamemission == hexen)
    {
      txt_add_widgets(
          window, txt_new_label("Character class "),
          txt_new_dropdown_list(&character_class, character_classes, 3),
          NULL);
    }

  txt_add_widgets(window, txt_new_separator("Server"),
                  txt_new_label("Connect to address: "),
                  address_box = txt_new_input_box(&connect_address, 30),

                  txt_new_button2("Find server on Internet...",
                                  find_internet_server, NULL),
                  TXT_TABLE_OVERFLOW_RIGHT,
                  txt_new_button2("Find server on local network...",
                                  find_lan_server, NULL),
                  TXT_TABLE_OVERFLOW_RIGHT, txt_new_strut(0, 1),
                  TXT_TABLE_OVERFLOW_RIGHT,
                  txt_new_button2("Add extra parameters...",
                                  open_extra_params_window, NULL),
                  NULL);

  txt_select_widget(window, address_box);

  txt_set_window_action(window, TXT_HORIZ_CENTER, wad_window_action());
  txt_set_window_action(window, TXT_HORIZ_RIGHT, join_game_action());
}

void set_chat_macro_defaults(void)
{
  int i;

  /* If the chat macros have not been set, initialize with defaults. */

  for (i = 0; i < 10; ++i)
    {
      if (chat_macros[i] == NULL)
        {
          chat_macros[i] = m_string_duplicate(g_defaults[i]);
        }
    }
}

void set_player_name_default(void)
{
  if (net_player_name == NULL)
    {
      net_player_name = "Player";
    }
}

void multiplayer_config(TXT_UNCAST_ARG(widget), void *user_data)
{
  txt_window_t *window;
  txt_label_t *label;
  txt_table_t *table;
  char buf[10];
  int i;

  window = txt_new_window("Multiplayer Configuration");
  txt_set_window_help_url(window, MULTI_CONFIG_HELP_URL);

  txt_add_widgets(
      window, txt_new_strut(0, 1),
      txt_new_horiz_box(txt_new_label("Player name:  "),
                        txt_new_input_box(&net_player_name, 25), NULL),
      txt_new_strut(0, 1), txt_new_separator("Chat macros"), NULL);

  table = txt_new_table(2);

  for (i = 0; i < 10; ++i)
    {
      snprintf(buf, sizeof(buf), "#%i ", i + 1);

      label = txt_new_label(buf);
      txt_set_fg_colour(label, TXT_COLOR_BRIGHT_CYAN);

      txt_add_widgets(table, label,
                      txt_new_input_box(&chat_macros[(i + 1) % 10], 40),
                      NULL);
    }

  txt_add_widget(window, table);
}

void bind_multiple_variables(void)
{
  char buf[15];
  int i;

  m_bind_string_variable("player_name", &net_player_name);

  for (i = 0; i < 10; ++i)
    {
      snprintf(buf, sizeof(buf), "chatmacro%i", i);
      m_bind_string_variable(buf, &chat_macros[i]);
    }

  switch (gamemission)
    {
    case doom:
      m_bind_chat_controls(4);
      key_multi_msgplayer[0] = 'g';
      key_multi_msgplayer[1] = 'i';
      key_multi_msgplayer[2] = 'b';
      key_multi_msgplayer[3] = 'r';
      break;

    case heretic:
      m_bind_chat_controls(4);
      key_multi_msgplayer[0] = 'g';
      key_multi_msgplayer[1] = 'y';
      key_multi_msgplayer[2] = 'r';
      key_multi_msgplayer[3] = 'b';
      break;

    case hexen:
      m_bind_chat_controls(8);
      key_multi_msgplayer[0] = 'b';
      key_multi_msgplayer[1] = 'r';
      key_multi_msgplayer[2] = 'y';
      key_multi_msgplayer[3] = 'g';
      key_multi_msgplayer[4] = 'j';
      key_multi_msgplayer[5] = 'w';
      key_multi_msgplayer[6] = 'h';
      key_multi_msgplayer[7] = 'p';
      break;

    default:
      break;
    }
}
