//
// The Eternity Engine
// Copyright(C) 2017 James Haley, Max Waine, et al.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//----------------------------------------------------------------------------
//
// Purpose: Heretic weapon action functions
// Some code is derived from Chocolate Doom, by Simon Howard, used under
// terms of the GPLv3.
//
// Authors: Max Waine
//

#include "z_zone.h"

#include "a_args.h"
#include "doomstat.h"
#include "d_player.h"
#include "e_args.h"
#include "e_things.h"
#include "e_ttypes.h"
#include "m_random.h"
#include "p_mobj.h"
#include "r_main.h"
#include "s_sound.h"
#include "tables.h"

#include "p_map.h"

#define WEAPONTOP    (FRACUNIT*32)

void A_StaffAttackPL1(actionargs_t *actionargs)
{
   player_t *player = actionargs->actor->player;
   int       damage = 5 + (P_Random(pr_staff) & 15);
   angle_t   angle  = player->mo->angle + (P_SubRandom(pr_staffangle) << 18);
   fixed_t   slope  =  P_AimLineAttack(player->mo, angle, MELEERANGE, 0);

   const int   tnum = E_SafeThingType(MT_STAFFPUFF);
   mobjinfo_t *puff = mobjinfo[tnum];

   P_LineAttack(player->mo, angle, MELEERANGE, slope, damage, puff);
   if(clip.linetarget)
   {
      //S_StartSound(player->mo, sfx_stfhit);
      // turn to face target
      player->mo->angle = R_PointToAngle2(player->mo->x,
                                          player->mo->y, clip.linetarget->x,
                                          clip.linetarget->y);
   }
}

void A_StaffAttackPL2(actionargs_t *actionargs)
{
   player_t *player = actionargs->actor->player;
   int       damage = 18 + (P_Random(pr_staff2) & 63);
   angle_t   angle = player->mo->angle + (P_SubRandom(pr_staffangle) << 18);
   fixed_t   slope = P_AimLineAttack(player->mo, angle, MELEERANGE, 0);

   const int   tnum = E_SafeThingType(MT_STAFFPUFF2);
   mobjinfo_t *puff = mobjinfo[tnum];

   P_LineAttack(player->mo, angle, MELEERANGE, slope, damage, puff);
   if(clip.linetarget)
   {
      //S_StartSound(player->mo, sfx_stfhit);
      // turn to face target
      player->mo->angle = R_PointToAngle2(player->mo->x,
                                          player->mo->y, clip.linetarget->x,
                                          clip.linetarget->y);
   }
}

void A_FireGoldWandPL1(actionargs_t *actionargs)
{
   Mobj     *mo     = actionargs->actor;
   player_t *player = mo->player;
   angle_t   angle  = mo->angle;
   const int damage = 7 + (P_Random(pr_goldwand) & 7);;

   P_SubtractAmmo(player, -1);
   P_BulletSlope(mo);
   if(player->refire)
      angle += P_SubRandom(pr_goldwand) << 18;

   const int    tnum = E_SafeThingType(MT_GOLDWANDPUFF1);
   mobjinfo_t  *puff = mobjinfo[tnum];

   P_LineAttack(mo, angle, MISSILERANGE, bulletslope, damage, puff);
   P_WeaponSound(mo, sfx_gldhit);
}

void A_FireGoldWandPL2(actionargs_t *actionargs)
{
   Mobj     *mo     = actionargs->actor;
   player_t *player = mo->player;
   angle_t   angle;
   fixed_t momz, z;
   int damage, i;

   z  = mo->z + 32 * FRACUNIT;
   mo = player->mo;

   P_SubtractAmmo(player, -1);
   P_BulletSlope(mo);

   const int   tnum = E_SafeThingType(MT_GOLDWANDFX2);
   mobjinfo_t *fx   = mobjinfo[tnum];

   momz = FixedMul(fx->speed, bulletslope);
   P_SpawnMissileAngle(mo, tnum, mo->angle - (ANG45 / 8), momz, z);
   P_SpawnMissileAngle(mo, tnum, mo->angle + (ANG45 / 8), momz, z);
   angle = mo->angle - (ANG45 / 8);

   for(i = 0; i < 5; i++)
   {
      const int    pnum = E_SafeThingType(MT_GOLDWANDPUFF2);
      mobjinfo_t  *puff = mobjinfo[pnum];
      damage = 1 + (P_Random(pr_goldwand2) & 7);
      P_LineAttack(mo, angle, MISSILERANGE, bulletslope, damage, puff);
      angle += ((ANG45 / 8) * 2) / 4;
   }

   P_WeaponSound(mo, sfx_gldhit);
}

void A_FireMacePL1B(actionargs_t *actionargs)
{
   player_t *player = actionargs->actor->player;
   Mobj     *pmo    = player->mo;
   Mobj     *ball;
   angle_t   angle;

   if(!P_CheckAmmo(player))
      return;

   P_SubtractAmmo(player, -1);
   pmo = player->mo;

   // In vanilla this is bugged: 
   // Original code here looks like: the footclip check turns into:
   //   (pmo->flags2 & 1)
   // due to C operator precedence and a lack of parens/brackets.
   const fixed_t z = comp[comp_terrain] || !(pmo->flags2 & MF2_FOOTCLIP) ?
                     pmo->z + 28 * FRACUNIT : pmo->z + 28 * FRACUNIT - pmo->floorclip ;
   ball = P_SpawnMobj(pmo->x, pmo->y, z, E_SafeThingType(MT_MACEFX2));

   const int   tnum = E_SafeThingType(MT_MACEFX2);
   mobjinfo_t *fx = mobjinfo[tnum];

   const fixed_t slope = P_PlayerPitchSlope(player);
   ball->momz = FixedMul(fx->speed, slope) + (2 * FRACUNIT);
   angle = pmo->angle;
   ball->target = pmo;
   ball->angle = angle;
   ball->z += 2 * slope;
   angle >>= ANGLETOFINESHIFT;
   ball->momx = (pmo->momx >> 1) + FixedMul(ball->info->speed, finecosine[angle]);
   ball->momy = (pmo->momy >> 1) + FixedMul(ball->info->speed, finesine[angle]);

   S_StartSound(ball, sfx_lobsht);
   P_CheckMissileSpawn(ball);
}

void A_FireMacePL1(actionargs_t *actionargs)
{
   player_t *player = actionargs->actor->player;
   pspdef_t *psp = actionargs->pspr;
   Mobj     *ball;

   if(!psp)
      return;

   if(P_Random(pr_firemace) < 28)
   {
      A_FireMacePL1B(actionargs);
      return;
   }
   if(!P_CheckAmmo(player))
      return;
   
   const int tnum = E_SafeThingType(MT_MACEFX1);

   P_SubtractAmmo(player, -1);
   psp->sx = ((P_Random(pr_firemace) & 3) - 2) * FRACUNIT;
   psp->sy = WEAPONTOP + (P_Random(pr_firemace) & 3) * FRACUNIT;
   ball = P_SpawnMissileAngleHeretic(player->mo, tnum, player->mo->angle
                                     + (((P_Random(pr_firemace) & 7) - 4) << 24));
   if(ball)
      ball->counters[0] = 16;    // tics till dropoff
}

void A_MacePL1Check(actionargs_t *actionargs)
{
   Mobj   *ball = actionargs->actor;
   angle_t angle;

   if(ball->counters[0] == 0)
      return;
   ball->counters[0] -= 4;
   if(ball->counters[0] > 0)
      return;
   ball->counters[0] = 0;
   ball->flags2 |= MF2_LOGRAV;
   angle = ball->angle >> ANGLETOFINESHIFT;
   ball->momx = FixedMul(7 * FRACUNIT, finecosine[angle]);
   ball->momy = FixedMul(7 * FRACUNIT, finesine[angle]);
   ball->momz -= ball->momz >> 1;
}

#define MAGIC_JUNK 1234
void A_MaceBallImpact(actionargs_t *actionargs)
{
   Mobj   *ball = actionargs->actor;
   if((ball->z <= ball->floorz) && E_HitFloor(ball))
   {                           // Landed in some sort of liquid
      ball->removeThinker();
      return;
   }
   if((ball->health != MAGIC_JUNK) && (ball->z <= ball->floorz)
      && ball->momz)
   {                           // Bounce
      ball->health = MAGIC_JUNK;
      ball->momz = (ball->momz * 192) >> 8;
      ball->flags4 &= ~MF4_HERETICBOUNCES;
      P_SetMobjState(ball, ball->info->spawnstate);
      S_StartSound(ball, sfx_bounce);
   }
   else
   {                           // Explode
      ball->flags |= MF_NOGRAVITY;
      ball->flags2 &= ~MF2_LOGRAV;
      S_StartSound(ball, sfx_lobhit);
   }
}

void A_MaceBallImpact2(actionargs_t *actionargs)
{
   Mobj     *ball = actionargs->actor;
   Mobj     *tiny;
   angle_t   angle;
   const int tnum = E_SafeThingType(MT_MACEFX3);

   if((ball->z <= ball->floorz) && E_HitFloor(ball))
   {                           // Landed in some sort of liquid
      ball->removeThinker();
      return;
   }
   if((ball->z != ball->floorz) || (ball->momz < 2 * FRACUNIT))
   {                           // Explode
      ball->momx = ball->momy = ball->momz = 0;
      ball->flags |= MF_NOGRAVITY;
      ball->flags2 &= ~(MF2_LOGRAV | MF4_HERETICBOUNCES);
   }
   else
   {                           // Bounce
      ball->momz = (ball->momz * 192) >> 8;
      P_SetMobjState(ball, ball->info->spawnstate);

      tiny = P_SpawnMobj(ball->x, ball->y, ball->z, tnum);
      angle = ball->angle + ANG90;
      tiny->target = ball->target;
      tiny->angle = angle;
      angle >>= ANGLETOFINESHIFT;
      tiny->momx = (ball->momx >> 1) + FixedMul(ball->momz - FRACUNIT,
         finecosine[angle]);
      tiny->momy = (ball->momy >> 1) + FixedMul(ball->momz - FRACUNIT,
         finesine[angle]);
      tiny->momz = ball->momz;
      P_CheckMissileSpawn(tiny);

      tiny = P_SpawnMobj(ball->x, ball->y, ball->z, tnum);
      angle = ball->angle - ANG90;
      tiny->target = ball->target;
      tiny->angle = angle;
      angle >>= ANGLETOFINESHIFT;
      tiny->momx = (ball->momx >> 1) + FixedMul(ball->momz - FRACUNIT,
         finecosine[angle]);
      tiny->momy = (ball->momy >> 1) + FixedMul(ball->momz - FRACUNIT,
         finesine[angle]);
      tiny->momz = ball->momz;
      P_CheckMissileSpawn(tiny);
   }
}

void A_FireCrossbowPL1(actionargs_t *actionargs)
{
   Mobj     *pmo    = actionargs->actor;
   player_t *player = pmo->player;
   const int tnum   = E_SafeThingType(MT_CRBOWFX3);

   P_SubtractAmmo(player, -1);
   P_SpawnPlayerMissile(pmo, E_SafeThingType(MT_CRBOWFX1));
   P_SpawnMissileAngleHeretic(pmo, tnum, pmo->angle - (ANG45 / 10));
   P_SpawnMissileAngleHeretic(pmo, tnum, pmo->angle + (ANG45 / 10));
}

void A_FireCrossbowPL2(actionargs_t *actionargs)
{
   Mobj     *pmo    = actionargs->actor;
   player_t *player = pmo->player;
   const int tnum2  = E_SafeThingType(MT_CRBOWFX2);
   const int tnum3  = E_SafeThingType(MT_CRBOWFX3);

   P_SubtractAmmo(player, -1);
   P_SpawnPlayerMissile(pmo, tnum2);
   P_SpawnMissileAngleHeretic(pmo, tnum2, pmo->angle - (ANG45 / 10));
   P_SpawnMissileAngleHeretic(pmo, tnum2, pmo->angle + (ANG45 / 10));
   P_SpawnMissileAngleHeretic(pmo, tnum3, pmo->angle - (ANG45 / 5));
   P_SpawnMissileAngleHeretic(pmo, tnum3, pmo->angle + (ANG45 / 5));
}

void A_BoltSpark(actionargs_t *actionargs)
{
   Mobj     *bolt = actionargs->actor;
   const int tnum = E_SafeThingType(MT_CRBOWFX4);

   if(P_Random(pr_boltspark) > 50)
   {
      Mobj *spark = P_SpawnMobj(bolt->x, bolt->y, bolt->z, tnum);
      spark->x += P_SubRandom(pr_boltspark) << 10;
      spark->y += P_SubRandom(pr_boltspark) << 10;
   }
}

void A_FireBlasterPL1(actionargs_t *actionargs)
{
   player_t *player = actionargs->actor->player;
   Mobj     *mo     = player->mo;
   angle_t   angle;
   int       damage;


   P_WeaponSound(mo, sfx_gldhit);
   P_SubtractAmmo(player, -1);
   P_BulletSlope(mo);
   damage = (1 + (P_Random(pr_blaster) & 7)) * 4;
   angle  = mo->angle;
   if(player->refire)
      angle += P_SubRandom(pr_blaster) << 18;

   const int    tnum = E_SafeThingType(MT_BLASTERPUFF1);
   mobjinfo_t  *puff = mobjinfo[tnum];

   P_LineAttack(mo, angle, MISSILERANGE, bulletslope, damage, puff);
   P_WeaponSound(mo, sfx_blssht);
}

void A_FireSkullRodPL1(actionargs_t *actionargs)
{
   player_t *player = actionargs->actor->player;

   if(!P_CheckAmmo(player))
      return;

   P_SubtractAmmo(player, -1);
   const int tnum = E_SafeThingType(MT_HORNRODFX1);
   Mobj     *mo   = P_SpawnPlayerMissile(player->mo, tnum);
   // Randomize the first frame
   if(mo && P_Random(pr_skullrod) > 128)
      P_SetMobjState(mo, mo->state->nextstate);
}

void A_FirePhoenixPL1(actionargs_t *actionargs)
{
   player_t *player = actionargs->actor->player;
   const int tnum   = E_SafeThingType(MT_PHOENIXFX1);
   angle_t   angle;

   P_SubtractAmmo(player, -1);
   P_SpawnPlayerMissile(player->mo, tnum);
   // TODO: This is from Choco, why is it commented out?
   //P_SpawnPlayerMissile(player->mo, MT_MNTRFX2);
   angle = player->mo->angle + ANG180;
   angle >>= ANGLETOFINESHIFT;
   player->mo->momx += FixedMul(4 * FRACUNIT, finecosine[angle]);
   player->mo->momy += FixedMul(4 * FRACUNIT, finesine[angle]);
}

void A_GauntletAttack(actionargs_t *actionargs)
{
   arglist_t  *args   = actionargs->args;
   player_t   *player = actionargs->actor->player;
   Mobj       *mo     = player->mo;
   pspdef_t   *psp    = actionargs->pspr;
   fixed_t     dist;
   angle_t     angle;
   mobjinfo_t *puff;
   int damage, slope, randVal, tnum;

   if(!psp)
      return;

   int powered = E_ArgAsInt(args, 0, 0);

   psp->sx = ((P_Random(pr_gauntlets) & 3) - 2) * FRACUNIT;
   psp->sy = WEAPONTOP + (P_Random(pr_gauntlets) & 3) * FRACUNIT;
   angle = player->mo->angle;

   if(powered)
   {
      damage = (1 + (P_Random(pr_gauntlets) & 7))* 2;
      dist = 4 * MELEERANGE;
      angle += P_SubRandom(pr_gauntletsangle) << 17;
      tnum = E_SafeThingType(MT_GAUNTLETPUFF2);
   }
   else
   {
      damage = (1 + (P_Random(pr_gauntlets) & 7)) * 2;
      dist = MELEERANGE + 1;
      angle += P_SubRandom(pr_gauntletsangle) << 18;
      tnum = E_SafeThingType(MT_GAUNTLETPUFF1);
   }

   puff  = mobjinfo[tnum];
   slope = P_AimLineAttack(player->mo, angle, dist, 0);
   P_LineAttack(player->mo, angle, dist, slope, damage, puff);

   if(!clip.linetarget)
   {
      if(P_Random(pr_gauntlets) > 64) // TODO: Maybe don't use pr_gauntlets?
         player->extralight = !player->extralight;
      P_WeaponSound(mo, sfx_gntful);
      return;
   }

   randVal = P_Random(pr_gauntlets); // TODO: Maybe don't use pr_gauntlets?
   if(randVal < 64)
      player->extralight = 0;
   else if(randVal < 160)
      player->extralight = 1;
   else
      player->extralight = 2;

   if(powered)
   {
      // FIXME: This needs to do damage vamp
      //P_GiveBody(player, damage >> 1);
      P_WeaponSound(mo, sfx_gntpow);
   }
   else
      P_WeaponSound(mo, sfx_gnthit);

   // turn to face target
   angle = R_PointToAngle2(player->mo->x, player->mo->y,
                           clip.linetarget->x, clip.linetarget->y);

   if(angle - player->mo->angle > ANG180)
   {
      if(angle - player->mo->angle < -ANG90 / 20)
         player->mo->angle = angle + ANG90 / 21;
      else
         player->mo->angle -= ANG90 / 20;
   }
   else
   {
      if(angle - player->mo->angle > ANG90 / 20)
         player->mo->angle = angle - ANG90 / 21;
      else
         player->mo->angle += ANG90 / 20;
   }

   player->mo->flags |= MF_JUSTATTACKED;
}

// EOF
