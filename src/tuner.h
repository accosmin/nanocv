#pragma once

#include "arch.h"
#include <random>
#include "scalar.h"
#include "stringi.h"

namespace nano
{
        ///
        /// \brief hyper-parameter tuning utility.
        ///
        class NANO_PUBLIC tuner_t
        {
        public:

                enum param_type
                {
                        linear,         ///< linear scale
                        base10,         ///< power of 10s scale
                        finite,         ///< list of finite values
                };

                ///
                /// \brief hyper-parameter description.
                ///
                struct param_t
                {
                        param_t() = default;
                        param_t(const char* name, const scalar_t min, const scalar_t max, const param_type type) :
                                m_name(name), m_min(min), m_max(max), m_type(type) {}
                        param_t(const char* name, scalars_t&& values) :
                                m_name(name), m_values(std::move(values)), m_type(param_type::finite) {}

                        // attributes
                        const char*     m_name{nullptr};
                        scalar_t        m_min{0};
                        scalar_t        m_max{0};
                        scalars_t       m_values;
                        param_type      m_type{param_type::linear};
                };

                ///
                /// \brief
                ///
                struct trial_t
                {
                        // attributes
                        scalars_t       m_values;       ///< value for each parameter
                        size_t          m_depth{1};     ///< number of refinement steps from the original configuration
                        scalar_t        m_score{0};     ///< score (aka goodness) of the configuration
                };

                ///
                /// \brief constructor
                ///
                tuner_t();

                ///
                /// \brief add a new hyper-parameter to tune
                ///
                void add_linear(const char* name, const scalar_t min, const scalar_t max)
                {
                        m_params.emplace_back(name, min, max, param_type::linear);
                }
                void add_base10(const char* name, const scalar_t min, const scalar_t max)
                {
                        m_params.emplace_back(name, min, max, param_type::base10);
                }
                void add_finite(const char* name, scalars_t values)
                {
                        m_params.emplace_back(name, std::move(values));
                }

                ///
                /// \brief retrive a JSON configuration of hyper-parameters to evaluate
                ///
                string_t get();

                ///
                /// \brief assign a score to the last evaluated configuration
                ///
                void score(const scalar_t score);

                ///
                /// \brief retrieve the best configuration so far
                ///
                string_t optimum() const;

        private:

                string_t json(const trial_t& trial) const;

                // attributes
                std::minstd_rand        m_rng;
                std::vector<param_t>    m_params;       ///< hyper-parameter description
                std::vector<trial_t>    m_trials;       ///< hyper-parameter trials evaluated so far
        };
}
