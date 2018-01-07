#include "tensor/momentum.h"
#include "text/json_writer.h"
#include "solver_stoch_adadelta.h"

using namespace nano;

strings_t stoch_adadelta_t::configs()
{
        strings_t configs;
        for (const auto momentum : {0.10, 0.50, 0.90})
        for (const auto epsilon : {1e-4, 1e-6})
        {
                configs.push_back(json_writer_t().object("momentum", momentum, "epsilon", epsilon).str());
        }
        return configs;
}

json_reader_t& stoch_adadelta_t::config(json_reader_t& reader)
{
        return reader.object("momentum", m_momentum, "epsilon", m_epsilon);
}

json_writer_t& stoch_adadelta_t::config(json_writer_t& writer) const
{
        return writer.object("momentum", m_momentum, "epsilon", m_epsilon);
}

solver_state_t stoch_adadelta_t::minimize(const stoch_params_t& param, const function_t& function, const vector_t& x0) const
{
        // second-order momentum of the gradient
        momentum_t<vector_t> gavg(m_momentum, x0.size());

        // second-order momentum of the step updates
        momentum_t<vector_t> davg(m_momentum, x0.size());

        // assembly the solver
        const auto solver = [&] (solver_state_t& cstate, const solver_state_t&)
        {
                // descent direction
                gavg.update(cstate.g.array().square());

                cstate.d = -cstate.g.array() *
                           (m_epsilon + davg.value().array().sqrt()) /
                           (m_epsilon + gavg.value().array().sqrt());

                davg.update(cstate.d.array().square());

                // update solution
                function.stoch_next();
                cstate.stoch_update(function, 1);
        };

        const auto snapshot = [&] (const solver_state_t& cstate, solver_state_t& sstate)
        {
                sstate.update(function, cstate.x);
        };

        return loop(param, function, x0, solver, snapshot);
}
