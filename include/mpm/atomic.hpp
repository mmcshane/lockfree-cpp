#pragma once

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
    #include "mpm/atomic_gcc.hpp"
#else
    error Atomic operations not defined for this compiler
#endif

