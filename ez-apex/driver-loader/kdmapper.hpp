#pragma once
#include <Windows.h>
#include <stdint.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

#include "portable_executable.hpp"
#include "utils.hpp"
#include "nt.hpp"
#include "intel_driver.hpp"

namespace kdmapper
{
	__forceinline void RelocateImageByDelta ( portable_executable::vec_relocs relocs, const uint64_t delta )
	{
		for ( const auto& current_reloc : relocs )
		{
			for ( auto i = 0u; i < current_reloc.count; ++i )
			{
				const uint16_t type = current_reloc.item [ i ] >> 12;
				const uint16_t offset = current_reloc.item [ i ] & 0xFFF;

				if ( type == IMAGE_REL_BASED_DIR64 )
					*reinterpret_cast< uint64_t* >( current_reloc.address + offset ) += delta;
			}
		}
	}

	__forceinline bool ResolveImports ( HANDLE iqvw64e_device_handle, portable_executable::vec_imports imports )
	{
		for ( const auto& current_import : imports )
		{
			if ( !utils::GetKernelModuleAddress ( current_import.module_name ) )
			{
				return false;
			}

			for ( auto& current_function_data : current_import.function_datas )
			{
				const uint64_t function_address = intel_driver::GetKernelModuleExport ( iqvw64e_device_handle, utils::GetKernelModuleAddress ( current_import.module_name ), current_function_data.name );

				if ( !function_address )
				{
					return false;
				}

				*current_function_data.address = function_address;
			}
		}

		return true;
	}

	__forceinline uint64_t MapDriver ( HANDLE iqvw64e_device_handle, unsigned char* driver_bytes )
	{
		const PIMAGE_NT_HEADERS64 nt_headers = portable_executable::GetNtHeaders ( reinterpret_cast< void* >( driver_bytes ) );

		if ( !nt_headers )
		{
			return 0;
		}

		if ( nt_headers->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC )
		{
			return 0;
		}

		const uint32_t image_size = nt_headers->OptionalHeader.SizeOfImage;

		void* local_image_base = VirtualAlloc ( nullptr, image_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
		if ( !local_image_base )
			return 0;

		DWORD TotalVirtualHeaderSize = ( IMAGE_FIRST_SECTION ( nt_headers ) )->VirtualAddress;

		uint64_t kernel_image_base = intel_driver::AllocatePool ( iqvw64e_device_handle, nt::POOL_TYPE::NonPagedPool, image_size - TotalVirtualHeaderSize );

		do
		{
			if ( !kernel_image_base )
			{
				break;
			}

			// Copy image headers

			memcpy ( local_image_base, driver_bytes, nt_headers->OptionalHeader.SizeOfHeaders );

			// Copy image sections

			const PIMAGE_SECTION_HEADER current_image_section = IMAGE_FIRST_SECTION ( nt_headers );

			for ( auto i = 0; i < nt_headers->FileHeader.NumberOfSections; ++i )
			{
				auto local_section = reinterpret_cast< void* >( reinterpret_cast< uint64_t >( local_image_base ) + current_image_section [ i ].VirtualAddress );
				memcpy ( local_section, reinterpret_cast< void* >( reinterpret_cast< uint64_t >( driver_bytes ) + current_image_section [ i ].PointerToRawData ), current_image_section [ i ].SizeOfRawData );
			}

			uint64_t realBase = kernel_image_base;
			kernel_image_base -= TotalVirtualHeaderSize;

			// Resolve relocs and imports

			RelocateImageByDelta ( portable_executable::GetRelocs ( local_image_base ), kernel_image_base - nt_headers->OptionalHeader.ImageBase );

			if ( !ResolveImports ( iqvw64e_device_handle, portable_executable::GetImports ( local_image_base ) ) )
			{
				kernel_image_base = realBase;
				break;
			}

			// Write fixed image to kernel

			if ( !intel_driver::WriteMemory ( iqvw64e_device_handle, realBase, ( PVOID ) ( ( uintptr_t ) local_image_base + TotalVirtualHeaderSize ), image_size - TotalVirtualHeaderSize ) )
			{
				kernel_image_base = realBase;
				break;
			}

			// Call driver entry point

			const uint64_t address_of_entry_point = kernel_image_base + nt_headers->OptionalHeader.AddressOfEntryPoint;

			NTSTATUS status = 0;

			if ( !intel_driver::CallKernelFunction ( iqvw64e_device_handle, &status, address_of_entry_point, kernel_image_base, static_cast< uint64_t >( image_size - TotalVirtualHeaderSize ) ) )
			{
				kernel_image_base = realBase;
				break;
			}

			VirtualFree ( local_image_base, 0, MEM_RELEASE );

			/* if hook fails, abort */
			if ( status != 0 )
				break;

			return realBase;
		} while ( false );


		VirtualFree ( local_image_base, 0, MEM_RELEASE );

		uint8_t* zeroes = new uint8_t [ image_size - TotalVirtualHeaderSize ] { 0 };
		intel_driver::WriteMemory ( iqvw64e_device_handle, kernel_image_base, zeroes, image_size - TotalVirtualHeaderSize );
		delete [ ] zeroes;

		intel_driver::FreePool ( iqvw64e_device_handle, kernel_image_base );

		return 0;
	}
}