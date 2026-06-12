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
#include "framework/FileSystem.h"
#include "idlib/Lib.h"
#include "vendor/nlohmann_json/include/nlohmann/json.hpp"

#include <exception>

namespace {

const char* const MAPGEN_PLAN_DIR = "mapgen/plans/";
const char* const MAPGEN_PLAN_EXTENSION = ".json";

using mapgenJson = nlohmann::json;

bool HasSlash(const idStr& value) {
    return value.Find('/') >= 0 || value.Find('\\') >= 0;
}

bool HasJsonExtension(const idStr& value) {
    return value.Length() >= 5 && idStr::Icmp(value.Right(5), MAPGEN_PLAN_EXTENSION) == 0;
}

idStr NormalizePlanPath(const char* planName) {
    idStr planPath = planName;
    planPath.BackSlashesToSlashes();

    if (!HasSlash(planPath) && !HasJsonExtension(planPath)) {
        planPath = idStr(MAPGEN_PLAN_DIR) + planPath;
    }
    if (!HasJsonExtension(planPath)) {
        planPath += MAPGEN_PLAN_EXTENSION;
    }
    return planPath;
}

bool ReadPlanFile(const char* planPath, idStr& contents) {
    void* buffer = NULL;
    int length = idLib::fileSystem->ReadFile(planPath, &buffer);
    if (length < 0) {
        return false;
    }

    contents = static_cast<const char*>(buffer);
    idLib::fileSystem->FreeFile(buffer);
    return true;
}

bool RequireObject(const mapgenJson& json, const char* description, idStr& status) {
    if (json.is_object()) {
        return true;
    }

    status = va("mapgen plan %s must be an object", description);
    return false;
}

bool RequireArray(const mapgenJson& json, const char* fieldName, idStr& status) {
    if (json.contains(fieldName) && json[fieldName].is_array()) {
        return true;
    }

    status = va("mapgen plan field '%s' must be an array", fieldName);
    return false;
}

bool RequireObjectField(const mapgenJson& json, const char* fieldName, idStr& status) {
    if (json.contains(fieldName) && json[fieldName].is_object()) {
        return true;
    }

    status = va("mapgen plan field '%s' must be an object", fieldName);
    return false;
}

bool RequireString(const mapgenJson& json, const char* fieldName, idStr& value, idStr& status) {
    if (json.contains(fieldName) && json[fieldName].is_string()) {
        value = json[fieldName].get<std::string>().c_str();
        if (!value.IsEmpty()) {
            return true;
        }
    }

    status = va("mapgen plan field '%s' must be a non-empty string", fieldName);
    return false;
}

bool FindMapId(const idList<idStr>& mapIds, const idStr& mapId, int& index) {
    for (int i = 0; i < mapIds.Num(); i++) {
        if (mapIds[i].Icmp(mapId) == 0) {
            index = i;
            return true;
        }
    }
    return false;
}

idStr PrefixFromMapId(const idStr& mapId) {
    return mapId + "_";
}

} // namespace

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
    status.Clear();

    if (LoadFromDisk(planName, status)) {
        return true;
    }
    if (!status.IsEmpty()) {
        return false;
    }

    status = va("unknown mapgen plan '%s'", planName);
    return false;
}

bool mapgenJoinPlan::LoadFromDisk(const char* planName, idStr& status) {
    idStr planPath = NormalizePlanPath(planName);
    idStr contents;
    if (!ReadPlanFile(planPath, contents)) {
        return false;
    }

    mapgenJson planJson;
    try {
        planJson = mapgenJson::parse(contents.c_str());
    } catch (const std::exception& exception) {
        status = va("could not parse mapgen plan '%s': %s", planPath.c_str(), exception.what());
        return false;
    }

    if (!RequireObject(planJson, "root", status) || !RequireArray(planJson, "maps", status)
        || !RequireArray(planJson, "joins", status)) {
        status = va("%s in '%s'", status.c_str(), planPath.c_str());
        return false;
    }

    const mapgenJson& mapsJson = planJson["maps"];
    idList<idStr> mapIds;
    for (int i = 0; i < static_cast<int>(mapsJson.size()); i++) {
        const mapgenJson& mapJson = mapsJson[i];
        if (!RequireObject(mapJson, "map entry", status)) {
            status = va("%s at maps[%d] in '%s'", status.c_str(), i, planPath.c_str());
            return false;
        }

        idStr mapId;
        idStr mapName;
        if (!RequireString(mapJson, "id", mapId, status) || !RequireString(mapJson, "map", mapName, status)) {
            status = va("%s at maps[%d] in '%s'", status.c_str(), i, planPath.c_str());
            return false;
        }

        int duplicateIndex = -1;
        if (FindMapId(mapIds, mapId, duplicateIndex)) {
            status = va("mapgen plan map id '%s' is duplicated in '%s'", mapId.c_str(), planPath.c_str());
            return false;
        }

        mapIds.Append(mapId);
        maps.Append(mapgenJoinPlanMap(mapName, PrefixFromMapId(mapId)));
    }

    const mapgenJson& joinsJson = planJson["joins"];
    for (int i = 0; i < static_cast<int>(joinsJson.size()); i++) {
        const mapgenJson& joinJson = joinsJson[i];
        if (!RequireObject(joinJson, "join entry", status) || !RequireObjectField(joinJson, "source", status)
            || !RequireObjectField(joinJson, "dest", status)) {
            status = va("%s at joins[%d] in '%s'", status.c_str(), i, planPath.c_str());
            return false;
        }

        idStr sourceMapId;
        idStr sourceSlotName;
        idStr destMapId;
        idStr destSlotName;
        if (!RequireString(joinJson["source"], "map", sourceMapId, status)
            || !RequireString(joinJson["source"], "slot", sourceSlotName, status)
            || !RequireString(joinJson["dest"], "map", destMapId, status)
            || !RequireString(joinJson["dest"], "slot", destSlotName, status)) {
            status = va("%s at joins[%d] in '%s'", status.c_str(), i, planPath.c_str());
            return false;
        }

        int sourceMapIndex = -1;
        int destMapIndex = -1;
        if (!FindMapId(mapIds, sourceMapId, sourceMapIndex)) {
            status = va("mapgen plan join source map '%s' is not defined at joins[%d] in '%s'", sourceMapId.c_str(), i,
                planPath.c_str());
            return false;
        }
        if (!FindMapId(mapIds, destMapId, destMapIndex)) {
            status = va("mapgen plan join dest map '%s' is not defined at joins[%d] in '%s'", destMapId.c_str(), i,
                planPath.c_str());
            return false;
        }

        joins.Append(mapgenJoinPlanJoin(sourceMapIndex, sourceSlotName, destMapIndex, destSlotName));
    }

    return true;
}
