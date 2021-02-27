#include "kdmapper.hpp"

#include "driver_interface.hpp"

int main(const int argc, char** argv) {
	std::cout << "Started" << std::endl;

	srand( time ( nullptr ));

	//if ( drv::is_loaded ( ) ) {
	//	std::cout << "[-] Driver is already loaded." << std::endl;
	//	std::cin.get ( );
	//	return 1;
	//}

	//if (argc != 2 || std::filesystem::path(argv[1]).extension().string().compare(".sys")) {
	//	std::cout << "[-] Incorrect usage" << std::endl;
	//	std::cin.get ( );
	//	return 1;
	//}

	//from https://github.com/ShoaShekelbergstein/kdmapper as some Drivers takes same device name
	if (intel_driver::IsRunning()) {
		std::cout << "[-] \\Device\\Nal is already in use." << std::endl;
		std::cin.get ( );
		return 1;
	}

	//const std::string driver_path = argv[1];

	if (!std::filesystem::exists("driver.sys")) {
		std::cout << "[-] File doesn't exist" << std::endl;
		std::cin.get ( );
		return 1;
	}

	HANDLE iqvw64e_device_handle = intel_driver::Load();

	if (!iqvw64e_device_handle || iqvw64e_device_handle == INVALID_HANDLE_VALUE) {
		std::cout << "[-] Failed to load driver iqvw64e.sys" << std::endl;
		intel_driver::Unload(iqvw64e_device_handle);
		std::cin.get ( );
		return 1;
	}

	if (!intel_driver::ClearPiDDBCacheTable(iqvw64e_device_handle)) {
		std::cout << "[-] Failed to Clear PiDDBCacheTable" << std::endl;
		intel_driver::Unload(iqvw64e_device_handle);
		std::cin.get ( );
		return 1;
	}

	if (!intel_driver::ClearKernelHashBucketList(iqvw64e_device_handle)) {
		std::cout << "[-] Failed to Clear KernelHashBucketList" << std::endl;
		intel_driver::Unload(iqvw64e_device_handle);
		std::cin.get ( );
		return 1;
	}

	if (!intel_driver::ClearMmUnloadedDrivers(iqvw64e_device_handle)) {
		std::cout << "[!] Failed to Clear MmUnloadedDrivers" << std::endl;
		intel_driver::Unload(iqvw64e_device_handle);
		std::cin.get ( );
		return 1;
	}

	if (!kdmapper::MapDriver(iqvw64e_device_handle, "driver.sys")) {
		std::cout << "[-] Failed to map driver" << std::endl;
		intel_driver::Unload(iqvw64e_device_handle);
		std::cin.get ( );
		return 1;
	}

	intel_driver::Unload(iqvw64e_device_handle);

	if ( !drv::is_loaded ( ) ) {
		std::cout << "[-] Driver failed to load." << std::endl;
		std::cin.get ( );
		return 1;
	}

	std::cout << "[+] success" << std::endl;
	std::cin.get ( );

	return 0;
}