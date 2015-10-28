#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "test_accumulator"

#include <boost/test/unit_test.hpp>
#include "math/abs.hpp"
#include "cortex/cortex.h"
#include "thread/thread.h"
#include "math/epsilon.hpp"
#include "text/to_string.hpp"
#include "cortex/util/timer.h"
#include "cortex/util/logger.h"
#include "cortex/accumulator.h"
#include "cortex/tasks/task_charset.h"

namespace test
{
        using namespace cortex;

        bool check_fold(const samples_t& samples, cortex::fold_t fold)
        {
                return std::find_if(samples.begin(), samples.end(),
                       [&] (const sample_t& sample) { return sample.m_fold != fold; }) == samples.end();
        }
}

BOOST_AUTO_TEST_CASE(test_accumulator)
{
        using namespace cortex;

        cortex::init();

        const size_t cmd_samples = 256;
        const scalar_t cmd_epsilon = math::epsilon1<scalar_t>();

        charset_task_t task(charset::numeric, 16, 16, color_mode::luma, cmd_samples);
        BOOST_CHECK_EQUAL(task.load(""), true);

        const samples_t samples = task.samples();
        BOOST_CHECK_EQUAL(samples.size(), cmd_samples);

        const string_t lmodel0;
        const string_t lmodel1 = lmodel0 + "linear:dims=32;act-snorm;";
        const string_t lmodel2 = lmodel1 + "linear:dims=32;act-snorm;";

        string_t cmodel;
        cmodel = cmodel + "conv:dims=3,rows=5,cols=5;pool-max;act-snorm;";
        cmodel = cmodel + "conv:dims=5,rows=3,cols=3;pool-max;act-snorm;";

        const string_t outlayer = "linear:dims=" + text::to_string(task.osize()) + ";";

        strings_t cmd_networks =
        {
                lmodel0 + outlayer,
                lmodel1 + outlayer,
                lmodel2 + outlayer,

                cmodel + outlayer
        };

        const rloss_t loss = cortex::get_losses().get("logistic");
        BOOST_CHECK_EQUAL(loss.operator bool(), true);

        // check various networks
        for (const string_t& cmd_network : cmd_networks)
        {
                log_info() << "<<< running network [" << cmd_network << "] ...";

                // create feed-forward network
                const rmodel_t model = cortex::get_models().get("forward-network", cmd_network);
                BOOST_CHECK_EQUAL(model.operator bool(), true);
                BOOST_CHECK_EQUAL(model->resize(task, true), true);

                // check various criteria
                const strings_t criteria = cortex::get_criteria().ids();
                for (const string_t& criterion : criteria)
                {
                        model->random_params();

                        const scalar_t lambda = 0.1;

                        // accumulators using 1 thread
                        accumulator_t lacc(*model, 1, criterion, criterion_t::type::value, lambda);
                        accumulator_t gacc(*model, 1, criterion, criterion_t::type::vgrad, lambda);

                        BOOST_CHECK_EQUAL(lacc.lambda(), lambda);
                        BOOST_CHECK_EQUAL(gacc.lambda(), lambda);

                        lacc.set_lambda(lambda);
                        gacc.set_lambda(lambda);

                        BOOST_CHECK_EQUAL(lacc.lambda(), lambda);
                        BOOST_CHECK_EQUAL(gacc.lambda(), lambda);

                        lacc.update(task, samples, *loss);
                        const scalar_t value1 = lacc.value();

                        BOOST_CHECK_EQUAL(lacc.count(), cmd_samples);

                        gacc.update(task, samples, *loss);
                        const scalar_t vgrad1 = gacc.value();
                        const vector_t pgrad1 = gacc.vgrad();

                        BOOST_CHECK_EQUAL(gacc.count(), cmd_samples);
                        BOOST_CHECK(std::isfinite(vgrad1));
                        BOOST_CHECK_LE(math::abs(vgrad1 - value1), cmd_epsilon);

                        // check results with multiple threads
                        for (size_t nthreads = 2; nthreads < 3 * thread::n_threads(); nthreads ++)
                        {
                                accumulator_t laccx(*model, nthreads, criterion, criterion_t::type::value, lambda);
                                accumulator_t gaccx(*model, nthreads, criterion, criterion_t::type::vgrad, lambda);

                                BOOST_CHECK_EQUAL(laccx.lambda(), lambda);
                                BOOST_CHECK_EQUAL(gaccx.lambda(), lambda);

                                laccx.set_lambda(lambda);
                                gaccx.set_lambda(lambda);

                                BOOST_CHECK_EQUAL(laccx.lambda(), lambda);
                                BOOST_CHECK_EQUAL(gaccx.lambda(), lambda);

                                laccx.update(task, samples, *loss);

                                BOOST_CHECK_EQUAL(laccx.count(), cmd_samples);
                                BOOST_CHECK_LE(math::abs(laccx.value() - value1), cmd_epsilon);

                                gaccx.update(task, samples, *loss);

                                BOOST_CHECK_EQUAL(gaccx.count(), cmd_samples);
                                BOOST_CHECK_LE(math::abs(gaccx.value() - vgrad1), cmd_epsilon);
                                BOOST_CHECK_LE((gaccx.vgrad() - pgrad1).lpNorm<Eigen::Infinity>(), cmd_epsilon);
                        }
                }
        }
}
