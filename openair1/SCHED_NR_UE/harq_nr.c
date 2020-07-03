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
* NAME :         get_dci_info_for_harq
*
* PARAMETERS :   pointer to ue context
*                id of current gNB
*                number of uplink processes
*                maximum number of uplink retransmissions
* RETURN :       none
*
* DESCRIPTION :  update HARQ entity with information from DCI
*                TS 38.212 7.3.1.2 DCI formats for scheduling PDSCH
*
*********************************************************************/

void get_dci_info_for_harq(PHY_VARS_NR_UE *ue, NR_DCI_INFO_EXTRACTED_t *nr_dci_info_extracted,
		                   NR_UE_DLSCH_t **dlsch, NR_UE_ULSCH_t *ulsch, uint8_t nr_tti_rx, uint8_t tx_offset)
{
  if (nr_dci_info_extracted->identifier_dci_formats == DL_DCI) {

	dlsch[0]->current_harq_pid = nr_dci_info_extracted->harq_process_number;

	NR_DL_UE_HARQ_t *dl_harq = dlsch[0]->harq_processes[dlsch[0]->current_harq_pid];

    dl_harq->harq_ack.vDAI_DL = nr_dci_info_extracted->dai+1;
    dl_harq->harq_ack.pucch_resource_indicator = nr_dci_info_extracted->pucch_resource_ind;
    dl_harq->harq_ack.slot_for_feedback_ack = (nr_tti_rx + nr_dci_info_extracted->pdsch_to_harq_feedback_time_ind)%ue->frame_parms.ttis_per_subframe;
    dl_harq->harq_ack.harq_id = nr_dci_info_extracted->harq_process_number;
    dl_harq->harq_ack.rx_status = downlink_harq_process(dl_harq, dlsch[0]->current_harq_pid, nr_dci_info_extracted->ndi, dlsch[0]->rnti_type);
  }
  else if (nr_dci_info_extracted->identifier_dci_formats == UL_DCI) {

	/* store harq id for which pusch should be transmitted at rx_slot + tx_offset */
	set_tx_harq_id(ulsch, nr_dci_info_extracted->harq_process_number, (nr_tti_rx + tx_offset)%ue->frame_parms.ttis_per_subframe);
    ulsch->harq_processes[nr_dci_info_extracted->harq_process_number]->tx_status = uplink_harq_process(ulsch, nr_dci_info_extracted->harq_process_number, nr_dci_info_extracted->ndi, ulsch->rnti_type);
  }
}

/*******************************************************************
*
* NAME :         config_uplink_harq_process
*
* PARAMETERS :   pointer to ue context
*                id of current gNB
*                number of uplink processes
*                maximum number of uplink retransmissions
* RETURN :       none
*
* DESCRIPTION :  configuration of uplink HARQ entity
*
*********************************************************************/

void config_uplink_harq_process(PHY_VARS_NR_UE *ue, int gNB_id, int thread_id, int code_word_idx, uint8_t number_harq_processes_pusch)
{
  NR_UE_ULSCH_t *ulsch;

  ulsch = (NR_UE_ULSCH_t *)malloc16(sizeof(NR_UE_ULSCH_t));

  if (ulsch != NULL) {

    memset(ulsch,0,sizeof(NR_UE_ULSCH_t));

    ue->ulsch[thread_id][gNB_id][code_word_idx] = ulsch;
  }
  else {
    LOG_E(PHY, "Fatal memory allocation problem at line %d in function %s of file %s \n", __LINE__ , __func__, __FILE__);
    assert(0);
  }

  ulsch->number_harq_processes_for_pusch = number_harq_processes_pusch;

  /* allocation of HARQ process context */
  for (int harq_pid = 0; harq_pid < number_harq_processes_pusch; harq_pid++) {

    ulsch->harq_processes[harq_pid] = (NR_UL_UE_HARQ_t *)malloc16(sizeof(NR_UL_UE_HARQ_t));

    if (ulsch->harq_processes[harq_pid] == NULL) {
      LOG_E(PHY, "Fatal memory allocation problem at line %d in function %s of file %s \n", __LINE__ , __func__, __FILE__);
      assert(0);
    }

    ulsch->harq_processes[harq_pid]->subframe_scheduling_flag = 0;
    ulsch->harq_processes[harq_pid]->first_tx = 1;
    ulsch->harq_processes[harq_pid]->round  = 0;
  }

  for (int slot_tx = 0; slot_tx < NR_MAX_SLOTS_PER_FRAME; slot_tx++) {
    ue->ulsch[thread_id][gNB_id][code_word_idx]->harq_process_id[slot_tx] = NR_MAX_HARQ_PROCESSES;
  }
}

/*******************************************************************
*
* NAME :         release_uplink_harq_process
*
* PARAMETERS :   pointer to ue context
*                id of current gNB
*
* RETURN :       none
*
* DESCRIPTION :  release of HARQ uplink entity
*
*********************************************************************/

void release_uplink_harq_process(PHY_VARS_NR_UE *ue, int gNB_id, int thread_id, int code_word_idx)
{
  NR_UE_ULSCH_t *ulsch = ue->ulsch[thread_id][gNB_id][code_word_idx];

  for (int process_id = 0; process_id < ulsch->number_harq_processes_for_pusch; process_id++) {

    free16(ulsch->harq_processes[process_id],sizeof(NR_UL_UE_HARQ_t));

    ulsch->harq_processes[process_id] = NULL;
  }

  free16(ulsch, sizeof(NR_UE_ULSCH_t));

  ue->ulsch[thread_id][gNB_id][code_word_idx] = NULL;
}

/*******************************************************************
*
* NAME :         set_tx_harq_id
*
* PARAMETERS :   ue context
*                slot_tx slot for transmission
*                gNB_id identifier
*
* RETURN :       none
*
* DESCRIPTION :  store tx harq process identifier for given transmission slot
*
*********************************************************************/

void set_tx_harq_id(NR_UE_ULSCH_t *ulsch, int harq_pid, int slot_tx)
{
  ulsch->harq_process_id[slot_tx] = harq_pid;
}

/*******************************************************************
*
* NAME :         get_tx_harq_id
*
* PARAMETERS :   ue context
*                slot_tx slot for transmission
*                gNB_id identifier
*
* RETURN :       harq process identifier
*
* DESCRIPTION :  return tx harq process identifier for given slot transmission
*
*********************************************************************/

int get_tx_harq_id(NR_UE_ULSCH_t *ulsch, int slot_tx)
{

  return (ulsch->harq_process_id[slot_tx]);
}

/*******************************************************************
*
* NAME :         uplink_harq_process
*
* PARAMETERS :   ue context
*                slot_tx slot for transmission
*                gNB_id identifier
*                ndi from DCI
*                rnti_type from DCI
*
* RETURN :      true it a new transmission
*               false it is a retransmission
*
* DESCRIPTION : manage uplink grant information for transmissions/retransmissions
*               TS 38.321 5.4.1 UL Grant reception
*               TS 38.321 5.4.2.1 HARQ Entity
*
*********************************************************************/

harq_result_t uplink_harq_process(NR_UE_ULSCH_t *ulsch, int harq_pid, int ndi, uint8_t rnti_type)
{
  harq_result_t result_harq = RETRANSMISSION_HARQ;

  if (rnti_type == _CS_RNTI_) {
    LOG_E(PHY, "Fatal error in HARQ entity due to not supported CS_RNTI at line %d in function %s of file %s \n", __LINE__ , __func__, __FILE__);
 	return(NEW_TRANSMISSION_HARQ);
  }
  else if ((rnti_type != _C_RNTI_) && (rnti_type != _TC_RNTI_)) {
    /* harq mechanism is not relevant for other rnti */
    return(NEW_TRANSMISSION_HARQ);
  }
  else if (harq_pid > ulsch->number_harq_processes_for_pusch) {
    LOG_E(PHY, "Fatal error in HARQ entity due to unknown process identity %d at line %d in function %s of file %s \n", harq_pid, __LINE__ , __func__, __FILE__);
    assert(0);
  }

  /* 38.321 5.4.2.1  2>  if the uplink grant was received on PDCCH for the C-RNTI and the HARQ buffer of the identified process is empty */
  if ((ulsch->harq_processes[harq_pid]->first_tx == 1) && (rnti_type == _C_RNTI_)) {  /* no transmission yet on this process so consider its harq buffer as empty */
    ulsch->harq_processes[harq_pid]->first_tx = 0;
    ulsch->harq_processes[harq_pid]->pusch_pdu.pusch_data.new_data_indicator = ndi;             /* store first value of ndi */
    ulsch->harq_processes[harq_pid]->round = 0;
    ulsch->harq_processes[harq_pid]->subframe_scheduling_flag = 1;

    result_harq = NEW_TRANSMISSION_HARQ;

    NR_TST_PHY_PRINTF("[HARQ-UL-PUSCH harqId : %d] first new transmission \n", harq_pid);
  }
  /* 38.321 5.4.2.1  2> if the received grant was not addressed to a Temporary C-RNTI on PDCCH, and the NDI provided in the associated HARQ */
  /* information has been toggled compared to the value in the previous transmission of this TB of this HARQ process */
  else if ((ulsch->harq_processes[harq_pid]->pusch_pdu.pusch_data.new_data_indicator != ndi) && (rnti_type != _TC_RNTI_)) {   /* is ndi toogled so this is a new grant ? */
    ulsch->harq_processes[harq_pid]->pusch_pdu.pusch_data.new_data_indicator = ndi;             /* store first value of ndi */
    ulsch->harq_processes[harq_pid]->round = 0;
    ulsch->harq_processes[harq_pid]->subframe_scheduling_flag = 1;

    result_harq = NEW_TRANSMISSION_HARQ;

    NR_TST_PHY_PRINTF("[HARQ-UL-PUSCH harqId : %d] new transmission due to toogle of ndi \n", harq_pid);
   }
   /* 38.321 5.4.2.1 2> else (i.e. retransmission): */
   else {
     ulsch->harq_processes[harq_pid]->pusch_pdu.pusch_data.new_data_indicator = ndi;             /* ndi has not toggled si this is a retransmission */
     ulsch->harq_processes[harq_pid]->round++;                  /* increment number of retransmission */

     result_harq = RETRANSMISSION_HARQ;

     NR_TST_PHY_PRINTF("[HARQ-UL-PUSCH harqId : %d] retransmission \n", harq_pid);
   }

  return (result_harq);
}

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
  dl_harq->first_tx = 1;
  dl_harq->round  = 0;
  dl_harq->harq_ack.ack = DL_ACKNACK_NO_SET;
  dl_harq->harq_ack.send_harq_status = 0;
  dl_harq->harq_ack.vDAI_UL = UL_DAI_NO_SET;
  dl_harq->harq_ack.vDAI_DL = DL_DAI_NO_SET;
  dl_harq->harq_ack.slot_for_feedback_ack = NR_MAX_SLOTS_PER_FRAME;
  dl_harq->harq_ack.pucch_resource_indicator = MAX_PUCCH_RESOURCE_INDICATOR;
  dl_harq->harq_ack.n_CCE = 0;
  dl_harq->harq_ack.N_CCE = 0;;
}

/*******************************************************************
*
* NAME :         config_downlink_harq_process
*
* PARAMETERS :   pointer to ue context
*                id of current gNB
*                number of downlink processes
*
* RETURN :       none
*
* DESCRIPTION :  configuration of downlink HARQ entity
*
*********************************************************************/

void config_downlink_harq_process(PHY_VARS_NR_UE *ue, int gNB_id, int TB_id, int execution_thread_number, uint8_t number_harq_processes_for_pdsch)
{
  NR_UE_DLSCH_t *dlsch;

  dlsch = (NR_UE_DLSCH_t *)malloc16(sizeof(NR_UE_DLSCH_t));

  if (dlsch != NULL) {

    memset(dlsch,0,sizeof(NR_UE_DLSCH_t));

    ue->dlsch[execution_thread_number][gNB_id][TB_id] = dlsch;
  }
  else {
    LOG_E(PHY, "Fatal memory allocation problem at line %d in function %s of file %s \n", __LINE__ , __func__, __FILE__);
    assert(0);
  }

  dlsch->Mdlharq = number_harq_processes_for_pdsch; /* an additional HARQ is reserved for PBCCH */
  dlsch->number_harq_processes_for_pdsch = number_harq_processes_for_pdsch;

  /* allocation of HARQ process context */
  for (int harq_pid = 0; harq_pid < number_harq_processes_for_pdsch; harq_pid++) {

    //dlsch->harq_processes[harq_pid] = (NR_DL_UE_HARQ_t *)malloc16(sizeof(NR_DL_UE_HARQ_t));

    /*if (dlsch->harq_processes[harq_pid] == NULL) {
      LOG_E(PHY, "Fatal memory allocation problem at line %d in function %s of file %s \n", __LINE__ , __func__, __FILE__);
      assert(0);
    }*/

    memset(&dlsch->harq_processes[harq_pid],0,sizeof(NR_DL_UE_HARQ_t));

    NR_DL_UE_HARQ_t *dl_harq = dlsch->harq_processes[harq_pid];

    init_downlink_harq_status(dl_harq);
  }
}

/*******************************************************************
*
* NAME :         release_downlink_harq_process
*
* PARAMETERS :   pointer to ue context
*                id of current gNB
*                TB_id transport block identity 0 or 1
*                execution_thread_number thread number for current downlink processing
* RETURN :       none
*
* DESCRIPTION :  release of HARQ downlink entity
*
*********************************************************************/

void release_downlink_harq_process(PHY_VARS_NR_UE *ue, int gNB_id, int TB_id, int execution_thread_number)
{
  NR_UE_DLSCH_t *dlsch = ue->dlsch[execution_thread_number][gNB_id][TB_id];

  /*for (int process_id = 0; process_id < dlsch->Mdlharq; process_id++) {

    free16(dlsch->harq_processes[process_id],sizeof(NR_DL_UE_HARQ_t));

    dlsch->harq_processes[process_id] = NULL;
  }*/

  free16(dlsch,sizeof(NR_UE_DLSCH_t));

  ue->dlsch[execution_thread_number][gNB_id][TB_id] = NULL;
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

harq_result_t downlink_harq_process(NR_DL_UE_HARQ_t *dl_harq, int harq_pid, int ndi, uint8_t rnti_type)
{
  harq_result_t result_harq = RETRANSMISSION_HARQ;

  if (rnti_type == _CS_RNTI_)
  {
    LOG_E(PHY, "Fatal error in HARQ entity due to not supported CS_RNTI at line %d in function %s of file %s \n", __LINE__ , __func__, __FILE__);
	return(NEW_TRANSMISSION_HARQ);
  }
  else if ((rnti_type != _C_RNTI_) && (rnti_type != _TC_RNTI_)) {
    /* harq mechanism is not relevant for other rnti */
    return(NEW_TRANSMISSION_HARQ);
  }

  if (dl_harq->first_tx == 1) {
    dl_harq->round = 0;
    dl_harq->status = ACTIVE;
    dl_harq->DCINdi = ndi;
    dl_harq->first_tx = 0;

    result_harq = NEW_TRANSMISSION_HARQ;

    NR_TST_PHY_PRINTF("[HARQ-DL-PDSCH harqId : %d] first new reception \n", harq_pid);
  }
  else if (dl_harq->DCINdi != ndi) {
    dl_harq->round = 0;
    dl_harq->status = ACTIVE;
    dl_harq->DCINdi = ndi;

    result_harq = NEW_TRANSMISSION_HARQ;

    NR_TST_PHY_PRINTF("[HARQ-DL-PDSCH harqId : %d] new reception due to toogle of ndi \n", harq_pid);
  }
  else {

    dl_harq->round++;

    result_harq = RETRANSMISSION_HARQ;

    NR_TST_PHY_PRINTF("[HARQ-DL-PDSCH harqId : %d] reception of a retransmission \n", harq_pid);
  }

  return (result_harq);
}
