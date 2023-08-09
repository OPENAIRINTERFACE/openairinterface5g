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
* FILENAME    :  pss_nr.h
*
* MODULE      :  primary synchronisation signal
*
* DESCRIPTION :  elements related to pss
*
************************************************************************/

#ifndef PSS_NR_H
#define PSS_NR_H

#include "PHY/defs_nr_UE.h"
#include "PHY/types.h"

#include "PHY/NR_REFSIG/ss_pbch_nr.h"

/************** CODE GENERATION ***********************************/

//#define PSS_DECIMATOR                          /* decimation of sample is done between time correlation */

//#define CIC_DECIMATOR                          /* it allows enabling decimation based on CIC filter. By default, decimation is based on a FIF filter */

#define TEST_SYNCHRO_TIMING_PSS        (1)       /* enable time profiling */

//#define DBG_PSS_NR

/************** DEFINE ********************************************/

/* PROFILING */
#define TIME_PSS                      (0)
#define TIME_RATE_CHANGE              (TIME_PSS+1)
#define TIME_SSS                      (TIME_RATE_CHANGE+1)
#define TIME_LAST                     (TIME_SSS+1)

/* PSS configuration */

#define SYNCHRO_FFT_SIZE_MAX           (8192)                       /* maximum size of fft for synchronisation */

#define  NO_RATE_CHANGE                (1)

#ifdef PSS_DECIMATOR
  #define  RATE_CHANGE                 (SYNCHRO_FFT_SIZE_MAX/SYNCHRO_FFT_SIZE_PSS)
  #define  SYNCHRO_FFT_SIZE_PSS        (256)
  #define  OFDM_SYMBOL_SIZE_PSS        (SYNCHRO_FFT_SIZE_PSS)
  #define  SYNCHRO_RATE_CHANGE_FACTOR  (SYNCHRO_FFT_SIZE_MAX/SYNCHRO_FFT_SIZE_PSS)
  #define  CIC_FILTER_STAGE_NUMBER     (4)
#else
  #define  RATE_CHANGE                 (1)
  #define  SYNCHRO_RATE_CHANGE_FACTOR  (1)
#endif

#define SYNC_TMP_SIZE                  (NB_ANTENNAS_RX*SYNCHRO_FFT_SIZE_MAX*IQ_SIZE) /* to be aligned with existing lte synchro */
#define SYNCF_TMP_SIZE                 (SYNCHRO_FFT_SIZE_MAX*IQ_SIZE)

void init_context_synchro_nr(NR_DL_FRAME_PARMS *frame_parms_ue);
void free_context_synchro_nr(void);
int pss_synchro_nr(PHY_VARS_NR_UE *PHY_vars_UE, int is, int rate_change);
int16_t *get_primary_synchro_nr2(const int nid2);

#endif /* PSS_NR_H */


