#include "glow.hpp"

#include "../sdk/sdk.hpp"

#undef max
#undef min

void features::glow::run ( ) {
	while ( true ) {
		std::this_thread::sleep_for ( 100ms );

		auto local = apex::local.get ( );

		if ( !local.is_valid ( ) )
			continue;

		const auto my_team = local.get_team ( );

		for ( auto i = 0; i < apex::max_players; i++ ) {
			const auto ent = apex::player ( apex::entity_list [ i ].ptr );

			if ( !ent.is_valid ( ) || ent.address ( ) == local.address ( ) || ent.get_team ( ) == my_team )
				continue;

			const auto health_norm = static_cast<float>( ent.is_downed( ) ? 0 : std::clamp ( ent.get_health ( ), 0, 100 ) ) / 100.0f;

			const auto ptr = ent.address ( );

			constexpr auto alpha = 20.0f;
			
			drv::write ( ptr + apex::offsets::glow_context + 0x90, true );
			drv::write ( ptr + apex::offsets::glow_context, 1 );
			drv::write ( ptr + apex::offsets::glow_color, std::array<float, 3>{( 1.0f - health_norm )* alpha, health_norm * alpha, 0.0f} );
			drv::write ( ptr + apex::offsets::glow_fade, std::array<float, 8>{std::numeric_limits<float>::max(), std::numeric_limits<float>::max ( ), std::numeric_limits<float>::max ( ), std::numeric_limits<float>::max ( ), std::numeric_limits<float>::max ( ), std::numeric_limits<float>::max( ), std::numeric_limits<float>::max( )} );
			drv::write ( ptr + apex::offsets::glow_distance, std::numeric_limits<float>::max ( ) );

			// flat chams std::array<uint8_t, 4>{ 101/*inside function*/, 102/*outside function*/, 20/*outline radius*/, 255/*custom state*/ }
			//drv::write ( ptr + apex::offsets::glow_type, std::array<uint8_t, 4>{ std::array<uint8_t , 4>{ 101/*inside function*/ , 102/*outside function*/ , 20/*outline radius*/ , 255/*custom state*/ } );
			//drv::write ( ptr + apex::offsets::glow_visible_type, std::array<uint8_t, 4>{ std::array<uint8_t , 4>{ 101/*inside function*/ , 102/*outside function*/ , 20/*outline radius*/ , 255/*custom state*/ } );
		}
	}
}