#include <mutex>
#include "layers/layer_affine.h"
#include "layers/layer_normalize.h"
#include "layers/layer_activation.h"
#include "layers/layer_convolution.h"

using namespace nano;

layer_t::layer_t(const string_t& config) :
        configurable_t(config)
{
}

void layer_t::param(const tensor1d_cmap_t& pdata)
{
        assert(pdata.dims() == pdims());

        m_pdata = pdata;
        m_gdata.resize(pdims());
        m_gdata.zero();
}

const tensor4d_t& layer_t::output(const tensor4d_t& idata)
{
        const auto count = idata.size<0>();
        const auto omaps = std::get<0>(odims());
        const auto orows = std::get<1>(odims());
        const auto ocols = std::get<2>(odims());

        assert(count > 0);
        assert(idims() == idata.tensor(0).dims());

        m_idata = idata;
        m_odata.resize(count, omaps, orows, ocols);
        output(m_idata, m_pdata, m_odata);
        return m_odata;
}

const tensor4d_t& layer_t::ginput(const tensor4d_t& odata)
{
        assert(m_odata.dims() == odata.dims());

        ginput(m_idata, m_pdata, odata);
        return m_idata;
}

const tensor1d_t& layer_t::gparam(const tensor4d_t& odata)
{
        assert(m_odata.dims() == odata.dims());

        gparam(m_idata, m_gdata, odata);
        return m_gdata;
}

layer_factory_t& nano::get_layers()
{
        static layer_factory_t manager;

        static std::once_flag flag;
        std::call_once(flag, [] ()
        {
                manager.add<activation_layer_unit_t>("act-unit",      "activation: a(x) = x");
                manager.add<activation_layer_sine_t>("act-sin",       "activation: a(x) = sin(x)");
                manager.add<activation_layer_tanh_t>("act-tanh",      "activation: a(x) = tanh(x)");
                manager.add<activation_layer_splus_t>("act-splus",    "activation: a(x) = log(1 + e^x)");
                manager.add<activation_layer_snorm_t>("act-snorm",    "activation: a(x) = x / sqrt(1 + x^2)");
                manager.add<activation_layer_sigm_t>("act-sigm",      "activation: a(x) = exp(x) / (1 + exp(x))");
                manager.add<activation_layer_pwave_t>("act-pwave",    "activation: a(x) = x / (1 + x^2)");
                manager.add<affine_layer_t>("affine",                 "transform:  L(x) = A * x + b");
                manager.add<normalize_layer_t>("normalize",           "zero-mean & one-variance transformation");
                manager.add<convolution_layer_t>("conv",              "transform:  L(x) = conv3D(x, kernel) + b");
        });

        return manager;
}
