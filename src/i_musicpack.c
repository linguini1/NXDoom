/****************************************************************************
 * apps/games/NXDoom/src/i_musicpack.c
 *
 * SPDX-License-Identifier: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
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
 * DESCRIPTION:
 *  System interface for music.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i_glob.h"

#include "config.h"
#include "doomtype.h"
#include "memio.h"
#include "mus2mid.h"

#include "deh_str.h"
#include "gusconf.h"
#include "i_sound.h"
#include "i_swap.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "sha1.h"
#include "w_wad.h"
#include "z_zone.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

char *music_pack_path = "";

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static boolean I_NULL_InitMusic(void)
{
  return false;
}

static void I_NULL_ShutdownMusic(void)
{
  return;
}

static void I_NULL_SetMusicVolume(int volume)
{
  return;
}

static void I_NULL_PauseSong(void)
{
  return;
}

static void I_NULL_ResumeSong(void)
{
  return;
}

static void *I_NULL_RegisterSong(void *data, int len)
{
  return NULL;
}

static void I_NULL_UnRegisterSong(void *handle)
{
  return;
}

static void I_NULL_PlaySong(void *handle, boolean looping)
{
  return;
}

static void I_NULL_StopSong(void)
{
  return;
}

static boolean I_NULL_MusicIsPlaying(void)
{
  return false;
}

static void I_NULL_PollMusic(void)
{
  return;
}

const music_module_t music_pack_module =
{
  NULL,
  0,
  I_NULL_InitMusic,
  I_NULL_ShutdownMusic,
  I_NULL_SetMusicVolume,
  I_NULL_PauseSong,
  I_NULL_ResumeSong,
  I_NULL_RegisterSong,
  I_NULL_UnRegisterSong,
  I_NULL_PlaySong,
  I_NULL_StopSong,
  I_NULL_MusicIsPlaying,
  I_NULL_PollMusic,
};
