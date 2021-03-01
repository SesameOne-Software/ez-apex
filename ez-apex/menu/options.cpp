#include "options.hpp"
#include "../security/security.hpp"
#include "../tinyxml2/tinyxml2.h"

void options::option::add_bool( const std::string& id, bool val ) {
	vars [ id ].type = option_type_t::boolean;
	vars [ id ].val.b = val;
}

void options::option::add_list( const std::string& id, int count ) {
	vars [ id ].type = option_type_t::list;
	vars [ id ].list_size = count;
	memset( vars [ id ].val.l, 0, sizeof( vars [ id ].val.l ) );
}

void options::option::add_int( const std::string& id, int val ) {
	vars [ id ].type = option_type_t::integer;
	vars [ id ].val.i = val;
}

void options::option::add_float( const std::string& id, float val ) {
	vars [ id ].type = option_type_t::floating_point;
	vars [ id ].val.f = val;
}

void options::option::add_str( const std::string& id, const char* val ) {
	vars [ id ].type = option_type_t::string;
	strcpy_s( vars [ id ].val.s, val );
}

void options::option::add_color( const std::string& id, const colorf& val ) {
	vars [ id ].type = option_type_t::color;
	vars [ id ].val.c = val;
}

std::vector< std::string > split( const std::string& str, const std::string& delim ) {
	std::vector< std::string > tokens;
	size_t prev = 0, pos = 0;

	do {
		pos = str.find( delim, prev );

		if ( pos == std::string::npos )
			pos = str.length( );

		std::string token = str.substr( prev, pos - prev );

		if ( !token.empty( ) )
			tokens.push_back( token );

		prev = pos + delim.length( );
	} while ( pos < str.length( ) && prev < str.length( ) );

	return tokens;
}

void options::save( const std::unordered_map< std::string, option >& options, const std::string& path ) {
	tinyxml2::XMLDocument doc;

	const auto root = doc.NewElement( _( "sesame" ) );

	doc.InsertFirstChild( root );

	for ( auto& option : options ) {
		const auto element = doc.NewElement( option.first.data( ) );

		switch ( option.second.type ) {
			case option_type_t::boolean: {
				element->SetText( option.second.val.b );
			} break;
			case option_type_t::list: {
				for ( auto i = 0; i < option.second.list_size; i++ ) {
					const auto list_element = doc.NewElement( _( "item" ) );
					list_element->SetText( option.second.val.l [ i ] );
					element->InsertEndChild( list_element );
				}
			} break;
			case option_type_t::integer: {
				element->SetText( option.second.val.i );
			} break;
			case option_type_t::floating_point: {
				element->SetText( option.second.val.f );
			} break;
			case option_type_t::string: {
				element->SetText( ( char* )option.second.val.s );
			} break;
			case option_type_t::color: {
				const auto r_element = doc.NewElement( _( "r" ) );
				const auto g_element = doc.NewElement( _( "g" ) );
				const auto b_element = doc.NewElement( _( "b" ) );
				const auto a_element = doc.NewElement( _( "a" ) );

				r_element->SetText( option.second.val.c.r );
				g_element->SetText( option.second.val.c.g );
				b_element->SetText( option.second.val.c.b );
				a_element->SetText( option.second.val.c.a );

				element->InsertEndChild( r_element );
				element->InsertEndChild( g_element );
				element->InsertEndChild( b_element );
				element->InsertEndChild( a_element );
			} break;
		}

		root->InsertEndChild( element );
	}

	doc.SaveFile( path.data( ) );
}

void options::load( std::unordered_map< std::string, option >& options, const std::string& path ) {
	tinyxml2::XMLDocument doc;

	const auto err = doc.LoadFile( path.data( ) );

	if ( err != tinyxml2::XML_SUCCESS ) {
		//dbg_print( _( "Failed to open configuration file.\n" ) );
		return;
	}

	const auto root = doc.FirstChild( );

	if ( !root ) {
		//dbg_print( _( "Failed to open configuration file.\n" ) );
		return;
	}

	for ( auto& option : options ) {
		const auto element = root->FirstChildElement( option.first.data( ) );

		if ( !element ) {
			//dbg_print( _( "Failed to find element.\n" ) );
			continue;
		}

		auto err = tinyxml2::XML_SUCCESS;

		switch ( option.second.type ) {
			case option_type_t::boolean: {
				err = element->QueryBoolText( &option.second.val.b );
			} break;
			case option_type_t::list: {
				auto list_element = element->FirstChildElement( _( "item" ) );
				auto i = 0;

				if ( !list_element ) {
					//dbg_print( _( "Element found had invalid value.\n" ) );
					continue;
				}

				while ( list_element && i < option.second.list_size ) {
					err = list_element->QueryBoolText( &option.second.val.l [ i ] );

					if ( err != tinyxml2::XML_SUCCESS ) {
						//dbg_print( _( "Element found had invalid value.\n" ) );
						break;
					}

					list_element = list_element->NextSiblingElement( _( "item" ) );
					i++;
				}
			} break;
			case option_type_t::integer: {
				err = element->QueryIntText( &option.second.val.i );
			} break;
			case option_type_t::floating_point: {
				err = element->QueryFloatText( &option.second.val.f );
			} break;
			case option_type_t::string: {
				const auto str = element->GetText( );
				
				if ( !str ) {
					//dbg_print( _( "Element found had invalid value.\n" ) );
					continue;
				}

				strcpy_s( option.second.val.s, str );
			} break;
			case option_type_t::color: {
				const auto r_element = element->FirstChildElement( _( "r" ) );
				const auto g_element = element->FirstChildElement( _( "g" ) );
				const auto b_element = element->FirstChildElement( _( "b" ) );
				const auto a_element = element->FirstChildElement( _( "a" ) );

				if ( !r_element || !g_element || !b_element || !a_element ) {
					//dbg_print( _( "Element found had invalid value.\n" ) );
					continue;
				}

				err = r_element->QueryFloatText( &option.second.val.c.r );

				if ( err != tinyxml2::XML_SUCCESS ) {
					//dbg_print( _( "Element found had invalid value.\n" ) );
					continue;
				}

				err = g_element->QueryFloatText( &option.second.val.c.g );

				if ( err != tinyxml2::XML_SUCCESS ) {
					//dbg_print( _( "Element found had invalid value.\n" ) );
					continue;
				}

				err = b_element->QueryFloatText( &option.second.val.c.b );

				if ( err != tinyxml2::XML_SUCCESS ) {
					//dbg_print( _( "Element found had invalid value.\n" ) );
					continue;
				}

				err = a_element->QueryFloatText( &option.second.val.c.a );
			} break;
		}

		if ( err != tinyxml2::XML_SUCCESS ) {
			//dbg_print( _( "Element found had invalid value.\n" ) );
			continue;
		}
	}
}

__forceinline void add_weapon_config( const std::string& weapon_category ) {
	using namespace options;

	const std::string prefix = _( "aimbot." ) + weapon_category + _( "." );

	option::add_bool( prefix + _( "inherit_default" ), false );
	option::add_bool( prefix + _( "enabled" ) , false );
	option::add_bool( prefix + _( "visible_check" ) , false );
	option::add_bool( prefix + _( "downed_check" ) , false );
	option::add_bool( prefix + _( "autoshoot" ) , false );
	option::add_bool( prefix + _( "dynamic_fov" ) , false );
	option::add_float( prefix + _( "smoothing" ) , 0.75f );
	option::add_float( prefix + _( "fov" ) , 0.0f );
	option::add_list( prefix + _( "hitboxes" ) , 7 ); /* head, neck, chest, pelvis */
}

__forceinline void add_player_visual_config( const std::string& player_category ) {
	using namespace options;

	const std::string prefix = _( "visuals." ) + player_category + _( "." );

	option::add_list( prefix + _( "options" ) , 9 ); /* model, esp box, health bar, ammo bar, shield bar, value text, nametag, weapon name, reloading flag*/
	option::add_int( prefix + _( "model_type" ) , 3 ); /* chams, flat chams, glow outline */
	option::add_int( prefix + _( "health_bar_location" ), 2 ); /* left, right, bottom, top */
	option::add_int( prefix + _( "ammo_bar_location" ), 1 ); /* left, right, bottom, top */
	option::add_int( prefix + _( "shield_bar_location" ), 1 ); /* left, right, bottom, top */
	option::add_int( prefix + _( "value_text_location" ), 0 ); /* left, right, bottom, top */
	option::add_int( prefix + _( "nametag_location" ), 3 ); /* left, right, bottom, top */
	option::add_int( prefix + _( "weapon_name_location" ), 2 ); /* left, right, bottom, top */
	option::add_int ( prefix + _ ( "reloading_flag_location" ), 1 ); /* left, right, bottom, top */
	option::add_color( prefix + _( "model_color" ), { 0.81f, 0.96f, 1.0f, 0.12f } );
	option::add_color( prefix + _( "box_color" ) , { 1.0f, 0.99f, 0.99f, 0.30f } );
	option::add_color( prefix + _( "health_bar_color" ) , { 0.99f, 1.0f, 0.99f, 0.42f } );
	option::add_color( prefix + _( "ammo_bar_color" ) , { 0.53f, 0.61f, 1.0f, 0.35f } );
	option::add_color( prefix + _( "shield_bar_color" ) , { 0.53f, 0.61f, 1.0f, 0.35f } );
	option::add_color( prefix + _( "value_text_color" ) , { 0.99f, 1.0f, 0.99f, 0.72f } );
	option::add_color( prefix + _( "nametag_color" ) , { 0.99f, 1.0f, 0.99f, 0.72f } );
	option::add_color( prefix + _( "weapon_name_color" ) , { 1.0f, 0.2f, 0.2f, 0.72f } );
	option::add_color( prefix + _( "reloading_flag_color" ) , { 0.5f, 1.0f, 0.3f, 0.72f } );
}

void options::init( ) {
	/* options should be structered in the following format: */
	/* aimbot */
	option::add_bool( _( "aimbot.enabled" ), 0 );
	option::add_int( _( "aimbot.key" ) , 0 );
	option::add_int( _( "aimbot.key_mode" ) , 0 );
	/* weapon configs */
	add_weapon_config( _( "default" ) );
	add_weapon_config( _( "pistol" ) );
	add_weapon_config( _( "shotgun" ) );
	add_weapon_config( _( "smg" ) );
	add_weapon_config( _( "rifle" ) );
	add_weapon_config( _( "sniper" ) );
	add_weapon_config( _( "lmg" ) );
	add_weapon_config( _( "special" ) );

	/* VISUALS */
	/* global visuals */
	option::add_list( _( "visuals.filters" ), 3 ); /* teammates, enemies, weapons */
	/* player visuals configs */
	add_player_visual_config( _( "teammates" ) );
	add_player_visual_config( _( "enemies" ) );
	add_player_visual_config( _( "weapons" ) );

	/* other visuals */
	option::add_list( _( "visuals.other.removals" ), 1 ); /* idk */
	option::add_float( _( "visuals.other.fov" ), 90.0f );
	option::add_float( _( "visuals.other.viewmodel_fov" ), 68.0f );
	option::add_float ( _ ( "visuals.other.viewmodel_offset_x" ), 1.0f );
	option::add_float ( _ ( "visuals.other.viewmodel_offset_y" ), 1.0f );
	option::add_float ( _ ( "visuals.other.viewmodel_offset_z" ), -1.0f );
	option::add_float( _( "visuals.other.aspect_ratio" ), 1.0f );
	option::add_bool( _( "visuals.other.offscreen_esp" ), false );
	option::add_float( _( "visuals.other.offscreen_esp_distance" ), 0.0f );
	option::add_float( _( "visuals.other.offscreen_esp_size" ), 0.0f );
	option::add_color( _( "visuals.other.offscreen_esp_color" ), { 0.69f, 1.0f, 0.92f, 0.36f } );
	option::add_bool( _( "visuals.other.watermark" ), false );
	option::add_bool( _( "visuals.other.keybind_list" ), false );

	/* SKINS */


	/* MISC */
	option::add_bool( _( "misc.movement.bhop" ), false );
	option::add_bool ( _ ( "misc.movement.accurate_move" ), false );
	option::add_bool( _( "misc.effects.third_person" ), false );
	option::add_float( _( "misc.effects.third_person_range" ), 0.0f );
	option::add_int( _( "misc.effects.third_person_key" ), 0 );
	option::add_int( _( "misc.effects.third_person_key_mode" ) , 0 );

	option::add_float( _( "gui.dpi" ), 1.0f );
}