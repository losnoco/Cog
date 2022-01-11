#ifndef _RARCH_CONFIG_H
#define _RARCH_CONFIG_H

#include <TargetConditionals.h>

#define HAVE_SSIZE_T 1
#define HAVE_STRL 1

#if TARGET_CPU_X86 || TARGET_CPU_X86_64
#define HAVE_SSE 1
#define __SSE__ 1
#define __SSE2__ 1
#define __AVX__ 1
#endif

#if TARGET_CPU_ARM || TARGET_CPU_ARM64
#define HAVE_NEON 1
#define __ARM_NEON__ 1
#endif

#endif

