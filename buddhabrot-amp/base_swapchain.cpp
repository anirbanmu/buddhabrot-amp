#include "utilities.h"
#include "base_swapchain.h"

CComPtr<IDXGISwapChain4> create_swap_chain(HWND window, ID3D11Device5* device)
{
    CComPtr<IDXGIFactory6> factory;

    {
        CComPtr<IDXGIAdapter> adapter;
        throw_hresult_on_failure(query_interface<IDXGIDevice>(device)->GetAdapter(&adapter));
        throw_hresult_on_failure(adapter->GetParent(IID_PPV_ARGS(&factory)));
    }

    CComPtr<IDXGISwapChain1> swapchain;

    {
        auto desc = DXGI_SWAP_CHAIN_DESC1();
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferCount = 2;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        throw_hresult_on_failure(factory->CreateSwapChainForHwnd(device, window, &desc, nullptr, nullptr, &swapchain));
    }

    return query_interface<IDXGISwapChain4>(swapchain);
}

template<typename TIBackbuffer, typename TISwapChain> CComPtr<TIBackbuffer> get_backbuffer(TISwapChain* swapchain)
{
    CComPtr<TIBackbuffer> buffer;
    throw_hresult_on_failure(swapchain->GetBuffer(0, IID_PPV_ARGS(&buffer)));
    return buffer;
}

BaseSwapChain::BaseSwapChain(HWND window, ID3D11Device5* device) :
    swapchain(create_swap_chain(window, device)),
    backbuffer(get_backbuffer<ID3D11Texture2D1>(swapchain.p))
{
}

void BaseSwapChain::resize()
{
    backbuffer = nullptr;
    throw_hresult_on_failure(swapchain->ResizeBuffers(2, 0, 0, DXGI_FORMAT(0), 0));
    backbuffer = get_backbuffer<ID3D11Texture2D1>(swapchain.p);
}

void BaseSwapChain::present()
{
    swapchain->Present(1, 0);
}