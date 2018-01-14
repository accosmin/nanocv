#include "text/cmdline.h"
#include "math/random.h"
#include "function.h"
#include "math/epsilon.h"
#include "solver_stoch.h"
#include "benchmark_solvers.h"

using namespace nano;

template <typename tostats>
static void check_function(const function_t& function, const strings_t& solvers, const size_t trials,
        const size_t epochs, const size_t tune_epochs, const size_t epoch_size, const scalar_t epsilon,
        tostats& gstats)
{
        auto rgen = make_rng(scalar_t(-1), scalar_t(+1));

        // generate fixed random trials
        std::vector<vector_t> x0s(trials);
        for (auto& x0 : x0s)
        {
                x0.resize(function.size());
                rgen(x0.data(), x0.data() + x0.size());
        }

        // per-problem statistics
        tostats stats;

        // evaluate all solvers
        for (const auto& id : solvers)
        {
                const auto solver = get_stoch_solvers().get(id);
                const auto tune_params = stoch_params_t(tune_epochs, epoch_size, epsilon);
                const auto params = stoch_params_t(epochs, epoch_size, epsilon);
                const auto& name = id;

                solver->tune(tune_params, function, x0s[0]);
                benchmark::benchmark_function(solver, params, function, x0s, name, stats, gstats);
        }

        // show per-problem statistics
        benchmark::show_table(function.name(), stats);
}

int main(int argc, const char* argv[])
{
        // parse the command line
        cmdline_t cmdline("benchmark stochastic solvers");
        cmdline.add("", "solvers",      "use this regex to select the solvers to benchmark", ".+");
        cmdline.add("", "functions",    "use this regex to select the functions to benchmark", ".+");
        cmdline.add("", "min-dims",     "minimum number of dimensions for each test function (if feasible)", "10");
        cmdline.add("", "max-dims",     "maximum number of dimensions for each test function (if feasible)", "100");
        cmdline.add("", "trials",       "number of random trials for each test function", "100");
        cmdline.add("", "epochs",       "optimization: number of epochs", "1000");
        cmdline.add("", "epoch-size",   "optimization: number of iterations per epoch", "100");
        cmdline.add("", "tune-epochs",  "optimization: number of epochs to use for tuning", "100");
        cmdline.add("", "epsilon",      "convergence criteria", 1e-4);
        cmdline.add("", "convex",       "use only convex test functions");

        cmdline.process(argc, argv);

        // check arguments and options
        const auto min_dims = cmdline.get<tensor_size_t>("min-dims");
        const auto max_dims = cmdline.get<tensor_size_t>("max-dims");
        const auto trials = cmdline.get<size_t>("trials");
        const auto epochs = cmdline.get<size_t>("epochs");
        const auto epoch_size = cmdline.get<size_t>("epoch-size");
        const auto tune_epochs = cmdline.get<size_t>("tune-epochs");
        const auto epsilon = cmdline.get<scalar_t>("epsilon");
        const auto is_convex = cmdline.has("convex");

        const auto solvers = get_stoch_solvers().ids(std::regex(cmdline.get<string_t>("solvers")));
        const auto names = std::regex(cmdline.get<string_t>("functions"));

        std::map<std::string, benchmark::solver_stat_t> gstats;

        const auto functions = (is_convex ? get_convex_functions : get_functions)(min_dims, max_dims, names);
        for (const auto& function : functions)
        {
                check_function(*function, solvers, trials, epochs, tune_epochs, epoch_size, epsilon, gstats);
        }

        // show global statistics
        benchmark::show_table(std::string(), gstats);

        // OK
        return EXIT_SUCCESS;
}
