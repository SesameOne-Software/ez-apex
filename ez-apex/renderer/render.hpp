#pragma once
#include <cstdint>
#include <vector>
#include <string_view>
#include <string>
#include <functional>

template < typename type >
constexpr uint32_t rgba( type r , type g , type b , type a ) {
	return ( ( static_cast< uint32_t >( r ) & 0xFF ) << 0 ) | ( ( static_cast< uint32_t >( g ) & 0xFF ) << 8 ) | ( ( static_cast< uint32_t >( b ) & 0xFF ) << 16 ) | ( ( static_cast< uint32_t >( a ) & 0xFF ) << 24 );
}

struct vec3_t {
	float x , y , z;
};

namespace render {
	std::unordered_map<std::string , void*>& get_font_list( );

	void create_font ( const uint8_t* data, size_t data_size, std::string_view family_name, float size, const uint16_t* ranges = nullptr, void* font_config = nullptr );
	inline void create_font ( const std::vector<uint8_t>& data, std::string_view family_name, float size, const uint16_t* ranges = nullptr, void* font_config = nullptr ) {
		create_font ( data.data(), data.size(), family_name, size, ranges, font_config );
	}

	void screen_size ( float& width, float& height );
	void text_size ( std::string_view text, std::string_view font, vec3_t& dimentions );
	void rect ( float x, float y, float width, float height, uint32_t color );
	void gradient ( float x, float y, float width, float height, uint32_t color1, uint32_t color2, bool is_horizontal );
	void outline ( float x, float y, float width, float height, uint32_t color );
	void line ( float x, float y, float x2, float y2, uint32_t color, float thickness = 1.0f );
	void text ( float x, float y, std::string_view text, std::string_view font, uint32_t color, bool outline = false );
	void circle ( float x, float y, float radius, int segments, uint32_t color, bool outline = false );
	void polygon ( const std::vector< vec3_t >& verticies, uint32_t color, bool outline = false, float thickness = 1.0f );
	void clip ( float x, float y, float width, float height, const std::function< void ( ) >& func );
}