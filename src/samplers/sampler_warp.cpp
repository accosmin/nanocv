#include "vision/warp.h"
#include "sampler_warp.h"
#include "text/to_params.h"
#include "text/from_params.h"

namespace nano
{
        sampler_warp_t::sampler_warp_t(const string_t& config) :
                sampler_t(to_params(config,
                "type", to_string(warp_type::mixed) + "[" + concatenate(enum_values<warp_type>(warp_type::mixed)) + "]",
                "noise", "0.1[0,1]",
                "sigma", "4.0[0,10]",
                "alpha", "1.0[0,10]",
                "beta", "1.0[0,10]"))
        {
        }

        tensor3d_t sampler_warp_t::input(const task_t& task, const fold_t& fold, const size_t index) const
        {
                const auto wtype = from_params<warp_type>(config(), "type");
                const auto noise = from_params<scalar_t>(config(), "noise");
                const auto sigma = from_params<scalar_t>(config(), "sigma");
                const auto alpha = from_params<scalar_t>(config(), "alpha");
                const auto beta = from_params<scalar_t>(config(), "beta");

                tensor3d_t iodata = task.input(fold, index);
                warp(iodata, wtype, noise, sigma, alpha, beta);
                return iodata;
        }

        tensor3d_t sampler_warp_t::target(const task_t& task, const fold_t& fold, const size_t index) const
        {
                return task.target(fold, index);
        }
}