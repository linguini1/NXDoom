//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	DOOM selection menu, options, episode etc.
//	Sliders and icons. Kinda widget stuff.
//

#include <ctype.h>
#include <stdlib.h>

#include "doomdef.h"
#include "doomkeys.h"
#include "dstrings.h"

#include "d_main.h"
#include "deh_main.h"

#include "i_input.h"
#include "i_joystick.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_misc.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "r_local.h"

#include "hu_stuff.h"

#include "g_game.h"

#include "m_argv.h"
#include "m_controls.h"
#include "p_saveg.h"
#include "p_setup.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

#include "doomstat.h"

// Data.

#include "m_menu.h"

//
// defaulted values
//
int g_mouse_sensitivity = 5;

// Show messages has default, 0 = off, 1 = on
int g_show_messages = 1;

// Blocky mode, has default, 0 = high, 1 = normal
int g_detail_level = 0;
int screenblocks = 9;

// temp for screenblocks (0-9)
int screenSize;

// -1 = no quicksave slot picked!
int quickSaveSlot;

// 1 = message to be printed
int messageToPrint;
// ...and here is the message string!
const char *messageString;

// message x & y
int messx;
int messy;
int messageLastMenuActive;

// timed message = no input from user
boolean messageNeedsInput;

void (*messageRoutine)(int response);

char gammamsg[5][26] = {GAMMALVL0, GAMMALVL1, GAMMALVL2, GAMMALVL3,
                        GAMMALVL4};

// we are going to be entering a savegame string
int saveStringEnter;
int saveSlot;                      // which slot to save in
int saveCharIndex;                 // which char we're editing
static boolean joypadSave = false; // was the save action initiated by joypad?
// old save description before edit
char saveOldString[SAVESTRINGSIZE];

boolean inhelpscreens;
boolean menuactive;

#define SKULLXOFF -32
#define LINEHEIGHT 16

char savegamestrings[10][SAVESTRINGSIZE];

char endstring[160];

#if 0
static boolean opldev;
#endif

//
// MENU TYPEDEFS
//
typedef struct
{
  // 0 = no cursor here, 1 = ok, 2 = arrows ok
  short status;

  char name[10];

  // choice = menu item #.
  // if status = 2,
  //   choice=0:leftarrow,1:rightarrow
  void (*routine)(int choice);

  // hotkey in menu
  char alphaKey;
} menuitem_t;

typedef struct menu_s
{
  short numitems;          // # of menu items
  struct menu_s *prevMenu; // previous menu
  menuitem_t *menuitems;   // menu items
  void (*routine)();       // draw routine
  short x;
  short y;      // x,y of menu
  short lastOn; // last item user was on in menu
} menu_t;

short itemOn;           // menu item skull is on
short skullAnimCounter; // skull animation counter
short whichSkull;       // which skull to draw

// graphic name of skulls
// warning: initializer-string for array of chars is too long
const char *skullName[2] = {"M_SKULL1", "M_SKULL2"};

// current menudef
menu_t *currentMenu;

//
// PROTOTYPES
//
static void M_NewGame(int choice);
static void M_Episode(int choice);
static void M_ChooseSkill(int choice);
static void M_LoadGame(int choice);
static void M_SaveGame(int choice);
static void M_Options(int choice);
static void M_EndGame(int choice);
static void M_ReadThis(int choice);
static void M_ReadThis2(int choice);
static void M_QuitDOOM(int choice);

static void M_ChangeMessages(int choice);
static void M_ChangeSensitivity(int choice);
static void M_SfxVol(int choice);
static void M_MusicVol(int choice);
static void M_ChangeDetail(int choice);
static void M_SizeDisplay(int choice);
static void M_Sound(int choice);

static void M_FinishReadThis(int choice);
static void M_LoadSelect(int choice);
static void M_SaveSelect(int choice);
static void M_ReadSaveStrings(void);
static void M_QuickSave(void);
static void M_QuickLoad(void);

static void M_Drawmain_menu(void);
static void M_DrawReadThis1(void);
static void M_DrawReadThis2(void);
static void M_DrawNewGame(void);
static void M_DrawEpisode(void);
static void M_DrawOptions(void);
static void M_DrawSound(void);
static void M_DrawLoad(void);
static void M_DrawSave(void);

static void M_DrawSaveLoadBorder(int x, int y);
static void M_SetupNextMenu(menu_t *menudef);
static void M_DrawThermo(int x, int y, int thermWidth, int thermDot);
static void M_WriteText(int x, int y, const char *string);
static int M_StringWidth(const char *string);
static int M_StringHeight(const char *string);
static void M_StartMessage(const char *string, void *routine, boolean input);
static void M_ClearMenus(void);

//
// DOOM MENU
//
enum
{
  newgame = 0,
  options,
  loadgame,
  savegame,
  readthis,
  quitdoom,
  main_end
} main_e;

menuitem_t main_menu[] = {{1, "M_NGAME", M_NewGame, 'n'},
                          {1, "M_OPTION", M_Options, 'o'},
                          {1, "M_LOADG", M_LoadGame, 'l'},
                          {1, "M_SAVEG", M_SaveGame, 's'},
                          // Another hickup with Special edition.
                          {1, "M_RDTHIS", M_ReadThis, 'r'},
                          {1, "M_QUITG", M_QuitDOOM, 'q'}};

menu_t MainDef = {main_end, NULL, main_menu, M_Drawmain_menu, 97, 64, 0};

//
// EPISODE SELECT
//
enum
{
  ep1,
  ep2,
  ep3,
  ep4,
  ep_end
} episodes_e;

menuitem_t EpisodeMenu[] = {{1, "M_EPI1", M_Episode, 'k'},
                            {1, "M_EPI2", M_Episode, 't'},
                            {1, "M_EPI3", M_Episode, 'i'},
                            {1, "M_EPI4", M_Episode, 't'}};

menu_t EpiDef = {
    ep_end,        // # of menu items
    &MainDef,      // previous menu
    EpisodeMenu,   // menuitem_t ->
    M_DrawEpisode, // drawing routine ->
    48,
    63, // x,y
    ep1 // lastOn
};

//
// NEW GAME
//
enum
{
  killthings,
  toorough,
  hurtme,
  violence,
  nightmare,
  newg_end
} newgame_e;

menuitem_t NewGameMenu[] = {{1, "M_JKILL", M_ChooseSkill, 'i'},
                            {1, "M_ROUGH", M_ChooseSkill, 'h'},
                            {1, "M_HURT", M_ChooseSkill, 'h'},
                            {1, "M_ULTRA", M_ChooseSkill, 'u'},
                            {1, "M_NMARE", M_ChooseSkill, 'n'}};

menu_t NewDef = {
    newg_end,      // # of menu items
    &EpiDef,       // previous menu
    NewGameMenu,   // menuitem_t ->
    M_DrawNewGame, // drawing routine ->
    48,
    63,    // x,y
    hurtme // lastOn
};

//
// OPTIONS MENU
//
enum
{
  endgame,
  messages,
  detail,
  scrnsize,
  option_empty1,
  mousesens,
  option_empty2,
  soundvol,
  opt_end
} options_e;

menuitem_t OptionsMenu[] = {{1, "M_ENDGAM", M_EndGame, 'e'},
                            {1, "M_MESSG", M_ChangeMessages, 'm'},
                            {1, "M_DETAIL", M_ChangeDetail, 'g'},
                            {2, "M_SCRNSZ", M_SizeDisplay, 's'},
                            {-1, "", 0, '\0'},
                            {2, "M_MSENS", M_ChangeSensitivity, 'm'},
                            {-1, "", 0, '\0'},
                            {1, "M_SVOL", M_Sound, 's'}};

menu_t OptionsDef = {opt_end, &MainDef, OptionsMenu, M_DrawOptions, 60,
                     37,      0};

//
// Read This! MENU 1 & 2
//
enum
{
  rdthsempty1,
  read1_end
} read_e;

menuitem_t ReadMenu1[] = {{1, "", M_ReadThis2, 0}};

menu_t ReadDef1 = {read1_end, &MainDef, ReadMenu1, M_DrawReadThis1,
                   280,       185,      0};

enum
{
  rdthsempty2,
  read2_end
} read_e2;

menuitem_t ReadMenu2[] = {{1, "", M_FinishReadThis, 0}};

menu_t ReadDef2 = {read2_end, &ReadDef1, ReadMenu2, M_DrawReadThis2,
                   330,       175,       0};

//
// SOUND VOLUME MENU
//
enum
{
  sfx_vol,
  sfx_empty1,
  music_vol,
  sfx_empty2,
  sound_end
} sound_e;

menuitem_t SoundMenu[] = {{2, "M_SFXVOL", M_SfxVol, 's'},
                          {-1, "", 0, '\0'},
                          {2, "M_MUSVOL", M_MusicVol, 'm'},
                          {-1, "", 0, '\0'}};

menu_t SoundDef = {sound_end, &OptionsDef, SoundMenu, M_DrawSound, 80, 64, 0};

//
// LOAD GAME MENU
//
enum
{
  load1,
  load2,
  load3,
  load4,
  load5,
  load6,
  load_end
} load_e;

menuitem_t LoadMenu[] = {
    {1, "", M_LoadSelect, '1'}, {1, "", M_LoadSelect, '2'},
    {1, "", M_LoadSelect, '3'}, {1, "", M_LoadSelect, '4'},
    {1, "", M_LoadSelect, '5'}, {1, "", M_LoadSelect, '6'}};

menu_t LoadDef = {load_end, &MainDef, LoadMenu, M_DrawLoad, 80, 54, 0};

//
// SAVE GAME MENU
//
menuitem_t SaveMenu[] = {
    {1, "", M_SaveSelect, '1'}, {1, "", M_SaveSelect, '2'},
    {1, "", M_SaveSelect, '3'}, {1, "", M_SaveSelect, '4'},
    {1, "", M_SaveSelect, '5'}, {1, "", M_SaveSelect, '6'}};

menu_t SaveDef = {load_end, &MainDef, SaveMenu, M_DrawSave, 80, 54, 0};

//
// M_ReadSaveStrings
//  read the strings from the savegame files
//
void M_ReadSaveStrings(void)
{
  FILE *handle;
  int i;
  char name[256];

  for (i = 0; i < load_end; i++)
    {
      int retval;
      m_str_copy(name, p_save_game_file(i), sizeof(name));

      handle = fopen(name, "rb");
      if (handle == NULL)
        {
          m_str_copy(savegamestrings[i], EMPTYSTRING, SAVESTRINGSIZE);
          LoadMenu[i].status = 0;
          continue;
        }
      retval = fread(&savegamestrings[i], 1, SAVESTRINGSIZE, handle);
      fclose(handle);
      LoadMenu[i].status = retval == SAVESTRINGSIZE;
    }
}

//
// M_LoadGame & Cie.
//
void M_DrawLoad(void)
{
  int i;

  v_draw_patch_direct(72, 28, w_cache_lump_name(("M_LOADG"), PU_CACHE));

  for (i = 0; i < load_end; i++)
    {
      M_DrawSaveLoadBorder(LoadDef.x, LoadDef.y + LINEHEIGHT * i);
      M_WriteText(LoadDef.x, LoadDef.y + LINEHEIGHT * i, savegamestrings[i]);
    }
}

//
// Draw border for the savegame description
//
void M_DrawSaveLoadBorder(int x, int y)
{
  int i;

  v_draw_patch_direct(x - 8, y + 7,
                      w_cache_lump_name(("M_LSLEFT"), PU_CACHE));

  for (i = 0; i < 24; i++)
    {
      v_draw_patch_direct(x, y + 7,
                          w_cache_lump_name(("M_LSCNTR"), PU_CACHE));
      x += 8;
    }

  v_draw_patch_direct(x, y + 7, w_cache_lump_name(("M_LSRGHT"), PU_CACHE));
}

//
// User wants to load this game
//
void M_LoadSelect(int choice)
{
  char name[256];

  m_str_copy(name, p_save_game_file(choice), sizeof(name));

  g_load_game(name);
  M_ClearMenus();
}

//
// Selected from DOOM menu
//
void M_LoadGame(int choice)
{
  if (netgame)
    {
      M_StartMessage((LOADNET), NULL, false);
      return;
    }

  M_SetupNextMenu(&LoadDef);
  M_ReadSaveStrings();
}

//
//  M_SaveGame & Cie.
//
void M_DrawSave(void)
{
  int i;

  v_draw_patch_direct(72, 28, w_cache_lump_name(("M_SAVEG"), PU_CACHE));
  for (i = 0; i < load_end; i++)
    {
      M_DrawSaveLoadBorder(LoadDef.x, LoadDef.y + LINEHEIGHT * i);
      M_WriteText(LoadDef.x, LoadDef.y + LINEHEIGHT * i, savegamestrings[i]);
    }

  if (saveStringEnter)
    {
      i = M_StringWidth(savegamestrings[saveSlot]);
      M_WriteText(LoadDef.x + i, LoadDef.y + LINEHEIGHT * saveSlot, "_");
    }
}

//
// m_responder calls this when user is finished
//
void M_DoSave(int slot)
{
  g_save_game(slot, savegamestrings[slot]);
  M_ClearMenus();

  // PICK QUICKSAVE SLOT YET?
  if (quickSaveSlot == -2) quickSaveSlot = slot;
}

//
// Generate a default save slot name when the user saves to
// an empty slot via the joypad.
//
static void SetDefaultSaveName(int slot)
{
  // map from IWAD or PWAD?
  if (w_is_iwad_lump(maplumpinfo) && strcmp(savegamedir, ""))
    {
      snprintf(savegamestrings[itemOn], SAVESTRINGSIZE, "%s",
               maplumpinfo->name);
    }
  else
    {
      char *wadname = m_string_duplicate(w_wad_name_for_lump(maplumpinfo));
      char *ext = strrchr(wadname, '.');

      if (ext != NULL)
        {
          *ext = '\0';
        }

      snprintf(savegamestrings[itemOn], SAVESTRINGSIZE, "%s (%s)",
               maplumpinfo->name, wadname);
      free(wadname);
    }
  m_force_uppercase(savegamestrings[itemOn]);
  joypadSave = false;
}

//
// User wants to save. Start string input for m_responder
//
void M_SaveSelect(int choice)
{
  int x, y;

  // we are going to be intercepting all chars
  saveStringEnter = 1;

  // We need to turn on text input:
  x = LoadDef.x - 11;
  y = LoadDef.y + choice * LINEHEIGHT - 4;
  i_start_text_input(x, y, x + 8 + 24 * 8 + 8, y + LINEHEIGHT - 2);

  saveSlot = choice;
  m_str_copy(saveOldString, savegamestrings[choice], SAVESTRINGSIZE);
  if (!strcmp(savegamestrings[choice], EMPTYSTRING))
    {
      savegamestrings[choice][0] = 0;

      if (joypadSave)
        {
          SetDefaultSaveName(choice);
        }
    }
  saveCharIndex = strlen(savegamestrings[choice]);
}

//
// Selected from DOOM menu
//
void M_SaveGame(int choice)
{
  if (!usergame)
    {
      M_StartMessage((SAVEDEAD), NULL, false);
      return;
    }

  if (gamestate != GS_LEVEL) return;

  M_SetupNextMenu(&SaveDef);
  M_ReadSaveStrings();
}

//
//      M_QuickSave
//
static char tempstring[90];

void M_QuickSaveResponse(int key)
{
  if (key == key_menu_confirm)
    {
      M_DoSave(quickSaveSlot);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(NULL, sfx_swtchx);
#endif
    }
}

void M_QuickSave(void)
{
  if (!usergame)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(NULL, sfx_oof);
#endif
      return;
    }

  if (gamestate != GS_LEVEL) return;

  if (quickSaveSlot < 0)
    {
      m_start_control_panel();
      M_ReadSaveStrings();
      M_SetupNextMenu(&SaveDef);
      quickSaveSlot = -2; // means to pick a slot now
      return;
    }
  snprintf(tempstring, sizeof(tempstring), QSPROMPT,
           savegamestrings[quickSaveSlot]);
  M_StartMessage(tempstring, M_QuickSaveResponse, true);
}

//
// M_QuickLoad
//
void M_QuickLoadResponse(int key)
{
  if (key == key_menu_confirm)
    {
      M_LoadSelect(quickSaveSlot);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(NULL, sfx_swtchx);
#endif
    }
}

void M_QuickLoad(void)
{
  if (netgame)
    {
      M_StartMessage((QLOADNET), NULL, false);
      return;
    }

  if (quickSaveSlot < 0)
    {
      M_StartMessage((QSAVESPOT), NULL, false);
      return;
    }
  snprintf(tempstring, sizeof(tempstring), QLPROMPT,
           savegamestrings[quickSaveSlot]);
  M_StartMessage(tempstring, M_QuickLoadResponse, true);
}

//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1(void)
{
  inhelpscreens = true;

  v_draw_patch_direct(0, 0, w_cache_lump_name(("HELP2"), PU_CACHE));
}

//
// Read This Menus - optional second page.
//
void M_DrawReadThis2(void)
{
  inhelpscreens = true;

  // We only ever draw the second page if this is
  // gameversion == exe_doom_1_9 and gamemode == registered

  v_draw_patch_direct(0, 0, w_cache_lump_name(("HELP1"), PU_CACHE));
}

void M_DrawReadThisCommercial(void)
{
  inhelpscreens = true;

  v_draw_patch_direct(0, 0, w_cache_lump_name(("HELP"), PU_CACHE));
}

//
// Change Sfx & Music volumes
//
void M_DrawSound(void)
{
  v_draw_patch_direct(60, 38, w_cache_lump_name(("M_SVOL"), PU_CACHE));

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (sfx_vol + 1), 16,
               g_sfx_volume);

  M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (music_vol + 1), 16,
               g_music_volume);
#else
  M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (sfx_vol + 1), 16, 0);

  M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (music_vol + 1), 16, 0);
#endif
}

void M_Sound(int choice) { M_SetupNextMenu(&SoundDef); }

void M_SfxVol(int choice)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  switch (choice)
    {
    case 0:
      if (g_sfx_volume) g_sfx_volume--;
      break;
    case 1:
      if (g_sfx_volume < 15) g_sfx_volume++;
      break;
    }

  s_set_sfx_volume(g_sfx_volume * 8);
#endif
}

void M_MusicVol(int choice)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  switch (choice)
    {
    case 0:
      if (g_music_volume) g_music_volume--;
      break;
    case 1:
      if (g_music_volume < 15) g_music_volume++;
      break;
    }

  s_set_music_volume(g_music_volume * 8);
#endif
}

//
// M_Drawmain_menu
//
void M_Drawmain_menu(void)
{
  v_draw_patch_direct(94, 2, w_cache_lump_name(("M_DOOM"), PU_CACHE));
}

//
// M_NewGame
//
void M_DrawNewGame(void)
{
  v_draw_patch_direct(96, 14, w_cache_lump_name(("M_NEWG"), PU_CACHE));
  v_draw_patch_direct(54, 38, w_cache_lump_name(("M_SKILL"), PU_CACHE));
}

void M_NewGame(int choice)
{
  if (netgame && !demoplayback)
    {
      M_StartMessage((NEWGAME), NULL, false);
      return;
    }

  // Chex Quest disabled the episode select screen, as did Doom II.

  if (gamemode == commercial || gameversion == exe_chex)
    M_SetupNextMenu(&NewDef);
  else
    M_SetupNextMenu(&EpiDef);
}

//
//      M_Episode
//
int epi;

void M_DrawEpisode(void)
{
  v_draw_patch_direct(54, 38, w_cache_lump_name(("M_EPISOD"), PU_CACHE));
}

void M_VerifyNightmare(int key)
{
  if (key != key_menu_confirm) return;

  g_deferred_init_new(nightmare, epi + 1, 1);
  M_ClearMenus();
}

void M_ChooseSkill(int choice)
{
  if (choice == nightmare)
    {
      M_StartMessage((NIGHTMARE), M_VerifyNightmare, true);
      return;
    }

  g_deferred_init_new(choice, epi + 1, 1);
  M_ClearMenus();
}

void M_Episode(int choice)
{
  if ((gamemode == shareware) && choice)
    {
      M_StartMessage((SWSTRING), NULL, false);
      M_SetupNextMenu(&ReadDef1);
      return;
    }

  epi = choice;
  M_SetupNextMenu(&NewDef);
}

//
// M_Options
//
static const char *detailNames[2] = {"M_GDHIGH", "M_GDLOW"};
static const char *msgNames[2] = {"M_MSGOFF", "M_MSGON"};

void M_DrawOptions(void)
{
  v_draw_patch_direct(108, 15, w_cache_lump_name(("M_OPTTTL"), PU_CACHE));

  v_draw_patch_direct(
      OptionsDef.x + 175, OptionsDef.y + LINEHEIGHT * detail,
      w_cache_lump_name((detailNames[g_detail_level]), PU_CACHE));

  v_draw_patch_direct(OptionsDef.x + 120,
                      OptionsDef.y + LINEHEIGHT * messages,
                      w_cache_lump_name((msgNames[g_show_messages]), PU_CACHE));

  M_DrawThermo(OptionsDef.x, OptionsDef.y + LINEHEIGHT * (mousesens + 1), 10,
               g_mouse_sensitivity);

  M_DrawThermo(OptionsDef.x, OptionsDef.y + LINEHEIGHT * (scrnsize + 1), 9,
               screenSize);
}

void M_Options(int choice) { M_SetupNextMenu(&OptionsDef); }

//
//      Toggle messages on/off
//
void M_ChangeMessages(int choice)
{
  // warning: unused parameter `int choice'
  choice = 0;
  g_show_messages = 1 - g_show_messages;

  if (!g_show_messages)
    players[consoleplayer].message = (MSGOFF);
  else
    players[consoleplayer].message = (MSGON);

  message_dontfuckwithme = true;
}

//
// M_EndGame
//
void M_EndGameResponse(int key)
{
  if (key != key_menu_confirm) return;

  currentMenu->lastOn = itemOn;
  M_ClearMenus();
  d_start_title();
}

void M_EndGame(int choice)
{
  choice = 0;
  if (!usergame)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(NULL, sfx_oof);
#endif
      return;
    }

  if (netgame)
    {
      M_StartMessage((NETEND), NULL, false);
      return;
    }

  M_StartMessage((ENDGAME), M_EndGameResponse, true);
}

//
// M_ReadThis
//
void M_ReadThis(int choice)
{
  choice = 0;
  M_SetupNextMenu(&ReadDef1);
}

void M_ReadThis2(int choice)
{
  choice = 0;
  M_SetupNextMenu(&ReadDef2);
}

void M_FinishReadThis(int choice)
{
  choice = 0;
  M_SetupNextMenu(&MainDef);
}

#ifdef CONFIG_GAMES_NXDOOM_SOUND
//
// M_QuitDOOM
//
int quitsounds[8] = {sfx_pldeth, sfx_dmpain, sfx_popain, sfx_slop,
                     sfx_telept, sfx_posit1, sfx_posit3, sfx_sgtatk};

int quitsounds2[8] = {sfx_vilact, sfx_getpow, sfx_boscub, sfx_slop,
                      sfx_skeswg, sfx_kntdth, sfx_bspact, sfx_sgtatk};
#endif

void M_QuitResponse(int key)
{
  if (key != key_menu_confirm) return;
  if (!netgame)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (gamemode == commercial)
        s_start_sound(NULL, quitsounds2[(gametic >> 2) & 7]);
      else
        s_start_sound(NULL, quitsounds[(gametic >> 2) & 7]);
#endif
      i_wait_vbl(105);
    }
  i_quit();
}

static const char *M_SelectEndMessage(void)
{
  const char **endmsg;

  if (logical_gamemission == doom)
    {
      // Doom 1

      endmsg = doom1_endmsg;
    }
  else
    {
      // Doom 2

      endmsg = doom2_endmsg;
    }

  return endmsg[gametic % NUM_QUITMESSAGES];
}

void M_QuitDOOM(int choice)
{
  snprintf(endstring, sizeof(endstring), "%s\n\n" DOSY,
           (M_SelectEndMessage()));

  M_StartMessage(endstring, M_QuitResponse, true);
}

void M_ChangeSensitivity(int choice)
{
  switch (choice)
    {
    case 0:
      if (g_mouse_sensitivity) g_mouse_sensitivity--;
      break;
    case 1:
      if (g_mouse_sensitivity < 9) g_mouse_sensitivity++;
      break;
    }
}

void M_ChangeDetail(int choice)
{
  choice = 0;
  g_detail_level = 1 - g_detail_level;

  r_set_view_size(screenblocks, g_detail_level);

  if (!g_detail_level)
    players[consoleplayer].message = (DETAILHI);
  else
    players[consoleplayer].message = (DETAILLO);
}

void M_SizeDisplay(int choice)
{
  switch (choice)
    {
    case 0:
      if (screenSize > 0)
        {
          screenblocks--;
          screenSize--;
        }
      break;
    case 1:
      if (screenSize < 8)
        {
          screenblocks++;
          screenSize++;
        }
      break;
    }

  r_set_view_size(screenblocks, g_detail_level);
}

//
//      Menu Functions
//
void M_DrawThermo(int x, int y, int thermWidth, int thermDot)
{
  int xx;
  int i;

  xx = x;
  v_draw_patch_direct(xx, y, w_cache_lump_name(("M_THERML"), PU_CACHE));
  xx += 8;
  for (i = 0; i < thermWidth; i++)
    {
      v_draw_patch_direct(xx, y, w_cache_lump_name(("M_THERMM"), PU_CACHE));
      xx += 8;
    }
  v_draw_patch_direct(xx, y, w_cache_lump_name(("M_THERMR"), PU_CACHE));

  v_draw_patch_direct((x + 8) + thermDot * 8, y,
                      w_cache_lump_name(("M_THERMO"), PU_CACHE));
}

void M_StartMessage(const char *string, void *routine, boolean input)
{
  messageLastMenuActive = menuactive;
  messageToPrint = 1;
  messageString = string;
  messageRoutine = routine;
  messageNeedsInput = input;
  menuactive = true;
  return;
}

//
// Find string width from hu_font chars
//
int M_StringWidth(const char *string)
{
  size_t i;
  int w = 0;
  int c;

  for (i = 0; i < strlen(string); i++)
    {
      c = toupper(string[i]) - HU_FONTSTART;
      if (c < 0 || c >= HU_FONTSIZE)
        w += 4;
      else
        w += SHORT(hu_font[c]->width);
    }

  return w;
}

//
//      Find string height from hu_font chars
//
int M_StringHeight(const char *string)
{
  size_t i;
  int h;
  int height = SHORT(hu_font[0]->height);

  h = height;
  for (i = 0; i < strlen(string); i++)
    if (string[i] == '\n') h += height;

  return h;
}

//
//      Write a string using the hu_font
//
void M_WriteText(int x, int y, const char *string)
{
  int w;
  const char *ch;
  int c;
  int cx;
  int cy;

  ch = string;
  cx = x;
  cy = y;

  while (1)
    {
      c = *ch++;
      if (!c) break;
      if (c == '\n')
        {
          cx = x;
          cy += 12;
          continue;
        }

      c = toupper(c) - HU_FONTSTART;
      if (c < 0 || c >= HU_FONTSIZE)
        {
          cx += 4;
          continue;
        }

      w = SHORT(hu_font[c]->width);
      if (cx + w > SCREENWIDTH) break;
      v_draw_patch_direct(cx, cy, hu_font[c]);
      cx += w;
    }
}

// These keys evaluate to a "null" key in Vanilla Doom that allows weird
// jumping in the menus. Preserve this behavior for accuracy.

static boolean IsNullKey(int key)
{
  return key == KEY_PAUSE || key == KEY_CAPSLOCK || key == KEY_SCRLCK ||
         key == KEY_NUMLOCK;
}

//
// CONTROL PANEL
//

//
// m_responder
//
boolean m_responder(event_t *ev)
{
  int ch;
  int key;
  int i;
  static int mousewait = 0;
  static int mousey = 0;
  static int lasty = 0;
  static int mousex = 0;
  static int lastx = 0;
  int dir;

  // In testcontrols mode, none of the function keys should do anything
  // - the only key is escape to quit.

  if (testcontrols)
    {
      if (ev->type == ev_quit ||
          (ev->type == ev_keydown &&
           (ev->data1 == key_menu_activate || ev->data1 == key_menu_quit)))
        {
          i_quit();
          return true;
        }

      return false;
    }

  // "close" button pressed on window?
  if (ev->type == ev_quit)
    {
      // First click on close button = bring up quit confirm message.
      // Second click on close button = confirm quit

      if (menuactive && messageToPrint && messageRoutine == M_QuitResponse)
        {
          M_QuitResponse(key_menu_confirm);
        }
      else
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_swtchn);
#endif
          M_QuitDOOM(0);
        }

      return true;
    }

  // key is the key pressed, ch is the actual character typed

  ch = 0;
  key = -1;

  if (ev->type == ev_joystick)
    {
      // Simulate key presses from joystick events to interact with the menu.

      if (menuactive)
        {
          if (JOY_GET_DPAD(ev->data6) != JOY_DIR_NONE)
            {
              dir = JOY_GET_DPAD(ev->data6);
            }
          else if (JOY_GET_LSTICK(ev->data6) != JOY_DIR_NONE)
            {
              dir = JOY_GET_LSTICK(ev->data6);
            }
          else
            {
              dir = JOY_GET_RSTICK(ev->data6);
            }

          if (dir & JOY_DIR_UP)
            {
              key = key_menu_up;
              joywait = i_get_time() + 5;
            }
          else if (dir & JOY_DIR_DOWN)
            {
              key = key_menu_down;
              joywait = i_get_time() + 5;
            }
          if (dir & JOY_DIR_LEFT)
            {
              key = key_menu_left;
              joywait = i_get_time() + 5;
            }
          else if (dir & JOY_DIR_RIGHT)
            {
              key = key_menu_right;
              joywait = i_get_time() + 5;
            }

#define JOY_BUTTON_MAPPED(x) ((x) >= 0)
#define JOY_BUTTON_PRESSED(x)                                                \
  (JOY_BUTTON_MAPPED(x) && (ev->data1 & (1 << (x))) != 0)

          if (JOY_BUTTON_PRESSED(joybfire))
            {
              // Simulate a 'Y' keypress when Doom show a Y/N dialog with Fire
              // button.
              if (messageToPrint && messageNeedsInput)
                {
                  key = key_menu_confirm;
                }
              // Simulate pressing "Enter" when we are supplying a save slot
              // name
              else if (saveStringEnter)
                {
                  key = KEY_ENTER;
                }
              else
                {
                  // if selecting a save slot via joypad, set a flag
                  if (currentMenu == &SaveDef)
                    {
                      joypadSave = true;
                    }
                  key = key_menu_forward;
                }
              joywait = i_get_time() + 5;
            }
          if (JOY_BUTTON_PRESSED(joybuse))
            {
              // Simulate a 'N' keypress when Doom show a Y/N dialog with Use
              // button.
              if (messageToPrint && messageNeedsInput)
                {
                  key = key_menu_abort;
                }
              // If user was entering a save name, back out
              else if (saveStringEnter)
                {
                  key = KEY_ESCAPE;
                }
              else
                {
                  key = key_menu_back;
                }
              joywait = i_get_time() + 5;
            }
        }
      if (JOY_BUTTON_PRESSED(joybmenu))
        {
          key = key_menu_activate;
          joywait = i_get_time() + 5;
        }
    }
  else
    {
      if (ev->type == ev_mouse && mousewait < i_get_time() && menuactive)
        {
          mousey += ev->data3;
          if (mousey < lasty - 30)
            {
              key = key_menu_down;
              mousewait = i_get_time() + 5;
              mousey = lasty -= 30;
            }
          else if (mousey > lasty + 30)
            {
              key = key_menu_up;
              mousewait = i_get_time() + 5;
              mousey = lasty += 30;
            }

          mousex += ev->data2;
          if (mousex < lastx - 30)
            {
              key = key_menu_left;
              mousewait = i_get_time() + 5;
              mousex = lastx -= 30;
            }
          else if (mousex > lastx + 30)
            {
              key = key_menu_right;
              mousewait = i_get_time() + 5;
              mousex = lastx += 30;
            }

          if (ev->data1 & 1)
            {
              key = key_menu_forward;
              mousewait = i_get_time() + 15;
            }

          if (ev->data1 & 2)
            {
              key = key_menu_back;
              mousewait = i_get_time() + 15;
            }
        }
      else
        {
          if (ev->type == ev_keydown)
            {
              key = ev->data1;
              ch = ev->data2;
            }
        }
    }

  if (key == -1) return false;

  // Save Game string input
  if (saveStringEnter)
    {
      switch (key)
        {
        case KEY_BACKSPACE:
          if (saveCharIndex > 0)
            {
              saveCharIndex--;
              savegamestrings[saveSlot][saveCharIndex] = 0;
            }
          break;

        case KEY_ESCAPE:
          saveStringEnter = 0;
          i_stop_text_input();
          m_str_copy(savegamestrings[saveSlot], saveOldString,
                     SAVESTRINGSIZE);
          break;

        case KEY_ENTER:
          saveStringEnter = 0;
          i_stop_text_input();
          if (savegamestrings[saveSlot][0]) M_DoSave(saveSlot);
          break;

        default:
          // Savegame name entry. This is complicated.
          // Vanilla has a bug where the shift key is ignored when entering
          // a savegame name. If vanilla_keyboard_mapping is on, we want
          // to emulate this bug by using ev->data1. But if it's turned off,
          // it implies the user doesn't care about Vanilla emulation:
          // instead, use ev->data3 which gives the fully-translated and
          // modified key input.

          if (ev->type != ev_keydown)
            {
              break;
            }
          if (vanilla_keyboard_mapping)
            {
              ch = ev->data1;
            }
          else
            {
              ch = ev->data3;
            }

          ch = toupper(ch);

          if (ch != ' ' &&
              (ch - HU_FONTSTART < 0 || ch - HU_FONTSTART >= HU_FONTSIZE))
            {
              break;
            }

          if (ch >= 32 && ch <= 127 && saveCharIndex < SAVESTRINGSIZE - 1 &&
              M_StringWidth(savegamestrings[saveSlot]) <
                  (SAVESTRINGSIZE - 2) * 8)
            {
              savegamestrings[saveSlot][saveCharIndex++] = ch;
              savegamestrings[saveSlot][saveCharIndex] = 0;
            }
          break;
        }
      return true;
    }

  // Take care of any messages that need input
  if (messageToPrint)
    {
      if (messageNeedsInput)
        {
          if (key != ' ' && key != KEY_ESCAPE && key != key_menu_confirm &&
              key != key_menu_abort)
            {
              return false;
            }
        }

      menuactive = messageLastMenuActive;
      messageToPrint = 0;
      if (messageRoutine) messageRoutine(key);

      menuactive = false;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(NULL, sfx_swtchx);
#endif
      return true;
    }

  if ((devparm && key == key_menu_help) ||
      (key != 0 && key == key_menu_screenshot))
    {
      g_screenshot();
      return true;
    }

  // F-Keys
  if (!menuactive)
    {
      if (key == key_menu_decscreen) // Screen size down
        {
          if (automapactive || chat_on) return false;
          M_SizeDisplay(0);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_stnmov);
#endif
          return true;
        }
      else if (key == key_menu_incscreen) // Screen size up
        {
          if (automapactive || chat_on) return false;
          M_SizeDisplay(1);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_stnmov);
#endif
          return true;
        }
      else if (key == key_menu_help) // Help key
        {
          m_start_control_panel();

          if (gameversion >= exe_ultimate)
            currentMenu = &ReadDef2;
          else
            currentMenu = &ReadDef1;

          itemOn = 0;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_swtchn);
#endif
          return true;
        }
      else if (key == key_menu_save) // Save
        {
          m_start_control_panel();
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_swtchn);
#endif
          M_SaveGame(0);
          return true;
        }
      else if (key == key_menu_load) // Load
        {
          m_start_control_panel();
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_swtchn);
#endif
          M_LoadGame(0);
          return true;
        }
      else if (key == key_menu_volume) // Sound Volume
        {
          m_start_control_panel();
          currentMenu = &SoundDef;
          itemOn = sfx_vol;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_swtchn);
#endif
          return true;
        }
      else if (key == key_menu_detail) // Detail toggle
        {
          M_ChangeDetail(0);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_swtchn);
#endif
          return true;
        }
      else if (key == key_menu_qsave) // Quicksave
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_swtchn);
#endif
          M_QuickSave();
          return true;
        }
      else if (key == key_menu_endgame) // End game
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_swtchn);
#endif
          M_EndGame(0);
          return true;
        }
      else if (key == key_menu_messages) // Toggle messages
        {
          M_ChangeMessages(0);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_swtchn);
#endif
          return true;
        }
      else if (key == key_menu_qload) // Quickload
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_swtchn);
#endif
          M_QuickLoad();
          return true;
        }
      else if (key == key_menu_quit) // Quit DOOM
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_swtchn);
#endif
          M_QuitDOOM(0);
          return true;
        }
      else if (key == key_menu_gamma) // gamma toggle
        {
          usegamma++;
          if (usegamma > 4) usegamma = 0;
          players[consoleplayer].message = (gammamsg[usegamma]);
          i_set_palette(w_cache_lump_name(("PLAYPAL"), PU_CACHE));
          return true;
        }
    }

  // Pop-up menu?
  if (!menuactive)
    {
      if (key == key_menu_activate)
        {
          m_start_control_panel();
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_swtchn);
#endif
          return true;
        }
      return false;
    }

  // Keys usable within menu

  if (key == key_menu_down)
    {
      // Move down to next item

      do
        {
          if (itemOn + 1 > currentMenu->numitems - 1)
            itemOn = 0;
          else
            itemOn++;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_pstop);
#endif
        }
      while (currentMenu->menuitems[itemOn].status == -1);

      return true;
    }
  else if (key == key_menu_up)
    {
      // Move back up to previous item

      do
        {
          if (!itemOn)
            itemOn = currentMenu->numitems - 1;
          else
            itemOn--;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_pstop);
#endif
        }
      while (currentMenu->menuitems[itemOn].status == -1);

      return true;
    }
  else if (key == key_menu_left)
    {
      // Slide slider left

      if (currentMenu->menuitems[itemOn].routine &&
          currentMenu->menuitems[itemOn].status == 2)
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_stnmov);
#endif
          currentMenu->menuitems[itemOn].routine(0);
        }
      return true;
    }
  else if (key == key_menu_right)
    {
      // Slide slider right

      if (currentMenu->menuitems[itemOn].routine &&
          currentMenu->menuitems[itemOn].status == 2)
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_stnmov);
#endif
          currentMenu->menuitems[itemOn].routine(1);
        }
      return true;
    }
  else if (key == key_menu_forward)
    {
      // Activate menu item

      if (currentMenu->menuitems[itemOn].routine &&
          currentMenu->menuitems[itemOn].status)
        {
          currentMenu->lastOn = itemOn;
          if (currentMenu->menuitems[itemOn].status == 2)
            {
              currentMenu->menuitems[itemOn].routine(1); // right arrow
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(NULL, sfx_stnmov);
#endif
            }
          else
            {
              currentMenu->menuitems[itemOn].routine(itemOn);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(NULL, sfx_pistol);
#endif
            }
        }
      return true;
    }
  else if (key == key_menu_activate)
    {
      // Deactivate menu

      currentMenu->lastOn = itemOn;
      M_ClearMenus();
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(NULL, sfx_swtchx);
#endif
      return true;
    }
  else if (key == key_menu_back)
    {
      // Go back to previous menu

      currentMenu->lastOn = itemOn;
      if (currentMenu->prevMenu)
        {
          currentMenu = currentMenu->prevMenu;
          itemOn = currentMenu->lastOn;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, sfx_swtchn);
#endif
        }
      return true;
    }

  // Keyboard shortcut?
  // Vanilla Doom has a weird behavior where it jumps to the scroll bars
  // when the certain keys are pressed, so emulate this.

  else if (ch != 0 || IsNullKey(key))
    {
      for (i = itemOn + 1; i < currentMenu->numitems; i++)
        {
          if (currentMenu->menuitems[i].alphaKey == ch)
            {
              itemOn = i;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(NULL, sfx_pstop);
#endif
              return true;
            }
        }

      for (i = 0; i <= itemOn; i++)
        {
          if (currentMenu->menuitems[i].alphaKey == ch)
            {
              itemOn = i;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(NULL, sfx_pstop);
#endif
              return true;
            }
        }
    }

  return false;
}

//
// m_start_control_panel
//
void m_start_control_panel(void)
{
  // intro might call this repeatedly
  if (menuactive) return;

  menuactive = 1;
  currentMenu = &MainDef;       // JDC
  itemOn = currentMenu->lastOn; // JDC
}

// Display OPL debug messages - hack for GENMIDI development.

#if 0
static void M_DrawOPLDev(void)
{
    char debug[1024];
    char *curr, *p;
    int line;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
    i_opl_dev_messages(debug, sizeof(debug));
#endif
    curr = debug;
    line = 0;

    for (;;)
    {
        p = strchr(curr, '\n');

        if (p != NULL)
        {
            *p = '\0';
        }

        M_WriteText(0, line * 8, curr);
        ++line;

        if (p == NULL)
        {
            break;
        }

        curr = p + 1;
    }
}
#endif

//
// m_drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void m_drawer(void)
{
  static short x;
  static short y;
  unsigned int i;
  unsigned int max;
  char string[80];
  const char *name;
  int start;

  inhelpscreens = false;

  // Horiz. & Vertically center string and print it.
  if (messageToPrint)
    {
      start = 0;
      y = SCREENHEIGHT / 2 - M_StringHeight(messageString) / 2;
      while (messageString[start] != '\0')
        {
          boolean foundnewline = false;

          for (i = 0; messageString[start + i] != '\0'; i++)
            {
              if (messageString[start + i] == '\n')
                {
                  m_str_copy(string, messageString + start, sizeof(string));
                  if (i < sizeof(string))
                    {
                      string[i] = '\0';
                    }

                  foundnewline = true;
                  start += i + 1;
                  break;
                }
            }

          if (!foundnewline)
            {
              m_str_copy(string, messageString + start, sizeof(string));
              start += strlen(string);
            }

          x = SCREENWIDTH / 2 - M_StringWidth(string) / 2;
          M_WriteText(x, y, string);
          y += SHORT(hu_font[0]->height);
        }

      return;
    }

#if 0
    if (opldev)
    {
        M_DrawOPLDev();
    }
#endif

  if (!menuactive) return;

  if (currentMenu->routine) currentMenu->routine(); // call Draw routine

  // DRAW MENU
  x = currentMenu->x;
  y = currentMenu->y;
  max = currentMenu->numitems;

  for (i = 0; i < max; i++)
    {
      name = (currentMenu->menuitems[i].name);

      if (name[0] && w_check_num_for_name(name) > 0)
        {
          v_draw_patch_direct(x, y, w_cache_lump_name(name, PU_CACHE));
        }
      y += LINEHEIGHT;
    }

  // DRAW SKULL
  v_draw_patch_direct(x + SKULLXOFF, currentMenu->y - 5 + itemOn * LINEHEIGHT,
                      w_cache_lump_name((skullName[whichSkull]), PU_CACHE));
}

//
// M_ClearMenus
//
void M_ClearMenus(void)
{
  menuactive = 0;
  // if (!netgame && usergame && paused)
  //       sendpause = true;
}

//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
  currentMenu = menudef;
  itemOn = currentMenu->lastOn;
}

//
// m_ticker
//
void m_ticker(void)
{
  if (--skullAnimCounter <= 0)
    {
      whichSkull ^= 1;
      skullAnimCounter = 8;
    }
}

//
// m_init
//
void m_init(void)
{
  currentMenu = &MainDef;
  menuactive = 0;
  itemOn = currentMenu->lastOn;
  whichSkull = 0;
  skullAnimCounter = 10;
  screenSize = screenblocks - 3;
  messageToPrint = 0;
  messageString = NULL;
  messageLastMenuActive = menuactive;
  quickSaveSlot = -1;

  // Here we could catch other version dependencies,
  //  like HELP1/2, and four episodes.

  // The same hacks were used in the original Doom EXEs.

  if (gameversion >= exe_ultimate)
    {
      main_menu[readthis].routine = M_ReadThis2;
      ReadDef2.prevMenu = NULL;
    }

  if (gameversion >= exe_final && gameversion <= exe_final2)
    {
      ReadDef2.routine = M_DrawReadThisCommercial;
    }

  if (gamemode == commercial)
    {
      main_menu[readthis] = main_menu[quitdoom];
      MainDef.numitems--;
      MainDef.y += 8;
      NewDef.prevMenu = &MainDef;
      ReadDef1.routine = M_DrawReadThisCommercial;
      ReadDef1.x = 330;
      ReadDef1.y = 165;
      ReadMenu1[rdthsempty1].routine = M_FinishReadThis;
    }

  // Versions of doom.exe before the Ultimate Doom release only had
  // three episodes; if we're emulating one of those then don't try
  // to show episode four. If we are, then do show episode four
  // (should crash if missing).
  if (gameversion < exe_ultimate)
    {
      EpiDef.numitems--;
    }
  // chex.exe shows only one episode.
  else if (gameversion == exe_chex)
    {
      EpiDef.numitems = 1;
    }

#if 0
    opldev = m_check_parm("-opldev") > 0;
#endif
}
