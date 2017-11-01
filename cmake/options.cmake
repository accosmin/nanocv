option(NANO_WITH_ASAN                   "build with address sanitizers suppport (if available)"         OFF)
option(NANO_WITH_MSAN                   "build with memory sanitizer suppport (if available)"           OFF)
option(NANO_WITH_TSAN                   "build with thread sanitizer suppport (if available)"           OFF)
option(NANO_WITH_TESTS                  "build the unit tests"                                          ON)
option(NANO_WITH_LIBCPP                 "use libc++ instead of default libstdc++ (if applicable)"       OFF)
option(NANO_WITH_GOLD                   "use gold linker (if available)"                                OFF)
option(NANO_WITH_LTO                    "use link time optimization (if available)"                     OFF)
option(NANO_WITH_WERROR                 "stop compilation at first warning"                             OFF)
option(NANO_WITH_CCACHE                 "use ccache (if available)"                                     OFF)
option(NANO_WITH_FLOAT_SCALAR           "use float as the default scalar"                               ON)
option(NANO_WITH_DOUBLE_SCALAR          "use double as the default scalar"                              OFF)
option(NANO_WITH_LONG_DOUBLE_SCALAR     "use long double as the default scalar"                         OFF)
option(NANO_WITH_TIME_REPORT            "report detailed compilation time"                              OFF)
option(NANO_WITH_CLANG_TIDY             "create clang-tidy target for static analysis"                  OFF)
option(NANO_WITH_TUNE_NATIVE            "tune for the native platform (-mtune=native -march=native)"    ON)
