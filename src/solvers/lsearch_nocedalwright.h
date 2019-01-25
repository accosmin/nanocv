#pragma once

#include "lsearch.h"

namespace nano
{
        ///
        /// \brief the More&Thuente-like line-search algorithm described here:
        ///     see "Numerical optimization", Nocedal & Wright, 2nd edition, p.60
        ///
        class lsearch_nocedalwright_t final : public lsearch_strategy_t
        {
        public:

                lsearch_nocedalwright_t() = default;
                bool get(const solver_state_t& state0, const scalar_t t0, solver_state_t& state) final;

        private:

                bool zoom(const solver_state_t&, step_t& l, step_t& h, solver_state_t&) const;
        };
}