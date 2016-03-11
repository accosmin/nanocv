#pragma once

#include "cortex/task.h"

namespace nano
{
        ///
        /// MNIST task:
        ///      - digit classification
        ///      - 28x28 grayscale images as inputs
        ///      - 10 outputs (10 labels)
        ///
        /// http://yann.lecun.com/exdb/mnist/
        ///
        class mnist_task_t : public task_t
        {
        public:

                NANO_MAKE_CLONABLE(mnist_task_t, "MNIST (digit classification)")

                // constructor
                explicit mnist_task_t(const string_t& configuration = string_t());

                // load images from the given directory
                virtual bool load(const string_t& dir) override;

                // access functions
                virtual tensor_size_t irows() const override { return 28; }
                virtual tensor_size_t icols() const override { return 28; }
                virtual tensor_size_t osize() const override { return 10; }
                virtual size_t fsize() const override { return 1; }
                virtual color_mode color() const override { return color_mode::luma; }

        private:

                // load binary file
                bool load(const string_t& ifile, const string_t& gfile, protocol p, size_t count);
        };
}
