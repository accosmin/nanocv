#include "solver_stoch_adagrad.h"

using namespace nano;

tuner_t stoch_adagrad_t::configs() const
{
        tuner_t tuner;
        tuner.add("alpha0", make_pow10_scalars(0, -1, +0)).precision(3);
        tuner.add("epsilon", make_pow10_scalars(0, -7, -2)).precision(7);
        return tuner;
}

void stoch_adagrad_t::from_json(const json_t& json)
{
        nano::from_json(json, "alpha0", m_alpha0, "epsilon", m_epsilon);
}

void stoch_adagrad_t::to_json(json_t& json) const
{
        nano::to_json(json, "alpha0", m_alpha0, "epsilon", m_epsilon);
}

solver_state_t stoch_adagrad_t::minimize(const stoch_params_t& param, const function_t& function, const vector_t& x0) const
{
        vector_t gsum2 = vector_t::Zero(x0.size());

        const auto solver = [&] (solver_state_t& cstate, const solver_state_t&)
        {
                // descent direction
                gsum2.array() += cstate.g.array().square();

                cstate.d = -cstate.g.array() / (m_epsilon + gsum2.array().sqrt());

                // update solution
                function.stoch_next();
                cstate.stoch_update(function, m_alpha0);
        };

        const auto snapshot = [&] (const solver_state_t& cstate, solver_state_t& sstate)
        {
                sstate.update(function, cstate.x);
        };

        return loop(param, function, x0, solver, snapshot);
}
