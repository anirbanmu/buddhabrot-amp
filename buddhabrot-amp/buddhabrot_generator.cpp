#define NOMINMAX

#include <chrono>
#include <vector>
#include <sstream>
#include <array>
#include <amp.h>
#include "tinymt1.1.1/tinymt32.h"

// including source because for functions to be called from a parallel_for_each restrict(amp) lambda
//  their definition needs to be in the same translation unit or LTCG has to be enabled (LTCG is
//  incompatible with the debug version)
#include "tinymt1.1.1/tinymt32.cpp"

#include "utilities.h"
#include "buddhabrot_generator.h"

using namespace std;

BuddhabrotGenerator::BuddhabrotGenerator(concurrency::accelerator_view accel_view, concurrency::extent<2> dims, unsigned points_per_iteration, array<tuple<unsigned, unsigned>, 3> iteration_ranges) :
    accel_view(accel_view),
    dims(dims),
    points_per_iteration(points_per_iteration),
    iteration_ranges(iteration_ranges),
    max_escape_iterations(max(get<1>(iteration_ranges[0]), max(get<1>(iteration_ranges[1]), get<1>(iteration_ranges[2])))),
    red_count_array(concurrency::array<unsigned, 2>(dims, accel_view)),
    green_count_array(concurrency::array<unsigned, 2>(dims, accel_view)),
    blue_count_array(concurrency::array<unsigned, 2>(dims, accel_view))
{
}

concurrency::array<unsigned, 2>* pick_array(concurrency::array<unsigned, 2>& r, concurrency::array<unsigned, 2>& g, concurrency::array<unsigned, 2>& b, unsigned i, const unsigned* mins, const unsigned* maxs) restrict(amp)
{
    if (i >= mins[0] && i < maxs[0])
    {
        return &r;
    }
    if (i >= mins[1] && i < maxs[1])
    {
        return &g;
    }
    if (i >= mins[2] && i < maxs[2])
    {
        return &b;
    }
    return nullptr;
}

const concurrency::array<unsigned, 2>& BuddhabrotGenerator::iterate()
{
    auto randoms = generate_random_numbers();

    auto& red_array = red_count_array;
    auto& green_array = green_count_array;
    auto& blue_array = blue_count_array;

    // auto& recording_array = count_array;

    const unsigned min_iterations[]{ get<0>(iteration_ranges[0]), get<0>(iteration_ranges[1]), get<0>(iteration_ranges[2]) };
    const unsigned max_iterations[]{ get<1>(iteration_ranges[0]), get<1>(iteration_ranges[1]), get<1>(iteration_ranges[2]) };
    const auto overall_max_iterations = max_escape_iterations;

    parallel_for_each(concurrency::extent<1>(points_per_iteration),
        [=, &randoms, &red_array, &green_array, &blue_array](concurrency::index<1> idx) restrict(amp)
        {
            const auto c = Complex<float>(randoms[concurrency::index<2>(idx[0], 0)], randoms[concurrency::index<2>(idx[0], 1)]);

            auto z = Complex<float>(0, 0);

            concurrency::array<unsigned, 2>* recording_array = nullptr;
            unsigned i;
            for (i = 0; i < overall_max_iterations; ++i)
            {
                z = c + (z * z);
                if (z.magnitude_squared() >= 4.0)
                {
                    recording_array = pick_array(red_array, green_array, blue_array, i, min_iterations, max_iterations);
                    break;
                }
            }
            if (recording_array != nullptr)
            {
                auto& arr = *recording_array;
                z = Complex<float>(0, 0);
                for (unsigned j = 0; j < i; j++)
                {
                    z = c + (z * z);
                    if (j >= 400)
                    {
                        const auto dims = arr.get_extent();
                        const auto y = unsigned(((z.r + 1.8) / 3.6) * dims[0]);
                        const auto x = unsigned(((z.i + 1.8) / 3.6) * dims[1]);
                        concurrency::atomic_fetch_inc(&arr[concurrency::index<2>(y, x)]);
                        concurrency::atomic_fetch_inc(&arr[concurrency::index<2>(y, dims[1] - x - 1)]);
                    }
                }
            }
        }
    );

    return red_array;
}

concurrency::array<float, 2> BuddhabrotGenerator::generate_random_numbers()
{
    auto rand_array = concurrency::array<float, 2>(concurrency::extent<2>(points_per_iteration, 2), accel_view);
    const auto seed = static_cast<unsigned>(chrono::system_clock::now().time_since_epoch().count());

    const auto threads = unsigned(sqrt(points_per_iteration));
    const auto per_thread = threads;

    parallel_for_each(concurrency::extent<1>(threads),
        [=, &rand_array](concurrency::index<1> thread_idx) restrict(amp)
        {
            auto tinymt = tinymt32_t();
            tinymt32_init(&tinymt, seed * thread_idx[0]);

            const auto start_idx = thread_idx[0] * per_thread;
            for (unsigned i = 0; i < per_thread; ++i)
            {
                const auto current_idx = start_idx + i;
                rand_array[concurrency::index<2>(current_idx, 0)] = tinymt32_generate_float(&tinymt) * 3.6f - 1.8f;
                rand_array[concurrency::index<2>(current_idx, 1)] = tinymt32_generate_float(&tinymt) * 3.6f - 1.8f;
            }
        }
    );

    return rand_array;
}