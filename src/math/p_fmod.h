#pragma once

/* Return the remainder of x / y. */
static inline float __fmod(const float x, const float y)
{
    float i;
    i = x / y;
    return x - i * y;
}

/* Map __fmod on a array. */
static inline void _fmod(const float *a, float *c, int n, const float x)
{
    int i;
    float tmp;
    for (i = 0; i < n; i++) {
        /* With temporary variable, a can overlap c. */
        tmp = __fmod(a[i], x);
        c[i] = tmp;
    }
}
