#pragma once

#include <type_traits>
#include <eigen3/Eigen/Core>

namespace nano
{
        ///
        /// \brief matrix types
        ///
        template
        <
                typename tscalar_,
                int trows = Eigen::Dynamic,
                int tcols = Eigen::Dynamic,
                typename tscalar = typename std::remove_const<tscalar_>::type
        >
        using tensor_matrix_t = Eigen::Matrix<tscalar, trows, tcols, Eigen::RowMajor>;

        ///
        /// \brief map non-constant data to matrices
        ///
        template
        <
                int alignment = Eigen::Unaligned,
                typename tscalar_,
                typename tsize,
                typename tscalar = typename std::remove_const<tscalar_>::type,
                typename tresult = Eigen::Map<tensor_matrix_t<tscalar>, alignment>
        >
        tresult map_matrix(tscalar_* data, const tsize rows, const tsize cols)
        {
                return tresult(data, rows, cols);
        }

        ///
        /// \brief map constant data to matrices
        ///
        template
        <
                int alignment = Eigen::Unaligned,
                typename tscalar_,
                typename tsize,
                typename tscalar = typename std::remove_const<tscalar_>::type,
                typename tresult = Eigen::Map<const tensor_matrix_t<tscalar>, alignment>
        >
        tresult map_matrix(const tscalar_* data, const tsize rows, const tsize cols)
        {
                return tresult(data, rows, cols);
        }
}
