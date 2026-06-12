#ifndef KS_SQRT_H
#define KS_SQRT_H

/* ks_sqrt - kernel-friendly double sqrt implementation
 *
 * Provides:
 *   double ks_sqrt(double x);
 *
 * Semantics:
 * - IEEE-754 binary64 assumed.
 * - No dependency on math.h or libc.
 * - Handles NaN/Inf/zero/subnormal via IEEE-like bit tests.
 * - Negative nonzero inputs return a quiet NaN.
 *
 * Note: If you compile in a freestanding environment without stdint.h,
 * provide a uint64_t typedef (unsigned long long on many targets).
 */

/* Prefer <stdint.h> if available; otherwise expect user to provide uint64_t. */
#if defined(__has_include)
#  if __has_include(<stdint.h>)
#    include <stdint.h>
#  endif
#endif

#ifndef UINT64_DEFINED_BY_USER
#  ifndef uint64_t
     /* Fallback: assume unsigned long long is 64-bit */
     typedef unsigned long long uint64_t;
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Public function: compute square root of a double.
 * Implementation is in the corresponding .c file (ks_sqrt).
 * Name chosen to avoid clashing with libc's sqrt symbol in environments
 * where libc may also be linked.
 */
double ks_sqrt(double x);

/* Optional: raw bit helpers (redeclared here for convenience if needed by callers).
 * These macros match the implementation assumptions and can be used by callers
 * performing IEEE-754 bit-level work.
 */
#define KS_DBL_EXP_BITS 11
#define KS_DBL_MANT_BITS 52
#define KS_DBL_EXP_BIAS 1023
#define KS_DBL_SIGN_MASK 0x8000000000000000ULL
#define KS_DBL_EXP_MASK  0x7FF0000000000000ULL
#define KS_DBL_FRAC_MASK 0x000FFFFFFFFFFFFFULL

#ifdef __cplusplus
}
#endif

#endif /* KS_SQRT_H */
