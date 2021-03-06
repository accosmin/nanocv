#include "utest.h"
#include "function.h"
#include "core/random.h"
#include "tensor/numeric.h"

using namespace nano;

NANO_BEGIN_MODULE(test_functions)

NANO_CASE(evaluate)
{
        for (const auto& rfunction : get_functions(1, 4, std::regex(".+")))
        {
                const auto& function = *rfunction;
                std::cout << function.name() << std::endl;

                const auto dims = function.size();
                NANO_CHECK_LESS_EQUAL(dims, 4);
                NANO_CHECK_GREATER_EQUAL(dims, 1);
                NANO_CHECK_GREATER_EQUAL(dims, function.min_size());
                NANO_CHECK_GREATER_EQUAL(function.max_size(), dims);

                auto rng = make_rng();
                auto udist = make_udist<scalar_t>(-10, +10);

                const auto trials = size_t(1000);
                for (size_t t = 0; t < trials; ++ t)
                {
                        vector_t x0(dims), x1(dims);
                        nano::set_random(udist, rng, x0, x1);

                        NANO_CHECK_LESS(function.grad_accuracy(x0), 10 * epsilon2<scalar_t>());
                        NANO_CHECK(!function.is_convex() || function.is_convex(x0, x1, 100));
                }
        }
}

NANO_END_MODULE()
