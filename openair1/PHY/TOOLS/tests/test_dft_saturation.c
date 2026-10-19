/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*
 * Overflow behaviour of the fixed-point DFTs.
 *
 * Input: x[k] = hi for even k, lo for odd k on the first 62% of the bins, 0 elsewhere.
 * The exact output has its peak at bin 0 just below full scale, and a smaller peak at N/2.
 * In a decimation-in-time transform the even half-size sub-DFT only sees the large
 * values, so its DC exceeds int16 although the final output fits. If that intermediate
 * wraps, bin 0 is destroyed and bin N/2 wins; a saturating transform keeps the peak at 0.
 * A wrong peak position here is what turns into a wrong delay/TA in nr_est_delay().
 */
#include <stdio.h>
#include <math.h>
#include "openair1/PHY/TOOLS/tools_defs.h"

static const int sizes[] = {256, 512, 1024, 2048, 4096, 8192};

static int peak_pos(const c16_t *y, int n)
{
  int pos = 0;
  int64_t max = -1;
  for (int i = 0; i < n; i++) {
    const int64_t p = (int64_t)y[i].r * y[i].r + (int64_t)y[i].i * y[i].i;
    if (p > max) {
      max = p;
      pos = i;
    }
  }
  return pos;
}

int main(void)
{
  load_dftslib();
  int ret = 0;
  for (int s = 0; s < sizeofArray(sizes); s++) {
    const int n = sizes[s];
    const int used = n * 1272 / 2048;
    // exact |y[0]| = (hi + lo) / 2 * used / sqrt(n), set to ~98.6% of full scale
    const double mean = 0.986 * 32767 * sqrt(n) / used;
    const int16_t hi = lrint(mean * 2100 / 1150);
    const int16_t lo = lrint(mean * 200 / 1150);
    c16_t *x = malloc16_clear(n * sizeof(*x));
    c16_t *y = malloc16_clear(n * sizeof(*y));
    for (int k = 0; k < used; k++)
      x[k].r = (k & 1) ? lo : hi;

    idft(get_idft(n), (int16_t *)x, (int16_t *)y, 1);
    const int ipos = peak_pos(y, n);
    dft(get_dft(n), (int16_t *)x, (int16_t *)y, 1);
    const int dpos = peak_pos(y, n);

    printf("size %5d hi %5d lo %4d: idft peak %4d, dft peak %4d\n", n, hi, lo, ipos, dpos);
    if (ipos != 0 || dpos != 0) {
      printf("size %d: peak moved by internal overflow, expected bin 0\n", n);
      ret = 1;
    }
    free(x);
    free(y);
  }
  return ret;
}
