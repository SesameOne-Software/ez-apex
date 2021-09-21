#include "options.hpp"
#include "security/security.hpp"

std::unordered_map< std::string, options::option > options::vars { };
std::unordered_map< std::string, options::option > options::script_vars { };

void options::option::add_bool ( const std::string& id, bool val ) {
	vars [ id ].type = option_type_t::boolean;
	vars [ id ].val.b = val;
}

void options::option::add_list ( const std::string& id, int count ) {
	vars [ id ].type = option_type_t::list;
	vars [ id ].list_size = count;
	memset ( vars [ id ].val.l, 0, sizeof ( vars [ id ].val.l ) );
}

void options::option::add_int ( const std::string& id, int val ) {
	vars [ id ].type = option_type_t::integer;
	vars [ id ].val.i = val;
}

void options::option::add_float ( const std::string& id, float val ) {
	vars [ id ].type = option_type_t::floating_point;
	vars [ id ].val.f = val;
}

void options::option::add_str ( const std::string& id, const char* val ) {
	vars [ id ].type = option_type_t::string;
	strcpy_s ( vars [ id ].val.s, val );
}

void options::option::add_color ( const std::string& id, const colorf& val ) {
	vars [ id ].type = option_type_t::color;
	vars [ id ].val.c = val;
}

std::vector< std::string > split ( const std::string& str, const std::string& delim ) {
	std::vector< std::string > tokens;
	size_t prev = 0, pos = 0;

	do {
		pos = str.find ( delim, prev );

		if ( pos == std::string::npos )
			pos = str.length ( );

		std::string token = str.substr ( prev, pos - prev );

		if ( !token.empty ( ) )
			tokens.push_back ( token );

		prev = pos + delim.length ( );
	} while ( pos < str.length ( ) && prev < str.length ( ) );

	return tokens;
}

void options::save ( const std::unordered_map< std::string, option >& options, const std::string& path ) {
	
}

void options::load ( std::unordered_map< std::string, option >& options, const std::string& path ) {
	
}

void options::init ( ) {
	VMP_BEGINULTRA ( );
	option::add_bool ( _ ( "aimbot.enable" ), false );
	option::add_bool ( _ ( "aimbot.predict" ), false );
	option::add_bool ( _ ( "aimbot.stable" ), false );
	option::add_bool ( _ ( "aimbot.vischeck" ), false );
	option::add_bool ( _ ( "aimbot.downed" ), false );
	option::add_int ( _ ( "aimbot.fov" ), 0 );
	option::add_int ( _ ( "aimbot.smooth" ), 0 );

	option::add_bool ( _ ( "visuals.enable" ), false );
	option::add_bool ( _ ( "visuals.chams" ), false );
	option::add_bool ( _ ( "visuals.xqz" ), false );
	option::add_bool ( _ ( "visuals.health_based" ), false );
	option::add_bool ( _ ( "visuals.target" ), false );
	option::add_color ( _ ( "visuals.chams_color" ), { 1.0f, 0.0f, 1.0f, 1.0f } );
	option::add_color ( _ ( "visuals.target_color" ), { 0.2f, 1.0f, 0.2f, 1.0f } );
	VMP_END ( );
}