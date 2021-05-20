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

/***********************************************************************
*
* FILENAME    :  harq_nr.h
*
* MODULE      :  HARQ
*
* DESCRIPTION :  header related to Hybrid Automatic Repeat Request Acknowledgment
*                This feature allows to acknowledge downlink and uplink transport blocks
*
************************************************************************/

#ifndef HARQ_NR_H
#define HARQ_NR_H

/************** DEFINE ********************************************/

#if 0

/* these define are in file PHY/impl_defs_top.h" because of compilation problems due to multiple header files inclusions */
#define NR_MAX_HARQ_PROCESSES                    (16)
#define NR_MAX_ULSCH_HARQ_PROCESSES              (NR_MAX_HARQ_PROCESSES)  /* TS 38.214 6.1 UE procedure for receiving the physical uplink shared channel */
#define NR_MAX_DLSCH_HARQ_PROCESSES              (NR_MAX_HARQ_PROCESSES)  /* TS 38.214 5.1 UE procedure for receiving the physical downlink shared channel */

#endif

#define NR_DEFAULT_DLSCH_HARQ_PROCESSES          (8)                      /* TS 38.214 5.1 */

#define DL_ACKNACK_NO_SET                        (2)
#define DL_NACK                                  (0)
#define DL_ACK                                   (1)
#define DL_MAX_NB_FEEDBACK                       (NR_DL_MAX_DAI * NR_DL_MAX_NB_CW)
#define DL_DAI_NO_SET                            (0xFF)
#define UL_DAI_NO_SET                            (DL_DAI_NO_SET)

/************** INCLUDE *******************************************/

#include "PHY/defs_nr_UE.h"

/************* TYPE ***********************************************/


/************** VARIABLES *****************************************/


/*************** FUNCTIONS ****************************************/


/** \brief This function configures uplink HARQ context
    @param PHY_VARS_NR_UE ue context
    @param gNB_id gNodeB identifier
    @param thread_id RXTX thread index
    @param code_word_idx code word index
    @param number_harq_processes_pusch maximum number of uplink HARQ processes
    @returns none */

void config_uplink_harq_process(PHY_VARS_NR_UE *ue, int gNB_id, int thread_id, int code_word_idx, uint8_t number_harq_processes_pusch);

/** \brief This function releases uplink HARQ context
    @param PHY_VARS_NR_UE ue context
    @param gNB_id gNodeB identifier
    @param thread_id RXTX thread index
    @param code_word_idx code word index
    @returns none */

void release_uplink_harq_process(PHY_VARS_NR_UE *ue, int gNB_id, int thread_id, int code_word_idx);

/** \brief This function stores slot for transmission in HARQ context
    @param ulsch uplink context
    @param harq process identifier harq_pid
    @param slot_tx slot for transmission related to current downlink PDCCH
    @returns 0 none */

void set_tx_harq_id(NR_UE_ULSCH_t *ulsch, int harq_pid, int slot_tx);

/** \brief This function initialises context of an uplink HARQ process
    @param ulsch uplink context
    @param harq process identifier harq_pid
    @returns harq number for tx slot */

int get_tx_harq_id(NR_UE_ULSCH_t *ulsch, int slot_tx);

/** \brief This function update uplink harq context and return transmission type (new transmission or retransmission)
    @param ulsch uplink harq context
    @param harq process identifier harq_pid
    @returns retransmission or new transmission */

harq_result_t uplink_harq_process(NR_UE_ULSCH_t *ulsch, int harq_pid, int ndi, uint8_t rnti_type);

/** \brief This function initialises downlink HARQ status
    @param pointer to downlink harq status
    @returns none */

void init_downlink_harq_status(NR_DL_UE_HARQ_t *dl_harq);


/** \brief This function update downlink harq context and return reception type (new transmission or retransmission)
    @param dlsch downlink harq context
    @param harq process identifier harq_pid
    @param rnti_type type of rnti
    @returns retransmission or new transmission */

void downlink_harq_process(NR_DL_UE_HARQ_t *dlsch, int harq_pid, int ndi, uint8_t rnti_type);

#undef EXTERN
#undef INIT_VARIABLES_HARQ_NR_H

#endif /* HARQ_NR_H */
