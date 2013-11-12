#ifndef NANOCV_TENSOR_TENSOR4D_HPP
#define NANOCV_TENSOR_TENSOR4D_HPP

#include "core/tensor/storage.hpp"

namespace ncv
{
        /////////////////////////////////////////////////////////////////////////////////////////
        // 4D tensor:
        //      - 1D/2D collection of fixed size matrices
        /////////////////////////////////////////////////////////////////////////////////////////

        namespace tensor
        {
                template
                <
                        typename tscalar,
                        typename tsize
                >
                class tensor4d_t : public tensor::storage_t<tscalar, tsize>
                {
                public:

                        typedef tensor::storage_t<tscalar, tsize>       base_t;
                        typedef typename base_t::matrix_t               matrix_t;

                        // constructor
                        tensor4d_t(tsize dim1 = 0, tsize dim2 = 0, tsize rows = 0, tsize cols = 0)
                        {
                                resize(dim1, dim2, rows, cols);
                        }

                        // resize to new dimensions
                        tsize resize(tsize dim1, tsize dim2, tsize rows, tsize cols)
                        {
                                m_dim1 = dim1;
                                m_dim2 = dim2;
                                return base_t::allocate(dim1 * dim2, rows, cols);
                        }

                        // access functions
                        tsize n_dim1() const { return m_dim1; }
                        tsize n_dim2() const { return m_dim2; }

                        const matrix_t& operator()(size_t d1, size_t d2) const
                        {
                                return base_t::get(d1 * n_dim2() + d2);
                        }
                        matrix_t& operator()(size_t d1, size_t d2)
                        {
                                return base_t::get(d1 * n_dim2() + d2);
                        }

                        const tscalar& operator()(tsize d1, tsize d2, size_t r, size_t c) const
                        {
                                return base_t::get(d1, d2)(r, c);
                        }
                        tscalar& operator()(tsize d1, tsize d2, size_t r, size_t c)
                        {
                                return base_t::get(d1, d2)(r, c);
                        }

                private:

                        // serialize
                        friend class boost::serialization::access;
                        template
                        <
                                class tarchive
                        >
                        void serialize(tarchive & ar, const unsigned int version)
                        {
                                ar & boost::serialization::base_object<matrix_t>(*this);
                                ar & m_dim1;
                                ar & m_dim2;
                        }

                private:

                        // attributes
                        tsize           m_dim1; // #dimension 1
                        tsize           m_dim2; // #dimension 2
                };
        }
}

namespace boost
{
        namespace serialization
        {
                /////////////////////////////////////////////////////////////////////////////////////////
                // serialize 4D tensor
                /////////////////////////////////////////////////////////////////////////////////////////

                template
                <
                        class tarchive,
                        class tscalar,
                        class tsize
                >
                void serialize(tarchive& ar, ncv::tensor::tensor4d_t<tscalar, tsize>& t4, const unsigned int version)
                {
                        ar & t4;
                }
        }
}

#endif // NANOCV_TENSOR_TENSOR4D_HPP
