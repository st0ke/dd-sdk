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

#include "idlib/geometry/Winding.h"
#include "idlib/math/Rotation.h"
#include "sys/platform.h"

#include "MapGenConstants.h"
#include "MapGenGeometry.h"

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
