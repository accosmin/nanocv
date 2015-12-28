#pragma once

#include "lrate.hpp"

namespace math
{
        ///
        /// \brief common parameters for stochastic optimization
        ///
        template
        <
                typename tproblem                       ///< optimization problem
        >
        struct stoch_params_t
        {
                using tstate = typename tproblem::tstate;
                using tscalar = typename tproblem::tscalar;
                using tvector = typename tproblem::tvector;
                using topulog = typename tproblem::topulog;

                ///
                /// \brief constructor
                ///
                stoch_params_t( std::size_t epochs,
                                std::size_t epoch_size,
                                tscalar alpha0,
                                tscalar decay,
                                tscalar momentum,
                                const topulog& ulog = topulog())
                        :       m_ulog(ulog),
                                m_epochs(epochs),
                                m_epoch_size(epoch_size),
                                m_decay(alpha0, decay),
                                m_momentum(momentum),
                                m_epsilon(tscalar(1e-6))
                {
                }

                ///
                /// \brief log the current optimization state
                ///
                bool ulog(const tstate& state) const
                {
                        return m_ulog ? m_ulog(state) : true;
                }

                ///
                /// \brief current learning rate (following the decay rate)
                ///
                tscalar alpha(const std::size_t iter) const
                {
                        return m_decay.get(iter);
                }

                // attributes
                topulog                 m_ulog;         ///< update log: (the current_state_after_each_epoch)
                std::size_t             m_epochs;       ///< number of epochs
                std::size_t             m_epoch_size;   ///< epoch size in number of iterations
                mutable lrate_t<tscalar>m_decay;        ///< learning rate decay (if applicable)
                tscalar                 m_momentum;     ///< exponential running average (if applicable)
                tscalar                 m_epsilon;      ///< constant (e.g. to prevent divide-by-zero, if applicable)
        };
}