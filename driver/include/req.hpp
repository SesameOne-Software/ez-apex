#pragma once

#include "util.hpp"

namespace req {
	enum class request_type_t : u8 {
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
		u32 pid_from;
		u32 pid_to;
		u64 addr_from;
		u64 addr_to;
		u64 sz;
	};
#pragma pack(pop)
}