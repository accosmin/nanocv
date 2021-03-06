#include <deque>
#include "solver_lbfgs.h"

using namespace nano;

tuner_t solver_lbfgs_t::tuner() const
{
        tuner_t tuner;
        tuner.add_enum<lsearch_t::initializer>("init");
        tuner.add_enum<lsearch_t::strategy>("strat");
        return tuner;
}

void solver_lbfgs_t::from_json(const json_t& json)
{
        nano::from_json(json,
                "init", m_init, "strat", m_strat,
                "c1", m_c1, "c2", m_c2, "history", m_history_size);
}

void solver_lbfgs_t::to_json(json_t& json) const
{
        nano::to_json(json,
                "init", m_init, "inits", join(enum_values<lsearch_t::initializer>()),
                "strat", m_strat, "strats", join(enum_values<lsearch_t::strategy>()),
                "c1", m_c1, "c2", m_c2, "history", m_history_size);
}

solver_state_t solver_lbfgs_t::minimize(const size_t max_iterations, const scalar_t epsilon,
        const solver_function_t& function, const vector_t& x0, const logger_t& logger) const
{
        lsearch_t lsearch(m_init, m_strat, m_c1, m_c2);

        // previous state
        solver_state_t pstate(function.size());

        std::deque<vector_t> ss, ys;
        vector_t q, r;

        auto cstate = solver_state_t{function, x0};
        for (size_t i = 0; i < max_iterations; ++ i, ++ cstate.m_iterations)
        {
                // descent direction
                //      (see "Numerical optimization", Nocedal & Wright, 2nd edition, p.178)
                q = cstate.g;

                const auto hsize = ss.size();

                std::vector<scalar_t> alphas(hsize);
                for (size_t j = 0; j < hsize; ++ j)
                {
                        const auto& s = ss[hsize - 1 - j];
                        const auto& y = ys[hsize - 1 - j];

                        const scalar_t alpha = s.dot(q) / s.dot(y);
                        q.noalias() -= alpha * y;
                        alphas[j] = alpha;
                }

                if (ss.empty())
                {
                        r = q;
                }
                else
                {
                        const auto& s = ss[hsize - 1];
                        const auto& y = ys[hsize - 1];

                        r = s.dot(y) / y.dot(y) * q;
                }

                for (size_t j = 0; j < hsize; ++ j)
                {
                        const auto& s = ss[j];
                        const auto& y = ys[j];

                        const scalar_t alpha = alphas[hsize - 1 - j];
                        const scalar_t beta = y.dot(r) / s.dot(y);
                        r.noalias() += s * (alpha - beta);
                }

                cstate.d = -r;

                // line-search
                pstate = cstate;
                const auto iter_ok = lsearch(function, cstate);
                if (solver_t::done(logger, function, cstate, epsilon, iter_ok))
                {
                        break;
                }

                // todo: may skip the update if the curvature condition is not satisfied
                // see: "A Multi-Batch L-BFGS Method for Machine Learning", page 6 - the non-convex case
                ss.emplace_back(cstate.x - pstate.x);
                ys.emplace_back(cstate.g - pstate.g);
                if (ss.size() > m_history_size)
                {
                        ss.pop_front();
                        ys.pop_front();
                }
        }

        return cstate;
}
