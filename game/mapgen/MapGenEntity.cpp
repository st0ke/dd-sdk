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

#include <cstdlib>

#include "MapGenConstants.h"
#include "MapGenEntity.h"

mapgenNamePair::mapgenNamePair() { }

mapgenNamePair::mapgenNamePair(const char* oldName, const char* prefix)
    : oldName(oldName)
    , newName(prefix) {
    newName += oldName;
}

bool mapgenNamePair::Matches(const char* name) const {
    return oldName.Icmp(name) == 0;
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
