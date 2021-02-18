#include "sdk.hpp"

#include <windows.h>
#include <iostream>

apex::entity_type_t apex::identify_entity ( uintptr_t ent ) {
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
	if ( !( entity_list = drv::pattern::search( "45 33 C9 48 8D 05 ? ? ? ? 4C 89 0D" ).add( 6 ).resolve_rip( ).get<uintptr_t>( ) ) ) return false;
	std::cout << "offsets::entity_list @ 0x" << std::uppercase << std::hex << entity_list << std::dec << std::nouppercase << std::endl;

	if ( !( local_entity = drv::pattern::search( "48 8B 05 ? ? ? ? 48 0F 44 C7" ).add( 3 ).resolve_rip( ).get<uintptr_t>( ) ) ) return false;
	std::cout << "offsets::local_entity @ 0x" << std::uppercase << std::hex << local_entity << std::dec << std::nouppercase << std::endl;

	if ( !( entity_life_state = 0x0798 ) ) return false;
	std::cout << "offsets::entity_life_state @ 0x" << std::uppercase << std::hex << entity_life_state << std::dec << std::nouppercase << std::endl;

	if ( !( entity_health = 0x0440 ) ) return false;
	std::cout << "offsets::entity_health @ 0x" << std::uppercase << std::hex << entity_health << std::dec << std::nouppercase << std::endl;

	if ( !( entity_team_num = 0x0450 ) ) return false;
	std::cout << "offsets::entity_team_num @ 0x" << std::uppercase << std::hex << entity_team_num << std::dec << std::nouppercase << std::endl;

	if ( !( player_bleedout_state = 0x25F0 ) ) return false;
	std::cout << "offsets::player_bleedout_state @ 0x" << std::uppercase << std::hex << player_bleedout_state << std::dec << std::nouppercase << std::endl;

	if ( !( player_angles = drv::pattern::search( "F2 0F 10 B6" ).add( 4 ).deref32( ).get<uint32_t>( ) ) ) return false;
	std::cout << "offsets::player_angles @ 0x" << std::uppercase << std::hex << player_angles << std::dec << std::nouppercase << std::endl;

	if ( !( player_dynamic_angles = player_angles - 0x10 ) ) return false;
	std::cout << "offsets::player_dynamic_angles @ 0x" << std::uppercase << std::hex << player_dynamic_angles << std::dec << std::nouppercase << std::endl;

	if ( !( player_last_primary_weapon = 0x19EC ) ) return false;
	std::cout << "offsets::player_last_primary_weapon @ 0x" << std::uppercase << std::hex << player_last_primary_weapon << std::dec << std::nouppercase << std::endl;

	if ( !( weapon_ammo_in_clip = 0x1634 ) ) return false;
	std::cout << "offsets::weapon_ammo_in_clip @ 0x" << std::uppercase << std::hex << weapon_ammo_in_clip << std::dec << std::nouppercase << std::endl;

	if ( !( glow_context = drv::pattern::search( "40 57 48 83 EC 20 8D 42 01" ).add( 23 ).deref32( ).get<uint32_t>( ) ) ) return false;
	std::cout << "offsets::glow_context @ 0x" << std::uppercase << std::hex << glow_context << std::dec << std::nouppercase << std::endl;

	if ( !( glow_life_time = glow_context - 36 ) ) return false;
	std::cout << "offsets::glow_life_time @ 0x" << std::uppercase << std::hex << glow_life_time << std::dec << std::nouppercase << std::endl;

	if ( !( glow_distance = glow_context - 20 ) ) return false;
	std::cout << "offsets::glow_distance @ 0x" << std::uppercase << std::hex << glow_distance << std::dec << std::nouppercase << std::endl;

	if ( !( glow_type = drv::pattern::search( "8B 84 81 ? ? ? ? C1 E8 18" ).add( 3 ).deref32( ).get<ptrdiff_t>( ) + 4 ) ) return false;
	std::cout << "offsets::glow_type @ 0x" << std::uppercase << std::hex << glow_type << std::dec << std::nouppercase << std::endl;

	if ( !( glow_color = drv::pattern::search( "E8 ? ? ? ? 8B 8F ? ? ? ? F6 C1 02" ).add( 1 ).resolve_rip( ).add( 2 ).deref32( ).get<ptrdiff_t>( ) + 12 * 2 ) ) return false;
	std::cout << "offsets::glow_color @ 0x" << std::uppercase << std::hex << glow_color << std::dec << std::nouppercase << std::endl;

	if ( !( glow_visible_type = glow_context + 16 ) ) return false;
	std::cout << "offsets::glow_visible_type @ 0x" << std::uppercase << std::hex << glow_visible_type << std::dec << std::nouppercase << std::endl;

	if ( !( glow_fade = glow_context - 64 ) ) return false;
	std::cout << "offsets::glow_fade @ 0x" << std::uppercase << std::hex << glow_fade << std::dec << std::nouppercase << std::endl;

	return true;
}

void apex::update ( ) {
	while ( true ) {
		std::this_thread::sleep_for ( 25ms );

		drv::read ( reinterpret_cast< void* >( offsets::entity_list ), entity_list.data ( ), sizeof ( entity_list ) );

		//for ( auto i = 0; i < apex::max_players; i++ )
		//	if ( entity_list [ i ].ptr )
		//		entity_types [ i ] = identify_entity ( entity_list [ i ].ptr );
	}
}

bool apex::init ( ) {
	if ( !( base = drv::get_base ( ) ) )
		return false;

	if(!offsets::dump ( ) )
		return false;

	std::thread ( update ).detach ( );

	return true;
}