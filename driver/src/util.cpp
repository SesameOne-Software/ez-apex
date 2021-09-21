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

typedef struct _HashBucketEntry {
	struct _HashBucketEntry* Next;
	UNICODE_STRING DriverName;
	ULONG CertHash [ 5 ];
} HashBucketEntry, * PHashBucketEntry;

struct _POOL_TRACKER_BIG_PAGES {
	volatile ULONGLONG Va;                                                  //0x0
	ULONG Key;                                                              //0x8
	ULONG Pattern : 8;                                                        //0xc
	ULONG PoolType : 12;                                                      //0xc
	ULONG SlushSize : 12;                                                     //0xc
	ULONGLONG NumberOfBytes;                                                //0x10
};

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
	VMP_BEGINMUTATION ( );
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
			//log ( _("Found module with name \"%s\" @ 0x%llX.\n"), reinterpret_cast< char* >( mod.FullPathName ), mod.ImageBase );

			address = u64 ( mod.ImageBase );
			size = u64 ( mod.ImageSize );
			break;
		}
	}

	fn::ExFreePoolWithTag ( modules, 'ases' );

	return address;
	VMP_END ( );
}

bool util::sec::clear_piddbcache ( ) {
	VMP_BEGINMUTATION ( );
	u64 size = 0;
	const auto ntoskrnl = get_kerneladdr ( _("ntoskrnl.exe"), size );

	if ( !valid_ptr(ntoskrnl) || !size ) {
		log ( _("Failed to clean PiDDBCacheTable.\n") );
		return false;
	}

	const auto PiDDBCacheTable_ptr = find_pattern< u64 > ( ntoskrnl, size, _("\x66\x03\xD2\x48\x8D\x0D"), _("xxxxxx") );

	if ( !valid_ptr(PiDDBCacheTable_ptr )) {
		log ( _("Failed to clean PiDDBCacheTable (1).\n") );
		return false;
	}

	const auto PiDDBCacheTable = *resolve_rip<PRTL_AVL_TABLE*> ( PiDDBCacheTable_ptr + 3, 3 );

	if ( !valid_ptr(PiDDBCacheTable) ) {
		log (_( "Failed to clean PiDDBCacheTable (2).\n" ));
		return false;
	}

	log (_( "PiDDBCacheTable @ 0x%llX\n"), PiDDBCacheTable );

	auto PiDDBLock_ptr = find_pattern< u64 > ( ntoskrnl, size, _("\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x4C\x8B\x8C\x24"), _("xxx????x????xxxx") );

	if (!PiDDBLock_ptr ) /* win11 sig */
		PiDDBLock_ptr = find_pattern< u64 > ( ntoskrnl, size, _ ( "\x48\x8D\x0D\x00\x00\x00\x00\xB2\x01\x66\xFF\x88\x00\x00\x00\x00\x90" ), _ ( "xxx????xxxxx????x" ) );

	if ( !valid_ptr ( PiDDBLock_ptr ) ) {
		log ( _ ( "Failed to clean PiDDBCacheTable (3).\n" ) );
		return false;
	}

	const auto PiDDBLock = resolve_rip<PERESOURCE> ( PiDDBLock_ptr, 3 );

	if ( !valid_ptr(PiDDBLock )) {
		log (_( "Failed to clean PiDDBCacheTable (4).\n") );
		return false;
	}

	log ( _("PiDDBLock @ 0x%llX\n"), PiDDBLock );

	ExEnterCriticalRegionAndAcquireResourceExclusive ( PiDDBLock );

	auto entry = reinterpret_cast<PiDDBCacheEntry*> ( reinterpret_cast< u64> ( PiDDBCacheTable->BalancedRoot.RightChild ) + sizeof ( RTL_BALANCED_LINKS ) );

	if ( entry->TimeDateStamp == 0x5284EAC3 ) {
		log ( _("Found bad PiDDBCacheTable driver entry.\n") );
	}
	else {
		u32 count = 0;

		for ( auto link = entry->List.Flink; link != entry->List.Blink; link = link->Flink, count++ ) {
			entry = reinterpret_cast< PiDDBCacheEntry* > ( link );

			if ( entry->TimeDateStamp == 0x5284EAC3 ) {
				log (_( "Found bad PiDDBCacheTable driver entry.\n") );
				break;
			}
		}
	}

	if ( !valid_ptr ( entry ) ) {
		ExReleaseResourceAndLeaveCriticalRegion ( PiDDBLock );
		log (_( "Failed to clean PiDDBCacheTable (5).\n" ));
		return false;
	}

	RemoveEntryList ( &entry->List );
	RtlDeleteElementGenericTableAvl ( PiDDBCacheTable, entry );

	ExReleaseResourceAndLeaveCriticalRegion ( PiDDBLock );

	log (_( "Cleaned PiDDBCacheTable.\n") );
	return true;
	VMP_END ( );
}

bool util::sec::clear_mmunloadeddrivers ( ) {
	VMP_BEGINMUTATION ( );
	u64 size = 0;
	const auto ntoskrnl = get_kerneladdr (_( "ntoskrnl.exe"), size );

	if ( !valid_ptr(ntoskrnl) ) {
		log (_( "Failed to clean MmUnloadedDrivers (1).\n" ));
		return false;
	}

	log (_( "ntoskrnl.exe @ 0x%llX, size: 0x%llX\n"), ntoskrnl, size );

	const auto MmUnloadedDrivers_ptr = find_pattern< u64 > ( ntoskrnl, size, _("\x4C\x8B\x15\x00\x00\x00\x00\x4C\x8B\xC9"), _("xxx????xxx") );
	const auto MmLastUnloadedDriver_ptr = find_pattern< u64 > ( ntoskrnl, size, _("\x8B\x05\x00\x00\x00\x00\x83\xF8\x32"), _("xx????xxx") );

	if ( !valid_ptr ( MmUnloadedDrivers_ptr ) || !valid_ptr ( MmLastUnloadedDriver_ptr ) ) {
		log ( _("Failed to clean MmUnloadedDrivers (2).\n") );
		return false;
	}

	log (_( "MmUnloadedDrivers_ptr @ 0x%llX\n"), MmUnloadedDrivers_ptr );

	const auto MmUnloadedDrivers = *resolve_rip<UNLOADED_DRIVERS**> ( MmUnloadedDrivers_ptr, 3 );
	auto MmLastUnloadedDriver = resolve_rip<u32*> ( MmLastUnloadedDriver_ptr, 2 );
	
	if ( !valid_ptr(MmUnloadedDrivers)
		|| !valid_ptr(MmLastUnloadedDriver) ) {
		log (_( "Failed to clean MmUnloadedDrivers (3).\n") );
		return false;
	}

	log (_( "MmUnloadedDrivers @ 0x%llX\n"), MmUnloadedDrivers );
	log ( _("MmLastUnloadedDriver @ 0x%llX\n"), MmLastUnloadedDriver );
	log ( _("Unloaded driver count @ %u\n"), *MmLastUnloadedDriver );

	int entr_idx = -1;
	bool cleaned_one_entry = false;

	for ( ULONG i = 0; i < 50; i++ ) {
		UNLOADED_DRIVERS* entry = &MmUnloadedDrivers [ i ];

		/* our driver does not have a file extension in the driver name */
		if ( entry->Name.Buffer && entry->Name.Length >= 4
			&& !( entry->Name.Buffer [ entry->Name.Length - 4 ] == L'.' && entry->Name.Buffer [ entry->Name.Length - 3 ] == L's' && entry->Name.Buffer [ entry->Name.Length - 2 ] == L'y' && entry->Name.Buffer [ entry->Name.Length - 1 ] == L's' ) ) {
			memset ( entry, 0, sizeof ( UNLOADED_DRIVERS ) );
			cleaned_one_entry = true;
			entr_idx = i;
		}
	}
	
	if ( entr_idx == *MmLastUnloadedDriver - 1 )
		*MmLastUnloadedDriver--;

	if ( cleaned_one_entry )
		log ( _("Cleaned MmUnloadedDrivers.\n") );
	else
		log ( _ ( "Failed to clean MmUnloadedDrivers (4).\n" ) );

	return true;
	VMP_END ( );
}

HANDLE backup_UniqueProcess = nullptr;
HANDLE backup_UniqueThread = nullptr;
void* backup_Win32StartAddress = nullptr;
void* backup_StackBase = nullptr;
u8 backup_SomeFlag = 0;

bool util::sec::hide_thread ( ) {
	VMP_BEGINMUTATION ( );
	const auto eprocess = reinterpret_cast< u64 >( PsGetCurrentProcess ( ) );
	const auto ethread = reinterpret_cast< MYTHREAD* >( PsGetCurrentThread ( ) );

	if ( !valid_ptr(eprocess)
		|| !valid_ptr(ethread) ) {
		log (_( "Failed to hide thread (eprocess or ethread invalid).\n" ));
		return false;
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

	return true;
	VMP_END ( );
}

bool util::sec::clear_big_pool_table ( ) {
	VMP_BEGINMUTATION ( );
	u64 size = 0;
	const auto ntoskrnl = get_kerneladdr ( _ ( "ntoskrnl.exe" ), size );

	if ( !valid_ptr ( ntoskrnl ) || !size ) {
		log ( _ ( "Failed to clean BigPoolTable (1).\n" ) );
		return false;
	}

	auto BigPoolTable_ptr = find_pattern< u64 > ( ntoskrnl, size, _("\x48\x8B\x15\x00\x00\x00\x00\x4C\x8D\x0D\x00\x00\x00\x00\x4C"), _("xxx????xxx????x") );

	if ( !BigPoolTable_ptr ) /* win11 sig */
		BigPoolTable_ptr = find_pattern< u64 > ( ntoskrnl, size, _ ( "\x48\x8B\x15\x00\x00\x00\x00\x4C\x8B\x0D\x00\x00\x00\x00\x48" ), _ ( "xxx????xxx????x" ) );

	auto BigPoolTable_size_ptr = find_pattern< u64 > ( ntoskrnl, size, _("\x4C\x8B\x15\x00\x00\x00\x00\x48\x85"), _("xxx????xx") );

	if ( !BigPoolTable_size_ptr ) /* win11 sig */
		BigPoolTable_size_ptr = find_pattern< u64 > ( ntoskrnl, size, _ ( "\x4C\x8B\x0D\x00\x00\x00\x00\x48\x85\xD2\x74\x50" ), _ ( "xxx????xxxxx" ) );

	if ( !valid_ptr ( BigPoolTable_ptr ) || !valid_ptr ( BigPoolTable_size_ptr ) ) {
		log ( _ ( "Failed to clean BigPoolTable (2).\n" ) );
		return false;
	}

	auto BigPoolTable = *resolve_rip<_POOL_TRACKER_BIG_PAGES**> ( BigPoolTable_ptr, 3 );
	auto BigPoolTable_size = *resolve_rip<size_t*> ( BigPoolTable_size_ptr, 3 );

	if ( !valid_ptr ( BigPoolTable ) ) {
		log ( _ ( "Failed to clean BigPoolTable (3).\n" ) );
		return false;
	}
	
	log ( _ ( "BigPoolTable length: 0x%d\n" ), BigPoolTable_size );

	bool found_at_least_one = false;

	for ( size_t i = 0; i < BigPoolTable_size; i++ ) {
		_POOL_TRACKER_BIG_PAGES* entry = &BigPoolTable [ i ];

		if ( !entry->Key )
			continue;

		if ( entry->Key == 'ases' ) {
			memset ( entry, 0, sizeof ( _POOL_TRACKER_BIG_PAGES ) );
			found_at_least_one = true;
		}
	}

	if ( found_at_least_one ) {
		log ( _ ( "Cleaned BigPoolTable.\n" ) );
		return true;
	}
	else
		log ( _ ( "Failed to clean BigPoolTable (4).\n" ) );

	return false;
	VMP_END ( );
}

bool util::sec::clear_kernelhashbucketlist ( ) {
	VMP_BEGINMUTATION ( );
	u64 size = 0;
	const auto ci = get_kerneladdr ( _ ( "CI.dll" ), size );

	if ( !valid_ptr ( ci ) || !size ) {
		log ( _ ( "Failed to clean KernelHashBucketList (1).\n" ) );
		return false;
	}

	const auto KernelHashBucketList_ptr = find_pattern< u64 > ( ci, size, _("\x48\x8B\x1D\x00\x00\x00\x00\xEB\x00\xF7\x43\x40\x00\x20\x00\x00"), _("xxx????x?xxxxxxx") );

	if ( !valid_ptr ( KernelHashBucketList_ptr ) ) {
		log ( _ ( "Failed to clean KernelHashBucketList (2).\n" ) );
		return false;
	}

	log ( _ ( "KernelHashBucketList_ptr: 0x%llX\n" ), KernelHashBucketList_ptr );
	
	const auto HashCacheLock_ptr = find_pattern< u64 > ( KernelHashBucketList_ptr - 50, 50, _("\x48\x8D\x0D"), _("xxx") );

	if ( !valid_ptr ( HashCacheLock_ptr ) ) {
		log ( _ ( "Failed to clean KernelHashBucketList (3).\n" ) );
		return false;
	}

	log ( _ ( "HashCacheLock_ptr: 0x%llX\n" ), HashCacheLock_ptr );

	auto KernelHashBucketList = *resolve_rip<SINGLE_LIST_ENTRY**> ( KernelHashBucketList_ptr, 3 );
	auto HashCacheLock = resolve_rip<PERESOURCE> ( HashCacheLock_ptr, 3 );

	if ( !valid_ptr ( KernelHashBucketList ) || !valid_ptr ( HashCacheLock ) ) {
		log ( _ ( "Failed to clean KernelHashBucketList (4).\n" ) );
		return false;
	}

	log ( _ ( "KernelHashBucketList: 0x%llX\n" ), KernelHashBucketList );
	log ( _ ( "HashCacheLock: 0x%llX\n" ), HashCacheLock );
	
	ExEnterCriticalRegionAndAcquireResourceExclusive ( HashCacheLock );
	
	UNICODE_STRING str = RTL_CONSTANT_STRING ( L"" );
	
	bool found_at_least_one = false;

	/* iterate throgh list entries */
	for ( SINGLE_LIST_ENTRY* iter = KernelHashBucketList, *prev = KernelHashBucketList; iter; prev = iter, iter = iter->Next ) {
		const auto entry = reinterpret_cast< PHashBucketEntry >( iter );

		/* find the driver */
		if ( entry->DriverName.Buffer && entry->DriverName.Length >= 4
			&& !( entry->DriverName.Buffer [ entry->DriverName.Length - 4 ] == L'.' && entry->DriverName.Buffer [ entry->DriverName.Length - 3 ] == L's' && entry->DriverName.Buffer [ entry->DriverName.Length - 2 ] == L'y' && entry->DriverName.Buffer [ entry->DriverName.Length - 1 ] == L's' ) ) {
			/* set previous entry to point to next */
			if ( prev ) {
				prev->Next = iter->Next;

				/* skip to next entry in our loop */
				iter = iter->Next;
			}

			/* free the entry */
			fn::ExFreePoolWithTag ( entry, 'UAEP' );

			found_at_least_one = true;
			break;
		}
	}

	ExReleaseResourceAndLeaveCriticalRegion ( HashCacheLock );

	if ( found_at_least_one ) {
		log ( _ ( "Cleaned KernelHashBucketList.\n" ) );
		return true;
	}

	log ( _ ( "Failed to clean KernelHashBucketList (5).\n" ) );

	return false;
	VMP_END ( );
}

void util::sec::restore_thread ( ) {
	VMP_BEGINMUTATION ( );
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
	VMP_END ( );
}

void util::delay ( ) {
	VMP_BEGINMUTATION ( );
	i64 delay_100ns = -1;
	fn::KeDelayExecutionThread ( KernelMode, false, reinterpret_cast< LARGE_INTEGER *>( &delay_100ns ));
	VMP_END ( );
}

bool util::data_cmp( const u8* data, const u8* sig, const char* mask ) {
	for ( ; *mask; ++mask, ++data, ++sig )
		if ( *mask == 'x' && *data != *sig )
			return 0;

	return !( *mask );
}

template <typename t>
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
	VMP_BEGINMUTATION ( );
	for ( auto& iter : cached_processes ) {
		if ( iter.proc )
			fn::ObfDereferenceObject ( reinterpret_cast< void* >( iter.proc ) );
	}
	VMP_END ( );
}

PEPROCESS util::get_process ( u64 pid ) {
	VMP_BEGINMUTATION ( );
	/* temp */
	PEPROCESS ret = nullptr;

	if ( !NT_SUCCESS ( fn::PsLookupProcessByProcessId ( reinterpret_cast< void* >( pid ), &ret ) ) )
		return nullptr;

	return ret;

	///* real */
	//if ( !pid )
	//	return nullptr;
	//
	///* return process if it exists */
	//for ( auto& iter : cached_processes ) {
	//	if ( iter.pid == pid ) {
	//		if ( !iter.proc )
	//			fn::PsLookupProcessByProcessId ( reinterpret_cast< void* >( pid ), &iter.proc );
	//
	//		return iter.proc;
	//	}
	//}
	//
	///* entry doesnt exist yet (find slot to put in) */
	//for ( auto& iter : cached_processes ) {
	//	if ( !iter.pid ) {
	//		iter.pid = pid;
	//		fn::PsLookupProcessByProcessId ( reinterpret_cast< void* >( pid ), &iter.proc );
	//		return iter.proc;
	//	}
	//}
	//
	///* if we hit this, something went wrong */
	//return nullptr;
	VMP_END ( );
}