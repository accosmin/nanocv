#pragma once

#include "task_mem.h"
#include "core/hash.h"
#include "core/image.h"

namespace nano
{
        struct mem_vision_sample_t
        {
                mem_vision_sample_t() = default;

                mem_vision_sample_t(
                        const size_t index,
                        const vector_t& target,
                        string_t label = string_t(),
                        const rect_t& region = rect_t()) :
                        m_index(index),
                        m_region(region),
                        m_target(nano::map_tensor(target.data(), target.size(), 1, 1)),
                        m_label(std::move(label))
                {
                }

                auto index() const { return m_index; }
                tensor3d_t input(const image_t& image) const;
                auto output() const { return m_target; }
                auto label() const { return m_label; }

                size_t ohash() const;
                size_t ihash(size_t seed) const;

                // attributes
                size_t          m_index{0};     ///< image index
                rect_t          m_region;       ///< patch region in image
                tensor3d_t      m_target;       ///<
                string_t        m_label;        ///<
        };

        inline tensor3d_t mem_vision_sample_t::input(const image_t& image) const
        {
                return m_region.empty() ? image.to_tensor() : image.to_tensor(m_region);
        }

        inline size_t mem_vision_sample_t::ihash(size_t seed) const
        {
                std::hash<coord_t> hasher;
                nano::hash_combine(seed, m_region.top(), hasher);
                nano::hash_combine(seed, m_region.left(), hasher);
                nano::hash_combine(seed, m_region.right(), hasher);
                nano::hash_combine(seed, m_region.bottom(), hasher);
                return seed;
        }

        inline size_t mem_vision_sample_t::ohash() const
        {
                return nano::hash_range(m_target.data(), m_target.data() + m_target.size());
        }

        ///
        /// \brief in-memory generic computer vision task consisting of images and
        ///     fixed-size rectangular samples from these images.
        ///
        struct mem_vision_task_t : public mem_task_t<image_t, mem_vision_sample_t>
        {
                ///
                /// \brief constructor
                ///
                mem_vision_task_t(
                        const tensor3d_dim_t& idims,
                        const tensor3d_dim_t& odims,
                        const size_t fsize) :
                        mem_task_t<image_t, mem_vision_sample_t>(idims, odims, fsize)
                {
                }

                ///
                /// \brief constructor
                ///
                mem_vision_task_t(
                        const color_mode color, const tensor_size_t irows, const tensor_size_t icols,
                        const tensor3d_dim_t& odims,
                        const size_t fsize) :
                        mem_vision_task_t(
                                make_dims(color == color_mode::rgba ? 4 : (color == color_mode::rgb ? 3 : 1), irows, icols),
                                odims, fsize)
                {
                }

                ///
                /// \brief retrieve the number of images
                ///
                size_t n_images() const { return n_chunks(); }

                ///
                /// \brief retrieve the given image
                ///
                const image_t& image(const size_t index) const { return chunk(index); }

                ///
                /// \brief retrieve the color mode
                ///
                color_mode color() const
                {
                        switch (std::get<0>(idims()))
                        {
                        case 1:         return color_mode::luma;
                        case 3:         return color_mode::rgb;
                        case 4:         return color_mode::rgba;
                        default:        return color_mode::luma;
                        }
                }

                ///
                /// \brief reconfigure
                ///
                using mem_task_t<image_t, mem_vision_sample_t>::reconfig;
                void reconfig(
                        const color_mode color, const tensor_size_t irows, const tensor_size_t icols,
                        const tensor3d_dim_t& odims,
                        const size_t fsize)
                {
                        reconfig(
                                make_dims(color == color_mode::rgba ? 4 : (color == color_mode::rgb ? 3 : 1), irows, icols),
                                odims, fsize);
                }
        };
}
