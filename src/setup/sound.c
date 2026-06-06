/****************************************************************************
 * apps/games/NXDoom/src/setup/sound.c
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

/* Sound control menu */

#include <stdlib.h>
#include <string.h>

#include "m_config.h"
#include "m_misc.h"
#include "textscreen.h"

#include "execute.h"
#include "mode.h"
#include "sound.h"

#ifndef DISABLE_SDL2MIXER
#include "SDL_mixer.h"
#endif /* DISABLE_SDL2MIXER */

#define WINDOW_HELP_URL "https://www.chocolate-doom.org/setup-sound"

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef enum
{
  OPLMODE_OPL2,
  OPLMODE_OPL3,
  NUM_OPLMODES,
} oplmode_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *opltype_strings[] =
{
  "OPL2",
  "OPL3",
};

static const char *cfg_extension[] =
{
  "cfg",
  NULL,
};

#ifdef HAVE_FLUIDSYNTH
static const char *sf_extension[] =
{
  "sf2",
  "sf3",
  NULL,
};
#endif

static int g_num_channels = 8;
static int g_sfx_volume = 8;
static int g_music_volume = 8;
static int g_voice_volume = 15;
static int show_talk = 0;

static char *g_gus_patch_path = NULL;
static int g_gus_ram_kb = 1024;

/* DOS specific variables: these are unused but should be maintained
 * so that the config file can be shared between chocolate
 * doom and doom.exe
 */

static int g_snd_sbport = 0;
static int g_snd_sbirq = 0;
static int g_snd_sbdma = 0;
static int g_snd_mport = 0;

static int g_snd_oplmode;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Config file variables: */

int snd_sfxdevice = SNDDEVICE_SB;
int snd_musicdevice = SNDDEVICE_SB;
int snd_samplerate = 44100;
int snd_cachesize = 64 * 1024 * 1024;
int snd_maxslicetime_ms = 28;
char *snd_musiccmd = "";
int snd_pitchshift = 0;

int use_libsamplerate = 0;
float libsamplerate_scale = 0.65;

char *music_pack_path = NULL;
char *timidity_cfg_path = NULL;

#ifdef HAVE_FLUIDSYNTH
char *fsynth_sf_path = NULL;
int fsynth_chorus_active = 1;
float fsynth_chorus_depth = 5.0f;
float fsynth_chorus_level = 0.35f;
int fsynth_chorus_nr = 3;
float fsynth_chorus_speed = 0.3f;
char *fsynth_midibankselect = "gs";
int fsynth_polyphony = 256;
int fsynth_reverb_active = 1;
float fsynth_reverb_damp = 0.4f;
float fsynth_reverb_level = 0.15f;
float fsynth_reverb_roomsize = 0.6f;
float fsynth_reverb_width = 4.0f;
float fsynth_gain = 1.0f;
#endif /* HAVE_FLUIDSYNTH */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void update_snd_devices(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(data))
{
#if 0
    switch (snd_oplmode)
    {
        default:
        case OPLMODE_OPL2:
            snd_dmxoption = "";
            break;

        case OPLMODE_OPL3:
            snd_dmxoption = "-opl3";
            break;
    }
#endif
}

static void open_musc_pack_dir(TXT_UNCAST_ARG(widget),
        TXT_UNCAST_ARG(unused))
{
  if (!open_folder(music_pack_path))
    {
      txt_message_box("Error", "Failed to open music pack directory.");
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void config_sound(TXT_UNCAST_ARG(widget), void *user_data)
{
  txt_window_t *window;
  txt_window_action_t *music_action;

  /* Build the window */

  window = txt_new_window("Sound configuration");
  txt_set_window_help_url(window, WINDOW_HELP_URL);

  txt_set_column_widths(window, 40);
  txt_set_window_position(window, TXT_HORIZ_CENTER, TXT_VERT_TOP,
                          TXT_SCREEN_W / 2, 3);

  music_action = txt_new_window_action('m', "Music Packs");
  txt_set_window_action(window, TXT_HORIZ_CENTER, music_action);
  txt_signal_connect(music_action, "pressed", open_music_packdir, NULL);

  txt_add_widgets(
      window, txt_new_separator("Sound effects"),
      txt_new_radio_button("Disabled", &snd_sfxdevice, SNDDEVICE_NONE),
      txt_if(gamemission == doom || gamemission == strife,
             txt_new_radio_button("PC speaker effects", &snd_sfxdevice,
                                  SNDDEVICE_PCSPEAKER)),
      txt_new_radio_button("Digital sound effects", &snd_sfxdevice,
                           SNDDEVICE_SB),
      txt_if(gamemission == doom || gamemission == heretic ||
                 gamemission == hexen,
             txt_new_conidtional(
                 &snd_sfxdevice, SNDDEVICE_SB,
                 txt_new_horiz_box(
                     txt_new_strut(4, 0),
                     txt_new_check_box("Pitch-shifted sounds",
                                     &snd_pitchshift),
                     NULL))),
      txt_if(gamemission == strife,
             txt_new_conidtional(
                 &snd_sfxdevice, SNDDEVICE_SB,
                 txt_new_horiz_box(
                     txt_new_strut(4, 0),
                     txt_new_check_box("Show text with voices", &show_talk),
                     NULL))),

      txt_new_separator("Music"),
      txt_new_radio_button("Disabled", &snd_musicdevice, SNDDEVICE_NONE),

      txt_new_radio_button("OPL (Adlib/Soundblaster)",
              &snd_musicdevice, SNDDEVICE_SB),

      txt_new_radio_button("GUS (emulated)", &snd_musicdevice,
              SNDDEVICE_GUS),
      txt_new_conidtional(
          &snd_musicdevice, SNDDEVICE_GUS,
          txt_make_table(
              2, txt_new_strut(4, 0), txt_new_label("Path to patch files: "),
              txt_new_strut(4, 0),
              txt_new_file_selector(
                  &g_gus_patch_path, 34,
                  "Select directory containing GUS patches",
                  TXT_DIRECTORY),
              NULL)),

      txt_new_radio_button("Native MIDI", &snd_musicdevice,
                           SNDDEVICE_GENMIDI),
      txt_new_conidtional(
          &snd_musicdevice, SNDDEVICE_GENMIDI,
          txt_make_table(2, txt_new_strut(4, 0),
                         txt_new_label("Timidity configuration file: "),
                         txt_new_strut(4, 0),
                         txt_new_file_selector(&timidity_cfg_path, 34,
                                               "Select Timidity config file",
                                               cfg_extension),
                         NULL)),
#ifdef HAVE_FLUIDSYNTH
      txt_new_radio_button("FluidSynth", &snd_musicdevice, SNDDEVICE_FSYNTH),
      txt_new_conidtional(
          &snd_musicdevice, SNDDEVICE_FSYNTH,
          txt_make_table(
              2, txt_new_strut(4, 0), txt_new_label("Soundfont file: "),
              txt_new_strut(4, 0),
              txt_new_file_selector(&fsynth_sf_path, 34,
                                    "Select FluidSynth soundfont file",
                                    sf_extension),
              NULL)),
#endif
      NULL);
}

void bind_sound_variables(void)
{
  m_bind_int_variable("snd_sfxdevice", &snd_sfxdevice);
  m_bind_int_variable("snd_musicdevice", &snd_musicdevice);
  m_bind_int_variable("snd_channels", &g_num_channels);
  m_bind_int_variable("snd_samplerate", &snd_samplerate);
  m_bind_int_variable("sfx_volume", &g_sfx_volume);
  m_bind_int_variable("music_volume", &g_music_volume);

  m_bind_int_variable("use_libsamplerate", &use_libsamplerate);
  m_bind_float_variable("libsamplerate_scale", &libsamplerate_scale);

  m_bind_int_variable("gus_ram_kb", &g_gus_ram_kb);
  m_bind_string_variable("gus_patch_path", &g_gus_patch_path);
  m_bind_string_variable("music_pack_path", &music_pack_path);
  m_bind_string_variable("timidity_cfg_path", &timidity_cfg_path);
#ifdef HAVE_FLUIDSYNTH
  m_bind_int_variable("fsynth_chorus_active", &fsynth_chorus_active);
  m_bind_float_variable("fsynth_chorus_depth", &fsynth_chorus_depth);
  m_bind_float_variable("fsynth_chorus_level", &fsynth_chorus_level);
  m_bind_int_variable("fsynth_chorus_nr", &fsynth_chorus_nr);
  m_bind_float_variable("fsynth_chorus_speed", &fsynth_chorus_speed);
  m_bind_string_variable("fsynth_midibankselect", &fsynth_midibankselect);
  m_bind_int_variable("fsynth_polyphony", &fsynth_polyphony);
  m_bind_int_variable("fsynth_reverb_active", &fsynth_reverb_active);
  m_bind_float_variable("fsynth_reverb_damp", &fsynth_reverb_damp);
  m_bind_float_variable("fsynth_reverb_level", &fsynth_reverb_level);
  m_bind_float_variable("fsynth_reverb_roomsize", &fsynth_reverb_roomsize);
  m_bind_float_variable("fsynth_reverb_width", &fsynth_reverb_width);
  m_bind_float_variable("fsynth_gain", &fsynth_gain);
  m_bind_string_variable("fsynth_sf_path", &fsynth_sf_path);
#endif /* HAVE_FLUIDSYNTH */

  m_bind_int_variable("snd_sbport", &g_snd_sbport);
  m_bind_int_variable("snd_sbirq", &g_snd_sbirq);
  m_bind_int_variable("snd_sbdma", &g_snd_sbdma);
  m_bind_int_variable("snd_mport", &g_snd_mport);
  m_bind_int_variable("snd_maxslicetime_ms", &snd_maxslicetime_ms);
  m_bind_string_variable("snd_musiccmd", &snd_musiccmd);

  m_bind_int_variable("snd_cachesize", &snd_cachesize);

  m_bind_int_variable("snd_pitchshift", &snd_pitchshift);

  if (gamemission == strife)
    {
      m_bind_int_variable("voice_volume", &g_voice_volume);
      m_bind_int_variable("show_talk", &show_talk);
    }

  music_pack_path = m_string_duplicate("");
  timidity_cfg_path = m_string_duplicate("");
  g_gus_patch_path = m_string_duplicate("");

#ifdef HAVE_FLUIDSYNTH
  fsynth_sf_path = m_string_duplicate("");
#endif

  /* All versions of Heretic and Hexen did pitch-shifting.
   * Most versions of Doom did not and Strife never did.
   */

  snd_pitchshift = gamemission == heretic || gamemission == hexen;

  /* Default sound volumes - different games use different values. */

  switch (gamemission)
    {
    case doom:
    default:
      g_sfx_volume = 8;
      g_music_volume = 8;
      break;
    case heretic:
    case hexen:
      g_sfx_volume = 10;
      g_music_volume = 10;
      break;
    case strife:
      g_sfx_volume = 8;
      g_music_volume = 13;
      break;
    }
}
