#include "input.hpp"

#include <map>
#include <mutex>
#include <thread>
#include <algorithm>
#include <array>
#include <Windows.h>

#include "../imgui/imgui_impl_dx11.h"
#include "../imgui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam );

namespace input {
	bool run_message_thread = true;
	
	WNDPROC backup_wndproc = nullptr;

	LRESULT __stdcall wndproc_hk( HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam ) {
		//ImGui_ImplWin32_WndProcHandler( hWnd , uMsg, wParam, lParam);

		return true;
	}

	std::array<bool, 256> last_key_states { false };
	std::array<bool , 256> key_states { false };

	void handle_input( ) {
		if ( ImGui::GetCurrentContext( ) == NULL )
			return;

		ImGuiIO& io = ImGui::GetIO( );

		for ( auto i = 0; i < 256; i++)
			key_states[ i ] = GetAsyncKeyState( i);

		memcpy( io.KeysDown + 5, key_states.data( ) + 5, (256 - 5)* sizeof( bool ) );

		io.MouseDown[ 0 ] = key_states[ VK_LBUTTON ];
		io.MouseDown[ 1 ] = key_states[ VK_RBUTTON ];
		io.MouseDown[ 2 ] = key_states[ VK_MBUTTON ];
		io.MouseDown[ 3 ] = key_states[ VK_XBUTTON1 ];
		io.MouseDown[ 4 ] = key_states[ VK_XBUTTON2 ];

		last_key_states = key_states;
	}

	void enable( HWND window , HWND game_window ) {
		backup_wndproc = reinterpret_cast< WNDPROC >( SetWindowLongPtr( window , GWLP_WNDPROC , reinterpret_cast< LONG_PTR >( wndproc_hk ) ) );

		std::thread( [ = ] {
			while ( run_message_thread ) {
				std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
				handle_input( );
				std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

				/* send mouse move message to force overlay to redraw */
				CallWindowProc( backup_wndproc , window , 0x200 , 0 , 0 );

				if ( game_window ) {
					//WINDOWINFO info;
					//GetWindowInfo( game_window , &info );

					if ( game_window == GetForegroundWindow( ) ) {
						SetWindowLongPtr( window , GWL_STYLE , WS_VISIBLE );
						SetWindowLongPtr( window , GWL_EXSTYLE , WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED |WS_EX_TOOLWINDOW);
						
						SetWindowPos( window , HWND_TOPMOST , 0 , 0 , 0 , 0 , SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE );
						ShowWindow( window, SW_SHOW );
					}
				}

				std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
			}
		
			run_message_thread = true;
		} ).detach();
	}

	void disable( HWND window , HWND game_window ) {
		run_message_thread = false;
		
		/* wait for thread to finish */
		while ( !run_message_thread )
			std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

		/* restore wndproc */
		SetWindowLongPtr( window , GWLP_WNDPROC , reinterpret_cast< LONG_PTR >( backup_wndproc ) );
	}
}