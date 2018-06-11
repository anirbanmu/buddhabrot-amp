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

template<typename DurationType = std::chrono::duration<double>> class Timer
{
    public:
        Timer(DurationType& result) : result(result)
        {
            start = clock.now();
        }
        ~Timer()
        {
            result = std::chrono::duration_cast<DurationType>(clock.now() - start);
        }

    private:
        std::chrono::high_resolution_clock clock;
        std::chrono::time_point<decltype(clock)> start;
        DurationType& result;
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

template<typename T, size_t dimension> T max_element_in_array_view_naive(const concurrency::array_view<T, dimension>& buffer_view)
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

template<typename T> T max_element_in_array_view(const concurrency::array_view<T, 1>& buffer_view)
{
    auto scratch_array = concurrency::array<T, 1>(buffer_view.get_extent());
    concurrency::copy(buffer_view, scratch_array);
    return max_element_in_array(scratch_array);
}

template<typename T> T max_element_in_array_view(const concurrency::array_view<T, 2>& buffer_view)
{
    const auto extent = buffer_view.get_extent();

    auto scratch_array = concurrency::array<T, 1>(extent[0] * extent[1]);
    parallel_for_each(extent,
        [=, &scratch_array](concurrency::index<2> idx) restrict(amp)
        {
            scratch_array[idx[0] * extent[1] + idx[1]] = buffer_view[idx];
        }
    );

    return max_element_in_array(scratch_array);
}

template<typename T> T max_element_in_array(concurrency::array<T, 1>& scratch_array)
{
    const auto extent = scratch_array.get_extent();

    for (int value_count = extent[0]; value_count > 1; value_count /= 2)
    {
        const int buckets = value_count / 2;
        parallel_for_each(concurrency::extent<1>(value_count),
            [&, buckets](concurrency::index<1> idx) restrict(amp)
            {
                concurrency::atomic_fetch_max(&scratch_array[idx[0] % buckets], scratch_array[idx]);
            }
        );
    }

    unsigned last_value[1];
    auto max_value = concurrency::array_view<T, 1>(1, last_value);
    parallel_for_each(max_value.get_extent(),
        [=, &scratch_array](concurrency::index<1>) restrict(amp)
        {
            max_value[0] = scratch_array[0];
        }
    );

    return max_value[0];
}

#endif
