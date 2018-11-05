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

int CALLBACK WinMain(HINSTANCE h_instance, HINSTANCE, LPSTR, int)
{
    vector<accelerator> accelerators = accelerator::get_all();
    auto default_accelerator = find_if(begin(accelerators), end(accelerators), [](const accelerator& a) { return !a.get_is_emulated(); });
    accelerator::set_default(default_accelerator->device_path);

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

    bool resized = false;
    auto window = BasicWindow(1280, 900, L"buddhabrot-amp", h_instance,
        [&resized]()
        {
            resized = true;
        }
    );

    auto presenter = BuddhabrotPresenter(window.handle(), default_accelerator->create_view());

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
            presenter.present();
        }
    }
    
    write_png(image_dimension, image_dimension, red.get_buffer(), green.get_buffer(), blue.get_buffer(), L"image.png");
    write_png_from_array_views(image_dimension, image_dimension, red.get_array_view(), green.get_array_view(), blue.get_array_view(), L"image-amp.png");
    return 0;
}
