#include <windows.h>

#include "utilities.h"
#include "basic_window.h"

using namespace std;

BasicWindow::BasicWindow(unsigned width, unsigned height, wstring title, HINSTANCE h_instance, function<void()> resized_callback) :
    resized_callback(resized_callback),
    hwindow(create_window(width, height, title, h_instance, this))
{
}

HWND BasicWindow::create_window(unsigned width, unsigned height, wstring title, HINSTANCE h_instance, BasicWindow* instance_pointer)
{
    auto window_class = WNDCLASSEX{ sizeof(WNDCLASSEX) };
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = static_window_proc;
    window_class.hInstance = h_instance;

    auto window_class_name = title + L"-class";
    window_class.lpszClassName = window_class_name.c_str();
    throw_hresult_on_failure(!RegisterClassEx(&window_class) ? E_UNEXPECTED : S_OK);

    auto hwnd = CreateWindowEx(0, window_class.lpszClassName, title.c_str(), WS_OVERLAPPEDWINDOW, 0, 0, width, height, NULL, NULL, h_instance, instance_pointer);
    throw_hresult_on_failure(!hwnd ? E_UNEXPECTED : S_OK);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return hwnd;
}

LRESULT CALLBACK BasicWindow::window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_SIZE:
        {
            resized_callback();
            return 0;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK BasicWindow::static_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    BasicWindow* p = nullptr;
    switch (msg)
    {
        case WM_NCCREATE:
        {
            LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lparam);
            p = static_cast<BasicWindow*>(lpcs->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(p));
            break;
        }
        case WM_DESTROY:
        {
            auto previous_long_ptr = SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            p = previous_long_ptr == 0 ? nullptr : reinterpret_cast<BasicWindow*>(previous_long_ptr);
            break;
        }
        default:
        {
            p = reinterpret_cast<BasicWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }
    }

    if (p != nullptr)
    {
        return p->window_proc(hwnd, msg, wparam, lparam);
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}
