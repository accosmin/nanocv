#include "unit_test.hpp"
#include "math/momentum.hpp"
#include "math/epsilon.hpp"

namespace test
{
        template
        <
                typename tscalar,
                typename tsize
        >
        void check_momentum(const tscalar momentum, const tsize range)
        {
                math::momentum_scalar_t<tscalar> mom00(momentum);
                math::momentum_scalar_t<tscalar> mom01(momentum);
                math::momentum_scalar_t<tscalar> mom10(1 - momentum);
                math::momentum_scalar_t<tscalar> mom11(1 - momentum);

                const auto epsilon = math::epsilon1<tscalar>();
                for (tsize i = 1; i <= range; ++ i)
                {
                        const auto base00 = momentum;
                        const auto base01 = 1 - momentum;
                        const auto base10 = momentum;
                        const auto base11 = 1 - momentum;

                        mom00.update(base00);
                        mom01.update(base01);
                        mom10.update(base10);
                        mom11.update(base11);

                        ZOB_CHECK_CLOSE(mom00.value(), base00, epsilon);
                        ZOB_CHECK_CLOSE(mom01.value(), base01, epsilon);
                        ZOB_CHECK_CLOSE(mom10.value(), base10, epsilon);
                        ZOB_CHECK_CLOSE(mom11.value(), base11, epsilon);
                }
        }

        template
        <
                typename tvector,
                typename tscalar = typename tvector::Scalar,
                typename tsize = typename tvector::Index
        >
        void check_momentum(const tsize dims, const tscalar momentum, const tsize range)
        {
                math::momentum_vector_t<tvector> mom00(momentum, dims);
                math::momentum_vector_t<tvector> mom01(momentum, dims);
                math::momentum_vector_t<tvector> mom10(1 - momentum, dims);
                math::momentum_vector_t<tvector> mom11(1 - momentum, dims);

                const auto epsilon = math::epsilon1<tscalar>();
                for (tsize i = 1; i <= range; ++ i)
                {
                        const auto base00 = tvector::Constant(dims, momentum);
                        const auto base01 = tvector::Constant(dims, 1 - momentum);
                        const auto base10 = tvector::Constant(dims, momentum);
                        const auto base11 = tvector::Constant(dims, 1 - momentum);

                        mom00.update(base00);
                        mom01.update(base01);
                        mom10.update(base10);
                        mom11.update(base11);

                        ZOB_CHECK_EIGEN_CLOSE(mom00.value(), base00, epsilon);
                        ZOB_CHECK_EIGEN_CLOSE(mom01.value(), base01, epsilon);
                        ZOB_CHECK_EIGEN_CLOSE(mom10.value(), base10, epsilon);
                        ZOB_CHECK_EIGEN_CLOSE(mom11.value(), base11, epsilon);
                }
        }
}

ZOB_BEGIN_MODULE(test_momentum)

ZOB_CASE(scalar)
{
        test::check_momentum<double>(0.1, 123);
        test::check_momentum<double>(0.5, 127);
        test::check_momentum<double>(0.9, 253);
}

ZOB_CASE(vector)
{
        test::check_momentum<Eigen::VectorXd>(13, 0.1, 98);
        test::check_momentum<Eigen::VectorXd>(17, 0.5, 75);
        test::check_momentum<Eigen::VectorXd>(11, 0.9, 54);
}

ZOB_END_MODULE()
