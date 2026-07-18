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

#include "game/CombatScaling.h"
#include "framework/Common.h"

#include <cmath>
#include <cstdio>
#include <stdexcept>

idCommon *common = NULL;

namespace {

void Expect( bool condition, const char *message ) {
	if ( !condition ) {
		throw std::runtime_error( message );
	}
}

bool Near( float actual, float expected, float epsilon = 0.0001f ) {
	return fabs( actual - expected ) <= epsilon;
}

idDict ValidConfig( int minLevel = 1, int maxLevel = 30, const char *growth = "1.20" ) {
	idDict config;
	config.Set( "config_type", "combat_progression" );
	config.SetInt( "min_level", minLevel );
	config.SetInt( "max_level", maxLevel );
	config.Set( "level_growth", growth );
	return config;
}

void TestLevelScaling( void ) {
	idCombatScaling scaling;
	idStr error;
	const idDict config = ValidConfig();
	Expect( scaling.Init( config, error ), error.c_str() );

	Expect( scaling.IsInitialized(), "scaling was not initialized" );
	Expect( scaling.GetMinLevel() == 1, "unexpected minimum level" );
	Expect( scaling.GetMaxLevel() == 30, "unexpected maximum level" );
	Expect( Near( scaling.GetGrowth(), 1.2f ), "unexpected growth" );
	Expect( Near( scaling.LevelScale( 1 ), 1.0f ), "S(1) must equal 1" );

	for( int level = 1; level < scaling.GetMaxLevel(); level++ ) {
		const float ratio = scaling.LevelScale( level + 1 ) / scaling.LevelScale( level );
		Expect( Near( ratio, scaling.GetGrowth() ), "adjacent level ratio differs from growth" );
	}

	Expect( scaling.ClampLevel( -1 ) == 1, "negative level was not clamped" );
	Expect( scaling.ClampLevel( 0 ) == 1, "zero level was not clamped" );
	Expect( scaling.ClampLevel( 31 ) == 30, "excessive level was not clamped" );
	Expect( Near( scaling.LevelScale( 0 ), scaling.LevelScale( 1 ) ), "low clamped scale differs" );
	Expect( Near( scaling.LevelScale( 31 ), scaling.LevelScale( 30 ) ), "high clamped scale differs" );
}

void TestConfigurationValidation( void ) {
	idCombatScaling scaling;
	idStr error;

	idDict config = ValidConfig( 0, 30 );
	Expect( !scaling.Init( config, error ), "zero minimum level was accepted" );

	config = ValidConfig( 5, 4 );
	Expect( !scaling.Init( config, error ), "reversed level range was accepted" );

	config = ValidConfig( 1, 30, "1" );
	Expect( !scaling.Init( config, error ), "unit growth was accepted" );

	config = ValidConfig( 1, MAX_CACHED_COMBAT_LEVELS + 1, "1.00001" );
	Expect( !scaling.Init( config, error ), "oversized cache was accepted" );

	config = ValidConfig( 1, 20, "10" );
	Expect( !scaling.Init( config, error ), "integral-stat scale overflow was accepted" );

	config = ValidConfig();
	config.Delete( "level_growth" );
	Expect( !scaling.Init( config, error ), "missing growth was accepted" );

	config = ValidConfig();
	config.Set( "config_type", "other" );
	Expect( !scaling.Init( config, error ), "wrong config type was accepted" );
}

void TestIntegralStatScaling( void ) {
	idCombatScaling scaling;
	idStr error;
	const idDict config = ValidConfig( 1, 30, "2" );
	Expect( scaling.Init( config, error ), error.c_str() );

	int scaledValue = -1;
	Expect( scaling.ScaleIntegralStat( 100, 1, scaledValue ), "level-one stat scaling failed" );
	Expect( scaledValue == 100, "level-one stat changed" );
	Expect( scaling.ScaleIntegralStat( 1, 2, scaledValue ), "level-two stat scaling failed" );
	Expect( scaledValue == 2, "level-two stat was rounded incorrectly" );
	Expect( !scaling.ScaleIntegralStat( -1, 1, scaledValue ), "negative stat was accepted" );
	Expect( !scaling.ScaleIntegralStat( 5, 30, scaledValue ), "overflowing stat was accepted" );

	const idDict fractionalConfig = ValidConfig( 1, 2, "1.2" );
	Expect( scaling.Init( fractionalConfig, error ), error.c_str() );
	Expect( scaling.ScaleIntegralStat( 3, 2, scaledValue ), "fractional stat scaling failed" );
	Expect( scaledValue == 4, "fractional stat was not rounded to nearest" );
}

void TestPropertyAggregation( void ) {
	float result = 0.0f;
	Expect( idCombatScaling::AggregatePropertyMultipliers( NULL, 0, 1.5f, result ), "empty property set failed" );
	Expect( Near( result, 1.0f ), "empty property set was not neutral" );

	const float valid[] = { 1.1f, 1.2f };
	Expect( idCombatScaling::AggregatePropertyMultipliers( valid, 2, 1.4f, result ), "valid properties failed" );
	Expect( Near( result, 1.32f ), "properties were aggregated incorrectly" );
	Expect( !idCombatScaling::AggregatePropertyMultipliers( valid, 2, 1.3f, result ), "aggregate limit was ignored" );

	const float zero[] = { 1.1f, 0.0f };
	Expect( !idCombatScaling::AggregatePropertyMultipliers( zero, 2, 1.4f, result ), "zero multiplier was accepted" );

	Expect( !idCombatScaling::AggregatePropertyMultipliers( NULL, 1, 1.4f, result ), "missing multiplier array was accepted" );
	Expect( !idCombatScaling::AggregatePropertyMultipliers( valid, -1, 1.4f, result ), "negative property count was accepted" );
}

}

int main( int argc, char **argv ) {
	try {
		TestLevelScaling();
		TestConfigurationValidation();
		TestIntegralStatScaling();
		TestPropertyAggregation();
	} catch( const std::exception &exception ) {
		std::fprintf( stderr, "combat_scaling_test failed: %s\n", exception.what() );
		return 1;
	}

	std::printf( "combat_scaling_test passed\n" );
	return 0;
}
