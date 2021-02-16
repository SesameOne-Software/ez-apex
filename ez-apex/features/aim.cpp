#include "aim.hpp"

#include "../sdk/sdk.hpp"

#include "position.hpp"

#undef max
#undef min

void move_mouse ( const apex::player& local, apex::vec3 aim_angle ) {
    if ( GetForegroundWindow ( ) != drv::window || !aim_angle.valid_angle ( ) )
        return;
	
    constexpr auto sens = 2.5f;
    constexpr auto pitch_rate = 0.032f;
    constexpr auto yaw_rate = 0.032f;

	aim_angle -= local.get_angles ( );
	aim_angle = aim_angle.normalize_angle ( );

	if ( !aim_angle.valid_angle ( ) )
		return;
	
	INPUT input;

	input.type = INPUT_MOUSE;
	input.mi.dx = static_cast< long >( -aim_angle.x / ( pitch_rate * sens ) );
	input.mi.dy = static_cast< long >( aim_angle.y / ( yaw_rate * sens ) );
	input.mi.mouseData = 0;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	input.mi.time = 0;
	input.mi.dwExtraInfo = 0;

	SendInput ( 1, &input, sizeof ( input ) );
}

apex::vec3 predict_pos ( const apex::player& local, const apex::weapon& weapon, const apex::player& target, apex::vec3 pos ) {
	const auto bullet_speed = weapon.get_bullet_speed ( );

	/* dont predict for instant weapons */
	if ( bullet_speed <= 1.0f )
		return pos;
	
	/* approxiamate bullet displacement per second (linear) */
	const auto t = local.get_camera_pos ( ).dist_to( pos ) / bullet_speed;

	/* predict player movement */
	pos += target.get_velocity() * t;

	/* predict bullet drop / gravity / bullet time */
	pos.z += ( 700.0f * weapon.get_bullet_gravity ( ) * 0.5f ) * ( t * t );

	return pos;
}

void features::aim::run ( ) {
	while ( true ) {
		std::this_thread::sleep_for ( 16ms );

		auto local = apex::local.get ( );

		if ( !local.is_valid ( )
			|| local.is_downed ( ) )
			continue;

		const auto weapon = local.get_weapon ( );

		if ( !weapon.is_valid ( ) || !weapon.is_gun() )
			continue;

		const auto angles = local.get_angles ( );
		
		/* aimbot / aim assist */
		if ( GetAsyncKeyState ( VK_RBUTTON ) ) {
			apex::vec3 aimbot_angle = angles;

			const auto camera = local.get_camera_pos ( );

			if ( camera.is_zero ( ) || !camera.is_valid ( ) )
				continue;

			auto calculate_fov = [ ] ( const apex::vec3& src, const apex::vec3& dst ) {
				apex::vec3 ang = src.to_vec ( ), aim = dst.to_vec ( );
				return apex::rad_to_deg ( acos ( aim.dot ( ang ) / aim.length_sqr ( ) ) );
			};

			auto closest_fov = std::numeric_limits<float>::max ( );

			const auto my_team = local.get_team ( );

			for ( auto i = 0; i < apex::max_players; i++ ) {
				/* only aim at visible players */
				if ( !position::is_visible ( i ) )
					continue;

				const auto ent = apex::player ( apex::entity_list [ i ].ptr );

				if ( !ent.is_valid ( ) || ent.address ( ) == local.address ( ) || ent.get_team ( ) == my_team )
					continue;

				const auto target_pos = position::get_bone ( i, 12 /* bones_t::head */ );

				if ( !target_pos || target_pos.value().is_zero () || !target_pos.value ( ).is_valid() )
					continue;

				const auto origin = ent.get_origin ( );

				if ( origin.is_zero ( ) || !origin.is_valid ( ) )
					continue;

				auto absolute_pos = target_pos.value ( ) + origin;
				const auto angle_to = camera.angle_to ( absolute_pos ).normalize_angle ( );

				if ( !angle_to.valid_angle ( ) )
					continue;

				const auto fov = calculate_fov ( angles, angle_to );

				if ( fov > 10.0f /* aimbot fov */ || fov > closest_fov )
					continue;

				/* apply angle with bullet prediction */
				aimbot_angle = camera.angle_to ( predict_pos( local, weapon, ent, absolute_pos ) ).normalize_angle ( );
				closest_fov = fov;
			}

			if ( closest_fov != std::numeric_limits<float>::max ( ) ) {
				const auto delta_ang = (aimbot_angle - local.get_dynamic_angles ( )).normalize_angle ( );

				if ( delta_ang.valid_angle ( ) ) {
					const auto delta_ang_smoothed = ( delta_ang - ( delta_ang * ( 1.0f - 0.016f ) /* change smoothing based on distance */ ) ).normalize_angle ( );

					if ( delta_ang_smoothed.valid_angle ( ) )
						local.set_angles ( ( local.get_angles ( ) + delta_ang_smoothed ).normalize_angle ( ) );
				}
			}
		}
	}
}