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
* FILENAME    :  pss_nr.c
*
* MODULE      :  synchronisation signal
*
* DESCRIPTION :  generation of pss
*                3GPP TS 38.211 7.4.2.2 Primary synchronisation signal
*
************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "PHY/defs_nr_UE.h"

#include "PHY/NR_REFSIG/ss_pbch_nr.h"

#define DEFINE_VARIABLES_PSS_NR_H
#include "PHY/NR_REFSIG/pss_nr.h"
#undef DEFINE_VARIABLES_PSS_NR_H

#include "PHY/NR_REFSIG/sss_nr.h"
#include "PHY/NR_UE_TRANSPORT/cic_filter_nr.h"

/*******************************************************************
*
* NAME :         get_idft
*
* PARAMETERS :   size of ofdm symbol
*
* RETURN :       function idft
*
* DESCRIPTION :  get idft function depending of ofdm size
*
*********************************************************************/

void *get_idft(int ofdm_symbol_size)
{
  void (*idft)(int16_t *,int16_t *, int);

  switch (ofdm_symbol_size) {
    case 128:
      idft = idft128;
      break;

    case 256:
      idft = idft256;
      break;

    case 512:
      idft = idft512;
      break;

    case 1024:
      idft = idft1024;
      break;

    case 1536:
      idft = idft1536;
      break;

    case 2048:
      idft = idft2048;
      break;

    default:
      printf("function get_idft : unsupported ofdm symbol size \n");
      assert(0);
      break;
 }
 return idft;
}

/*******************************************************************
*
* NAME :         get_dft
*
* PARAMETERS :   size of ofdm symbol
*
* RETURN :       function for discrete fourier transform
*
* DESCRIPTION :  get dft function depending of ofdm size
*
*********************************************************************/

void *get_dft(int ofdm_symbol_size)
{
  void (*dft)(int16_t *,int16_t *, int);

  switch (ofdm_symbol_size) {
    case 128:
      dft = dft128;
      break;

    case 256:
      dft = dft256;
      break;

    case 512:
      dft = dft512;
      break;

    case 1024:
      dft = dft1024;
      break;

    case 1536:
      dft = dft1536;
      break;

    case 2048:
      dft = dft2048;
      break;

    default:
      printf("function get_dft : unsupported ofdm symbol size \n");
      assert(0);
      break;
 }
 return dft;
}

/*******************************************************************
*
* NAME :         generate_pss_nr
*
* PARAMETERS :   N_ID_2 : element 2 of physical layer cell identity
*                value : { 0, 1, 2}
*
* RETURN :       generate binary pss sequence (this is a m-sequence)
*
* DESCRIPTION :  3GPP TS 38.211 7.4.2.2 Primary synchronisation signal
*                Sequence generation
*
*********************************************************************/

void generate_pss_nr(int N_ID_2, int ofdm_symbol_size)
{
  int16_t d_pss[LENGTH_PSS_NR];
  int16_t x[LENGTH_PSS_NR];
  int16_t *primary_synchro_time = primary_synchro_time_nr[N_ID_2];
  unsigned int length = ofdm_symbol_size;
  unsigned int size = length * IQ_SIZE; /* i & q */
  int16_t *primary_synchro = primary_synchro_nr[N_ID_2]; /* pss in complex with alternatively i then q */
  void (*idft)(int16_t *,int16_t *, int);

  #define INITIAL_PSS_NR    (7)
  const int x_initial[INITIAL_PSS_NR] = {0, 1, 1 , 0, 1, 1, 1};

  assert(N_ID_2 < NUMBER_PSS_SEQUENCE);
  assert(size <= SYNCF_TMP_SIZE);
  assert(size <= SYNC_TMP_SIZE);

  bzero(synchroF_tmp, size);
  bzero(synchro_tmp, size);

  for (int i=0; i < INITIAL_PSS_NR; i++) {
    x[i] = x_initial[i];
  }

  for (int i=0; i < (LENGTH_PSS_NR - INITIAL_PSS_NR); i++) {
    x[i+INITIAL_PSS_NR] = (x[i + 4] + x[i])%(2);
  }

  for (int n=0; n < LENGTH_PSS_NR; n++) {
	int m = (n + 43*N_ID_2)%(LENGTH_PSS_NR);
    d_pss[n] = 1 - 2*x[m];
  }

  /* PSS is directly mapped to subcarrier without modulation 38.211 */
  for (int i=0; i < LENGTH_PSS_NR; i++) {
#if 1
    primary_synchro[2*i] = (d_pss[i] * SHRT_MAX)>>SCALING_PSS_NR; /* Maximum value for type short int ie int16_t */
    primary_synchro[2*i+1] = 0;
#else
    primary_synchro[2*i] = d_pss[i] * AMP;
    primary_synchro[2*i+1] = 0;
#endif
  }

#ifdef DBG_PSS_NR

  if (N_ID_2 == 0) {
    char output_file[255];
    char sequence_name[255];
    sprintf(output_file, "pss_seq_%d_%d.m", N_ID_2, length);
    sprintf(sequence_name, "pss_seq_%d_%d", N_ID_2, length);
    printf("file %s sequence %s\n", output_file, sequence_name);

    write_output(output_file, sequence_name, primary_synchro, LENGTH_PSS_NR, 1, 1);
  }

#endif

  /* call of IDFT should be done with ordered input as below
  *
  *                n input samples
  *  <------------------------------------------------>
  *  0                                                n
  *  are written into input buffer for IFFT
  *   -------------------------------------------------
  *  |xxxxxxx                       N/2       xxxxxxxx|
  *  --------------------------------------------------
  *  ^      ^                 ^               ^          ^
  *  |      |                 |               |          |
  * n/2    end of            n=0            start of    n/2-1
  *         pss                               pss
  *
  *                   Frequencies
  *      positives                   negatives
  * 0                 (+N/2)(-N/2)
  * |-----------------------><-------------------------|
  *
  * sample 0 is for continuous frequency which is used here
  */

  unsigned int k = length - (LENGTH_PSS_NR/2+1);

  for (int i=0; i < LENGTH_PSS_NR; i++) {
    synchroF_tmp[2*k] = primary_synchro[2*i];
    synchroF_tmp[2*k+1] = primary_synchro[2*i+1];

    k++;

    if (k >= length) {
      k++;
      k-=length;
    }
  }

  /* IFFT will give temporal signal of Pss */

  idft = get_idft(length);

  idft(synchroF_tmp,          /* complex input */
       synchro_tmp,           /* complex output */
       1);                 /* scaling factor */

  /* then get final pss in time */
  for (unsigned int i=0; i<length; i++) {
    ((int32_t *)primary_synchro_time)[i] = ((int32_t *)synchro_tmp)[i];
  }

#ifdef DBG_PSS_NR

  if (N_ID_2 == 0) {
    char output_file[255];
    char sequence_name[255];
    sprintf(output_file, "%s%d_%d%s","pss_seq_t_", N_ID_2, length, ".m");
    sprintf(sequence_name, "%s%d_%d","pss_seq_t_", N_ID_2, length);

    printf("file %s sequence %s\n", output_file, sequence_name);

    write_output(output_file, sequence_name, primary_synchro_time, length, 1, 1);
  }

#endif


#if 0

/* it allows checking that process of idft on a signal and then dft gives same signal with limited errors */

  if ((N_ID_2 == 0) && (length == 256)) {

    write_output("pss_f00.m","pss_f00",synchro_tmp,length,1,1);


    bzero(synchroF_tmp, size);

    void (*dft)(int16_t *,int16_t *, int) = get_dft(length);

    /* get pss in the time domain by applying an inverse FFT */
    dft(synchro_tmp,           /* complex input */
        synchroF_tmp,          /* complex output */
        1);                 /* scaling factor */

    if ((N_ID_2 == 0) && (length == 256)) {
      write_output("pss_f_0.m","pss_f_0",synchroF_tmp,length,1,1);
    }

    /* check Pss */
    k = length - (LENGTH_PSS_NR/2);

#define LIMIT_ERROR_FFT   (10)

    for (int i=0; i < LENGTH_PSS_NR; i++) {
      if (abs(synchroF_tmp[2*k] - primary_synchro[2*i]) > LIMIT_ERROR_FFT) {
      printf("Pss Error[%d] Compute %d Reference %d \n", k, synchroF_tmp[2*k], primary_synchro[2*i]);
      }
    
      if (abs(synchroF_tmp[2*k+1] - primary_synchro[2*i+1]) > LIMIT_ERROR_FFT) {
        printf("Pss Error[%d] Compute %d Reference %d\n", (2*k+1), synchroF_tmp[2*k+1], primary_synchro[2*i+1]);
      }

      k++;

      if (k >= length) {
        k-=length;
      }
    }
  }
#endif
}

/*******************************************************************
*
* NAME :         init_context_pss_nr
*
* PARAMETERS :   structure NR_DL_FRAME_PARMS give frame parameters
*
* RETURN :       generate binary pss sequences (this is a m-sequence)
*
* DESCRIPTION :  3GPP TS 38.211 7.4.2.2 Primary synchronisation signal
*                Sequence generation
*
*********************************************************************/

void init_context_pss_nr(NR_DL_FRAME_PARMS *frame_parms_ue)
{
  int ofdm_symbol_size = frame_parms_ue->ofdm_symbol_size;
  int sizePss = LENGTH_PSS_NR * IQ_SIZE;  /* complex value i & q signed 16 bits */
  int size = ofdm_symbol_size * IQ_SIZE; /* i and q samples signed 16 bits */
  int16_t *p = NULL;
  int *q = NULL;

  for (int i = 0; i < NUMBER_PSS_SEQUENCE; i++) {

    p = malloc16(sizePss); /* pss in complex with alternatively i then q */
    if (p != NULL) {
      primary_synchro_nr[i] = p;
      bzero( primary_synchro_nr[i], sizePss);
    }
    else {
     msg("Fatal memory allocation problem \n");
     assert(0);
    }

    p = malloc16(size);
    if (p != NULL) {
      primary_synchro_time_nr[i] = p;
      bzero( primary_synchro_time_nr[i], size);
    }
    else {
     msg("Fatal memory allocation problem \n");
     assert(0);
    }

    size = LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*sizeof(int)*frame_parms_ue->samples_per_subframe;
    q = malloc16(size);
    if (q != NULL) {
      pss_corr_ue[i] = q;
      bzero( pss_corr_ue[i], size);
    }
    else {
      msg("Fatal memory allocation problem \n");
      assert(0);
    }

    generate_pss_nr(i, ofdm_symbol_size);
  }
}

/*******************************************************************
*
* NAME :         free_context_pss_nr
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  free context related to pss
*
*********************************************************************/

void free_context_pss_nr(void)
{
  for (int i = 0; i < NUMBER_PSS_SEQUENCE; i++) {

    if (primary_synchro_time_nr[i] != NULL) {
      free(primary_synchro_time_nr[i]);
      primary_synchro_time_nr[i] = NULL;
    }
    else {
      msg("Fatal memory deallocation problem \n");
      assert(0);
    }

    if (primary_synchro_nr[i] != NULL) {
      free(primary_synchro_nr[i]);
      primary_synchro_nr[i] = NULL;
    }
    else {
      msg("Fatal memory deallocation problem \n");
      assert(0);
    }

    if (pss_corr_ue[i] != NULL) {
      free(pss_corr_ue[i]);
      pss_corr_ue[i] = NULL;
    }
    else {
      msg("Fatal memory deallocation problem \n");
      assert(0);
    }
  }
}

/*******************************************************************
*
* NAME :         init_context_synchro_nr
*
* PARAMETERS :   none
*
* RETURN :       generate context for pss and sss
*
* DESCRIPTION :  initialise contexts and buffers for synchronisation
*
*********************************************************************/

void init_context_synchro_nr(NR_DL_FRAME_PARMS *frame_parms_ue)
{
#ifndef STATIC_SYNC_BUFFER

  /* initialise global buffers for synchronisation */
  synchroF_tmp = malloc16(SYNCF_TMP_SIZE);
  if (synchroF_tmp == NULL) {
    msg("Fatal memory allocation problem \n");
    assert(0);
  }

  synchro_tmp = malloc16(SYNC_TMP_SIZE);
  if (synchro_tmp == NULL) {
    msg("Fatal memory allocation problem \n");
    assert(0);
  }

#endif

  init_context_pss_nr(frame_parms_ue);

  init_context_sss_nr(AMP);
}

/*******************************************************************
*
* NAME :         free_context_synchro_nr
*
* PARAMETERS :   none
*
* RETURN :       free context for pss and sss
*
* DESCRIPTION :  deallocate memory of synchronisation
*
*********************************************************************/

void free_context_synchro_nr(void)
{
#ifndef STATIC_SYNC_BUFFER

  if (synchroF_tmp != NULL) {
    free(synchroF_tmp);
    synchroF_tmp = NULL;
  }
  else {
    msg("Fatal memory deallocation problem \n");
    assert(0);
  }

  if (synchro_tmp != NULL) {
    free(synchro_tmp);
    synchro_tmp = NULL;
  }
  else {
    msg("Fatal memory deallocation problem \n");
    assert(0);
  }

#endif

  free_context_pss_nr();
}

/*******************************************************************
*
* NAME :         set_frame_context_pss_nr
*
* PARAMETERS :   configuration for UE with new FFT size
*
* RETURN :       0 if OK else error
*
* DESCRIPTION :  initialisation of UE contexts
*
*********************************************************************/

void set_frame_context_pss_nr(NR_DL_FRAME_PARMS *frame_parms_ue, int rate_change)
{
  /* set new value according to rate_change */
  frame_parms_ue->ofdm_symbol_size = (frame_parms_ue->ofdm_symbol_size / rate_change);
  frame_parms_ue->samples_per_tti = (frame_parms_ue->samples_per_tti / rate_change);
  frame_parms_ue->samples_per_subframe = (frame_parms_ue->samples_per_subframe / rate_change);

  free_context_pss_nr();

  /* pss reference have to be rebuild with new parameters ie ofdm symbol size */
  init_context_synchro_nr(frame_parms_ue);

#ifdef SYNCHRO_DECIMAT
  set_pss_nr(frame_parms_ue->ofdm_symbol_size);
#endif
}

/*******************************************************************
*
* NAME :         restore_frame_context_pss_nr
*
* PARAMETERS :   configuration for UE and eNB with new FFT size
*
* RETURN :       0 if OK else error
*
* DESCRIPTION :  initialisation of UE and eNode contexts
*
*********************************************************************/

void restore_frame_context_pss_nr(NR_DL_FRAME_PARMS *frame_parms_ue, int rate_change)
{
  frame_parms_ue->ofdm_symbol_size = frame_parms_ue->ofdm_symbol_size * rate_change;
  frame_parms_ue->samples_per_tti = frame_parms_ue->samples_per_tti * rate_change;
  frame_parms_ue->samples_per_subframe = frame_parms_ue->samples_per_subframe * rate_change;

  free_context_pss_nr();

  /* pss reference have to be rebuild with new parameters ie ofdm symbol size */
  init_context_synchro_nr(frame_parms_ue);
#ifdef SYNCHRO_DECIMAT
  set_pss_nr(frame_parms_ue->ofdm_symbol_size);
#endif
}

/********************************************************************
*
* NAME :         decimation_synchro_nr
*
* INPUT :        UE context
*                for first and second pss sequence
*                - position of pss in the received UE buffer
*                - number of pss sequence
*
* RETURN :      0 if OK else error
*
* DESCRIPTION :  detect pss sequences in the received UE buffer
*
********************************************************************/

void decimation_synchro_nr(PHY_VARS_NR_UE *PHY_vars_UE, int rate_change, int **rxdata)
{
  NR_DL_FRAME_PARMS *frame_parms = &(PHY_vars_UE->frame_parms);
  int samples_for_frame = LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_tti;

#if TEST_SYNCHRO_TIMING_PSS

  opp_enabled = 1;

  start_meas(&generic_time[TIME_RATE_CHANGE]);

#endif

/* build with cic filter does not work properly. Performances are significantly deteriorated */
#ifdef CIC_DECIMATOR

  cic_decimator((int16_t *)&(PHY_vars_UE->common_vars.rxdata[0][0]), (int16_t *)&(rxdata[0][0]),
                            samples_for_frame, rate_change, CIC_FILTER_STAGE_NUMBER, 0, FIR_RATE_CHANGE);
#else

  fir_decimator((int16_t *)&(PHY_vars_UE->common_vars.rxdata[0][0]), (int16_t *)&(rxdata[0][0]),
                            samples_for_frame, rate_change, 0);

#endif

  set_frame_context_pss_nr(frame_parms, rate_change);

#if TEST_SYNCHRO_TIMING_PSS

  stop_meas(&generic_time[TIME_RATE_CHANGE]);

  printf("Rate change execution duration %5.2f \n", generic_time[TIME_RATE_CHANGE].p_time/(cpuf*1000.0));

#endif
}

/*******************************************************************
*
* NAME :         pss_synchro_nr
*
* PARAMETERS :   int rate_change
*
* RETURN :       position of detected pss
*
* DESCRIPTION :  pss search can be done with sampling decimation.*
*
*********************************************************************/

int pss_synchro_nr(PHY_VARS_NR_UE *PHY_vars_UE, int rate_change)
{
  NR_DL_FRAME_PARMS *frame_parms = &(PHY_vars_UE->frame_parms);
  int synchro_position;
  int **rxdata = NULL;

#ifdef DBG_PSS_NR

  int samples_for_frame = frame_parms->samples_per_subframe*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME;

  write_output("rxdata0_rand.m","rxd0_rand", &PHY_vars_UE->common_vars.rxdata[0][0], samples_for_frame, 1, 1);

#endif

  if (rate_change != 1) {

    rxdata = (int32_t**)malloc16(frame_parms->nb_antennas_rx*sizeof(int32_t*));

    for (int aa=0; aa < frame_parms->nb_antennas_rx; aa++) {
      rxdata[aa] = (int32_t*) malloc16_clear( (frame_parms->samples_per_subframe*10+2048)*sizeof(int32_t));
    }
#ifdef SYNCHRO_DECIMAT

    decimation_synchro_nr(PHY_vars_UE, rate_change, rxdata);

#endif
  }
  else {

    rxdata = PHY_vars_UE->common_vars.rxdata;
  }

#ifdef DBG_PSS_NR

  write_output("rxdata0_des.m","rxd0_des", &rxdata[0][0], samples_for_frame,1,1);

#endif

#if TEST_SYNCHRO_TIMING_PSS

  opp_enabled = 1;

  start_meas(&generic_time[TIME_PSS]);

#endif

  synchro_position = pss_search_time_nr(rxdata,
                                        frame_parms,
                                        (int *)&PHY_vars_UE->common_vars.eNb_id);

#if TEST_SYNCHRO_TIMING_PSS

  stop_meas(&generic_time[TIME_PSS]);

  int duration_ms = generic_time[TIME_PSS].p_time/(cpuf*1000.0);

  #ifndef NR_UNIT_TEST

    printf("PSS execution duration %4d microseconds \n", duration_ms);

  #endif

#endif

#ifdef SYNCHRO_DECIMAT

  if (rate_change != 1) {

    if (rxdata[0] != NULL) {

      for (int aa=0;aa<frame_parms->nb_antennas_rx;aa++) {
        free(rxdata[aa]);
      }

      free(rxdata);
    }

    restore_frame_context_pss_nr(frame_parms, rate_change);  
  }
#endif

  return synchro_position;
}

static inline int abs32(int x)
{
  return (((int)((short*)&x)[0])*((int)((short*)&x)[0]) + ((int)((short*)&x)[1])*((int)((short*)&x)[1]));
}

/*******************************************************************
*
* NAME :         pss_search_time_nr
*
* PARAMETERS :   received buffer
*                frame parameters
*
* RETURN :       position of detected pss
*
* DESCRIPTION :  Synchronisation on pss sequence is based on a time domain correlation between received samples and pss sequence
*                A maximum likelihood detector finds the timing offset (position) that corresponds to the maximum correlation
*                Length of received buffer should be a minimum of 2 frames (see TS 38.213 4.1 Cell search)
*                Search pss in the received buffer is done each 4 samples which ensures a memory alignment to 128 bits (32 bits x 4).
*                This is required by SIMD (single instruction Multiple Data) Extensions of Intel processors
*                Correlation computation is based on a a dot product which is realized thank to SIMS extensions
*
*                                    (x frames)
*     <--------------------------------------------------------------------------->
*
*
*     -----------------------------------------------------------------------------
*     |                      Received UE data buffer                              |
*     ----------------------------------------------------------------------------
*                -------------
*     <--------->|    pss    |
*      position  -------------
*                ^
*                |
*            peak position
*            given by maximum of correlation result
*            position matches beginning of first ofdm symbol of pss sequence
*
*     Remark: memory position should be aligned on a multiple of 4 due to I & Q samples of int16
*             An OFDM symbol is composed of x number of received samples depending of Rf front end sample rate.
*
*     I & Q storage in memory
*
*             First samples       Second  samples
*     ------------------------- -------------------------  ...
*     |     I1     |     Q1    |     I2     |     Q2    |
*     ---------------------------------------------------  ...
*     ^    16  bits   16 bits  ^
*     |                        |
*     ---------------------------------------------------  ...
*     |         sample 1       |    sample   2          |
*    ----------------------------------------------------  ...
*     ^
*
*********************************************************************/

#define DOT_PRODUCT_SCALING_SHIFT    (17)

int pss_search_time_nr(int **rxdata, ///rx data in time domain
                       NR_DL_FRAME_PARMS *frame_parms,
                       int *eNB_id)
{
  unsigned int n, ar, peak_position, peak_value, pss_source;
  int result;
  int synchro_out;
  unsigned int tmp[NUMBER_PSS_SEQUENCE];
  unsigned int length = (LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->ttis_per_subframe*frame_parms->samples_per_tti);  /* 1 frame for now, it should be 2 TODO_NR */

  for (int i = 0; i < NUMBER_PSS_SEQUENCE; i++) {
	  tmp[i] = 0;
    if (pss_corr_ue[i] == NULL) {
      msg("[SYNC TIME] pss_corr_ue[%d] not yet allocated! Exiting.\n", i);
      return(-1);
    }
  }

  peak_value = 0;
  peak_position = 0;
  pss_source = 0;

  /* Search pss in the received buffer each 4 samples which ensures a memory alignment on 128 bits (32 bits x 4 ) */
  /* This is required by SIMD (single instruction Multiple Data) Extensions of Intel processors. */
  /* Correlation computation is based on a a dot product which is realized thank to SIMS extensions */
  for (n=0; n < length; n+=4) {

#ifdef RTAI_ENABLED

    // This is necessary since the sync takes a long time and it seems to block all other threads thus screwing up RTAI. If we pause it for a little while during its execution we give RTAI a chance to catch up with its other tasks.
    if ((n%frame_parms->samples_per_subframe == 0) && (n>0) && (openair_daq_vars.sync_state==0)) {
#ifdef DEBUG_PHY
      msg("[SYNC TIME] pausing for 1000ns, n=%d\n",n);
#endif
      rt_sleep(nano2count(1000));
    }

#endif

    for (int pss_index = 0; pss_index < NUMBER_PSS_SEQUENCE; pss_index++) {

      pss_corr_ue[pss_index][n] = 0; /* clean correlation for position n */

      synchro_out = 0;

      if ( n < (length - frame_parms->ofdm_symbol_size)) {

        /* calculate dot product of primary_synchro_time_nr and rxdata[ar][n] (ar=0..nb_ant_rx) and store the sum in temp[n]; */
        for (ar=0; ar<frame_parms->nb_antennas_rx; ar++) {

          /* perform correlation of rx data and pss sequence ie it is a dot product */
          result  = dot_product((short*)primary_synchro_time_nr[pss_index], (short*) &(rxdata[ar][n]), frame_parms->ofdm_symbol_size, DOT_PRODUCT_SCALING_SHIFT);

          ((short*)pss_corr_ue[pss_index])[2*n] += ((short*) &result)[0];   /* real part */
          ((short*)pss_corr_ue[pss_index])[2*n+1] += ((short*) &result)[1]; /* imaginary part */
          ((short*)&synchro_out)[0] += ((short*) &result)[0];               /* real part */
          ((short*)&synchro_out)[1] += ((short*) &result)[1];               /* imaginary part */
        }
      }

      pss_corr_ue[pss_index][n] = abs32(pss_corr_ue[pss_index][n]);

      /* calculate the absolute value of sync_corr[n] */
      tmp[pss_index] = (abs32(synchro_out)) ;

      if (tmp[pss_index] > peak_value) {
        peak_value = tmp[pss_index];
        peak_position = n;
        pss_source = pss_index;

        //printf("pss_index %d: n %6d peak_value %10d, synchro_out (% 6d,% 6d) \n", pss_index, n, abs32(synchro_out),((int16_t*)&synchro_out)[0],((int16_t*)&synchro_out)[1]);
      }
    }
  }

  *eNB_id = pss_source;

  LOG_I(PHY,"[UE] nr_synchro_time: Sync source = %d, Peak found at pos %d, val = %d (%d dB)\n", pss_source, peak_position, peak_value, dB_fixed(peak_value)/2);

//#ifdef DEBUG_PSS_NR

#define  PSS_DETECTION_FLOOR_NR     (31)
  if ((dB_fixed(peak_value)/2) > PSS_DETECTION_FLOOR_NR) {

    printf("[UE] nr_synchro_time: Sync source = %d, Peak found at pos %d, val = %d (%d dB)\n", pss_source, peak_position, peak_value,dB_fixed(peak_value)/2);
  }
//#endif

#ifdef DBG_PSS_NR

  static debug_cnt = 0;

  if (debug_cnt == 0) {
    write_output("pss_corr_ue0.m","pss_corr_ue0",pss_corr_ue[0],length,1,2);
    write_output("pss_corr_ue1.m","pss_corr_ue1",pss_corr_ue[1],length,1,2);
    write_output("pss_corr_ue2.m","pss_corr_ue2",pss_corr_ue[2],length,1,2);
    write_output("rxdata0.m","rxd0",rxdata[0],length,1,1);
  } else {
    debug_cnt++;
  }

#endif

  return(peak_position);
}

