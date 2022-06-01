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

#ifdef DEFINE_VARIABLES_PSS_NR_H
#define EXTERN
#else
#define EXTERN  extern
#endif

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

/************* STRUCTURES *****************************************/


/************** VARIABLES *****************************************/

//#define STATIC_SYNC_BUFFER

#ifdef STATIC_SYNC_BUFFER
/* buffer defined in file lte_sync_time */
EXTERN int16_t synchro_tmp[SYNC_TMP_SIZE]   __attribute__((aligned(32)));
EXTERN int16_t synchroF_tmp[SYNCF_TMP_SIZE] __attribute__((aligned(32)));
#else
EXTERN int16_t *synchro_tmp;
EXTERN int16_t *synchroF_tmp;
#endif

EXTERN int16_t *primary_synchro_nr[NUMBER_PSS_SEQUENCE]
#ifdef INIT_VARIABLES_PSS_NR_H
= { NULL, NULL, NULL}
#endif
;
EXTERN int16_t *primary_synchro_nr2[NUMBER_PSS_SEQUENCE]
#ifdef INIT_VARIABLES_PSS_NR_H
= { NULL, NULL, NULL}
#endif
;
EXTERN int16_t *primary_synchro_time_nr[NUMBER_PSS_SEQUENCE]
#ifdef INIT_VARIABLES_PSS_NR_H
= { NULL, NULL, NULL}
#endif
;

/* profiling structure */
EXTERN time_stats_t generic_time[TIME_LAST];

#ifndef DEFINE_HEADER_ONLY

/************** FUNCTION ******************************************/

void init_context_synchro_nr(NR_DL_FRAME_PARMS *frame_parms_ue);
void free_context_synchro_nr(void);
void init_context_pss_nr(NR_DL_FRAME_PARMS *frame_parms_ue);
void free_context_pss_nr(void);
int set_pss_nr(int ofdm_symbol_size);
int pss_synchro_nr(PHY_VARS_NR_UE *PHY_vars_UE, int is, int rate_change);
int pss_search_time_nr(int **rxdata, ///rx data in time domain
                       NR_DL_FRAME_PARMS *frame_parms,
		       int fo_flag,
                       int is,
                       int *eNB_id,
		       int *f_off);

#endif
#undef EXTERN

#endif /* PSS_NR_H */


