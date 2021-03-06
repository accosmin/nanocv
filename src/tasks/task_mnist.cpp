#include "task_mnist.h"
#include "core/logger.h"
#include "core/archive.h"

using namespace nano;

namespace
{
        template <mnist_type ttype>
        const char* name()
        {
                switch (ttype)
                {
                case mnist_type::digits:        return "MNIST";
                case mnist_type::fashion:       return "Fashion-MNIST";
                default:                        return "???";
                }
        }

        template <mnist_type ttype>
        const char* dirname()
        {
                switch (ttype)
                {
                case mnist_type::digits:        return "/experiments/databases/mnist";
                case mnist_type::fashion:       return "/experiments/databases/fashion-mnist";
                default:                        return "???";
                }
        }

        template <mnist_type ttype>
        strings_t labels()
        {
                switch (ttype)
                {
                case mnist_type::digits:        return
                        {
                                "digit0", "digit1", "digit2", "digit3", "digit4",
                                "digit5", "digit6", "digit7", "digit8", "digit9"
                        };
                case mnist_type::fashion:       return
                        {
                                "T-shirt/top", "Trouser", "Pullover", "Dress", "Coat",
                                "Sandal", "Shirt", "Sneaker", "Bag", "Ankle boot"
                        };
                default:                        assert(false); return {};
                }
        }
}

template <mnist_type ttype>
base_mnist_task_t<ttype>::base_mnist_task_t() :
        mem_vision_task_t(make_dims(1, 28, 28), make_dims(10, 1, 1), 10),
        m_dir(string_t(std::getenv("HOME")) + dirname<ttype>())
{
}

template <mnist_type ttype>
void base_mnist_task_t<ttype>::from_json(const json_t& json)
{
        nano::from_json(json, "dir", m_dir, "folds", m_folds);
        reconfig(make_dims(1, 28, 28), make_dims(10, 1, 1), m_folds);
}

template <mnist_type ttype>
void base_mnist_task_t<ttype>::to_json(json_t& json) const
{
        nano::to_json(json, "dir", m_dir, "folds", m_folds);
}

template <mnist_type ttype>
bool base_mnist_task_t<ttype>::populate()
{
        const auto test_ifile = m_dir + "/t10k-images-idx3-ubyte.gz";
        const auto test_gfile = m_dir + "/t10k-labels-idx1-ubyte.gz";

        const auto train_ifile = m_dir + "/train-images-idx3-ubyte.gz";
        const auto train_gfile = m_dir + "/train-labels-idx1-ubyte.gz";

        return  load_binary(train_ifile, train_gfile, protocol::train, 60000) &&
                load_binary(test_ifile, test_gfile, protocol::test, 10000);
}

template <mnist_type ttype>
bool base_mnist_task_t<ttype>::load_binary(const string_t& ifile, const string_t& gfile, const protocol p, const size_t count)
{
        const auto chunk_begin = n_chunks();
        const auto sample_begin = size();

        const auto irows = std::get<1>(idims());
        const auto icols = std::get<2>(idims());
        const auto buffer_size = irows * icols;

        std::vector<char> buffer(static_cast<size_t>(buffer_size));
        char label[2];

        const auto error_op = [&] (const string_t& message)
        {
                log_error() << name<ttype>() << ": " << message;
        };

        // load images
        const auto iop = [&] (const string_t&, istream_t& stream)
        {
                if (stream.read(buffer.data(), 16) != 16)
                {
                        return false;
                }

                while (stream.read(buffer.data(), buffer_size) == buffer_size)
                {
                        image_t image;
                        image.load_luma(buffer.data(), irows, icols);
                        add_chunk(image, image.hash());
                }

                return true;
        };

        log_info() << name<ttype>() << ": loading file <" << ifile << "> ...";
        if (!nano::load_archive(ifile, iop, error_op))
        {
                log_error() << name<ttype>() << ": failed to load file <" << ifile << ">!";
                return false;
        }

        // load ground truth
        const auto gop = [&] (const string_t&, istream_t& stream)
        {
                if (stream.read(buffer.data(), 8) != 8)
                {
                        return false;
                }

                std::vector<tensor_size_t> ilabels;
                while (stream.read(label, 1) == 1)
                {
                        const auto ilabel = static_cast<tensor_size_t>(label[0]);
                        if (ilabel < 0 || ilabel >= nano::size(odims()))
                        {
                                log_error() << name<ttype>() << ": invalid label!";
                                return false;
                        }
                        ilabels.push_back(ilabel);
                }
                if (ilabels.size() != count)
                {
                        log_error() << name<ttype>() << ": invalid number of labels!";
                        return false;
                }

                // generate folds
                add_samples(p, ilabels, ::labels<ttype>());
                return true;
        };

        log_info() << name<ttype>() << ": loading file <" << gfile << "> ...";
        if (!nano::load_archive(gfile, gop, error_op))
        {
                log_error() << name<ttype>() << ": failed to load file <" << gfile << ">!";
                return false;
        }

        // OK
        log_info() << name<ttype>() << ": loaded " << (n_chunks() - chunk_begin) << " samples.";
        return  n_chunks() == chunk_begin + count &&
                size() == sample_begin + count * m_folds;
}

template class nano::base_mnist_task_t<mnist_type::digits>;
template class nano::base_mnist_task_t<mnist_type::fashion>;
