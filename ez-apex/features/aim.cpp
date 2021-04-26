#include "aim.hpp"

#include "../sdk/sdk.hpp"

#include "position.hpp"
#include "../options.hpp"

#undef max
#undef min

int cur_target = 0;

__forceinline void move_mouse ( const apex::player& local, apex::vec3 aim_angle ) {
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

/* account for acceleration later */
__forceinline apex::vec3 predict_pos ( const apex::player& local, const apex::weapon& weapon, const apex::player& target, apex::vec3 pos ) {
	const auto v0 = weapon.get_bullet_speed ( );

	/* dont predict for instant weapons */
	if ( v0 <= 1.0f )
		return pos;

	//const auto g = 750.0f;
	//const auto src = local.get_camera_pos( );
	//const auto dp = pos - src;
	//const auto theta = 45.0f + atan2( dp.y, dp.x );
	//const auto sin_theta = sin( theta );
	//const auto cos_theta = cos( theta );
	//const auto len = ( ( v0 * v0 ) / g ) * ( sin_theta + ( cos_theta * cos_theta ) * atanh( sin_theta ) );
	//const auto dt = len / v0;
	
	const auto src = local.get_camera_pos( );
	const auto g = 750.0f;
	const auto dt = src.dist_to( pos ) / v0;

	/* predict player movement */
	pos += ( target.get_velocity( ) - local.get_velocity() ) * dt;

	/* predict bullet drop / gravity / bullet time */
	pos.z += ( g * weapon.get_bullet_gravity( ) ) / 2.0f * ( dt * dt );

	return pos;
}

int features::aim::get_target( ) {
	return cur_target;
}

void features::aim::run ( ) {
	MUTATE_START;
	static auto& aimbot_emable = options::vars [ _ ( "aimbot.enable" ) ].val.b;
	static auto& aimbot_predict = options::vars [ _ ( "aimbot.predict" ) ].val.b;
	static auto& aimbot_stable = options::vars [ _ ( "aimbot.stable" ) ].val.b;
	static auto& aimbot_vischeck = options::vars [ _ ( "aimbot.vischeck" ) ].val.b;
	static auto& aimbot_downed = options::vars [ _ ( "aimbot.downed" ) ].val.b;
	static auto& aimbot_fov = options::vars [ _ ( "aimbot.fov" ) ].val.i;
	static auto& aimbot_smooth = options::vars [ _ ( "aimbot.smooth" ) ].val.i;
	static auto& aimbot_hitbox = options::vars [ _ ( "aimbot.hitbox" ) ].val.i;

	while ( true ) {
		if ( !aimbot_emable ) {
			apex::sleep ( 0.1 );
			cur_target = 0;
			continue;
		}

		apex::sleep( 0.015 );

		auto local = apex::local.get ( );
		
		if ( !local.is_valid( )
			|| local.is_downed( ) ) {
			cur_target = 0;
			continue;
		}

		auto weapon = local.get_weapon ( );

		if ( !weapon.address() ) {
			cur_target = 0;
			continue;
		}

		const auto angles = local.get_angles ( ).normalize_angle();
		
		/* aimbot / aim assist */
		if ( !GetAsyncKeyState(VK_RBUTTON ) ) {
			cur_target = 0;
			continue;
		}

		const auto camera = local.get_camera_pos( );

		if ( camera.is_zero( ) || !camera.is_valid( ) ) {
			cur_target = 0;
			continue;
		}

		auto calculate_fov = [ ] ( const apex::vec3& src, const apex::vec3& dst, const apex::vec3& ang ) {
			const auto ang_to = src.angle_to( dst );
			const auto delta_ang = ( ang_to - ang.normalize_angle( ) ).normalize_angle( );
			return delta_ang.length_2d( );
		};

		auto closest_target_idx = 0;
		auto closest_angle = apex::vec3( );
		auto closest_fov = std::numeric_limits<float>::max( );

		const auto my_team = local.get_team( );

		auto target_bone = 12;
		if ( aimbot_hitbox == 0 ) target_bone = 12;
		else if ( aimbot_hitbox == 1 ) target_bone = 10;
		else if ( aimbot_hitbox == 2 ) target_bone = 13;
		else if ( aimbot_hitbox == 3 ) target_bone = 7;
		else if ( aimbot_hitbox == 4 ) target_bone = 6;
		else if ( aimbot_hitbox == 5 ) target_bone = 5;

		for ( auto i = 0; i < apex::max_players; i++ ) {
			/* only aim at visible players */
			if ( aimbot_vischeck && !position::is_visible( i ) )
				continue;

			const auto ent = apex::player::get( i );

			if ( !ent.is_valid( ) || ent.address( ) == local.address( ) || ent.get_team( ) == my_team || ( aimbot_downed?false: ent.is_downed ( ) ) )
				continue;

			const auto target_pos = position::get_bone( i , target_bone /* bones_t::head */ );

			if ( !target_pos || target_pos.value( ).is_zero( ) || !target_pos.value( ).is_valid( ) )
				continue;

			const auto origin = ent.get_origin( );

			if ( origin.is_zero( ) || !origin.is_valid( ) )
				continue;
			
			auto absolute_pos = aimbot_predict ? predict_pos( local , weapon , ent , target_pos.value( ) + origin ) : ( target_pos.value ( ) + origin );
			const auto angle_to = camera.angle_to( absolute_pos ).normalize_angle( );

			if ( !angle_to.valid_angle( ) )
				continue;

			const auto fov = calculate_fov( camera , absolute_pos , angles );
			const auto dist = camera.dist_to( absolute_pos ) / 650.0f;
			const auto scaled_fov = std::clamp( static_cast< float >( aimbot_fov ) / dist , 0.8f , static_cast< float >( aimbot_fov ) );

			if ( fov > scaled_fov /* aimbot fov scaled by distance */ || fov >= closest_fov )
				continue;

			/* apply angle with bullet prediction */
			closest_angle = angle_to;
			closest_fov = fov;
			closest_target_idx = i;
		}

		if ( closest_fov != std::numeric_limits<float>::max( ) ) {
			if ( aimbot_stable ) {
				closest_angle -= local.get_dynamic_angles ( ) - local.get_angles ( );
				closest_angle = closest_angle.normalize_angle ( );
			}

			const auto delta_ang = ( closest_angle - local.get_angles ( ) ).normalize_angle ( );

			if ( delta_ang.valid_angle ( ) ) {
				const auto delta_ang_smoothed = delta_ang * pow ( static_cast< float >( 100.0f - aimbot_smooth ) / 100.0f, 2.2f );

				if ( delta_ang_smoothed.valid_angle ( ) )
					local.set_angles ( ( local.get_angles ( ) + delta_ang_smoothed ).normalize_angle ( ) );
			}

			cur_target = closest_target_idx;
		}
		else {
			cur_target = 0;
		}
	}
	MUTATE_END;
}