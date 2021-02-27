#pragma once

#include "../sdk/sdk.hpp"

#include <array>

namespace features {
	namespace position {
		std::optional <apex::vec3> get_bone ( int ent_idx, int i );
		apex::vec3 camera_velocity( );
		bool is_visible ( int ent_idx );

		void run ( );
	}
}