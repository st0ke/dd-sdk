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

#include "CombatScaling.h"

#include <climits>
#include <cmath>

/*
================
idCombatScaling::idCombatScaling
================
*/
idCombatScaling::idCombatScaling() {
	Clear();
}

/*
================
idCombatScaling::Clear
================
*/
void idCombatScaling::Clear( void ) {
	minLevel = 1;
	maxLevel = 1;
	growth = 1.0f;
	cachedLevelScales.Clear();
}

/*
================
idCombatScaling::Init
================
*/
bool idCombatScaling::Init( const idDict &config, idStr &error ) {
	error.Clear();

	if ( idStr::Cmp( config.GetString( "config_type", "" ), "combat_progression" ) != 0 ) {
		error = "'config_type' must be 'combat_progression'";
		return false;
	}

	int configuredMinLevel;
	int configuredMaxLevel;
	float configuredGrowth;
	if ( !config.GetInt( "min_level", "0", configuredMinLevel ) ) {
		error = "missing 'min_level'";
		return false;
	}
	if ( !config.GetInt( "max_level", "0", configuredMaxLevel ) ) {
		error = "missing 'max_level'";
		return false;
	}
	if ( !config.GetFloat( "level_growth", "0", configuredGrowth ) ) {
		error = "missing 'level_growth'";
		return false;
	}

	if ( configuredMinLevel < 1 ) {
		error = "'min_level' must be at least 1";
		return false;
	}
	if ( configuredMaxLevel < configuredMinLevel ) {
		error = "'max_level' must not be less than 'min_level'";
		return false;
	}
	if ( configuredGrowth <= 1.0f ) {
		error = "'level_growth' must be greater than 1";
		return false;
	}

	const long long levelCount = static_cast<long long>( configuredMaxLevel ) - configuredMinLevel + 1;
	if ( levelCount > MAX_CACHED_COMBAT_LEVELS ) {
		error = va( "configured level range exceeds the cache limit of %d", MAX_CACHED_COMBAT_LEVELS );
		return false;
	}

	idList<float> configuredScales;
	configuredScales.SetNum( static_cast<int>( levelCount ) );

	double scale = pow( static_cast<double>( configuredGrowth ), static_cast<double>( configuredMinLevel - 1 ) );
	for( int index = 0; index < static_cast<int>( levelCount ); index++ ) {
		const int level = configuredMinLevel + index;
		if ( scale <= 0.0 || scale > INT_MAX ) {
			error = va( "scale at level %d exceeds the supported integral-stat range", level );
			return false;
		}
		configuredScales[ index ] = static_cast<float>( scale );
		scale *= configuredGrowth;
	}

	minLevel = configuredMinLevel;
	maxLevel = configuredMaxLevel;
	growth = configuredGrowth;
	cachedLevelScales.Swap( configuredScales );
	return true;
}

/*
================
idCombatScaling::IsInitialized
================
*/
bool idCombatScaling::IsInitialized( void ) const {
	return cachedLevelScales.Num() > 0;
}

/*
================
idCombatScaling::GetMinLevel
================
*/
int idCombatScaling::GetMinLevel( void ) const {
	return minLevel;
}

/*
================
idCombatScaling::GetMaxLevel
================
*/
int idCombatScaling::GetMaxLevel( void ) const {
	return maxLevel;
}

/*
================
idCombatScaling::GetGrowth
================
*/
float idCombatScaling::GetGrowth( void ) const {
	return growth;
}

/*
================
idCombatScaling::ClampLevel
================
*/
int idCombatScaling::ClampLevel( int level ) const {
	if ( level < minLevel ) {
		return minLevel;
	}
	if ( level > maxLevel ) {
		return maxLevel;
	}
	return level;
}

/*
================
idCombatScaling::LevelScale
================
*/
float idCombatScaling::LevelScale( int level ) const {
	assert( IsInitialized() );
	if ( !IsInitialized() ) {
		return 1.0f;
	}
	return cachedLevelScales[ ClampLevel( level ) - minLevel ];
}

/*
================
idCombatScaling::ScaleIntegralStat
================
*/
bool idCombatScaling::ScaleIntegralStat( int baseValue, int level, int &scaledValue ) const {
	if ( !IsInitialized() || baseValue < 0 ) {
		return false;
	}

	const double scaled = static_cast<double>( baseValue ) * LevelScale( level );
	const double rounded = floor( scaled + 0.5 );
	if ( rounded > INT_MAX ) {
		return false;
	}

	scaledValue = static_cast<int>( rounded );
	return true;
}

/*
================
idCombatScaling::AggregatePropertyMultipliers
================
*/
bool idCombatScaling::AggregatePropertyMultipliers( const float *multipliers, int count, float maximum, float &result ) {
	if ( count < 0 || ( count > 0 && !multipliers ) || maximum < 1.0f ) {
		return false;
	}

	double aggregate = 1.0;
	for( int i = 0; i < count; i++ ) {
		if ( multipliers[ i ] <= 0.0f ) {
			return false;
		}
		aggregate *= multipliers[ i ];
	}

	if ( aggregate > maximum ) {
		return false;
	}

	result = static_cast<float>( aggregate );
	return true;
}
