#include "driver_interface.hpp"
#include "driver-loader/kdmapper.hpp"

#include "features/aim.hpp"
#include "features/glow.hpp"
#include "features/position.hpp"

#include "overlay/overlay.hpp"

#include "menu/menu.hpp"
#include "menu/options.hpp"

#include "driver_bytes.h"

#include <iostream>

#pragma comment(lib, "winmm.lib")

bool load_driver( ) {
	srand( time( nullptr ) );

	if ( drv::is_loaded( ) ) {
		std::cout << _( "[-] Driver is already loaded." ) << std::endl;
		return true;
	}

	//from https://github.com/ShoaShekelbergstein/kdmapper as some Drivers takes same device name
	if ( intel_driver::IsRunning( ) ) {
		std::cout << _( "[-] \\Device\\Nal is already in use." ) << std::endl;
		return false;
	}

	HANDLE iqvw64e_device_handle = intel_driver::Load( );

	if ( !iqvw64e_device_handle || iqvw64e_device_handle == INVALID_HANDLE_VALUE ) {
		std::cout << _( "[-] Failed to load driver iqvw64e.sys" ) << std::endl;
		intel_driver::Unload( iqvw64e_device_handle );
		return false;
	}

	if ( !intel_driver::ClearPiDDBCacheTable( iqvw64e_device_handle ) ) {
		std::cout << _( "[-] Failed to Clear PiDDBCacheTable" ) << std::endl;
		intel_driver::Unload( iqvw64e_device_handle );
		return false;
	}

	if ( !intel_driver::ClearKernelHashBucketList( iqvw64e_device_handle ) ) {
		std::cout << _( "[-] Failed to Clear KernelHashBucketList" ) << std::endl;
		intel_driver::Unload( iqvw64e_device_handle );
		return false;
	}

	if ( !intel_driver::ClearMmUnloadedDrivers( iqvw64e_device_handle ) ) {
		std::cout << _( "[!] Failed to Clear MmUnloadedDrivers" ) << std::endl;
		intel_driver::Unload( iqvw64e_device_handle );
		return false;
	}

	if ( !kdmapper::MapDriver( iqvw64e_device_handle , driver_bytes ) ) {
		std::cout << _( "[-] Failed to map driver" ) << std::endl;
		intel_driver::Unload( iqvw64e_device_handle );
		return false;
	}

	intel_driver::Unload( iqvw64e_device_handle );

	if ( !drv::is_loaded( ) ) {
		std::cout << _( "[-] Driver failed to load." ) << std::endl;
		return false;
	}

	std::cout << _( "[+] success" ) << std::endl;

	return true;
}

#ifdef DLL_MODE
void entry( HMODULE hmodule ) {
	//AllocConsole( );
	//
	//FILE* fDummy;
	//freopen_s( &fDummy , "CONIN$" , "r" , stdin );
	//freopen_s( &fDummy , "CONOUT$" , "w" , stderr );
	//freopen_s( &fDummy , "CONOUT$" , "w" , stdout );

	if ( !load_driver( ) ) {
		MessageBoxA( nullptr , _( "Driver failed to load." ) , _( "Error" ) , MB_OK );
		return 1;
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
#endif

int main( ) {
	if ( !load_driver() ) {
		MessageBoxA( nullptr , _( "Driver failed to load." ) , _( "Error" ) , MB_OK );
		return 1;
	}

	Beep( 500 , 500 );

	std::cout << _("Driver loaded. Initializing hex.") << std::endl;

	drv::window = nullptr;

	while ( !( drv::window = FindWindowA( _( "Respawn001" ) , _( "Apex Legends" ) ) ) )
		std::this_thread::sleep_for( 1s );

	//std::this_thread::sleep_for( 10s );

	DWORD pid;
	GetWindowThreadProcessId( drv::window , &pid );

	drv::set_target( pid );

	while ( !drv::get_base( ) )
		std::this_thread::sleep_for( 1s );

	if ( !apex::init( ) ) {
		MessageBoxA( nullptr , _( "Startup failed." ) , _( "Error" ) , MB_OK );
		return 1;
	}

	std::this_thread::sleep_for( 1s );

	// features
	std::thread( features::aim::run ).detach( );
	std::thread( features::glow::run ).detach( );
	std::thread( features::position::run ).detach( );

	while ( !GetAsyncKeyState( VK_END ) ) {
		std::this_thread::sleep_for( 100ms );
	}

	Beep( 500 , 500 );

	return 0;
}