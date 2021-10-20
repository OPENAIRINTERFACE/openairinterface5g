/* Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
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

/************************************************************************
*
* MODULE      :  PUCCH Packed Uplink Control Channel for UE NR
*                PUCCH is used to transmit Uplink Control Information UCI
*                which is composed of:
*                - SR Scheduling Request
*                - HARQ ACK/NACK
*                - CSI Channel State Information
*                UCI can also be transmitted on a PUSCH if it schedules.
*
* DESCRIPTION :  functions related to PUCCH UCI management
*                TS 38.213 9  UE procedure for reporting control information
*
**************************************************************************/

#include "executables/softmodem-common.h"
#include "PHY/NR_REFSIG/ss_pbch_nr.h"
#include "PHY/defs_nr_UE.h"
#include <openair1/SCHED/sched_common.h>
#include <openair1/PHY/NR_UE_TRANSPORT/pucch_nr.h>
#include "openair1/PHY/NR_UE_ESTIMATION/nr_estimation.h"
#include <openair1/PHY/impl_defs_nr.h>
#include <common/utils/nr/nr_common.h>

#ifndef NO_RAT_NR

#include "SCHED_NR_UE/defs.h"
#include "SCHED_NR_UE/harq_nr.h"

#define DEFINE_VARIABLES_PUCCH_UE_NR_H
#include "SCHED_NR_UE/pucch_uci_ue_nr.h"
#undef DEFINE_VARIABLES_PUCCH_UE_NR_H

#endif


uint8_t nr_is_cqi_TXOp(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t gNB_id);
uint8_t nr_is_ri_TXOp(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t gNB_id);

long
binary_search_float_nr(
  float elements[],
  long numElem,
  float value
)
//-----------------------------------------------------------------------------
{
  long first, last, middle;
  first = 0;
  last = numElem-1;
  middle = (first+last)/2;

  if(value < elements[0]) {
    return first;
  }

  if(value >= elements[last]) {
    return last;
  }

  while (last - first > 1) {
    if (elements[middle] > value) {
      last = middle;
    } else {
      first = middle;
    }

    middle = (first+last)/2;
  }

  if (first < 0 || first >= numElem) {
    LOG_E(RRC,"\n Error in binary search float!");
  }

  return first;
}
/*
void nr_generate_pucch0(int32_t **txdataF,
                        NR_DL_FRAME_PARMS *frame_parms,
                        PUCCH_CONFIG_DEDICATED *pucch_config_dedicated,
                        int16_t amp,
                        int nr_slot_tx,
                        uint8_t mcs,
                        uint8_t nrofSymbols,
                        uint8_t startingSymbolIndex,
                        uint16_t startingPRB);

void nr_generate_pucch1(int32_t **txdataF,
                        NR_DL_FRAME_PARMS *frame_parms,
                        PUCCH_CONFIG_DEDICATED *pucch_config_dedicated,
                        uint64_t payload,
                        int16_t amp,
                        int nr_slot_tx,
                        uint8_t nrofSymbols,
                        uint8_t startingSymbolIndex,
                        uint16_t startingPRB,
                        uint16_t startingPRB_intraSlotHopping,
                        uint8_t timeDomainOCC,
                        uint8_t nr_bit);

void nr_generate_pucch2(int32_t **txdataF,
                        NR_DL_FRAME_PARMS *frame_parms,
                        PUCCH_CONFIG_DEDICATED *pucch_config_dedicated,
                        uint64_t payload,
                        int16_t amp,
                        int nr_slot_tx,
                        uint8_t nrofSymbols,
                        uint8_t startingSymbolIndex,
                        uint8_t nrofPRB,
                        uint16_t startingPRB,
                        uint8_t nr_bit);

void nr_generate_pucch3_4(int32_t **txdataF,
                         NR_DL_FRAME_PARMS *frame_parms,
                         pucch_format_nr_t fmt,
                         PUCCH_CONFIG_DEDICATED *pucch_config_dedicated,
                         uint64_t payload,
                         int16_t amp,
                         int nr_slot_tx,
                         uint8_t nrofSymbols,
                         uint8_t startingSymbolIndex,
                         uint8_t nrofPRB,
                         uint16_t startingPRB,
                         uint8_t nr_bit,
                         uint8_t occ_length_format4,
                         uint8_t occ_index_format4);
*/
/**************** variables **************************************/


/**************** functions **************************************/

//extern uint8_t is_cqi_TXOp(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id);
//extern uint8_t is_ri_TXOp(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id);
/*******************************************************************
*
* NAME :         pucch_procedures_ue_nr
*
* PARAMETERS :   ue context
*                processing slots of reception/transmission
*                gNB_id identifier
*
* RETURN :       bool TRUE  PUCCH will be transmitted
*                     FALSE No PUCCH to transmit
*
* DESCRIPTION :  determines UCI (uplink Control Information) payload
*                and PUCCH format and its parameters.
*                PUCCH is no transmitted if:
*                - there is no valid data to transmit
*                - Pucch parameters are not valid
*
* Below information is scanned in order to know what information should be transmitted to network.
*
* (SR Scheduling Request)   (HARQ ACK/NACK)    (CSI Channel State Information)
*          |                        |               - CQI Channel Quality Indicator
*          |                        |                - RI  Rank Indicator
*          |                        |                - PMI Primary Matrux Indicator
*          |                        |                - LI Layer Indicator
*          |                        |                - L1-RSRP
*          |                        |                - CSI-RS resource idicator
*          |                        V                    |
*          +-------------------- -> + <------------------
*                                   |
*                   +--------------------------------+
*                   | UCI Uplink Control Information |
*                   +--------------------------------+
*                                   V                                            PUCCH Configuration
*               +----------------------------------------+                   +--------------------------+
*               | Determine PUCCH  payload and its       |                   |     PUCCH Resource Set   |
*               +----------------------------------------+                   |     PUCCH Resource       |
*                                   V                                        |     Format parameters    |
*               +-----------------------------------------+                  |                          |
*               | Select PUCCH format with its parameters | <----------------+--------------------------+
*               +-----------------------------------------+
*                                   V
*                          +-----------------+
*                          |  Generate PUCCH |
*                          +-----------------+
*
* TS 38.213 9  UE procedure for reporting control information
*
*********************************************************************/

void pucch_procedures_ue_nr(PHY_VARS_NR_UE *ue, 
                            uint8_t gNB_id,
                            UE_nr_rxtx_proc_t *proc) {

  int       nr_slot_tx = proc->nr_slot_tx;
  fapi_nr_ul_config_pucch_pdu *pucch_pdu;
  NR_UE_PUCCH *pucch_vars = ue->pucch_vars[proc->thread_id][gNB_id];

  for (int i=0; i<2; i++) {
    if(pucch_vars->active[i]) {

      pucch_pdu = &pucch_vars->pucch_pdu[i];
      uint16_t nb_of_prbs = pucch_pdu->prb_size;
      /* Generate PUCCH signal according to its format and parameters */
      ue->generate_ul_signal[gNB_id] = 1;

      int16_t PL = get_nr_PL(ue->Mod_id, ue->CC_id, gNB_id); /* LTE function because NR path loss not yet implemented FFS TODO NR */
      int contributor = (10 * log10((double)(pow(2,(ue->frame_parms.numerology_index)) * nb_of_prbs)));

      int16_t pucch_tx_power = pucch_pdu->pucch_tx_power + contributor + PL;

      if (pucch_tx_power > ue->tx_power_max_dBm)
        pucch_tx_power = ue->tx_power_max_dBm;

      /* set tx power */
      ue->tx_power_dBm[nr_slot_tx] = pucch_tx_power;
      ue->tx_total_RE[nr_slot_tx] = nb_of_prbs*N_SC_RB;

      int tx_amp;

      /*
      tx_amp = nr_get_tx_amp(pucch_tx_power,
                             ue->tx_power_max_dBm,
                             ue->frame_parms.N_RB_UL,
                             nb_of_prbs);
      if (tx_amp == 0)*/
      // FIXME temporarly using fixed amplitude before pucch power control implementation revised
      tx_amp = AMP;


      LOG_D(PHY,"Generation of PUCCH format %d at frame.slot %d.%d\n",pucch_pdu->format_type,proc->frame_tx,nr_slot_tx);

      switch(pucch_pdu->format_type) {
        case 0:
          nr_generate_pucch0(ue,
                             ue->common_vars.txdataF,
                             &ue->frame_parms,
                             tx_amp,
                             nr_slot_tx,
                             pucch_pdu);
          break;
        case 1:
          nr_generate_pucch1(ue,
                             ue->common_vars.txdataF,
                             &ue->frame_parms,
                             tx_amp,
                             nr_slot_tx,
                             pucch_pdu);
          break;
        case 2:
          nr_generate_pucch2(ue,
                             ue->common_vars.txdataF,
                             &ue->frame_parms,
                             tx_amp,
                             nr_slot_tx,
                             pucch_pdu);
          break;
        case 3:
        case 4:
          nr_generate_pucch3_4(ue,
                               ue->common_vars.txdataF,
                               &ue->frame_parms,
                               tx_amp,
                               nr_slot_tx,
                               pucch_pdu);
          break;
      }
    }
    pucch_vars->active[i] = false;
  }
}



int      dummy_csi_status = 0;
uint32_t dummy_csi_payload = 0;


/* FFS TODO_NR code that should be removed */

void set_csi_nr(int csi_status, uint32_t csi_payload)
{
  dummy_csi_status = csi_status;

  if (dummy_csi_status == 0) {
    dummy_csi_payload = 0;
  }
  else {
    dummy_csi_payload = csi_payload;
  }
}

uint8_t get_nb_symbols_pucch(NR_PUCCH_Resource_t *pucch_resource, pucch_format_nr_t format_type)
{
  switch (format_type) {
    case pucch_format0_nr:
      return pucch_resource->format.choice.format0->nrofSymbols;

    case pucch_format1_nr:
      return pucch_resource->format.choice.format1->nrofSymbols;

    case pucch_format2_nr:
      return pucch_resource->format.choice.format2->nrofSymbols;

    case pucch_format3_nr:
      return pucch_resource->format.choice.format3->nrofSymbols;

    case pucch_format4_nr:
      return pucch_resource->format.choice.format4->nrofSymbols;
  }
  return 0;
}

uint16_t get_starting_symb_idx(NR_PUCCH_Resource_t *pucch_resource, pucch_format_nr_t format_type)
{
  switch (format_type) {
    case pucch_format0_nr:
      return pucch_resource->format.choice.format0->startingSymbolIndex;

    case pucch_format1_nr:
      return pucch_resource->format.choice.format1->startingSymbolIndex;

    case pucch_format2_nr:
      return pucch_resource->format.choice.format2->startingSymbolIndex;

    case pucch_format3_nr:
      return pucch_resource->format.choice.format3->startingSymbolIndex;

    case pucch_format4_nr:
      return pucch_resource->format.choice.format4->startingSymbolIndex;
  }
  return 0;
}

int get_ics_pucch(NR_PUCCH_Resource_t *pucch_resource, pucch_format_nr_t format_type)
{
  switch (format_type) {
    case pucch_format0_nr:
      return pucch_resource->format.choice.format0->initialCyclicShift;

    case pucch_format1_nr:
      return pucch_resource->format.choice.format1->initialCyclicShift;
      
    case pucch_format2_nr:
      return 0;

    default:
      return -1;
  }
  return -1;
}

NR_PUCCH_Resource_t *select_resource_by_id(int resource_id, NR_PUCCH_Config_t *pucch_config)
{
  int n_list = pucch_config->resourceToAddModList->list.count; 
  NR_PUCCH_Resource_t *pucchres;
  AssertFatal(n_list>0,"PUCCH resourceToAddModList is empty\n");

  for (int i=0; i<n_list; i++) {
    pucchres = pucch_config->resourceToAddModList->list.array[i];
    if (pucchres->pucch_ResourceId == resource_id)
      return pucchres;
  }
  return NULL;
}

