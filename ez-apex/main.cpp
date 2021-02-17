#include "driver_interface.hpp"

#include "features/aim.hpp"
#include "features/glow.hpp"
#include "features/position.hpp"

#include <iostream>

int main ( ) {
	if ( !drv::is_loaded ( ) ) {
		std::cout << "Driver is not loaded." << std::endl;
		std::cin.get ( );
		return 1;
	}

	std::cout << "Driver loaded. Initializing hex." << std::endl;

	drv::window = nullptr;

	while ( !( drv::window = FindWindowA ( "Respawn001", "Apex Legends" ) ) )
		std::this_thread::sleep_for ( 1s );

	DWORD pid;
	GetWindowThreadProcessId ( drv::window, &pid );

	drv::set_target ( pid );

	while ( !drv::get_base ( ) )
		std::this_thread::sleep_for ( 1s );

	if ( !apex::init ( ) ) {
		std::cout << "Startup failed." << std::endl;
		std::cin.get ( );
		return 1;
	}

	std::this_thread::sleep_for ( 1s );

	// features
	std::thread ( features::aim::run ).detach ( );
	std::thread ( features::glow::run ).detach ( );
	std::thread ( features::position::run ).detach ( );

	std::cout << "Initialized." << std::endl;

	std::cin.get ( );

	return 0;
}