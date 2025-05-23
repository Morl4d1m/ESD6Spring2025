/*
 *  Copyright (c) 2003-2010, Mark Borgerding. All rights reserved.
 *  This file is part of KISS FFT - https://github.com/mborgerding/kissfft
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  See COPYING file for more information.
 */

#ifndef KISS_FFT_H
#define KISS_FFT_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

/*
 *  Force all dynamic allocations inside KissFFT into Teensy PSRAM
 */
extern void* extmem_malloc(size_t size);
extern void  extmem_free(void* ptr);
#define KISS_FFT_MALLOC extmem_malloc
#define KISS_FFT_FREE   extmem_free

/*
 *  Undef tmp alloc/free macros before overriding to avoid redefinition warnings
 */
#undef KISS_FFT_TMP_MALLOC
#undef KISS_FFT_TMP_FREE

/*
 *  Provide no‑ops for the alignment macros so that
 *  kiss_fft.c, kiss_fftnd.c, kiss_fftr.c, etc. compile cleanly.
 */
#ifndef KISS_FFT_ALIGN_CHECK
#  define KISS_FFT_ALIGN_CHECK(ptr)       /* no-op */
#endif
#ifndef KISS_FFT_ALIGN_SIZE_UP
#  define KISS_FFT_ALIGN_SIZE_UP(size)    (size)
#endif

// Define KISS_FFT_SHARED macro to properly export symbols
#ifdef KISS_FFT_SHARED
# ifdef _WIN32
#  ifdef KISS_FFT_BUILD
#   define KISS_FFT_API __declspec(dllexport)
#  else
#   define KISS_FFT_API __declspec(dllimport)
#  endif
# else
#  define KISS_FFT_API __attribute__ ((visibility ("default")))
# endif
#else
# define KISS_FFT_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_SIMD
# include <xmmintrin.h>
# define kiss_fft_scalar __m128
# ifndef KISS_FFT_MALLOC
#  define KISS_FFT_MALLOC(nbytes) _mm_malloc(nbytes,16)
#  define KISS_FFT_ALIGN_CHECK(ptr)
#  define KISS_FFT_ALIGN_SIZE_UP(size) ((size + 15UL) & ~0xFUL)
# endif
# ifndef KISS_FFT_FREE
#  define KISS_FFT_FREE _mm_free
# endif
#else
/* Already defined above for PSRAM, but ensure no-op defaults */
# ifndef KISS_FFT_ALIGN_CHECK
#  define KISS_FFT_ALIGN_CHECK(ptr)
# endif
# ifndef KISS_FFT_ALIGN_SIZE_UP
#  define KISS_FFT_ALIGN_SIZE_UP(size) (size)
# endif
# ifndef KISS_FFT_MALLOC
#  define KISS_FFT_MALLOC malloc
# endif
# ifndef KISS_FFT_FREE
#  define KISS_FFT_FREE free
# endif
#endif

#ifdef FIXED_POINT
# include <stdint.h>
# if (FIXED_POINT == 32)
#  define kiss_fft_scalar int32_t
# else  
#  define kiss_fft_scalar int16_t
# endif
#else
# ifndef kiss_fft_scalar
/* default is float */
#  define kiss_fft_scalar float
# endif
#endif

/** Complex data type **/
typedef struct {
    kiss_fft_scalar r;
    kiss_fft_scalar i;
} kiss_fft_cpx;

/* Single global scratch buffer for in-place FFT (
   must be allocated by the application before any FFT calls) */
extern kiss_fft_cpx* kissfft_scratch;

/* Redirect all internal temp alloc/free to the single global scratch */
#define KISS_FFT_TMP_MALLOC(nbytes)  (kissfft_scratch)
#define KISS_FFT_TMP_FREE(ptr)       /* no-op */

/** Opaque FFT config handle **/
typedef struct kiss_fft_state* kiss_fft_cfg;

/**
 *  kiss_fft_alloc
 *  Initialize a FFT (or IFFT) algorithm's cfg/state buffer.
 */
kiss_fft_cfg KISS_FFT_API kiss_fft_alloc(int nfft,
                                         int inverse_fft,
                                         void* mem,
                                         size_t* lenmem);

/**
 * Perform a complex FFT or IFFT.
 */
void KISS_FFT_API kiss_fft(kiss_fft_cfg cfg,
                           const kiss_fft_cpx* fin,
                           kiss_fft_cpx* fout);

/**
 * Generic version with stride
 */
void KISS_FFT_API kiss_fft_stride(kiss_fft_cfg cfg,
                                  const kiss_fft_cpx* fin,
                                  kiss_fft_cpx* fout,
                                  int fin_stride);

/**
 * Free a cfg buffer allocated by kiss_fft_alloc
 */
#define kiss_fft_free KISS_FFT_FREE

/**
 * Optional cleanup (flush cached twiddles, etc.)
 */
void KISS_FFT_API kiss_fft_cleanup(void);

/**
 * Returns the smallest integer k ≥ n with factors only 2, 3, or 5
 */
int KISS_FFT_API kiss_fft_next_fast_size(int n);

/**
 * For real FFTs, next fast even size
 */
#define kiss_fftr_next_fast_size_real(n) \
        (kiss_fft_next_fast_size((((n)+1)>>1))<<1)

#ifdef __cplusplus
}
#endif

#endif // KISS_FFT_H
