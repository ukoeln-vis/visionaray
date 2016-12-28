// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

//--------------------------------------------------------------------------------------------------
// Detect architecture
//

#define VSNRAY_ARCH_UNKNOWN          0
#define VSNRAY_ARCH_X86             10
#define VSNRAY_ARCH_X86_64          11
#define VSNRAY_ARCH_ARM             20
#define VSNRAY_ARCH_ARM64           21

#if defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
// SSE2 is always available on 64-bit platforms
#define VSNRAY_ARCH VSNRAY_ARCH_X86_64
#define VSNRAY_SIMD_ON_64_BIT 1
#elif defined(__arm__) || defined(__arm) || defined(_ARM) || defined(_M_ARM)
#define VSNRAY_ARCH VSNRAY_ARCH_ARM
#define VSNRAY_SIMD_ON_64_BIT 0
#elif defined(__aarch64__)
#define VSNRAY_ARCH VSNRAY_ARCH_ARM64
#define VSNRAY_SIMD_ON_64_BIT 1
#endif

//--------------------------------------------------------------------------------------------------
// Detect instruction set
//

#define VSNRAY_SIMD_ISA_SSE         10
#define VSNRAY_SIMD_ISA_SSE2        20
#define VSNRAY_SIMD_ISA_SSE3        30
#define VSNRAY_SIMD_ISA_SSSE3       31
#define VSNRAY_SIMD_ISA_SSE4_1      41
#define VSNRAY_SIMD_ISA_SSE4_2      42
#define VSNRAY_SIMD_ISA_AVX         50
#define VSNRAY_SIMD_ISA_AVX2        60
#define VSNRAY_SIMD_ISA_AVX512F     70

#ifndef VSNRAY_SIMD_ISA
#if defined(__AVX512F__)                            && !defined(__CUDACC__) // nvcc does not support AVX intrinsics
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_AVX512F
#elif defined(__AVX2__)                             && !defined(__CUDACC__)
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_AVX2
#elif defined(__AVX__)                              && !defined(__CUDACC__)
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_AVX
#elif defined(__SSE4_2__)
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_SSE4_2
#elif defined(__SSE4_1__)
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_SSE4_1
#elif defined(__SSSE3__)
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_SSSE3
#elif defined(__SSE3__)
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_SSE3
#elif defined(__SSE2__) || defined(VSNRAY_SIMD_ON_64_BIT)
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_SSE2
#else
#define VSNRAY_SIMD_ISA 0
#endif
#endif

// Intel Short Vector Math Library available?
#ifndef VSNRAY_SIMD_HAS_SVML
#if defined(__INTEL_COMPILER)
#define VSNRAY_SIMD_HAS_SVML 1
#endif
#endif

//--------------------------------------------------------------------------------------------------
// SIMD intrinsic #include's
//

#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_SSE2
#include <emmintrin.h>
#endif
#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_SSE3
#include <pmmintrin.h>
#endif
#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_SSSE3
#include <tmmintrin.h>
#endif
#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_SSE4_1
#include <smmintrin.h>
#endif
#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_SSE4_2
#include <nmmintrin.h>
#endif
#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_AVX
#include <immintrin.h>
#endif
