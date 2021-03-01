#include "renderer.hpp"

namespace overlay::renderer {
    bool initialized() {
        return device != nullptr;
    }

    void create(ID3D11Device* device, IDXGISwapChain* swap_chain) {
        renderer::device = device;

        device->GetImmediateContext(&context);

        ID3D11Texture2D* pBackBuffer;
        swap_chain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

        if (pBackBuffer == nullptr)
            return;

        device->CreateRenderTargetView(pBackBuffer, nullptr, &target_view);

        pBackBuffer->Release();
    }

    void destroy() {
        target_view->Release();

        device = nullptr;
        context = nullptr;

        target_view = nullptr;
    }
}
