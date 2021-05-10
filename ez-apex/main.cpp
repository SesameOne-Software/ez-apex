#include "driver_interface.hpp"
#include "driver-loader/kdmapper.hpp"

#include "features/aim.hpp"
#include "features/glow.hpp"
#include "features/position.hpp"

#include "driver_bytes.h"
#include "gui.hpp"

#include "PH/PH_API/PH_API.hpp"

#include <iostream>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d3d9.lib")

bool load_driver( ) {
	VM_TIGER_WHITE_START;

	if ( drv::is_loaded( ) )
		return true;

	if ( FindWindowA ( _ ( "Respawn001" ), _ ( "Apex Legends" ) )
		|| FindWindowA ( _ ( "EACLauncherWnd" ), nullptr )
		|| FindWindowA ( _ ( "EACLauncherChildWnd" ), nullptr ) )
		return false;

	//from https://github.com/ShoaShekelbergstein/kdmapper as some Drivers takes same device name
	if ( intel_driver::IsRunning( ) )
		return false;

	HANDLE iqvw64e_device_handle = intel_driver::Load( );

	if ( !iqvw64e_device_handle || iqvw64e_device_handle == INVALID_HANDLE_VALUE ) {
		intel_driver::Unload( iqvw64e_device_handle );
		return false;
	}

	if ( !intel_driver::ClearPiDDBCacheTable( iqvw64e_device_handle ) ) {
		intel_driver::Unload( iqvw64e_device_handle );
		return false;
	}

	if ( !intel_driver::ClearKernelHashBucketList( iqvw64e_device_handle ) ) {
		intel_driver::Unload( iqvw64e_device_handle );
		return false;
	}

	if ( !kdmapper::MapDriver( iqvw64e_device_handle , driver_bytes ) ) {
		intel_driver::Unload( iqvw64e_device_handle );
		return false;
	}

	if ( !intel_driver::ClearMmUnloadedDrivers ( iqvw64e_device_handle ) ) {
		intel_driver::Unload ( iqvw64e_device_handle );
		return false;
	}

	intel_driver::Unload( iqvw64e_device_handle );

	if ( !drv::is_loaded( ) )
		return false;

	return true;
	VM_TIGER_WHITE_END;
}

std::thread aim_thread;
std::thread glow_thread;
std::thread position_thread;

void scan_game ( ) {
	while ( FindWindowA ( _ ( "Respawn001" ), _ ( "Apex Legends" ) ) )
		std::this_thread::sleep_for ( 100ms );

	TerminateThread ( aim_thread.native_handle ( ), 0 );
	std::this_thread::sleep_for ( 100ms );
	TerminateThread ( glow_thread.native_handle ( ), 0 );
	std::this_thread::sleep_for ( 100ms );
	TerminateThread ( position_thread.native_handle ( ), 0 );
	std::this_thread::sleep_for ( 100ms );

	exit ( 0 );
}

bool load_cheat ( ) {
	VM_TIGER_WHITE_START;
	drv::window = nullptr;

	while ( !( drv::window = FindWindowA ( _ ( "Respawn001" ), _ ( "Apex Legends" ) ) ) )
		std::this_thread::sleep_for ( 1s );

	DWORD pid = 0;
	GetWindowThreadProcessId ( drv::window, &pid );

	if ( !drv::window || !pid )
		return false;

	drv::set_target ( pid );

	while ( !drv::get_base ( ) )
		std::this_thread::sleep_for ( 1s );

	//std::this_thread::sleep_for ( 10s );

	g_searching_offsets = true;

	if ( !apex::init ( ) ) {
		g_searching_offsets = false;
		return false;
	}

	std::this_thread::sleep_for ( 1s );

	/* features */
	aim_thread = std::thread ( features::aim::run );
	glow_thread = std::thread ( features::glow::run );
	position_thread = std::thread ( features::position::run );

	aim_thread.detach ( );
	glow_thread.detach ( );
	position_thread.detach ( );

	/* wait for game to close and exit threads + process */
	std::thread ( scan_game ).detach ( );

	g_searching_offsets = false;

	return true;
	VM_TIGER_WHITE_END;
}

int __stdcall main ( HMODULE mod ) {
	VM_SHARK_BLACK_START;
	srand ( time ( nullptr ) );

	//ShowWindow ( GetConsoleWindow ( ), SW_HIDE );

	gui::create_window ( 300, 216, _ ( "Sesame Apex" ) );

	return gui::render ( );
	VM_SHARK_BLACK_END;
}

#ifdef DLL_MODE
BOOL WINAPI DllMain( HINSTANCE hinstDLL , DWORD fdwReason , LPVOID lpReserved ) {
	DisableThreadLibraryCalls( hinstDLL );

	if ( fdwReason == DLL_PROCESS_ATTACH ) {
		//auto info = reinterpret_cast< ph_heartbeat::heartbeat_info* >( hinstDLL );
		//ph_heartbeat::initialize_heartbeat ( info );
		//
		//if ( const auto thread = CreateThread( nullptr , 0 , reinterpret_cast< LPTHREAD_START_ROUTINE >( main ) , ( HMODULE ) info->image_base, 0 , nullptr ) )
		//	CloseHandle( thread );

		if ( const auto thread = CreateThread ( nullptr, 0, reinterpret_cast< LPTHREAD_START_ROUTINE >( main ), ( HMODULE ) hinstDLL, 0, nullptr ) )
			CloseHandle ( thread );
	}

	return TRUE;
}
#endif