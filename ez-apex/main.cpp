#include "driver_interface.hpp"

#include "features/aim.hpp"
#include "features/glow.hpp"
#include "features/position.hpp"

#include "overlay/overlay.hpp"

#include "menu/menu.hpp"
#include "menu/options.hpp"

#include <iostream>

#pragma comment(lib, "winmm.lib")

void entry( HMODULE hmodule ) {
	//AllocConsole( );
	//
	//FILE* fDummy;
	//freopen_s( &fDummy , "CONIN$" , "r" , stdin );
	//freopen_s( &fDummy , "CONOUT$" , "w" , stderr );
	//freopen_s( &fDummy , "CONOUT$" , "w" , stdout );

	if ( !drv::is_loaded( ) ) {
		MessageBoxA( nullptr, _("Driver is not loaded."), _("Error"), MB_OK );
		//std::cin.get( );
		goto unload;
	}

	//std::cout << "Driver loaded. Initializing hex." << std::endl;

	drv::window = nullptr;

	while ( !( drv::window = FindWindowA( _("Respawn001") , _("Apex Legends") ) ) )
		std::this_thread::sleep_for( 1s );

	//std::this_thread::sleep_for( 10s );

	DWORD pid;
	GetWindowThreadProcessId( drv::window , &pid );

	drv::set_target( pid );

	while ( !drv::get_base( ) )
		std::this_thread::sleep_for( 1s );

	if ( !apex::init( ) ) {
		MessageBoxA( nullptr , _("Startup failed.") , _("Error") , MB_OK );
		//std::cin.get( );
		goto unload;
	}

	std::this_thread::sleep_for( 1s );

	options::init( );
	menu::load_cfg_list( );

	// features
	std::thread( features::aim::run ).detach( );
	std::thread( features::glow::run ).detach( );
	std::thread( features::position::run ).detach( );

	overlay::enable( );

	while ( !GetAsyncKeyState( VK_END ) ) {
		std::this_thread::sleep_for( 10ms );

		/* run cheat here */
		apex::enable_input( !menu::opened );
	}

	overlay::disable( );

	Beep( 500 , 500 );

	//FreeConsole( );
unload:
	FreeLibraryAndExitThread( hmodule , 0 );
}

BOOL WINAPI DllMain( HINSTANCE hinstDLL , DWORD fdwReason , LPVOID lpReserved ) {
	DisableThreadLibraryCalls( hinstDLL );

	if ( fdwReason == DLL_PROCESS_ATTACH ) {
		if ( const auto thread = CreateThread( nullptr , 0 , reinterpret_cast< LPTHREAD_START_ROUTINE >( entry ) , nullptr , 0 , nullptr ) )
			CloseHandle( thread );
	}

	return TRUE;
}