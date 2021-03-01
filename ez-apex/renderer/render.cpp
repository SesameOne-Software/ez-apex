#include "render.hpp"

#include <unordered_map>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx11.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"

std::unordered_map<std::string, void*> font_list {};

std::unordered_map<std::string , void*>& render::get_font_list( ) {
	return font_list;
}

void render::create_font ( const uint8_t* data, size_t data_size, std::string_view family_name, float size, const uint16_t* ranges, void* font_config ) {
	ImGuiIO& io = ImGui::GetIO ( );

	font_list[ family_name.data ( ) ] = io.Fonts->AddFontFromMemoryTTF ( (void*) data, data_size, size, reinterpret_cast< ImFontConfig *>( font_config), ranges ? ranges : io.Fonts->GetGlyphRangesCyrillic() );
	reinterpret_cast<ImFont*>( font_list [ family_name.data ( ) ] )->SetFallbackChar ( '?' );
}

void render::screen_size ( float& width, float& height ) {
	const auto& io = ImGui::GetIO( );
	width = io.DisplaySize.x;
	height = io.DisplaySize.y;
}

void render::text_size ( std::string_view text, std::string_view font, vec3_t& dimentions ) {
	ImGui::PushFont ( reinterpret_cast<ImFont*>( font_list [ font.data() ] ) );
	const auto text_size = ImGui::CalcTextSize ( text.data ( ) );
	ImGui::PopFont ( );
	dimentions = { text_size.x, text_size.y };
}

void render::rect ( float x, float y, float width, float height, uint32_t color ) {
	ImGui::GetWindowDrawList ( )->AddRectFilled ( { round(x),round(y) }, { round(x + width), round(y + height)}, color );
}

void render::gradient ( float x, float y, float width, float height, uint32_t color1, uint32_t color2, bool is_horizontal ) {
	ImGui::GetWindowDrawList ( )->AddRectFilledMultiColor ( { round (x),round (y) }, { round(x + width), round (y + height) }, color1, is_horizontal ? color2 : color1, color2, is_horizontal ? color1 : color2 );
}

void render::outline ( float x, float y, float width, float height, uint32_t color ) {
	ImGui::GetWindowDrawList ( )->AddRect ( { round (x),round (y) }, { round (x + width), round (y + height) }, color );
}

void render::line ( float x, float y, float x2, float y2, uint32_t color, float thickness ) {
	ImGui::GetWindowDrawList ( )->AddLine ( { round (x),round (y) }, { round (x2),round (y2) }, color, thickness );
}

void render::text ( float x, float y, std::string_view text, std::string_view font, uint32_t color, bool outline ) {
	const auto draw_list = ImGui::GetWindowDrawList ( );

	ImGui::PushFont ( reinterpret_cast< ImFont* >( font_list [ font.data ( ) ] ) );

	if ( outline ) {
		draw_list->AddText ( { round(x - 1.0f), round(y - 1.0f) }, color & IM_COL32_A_MASK, text.data ( ) );
		draw_list->AddText ( { round(x - 1.0f), round(y + 1.0f) }, color & IM_COL32_A_MASK, text.data ( ) );
		draw_list->AddText ( { round(x + 1.0f), round(y + 1.0f) }, color & IM_COL32_A_MASK, text.data ( ) );
		draw_list->AddText ( { round(x + 1.0f), round(y - 1.0f) }, color & IM_COL32_A_MASK, text.data ( ) );
	}

	draw_list->AddText ( { round (x),round (y)}, color, text.data ( ) );

	ImGui::PopFont ( );
}

void render::circle ( float x, float y, float radius, int segments, uint32_t color, bool outline ) {
	if(!outline )
		ImGui::GetWindowDrawList ( )->AddCircleFilled ( { round (x),round (y) }, radius, color, segments );
	else
		ImGui::GetWindowDrawList ( )->AddCircle ( { round (x),(y) }, radius, color, segments, 2.5f );
}

void render::polygon ( const std::vector< vec3_t >& verticies, uint32_t color, bool outline, float thickness ) {
	std::vector< ImVec2 > points_2d {};

	for ( auto& point : verticies )
		points_2d.push_back ( { round (point.x), round (point.y)} );
		
	if ( outline )
		ImGui::GetWindowDrawList ( )->AddPolyline ( points_2d.data ( ), points_2d.size ( ), color, true, thickness );
	else
		ImGui::GetWindowDrawList ( )->AddConvexPolyFilled ( points_2d.data(), points_2d.size(), color );
}

void render::clip ( float x, float y, float width, float height, const std::function< void ( ) >& func ) {
	ImGui::PushClipRect ( { round (x), round (y) }, { round (x + width),round (y + height)},true );

	func ( );

	ImGui::PopClipRect ( );
}