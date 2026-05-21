include $(APPDIR)/Make.defs

# Program options

MODULE    = $(CONFIG_GAMES_NXDOOM)
PRIORITY  = $(CONFIG_GAMES_NXDOOM_PRIORITY)
STACKSIZE = $(CONFIG_GAMES_NXDOOM_STACKSIZE)
PROGNAME  = nxdoom

# Source files

COMMONSRCDIR = src
TEXTSCREENSRCDIR = textscreen
DOOMSRCDIR = $(COMMONSRCDIR)/doom
OPLSRCDIR = opl
PCSOUNDSRCDIR = pcsound

# Includes

CFLAGS += -I$(COMMONSRCDIR) -I$(DOOMSRCDIR) -I$(TEXTSCREENSRCDIR)
CFLAGS += -I$(OPLSRCDIR) -I$(PCSOUNDSRCDIR)

# Source files

COMMONSRCS = i_system.c m_argv.c m_misc.c # Excluding i_main.c
COMMONSRCS += z_native.c # Native memory allocation

DEHACKEDSRCS = deh_io.c deh_main.c deh_mapping.c deh_text.c

# Used by chocolate-doom
BASESRCS =        \
  aes_prng.c      \
  d_event.c       \
  d_iwad.c        \
  d_loop.c        \
  d_mode.c        \
  deh_str.c       \
  gusconf.c       \
  i_cdmus.c       \
  i_endoom.c      \
  i_flmusic.c     \
  i_glob.c        \
  i_input.c       \
  i_joystick.c    \
  i_musicpack.c   \
  i_oplmusic.c    \
  i_pcsound.c     \
  i_sdlmusic.c    \
  i_sdlsound.c    \
  i_sound.c       \
  i_timer.c       \
  i_video.c       \
  i_videohr.c     \
  i_winmusic.c    \
  midifallback.c  \
  midifile.c      \
  mus2mid.c       \
  m_bbox.c        \
  m_cheat.c       \
  m_config.c      \
  m_controls.c    \
  m_fixed.c       \
  net_client.c    \
  net_common.c    \
  net_dedicated.c \
  net_gui.c       \
  net_io.c        \
  net_loop.c      \
  net_packet.c    \
  net_petname.c   \
  net_query.c     \
  net_sdl.c       \
  net_server.c    \
  net_structrw.c  \
  p_rejectpad.c   \
  sha1.c          \
  memio.c         \
  tables.c        \
  v_diskicon.c    \
  v_video.c       \
  w_checksum.c    \
  w_main.c        \
  w_wad.c         \
  w_file.c        \
  w_file_stdc.c   \
  w_file_posix.c  \
  w_file_win32.c  \
  w_merge.c       \

DOOMSRCS =     \
  am_map.c     \
  deh_ammo.c   \
  deh_bexstr.c \
  deh_cheat.c  \
  deh_doom.c   \
  deh_frame.c  \
  deh_misc.c   \
  deh_ptr.c    \
  deh_sound.c  \
  deh_thing.c  \
  deh_weapon.c \
  d_items.c    \
  d_main.c     \
  d_net.c      \
  doomdef.c    \
  doomstat.c   \
  dstrings.c   \
  f_finale.c   \
  f_wipe.c     \
  g_game.c     \
  hu_lib.c     \
  hu_stuff.c   \
  info.c       \
  m_menu.c     \
  m_random.c   \
  p_ceilng.c   \
  p_doors.c    \
  p_enemy.c    \
  p_floor.c    \
  p_inter.c    \
  p_lights.c   \
  p_map.c      \
  p_maputl.c   \
  p_mobj.c     \
  p_plats.c    \
  p_pspr.c     \
  p_saveg.c    \
  p_setup.c    \
  p_sight.c    \
  p_spec.c     \
  p_switch.c   \
  p_telept.c   \
  p_tick.c     \
  p_user.c     \
  r_bsp.c      \
  r_data.c     \
  r_draw.c     \
  r_main.c     \
  r_plane.c    \
  r_segs.c     \
  r_sky.c      \
  r_things.c   \
  s_sound.c    \
  sounds.c     \
  statdump.c   \
  st_lib.c     \
  st_stuff.c   \
  wi_stuff.c   

TXTSCREENSRCS =       \
  txt_conditional.c   \
  txt_checkbox.c      \
  txt_desktop.c       \
  txt_dropdown.c      \
  txt_fileselect.c    \
  txt_gui.c           \
  txt_inputbox.c      \
  txt_io.c            \
  txt_button.c        \
  txt_label.c         \
  txt_radiobutton.c   \
  txt_scrollpane.c    \
  txt_separator.c     \
  txt_spinctrl.c      \
  txt_sdl.c           \
  txt_strut.c         \
  txt_table.c         \
  txt_utf8.c          \
  txt_widget.c        \
  txt_window.c        \
  txt_window_action.c 

OPLSRCS =      \
  opl.c        \
  opl_queue.c  \
  opl_sdl.c    \
  opl_timer.c  \
  ioperm_sys.c \
  opl3.c

PCSOUNDSRCS =     \
  pcsound.c       \
  pcsound_sdl.c   \

CSRCS += $(patsubst %,$(OPLSRCDIR)/%,$(OPLSRCS))
CSRCS += $(patsubst %,$(PCSOUNDSRCDIR)/%,$(PCSOUNDSRCS))
CSRCS += $(patsubst %,$(TEXTSCREENSRCDIR)/%,$(TXTSCREENSRCS))
CSRCS += $(patsubst %,$(COMMONSRCDIR)/%,$(COMMONSRCS))
CSRCS += $(patsubst %,$(COMMONSRCDIR)/%,$(DEHACKEDSRCS))
CSRCS += $(patsubst %,$(COMMONSRCDIR)/%,$(BASESRCS))
CSRCS += $(patsubst %,$(DOOMSRCDIR)/%,$(DOOMSRCS))

MAINSRC  = $(COMMONSRCDIR)/i_main.c

include $(APPDIR)/Application.mk
