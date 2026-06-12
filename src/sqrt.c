#include <stdint.h>

/* IEEE-754 binary64 layout constants */
#define DBL_EXP_BITS 11
#define DBL_MANT_BITS 52
#define DBL_EXP_BIAS 1023
#define DBL_SIGN_MASK 0x8000000000000000ULL
#define DBL_EXP_MASK  0x7FF0000000000000ULL
#define DBL_FRAC_MASK 0x000FFFFFFFFFFFFFULL

static inline uint64_t make_qnan_payload(uint64_t payload) {
    payload &= DBL_FRAC_MASK;
    payload |= (1ULL << (DBL_MANT_BITS - 1)); /* set quiet-NaN bit */
    return payload;
}

static inline double bits_to_double(uint64_t u) {
    union { uint64_t u; double d; } v;
    v.u = u;
    return v.d;
}

static inline uint64_t double_to_bits(double d) {
    union { uint64_t u; double d; } v;
    v.d = d;
    return v.u;
}

/* Kernel-friendly double sqrt */
double ks_sqrt(double a) {
    uint64_t ux = double_to_bits(a);
    uint64_t sign = ux & DBL_SIGN_MASK;
    uint64_t exp  = ux & DBL_EXP_MASK;
    uint64_t frac = ux & DBL_FRAC_MASK;

    /* Preserve used variables to avoid -Werror unused warnings in builds that treat warnings as errors */
    (void)sign; (void)exp; (void)frac;

    /* NaN: exponent all ones and fraction nonzero -> return quiet NaN preserving payload if possible */
    if ((ux & DBL_EXP_MASK) == DBL_EXP_MASK) {
        if ((ux & DBL_FRAC_MASK) != 0) {
            uint64_t payload = ux & DBL_FRAC_MASK;
            uint64_t qnan = DBL_EXP_MASK | make_qnan_payload(payload);
            return bits_to_double(qnan);
        }
        /* Infinity: if negative, sqrt -> NaN; if positive, sqrt -> +Inf */
        if (ux & DBL_SIGN_MASK) {
            uint64_t qnan = DBL_EXP_MASK | (1ULL << (DBL_MANT_BITS - 1));
            return bits_to_double(qnan);
        } else {
            return bits_to_double(ux); /* +Inf */
        }
    }

    /* Negative (except -0): return quiet NaN */
    if ((ux & DBL_SIGN_MASK) && (ux & ~DBL_SIGN_MASK) != 0) {
        uint64_t qnan = DBL_EXP_MASK | (1ULL << (DBL_MANT_BITS - 1));
        return bits_to_double(qnan);
    }

    /* +0 or -0 -> return same zero (preserve sign) */
    if ((ux & ~DBL_SIGN_MASK) == 0) {
        return bits_to_double(ux);
    }

    /*
     * For positive normal/subnormal inputs compute sqrt:
     * - If subnormal, scale up by 2^54 to normalize, remember to scale down later.
     * - Make a bit-level initial approximation.
     * - Perform three Newton iterations (safe for full double precision).
     */

    int is_subnormal = ((ux & DBL_EXP_MASK) == 0);
    double x_in = a;
    if (is_subnormal) {
        /* Multiply by 2^54 to normalize subnormals */
        uint64_t pow54_bits = ((uint64_t)(DBL_EXP_BIAS + 54) << DBL_MANT_BITS);
        double pow54 = bits_to_double(pow54_bits);
        x_in = a * pow54;
    }

    /* Initial approximation: bit trick */
    union { double d; uint64_t u; } v;
    v.d = x_in;
    uint64_t approx = (v.u >> 1) + 0x1FF8000000000000ULL;
    v.u = approx;
    double x = v.d;

    /* Newton-Raphson iterations: x_{n+1} = 0.5 * (x_n + a / x_n) */
    x = 0.5 * (x + x_in / x);
    x = 0.5 * (x + x_in / x);
    x = 0.5 * (x + x_in / x); /* third iteration for safety/precision */

    /* If we scaled up a subnormal, scale down by sqrt(2^54) = 2^27 */
    if (is_subnormal) {
        int adjust = 27;
        uint64_t adjust_bits = ((uint64_t)(DBL_EXP_BIAS - adjust) << DBL_MANT_BITS);
        double scale = bits_to_double(adjust_bits);
        x = x * scale;
    }

    /* Final Newton refinement using original 'a' to reduce rounding error for scaled case */
    x = 0.5 * (x + a / x);

    return x;
}
