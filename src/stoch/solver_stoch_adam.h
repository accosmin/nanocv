#pragma once

#include "solver_stoch.h"

namespace nano
{
        ///
        /// \brief stochastic Adam,
        ///     see "Adam: A method for stochastic optimization", by Diederik P. Kingma & Jimmy Lei Ba
        ///
        class stoch_adam_t final : public stoch_solver_t
        {
        public:

                tuner_t configs() const final;
                void to_json(json_t&) const final;
                void from_json(const json_t&) final;

                solver_state_t minimize(const stoch_params_t&, const function_t&, const vector_t& x0) const final;

        private:

                // attributes
                scalar_t        m_alpha0{static_cast<scalar_t>(1e-2)};
                scalar_t        m_decay{static_cast<scalar_t>(0.5)};
                scalar_t        m_beta1{static_cast<scalar_t>(0.900)};
                scalar_t        m_beta2{static_cast<scalar_t>(0.999)};
                scalar_t        m_epsilon{static_cast<scalar_t>(1e-6)};
        };
}
