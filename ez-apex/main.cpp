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
	VMP_BEGINULTRA ( );

	if ( drv::is_loaded( ) )
		return true;

	if ( FindWindowA ( _ ( "Respawn001" ), _ ( "Apex Legends" ) )
		|| FindWindowA ( _ ( "EACLauncherWnd" ), nullptr )
		|| FindWindowA ( _ ( "EACLauncherChildWnd" ), nullptr ) )
		return false;

	//from https://github.com/ShoaShekelbergstein/kdmapper as some Drivers takes same device name
	if ( intel_driver::IsRunning( ) )
		return false;

	//FILE* fileptr;
	//char* buffer;
	//long filelen;
	//
	//fileptr = fopen ( "driver.sys", "rb" );  // Open the file in binary mode
	//fseek ( fileptr, 0, SEEK_END );          // Jump to the end of the file
	//filelen = ftell ( fileptr );             // Get the current byte offset in the file
	//rewind ( fileptr );                      // Jump back to the beginning of the file
	//
	//buffer = ( char* ) malloc ( filelen * sizeof ( char ) ); // Enough memory for the file
	//fread ( buffer, filelen, 1, fileptr ); // Read in the entire file
	//fclose ( fileptr ); // Close the file

	HANDLE iqvw64e_device_handle = intel_driver::Load( );

	if ( !iqvw64e_device_handle || iqvw64e_device_handle == INVALID_HANDLE_VALUE || !kdmapper::MapDriver ( iqvw64e_device_handle, /*( unsigned char* ) buffer*/driver_bytes ) ) {
		intel_driver::Unload( iqvw64e_device_handle );
		return false;
	}

	intel_driver::Unload( iqvw64e_device_handle );

	if ( !drv::clean_traces ( ) ) {
		g_clean_failed = true;

		drv::unload ( );

		return false;
	}

	if ( !drv::is_loaded( ) )
		return false;

	return true;
	VMP_END ( );
}

std::thread aim_thread;
std::thread glow_thread;
std::thread position_thread;

void scan_game ( ) {
	VMP_BEGINMUTATION ( );
	while ( FindWindowA ( _ ( "Respawn001" ), _ ( "Apex Legends" ) ) )
		std::this_thread::sleep_for ( 100ms );

	TerminateThread ( aim_thread.native_handle ( ), 0 );
	std::this_thread::sleep_for ( 100ms );
	TerminateThread ( glow_thread.native_handle ( ), 0 );
	std::this_thread::sleep_for ( 100ms );
	TerminateThread ( position_thread.native_handle ( ), 0 );
	std::this_thread::sleep_for ( 100ms );

	__debugbreak ( );
	VMP_END ( );
}

bool load_cheat ( ) {
	VMP_BEGINULTRA ( );
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
	VMP_END ( );
}

int __stdcall main ( HMODULE mod ) {
	VMP_BEGINULTRA ( );
	srand ( time ( nullptr ) );

	ShowWindow ( GetConsoleWindow ( ), SW_HIDE );

	gui::create_window ( 300, 216, _ ( "Sesame Apex" ) );

	VMP_END ( );
	VMP_BEGINULTRA ( );
	return gui::render ( );
	VMP_END ( );
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