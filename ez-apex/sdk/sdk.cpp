#include "sdk.hpp"

#include <windows.h>
#include <iostream>

apex::entity_type_t apex::identify_entity ( uintptr_t ent ) {
	if ( !ent )
		return entity_type_t::none;

	const auto networkable = drv::read<uintptr_t> ( ent + 24 );

	if ( !networkable )
		return entity_type_t::none;

	const auto get_client_class = drv::read<uintptr_t> ( networkable + 24 );

	if ( !get_client_class )
		return entity_type_t::none;

	const auto disp = drv::read<int> ( get_client_class + 3 );

	if ( !disp )
		return entity_type_t::none;

	struct client_class_t {
		uint64_t create_fn;
		uint64_t create_event_fn;
		uint64_t network_name;
		uint64_t recv_table;
		uint64_t next;
		uint32_t class_id;
		uint32_t class_size;
	};

	const auto client_class_ptr = drv::read<client_class_t> ( get_client_class + disp + 7 );

	if ( !client_class_ptr.network_name )
		return entity_type_t::none;

	auto class_name = drv::read<std::array<char, 128>> ( client_class_ptr.network_name );

	class_name [ class_name.size() - 1 ] = '\0';

	switch ( hash ( class_name.data ( ) ) ) {
	case hash ( "CPlayer" ): return entity_type_t::player;
	case hash ( "CAI_BaseNPC" ): return entity_type_t::npc;
	case hash ( "CPropSurvival" ): return entity_type_t::prop;
	case hash ( "CWeaponX" ): return entity_type_t::weapon;
	case hash ( "CWorld" ): return entity_type_t::world;
	}

	return entity_type_t::none;
}

bool apex::offsets::dump( ) {
	static bool tried_dump = false;

	if ( tried_dump ) {
		return false;
	}
	else {
		VM_DOLPHIN_WHITE_START;
		tried_dump = true;
		if ( !( entity_list = drv::pattern::search ( _ ( "45 33 C9 48 8D 05 ? ? ? ? 4C 89 0D" ) ).add ( 6 ).resolve_rip ( ).get<uintptr_t> ( ) ) ) return false;

		if ( !( local_entity = drv::pattern::search ( _ ( "48 8B 05 ? ? ? ? 48 0F 44 C7" ) ).add ( 3 ).resolve_rip ( ).get<uintptr_t> ( ) ) ) return false;

		if ( !( entity_index = 0x38 ) ) return false;

		if ( !( entity_life_state = drv::pattern::search ( _ ( "0F BE 81 ? ? ? ? C3" ) ).add ( 3 ).deref32 ( ).get<uintptr_t> ( ) ) ) return false;

		if ( !( player_name_list = drv::pattern::search ( _ ( "48 8D 05 ? ? ? ? 48 8B E9 41" ) ).add ( 3 ).resolve_rip ( ).get<uintptr_t> ( ) ) ) return false;

		if ( !( entity_health = 0x438 ) ) return false;

		if ( !( entity_team_num = entity_health + 0x10 ) ) return false;

		if ( !( player_bleedout_state = drv::pattern::search ( _ ( "8B 81 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC 40 53 48 83 EC 20 48 8B D9 48 8B CA" ) ).add ( 2 ).deref32 ( ).get<uintptr_t> ( ) ) ) return false;

		if ( !( player_angles = drv::pattern::search ( _ ( "F2 0F 10 B6" ) ).add ( 4 ).deref32 ( ).get<uint32_t> ( ) ) ) return false;

		if ( !( player_dynamic_angles = player_angles - 0x10 ) ) return false;

		if ( !( player_bones = drv::pattern::search ( _ ( "48 8B 97 ? ? ? ? 48 8D 0C 40 0F 28 35" ) ).add ( 3 ).deref32 ( ).get<uint32_t> ( ) ) ) return false;

		if ( !( player_last_primary_weapon = drv::pattern::search ( _ ( "48 C7 44 24 24 ? ? ? ? 48 ? ? ? ? 33 D2 89 74 24 20" ) ).add ( 5 ).deref32 ( ).get<uint32_t> ( ) ) ) return false;

		if ( !( player_camera = drv::pattern::search ( _ ( "F3 0F 10 83 ? ? ? ? F3 0F 10 8B ? ? ? ? F3 0F 11 45 EF" ) ).add ( 4 ).deref32 ( ).get<uint32_t> ( ) ) ) return false;

		if ( !( player_velocity = drv::pattern::search ( _ ( "F2 0F 10 97 ? ? ? ? F3" ) ).add ( 4 ).deref32 ( ).get<uint32_t> ( ) ) ) return false;

		if ( !( player_last_visible_time = drv::pattern::search ( _ ( "8B 8F ? ? ? ? 89 08 48 8D 15" ) ).add ( 2 ).deref32 ( ).get<uint32_t> ( ) ) ) return false;

		if ( !( player_last_crosshair_target_time = player_last_visible_time + 0x8 ) ) return false;

		if ( !( weapon_ammo_in_clip = drv::pattern::search ( _ ( "8B 83 ? ? ? ? 48 ? ? ? ? 48 ? ? ? ? 48 83 C4 20 5F C3 8B 93" ) ).add ( 2 ).deref32 ( ).get<uint32_t> ( ) + 0x10 ) ) return false;

		if ( !( weapon_bullet_speed = drv::pattern::search ( _ ( "F3 0F 59 B3 ? ? ? ? 0F 29 BC 24" ) ).add ( 4 ).deref32 ( ).get<uint32_t> ( ) ) ) return false;

		if ( !( weapon_bullet_gravity = drv::pattern::search ( _ ( "F3 0F 10 93 ? ? ? ? F3 0F 5E C1 F3 0F 59 50 68" ) ).add ( 4 ).deref32 ( ).get<uint32_t> ( ) ) ) return false;

		if ( !( weapon_definition_index = drv::pattern::search ( _ ( "48 89 9B ? ? ? ? 4C 89 A3 ? ? ? ? 4C 89 A3" ) ).add ( 3 ).deref32 ( ).get<uint32_t> ( ) ) ) return false;

		if ( !( glow_context = drv::pattern::search ( _ ( "40 57 48 83 EC 20 8D 42 01" ) ).add ( 23 ).deref32 ( ).get<uint32_t> ( ) ) ) return false;

		if ( !( glow_life_time = glow_context - 36 ) ) return false;

		if ( !( glow_distance = glow_context - 20 ) ) return false;

		if ( !( glow_type = drv::pattern::search ( _ ( "8B 84 81 ? ? ? ? C1 E8 18" ) ).add ( 3 ).deref32 ( ).get<ptrdiff_t> ( ) + 4 ) ) return false;

		if ( !( glow_color = drv::pattern::search ( _ ( "E8 ? ? ? ? 8B 8F ? ? ? ? F6 C1 02" ) ).add ( 1 ).resolve_rip ( ).add ( 2 ).deref32 ( ).get<ptrdiff_t> ( ) + 12 * 2 ) ) return false;

		if ( !( glow_visible_type = glow_context + 16 ) ) return false;

		if ( !( glow_fade = glow_context - 64 ) ) return false;

		if ( !( input_system = drv::pattern::search ( _ ( "48 83 EC 28 0F B6 D1 4C 8D 05 ? ? ? ? 48 8B 0D ? ? ? ? 48 8B 01 49 3B" ) ).add ( 17 ).resolve_rip ( ).deref ( ).get<ptrdiff_t> ( ) ) ) return false;

		if ( !( game_window = drv::pattern::search ( _ ( "48 83 EC 38 48 8B 0D ? ? ? ? 48 85 C9 0F 84 ? ? ? ? BA FC FF FF FF" ) ).add ( 7 ).resolve_rip ( ).get<ptrdiff_t> ( ) ) ) return false;
		backup_game_window = drv::read<HWND> ( game_window );

		if ( !( global_vars = drv::pattern::search ( _ ( "48 8B 05 ? ? ? ? 8B 48 10 89 8B" ) ).add ( 3 ).resolve_rip ( ).deref ( ).get<ptrdiff_t> ( ) ) ) return false;

		return true;
		VM_DOLPHIN_WHITE_END;
	}
	
	return false;
}

bool apex::init ( ) {
	if ( !( base = drv::get_base ( ) ) )
		return false;

	if(!offsets::dump ( ) )
		return false;

	return true;
}