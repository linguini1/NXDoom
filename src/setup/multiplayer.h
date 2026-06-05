/****************************************************************************
 * apps/games/NXDoom/src/setup/multiplayer.h
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

#ifndef SETUP_MULTIPLAYER_H
#define SETUP_MULTIPLAYER_H

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void start_multi_game(void *widget, void *user_data);
void warp_menu(void *widget, void *user_data);
void join_multi_game(void *widget, void *user_data);
void multiplayer_config(void *widget, void *user_data);

void set_chat_macro_defaults(void);
void set_player_name_default(void);

void bind_multiple_variables(void);

#endif /* SETUP_MULTIPLAYER_H */
