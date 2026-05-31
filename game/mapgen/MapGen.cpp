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

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "idlib/MapFile.h"
#include "idlib/math/Rotation.h"

#include "MapGen.h"

static const char * const MAPGEN_SLOT_MATERIAL = "textures/common/mapgen_slot";
static const char * const MAPGEN_OUTPUT_MAP = "maps/mapgen/current";

typedef struct mapGenSlot_s {
	idPlane		plane;
	idVec3		center;
	idVec3		rotationAxis;
	idMat3		rotation;
	int			entityNum;
	int			primitiveNum;
	int			sideNum;
	int			oppositeSideNum;
} mapGenSlot_t;

typedef struct mapGenNamePair_s {
	idStr		oldName;
	idStr		newName;
} mapGenNamePair_t;

static bool MapGen_IsNumericString( const char *value ) {
	char *end;

	if ( value == NULL || value[0] == '\0' ) {
		return false;
	}

	strtod( value, &end );
	return ( end != value && *end == '\0' );
}

static void MapGen_NormalizeInputMapName( const char *sourceMapName, idStr &mapName ) {
	mapName = sourceMapName;
	mapName.BackSlashesToSlashes();
	mapName.StripFileExtension();

	if ( mapName.Icmpn( "maps/", 5 ) != 0 ) {
		mapName = "maps/" + mapName;
	}
}

static bool MapGen_NormalizePlane( idPlane &plane ) {
	idVec3 normal = plane.Normal();
	float length = normal.Normalize();

	if ( length <= 0.0f ) {
		return false;
	}

	plane.SetNormal( normal );
	plane[3] /= length;
	plane.FixDegeneracies( 0.001f );
	return true;
}

static idVec3 MapGen_RotateVector( const idVec3 &v, const mapGenSlot_t &slot ) {
	return v * slot.rotation;
}

static idVec3 MapGen_RotatePoint( const idVec3 &p, const mapGenSlot_t &slot ) {
	return ( p - slot.center ) * slot.rotation + slot.center;
}

static idPlane MapGen_LocalPlaneToWorld( const idPlane &localPlane, const idVec3 &origin ) {
	idPlane worldPlane = localPlane;
	MapGen_NormalizePlane( worldPlane );
	worldPlane[3] -= origin * worldPlane.Normal();
	return worldPlane;
}

static idPlane MapGen_WorldPlaneToLocal( const idPlane &worldPlane, const idVec3 &origin ) {
	idPlane localPlane = worldPlane;
	MapGen_NormalizePlane( localPlane );
	localPlane[3] += origin * localPlane.Normal();
	return localPlane;
}

static idPlane MapGen_RotatePlane( const idPlane &plane, const mapGenSlot_t &slot ) {
	idPlane normalizedPlane = plane;
	MapGen_NormalizePlane( normalizedPlane );

	idVec3 point = normalizedPlane.Normal() * normalizedPlane.Dist();
	idVec3 rotatedPoint = MapGen_RotatePoint( point, slot );
	idVec3 rotatedNormal = MapGen_RotateVector( normalizedPlane.Normal(), slot );

	idPlane rotatedPlane;
	rotatedPlane.SetNormal( rotatedNormal );
	rotatedPlane.Normalize();
	rotatedPlane.FitThroughPoint( rotatedPoint );
	rotatedPlane.FixDegeneracies( 0.001f );
	return rotatedPlane;
}

static idVec3 MapGen_GetEntityOrigin( const idMapEntity *mapEnt ) {
	idVec3 origin;
	mapEnt->epairs.GetVector( "origin", "0 0 0", origin );
	return origin;
}

static int MapGen_FindOppositeSide( const idPlane planes[6], int sideNum ) {
	for ( int i = 0; i < 6; i++ ) {
		if ( i == sideNum ) {
			continue;
		}
		if ( planes[sideNum].Normal() * planes[i].Normal() < -0.999f ) {
			return i;
		}
	}
	return -1;
}

static bool MapGen_CalculateBrushCenter( const idPlane planes[6], idVec3 &center ) {
	bool used[6];
	int numPairs;

	memset( used, 0, sizeof( used ) );
	center.Zero();
	numPairs = 0;

	for ( int i = 0; i < 6; i++ ) {
		if ( used[i] ) {
			continue;
		}

		int oppositeSideNum = MapGen_FindOppositeSide( planes, i );
		if ( oppositeSideNum < 0 || used[oppositeSideNum] ) {
			return false;
		}

		float centerDist = ( planes[i].Dist() - planes[oppositeSideNum].Dist() ) * 0.5f;
		center += planes[i].Normal() * centerDist;
		used[i] = true;
		used[oppositeSideNum] = true;
		numPairs++;
	}

	return ( numPairs == 3 );
}

static bool MapGen_CalculateRotationAxis( const idPlane planes[6], int slotSideNum, int oppositeSideNum, idVec3 &rotationAxis ) {
	idVec3 projectedUp;
	float bestDot;
	int bestSideNum;

	projectedUp = idVec3( 0.0f, 0.0f, 1.0f ) - planes[slotSideNum].Normal() * ( planes[slotSideNum].Normal().z );
	if ( projectedUp.Normalize() == 0.0f ) {
		projectedUp.Zero();
	}

	bestDot = -1.0f;
	bestSideNum = -1;
	for ( int i = 0; i < 6; i++ ) {
		if ( i == slotSideNum || i == oppositeSideNum ) {
			continue;
		}

		idVec3 normal = planes[i].Normal();
		float dot;

		if ( projectedUp == vec3_zero ) {
			dot = 1.0f;
		} else {
			dot = idMath::Fabs( normal * projectedUp );
		}

		if ( dot > bestDot ) {
			bestDot = dot;
			bestSideNum = i;
		}
	}

	if ( bestSideNum < 0 ) {
		return false;
	}

	rotationAxis = planes[bestSideNum].Normal();
	return ( rotationAxis.Normalize() != 0.0f );
}

static bool MapGen_CalculateSlotTransform( idMapBrush *brush, const idVec3 &origin, int slotSideNum, mapGenSlot_t &slot ) {
	idPlane planes[6];

	if ( brush->GetNumSides() != 6 ) {
		return false;
	}

	for ( int i = 0; i < 6; i++ ) {
		planes[i] = MapGen_LocalPlaneToWorld( brush->GetSide( i )->GetPlane(), origin );
		if ( !MapGen_NormalizePlane( planes[i] ) ) {
			return false;
		}
	}

	slot.oppositeSideNum = MapGen_FindOppositeSide( planes, slotSideNum );
	if ( slot.oppositeSideNum < 0 ) {
		return false;
	}

	if ( !MapGen_CalculateBrushCenter( planes, slot.center ) ) {
		return false;
	}

	if ( !MapGen_CalculateRotationAxis( planes, slotSideNum, slot.oppositeSideNum, slot.rotationAxis ) ) {
		return false;
	}

	slot.plane = planes[slotSideNum];
	slot.rotation = idRotation( slot.center, slot.rotationAxis, 180.0f ).ToMat3();
	return true;
}

static bool MapGen_FindSlot( const idMapFile &mapFile, mapGenSlot_t &slot ) {
	for ( int entityNum = 0; entityNum < mapFile.GetNumEntities(); entityNum++ ) {
		idMapEntity *mapEnt = mapFile.GetEntity( entityNum );
		idVec3 origin = MapGen_GetEntityOrigin( mapEnt );

		for ( int primitiveNum = 0; primitiveNum < mapEnt->GetNumPrimitives(); primitiveNum++ ) {
			idMapPrimitive *mapPrim = mapEnt->GetPrimitive( primitiveNum );
			if ( mapPrim->GetType() != idMapPrimitive::TYPE_BRUSH ) {
				continue;
			}

			idMapBrush *brush = static_cast<idMapBrush *>( mapPrim );
			for ( int sideNum = 0; sideNum < brush->GetNumSides(); sideNum++ ) {
				idMapBrushSide *side = brush->GetSide( sideNum );
				if ( idStr::Icmp( side->GetMaterial(), MAPGEN_SLOT_MATERIAL ) != 0 ) {
					continue;
				}

				if ( !MapGen_CalculateSlotTransform( brush, origin, sideNum, slot ) ) {
					return false;
				}
				slot.entityNum = entityNum;
				slot.primitiveNum = primitiveNum;
				slot.sideNum = sideNum;
				return true;
			}
		}
	}

	return false;
}

static idMapBrushSide *MapGen_CloneBrushSide( idMapBrushSide *srcSide, const mapGenSlot_t &slot, const idVec3 &srcOrigin, const idVec3 &dstOrigin ) {
	idMapBrushSide *dstSide = new idMapBrushSide();
	idVec3 texMat[2];

	srcSide->GetTextureMatrix( texMat[0], texMat[1] );
	dstSide->SetTextureMatrix( texMat );
	dstSide->SetMaterial( srcSide->GetMaterial() );

	idPlane worldPlane = MapGen_LocalPlaneToWorld( srcSide->GetPlane(), srcOrigin );
	idPlane rotatedWorldPlane = MapGen_RotatePlane( worldPlane, slot );
	dstSide->SetPlane( MapGen_WorldPlaneToLocal( rotatedWorldPlane, dstOrigin ) );

	return dstSide;
}

static idMapBrush *MapGen_CloneBrush( idMapBrush *srcBrush, const mapGenSlot_t &slot, const idVec3 &srcOrigin, const idVec3 &dstOrigin ) {
	idMapBrush *dstBrush = new idMapBrush();
	dstBrush->epairs = srcBrush->epairs;

	for ( int i = 0; i < srcBrush->GetNumSides(); i++ ) {
		dstBrush->AddSide( MapGen_CloneBrushSide( srcBrush->GetSide( i ), slot, srcOrigin, dstOrigin ) );
	}

	return dstBrush;
}

static idMapPatch *MapGen_ClonePatch( idMapPatch *srcPatch, const mapGenSlot_t &slot, const idVec3 &srcOrigin, const idVec3 &dstOrigin ) {
	idMapPatch *dstPatch = new idMapPatch( srcPatch->GetWidth(), srcPatch->GetHeight() );
	dstPatch->epairs = srcPatch->epairs;
	dstPatch->SetMaterial( srcPatch->GetMaterial() );
	dstPatch->SetSize( srcPatch->GetWidth(), srcPatch->GetHeight() );
	dstPatch->SetHorzSubdivisions( srcPatch->GetHorzSubdivisions() );
	dstPatch->SetVertSubdivisions( srcPatch->GetVertSubdivisions() );
	dstPatch->SetExplicitlySubdivided( srcPatch->GetExplicitlySubdivided() );

	for ( int i = 0; i < srcPatch->GetWidth() * srcPatch->GetHeight(); i++ ) {
		idDrawVert vert = ( *srcPatch )[ i ];
		vert.xyz = MapGen_RotatePoint( vert.xyz + srcOrigin, slot ) - dstOrigin;
		vert.normal = MapGen_RotateVector( vert.normal, slot );
		vert.tangents[0] = MapGen_RotateVector( vert.tangents[0], slot );
		vert.tangents[1] = MapGen_RotateVector( vert.tangents[1], slot );
		( *dstPatch )[ i ] = vert;
	}

	return dstPatch;
}

static idMapPrimitive *MapGen_ClonePrimitive( idMapPrimitive *srcPrim, const mapGenSlot_t &slot, const idVec3 &srcOrigin, const idVec3 &dstOrigin ) {
	switch ( srcPrim->GetType() ) {
		case idMapPrimitive::TYPE_BRUSH:
			return MapGen_CloneBrush( static_cast<idMapBrush *>( srcPrim ), slot, srcOrigin, dstOrigin );
		case idMapPrimitive::TYPE_PATCH:
			return MapGen_ClonePatch( static_cast<idMapPatch *>( srcPrim ), slot, srcOrigin, dstOrigin );
		default:
			return NULL;
	}
}

static bool MapGen_NameIsUsed( idMapFile &mapFile, const idList<idStr> &usedNames, const char *name ) {
	if ( name == NULL || name[0] == '\0' ) {
		return true;
	}

	if ( mapFile.FindEntity( name ) != NULL ) {
		return true;
	}

	for ( int i = 0; i < usedNames.Num(); i++ ) {
		if ( usedNames[i].Icmp( name ) == 0 ) {
			return true;
		}
	}

	return false;
}

static idStr MapGen_UniqueMirrorName( idMapFile &mapFile, const idList<idStr> &usedNames, const char *oldName ) {
	idStr baseName = oldName;
	baseName += "_mapgen_mirror";

	idStr newName = baseName;
	for ( int i = 1; MapGen_NameIsUsed( mapFile, usedNames, newName.c_str() ); i++ ) {
		newName = va( "%s%d", baseName.c_str(), i );
	}

	return newName;
}

static const char *MapGen_FindMirroredName( const idList<mapGenNamePair_t> &namePairs, const char *oldName ) {
	for ( int i = 0; i < namePairs.Num(); i++ ) {
		if ( namePairs[i].oldName.Icmp( oldName ) == 0 ) {
			return namePairs[i].newName.c_str();
		}
	}
	return NULL;
}

static bool MapGen_PairExists( const idList<mapGenNamePair_t> &pairs, const char *oldName ) {
	return ( MapGen_FindMirroredName( pairs, oldName ) != NULL );
}

static void MapGen_BuildNamePairs( idMapFile &mapFile, idList<mapGenNamePair_t> &namePairs ) {
	idList<idStr> usedNames;

	for ( int i = 1; i < mapFile.GetNumEntities(); i++ ) {
		const char *oldName = mapFile.GetEntity( i )->epairs.GetString( "name" );
		if ( oldName[0] == '\0' ) {
			continue;
		}

		mapGenNamePair_t pair;
		pair.oldName = oldName;
		pair.newName = MapGen_UniqueMirrorName( mapFile, usedNames, oldName );
		usedNames.Append( pair.newName );
		namePairs.Append( pair );
	}
}

static bool MapGen_GroupNameIsUsed( idMapFile &mapFile, const idList<idStr> &usedNames, const char *name ) {
	if ( name == NULL || name[0] == '\0' ) {
		return true;
	}

	for ( int i = 1; i < mapFile.GetNumEntities(); i++ ) {
		const idKeyValue *kv = mapFile.GetEntity( i )->epairs.FindKey( "team" );
		if ( kv != NULL && kv->GetValue().Icmp( name ) == 0 ) {
			return true;
		}
	}

	for ( int i = 0; i < usedNames.Num(); i++ ) {
		if ( usedNames[i].Icmp( name ) == 0 ) {
			return true;
		}
	}

	return false;
}

static idStr MapGen_UniqueMirrorGroupName( idMapFile &mapFile, const idList<idStr> &usedNames, const char *oldName ) {
	idStr baseName = oldName;
	baseName += "_mapgen_mirror";

	idStr newName = baseName;
	for ( int i = 1; MapGen_GroupNameIsUsed( mapFile, usedNames, newName.c_str() ); i++ ) {
		newName = va( "%s%d", baseName.c_str(), i );
	}

	return newName;
}

static void MapGen_BuildGroupPairs( idMapFile &mapFile, idList<mapGenNamePair_t> &groupPairs ) {
	idList<idStr> usedNames;

	for ( int i = 1; i < mapFile.GetNumEntities(); i++ ) {
		const idKeyValue *kv = mapFile.GetEntity( i )->epairs.FindKey( "team" );
		if ( kv == NULL || kv->GetValue()[0] == '\0' || MapGen_IsNumericString( kv->GetValue() ) ) {
			continue;
		}

		if ( MapGen_PairExists( groupPairs, kv->GetValue() ) ) {
			continue;
		}

		mapGenNamePair_t pair;
		pair.oldName = kv->GetValue();
		pair.newName = MapGen_UniqueMirrorGroupName( mapFile, usedNames, kv->GetValue() );
		usedNames.Append( pair.newName );
		groupPairs.Append( pair );
	}
}

static void MapGen_RotateMatrixKey( idDict &epairs, const char *key, const mapGenSlot_t &slot ) {
	idMat3 axis;

	if ( !epairs.GetMatrix( key, NULL, axis ) ) {
		return;
	}

	for ( int i = 0; i < 3; i++ ) {
		axis[i] = MapGen_RotateVector( axis[i], slot );
		axis[i].FixDegenerateNormal();
	}
	epairs.SetMatrix( key, axis );
}

static void MapGen_RotateVectorKey( idDict &epairs, const char *key, const mapGenSlot_t &slot ) {
	idVec3 value;

	if ( epairs.GetVector( key, NULL, value ) ) {
		epairs.SetVector( key, MapGen_RotateVector( value, slot ) );
	}
}

static void MapGen_RotatePointKey( idDict &epairs, const char *key, const mapGenSlot_t &slot ) {
	idVec3 value;

	if ( epairs.GetVector( key, NULL, value ) ) {
		epairs.SetVector( key, MapGen_RotatePoint( value, slot ) );
	}
}

static void MapGen_RotateAngleKey( idDict &epairs, const char *key, const mapGenSlot_t &slot ) {
	const idKeyValue *kv = epairs.FindKey( key );

	if ( kv == NULL ) {
		return;
	}

	float yaw = atof( kv->GetValue() );
	idAngles angles( 0.0f, yaw, 0.0f );
	idVec3 forward = MapGen_RotateVector( angles.ToForward(), slot );
	epairs.SetFloat( key, idMath::AngleNormalize360( forward.ToYaw() ) );
}

static void MapGen_RotateMoveDirKey( idDict &epairs, const char *key, const mapGenSlot_t &slot ) {
	const idKeyValue *kv = epairs.FindKey( key );

	if ( kv == NULL ) {
		return;
	}

	float yaw = atof( kv->GetValue() );
	idVec3 direction;
	if ( yaw == -1.0f ) {
		direction = idVec3( 0.0f, 0.0f, 1.0f );
	} else if ( yaw == -2.0f ) {
		direction = idVec3( 0.0f, 0.0f, -1.0f );
	} else {
		direction = idAngles( 0.0f, yaw, 0.0f ).ToForward();
	}

	direction = MapGen_RotateVector( direction, slot );
	direction.Normalize();
	direction.FixDegenerateNormal();

	if ( direction.z > 0.999f ) {
		epairs.Set( key, "-1" );
	} else if ( direction.z < -0.999f ) {
		epairs.Set( key, "-2" );
	} else {
		epairs.SetFloat( key, idMath::AngleNormalize360( direction.ToYaw() ) );
	}
}

static bool MapGen_KeyHasPrefix( const idKeyValue *kv, const char *prefix ) {
	return ( kv != NULL && kv->GetKey().Icmpn( prefix, idStr::Length( prefix ) ) == 0 );
}

static void MapGen_RetargetValue( idDict &epairs, const idKeyValue *kv, const idList<mapGenNamePair_t> &namePairs ) {
	const char *newName = MapGen_FindMirroredName( namePairs, kv->GetValue() );
	if ( newName != NULL ) {
		epairs.Set( kv->GetKey(), newName );
	}
}

static void MapGen_RetargetExactKey( idDict &epairs, const char *key, const idList<mapGenNamePair_t> &namePairs ) {
	const idKeyValue *kv = epairs.FindKey( key );
	if ( kv != NULL ) {
		MapGen_RetargetValue( epairs, kv, namePairs );
	}
}

static void MapGen_RetargetNames( idDict &epairs, const idList<mapGenNamePair_t> &namePairs ) {
	int numKeyVals = epairs.GetNumKeyVals();

	for ( int i = 0; i < numKeyVals; i++ ) {
		const idKeyValue *kv = epairs.GetKeyVal( i );
		if ( !MapGen_KeyHasPrefix( kv, "target" ) && !MapGen_KeyHasPrefix( kv, "guiTarget" ) && !MapGen_KeyHasPrefix( kv, "buddy" ) ) {
			continue;
		}

		MapGen_RetargetValue( epairs, kv, namePairs );
	}

	MapGen_RetargetExactKey( epairs, "bind", namePairs );
	MapGen_RetargetExactKey( epairs, "cameraTarget", namePairs );
	MapGen_RetargetExactKey( epairs, "syncLock", namePairs );
}

static void MapGen_RetargetGroups( idDict &epairs, const idList<mapGenNamePair_t> &groupPairs ) {
	const idKeyValue *kv = epairs.FindKey( "team" );
	if ( kv != NULL ) {
		MapGen_RetargetValue( epairs, kv, groupPairs );
	}
}

static idMapEntity *MapGen_CloneEntity( idMapEntity *srcEnt, const mapGenSlot_t &slot, const idList<mapGenNamePair_t> &namePairs, const idList<mapGenNamePair_t> &groupPairs ) {
	idVec3 srcOrigin = MapGen_GetEntityOrigin( srcEnt );
	idVec3 dstOrigin = MapGen_RotatePoint( srcOrigin, slot );

	idMapEntity *dstEnt = new idMapEntity();
	dstEnt->epairs = srcEnt->epairs;
	dstEnt->epairs.SetVector( "origin", dstOrigin );

	const char *oldName = srcEnt->epairs.GetString( "name" );
	const char *newName = MapGen_FindMirroredName( namePairs, oldName );
	if ( newName != NULL ) {
		dstEnt->epairs.Set( "name", newName );
	}

	MapGen_RetargetNames( dstEnt->epairs, namePairs );
	MapGen_RetargetGroups( dstEnt->epairs, groupPairs );
	MapGen_RotatePointKey( dstEnt->epairs, "light_origin", slot );
	MapGen_RotateVectorKey( dstEnt->epairs, "light_target", slot );
	MapGen_RotateVectorKey( dstEnt->epairs, "light_right", slot );
	MapGen_RotateVectorKey( dstEnt->epairs, "light_up", slot );
	MapGen_RotateVectorKey( dstEnt->epairs, "light_start", slot );
	MapGen_RotateVectorKey( dstEnt->epairs, "light_end", slot );
	MapGen_RotateVectorKey( dstEnt->epairs, "light_center", slot );
	MapGen_RotateMatrixKey( dstEnt->epairs, "rotation", slot );
	MapGen_RotateMatrixKey( dstEnt->epairs, "light_rotation", slot );
	MapGen_RotateAngleKey( dstEnt->epairs, "angle", slot );
	MapGen_RotateMoveDirKey( dstEnt->epairs, "movedir", slot );

	for ( int i = 0; i < srcEnt->GetNumPrimitives(); i++ ) {
		idMapPrimitive *dstPrim = MapGen_ClonePrimitive( srcEnt->GetPrimitive( i ), slot, srcOrigin, dstOrigin );
		if ( dstPrim != NULL ) {
			dstEnt->AddPrimitive( dstPrim );
		}
	}

	return dstEnt;
}

static bool MapGen_DuplicateWorldspawn( idMapEntity *worldspawn, const mapGenSlot_t &slot ) {
	idVec3 origin = MapGen_GetEntityOrigin( worldspawn );
	int numPrimitives = worldspawn->GetNumPrimitives();

	for ( int i = 0; i < numPrimitives; i++ ) {
		idMapPrimitive *dstPrim = MapGen_ClonePrimitive( worldspawn->GetPrimitive( i ), slot, origin, origin );
		if ( dstPrim == NULL ) {
			return false;
		}
		worldspawn->AddPrimitive( dstPrim );
	}

	return true;
}

bool MapGen_DMap( const char *sourceMapName, idStr &outputMapName, idStr &status ) {
	idStr inputMapName;
	MapGen_NormalizeInputMapName( sourceMapName, inputMapName );

	idMapFile mapFile;
	if ( !mapFile.Parse( inputMapName, true ) ) {
		status = va( "could not parse '%s.map'", inputMapName.c_str() );
		return false;
	}

	if ( mapFile.GetNumEntities() <= 0 ) {
		status = "map has no worldspawn";
		return false;
	}

	mapGenSlot_t slot;
	if ( !MapGen_FindSlot( mapFile, slot ) ) {
		status = va( "could not find brush side using material '%s'", MAPGEN_SLOT_MATERIAL );
		return false;
	}

	idList<mapGenNamePair_t> namePairs;
	MapGen_BuildNamePairs( mapFile, namePairs );

	idList<mapGenNamePair_t> groupPairs;
	MapGen_BuildGroupPairs( mapFile, groupPairs );

	idMapEntity *worldspawn = mapFile.GetEntity( 0 );
	if ( !MapGen_DuplicateWorldspawn( worldspawn, slot ) ) {
		status = "could not duplicate worldspawn primitives";
		return false;
	}

	int numEntities = mapFile.GetNumEntities();
	for ( int i = 1; i < numEntities; i++ ) {
		mapFile.AddEntity( MapGen_CloneEntity( mapFile.GetEntity( i ), slot, namePairs, groupPairs ) );
	}

	idStr outputMapBase = MAPGEN_OUTPUT_MAP;
	outputMapName = outputMapBase;
	outputMapName.SetFileExtension( "map" );

	if ( !mapFile.Write( outputMapBase, ".map" ) ) {
		status = va( "could not write '%s'", outputMapName.c_str() );
		return false;
	}

	status = va( "using slot entity %d primitive %d side %d opposite %d", slot.entityNum, slot.primitiveNum, slot.sideNum, slot.oppositeSideNum );
	return true;
}
