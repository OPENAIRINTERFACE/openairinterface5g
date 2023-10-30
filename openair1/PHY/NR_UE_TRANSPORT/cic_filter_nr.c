/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/**********************************************************************
*
* FILENAME    :  cic_filter_nr.c
*
* MODULE      :  generic functions for cic filters
*
* DESCRIPTION :  generic functions useful for decimation based on cascaded integrator comb filter
*
************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

#include "PHY/defs_nr_UE.h"

#include "PHY/NR_UE_TRANSPORT/cic_filter_nr.h"

/*******************************************************************
*
* NAME :         integrator_stage
*
* PARAMETERS :   pointer to input complex data
*                pointer to output complex data
*
* RETURN :
*
* DESCRIPTION :  single stage of an integrator
*
*                It is a single-pole IIR filter with a unity feedback coefficient
*
*                y(n) = y(n-1) +x(n)*
*
*********************************************************************/

void integrator_stage(int32_t *input, int32_t *output, int length)
{
  /* first sample is just copied */
  for (int i = 0; i < 1; i++) {
    output[2*i]   = input[2*i];
    output[2*i+1] = input[2*i+1];
  }

  /* then process all other samples */
  for (int i = 1; i < length; i++) {
    output[2*i]   = output[2*(i-1)]   + input[2*i];
    output[2*i+1] = output[2*(i-1)+1] + input[2*i+1];

#ifdef DBG_CIC_FILTER_NR
    if (i < 20) {
      printf("output[%d] = output[%d]    + input[%d] \n", (2*i), 2*(i-1), 2*i);
      printf("%d         =    %d         + %d \n", output[2*i], output[2*(i-1)], input[2*i]);
    }
#endif
  }
}

/*******************************************************************
*
* NAME :         comb_stage
*
* PARAMETERS :   pointer to input complex data
*                pointer to output complex data
*
* RETURN :
*
* DESCRIPTION :  single stage of a comb filter which is an odd-symetric FIR filter
*
*                y(n) = x(n) - x(n - M)
*
*                M is called the differential delay. It can take any positive
*                integer but it is usually limited to 1 or 2.
*
*********************************************************************/

void comb_stage(int32_t *input, int32_t *output, int length, int differential_delay)
{
  /* first sample is just copied */
  for (int i = 0; i < differential_delay; i++) {
    output[2*i]   = input[2*i];
	output[2*i+1] = input[2*i+1];
  }

  /* then process all other samples */
  for (int i = differential_delay; i < length; i++) {
    output[2*i]   = input[2*i]   - input[2*(i-differential_delay)];
    output[2*i+1] = input[2*i+1] - input[2*(i-differential_delay)+1];

#ifdef DBG_CIC_FILTER_NR
    if (i < 20) {
      printf("output[%d] = input[%d]    - input[%d] \n", (2*i), (2*i), 2*(i-differential_delay));
      printf("%d         =    %d        - %d \n", output[2*i], input[2*i], input[2*(i-differential_delay)]);
    }
#endif
  }
}

/*******************************************************************
*
* NAME :         rate_change_stage
*
* PARAMETERS :   pointer to input complex data
*                pointer to output complex data
*
* RETURN :       none
*
* DESCRIPTION :  change sampling rate by a factor of R
*                just copy one samples over R from input to output buffer
*
*********************************************************************/

void rate_change_stage(int32_t *input, int32_t *output, int length, int rate_change)
{
  for (int i = 0; (i*rate_change) < length; i++) {
    output[2*i]   = input[2*i*rate_change];
    output[2*i+1] = input[(2*i*rate_change)+1];

#ifdef DBG_CIC_FILTER_NR
    if (i < 20) {
      printf("input[%d] : %d \n", 2*i*rate_change, input[2*i*rate_change]);
      printf("output[%d] : %d \n", 2*i, output[2*i]);
    }
#endif
  }
}

/*******************************************************************
*
* NAME :         fir_filter
*
* PARAMETERS :   pointer to input complex data
*                pointer to output complex data
*
* RETURN :       none
*
* DESCRIPTION :  implement a fir filter with taps from an array
*
*********************************************************************/

void fir_filter_init(fir_filter_t* f, int taps_fir_number, int32_t *filter_taps) {
  int i;
  for(i = 0; i < taps_fir_number; ++i)
    f->history[i] = 0;
  f->last_index = 0;
  if (taps_fir_number > MAX_SAMPLEFILTER_TAP_NUM) {
    printf("Number of taps of this FIR filter %d exceeds maximum value %d \n", taps_fir_number, MAX_SAMPLEFILTER_TAP_NUM);
    assert(0);
  }
  f->filter_tap_number = taps_fir_number;
  f->filter_taps = filter_taps;
}

void fir_filter_put(fir_filter_t* f, int32_t input) {
  f->history[f->last_index++] = input;
  if(f->last_index == f->filter_tap_number)
    f->last_index = 0;
}

int32_t fir_filter_get(fir_filter_t* f, int scaling_shift) {
  long long acc = 0;
  int index = f->last_index, i;
  for(i = 0; i < f->filter_tap_number; ++i) {
    index = index != 0 ? index-1 : f->filter_tap_number-1;
    acc += (long long)f->history[index] * f->filter_taps[i];
  };
  return acc >> scaling_shift;
}

void fir_filter_basic(int32_t *input, int32_t *output, int length, int taps_fir_number, int32_t *filter_taps, int scaling_shift)
{
  fir_filter_t filter;

  for (int j = 0; j < 2; j++) {

    fir_filter_init(&filter, taps_fir_number, filter_taps);

    for (int i = 0; i < length; i++) {
      fir_filter_put(&filter, input[2*i+j]);
      output[2*i+j] = fir_filter_get(&filter, scaling_shift);
      //printf("i %d input %d output %d ", 2*i+j, input[2*i+j], output[2*i + j]);
    }
  }
}

/*******************************************************************
*
* NAME :         fir_filter
*
* PARAMETERS :   pointer to input complex data
*                pointer to output complex data
*
* RETURN :
*
* DESCRIPTION :  single stage of a fir filter which is an odd-symetric FIR filter
*
*                y(n) = SUM ( bi * x(n - i) )
*
*                bi are tags of FIR filter
*
*********************************************************************/

void fir_filter(int32_t *input, int32_t *output, int length, int taps_fir_number, const int *taps_fir, int scaling_shift)
{
  int32_t current;

  /* taps start from 1 to taps_fir_number */
  for (int i = 0; i < taps_fir_number; i++) {
    for (int j=0; j<2 ; j++) {
      current = 0;
      for (int taps_number = 0; taps_number < taps_fir_number; taps_number++) {
        if (i >= taps_number) {
          current += taps_fir[taps_number]*input[2*(i-taps_number)+j];
#ifdef DBG_CIC_FILTER_NR
          printf("current[%d] %d = taps[%d] : %d x input[%d] : %d \n", (2*i+j), current, taps_number, taps_fir[taps_number], (2*(i-taps_number)+j), input[2*(i-taps_number)+j]);
#endif
        }
      }
      output[2*i+j] = current >> scaling_shift;
    }
  }

  for (int i = taps_fir_number; i < length; i++) {
    for (int j=0; j<2 ; j++) {
      current = 0;
      for (int taps_number = 0; taps_number < taps_fir_number; taps_number++) {
        current += taps_fir[taps_number]*input[2*(i-taps_number)+j];
#ifdef DBG_CIC_FILTER_NR
        if (i < taps_fir_number + 16) {
          printf("current[%d] %d = taps[%d] : %d x input[%d] : %d \n", (2*i+j), current, taps_number, taps_fir[taps_number], (2*(i-taps_number)+j), input[2*(i-taps_number)+j]);
        }
#endif
      }
      output[2*i+j] = current >> scaling_shift; // SHARPENED_FIR_SCALING_ACC;
    }
  }
}

/*******************************************************************
*
* NAME :         cic_decimator
*
* PARAMETERS :   pointer to input complex data
*                pointer to output complex data
*                length size of input buffer
*                decimation by a factor of R
*
* RETURN :       result of decimator is the output buffer
*
* DESCRIPTION :  Cascaded integrator-comb filter
*                rate change by a factor of R
*
*                CIC filter with N stages is composed of:
*                - N cascaded integrator filters clocked at high sampling rate fs
*                - N cascaded comb stage running at fs/R
*
*              +-----------+  +----------+  +----------+  +----+  +----------+  +----------+  +----------+         +--------------+
*              | Integrator|  |Integrator|  |Integrator|  |  R |  |   Comb   |  |   Comb   |  |   Comb   |         | Compensation |
*    input --->|   stage   |->|   stage  |->| stage    |->|    |->|   stage  |->|   stage  |->|   stage  |-------->|   FIR Filter |---> output
*              +-----------+  +----------+  +----------+  +----+  +----------+  +----------+  +----------+         +--------------+
*             <---------------------------------------->    |      <-------------------------------------->
*                   N cascaded integrator filters           |        N cascaded comb stage running at fs/R
*                 clocked at high sampling rate fs          V
*                                                        downsampling
*                                                      from fs to fs/R
*
* In order that cic filter works properly, some caution should be taken for getting good accuracy.
* Firstly data should be in fixed point in order to balance integrator and comb stages. If does not work with data in floating point.
*
* k1 is the number of significant bits for input
* k2 is the number of significant bits for output
* m = k2 - k2 > N * log2(RM - 1)   N number of stage, M differential delay and R rate change factor
* for example with R=16  N=4 and M=1 with input data on 16 bits
* output data should be on k2 = m + k1 > 31
*
* Additionally there is a bit growth so processing has a gain of
* G = (RM)power(N)
* For example with R=16 N=4 M=1  G=(16*1)power(4)=65536
*
* It gives size of output data with number of significant bit Bout = (N x log2(R x M)) + Bin
* With R=16 N=4 M=1 Bin=16 => Bout = (4 x log2(16 x 1)) + 16 = (4 x 4) + 16 = 32 so output significant bits should be 32.
* As a consequence, all computations are done with 32 bits up to the last stage (no rounding or truncation).
* Except for the final result which can be casted from 32 bits to 16 bits.
*
*********************************************************************/

void cic_decimator(int16_t *input_buffer, int16_t *output_buffer, int length, int rate_change, int number_cic_stage, int set_scaling_factor, int fir_rate_change)
{
  int32_t *buffer_one;
  int32_t *buffer_two;
  int32_t *input;
  int32_t *output;
  int32_t *tmp;
  int new_length;
  int current_rate_change;

  /* for cic decimator, gain g of the output depends on rate change R, differential delay M and number of stage N */
  /* g = power(R x M, N) / power(2 , log2 (R x M)  */
  /* so a scale factor should be applied to output in order to get a unity DC gain */

  int scaling_factor =  0; // pow(2, log2(rate_change * M_DIFFERENTIAL_DELAY))/pow((rate_change * M_DIFFERENTIAL_DELAY), number_cic_stage);

  if (set_scaling_factor != 0) {
    scaling_factor = set_scaling_factor;
    printf("Decimation with rate change of %d with set gain of %d \n", rate_change, scaling_factor);
  }

  /* get working buffers */
  buffer_one = calloc(length, sizeof(int32_t) * 2);   /* for i&q samples */
  if (buffer_one == NULL) {
     msg("Fatal memory allocation problem \n");
     assert(0);
  }

  buffer_two = malloc(length*sizeof(int32_t) * 2);  /* for i&q samples */
  if (buffer_two == NULL) {
    msg("Fatal memory allocation problem \n");
    assert(0);
  }

  /* copy of input buffer from int16 to int32 */
  for (int i = 0; i < length; i++) {
    buffer_one[2*i]     = input_buffer[2*i];
    buffer_one[2*i + 1] = input_buffer[2*i + 1];

#ifdef DBG_CIC_FILTER_NR
    if (i < 10) {
      printf("buffer_one[%d] : %d %d \n", i, buffer_one[2*i], buffer_one[2*i + 1]);
    }
#endif
  }

  input = buffer_one;
  output = buffer_two;

  /* call of integrator stages running at Fs */
  for (int stage = 0; stage < number_cic_stage; stage++)
  {
    integrator_stage(input, output, length);

    /* swap between output and input */
    tmp = input;
    input = output;
    output = tmp;

#if 0
    char output_file[255];
    char sequence_name[255];
    sprintf(output_file, "int_seq_%d.m", stage);
    sprintf(sequence_name, "int_seq_%d", stage);

    printf("file %s signal %s\n", output_file, sequence_name);
    write_output(output_file, sequence_name, input, length, 1, 3); /* format = 3 is for int32_t */
#endif
  }

  /* a sharpened Fir filter can be added which can provide also a rate change */
  current_rate_change = rate_change/fir_rate_change;

  /* rate change from sampling frequency Fs to Fs/R */
  rate_change_stage(input, output, length, current_rate_change);

  /* new length of samples should be computed based on the rate change factor R */
  new_length = length/current_rate_change;

  /* swap between output and input */
  tmp = input;
  input = output;
  output = tmp;

  /* call of comb stages running at Fs/R */
  for (int stage = 0; stage < number_cic_stage; stage++)
  {
    comb_stage(input, output, new_length, M_DIFFERENTIAL_DELAY);

    /* swap between output and input */
    tmp = input;
    input = output;
    output = tmp;

#if 0
    char output_file[255];
    char sequence_name[255];
    sprintf(output_file, "comb_seq_%d.m", stage);
    sprintf(sequence_name, "comb_seq_%d", stage);
    printf("file %s signal %s\n", output_file, sequence_name);
    write_output(output_file, sequence_name, input, new_length, 1, 3); /* format = 3 for int32_t */
#endif
  }

#if 1

  if (fir_rate_change > 1)
  {
#define FIR_TAPS_SCALING   (10000)

    /* convert float taps of fir filter to int32 taps */
    int32_t *filter_taps_fixed_point = malloc(FIR_TAPS_NUMBER*sizeof(int32_t));
    if (filter_taps_fixed_point == NULL) {
      msg("Fatal memory allocation problem \n");
      assert(0);
    }

    for (int i = 0; i < FIR_TAPS_NUMBER; i++) {
      filter_taps_fixed_point[i] = (int32_t)(sharpened_fir_taps[i] * FIR_TAPS_SCALING);
    }

#if 1

    /* compensation filter for equalizing the passband drop and perform a low rate change */
    fir_filter(input, output, new_length, FIR_TAPS_NUMBER, filter_taps_fixed_point, FIR_FITER_SCALING_ACC);

#else

    fir_filter_basic(input, output, new_length,  FIR_TAPS_NUMBER, filter_taps, SHARPENED_FIR_SCALING_ACC);

#endif
    free(filter_taps_fixed_point);
  }
  else
  {
    output = input;
  }

  /* new length of samples should be computed based on the rate change */
  current_rate_change = fir_rate_change;
  new_length = new_length/current_rate_change;

  /* filter has created a delay on the signal which can be compensated */
  //int delay = FIR_TAPS_NUMBER;
  int delay = 0;

  for (int i = 0; (i*fir_rate_change) < new_length; i++) {
    output_buffer[2*i] = output[(2*i+delay)*current_rate_change]>> scaling_factor;
    output_buffer[2*i+1] = output[((2*i+delay)*current_rate_change)+1]>> scaling_factor;

#ifdef DBG_CIC_FILTER_NR
    if (i < 10) {
      printf("output_buffer[%d] = output[%d] >> scaling_factor \n", (2*i), ((2*i+delay)*current_rate_change));
      printf("      %d           =     %d     >> %d \n", output_buffer[2*i], output[((2*i+delay)*current_rate_change)], scaling_factor);
    }
#endif
  }

#else

  scaling_factor = 0;

  /* copy of final result from int32 to int16 with a scaling factor and a rate change */
  for (int i = 0; i < new_length; i++) {
    output_buffer[2*i] = input[2*i] >> scaling_factor;
    output_buffer[2*i + 1] = input[2*i + 1] >> scaling_factor;

#ifdef DBG_CIC_FILTER_NR
    if (i < 10) {
      printf("output_buffer[%d] : %d \n", (2*i), output_buffer[2*i]);
      printf("input[%d] : %d \n", 2*i, input[2*i] >> scaling_factor);
    }
#endif
  }

#endif

#if 1
  /* clear till end of the buffer */
  for (int i = new_length; i < length; i++) {
    output_buffer[2*i] = 0;
    output_buffer[2*i + 1] = 0;
  }
#endif
}

/*******************************************************************
*
* NAME :         fir_decimator
*
* PARAMETERS :   pointer to input complex data
*                pointer to output complex data
*                length size of input buffer
*                decimation by a factor of R
*
* RETURN :       result of decimator is the output buffer
*
* DESCRIPTION :  fir filter
*                rate change by a factor of R
*
*
*********************************************************************/

void fir_decimator(int16_t *input_buffer, int16_t *output_buffer, int length, int rate_change, int scaling_factor)
{
  int32_t *buffer_one;
  int32_t *buffer_two;
  int32_t *input;
  int32_t *output;
  int new_length;

  /* get working buffers */
  buffer_one = malloc(length*sizeof(int32_t) * 2);   /* for i&q samples */
  if (buffer_one == NULL) {
    msg("Fatal memory allocation problem \n");
    assert(0);
  }

  buffer_two = malloc(length*sizeof(int32_t) * 2);  /* for i&q samples */
  if (buffer_two == NULL) {
    msg("Fatal memory allocation problem \n");
    assert(0);
  }

  /* copy of input buffer from int16 to int32 */
  for (int i = 0; i < length; i++) {
    buffer_one[2*i]     = input_buffer[2*i];
    buffer_one[2*i + 1] = input_buffer[2*i + 1];
  }

  input = buffer_one;
  output = buffer_two;

#if 1

  fir_filter(input, output, length, SAMPLEFILTER_TAP_NUM, filter_taps, FIR_SCALING_ACC);

#else

  fir_filter_basic(input, output, length,  SAMPLEFILTER_TAP_NUM, filter_taps, FIR_SCALING_ACC);

#endif

  /* rate change from sampling frequency Fs to Fs/R */
  rate_change_stage(output, input, length, rate_change);

  new_length = length/rate_change;

  /* copy of final result from int32 to int16 with a scaling factor and a rate change */
  for (int i = 0; i < new_length; i++) {
    output_buffer[2*i] = input[2*i] >> scaling_factor;
    output_buffer[2*i + 1] = input[2*i + 1] >> scaling_factor;
  }

#if 1
  /* clear till end of the buffer */
  for (int i = new_length; i < length; i++) {
    output_buffer[2*i] = 0;
    output_buffer[2*i + 1] = 0;
  }
#endif

}







