#pragma once

// export symbols in shared libraries
#if defined _WIN32 || defined __CYGWIN__
        #ifdef __GNUC__
                #define NANO_PUBLIC __attribute__ ((dllexport))
        #else
                #define NANO_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
        #endif
        #define NANO_PRIVATE
#else
        #if __GNUC__ >= 4
                #define NANO_PUBLIC __attribute__ ((visibility ("default")))
                #define NANO_PRIVATE  __attribute__ ((visibility ("hidden")))
        #else
                #define NANO_PUBLIC
                #define NANO_PRIVATE
        #endif
#endif

// fix "unused variable" warnings
#define NANO_UNUSED1(x) (void)(x)
#define NANO_UNUSED2(x, y) NANO_UNUSED1(x); NANO_UNUSED1(y)
#define NANO_UNUSED3(x, y, z) NANO_UNUSED1(x); NANO_UNUSED1(y); NANO_UNUSED1(z)

// fix "unused variable" warnings (only for release mode)
#ifdef NANO_DEBUG
        #define NANO_UNUSED1_RELEASE(x)
#else
        #define NANO_UNUSED1_RELEASE(x) NANO_UNUSED1(x)
#endif

// string a given variable
#define NANO_STRINGIFY_(x) #x
#define NANO_STRINGIFY(x) NANO_STRINGIFY_(x)
