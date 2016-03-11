#include "minibatch_trainer.h"
#include "cortex/model.h"
#include "math/numeric.hpp"
#include "cortex/minibatch.h"
#include "cortex/util/logger.h"
#include "text/from_params.hpp"

namespace nano
{
        minibatch_trainer_t::minibatch_trainer_t(const string_t& parameters)
                :       trainer_t(parameters)
        {
        }

        trainer_result_t minibatch_trainer_t::train(
                const task_t& task, const fold_t& fold, const loss_t& loss, size_t nthreads, const criterion_t& criterion,
                model_t& model) const
        {
                if (fold.second != protocol::train)
                {
                        log_error() << "minibatch trainer: can only train models with training samples!";
                        return trainer_result_t();
                }

                // initialize the model
                model.resize(task, true);
                model.random_params();

                // parameters
                const size_t epochs = nano::clamp(nano::from_params<size_t>(configuration(), "epoch", 16), 1, 1024);
                const scalar_t epsilon = nano::clamp(nano::from_params<scalar_t>(configuration(), "eps", 1e-4), 1e-8, 1e-3);

                const nano::batch_optimizer optimizer = nano::from_string<nano::batch_optimizer>
                        (nano::from_params<string_t>(configuration(), "opt", "gd"));

                // train the model
                const trainer_result_t result = nano::minibatch_train(
                        model, task, fold, nthreads, loss, criterion, optimizer, epochs, epsilon);

                const trainer_state_t state = result.optimum_state();

                log_info() << "optimum [train = " << state.m_tvalue << "/" << state.m_terror_avg
                           << ", valid = " << state.m_vvalue << "/" << state.m_verror_avg
                           << ", epoch = " << result.optimum_epoch()
                           << ", config = " << result.optimum_config()
                           << "].";

                // OK
                if (result.valid())
                {
                        model.load_params(result.optimum_params());
                }
                return result;
        }
}
