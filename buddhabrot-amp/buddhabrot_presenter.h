#ifndef _BUDDHABROT_PRESENTER_H_
#define _BUDDHABROT_PRESENTER_H_

#include <amp.h>

#include "base_swapchain.h"

class BuddhabrotPresenter
{
    public:
        BuddhabrotPresenter(HWND, CComPtr<ID3D11Device5>);
        void resize();
        void render_and_present(const concurrency::array<unsigned, 2>& r, const concurrency::array<unsigned, 2>& g, const concurrency::array<unsigned, 2>& b);

    private:
        void present();
        void create_shaders();
        void create_pipeline_objects();
        void create_backbuffer_render_target();
        void set_pipeline();
        void set_viewport();

        CComPtr<ID3D11Device5> device;
        CComPtr<ID3D11DeviceContext4> context;
        BaseSwapChain swapchain;

        // pipeline state
        CComPtr<ID3D11VertexShader> vertex_shader;
        CComPtr<ID3D11PixelShader> pixel_shader;
        CComPtr<ID3D11SamplerState> sampler_state;
        CComPtr<ID3D11RenderTargetView> render_target_view;

        concurrency::graphics::texture<concurrency::graphics::unorm_4, 2> intermediate_texture;
};

#endif