#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#include <string>
#include <vector>
#include <chrono>

#include <amp.h>
#include <atlbase.h>

template<typename T, size_t dimension> class AmpArray
{
public:
    AmpArray(size_t x) :
        buffer(x, 0),
        view(concurrency::array_view<T, dimension>(int(x), this->buffer))
    {
    }

    AmpArray(size_t y, size_t x) :
        buffer(y * x, 0),
        view(concurrency::array_view<T, dimension>(int(y), int(x), this->buffer))
    {
    }

    std::vector<T>& get_buffer()
    {
        return this->buffer;
    }

    concurrency::array_view<T, dimension>& get_array_view()
    {
        return this->view;
    }

private:
    std::vector<T> buffer;
    concurrency::array_view<T, dimension> view;
};

class Timer
{
    public:
        Timer(std::chrono::duration<double>& result) : result(result)
        {
            start = clock.now();
        }
        ~Timer()
        {
            result = clock.now() - start;
        }

    private:
        std::chrono::high_resolution_clock clock;
        std::chrono::time_point<std::chrono::high_resolution_clock> start;
        std::chrono::duration<double>& result;
        Timer& operator=(Timer& t);
};

void write_png(UINT width, UINT height, std::vector<unsigned>& red, std::vector<unsigned>& green, std::vector<unsigned>& blue, const std::wstring filename);
void write_png_from_array_views(UINT width, UINT height, const concurrency::array_view<unsigned, 2>& red, const concurrency::array_view<unsigned, 2>& green, const concurrency::array_view<unsigned, 2>& blue, const std::wstring filename);

void throw_hresult_on_failure(HRESULT);

template<typename IType> CComPtr<IType> query_interface(IUnknown* p)
{
    CComPtr<IType> output_interface;
    throw_hresult_on_failure(p->QueryInterface<IType>(&output_interface));
    return output_interface;
}

template<typename T, size_t dimension> T max_element_in_array_view(const concurrency::array_view<T, dimension>& buffer_view)
{
    unsigned a[1]{ 0 };
    auto max_value = concurrency::array_view<T, 1>(1, a);
    parallel_for_each(buffer_view.get_extent(),
        [=](concurrency::index<dimension> idx) restrict(amp)
        {
            concurrency::atomic_fetch_max(&max_value[0], buffer_view[idx]);
        }
    );
    return max_value[0];
}

#endif
