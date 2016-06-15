#pragma once

#include "function.h"

namespace nano
{
        ///
        /// \brief create sum of squares test functions
        ///
        struct NANO_PUBLIC function_sum_squares_t : public function_t
        {
                explicit function_sum_squares_t(const tensor_size_t dims);

                virtual std::string name() const override;
                virtual problem_t problem() const override;
                virtual bool is_valid(const vector_t& x) const override;
                virtual bool is_minima(const vector_t& x, const scalar_t epsilon) const override;

                tensor_size_t   m_dims;
                vector_t        m_weights;
        };
}