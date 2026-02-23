#ifdef JIT_ENABLED

#if defined(__x86_64__)
#include "ARMJIT_x64/ARMJIT_Branch.cpp"
#elif defined(__aarch64__)
#include "ARMJIT_A64/ARMJIT_Branch.cpp"
#else
#error "The current target platform doesn't have a JIT backend"
#endif

#endif
