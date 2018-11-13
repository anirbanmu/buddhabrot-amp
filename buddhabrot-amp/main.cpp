#include <iostream>
#include <sstream>
#include <algorithm>
#include <random>
#include <limits>

#include <windows.h>
#include <amp.h>
#include <amp_graphics.h>
#include <d3d11_4.h>

#include "utilities.h"
#include "basic_window.h"
#include "buddhabrot_presenter.h"
#include "buddhabrot_generator.h"

using namespace std;
using concurrency::accelerator;

const unsigned image_dimension = 4096;
const unsigned image_size = image_dimension * image_dimension;

namespace
{
#if defined(_DEBUG)
    const auto CREATE_DEVICE_FLAGS = D3D11_CREATE_DEVICE_DEBUG;
#else
    const auto CREATE_DEVICE_FLAGS = D3D11_CREATE_DEVICE_FLAG();
#endif
}

CComPtr<ID3D11Device5> create_device()
{
    CComPtr<ID3D11Device> device;
    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, CREATE_DEVICE_FLAGS, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, nullptr);
    return query_interface<ID3D11Device5>(device);
}

int CALLBACK WinMain(HINSTANCE h_instance, HINSTANCE, LPSTR, int)
{
    auto d3d_device = create_device();
    auto accelerator_view = concurrency::direct3d::create_accelerator_view(d3d_device);

    bool resized = false;
    auto window = BasicWindow(800, 800, L"buddhabrot-amp", h_instance,
        [&resized]()
        {
            resized = true;
        }
    );

    auto presenter = BuddhabrotPresenter(window.handle(), d3d_device);

    auto red_generator = BuddhabrotGenerator(accelerator_view, concurrency::extent<2>(image_dimension, image_dimension), 512 * 512, 1024);
    auto green_generator = BuddhabrotGenerator(accelerator_view, concurrency::extent<2>(image_dimension, image_dimension), 512 * 512, 2048);
    auto blue_generator = BuddhabrotGenerator(accelerator_view, concurrency::extent<2>(image_dimension, image_dimension), 512 * 512, 4096);

    auto msg = MSG();
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            if (resized) {
                resized = false;
                // wstringstream s;
                // s << "resized" << endl;
                // OutputDebugString(s.str().c_str());
                presenter.resize();
            }
            presenter.render_and_present(red_generator.iterate(), green_generator.iterate(), blue_generator.iterate());
        }
    }
    
    write_png_from_arrays(image_dimension, image_dimension, red_generator.get_record_array(), green_generator.get_record_array(), blue_generator.get_record_array(), L"image-amp.png");
    return 0;
}
