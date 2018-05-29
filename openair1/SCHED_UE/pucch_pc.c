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

/*! \file pusch_pc.c
 * \brief Implementation of UE PUSCH Power Control procedures from 36.213 LTE specifications (Section
 * \author R. Knopp
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */

#include "PHY/defs_UE.h"
#include "SCHED_UE/sched_UE.h"
#include "SCHED/sched_common_extern.h"
#include "PHY/LTE_UE_TRANSPORT/transport_proto_ue.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"

int16_t pucch_power_cntl(PHY_VARS_UE *ue,UE_rxtx_proc_t *proc,uint8_t subframe,uint8_t eNB_id,PUCCH_FMT_t pucch_fmt)
{

  int16_t Po_PUCCH;
  //uint8_t harq_pid;

  // P_pucch =  P_opucch+ PL + h(nCQI,nHARQ) + delta_pucchF(pucch_fmt) + g(i))
  //
  //if ((pucch_fmt == pucch_format1a) ||
  //    (pucch_fmt == pucch_format1b)) {  // Update g_pucch based on TPC/delta_PUCCH received in PDCCH for this process
    //harq_pid = ue->dlsch[eNB_id][0]->harq_ack[subframe].harq_id;
    //this is now done in dci_tools
    //ue->g_pucch[eNB_id] += ue->dlsch[eNB_id][0]->harq_processes[harq_pid]->delta_PUCCH;
  //}

  Po_PUCCH = get_PL(ue->Mod_id,ue->CC_id,eNB_id)+
    ue->frame_parms.ul_power_control_config_common.p0_NominalPUCCH+
    ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->g_pucch;

  switch (pucch_fmt) {
  case pucch_format1:
  case pucch_format2a:
  case pucch_format2b:
    Po_PUCCH += (-2+(ue->frame_parms.ul_power_control_config_common.deltaF_PUCCH_Format1<<1));
    break;

  case pucch_format1a:
  case pucch_format1b:
  case pucch_format1b_csA2:
  case pucch_format1b_csA3:
  case pucch_format1b_csA4:
    Po_PUCCH += (1+(ue->frame_parms.ul_power_control_config_common.deltaF_PUCCH_Format1b<<1));
    break;

  case pucch_format2:
    switch (ue->frame_parms.ul_power_control_config_common.deltaF_PUCCH_Format2a) {
    case 0:
      Po_PUCCH -= 2;
      break;

    case 2:
      Po_PUCCH += 1;
      break;

    case 3:
      Po_PUCCH += 2;
      break;

    case 1:
    default:
      break;
    }

    break;

  case pucch_format3:
    fprintf(stderr, "PUCCH format 3 not handled\n");
    abort();
  }

  if (pucch_fmt!=pucch_format1) {
    LOG_D(PHY,"[UE  %d][PDSCH %x] AbsSubframe %d.%d: Po_PUCCH %d dBm : Po_NOMINAL_PUCCH %d dBm, PL %d dB, g_pucch %d dB\n",
          ue->Mod_id,
          ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->rnti,proc->frame_tx%1024,subframe,
          Po_PUCCH,
          ue->frame_parms.ul_power_control_config_common.p0_NominalPUCCH,
          get_PL(ue->Mod_id,ue->CC_id,eNB_id),
          ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->g_pucch);
  } else {
    LOG_D(PHY,"[UE  %d][SR %x] AbsSubframe %d.%d: Po_PUCCH %d dBm : Po_NOMINAL_PUCCH %d dBm, PL %d dB g_pucch %d dB\n",
          ue->Mod_id,
          ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->rnti,proc->frame_tx%1024,subframe,
          Po_PUCCH,
          ue->frame_parms.ul_power_control_config_common.p0_NominalPUCCH,
          get_PL(ue->Mod_id,ue->CC_id,eNB_id),
          ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->g_pucch);
  }

  return(Po_PUCCH);
}
