#pragma once

#include <thread>
#include <chrono>
#include <windows.h>
#include <intrin.h>
#include <vector>
#include <cstdint>
#include <array>

using namespace std::chrono_literals;

namespace drv {
	constexpr ptrdiff_t text_size = 0xE1E000;

	enum class request_type_t : uint8_t {
		none,
		copy,
		copy_protected,
		get_base,
		clean,
		spoof,
		query
	};

#pragma pack(push, 1)
	struct request_t {
		request_type_t type;
		uint32_t pid_from;
		uint32_t pid_to;
		uint64_t addr_from;
		uint64_t addr_to;
		uint64_t sz;
	};

	struct hook_args_t {
		uint32_t magic;
		request_t* args;
	};
#pragma pack(pop)

	inline HWND window = nullptr;
	inline uint32_t target_pid = 0;

	constexpr uint32_t magic_number = 0x1337;
	constexpr uint64_t sesame_success = 0x5E5A000000000001;
	constexpr uint64_t sesame_error = 0x5E5A000000000000;
	constexpr uint64_t sesame_query = 0x5E5A000000000002;

	bool dispatch_request ( request_t& args );
	bool is_loaded ( );

	/* sets target process */
	void set_target ( uint32_t target );

	/* security functions */
	bool clean_traces ( );
	bool spoof_hwid ( );

	/* memory management functions */
	bool read ( void* addr, void* buf, uint64_t sz );
	bool write ( void* addr, void* buf, uint64_t sz );
	bool force_write ( void* addr, void* buf, uint64_t sz );

	/* target process base address */
	uint64_t get_base ( );

	/* wrappers for memory management functions */
	template <typename type, typename address_type>
	constexpr type read ( address_type addr ) {
		type ret;
		read ( reinterpret_cast< void* >( addr ), reinterpret_cast< void* >( &ret ), sizeof ( ret ) );
		return ret;
	}

	template <typename address_type, typename type>
	constexpr void write ( address_type addr, type val, bool force = false ) {
		if ( force )
			force_write ( reinterpret_cast< void* >( addr ), reinterpret_cast< void* >( &val ), sizeof ( val ) );
		else
			write ( reinterpret_cast< void* >( addr ), reinterpret_cast< void* >( &val ), sizeof ( val ) );
	}

#define in_range( x, a, b ) ( x >= a && x <= b )
#define get_bits( x ) ( in_range( ( x & ( ~0x20 ) ), 'A', 'F' ) ? ( ( x & ( ~0x20 ) ) - 'A' + 0xA ) : ( in_range( x, '0', '9' ) ? x - '0' : 0 ) )
#define get_byte( x ) ( get_bits( x[ 0 ] ) << 4 | get_bits( x[ 1 ] ) )

	class pattern {
		/* cache game memory once for sig scanning (we only need .text section) */
		static inline std::array<uint8_t, text_size> m_game_data;
		static inline uintptr_t m_game_base = 0;

		uintptr_t m_addr;

	public:
		pattern ( uintptr_t addr ) {
			m_addr = addr;
		}

		template < typename t >
		t get ( ) {
			return t ( m_addr );
		}

		pattern sub ( uintptr_t bytes ) {
			return pattern ( m_addr - bytes );
		}

		pattern add ( uintptr_t bytes ) {
			return pattern ( m_addr + bytes );
		}

		pattern deref ( ) {
			return pattern ( read<uintptr_t>( m_addr) );
		}

		pattern deref32 ( ) {
			return pattern ( read<uint32_t> ( m_addr ) );
		}

		pattern resolve_rip ( ) {
			return pattern ( m_addr + read<int>( m_addr ) + sizeof(uint32_t) );
		}

		static pattern search ( const char* pat ) {
			if ( !m_game_base ) {
				m_game_base = get_base ( );
				read ( reinterpret_cast<void*>( m_game_base ), m_game_data.data(), sizeof( m_game_data ) );
			}

			auto pat1 = const_cast< char* >( pat );
			auto range_start = m_game_data.data ( );

			uint8_t* first_match = nullptr;

			for ( uint8_t* current_address = range_start; current_address < range_start + sizeof ( m_game_data ); current_address++ ) {
				if ( !*pat1 )
					return pattern ( m_game_base + reinterpret_cast<uintptr_t>( first_match ) - reinterpret_cast< uintptr_t >( range_start ) );

				if ( *reinterpret_cast< uint8_t* >( pat1 ) == '\?' || *reinterpret_cast< uint8_t* >( current_address ) == get_byte ( pat1 ) ) {
					if ( !first_match )
						first_match = current_address;

					if ( !pat1 [ 2 ] )
						return pattern ( m_game_base + reinterpret_cast< uintptr_t >( first_match ) - reinterpret_cast< uintptr_t >( range_start ) );

					if ( *reinterpret_cast< uint16_t* >( pat1 ) == '\?\?' || *reinterpret_cast< uint8_t* >( pat1 ) != '\?' )
						pat1 += 3;
					else
						pat1 += 2;
				}
				else {
					pat1 = const_cast< char* >( pat );
					first_match = 0;
				}
			}

			return pattern ( 0 );
		}
	};
}