#pragma once

#include <d3d11.h>

namespace overlay::renderer {
	inline ID3D11Device* device;
	inline ID3D11DeviceContext* context;
	inline ID3D11RenderTargetView* target_view;

	bool initialized( );

	void create( ID3D11Device* device , IDXGISwapChain* swap_chain );
	void destroy( );
}