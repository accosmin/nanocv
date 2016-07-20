#include "text/table.h"
#include "text/cmdline.h"
#include "cortex/cortex.h"
#include "cortex/logger.h"
#include "cortex/measure.hpp"
#include "text/to_params.hpp"
#include "optim/batch/types.h"
#include "optim/stoch/types.h"
#include "cortex/tasks/task_charset.h"
#include "cortex/layers/make_layers.h"

using namespace nano;

size_t gmin_threads = 1;
size_t gmax_threads = 1;

template
<
        typename ttrainer
>
static void evaluate_trainer(model_t& model, const string_t& name,
        table_t& table, const vector_t& x0, const ttrainer& trainer)
{
        auto& row = table.append(name);
        for (size_t nthreads = gmin_threads; nthreads <= gmax_threads; ++ nthreads)
        {
                model.load_params(x0);

                log_info() << "<<< running " << name << " with " << nthreads << " threads...";

                const nano::timer_t timer;

                const auto result = trainer(nthreads);
                NANO_UNUSED1(result);

                row << timer.seconds().count();
        }
}

static void evaluate(model_t& model,
        const task_t& task, const size_t fold,
        const loss_t& loss, const criterion_t& criterion, const vector_t& x0, const size_t epochs,
        const std::vector<batch_optimizer>& batch_optimizers,
        const std::vector<stoch_optimizer>& stochastic_optimizers,
        const string_t& basename, table_t& table)
{
        const auto policy = trainer_policy::all_epochs;

        for (auto optimizer : batch_optimizers)
        {
                const auto optname = "batch-" + to_string(optimizer);
                const auto params = to_params("opt", optimizer, "epochs", epochs, "policy", policy);
                evaluate_trainer(model, basename + optname, table, x0, [&] (const size_t nthreads)
                {
                        const auto trainer = get_trainers().get("batch", params);
                        return trainer->train(task, fold, nthreads, loss, criterion, model);
                });
        }

        for (auto optimizer : stochastic_optimizers)
        {
                const auto optname = "stochastic-" + to_string(optimizer);
                const auto params = to_params("opt", optimizer, "epochs", epochs, "policy", policy);
                evaluate_trainer(model, basename + optname, table, x0, [&] (const size_t nthreads)
                {
                        const auto trainer = get_trainers().get("stochastic", params);
                        return trainer->train(task, fold, nthreads, loss, criterion, model);
                });
        }
}

int main(int argc, const char* argv[])
{
        using namespace nano;

        // parse the command line
        cmdline_t cmdline("benchmark trainers");
        cmdline.add("", "mlps",                 "use MLPs with varying number of hidden layers");
        cmdline.add("", "mlp0",                 "use MLPs with 0 hidden layers");
        cmdline.add("", "mlp1",                 "use MLPs with 1 hidden layer");
        cmdline.add("", "mlp2",                 "use MLPs with 2 hidden layers");
        cmdline.add("", "mlp3",                 "use MLPs with 3 hidden layers");
        cmdline.add("", "convnets",             "use convolution networks with varying number of convolution layers");
        cmdline.add("", "convnet1",             "use convolution networks with 1 convolution layer");
        cmdline.add("", "convnet2",             "use convolution networks with 2 convolution layers");
        cmdline.add("", "convnet3",             "use convolution networks with 3 convolution layers");
        cmdline.add("", "batch",                "evaluate batch optimizers");
        cmdline.add("", "batch-gd",             "evaluate batch optimizer GD (gradient descent)");
        cmdline.add("", "batch-cgd",            "evaluate batch optimizer CGD (conjugate gradient descent)");
        cmdline.add("", "batch-lbfgs",          "evaluate batch optimizer LBFGS");
        cmdline.add("", "stochastic",           "evaluate stochastic optimizers");
        cmdline.add("", "stochastic-sg",        "evaluate stochastic optimizer SG (stochastic gradient)");
        cmdline.add("", "stochastic-ngd",       "evaluate stochastic optimizer NGS (normalized gradient descent)");
        cmdline.add("", "stochastic-sgm",       "evaluate stochastic optimizer SGM (stochastic gradient with momentum)");
        cmdline.add("", "stochastic-ag",        "evaluate stochastic optimizer AG (Nesterov's accelerated gradient)");
        cmdline.add("", "stochastic-agfr",      "evaluate stochastic optimizer AG (AG + function value restarts)");
        cmdline.add("", "stochastic-aggr",      "evaluate stochastic optimizer AG (AG + gradient restarts)");
        cmdline.add("", "stochastic-adam",      "evaluate stochastic optimizer ADAM");
        cmdline.add("", "stochastic-adagrad",   "evaluate stochastic optimizer ADAGRAD");
        cmdline.add("", "stochastic-adadelta",  "evaluate stochastic optimizer ADADELTA");
        cmdline.add("", "epochs",               "number of epochs", "100");
        cmdline.add("", "min-threads",          "minimum number of threads to use", 1);
        cmdline.add("", "max-threads",          "maximum number of threads to use", nano::logical_cpus());

        cmdline.process(argc, argv);

        // check arguments and options
        const bool use_mlps = cmdline.has("mlps");
        const bool use_mlp0 = cmdline.has("mlp0");
        const bool use_mlp1 = cmdline.has("mlp1");
        const bool use_mlp2 = cmdline.has("mlp2");
        const bool use_mlp3 = cmdline.has("mlp3");
        const bool use_convnets = cmdline.has("convnets");
        const bool use_convnet1 = cmdline.has("convnet1");
        const bool use_convnet2 = cmdline.has("convnet2");
        const bool use_convnet3 = cmdline.has("convnet3");
        const auto epochs = cmdline.get<size_t>("epochs");
        gmin_threads = cmdline.get<size_t>("min-threads");
        gmax_threads = cmdline.get<size_t>("max-threads");

        if (    !use_mlps &&
                !use_mlp0 &&
                !use_mlp1 &&
                !use_mlp2 &&
                !use_mlp3 &&
                !use_convnets &&
                !use_convnet1 &&
                !use_convnet2 &&
                !use_convnet3)
        {
                cmdline.usage();
        }

        std::vector<batch_optimizer> batch_optimizers;
        if (cmdline.has("batch") || cmdline.has("batch-gd")) batch_optimizers.push_back(batch_optimizer::GD);
        if (cmdline.has("batch") || cmdline.has("batch-cgd")) batch_optimizers.push_back(batch_optimizer::CGD);
        if (cmdline.has("batch") || cmdline.has("batch-lbfgs")) batch_optimizers.push_back(batch_optimizer::LBFGS);

        std::vector<stoch_optimizer> stochastic_optimizers;
        if (cmdline.has("stochastic") || cmdline.has("stochastic-sg")) stochastic_optimizers.push_back(stoch_optimizer::SG);
        if (cmdline.has("stochastic") || cmdline.has("stochastic-ngd")) stochastic_optimizers.push_back(stoch_optimizer::NGD);
        if (cmdline.has("stochastic") || cmdline.has("stochastic-sgm")) stochastic_optimizers.push_back(stoch_optimizer::SGM);
        if (cmdline.has("stochastic") || cmdline.has("stochastic-ag")) stochastic_optimizers.push_back(stoch_optimizer::AG);
        if (cmdline.has("stochastic") || cmdline.has("stochastic-agfr")) stochastic_optimizers.push_back(stoch_optimizer::AGFR);
        if (cmdline.has("stochastic") || cmdline.has("stochastic-aggr")) stochastic_optimizers.push_back(stoch_optimizer::AGGR);
        if (cmdline.has("stochastic") || cmdline.has("stochastic-adam")) stochastic_optimizers.push_back(stoch_optimizer::ADAM);
        if (cmdline.has("stochastic") || cmdline.has("stochastic-adagrad")) stochastic_optimizers.push_back(stoch_optimizer::ADAGRAD);
        if (cmdline.has("stochastic") || cmdline.has("stochastic-adadelta")) stochastic_optimizers.push_back(stoch_optimizer::ADADELTA);

        if (    batch_optimizers.empty() &&
                stochastic_optimizers.empty())
        {
                cmdline.usage();
        }

        // create task
        const size_t rows = 20;
        const size_t cols = 20;
        const size_t count = nano::logical_cpus() * 32 * 100;
        const color_mode color = color_mode::rgb;

        charset_task_t task(charset::digit, color, rows, cols, count);
        task.load();

        const size_t fold = 0;
        const auto outputs = task.osize();

        // construct models
        const auto activation = "act-snorm";
        const auto pooling = "pool-full";

        const auto mlp0 = string_t();
        const auto mlp1 = mlp0 + make_affine_layer(64, activation);
        const auto mlp2 = mlp1 + make_affine_layer(64, activation);
        const auto mlp3 = mlp2 + make_affine_layer(64, activation);

        const auto convnet1 =
                make_conv_pool_layer(32, 7, 7, 1, activation, pooling);

        const auto convnet2 =
                make_conv_pool_layer(32, 7, 7, 1, activation, pooling) +
                make_conv_layer(64, 5, 5, 4);

        const auto convnet3 =
                make_conv_layer(32, 7, 7, 1, activation) +
                make_conv_layer(64, 7, 7, 4, activation) +
                make_conv_layer(128, 5, 5, 8, activation);

        const string_t outlayer = make_output_layer(outputs, activation);

        std::vector<std::pair<string_t, string_t>> networks;
        if (use_mlps || use_mlp0) networks.emplace_back(mlp0 + outlayer, "mlp0");
        if (use_mlps || use_mlp1) networks.emplace_back(mlp1 + outlayer, "mlp1");
        if (use_mlps || use_mlp2) networks.emplace_back(mlp2 + outlayer, "mlp2");
        if (use_mlps || use_mlp3) networks.emplace_back(mlp3 + outlayer, "mlp3");
        if (use_convnets || use_convnet1) networks.emplace_back(convnet1 + outlayer, "convnet1");
        if (use_convnets || use_convnet2) networks.emplace_back(convnet2 + outlayer, "convnet2");
        if (use_convnets || use_convnet3) networks.emplace_back(convnet3 + outlayer, "convnet3");

        const strings_t losses = { "classnll" }; //get_losses().ids();
        const strings_t criteria = { "avg" }; //get_criteria().ids();

        table_t table("training time [s]");
        for (size_t nthreads = gmin_threads; nthreads <= gmax_threads; ++ nthreads)
        {
                table.header() << (to_string(nthreads) + "xCPU");
        }

        // vary the model
        for (const auto& net : networks)
        {
                const auto& network = net.first;
                const auto& netname = net.second;

                log_info() << "<<< running network [" << network << "] ...";

                const auto model = get_models().get("forward-network", network);
                model->resize(task, false);

                // generate fixed random starting point
                vector_t x0;
                model->random_params();
                model->save_params(x0);

                // vary the loss
                for (const string_t& iloss : losses)
                {
                        log_info() << "<<< running loss [" << iloss << "] ...";

                        const auto loss = get_losses().get(iloss);

                        // vary the criteria
                        for (const string_t& icriterion : criteria)
                        {
                                const auto criterion = get_criteria().get(icriterion);
                                const auto basename = netname + "-" + iloss + "-" + icriterion + "-";

                                evaluate(*model, task, fold, *loss, *criterion, x0, epochs,
                                         batch_optimizers, stochastic_optimizers,
                                         basename, table);
                        }
                }
        }

        // show results
        table.print(std::cout);

        // OK
        log_info() << done;
        return EXIT_SUCCESS;
}
