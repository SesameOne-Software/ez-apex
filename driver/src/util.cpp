#include "util.hpp"

struct PiDDBCacheEntry {
	LIST_ENTRY List;
	UNICODE_STRING DriverName;
	u32 TimeDateStamp;
	NTSTATUS LoadStatus;
	u8 pad [ 16 ];
};

typedef struct {
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR FullPathName [ 256 ];
} SYSTEM_MODULE, * PSYSTEM_MODULE;

typedef struct {
	u32 ModulesCount;
	SYSTEM_MODULE Modules [ 1 ];
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;

typedef struct _UNLOADED_DRIVERS {
	UNICODE_STRING Name;
	PVOID StartAddress;
	PVOID EndAddress;
	LARGE_INTEGER CurrentTime;
} UNLOADED_DRIVERS, * PUNLOADED_DRIVERS;

struct MYTHREAD {
	u8 p [ 0x28 ];
	u64 InitialStack;
	u64 volatile StackLimit;
	u64 StackBase;
	u64 ThreadLock;
	volatile u64 CycleTime;
	u32 CurrentRunTime;
	u32 ExpectedRunTime;
	u64 KernelStack;
	u8 p2 [ 0x648 - sizeof ( u8 ) - 0x58 ];
	CLIENT_ID Cid;
	u8 p3 [ 0x6a0 - sizeof ( CLIENT_ID ) - 0x648 ];
	PVOID Win32StartAddress;
	u8 p4 [ 0x6b8 - sizeof ( u8 ) - 0x6a0 ];
	LIST_ENTRY ThreadListEntry;
	u8 p5 [ 0x810 - sizeof ( LIST_ENTRY ) - 0x6b8 ];
};

using PMYTHREAD = MYTHREAD*;

u64 util::get_kerneladdr ( const char* name, u64& size ) {
	u32 cb_needed = 0;

	auto status = fn::ZwQuerySystemInformation (
		fn::types::SYSTEM_INFORMATION_CLASS( 11 ),
		nullptr,
		0,
		&cb_needed
	);

	if ( STATUS_INFO_LENGTH_MISMATCH != status ) {
		log (_( "! ZwQuerySystemInformation for size failed: 0x%llX !\n"), status );
		return 0;
	}

	auto modules = ( PSYSTEM_MODULE_INFORMATION ) fn::ExAllocatePoolWithTag ( NonPagedPool, cb_needed, 'ases' );

	if ( !valid_ptr(modules) ) {
		log ( _("! failed to allocate %u bytes for modules !\n"), size );
		return 0;
	}

	if ( !NT_SUCCESS ( status = fn::ZwQuerySystemInformation ( fn::types::SYSTEM_INFORMATION_CLASS ( 11 ), modules, cb_needed, nullptr ) ) ) {
		fn::ExFreePoolWithTag ( modules, 'ases' );
		log ( _("! ZwQuerySystemInformation failed: 0x%llX !\n"), status );
		return 0;
	}

	u64 address = 0;

	for ( u32 i = 0; i < modules->ModulesCount; ++i ) {
		SYSTEM_MODULE mod = modules->Modules [ i ];
		
		if ( strstr ( reinterpret_cast< char*>( mod.FullPathName ), name ) ) {
			log ( _("Found module with name \"%s\" @ 0x%llX.\n"), reinterpret_cast< char* >( mod.FullPathName ), mod.ImageBase );

			address = u64 ( mod.ImageBase );
			size = u64 ( mod.ImageSize );
			break;
		}
	}

	fn::ExFreePoolWithTag ( modules, 'ases' );

	return address;
}

void util::sec::clear_piddbcache ( ) {
	u64 size = 0;
	const auto ntoskrnl = get_kerneladdr ( _("ntoskrnl.exe"), size );

	if ( !valid_ptr(ntoskrnl) || !size ) {
		log ( _("Failed to clean PiDDBCacheTable.\n") );
		return;
	}

	const auto PiDDBCacheTable_ptr = find_pattern< u64 > ( ntoskrnl, size, "\x48\x8D\x0D\x00\x00\x00\x00\x45\x8D\x41\x38\xE8", "xxx????xxxxx" );

	if ( !valid_ptr(PiDDBCacheTable_ptr )) {
		log ( _("Failed to clean PiDDBCacheTable (1).\n") );
		return;
	}

	const auto PiDDBCacheTable = resolve_rip<PRTL_AVL_TABLE> ( PiDDBCacheTable_ptr, 3 );

	if ( !valid_ptr(PiDDBCacheTable) ) {
		log (_( "Failed to clean PiDDBCacheTable (2).\n" ));
		return;
	}

	log (_( "PiDDBCacheTable @ 0x%llX\n"), PiDDBCacheTable );

	const auto PiDDBLock_ptr = find_pattern< u64 > ( ntoskrnl, size, "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x4C\x8B\x8C", "xxx????x????xxx" );

	if ( !valid_ptr(PiDDBLock_ptr) ) {
		log ( _("Failed to clean PiDDBCacheTable (3).\n" ));
		return;
	}

	const auto PiDDBLock = resolve_rip<PERESOURCE> ( PiDDBLock_ptr, 3 );

	if ( !valid_ptr(PiDDBLock )) {
		log (_( "Failed to clean PiDDBCacheTable (4).\n") );
		return;
	}

	log ( _("PiDDBLock @ 0x%llX\n"), PiDDBLock );

	/*
	ExAcquireResourceExclusiveLite ( PiDDBLock, true );
	
	PiDDBCacheEntry* entry = nullptr;

	for ( auto p = reinterpret_cast<PiDDBCacheEntry* >( RtlEnumerateGenericTableAvl ( PiDDBCacheTable, true ) );
		p;
		p = reinterpret_cast< PiDDBCacheEntry* >( RtlEnumerateGenericTableAvl ( PiDDBCacheTable, false )) ) {
		if ( p->TimeDateStamp == 0x5284EAC3 ) {
			entry = p;
			break;
		}
	}

	if ( !valid_ptr(entry) ) {
		log ( "Failed to clean PiDDBCacheTable.\n" );
		ExReleaseResourceLite ( PiDDBLock );
		return;
	}

	log ( "PiDDBCacheEntry @ 0x%llX\n", entry );
	
	RemoveEntryList ( &entry->List );
	RtlDeleteElementGenericTableAvl ( PiDDBCacheTable, entry );

	ExReleaseResourceLite ( PiDDBLock );
	*/

	ExAcquireResourceExclusiveLite ( PiDDBLock, true );

	auto entry = reinterpret_cast<PiDDBCacheEntry*> ( reinterpret_cast< u64> ( PiDDBCacheTable->BalancedRoot.RightChild ) + sizeof ( RTL_BALANCED_LINKS ) );

	if ( entry->TimeDateStamp == 0x5284EAC3 ) {
		log ( _("Found bad PiDDBCacheTable driver entry (1).\n") );
	}
	else {
		u32 count = 0;

		for ( auto link = entry->List.Flink; link != entry->List.Blink; link = link->Flink, count++ ) {
			entry = reinterpret_cast< PiDDBCacheEntry* > ( link );

			if ( entry->TimeDateStamp == 0x5284EAC3 ) {
				log (_( "Found bad PiDDBCacheTable driver entry (2).\n") );
				break;
			}
		}
	}

	if ( !valid_ptr ( entry ) ) {
		ExReleaseResourceLite ( PiDDBLock );
		log (_( "Failed to clean PiDDBCacheTable (5).\n" ));
		return;
	}

	RemoveEntryList ( &entry->List );
	RtlDeleteElementGenericTableAvl ( PiDDBCacheTable, entry );

	ExReleaseResourceLite ( PiDDBLock );

	log (_( "Cleaned PiDDBCacheTable.\n") );
}

void util::sec::clear_mmunloadeddrivers ( ) {
	u64 size = 0;
	const auto ntoskrnl = get_kerneladdr (_( "ntoskrnl.exe"), size );

	if ( !valid_ptr(ntoskrnl) ) {
		log (_( "Failed to clean MmUnloadedDrivers.\n" ));
		return;
	}

	log (_( "ntoskrnl.exe @ 0x%llX, size: 0x%llX\n"), ntoskrnl, size );

	const auto MmUnloadedDrivers_ptr = find_pattern< u64 > ( ntoskrnl, size, "\x4C\x39\x3D\x00\x00\x00\x00\x0F\x84\x00\x00\x00\x00\x8B\x05", "xxx????xx????xx" );

	if ( !valid_ptr(MmUnloadedDrivers_ptr )) {
		log ( _("Failed to clean MmUnloadedDrivers.\n") );
		return;
	}

	log (_( "MmUnloadedDrivers_ptr @ 0x%llX\n"), MmUnloadedDrivers_ptr );

	const auto MmUnloadedDrivers = *resolve_rip<UNLOADED_DRIVERS**> ( MmUnloadedDrivers_ptr, 3 );
	auto MmLastUnloadedDriver = resolve_rip<u32*> ( MmUnloadedDrivers_ptr + 13, 2 );
	
	if ( !valid_ptr(MmUnloadedDrivers)
		|| !valid_ptr(MmLastUnloadedDriver) ) {
		log (_( "Failed to clean MmUnloadedDrivers.\n") );
		return;
	}

	log (_( "MmUnloadedDrivers @ 0x%llX\n"), MmUnloadedDrivers );
	log ( _("MmLastUnloadedDriver @ 0x%llX\n"), MmLastUnloadedDriver );
	log ( _("Unloaded driver count @ %u\n"), *MmLastUnloadedDriver );

	(*MmLastUnloadedDriver)--;

	ANSI_STRING bad_driver_name;
	RtlUnicodeStringToAnsiString ( &bad_driver_name, &MmUnloadedDrivers [ *MmLastUnloadedDriver ].Name, true );

	log (_( "Bad driver name: @ 0x%llX\n"), bad_driver_name.Buffer );

	RtlFreeAnsiString ( &bad_driver_name );

	memset ( &MmUnloadedDrivers [ *MmLastUnloadedDriver ], 0, sizeof ( UNLOADED_DRIVERS ) );

	log ( _("Cleaned MmUnloadedDrivers.\n") );
}

HANDLE backup_UniqueProcess = nullptr;
HANDLE backup_UniqueThread = nullptr;
void* backup_Win32StartAddress = nullptr;
void* backup_StackBase = nullptr;
u8 backup_SomeFlag = 0;

void util::sec::hide_thread ( ) {
	const auto eprocess = reinterpret_cast< u64 >( PsGetCurrentProcess ( ) );
	const auto ethread = reinterpret_cast< MYTHREAD* >( PsGetCurrentThread ( ) );

	if ( !valid_ptr(eprocess)
		|| !valid_ptr(ethread) ) {
		log (_( "Failed to hide thread (eprocess or ethread invalid).\n" ));
		return;
	}

	/* decrease thread count by 1 (so it was never created in the first place)*/
	//(*reinterpret_cast< u32* >( eprocess + 0x498 ))--;
	
	log ( _("Decrimented thread count.\n" ));

	backup_UniqueProcess = ethread->Cid.UniqueProcess;
	backup_UniqueThread = ethread->Cid.UniqueThread;
	backup_Win32StartAddress = ethread->Win32StartAddress;
	backup_StackBase = *reinterpret_cast< void** >( &ethread->Win32StartAddress + 1 );
	backup_SomeFlag = *reinterpret_cast< u8* >( ethread );

	/* spoof thread information */
	ethread->Cid.UniqueProcess = nullptr;
	ethread->Cid.UniqueThread = nullptr;
	ethread->Win32StartAddress = nullptr;
	/* ethread->StackBase */
	*reinterpret_cast< void** >( &ethread->Win32StartAddress + 1 ) = nullptr;
	*reinterpret_cast< u8* >( ethread ) = 4;

	log ( _("Spoofed thread information.\n") );
}

void util::sec::restore_thread ( ) {
	/* reverse thread removal */
	const auto eprocess = reinterpret_cast< u64 >( PsGetCurrentProcess ( ) );
	const auto ethread = reinterpret_cast< MYTHREAD* >( PsGetCurrentThread ( ) );
	
	//(*reinterpret_cast< u32* >( eprocess + 0x498 ))++;
	
	ethread->Cid.UniqueProcess = backup_UniqueProcess;
	ethread->Cid.UniqueThread = backup_UniqueThread;
	ethread->Win32StartAddress = backup_Win32StartAddress;
	*reinterpret_cast< void** >( &ethread->Win32StartAddress + 1 ) = backup_StackBase;
	*reinterpret_cast< u8* >( ethread ) = backup_SomeFlag;

	log ( _("Restored thread information.\n") );
}

void util::delay ( ) {
	i64 delay_100ns = -1;
	fn::KeDelayExecutionThread ( KernelMode, false, reinterpret_cast< LARGE_INTEGER *>( &delay_100ns ));
}

bool util::data_cmp( const u8* data, const u8* sig, const char* mask ) {
	for ( ; *mask; ++mask, ++data, ++sig )
		if ( *mask == 'x' && *data != *sig )
			return 0;

	return !( *mask );
}

template <typename t = u64>
t util::find_pattern ( u64 start, u64 length, const char* pattern, const char* mask ) {
	const auto data = reinterpret_cast< const char* >( start );
	const auto pattern_length = strlen ( mask );

	for ( size_t i = 0; i <= length - pattern_length; i++ ) {
		auto accumulative_found = true;

		for ( size_t j = 0; j < pattern_length; j++ ) {
			if ( !MmIsAddressValid ( reinterpret_cast< void* >( reinterpret_cast< u64 >( data ) + i + j ) ) ) {
				accumulative_found = false;
				break;
			}

			if ( data [ i + j ] != pattern [ j ] && mask [ j ] != '?' ) {
				accumulative_found = false;
				break;
			}
		}

		if ( accumulative_found )
			return t ( reinterpret_cast< u64 >( data ) + i );
	}

	return reinterpret_cast< t > ( nullptr );
}

/* cached processes */
/* pid, process */
struct cached_process_t {
	u64 pid = 0;
	PEPROCESS proc = nullptr;
};

cached_process_t cached_processes [ 6 ] { { 0, nullptr } };

void util::free_processes ( ) {
	for ( auto& iter : cached_processes ) {
		if ( iter.proc )
			fn::ObfDereferenceObject ( reinterpret_cast< void* >( iter.proc ) );
	}
}

PEPROCESS util::get_process ( u64 pid ) {
	/* temp */
	PEPROCESS ret = nullptr;
	fn::PsLookupProcessByProcessId ( reinterpret_cast< void* >( pid ), &ret );
	return ret;

	/* real */
	if ( !pid )
		return nullptr;

	/* return process if it exists */
	for ( auto& iter : cached_processes ) {
		if ( iter.pid == pid ) {
			if ( !iter.proc )
				fn::PsLookupProcessByProcessId ( reinterpret_cast< void* >( pid ), &iter.proc );

			return iter.proc;
		}
	}

	/* entry doesnt exist yet (find slot to put in) */
	for ( auto& iter : cached_processes ) {
		if ( !iter.pid ) {
			iter.pid = pid;
			fn::PsLookupProcessByProcessId ( reinterpret_cast< void* >( pid ), &iter.proc );
			return iter.proc;
		}
	}

	/* if we hit this, something went wrong */
	return nullptr;
}