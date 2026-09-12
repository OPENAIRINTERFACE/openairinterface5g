/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "phy_ofdm_mod.h"
#include "openair1/PHY/TOOLS/tools_defs.h"
//#define DEBUG_OFDM_MOD

// Use 64-byte alignment for IDFT output buffer to ensure no
// runtime error in case IDFT implementation uses AVX-512.
#define IDFT_OUTPUT_BUFFER_ALIGNMENT 64

void PHY_ofdm_mod(const int *input, /// pointer to complex input
                  int *output, /// pointer to complex output
                  int fftsize, /// FFT_SIZE
                  unsigned char nb_symbols, /// number of OFDM symbols
                  unsigned short nb_prefix_samples, /// cyclic prefix length
                  Extension_t etype) /// type of extension
{
  if (nb_symbols == 0)
    return;

  idft_size_idx_t idft_size = get_idft(fftsize);

#ifdef DEBUG_OFDM_MOD
  printf("[PHY] OFDM mod (size %d,prefix %d) Symbols %d, input %p, output %p\n",
         fftsize,
         nb_prefix_samples,
         nb_symbols,
         input,
         output);
#endif

  for (int i = 0; i < nb_symbols; i++) {
#ifdef DEBUG_OFDM_MOD
    printf("[PHY] symbol %d/%d offset %d (%p,%p -> %p)\n",
           i,
           nb_symbols,
           i * fftsize + (i * nb_prefix_samples),
           input,
           &input[i * fftsize],
           &output[(i * fftsize) + ((i)*nb_prefix_samples)]);
#endif

    // on AVX2 need 256-bit alignment

    // Copy to frame buffer with Cyclic Extension
    // Note:  will have to adjust for synchronization offset!

    switch (etype) {
      case CYCLIC_PREFIX: {
        int *output_ptr = &output[(i * fftsize) + ((1 + i) * nb_prefix_samples)];
        // Current idft implementation uses AVX-256: Check if buffer is already aligned to 256 bits (32 bytes)
        if ((uintptr_t)output_ptr % 32 == 0) {
          // output ptr is aligned, do ifft inplace
          idft(idft_size, (int16_t *)&input[i * fftsize], (int16_t *)output_ptr, 1);
        } else {
          // output ptr is not aligned, needs an extra memcpy
          c16_t temp[fftsize] __attribute__((aligned(IDFT_OUTPUT_BUFFER_ALIGNMENT)));
          idft(idft_size, (int16_t *)&input[i * fftsize], (int16_t *)temp, 1);
          memcpy((void *)output_ptr, (void *)temp, sizeof(temp));
        }
        // perform cyclic prefix insertion
        memcpy((void *)&output_ptr[-nb_prefix_samples], (void *)&output_ptr[fftsize - nb_prefix_samples], nb_prefix_samples * sizeof(c16_t));
        break;
      }

      case CYCLIC_SUFFIX: {
        // Use alignment of 64 bytes
        c16_t temp[fftsize] __attribute__((aligned(IDFT_OUTPUT_BUFFER_ALIGNMENT)));
        idft(idft_size, (int16_t *)&input[i * fftsize], (int16_t *)temp, 1);
        int *output_ptr = &output[(i * fftsize) + (i * nb_prefix_samples)];
        memcpy(output_ptr, temp, sizeof(temp));
        memcpy(&output_ptr[fftsize], temp, nb_prefix_samples * sizeof(c16_t));
        break;
      }

      case ZEROS:
        break;

      case NONE: {
        c16_t temp[fftsize] __attribute__((aligned(IDFT_OUTPUT_BUFFER_ALIGNMENT)));
        idft(idft_size, (int16_t *)&input[i * fftsize], (int16_t *)temp, 1);
        int *output_ptr = &output[i * fftsize];
        memcpy(output_ptr, temp, sizeof(temp));
        break;
      }

      default:
        break;
    }
  }
}
