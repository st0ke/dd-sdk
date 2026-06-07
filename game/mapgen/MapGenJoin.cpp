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

#include "MapGenJoin.h"

mapgenMapJoiner::mapgenMapJoiner(idMapFile& destMap, idMapFile& sourceMap, const mapgenSlot& sourceSlot,
    const mapgenSlot& destSlot, const char* prefix)
    : destMap(destMap)
    , sourceMap(sourceMap)
    , sourceNames()
    , transform(sourceSlot, destSlot) {
    sourceNames.Build(sourceMap, sourceMap.GetNumEntities(), prefix);
}

bool mapgenMapJoiner::Join(idStr& status) {
    if (!CopyWorldspawn(status)) {
        return false;
    }
    return CopyEntities(status);
}

bool mapgenMapJoiner::CopyWorldspawn(idStr& status) {
    idMapEntity* sourceWorldspawn = sourceMap.GetEntity(0);
    idMapEntity* destWorldspawn = destMap.GetEntity(0);
    idVec3 sourceOrigin = GetEntityOrigin(sourceWorldspawn);
    idVec3 destOrigin = GetEntityOrigin(destWorldspawn);
    mapgenPrimitiveCloner cloner(transform, sourceOrigin, destOrigin);

    if (!cloner.ClonePrimitives(destWorldspawn, sourceWorldspawn)) {
        status = "could not copy source worldspawn primitives";
        return false;
    }
    return true;
}

bool mapgenMapJoiner::CopyEntities(idStr& status) {
    for (int i = 1; i < sourceMap.GetNumEntities(); i++) {
        mapgenEntityDuplicator duplicator(sourceMap.GetEntity(i), transform, sourceNames);
        idMapEntity* clonedEntity = duplicator.Clone();
        if (clonedEntity == NULL) {
            status = va("could not copy source entity %d primitives", i);
            return false;
        }
        destMap.AddEntity(clonedEntity);
    }
    return true;
}

idVec3 mapgenMapJoiner::GetEntityOrigin(const idMapEntity* mapEnt) const {
    idVec3 origin;
    mapEnt->epairs.GetVector("origin", "0 0 0", origin);
    return origin;
}

mapgenJoinJob::mapgenJoinJob()
    : outputMap()
    , firstMapNumEntities(0)
    , status() { }

bool mapgenJoinJob::Run(const char* planName, const char* outputMapName, idStr& status) {
    bool result = Generate(planName, outputMapName);

    status = this->status;
    return result;
}

bool mapgenJoinJob::Generate(const char* planName, const char* outputMapName) {
    mapgenJoinPlan plan;
    if (!plan.Load(planName, status)) {
        return false;
    }

    if (plan.NumMaps() <= 0) {
        status = va("mapgen plan '%s' has no maps", planName);
        return false;
    }

    if (!ParseMap(plan.Map(0).MapName(), outputMap)) {
        return false;
    }
    firstMapNumEntities = outputMap.GetNumEntities();

    for (int i = 0; i < plan.NumJoins(); i++) {
        if (!ApplyJoin(plan, plan.Join(i))) {
            return false;
        }
    }
    PrefixFirstMapEntities(plan.Map(0).Prefix());

    if (!WriteOutputMap(outputMapName)) {
        return false;
    }

    SetSuccessStatus(planName, plan.NumJoins());
    return true;
}

idStr mapgenJoinJob::NormalizeMapName(const char* mapName) const {
    idStr normalizedName = mapName;
    normalizedName.BackSlashesToSlashes();
    normalizedName.StripFileExtension();

    if (normalizedName.Icmpn("maps/", 5) != 0) {
        normalizedName = "maps/" + normalizedName;
    }
    return normalizedName;
}

idStr mapgenJoinJob::NormalizeOutputMapName(const char* outputMapName) const {
    idStr normalizedName = outputMapName;
    normalizedName.BackSlashesToSlashes();
    normalizedName.StripFileExtension();
    return normalizedName;
}

bool mapgenJoinJob::ParseMap(const char* mapName, idMapFile& mapFile) {
    idStr normalizedName = NormalizeMapName(mapName);
    if (!mapFile.Parse(normalizedName, true)) {
        status = va("could not parse '%s.map'", normalizedName.c_str());
        return false;
    }

    if (mapFile.GetNumEntities() <= 0) {
        status = va("map '%s.map' has no worldspawn", normalizedName.c_str());
        return false;
    }

    return true;
}

bool mapgenJoinJob::FindSlot(const idMapFile& mapFile, const char* mapName, const char* slotName, mapgenSlot& slot) {
    mapgenSlotFinder finder(mapFile, slotName);
    if (finder.Find(slot, status)) {
        return true;
    }

    idStr slotStatus = status;
    status = va("map '%s': %s", mapName, slotStatus.c_str());
    return false;
}

bool mapgenJoinJob::ApplyJoin(const mapgenJoinPlan& plan, const mapgenJoinPlanJoin& join) {
    if (join.SourceMapIndex() < 0 || join.SourceMapIndex() >= plan.NumMaps() || join.DestMapIndex() < 0
        || join.DestMapIndex() >= plan.NumMaps()) {
        status = "mapgen plan join has an invalid map index";
        return false;
    }
    if (join.DestMapIndex() != 0) {
        status = "mapgen plan joins must currently target the first map instance";
        return false;
    }

    const mapgenJoinPlanMap& sourcePlanMap = plan.Map(join.SourceMapIndex());
    const mapgenJoinPlanMap& destPlanMap = plan.Map(join.DestMapIndex());
    idMapFile sourceMap;
    if (!ParseMap(sourcePlanMap.MapName(), sourceMap)) {
        return false;
    }

    mapgenSlot sourceSlot;
    if (!FindSlot(sourceMap, sourcePlanMap.MapName(), join.SourceSlotName(), sourceSlot)) {
        return false;
    }

    mapgenSlot destSlot;
    if (!FindSlot(outputMap, destPlanMap.MapName(), join.DestSlotName(), destSlot)) {
        return false;
    }

    mapgenMapJoiner joiner(outputMap, sourceMap, sourceSlot, destSlot, sourcePlanMap.Prefix());
    if (joiner.Join(status)) {
        return true;
    }

    idStr joinStatus = status;
    status = va("could not join %s:%s to %s:%s: %s", sourcePlanMap.MapName(), join.SourceSlotName(),
        destPlanMap.MapName(), join.DestSlotName(), joinStatus.c_str());
    return false;
}

void mapgenJoinJob::PrefixFirstMapEntities(const char* prefix) {
    mapgenNameRemapper firstMapNames;
    firstMapNames.Build(outputMap, firstMapNumEntities, prefix);

    for (int i = 1; i < firstMapNumEntities; i++) {
        firstMapNames.Apply(outputMap.GetEntity(i)->epairs);
    }
}

bool mapgenJoinJob::WriteOutputMap(const char* outputMapName) {
    idStr outputMapBase = NormalizeOutputMapName(outputMapName);
    idStr outputPath = outputMapBase;
    outputPath.SetFileExtension("map");

    if (outputMapBase.IsEmpty()) {
        status = "output map file is empty";
        return false;
    }

    if (!outputMap.Write(outputMapBase, ".map")) {
        status = va("could not write '%s'", outputPath.c_str());
        return false;
    }
    return true;
}

void mapgenJoinJob::SetSuccessStatus(const char* planName, int numJoins) {
    status = va("generated plan '%s' with %d joins", planName, numJoins);
}
