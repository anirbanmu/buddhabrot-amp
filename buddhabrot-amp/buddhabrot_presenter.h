#ifndef _BUDDHABROT_PRESENTER_H_
#define _BUDDHABROT_PRESENTER_H_

#include <amp.h>

#include "base_swapchain.h"

class BuddhabrotPresenter
{
    public:
        BuddhabrotPresenter(HWND, const concurrency::accelerator_view&);
        void present();
        void resize();

    private:
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
};

#endif