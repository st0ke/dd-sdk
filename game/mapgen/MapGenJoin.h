// DD game project
// Copyright (C) 2026 Alexander Boldyrev <boldir@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see https://www.gnu.org/licenses/.

#ifndef __GAME_MAPGEN_JOIN_H__
#define __GAME_MAPGEN_JOIN_H__

#include "MapGenEntity.h"
#include "MapGenGeometry.h"
#include "MapGenPlan.h"
#include "idlib/MapFile.h"

class mapgenMapJoiner {
public:
    mapgenMapJoiner(idMapFile& destMap, idMapFile& sourceMap, const mapgenSlot& sourceSlot, const mapgenSlot& destSlot,
        const char* prefix);
    bool Join(idStr& status);

private:
    idMapFile& destMap;
    idMapFile& sourceMap;
    mapgenNameRemapper sourceNames;
    mapgenTransform transform;

    bool CopyWorldspawn(idStr& status);
    bool CopyEntities(idStr& status);
    idVec3 GetEntityOrigin(const idMapEntity* mapEnt) const;
};

class mapgenJoinJob {
public:
    mapgenJoinJob();
    bool Run(const char* planName, const char* outputMapName, idStr& status);

private:
    idMapFile outputMap;
    int firstMapNumEntities;
    idStr status;

    bool Generate(const char* planName, const char* outputMapName);
    idStr NormalizeMapName(const char* mapName) const;
    idStr NormalizeOutputMapName(const char* outputMapName) const;
    bool ParseMap(const char* mapName, idMapFile& mapFile);
    bool FindSlot(const idMapFile& mapFile, const char* mapName, const char* slotName, mapgenSlot& slot);
    bool ApplyJoin(const mapgenJoinPlan& plan, const mapgenJoinPlanJoin& join);
    void PrefixFirstMapEntities(const char* prefix);
    bool WriteOutputMap(const char* outputMapName);
    void SetSuccessStatus(const char* planName, int numJoins);
};

#endif /* !__GAME_MAPGEN_JOIN_H__ */
