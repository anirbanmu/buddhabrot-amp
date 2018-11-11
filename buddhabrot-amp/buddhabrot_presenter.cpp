#include <amp.h>
#include <amp_graphics.h>
#include <amp_math.h>

#include "utilities.h"
#include "buddhabrot_presenter.h"
#include "full_quad_vertex_shader.h"
#include "textured_quad_pixel_shader.h"

CComPtr<ID3D11DeviceContext3> immediate_context(ID3D11Device5* device)
{
    CComPtr<ID3D11DeviceContext3> context;
    device->GetImmediateContext3(&context);
    return context;
}

BuddhabrotPresenter::BuddhabrotPresenter(HWND hwnd, CComPtr<ID3D11Device5> device) :
    intermediate_texture(concurrency::extent<2>(1, 1), 8),
    device(device),
    context(query_interface<ID3D11DeviceContext4>(immediate_context(device))),
    swapchain(BaseSwapChain(hwnd, device))
{
    create_shaders();
    create_pipeline_objects();
    create_backbuffer_render_target();
    set_pipeline();
}

void BuddhabrotPresenter::create_shaders()
{
    throw_hresult_on_failure(device->CreateVertexShader(FULL_QUAD_VERTEX_SHADER_BYTES, sizeof(FULL_QUAD_VERTEX_SHADER_BYTES), nullptr, &vertex_shader));
    throw_hresult_on_failure(device->CreatePixelShader(TEXTURED_QUAD_PIXEL_SHADER_BYTES, sizeof(TEXTURED_QUAD_PIXEL_SHADER_BYTES), nullptr, &pixel_shader));
}

void BuddhabrotPresenter::create_pipeline_objects()
{
    auto sampler_desc = D3D11_SAMPLER_DESC();
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = sampler_desc.AddressV = sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    throw_hresult_on_failure(device->CreateSamplerState(&sampler_desc, &sampler_state));
}

void BuddhabrotPresenter::create_backbuffer_render_target()
{
    throw_hresult_on_failure(device->CreateRenderTargetView(swapchain.backbuffer, nullptr, &render_target_view));
}

void BuddhabrotPresenter::set_pipeline()
{
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vertex_shader, nullptr, 0);
    context->PSSetShader(pixel_shader, nullptr, 0);
    context->PSSetSamplers(0, 1, &sampler_state.p);
    set_viewport();
}

void BuddhabrotPresenter::set_viewport()
{
    const auto swapchain_desc = swapchain.get_desc();

    auto view_port_desc = D3D11_VIEWPORT();
    view_port_desc.Width = float(swapchain_desc.Width);
    view_port_desc.Height = float(swapchain_desc.Height);
    view_port_desc.MinDepth = 0.0f;
    view_port_desc.MaxDepth = 1.0f;
    view_port_desc.TopLeftX = 0.0f;
    view_port_desc.TopLeftY = 0.0f;
    context->RSSetViewports(1, &view_port_desc);
}

void BuddhabrotPresenter::present()
{
    context->OMSetRenderTargets(1, &render_target_view.p, nullptr);
    context->Draw(3, 0);
    context->OMSetRenderTargets(0, nullptr, nullptr);
    swapchain.present();
}

void BuddhabrotPresenter::resize()
{
    render_target_view = nullptr;
    swapchain.resize();
    set_viewport();
    create_backbuffer_render_target();
}

void BuddhabrotPresenter::render_and_present(concurrency::array<unsigned, 2>& r, concurrency::array<unsigned, 2>& g, concurrency::array<unsigned, 2>& b)
{
    if (intermediate_texture.get_extent() != r.get_extent())
    {
        intermediate_texture = concurrency::graphics::texture<concurrency::graphics::unorm_4, 2>(r.get_extent(), 8, r.accelerator_view);
    }

    auto intermediate_view = concurrency::graphics::texture_view<concurrency::graphics::unorm_4, 2>(intermediate_texture);

    const auto red_max = max_element_in_concurrency_array(r);
    const auto green_max = max_element_in_concurrency_array(g);
    const auto blue_max = max_element_in_concurrency_array(b);

    parallel_for_each(intermediate_texture.get_extent(),
        [=, &r, &g, &b](concurrency::index<2> idx) restrict(amp)
        {
            concurrency::graphics::unorm_4 value(
                concurrency::fast_math::sqrt(r[idx] / static_cast<float>(red_max)),
                concurrency::fast_math::sqrt(g[idx] / static_cast<float>(green_max)),
                concurrency::fast_math::sqrt(b[idx] / static_cast<float>(blue_max)),
                1.0);

            intermediate_view.set(idx, value);
        }
    );

    auto d3d_texture = query_interface<ID3D11Texture2D>(CComPtr<IUnknown>(concurrency::graphics::direct3d::get_texture(intermediate_texture)));

    D3D11_TEXTURE2D_DESC d;
    d3d_texture->GetDesc(&d);

    CComPtr<ID3D11ShaderResourceView> srv;
    {
        auto srv_desc = CD3D11_SHADER_RESOURCE_VIEW_DESC(d3d_texture.p, D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM);
        throw_hresult_on_failure(device->CreateShaderResourceView(d3d_texture, &srv_desc, &srv));
    }

    context->PSSetShaderResources(0, 1, &srv.p);

    present();
}