#include "req.hpp"

constexpr u32 magic_number = 0x1337;
constexpr u64 sesame_success = 0x5E5A000000000001;
constexpr u64 sesame_error = 0x5E5A000000000000;
constexpr u64 sesame_query = 0x5E5A000000000002;

bool safe_memcpy ( void* dest, void* src, size_t size ) {
	size_t returnSize = 0;
	return NT_SUCCESS ( fn::MmCopyVirtualMemory ( PsGetCurrentProcess ( ), src, PsGetCurrentProcess ( ), dest, size, KernelMode, &returnSize ) ) && returnSize == size;
}

bool dispatch_request( req::request_t& req ) {
	uintptr_t res = 0;
	
	PEPROCESS from_proc = nullptr;
	PEPROCESS to_proc = nullptr;

	auto deref_process = [ & ] ( ) {
		if ( from_proc ) {
			fn::ObfDereferenceObject ( from_proc );
			from_proc = nullptr;
		}

		if ( to_proc ) {
			fn::ObfDereferenceObject ( to_proc );
			to_proc = nullptr;
		}
	};

	switch ( req.type ) {
	case req::request_type_t::copy: {
		if ( !( from_proc = util::get_process( req.pid_from ) ) ) {
			log( _("Failed to find source process.\n" ));
			deref_process ( );
			return false;
		}

		if ( !( to_proc = util::get_process ( req.pid_to ) ) ) {
			log( _("Failed to find destination process.\n" ));
			deref_process ( );
			return false;
		}

		auto ret = fn::MmCopyVirtualMemory( from_proc, ( void* )req.addr_from, to_proc, ( void* )req.addr_to, req.sz, KernelMode, &res );

		if ( !NT_SUCCESS( ret ) ) {
			log( _("Failed to write to memory.\n" ));
			deref_process ( );
			return false;
		}
	} break;
	case req::request_type_t::copy_protected: {
		uintptr_t res = 0;
		PEPROCESS from_proc = nullptr;
		PEPROCESS to_proc = nullptr;

		if ( !( from_proc = util::get_process ( req.pid_from ) ) ) {
			log ( _ ( "Failed to find source process.\n" ) );
			deref_process ( );
			return false;
		}

		if ( !( to_proc = util::get_process ( req.pid_to ) ) ) {
			log ( _ ( "Failed to find destination process.\n" ) );
			deref_process ( );
			return false;
		}

		auto mdl = fn::IoAllocateMdl( ( void* )req.addr_to, ( u32 )req.sz, false, false, nullptr );

		if ( !mdl ) {
			log(_( "Failed to allocate MDL.\n" ));
			deref_process ( );
			return false;
		}

		__try {
			fn::MmProbeAndLockProcessPages( mdl, to_proc, KernelMode, IoReadAccess );

			auto mapping = fn::MmMapLockedPagesSpecifyCache( mdl, KernelMode, MmNonCached, nullptr, 0, NormalPagePriority );

			fn::MmProtectMdlSystemAddress( mdl, PAGE_READWRITE );

			safe_memcpy ( mapping, &req.addr_from, req.sz );

			fn::MmUnmapLockedPages( mapping, mdl );
			fn::MmUnlockPages( mdl );
			fn::IoFreeMdl( mdl );
		}
		__except ( EXCEPTION_EXECUTE_HANDLER ) {
			fn::IoFreeMdl( mdl );
		}
	} break;
	case req::request_type_t::get_base: {
		uintptr_t res1 = 0;
		PEPROCESS from_proc = nullptr;
		PEPROCESS to_proc = nullptr;

		if ( !( from_proc = util::get_process ( req.pid_from ) ) ) {
			log ( _ ( "Failed to find source process.\n" ) );
			deref_process ( );
			return false;
		}

		if ( !( to_proc = util::get_process ( req.pid_to ) ) ) {
			log ( _ ( "Failed to find destination process.\n" ) );
			deref_process ( );
			return false;
		}

		fn::types::KAPC_STATE apc;
		fn::KeStackAttachProcess( ( PRKPROCESS )to_proc, &apc );

		auto base = fn::PsGetProcessSectionBaseAddress( from_proc );

		fn::KeUnstackDetachProcess( &apc );

		auto res = fn::MmCopyVirtualMemory( PsGetCurrentProcess( ), ( void* )&base, to_proc, ( void* )req.addr_to, sizeof( void* ), KernelMode, &res1 );

		if ( !NT_SUCCESS( res ) ) {
			log(_( "Failed to get process base address.\n") );
			deref_process ( );
			return false;
		}
	} break;
	case req::request_type_t::clean: {
		/* secure driver from anticheat scans */
		util::sec::clear_mmunloadeddrivers( );
		util::sec::clear_piddbcache( );
		//util::sec::hide_thread( );

		log( _("Cleaned traces.\n") );
	} break;
	case req::request_type_t::spoof: {
		//spoofer::spoof( );

		log( _("Spoofed HWIDs.\n") );
	} break;
	default: {
		return false;
	}break;
	}

	deref_process ( );

	return true;
}

bool probe_user_address ( void* addr, size_t size, u32 alignment ) {
	if ( !size )
		return true;

	auto current = ( uintptr_t ) addr;

	if ( ( ( uintptr_t ) addr & ( alignment - 1 ) ) != 0 )
		return false;

	auto last = current + size - 1;

	if ( ( last < current ) || ( last >= MmUserProbeAddress ) )
		return false;

	return true;
}

#pragma pack(push, 1)
struct hook_args_t {
	u32 magic;
	req::request_t* args;
};
#pragma pack(pop)

void callback ( hook_args_t* data, u64* status ) {
	log ( _ ( "Hook called.\n" ) );

	if ( !data )
		return;

	hook_args_t safe_data { };

	if ( !probe_user_address ( data, sizeof ( safe_data ), sizeof ( u32 ) ) || !safe_memcpy ( &safe_data, data, sizeof ( safe_data ) ) || safe_data.magic != magic_number )
		return;

	req::request_t safe_request;

	if ( !safe_memcpy ( &safe_request, safe_data.args, sizeof ( safe_request ) ) ) {
		*status = sesame_error;
		return;
	}

	if ( safe_request.type == req::request_type_t::query ) {
		*status = sesame_success;
		return;
	}

	if ( dispatch_request ( safe_request ) )
		/* success code */
		*status = sesame_success;
	else
		/* error code */
		*status = sesame_error;
}

extern "C" {
	PVOID
		NTAPI
		RtlFindExportedRoutineByName (
			_In_ PVOID ImageBase,
			_In_ PCCH RoutineNam
		);
}

bool install_hooks ( ) {
	u64 dxgkrnl_size = 0;
	u64 dxgkrnl_base = util::get_kerneladdr ( _("dxgkrnl.sys"), dxgkrnl_size );

	if ( !dxgkrnl_size || !dxgkrnl_base ) {
		log ( _ ( "Failed to locate dxgkrnl.sys.\n" ) );
		return false;
	}

	const auto DxgkGetProcessDeviceRemovalSupport = RtlFindExportedRoutineByName ( reinterpret_cast<void*>( dxgkrnl_base ), _("DxgkGetProcessDeviceRemovalSupport") );

	log ( _ ( "DxgkGetProcessDeviceRemovalSupport @ 0x%llX.\n" ), DxgkGetProcessDeviceRemovalSupport );

	u8 shellcode [ ] { 0x48, 0xB8, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0xFF, 0xE0, 0xC3 };
	*reinterpret_cast<u64*>( shellcode + 2 ) = reinterpret_cast< u64 >( callback );

	void* mapped = MmMapIoSpaceEx ( MmGetPhysicalAddress ( DxgkGetProcessDeviceRemovalSupport ), 0x1000, PAGE_READWRITE );
	memcpy ( mapped, shellcode, sizeof( shellcode ) );
	MmUnmapIoSpace ( mapped, 0x1000 );

	log ( _ ( "Hooked DxgkGetProcessDeviceRemovalSupport.\n" ) );

	return true;
}

u32 main( ) {
	fn::init( );

	install_hooks ( );
	
	return STATUS_SUCCESS;
}