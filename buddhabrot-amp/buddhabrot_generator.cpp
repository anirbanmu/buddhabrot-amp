#include <chrono>
#include <vector>
#include <sstream>
#include <amp.h>
#include "tinymt1.1.1/tinymt32.h"

// including source because for functions to be called from a parallel_for_each restrict(amp) lambda
//  their definition needs to be in the same translation unit or LTCG has to be enabled (LTCG is
//  incompatible with the debug version)
#include "tinymt1.1.1/tinymt32.cpp"

#include "utilities.h"
#include "buddhabrot_generator.h"

using namespace std;

BuddhabrotGenerator::BuddhabrotGenerator(concurrency::accelerator_view accel_view, concurrency::extent<2> dims, unsigned points_per_iteration, unsigned max_escape_iterations) :
    accel_view(accel_view),
    dims(dims),
    points_per_iteration(points_per_iteration),
    max_escape_iterations(max_escape_iterations),
    count_array(concurrency::array<unsigned, 2>(dims, accel_view))
{
}

concurrency::array<unsigned, 2>& BuddhabrotGenerator::iterate()
{
    auto randoms = generate_random_numbers();
    auto max_iterations = max_escape_iterations;
    auto& recording_array = count_array;

    parallel_for_each(concurrency::extent<1>(points_per_iteration),
        [=, &randoms, &recording_array](concurrency::index<1> idx) restrict(amp)
        {
            const auto c = Complex<float>(randoms[concurrency::index<2>(idx[0], 0)], randoms[concurrency::index<2>(idx[0], 1)]);

            auto z = Complex<float>(0, 0);

            bool iterate_again_and_record = false;
            unsigned i;
            for (i = 0; i < max_iterations; ++i)
            {
                z = c + (z * z);
                if (z.magnitude_squared() >= 4.0)
                {
                    iterate_again_and_record = true;
                    break;
                }
            }
            if (iterate_again_and_record)
            {
                z = Complex<float>(0, 0);
                for (unsigned j = 0; j < i; j++)
                {
                    z = c + (z * z);
                    if (j >= 400)
                    {
                        const auto dims = recording_array.get_extent();
                        const auto y = unsigned(((z.r + 1.8) / 3.6) * dims[0]);
                        const auto x = unsigned(((z.i + 1.8) / 3.6) * dims[1]);
                        concurrency::atomic_fetch_inc(&recording_array[concurrency::index<2>(y, x)]);
                        concurrency::atomic_fetch_inc(&recording_array[concurrency::index<2>(y, dims[1] - x - 1)]);
                    }
                }
            }
        }
    );

    return recording_array;
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