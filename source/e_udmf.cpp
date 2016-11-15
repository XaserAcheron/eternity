// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright (C) 2015 Ioan Chera et al.
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
//----------------------------------------------------------------------------
//
// Universal Doom Map Format, for Eternity
//
//----------------------------------------------------------------------------

#include "z_zone.h"

#include "doomstat.h"
#include "e_exdata.h"
#include "e_hash.h"
#include "e_mod.h"
#include "e_ttypes.h"
#include "e_udmf.h"
#include "m_compare.h"
#include "p_setup.h"
#include "p_spec.h"
#include "r_data.h"
#include "r_defs.h"
#include "r_main.h" // Needed for PI
#include "r_portal.h" // Needed for portalflags
#include "r_state.h"
#include "w_wad.h"
#include "z_auto.h"

// SOME CONSTANTS
static const char DEFAULT_default[] = "@default";
static const char DEFAULT_flat[] = "@flat";

static const char RENDERSTYLE_translucent[] = "translucent";
static const char RENDERSTYLE_add[] = "add";

//
// Initializes the internal structure with the sector count
//
void UDMFSetupSettings::useSectorCount()
{
   if(mSectorInitData)
      return;
   mSectorInitData = estructalloc(sectorinfo_t, ::numsectors);
}

void UDMFSetupSettings::useLineCount()
{
   if (mLineInitData)
      return;
   mLineInitData = estructalloc(lineinfo_t, ::numlines);
}

//==============================================================================
//
// Collecting and processing
//
//==============================================================================

//
// Reads the raw vertices and obtains final ones
//
void UDMFParser::loadVertices() const
{
   numvertexes = (int)mVertices.getLength();
   vertexes = estructalloctag(vertex_t, numvertexes, PU_LEVEL);
   for(int i = 0; i < numvertexes; i++)
   {
      vertexes[i].x = mVertices[i].x;
      vertexes[i].y = mVertices[i].y;
      // SoM: Cardboard stores float versions of vertices.
      vertexes[i].fx = M_FixedToFloat(vertexes[i].x);
      vertexes[i].fy = M_FixedToFloat(vertexes[i].y);
   }
}

//
// Loads sectors
//
void UDMFParser::loadSectors(UDMFSetupSettings &setupSettings) const
{
   numsectors = (int)mSectors.getLength();
   sectors = estructalloctag(sector_t, numsectors, PU_LEVEL);

   for(int i = 0; i < numsectors; ++i)
   {
      sector_t *ss = sectors + i;
      const USector &us = mSectors[i];

      if(mNamespace == namespace_Eternity)
      {
         // These two pass the fixed_t value now
         ss->floorheight = us.heightfloor;
         ss->ceilingheight = us.heightceiling;

         // New to Eternity
         ss->floor_xoffs = us.xpanningfloor;
         ss->floor_yoffs = us.ypanningfloor;
         ss->ceiling_xoffs = us.xpanningceiling;
         ss->ceiling_yoffs = us.ypanningceiling;
         ss->floorbaseangle = static_cast<float>
            (E_NormalizeFlatAngle(us.rotationfloor) *  PI / 180.0f);
         ss->ceilingbaseangle = static_cast<float>
            (E_NormalizeFlatAngle(us.rotationceiling) *  PI / 180.0f);

         // Flags
         ss->flags |= us.secret ? SECF_SECRET : 0;

         // Friction: set the parameter directly from UDMF
         if(us.friction >= 0)   // default: -1
         {
            int friction, movefactor;
            P_CalcFriction(us.friction, friction, movefactor);

            ss->flags |= SECF_FRICTION;   // add the flag too
            ss->friction = friction;
            ss->movefactor = movefactor;
         }

         // Damage
         ss->damage = us.damageamount;
         ss->damagemask = us.damageinterval;
         ss->damagemod = E_DamageTypeNumForName(us.damagetype.constPtr());
         // If the following flags are true for the current sector, then set the
         // appropriate damageflags to true, otherwise don't set them.
         ss->damageflags |= us.damage_endgodmode ? SDMG_ENDGODMODE : 0;
         ss->damageflags |= us.damage_exitlevel ? SDMG_EXITLEVEL : 0;
         ss->damageflags |= us.damageterraineffect ? SDMG_TERRAINHIT : 0;
         ss->leakiness = eclamp(us.leakiness, 0, 256);

         // Terrain types
         if(us.floorterrain.strCaseCmp(DEFAULT_flat))
            ss->floorterrain = E_TerrainForName(us.floorterrain.constPtr());
         if (us.ceilingterrain.strCaseCmp(DEFAULT_flat))
            ss->ceilingterrain = E_TerrainForName(us.ceilingterrain.constPtr());

         // Lights
         ss->floorlightdelta = static_cast<int16_t>(us.lightfloor);
         ss->ceilinglightdelta = static_cast<int16_t>(us.lightceiling);
         ss->flags |=
         (us.lightfloorabsolute ? SECF_FLOORLIGHTABSOLUTE : 0) |
         (us.lightceilingabsolute ? SECF_CEILLIGHTABSOLUTE : 0);

         // sector colormaps
         ss->topmap = ss->midmap = ss->bottommap = -1; // mark as not specified

         setupSettings.setSectorPortals(i, us.portalceiling, us.portalfloor);
      }
      else
      {
         ss->floorheight = us.heightfloor << FRACBITS;
         ss->ceilingheight = us.heightceiling << FRACBITS;
      }
      ss->floorpic = R_FindFlat(us.texturefloor.constPtr());
      P_SetSectorCeilingPic(ss,
                            R_FindFlat(us.textureceiling.constPtr()));
      ss->lightlevel = us.lightlevel;
      ss->special = us.special;
      ss->tag = us.identifier;
      P_InitSector(ss);

      //
      // HERE GO THE PROPERTIES THAT MUST TAKE EFFECT AFTER P_InitSector
      //
      if(mNamespace == namespace_Eternity)
      {
         if(us.colormaptop.strCaseCmp(DEFAULT_default))
         {
            ss->topmap    = R_ColormapNumForName(us.colormaptop.constPtr());
            setupSettings.setSectorFlag(i, UDMF_SECTOR_INIT_COLORMAPPED);
         }
         if(us.colormapmid.strCaseCmp(DEFAULT_default))
         {
            ss->midmap    = R_ColormapNumForName(us.colormapmid.constPtr());
            setupSettings.setSectorFlag(i, UDMF_SECTOR_INIT_COLORMAPPED);
         }
         if(us.colormapbottom.strCaseCmp(DEFAULT_default))
         {
            ss->bottommap = R_ColormapNumForName(us.colormapbottom.constPtr());
            setupSettings.setSectorFlag(i, UDMF_SECTOR_INIT_COLORMAPPED);
         }

         // Portal fields
         // Floors
         ss->f_pflags |= us.portal_floor_alpha << PO_OPACITYSHIFT;
         ss->f_pflags |= us.portal_floor_blocksound ? PF_BLOCKSOUND : 0;
         ss->f_pflags |= us.portal_floor_disabled ? PF_DISABLED : 0;
         ss->f_pflags |= us.portal_floor_nopass ? PF_NOPASS : 0;
         ss->f_pflags |= us.portal_floor_norender ? PF_NORENDER : 0;
         if(!us.portal_floor_overlaytype.strCaseCmp(RENDERSTYLE_translucent))
            ss->f_pflags |= PS_OVERLAY;
         else if(!us.portal_floor_overlaytype.strCaseCmp(RENDERSTYLE_add))
            ss->f_pflags |= PS_OBLENDFLAGS; // PS_OBLENDFLAGS is PS_OVERLAY | PS_ADDITIVE
         ss->f_pflags |= us.portal_floor_useglobaltex ? PS_USEGLOBALTEX : 0;

         // Ceilings
         ss->c_pflags |= us.portal_ceil_alpha << PO_OPACITYSHIFT;
         ss->c_pflags |= us.portal_ceil_blocksound ? PF_BLOCKSOUND : 0;
         ss->c_pflags |= us.portal_ceil_disabled ? PF_DISABLED : 0;
         ss->c_pflags |= us.portal_ceil_nopass ? PF_NOPASS : 0;
         ss->c_pflags |= us.portal_ceil_norender ? PF_NORENDER : 0;
         if(!us.portal_ceil_overlaytype.strCaseCmp(RENDERSTYLE_translucent))
            ss->c_pflags |= PS_OVERLAY;
         else if(!us.portal_ceil_overlaytype.strCaseCmp(RENDERSTYLE_add))
            ss->c_pflags |= PS_OBLENDFLAGS; // PS_OBLENDFLAGS is PS_OVERLAY | PS_ADDITIVE
         ss->c_pflags |= us.portal_ceil_useglobaltex ? PS_USEGLOBALTEX : 0;
      }
   }
}

//
// Loads sidedefs
//
void UDMFParser::loadSidedefs() const
{
   numsides = (int)mSidedefs.getLength();
   sides = estructalloctag(side_t, numsides, PU_LEVEL);
}

//
// Loads linedefs. Returns false on error.
//
bool UDMFParser::loadLinedefs(UDMFSetupSettings &setupSettings)
{
   numlines = (int)mLinedefs.getLength();
   lines = estructalloctag(line_t, numlines, PU_LEVEL);
   for(int i = 0; i < numlines; ++i)
   {
      line_t *ld = lines + i;
      const ULinedef &uld = mLinedefs[i];
      if(uld.blocking)
         ld->flags |= ML_BLOCKING;
      if(uld.blockmonsters)
         ld->flags |= ML_BLOCKMONSTERS;
      if(uld.twosided)
         ld->flags |= ML_TWOSIDED;
      if(uld.dontpegtop)
         ld->flags |= ML_DONTPEGTOP;
      if(uld.dontpegbottom)
         ld->flags |= ML_DONTPEGBOTTOM;
      if(uld.secret)
         ld->flags |= ML_SECRET;
      if(uld.blocksound)
         ld->flags |= ML_SOUNDBLOCK;
      if(uld.dontdraw)
         ld->flags |= ML_DONTDRAW;
      if(uld.mapped)
         ld->flags |= ML_MAPPED;

      if(mNamespace == namespace_Doom || mNamespace == namespace_Eternity)
      {
         if(uld.passuse)
            ld->flags |= ML_PASSUSE;
      }

      // Eternity
      if(mNamespace == namespace_Eternity)
      {
         if(uld.midtex3d)
            ld->flags |= ML_3DMIDTEX;
         if(uld.firstsideonly)
            ld->extflags |= EX_ML_1SONLY;
         if(uld.blockeverything)
            ld->extflags |= EX_ML_BLOCKALL;
         if(uld.zoneboundary)
            ld->extflags |= EX_ML_ZONEBOUNDARY;
         if(uld.clipmidtex)
            ld->extflags |= EX_ML_CLIPMIDTEX;
         if(uld.midtex3dimpassible)
            ld->extflags |= EX_ML_3DMTPASSPROJ;
         if(uld.lowerportal)
            ld->extflags |= EX_ML_LOWERPORTAL;
         if(uld.upperportal)
            ld->extflags |= EX_ML_UPPERPORTAL;
         setupSettings.setCopyPortal(i, uld.copyceilingportal, 
            uld.copyfloorportal);
      }

      // TODO: Strife

      if(mNamespace == namespace_Hexen || mNamespace == namespace_Eternity)
      {
         if(uld.playercross)
            ld->extflags |= EX_ML_PLAYER | EX_ML_CROSS;
         if(uld.playeruse)
            ld->extflags |= EX_ML_PLAYER | EX_ML_USE;
         if(uld.monstercross)
            ld->extflags |= EX_ML_MONSTER | EX_ML_CROSS;
         if(uld.monsteruse)
            ld->extflags |= EX_ML_MONSTER | EX_ML_USE;
         if(uld.impact)
            ld->extflags |= EX_ML_MISSILE | EX_ML_IMPACT;
         if(uld.playerpush)
            ld->extflags |= EX_ML_PLAYER | EX_ML_PUSH;
         if(uld.monsterpush)
            ld->extflags |= EX_ML_MONSTER | EX_ML_PUSH;
         if(uld.missilecross)
            ld->extflags |= EX_ML_MISSILE | EX_ML_CROSS;
         if(uld.repeatspecial)
            ld->extflags |= EX_ML_REPEAT;
      }

      ld->special = uld.special;
      ld->tag = uld.identifier;
      ld->args[0] = uld.arg[0];
      ld->args[1] = uld.arg[1];
      ld->args[2] = uld.arg[2];
      ld->args[3] = uld.arg[3];
      ld->args[4] = uld.arg[4];
      if(uld.v1 < 0 || uld.v1 >= numvertexes ||
         uld.v2 < 0 || uld.v2 >= numvertexes ||
         uld.sidefront < 0 || uld.sidefront >= numsides ||
         uld.sideback < -1 || uld.sideback >= numsides)
      {
         mLine = uld.errorline;
         mColumn = 1;
         mError = "Vertex or sidedef overflow";
         return false;
      }
      ld->v1 = &vertexes[uld.v1];
      ld->v2 = &vertexes[uld.v2];
      ld->sidenum[0] = uld.sidefront;
      ld->sidenum[1] = uld.sideback;
      P_InitLineDef(ld);
      P_PostProcessLineFlags(ld);

      // more Eternity
      if(mNamespace == namespace_Eternity)
      {
         ld->alpha = uld.alpha;
         if(!uld.renderstyle.strCaseCmp(RENDERSTYLE_add))
            ld->extflags |= EX_ML_ADDITIVE;
         if(!uld.tranmap.empty())
         {
            if(uld.tranmap != "TRANMAP")
            {
               int special = W_CheckNumForName(uld.tranmap.constPtr());
               if(special < 0 || W_LumpLength(special) != 65536)
                  ld->tranlump = 0;
               else
                  ld->tranlump = special + 1;
            }
            else
               ld->tranlump = 0;
         }
      }
   }
   return true;
}

//
// Loads sidedefs (2)
//
bool UDMFParser::loadSidedefs2()
{
   for(int i = 0; i < numsides; ++i)
   {
      side_t *sd = sides + i;
      const USidedef &usd = mSidedefs[i];

      if(mNamespace == namespace_Eternity)
      {
         sd->textureoffset = usd.offsetx;
         sd->rowoffset = usd.offsety;
      }
      else
      {
         sd->textureoffset = usd.offsetx << FRACBITS;
         sd->rowoffset = usd.offsety << FRACBITS;
      }
      if(usd.sector < 0 || usd.sector >= numsectors)
      {
         mLine = usd.errorline;
         mColumn = 1;
         mError = "Sector overflow";
         return false;
      }
      sd->sector = &sectors[usd.sector];
      P_SetupSidedefTextures(*sd, usd.texturebottom.constPtr(),
                             usd.texturemiddle.constPtr(),
                             usd.texturetop.constPtr());
   }
   return true;
}

//
// Loads things
//
bool UDMFParser::loadThings()
{
   mapthing_t *mapthings;
   numthings = (int)mThings.getLength();
   mapthings = ecalloc(mapthing_t *, numthings, sizeof(mapthing_t));
   for(int i = 0; i < numthings; ++i)
   {
      mapthing_t *ft = &mapthings[i];
      const uthing_t &ut = mThings[i];
      ft->type = ut.type;
      // no Doom thing ban in UDMF
      ft->tid = ut.identifier;
      ft->x = ut.x;
      ft->y = ut.y;
      ft->height = ut.height;
      ft->angle = ut.angle;
      if(ut.skill1 ^ ut.skill2)
         ft->extOptions |= MTF_EX_BABY_TOGGLE;
      if(ut.skill2)
         ft->options |= MTF_EASY;
      if(ut.skill3)
         ft->options |= MTF_NORMAL;
      if(ut.skill4)
         ft->options |= MTF_HARD;
      if(ut.skill4 ^ ut.skill5)
         ft->extOptions |= MTF_EX_NIGHTMARE_TOGGLE;
      if(ut.ambush)
         ft->options |= MTF_AMBUSH;
      if(!ut.single)
         ft->options |= MTF_NOTSINGLE;
      if(!ut.dm)
         ft->options |= MTF_NOTDM;
      if(!ut.coop)
         ft->options |= MTF_NOTCOOP;
      if(ut.friendly && (mNamespace == namespace_Doom || mNamespace == namespace_Eternity))
         ft->options |= MTF_FRIEND;
      if(ut.dormant && (mNamespace == namespace_Hexen || mNamespace == namespace_Eternity))
         ft->options |= MTF_DORMANT;
      // TODO: class1, 2, 3
      // TODO: STRIFE
      if(mNamespace == namespace_Hexen || mNamespace == namespace_Eternity)
      {
         ft->special = ut.special;
         ft->args[0] = ut.arg[0];
         ft->args[1] = ut.arg[1];
         ft->args[2] = ut.arg[2];
         ft->args[3] = ut.arg[3];
         ft->args[4] = ut.arg[4];
      }

      // haleyjd 10/05/05: convert heretic things
      if(mNamespace == namespace_Heretic)
         P_ConvertHereticThing(ft);

      P_ConvertDoomExtendedSpawnNum(ft);
      P_SpawnMapThing(ft);
   }

   // haleyjd: all player things for players in this game should now be valid
   if(GameType != gt_dm)
   {
      for(int i = 0; i < MAXPLAYERS; i++)
      {
         if(playeringame[i] && !players[i].mo)
         {
            mError = "Missing required player start";
            efree(mapthings);
            return false;
         }
      }
   }

   efree(mapthings);
   return true;
}

//==============================================================================
//
// TEXTMAP parsing
//
//==============================================================================

//
// A means to map a string to a numeric token, so it can be easily passed into
// switch/case
//

enum token_e
{
   t_alpha,
   t_ambush,
   t_angle,
   t_arg0,
   t_arg1,
   t_arg2,
   t_arg3,
   t_arg4,
   t_blockeverything,
   t_blockfloaters,
   t_blocking,
   t_blockmonsters,
   t_blocksound,
   t_ceilingterrain,
   t_class1,
   t_class2,
   t_class3,
   t_clipmidtex,
   t_colormapbottom,
   t_colormapmid,
   t_colormaptop,
   t_coop,
   t_copyceilingportal,
   t_copyfloorportal,
   t_damage_endgodmode,
   t_damage_exitlevel,
   t_damageamount,
   t_damageinterval,
   t_damageterraineffect,
   t_damagetype,
   t_dm,
   t_dontdraw,
   t_dontpegbottom,
   t_dontpegtop,
   t_dormant,
   t_firstsideonly,
   t_floorterrain,
   t_friction,
   t_friend,
   t_height,
   t_heightceiling,
   t_heightfloor,
   t_id,
   t_impact,
   t_invisible,
   t_jumpover,
   t_leakiness,
   t_lightceiling,
   t_lightceilingabsolute,
   t_lightfloor,
   t_lightfloorabsolute,
   t_lightlevel,
   t_lowerportal,
   t_mapped,
   t_midtex3d,
   t_midtex3dimpassible,
   t_missilecross,
   t_monstercross,
   t_monsterpush,
   t_monsteruse,
   t_offsetx,
   t_offsety,
   t_portalceiling,
   t_portal_ceil_alpha,
   t_portal_ceil_blocksound,
   t_portal_ceil_disabled,
   t_portal_ceil_nopass,
   t_portal_ceil_norender,
   t_portal_ceil_overlaytype,
   t_portal_ceil_useglobaltex,
   t_portalfloor,
   t_portal_floor_alpha,
   t_portal_floor_blocksound,
   t_portal_floor_disabled,
   t_portal_floor_nopass,
   t_portal_floor_norender,
   t_portal_floor_overlaytype,
   t_portal_floor_useglobaltex,
   t_passuse,
   t_playercross,
   t_playerpush,
   t_playeruse,
   t_renderstyle,
   t_repeatspecial,
   t_rotationceiling,
   t_rotationfloor,
   t_secret,
   t_sector,
   t_sideback,
   t_sidefront,
   t_single,
   t_skill1,
   t_skill2,
   t_skill3,
   t_skill4,
   t_skill5,
   t_special,
   t_standing,
   t_strifeally,
   t_texturebottom,
   t_textureceiling,
   t_texturefloor,
   t_texturemiddle,
   t_texturetop,
   t_tranmap,
   t_translucent,
   t_twosided,
   t_type,
   t_upperportal,
   t_v1,
   t_v2,
   t_x,
   t_xpanningceiling,
   t_xpanningfloor,
   t_y,
   t_ypanningceiling,
   t_ypanningfloor,
   t_zoneboundary,
};

struct keytoken_t
{
   const char *string;
   DLListItem<keytoken_t> link;
   token_e token;
};

#define TOKEN(a) { #a, DLListItem<keytoken_t>(), t_##a }

static keytoken_t gTokenList[] =
{
   TOKEN(alpha),
   TOKEN(ambush),
   TOKEN(angle),
   TOKEN(arg0),
   TOKEN(arg1),
   TOKEN(arg2),
   TOKEN(arg3),
   TOKEN(arg4),
   TOKEN(blockeverything),
   TOKEN(blockfloaters),
   TOKEN(blocking),
   TOKEN(blockmonsters),
   TOKEN(blocksound),
   TOKEN(ceilingterrain),
   TOKEN(class1),
   TOKEN(class2),
   TOKEN(class3),
   TOKEN(clipmidtex),
   TOKEN(colormapbottom),
   TOKEN(colormapmid),
   TOKEN(colormaptop),
   TOKEN(copyceilingportal),
   TOKEN(copyfloorportal),
   TOKEN(coop),
   TOKEN(damage_endgodmode),
   TOKEN(damage_exitlevel),
   TOKEN(damageamount),
   TOKEN(damageinterval),
   TOKEN(damageterraineffect),
   TOKEN(damagetype),
   TOKEN(dm),
   TOKEN(dontdraw),
   TOKEN(dontpegbottom),
   TOKEN(dontpegtop),
   TOKEN(dormant),
   TOKEN(firstsideonly),
   TOKEN(floorterrain),
   TOKEN(friction),
   TOKEN(friend),
   TOKEN(height),
   TOKEN(heightceiling),
   TOKEN(heightfloor),
   TOKEN(id),
   TOKEN(impact),
   TOKEN(invisible),
   TOKEN(jumpover),
   TOKEN(leakiness),
   TOKEN(lightceiling),
   TOKEN(lightceilingabsolute),
   TOKEN(lightfloor),
   TOKEN(lightfloorabsolute),
   TOKEN(lightlevel),
   TOKEN(lowerportal),
   TOKEN(mapped),
   TOKEN(midtex3d),
   TOKEN(midtex3dimpassible),
   TOKEN(missilecross),
   TOKEN(monstercross),
   TOKEN(monsterpush),
   TOKEN(monsteruse),
   TOKEN(offsetx),
   TOKEN(offsety),
   TOKEN(portalceiling),
   TOKEN(portal_ceil_alpha),
   TOKEN(portal_ceil_blocksound),
   TOKEN(portal_ceil_disabled),
   TOKEN(portal_ceil_nopass),
   TOKEN(portal_ceil_norender),
   TOKEN(portal_ceil_overlaytype),
   TOKEN(portal_ceil_useglobaltex),
   TOKEN(portalfloor),
   TOKEN(portal_floor_alpha),
   TOKEN(portal_floor_blocksound),
   TOKEN(portal_floor_disabled),
   TOKEN(portal_floor_nopass),
   TOKEN(portal_floor_norender),
   TOKEN(portal_floor_overlaytype),
   TOKEN(portal_floor_useglobaltex),
   TOKEN(passuse),
   TOKEN(playercross),
   TOKEN(playerpush),
   TOKEN(playeruse),
   TOKEN(renderstyle),
   TOKEN(repeatspecial),
   TOKEN(rotationceiling),
   TOKEN(rotationfloor),
   TOKEN(secret),
   TOKEN(sector),
   TOKEN(sideback),
   TOKEN(sidefront),
   TOKEN(single),
   TOKEN(skill1),
   TOKEN(skill2),
   TOKEN(skill3),
   TOKEN(skill4),
   TOKEN(skill5),
   TOKEN(special),
   TOKEN(standing),
   TOKEN(strifeally),
   TOKEN(texturebottom),
   TOKEN(textureceiling),
   TOKEN(texturefloor),
   TOKEN(texturemiddle),
   TOKEN(texturetop),
   TOKEN(tranmap),
   TOKEN(translucent),
   TOKEN(twosided),
   TOKEN(type),
   TOKEN(upperportal),
   TOKEN(v1),
   TOKEN(v2),
   TOKEN(x),
   TOKEN(xpanningceiling),
   TOKEN(xpanningfloor),
   TOKEN(y),
   TOKEN(ypanningceiling),
   TOKEN(ypanningfloor),
   TOKEN(zoneboundary),
};

static EHashTable<keytoken_t, ENCStringHashKey, &keytoken_t::string, &keytoken_t::link> gTokenTable;

static void registerAllKeys()
{
   static bool called = false;
   if(called)
      return;
   for(size_t i = 0; i < earrlen(gTokenList); ++i)
      gTokenTable.addObject(gTokenList[i]);
   called = true;
}

//
// Tries to parse a UDMF TEXTMAP document. If it fails, it returns false and
// you can check the error message with error()
//
bool UDMFParser::parse(WadDirectory &setupwad, int lump)
{
   {
      ZAutoBuffer buf;

      setupwad.cacheLumpAuto(lump, buf);
      auto data = buf.getAs<const char *>();

      // store it conveniently
      setData(data, setupwad.lumpLength(lump));
   }

   readresult_e result = readItem();
   if(result == result_Error)
      return false;
   if(result != result_Assignment || mKey.strCaseCmp("namespace") ||
      mValue.type != Token::type_String)
   {
      mError = "TEXTMAP must begin with a namespace assignment";
      return false;
   }

   // Set namespace (we'll think later about checking it
   if(!mValue.text.strCaseCmp("eternity"))
      mNamespace = namespace_Eternity;
   else if(!mValue.text.strCaseCmp("heretic"))
      mNamespace = namespace_Heretic;
   else if(!mValue.text.strCaseCmp("hexen"))
      mNamespace = namespace_Hexen;
   else if(!mValue.text.strCaseCmp("strife"))
      mNamespace = namespace_Strife;
   else if(!mValue.text.strCaseCmp("doom"))
      mNamespace = namespace_Doom;
   else
   {
      mError = "Unsupported namespace '";
      mError << mValue.text;
      mError << "'";
      return false;
   }

   // Gamestuff. Must be null when out of block and only one be set when in
   // block
   ULinedef *linedef = nullptr;
   USidedef *sidedef = nullptr;
   uvertex_t *vertex = nullptr;
   USector *sector = nullptr;
   uthing_t *thing = nullptr;

   registerAllKeys();   // now it's the time

   while((result = readItem()) != result_Eof)
   {
      if(result == result_Error)
         return false;
      if(result == result_BlockEntry)
      {
         // we're now in some block. Alloc stuff
         if(!mBlockName.strCaseCmp("linedef"))
         {
            linedef = &mLinedefs.addNew();
            linedef->errorline = mLine;
            linedef->renderstyle = RENDERSTYLE_translucent;
         }
         else if(!mBlockName.strCaseCmp("sidedef"))
         {
            sidedef = &mSidedefs.addNew();
            sidedef->texturetop = "-";
            sidedef->texturebottom = "-";
            sidedef->texturemiddle = "-";
            sidedef->errorline = mLine;
         }
         else if(!mBlockName.strCaseCmp("vertex"))
            vertex = &mVertices.addNew();
         else if(!mBlockName.strCaseCmp("sector"))
            sector = &mSectors.addNew();
         else if(!mBlockName.strCaseCmp("thing"))
            thing = &mThings.addNew();
         continue;
      }
      if(result == result_Assignment && mInBlock)
      {

#define REQUIRE_INT(obj, field, flag) case t_##field: requireInt(obj->field, obj->flag); break
#define READ_NUMBER(obj, field) case t_##field: readNumber(obj->field); break
#define READ_BOOL(obj, field) case t_##field: readBool(obj->field); break
#define READ_STRING(obj, field) case t_##field: readString(obj->field); break
#define READ_FIXED(obj, field) case t_##field: readFixed(obj->field); break
#define REQUIRE_FIXED(obj, field, flag) case t_##field: requireFixed(obj->field, obj->flag); break

         const keytoken_t *kt = gTokenTable.objectForKey(mKey.constPtr());
         if(kt)
         {
            if(linedef)
            {
               switch(kt->token)
               {
                  case t_id: readNumber(linedef->identifier); break;
                  REQUIRE_INT(linedef, v1, v1set);
                  REQUIRE_INT(linedef, v2, v2set);
                  REQUIRE_INT(linedef, sidefront, sfrontset);
                  READ_NUMBER(linedef, sideback);
                  READ_BOOL(linedef, blocking);
                  READ_BOOL(linedef, blockmonsters);
                  READ_BOOL(linedef, twosided);
                  READ_BOOL(linedef, dontpegtop);
                  READ_BOOL(linedef, dontpegbottom);
                  READ_BOOL(linedef, secret);
                  READ_BOOL(linedef, blocksound);
                  READ_BOOL(linedef, dontdraw);
                  READ_BOOL(linedef, mapped);
                  case t_passuse:
                        readBool(linedef->passuse);
                     break;
                  case t_translucent:
                        readBool(linedef->translucent);
                     break;
                  case t_jumpover:
                        readBool(linedef->jumpover);
                     break;
                  case t_blockfloaters:
                        readBool(linedef->blockfloaters);
                     break;
                  READ_NUMBER(linedef, special);
                  case t_arg0: readNumber(linedef->arg[0]); break;
                  case t_arg1: readNumber(linedef->arg[1]); break;
                  case t_arg2: readNumber(linedef->arg[2]); break;
                  case t_arg3: readNumber(linedef->arg[3]); break;
                  case t_arg4: readNumber(linedef->arg[4]); break;
                  READ_BOOL(linedef, playercross);
                  READ_BOOL(linedef, playeruse);
                  READ_BOOL(linedef, monstercross);
                  READ_BOOL(linedef, monsteruse);
                  READ_BOOL(linedef, impact);
                  READ_BOOL(linedef, playerpush);
                  READ_BOOL(linedef, monsterpush);
                  READ_BOOL(linedef, missilecross);
                  READ_BOOL(linedef, repeatspecial);

                  READ_BOOL(linedef, midtex3d);
                  READ_BOOL(linedef, midtex3dimpassible);
                  READ_BOOL(linedef, firstsideonly);
                  READ_BOOL(linedef, blockeverything);
                  READ_BOOL(linedef, zoneboundary);
                  READ_BOOL(linedef, clipmidtex);
                  READ_BOOL(linedef, lowerportal);
                  READ_BOOL(linedef, upperportal);
                  READ_NUMBER(linedef, copyceilingportal);
                  READ_NUMBER(linedef, copyfloorportal);
                  READ_NUMBER(linedef, alpha);
                  READ_STRING(linedef, renderstyle);
                  READ_STRING(linedef, tranmap);
                  default:
                     break;
               }

            }
            else if(sidedef)
            {
               switch(kt->token)
               {
                  case t_offsetx:
                     if(mNamespace == namespace_Eternity)
                        readFixed(sidedef->offsetx);
                     else
                        readNumber(sidedef->offsetx);
                     break;
                  case t_offsety:
                     if(mNamespace == namespace_Eternity)
                        readFixed(sidedef->offsety);
                     else
                        readNumber(sidedef->offsety);
                     break;
                  READ_STRING(sidedef, texturetop);
                  READ_STRING(sidedef, texturebottom);
                  READ_STRING(sidedef, texturemiddle);
                  REQUIRE_INT(sidedef, sector, sset);
                  default:
                     break;
               }
            }
            else if(vertex)
            {
               if(kt->token == t_x)
                  requireFixed(vertex->x, vertex->xset);
               else if(kt->token == t_y)
                  requireFixed(vertex->y, vertex->yset);
            }
            else if(sector)
            {
               switch(kt->token)
               {
                  case t_texturefloor:
                     requireString(sector->texturefloor, sector->tfloorset);
                     break;
                  case t_textureceiling:
                     requireString(sector->textureceiling, sector->tceilset);
                     break;
                  READ_NUMBER(sector, lightlevel);
                  READ_NUMBER(sector, special);
                  case t_id:
                     readNumber(sector->identifier);
                     break;
                  case t_heightfloor:
                     if(mNamespace != namespace_Eternity)
                        readNumber(sector->heightfloor);
                     else
                        readFixed(sector->heightfloor);
                     break;
                  case t_heightceiling:
                     if(mNamespace != namespace_Eternity)
                        readNumber(sector->heightceiling);
                     else
                        readFixed(sector->heightceiling);
                  default:
                     break;
               }
               if(mNamespace == namespace_Eternity)
               {
                  switch(kt->token)
                  {
                     READ_FIXED(sector, xpanningfloor);
                     READ_FIXED(sector, ypanningfloor);
                     READ_FIXED(sector, xpanningceiling);
                     READ_FIXED(sector, ypanningceiling);
                     READ_NUMBER(sector, rotationfloor);
                     READ_NUMBER(sector, rotationceiling);

                     READ_BOOL(sector, secret);
                     READ_NUMBER(sector, friction);

                     READ_NUMBER(sector, lightfloor);
                     READ_NUMBER(sector, lightceiling);
                     READ_BOOL(sector, lightfloorabsolute);
                     READ_BOOL(sector, lightceilingabsolute);

                     READ_STRING(sector, colormaptop);
                     READ_STRING(sector, colormapmid);
                     READ_STRING(sector, colormapbottom);

                     READ_NUMBER(sector, leakiness);
                     READ_NUMBER(sector, damageamount);
                     READ_NUMBER(sector, damageinterval);
                     READ_BOOL(sector, damage_endgodmode);
                     READ_BOOL(sector, damage_exitlevel);
                     READ_BOOL(sector, damageterraineffect);
                     READ_STRING(sector, damagetype);

                     READ_STRING(sector, floorterrain);
                     READ_STRING(sector, ceilingterrain);

                     READ_STRING(sector, portal_floor_overlaytype);
                     READ_NUMBER(sector, portal_floor_alpha);
                     READ_BOOL(sector, portal_floor_blocksound);
                     READ_BOOL(sector, portal_floor_disabled);
                     READ_BOOL(sector, portal_floor_nopass);
                     READ_BOOL(sector, portal_floor_norender);
                     READ_BOOL(sector, portal_floor_useglobaltex);

                     READ_STRING(sector, portal_ceil_overlaytype);
                     READ_NUMBER(sector, portal_ceil_alpha);
                     READ_BOOL(sector, portal_ceil_blocksound);
                     READ_BOOL(sector, portal_ceil_disabled);
                     READ_BOOL(sector, portal_ceil_nopass);
                     READ_BOOL(sector, portal_ceil_norender);
                     READ_BOOL(sector, portal_ceil_useglobaltex);

                     READ_NUMBER(sector, portalceiling);
                     READ_NUMBER(sector, portalfloor);
                     default:
                        break;
                  }
               }
            }
            else if(thing)
            {
               switch(kt->token)
               {
                  case t_id: readNumber(thing->identifier); break;
                  REQUIRE_FIXED(thing, x, xset);
                  REQUIRE_FIXED(thing, y, yset);
                  READ_FIXED(thing, height);
                  READ_NUMBER(thing, angle);
                  REQUIRE_INT(thing, type, typeset);
                  READ_BOOL(thing, skill1);
                  READ_BOOL(thing, skill2);
                  READ_BOOL(thing, skill3);
                  READ_BOOL(thing, skill4);
                  READ_BOOL(thing, skill5);
                  READ_BOOL(thing, ambush);
                  READ_BOOL(thing, single);
                  READ_BOOL(thing, dm);
                  READ_BOOL(thing, coop);
                  case t_friend:
                        readBool(thing->friendly);
                     break;
                  READ_BOOL(thing, dormant);
                  READ_BOOL(thing, class1);
                  READ_BOOL(thing, class2);
                  READ_BOOL(thing, class3);
                  READ_BOOL(thing, standing);
                  READ_BOOL(thing, strifeally);
                  READ_BOOL(thing, translucent);
                  READ_BOOL(thing, invisible);
                  case t_special:
                        readNumber(thing->special);
                     break;
                  case t_arg0:
                        readNumber(thing->arg[0]);
                     break;
                  case t_arg1:
                        readNumber(thing->arg[1]);
                     break;
                  case t_arg2:
                        readNumber(thing->arg[2]);
                     break;
                  case t_arg3:
                        readNumber(thing->arg[3]);
                     break;
                  case t_arg4:
                        readNumber(thing->arg[4]);
                     break;
                  default:
                     break;
               }
            }
         }
         continue;
      }
      if(result == result_BlockExit)
      {
         if(linedef)
         {
            if(!linedef->v1set || !linedef->v2set || !linedef->sfrontset)
            {
               mError = "Incompletely defined linedef";
               return false;
            }
            linedef = nullptr;
         }
         else if(sidedef)
         {
            if(!sidedef->sset)
            {
               mError = "Incompletely defined sidedef";
               return false;
            }
            sidedef = nullptr;
         }
         else if(vertex)
         {
            if(!vertex->xset || !vertex->yset)
            {
               mError = "Incompletely defined vertex";
               return false;
            }
            vertex = nullptr;
         }
         else if(sector)
         {
            if(!sector->tfloorset || !sector->tceilset)
            {
               mError = "Incompletely defined sector";
               return false;
            }
            sector = nullptr;
         }
         else if(thing)
         {
            if(!thing->xset || !thing->yset || !thing->typeset)
            {
               mError = "Incompletely defined thing";
               return false;
            }
            thing = nullptr;
         }
      }
   }

   return true;
}

//
// Quick error message
//
qstring UDMFParser::error() const
{
   qstring message("TEXTMAP error at ");
   message << (int)mLine << ':' << (int)mColumn << " - " << mError;
   return message;
}

//
// Returns the level info map format from the namespace, chiefly for linedef
// specials
//
int UDMFParser::getMapFormat() const
{
   switch(mNamespace)
   {
      case namespace_Doom:
      case namespace_Heretic:
      case namespace_Strife:
         // Just use the normal doom linedefs
         return LEVEL_FORMAT_DOOM;
      case namespace_Eternity:
         return LEVEL_FORMAT_UDMF_ETERNITY;
      case namespace_Hexen:
         return LEVEL_FORMAT_HEXEN;
      default:
         return LEVEL_FORMAT_INVALID; // Unsupported namespace
   }
}

//
// Loads a new TEXTMAP and clears all variables
//
void UDMFParser::setData(const char *data, size_t size)
{
   mData.copy(data, size);
   mPos = 0;
   mLine = 1;
   mColumn = 1;
   mError.clear();

   mKey.clear();
   mValue.clear();
   mInBlock = false;
   mBlockName.clear();

   // Game stuff
   mNamespace = namespace_Doom;  // default to Doom
   mLinedefs.makeEmpty();
   mSidedefs.makeEmpty();
   mVertices.makeEmpty();
   mSectors.makeEmpty();
   mThings.makeEmpty();
}

//
// Passes a fixed_t
//
void UDMFParser::readFixed(fixed_t &target) const
{
   if(mValue.type == Token::type_Number)
      target = M_DoubleToFixed(mValue.number);
}

//
// Passes a float to an object and flags a required element
//
void UDMFParser::requireFixed(fixed_t &target,
                              bool &flagtarget) const
{
   if(mValue.type == Token::type_Number)
   {
      target = M_DoubleToFixed(mValue.number);
      flagtarget = true;
   }
}

//
// Requires an int
//
void UDMFParser::requireInt(int &target, bool &flagtarget) const
{
   if(mValue.type == Token::type_Number)
   {
      target = static_cast<int>(mValue.number);
      flagtarget = true;
   }
}

//
// Reads a string
//
void UDMFParser::readString(qstring &target) const
{
   if(mValue.type == Token::type_String)
      target = mValue.text;
}

//
// Passes a string
//
void UDMFParser::requireString(qstring &target, bool &flagtarget) const
{
   if(mValue.type == Token::type_String)
   {
      target = mValue.text;
      flagtarget = true;
   }
}

//
// Passes a boolean
//
void UDMFParser::readBool(bool &target) const
{
   if(mValue.type == Token::type_Keyword)
   {
      target = ectype::toUpper(mValue.text[0]) == 'T';
   }
}

//
// Passes a number (float/double/int)
//
template<typename T>
void UDMFParser::readNumber(T &target) const
{
   if(mValue.type == Token::type_Number)
   {
      target = static_cast<T>(mValue.number);
   }

}

//
// Reads a line or block item. Returns false on error
//
UDMFParser::readresult_e UDMFParser::readItem()
{
   Token token;
   if(!next(token))
      return result_Eof;

   if(token.type == Token::type_Symbol && token.symbol == '}')
   {
      if(mInBlock)
      {
         mInBlock = false;
         mKey = mBlockName; // preserve the name into "key"
         mBlockName.clear();
         return result_BlockExit;
      }
      // not in block: error
      mError = "Unexpected '}'";
      return result_Error;
   }

   if(token.type != Token::type_Keyword)
   {
      mError = "Expected a keyword";
      return result_Error;
   }
   mKey = token.text;

   if(!next(token) || token.type != Token::type_Symbol ||
      (token.symbol != '=' && token.symbol != '{'))
   {
      mError = "Expected '=' or '{'";
      return result_Error;
   }

   if(token.symbol == '=')
   {
      // assignment
      if(!next(token) || (token.type != Token::type_Keyword &&
         token.type != Token::type_String && token.type != Token::type_Number))
      {
         mError = "Expected a number, string or true/false";
         return result_Error;
      }

      if(token.type == Token::type_Keyword && token.text.strCaseCmp("true") &&
         token.text.strCaseCmp("false"))
      {
         mError = "Identifier can only be true or false";
         return result_Error;
      }
      mValue = token;

      if(!next(token) || token.type != Token::type_Symbol ||
         token.symbol != ';')
      {
         mError = "Expected ; after assignment";
         return result_Error;
      }

      return result_Assignment;
   }
   else  // {
   {
      // block
      if(!mInBlock)
      {
         mInBlock = true;
         mBlockName = mKey;
         return result_BlockEntry;
      }
      else
      {
         mError = "Blocks cannot be nested";
         return result_Error;
      }
   }
}

//
// Gets the next token from mData. Returns false if EOF. It will not return
// false if there's something to return
//
bool UDMFParser::next(Token &token)
{
   // Skip all leading whitespace
   bool checkWhite = true;
   size_t size = mData.length();

   while(checkWhite)
   {
      checkWhite = false;  // reset it unless someone else restores it

      while(mPos != size && ectype::isSpace(mData[mPos]))
         addPos(1);
      if(mPos == size)
         return false;

      // Skip comments
      if(mData[mPos] == '/' && mPos + 1 < size && mData[mPos + 1] == '/')
      {
         addPos(2);
         // one line comment
         while(mPos != size && mData[mPos] != '\n')
            addPos(1);
         if(mPos == size)
            return false;

         // If here, we hit an "enter"
         addPos(1);
         if(mPos == size)
            return false;

         checkWhite = true;   // look again for whitespaces if reached here
      }

      if(mData[mPos] == '/' && mPos + 1 < size && mData[mPos + 1] == '*')
      {
         addPos(2);
         while(mPos + 1 < size && mData[mPos] != '*' && mData[mPos + 1] != '/')
            addPos(1);
         if(mPos + 1 >= size)
         {
            addPos(1);
            return false;
         }
         if(mData[mPos] == '*' && mData[mPos] == '/')
            addPos(2);
         if(mPos == size)
            return false;
         checkWhite = true;
      }
   }

   // now we're clear from whitespaces and comments

   // Check for number
   char *result = nullptr;
   double number = strtod(&mData[mPos], &result);
   if(result > &mData[mPos])  // we have something
   {
      // copy it
      token.type = Token::type_Number;
      token.number = number;
      addPos(result - mData.constPtr() - mPos);
      return true;
   }

   // Check for string
   if(mData[mPos] == '"')
   {
      addPos(1);

      // we entered a string
      // find the next string
      token.type = Token::type_String;

      // we must escape things here
      token.text.clear();
      bool escape = false;
      while(mPos != size)
      {
         if(!escape)
         {
            if(mData[mPos] == '\\')
               escape = true;
            else if(mData[mPos] == '"')
            {
               addPos(1);     // skip the quote
               return true;   // we're done
            }
            else
               token.text.Putc(mData[mPos]);
         }
         else
         {
            token.text.Putc(mData[mPos]);
            escape = false;
         }
         addPos(1);
      }
      return true;
   }

   // keyword: start with a letter or _
   if(ectype::isAlpha(mData[mPos]) || mData[mPos] == '_')
   {
      token.type = Token::type_Keyword;
      token.text.clear();
      while(mPos != size &&
            (ectype::isAlnum(mData[mPos]) || mData[mPos] == '_'))
      {
         token.text.Putc(mData[mPos]);
         addPos(1);
      }
      return true;
   }

   // symbol. Just put one character
   token.type = Token::type_Symbol;
   token.symbol = mData[mPos];
   addPos(1);

   return true;
}

//
// Increases position by given amount. Updates line and column accordingly.
//
void UDMFParser::addPos(int amount)
{
   for(int i = 0; i < amount; ++i)
   {
      if(mPos == mData.length())
         return;
      if(mData[mPos] == '\n')
      {
         mColumn = 1;
         mLine++;
      }
      else
         mColumn++;
      mPos++;
   }
}

// EOF
