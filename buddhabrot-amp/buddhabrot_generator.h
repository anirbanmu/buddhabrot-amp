#ifndef _BUDDHABROT_GENERATOR_H_
#define _BUDDHABROT_GENERATOR_H_

class BuddhabrotGenerator
{
    public:
        // dimensions is the size of the canvas we are going to generate
        // points_per_iteration should be a square
        // we will record the path taken by an initial point if it escapes in iterations that fall in the iteration_range;
        //  if the initial point does not escape within std::get<1>(iteration_range) we will consider it "non-escaping" the manderbrot set
        BuddhabrotGenerator(concurrency::accelerator_view, concurrency::extent<2> dimensions, unsigned points_per_iteration, std::tuple<unsigned, unsigned> iteration_range);
        const concurrency::array<unsigned, 2>& iterate();
        const concurrency::array<unsigned, 2>& get_record_array()
        {
            return count_array;
        }

    private:
        concurrency::array<float, 2> generate_random_numbers();

        concurrency::accelerator_view accel_view;
        const concurrency::extent<2> dims;
        const unsigned points_per_iteration;
        const std::tuple<unsigned, unsigned> iteration_range;
        concurrency::array<unsigned, 2> count_array;
};

#endif