#include "core/table.h"
#include "core/timer.h"
#include "math/abs.hpp"
#include "core/logger.h"
#include "text/align.hpp"
#include "math/clamp.hpp"
#include "math/stats.hpp"
#include "math/random.hpp"
#include "core/minimize.h"
#include "math/numeric.hpp"
#include "math/epsilon.hpp"
#include "thread/loopi.hpp"
#include "text/from_string.hpp"
#include "text/starts_with.hpp"
#include "core/table_row_comp.h"

#include "func/function_trid.h"
#include "func/function_beale.h"
#include "func/function_booth.h"
#include "func/function_cauchy.h"
#include "func/function_sphere.h"
#include "func/function_matyas.h"
#include "func/function_powell.h"
#include "func/function_colville.h"
#include "func/function_zakharov.h"
#include "func/function_mccormick.h"
#include "func/function_himmelblau.h"
#include "func/function_rosenbrock.h"
#include "func/function_3hump_camel.h"
#include "func/function_sum_squares.h"
#include "func/function_dixon_price.h"
#include "func/function_bohachevsky.h"
#include "func/function_goldstein_price.h"
#include "func/function_styblinski_tang.h"
#include "func/function_rotated_ellipsoid.h"

#include <map>
#include <tuple>

using namespace ncv;

namespace
{
        struct optimizer_stat_t
        {
                math::stats_t<scalar_t>       m_times;        ///< optimization time (microseconds)
                math::stats_t<scalar_t>       m_crits;        ///< convergence criteria
                math::stats_t<scalar_t>       m_fail0s;       ///< #convergence failures
                math::stats_t<scalar_t>       m_fail1s;       ///< #convergence failures
                math::stats_t<scalar_t>       m_fail2s;       ///< #convergence failures
                math::stats_t<scalar_t>       m_fail3s;       ///< #convergence failures
                math::stats_t<scalar_t>       m_iters;        ///< #iterations
                math::stats_t<scalar_t>       m_fcalls;       ///< #function value calls
                math::stats_t<scalar_t>       m_gcalls;       ///< #gradient calls
                math::stats_t<scalar_t>       m_speeds;       ///< convergence speed (actually the average decrease ratio of the convergence criteria)
        };

        math::stats_t<scalar_t> make_stats(const scalars_t& values, const scalars_t& flags)
        {
                assert(values.size() == flags.size());

                math::stats_t<scalar_t> stats;
                for (size_t i = 0; i < values.size(); i ++)
                {
                        if (flags[i] >= 0.0)
                        {
                                stats(values[i]);
                        }
                }
                return stats;
        }

        void show_table(const string_t& name, const std::map<string_t, optimizer_stat_t>& ostats)
        {
                // show global statistics
                table_t table(text::align(name.empty() ? string_t("optimizer") : (name + " optimizer"), 32));
                table.header() << "cost"
                               << "time [us]"
                               << "|grad|/|fval|"
                               << "#fails3"
                               << "#fails2"
                               << "#fails1"
                               << "#fails0"
                               << "#iters"
                               << "#fcalls"
                               << "#gcalls"
                               << "speed";

                for (const auto& it : ostats)
                {
                        const auto& name = it.first;
                        const auto& stat = it.second;

                        table.append(name) << static_cast<size_t>(stat.m_fcalls.avg() + 2 * stat.m_gcalls.avg())
                                           << stat.m_times.avg()
                                           << stat.m_crits.avg()
                                           << static_cast<size_t>(stat.m_fail3s.sum())
                                           << static_cast<size_t>(stat.m_fail2s.sum())
                                           << static_cast<size_t>(stat.m_fail1s.sum())
                                           << static_cast<size_t>(stat.m_fail0s.sum())
                                           << stat.m_iters.avg()
                                           << stat.m_fcalls.avg()
                                           << stat.m_gcalls.avg()
                                           << stat.m_speeds.avg();
                }

                table.sort(ncv::make_table_row_ascending_comp<scalar_t>(indices_t({3, 4, 0})));
                table.print(std::cout);
        }

        template <typename tostats>
        void check_function(const function_t& func, tostats& ostats)
        {
                const auto iterations = opt_size_t(8 * 1024);
                const auto epsilon = math::epsilon0<opt_scalar_t>();
                const auto trials = size_t(1024);

                const size_t dims = func.problem().size();

                math::random_t<opt_scalar_t> rgen(-1.0, +1.0);

                // generate fixed random trials
                std::vector<opt_vector_t> x0s(trials);
                for (auto& x0 : x0s)
                {
                        x0.resize(dims);
                        rgen(x0.data(), x0.data() + x0.size());
                }

                // optimizers to try
                const auto optimizers =
                {
                        min::batch_optimizer::GD,
                        min::batch_optimizer::CGD_CD,
                        min::batch_optimizer::CGD_DY,
                        min::batch_optimizer::CGD_FR,
                        min::batch_optimizer::CGD_HS,
                        min::batch_optimizer::CGD_LS,
                        min::batch_optimizer::CGD_DYCD,
                        min::batch_optimizer::CGD_DYHS,
                        min::batch_optimizer::CGD_PRP,
                        min::batch_optimizer::CGD_N,
                        min::batch_optimizer::LBFGS
                };

                // line search initialization methods to try
                const auto ls_initializers =
                {
                        min::ls_initializer::unit,
                        min::ls_initializer::quadratic,
                        min::ls_initializer::consistent
                };

                // line search strategies to try
                const auto ls_strategies =
                {
                        min::ls_strategy::backtrack_armijo,
                        min::ls_strategy::backtrack_wolfe,
                        min::ls_strategy::backtrack_strong_wolfe,
                        min::ls_strategy::interpolation,
                        min::ls_strategy::cg_descent
                };

                thread::pool_t pool;

                // per-problem statistics
                tostats stats;

                // evaluate all possible combinations
                for (min::batch_optimizer optimizer : optimizers)
                        for (min::ls_initializer ls_init : ls_initializers)
                                for (min::ls_strategy ls_strat : ls_strategies)
                {
                        scalars_t times(trials);
                        scalars_t crits(trials);
                        scalars_t iters(trials);                        
                        scalars_t fail0s(trials);
                        scalars_t fail1s(trials);
                        scalars_t fail2s(trials);
                        scalars_t fail3s(trials);
                        scalars_t fcalls(trials);
                        scalars_t gcalls(trials);
                        scalars_t speeds(trials);

                        thread::loopi(trials, pool, [&] (size_t t)
                        {
                                const auto& x0 = x0s[t];

                                const auto problem = func.problem();
                                const auto state0 = opt_state_t(problem, x0);
                                const auto g0 = state0.convergence_criteria();

                                // optimize
                                const ncv::timer_t timer;

                                const auto state = ncv::minimize(
                                        problem, nullptr,
                                        x0, optimizer, iterations, epsilon, ls_init, ls_strat);

                                const auto g = state.convergence_criteria();
                                const auto speed = std::pow(g / g0, 1.0 / (1.0 + state.m_iterations));

                                // ignore out-of-domain solutions
                                if (func.is_valid(state.x))
                                {
                                        // update stats
                                        times[t] = timer.microseconds();
                                        crits[t] = g;
                                        iters[t] = state.m_iterations;
                                        fail0s[t] = !state.converged(math::epsilon0<opt_scalar_t>()) ? 1.0 : 0.0;
                                        fail1s[t] = !state.converged(math::epsilon1<opt_scalar_t>()) ? 1.0 : 0.0;
                                        fail2s[t] = !state.converged(math::epsilon2<opt_scalar_t>()) ? 1.0 : 0.0;
                                        fail3s[t] = !state.converged(math::epsilon3<opt_scalar_t>()) ? 1.0 : 0.0;
                                        fcalls[t] = state.m_fcalls;
                                        gcalls[t] = state.m_gcalls;
                                        speeds[t] = speed;
                                }
                                else
                                {
                                        // skip this from statistics!
                                        times[t] = -1.0;
                                }
                        });

                        // update per-problem statistics
                        const string_t name =
                                text::to_string(optimizer) + "[" +
                                text::to_string(ls_init) + "][" +
                                text::to_string(ls_strat) + "]";

                        optimizer_stat_t& stat = stats[name];
                        stat.m_times(make_stats(times, times));
                        stat.m_crits(make_stats(crits, times));
                        stat.m_iters(make_stats(iters, times));
                        stat.m_fail0s(make_stats(fail0s, times));
                        stat.m_fail1s(make_stats(fail1s, times));
                        stat.m_fail2s(make_stats(fail2s, times));
                        stat.m_fail3s(make_stats(fail3s, times));
                        stat.m_fcalls(make_stats(fcalls, times));
                        stat.m_gcalls(make_stats(gcalls, times));
                        stat.m_speeds(make_stats(speeds, times));

                        // update global statistics
                        optimizer_stat_t& ostat = ostats[name];
                        ostat.m_times(stat.m_times);
                        ostat.m_crits(stat.m_crits);
                        ostat.m_iters(stat.m_iters);
                        ostat.m_fail0s(stat.m_fail0s);
                        ostat.m_fail1s(stat.m_fail1s);
                        ostat.m_fail2s(stat.m_fail2s);
                        ostat.m_fail3s(stat.m_fail3s);
                        ostat.m_fcalls(stat.m_fcalls);
                        ostat.m_gcalls(stat.m_gcalls);
                        ostat.m_speeds(stat.m_speeds);
                }

                show_table(func.name(), stats);
        }

        template <typename tstats>
        void check_function(const functions_t& funcs, tstats& ostats)
        {
                for (const auto& func : funcs)
                {
                        check_function(*func, ostats);
                }
        }
}

int main(int, char* [])
{
        using namespace ncv;

        std::map<string_t, optimizer_stat_t> ostats;

        check_function(ncv::make_beale_funcs(), ostats);
        check_function(ncv::make_booth_funcs(), ostats);
        check_function(ncv::make_matyas_funcs(), ostats);
        check_function(ncv::make_trid_funcs(8), ostats);
        check_function(ncv::make_cauchy_funcs(8), ostats);
        check_function(ncv::make_sphere_funcs(8), ostats);
        check_function(ncv::make_powell_funcs(8), ostats);
        check_function(ncv::make_colville_funcs(), ostats);
        check_function(ncv::make_zakharov_funcs(8), ostats);
        check_function(ncv::make_mccormick_funcs(), ostats);
        check_function(ncv::make_himmelblau_funcs(), ostats);
        check_function(ncv::make_rosenbrock_funcs(), ostats);
        check_function(ncv::make_bohachevsky_funcs(), ostats);
        check_function(ncv::make_3hump_camel_funcs(), ostats);
        check_function(ncv::make_dixon_price_funcs(8), ostats);
        check_function(ncv::make_sum_squares_funcs(8), ostats);
        check_function(ncv::make_goldstein_price_funcs(), ostats);
        check_function(ncv::make_styblinski_tang_funcs(8), ostats);
        check_function(ncv::make_rotated_ellipsoid_funcs(8), ostats);

        // show global statistics
        show_table(string_t(), ostats);

        // show global statistics per optimizer
        const auto optimizers =
        {
                min::batch_optimizer::GD,
                min::batch_optimizer::CGD_CD,
                min::batch_optimizer::CGD_DY,
                min::batch_optimizer::CGD_FR,
                min::batch_optimizer::CGD_HS,
                min::batch_optimizer::CGD_LS,
                min::batch_optimizer::CGD_DYCD,
                min::batch_optimizer::CGD_DYHS,
                min::batch_optimizer::CGD_PRP,
                min::batch_optimizer::CGD_N,
                min::batch_optimizer::LBFGS
        };

        for (min::batch_optimizer optimizer : optimizers)
        {
                const string_t name = text::to_string(optimizer) + "[";

                std::map<string_t, optimizer_stat_t> stats;
                for (const auto& ostat : ostats)
                {
                        if (text::starts_with(ostat.first, name))
                        {
                                stats[ostat.first] = ostat.second;
                        }
                }

                show_table(string_t(), stats);
        }

        // OK
        log_info() << done;
        return EXIT_SUCCESS;
}

