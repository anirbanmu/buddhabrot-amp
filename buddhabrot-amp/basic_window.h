#ifndef _BASIC_WINDOW_H_
#define _BASIC_WINDOW_H_

#include <functional>

#include <windows.h>

class BasicWindow
{
    public:
        BasicWindow(unsigned width, unsigned height, std::wstring title, HINSTANCE h_instance, std::function<void()> resized_callback);
        HWND handle()
        {
            return hwindow;
        }

    private:
        LRESULT CALLBACK window_proc(HWND, UINT, WPARAM, LPARAM);
        static LRESULT CALLBACK static_window_proc(HWND, UINT, WPARAM, LPARAM);
        static HWND create_window(unsigned width, unsigned height, std::wstring title, HINSTANCE h_instance, BasicWindow*);
        std::function<void()> resized_callback;
        HWND hwindow;
};

#endif