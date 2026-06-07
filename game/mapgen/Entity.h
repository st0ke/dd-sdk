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

#ifndef __GAME_MAPGEN_ENTITY_H__
#define __GAME_MAPGEN_ENTITY_H__

#include "Geometry.h"
#include "idlib/MapFile.h"

class mapgenNamePair {
public:
    mapgenNamePair();
    mapgenNamePair(const char* oldName, const char* prefix);

    bool Matches(const char* name) const;
    const char* NewName(void) const { return newName.c_str(); }

private:
    idStr oldName;
    idStr newName;
};

class mapgenNameRemapper {
public:
    void Build(idMapFile& mapFile, int numEntities, const char* prefix);
    void Apply(idDict& epairs) const;

private:
    idList<mapgenNamePair> namePairs;
    idList<mapgenNamePair> groupPairs;

    const char* FindName(const idList<mapgenNamePair>& pairs, const char* oldName) const;
    bool IsNumericString(const char* value) const;
    bool KeyHasPrefix(const idKeyValue* kv, const char* prefix) const;
    void RetargetValue(idDict& epairs, const idKeyValue* kv, const idList<mapgenNamePair>& pairs) const;
    void RetargetExactKey(idDict& epairs, const char* key) const;
};

class mapgenSpawnArgTransformer {
public:
    mapgenSpawnArgTransformer(const mapgenTransform& transform);
    void Apply(idDict& epairs) const;

private:
    const mapgenTransform& transform;

    void RotateMatrixKey(idDict& epairs, const char* key) const;
    void RotateVectorKey(idDict& epairs, const char* key) const;
    void RotatePointKey(idDict& epairs, const char* key) const;
    void RotateAngleKey(idDict& epairs, const char* key) const;
    void RotateMoveDirKey(idDict& epairs, const char* key) const;
};

class mapgenPrimitiveCloner {
public:
    mapgenPrimitiveCloner(const mapgenTransform& transform, const idVec3& srcOrigin, const idVec3& dstOrigin);
    bool ClonePrimitives(idMapEntity* dstEnt, idMapEntity* srcEnt) const;

private:
    const mapgenTransform& transform;
    idVec3 srcOrigin;
    idVec3 dstOrigin;

    idMapBrushSide* CloneBrushSide(idMapBrushSide* srcSide) const;
    idMapBrush* CloneBrush(idMapBrush* srcBrush) const;
    idMapPatch* ClonePatch(idMapPatch* srcPatch) const;
    idMapPrimitive* ClonePrimitive(idMapPrimitive* srcPrim) const;
    bool NormalizePlane(idPlane& plane) const;
    idPlane LocalPlaneToWorld(const idPlane& localPlane, const idVec3& origin) const;
    idPlane WorldPlaneToLocal(const idPlane& worldPlane, const idVec3& origin) const;
};

class mapgenEntityDuplicator {
public:
    mapgenEntityDuplicator(idMapEntity* srcEnt, const mapgenTransform& transform, const mapgenNameRemapper& remapper);
    idMapEntity* Clone(void) const;

private:
    idMapEntity* srcEnt;
    const mapgenTransform& transform;
    const mapgenNameRemapper& remapper;
    mapgenSpawnArgTransformer spawnArgTransformer;
    idVec3 srcOrigin;
    idVec3 dstOrigin;

    idVec3 GetEntityOrigin(const idMapEntity* mapEnt) const;
};

#endif /* !__GAME_MAPGEN_ENTITY_H__ */
