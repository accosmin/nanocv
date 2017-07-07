#pragma once

#include "solver_batch.h"

namespace nano
{
        ///
        /// \brief gradient descent
        ///
        struct batch_gd_t final : public batch_solver_t
        {
                explicit batch_gd_t(const string_t& params = string_t());

                function_state_t minimize(const batch_params_t&, const function_t&, const vector_t& x0) const override;

                function_state_t minimize(const batch_params_t&, const function_t&, const vector_t& x0,
                        const ls_initializer, const ls_strategy, const scalar_t c1, const scalar_t c2) const;
        };
}