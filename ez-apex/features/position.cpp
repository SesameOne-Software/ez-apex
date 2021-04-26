#include "position.hpp"

#include <chrono>

std::array<double, apex::max_players> visible_change_timer { };
std::array<float, apex::max_players> last_visible_times { 0.0f };
std::array<bool, apex::max_players> players_visible { false };

std::array<double, apex::max_players> crosshair_change_timer { };
std::array<float , apex::max_players> last_crosshair_times { 0.0f };
std::array<bool , apex::max_players> in_crosshair { false };

std::array<std::optional<std::array<apex::matrix3x4, 128>>, apex::max_players> player_bones;

apex::vec3 camera_vel;
apex::vec3 last_position;
double last_position_time = 0.0f;

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
	MUTATE_START;
	while ( true ) {
		apex::sleep( 0.005 );

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
			const auto camera_time = static_cast< double >( std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::system_clock::now ( ).time_since_epoch ( ) ).count ( ) ) / 1000.0;

			camera_vel = ( pos - last_position ) / abs( camera_time - last_position_time );

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
			const auto now = static_cast<double>( std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::system_clock::now ( ).time_since_epoch ( ) ).count ( ) ) / 1000.0;

			if ( visible_time != last_visible_times [ i ] ) {
				visible_change_timer [ i ] = now;
				last_visible_times [ i ] = visible_time;
			}

			const auto crosshair_time = ent.get_crosshair_time( );

			if ( crosshair_time != last_crosshair_times[ i ] ) {
				crosshair_change_timer[ i ] = now;
				last_crosshair_times[ i ] = crosshair_time;
			}

			players_visible[ i ] = abs( now - visible_change_timer[ i ] ) < 0.02;
			in_crosshair[ i ] = abs( now - crosshair_change_timer[ i ] ) < 0.02;
			player_bones [ i ] = ent.get_bone_matrix ( );
		}
	}
	MUTATE_END;
}