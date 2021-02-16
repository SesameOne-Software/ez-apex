#include "position.hpp"

#include <chrono>

std::array<std::chrono::steady_clock::time_point, apex::max_players> visible_change_timer { };
std::array<float, apex::max_players> last_visible_times { 0.0f };
std::array<bool, apex::max_players> players_visible { false };
std::array<std::optional<std::array<apex::matrix3x4, 128>>, apex::max_players> player_bones;

std::optional <apex::vec3> features::position::get_bone ( int ent_idx, int i ) {
	if ( !player_bones [ ent_idx ] )
		return std::nullopt;

	return player_bones [ ent_idx ].value()[i].origin();
}

bool features::position::is_visible ( int ent_idx ) {
	return players_visible [ ent_idx ];
}

void features::position::run ( ) {
	while ( true ) {
		std::this_thread::sleep_for ( 16ms );

		auto local = apex::local.get ( );

		if ( !local.is_valid ( ) ) {
			last_visible_times.fill ( 0.0f );
			player_bones.fill ( std::nullopt );
			players_visible.fill ( false );
			continue;
		}

		for ( auto i = 0; i < apex::max_players; i++ ) {
			const auto ent = apex::player ( apex::entity_list [ i ].ptr );

			if ( !ent.is_valid ( ) || ent.address ( ) == local.address ( ) ) {
				last_visible_times [ i ] = 0.0f;
				player_bones [ i ] = std::nullopt;
				players_visible [ i ] = false;
				continue;
			}

			const auto visible_time = ent.get_visible_time ( );
			
			if ( visible_time != last_visible_times [ i ] ) {
				visible_change_timer [ i ] = std::chrono::steady_clock::now ( );
				last_visible_times [ i ] = visible_time;
			}

			players_visible[i] = abs(std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::high_resolution_clock::now ( ) - visible_change_timer [ i ] ).count ( )) < 20;
			player_bones [ i ] = ent.get_bone_matrix ( );
		}
	}
}