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

#ifndef __GAME_MAPGEN_GEOMETRY_H__
#define __GAME_MAPGEN_GEOMETRY_H__

#include "idlib/MapFile.h"

class mapgenSlot;

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

class mapgenSlotFinder {
public:
    mapgenSlotFinder(const idMapFile& mapFile, const char* slotName);
    bool Find(mapgenSlot& slot, idStr& status) const;

private:
    const idMapFile& mapFile;
    idStr slotName;

    idVec3 GetEntityOrigin(const idMapEntity* mapEnt) const;
};

#endif /* !__GAME_MAPGEN_GEOMETRY_H__ */
