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
    vector<accelerator> accelerators = accelerator::get_all();
    auto default_accelerator = find_if(begin(accelerators), end(accelerators), [](const accelerator& a) { return !a.get_is_emulated(); });
    accelerator::set_default(default_accelerator->device_path);

    auto d3d_device = create_device();
    auto accelerator_view = concurrency::direct3d::create_accelerator_view(d3d_device);

    AmpArray<unsigned, 2> red(image_dimension, image_dimension);
    AmpArray<unsigned, 2> green(image_dimension, image_dimension);
    AmpArray<unsigned, 2> blue(image_dimension, image_dimension);

    auto blue_view = blue.get_array_view();
    auto red_view = red.get_array_view();
    parallel_for_each(blue_view.get_extent(),
        [=](concurrency::index<2> idx) restrict(amp)
        {
            red_view[idx] = idx[0] ^ idx[1];
            blue_view[idx] = idx[0] + idx[1];
        }
    );

    red_view.synchronize();
    blue_view.synchronize();

    auto red_array = concurrency::array<unsigned, 2>(image_dimension, image_dimension, red.get_buffer().begin(), red.get_buffer().end(), accelerator_view);
    auto blue_array = concurrency::array<unsigned, 2>(image_dimension, image_dimension, blue.get_buffer().begin(), blue.get_buffer().end(), accelerator_view);
    auto green_array = concurrency::array<unsigned, 2>(image_dimension, image_dimension, green.get_buffer().begin(), green.get_buffer().end(), accelerator_view);

    bool resized = false;
    auto window = BasicWindow(1280, 900, L"buddhabrot-amp", h_instance,
        [&resized]()
        {
            resized = true;
        }
    );

    auto presenter = BuddhabrotPresenter(window.handle(), d3d_device);

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
                wstringstream s;
                s << "resized" << endl;
                OutputDebugString(s.str().c_str());
                presenter.resize();
            }
            presenter.render_and_present(red_array, green_array, blue_array);
        }
    }
    
    write_png(image_dimension, image_dimension, red.get_buffer(), green.get_buffer(), blue.get_buffer(), L"image.png");
    write_png_from_array_views(image_dimension, image_dimension, red.get_array_view(), green.get_array_view(), blue.get_array_view(), L"image-amp.png");
    return 0;
}
