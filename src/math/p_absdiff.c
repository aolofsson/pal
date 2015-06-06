#include <pal.h>

/**
 *
 * Compute an element wise absolute difference of two vectors 'a' and 'b'.
 *
 * @param a     Pointer to input vector
 *
 * @param b     Pointer to input vector
 *
 * @param c     Pointer to output vector
 *
 * @param n     Size of 'a' and 'c' vector.
 *
 * @param p     Number of processor to use (task parallelism)
 *
 * @param team  Team to work with 
 *
 * @return      None
 *
 */

void p_absdiff_f32(float *a, float *b, float *c, int n, int p, p_team_t team)
{

    int i;
    for (i = 0; i < n; i++) {
        float v = *(a + i) - *(b + i);
        uint32_t tmp = *(uint32_t*) &v;
        tmp &= 0x7FFFFFFF;
        *(c + i) = *(float*) &tmp;
    }
}
