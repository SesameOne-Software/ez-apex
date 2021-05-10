#include "glow.hpp"
#include "aim.hpp"

#include "../sdk/sdk.hpp"
#include "../options.hpp"

#undef max
#undef min

void features::glow::run ( ) {
	MUTATE_START;
	static auto& visuals_enable = options::vars [ _ ( "visuals.enable" ) ].val.b;
	static auto& visuals_chams = options::vars [ _ ( "visuals.chams" ) ].val.b;
	static auto& visuals_xqz = options::vars [ _ ( "visuals.xqz" ) ].val.b;
	static auto& visuals_health_based = options::vars [ _ ( "visuals.health_based" ) ].val.b;
	static auto& visuals_target = options::vars [ _ ( "visuals.target" ) ].val.b;
	static auto& visuals_chams_color = options::vars [ _ ( "visuals.chams_color" ) ].val.c;
	static auto& visuals_target_color = options::vars [ _ ( "visuals.target_color" ) ].val.c;

	while ( true ) {
		if ( !visuals_enable || !visuals_chams ) {
			apex::sleep ( 0.1 );
			continue;
		}

		apex::sleep( 0.050 );

		auto local = apex::local.get ( );

		if ( !local.is_valid() || local.get_origin ( ).z > 11000.0f )
			continue;

		const auto aim_target = aim::get_target( );
		const auto my_team = local.get_team ( );

		for ( auto i = 0; i < apex::max_players; i++ ) {
			const auto ent = apex::player::get( i );

			if ( !ent.is_valid ( ) || ent.address ( ) == local.address ( ) || ent.get_team ( ) == my_team || ent.get_origin ( ).z > 11000.0f )
				continue;

			const auto health_norm = static_cast<float>( ent.is_downed( ) ? 0 : std::clamp ( ent.get_health ( ), 0, 100 ) ) / 100.0f;
			const auto ptr = ent.address ( );
			constexpr auto alpha = 1.0f;
			
			drv::write ( ptr + apex::offsets::glow_context + 0x90, true );
			drv::write ( ptr + apex::offsets::glow_context, 1 );

			const auto name = ent.get_player_name ( );
			const auto is_streamer = name.size() >= 3 && tolower ( name [ 0 ] ) == 't' && tolower ( name [ 1 ] ) == 't' && tolower ( name [ 2 ] ) == 'v';

			if ( visuals_target &&( aim_target == i || is_streamer) )
				drv::write ( ptr + apex::offsets::glow_color, std::array<float, 3>{visuals_target_color.r* visuals_target_color.a, visuals_target_color.g* visuals_target_color.a, visuals_target_color.b* visuals_target_color.a} );
			else if ( visuals_health_based )
				drv::write ( ptr + apex::offsets::glow_color, std::array<float, 3>{( 1.0f - health_norm )* alpha, health_norm* alpha, 0.0f} );
			else
				drv::write ( ptr + apex::offsets::glow_color, std::array<float, 3>{visuals_chams_color.r* visuals_chams_color.a, visuals_chams_color.g* visuals_chams_color.a, visuals_chams_color.b* visuals_chams_color.a} );

			drv::write ( ptr + apex::offsets::glow_fade, std::array<float, 7>{std::numeric_limits<float>::max(), std::numeric_limits<float>::max ( ), std::numeric_limits<float>::max ( ), std::numeric_limits<float>::max ( ), std::numeric_limits<float>::max ( ), std::numeric_limits<float>::max ( ), std::numeric_limits<float>::max ( )} );
			drv::write ( ptr + apex::offsets::glow_distance, std::numeric_limits<float>::max ( ) );

			if ( !visuals_xqz ) {
				ent.set_highlight_funcs ( apex::HIGHLIGHT_CONTEXT_ENEMY, apex::HIGHLIGHT_FILL_NONE, true, apex::HIGHLIGHT_OUTLINE_CUSTOM_COLOR, 20.0f, 0, true );
				ent.set_highlight_funcs ( apex::HIGHLIGHT_CONTEXT_ENEMY, apex::HIGHLIGHT_FILL_NONE, false, apex::HIGHLIGHT_FILL_NONE, 20.0f, 0, true );
				ent.set_highlight_funcs ( apex::HIGHLIGHT_CONTEXT_SONAR, apex::HIGHLIGHT_FILL_NONE, true, apex::HIGHLIGHT_OUTLINE_CUSTOM_COLOR, 20.0f, 0, true );
				ent.set_highlight_funcs ( apex::HIGHLIGHT_CONTEXT_SONAR, apex::HIGHLIGHT_FILL_NONE, false, apex::HIGHLIGHT_FILL_NONE, 20.0f, 0, true );
			}
			else {
				ent.set_highlight_funcs ( apex::HIGHLIGHT_CONTEXT_ENEMY, apex::HIGHLIGHT_FILL_NONE, true, apex::HIGHLIGHT_OUTLINE_CUSTOM_COLOR, 20.0f, 0, true );
				ent.set_highlight_funcs ( apex::HIGHLIGHT_CONTEXT_ENEMY, apex::HIGHLIGHT_FILL_CUSTOM_COLOR, false, apex::HIGHLIGHT_OUTLINE_CUSTOM_COLOR, 20.0f, 0, true );
				ent.set_highlight_funcs ( apex::HIGHLIGHT_CONTEXT_SONAR, apex::HIGHLIGHT_FILL_NONE, true, apex::HIGHLIGHT_OUTLINE_CUSTOM_COLOR, 20.0f, 0, true );
				ent.set_highlight_funcs ( apex::HIGHLIGHT_CONTEXT_SONAR, apex::HIGHLIGHT_FILL_CUSTOM_COLOR, false, apex::HIGHLIGHT_OUTLINE_CUSTOM_COLOR, 0.0f, 0, true );
			}
		}
	}
	MUTATE_END;
}