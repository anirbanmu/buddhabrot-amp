#ifndef _BASE_SWAPCHAIN_H_
#define _BASE_SWAPCHAIN_H_

#include <atlbase.h>
#include <d3d11_4.h>
#include <dxgi1_6.h>

struct BaseSwapChain
{
    BaseSwapChain(HWND, ID3D11Device5* device);
    void present();
    void resize();

    DXGI_SWAP_CHAIN_DESC1 get_desc() const
    {
        auto desc = DXGI_SWAP_CHAIN_DESC1();
        swapchain->GetDesc1(&desc);
        return desc;
    }

    CComPtr<IDXGISwapChain4> swapchain;
    CComPtr<ID3D11Texture2D1> backbuffer;
};

#endif