#include <pal.h>

/**
 *
 * Calculate exponent (e^a), where e is the base of the natural logarithm
 * (2.71828.)
 *
 * @param a     Pointer to input vector
 *
 * @param c     Pointer to output vector
 *
 * @param n     Size of 'a' and 'c' vector.
 *
 * @return      None
 *
 */
#include <math.h>
void p_exp_f32(const float *a, float *c, int n)
{

    int i;
    for (i = 0; i < n; i++) {
        *(c + i) = expf(*(a + i));
    }
}
