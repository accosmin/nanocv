#pragma once

#include "solver_stoch.h"

namespace nano
{
        ///
        /// \brief stochastic RMSProp (AdaGrad with an exponentially weighted running average of the gradients)
        ///     see Lecture 6.5 - rmsprop, COURSERA: Neural Networks for Machine Learning
        ///      by Tieleman, T. and Hinton, G. (2012)
        ///
        class stoch_rmsprop_t final : public stoch_solver_t
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
                scalar_t        m_momentum{static_cast<scalar_t>(0.90)};
                scalar_t        m_epsilon{static_cast<scalar_t>(1e-6)};
        };
}
