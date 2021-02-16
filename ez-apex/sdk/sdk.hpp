#pragma once

#include <cstdint>
#include "../driver_interface.hpp"
#include <array>
#include <iostream>
#include <mutex>
#include <numbers>
#include <algorithm>
#include <optional>

constexpr uint32_t hash ( char const* input ) {
	return *input ?
		static_cast< uint32_t >( *input ) + 33 * hash ( input + 1 ) :
		1234;
}

namespace apex {
	namespace offsets {
		inline uintptr_t entity_list = 0;
		inline uintptr_t local_entity = 0;

		inline ptrdiff_t entity_life_state = 0;
		inline ptrdiff_t entity_health = 0;
		inline ptrdiff_t entity_team_num = 0;

		inline ptrdiff_t player_bleedout_state = 0;
		inline ptrdiff_t player_angles = 0;
		inline ptrdiff_t player_dynamic_angles = 0;
		inline ptrdiff_t player_last_primary_weapon = 0;

		inline ptrdiff_t weapon_ammo_in_clip = 0;

		inline ptrdiff_t glow_context = 0;
		inline ptrdiff_t glow_life_time = 0;
		inline ptrdiff_t glow_distance = 0;
		inline ptrdiff_t glow_type = 0;
		inline ptrdiff_t glow_color = 0;
		inline ptrdiff_t glow_visible_type = 0;
		inline ptrdiff_t glow_fade = 0;

		bool dump ( );
	};

	template <typename type>
	constexpr type rad_to_deg ( type rad ) {
		return rad * ( 180.0f / std::numbers::pi );
	}

	template <typename type>
	constexpr type deg_to_rad ( type deg ) {
		return deg * ( std::numbers::pi / 180.0f );
	}

	template <typename type>
	constexpr void sin_cos ( type rad, type& sin_out, type& cos_out ) {
		sin_out = sin ( rad );
		cos_out = cos ( rad );
	}

	class vec3 {
	public:
		float x, y, z;

		vec3 ( ) {
			init ( );
		}

		vec3 ( float x, float y, float z = 0.0f ) {
			this->x = x;
			this->y = y;
			this->z = z;
		}

		void init ( ) {
			this->x = this->y = this->z = 0.0f;
		}

		void init ( float x, float y, float z ) {
			this->x = x;
			this->y = y;
			this->z = z;
		}

		bool is_valid ( ) const {
			return std::isfinite ( this->x ) && std::isfinite ( this->y ) && std::isfinite ( this->z );
		}

		bool is_zero ( ) const {
			return vec3 ( this->x, this->y, this->z ) == vec3 ( 0.0f, 0.0f, 0.0f );
		}

		void invalidate ( ) {
			this->x = this->y = this->z = std::numeric_limits< float >::infinity ( );
		}

		void clear ( ) {
			this->x = this->y = this->z = 0.0f;
		}

		float& operator[]( int i ) {
			return ( ( float* ) this ) [ i ];
		}

		float operator[]( int i ) const {
			return ( ( float* ) this ) [ i ];
		}

		void zero ( ) {
			this->x = this->y = this->z = 0.0f;
		}

		bool operator==( const vec3& src ) const {
			return ( src.x == this->x ) && ( src.y == y ) && ( src.z == z );
		}

		bool operator!=( const vec3& src ) const {
			return ( src.x != this->x ) || ( src.y != y ) || ( src.z != z );
		}

		vec3& operator+=( const vec3& v ) {
			this->x += v.x; this->y += v.y; this->z += v.z;

			return *this;
		}

		vec3& operator-=( const vec3& v ) {
			this->x -= v.x; this->y -= v.y; this->z -= v.z;

			return *this;
		}

		vec3& operator*=( float fl ) {
			this->x *= fl;
			this->y *= fl;
			this->z *= fl;

			return *this;
		}

		vec3& operator*=( const vec3& v ) {
			this->x *= v.x;
			this->y *= v.y;
			this->z *= v.z;

			return *this;
		}

		vec3& operator/=( const vec3& v ) {
			this->x /= v.x;
			this->y /= v.y;
			this->z /= v.z;

			return *this;
		}

		vec3& operator+=( float fl ) {
			this->x += fl;
			this->y += fl;
			this->z += fl;

			return *this;
		}

		vec3& operator/=( float fl ) {
			this->x /= fl;
			this->y /= fl;
			this->z /= fl;

			return *this;
		}

		vec3& operator-=( float fl ) {
			this->x -= fl;
			this->y -= fl;
			this->z -= fl;

			return *this;
		}

		void normalize ( ) {
			*this = normalized ( );
		}

		vec3 normalized ( ) const {
			auto res = *this;
			auto l = res.length ( );

			if ( l != 0.0f )
				res /= l;
			else
				res.x = res.y = res.z = 0.0f;

			return res;
		}

		void normalize_place ( ) {
			auto res = *this;
			auto radius = std::sqrtf ( x * x + y * y + z * z );
			auto iradius = 1.0f / ( radius + FLT_EPSILON );

			res.x *= iradius;
			res.y *= iradius;
			res.z *= iradius;
		}

		float dist_to ( const vec3& vec ) const {
			vec3 delta;

			delta.x = this->x - vec.x;
			delta.y = this->y - vec.y;
			delta.z = this->z - vec.z;

			return delta.length ( );
		}

		float dist_to_sqr ( const vec3& vec ) const {
			vec3 delta;

			delta.x = this->x - vec.x;
			delta.y = this->y - vec.y;
			delta.z = this->z - vec.z;

			return delta.length_sqr ( );
		}

		float dot ( const vec3& vec ) const {
			return this->x * vec.x + this->y * vec.y + this->z * vec.z;
		}

		vec3 cross ( const vec3& vec ) const {
			return vec3 ( this->y * vec.z - this->z * vec.y, this->z * vec.x - this->x * vec.z, this->x * vec.y - this->y * vec.x );
		}

		float length ( ) const {
			return std::sqrtf ( this->x * this->x + this->y * this->y + this->z * this->z );
		}

		float length_sqr ( ) const {
			return this->x * this->x + this->y * this->y + this->z * this->z;
		}

		float length_2d_sqr ( ) const {
			return this->x * this->x + this->y * this->y;
		}

		float length_2d ( ) const {
			return std::sqrtf ( this->x * this->x + this->y * this->y );
		}

		vec3& operator=( const vec3& vec ) {
			this->x = vec.x; this->y = vec.y; this->z = vec.z;

			return *this;
		}

		vec3 operator-( ) const {
			return vec3 ( -this->x, -this->y, -this->z );
		}

		vec3 operator+( const vec3& v ) const {
			return vec3 ( this->x + v.x, this->y + v.y, this->z + v.z );
		}

		vec3 operator-( const vec3& v ) const {
			return vec3 ( this->x - v.x, this->y - v.y, this->z - v.z );
		}

		vec3 operator*( float fl ) const {
			return vec3 ( this->x * fl, this->y * fl, this->z * fl );
		}

		vec3 operator*( const vec3& v ) const {
			return vec3 ( this->x * v.x, this->y * v.y, this->z * v.z );
		}

		vec3 operator/( float fl ) const {
			return vec3 ( this->x / fl, this->y / fl, this->z / fl );
		}

		vec3 operator/( const vec3& v ) const {
			return vec3 ( this->x / v.x, this->y / v.y, this->z / v.z );
		}

		/* angle stuff */
		bool valid_angle ( ) const {
			return !isnan ( this->x ) && !isinf ( this->x )
				&& !isnan ( this->y ) && !isinf ( this->y )
				&& this->x
				&& this->y;
		}

		vec3 normalize_angle ( ) const {
			vec3 ret = *this;

			if ( !valid_angle ( ) )
				ret = { 0.0f, 0.0f, 0.0f };

			ret.x = std::clamp ( ret.x, -90.0f, 90.0f );

			while ( ret.y <= -180.0f )
				ret.y += 360.0f;

			while ( ret.y > 180.0f )
				ret.y -= 360.0f;

			ret.y = std::clamp ( ret.y, -180.0f, 180.0f );
			ret.z = 0.0f;

			return ret;
		}

		vec3 to_angle ( ) const {
			vec3 ret;

			if ( this->x == 0.0f && this->y == 0.0f ) {
				ret.x = ( this->z > 0.0f ) ? 270.0f : 90.0f;
				ret.y = 0.0f;
			}
			else {
				ret.x = rad_to_deg ( atan2 ( -this->z, length_2d ( ) ) );
				ret.y = rad_to_deg ( atan2 ( this->y, this->x ) );

				if ( ret.y < 0.0f )
					ret.y += 360.0f;

				if ( ret.x < 0.0f )
					ret.x += 360.0f;
			}

			ret.z = 0.0f;

			return ret.normalize_angle ( );
		}

		vec3 to_vec ( ) const {
			float sp, sy, cp, cy;

			sin_cos ( deg_to_rad ( this->y ), sy, cy );
			sin_cos ( deg_to_rad ( this->x ), sp, cp );

			return vec3 ( cp * cy, cp * sy, -sp ).normalized ( );
		}

		vec3 angle_to ( const vec3& to ) const {
			return ( to - *this ).to_angle ( ).normalize_angle ( );
		}
	};

	inline vec3 operator*( float lhs, const vec3& rhs ) {
		return rhs * lhs;
	}

	inline vec3 operator/( float lhs, const vec3& rhs ) {
		return rhs / lhs;
	}

	class matrix3x4 {
	public:
		float m_values [ 3 ][ 4 ];

		matrix3x4 ( void ) {
			m_values [ 0 ][ 0 ] = 0.0f; m_values [ 0 ][ 1 ] = 0.0f; m_values [ 0 ][ 2 ] = 0.0f; m_values [ 0 ][ 3 ] = 0.0f;
			m_values [ 1 ][ 0 ] = 0.0f; m_values [ 1 ][ 1 ] = 0.0f; m_values [ 1 ][ 2 ] = 0.0f; m_values [ 1 ][ 3 ] = 0.0f;
			m_values [ 2 ][ 0 ] = 0.0f; m_values [ 2 ][ 1 ] = 0.0f; m_values [ 2 ][ 2 ] = 0.0f; m_values [ 2 ][ 3 ] = 0.0f;
		}

		matrix3x4 (
			float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23 ) {
			m_values [ 0 ][ 0 ] = m00; m_values [ 0 ][ 1 ] = m01; m_values [ 0 ][ 2 ] = m02; m_values [ 0 ][ 3 ] = m03;
			m_values [ 1 ][ 0 ] = m10; m_values [ 1 ][ 1 ] = m11; m_values [ 1 ][ 2 ] = m12; m_values [ 1 ][ 3 ] = m13;
			m_values [ 2 ][ 0 ] = m20; m_values [ 2 ][ 1 ] = m21; m_values [ 2 ][ 2 ] = m22; m_values [ 2 ][ 3 ] = m23;
		}

		void init ( const vec3& x, const vec3& y, const vec3& z, const vec3& origin ) {
			m_values [ 0 ][ 0 ] = x.x; m_values [ 0 ][ 1 ] = y.x; m_values [ 0 ][ 2 ] = z.x; m_values [ 0 ][ 3 ] = origin.x;
			m_values [ 1 ][ 0 ] = x.y; m_values [ 1 ][ 1 ] = y.y; m_values [ 1 ][ 2 ] = z.y; m_values [ 1 ][ 3 ] = origin.y;
			m_values [ 2 ][ 0 ] = x.z; m_values [ 2 ][ 1 ] = y.z; m_values [ 2 ][ 2 ] = z.z; m_values [ 2 ][ 3 ] = origin.z;
		}

		matrix3x4 ( const vec3& x, const vec3& y, const vec3& z, const vec3& origin ) {
			init ( x, y, z, origin );
		}

		inline void set_origin ( vec3 const& p ) {
			m_values [ 0 ][ 3 ] = p.x;
			m_values [ 1 ][ 3 ] = p.y;
			m_values [ 2 ][ 3 ] = p.z;
		}

		inline void invalidate ( void ) {
			for ( int i = 0; i < 3; i++ ) {
				for ( int j = 0; j < 4; j++ ) {
					m_values [ i ][ j ] = std::numeric_limits<float>::infinity ( );
				}
			}
		}

		vec3 get_x_axis ( void ) const {
			return at ( 0 );
		}

		vec3 get_y_axis ( void ) const {
			return at ( 1 );
		}

		vec3 get_z_axis ( void ) const {
			return at ( 2 );
		}

		vec3 origin ( void ) const {
			return at ( 3 );
		}

		vec3 at ( int i ) const {
			return vec3 { m_values [ 0 ][ i ], m_values [ 1 ][ i ], m_values [ 2 ][ i ] };
		}

		float* operator[]( int i ) {
			return m_values [ i ];
		}

		const float* operator[]( int i ) const {
			return m_values [ i ];
		}

		float* base ( ) {
			return &m_values [ 0 ][ 0 ];
		}

		const float* base ( ) const {
			return &m_values [ 0 ][ 0 ];
		}

		const bool operator==( matrix3x4 matrix ) const {
			return
				m_values [ 0 ][ 0 ] == matrix [ 0 ][ 0 ] && m_values [ 0 ][ 1 ] == matrix [ 0 ][ 1 ] && m_values [ 0 ][ 2 ] == matrix [ 0 ][ 2 ] && m_values [ 0 ][ 3 ] == matrix [ 0 ][ 3 ] &&
				m_values [ 1 ][ 0 ] == matrix [ 1 ][ 0 ] && m_values [ 1 ][ 1 ] == matrix [ 1 ][ 1 ] && m_values [ 1 ][ 2 ] == matrix [ 1 ][ 2 ] && m_values [ 1 ][ 3 ] == matrix [ 1 ][ 3 ] &&
				m_values [ 2 ][ 0 ] == matrix [ 2 ][ 0 ] && m_values [ 2 ][ 1 ] == matrix [ 2 ][ 1 ] && m_values [ 2 ][ 2 ] == matrix [ 2 ][ 2 ] && m_values [ 2 ][ 3 ] == matrix [ 2 ][ 3 ];
		}
	};

	constexpr int max_entities = 0x10000;
	constexpr int max_players = 128;

	inline uintptr_t base = 0;

	enum class entity_type_t : int {
		none = 0,
		player,
		npc,
		prop,
		weapon,
		world
	};

	entity_type_t identify_entity ( uintptr_t ent );

	struct entity_info_t {
		uint64_t ptr;
		int64_t serial_number;
		uint64_t prev;
		uint64_t next;
	};

	inline std::array<entity_info_t, max_entities> entity_list;
	inline std::array<entity_type_t, max_entities> entity_types;

	enum class weapons_t : uint32_t {
		invalid = -1,
		r301 = 0,
		sentinel = 1,
		melee_survival = 17,
		alternator = 59,
		re45,
		devotion,
		longbow,
		eva8_auto,
		flatline,
		g7_scout,
		hemlok,
		kraber,
		mastiff,
		mozambique,
		prowler,
		peacekeeper,
		r99,
		p2020,
		spitfire,
		triple_take,
		wingman,
		havoc,
		lstar,
		charge_rifle,
		volt
	};

	class weapon {
		entity_type_t m_ent_type;
		uintptr_t m_addr;

	public:
		weapon ( ) {
			m_addr = 0;
			m_ent_type = entity_type_t::none;
		}

		weapon ( uintptr_t addr ) {
			m_addr = addr;
			m_ent_type = entity_type_t::weapon;
		}

		auto address ( ) {
			return m_addr;
		}

		bool is_weapon ( ) const {
			if ( !m_addr )
				return false;

			return m_ent_type == entity_type_t::weapon;
		}

		bool is_gun ( ) const {
			if ( !m_addr )
				return false;

			const auto definition_index = get_definition_index ( );

			return ( definition_index >= weapons_t::r301 && definition_index <= weapons_t::sentinel )
				|| ( definition_index >= weapons_t::alternator && definition_index <= weapons_t::volt );
		}

		bool is_melee ( ) const {
			if ( !m_addr )
				return false;

			return get_definition_index ( ) == weapons_t::melee_survival;
		}

		bool is_valid ( ) const {
			if ( !m_addr )
				return false;

			const auto definition_index = get_definition_index ( );

			return m_addr && is_weapon ( ) && ( is_gun ( ) || is_melee ( ) );
		}

		int get_ammo ( ) const {
			if ( !m_addr )
				return 0;

			return drv::read<int> ( m_addr + offsets::weapon_ammo_in_clip );
		}

		weapons_t get_definition_index ( ) const {
			if ( !m_addr )
				return weapons_t::invalid;

			return drv::read<weapons_t> ( m_addr + offsets::weapon_ammo_in_clip );
		}

		float get_bullet_speed ( ) const {
			if ( !m_addr )
				return 0.0f;

			return drv::read<float> ( m_addr + 0x1e1c );
		}

		float get_bullet_gravity ( ) const {
			if ( !m_addr )
				return 0.0f;

			return drv::read<float> ( m_addr + 0x1e24 /* get_bullet_speed + 8 */ );
		}
	};

	class player {
		entity_type_t m_ent_type;
		uintptr_t m_addr;

	public:
		player ( ) {
			m_addr = 0;
			m_ent_type = entity_type_t::none;
		}

		player ( uintptr_t addr ) {
			m_addr = addr;
			m_ent_type = entity_type_t::player;
		}

		auto address ( ) const {
			return m_addr;
		}

		uint8_t get_life_state ( ) const {
			if ( !m_addr )
				return 0xFF;

			return drv::read<uint8_t> ( m_addr + offsets::entity_life_state );
		}

		bool is_alive ( ) const {
			if ( !m_addr )
				return false;

			return !get_life_state ( );
		}

		int get_health ( ) const {
			if ( !m_addr )
				return 0;

			return drv::read<int> ( m_addr + offsets::entity_health );
		}

		bool get_bleedout_state ( ) const {
			if ( !m_addr )
				return 0;

			return drv::read<bool> ( m_addr + offsets::player_bleedout_state );
		}

		int get_team ( ) const {
			if ( !m_addr )
				return 0xFFFFFFFF;

			return drv::read<int> ( m_addr + offsets::entity_team_num );
		}

		bool is_player ( ) const {
			if ( !m_addr )
				return false;

			return m_ent_type == entity_type_t::player;
		}

		bool is_valid ( ) const {
			if ( !m_addr )
				return false;

			return m_addr && is_player ( ) && is_alive ( );
		}

		bool is_downed ( ) const {
			if ( !m_addr )
				return false;

			return get_bleedout_state ( );
		}

		vec3 get_angles ( ) const {
			if ( !m_addr )
				return { 0.0f, 0.0f, 0.0f };

			return drv::read<vec3> ( m_addr + offsets::player_angles );
		}

		void set_angles ( const vec3& ang ) const {
			if ( !m_addr )
				return;

			drv::write ( m_addr + offsets::player_angles, ang );
		}

		vec3 get_dynamic_angles ( ) const {
			if ( !m_addr )
				return { 0.0f, 0.0f, 0.0f };

			return drv::read<vec3> ( m_addr + offsets::player_dynamic_angles );
		}

		void set_dynamic_angles ( const vec3& ang ) const {
			if ( !m_addr )
				return;

			drv::write ( m_addr + offsets::player_dynamic_angles, ang );
		}

		weapon get_weapon ( ) const {
			if ( !m_addr )
				return weapon ( );

			const auto handle = drv::read<uint32_t> ( m_addr + offsets::player_last_primary_weapon );

			if ( !handle || handle == 0xFFFFFFFF )
				return weapon ( );

			return weapon ( entity_list[ static_cast< int >( handle & static_cast< uint32_t >( max_entities - 1 ) ) ].ptr );
		}

		std::optional<std::array<matrix3x4, 128>> get_bone_matrix ( ) const {
			if ( !m_addr )
				return std::nullopt;

			const auto bone_ptr = drv::read<uintptr_t> ( m_addr + 0xF18 );

			if ( !bone_ptr )
				return std::nullopt;

			return drv::read<std::array<matrix3x4, 128>> ( bone_ptr );
		}

		vec3 get_origin ( ) const {
			if ( !m_addr )
				return { 0.0f, 0.0f, 0.0f };

			return drv::read<vec3> ( m_addr + 0x14C );
		}

		vec3 get_camera_pos ( ) const {
			if ( !m_addr )
				return { 0.0f, 0.0f, 0.0f };

			return drv::read<vec3> ( m_addr + 0x1E6C );
		}

		vec3 get_velocity ( ) const {
			if ( !m_addr )
				return { 0.0f, 0.0f, 0.0f };

			return drv::read<vec3> ( m_addr + 0x14C - 12 /* get_origin - 12 */ );
		}

		float get_visible_time ( ) const {
			if ( !m_addr )
				return 0.0f;

			return drv::read<float> ( m_addr + 0x1A6C );
		}
	};

	class local_player {
	public:
		auto get ( ) {
			return player ( drv::read<uintptr_t> ( offsets::local_entity ) );
		}
	};

	inline local_player local;

	void update ( );
	bool init ( );
}