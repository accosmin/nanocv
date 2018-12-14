#include "task.h"
#include "utest.h"
#include "core/numeric.h"

using namespace nano;

UTEST_BEGIN_MODULE(test_task_svhn)

UTEST_CASE(failed)
{
        const auto task = get_tasks().get("svhn");
        UTEST_REQUIRE(task);

        task->from_json(to_json("dir", "/dev/null?!"));
        UTEST_CHECK(!task->load());
}

UTEST_CASE(default_config)
{
        const auto task = nano::get_tasks().get("svhn");
        UTEST_REQUIRE(task);

        json_t json;
        task->to_json(json);

        size_t folds = 0;
        from_json(json, "folds", folds);

        UTEST_CHECK_EQUAL(folds, 10u);
}

UTEST_CASE(loading)
{
        const auto idims = tensor3d_dim_t{3, 32, 32};
        const auto odims = tensor3d_dim_t{10, 1, 1};
        const auto target_sum = scalar_t(2) - static_cast<scalar_t>(nano::size(odims));

        const auto folds = size_t(3);
        const auto train_samples = size_t(73257 + 531131);
        const auto test_samples = size_t(26032);

        const auto task = nano::get_tasks().get("svhn");
        UTEST_REQUIRE(task);
        task->from_json(to_json("folds", folds));
        UTEST_REQUIRE(task->load());
        task->describe("svhn");

        UTEST_CHECK_EQUAL(task->idims(), idims);
        UTEST_CHECK_EQUAL(task->odims(), odims);
        UTEST_CHECK_EQUAL(task->fsize(), folds);
        UTEST_CHECK_EQUAL(task->size(), folds * (train_samples + test_samples));

        for (size_t f = 0; f < task->fsize(); ++ f)
        {
                for (const auto p : {protocol::train, protocol::valid, protocol::test})
                {
                        const auto fold = fold_t{f, p};
                        task->shuffle(fold);

                        for (size_t i = 0; i < std::min(size_t(128), task->size(fold)); ++ i)
                        {
                                const auto input = task->input(fold, i);
                                const auto target = task->target(fold, i);

                                UTEST_CHECK_EQUAL(input.dims(), idims);
                                UTEST_CHECK_EQUAL(target.dims(), odims);
                                UTEST_CHECK_CLOSE(target.vector().sum(), target_sum, epsilon0<scalar_t>());
                        }

                        UTEST_CHECK_EQUAL(task->labels({f, p}).size(), static_cast<size_t>(nano::size(odims)));
                }

                UTEST_CHECK_EQUAL(task->size({f, protocol::train}), 483509);
                UTEST_CHECK_EQUAL(task->size({f, protocol::valid}), 120879);
                UTEST_CHECK_EQUAL(task->size({f, protocol::test}), test_samples);

                UTEST_CHECK_EQUAL(
                        task->size({f, protocol::train}) +
                        task->size({f, protocol::valid}) +
                        task->size({f, protocol::test}),
                        task->size() / task->fsize());

                UTEST_CHECK_LESS_EQUAL(task->duplicates(f), size_t(2));
                UTEST_CHECK_LESS_EQUAL(task->intersections(f), size_t(0));
        }

        UTEST_CHECK_LESS_EQUAL(task->duplicates(), size_t(2));
        UTEST_CHECK_LESS_EQUAL(task->intersections(), size_t(0));
        UTEST_CHECK_EQUAL(task->labels().size(), static_cast<size_t>(nano::size(odims)));
}

UTEST_END_MODULE()
