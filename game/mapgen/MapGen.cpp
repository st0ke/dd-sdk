/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 2026 st0ke <boldir@gmail.com>

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of
these additional terms immediately following the terms and conditions of the GNU General Public License which
accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software
LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "idlib/MapFile.h"
#include "idlib/geometry/Winding.h"
#include "idlib/math/Rotation.h"
#include "sys/platform.h"

#include "MapGen.h"

static const float MAPGEN_PLANE_DIST_EPSILON = 0.001f;
static const float MAPGEN_VERTICAL_SLOT_EPSILON = 0.001f;

static const char* const MAPGEN_SLOT_MATERIAL = "textures/common/mapgen_slot";
static const char* const MAPGEN_OUTPUT_MAP = "maps/mapgen/current";
static const char* const MAPGEN_BUILTIN_PLAN_NAME = "gate2_testgg";
static const char* const MAPGEN_TESTGG_PREFIX = "m0__";
static const char* const MAPGEN_FIRST_GATE_PREFIX = "m1__";
static const char* const MAPGEN_SECOND_GATE_PREFIX = "m2__";

class mapgenTransform;

class mapgenSlot {
public:
    mapgenSlot();

    bool LoadFromSide(const char* slotName, int entityNum, int primitiveNum, int sideNum, idMapBrush* brush,
        const idVec3& origin, idStr& status);
    bool IsVertical(void) const;
    mapgenTransform BuildJoinTransform(const mapgenSlot& destSlot) const;

    const idPlane& WorldPlane(void) const { return plane; }
    const idVec3& Anchor(void) const { return anchor; }
    int EntityNum(void) const { return entityNum; }
    int PrimitiveNum(void) const { return primitiveNum; }
    int SideNum(void) const { return sideNum; }

private:
    idStr name;
    idPlane plane;
    idVec3 anchor;
    int entityNum;
    int primitiveNum;
    int sideNum;

    bool NormalizePlane(idPlane& plane) const;
    idPlane LocalPlaneToWorld(const idPlane& localPlane, const idVec3& origin) const;
    bool CalculateFaceCenter(idMapBrush* brush, int sideNum, const idVec3& origin, idVec3& center) const;
};

class mapgenTransform {
public:
    mapgenTransform();
    mapgenTransform(const mapgenSlot& sourceSlot, const mapgenSlot& destSlot);

    idVec3 TransformVector(const idVec3& v) const;
    idVec3 TransformPoint(const idVec3& p) const;
    idPlane TransformPlane(const idPlane& plane) const;

private:
    idMat3 rotation;
    idVec3 translation;

    void SetJoin(const mapgenSlot& sourceSlot, const mapgenSlot& destSlot);
    bool NormalizePlane(idPlane& plane) const;
};

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

class mapgenSlotFinder {
public:
    mapgenSlotFinder(const idMapFile& mapFile, const char* slotName);
    bool Find(mapgenSlot& slot, idStr& status) const;

private:
    const idMapFile& mapFile;
    idStr slotName;

    idVec3 GetEntityOrigin(const idMapEntity* mapEnt) const;
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
    void BuildGate2TestggPlan(void);
};

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

class mapgenDMapJob {
public:
    mapgenDMapJob();
    bool Run(const char* planName, idStr& outputMapName, idStr& status);

private:
    idMapFile outputMap;
    int firstMapNumEntities;
    idStr status;

    bool Generate(const char* planName, idStr& outputMapName);
    idStr NormalizeMapName(const char* mapName) const;
    bool ParseMap(const char* mapName, idMapFile& mapFile);
    bool FindSlot(const idMapFile& mapFile, const char* mapName, const char* slotName, mapgenSlot& slot);
    bool ApplyJoin(const mapgenJoinPlan& plan, const mapgenJoinPlanJoin& join);
    void PrefixFirstMapEntities(const char* prefix);
    bool WriteOutputMap(idStr& outputMapName);
    void SetSuccessStatus(const char* planName, int numJoins);
};

mapgenSlot::mapgenSlot()
    : name("")
    , anchor(vec3_origin)
    , entityNum(-1)
    , primitiveNum(-1)
    , sideNum(-1) { }

bool mapgenSlot::NormalizePlane(idPlane& plane) const {
    idVec3 normal = plane.Normal();
    float length = normal.Normalize();

    if (length <= 0.0f) {
        return false;
    }

    plane.SetNormal(normal);
    plane[3] /= length;
    plane.FixDegeneracies(MAPGEN_PLANE_DIST_EPSILON);
    return true;
}

idPlane mapgenSlot::LocalPlaneToWorld(const idPlane& localPlane, const idVec3& origin) const {
    idPlane worldPlane = localPlane;
    NormalizePlane(worldPlane);
    worldPlane[3] -= origin * worldPlane.Normal();
    return worldPlane;
}

bool mapgenSlot::CalculateFaceCenter(idMapBrush* brush, int sideNum, const idVec3& origin, idVec3& center) const {
    idWinding winding(brush->GetSide(sideNum)->GetPlane());

    for (int i = 0; i < brush->GetNumSides(); i++) {
        if (i == sideNum) {
            continue;
        }
        if (!winding.ClipInPlace(-brush->GetSide(i)->GetPlane(), ON_EPSILON, true)) {
            return false;
        }
    }

    if (winding.GetNumPoints() < 3 || winding.IsTiny() || winding.IsHuge()) {
        return false;
    }

    center = winding.GetCenter() + origin;
    return true;
}

bool mapgenSlot::LoadFromSide(const char* slotName, int entityNum, int primitiveNum, int sideNum, idMapBrush* brush,
    const idVec3& origin, idStr& status) {
    idMapBrushSide* side = brush->GetSide(sideNum);

    name = slotName;
    plane = LocalPlaneToWorld(side->GetPlane(), origin);
    if (!NormalizePlane(plane)) {
        status = va("slot '%s' face has an invalid plane", slotName);
        return false;
    }
    if (!IsVertical()) {
        status = va("slot '%s' face must be vertical", slotName);
        return false;
    }

    if (!CalculateFaceCenter(brush, sideNum, origin, anchor)) {
        status = va("slot '%s' face must form a finite brush polygon", slotName);
        return false;
    }
    this->entityNum = entityNum;
    this->primitiveNum = primitiveNum;
    this->sideNum = sideNum;
    return true;
}

bool mapgenSlot::IsVertical(void) const {
    return idMath::Fabs(plane.Normal().z) <= MAPGEN_VERTICAL_SLOT_EPSILON;
}

mapgenTransform mapgenSlot::BuildJoinTransform(const mapgenSlot& destSlot) const {
    return mapgenTransform(*this, destSlot);
}

mapgenTransform::mapgenTransform()
    : rotation(mat3_identity)
    , translation(vec3_origin) { }

mapgenTransform::mapgenTransform(const mapgenSlot& sourceSlot, const mapgenSlot& destSlot)
    : rotation(mat3_identity)
    , translation(vec3_origin) {
    SetJoin(sourceSlot, destSlot);
}

void mapgenTransform::SetJoin(const mapgenSlot& sourceSlot, const mapgenSlot& destSlot) {
    idVec3 sourceNormal = sourceSlot.WorldPlane().Normal();
    idVec3 joinedDestNormal = -destSlot.WorldPlane().Normal();
    float sourceYaw = idMath::ATan(sourceNormal.y, sourceNormal.x);
    float destYaw = idMath::ATan(joinedDestNormal.y, joinedDestNormal.x);
    float yaw = RAD2DEG(destYaw - sourceYaw);

    rotation = idAngles(0.0f, yaw, 0.0f).ToMat3();
    translation = destSlot.Anchor() - rotation * sourceSlot.Anchor();
}

bool mapgenTransform::NormalizePlane(idPlane& plane) const {
    idVec3 normal = plane.Normal();
    float length = normal.Normalize();

    if (length <= 0.0f) {
        return false;
    }

    plane.SetNormal(normal);
    plane[3] /= length;
    plane.FixDegeneracies(MAPGEN_PLANE_DIST_EPSILON);
    return true;
}

idVec3 mapgenTransform::TransformVector(const idVec3& v) const {
    return rotation * v;
}

idVec3 mapgenTransform::TransformPoint(const idVec3& p) const {
    return TransformVector(p) + translation;
}

idPlane mapgenTransform::TransformPlane(const idPlane& plane) const {
    idPlane normalizedPlane = plane;
    NormalizePlane(normalizedPlane);

    idVec3 point = normalizedPlane.Normal() * normalizedPlane.Dist();
    idVec3 rotatedPoint = TransformPoint(point);
    idVec3 rotatedNormal = TransformVector(normalizedPlane.Normal());

    idPlane rotatedPlane;
    rotatedPlane.SetNormal(rotatedNormal);
    rotatedPlane.Normalize();
    rotatedPlane.FitThroughPoint(rotatedPoint);
    rotatedPlane.FixDegeneracies(MAPGEN_PLANE_DIST_EPSILON);
    return rotatedPlane;
}

mapgenNamePair::mapgenNamePair() { }

mapgenNamePair::mapgenNamePair(const char* oldName, const char* prefix)
    : oldName(oldName)
    , newName(prefix) {
    newName += oldName;
}

bool mapgenNamePair::Matches(const char* name) const {
    return oldName.Icmp(name) == 0;
}

mapgenSlotFinder::mapgenSlotFinder(const idMapFile& mapFile, const char* slotName)
    : mapFile(mapFile)
    , slotName(slotName) { }

idVec3 mapgenSlotFinder::GetEntityOrigin(const idMapEntity* mapEnt) const {
    idVec3 origin;
    mapEnt->epairs.GetVector("origin", "0 0 0", origin);
    return origin;
}

bool mapgenSlotFinder::Find(mapgenSlot& slot, idStr& status) const {
    int numNamedSlots = 0;

    for (int entityNum = 1; entityNum < mapFile.GetNumEntities(); entityNum++) {
        idMapEntity* mapEnt = mapFile.GetEntity(entityNum);
        if (idStr::Icmp(mapEnt->epairs.GetString("classname"), "func_static") != 0) {
            continue;
        }
        if (idStr::Icmp(mapEnt->epairs.GetString("name"), slotName.c_str()) != 0) {
            continue;
        }

        numNamedSlots++;
        if (numNamedSlots > 1) {
            status = va("found multiple func_static slots named '%s'", slotName.c_str());
            return false;
        }

        idVec3 origin = GetEntityOrigin(mapEnt);
        int numSlotFaces = 0;

        for (int primitiveNum = 0; primitiveNum < mapEnt->GetNumPrimitives(); primitiveNum++) {
            idMapPrimitive* mapPrim = mapEnt->GetPrimitive(primitiveNum);
            if (mapPrim->GetType() != idMapPrimitive::TYPE_BRUSH) {
                continue;
            }

            idMapBrush* brush = static_cast<idMapBrush*>(mapPrim);
            for (int sideNum = 0; sideNum < brush->GetNumSides(); sideNum++) {
                idMapBrushSide* side = brush->GetSide(sideNum);
                if (idStr::Icmp(side->GetMaterial(), MAPGEN_SLOT_MATERIAL) != 0) {
                    continue;
                }

                numSlotFaces++;
                if (numSlotFaces > 1) {
                    status = va("slot '%s' has multiple '%s' faces", slotName.c_str(), MAPGEN_SLOT_MATERIAL);
                    return false;
                }

                if (!slot.LoadFromSide(slotName.c_str(), entityNum, primitiveNum, sideNum, brush, origin, status)) {
                    return false;
                }
            }
        }

        if (numSlotFaces == 0) {
            status = va("slot '%s' has no face using material '%s'", slotName.c_str(), MAPGEN_SLOT_MATERIAL);
            return false;
        }
    }

    if (numNamedSlots == 1) {
        return true;
    }

    status = va("could not find func_static slot named '%s'", slotName.c_str());
    return false;
}

mapgenPrimitiveCloner::mapgenPrimitiveCloner(
    const mapgenTransform& transform, const idVec3& srcOrigin, const idVec3& dstOrigin)
    : transform(transform)
    , srcOrigin(srcOrigin)
    , dstOrigin(dstOrigin) { }

bool mapgenPrimitiveCloner::NormalizePlane(idPlane& plane) const {
    idVec3 normal = plane.Normal();
    float length = normal.Normalize();

    if (length <= 0.0f) {
        return false;
    }

    plane.SetNormal(normal);
    plane[3] /= length;
    plane.FixDegeneracies(MAPGEN_PLANE_DIST_EPSILON);
    return true;
}

idPlane mapgenPrimitiveCloner::LocalPlaneToWorld(const idPlane& localPlane, const idVec3& origin) const {
    idPlane worldPlane = localPlane;
    NormalizePlane(worldPlane);
    worldPlane[3] -= origin * worldPlane.Normal();
    return worldPlane;
}

idPlane mapgenPrimitiveCloner::WorldPlaneToLocal(const idPlane& worldPlane, const idVec3& origin) const {
    idPlane localPlane = worldPlane;
    NormalizePlane(localPlane);
    localPlane[3] += origin * localPlane.Normal();
    return localPlane;
}

idMapBrushSide* mapgenPrimitiveCloner::CloneBrushSide(idMapBrushSide* srcSide) const {
    idMapBrushSide* dstSide = new idMapBrushSide();
    idVec3 texMat[2];

    srcSide->GetTextureMatrix(texMat[0], texMat[1]);
    dstSide->SetTextureMatrix(texMat);
    dstSide->SetMaterial(srcSide->GetMaterial());

    idPlane worldPlane = LocalPlaneToWorld(srcSide->GetPlane(), srcOrigin);
    idPlane rotatedWorldPlane = transform.TransformPlane(worldPlane);
    dstSide->SetPlane(WorldPlaneToLocal(rotatedWorldPlane, dstOrigin));

    return dstSide;
}

idMapBrush* mapgenPrimitiveCloner::CloneBrush(idMapBrush* srcBrush) const {
    idMapBrush* dstBrush = new idMapBrush();
    dstBrush->epairs = srcBrush->epairs;

    for (int i = 0; i < srcBrush->GetNumSides(); i++) {
        dstBrush->AddSide(CloneBrushSide(srcBrush->GetSide(i)));
    }

    return dstBrush;
}

idMapPatch* mapgenPrimitiveCloner::ClonePatch(idMapPatch* srcPatch) const {
    idMapPatch* dstPatch = new idMapPatch(srcPatch->GetWidth(), srcPatch->GetHeight());
    dstPatch->epairs = srcPatch->epairs;
    dstPatch->SetMaterial(srcPatch->GetMaterial());
    dstPatch->SetSize(srcPatch->GetWidth(), srcPatch->GetHeight());
    dstPatch->SetHorzSubdivisions(srcPatch->GetHorzSubdivisions());
    dstPatch->SetVertSubdivisions(srcPatch->GetVertSubdivisions());
    dstPatch->SetExplicitlySubdivided(srcPatch->GetExplicitlySubdivided());

    for (int i = 0; i < srcPatch->GetWidth() * srcPatch->GetHeight(); i++) {
        idDrawVert vert = (*srcPatch)[i];
        vert.xyz = transform.TransformPoint(vert.xyz + srcOrigin) - dstOrigin;
        vert.normal = transform.TransformVector(vert.normal);
        vert.tangents[0] = transform.TransformVector(vert.tangents[0]);
        vert.tangents[1] = transform.TransformVector(vert.tangents[1]);
        (*dstPatch)[i] = vert;
    }

    return dstPatch;
}

idMapPrimitive* mapgenPrimitiveCloner::ClonePrimitive(idMapPrimitive* srcPrim) const {
    switch (srcPrim->GetType()) {
    case idMapPrimitive::TYPE_BRUSH:
        return CloneBrush(static_cast<idMapBrush*>(srcPrim));
    case idMapPrimitive::TYPE_PATCH:
        return ClonePatch(static_cast<idMapPatch*>(srcPrim));
    default:
        return NULL;
    }
}

mapgenSpawnArgTransformer::mapgenSpawnArgTransformer(const mapgenTransform& transform)
    : transform(transform) { }

void mapgenSpawnArgTransformer::RotateMatrixKey(idDict& epairs, const char* key) const {
    idMat3 axis;

    if (!epairs.GetMatrix(key, NULL, axis)) {
        return;
    }

    for (int i = 0; i < 3; i++) {
        axis[i] = transform.TransformVector(axis[i]);
        axis[i].FixDegenerateNormal();
    }
    epairs.SetMatrix(key, axis);
}

void mapgenSpawnArgTransformer::RotateVectorKey(idDict& epairs, const char* key) const {
    idVec3 value;

    if (epairs.GetVector(key, NULL, value)) {
        epairs.SetVector(key, transform.TransformVector(value));
    }
}

void mapgenSpawnArgTransformer::RotatePointKey(idDict& epairs, const char* key) const {
    idVec3 value;

    if (epairs.GetVector(key, NULL, value)) {
        epairs.SetVector(key, transform.TransformPoint(value));
    }
}

void mapgenSpawnArgTransformer::RotateAngleKey(idDict& epairs, const char* key) const {
    const idKeyValue* kv = epairs.FindKey(key);

    if (kv == NULL) {
        return;
    }

    float yaw = atof(kv->GetValue());
    idAngles angles(0.0f, yaw, 0.0f);
    idVec3 forward = transform.TransformVector(angles.ToForward());
    epairs.SetFloat(key, idMath::AngleNormalize360(forward.ToYaw()));
}

void mapgenSpawnArgTransformer::RotateMoveDirKey(idDict& epairs, const char* key) const {
    const idKeyValue* kv = epairs.FindKey(key);

    if (kv == NULL) {
        return;
    }

    float yaw = atof(kv->GetValue());
    idVec3 direction;
    if (yaw == -1.0f) {
        direction = idVec3(0.0f, 0.0f, 1.0f);
    } else if (yaw == -2.0f) {
        direction = idVec3(0.0f, 0.0f, -1.0f);
    } else {
        direction = idAngles(0.0f, yaw, 0.0f).ToForward();
    }

    direction = transform.TransformVector(direction);
    direction.Normalize();
    direction.FixDegenerateNormal();

    if (direction.z > 0.999f) {
        epairs.Set(key, "-1");
    } else if (direction.z < -0.999f) {
        epairs.Set(key, "-2");
    } else {
        epairs.SetFloat(key, idMath::AngleNormalize360(direction.ToYaw()));
    }
}

bool mapgenNameRemapper::IsNumericString(const char* value) const {
    char* end;

    if (value == NULL || value[0] == '\0') {
        return false;
    }

    strtod(value, &end);
    return (end != value && *end == '\0');
}

bool mapgenNameRemapper::KeyHasPrefix(const idKeyValue* kv, const char* prefix) const {
    return (kv != NULL && kv->GetKey().Icmpn(prefix, idStr::Length(prefix)) == 0);
}

const char* mapgenNameRemapper::FindName(const idList<mapgenNamePair>& pairs, const char* oldName) const {
    for (int i = 0; i < pairs.Num(); i++) {
        if (pairs[i].Matches(oldName)) {
            return pairs[i].NewName();
        }
    }
    return NULL;
}

void mapgenNameRemapper::RetargetValue(
    idDict& epairs, const idKeyValue* kv, const idList<mapgenNamePair>& pairs) const {
    const char* newName = FindName(pairs, kv->GetValue());
    if (newName != NULL) {
        epairs.Set(kv->GetKey(), newName);
    }
}

void mapgenNameRemapper::RetargetExactKey(idDict& epairs, const char* key) const {
    const idKeyValue* kv = epairs.FindKey(key);
    if (kv != NULL) {
        RetargetValue(epairs, kv, namePairs);
    }
}

void mapgenNameRemapper::Build(idMapFile& mapFile, int numEntities, const char* prefix) {
    namePairs.Clear();
    groupPairs.Clear();

    for (int i = 1; i < numEntities; i++) {
        const char* oldName = mapFile.GetEntity(i)->epairs.GetString("name");
        if (oldName[0] != '\0') {
            namePairs.Append(mapgenNamePair(oldName, prefix));
        }

        const idKeyValue* kv = mapFile.GetEntity(i)->epairs.FindKey("team");
        if (kv == NULL || kv->GetValue()[0] == '\0' || IsNumericString(kv->GetValue())) {
            continue;
        }

        if (FindName(groupPairs, kv->GetValue()) != NULL) {
            continue;
        }

        groupPairs.Append(mapgenNamePair(kv->GetValue(), prefix));
    }
}

void mapgenNameRemapper::Apply(idDict& epairs) const {
    static const char* const retargetPrefixes[] = { "target", "guiTarget", "buddy" };
    static const char* const retargetExactKeys[] = { "bind", "cameraTarget", "model", "syncLock" };

    const char* oldName = epairs.GetString("name");
    const char* newName = FindName(namePairs, oldName);
    if (newName != NULL) {
        epairs.Set("name", newName);
    }

    int numKeyVals = epairs.GetNumKeyVals();
    for (int i = 0; i < numKeyVals; i++) {
        const idKeyValue* kv = epairs.GetKeyVal(i);
        bool shouldRetarget = false;

        for (int prefixIndex = 0; prefixIndex < sizeof(retargetPrefixes) / sizeof(retargetPrefixes[0]); prefixIndex++) {
            if (KeyHasPrefix(kv, retargetPrefixes[prefixIndex])) {
                shouldRetarget = true;
                break;
            }
        }
        if (!shouldRetarget) {
            continue;
        }
        RetargetValue(epairs, kv, namePairs);
    }

    for (int i = 0; i < sizeof(retargetExactKeys) / sizeof(retargetExactKeys[0]); i++) {
        RetargetExactKey(epairs, retargetExactKeys[i]);
    }

    const idKeyValue* team = epairs.FindKey("team");
    if (team != NULL) {
        RetargetValue(epairs, team, groupPairs);
    }
}

void mapgenSpawnArgTransformer::Apply(idDict& epairs) const {
    static const char* const pointKeys[] = { "light_origin" };
    static const char* const vectorKeys[]
        = { "light_target", "light_right", "light_up", "light_start", "light_end", "light_center" };
    static const char* const matrixKeys[] = { "rotation", "light_rotation" };

    for (int i = 0; i < sizeof(pointKeys) / sizeof(pointKeys[0]); i++) {
        RotatePointKey(epairs, pointKeys[i]);
    }
    for (int i = 0; i < sizeof(vectorKeys) / sizeof(vectorKeys[0]); i++) {
        RotateVectorKey(epairs, vectorKeys[i]);
    }
    for (int i = 0; i < sizeof(matrixKeys) / sizeof(matrixKeys[0]); i++) {
        RotateMatrixKey(epairs, matrixKeys[i]);
    }
    RotateAngleKey(epairs, "angle");
    RotateMoveDirKey(epairs, "movedir");
}

bool mapgenPrimitiveCloner::ClonePrimitives(idMapEntity* dstEnt, idMapEntity* srcEnt) const {
    idList<idMapPrimitive*> clonedPrimitives;
    int numPrimitives = srcEnt->GetNumPrimitives();

    for (int i = 0; i < numPrimitives; i++) {
        idMapPrimitive* dstPrim = ClonePrimitive(srcEnt->GetPrimitive(i));
        if (dstPrim == NULL) {
            clonedPrimitives.DeleteContents(true);
            return false;
        }
        clonedPrimitives.Append(dstPrim);
    }

    for (int i = 0; i < clonedPrimitives.Num(); i++) {
        dstEnt->AddPrimitive(clonedPrimitives[i]);
    }
    clonedPrimitives.Clear();
    return true;
}

mapgenEntityDuplicator::mapgenEntityDuplicator(
    idMapEntity* srcEnt, const mapgenTransform& transform, const mapgenNameRemapper& remapper)
    : srcEnt(srcEnt)
    , transform(transform)
    , remapper(remapper)
    , spawnArgTransformer(transform)
    , srcOrigin(vec3_origin)
    , dstOrigin(vec3_origin) {
    this->srcOrigin = GetEntityOrigin(srcEnt);
    this->dstOrigin = transform.TransformPoint(this->srcOrigin);
}

idVec3 mapgenEntityDuplicator::GetEntityOrigin(const idMapEntity* mapEnt) const {
    idVec3 origin;
    mapEnt->epairs.GetVector("origin", "0 0 0", origin);
    return origin;
}

idMapEntity* mapgenEntityDuplicator::Clone(void) const {
    idMapEntity* dstEnt = new idMapEntity();
    dstEnt->epairs = srcEnt->epairs;
    dstEnt->epairs.SetVector("origin", dstOrigin);

    remapper.Apply(dstEnt->epairs);
    spawnArgTransformer.Apply(dstEnt->epairs);

    mapgenPrimitiveCloner cloner(transform, srcOrigin, dstOrigin);
    if (!cloner.ClonePrimitives(dstEnt, srcEnt)) {
        delete dstEnt;
        return NULL;
    }

    return dstEnt;
}

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

mapgenDMapJob::mapgenDMapJob()
    : outputMap()
    , firstMapNumEntities(0)
    , status() { }

bool mapgenDMapJob::Run(const char* planName, idStr& outputMapName, idStr& status) {
    bool result = Generate(planName, outputMapName);

    status = this->status;
    return result;
}

bool mapgenDMapJob::Generate(const char* planName, idStr& outputMapName) {
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

idStr mapgenDMapJob::NormalizeMapName(const char* mapName) const {
    idStr normalizedName = mapName;
    normalizedName.BackSlashesToSlashes();
    normalizedName.StripFileExtension();

    if (normalizedName.Icmpn("maps/", 5) != 0) {
        normalizedName = "maps/" + normalizedName;
    }
    return normalizedName;
}

bool mapgenDMapJob::ParseMap(const char* mapName, idMapFile& mapFile) {
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

bool mapgenDMapJob::FindSlot(const idMapFile& mapFile, const char* mapName, const char* slotName, mapgenSlot& slot) {
    mapgenSlotFinder finder(mapFile, slotName);
    if (finder.Find(slot, status)) {
        return true;
    }

    idStr slotStatus = status;
    status = va("map '%s': %s", mapName, slotStatus.c_str());
    return false;
}

bool mapgenDMapJob::ApplyJoin(const mapgenJoinPlan& plan, const mapgenJoinPlanJoin& join) {
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

void mapgenDMapJob::PrefixFirstMapEntities(const char* prefix) {
    mapgenNameRemapper firstMapNames;
    firstMapNames.Build(outputMap, firstMapNumEntities, prefix);

    for (int i = 1; i < firstMapNumEntities; i++) {
        firstMapNames.Apply(outputMap.GetEntity(i)->epairs);
    }
}

bool mapgenDMapJob::WriteOutputMap(idStr& outputMapName) {
    idStr outputMapBase = MAPGEN_OUTPUT_MAP;
    idStr outputPath = outputMapBase;
    outputPath.SetFileExtension("map");

    if (!outputMap.Write(outputMapBase, ".map")) {
        status = va("could not write '%s'", outputPath.c_str());
        return false;
    }
    outputMapName = outputPath;
    return true;
}

void mapgenDMapJob::SetSuccessStatus(const char* planName, int numJoins) {
    status = va("generated plan '%s' with %d joins", planName, numJoins);
}

bool MapGen_DMap(const char* planName, idStr& outputMapName, idStr& status) {
    mapgenDMapJob job;
    return job.Run(planName, outputMapName, status);
}
