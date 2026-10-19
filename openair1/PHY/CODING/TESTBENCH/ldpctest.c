/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "assertions.h"
#include "SIMULATION/TOOLS/sim.h"
#include "common/config/config_userapi.h"
#include "common/utils/load_module_shlib.h"
#include "PHY/CODING/nrLDPC_extern.h"
// #include "openair1/SIMULATION/NR_PHY/nr_unitary_defs.h"
#include "openair1/PHY/defs_nr_common.h"
#include "coding_unitary_defs.h"
#include "common/utils/LOG/log.h"
#ifdef LDPC_CUDA
#include "PHY/gpu_compat.h"
#include "PHY/gpu_alloc.h"
#endif

#define MAX_BLOCK_LENGTH 8448

#ifndef malloc16
#define malloc16(x) memalign(32, x)
#endif

#define NR_LDPC_PROFILER_DETAIL

ldpc_interface_t ldpc_orig, ldpc_toCompare;
static double modulated_input[MAX_NUM_NR_DLSCH_SEGMENTS_PER_LAYER * 4][68 * 384];

// 4-bit quantizer
int8_t quantize4bit(double D, double x)
{
  double qxd;
  qxd = floor(x / D);
  //  printf("x=%f,qxd=%f\n",x,qxd);

  if (qxd <= -8)
    qxd = -8;
  else if (qxd > 7)
    qxd = 7;

  return ((int8_t)qxd);
}

int8_t quantize8bit(double D, double x)
{
  double qxd;
  // int8_t maxlev;
  qxd = floor(x / D);

  // maxlev = 1<<(B-1);

  // printf("x=%f,qxd=%f,maxlev=%d\n",x,qxd, maxlev);

  if (qxd <= -128)
    qxd = -128;
  else if (qxd >= 128)
    qxd = 127;

  return ((int8_t)qxd);
}

typedef struct {
  double n_iter_mean;
  double n_iter_std;
  int n_iter_max;
  double snr;
  double ber;
  double bler;
} n_iter_stats_t;

typedef struct {
  unsigned int errors;
  unsigned int errors_bit;
  double errors_bit_uncoded;
  unsigned int crc_misses;
  time_stats_t time_optim;
  time_stats_t time_decoder;
  n_iter_stats_t dec_iter;
} one_measurement_t;

uint8_t *estimated_output_dev, *estimated_output;
int8_t *channel_output_fixed_dev, *channel_output_fixed;

one_measurement_t test_ldpc(short max_iterations,
                            int nom_rate,
                            int denom_rate,
                            double SNR,
                            unsigned char qbits,
                            short Kprime,
                            unsigned int ntrials,
                            int n_segments,
                            int gen_code,
                            int use32bit,
                            bool print_wrong_segments)
{
  one_measurement_t ret = {0};
  reset_meas(&ret.time_optim);
  reset_meas(&ret.time_decoder);
  // clock initiate
  // time_stats_t time,time_optim,tinput,tprep,tparity,toutput, time_decoder;
  time_stats_t time, tinput, tprep, tparity, toutput;
  double n_iter_mean = 0;
  double n_iter_std = 0;
  int n_iter_max = 0;

  double sigma;
  sigma = 1.0 / sqrt(2 * SNR);
  cpu_meas_enabled = 1;
  memset(estimated_output, 0, sizeof(*estimated_output) * n_segments * Kprime);
#ifdef LDPC_CUDA
  uint8_t **test_input, *test_input_p;
#else
  uint8_t *test_input[n_segments * NR_MAX_NB_LAYERS];
#endif
  uint8_t *channel_input[n_segments];
  uint8_t *channel_input_optim;

  // double channel_output[68 * 384];

  memset(modulated_input, 0, sizeof(modulated_input));
  short BG = 0, nrows = 0; //,ncols;
  int i1, Kb = 0;
  int R_ind = 0;
  // int n_segments=1;
  int code_rate_vec[8] = {15, 13, 25, 12, 23, 34, 56, 89};
  // double code_rate_actual_vec[8] = {0.2, 0.33333, 0.4, 0.5, 0.66667, 0.73333, 0.81481, 0.88};

  t_nrLDPC_dec_params decParams[n_segments];
  memset(decParams, 0, sizeof(decParams));

  t_nrLDPC_time_stats decoder_profiler = {0};

  int32_t n_iter = 0;

  reset_meas(&time);
  reset_meas(&tinput);
  reset_meas(&tprep);
  reset_meas(&tparity);
  reset_meas(&toutput);
  // Reset Decoder profiles
  reset_meas(&decoder_profiler.llr2llrProcBuf);
  reset_meas(&decoder_profiler.llr2CnProcBuf);
  reset_meas(&decoder_profiler.cnProc);
  reset_meas(&decoder_profiler.cnProcPc);
  reset_meas(&decoder_profiler.bnProc);
  reset_meas(&decoder_profiler.bnProcPc);
  reset_meas(&decoder_profiler.cn2bnProcBuf);
  reset_meas(&decoder_profiler.bn2cnProcBuf);
  reset_meas(&decoder_profiler.llrRes2llrOut);
  reset_meas(&decoder_profiler.llr2bit);
  // reset_meas(&decoder_profiler.total);

  // determine number of bits in codeword
  if (Kprime > 3840) {
    BG = 1;
    Kb = 22;
    nrows = 46; // parity check bits
    // ncols=22; //info bits
  } else {
    BG = 2;
    nrows = 42; // parity check bits
    // ncols=10; // info bits
    if (Kprime > 640)
      Kb = 10;
    else if (Kprime > 560)
      Kb = 9;
    else if (Kprime > 192)
      Kb = 8;
    else
      Kb = 6;
  }

  bool error = false;
  switch (nom_rate) {
    case 1:
      if (denom_rate == 5)
        if (BG == 2)
          R_ind = 0;
        else
          error = true;
      else if (denom_rate == 3)
        R_ind = 1;
      else if (denom_rate == 2)
        // R_ind = 3;
        error = true;
      else
        error = true;
      break;
    case 2:
      if (denom_rate == 5)
        // R_ind = 2;
        error = true;
      else if (denom_rate == 3)
        R_ind = 4;
      else
        error = true;
      break;
    case 22:
      if (denom_rate == 25 && BG == 1)
        R_ind = 7;
      else
        error = true;
      break;
    default:
      error = true;
  }

  if (error) {
    printf("Not supported: nom_rate: %d, denom_rate: %d\n", nom_rate, denom_rate);
    exit(1);
  }

  // find minimum value in all sets of lifting size
  int Zc = 0;
  const short lift_size[51] = {2,  3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  18,  20,
                               22, 24,  26,  28,  30,  32,  36,  40,  44,  48,  52,  56,  60,  64,  72,  80,  88,
                               96, 104, 112, 120, 128, 144, 160, 176, 192, 208, 224, 240, 256, 288, 320, 352, 384};
  for (i1 = 0; i1 < 51; i1++) {
    if (lift_size[i1] >= (double)Kprime / Kb) {
      Zc = lift_size[i1];
      break;
    }
  }

  // K: Segment size = number of columns in the parity check matrix * lifting size (number of columns of the square parity check
  // sub-matrix)
  int K = 0;
  if (BG == 1) {
    K = 22 * Zc;
  } else if (BG == 2) {
    K = 10 * Zc;
  } else {
    printf("Not supported: BG: %d\n", BG);
    exit(1);
  }

  printf("ldpc_test: codeword_length %d, n_segments %d, Kprime %d, BG %d, Zc %d, Kb %d\n",
         n_segments * K,
         n_segments,
         Kprime,
         BG,
         Zc,
         Kb);
  const int no_punctured_columns = (int)((nrows - 2) * Zc + K - K * (1 / ((float)nom_rate / (float)denom_rate))) / Zc;
  //  printf("puncture:%d\n",no_punctured_columns);
  const int removed_bit = (nrows - no_punctured_columns - 2) * Zc + K - (int)(K / ((float)nom_rate / (float)denom_rate));

  printf("nrows: %d\n", nrows);
  printf("no_punctured_columns: %d\n", no_punctured_columns);
  printf("removed_bit: %d\n", removed_bit);
  printf("To: %d\n", (Kb + nrows - no_punctured_columns) * Zc - removed_bit);
  printf("number of undecoded bits: %d\n", (Kb + nrows - no_punctured_columns - 2) * Zc - removed_bit);

  // generate input block
#ifdef LDPC_CUDA
  test_input_p = gpuHostAlloc_or_fail(n_segments * sizeof(uint8_t *), gpuHostAllocMapped);
  test_input = (uint8_t **)test_input_p;
#endif
  for (int j = 0; j < n_segments; j++) {
#ifdef LDPC_CUDA
    test_input[j] = gpuHostAlloc_or_fail(((K + 7) & ~7) / 8, gpuHostAllocMapped);
#else
    test_input[j] = malloc16(((K + 7) & ~7) / 8);
    memset(test_input[j], 0, ((K + 7) & ~7) / 8);
#endif
    channel_input[j] = malloc16(68 * 384);
    memset(channel_input[j], 0, 68 * 384);
  }
  channel_input_optim = malloc16(68 * 384 * sizeof(uint32_t));
  memset(channel_input_optim, 0, 68 * 384 * sizeof(uint32_t));

  // Fill input segments with random values
  for (int j = 0; j < n_segments; j++) {
    int i = 0;
    for (i = 0; i < ((Kprime + 7) & ~7) / 8; i++)
      test_input[j][i] = (uint8_t)rand();
    // Mask the last byte in order to keep filler bits to 0
    if (Kprime % 8 != 0) {
      uint8_t last_byte_mask = (1 << (Kprime % 8)) - 1;
      test_input[j][i - 1] = test_input[j][i - 1] & last_byte_mask;
    }
  }

  encoder_implemparams_t impp = {.Zc = Zc, .Kb = Kb, .BG = BG, .K = K};
  impp.gen_code = gen_code;
  impp.tparity = &tparity;
  impp.tinput = &tinput;
  impp.toutput = &toutput;

  if (ntrials == 0)
    ldpc_orig.LDPCencoder(test_input, channel_input[0], &impp);
  impp.gen_code = 0;
  decode_abort_t dec_abort;
  init_abort(&dec_abort);
  uint32_t **output32 = NULL;
  for (int trial = 0; trial < ntrials; trial++) {
    unsigned int segment_bler = 0;
    //// encoder
    start_meas(&time);
    for (int j = 0; j < n_segments; j++) {
      ldpc_orig.LDPCencoder(&test_input[j], channel_input[j], &impp);
    }
    stop_meas(&time);

    impp.n_segments = n_segments;
    start_meas(&ret.time_optim);
    impp.first_seg = 0;
    if (use32bit == 0)
      ldpc_toCompare.LDPCencoder(test_input, channel_input_optim, &impp);
    else
      output32 = ldpc_toCompare.LDPCencoder32(test_input, &impp);

    stop_meas(&ret.time_optim);

    if (ntrials == 1)
      for (int j = 0; j < n_segments; j++)
        for (int i = 0; i < K + (nrows - no_punctured_columns) * Zc - removed_bit; i++) {
          if (((use32bit == 0) && (channel_input[j][i] != ((channel_input_optim[i] >> j) & 0x1)))
              || ((use32bit == 1) && (channel_input[j][i] != (((output32[j >> 5][i] >> (j & 31)) & 0x1))))) {
            printf("differ in seg %d pos %d (%u,%u)\n",
                   j,
                   i,
                   channel_input[j][i],
                   (((uint32_t *)channel_input_optim)[i] >> j) & 0x1);
            return ret;
          }
        }

    for (int j = 0; j < n_segments; j++) {
      for (int i = 2 * Zc; i < (Kb + nrows - no_punctured_columns) * Zc - removed_bit; i++) {
#ifdef DEBUG_CODER
        if ((i & 0xf) == 0)
          printf("\ne %d..%d:    ", i, i + 15);
#endif
        int bit =
            (use32bit == 0) ? ((channel_input_optim[i - 2 * Zc] >> j) & 0x1) : ((output32[j >> 5][i - 2 * Zc] >> (j & 31)) & 0x1);

        if (bit == 0)
          modulated_input[j][i] = 1.0; /// sqrt(2);  //QPSK
        else
          modulated_input[j][i] = -1.0; /// sqrt(2);

        channel_output_fixed[j * (384 * 68) + i] =
            (int8_t)quantize(sigma / 4.0 / 4.0, modulated_input[j][i] + sigma * gaussdouble(0.0, 1.0), qbits);

        // Uncoded BER
        uint8_t channel_output_uncoded = channel_output_fixed[(j * 384 * 68) + i] < 0 ? 1 /* QPSK demod */ : 0;
        if (channel_output_uncoded != bit)
          ret.errors_bit_uncoded++;
      }

#ifdef DEBUG_CODER
      printf("\n");
      exit(-1);
#endif

      decParams[j].BG = BG;
      decParams[j].Z = Zc;
      decParams[j].R = code_rate_vec[R_ind]; // 13;
      decParams[j].numMaxIter = max_iterations;
      decParams[j].outMode = nrLDPC_outMode_BIT;
      decParams[j].Kprime = Kprime;
      decParams[j].n_segments = n_segments;
    }

    // Decoder
    bool seg_error[n_segments];
    for (int j = 0; j < n_segments; j++) {
      start_meas(&ret.time_decoder);
      set_abort(&dec_abort, false);
      int8_t *input = use32bit ? channel_output_fixed_dev : &channel_output_fixed[j];
      uint8_t *output = use32bit ? estimated_output_dev : &estimated_output[j * Kprime];
      // when using LDPC CUDA, the segment interface for decoding handles all
      // segments, unlike the optim/orig libraries (so the API is handled
      // differently). Account for this below by calling into it only on first
      // segment; subsequent segments will be counted as 0 time, so the average
      // per segment still holds.
      if (!use32bit || j == 0)
        n_iter = ldpc_toCompare.LDPCdecoder(&decParams[j], input, output, &decoder_profiler, &dec_abort);
      stop_meas(&ret.time_decoder);

      // count errors
      seg_error[j] = memcmp(&estimated_output[j * (Kprime >> 3)], test_input[j], ((Kprime + 7) & ~7) / 8) != 0;
      if (seg_error[j])
        segment_bler++;
      for (int i = 0; i < Kprime; i++) {
        unsigned char estoutputbit = (estimated_output[j * (Kprime >> 3) + (i / 8)] & (1 << (i & 7))) >> (i & 7);
        unsigned char inputbit = (test_input[j][i / 8] & (1 << (i & 7))) >> (i & 7); // Further correct for multiple segments
        if (estoutputbit != inputbit)
          ret.errors_bit++;
      }

      n_iter_mean += n_iter;
      n_iter_std += pow(n_iter - 1, 2);

      if (n_iter > n_iter_max)
        n_iter_max = n_iter;

    } // end segments

    if (segment_bler != 0)
      ret.errors++;
    if (segment_bler != 0 && print_wrong_segments) {
      printf("trial %d: segments in error:", trial);
      for (int j = 0; j < n_segments; j++)
        if (seg_error[j])
          printf(" %d", j);
      printf("\n");
    }
  }

  ret.dec_iter.n_iter_mean = n_iter_mean / (double)ntrials / (double)n_segments - 1;
  ret.dec_iter.n_iter_std =
      sqrt(n_iter_std / (double)ntrials / (double)n_segments - pow(n_iter_mean / (double)ntrials / (double)n_segments - 1, 2));
  ret.dec_iter.n_iter_max = n_iter_max - 1;

  ret.errors_bit_uncoded = ret.errors_bit_uncoded / (double)((Kb + nrows - no_punctured_columns - 2) * Zc - removed_bit);

  for (int j = 0; j < n_segments; j++) {
#ifdef LDPC_CUDA
    gpuFreeHost(test_input[j]);
#else
    free(test_input[j]);
#endif
    free(channel_input[j]);
  }
#ifdef LDPC_CUDA
  gpuFreeHost(test_input);
#endif
  free(channel_input_optim);

  print_meas(&time, "ldpc_encoder", NULL, NULL);
  print_meas(&ret.time_optim, "ldpc_encoder_optim", NULL, NULL);
  print_meas(&tinput, "ldpc_encoder_optim(input)", NULL, NULL);
  print_meas(&tprep, "ldpc_encoder_optim(prep)", NULL, NULL);
  print_meas(&tparity, "ldpc_encoder_optim(parity)", NULL, NULL);
  print_meas(&toutput, "ldpc_encoder_optim(output)", NULL, NULL);
  printf("\n");
  print_meas(&ret.time_decoder, "ldpc_decoder", NULL, NULL);
  print_meas(&decoder_profiler.llr2llrProcBuf, "llr2llrProcBuf", NULL, NULL);
  print_meas(&decoder_profiler.llr2CnProcBuf, "llr2CnProcBuf", NULL, NULL);
  print_meas(&decoder_profiler.cnProc, "cnProc (per iteration)", NULL, NULL);
  print_meas(&decoder_profiler.cnProcPc, "cnProcPc (per iteration)", NULL, NULL);
  print_meas(&decoder_profiler.bnProc, "bnProc (per iteration)", NULL, NULL);
  print_meas(&decoder_profiler.bnProcPc, "bnProcPc(per iteration)", NULL, NULL);
  print_meas(&decoder_profiler.cn2bnProcBuf, "cn2bnProcBuf (per iteration)", NULL, NULL);
  print_meas(&decoder_profiler.bn2cnProcBuf, "bn2cnProcBuf (per iteration)", NULL, NULL);
  print_meas(&decoder_profiler.llrRes2llrOut, "llrRes2llrOut", NULL, NULL);
  print_meas(&decoder_profiler.llr2bit, "llr2bit", NULL, NULL);
  printf("\n");

  return ret;
}

configmodule_interface_t *uniqCfg = NULL;
int main(int argc, char *argv[])
{
  short Kprime = 8448;
  // default to check output inside ldpc, the NR version checks the outer CRC defined by 3GPP
  char *ldpc_version = "";
  /* version of the ldpc decoder library to use (XXX suffix to use when loading libldpc_XXX.so */
  short max_iterations = 5;
  int n_segments = 1;
  // double rate=0.333;
  int ret = 1;
  int nom_rate = 1;
  int denom_rate = 3;
  double SNR0 = -2.0;
  uint8_t qbits = 8;
  unsigned int decoded_errors[10000]; // initiate the size of matrix equivalent to size of SNR
  int c, i = 0;
  bool print_wrong_segments = false;

  int n_trials = 1;
  double SNR_step = 0.1;

  int gen_code = 1;
  randominit();
  int test_uncoded = 0;
  n_iter_stats_t dec_iter[400] = {0};

  short BG = 0, Zc;

  if ((uniqCfg = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY)) == 0) {
    exit_fun("[LDPCTEST] Error, configuration module init failed\n");
  }
  logInit();

  while ((c = getopt(argc, argv, "--:O:q:r:s:S:l:Mn:d:i:t:u:hv:g:n:")) != -1) {
    /* ignore long options starting with '--', option '-O' and their arguments that are handled by configmodule */
    /* with this opstring getopt returns 1 for non-option arguments, refer to 'man 3 getopt' */
    if (c == 1 || c == '-' || c == 'O')
      continue;

    printf("handling optarg %c\n", c);
    switch (c) {
      case 'q':
        qbits = atoi(optarg);
        break;

      case 'r':
        nom_rate = atoi(optarg);
        break;

      case 'd':
        denom_rate = atoi(optarg);
        break;

      case 'l':
        Kprime = atoi(optarg);
        break;

      case 'n':
        n_trials = atoi(optarg);
        break;

      case 'M':
        print_wrong_segments = true;
        break;

      case 's':
        SNR0 = atof(optarg);
        break;

      case 'S':
        n_segments = atof(optarg);
        break;

      case 't':
        SNR_step = atof(optarg);
        break;

      case 'i':
        max_iterations = atoi(optarg);
        break;

      case 'u':
        test_uncoded = atoi(optarg);
        break;
      case 'v':
        ldpc_version = strdup(optarg);
        break;
      case 'g':
        gen_code = atoi(optarg);
        AssertFatal(gen_code <= 4, "gen_code %d is not allowed\n", gen_code);
        n_trials = 0;
        break;
      case 'h':
      default:
        printf("CURRENTLY SUPPORTED CODE RATES: \n");
        printf("BG1 (K' > 3840): 1/3, 2/3, 22/25 (8/9) \n");
        printf("BG2 (K' <= 3840): 1/5, 1/3, 2/3 \n\n");
        printf("-h This message\n");
        printf("-q Quantization bits, Default: 8\n");
        printf("-r Nominator rate, (1, 2, 22), Default: 1\n");
        printf("-d Denominator rate, (3, 5, 25), Default: 3\n");
        printf("-l Length of payload bits in a segment (K' in 38.212-5.2.2), [1, 8448], Default: 8448\n");
        printf("-n Number of simulation trials, Default: 1\n");
        printf("-M print which segments have errors, if any\n");
        printf("-s SNR per information bit (EbNo) in dB, Default: -2\n");
        printf("-S Number of segments (Maximum: 8), Default: 1\n");
        printf("-t SNR simulation step, Default: 0.1\n");
        printf("-i Max decoder iterations, Default: 5\n");
        printf("-u Set SNR per coded bit, Default: 0\n");
        printf("-v _XXX Set ldpc shared library version. libldpc_XXX.so will be used \n");
        exit(1);
        break;
    }
  }
  printf("block length %d: \n", Kprime);
  printf("n_trials %d: \n", n_trials);
  printf("SNR0 %f: \n", SNR0);

  printf("loading %s\n", ldpc_version);
  load_LDPClib(ldpc_version, &ldpc_toCompare);
  int use32bit = strcmp(ldpc_version, "_cuda") == 0;
  if (use32bit)
    printf("ldpc_cuda detected, will call LDPCencoder32() instead of LDPCencoder()\n");
  ldpc_toCompare.LDPCinit();
  load_LDPClib("_orig", &ldpc_orig);
  ldpc_orig.LDPCinit();

  // find minimum value in all sets of lifting size
  Zc = 0;
#ifdef LDPC_CUDA
  estimated_output = gpuHostAlloc_or_fail(sizeof(uint8_t) * n_segments * Kprime, gpuHostAllocMapped);
  channel_output_fixed = gpuHostAlloc_or_fail(sizeof(int8_t) * n_segments * 68 * 384, gpuHostAllocMapped);
  if (use32bit == 1) {
    estimated_output_dev = gpuHostGetDevicePointer_or_fail(estimated_output);
    channel_output_fixed_dev = gpuHostGetDevicePointer_or_fail(channel_output_fixed);
    printf("estimated_output_dev %p, channel_output_fixed_dev %p\n", estimated_output_dev, channel_output_fixed_dev);
  }
#else
  estimated_output = malloc(MAX_NUM_DLSCH_SEGMENTS * Kprime);
  channel_output_fixed = calloc(1, MAX_NUM_DLSCH_SEGMENTS * 68 * 384);
#endif

  char fname[200];
  sprintf(fname, "ldpctest_BG_%d_Zc_%d_rate_%d-%d_Kprime_%d_maxit_%d.txt", BG, Zc, nom_rate, denom_rate, Kprime, max_iterations);
  FILE *fd = fopen(fname, "w");
  AssertFatal(fd != NULL, "cannot open %s\n", fname);

  fprintf(fd,
          "SNR BLER BER UNCODED_BER ENCODER_MEAN ENCODER_STD ENCODER_MAX DECODER_TIME_MEAN DECODER_TIME_STD DECODER_TIME_MAX "
          "DECODER_ITER_MEAN DECODER_ITER_STD DECODER_ITER_MAX\n");

  for (double SNR = SNR0; SNR < SNR0 + 20.0; SNR += SNR_step) {
    double SNR_lin;
    if (test_uncoded == 1)
      SNR_lin = pow(10, SNR / 10.0);
    else
      SNR_lin = pow(10, SNR / 10.0) * nom_rate / denom_rate;
    printf("Linear SNR: %d %f, %d/%d, Kprime %d\n", max_iterations, SNR_lin, nom_rate, denom_rate, Kprime);
    one_measurement_t res = test_ldpc(max_iterations,
                                      nom_rate,
                                      denom_rate,
                                      SNR_lin, // noise standard deviation
                                      qbits,
                                      Kprime, // block length bytes
                                      n_trials,
                                      n_segments,
                                      gen_code,
                                      use32bit,
                                      print_wrong_segments);

    decoded_errors[i] = res.errors;
    dec_iter[i] = res.dec_iter;
    dec_iter[i].snr = SNR;
    dec_iter[i].ber = (float)res.errors_bit / (float)n_trials / (float)Kprime / (double)n_segments;
    dec_iter[i].bler = (float)decoded_errors[i] / (float)n_trials;
    printf("SNR %f, BLER %f (%u/%d)\n", SNR, dec_iter[i].bler, decoded_errors[i], n_trials);
    printf("SNR %f, BER %f (%u/%d)\n", SNR, dec_iter[i].ber, decoded_errors[i], n_trials);
    printf("SNR %f, Uncoded BER %f (%u/%d)\n",
           SNR,
           res.errors_bit_uncoded / (float)n_trials / (double)n_segments,
           decoded_errors[i],
           n_trials);
    printf("SNR %f, Mean iterations: %f\n", SNR, dec_iter[i].n_iter_mean);
    printf("SNR %f, Std iterations: %f\n", SNR, dec_iter[i].n_iter_std);
    printf("SNR %f, Max iterations: %d\n", SNR, dec_iter[i].n_iter_max);
    printf("\n");

    time_stats_t *t_optim = &res.time_optim;
    printf("Encoding time mean: %15.3f us\n", (double)t_optim->diff / t_optim->trials / 1000.0);
    printf("Encoding time std: %15.3f us\n",
           sqrt((double)t_optim->diff_square / t_optim->trials / pow(1000, 2)
                - pow((double)t_optim->diff / t_optim->trials / 1000.0, 2)));
    printf("Encoding time max: %15.3f us\n", (double)t_optim->max / 1000.0);
    printf("\n");

    time_stats_t *t_decoder = &res.time_decoder;
    if (use32bit)
      printf("use32bit: decoding time mean per segment in average!\n");
    printf("Decoding time mean: %15.3f us\n", (double)t_decoder->diff / t_decoder->trials / 1000.0);
    printf("Decoding time std: %15.3f us\n",
           sqrt((double)t_decoder->diff_square / t_decoder->trials / pow(1000, 2)
                - pow((double)t_decoder->diff / t_decoder->trials / 1000.0, 2)));
    printf("Decoding time max: %15.3f us\n", (double)t_decoder->max / 1000.0);

    fprintf(fd,
            "%f %f %f %f %f %f %f %f %f %f %f %f %d \n",
            SNR,
            (double)decoded_errors[i] / (double)n_trials,
            (double)res.errors_bit / (double)n_trials / (double)Kprime / (double)n_segments,
            res.errors_bit_uncoded / (double)n_trials / (double)n_segments,
            (double)t_optim->diff / t_optim->trials / 1000.0 ,
            sqrt((double)t_optim->diff_square / t_optim->trials / pow(1000, 2)
                 - pow((double)t_optim->diff / t_optim->trials / 1000.0, 2)),
            (double)t_optim->max / 1000.0,
            (double)t_decoder->diff / t_decoder->trials / 1000.0,
            sqrt((double)t_decoder->diff_square / t_decoder->trials / pow(1000, 2)
                 - pow((double)t_decoder->diff / t_decoder->trials / 1000.0, 2)),
            (double)t_decoder->max / 1000.0,
            dec_iter[i].n_iter_mean,
            dec_iter[i].n_iter_std,
            dec_iter[i].n_iter_max);

    i = i + 1;
    if (decoded_errors[i - 1] == 0) {
      ret = 0;
      break;
    }
  }
  fclose(fd);
  LOG_M("ldpctestStats.m", "SNR", &dec_iter[0].snr, i, 1, 7);
  LOG_MM("ldpctestStats.m", "BLER", &dec_iter[0].bler, i, 1, 7);
  LOG_MM("ldpctestStats.m", "BER", &dec_iter[0].ber, i, 1, 7);
  LOG_MM("ldpctestStats.m", "meanIter", &dec_iter[0].n_iter_mean, i, 1, 7);

  loader_reset();
  logTerm();

  return ret;
}
