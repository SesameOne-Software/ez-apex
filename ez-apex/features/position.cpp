#include "position.hpp"

#include <chrono>

std::array<std::chrono::high_resolution_clock::time_point, apex::max_players> visible_change_timer { };
std::array<float, apex::max_players> last_visible_times { 0.0f };
std::array<bool, apex::max_players> players_visible { false };

std::array<std::chrono::high_resolution_clock::time_point , apex::max_players> crosshair_change_timer { };
std::array<float , apex::max_players> last_crosshair_times { 0.0f };
std::array<bool , apex::max_players> in_crosshair { false };

std::array<std::optional<std::array<apex::matrix3x4, 128>>, apex::max_players> player_bones;

apex::vec3 camera_vel;
apex::vec3 last_position;
std::chrono::high_resolution_clock::time_point last_position_time;

std::optional <apex::vec3> features::position::get_bone ( int ent_idx, int i ) {
	if ( !player_bones [ ent_idx ] )
		return std::nullopt;

	return player_bones [ ent_idx ].value()[i].origin();
}

apex::vec3 features::position::camera_velocity( ) {
	return camera_vel;
}

bool features::position::is_visible ( int ent_idx ) {
	return players_visible [ ent_idx ];
}

void features::position::run ( ) {
	while ( true ) {
		apex::sleep( 0.001 );

		auto local = apex::local.get ( );

		if ( !local.is_valid ( ) ) {
			last_visible_times.fill( 0.0f );
			last_crosshair_times.fill( 0.0f );
			player_bones.fill ( std::nullopt );
			players_visible.fill( false );
			in_crosshair.fill( false );
			continue;
		}

		/* solve for local camera velocity */ {
			const auto pos = local.get_camera_pos( );
			const auto camera_time = std::chrono::high_resolution_clock::now( );

			camera_vel = ( pos - last_position ) / abs( static_cast< float >( std::chrono::duration_cast< std::chrono::milliseconds >( camera_time - last_position_time ).count( ) ) / 1000.0f );

			last_position = pos;
			last_position_time = camera_time;
		}

		for ( auto i = 0; i < apex::max_players; i++ ) {
			const auto ent = apex::player::get( i );

			if ( !ent.is_valid ( ) || ent.address ( ) == local.address ( ) ) {
				last_visible_times[ i ] = 0.0f;
				last_crosshair_times [ i ] = 0.0f;
				player_bones [ i ] = std::nullopt;
				players_visible[ i ] = false;
				in_crosshair[ i ] = false;
				continue;
			}

			const auto visible_time = ent.get_visible_time ( );
			
			if ( visible_time != last_visible_times [ i ] ) {
				visible_change_timer [ i ] = std::chrono::high_resolution_clock::now ( );
				last_visible_times [ i ] = visible_time;
			}

			const auto crosshair_time = ent.get_crosshair_time( );

			if ( crosshair_time != last_crosshair_times[ i ] ) {
				crosshair_change_timer[ i ] = std::chrono::high_resolution_clock::now( );
				last_crosshair_times[ i ] = crosshair_time;
			}

			players_visible[ i ] = abs( std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::high_resolution_clock::now( ) - visible_change_timer[ i ] ).count( ) ) < 10;
			in_crosshair[ i ] = abs( std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::high_resolution_clock::now( ) - crosshair_change_timer[ i ] ).count( ) ) < 10;
			player_bones [ i ] = ent.get_bone_matrix ( );
		}
	}
}