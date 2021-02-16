#pragma once

#include "types.hpp"

#include <ntdef.h>
#include <ntddk.h>

#include "xorstr/xorstr.hpp"

namespace fn {
	namespace types {
		typedef enum _SYSTEM_INFORMATION_CLASS {
			SystemModuleInformation = 1
		} SYSTEM_INFORMATION_CLASS;

#pragma pack(push, 1)
		typedef struct _KAPC_STATE {
			LIST_ENTRY ApcListHead [ 2 ];
			struct _KPROCESS* Process;
			BOOLEAN KernelApcInProgress;
			BOOLEAN KernelApcPending;
			BOOLEAN UserApcPending;
		} KAPC_STATE, * PKAPC_STATE, * PRKAPC_STATE;

		typedef struct _LDR_DATA_TABLE_ENTRY
		{
			LIST_ENTRY InLoadOrderLinks;
			LIST_ENTRY InMemoryOrderLinks;
			union
			{
				LIST_ENTRY InInitializationOrderLinks;
				LIST_ENTRY InProgressLinks;
			};
			PVOID           DllBase;
			PVOID           Entrypoint;
			ULONG           SizeOfImage;
			UNICODE_STRING  FullDllName;
			UNICODE_STRING  BaseDllName;
		} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

		typedef struct _PEB_LDR_DATA
		{
			ULONG       Length;
			UCHAR       Initialized;
			PVOID       SsHandle;
			LIST_ENTRY  InLoadOrderModuleList;
			LIST_ENTRY  InMemoryOrderModuleList;
			LIST_ENTRY  InInitializationOrderModuleList;
		} PEB_LDR_DATA, * PPEB_LDR_DATA;

		typedef struct _RTL_USER_PROCESS_PARAMETERS {
			CHAR           Reserved1 [ 16 ];
			PVOID          Reserved2 [ 10 ];
			UNICODE_STRING ImagePathName;
			UNICODE_STRING CommandLine;
		} RTL_USER_PROCESS_PARAMETERS, * PRTL_USER_PROCESS_PARAMETERS;

		typedef struct _PEB
		{
			BOOLEAN                         InheritedAddressSpace;
			BOOLEAN                         ReadImageFileExecOptions;
			BOOLEAN                         BeingDebugged;
			BOOLEAN                         BitField;
			HANDLE                          Mutant;
			PVOID                           ImageBaseAddress;
			PPEB_LDR_DATA                   Ldr;
			PRTL_USER_PROCESS_PARAMETERS    ProcessParameters;
			PVOID                           SubSystemData;
			PVOID                           ProcessHeap;
			PVOID                           FastPebLock;
		} PEB, * PPEB;

		typedef struct _RTL_PROCESS_MODULE_INFORMATION {
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
		} RTL_PROCESS_MODULE_INFORMATION, * PRTL_PROCESS_MODULE_INFORMATION;

		typedef struct _RTL_PROCESS_MODULES {
			ULONG NumberOfModules;
			RTL_PROCESS_MODULE_INFORMATION Modules [ 1 ];
		} RTL_PROCESS_MODULES, * PRTL_PROCESS_MODULES;

#pragma pack(pop)

		using MmCopyVirtualMemory_fn = NTSTATUS( __stdcall* )( PEPROCESS, PVOID, PEPROCESS, PVOID, SIZE_T, KPROCESSOR_MODE, PSIZE_T );
		using ZwQuerySystemInformation_fn = NTSTATUS( __stdcall* )( SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG );
		using PsCreateSystemThread_fn = NTSTATUS( __stdcall* )( PHANDLE, ULONG, POBJECT_ATTRIBUTES, HANDLE, PCLIENT_ID, PKSTART_ROUTINE, PVOID );
		using PsTerminateSystemThread_fn = NTSTATUS( __stdcall* )( NTSTATUS );
		using ZwClose_fn = NTSTATUS( __stdcall* )( HANDLE );
		using PsLookupProcessByProcessId_fn = NTSTATUS( __stdcall* )( HANDLE, PEPROCESS* );
		using ZwAllocateVirtualMemory_fn = NTSTATUS( __stdcall* )( HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG );
		using KeStackAttachProcess_fn = NTSTATUS( __stdcall* )( PRKPROCESS, PRKAPC_STATE );
		using KeUnstackDetachProcess_fn = NTSTATUS( __stdcall* )( PRKAPC_STATE );
		using ObfDereferenceObject_fn = NTSTATUS( __stdcall* )( PVOID );
		using IoAllocateMdl_fn = PMDL( __stdcall* )( PVOID, ULONG, BOOLEAN, BOOLEAN, PIRP );
		using MmProbeAndLockPages_fn = void( __stdcall* )( PMDL, KPROCESSOR_MODE, LOCK_OPERATION );
		using MmMapLockedPagesSpecifyCache_fn = PVOID( __stdcall* )( PMDL, KPROCESSOR_MODE, MEMORY_CACHING_TYPE, PVOID, ULONG, ULONG );
		using MmProtectMdlSystemAddress_fn = NTSTATUS( __stdcall* )( PMDL, ULONG );
		using MmUnmapLockedPages_fn = void( __stdcall* )( PVOID, PMDL );
		using MmUnlockPages_fn = void( __stdcall* )( PMDL );
		using IoFreeMdl_fn = void( __stdcall* )( PMDL );
		using ExAllocatePool_fn = PVOID( __stdcall* )( POOL_TYPE, SIZE_T );
		using KeDelayExecutionThread_fn = PVOID( __stdcall* )( KPROCESSOR_MODE, BOOLEAN, PLARGE_INTEGER );
		using ExFreePoolWithTag_fn = void( __stdcall* )( PVOID, ULONG );
		using RtlZeroMemory_fn = void( __stdcall* )( void*, SIZE_T );
		using ExFreePool_fn = void( __stdcall* )( PVOID );
		using PsGetProcessSectionBaseAddress_fn = PVOID( __stdcall* )( PEPROCESS );
		using ExAllocatePoolWithTag_fn = PVOID( __stdcall* )( POOL_TYPE, SIZE_T, ULONG );
		using MmBuildMdlForNonPagedPool_fn = void( __stdcall* )( PMDL );
		using PsGetProcessPeb_fn = PPEB( __stdcall* )( PEPROCESS );
		using PsGetProcessWow64Process_fn = PVOID( __stdcall* )( PEPROCESS );
		using KeAttachProcess_fn = void( __stdcall* )( PRKPROCESS );
		using KeDetachProcess_fn = void( __stdcall* )( );
		using RtlLookupElementGenericTableAvl_fn = PVOID( __stdcall* )( PRTL_AVL_TABLE, PVOID );
		using PspTerminateThreadByPointer_fn = NTSTATUS( __stdcall* )( PETHREAD, NTSTATUS, BOOLEAN );
		using KphTerminateThread_fn = NTSTATUS( __stdcall* )( HANDLE, u32 );
		using PsLookupThreadByThreadId_fn = NTSTATUS ( __stdcall* )( HANDLE, PETHREAD* );
		using MmProbeAndLockProcessPages_fn = void( __stdcall* )( PMDL, PEPROCESS, KPROCESSOR_MODE, LOCK_OPERATION );
		using ObReferenceObjectByName_fn = NTSTATUS ( __stdcall* )( PUNICODE_STRING, ULONG, PACCESS_STATE, ACCESS_MASK, POBJECT_TYPE, KPROCESSOR_MODE, PVOID, PVOID* );
	}

	inline types::MmCopyVirtualMemory_fn MmCopyVirtualMemory = nullptr;
	inline types::ZwQuerySystemInformation_fn ZwQuerySystemInformation = nullptr;
	inline types::PsCreateSystemThread_fn PsCreateSystemThread = nullptr;
	inline types::PsTerminateSystemThread_fn PsTerminateSystemThread = nullptr;
	inline types::ZwClose_fn ZwClose = nullptr;
	inline types::PsLookupProcessByProcessId_fn PsLookupProcessByProcessId = nullptr;
	inline types::ZwAllocateVirtualMemory_fn ZwAllocateVirtualMemory = nullptr;
	inline types::KeStackAttachProcess_fn KeStackAttachProcess = nullptr;
	inline types::KeUnstackDetachProcess_fn KeUnstackDetachProcess = nullptr;
	inline types::ObfDereferenceObject_fn ObfDereferenceObject = nullptr;
	inline types::IoAllocateMdl_fn IoAllocateMdl = nullptr;
	inline types::MmProbeAndLockPages_fn MmProbeAndLockPages = nullptr;
	inline types::MmMapLockedPagesSpecifyCache_fn MmMapLockedPagesSpecifyCache = nullptr;
	inline types::MmProtectMdlSystemAddress_fn MmProtectMdlSystemAddress = nullptr;
	inline types::MmUnmapLockedPages_fn MmUnmapLockedPages = nullptr;
	inline types::MmUnlockPages_fn MmUnlockPages = nullptr;
	inline types::IoFreeMdl_fn IoFreeMdl = nullptr;
	inline types::ExAllocatePool_fn ExAllocatePool = nullptr;
	inline types::KeDelayExecutionThread_fn KeDelayExecutionThread = nullptr;
	inline types::ExFreePoolWithTag_fn ExFreePoolWithTag = nullptr;
	inline types::RtlZeroMemory_fn RtlZeroMemory = nullptr;
	inline types::ExFreePool_fn ExFreePool = nullptr;
	inline types::PsGetProcessSectionBaseAddress_fn PsGetProcessSectionBaseAddress = nullptr;
	inline types::ExAllocatePoolWithTag_fn ExAllocatePoolWithTag = nullptr;
	inline types::MmBuildMdlForNonPagedPool_fn MmBuildMdlForNonPagedPool = nullptr;
	inline types::PsGetProcessPeb_fn PsGetProcessPeb = nullptr;
	inline types::PsGetProcessWow64Process_fn PsGetProcessWow64Process = nullptr;
	inline types::KeAttachProcess_fn KeAttachProcess = nullptr;
	inline types::KeDetachProcess_fn KeDetachProcess = nullptr;
	inline types::RtlLookupElementGenericTableAvl_fn RtlLookupElementGenericTableAvl = nullptr;
	inline types::PspTerminateThreadByPointer_fn PspTerminateThreadByPointer = nullptr;
	inline types::KphTerminateThread_fn KphTerminateThread = nullptr;
	inline types::PsLookupThreadByThreadId_fn PsLookupThreadByThreadId = nullptr;
	inline types::MmProbeAndLockProcessPages_fn MmProbeAndLockProcessPages = nullptr;
	inline types::ObReferenceObjectByName_fn ObReferenceObjectByName = nullptr;

	namespace util {
		template < typename t >
		inline t get_routine( const wchar_t* name ) {
			UNICODE_STRING routine_name;
			RtlInitUnicodeString ( &routine_name, name );
			return ( t ) MmGetSystemRoutineAddress( &routine_name );
		}
	}

	inline void init ( ) {
		MmCopyVirtualMemory = util::get_routine< types::MmCopyVirtualMemory_fn > ( _w(L"MmCopyVirtualMemory") );
		ZwQuerySystemInformation = util::get_routine< types::ZwQuerySystemInformation_fn > ( _w(L"ZwQuerySystemInformation") );
		PsCreateSystemThread = util::get_routine< types::PsCreateSystemThread_fn > ( _w(L"PsCreateSystemThread") );
		PsTerminateSystemThread = util::get_routine< types::PsTerminateSystemThread_fn > ( _w(L"PsTerminateSystemThread"));
		ZwClose = util::get_routine< types::ZwClose_fn > ( _w(L"ZwClose") );
		PsLookupProcessByProcessId = util::get_routine< types::PsLookupProcessByProcessId_fn > ( _w(L"PsLookupProcessByProcessId") );
		ZwAllocateVirtualMemory = util::get_routine< types::ZwAllocateVirtualMemory_fn > ( _w(L"ZwAllocateVirtualMemory") );
		KeStackAttachProcess = util::get_routine< types::KeStackAttachProcess_fn > ( _w(L"KeStackAttachProcess") );
		KeUnstackDetachProcess = util::get_routine< types::KeUnstackDetachProcess_fn > ( _w(L"KeUnstackDetachProcess" ));
		ObfDereferenceObject = util::get_routine< types::ObfDereferenceObject_fn > (_w( L"ObfDereferenceObject" ));
		IoAllocateMdl = util::get_routine< types::IoAllocateMdl_fn > ( _w(L"IoAllocateMdl") );
		MmProbeAndLockPages = util::get_routine< types::MmProbeAndLockPages_fn > ( _w(L"MmProbeAndLockPages" ));
		MmMapLockedPagesSpecifyCache = util::get_routine< types::MmMapLockedPagesSpecifyCache_fn > (_w( L"MmMapLockedPagesSpecifyCache") );
		MmProtectMdlSystemAddress = util::get_routine< types::MmProtectMdlSystemAddress_fn > ( _w(L"MmProtectMdlSystemAddress") );
		MmUnmapLockedPages = util::get_routine< types::MmUnmapLockedPages_fn > ( _w(L"MmUnmapLockedPages") );
		MmUnlockPages = util::get_routine< types::MmUnlockPages_fn > ( _w(L"MmUnlockPages") );
		IoFreeMdl = util::get_routine< types::IoFreeMdl_fn > ( _w(L"IoFreeMdl") );
		ExAllocatePool = util::get_routine< types::ExAllocatePool_fn > ( _w(L"ExAllocatePool" ));
		KeDelayExecutionThread = util::get_routine< types::KeDelayExecutionThread_fn > ( _w(L"KeDelayExecutionThread" ));
		ExFreePoolWithTag = util::get_routine< types::ExFreePoolWithTag_fn > (_w( L"ExFreePoolWithTag") );
		RtlZeroMemory = util::get_routine< types::RtlZeroMemory_fn > ( _w(L"RtlZeroMemory") );
		ExFreePool = util::get_routine< types::ExFreePool_fn > ( _w(L"ExFreePool" ));
		PsGetProcessSectionBaseAddress = util::get_routine< types::PsGetProcessSectionBaseAddress_fn > ( _w(L"PsGetProcessSectionBaseAddress") );
		ExAllocatePoolWithTag = util::get_routine< types::ExAllocatePoolWithTag_fn > ( _w(L"ExAllocatePoolWithTag") );
		MmBuildMdlForNonPagedPool = util::get_routine< types::MmBuildMdlForNonPagedPool_fn > ( _w(L"MmBuildMdlForNonPagedPool" ));
		PsGetProcessPeb = util::get_routine< types::PsGetProcessPeb_fn > ( _w(L"PsGetProcessPeb" ));
		PsGetProcessWow64Process = util::get_routine< types::PsGetProcessWow64Process_fn > ( _w(L"PsGetProcessWow64Process") );
		KeAttachProcess = util::get_routine< types::KeAttachProcess_fn > (_w( L"KeAttachProcess" ));
		KeDetachProcess = util::get_routine< types::KeDetachProcess_fn > ( _w(L"KeDetachProcess") );
		RtlLookupElementGenericTableAvl = util::get_routine< types::RtlLookupElementGenericTableAvl_fn > ( _w(L"RtlLookupElementGenericTableAvl") );
		PsLookupThreadByThreadId = util::get_routine< types::PsLookupThreadByThreadId_fn > (_w( L"PsLookupThreadByThreadId" ));
		MmProbeAndLockProcessPages = util::get_routine< types::MmProbeAndLockProcessPages_fn > (_w( L"MmProbeAndLockProcessPages") );
		ObReferenceObjectByName = util::get_routine< types::ObReferenceObjectByName_fn > ( _w(L"ObReferenceObjectByName") );
		//KphTerminateThread = types::KphTerminateThread_fn( ::util::find( "\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x48\x83\xEC\x40", "xxxx?xxxx?xxxxx" ) );
		//PspTerminateThreadByPointer = types::PspTerminateThreadByPointer_fn( ::util::find( "\x41\x8A\xE8\x48\x8B\xB9\x00\x00\x00\x00\x8B\xF2\x48\x8B\xD9", "xxxxxx????xxxxx" ) - 0x1a );
	}
}