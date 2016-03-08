#pragma once

#include "arch.h"

namespace thread
{
        ///
        /// \brief the number of threads available on the system (usually #CPU cores x 2 (HT))
        ///
        ZOB_PUBLIC unsigned int n_threads();
}
