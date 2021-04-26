#include "gui.hpp"

#include "security/security.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/custom.hpp"

#include "resources/sesame_ui.hpp"
#include "resources/fontawesome.hpp"

#include "options.hpp"

#include <chrono>
#include <shlwapi.h>
#include <shlobj_core.h>
#include <fstream>
#include <dwmapi.h>
#include <filesystem>
#include <future>

using namespace std::literals;
namespace fs = std::filesystem;

/* globals */
WNDCLASSEX g_wc{ };
IDirect3D9* g_d3d = nullptr;
D3DPRESENT_PARAMETERS g_d3d_pparams = {};

bool load_driver ( );
bool load_cheat ( );

__forceinline bool create_device(HWND hWnd) {
	if (!(g_d3d = Direct3DCreate9(D3D_SDK_VERSION)))
		return false;

	memset(&g_d3d_pparams, 0, sizeof(g_d3d_pparams));
	g_d3d_pparams.Windowed = TRUE;
	g_d3d_pparams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3d_pparams.hDeviceWindow = hWnd;
	g_d3d_pparams.MultiSampleQuality = D3DMULTISAMPLE_NONE;
	g_d3d_pparams.BackBufferFormat = D3DFMT_A8R8G8B8;
	g_d3d_pparams.EnableAutoDepthStencil = TRUE;
	g_d3d_pparams.AutoDepthStencilFormat = D3DFMT_D16;

	if (g_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3d_pparams, &g_d3d_device) < 0)
		return false;

	return true;
}

void cleanup_device() {
	if (g_d3d_device) {
		g_d3d_device->Release();
		g_d3d_device = nullptr;
	}

	if (g_d3d) {
		g_d3d->Release();
		g_d3d = nullptr;
	}
}

void reset_device() {
	ImGui_ImplDX9_InvalidateDeviceObjects( );

	if ( g_d3d_device->Reset ( &g_d3d_pparams ) == D3DERR_INVALIDCALL ) {
		MessageBoxA( nullptr, _ ( "Fatal error while attempting to reset device!" ), _ ( "Error" ), MB_OK | MB_ICONEXCLAMATION );
		exit( 1 );
	}

	ImGui_ImplDX9_CreateDeviceObjects( );
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam );

long __stdcall wnd_proc(HWND window, uint32_t msg, uintptr_t wparam, uintptr_t lparam) {
	static POINTS m {};
	static bool mdown = false;

	if ( ImGui_ImplWin32_WndProcHandler( window , msg , wparam , lparam ) )
		return true;

	switch (msg) {
	case WM_LBUTTONDOWN:
		SetCapture(g_window);
		m = MAKEPOINTS(lparam);
		mdown = true;
		return true;
	case WM_LBUTTONUP:
		ReleaseCapture ();
		mdown = false;
		return true;
	case WM_SIZE:
		if (g_d3d_device && wparam != SIZE_MINIMIZED) {
			g_d3d_pparams.BackBufferWidth = LOWORD(lparam);
			g_d3d_pparams.BackBufferHeight = HIWORD(lparam);
			reset_device();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wparam & 0xfff0) == SC_KEYMENU)
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_MOUSEMOVE:
		const auto mouse_x = short(lparam);
		const auto mouse_y = short(lparam >> 16);

		if (wparam == MK_LBUTTON) {
			POINTS p = MAKEPOINTS(lparam);
			RECT rect;
			GetWindowRect (g_window, &rect);

			rect.left += p.x - m.x;
			rect.top += p.y - m.y;

			if( !ImGui::IsPopupOpen ( nullptr, ImGuiPopupFlags_AnyPopupId ) )
				SetWindowPos(g_window, HWND_TOPMOST, rect.left, rect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);
		}

		return true;
	}

	return DefWindowProcA(window, msg, wparam, lparam);
}

void gui::create_window(int w, int h, const char* window_name) {
	g_wc = { sizeof(WNDCLASSEX), 0, WNDPROC(wnd_proc), 0L, 0L, GetModuleHandleA(nullptr), LoadIconA( nullptr, MAKEINTRESOURCEA ( 32512 ) ), nullptr, ( HBRUSH ) CreateSolidBrush( RGB ( 0, 0, 0 ) ), window_name, window_name, LoadIconA( nullptr, MAKEINTRESOURCEA ( 32512 ) ) };

	RegisterClassExA(&g_wc);
	g_window = CreateWindowExA( 0 , g_wc.lpszClassName, window_name, WS_POPUP, 100, 100, w, h, nullptr, nullptr, g_wc.hInstance, nullptr);

	if (!create_device(g_window)) {
		cleanup_device();
		UnregisterClassA(g_wc.lpszClassName, g_wc.hInstance);
		return;
	}

	ShowWindow(g_window, SW_SHOW );
	UpdateWindow(g_window);

	options::init ( );
}

int cur_tab_idx = 0;
bool open_popup_next_frame = false;
bool open_popup = false;
std::string popup_title;
std::string popup_text;

bool driver_loaded = false;
bool running_driver_loader = false;
bool running_waiting_for_game = false;
bool found_game = false;

int gui::render( ) {
	VM_TIGER_RED_START;
	MSG msg {};
	LARGE_INTEGER li { 0 };

	ImGui::CreateContext( );
	ImGuiIO& io = ImGui::GetIO( ); ( void ) io;
	ImGui::StyleColorsSesame( );

	ImGui_ImplWin32_Init( g_window );
	ImGui_ImplDX9_Init( g_d3d_device );

	io.MouseDrawCursor = false;

	static const ImWchar custom_font_ranges_all [ ] = { 0x20, 0xFFFF, 0 };

	auto font_cfg = ImFontConfig( );

	font_cfg.FontDataOwnedByAtlas = false;
	font_cfg.OversampleH = 2;
	font_cfg.PixelSnapH = false;

	font_cfg.RasterizerMultiply = 1.1f;
	const auto gui_ui_font = io.Fonts->AddFontFromMemoryCompressedTTF ( sesame_ui_compressed_data, sesame_ui_compressed_size, 13.5f , &font_cfg , io.Fonts->GetGlyphRangesCyrillic ( ) );
	gui_ui_font->SetFallbackChar( '?' );

	font_cfg.RasterizerMultiply = 1.2f;
	const auto gui_small_font = io.Fonts->AddFontFromMemoryCompressedTTF ( sesame_ui_compressed_data, sesame_ui_compressed_size, 12.0f , &font_cfg , io.Fonts->GetGlyphRangesCyrillic( ) );
	gui_small_font->SetFallbackChar( '?' );

	font_cfg.RasterizerMultiply = 1.0f;
	const auto gui_icons_font = io.Fonts->AddFontFromMemoryCompressedTTF( fontawesome_compressed_data, fontawesome_compressed_size, 18.0f , &font_cfg, custom_font_ranges_all );
	gui_icons_font->SetFallbackChar( '?' );

	ImGui::GetStyle( ).AntiAliasedFill = ImGui::GetStyle( ).AntiAliasedLines = true;
	VM_TIGER_RED_END;

	while ( msg.message != WM_QUIT && !g_close_gui ) {
		VM_TIGER_RED_START;
		if ( PeekMessageA( &msg , nullptr , 0 , 0 , PM_REMOVE ) ) {
			TranslateMessage( &msg );
			DispatchMessageA( &msg );
			continue;
		}

		ImGui_ImplDX9_NewFrame( );
		ImGui_ImplWin32_NewFrame( );
		ImGui::NewFrame( );

		ImGui::PushFont( gui_ui_font );

		bool open = true;
		VM_TIGER_RED_END;

		if ( ImGui::custom::Begin( _( "© 2019-2021 Sesame Software" ) , &open , gui_small_font ) ) {
			VM_TIGER_RED_START;
			if ( ImGui::custom::BeginTabs( &cur_tab_idx , gui_icons_font ) ) {
				/* aimbot, visuals, misc, config */
				if ( driver_loaded && found_game ) {
					ImGui::custom::AddTab ( _ ( "\uf54c" ) ); /* aimbot */
					ImGui::custom::AddTab ( _ ( "\uf1fb" ) ); /* visuals */
					ImGui::custom::AddTab ( _ ( "\uf11b" ) ); /* misc */
					ImGui::custom::AddTab ( _ ( "\uf0c7" ) ); /* config */
				}
				else {
					ImGui::custom::AddTab ( _ ( "\uf49e" ) ); /* load */
				}

				ImGui::custom::EndTabs( );
			}

			ImGui::SetNextWindowPos( ImVec2( ImGui::GetWindowPos( ).x + ImGui::GetWindowSize( ).x * 0.5f , ImGui::GetWindowPos( ).y + ImGui::GetWindowSize( ).y * 0.5f ) , ImGuiCond_Always , ImVec2( 0.5f , 0.5f ) );

			if ( ImGui::BeginPopupModal( ( popup_title + _( "##popup" ) ).c_str() , nullptr , ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings ) ) {
				ImGui::Text( popup_text.c_str() );

				if ( ImGui::Button( _("Ok") , ImVec2( -1.0f , 0.0f ) ) )
					ImGui::CloseCurrentPopup( );

				ImGui::EndPopup( );
			}

			VM_TIGER_RED_END;

			if ( !driver_loaded || !found_game ) {
				VM_TIGER_RED_START;
				if ( running_driver_loader || running_waiting_for_game ) {
					const auto r = 8.0f;
					ImGui::SetCursorPos ( ImVec2 ( ImGui::GetWindowContentRegionMax ( ).x * 0.5f - r, ImGui::GetWindowContentRegionMax ( ).y * 0.5f - r ) );
					ImGui::custom::Spinner ( _ ( "##Spinner" ), r, 3.0f, ImGui::ColorConvertFloat4ToU32 ( ImGui::GetStyleColorVec4 ( ImGuiCol_Button ) ) );

					ImGui::PushStyleColor ( ImGuiCol_Text, ImVec4 ( 1.0, 1.0f, 1.0f, ( sin ( ImGui::GetTime ( ) * IM_PI * 2.0f ) * 0.5f + 0.5f ) * 0.5f + 0.5f ) );
					const auto text_size = ImGui::CalcTextSize ( running_driver_loader ? _ ( "Loading Driver..." ) : ( g_searching_offsets ? _ ( "Searching for offsets..." ) : _ ( "Searching for game..." ) ) );
					ImGui::SetCursorPos ( ImVec2 ( ImGui::GetWindowContentRegionMax ( ).x * 0.5f - text_size.x * 0.5f, ImGui::GetWindowContentRegionMax ( ).y * 0.5f + r + 8.0f ) );
					ImGui::Text ( running_driver_loader ? _ ( "Loading Driver..." ) : ( g_searching_offsets ? _ ( "Searching for offsets..." ) : _ ( "Searching for game..." ) ) );
					ImGui::PopStyleColor ( );
				}
				else {
					if ( ImGui::Button ( _ ( "Load" ), ImVec2 ( -1.0f, 0.0f ) ) ) {
						std::thread ( [ ] ( ) {
							if ( !driver_loaded ) {
								running_driver_loader = true; {
									if ( !( driver_loaded = load_driver ( ) ) ) {
										if ( FindWindowA ( _ ( "Respawn001" ), _ ( "Apex Legends" ) )
											|| FindWindowA ( _ ( "EACLauncherWnd" ), nullptr )
											|| FindWindowA ( _ ( "EACLauncherChildWnd" ), nullptr ) ) {
											popup_text = _ ( "Please close the game." );
											popup_title = _ ( "Error" );
											open_popup = true;
										}
										else {
											popup_text = _ ( "Failed to load driver." );
											popup_title = _ ( "Error" );
											open_popup = true;
										}
									}
								}
								std::this_thread::sleep_for ( 1s );
								running_driver_loader = false;

								if ( open_popup )
									return;
							}

							if ( !found_game ) {
								running_waiting_for_game = true; {
									if ( !( found_game = load_cheat ( ) ) ) {
										popup_text = _ ( "Failed to find game or offsets." );
										popup_title = _ ( "Error" );
										open_popup = true;
									}
								}
								running_waiting_for_game = false;

								if ( open_popup )
									return;
							}
						} ).detach ( );
					}
				}
				VM_TIGER_RED_END;
			}
			else {
				ImGui::PushItemWidth ( -1.0f );
				switch ( cur_tab_idx ) {
				case 0: /* aimbot */ {
					VM_TIGER_RED_START;
					ImGui::Checkbox ( _ ( "Enable aimbot" ), &options::vars [ _ ( "aimbot.enable" ) ].val.b );

					if ( !options::vars [ _ ( "aimbot.enable" ) ].val.b ) {
						ImGui::PushItemFlag ( ImGuiItemFlags_Disabled, true );
						ImGui::PushStyleVar ( ImGuiStyleVar_Alpha, ImGui::GetStyle ( ).Alpha * 0.3f );
						ImGui::PushStyleColor ( ImGuiCol_Button, ImGui::ColorConvertFloat4ToU32 ( ImVec4 ( 0.5f, 0.5f, 0.5f, 1.0f ) ) );
						ImGui::PushStyleColor ( ImGuiCol_ButtonHovered, ImGui::ColorConvertFloat4ToU32 ( ImVec4 ( 0.5f, 0.5f, 0.5f, 1.0f ) ) );
						ImGui::PushStyleColor ( ImGuiCol_ButtonActive, ImGui::ColorConvertFloat4ToU32 ( ImVec4 ( 0.5f, 0.5f, 0.5f, 1.0f ) ) );
					}

					ImGui::Checkbox ( _ ( "Prediction" ), &options::vars [ _ ( "aimbot.predict" ) ].val.b );
					ImGui::Checkbox ( _ ( "Stabilize aim" ), &options::vars [ _ ( "aimbot.stable" ) ].val.b );
					ImGui::Checkbox ( _ ( "Check if visible" ), &options::vars [ _ ( "aimbot.vischeck" ) ].val.b );
					ImGui::Checkbox ( _ ( "Target if downed" ), &options::vars [ _ ( "aimbot.downed" ) ].val.b );
					ImGui::SliderInt ( _ ( "Aim FOV" ), &options::vars [ _ ( "aimbot.fov" ) ].val.i, 0, 25, _ ( "%d°" ) );
					ImGui::SliderInt ( _ ( "Smoothing Amount" ), &options::vars [ _ ( "aimbot.smooth" ) ].val.i, 0, 100, _ ( "%d%" ) );
					std::vector<const char*> tag_anims { _ ( "Head" ), _ ( "Neck" ), _ ( "Chest" ),_ ( "Stomach" ), _ ( "Pelvis" ) };
					ImGui::Combo ( _ ( "Hitbox" ), &options::vars [ _ ( "aimbot.hitbox" ) ].val.i, tag_anims.data(), tag_anims.size() );

					if ( !options::vars [ _ ( "aimbot.enable" ) ].val.b ) {
						ImGui::PopItemFlag ( );
						ImGui::PopStyleVar ( );
						ImGui::PopStyleColor ( );
						ImGui::PopStyleColor ( );
						ImGui::PopStyleColor ( );
					}
					VM_TIGER_RED_END;
				} break;
				case 1: /* visuals */ {
					VM_TIGER_RED_START;
					ImGui::Checkbox ( _ ( "Enable visuals" ), &options::vars [ _ ( "visuals.enable" ) ].val.b );
					if ( !options::vars [ _ ( "visuals.enable" ) ].val.b ) {
						ImGui::PushItemFlag ( ImGuiItemFlags_Disabled, true );
						ImGui::PushStyleVar ( ImGuiStyleVar_Alpha, ImGui::GetStyle ( ).Alpha * 0.3f );
						ImGui::PushStyleColor ( ImGuiCol_Button, ImGui::ColorConvertFloat4ToU32 ( ImVec4 ( 0.5f, 0.5f, 0.5f, 1.0f ) ) );
						ImGui::PushStyleColor ( ImGuiCol_ButtonHovered, ImGui::ColorConvertFloat4ToU32 ( ImVec4 ( 0.5f, 0.5f, 0.5f, 1.0f ) ) );
						ImGui::PushStyleColor ( ImGuiCol_ButtonActive, ImGui::ColorConvertFloat4ToU32 ( ImVec4 ( 0.5f, 0.5f, 0.5f, 1.0f ) ) );
					}
					ImGui::Checkbox ( _ ( "Chams" ), &options::vars [ _ ( "visuals.chams" ) ].val.b );
					ImGui::SameLine ( );
					ImGui::ColorEdit4 ( _ ( "##Chams Color" ), ( float* ) &options::vars [ _ ( "visuals.chams_color" ) ].val.c );
					ImGui::Checkbox ( _ ( "XQZ" ), &options::vars [ _ ( "visuals.xqz" ) ].val.b );
					ImGui::Checkbox ( _ ( "Health-based" ), &options::vars [ _ ( "visuals.health_based" ) ].val.b );
					ImGui::Checkbox ( _ ( "Show target" ), &options::vars [ _ ( "visuals.target" ) ].val.b );
					ImGui::SameLine ( );
					ImGui::ColorEdit4 ( _ ( "##Target Color" ), ( float* ) &options::vars [ _ ( "visuals.target_color" ) ].val.c );
					if ( !options::vars [ _ ( "visuals.enable" ) ].val.b ) {
						ImGui::PopItemFlag ( );
						ImGui::PopStyleVar ( );
						ImGui::PopStyleColor ( );
						ImGui::PopStyleColor ( );
						ImGui::PopStyleColor ( );
					}
					VM_TIGER_RED_END;
				} break;
				case 2: /* misc */ {
					VM_TIGER_RED_START;
					VM_TIGER_RED_END;
				} break;
				case 3: /* config */ {
					VM_TIGER_RED_START;
					if ( ImGui::Button ( _ ( "Save" ), ImVec2 ( -1.0f, 0.0f ) ) );
					if ( ImGui::Button ( _ ( "Load" ), ImVec2 ( -1.0f, 0.0f ) ) );
					VM_TIGER_RED_END;
				} break;
				}

				ImGui::PopItemWidth ( );
			}

			VM_TIGER_RED_START;
			if ( open_popup_next_frame ) {
				ImGui::OpenPopup( ( popup_title + _( "##Popup" ) ).c_str( ) );
				open_popup_next_frame = false;
			}

			if ( open_popup ) {
				open_popup_next_frame = true;
				open_popup = false;
			}

			ImGui::custom::End( );
			VM_TIGER_RED_END;
		}

		VM_TIGER_RED_START;
		ImGui::PopFont( );
		ImGui::EndFrame( );

		g_d3d_device->Clear( 0 , nullptr , D3DCLEAR_TARGET , D3DCOLOR_RGBA( 55 , 55 , 55 , 255 ) , 1.0f , 0 );

		if ( g_d3d_device->BeginScene( ) >= 0 ) {
			ImGui::Render( );
			ImGui_ImplDX9_RenderDrawData( ImGui::GetDrawData( ) );

			g_d3d_device->EndScene( );
		}

		const auto result = g_d3d_device->Present( nullptr , nullptr , nullptr , nullptr );

		if ( result == D3DERR_DEVICELOST && g_d3d_device->TestCooperativeLevel( ) == D3DERR_DEVICENOTRESET )
			reset_device( );
		VM_TIGER_RED_END;
	}

	VM_TIGER_RED_START;
	ImGui_ImplDX9_Shutdown( );
	ImGui_ImplWin32_Shutdown( );
	ImGui::DestroyContext( );

	cleanup_device( );

	DestroyWindow( g_window );
	UnregisterClassA( g_wc.lpszClassName , g_wc.hInstance );

	return 0;
	VM_TIGER_RED_END;
}