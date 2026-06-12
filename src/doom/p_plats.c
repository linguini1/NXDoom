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
//	Plats (i.e. elevator platforms) code, raising/lowering.
//

#include <stdio.h>

#include "i_system.h"
#include "m_random.h"
#include "z_zone.h"

#include "doomdef.h"
#include "p_local.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

// State.
#include "doomstat.h"
#include "r_state.h"

plat_t *activeplats[MAXPLATS];

//
// Move a plat up and down
//
void T_PlatRaise(plat_t *plat)
{
  result_e res;

  switch (plat->status)
    {
    case up:
      res = T_MovePlane(plat->sector, plat->speed, plat->high, plat->crush, 0,
                        1);

#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (plat->type == PLAT_RAISEANDCHANGE ||
          plat->type == PLAT_RAISETONEARESTANDCHANGE)
        {
          if (!(leveltime & 7))
            s_start_sound(&plat->sector->soundorg, SFX_STNMOV);
        }
#endif

      if (res == crushed && (!plat->crush))
        {
          plat->count = plat->wait;
          plat->status = down;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(&plat->sector->soundorg, SFX_PSTART);
#endif
        }
      else
        {
          if (res == pastdest)
            {
              plat->count = plat->wait;
              plat->status = waiting;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(&plat->sector->soundorg, SFX_PSTOP);
#endif

              switch (plat->type)
                {
                case PLAT_BLAZEDWUS:
                case PLAT_DOWNWAITUPSTAY:
                  P_RemoveActivePlat(plat);
                  break;

                case PLAT_RAISEANDCHANGE:
                case PLAT_RAISETONEARESTANDCHANGE:
                  P_RemoveActivePlat(plat);
                  break;

                default:
                  break;
                }
            }
        }
      break;

    case down:
      res = T_MovePlane(plat->sector, plat->speed, plat->low, false, 0, -1);

      if (res == pastdest)
        {
          plat->count = plat->wait;
          plat->status = waiting;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(&plat->sector->soundorg, SFX_PSTOP);
#endif
        }
      break;

    case waiting:
      if (!--plat->count)
        {
          if (plat->sector->floorheight == plat->low)
            plat->status = up;
          else
            plat->status = down;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(&plat->sector->soundorg, SFX_PSTART);
#endif
        }
    case in_stasis:
      break;
    }
}

//
// Do Platforms
//  "amount" is only used for SOME platforms.
//
int ev_do_plat(line_t *line, plattype_e type, int amount)
{
  plat_t *plat;
  int secnum;
  int rtn;
  sector_t *sec;

  secnum = -1;
  rtn = 0;

  //	Activate all <type> plats that are in_stasis
  switch (type)
    {
    case PLAT_PERPETUALRAISE:
      P_ActivateInStasis(line->tag);
      break;

    default:
      break;
    }

  while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
    {
      sec = &sectors[secnum];

      if (sec->specialdata) continue;

      // Find lowest & highest floors around sector
      rtn = 1;
      plat = z_malloc(sizeof(*plat), PU_LEVSPEC, 0);
      p_add_thinker(&plat->thinker);

      plat->type = type;
      plat->sector = sec;
      plat->sector->specialdata = plat;
      plat->thinker.function.acp1 = (actionf_p1)T_PlatRaise;
      plat->crush = false;
      plat->tag = line->tag;

      switch (type)
        {
        case PLAT_RAISETONEARESTANDCHANGE:
          plat->speed = PLATSPEED / 2;
          sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
          plat->high = P_FindNextHighestFloor(sec, sec->floorheight);
          plat->wait = 0;
          plat->status = up;
          // NO MORE DAMAGE, IF APPLICABLE
          sec->special = 0;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(&sec->soundorg, SFX_STNMOV);
#endif
          break;

        case PLAT_RAISEANDCHANGE:
          plat->speed = PLATSPEED / 2;
          sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
          plat->high = sec->floorheight + amount * FRACUNIT;
          plat->wait = 0;
          plat->status = up;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(&sec->soundorg, SFX_STNMOV);
#endif
          break;

        case PLAT_DOWNWAITUPSTAY:
          plat->speed = PLATSPEED * 4;
          plat->low = P_FindLowestFloorSurrounding(sec);

          if (plat->low > sec->floorheight) plat->low = sec->floorheight;

          plat->high = sec->floorheight;
          plat->wait = TICRATE * PLATWAIT;
          plat->status = down;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(&sec->soundorg, SFX_PSTART);
#endif
          break;

        case PLAT_BLAZEDWUS:
          plat->speed = PLATSPEED * 8;
          plat->low = P_FindLowestFloorSurrounding(sec);

          if (plat->low > sec->floorheight) plat->low = sec->floorheight;

          plat->high = sec->floorheight;
          plat->wait = TICRATE * PLATWAIT;
          plat->status = down;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(&sec->soundorg, SFX_PSTART);
#endif
          break;

        case PLAT_PERPETUALRAISE:
          plat->speed = PLATSPEED;
          plat->low = P_FindLowestFloorSurrounding(sec);

          if (plat->low > sec->floorheight) plat->low = sec->floorheight;

          plat->high = P_FindHighestFloorSurrounding(sec);

          if (plat->high < sec->floorheight) plat->high = sec->floorheight;

          plat->wait = TICRATE * PLATWAIT;
          plat->status = p_random() & 1;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(&sec->soundorg, SFX_PSTART);
#endif
          break;
        }
      P_AddActivePlat(plat);
    }
  return rtn;
}

void P_ActivateInStasis(int tag)
{
  int i;

  for (i = 0; i < MAXPLATS; i++)
    if (activeplats[i] && (activeplats[i])->tag == tag &&
        (activeplats[i])->status == in_stasis)
      {
        (activeplats[i])->status = (activeplats[i])->oldstatus;
        (activeplats[i])->thinker.function.acp1 = (actionf_p1)T_PlatRaise;
      }
}

void EV_StopPlat(line_t *line)
{
  int j;

  for (j = 0; j < MAXPLATS; j++)
    if (activeplats[j] && ((activeplats[j])->status != in_stasis) &&
        ((activeplats[j])->tag == line->tag))
      {
        (activeplats[j])->oldstatus = (activeplats[j])->status;
        (activeplats[j])->status = in_stasis;
        (activeplats[j])->thinker.function.acv = (actionf_v)NULL;
      }
}

void P_AddActivePlat(plat_t *plat)
{
  int i;

  for (i = 0; i < MAXPLATS; i++)
    if (activeplats[i] == NULL)
      {
        activeplats[i] = plat;
        return;
      }
  i_error("P_AddActivePlat: no more plats!");
}

void P_RemoveActivePlat(plat_t *plat)
{
  int i;
  for (i = 0; i < MAXPLATS; i++)
    if (plat == activeplats[i])
      {
        (activeplats[i])->sector->specialdata = NULL;
        p_remove_thinker(&(activeplats[i])->thinker);
        activeplats[i] = NULL;

        return;
      }
  i_error("P_RemoveActivePlat: can't find plat!");
}
