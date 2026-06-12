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

#ifndef __GAME_MAPGEN_PLAN_H__
#define __GAME_MAPGEN_PLAN_H__

#include "idlib/MapFile.h"

class mapgenJoinPlanMap {
public:
    mapgenJoinPlanMap();
    mapgenJoinPlanMap(const char* mapName, const char* prefix);

    const char* MapName(void) const { return mapName.c_str(); }
    const char* Prefix(void) const { return prefix.c_str(); }

private:
    idStr mapName;
    idStr prefix;
};

class mapgenJoinPlanJoin {
public:
    mapgenJoinPlanJoin();
    mapgenJoinPlanJoin(int sourceMapIndex, const char* sourceSlotName, int destMapIndex, const char* destSlotName);

    int SourceMapIndex(void) const { return sourceMapIndex; }
    const char* SourceSlotName(void) const { return sourceSlotName.c_str(); }
    int DestMapIndex(void) const { return destMapIndex; }
    const char* DestSlotName(void) const { return destSlotName.c_str(); }

private:
    int sourceMapIndex;
    idStr sourceSlotName;
    int destMapIndex;
    idStr destSlotName;
};

class mapgenJoinPlan {
public:
    mapgenJoinPlan();

    bool Load(const char* planName, idStr& status);
    int NumMaps(void) const { return maps.Num(); }
    const mapgenJoinPlanMap& Map(int index) const { return maps[index]; }
    int NumJoins(void) const { return joins.Num(); }
    const mapgenJoinPlanJoin& Join(int index) const { return joins[index]; }

private:
    idList<mapgenJoinPlanMap> maps;
    idList<mapgenJoinPlanJoin> joins;

    bool LoadFromDisk(const char* planName, idStr& status);
};

#endif /* !__GAME_MAPGEN_PLAN_H__ */
