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

/*! \file phy_emulation.c
 *  \brief implements the underlying protocol for emulated data exchange over Ethernet using IP multicast
 *  \author Navid Nikaein
 *  \date 2011 - 2014
 *  \version 1.1
 *  \company Eurecom
 *  \email: navid.nikaein@eurecom.fr
 */


#include "PHY/defs.h"
#include "PHY/extern.h"
#include "common/ran_context.h"
#include "defs.h"
#include "extern.h"
#include "proto.h"
#include "UTIL/OCG/OCG.h"
#include "UTIL/OCG/OCG_extern.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

extern unsigned int Master_list_rx;

extern unsigned char NB_INST;
//#define DEBUG_CONTROL 1
//#define DEBUG_EMU   1

void emu_transport_sync(void)
{
  LOG_D(EMU, "Entering EMU transport SYNC is primary master %d\n",
        oai_emulation.info.is_primary_master);

  if (oai_emulation.info.is_primary_master == 0) {
retry:
    bypass_tx_data(WAIT_SM_TRANSPORT,0,0);
    // just wait to recieve the  master 0 msg
    Master_list_rx = oai_emulation.info.master_list - 1;

    if (bypass_rx_data(0,0,0,1) == -1) {
      /* In case the master is not ready at time we send the first message */
      sleep(1);
      goto retry;
    }
  } else {
    bypass_rx_data(0,0,0,0);
    bypass_tx_data(WAIT_PM_TRANSPORT,0,0);
  }

  if (oai_emulation.info.master_list!=0) {
retry2:
    bypass_tx_data(SYNC_TRANSPORT,0,0);

    if (bypass_rx_data(0,0,0,0) == -1) {
      goto retry2;
    }

    // i received the sync from all secondary masters
    if (emu_rx_status == SYNCED_TRANSPORT) {
      emu_tx_status = SYNCED_TRANSPORT;
    }

    LOG_D(EMU,"TX secondary master SYNC_TRANSPORT state \n");
  }

  LOG_D(EMU, "Leaving EMU transport SYNC is primary master %d\n",
        oai_emulation.info.is_primary_master);
}

void emu_transport(unsigned int frame, unsigned int last_slot,
                   unsigned int next_slot,lte_subframe_t direction,
                   unsigned char frame_type,
                   int ethernet_flag )
{
  if (ethernet_flag == 0) {
    return;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(
    VCD_SIGNAL_DUMPER_FUNCTIONS_EMU_TRANSPORT, VCD_FUNCTION_IN);

  if ((frame_type == 1) &&  (direction == SF_S)) {
    if ((next_slot % 2) == 0) {
      emu_transport_DL(frame, last_slot, next_slot);
    } else {
      emu_transport_UL(frame, last_slot , next_slot);
    }

    //DL
  } else {
    if ((next_slot % 2) == 0) {
      if (((frame_type == 1) && (direction == SF_DL )) || (frame_type == 0) ) {
        emu_transport_DL(frame, last_slot, next_slot);
      }
    }

    // UL
    if (((frame_type == 1) && (direction == SF_UL)) || (frame_type == 0) ) {
      emu_transport_UL(frame, last_slot, next_slot);
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(
    VCD_SIGNAL_DUMPER_FUNCTIONS_EMU_TRANSPORT, VCD_FUNCTION_OUT);
}


void emu_transport_DL(unsigned int frame, unsigned int last_slot,
                      unsigned int next_slot)
{
  LOG_D(EMU, "Entering EMU transport DL, is primary master %d\n",
        oai_emulation.info.is_primary_master);

  if (oai_emulation.info.is_primary_master == 0) {
    if (oai_emulation.info.nb_enb_local > 0) { // send in DL if
      bypass_tx_data(ENB_TRANSPORT, frame, next_slot);
      bypass_rx_data(frame, last_slot, next_slot, 1);
    } else {
      bypass_tx_data(WAIT_SM_TRANSPORT,frame,next_slot);
      bypass_rx_data(frame, last_slot, next_slot, 0);
    }
  } else { // PM
    if (oai_emulation.info.nb_enb_local>0) { // send in DL if
      bypass_rx_data(frame, last_slot, next_slot, 1);
      bypass_tx_data(ENB_TRANSPORT, frame, next_slot);
    } else {
      bypass_rx_data(frame, last_slot, next_slot, 0);
      bypass_tx_data(WAIT_SM_TRANSPORT,frame,next_slot);
    }
  }

  LOG_D(EMU, "Leaving EMU transport DL, is primary master %d\n",
        oai_emulation.info.is_primary_master);
}

void emu_transport_UL(unsigned int frame, unsigned int last_slot,
                      unsigned int next_slot)
{
  LOG_D(EMU, "Entering EMU transport UL, is primary master %d\n",
        oai_emulation.info.is_primary_master);

  if (oai_emulation.info.is_primary_master == 0) {
    if (oai_emulation.info.nb_ue_local > 0) {
      bypass_tx_data(UE_TRANSPORT, frame, next_slot);
      bypass_rx_data(frame, last_slot, next_slot, 1);
    } else {
      bypass_tx_data(WAIT_SM_TRANSPORT, frame, next_slot);
      bypass_rx_data(frame,last_slot, next_slot, 0);
    }
  } else {
    if (oai_emulation.info.nb_ue_local > 0) {
      bypass_rx_data(frame, last_slot, next_slot, 1);
      bypass_tx_data(UE_TRANSPORT, frame, next_slot);
    } else {
      bypass_rx_data(frame,last_slot, next_slot, 0);
      bypass_tx_data(WAIT_SM_TRANSPORT, frame, next_slot);
    }
  }

  LOG_D(EMU, "Leaving EMU transport UL, is primary master %d\n",
        oai_emulation.info.is_primary_master);
}

void emu_transport_release(void)
{
  bypass_tx_data(RELEASE_TRANSPORT,0,0);
  LOG_E(EMU," tx RELEASE_TRANSPORT  \n");
}

unsigned int emul_tx_handler(unsigned char Mode,char *Tx_buffer,
                             unsigned int *Nbytes,unsigned int *Nb_flows)
{

  return *Nbytes;
}

unsigned int emul_rx_data(void)
{
  return(0);
}

unsigned int emul_rx_handler(unsigned char Mode,char *rx_buffer,
                             unsigned int Nbytes)
{
  unsigned short Rx_size=0;
  return (Rx_size+2);
}

void clear_eNB_transport_info(uint8_t nb_eNB)
{
  uint8_t eNB_id;
  uint8_t CC_id;

  for (eNB_id=0; eNB_id<nb_eNB; eNB_id++) {
    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      eNB_transport_info_TB_index[eNB_id][CC_id]=0;
      memset((void *)&eNB_transport_info[eNB_id][CC_id].cntl,0,sizeof(eNB_cntl));
      eNB_transport_info[eNB_id][CC_id].num_common_dci=0;
      eNB_transport_info[eNB_id][CC_id].num_ue_spec_dci=0;
    }
  }

#ifdef DEBUG_EMU
  LOG_D(EMU, "EMUL clear_eNB_transport_info\n");
#endif
}

void clear_UE_transport_info(uint8_t nb_UE)
{
  uint8_t UE_id,CC_id;

  for (UE_id=0; UE_id<nb_UE; UE_id++)
    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      UE_transport_info_TB_index[UE_id][CC_id]=0;
      memset((void *)&UE_transport_info[UE_id][CC_id].cntl,0,sizeof(UE_cntl));
    }

#ifdef DEBUG_EMU
  LOG_D(EMU, "EMUL clear_UE_transport_info\n");
#endif
}

void fill_phy_enb_vars(unsigned int enb_id, uint8_t CC_id,unsigned int next_slot)
{
  int n_dci = 0, n_dci_dl;
  int payload_offset = 0;
  unsigned int harq_pid;
  LTE_eNB_DLSCH_t *dlsch_eNB;
  unsigned short ue_id;
  uint8_t nb_total_dci;

  // eNB
  // PBCH : copy payload

  *(uint32_t *)RC.eNB[enb_id][CC_id]->pbch_pdu =
    eNB_transport_info[enb_id][CC_id].cntl.pbch_payload;
  /*  LOG_I(EMU," RX slot %d ENB TRANSPORT pbch payload %d pdu[0] %d  pdu[0] %d \n",
    next_slot ,
    eNB_transport_info[enb_id][CC_id].cntl.pbch_payload,
    ((uint8_t*)RC.eNB[enb_id]->pbch_pdu)[0],
    ((uint8_t*)RC.eNB[enb_id]->pbch_pdu)[1]);
  */
  //  }
  //CFI
  // not needed yet

  //PHICH
  // to be added later

  //DCI
  nb_total_dci= eNB_transport_info[enb_id][CC_id].num_pmch +
                eNB_transport_info[enb_id][CC_id].num_ue_spec_dci +
                eNB_transport_info[enb_id][CC_id].num_common_dci;

  RC.eNB[enb_id][CC_id]->num_ue_spec_dci[(next_slot>>1)&1] =
    eNB_transport_info[enb_id][CC_id].num_ue_spec_dci;
  RC.eNB[enb_id][CC_id]->num_common_dci[(next_slot>>1)&1]  =
    eNB_transport_info[enb_id][CC_id].num_common_dci;
#ifdef DEBUG_EMU
  LOG_D(EMU, "Fill phy vars eNB %d for slot %d, DCI found %d  \n",
        enb_id, next_slot, nb_total_dci);
#endif

  if (nb_total_dci >0) {

    memcpy(RC.eNB[enb_id][CC_id]->dci_alloc[(next_slot>>1)&1],
           eNB_transport_info[enb_id][CC_id].dci_alloc,
           (nb_total_dci) * sizeof(DCI_ALLOC_t));

    n_dci_dl=0;

    // fill dlsch_eNB structure from DCI
    for (n_dci = 0; n_dci < nb_total_dci; n_dci++) {

      //exclude ul dci
      if ((eNB_transport_info[enb_id][CC_id].dci_alloc[n_dci_dl].format > 0) || (eNB_transport_info[enb_id][CC_id].num_pmch > 0 )) {
#ifdef DEBUG_EMU
        LOG_D(EMU, "dci spec %d common %d pmch %d tbs is %d payload offset %d\n",
              eNB_transport_info[enb_id][CC_id].num_ue_spec_dci,
              eNB_transport_info[enb_id][CC_id].num_common_dci,
              eNB_transport_info[enb_id][CC_id].num_pmch,
              eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl],
              payload_offset);
#endif

        switch (eNB_transport_info[enb_id][CC_id].dlsch_type[n_dci_dl]) {
        case 0: //SI:
          memcpy(RC.eNB[enb_id][CC_id]->dlsch_SI->harq_processes[0]->b,
                 &eNB_transport_info[enb_id][CC_id].transport_blocks[payload_offset],
                 eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl]);
#ifdef DEBUG_EMU
          LOG_D(EMU, "SI eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl]%d \n",
                eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl]);
#endif
          break;

        case 1: //RA:
          memcpy(RC.eNB[enb_id][CC_id]->dlsch_ra->harq_processes[0]->b,
                 &eNB_transport_info[enb_id][CC_id].transport_blocks[payload_offset],
                 eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl]);
#ifdef DEBUG_EMU
          LOG_D(EMU, "RA eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl]%d \n",
                eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl]);
#endif
          break;

        case 2://TB0:
          harq_pid  = eNB_transport_info[enb_id][CC_id].harq_pid[n_dci_dl];
          ue_id = eNB_transport_info[enb_id][CC_id].ue_id[n_dci_dl];
          RC.eNB[enb_id][CC_id]->dlsch[ue_id][0]->rnti=
            eNB_transport_info[enb_id][CC_id].dci_alloc[n_dci_dl].rnti;
          dlsch_eNB = RC.eNB[enb_id][CC_id]->dlsch[ue_id][0];
#ifdef DEBUG_EMU
          LOG_D(EMU,
                " enb_id %d ue id is %d rnti is %x dci index %d, harq_pid %d tbs %d \n",
                enb_id, ue_id, eNB_transport_info[enb_id][CC_id].dci_alloc[n_dci_dl].rnti,
                n_dci_dl, harq_pid, eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl]);

          for (i=0; i<eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl]; i++) {
            LOG_T(EMU, "%x.",
                  (unsigned char) eNB_transport_info[enb_id][CC_id].transport_blocks[payload_offset+i]);
          }

#endif
          memcpy(dlsch_eNB->harq_processes[harq_pid]->b,
                 &eNB_transport_info[enb_id][CC_id].transport_blocks[payload_offset],
                 eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl]);
          break;

        case 3://TB1:
          harq_pid = eNB_transport_info[enb_id][CC_id].harq_pid[n_dci_dl];
          ue_id = eNB_transport_info[enb_id][CC_id].ue_id[n_dci_dl];
          RC.eNB[enb_id][CC_id]->dlsch[ue_id][1]->rnti=
            eNB_transport_info[enb_id][CC_id].dci_alloc[n_dci_dl].rnti;
          dlsch_eNB = RC.eNB[enb_id][CC_id]->dlsch[ue_id][1];

          memcpy(dlsch_eNB->harq_processes[harq_pid]->b,
                 &eNB_transport_info[enb_id][CC_id].transport_blocks[payload_offset],
                 eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl]);
          break;

        case 5:
          memcpy(RC.eNB[enb_id][CC_id]->dlsch_MCH->harq_processes[0]->b,
                 &eNB_transport_info[enb_id][CC_id].transport_blocks[payload_offset],
                 eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl]);
#ifdef DEBUG_EMU
          LOG_D(EMU, "PMCH eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl] %d \n",
                eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl]);
#endif
          break;

        default:
          LOG_W(EMU,"not supported dlsch type %d \n", eNB_transport_info[enb_id][CC_id].dlsch_type[n_dci_dl]);
          break;
        }

        payload_offset += eNB_transport_info[enb_id][CC_id].tbs[n_dci_dl];
      }

      n_dci_dl++;
    }

#ifdef DEBUG_EMU
    LOG_D(EMU, "Fill phy eNB vars done next slot %d !\n", next_slot);
#endif
  }
}

void fill_phy_ue_vars(unsigned int ue_id, uint8_t CC_id,unsigned int last_slot)
{
  int n_enb;//index
  int enb_id;
  //  int harq_id;
  //  int payload_offset = 0;
  unsigned short rnti;
  unsigned int harq_pid;
  LTE_UE_ULSCH_t *ulsch;
  PUCCH_FMT_t pucch_format;
  //  uint8_t ue_transport_info_index[NUMBER_OF_eNB_MAX];
  uint8_t subframe = (last_slot+1)>>1;

  memcpy(&ue_cntl_delay[ue_id][CC_id][(last_slot+1)%2], &UE_transport_info[ue_id][CC_id].cntl,
         sizeof(UE_cntl));

#ifdef DEBUG_EMU
  LOG_D(EMU,
        "Last slot %d subframe %d CC_id %d: Fill phy vars UE %d : PRACH is (%d,%d) preamble (%d,%d) SR (%d,%d), pucch_sel (%d, %d)\n",

        last_slot,subframe,CC_id, ue_id,
        UE_transport_info[ue_id][CC_id].cntl.prach_flag,
        ue_cntl_delay[ue_id][CC_id][last_slot%2].prach_flag,
        UE_transport_info[ue_id][CC_id].cntl.prach_id,
        ue_cntl_delay[ue_id][CC_id][last_slot%2].prach_id,
        UE_transport_info[ue_id][CC_id].cntl.sr,
        ue_cntl_delay[ue_id][CC_id][last_slot%2].sr,
        UE_transport_info[ue_id][CC_id].cntl.pucch_sel,
        ue_cntl_delay[ue_id][CC_id][last_slot%2].pucch_sel);
#endif
  //ue_cntl_delay[subframe%2].prach_flag ;
  PHY_vars_UE_g[ue_id][CC_id]->generate_prach = ue_cntl_delay[ue_id][CC_id][last_slot%2].prach_flag;//UE_transport_info[ue_id][CC_id].cntl.prach_flag;

  if (PHY_vars_UE_g[ue_id][CC_id]->generate_prach == 1) {
    //     if (PHY_vars_UE_g[ue_id][CC_id]->prach_resources[enb_id] == NULL)
    //  PHY_vars_UE_g[ue_id][CC_id]->prach_resources[enb_id] = malloc(sizeof(PRACH_RESOURCES_t));
    //ue_cntl_delay[subframe%2].prach_id;
    PHY_vars_UE_g[ue_id][CC_id]->prach_PreambleIndex =
      ue_cntl_delay[ue_id][CC_id][last_slot%2].prach_id;
  }

  pucch_format= ue_cntl_delay[ue_id][CC_id][last_slot%2].pucch_flag;// UE_transport_info[ue_id][CC_id].cntl.pucch_flag;

  if ((last_slot + 1) % 2 == 0) {
    if (pucch_format == pucch_format1) { // UE_transport_info[ue_id][CC_id].cntl.sr;
      PHY_vars_UE_g[ue_id][CC_id]->sr[subframe] = ue_cntl_delay[ue_id][CC_id][last_slot%2].sr;
    } else if ((pucch_format == pucch_format1a) ||
               (pucch_format == pucch_format1b)) {
      PHY_vars_UE_g[ue_id][CC_id]->pucch_payload[0] = ue_cntl_delay[ue_id][CC_id][last_slot%2].pucch_payload;
      //UE_transport_info[ue_id][CC_id].cntl.pucch_payload;
    }

    PHY_vars_UE_g[ue_id][CC_id]->pucch_sel[subframe] =
      ue_cntl_delay[ue_id][CC_id][last_slot%2].pucch_sel;
  }

#ifdef DEBUG_EMU
  LOG_D(EMU,"subframe %d trying to copy the payload from num eNB %d to UE %d \n",
        subframe, UE_transport_info[ue_id][CC_id].num_eNB, ue_id);
#endif

  for (n_enb = 0; n_enb < UE_transport_info[ue_id][CC_id].num_eNB; n_enb++) {

    rnti = UE_transport_info[ue_id][CC_id].rnti[n_enb];
    enb_id = UE_transport_info[ue_id][CC_id].eNB_id[n_enb];

    PHY_vars_UE_g[ue_id][CC_id]->pdcch_vars[0][enb_id]->crnti=rnti;
    PHY_vars_UE_g[ue_id][CC_id]->pdcch_vars[1][enb_id]->crnti=rnti;

    harq_pid = UE_transport_info[ue_id][CC_id].harq_pid[n_enb];

    PHY_vars_UE_g[ue_id][CC_id]->ulsch[enb_id]->o_RI[0] =
      ue_cntl_delay[ue_id][CC_id][last_slot%2].pusch_ri & 0x1;
    PHY_vars_UE_g[ue_id][CC_id]->ulsch[enb_id]->o_RI[1] =
      (ue_cntl_delay[ue_id][CC_id][last_slot%2].pusch_ri>>1) & 0x1;

    PHY_vars_UE_g[ue_id][CC_id]->ulsch[enb_id]->o_ACK[0]=
      ue_cntl_delay[ue_id][CC_id][last_slot%2].pusch_ack & 0x1;
    PHY_vars_UE_g[ue_id][CC_id]->ulsch[enb_id]->o_ACK[1]=
      (ue_cntl_delay[ue_id][CC_id][last_slot%2].pusch_ack>>1) & 0x1;
    //*(uint32_t *)ulsch->o                        = ue_cntl_delay[ue_id][CC_id][last_slot%2].pusch_uci;

    if ((last_slot % 2) == 1) {
      PHY_vars_UE_g[ue_id][CC_id]->ulsch[enb_id]->O =
        ue_cntl_delay[ue_id][CC_id][last_slot%2].length_uci;
      PHY_vars_UE_g[ue_id][CC_id]->ulsch[enb_id]->uci_format =
        ue_cntl_delay[ue_id][CC_id][last_slot%2].uci_format;

      memcpy(PHY_vars_UE_g[ue_id][CC_id]->ulsch[enb_id]->o,
             ue_cntl_delay[ue_id][CC_id][last_slot%2].pusch_uci,
             MAX_CQI_BYTES);

      ulsch = PHY_vars_UE_g[ue_id][CC_id]->ulsch[enb_id];
      // if (((HLC_subband_cqi_rank1_2A_5MHz *)ulsch->o)->cqi1)
      LOG_D(EMU,
            "[UE %d] subframe %d last slot %d copy the payload from eNB %d to UE %d with harq id %d cqi (val %d, length %d) \n",
            ue_id, subframe, last_slot, enb_id, ue_id, harq_pid,
            ((HLC_subband_cqi_rank1_2A_5MHz *)ulsch->o)->cqi1,
            PHY_vars_UE_g[ue_id][CC_id]->ulsch[enb_id]->O);
    }

    memcpy(PHY_vars_UE_g[ue_id][CC_id]->ulsch[enb_id]->harq_processes[harq_pid]->b,
           UE_transport_info[ue_id][CC_id].transport_blocks,
           UE_transport_info[ue_id][CC_id].tbs[enb_id]);

    //ue_transport_info_index[enb_id]+=UE_transport_info[ue_id][CC_id].tbs[enb_id];

    //UE_transport_info[ue_id][CC_id].transport_blocks+=ue_transport_info_index[enb_id];
    //LOG_T(EMU,"ulsch tbs is %d\n", UE_transport_info[ue_id][CC_id].tbs[enb_id]);
  }
}
