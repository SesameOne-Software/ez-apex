#pragma once

namespace menu {
	inline bool opened = false;
	inline int cur_tab_idx = 0;

	void load_cfg_list( );
	void draw( );
}