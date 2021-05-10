#pragma once

#include <cstdint>
#include "../driver_interface.hpp"
#include <array>
#include <iostream>
#include <mutex>
#include <numbers>
#include <algorithm>
#include <optional>
#include <string>

#include "../security/security.hpp"

constexpr __forceinline uint32_t hash ( char const* input ) {
	return *input ?
		static_cast< uint32_t >( *input ) + 33 * hash ( input + 1 ) :
		1234;
}

namespace apex {
	namespace offsets {
		inline uintptr_t entity_list = 0;
		inline uintptr_t local_entity = 0;
		inline uintptr_t player_name_list = 0;

		inline ptrdiff_t entity_index = 0;
		inline ptrdiff_t entity_life_state = 0;
		inline ptrdiff_t entity_health = 0;
		inline ptrdiff_t entity_team_num = 0;

		inline ptrdiff_t player_bleedout_state = 0;
		inline ptrdiff_t player_angles = 0;
		inline ptrdiff_t player_dynamic_angles = 0;
		inline ptrdiff_t player_last_primary_weapon = 0;
		inline ptrdiff_t player_bones = 0;
		inline ptrdiff_t player_camera = 0;
		inline ptrdiff_t player_velocity = 0;
		inline ptrdiff_t player_last_visible_time = 0;
		inline ptrdiff_t player_last_crosshair_target_time = 0;

		inline ptrdiff_t weapon_definition_index = 0;
		inline ptrdiff_t weapon_ammo_in_clip = 0;
		inline ptrdiff_t weapon_bullet_speed = 0;
		inline ptrdiff_t weapon_bullet_gravity = 0;

		inline ptrdiff_t glow_context = 0;
		inline ptrdiff_t glow_life_time = 0;
		inline ptrdiff_t glow_distance = 0;
		inline ptrdiff_t glow_type = 0;
		inline ptrdiff_t glow_color = 0;
		inline ptrdiff_t glow_visible_type = 0;
		inline ptrdiff_t glow_fade = 0;

		inline ptrdiff_t global_vars = 0;
		inline ptrdiff_t input_system = 0;
		inline ptrdiff_t game_window = 0;
		inline HWND backup_game_window = nullptr;

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

			while ( ret.x > 180.0f )
				ret.x -= 360.0f;

			while ( ret.x < -180.0f )
				ret.x += 360.0f;

			while ( ret.y > 180.0f )
				ret.y -= 360.0f;

			while ( ret.y < -180.0f )
				ret.y += 360.0f;

			ret.x = std::clamp ( ret.x, -90.0f, 90.0f );
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

	struct player_name_info_t {
		const char* name_with_clantag;
		const char* name;
	};

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

	//Highlight_SetVisibilityType
	enum highlight_visibility_types_t {
		HIGHLIGHT_VIS_NONE = 0 ,
		HIGHLIGHT_VIS_FORCE_ON ,
		HIGHLIGHT_VIS_ALWAYS ,
		HIGHLIGHT_VIS_LOS ,
		HIGHLIGHT_VIS_LOS_ENTSONLYCONTENTSBLOCK ,
		HIGHLIGHT_VIS_OCCLUDED ,
		HIGHLIGHT_VIS_FULL_VIEW
	};

	enum highlight_contexts_t {
		HIGHLIGHT_CONTEXT_NONE = -1,
		HIGHLIGHT_CONTEXT_NEUTRAL = 0,
		HIGHLIGHT_CONTEXT_FRIENDLY = 1,
		HIGHLIGHT_CONTEXT_ENEMY = 2,
		HIGHLIGHT_CONTEXT_OWNED = 3,
		HIGHLIGHT_CONTEXT_PRIVATE_MATCH_OBSERVER = 4,
		HIGHLIGHT_CHARACTER_SPECIAL_HIGHLIGHT = 5,
		HIGHLIGHT_CONTEXT_DEATH_RECAP = 6,
		HIGHLIGHT_CONTEXT_SONAR = 7,
		HIGHLIGHT_CHARACTER_SPECIAL_HIGHLIGHT_2 = 8,
		HIGHLIGHT_CONTEXT_FRIENDLY_REVEALED = 9,
		HIGHLIGHT_CONTEXT_MOVEMENT_REVEALED = 10,
		HIGHLIGHT_MAX_CONTEXTS = 11
	};

	enum highlight_outline_funcs_t {
		HIGHLIGHT_OUTLINE_CUSTOM_COLOR = 101 ,
		HIGHLIGHT_OUTLINE_CUSTOM_COLOR_WEAPON_PICKUP = 110 ,
		HIGHLIGHT_OUTLINE_CUSTOM_COLOR_PULSE = 120 ,
		HIGHLIGHT_OUTLINE_CUSTOM_COLOR_OBEY_Z = 121 ,
		HIGHLIGHT_OUTLINE_CUSTOM_COLOR_OCCLUDED_NOSCANLINES = 129 ,
		HIGHLIGHT_OUTLINE_CUSTOM_COLOR_NOZ_NOSCANLINES = 169 ,
		HIGHLIGHT_OUTLINE_SONAR = 103 ,
		HIGHLIGHT_OUTLINE_INTERACT_BUTTON = 105 ,
		HIGHLIGHT_OUTLINE_OBJECTIVE = 125 ,
		HIGHLIGHT_OUTLINE_VM_CUSTOM_COLOR = 114 ,
		HIGHLIGHT_OUTLINE_LOOT_DEFAULT = 135 ,
		HIGHLIGHT_OUTLINE_LOOT_FOCUSED = 136 ,
		HIGHLIGHT_OUTLINE_MENU_MODEL_REVEAL = 75
	};

	enum highlight_fill_funcs_t {
		HIGHLIGHT_FILL_CUSTOM_COLOR = 101,
		HIGHLIGHT_FILL_INTERACT_BUTTON = 103 ,
		HIGHLIGHT_FILL_VM_CUSTOM_COLOR = 114 ,
		HIGHLIGHT_FILL_NONE = 0 ,
		HIGHLIGHT_FILL_LOBBY_IN_MATCH = 109 ,
		HIGHLIGHT_FILL_SONAR = 103 ,
		HIGHLIGHT_FILL_LOOT_DEFAULT = 135 ,
		HIGHLIGHT_FILL_LOOT_FOCUSED = 136 ,
		HIGHLIGHT_FILL_OBJECTIVE = 126 ,
		HIGHLIGHT_FILL_MENU_MODEL_REVEAL = 75 ,
		HIGHLIGHT_FILL_BLOODHOUND = 12 ,
		HIGHLIGHT_FILL_CAUSTIC_THREAT = 133 ,
		HIGHLIGHT_FILL_CAUSTIC_CANISTER = 134
	};
	/*
	Highlight_SetFunctions/Script_Highlight_SetFunctions
	- args ( contextID, insideSlot, entityVisible, outlineSlot, outlineRadius, state, afterPostProcess)
	xref: "Highlight: inside function slot should be >= 0 and < 256. Actual value is %d.\n"
	*/

	class globals_vars {
	public:
		auto curtime( ) const {
			return drv::read<float>( offsets::global_vars + 4 );
		}
	};

	inline globals_vars globals;

	class weapon {
		uintptr_t m_addr;

	public:
		weapon ( ) {
			m_addr = 0;
		}

		weapon ( uintptr_t addr ) {
			m_addr = addr;
		}

		auto address ( ) {
			return m_addr;
		}

		bool is_weapon ( ) const {
			if ( !m_addr )
				return false;

			return identify_entity( m_addr ) == entity_type_t::weapon;
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

			return drv::read<weapons_t> ( m_addr + offsets::weapon_definition_index );
		}

		float get_bullet_speed ( ) const {
			if ( !m_addr )
				return 0.0f;

			return drv::read<float> ( m_addr + offsets::weapon_bullet_speed );
		}

		float get_bullet_gravity ( ) const {
			if ( !m_addr )
				return 0.0f;

			return drv::read<float> ( m_addr + offsets::weapon_bullet_gravity );
		}
	};

	class player {
		uintptr_t m_addr;

	public:
		player ( ) {
			m_addr = 0;
		}

		player ( uintptr_t addr ) {
			m_addr = addr;
		}

		static player get( int idx ) {
			return player( drv::read<entity_info_t>( offsets::entity_list + sizeof( entity_info_t ) * idx ).ptr );
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

			return !get_life_state ( ) && get_health() > 0;
		}

		int get_index ( ) const {
			if ( !m_addr )
				return 0;

			return drv::read<int> ( m_addr + offsets::entity_index );
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

			return identify_entity(m_addr) == entity_type_t::player;
		}

		bool is_npc ( ) const {
			if ( !m_addr )
				return false;

			return identify_entity ( m_addr ) == entity_type_t::npc;
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

			return weapon ( drv::read<entity_info_t>( offsets::entity_list + sizeof( entity_info_t ) * static_cast< int >( handle & static_cast< uint32_t >( max_entities - 1 ) ) ).ptr );
		}

		std::optional<std::array<matrix3x4, 128>> get_bone_matrix ( ) const {
			if ( !m_addr )
				return std::nullopt;

			const auto bone_ptr = drv::read<uintptr_t> ( m_addr + offsets::player_bones );

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

			return drv::read<vec3> ( m_addr + offsets::player_camera );
		}

		vec3 get_velocity ( ) const {
			if ( !m_addr )
				return { 0.0f, 0.0f, 0.0f };

			return drv::read<vec3> ( m_addr + offsets::player_velocity );
		}

		float get_visible_time ( ) const {
			if ( !m_addr )
				return 0.0f;

			return drv::read<float> ( m_addr + offsets::player_last_visible_time );
		}

		std::string get_player_name ( ) const {
			if ( !m_addr )
				return _ ( "" );

			const auto unknown_player_name = drv::read<player_name_info_t> ( offsets::player_name_list + sizeof ( player_name_info_t ) * 0 );
			const auto player_name = drv::read<player_name_info_t> ( offsets::player_name_list + sizeof ( player_name_info_t ) * get_index ( ) );

			if ( unknown_player_name.name == player_name.name )
				return _ ( "" );

			return drv::read<std::array<char, 128>> ( ( uintptr_t ) player_name.name ).data ( );
		}

		std::string get_player_name_with_clantag ( ) const {
			if ( !m_addr )
				return _ ( "" );

			const auto unknown_player_name = drv::read<player_name_info_t> ( offsets::player_name_list + sizeof ( player_name_info_t ) * 0 );
			const auto player_name = drv::read<player_name_info_t> ( offsets::player_name_list + sizeof ( player_name_info_t ) * get_index ( ) );

			if ( unknown_player_name.name_with_clantag == player_name.name_with_clantag )
				return _ ( "" );

			return drv::read<std::array<char, 128>> ( ( uintptr_t ) player_name.name_with_clantag ).data ( );
		}

		float get_crosshair_time( ) const {
			if ( !m_addr )
				return 0.0f;

			return drv::read<float>( m_addr + offsets::player_last_crosshair_target_time );
		}

		void set_highlight_funcs( int context_id , uint32_t inside_slot , bool ent_visible , uint32_t outline_slot , float outline_radius , uint32_t state , bool after_post_process ) const {
			uint32_t glow_funcs = 0;

			glow_funcs |= ( inside_slot & 0xFF ) << 0;
			glow_funcs |= ( outline_slot & 0xFF ) << 8;
			glow_funcs |= ( static_cast< uint32_t >( ( ( outline_radius * 255.0f ) * 0.125f ) + 0.5f ) & 0xFF ) << 16;
			glow_funcs |= (( ( static_cast< uint32_t >( ent_visible ) << 6 ) | ( state & 0x3F ) | ( static_cast< uint32_t >( after_post_process ) << 7 ) ) & 0xFF) << 24;

			drv::write( m_addr +  context_id * sizeof( context_id ) + 0x2C0 , glow_funcs );
			drv::write( m_addr + 0x3A4 , globals.curtime() );

			if ( context_id == drv::read<int> ( m_addr + 0x3C8 ) ) {
				drv::write<uint64_t>( m_addr + 0x388 , 0 );
				drv::write<uint64_t>( m_addr + 0x390 , 0 );
				drv::write<uint64_t>( m_addr + 0x398 , 0 );
			}
		}
	};

	class local_player {
	public:
		auto get ( ) {
			return player ( drv::read<uintptr_t> ( offsets::local_entity ) );
		}
	};

	inline void enable_input( bool enable ) {
		return drv::write( offsets::game_window, enable ? offsets::backup_game_window : nullptr );
	}

	inline local_player local;

	bool init ( );

	inline void sleep( double t ) {
		timeBeginPeriod( 1 );
		Sleep( static_cast< int >( t * 1000.0 ) );
		timeEndPeriod( 1 );
	}
}