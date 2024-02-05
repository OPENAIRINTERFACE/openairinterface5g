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
* FILENAME    :  harq_nr.c
*
* MODULE      :  HARQ
*
* DESCRIPTION :  functions related to HARQ feature (Hybrid Automatic Repeat Request Acknowledgment)
*                This feature allows to acknowledge downlink and uplink transport blocks
*                TS 38.214 5.1 UE procedure for transmitting the physical downlink shared channel
*                TS 38.214 6.1 UE procedure for transmitting the physical uplink shared channel
*                TS 38.214 6.1.2.1 Resource allocation in time domain
*                TS 38.212 7.3 Downlink control information
*                TS 38.213 9.2.3 UE procedure for reporting HARQ-ACK
*                TS 38.321 5.4.1 UL Grant reception
*                TS 38.321 5.4.2.1 HARQ Entity
*
*  Downlink HARQ mechanism
*  -----------------------
*  A downlink DCI is received in a PDCCH.
*  Then received parameters are communicated to HARQ entity (including NDI new data indicator and K which is the number of slots
*  between current reception and transmission of this downlink acknowledgment.
*
*            Reception on slot n                                        transmission of acknowledgment
*                                                                               slot k
*                                                                      ---+---------------+---
*                                                                         |               |
*                Frame                                                    | PUCCH / PUSCH |
*                Subframe                                                 |               |
*                Slot n                                                ---+------------+--+---
*           ---+-------------+---                                       / |
*              |   PDCCH     |                                         /  |
*              |    DCI      |                                        /   |
*              |   downlink  |                      +---------------+/    |
*              |     NDI--->------------->----------| downlink HARQ |     |
*              |     k       |                      |    entity     |     |
*           ---+-----|-------+---                   +---------------+     |
*                    |       |                                            |
*                    v       |/__________________________________________\|
*                    |        \ slot between reception and transmission  /|
*                    |________________________^
*
*  Uplink HARQ mechanism
*  ---------------------
*  An uplink DCI is received in a PDCCH.
*  Then received parameters are communicated to HARQ entity (including NDI new data indicator and K which is the number of slots
*  between current reception and related PUSCH transmission).
*  Uplink HARQ entity decides to transmit a new block or to retransmit current one.
*  transmission/retransmission parameters should be determined based on received parameters.
*
*            Reception on slot n                                        Transmission on slot k
*                                                                               slot k
*                                                                      ---+---------------+---
*                                                                         |    PUSCH      |
*                Frame                                                    | Transmission  |
*                Subframe                                                 | Retransmission|
*                Slot n                                                ---+------------+--+---
*           ---+-------------+---                                       / |
*              |   PDCCH     |                                         /  |
*              |    DCI      |                                        /   |
*              |   uplink    |                        +-------------+/    |
*              |     NDI--->------------->----------->| uplink HARQ |     |
*              |     k       |                        |   entity    |     |
*           ---+-----|-------+---                     +-------------+     |
*                    |       |                                            |
*                    v       |/__________________________________________\|
*                    |        \ slot between reception and transmission  /|
*                    |________________________^

************************************************************************/

#include "PHY/defs_nr_UE.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "SCHED_NR_UE/harq_nr.h"

/********************* define **************************************/

#define DL_DCI              (1)
#define UL_DCI              (0)

/*******************************************************************
*
* NAME :         init_downlink_harq_status
*
* PARAMETERS :   pointer to dl harq status
*
* RETURN :       none
*
* DESCRIPTION :  initialisation of downlink HARQ status
*
*********************************************************************/

void init_downlink_harq_status(NR_DL_UE_HARQ_t *dl_harq)
{
  dl_harq->status = SCH_IDLE;
  dl_harq->first_rx = 1;
  dl_harq->DLround  = 0;
  dl_harq->ack = DL_ACKNACK_NO_SET;
}

/*******************************************************************
*
* NAME :         downlink_harq_process
*
* PARAMETERS :   downlink harq context
*                harq identifier
*                ndi (new data indicator) from DCI
*                rnti_type from DCI
*
* RETURN :      none
*
* DESCRIPTION : manage downlink information from DCI for downlink transmissions/retransmissions
*               TS 38.321 5.3.1 DL Assignment reception
*               TS 38.321 5.3.2 HARQ operation
*
*********************************************************************/

void downlink_harq_process(NR_DL_UE_HARQ_t *dl_harq, int harq_pid, int dci_ndi, int rv, uint8_t rnti_type) {

  if (rnti_type == TYPE_SI_RNTI_ ||
      rnti_type == TYPE_P_RNTI_ ||
      rnti_type == TYPE_RA_RNTI_) {
    dl_harq->DLround = 0;
    dl_harq->status = ACTIVE;
    dl_harq->first_rx = 1;
  }  else {
    LOG_D(PHY,
          "receive harq process: %p harqPid=%d, rv=%d, ndi=%d, rntiType=%d new transmission= %s\n",
          dl_harq,
          harq_pid,
          rv,
          dci_ndi,
          rnti_type,
          dci_ndi ? "yes" : "no");
    AssertFatal(rv<4 && rv>=0, "invalid redondancy version %d\n", rv);
    dl_harq->status = ACTIVE;
  }
}

