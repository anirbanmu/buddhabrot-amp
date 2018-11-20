#include <iostream>
#include <sstream>
#include <algorithm>
#include <random>
#include <limits>

#define NOMINMAX
#include <windows.h>
#include <amp.h>
#include <amp_graphics.h>
#include <d3d11_4.h>

#include "args-6.2.0/args.hxx"

#include "utilities.h"
#include "basic_window.h"
#include "buddhabrot_presenter.h"
#include "buddhabrot_generator.h"

using namespace std;
using concurrency::accelerator;

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

struct CommandLineArguments
{
    void parse(int argc, const char * const * argv)
    {
        parser.ParseCLI(argc, argv);
        if (dimension_flag) dimension = args::get(dimension_flag);
        if (points_flag) points_per_iteration = args::get(points_flag);
        if (filename_flag)
        {
            wstringstream ss;
            ss << args::get(filename_flag).c_str();
            filename = ss.str();
        }
    }

    unsigned dimension{ 4096 };
    unsigned points_per_iteration{ 2048 * 2048 };
    wstring filename{ L"buddhabrot-amp.png" };
    args::ArgumentParser parser{ "Usage: buddhabrot-amp.exe {OPTIONS}...", "Source & help at: <https://github.com/anirbanmu/buddhabrot-amp>" };
    args::HelpFlag help{ parser, "help", "Display this help menu", { 'h', "help" } };
    args::ValueFlag<unsigned> dimension_flag{ parser, "dimension", "Dimension in pixels of the buddhabrot generated", { 'd', "dimension" } };
    args::ValueFlag<unsigned> points_flag{ parser, "points", "Number of points iterated on each frame", { 'p', "points" } };
    args::ValueFlag<string> filename_flag{ parser, "filename", "Path of output PNG file", { 'f', "file" } };
};

class ConsoleAttacher
{
    public:
        ConsoleAttacher()
        {
            if (AttachConsole(ATTACH_PARENT_PROCESS))
            {
                freopen_s(&new_stdout, "CONOUT$", "w", stdout);
                freopen_s(&new_stderr, "CONOUT$", "w", stderr);
            }
        }

        ~ConsoleAttacher()
        {
            if (new_stdout != nullptr) fclose(new_stdout);
            if (new_stderr != nullptr) fclose(new_stderr);
            FreeConsole();
        }

    private:
        FILE* new_stdout{ nullptr };
        FILE* new_stderr{ nullptr };
};

int CALLBACK WinMain(HINSTANCE h_instance, HINSTANCE, LPSTR, int)
{
    ConsoleAttacher attached_console;
    CommandLineArguments cli;
    try
    {
        cli.parse(__argc, __argv);
    }
    catch (const args::Help&)
    {
        cout << cli.parser;
        return 0;
    }
    catch (const args::ParseError& e)
    {
        cerr << e.what() << endl;
        cerr << cli.parser;
        return 1;
    }

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

    std::array<std::tuple<unsigned, unsigned>, 3> iteration_ranges{ make_tuple(0, 1024), make_tuple(1024, 2048), make_tuple(2048, 4096) };
    auto generator = BuddhabrotGenerator(accelerator_view, concurrency::extent<2>(cli.dimension, cli.dimension), cli.points_per_iteration, iteration_ranges);
    // auto green_generator = BuddhabrotGenerator(accelerator_view, concurrency::extent<2>(cli.dimension, cli.dimension), cli.points_per_iteration, make_tuple(10240, 20000));
    // auto blue_generator = BuddhabrotGenerator(accelerator_view, concurrency::extent<2>(cli.dimension, cli.dimension), cli.points_per_iteration, make_tuple(20000, 30000));

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
            generator.iterate();
            presenter.render_and_present(generator.get_record_array<0>(), generator.get_record_array<1>(), generator.get_record_array<2>());
        }
    }
    
    write_png_from_arrays(cli.dimension, cli.dimension, generator.get_record_array<0>(), generator.get_record_array<1>(), generator.get_record_array<2>(), cli.filename);
    return 0;
}
