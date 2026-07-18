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

#ifndef __GAME_COMBATSCALING_H__
#define __GAME_COMBATSCALING_H__

#include "idlib/Dict.h"
#include "idlib/containers/List.h"

const int MAX_CACHED_COMBAT_LEVELS = 4096;

class idCombatScaling {
public:
							idCombatScaling();

	bool					Init( const idDict &config, idStr &error );
	bool					IsInitialized( void ) const;

	int						GetMinLevel( void ) const;
	int						GetMaxLevel( void ) const;
	float					GetGrowth( void ) const;

	int						ClampLevel( int level ) const;
	float					LevelScale( int level ) const;
	bool					ScaleIntegralStat( int baseValue, int level, int &scaledValue ) const;

	static bool				AggregatePropertyMultipliers( const float *multipliers, int count, float maximum, float &result );

private:
	void					Clear( void );

	int						minLevel;
	int						maxLevel;
	float					growth;
	idList<float>			cachedLevelScales;
};

#endif
