#include <algorithm>
#include <vector>
#include <string>

#include <atlbase.h>
#include <wincodec.h>
#include <wincodecsdk.h>

#include <amp.h>
#include <amp_math.h>

#include "utilities.h"

using namespace std;
using concurrency::array_view;
using concurrency::index;

struct PngWriterResources
{
    PngWriterResources(const wstring filename)
    {
        throw_hresult_on_failure(CoInitializeEx(NULL, COINIT_MULTITHREADED));
        throw_hresult_on_failure(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&this->factory)));
        throw_hresult_on_failure(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &this->encoder));
        throw_hresult_on_failure(factory->CreateStream(&this->stream));
        throw_hresult_on_failure(stream->InitializeFromFilename(filename.c_str(), GENERIC_WRITE));
        throw_hresult_on_failure(encoder->Initialize(stream, WICBitmapEncoderNoCache));
    }

    CComPtr<IWICImagingFactory> factory;
    CComPtr<IWICBitmapEncoder> encoder;
    CComPtr<IWICStream> stream;
};

void write_png_from_arrays(UINT width, UINT height, const concurrency::array<unsigned, 2>& red, const concurrency::array<unsigned, 2>& green, const concurrency::array<unsigned, 2>& blue, const wstring filename)
{
    auto resources = PngWriterResources(filename);

    CComPtr<IWICBitmapFrameEncode> frame;
    throw_hresult_on_failure(resources.encoder->CreateNewFrame(&frame, nullptr));
    throw_hresult_on_failure(frame->Initialize(nullptr));
    throw_hresult_on_failure(frame->SetSize(width, height));

    GUID pixel_format = GUID_WICPixelFormat32bppBGRA;
    throw_hresult_on_failure(frame->SetPixelFormat(&pixel_format));

    const auto max_red_array = max_element_in_concurrency_array(red);
    const auto max_green_array = max_element_in_concurrency_array(green);
    const auto max_blue_array = max_element_in_concurrency_array(blue);

    // sqrt cheats to pull up lows comparatively to highs
    auto buffer = vector<BYTE>(width * height * 4);
    {
        array_view<unsigned, 2> buffer_view(height, width, reinterpret_cast<unsigned*>(buffer.data()));

        parallel_for_each(buffer_view.extent,
            [&, buffer_view](index<2> idx) restrict(amp)
        {
            buffer_view[idx] = 255 << 24 |
                static_cast<unsigned>(255 * concurrency::fast_math::sqrt(red[idx] / static_cast<float>(max_red_array[0]))) << 16 |
                static_cast<unsigned>(255 * concurrency::fast_math::sqrt(green[idx] / static_cast<float>(max_green_array[0]))) << 8 |
                static_cast<unsigned>(255 * concurrency::fast_math::sqrt(blue[idx] / static_cast<float>(max_blue_array[0])));
        }
        );
    }

    throw_hresult_on_failure(frame->WritePixels(height, width * 4, width * height * 4, buffer.data()));
    throw_hresult_on_failure(frame->Commit());
    throw_hresult_on_failure(resources.encoder->Commit());
}

void write_png_from_array_views(UINT width, UINT height, const array_view<unsigned, 2>& red, const array_view<unsigned, 2>& green, const array_view<unsigned, 2>& blue, const wstring filename)
{
    auto resources = PngWriterResources(filename);

    CComPtr<IWICBitmapFrameEncode> frame;
    throw_hresult_on_failure(resources.encoder->CreateNewFrame(&frame, nullptr));
    throw_hresult_on_failure(frame->Initialize(nullptr));
    throw_hresult_on_failure(frame->SetSize(width, height));

    GUID pixel_format = GUID_WICPixelFormat32bppBGRA;
    throw_hresult_on_failure(frame->SetPixelFormat(&pixel_format));

    const auto max_red = max_element_in_array_view(red);
    const auto max_green = max_element_in_array_view(green);
    const auto max_blue = max_element_in_array_view(blue);

    // cout << max_red << ":" << max_green << ":" << max_blue << endl;

    // sqrt cheats to pull up lows comparatively to highs
    auto buffer = vector<BYTE>(width * height * 4);
    {
        array_view<unsigned, 2> bufferView(height, width, reinterpret_cast<unsigned*>(buffer.data()));

        parallel_for_each(bufferView.extent,
            [=](index<2> idx) restrict(amp)
            {
                bufferView[idx] = 255 << 24 |
                    static_cast<unsigned>(255 * concurrency::fast_math::sqrt(red[idx] / static_cast<float>(max_red))) << 16 |
                    static_cast<unsigned>(255 * concurrency::fast_math::sqrt(green[idx] / static_cast<float>(max_green))) << 8 |
                    static_cast<unsigned>(255 * concurrency::fast_math::sqrt(blue[idx] / static_cast<float>(max_blue)));
            }
        );
    }

    throw_hresult_on_failure(frame->WritePixels(height, width * 4, width * height * 4, buffer.data()));
    throw_hresult_on_failure(frame->Commit());
    throw_hresult_on_failure(resources.encoder->Commit());
}

void write_png(UINT width, UINT height, vector<unsigned>& red, vector<unsigned>& green, vector<unsigned>& blue, const wstring filename)
{
    auto resources = PngWriterResources(filename);

    CComPtr<IWICBitmapFrameEncode> frame;
    throw_hresult_on_failure(resources.encoder->CreateNewFrame(&frame, nullptr));
    throw_hresult_on_failure(frame->Initialize(nullptr));
    throw_hresult_on_failure(frame->SetSize(width, height));

    GUID pixel_format = GUID_WICPixelFormat32bppBGRA;
    throw_hresult_on_failure(frame->SetPixelFormat(&pixel_format));

    const unsigned max_red = *max_element(begin(red), end(red));
    const unsigned max_green = *max_element(begin(green), end(green));
    const unsigned max_blue = *max_element(begin(blue), end(blue));

    // cout << max_red << ":" << max_green << ":" << max_blue << endl;

    auto buffer = vector<BYTE>(width * height * 4);
    {
        array_view<unsigned, 2> bufferView(height, width, reinterpret_cast<unsigned*>(buffer.data()));
        array_view<unsigned, 1> red_view(height * width, red);
        array_view<unsigned, 1> green_view(height * width, green);
        array_view<unsigned, 1> blue_view(height * width, blue);

        parallel_for_each(bufferView.extent,
            [=](index<2> idx) restrict(amp)
            {
                bufferView[idx] = 255 << 24 |
                    static_cast<unsigned>(255 * concurrency::fast_math::sqrt(red_view[idx[1] + (width * idx[0])] / static_cast<float>(max_red))) << 16 |
                    static_cast<unsigned>(255 * concurrency::fast_math::sqrt(green_view[idx[1] + (width * idx[0])] / static_cast<float>(max_green))) << 8 |
                    static_cast<unsigned>(255 * concurrency::fast_math::sqrt(blue_view[idx[1] + (width * idx[0])] / static_cast<float>(max_blue)));
            }
        );

        bufferView.synchronize();
    }

    throw_hresult_on_failure(frame->WritePixels(height, width * 4, width * height * 4, buffer.data()));
    throw_hresult_on_failure(frame->Commit());
    throw_hresult_on_failure(resources.encoder->Commit());
}
