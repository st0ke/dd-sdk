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

static const float MAPGEN_PLANE_DIST_EPSILON = 0.001f;
static const float MAPGEN_VERTICAL_SLOT_EPSILON = 0.001f;

static const char * const MAPGEN_SLOT_MATERIAL = "textures/common/mapgen_slot";
static const char * const MAPGEN_OUTPUT_MAP = "maps/mapgen/current";
static const char * const MAPGEN_HARDCODED_SLOT_NAME = "slot_0";
static const char * const MAPGEN_FIRST_INSTANCE_PREFIX = "m0__";
static const char * const MAPGEN_SECOND_INSTANCE_PREFIX = "m1__";

typedef struct mapgenSlot_s {
	idPlane		plane;
	idVec3		anchor;
	int			entityNum;
	int			primitiveNum;
	int			sideNum;
} mapgenSlot_t;

class mapgenTransform {
public:
	idMat3		rotation;
	idVec3		translation;

	void		SetJoin( const mapgenSlot_t &sourceSlot, const mapgenSlot_t &destSlot );
	idVec3		TransformVector( const idVec3 &v ) const;
	idVec3		TransformPoint( const idVec3 &p ) const;
	idPlane		TransformPlane( const idPlane &plane ) const;
};

typedef struct mapGenNamePair_s {
	idStr		oldName;
	idStr		newName;
} mapGenNamePair_t;

class mapgenNameRemapper {
public:
	void		Build( idMapFile &mapFile, int numEntities, const char *prefix );
	void		Apply( idDict &epairs ) const;

private:
	idList<mapGenNamePair_t> namePairs;
	idList<mapGenNamePair_t> groupPairs;

	const char *FindName( const idList<mapGenNamePair_t> &pairs, const char *oldName ) const;
	void		RetargetValue( idDict &epairs, const idKeyValue *kv, const idList<mapGenNamePair_t> &pairs ) const;
	void		RetargetExactKey( idDict &epairs, const char *key ) const;
};

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
	plane.FixDegeneracies( MAPGEN_PLANE_DIST_EPSILON );
	return true;
}

void mapgenTransform::SetJoin( const mapgenSlot_t &sourceSlot, const mapgenSlot_t &destSlot ) {
	idVec3 sourceNormal = sourceSlot.plane.Normal();
	idVec3 joinedDestNormal = -destSlot.plane.Normal();
	float sourceYaw = idMath::ATan( sourceNormal.y, sourceNormal.x );
	float destYaw = idMath::ATan( joinedDestNormal.y, joinedDestNormal.x );
	float yaw = RAD2DEG( destYaw - sourceYaw );

	rotation = idAngles( 0.0f, yaw, 0.0f ).ToMat3();
	translation = destSlot.anchor - rotation * sourceSlot.anchor;
}

idVec3 mapgenTransform::TransformVector( const idVec3 &v ) const {
	return rotation * v;
}

idVec3 mapgenTransform::TransformPoint( const idVec3 &p ) const {
	return TransformVector( p ) + translation;
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

idPlane mapgenTransform::TransformPlane( const idPlane &plane ) const {
	idPlane normalizedPlane = plane;
	MapGen_NormalizePlane( normalizedPlane );

	idVec3 point = normalizedPlane.Normal() * normalizedPlane.Dist();
	idVec3 rotatedPoint = TransformPoint( point );
	idVec3 rotatedNormal = TransformVector( normalizedPlane.Normal() );

	idPlane rotatedPlane;
	rotatedPlane.SetNormal( rotatedNormal );
	rotatedPlane.Normalize();
	rotatedPlane.FitThroughPoint( rotatedPoint );
	rotatedPlane.FixDegeneracies( MAPGEN_PLANE_DIST_EPSILON );
	return rotatedPlane;
}

static idVec3 MapGen_GetEntityOrigin( const idMapEntity *mapEnt ) {
	idVec3 origin;
	mapEnt->epairs.GetVector( "origin", "0 0 0", origin );
	return origin;
}

static bool MapGen_CalculateSlotTransform( idMapBrushSide *side, const idVec3 &origin, mapgenSlot_t &slot ) {
	slot.plane = MapGen_LocalPlaneToWorld( side->GetPlane(), origin );
	if ( !MapGen_NormalizePlane( slot.plane ) ) {
		return false;
	}
	if ( idMath::Fabs( slot.plane.Normal().z ) > MAPGEN_VERTICAL_SLOT_EPSILON ) {
		return false;
	}
	slot.anchor = slot.plane.Normal() * slot.plane.Dist();
	return true;
}

static bool MapGen_FindSlot( const idMapFile &mapFile, const char *slotName, mapgenSlot_t &slot, idStr &status ) {
	int numNamedSlots = 0;

	for ( int entityNum = 1; entityNum < mapFile.GetNumEntities(); entityNum++ ) {
		idMapEntity *mapEnt = mapFile.GetEntity( entityNum );
		if ( idStr::Icmp( mapEnt->epairs.GetString( "classname" ), "func_static" ) != 0 ) {
			continue;
		}
		if ( idStr::Icmp( mapEnt->epairs.GetString( "name" ), slotName ) != 0 ) {
			continue;
		}

		numNamedSlots++;
		if ( numNamedSlots > 1 ) {
			status = va( "found multiple func_static slots named '%s'", slotName );
			return false;
		}

		idVec3 origin = MapGen_GetEntityOrigin( mapEnt );
		int numSlotFaces = 0;

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

				numSlotFaces++;
				if ( numSlotFaces > 1 ) {
					status = va( "slot '%s' has multiple '%s' faces", slotName, MAPGEN_SLOT_MATERIAL );
					return false;
				}

				if ( !MapGen_CalculateSlotTransform( side, origin, slot ) ) {
					status = va( "slot '%s' face must be vertical", slotName );
					return false;
				}
				slot.entityNum = entityNum;
				slot.primitiveNum = primitiveNum;
				slot.sideNum = sideNum;
			}
		}

		if ( numSlotFaces == 0 ) {
			status = va( "slot '%s' has no face using material '%s'", slotName, MAPGEN_SLOT_MATERIAL );
			return false;
		}
	}

	if ( numNamedSlots == 1 ) {
		return true;
	}

	status = va( "could not find func_static slot named '%s'", slotName );
	return false;
}

static idMapBrushSide *MapGen_CloneBrushSide( idMapBrushSide *srcSide, const mapgenTransform &transform, const idVec3 &srcOrigin, const idVec3 &dstOrigin ) {
	idMapBrushSide *dstSide = new idMapBrushSide();
	idVec3 texMat[2];

	srcSide->GetTextureMatrix( texMat[0], texMat[1] );
	dstSide->SetTextureMatrix( texMat );
	dstSide->SetMaterial( srcSide->GetMaterial() );

	idPlane worldPlane = MapGen_LocalPlaneToWorld( srcSide->GetPlane(), srcOrigin );
	idPlane rotatedWorldPlane = transform.TransformPlane( worldPlane );
	dstSide->SetPlane( MapGen_WorldPlaneToLocal( rotatedWorldPlane, dstOrigin ) );

	return dstSide;
}

static idMapBrush *MapGen_CloneBrush( idMapBrush *srcBrush, const mapgenTransform &transform, const idVec3 &srcOrigin, const idVec3 &dstOrigin ) {
	idMapBrush *dstBrush = new idMapBrush();
	dstBrush->epairs = srcBrush->epairs;

	for ( int i = 0; i < srcBrush->GetNumSides(); i++ ) {
		dstBrush->AddSide( MapGen_CloneBrushSide( srcBrush->GetSide( i ), transform, srcOrigin, dstOrigin ) );
	}

	return dstBrush;
}

static idMapPatch *MapGen_ClonePatch( idMapPatch *srcPatch, const mapgenTransform &transform, const idVec3 &srcOrigin, const idVec3 &dstOrigin ) {
	idMapPatch *dstPatch = new idMapPatch( srcPatch->GetWidth(), srcPatch->GetHeight() );
	dstPatch->epairs = srcPatch->epairs;
	dstPatch->SetMaterial( srcPatch->GetMaterial() );
	dstPatch->SetSize( srcPatch->GetWidth(), srcPatch->GetHeight() );
	dstPatch->SetHorzSubdivisions( srcPatch->GetHorzSubdivisions() );
	dstPatch->SetVertSubdivisions( srcPatch->GetVertSubdivisions() );
	dstPatch->SetExplicitlySubdivided( srcPatch->GetExplicitlySubdivided() );

	for ( int i = 0; i < srcPatch->GetWidth() * srcPatch->GetHeight(); i++ ) {
		idDrawVert vert = ( *srcPatch )[ i ];
		vert.xyz = transform.TransformPoint( vert.xyz + srcOrigin ) - dstOrigin;
		vert.normal = transform.TransformVector( vert.normal );
		vert.tangents[0] = transform.TransformVector( vert.tangents[0] );
		vert.tangents[1] = transform.TransformVector( vert.tangents[1] );
		( *dstPatch )[ i ] = vert;
	}

	return dstPatch;
}

static idMapPrimitive *MapGen_ClonePrimitive( idMapPrimitive *srcPrim, const mapgenTransform &transform, const idVec3 &srcOrigin, const idVec3 &dstOrigin ) {
	switch ( srcPrim->GetType() ) {
		case idMapPrimitive::TYPE_BRUSH:
			return MapGen_CloneBrush( static_cast<idMapBrush *>( srcPrim ), transform, srcOrigin, dstOrigin );
		case idMapPrimitive::TYPE_PATCH:
			return MapGen_ClonePatch( static_cast<idMapPatch *>( srcPrim ), transform, srcOrigin, dstOrigin );
		default:
			return NULL;
	}
}

static void MapGen_RotateMatrixKey( idDict &epairs, const char *key, const mapgenTransform &transform ) {
	idMat3 axis;

	if ( !epairs.GetMatrix( key, NULL, axis ) ) {
		return;
	}

	for ( int i = 0; i < 3; i++ ) {
		axis[i] = transform.TransformVector( axis[i] );
		axis[i].FixDegenerateNormal();
	}
	epairs.SetMatrix( key, axis );
}

static void MapGen_RotateVectorKey( idDict &epairs, const char *key, const mapgenTransform &transform ) {
	idVec3 value;

	if ( epairs.GetVector( key, NULL, value ) ) {
		epairs.SetVector( key, transform.TransformVector( value ) );
	}
}

static void MapGen_RotatePointKey( idDict &epairs, const char *key, const mapgenTransform &transform ) {
	idVec3 value;

	if ( epairs.GetVector( key, NULL, value ) ) {
		epairs.SetVector( key, transform.TransformPoint( value ) );
	}
}

static void MapGen_RotateAngleKey( idDict &epairs, const char *key, const mapgenTransform &transform ) {
	const idKeyValue *kv = epairs.FindKey( key );

	if ( kv == NULL ) {
		return;
	}

	float yaw = atof( kv->GetValue() );
	idAngles angles( 0.0f, yaw, 0.0f );
	idVec3 forward = transform.TransformVector( angles.ToForward() );
	epairs.SetFloat( key, idMath::AngleNormalize360( forward.ToYaw() ) );
}

static void MapGen_RotateMoveDirKey( idDict &epairs, const char *key, const mapgenTransform &transform ) {
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

	direction = transform.TransformVector( direction );
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

const char *mapgenNameRemapper::FindName( const idList<mapGenNamePair_t> &pairs, const char *oldName ) const {
	for ( int i = 0; i < pairs.Num(); i++ ) {
		if ( pairs[i].oldName.Icmp( oldName ) == 0 ) {
			return pairs[i].newName.c_str();
		}
	}
	return NULL;
}

void mapgenNameRemapper::RetargetValue( idDict &epairs, const idKeyValue *kv, const idList<mapGenNamePair_t> &pairs ) const {
	const char *newName = FindName( pairs, kv->GetValue() );
	if ( newName != NULL ) {
		epairs.Set( kv->GetKey(), newName );
	}
}

void mapgenNameRemapper::RetargetExactKey( idDict &epairs, const char *key ) const {
	const idKeyValue *kv = epairs.FindKey( key );
	if ( kv != NULL ) {
		RetargetValue( epairs, kv, namePairs );
	}
}

void mapgenNameRemapper::Build( idMapFile &mapFile, int numEntities, const char *prefix ) {
	namePairs.Clear();
	groupPairs.Clear();

	for ( int i = 1; i < numEntities; i++ ) {
		const char *oldName = mapFile.GetEntity( i )->epairs.GetString( "name" );
		if ( oldName[0] != '\0' ) {
			mapGenNamePair_t pair;
			pair.oldName = oldName;
			pair.newName = prefix;
			pair.newName += oldName;
			namePairs.Append( pair );
		}

		const idKeyValue *kv = mapFile.GetEntity( i )->epairs.FindKey( "team" );
		if ( kv == NULL || kv->GetValue()[0] == '\0' || MapGen_IsNumericString( kv->GetValue() ) ) {
			continue;
		}

		if ( FindName( groupPairs, kv->GetValue() ) != NULL ) {
			continue;
		}

		mapGenNamePair_t pair;
		pair.oldName = kv->GetValue();
		pair.newName = prefix;
		pair.newName += kv->GetValue();
		groupPairs.Append( pair );
	}
}

void mapgenNameRemapper::Apply( idDict &epairs ) const {
	static const char * const retargetPrefixes[] = {
		"target",
		"guiTarget",
		"buddy"
	};
	static const char * const retargetExactKeys[] = {
		"bind",
		"cameraTarget",
		"syncLock"
	};

	const char *oldName = epairs.GetString( "name" );
	const char *newName = FindName( namePairs, oldName );
	if ( newName != NULL ) {
		epairs.Set( "name", newName );
	}

	int numKeyVals = epairs.GetNumKeyVals();
	for ( int i = 0; i < numKeyVals; i++ ) {
		const idKeyValue *kv = epairs.GetKeyVal( i );
		bool shouldRetarget = false;

		for ( int prefixIndex = 0; prefixIndex < sizeof( retargetPrefixes ) / sizeof( retargetPrefixes[0] ); prefixIndex++ ) {
			if ( MapGen_KeyHasPrefix( kv, retargetPrefixes[prefixIndex] ) ) {
				shouldRetarget = true;
				break;
			}
		}
		if ( !shouldRetarget ) {
			continue;
		}
		RetargetValue( epairs, kv, namePairs );
	}

	for ( int i = 0; i < sizeof( retargetExactKeys ) / sizeof( retargetExactKeys[0] ); i++ ) {
		RetargetExactKey( epairs, retargetExactKeys[i] );
	}

	const idKeyValue *team = epairs.FindKey( "team" );
	if ( team != NULL ) {
		RetargetValue( epairs, team, groupPairs );
	}
}

static void MapGen_TransformEntitySpawnArgs( idDict &epairs, const mapgenTransform &transform ) {
	static const char * const pointKeys[] = {
		"light_origin"
	};
	static const char * const vectorKeys[] = {
		"light_target",
		"light_right",
		"light_up",
		"light_start",
		"light_end",
		"light_center"
	};
	static const char * const matrixKeys[] = {
		"rotation",
		"light_rotation"
	};

	for ( int i = 0; i < sizeof( pointKeys ) / sizeof( pointKeys[0] ); i++ ) {
		MapGen_RotatePointKey( epairs, pointKeys[i], transform );
	}
	for ( int i = 0; i < sizeof( vectorKeys ) / sizeof( vectorKeys[0] ); i++ ) {
		MapGen_RotateVectorKey( epairs, vectorKeys[i], transform );
	}
	for ( int i = 0; i < sizeof( matrixKeys ) / sizeof( matrixKeys[0] ); i++ ) {
		MapGen_RotateMatrixKey( epairs, matrixKeys[i], transform );
	}
	MapGen_RotateAngleKey( epairs, "angle", transform );
	MapGen_RotateMoveDirKey( epairs, "movedir", transform );
}

static bool MapGen_ClonePrimitives( idMapEntity *dstEnt, idMapEntity *srcEnt, const mapgenTransform &transform, const idVec3 &srcOrigin, const idVec3 &dstOrigin ) {
	idList<idMapPrimitive *> clonedPrimitives;
	int numPrimitives = srcEnt->GetNumPrimitives();

	for ( int i = 0; i < numPrimitives; i++ ) {
		idMapPrimitive *dstPrim = MapGen_ClonePrimitive( srcEnt->GetPrimitive( i ), transform, srcOrigin, dstOrigin );
		if ( dstPrim == NULL ) {
			clonedPrimitives.DeleteContents( true );
			return false;
		}
		clonedPrimitives.Append( dstPrim );
	}

	for ( int i = 0; i < clonedPrimitives.Num(); i++ ) {
		dstEnt->AddPrimitive( clonedPrimitives[i] );
	}
	clonedPrimitives.Clear();
	return true;
}

static idMapEntity *MapGen_CloneEntity( idMapEntity *srcEnt, const mapgenTransform &transform, const mapgenNameRemapper &remapper ) {
	idVec3 srcOrigin = MapGen_GetEntityOrigin( srcEnt );
	idVec3 dstOrigin = transform.TransformPoint( srcOrigin );

	idMapEntity *dstEnt = new idMapEntity();
	dstEnt->epairs = srcEnt->epairs;
	dstEnt->epairs.SetVector( "origin", dstOrigin );

	remapper.Apply( dstEnt->epairs );
	MapGen_TransformEntitySpawnArgs( dstEnt->epairs, transform );

	if ( !MapGen_ClonePrimitives( dstEnt, srcEnt, transform, srcOrigin, dstOrigin ) ) {
		delete dstEnt;
		return NULL;
	}

	return dstEnt;
}

static bool MapGen_DuplicateWorldspawn( idMapEntity *worldspawn, const mapgenTransform &transform ) {
	idVec3 origin = MapGen_GetEntityOrigin( worldspawn );
	return MapGen_ClonePrimitives( worldspawn, worldspawn, transform, origin, origin );
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

	mapgenSlot_t slot;
	if ( !MapGen_FindSlot( mapFile, MAPGEN_HARDCODED_SLOT_NAME, slot, status ) ) {
		return false;
	}

	int numEntities = mapFile.GetNumEntities();
	mapgenNameRemapper firstInstanceNames;
	firstInstanceNames.Build( mapFile, numEntities, MAPGEN_FIRST_INSTANCE_PREFIX );

	mapgenNameRemapper secondInstanceNames;
	secondInstanceNames.Build( mapFile, numEntities, MAPGEN_SECOND_INSTANCE_PREFIX );

	mapgenTransform secondInstanceTransform;
	secondInstanceTransform.SetJoin( slot, slot );

	idMapEntity *worldspawn = mapFile.GetEntity( 0 );
	if ( !MapGen_DuplicateWorldspawn( worldspawn, secondInstanceTransform ) ) {
		status = "could not duplicate worldspawn primitives";
		return false;
	}

	for ( int i = 1; i < numEntities; i++ ) {
		idMapEntity *clonedEntity = MapGen_CloneEntity( mapFile.GetEntity( i ), secondInstanceTransform, secondInstanceNames );
		if ( clonedEntity == NULL ) {
			status = va( "could not duplicate entity %d primitives", i );
			return false;
		}
		mapFile.AddEntity( clonedEntity );
	}

	for ( int i = 1; i < numEntities; i++ ) {
		firstInstanceNames.Apply( mapFile.GetEntity( i )->epairs );
	}

	idStr outputMapBase = MAPGEN_OUTPUT_MAP;
	outputMapName = outputMapBase;
	outputMapName.SetFileExtension( "map" );

	if ( !mapFile.Write( outputMapBase, ".map" ) ) {
		status = va( "could not write '%s'", outputMapName.c_str() );
		return false;
	}

	status = va( "joined %s as %s%s to %s%s using entity %d primitive %d side %d", MAPGEN_HARDCODED_SLOT_NAME, MAPGEN_FIRST_INSTANCE_PREFIX, MAPGEN_HARDCODED_SLOT_NAME, MAPGEN_SECOND_INSTANCE_PREFIX, MAPGEN_HARDCODED_SLOT_NAME, slot.entityNum, slot.primitiveNum, slot.sideNum );
	return true;
}
