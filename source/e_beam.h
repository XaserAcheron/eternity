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

#ifndef E_BEAM_H_
#define E_BEAM_H_

#define EDF_SEC_BEAMTYPE "beamtype"
#define EDF_SEC_BEAMDELTA "beamdelta"

#include "metaapi.h"

struct cfg_opt_t;
struct cfg_t;
class MetaTable;

static const double beamDensityDefault = 16.0;

extern MetaKeyIndex keyBeamThingType;
extern MetaKeyIndex keyBeamDensity;
extern MetaKeyIndex keyBeamTaperDist;
extern MetaKeyIndex keyBeamXDrift;
extern MetaKeyIndex keyBeamYDrift;
extern MetaKeyIndex keyBeamZDrift;
extern MetaKeyIndex keyBeamXDriftMax;
extern MetaKeyIndex keyBeamYDriftMax;
extern MetaKeyIndex keyBeamZDriftMax;

extern cfg_opt_t edf_beam_opts[];
extern cfg_opt_t edf_beam_delta_opts[];

void E_ProcessBeams(cfg_t *cfg);
const MetaTable *E_BeamForName(const char *name);
const MetaTable *E_BeamForIndex(size_t index);

#endif

// EOF
