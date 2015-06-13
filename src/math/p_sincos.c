#include <pal.h>

/**
 *
 * Compute the sine and cosine of the vector 'a'.
 *
 * Angles are specified in * radians.
 * The radian number must be in the range 0 to 2pi,
 *
 * @param a     Pointer to input vector
 *
 * @param c     Pointer to sine output vector
 *
 * @param z     Pointer to cosine output vector
 *
 * @param n     Size of 'a' and 'c' vector.
 * 
 * @return      None
 *
 */
void p_sincos_f32(const float *a, float *c, float *z,
                  int n)
{
    p_sin_f32(a, c, n);
    p_cos_f32(a, z, n);
}
