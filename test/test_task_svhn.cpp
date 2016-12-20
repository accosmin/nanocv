#include <set>
#include "nano.h"
#include "utest.h"
#include "task_util.h"
#include "math/epsilon.h"

NANO_BEGIN_MODULE(test_svhn)

NANO_CASE(construction)
{
        using namespace nano;

        const auto path = string_t(std::getenv("HOME")) + "/experiments/databases/svhn";

        const auto idims = 3;
        const auto irows = 32;
        const auto icols = 32;
        const auto odims = 10;
        const auto orows = 1;
        const auto ocols = 1;
        const auto target_sum = scalar_t(2) - static_cast<scalar_t>(odims);

        const auto folds = size_t(1);
        const auto train_samples = size_t(73257 + 531131);
        const auto test_samples = size_t(26032);
        const auto train_fold = fold_t{0, protocol::train};
        const auto valid_fold = fold_t{0, protocol::valid};
        const auto test_fold = fold_t{0, protocol::test};

        const auto task = nano::get_tasks().get("svhn", "dir=" + path);
        NANO_REQUIRE(task);
        NANO_REQUIRE(task->load());

        // check dimensions
        NANO_CHECK_EQUAL(task->idims(), idims);
        NANO_CHECK_EQUAL(task->irows(), irows);
        NANO_CHECK_EQUAL(task->icols(), icols);
        NANO_CHECK_EQUAL(task->odims(), odims);
        NANO_CHECK_EQUAL(task->orows(), orows);
        NANO_CHECK_EQUAL(task->ocols(), ocols);

        // check folds
        NANO_CHECK_EQUAL(task->n_folds(), folds);
        NANO_CHECK_EQUAL(task->n_samples(), train_samples + test_samples);

        NANO_CHECK_GREATER(task->n_samples(train_fold), train_samples / 10);
        NANO_CHECK_GREATER(task->n_samples(valid_fold), train_samples / 10);
        NANO_CHECK_EQUAL(task->n_samples(train_fold) + task->n_samples(valid_fold), train_samples);

        NANO_CHECK_EQUAL(task->n_samples(test_fold), test_samples);

        // check samples
        std::set<string_t> labels;
        for (const auto fold : {train_fold, valid_fold, test_fold})
        {
                const auto size = task->n_samples(fold);
                for (size_t i = 0; i < size; ++ i)
                {
                        const auto input = task->input(fold, i);
                        const auto label = task->label(fold, i);
                        const auto target = task->target(fold, i);

                        NANO_CHECK_EQUAL(input.size<0>(), idims);
                        NANO_CHECK_EQUAL(input.size<1>(), irows);
                        NANO_CHECK_EQUAL(input.size<2>(), icols);

                        NANO_CHECK_EQUAL(target.size<0>(), odims);
                        NANO_CHECK_EQUAL(target.size<1>(), orows);
                        NANO_CHECK_EQUAL(target.size<2>(), ocols);
                        NANO_CHECK_CLOSE(target.vector().sum(), target_sum, epsilon0<scalar_t>());

                        labels.insert(label);
                }
        }

        NANO_CHECK(nano::check_duplicates(*task));
        NANO_CHECK(nano::check_intersection(*task));
        NANO_CHECK_EQUAL(labels.size(), odims);
}

NANO_END_MODULE()
