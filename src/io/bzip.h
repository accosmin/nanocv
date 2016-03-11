#pragma once

#include "buffer.h"

namespace nano
{
        class imstream_t;

        ///
        /// \brief uncompress a stream of bytes (using bzip2)
        ///
        NANO_PUBLIC bool uncompress_bzip2(std::istream& istream, std::streamsize num_bytes, buffer_t&);
        NANO_PUBLIC bool uncompress_bzip2(std::istream& istream, buffer_t&);
        NANO_PUBLIC bool uncompress_bzip2(imstream_t& istream, buffer_t&);
}
