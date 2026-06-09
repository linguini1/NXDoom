/****************************************************************************
 * apps/games/NXDoom/src/setup/keyboard.c
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

#include "doomtype.h"
#include "m_config.h"
#include "m_controls.h"
#include "m_misc.h"
#include "textscreen.h"

#include "execute.h"
#include "txt_keyinput.h"

#include "joystick.h"
#include "keyboard.h"
#include "mode.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WINDOW_HELP_URL "https://www.chocolate-doom.org/setup-keyboard"

/****************************************************************************
 * Public Data
 ****************************************************************************/

int vanilla_keyboard_mapping = 1;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int always_run = 0;

/* Keys within these groups cannot have the same value. */

static int *controls[] =
{
    &key_left,
    &key_right,
    &key_up,
    &key_down,
    &key_strafeleft,
    &key_straferight,
    &key_fire,
    &key_use,
    &key_strafe,
    &key_speed,
    &key_jump,
    &key_flyup,
    &key_flydown,
    &key_flycenter,
    &key_lookup,
    &key_lookdown,
    &key_lookcenter,
    &key_invleft,
    &key_invright,
    &key_invquery,
    &key_invuse,
    &key_invpop,
    &key_mission,
    &key_invkey,
    &key_invhome,
    &key_invend,
    &key_invdrop,
    &key_useartifact,
    &key_pause,
    &key_usehealth,
    &key_weapon1,
    &key_weapon2,
    &key_weapon3,
    &key_weapon4,
    &key_weapon5,
    &key_weapon6,
    &key_weapon7,
    &key_weapon8,
    &key_arti_quartz,
    &key_arti_urn,
    &key_arti_bomb,
    &key_arti_tome,
    &key_arti_ring,
    &key_arti_chaosdevice,
    &key_arti_shadowsphere,
    &key_arti_wings,
    &key_arti_torch,
    &key_arti_morph,
    &key_arti_all,
    &key_arti_health,
    &key_arti_poisonbag,
    &key_arti_blastradius,
    &key_arti_teleport,
    &key_arti_teleportother,
    &key_arti_egg,
    &key_arti_invulnerability,
    &key_prevweapon,
    &key_nextweapon,
    NULL,
};

static int *menu_nav[] =
{
    &key_menu_activate, &key_menu_up,   &key_menu_down,    &key_menu_left,
    &key_menu_right,    &key_menu_back, &key_menu_forward, NULL,
};

static int *shortcuts[] =
{
    &key_menu_help,
    &key_menu_save,
    &key_menu_load,
    &key_menu_volume,
    &key_menu_detail,
    &key_menu_qsave,
    &key_menu_endgame,
    &key_menu_messages,
    &key_spy,
    &key_menu_qload,
    &key_menu_quit,
    &key_menu_gamma,
    &key_menu_incscreen,
    &key_menu_decscreen,
    &key_menu_screenshot,
    &key_message_refresh,
    &key_multi_msg,
    &key_multi_msgplayer[0],
    &key_multi_msgplayer[1],
    &key_multi_msgplayer[2],
    &key_multi_msgplayer[3],
    NULL,
};

static int *map_keys[] =
{
    &key_map_north,
    &key_map_south,
    &key_map_east,
    &key_map_west,
    &key_map_zoomin,
    &key_map_zoomout,
    &key_map_toggle,
    &key_map_maxzoom,
    &key_map_follow,
    &key_map_grid,
    &key_map_mark,
    &key_map_clearmark,
    NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void update_joyb_speed(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(var))
{
  if (always_run)
    {
      /* <Janizdreg> if you want to pick one for chocolate doom to use,
       *             pick 29, since that is the most universal one that
       *             also works with heretic, hexen and strife =P
       * NB. This choice also works with original, ultimate and final exes.
       */

      joybspeed = 29;
    }
  else
    {
      joybspeed = 2;
    }
}

static int var_in_group(int *variable, int **group)
{
  unsigned int i;

  for (i = 0; group[i] != NULL; ++i)
    {
      if (group[i] == variable)
        {
          return 1;
        }
    }

  return 0;
}

static void check_key_group(int *variable, int **group)
{
  unsigned int i;

  /* Don't check unless the variable is in this group. */

  if (!var_in_group(variable, group))
    {
      return;
    }

  /* If another variable has the same value as the new value, reset it. */

  for (i = 0; group[i] != NULL; ++i)
    {
      if (*variable == *group[i] && group[i] != variable)
        {
          /* A different key has the same value.  Clear the existing
           * value. This ensures that no two keys can have the same
           * value.
           */

          *group[i] = 0;
        }
    }
}

/* Callback invoked when a key control is set */

static void key_set_callback(TXT_UNCAST_ARG(widget),
        TXT_UNCAST_ARG(variable))
{
  TXT_CAST_ARG(int, variable);

  check_key_group(variable, controls);
  check_key_group(variable, menu_nav);
  check_key_group(variable, shortcuts);
  check_key_group(variable, map_keys);
}

/* Add a label and keyboard input to the specified table. */

static void add_key_control(TXT_UNCAST_ARG(table),
        const char *name, int *var)
{
  TXT_CAST_ARG(txt_table_t, table);
  txt_key_input_t *key_input;

  txt_add_widget(table, txt_new_label(name));
  key_input = txt_new_key_input(var);
  txt_add_widget(table, key_input);

  txt_signal_connect(key_input, "set", key_set_callback, var);
}

static void add_section_label(TXT_UNCAST_ARG(table), const char *title,
                            boolean add_space)
{
  TXT_CAST_ARG(txt_table_t, table);
  char buf[64];

  if (add_space)
    {
      txt_add_widgets(table, txt_new_strut(0, 1), TXT_TABLE_EOL, NULL);
    }

  snprintf(buf, sizeof(buf), " - %s - ", title);

  txt_add_widgets(table, txt_new_label(buf), TXT_TABLE_EOL, NULL);
}

static void config_extra_keys(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(unused))
{
  txt_window_t *window;
  txt_scrollpane_t *scrollpane;
  txt_table_t *table;
  boolean extra_keys;

  extra_keys = gamemission == heretic || gamemission == hexen ||
      gamemission == strife;

  window = txt_new_window("Extra keyboard controls");

  txt_set_window_help_url(window, WINDOW_HELP_URL);

  table = txt_new_table(2);

  txt_set_column_widths(table, 21, 9);

  if (extra_keys)
    {
      /* When we have extra controls, a scrollable pane must be used. */

      scrollpane = txt_new_scrollpane(0, 13, table);
      txt_add_widget(window, scrollpane);

      add_section_label(table, "View", false);

      add_key_control(table, "Look up", &key_lookup);
      add_key_control(table, "Look down", &key_lookdown);
      add_key_control(table, "Center view", &key_lookcenter);

      if (gamemission == heretic || gamemission == hexen)
        {
          add_section_label(table, "Flying", true);

          add_key_control(table, "Fly up", &key_flyup);
          add_key_control(table, "Fly down", &key_flydown);
          add_key_control(table, "Fly center", &key_flycenter);
        }

      add_section_label(table, "Inventory", true);

      add_key_control(table, "Inventory left", &key_invleft);
      add_key_control(table, "Inventory right", &key_invright);

      if (gamemission == strife)
        {
          add_key_control(table, "Home", &key_invhome);
          add_key_control(table, "End", &key_invend);
          add_key_control(table, "Query", &key_invquery);
          add_key_control(table, "Drop", &key_invdrop);
          add_key_control(table, "Show weapons", &key_invpop);
          add_key_control(table, "Show mission", &key_mission);
          add_key_control(table, "Show keys", &key_invkey);
          add_key_control(table, "Use", &key_invuse);
          add_key_control(table, "Use health", &key_usehealth);
        }
      else
        {
          add_key_control(table, "Use artifact", &key_useartifact);
        }

      if (gamemission == heretic)
        {
          add_section_label(table, "Artifacts", true);

          add_key_control(table, "Quartz Flask", &key_arti_quartz);
          add_key_control(table, "Mystic Urn", &key_arti_urn);
          add_key_control(table, "Timebomb", &key_arti_bomb);
          add_key_control(table, "Tome of Power", &key_arti_tome);
          add_key_control(table, "Ring of Invincibility ", &key_arti_ring);
          add_key_control(table, "Chaos Device", &key_arti_chaosdevice);
          add_key_control(table, "Shadowsphere", &key_arti_shadowsphere);
          add_key_control(table, "Wings of Wrath", &key_arti_wings);
          add_key_control(table, "Torch", &key_arti_torch);
          add_key_control(table, "Morph Ovum", &key_arti_morph);
        }

      if (gamemission == hexen)
        {
          add_section_label(table, "Artifacts", true);

          add_key_control(table, "One of each", &key_arti_all);
          add_key_control(table, "Quartz Flask", &key_arti_health);
          add_key_control(table, "Flechette", &key_arti_poisonbag);
          add_key_control(table, "Disc of Repulsion", &key_arti_blastradius);
          add_key_control(table, "Chaos Device", &key_arti_teleport);
          add_key_control(table, "Banishment Device",
                          &key_arti_teleportother);
          add_key_control(table, "Porkalator", &key_arti_egg);
          add_key_control(table, "Icon of the Defender",
                          &key_arti_invulnerability);
        }
    }
  else
    {
      txt_add_widget(window, table);
    }

  add_section_label(table, "Weapons", extra_keys);

  add_key_control(table, "Weapon 1", &key_weapon1);
  add_key_control(table, "Weapon 2", &key_weapon2);
  add_key_control(table, "Weapon 3", &key_weapon3);
  add_key_control(table, "Weapon 4", &key_weapon4);
  add_key_control(table, "Weapon 5", &key_weapon5);
  add_key_control(table, "Weapon 6", &key_weapon6);
  add_key_control(table, "Weapon 7", &key_weapon7);
  add_key_control(table, "Weapon 8", &key_weapon8);
  add_key_control(table, "Previous weapon", &key_prevweapon);
  add_key_control(table, "Next weapon", &key_nextweapon);
}

static void other_keys_dialog(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(unused))
{
  txt_window_t *window;
  txt_table_t *table;
  txt_scrollpane_t *scrollpane;

  window = txt_new_window("Other keys");

  txt_set_window_help_url(window, WINDOW_HELP_URL);

  table = txt_new_table(2);

  txt_set_column_widths(table, 25, 9);

  add_section_label(table, "Menu navigation", false);

  add_key_control(table, "Activate menu", &key_menu_activate);
  add_key_control(table, "Move cursor up", &key_menu_up);
  add_key_control(table, "Move cursor down", &key_menu_down);
  add_key_control(table, "Move slider left", &key_menu_left);
  add_key_control(table, "Move slider right", &key_menu_right);
  add_key_control(table, "Go to previous menu", &key_menu_back);
  add_key_control(table, "Activate menu item", &key_menu_forward);
  add_key_control(table, "Confirm action", &key_menu_confirm);
  add_key_control(table, "Cancel action", &key_menu_abort);

  add_section_label(table, "Shortcut keys", true);

  add_key_control(table, "Pause game", &key_pause);
  add_key_control(table, "Help screen", &key_menu_help);
  add_key_control(table, "Save game", &key_menu_save);
  add_key_control(table, "Load game", &key_menu_load);
  add_key_control(table, "Sound volume", &key_menu_volume);
  add_key_control(table, "Toggle detail", &key_menu_detail);
  add_key_control(table, "Quick save", &key_menu_qsave);
  add_key_control(table, "End game", &key_menu_endgame);
  add_key_control(table, "Toggle messages", &key_menu_messages);
  add_key_control(table, "Quick load", &key_menu_qload);
  add_key_control(table, "Quit game", &key_menu_quit);
  add_key_control(table, "Toggle gamma", &key_menu_gamma);
  add_key_control(table, "Multiplayer spy", &key_spy);

  add_key_control(table, "Increase screen size", &key_menu_incscreen);
  add_key_control(table, "Decrease screen size", &key_menu_decscreen);
  add_key_control(table, "Save a screenshot", &key_menu_screenshot);

  add_key_control(table, "Display last message", &key_message_refresh);
  add_key_control(table, "Finish recording demo", &key_demo_quit);

  add_section_label(table, "Map", true);
  add_key_control(table, "Toggle map", &key_map_toggle);
  add_key_control(table, "Zoom in", &key_map_zoomin);
  add_key_control(table, "Zoom out", &key_map_zoomout);
  add_key_control(table, "Maximum zoom out", &key_map_maxzoom);
  add_key_control(table, "Follow mode", &key_map_follow);
  add_key_control(table, "Pan north", &key_map_north);
  add_key_control(table, "Pan south", &key_map_south);
  add_key_control(table, "Pan east", &key_map_east);
  add_key_control(table, "Pan west", &key_map_west);
  add_key_control(table, "Toggle grid", &key_map_grid);
  add_key_control(table, "Mark location", &key_map_mark);
  add_key_control(table, "Clear all marks", &key_map_clearmark);

  add_section_label(table, "Multiplayer", true);

  add_key_control(table, "Send message", &key_multi_msg);
  add_key_control(table, "- to player 1", &key_multi_msgplayer[0]);
  add_key_control(table, "- to player 2", &key_multi_msgplayer[1]);
  add_key_control(table, "- to player 3", &key_multi_msgplayer[2]);
  add_key_control(table, "- to player 4", &key_multi_msgplayer[3]);

  if (gamemission == hexen || gamemission == strife)
    {
      add_key_control(table, "- to player 5", &key_multi_msgplayer[4]);
      add_key_control(table, "- to player 6", &key_multi_msgplayer[5]);
      add_key_control(table, "- to player 7", &key_multi_msgplayer[6]);
      add_key_control(table, "- to player 8", &key_multi_msgplayer[7]);
    }

  scrollpane = txt_new_scrollpane(0, 13, table);

  txt_add_widget(window, scrollpane);
}

void config_keyboard(TXT_UNCAST_ARG(widget), void *user_data)
{
  txt_window_t *window;
  txt_checkbox_t *run_control;

  always_run = joybspeed >= 20;

  window = txt_new_window("Keyboard configuration");

  txt_set_window_help_url(window, WINDOW_HELP_URL);

  /* The window is on a 5-column grid layout that looks like:
   * Label | Control | | Label | Control
   * There is a small gap between the two conceptual "columns" of
   * controls, just for spacing.
   */

  txt_set_table_columns(window, 5);
  txt_set_column_widths(window, 15, 8, 2, 15, 8);

  txt_add_widget(window, txt_new_separator("Movement"));
  add_key_control(window, "Move Forward", &key_up);
  txt_add_widget(window, TXT_TABLE_EMPTY);
  add_key_control(window, "Strafe Left", &key_strafeleft);

  add_key_control(window, "Move Backward", &key_down);
  txt_add_widget(window, TXT_TABLE_EMPTY);
  add_key_control(window, "Strafe Right", &key_straferight);

  add_key_control(window, "Turn Left", &key_left);
  txt_add_widget(window, TXT_TABLE_EMPTY);
  add_key_control(window, "Run", &key_speed);

  add_key_control(window, "Turn Right", &key_right);
  txt_add_widget(window, TXT_TABLE_EMPTY);
  add_key_control(window, "Strafe On", &key_strafe);

  if (gamemission == hexen || gamemission == strife)
    {
      add_key_control(window, "Jump", &key_jump);
    }

  txt_add_widget(window, txt_new_separator("Action"));
  add_key_control(window, "Fire/Attack", &key_fire);
  txt_add_widget(window, TXT_TABLE_EMPTY);
  add_key_control(window, "Use", &key_use);

  txt_add_widgets(
      window, txt_new_button2("More controls...", config_extra_keys, NULL),
      TXT_TABLE_OVERFLOW_RIGHT, TXT_TABLE_EMPTY,
      txt_new_button2("Other keys...", other_keys_dialog, NULL),
      TXT_TABLE_OVERFLOW_RIGHT,

      txt_new_separator("Misc."),
      run_control = txt_new_check_box("Always run", &always_run),
      TXT_TABLE_EOL,
      txt_new_inverted_checkbox("Use native keyboard mapping",
                                &vanilla_keyboard_mapping),
      TXT_TABLE_EOL, NULL);

  txt_signal_connect(run_control, "changed", update_joyb_speed, NULL);
  txt_set_window_action(window, TXT_HORIZ_CENTER, test_config_action());
}

void bind_keyboard_variables(void)
{
  m_bind_int_variable("vanilla_keyboard_mapping", &vanilla_keyboard_mapping);
}
