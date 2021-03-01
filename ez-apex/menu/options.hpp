#pragma once
#include <unordered_map>

namespace options {
	enum class option_type_t : int {
		boolean = 0,
		list,
		integer,
		floating_point,
		string,
		color
	};

	class option {
	public:
		struct colorf {
			float r, g, b, a;
		};

		option_type_t type;

		union val {
			bool b;
			int i;
			float f;
			char s [ 128 ];
			bool l [ 128 ];
			colorf c;

			val ( ) {}
			~val ( ) {}
		} val;

		int list_size = 0;

		static void add_list ( const std::string& id, int count );
		static void add_bool ( const std::string& id, bool val );
		static void add_int ( const std::string& id, int val );
		static void add_float ( const std::string& id, float val );
		static void add_str ( const std::string& id, const char* val );
		static void add_color ( const std::string& id, const colorf& val );
	};

	inline std::unordered_map< std::string, option > vars;

	void save ( const std::unordered_map< std::string, option >& options, const std::string& path );
	void load ( std::unordered_map< std::string, option >& options, const std::string& path );
	void init ( );
}