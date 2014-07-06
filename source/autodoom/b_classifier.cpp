// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2013 Ioan Chera
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
// Additional terms and conditions compatible with the GPLv3 apply. See the
// file COPYING-EE for details.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//      Data classification and decision
//
//-----------------------------------------------------------------------------

#include <queue>
#include <unordered_set>
#include "../z_zone.h"

#include "../a_common.h"
#include "b_classifier.h"
#include "../e_args.h"
//#include "../e_hash.h"
#include "../e_inventory.h"
#include "../e_states.h"
#include "../m_collection.h"
#include "../p_mobj.h"

typedef std::unordered_set<statenum_t> StateSet;
typedef std::queue<statenum_t> StateQue;

void A_VileChase(actionargs_t *);
void A_CPosRefire(actionargs_t *actionargs);
void A_SkullAttack(actionargs_t *actionargs);
void A_SpidRefire(actionargs_t *);
void A_PainDie(actionargs_t *);
void A_KeenDie(actionargs_t *);
void A_RandomJump(actionargs_t *);      // killough 11/98

void A_SetFlags(actionargs_t *);
void A_UnSetFlags(actionargs_t *);
void A_GenRefire(actionargs_t *);
void A_KeepChasing(actionargs_t *);

void A_HealthJump(actionargs_t *);
void A_CounterJump(actionargs_t *);
void A_CounterSwitch(actionargs_t *);

void A_RandomWalk(actionargs_t *);
void A_TargetJump(actionargs_t *);
void A_CasingThrust(actionargs_t *);
void A_HticDrop(actionargs_t *);

void A_GenWizard(actionargs_t *);
void A_Sor2DthLoop(actionargs_t *);
void A_Srcr1Attack(actionargs_t *);
void A_Srcr2Decide(actionargs_t *);

void A_SorcererRise(actionargs_t *);

void A_MinotaurDecide(actionargs_t *);
void A_MinotaurAtk3(actionargs_t *);
void A_MinotaurCharge(actionargs_t *);

void A_WhirlwindSeek(actionargs_t *);
void A_LichFireGrow(actionargs_t *);
void A_ImpChargeAtk(actionargs_t *);

void A_ImpDeath(actionargs_t *);
void A_ImpXDeath1(actionargs_t *);
void A_ImpXDeath2(actionargs_t *);
void A_ImpExplode(actionargs_t *);

void A_JumpIfTargetInLOS(actionargs_t *);
void A_CheckPlayerDone(actionargs_t *);
void A_Jump(actionargs_t *);

void A_MissileAttack(actionargs_t *);
void A_MissileSpread(actionargs_t *);

void A_SnakeAttack(actionargs_t *);
void A_SnakeAttack2(actionargs_t *);

//
// B_getBranchingStateSeq
//
// Puts any secondary state sequence into the queue if action says something
//
static void B_getBranchingStateSeq(statenum_t sn,
                            StateQue &alterQueue,
                            const StateSet &stateSet,
                            Mobj &mo)
{
   const state_t &st = *states[sn];
   PODCollection<statenum_t> dests(17);
   
   const mobjinfo_t &mi = *mo.info;
   
   if(sn == NullStateNum)
      return;  // do nothing
      
   if(st.action == A_Look || st.action == A_CPosRefire
      || st.action == A_SpidRefire)
   {
      dests.add(mi.seestate);
   }
   else if(st.action == A_Chase || st.action == A_VileChase)
   {
      dests.add(mi.spawnstate);
      if(mi.meleestate != NullStateNum)
         dests.add(mi.meleestate);
      if(mi.missilestate != NullStateNum)
         dests.add(mi.missilestate);
      if(st.action == A_VileChase)
         dests.add(E_SafeState(S_VILE_HEAL1));
   }
   else if(st.action == A_SkullAttack)
      dests.add(mi.spawnstate);
   else if(st.action == A_RandomJump && st.misc2 > 0)
   {
      int rezstate = E_StateNumForDEHNum(st.misc1);
      if (rezstate >= 0)
         dests.add(rezstate);
   }
   else if(st.action == A_GenRefire)
   {
      if (E_ArgAsInt(st.args, 1, 0) > 0 || mo.flags & MF_FRIEND)
         dests.add(E_ArgAsStateNum(st.args, 0, &mo));
   }
   else if(st.action == A_HealthJump && mo.flags & MF_SHOOTABLE &&
           !(mo.flags2 & MF2_INVULNERABLE))
   {
      int statenum = E_ArgAsStateNumNI(st.args, 0, &mo);
      int checkhealth = E_ArgAsInt(st.args, 2, 0);
      
      if(statenum >= 0 && checkhealth < NUMMOBJCOUNTERS && checkhealth >= 0)
         dests.add(statenum);
   }
   else if(st.action == A_CounterJump)
   {
      // TODO: check if cnum has been touched or will be touched in such a way
      // to reach comparison with value. Only then accept the jump possibility
      int statenum  = E_ArgAsStateNumNI(st.args, 0, &mo);
      int cnum      = E_ArgAsInt(st.args, 3, 0);
      if(statenum >= 0 && cnum >= 0 && cnum < NUMMOBJCOUNTERS)
         dests.add(statenum);
   }
   else if(st.action == A_CounterSwitch)
   {
      int cnum       = E_ArgAsInt       (st.args, 0,  0);
      int startstate = E_ArgAsStateNumNI(st.args, 1, &mo);
      int numstates  = E_ArgAsInt       (st.args, 2,  0) - 1;

      if (startstate >= 0 && startstate + numstates < NUMSTATES && cnum >= 0 &&
          cnum < NUMMOBJCOUNTERS)
      {
         for (int i = 0; i < numstates; ++i)
            dests.add(startstate + i);
      }
   }
   else if(st.action == A_TargetJump)
   {
      int statenum = E_ArgAsStateNumNI(st.args, 0, &mo);
      if(statenum >= 0)
         dests.add(statenum);
   }
   else if(st.action == A_GenWizard)
   {
      dests.add(NullStateNum);
   }
   else if(st.action == A_Sor2DthLoop)
   {
      dests.add(E_SafeState(S_SOR2_DIE4));
   }
   else if(st.action == A_Srcr1Attack)
   {
      dests.add(E_SafeState(S_SRCR1_ATK4));
   }
   else if(st.action == A_Srcr2Decide)
   {
      dests.add(E_SafeState(S_SOR2_TELE1));
   }
   else if(st.action == A_MinotaurDecide)
   {
      dests.add(E_SafeState(S_MNTR_ATK4_1));
      dests.add(E_SafeState(S_MNTR_ATK3_1));
   }
   else if(st.action == A_MinotaurAtk3)
   {
      dests.add(E_SafeState(S_MNTR_ATK3_4));
   }
   else if(st.action == A_MinotaurCharge)
   {
      dests.add(mi.seestate);
      dests.add(mi.spawnstate);
   }
   else if(st.action == A_WhirlwindSeek)
   {
      dests.add(mi.deathstate);
   }
   else if(st.action == A_LichFireGrow)
   {
      dests.add(E_SafeState(S_LICHFX3_4));
   }
   else if(st.action == A_ImpChargeAtk)
   {
      dests.add(mi.seestate);
   }
   else if(st.action == A_ImpDeath || st.action == A_ImpXDeath2)
   {
      dests.add(mi.crashstate);
   }
   else if(st.action == A_ImpExplode)
   {
      dests.add(E_SafeState(S_IMP_XCRASH1));
   }
   else if(st.action == A_JumpIfTargetInLOS || st.action == A_CheckPlayerDone)
   {
      int statenum = E_ArgAsStateNumNI(st.args, 0, &mo);
      if(statenum >= 0)
         dests.add(statenum);
   }
   else if(st.action == A_Jump)
   {
      int chance = E_ArgAsInt(st.args, 0, 0);
      if(chance && st.args && st.args->numargs >= 2)
      {
         state_t *state;
         for(int i = 0; i < st.args->numargs; ++i)
         {
            state = E_ArgAsStateLabel(&mo, i);
            dests.add(state->index);
         }
      }
   }
   else if(st.action == A_MissileAttack || st.action == A_MissileSpread)
   {
      int statenum = E_ArgAsStateNumG0(st.args, 4, &mo);
      if(statenum >= 0 && statenum < NUMSTATES)
         dests.add(statenum);
   }
   else if(st.action == A_SnakeAttack || st.action == A_SnakeAttack2)
   {
      dests.add(mi.spawnstate);
   }
   
   // add the destinations
   for (auto it = dests.begin(); it != dests.end(); ++it)
   {
      if(!stateSet.count(*it))
         alterQueue.push(*it);
   }
}

//
// B_actionRemovesSolid
//
// Utility test if an action removes the solid flag
//
static bool B_actionRemovesSolid(void (*action)(actionargs_t *), statenum_t sn)
{
   if(action == A_Fall)
      return true;
   if(action == A_PainDie)
      return true;
   if(action == A_KeenDie)
      return true;
   if(action == A_HticDrop)
      return true;
   if(action == A_GenWizard)
      return true;
   if(action == A_SorcererRise)
      return true;
   if(action == A_ImpXDeath1)
      return true;
   if (action == A_SetFlags)
   {
      unsigned int *flags = E_ArgAsThingFlags(states[sn]->args, 1);
      if(!flags)
         return false;
      int flagfield = E_ArgAsInt(states[sn]->args, 0, 0);
      
      if (flagfield <= 1 && flags[0] & (MF_NOBLOCKMAP | MF_NOCLIP))
         return true;
      if ((!flagfield || flagfield == 2) && flags[1] & MF2_PUSHABLE)
         return true;
      
      return false;
   }
   if (action == A_UnSetFlags)
   {
      unsigned int *flags = E_ArgAsThingFlags(states[sn]->args, 1);
      if(!flags)
         return false;
      int flagfield = E_ArgAsInt(states[sn]->args, 0, 0);
      switch (flagfield)
      {
         case 0:
         case 1:
            if (flags[0] & (MF_SOLID))
               return true;
         default:
            return false;
      }
      return false;
   }
   return false;
}

//
// B_cantBeSolid
//
// Used as a callback function by mobj-solid-decor tester
//
static bool B_stateCantBeSolidDecor(statenum_t sn, const mobjinfo_t &mi)
{
   if (sn == NullStateNum) // null state: it dissipates
      return true;
   const state_t &st = *states[sn];
   if ((st.action == A_Chase || st.action == A_KeepChasing ||
        st.action == A_RandomWalk) &&
       mi.speed != 0)   // chase state with nonzero walk
   {
      return true;
   }
   if(st.action == A_CasingThrust)
   {
      fixed_t moml, momz;
      
      moml = E_ArgAsInt(st.args, 0, 0) * FRACUNIT / 16;
      momz = E_ArgAsInt(st.args, 1, 0) * FRACUNIT / 16;
      
      if (moml || momz)
         return true;
   }
   
   if (B_actionRemovesSolid(st.action, sn))
      return true;
   
   return false;
}

//
// B_stateEncounters
//
// True if state leads into a disappearance
//
static bool B_stateEncounters(statenum_t firstState,
                               Mobj &mo,
                              const std::function<bool(statenum_t,
                                                 const mobjinfo_t &)> &statecase)
{
   StateSet stateSet;  // set of visited states
   StateQue alterQueue;        // set of alternate chains
   // (RandomJump, Jump and so on)
   stateSet.rehash(47);

   statenum_t sn;
   alterQueue.push(firstState);
   if(mo.flags & MF_SHOOTABLE)
   {
      if(mo.info->painchance > 0)
         alterQueue.push(mo.info->painstate);
      alterQueue.push(mo.info->deathstate);
      if(mo.info->xdeathstate > 0)
         alterQueue.push(mo.info->xdeathstate);
   }
      
   while (alterQueue.size() > 0)
   {
      for(sn = alterQueue.front(), alterQueue.pop();
          ;
          sn = states[sn]->nextstate)
      {
         if (stateSet.count(sn))
         {
            // found a cycle
            break;
         }
         
         if (statecase(sn, *mo.info))
         {
            // got to state 0. This means it dissipates
            return true;
         }
         stateSet.insert(sn);
         
         
         B_getBranchingStateSeq(sn, alterQueue, stateSet, mo);
         if(states[sn]->tics < 0 || sn == NullStateNum)
            break;   // don't go to next state if current has neg. duration
      }
   }
   
   return false;
}

//
// B_IsMobjSolidDecor
//
// Checks if mobj is a permanent solid decoration (needed by the bot map)
//
bool B_IsMobjSolidDecor( Mobj &mo)
{
   if (!(mo.flags & MF_SOLID))
      return false;
   if (mo.flags & MF_SHOOTABLE && !(mo.flags2 & MF2_INVULNERABLE))
      return false;
   if (mo.flags & (MF_NOBLOCKMAP | MF_NOCLIP))
      return false;
   if (mo.flags2 & MF2_PUSHABLE)
      return false;
   
   const mobjinfo_t &mi = *mo.info;
   
   if (mi.spawnstate == NullStateNum)
      return false;  // has null start frame, invalid
   if (B_stateEncounters(mi.spawnstate, mo, B_stateCantBeSolidDecor))
      return false;  // state goes to null or disables solidity or moves
   
   return true;
}

//
// Copy constructor
//
PlayerStats::Values::Values(const Values &other) : health(other.health), 
   armorpoints(other.armorpoints), armortype(other.armortype),
   itemcount(other.itemcount), inv_size(other.inv_size)
{
   memcpy(powers, other.powers, sizeof(powers));
   memcpy(weaponowned, other.weaponowned, sizeof(weaponowned));
   inventory = estructalloc(inventoryslot_t, inv_size);
   memcpy(inventory, other.inventory, inv_size * sizeof(*inventory));
}

//
// Move constructor
//
PlayerStats::Values::Values(Values &&other) : health(other.health), 
   armorpoints(other.armorpoints), armortype(other.armortype),
   itemcount(other.itemcount), inventory(other.inventory), 
   inv_size(other.inv_size)
{
   memcpy(powers, other.powers, sizeof(powers));
   memcpy(weaponowned, other.weaponowned, sizeof(weaponowned));

   // move happens here
   other.inventory = nullptr;
   other.inv_size = 0;
}

//
// PlayerStats::Values::reset
//
// Resets the values
//
void PlayerStats::Values::reset(bool maxOut)
{
   int sup = maxOut ? INT_MAX : 0;
   fixed_t fsup = maxOut ? D_MAXINT : 0;
   health = sup;
   armorpoints = sup;
   armortype = fsup;
   itemcount = sup;

   int i;
   for(i = 0; i < NUMPOWERS; ++i)
      powers[i] = sup;
   for(i = 0; i < NUMWEAPONS; ++i)
      weaponowned[i] = sup;
   inventory = estructalloc(inventoryslot_t, inv_size = e_maxitemid);
   for(i = 0; i < e_maxitemid; ++i)
   {
      inventory[i].item = i;
      inventory[i].amount = sup;
   }
}

//
// PlayerStats::reduceByCurrentState
//
// Reduces the stored values to the minimum of player's current
//
void PlayerStats::reduceByCurrentState(const player_t &pl)
{
   if(pl.health < data.health)
      data.health = pl.health;
   if(pl.armorpoints < data.armorpoints)
      data.armorpoints = pl.armorpoints;
   fixed_t plarmor = pl.armordivisor ? FRACUNIT * pl.armorfactor / pl.armordivisor :
      0;
   if(plarmor < data.armortype)
      data.armortype = plarmor;
   int i;
   int plpowers;
   for(i = 0; i < NUMPOWERS; ++i)
   {
      plpowers = pl.powers[i] == -1 ? INT_MAX : pl.powers[i];
      if(plpowers < data.powers[i])
         data.powers[i] = plpowers;
   }
   for(i = 0; i < NUMWEAPONS; ++i)
   {
      if(pl.weaponowned[i] < data.weaponowned[i])
         data.weaponowned[i] = pl.weaponowned[i];
   }
   if(pl.itemcount < data.itemcount)
      data.itemcount = pl.itemcount;
   for(i = 0; i < e_maxitemid; ++i)
   {
      if(pl.inventory[i].item != -1 && pl.inventory[i].amount < 
         data.inventory[pl.inventory[i].item].amount)
      {
         data.inventory[pl.inventory[i].item].amount = pl.inventory[i].amount;
      }
   }
}

//
// PlayerStats::getPriorState
//
// Gets the current player state in the 'prior' struct member
//
void PlayerStats::setPriorState(const player_t &pl)
{
   prior.health = pl.health;
   prior.armorpoints = pl.armorpoints;
   prior.armortype = pl.armordivisor ? FRACUNIT * pl.armorfactor / pl.armordivisor :
      0;
   int i;
   for(i = 0; i < NUMPOWERS; ++i)
      prior.powers[i] = pl.powers[i];
   for(i = 0; i < NUMWEAPONS; ++i)
      prior.weaponowned[i] = pl.weaponowned[i];
   prior.itemcount = pl.itemcount;
   for(i = 0; i < e_maxitemid; ++i)
   {
      if(pl.inventory[i].item != -1)
         prior.inventory[pl.inventory[i].item].amount = pl.inventory[i].amount;
   }
}

//
// PlayerStats::maximizeByStateDelta
//
// Increases the gain
//
void PlayerStats::maximizeByStateDelta(const player_t &pl)
{
   int delta;
   fixed_t fdelta;
   delta = pl.health - prior.health;
   if(delta > data.health)
      data.health = delta;
   delta = pl.armorpoints - prior.armorpoints;
   if(delta > data.armorpoints)
      data.armorpoints = delta;
   fixed_t plarmor = pl.armordivisor ? FRACUNIT * pl.armorfactor / pl.armordivisor :
      0;
   fdelta = plarmor - prior.armortype;
   if(fdelta > data.armortype)
      data.armortype = fdelta;
   int i, plpowers;
   for(i = 0; i < NUMPOWERS; ++i)
   {
      plpowers = pl.powers[i] == -1 ? INT_MAX : pl.powers[i];
      delta = plpowers - prior.powers[i];
      if(delta > data.powers[i])
         data.powers[i] = delta;
   }
   for(i = 0; i < NUMWEAPONS; ++i)
   {
      delta = pl.weaponowned[i] - prior.weaponowned[i];
      if(delta > data.weaponowned[i])
         data.weaponowned[i] = delta;
   }
   delta = pl.itemcount - prior.itemcount;
   if(delta > data.itemcount)
      data.itemcount = delta;
   for(i = 0; i < e_maxitemid; ++i)
   {
      if(pl.inventory[i].item != -1)
      {
         delta = pl.inventory[i].amount - prior.inventory[pl.inventory[i].item].amount;
         if(delta > data.inventory[pl.inventory[i].item].amount)
            data.inventory[pl.inventory[i].item].amount = delta;
      }
   }
}

//
// PlayerStats::greaterThan
//
// Returns true if one of the stored values is greater than its equivalent
// player value. Used to lower the minimum required stats for items
//
bool PlayerStats::greaterThan(player_t &pl) const
{
   fixed_t plarmor = pl.armordivisor ? FRACUNIT * pl.armorfactor / 
      pl.armordivisor : 0;

   int i;
   for(i = 0; i < NUMPOWERS; ++i)
      if(pl.powers[i] != -1 && data.powers[i] > pl.powers[i])
         return true;
   for(i = 0; i < NUMWEAPONS; ++i)
      if(data.weaponowned[i] > pl.weaponowned[i])
         return true;

   inventoryslot_t *islot;
   for(i = 0; i < e_maxitemid; ++i)
   {
      islot = E_InventorySlotForItemID(&pl, i);
      if((!islot && data.inventory[i].amount > 0) || 
         data.inventory[i].amount > islot->amount)
         return true;
   }
   return data.health > pl.health || data.armorpoints > pl.armorpoints ||
      data.armortype > plarmor || data.itemcount > pl.itemcount;
}

//
// PlayerStats::fillsGap
//
// Returns true if player pl would benefit from this additive PlayerStats
//
bool PlayerStats::fillsGap(player_t &pl, const PlayerStats &cap) const 
{
   fixed_t plarmor = pl.armordivisor ? FRACUNIT * pl.armorfactor / 
      pl.armordivisor : 0;

   if(data.health && pl.health < cap.data.health)
      return true;
   if(data.armorpoints && pl.armorpoints < cap.data.armorpoints)
      return true;
   if(data.armortype && plarmor < cap.data.armortype)
      return true;
   int i;
   int plpower;
   for(i = 0; i < NUMPOWERS; ++i)
   {
      plpower = pl.powers[i] == -1 ? INT_MAX : pl.powers[i];
      if(data.powers[i] && plpower < cap.data.powers[i])
         return true;
   }

   for(i = 0; i < NUMWEAPONS; ++i)
      if(data.weaponowned[i] && pl.weaponowned[i] < cap.data.weaponowned[i])
         return true;

   if(data.itemcount && pl.itemcount < cap.data.itemcount)
      return true;

   inventoryslot_t *islot;
   for(i = 0; i < e_maxitemid; ++i)
   {
      islot = E_InventorySlotForItemID(&pl, i);
      if(data.inventory[i].amount && 
         (!islot || islot->amount < cap.data.inventory[i].amount))
         return true;
   }

   return false;
}

//
// PlayerStats::overlaps
//
// Pretty much the reverse of the operation above.
//
bool PlayerStats::overlaps(player_t &pl, const PlayerStats &cap) const 
{
   fixed_t plarmor = pl.armordivisor ? FRACUNIT * pl.armorfactor / 
      pl.armordivisor : 0;

   if(data.health && pl.health > cap.data.health)
      return true;
   if(data.armorpoints && pl.armorpoints > cap.data.armorpoints)
      return true;
   if(data.armortype && plarmor > cap.data.armortype)
      return true;
   int i;
   int plpower;
   for(i = 0; i < NUMPOWERS; ++i)
   {
      plpower = pl.powers[i] == -1 ? INT_MAX : pl.powers[i];
      if(data.powers[i] && plpower > cap.data.powers[i])
         return true;
   }

   for(i = 0; i < NUMWEAPONS; ++i)
      if(data.weaponowned[i] && pl.weaponowned[i] > cap.data.weaponowned[i])
         return true;

   if(data.itemcount && pl.itemcount > cap.data.itemcount)
      return true;

   inventoryslot_t *islot;
   for(i = 0; i < e_maxitemid; ++i)
   {
      islot = E_InventorySlotForItemID(&pl, i);
      if(islot && islot->amount > cap.data.inventory[i].amount)
         return true;
   }

   return false;
}

// EOF
