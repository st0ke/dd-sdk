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

#include "sys/platform.h"

#include "Plan.h"

static const char* const MAPGEN_BUILTIN_PLAN_NAME = "gate2_testgg";
static const char* const MAPGEN_TESTGG_PREFIX = "m0__";
static const char* const MAPGEN_FIRST_GATE_PREFIX = "m1__";
static const char* const MAPGEN_SECOND_GATE_PREFIX = "m2__";

mapgenJoinPlanMap::mapgenJoinPlanMap()
    : mapName("")
    , prefix("") { }

mapgenJoinPlanMap::mapgenJoinPlanMap(const char* mapName, const char* prefix)
    : mapName(mapName)
    , prefix(prefix) { }

mapgenJoinPlanJoin::mapgenJoinPlanJoin()
    : sourceMapIndex(-1)
    , sourceSlotName("")
    , destMapIndex(-1)
    , destSlotName("") { }

mapgenJoinPlanJoin::mapgenJoinPlanJoin(
    int sourceMapIndex, const char* sourceSlotName, int destMapIndex, const char* destSlotName)
    : sourceMapIndex(sourceMapIndex)
    , sourceSlotName(sourceSlotName)
    , destMapIndex(destMapIndex)
    , destSlotName(destSlotName) { }

mapgenJoinPlan::mapgenJoinPlan()
    : maps()
    , joins() { }

bool mapgenJoinPlan::Load(const char* planName, idStr& status) {
    maps.Clear();
    joins.Clear();

    if (LoadFromDisk(planName, status)) {
        return true;
    }
    if (idStr::Icmp(planName, MAPGEN_BUILTIN_PLAN_NAME) == 0) {
        BuildGate2TestggPlan();
        return true;
    }

    status = va("unknown mapgen plan '%s'", planName);
    return false;
}

bool mapgenJoinPlan::LoadFromDisk(const char* planName, idStr& status) {
    // Future plan-file parsing enters here; built-in plans remain the fallback.
    (void) planName;
    (void) status;
    return false;
}

void mapgenJoinPlan::BuildGate2TestggPlan(void) {
    maps.Append(mapgenJoinPlanMap("mapgen/testgg", MAPGEN_TESTGG_PREFIX));
    maps.Append(mapgenJoinPlanMap("mapgen/gate2", MAPGEN_FIRST_GATE_PREFIX));
    maps.Append(mapgenJoinPlanMap("mapgen/gate2", MAPGEN_SECOND_GATE_PREFIX));
    joins.Append(mapgenJoinPlanJoin(1, "slot_0", 0, "slot_gg0"));
    joins.Append(mapgenJoinPlanJoin(2, "slot_0", 0, "slot_gg1"));
}
