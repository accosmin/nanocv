#pragma once

#include "arch.h"
#include "cortex.h"
#include "core/json.h"
#include "core/factory.h"

namespace nano
{
        class task_t;
        using task_factory_t = factory_t<task_t>;
        using rtask_t = task_factory_t::trobject;

        NANO_PUBLIC task_factory_t& get_tasks();

        ///
        /// \brief machine learning task consisting of a collection of fixed-size 3D input tensors
        ///     split into training, validation and testing datasets.
        /// NB: the samples may be organized in folds depending on the established protocol.
        ///
        class NANO_PUBLIC task_t : public json_configurable_t
        {
        public:

                ///
                /// \brief populate the task with samples
                ///
                virtual bool load() = 0;

                ///
                /// \brief input size
                ///
                virtual tensor3d_dim_t idims() const = 0;

                ///
                /// \brief output size
                ///
                virtual tensor3d_dim_t odims() const = 0;

                ///
                /// \brief number of folds
                ///
                virtual size_t fsize() const = 0;

                ///
                /// \brief total number of samples
                ///
                virtual size_t size() const = 0;

                ///
                /// \brief number of samples for the given fold
                ///
                virtual size_t size(const fold_t&) const = 0;

                ///
                /// \brief randomly shuffle the samples associated for the given fold
                ///
                virtual void shuffle(const fold_t&) const = 0;

                ///
                /// \brief retrieve the given [begin, end) range of samples as a minibatch
                ///
                virtual minibatch_t get(const fold_t&, const size_t begin, const size_t end) const = 0;

                ///
                /// \brief retrieve the hash for a given input/target
                ///
                virtual size_t ihash(const fold_t&, const size_t index) const = 0;
                virtual size_t ohash(const fold_t&, const size_t index) const = 0;

                ///
                /// \brief retrieve the label (if available) for a given sample
                ///
                virtual string_t label(const fold_t&, const size_t index) const = 0;

                ///
                /// \brief print a short description of the task
                ///
                void describe(const string_t& name) const;

                ///
                /// \brief returns the number of duplicated samples globally or for the given fold index
                ///
                size_t duplicates() const;
                size_t duplicates(const size_t fold_index) const;

                ///
                /// \brief returns the number of intersecting samples between training, validation and test datasets,
                ///     globally or for the given fold index
                ///
                size_t intersections() const;
                size_t intersections(const size_t fold_index) const;

                ///
                /// \brief returns the number of samples for each distinct label globally or for the given fold index
                ///
                std::map<string_t, size_t> labels() const;
                std::map<string_t, size_t> labels(const fold_t& fold) const;
        };
}
