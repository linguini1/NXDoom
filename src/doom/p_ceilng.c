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
// DESCRIPTION:  Ceiling aninmation (lowering, crushing, raising)
//

#include "doomdef.h"
#include "p_local.h"
#include "z_zone.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

// State.
#include "doomstat.h"
#include "r_state.h"

//
// CEILINGS
//

ceiling_t *activeceilings[MAXCEILINGS];

//
// T_MoveCeiling
//

void T_MoveCeiling(ceiling_t *ceiling)
{
  result_e res;

  switch (ceiling->direction)
    {
    case 0:
      // IN STASIS
      break;
    case 1:
      // UP
      res = T_MovePlane(ceiling->sector, ceiling->speed, ceiling->topheight,
                        false, 1, ceiling->direction);

      if (!(leveltime & 7))
        {
          switch (ceiling->type)
            {
            case CEIL_SILENTCRUSHANDRAISE:
              break;
            default:
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(&ceiling->sector->soundorg, SFX_STNMOV);
#endif
              // ?
              break;
            }
        }

      if (res == pastdest)
        {
          switch (ceiling->type)
            {
            case CEIL_RAISETOHIGHEST:
              P_RemoveActiveCeiling(ceiling);
              break;

            case CEIL_SILENTCRUSHANDRAISE:
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(&ceiling->sector->soundorg, SFX_PSTOP);
#endif
            case CEIL_FASTCRUSHANDRAISE:
            case CEIL_CRUSHANDRAISE:
              ceiling->direction = -1;
              break;

            default:
              break;
            }
        }
      break;

    case -1:
      // DOWN
      res =
          T_MovePlane(ceiling->sector, ceiling->speed, ceiling->bottomheight,
                      ceiling->crush, 1, ceiling->direction);

      if (!(leveltime & 7))
        {
          switch (ceiling->type)
            {
            case CEIL_SILENTCRUSHANDRAISE:
              break;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
            default:
              s_start_sound(&ceiling->sector->soundorg, SFX_STNMOV);
#else
            default:
              break;
#endif
            }
        }

      if (res == pastdest)
        {
          switch (ceiling->type)
            {
            case CEIL_SILENTCRUSHANDRAISE:
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(&ceiling->sector->soundorg, SFX_PSTOP);
#endif
            case CEIL_CRUSHANDRAISE:
              ceiling->speed = CEILSPEED;
            case CEIL_FASTCRUSHANDRAISE:
              ceiling->direction = 1;
              break;

            case CEIL_LOWERANDCRUSH:
            case CEIL_LOWERTOFLOOR:
              P_RemoveActiveCeiling(ceiling);
              break;

            default:
              break;
            }
        }
      else // ( res != pastdest )
        {
          if (res == crushed)
            {
              switch (ceiling->type)
                {
                case CEIL_SILENTCRUSHANDRAISE:
                case CEIL_CRUSHANDRAISE:
                case CEIL_LOWERANDCRUSH:
                  ceiling->speed = CEILSPEED / 8;
                  break;

                default:
                  break;
                }
            }
        }
      break;
    }
}

//
// ev_do_ceiling
// Move a ceiling up/down and all around!
//
int ev_do_ceiling(line_t *line, ceiling_e type)
{
  int secnum;
  int rtn;
  sector_t *sec;
  ceiling_t *ceiling;

  secnum = -1;
  rtn = 0;

  //	Reactivate in-stasis ceilings...for certain types.
  switch (type)
    {
    case CEIL_FASTCRUSHANDRAISE:
    case CEIL_SILENTCRUSHANDRAISE:
    case CEIL_CRUSHANDRAISE:
      P_ActivateInStasisCeiling(line);
    default:
      break;
    }

  while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
    {
      sec = &sectors[secnum];
      if (sec->specialdata) continue;

      // new door thinker
      rtn = 1;
      ceiling = z_malloc(sizeof(*ceiling), PU_LEVSPEC, 0);
      p_add_thinker(&ceiling->thinker);
      sec->specialdata = ceiling;
      ceiling->thinker.function.acp1 = (actionf_p1)T_MoveCeiling;
      ceiling->sector = sec;
      ceiling->crush = false;

      switch (type)
        {
        case CEIL_FASTCRUSHANDRAISE:
          ceiling->crush = true;
          ceiling->topheight = sec->ceilingheight;
          ceiling->bottomheight = sec->floorheight + (8 * FRACUNIT);
          ceiling->direction = -1;
          ceiling->speed = CEILSPEED * 2;
          break;

        case CEIL_SILENTCRUSHANDRAISE:
        case CEIL_CRUSHANDRAISE:
          ceiling->crush = true;
          ceiling->topheight = sec->ceilingheight;
        case CEIL_LOWERANDCRUSH:
        case CEIL_LOWERTOFLOOR:
          ceiling->bottomheight = sec->floorheight;
          if (type != CEIL_LOWERTOFLOOR) ceiling->bottomheight += 8 * FRACUNIT;
          ceiling->direction = -1;
          ceiling->speed = CEILSPEED;
          break;

        case CEIL_RAISETOHIGHEST:
          ceiling->topheight = P_FindHighestCeilingSurrounding(sec);
          ceiling->direction = 1;
          ceiling->speed = CEILSPEED;
          break;
        }

      ceiling->tag = sec->tag;
      ceiling->type = type;
      P_AddActiveCeiling(ceiling);
    }
  return rtn;
}

//
// Add an active ceiling
//
void P_AddActiveCeiling(ceiling_t *c)
{
  int i;

  for (i = 0; i < MAXCEILINGS; i++)
    {
      if (activeceilings[i] == NULL)
        {
          activeceilings[i] = c;
          return;
        }
    }
}

//
// Remove a ceiling's thinker
//
void P_RemoveActiveCeiling(ceiling_t *c)
{
  int i;

  for (i = 0; i < MAXCEILINGS; i++)
    {
      if (activeceilings[i] == c)
        {
          activeceilings[i]->sector->specialdata = NULL;
          p_remove_thinker(&activeceilings[i]->thinker);
          activeceilings[i] = NULL;
          break;
        }
    }
}

//
// Restart a ceiling that's in-stasis
//
void P_ActivateInStasisCeiling(line_t *line)
{
  int i;

  for (i = 0; i < MAXCEILINGS; i++)
    {
      if (activeceilings[i] && (activeceilings[i]->tag == line->tag) &&
          (activeceilings[i]->direction == 0))
        {
          activeceilings[i]->direction = activeceilings[i]->olddirection;
          activeceilings[i]->thinker.function.acp1 =
              (actionf_p1)T_MoveCeiling;
        }
    }
}

//
// EV_CeilingCrushStop
// Stop a ceiling from crushing!
//
int EV_CeilingCrushStop(line_t *line)
{
  int i;
  int rtn;

  rtn = 0;
  for (i = 0; i < MAXCEILINGS; i++)
    {
      if (activeceilings[i] && (activeceilings[i]->tag == line->tag) &&
          (activeceilings[i]->direction != 0))
        {
          activeceilings[i]->olddirection = activeceilings[i]->direction;
          activeceilings[i]->thinker.function.acv = (actionf_v)NULL;
          activeceilings[i]->direction = 0; // in-stasis
          rtn = 1;
        }
    }

  return rtn;
}
