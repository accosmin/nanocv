#pragma once

#include "layer.h"

namespace nano
{
        ///
        /// \brief fully-connected affine layer that works with 1D tensors (as in MLP models).
        ///
        /// parameters:
        ///     dims    - number of output dimensions
        ///
        struct affine_layer_t final : public layer_t
        {
                explicit affine_layer_t(const string_t& params = string_t());

                virtual rlayer_t clone() const override;
                virtual void configure(const tensor3d_dims_t&, const string_t& name) override;
                virtual void output(tensor3d_const_map_t, tensor1d_const_map_t, tensor3d_map_t) override;
                virtual void ginput(tensor3d_map_t, tensor1d_const_map_t, tensor3d_const_map_t) override;
                virtual void gparam(tensor3d_const_map_t, tensor1d_map_t, tensor3d_const_map_t) override;

                virtual tensor3d_dims_t idims() const override { return m_idims; }
                virtual tensor3d_dims_t odims() const override { return m_odims; }
                virtual tensor_size_t fanin() const override;
                virtual tensor_size_t psize() const override { return m_psize; }
                virtual const probe_t& probe_output() const override { return m_probe_output; }
                virtual const probe_t& probe_ginput() const override { return m_probe_ginput; }
                virtual const probe_t& probe_gparam() const override { return m_probe_gparam; }

        private:

                tensor_size_t wsize() const { return osize() * isize(); }

                template <typename tmap>
                auto wdata(tmap param) const { return map_matrix(param.data(), osize(), isize()); }
                template <typename tmap>
                auto bdata(tmap param) const { return map_vector(param.data() + wsize(), osize()); }

                // attributes
                tensor3d_dims_t m_idims;
                tensor3d_dims_t m_odims;
                tensor_size_t   m_psize;
                probe_t         m_probe_output;
                probe_t         m_probe_ginput;
                probe_t         m_probe_gparam;
        };
}