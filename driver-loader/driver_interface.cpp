#pragma once
#include "driver_interface.hpp"

bool drv::dispatch_request ( request_t& args ) {
	static auto NtGdiDdDDIGetProcessDeviceRemovalSupport
		= reinterpret_cast< uint64_t ( __stdcall* )( hook_args_t*, uint64_t* ) >(
			GetProcAddress ( LoadLibraryA ( "win32u.dll" ), "NtGdiDdDDIGetProcessDeviceRemovalSupport" )
			);

	hook_args_t data;

	data.magic = magic_number;
	data.args = &args;
	
	uint64_t status = sesame_error;

	NtGdiDdDDIGetProcessDeviceRemovalSupport ( &data, &status );

	return status == sesame_success;
}

bool drv::is_loaded ( ) {
	request_t req;

	req.type = request_type_t::query;

	return dispatch_request ( req );
}

bool drv::clean_traces ( ) {
	request_t req;
	
	req.type = request_type_t::clean;

	return dispatch_request ( req );
}

bool drv::spoof_hwid ( ) {
	request_t req;

	req.type = request_type_t::spoof;

	return dispatch_request ( req );
}

void drv::set_target ( uint32_t target ) {
	target_pid = target;
}

bool drv::read ( void* addr, void* buf, uint64_t sz ) {
	request_t req;

	req.type = request_type_t::copy;
	req.pid_from = target_pid;
	req.pid_to = GetCurrentProcessId ( );
	req.addr_from = reinterpret_cast< uint64_t >( addr );
	req.addr_to = reinterpret_cast< uint64_t >( buf );
	req.sz = sz;

	return dispatch_request ( req );
}

bool drv::write ( void* addr, void* buf, uint64_t sz ) {
	request_t req;

	req.type = request_type_t::copy;
	req.pid_from = GetCurrentProcessId ( );
	req.pid_to = target_pid;
	req.addr_from = reinterpret_cast< uint64_t >( buf );
	req.addr_to = reinterpret_cast< uint64_t >( addr );
	req.sz = sz;

	return dispatch_request ( req );
}

bool drv::force_write ( void* addr, void* buf, uint64_t sz ) {
	request_t req;

	req.type = request_type_t::copy_protected;
	req.pid_from = GetCurrentProcessId ( );
	req.pid_to = target_pid;
	req.addr_from = reinterpret_cast< uint64_t >( buf );
	req.addr_to = reinterpret_cast< uint64_t >( addr );
	req.sz = sz;

	return dispatch_request ( req );
}

uint64_t drv::get_base ( ) {
	request_t req;

	req.type = request_type_t::get_base;
	req.pid_from = target_pid;
	req.pid_to = GetCurrentProcessId ( );
	req.addr_to = reinterpret_cast< uint64_t >( &req.addr_to );

	dispatch_request ( req );

	return req.addr_to;
}