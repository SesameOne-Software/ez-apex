#pragma once

#include <time.h>
#include "fn.hpp"

#include "vmp.hpp"

#define log //DbgPrint

#define offsetof(s,m) ((size_t)&(((s*)0)->m))

namespace util {
	u64 get_kerneladdr ( const char* name, u64& size );

	namespace sec {
		void restore_thread ( );
		bool clear_piddbcache ( );
		bool clear_mmunloadeddrivers ( );
		bool clear_kernelhashbucketlist ( );
		bool hide_thread ( );
		bool clear_big_pool_table ( );
	}

	void delay ( );
	bool data_cmp( const u8* data, const u8* sig, const char* mask );

	template <typename t = u64>
	t find_pattern ( u64 start, u64 length, const char* pattern, const char* mask );

	template <typename t = void*>
	inline t resolve_rip ( u64 addr, u32 offset ) {
		return reinterpret_cast< t >( addr + *( i32* ) ( addr + offset ) + sizeof ( u32 ) + offset );
	}

	template <typename t>
	inline bool valid_ptr ( t addr ) {
		return u64 ( addr ) && u64 ( addr ) > 0x1000 && MmIsAddressValid ( ( void* ) addr );
	}
	
	void free_processes ( );
	PEPROCESS get_process ( u64 pid );

	inline u32 rand32 ( ) {
		static u32 seed = u32 ( time ( nullptr ) );
		seed *= 48271u;
		return seed;
	}

	inline int rand_serial ( char* out ) {
		VMP_BEGINMUTATION ( );
		static const char letters_caps [ ] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		static const char letters_min [ ] = "abcdefghijklmnopqrstuvwxyz";
		static const char numbers [ ] = "1234567890";

		const auto len = strlen ( out );

		for ( int i = 0; i < len; i++ ) {
			const auto old_val = out [ i ];

			if ( old_val >= 'A' && old_val <= 'Z' )
				out [ i ] = letters_caps [ util::rand32 ( ) % ( sizeof ( letters_caps ) - 1 ) ];
			else if ( old_val >= 'a' && old_val <= 'z' )
				out [ i ] = letters_min [ util::rand32 ( ) % ( sizeof ( letters_min ) - 1 ) ];
			else if ( old_val >= '0' && old_val <= '9' )
				out [ i ] = numbers [ util::rand32 ( ) % ( sizeof ( numbers ) - 1 ) ];
		}

		return len;
		VMP_END ( );
	}
}