/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/***********************************************************************
*
* FILENAME    :  sss_nr.h
*
* MODULE      :  Secondary synchronisation signal
*
* DESCRIPTION :  variables related to sss
*
************************************************************************/

#ifndef SSS_NR_H
#define SSS_NR_H

#include "limits.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/types.h"

#include "pss_nr.h"

#ifdef DEFINE_VARIABLES_SSS_NR_H
#define EXTERN
#define INIT_VARIABLES_SSS_NR_H
#else
#define EXTERN  extern
#endif

/************** DEFINE ********************************************/

#define  SAMPLES_IQ                   (sizeof(int16_t)*2)
#define  NUMBER_SSS_SEQUENCE          (336)
#define  INVALID_SSS_SEQUENCE         (NUMBER_SSS_SEQUENCE)
#define  LENGTH_SSS_NR                (127)
#define  SCALING_METRIC_SSS_NR        (15)//(19)

#define  N_ID_2_NUMBER                (NUMBER_PSS_SEQUENCE)
#define  N_ID_1_NUMBER                (NUMBER_SSS_SEQUENCE)

#define  GET_NID2(Nid_cell)           (Nid_cell%3)
#define  GET_NID1(Nid_cell)           (Nid_cell/3)

#define  PSS_SC_START_NR              (52)     /* see from TS 38.211 table 7.4.3.1-1: Resources within an SS/PBCH block for PSS... */

/************** VARIABLES *****************************************/

#define PHASE_HYPOTHESIS_NUMBER       (16)
#define INDEX_NO_PHASE_DIFFERENCE     (3)          /* this is for no phase shift case */

EXTERN const int16_t phase_re_nr[PHASE_HYPOTHESIS_NUMBER]
#ifdef INIT_VARIABLES_SSS_NR_H
// -pi/3 ---- pi/3
= {16384,20173,23571,26509,28932,30791,32051,32687,32687,32051,30791,
   28932,26509,23571,20173,16384}
#endif
;

EXTERN const int16_t phase_im_nr[PHASE_HYPOTHESIS_NUMBER]
#ifdef INIT_VARIABLES_SSS_NR_H
// -pi/3 ---- pi/3
= {-28377,-25821,-22762,-19260,-15383,-11207,-6813,-2286,2286,6813,11207,
   15383,19260,22762,25821,28377}
#endif
;

EXTERN int16_t d_sss[N_ID_2_NUMBER][N_ID_1_NUMBER][LENGTH_SSS_NR];

/************** FUNCTION ******************************************/

void init_context_sss_nr(int amp);
void free_context_sss_nr(void);

void insert_sss_nr(int16_t *sss_time,
                   NR_DL_FRAME_PARMS *frame_parms);

int pss_ch_est_nr(PHY_VARS_NR_UE *ue,
                  int32_t pss_ext[NB_ANTENNAS_RX][LENGTH_PSS_NR],
                  int32_t sss_ext[NB_ANTENNAS_RX][LENGTH_SSS_NR]);

int rx_sss_nr(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, int32_t *tot_metric, uint8_t *phase_max);

#undef INIT_VARIABLES_SSS_NR_H
#undef EXTERN

#endif /* SSS_NR_H */

