//
// The Eternity Engine
// Copyright (C) 2020 James Haley et al.
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
// Purpose: EDF particle/actor beams
// Authors: Xaser Acheron
//

#define NEED_EDF_DEFINITIONS

#include "z_zone.h"

#include "Confuse/confuse.h"
#include "d_gi.h"
#include "e_edfmetatable.h"
#include "e_beam.h"
#include "e_states.h"
#include "e_things.h"

// TODO: particles. Need to add a "particletype" concept first.

// metakey vocabulary
#define ITEM_BEAM_THINGTYPE       "thingtype"
#define ITEM_BEAM_DENSITY         "density"
#define ITEM_BEAM_TAPERDIST       "taperdistance"
#define ITEM_BEAM_XDRIFT          "x.drift"
#define ITEM_BEAM_YDRIFT          "y.drift"
#define ITEM_BEAM_ZDRIFT          "z.drift"
#define ITEM_BEAM_XDRIFTMAX       "x.maxdrift"
#define ITEM_BEAM_YDRIFTMAX       "y.maxdrift"
#define ITEM_BEAM_ZDRIFTMAX       "z.maxdrift"

// Interned metatable keys
MetaKeyIndex keyBeamThingType(ITEM_BEAM_THINGTYPE);
MetaKeyIndex keyBeamDensity  (ITEM_BEAM_DENSITY  );
MetaKeyIndex keyBeamTaperDist(ITEM_BEAM_TAPERDIST);
MetaKeyIndex keyBeamXDrift   (ITEM_BEAM_XDRIFT   );
MetaKeyIndex keyBeamYDrift   (ITEM_BEAM_YDRIFT   );
MetaKeyIndex keyBeamZDrift   (ITEM_BEAM_ZDRIFT   );
MetaKeyIndex keyBeamXDriftMax(ITEM_BEAM_XDRIFTMAX);
MetaKeyIndex keyBeamYDriftMax(ITEM_BEAM_YDRIFTMAX);
MetaKeyIndex keyBeamZDriftMax(ITEM_BEAM_ZDRIFTMAX);

#define BEAM_CONFIGS \
   CFG_STR(ITEM_BEAM_THINGTYPE,        "",                 CFGF_NONE), \
   CFG_FLOAT(ITEM_BEAM_DENSITY,     16.0f,                 CFGF_NONE), \
   CFG_FLOAT(ITEM_BEAM_TAPERDIST,    0.0f,                 CFGF_NONE), \
   CFG_FLOAT(ITEM_BEAM_XDRIFT,       0.0f,                 CFGF_NONE), \
   CFG_FLOAT(ITEM_BEAM_YDRIFT,       0.0f,                 CFGF_NONE), \
   CFG_FLOAT(ITEM_BEAM_ZDRIFT,       0.0f,                 CFGF_NONE), \
   CFG_FLOAT(ITEM_BEAM_XDRIFTMAX,    0.0f,                 CFGF_NONE), \
   CFG_FLOAT(ITEM_BEAM_YDRIFTMAX,    0.0f,                 CFGF_NONE), \
   CFG_FLOAT(ITEM_BEAM_ZDRIFTMAX,    0.0f,                 CFGF_NONE), \
   CFG_END()

//
// EDF beam options
//
cfg_opt_t edf_beam_opts[] =
{
   CFG_TPROPS(edf_generic_tprops, CFGF_NOCASE),
   BEAM_CONFIGS
};

cfg_opt_t edf_beam_delta_opts[] =
{
   CFG_STR("name", 0, CFGF_NONE),
   BEAM_CONFIGS
};

static MetaTable e_beamTable; // the beamtype metatable

//
// Replaces thing names with mobjtypes
//
static void E_postprocessThingType(MetaTable *table)
{
   const char *name = table->getString(keyBeamThingType, nullptr);
   if(!name)
      return;
   table->addInt(keyBeamThingType, E_SafeThingName(name));
   table->removeStringNR(keyBeamThingType);
}

//
// Process the beamtype settings
//
void E_ProcessBeams(cfg_t *cfg)
{
   E_BuildGlobalMetaTableFromEDF(cfg, EDF_SEC_BEAMTYPE, EDF_SEC_BEAMDELTA,
                                 e_beamTable);

   // Now perform some postprocessing
   // Replace all alternate object names with tables, if available
   MetaObject *object = nullptr;
   while((object = e_beamTable.tableIterator(object)))
   {
      auto table = runtime_cast<MetaTable *>(object);
      if(!table)
         continue;
      E_postprocessThingType(table);
   }
}

//
// Get a beam for name
//
const MetaTable *E_BeamForName(const char *name)
{
   return e_beamTable.getObjectKeyAndTypeEx<MetaTable>(name);
}

const MetaTable *E_BeamForIndex(size_t index)
{
   return e_beamTable.getObjectKeyAndTypeEx<MetaTable>(index);
}
