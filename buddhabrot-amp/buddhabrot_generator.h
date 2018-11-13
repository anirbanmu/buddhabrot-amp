#ifndef _BUDDHABROT_GENERATOR_H_
#define _BUDDHABROT_GENERATOR_H_

class BuddhabrotGenerator
{
    public:
        // dimensions is the size of the canvas we are going to generate
        // points_per_iteration should be a square
        // max_escape_iterations is how many iterations is considered before counting an initial complex number to be "escaping" the mandelbrot set
        BuddhabrotGenerator(concurrency::accelerator_view, concurrency::extent<2> dimensions, unsigned points_per_iteration, unsigned max_escape_iterations);
        concurrency::array<unsigned, 2>& iterate();
        concurrency::array<unsigned, 2>& get_record_array()
        {
            return count_array;
        }

    private:
        concurrency::array<float, 2> generate_random_numbers();

        concurrency::accelerator_view accel_view;
        const concurrency::extent<2> dims;
        const unsigned points_per_iteration;
        const unsigned max_escape_iterations;
        concurrency::array<unsigned, 2> count_array;
};

#endif