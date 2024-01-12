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

/************** DEFINE ********************************************/

#define  SAMPLES_IQ                   (sizeof(int16_t)*2)
#define  NUMBER_SSS_SEQUENCE          (336)
#define  INVALID_SSS_SEQUENCE         (NUMBER_SSS_SEQUENCE)
#define  LENGTH_SSS_NR                (127)
#define  SCALING_METRIC_SSS_NR        (15)//(19)

#define  N_ID_2_NUMBER                (NUMBER_PSS_SEQUENCE)
#define  N_ID_2_NUMBER_SL             (NUMBER_PSS_SEQUENCE_SL)
#define  N_ID_1_NUMBER                (NUMBER_SSS_SEQUENCE)

#define  GET_NID2(Nid_cell)           (Nid_cell%3)
#define  GET_NID1(Nid_cell)           (Nid_cell/3)

#define  GET_NID2_SL(Nid_SL)          (Nid_SL/NUMBER_SSS_SEQUENCE)
#define  GET_NID1_SL(Nid_SL)          (Nid_SL%NUMBER_SSS_SEQUENCE)

#define  PSS_SC_START_NR              (52)     /* see from TS 38.211 table 7.4.3.1-1: Resources within an SS/PBCH block for PSS... */

#define  SSS_START_IDX                (3)      /* [0:PSBCH 1:PSS0 2:PSS1 3:SSS0 4:SSS1] */
#define  NUM_SSS_SYMBOLS              (2)

#define SSS_METRIC_FLOOR_NR   (30000)

/************** VARIABLES *****************************************/

#define PHASE_HYPOTHESIS_NUMBER       (16)
#define INDEX_NO_PHASE_DIFFERENCE (3) /* this is for no phase shift case */
/************** FUNCTION ******************************************/

void init_context_sss_nr(int amp);
void free_context_sss_nr(void);

void insert_sss_nr(int16_t *sss_time,
                   NR_DL_FRAME_PARMS *frame_parms);

bool rx_sss_nr(PHY_VARS_NR_UE *ue,
               const UE_nr_rxtx_proc_t *proc,
               int32_t *tot_metric,
               uint8_t *phase_max,
               int *freq_offset_sss,
               c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]);
#endif /* SSS_NR_H */

