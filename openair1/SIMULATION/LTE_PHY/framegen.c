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

#include <string.h>
#include <math.h>
#include <unistd.h>
#include "SIMULATION/TOOLS/defs.h"
#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/vars.h"
#include "SCHED/extern.h"
#include "OCG_vars.h"
#include "MAC_INTERFACE/vars.h"
#ifdef IFFT_FPGA
#include "PHY/LTE_REFSIG/mod_table.h"
#endif

#include "SCHED/defs.h"
#include "SCHED/vars.h"
#include "ARCH/CBMIMO1/DEVICE_DRIVER/vars.h"

#include "LAYER2/MAC/defs.h"
#include "PHY_INTERFACE/defs.h"
#include "LAYER2/MAC/vars.h"
#include "UTIL/LOG/log_extern.h"

uint16_t n_rnti = 0x1235;
int n_users = 1;
uint8_t mcs = 0;
uint32_t DLSCH_RB_ALLOC = 0x1000;

uint8_t BCCH_alloc_pdu[8];
uint64_t DLSCH_alloc_pdu_1[2];

void eNB_scheduler(uint8_t Mod_id, uint8_t cooperation_flag, uint8_t frame, uint8_t subframe)
{

  msg("Doing Scheduler for eNB, frame %d subframe %d\n",frame,subframe);
}

DCI_PDU DCI_pdu;

DCI_PDU *get_dci(uint8_t Mod_id, uint8_t frame, uint8_t subframe)
{
  int i,k;
  uint8_t transmission_mode = PHY_vars_eNB_g[0]->transmission_mode[0];
  int dci_length_bytes,dci_length;
  int BCCH_pdu_size_bits, BCCH_pdu_size_bytes;

  DCI_pdu.Num_dci = 0;

  if (subframe_select(&PHY_vars_eNB_g[0]->lte_frame_parms, subframe) == SF_S) {
    return (&DCI_pdu);
  }

  // UE specific DCI
  for(k=0; k<n_users; k++) {
    switch(transmission_mode) {
    case 1:
    case 2:
      if (PHY_vars_eNB_g[0]->lte_frame_parms.frame_type == TDD) {
        switch (PHY_vars_eNB_g[0]->lte_frame_parms.N_RB_DL) {
        case 6:
          dci_length = sizeof_DCI1_1_5MHz_TDD_t;
          dci_length_bytes = sizeof(DCI1_1_5MHz_TDD_t);
          ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
          ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
          ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs              = mcs;
          ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
          ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi              = 1;
          ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv               = 0;
          ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
          ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;

          ((DCI1A_1_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->type                 = 1;
          ((DCI1A_1_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->vrb_type             = 0;
          ((DCI1A_1_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->rballoc              = computeRIV(PHY_vars_eNB_g[0]->lte_frame_parms.N_RB_DL, 0, 4);
          ((DCI1A_1_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->ndi                  = 1;
          ((DCI1A_1_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->rv                   = 0;
          ((DCI1A_1_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->mcs                  = 2;
          ((DCI1A_1_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->harq_pid             = 0;
          ((DCI1A_1_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->TPC                  = 1;
          BCCH_pdu_size_bits  = sizeof_DCI1A_1_5MHz_TDD_1_6_t;
          BCCH_pdu_size_bytes = sizeof(DCI1A_1_5MHz_TDD_1_6_t);
          break;

        case 25:
          dci_length = sizeof_DCI1_5MHz_TDD_t;
          dci_length_bytes = sizeof(DCI1_5MHz_TDD_t);
          ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
          ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
          ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs              = mcs;
          ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
          ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi              = 1;
          ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv               = 0;
          ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
          ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;

          ((DCI1A_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->type                 = 1;
          ((DCI1A_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->vrb_type             = 0;
          ((DCI1A_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->rballoc              = computeRIV(PHY_vars_eNB_g[0]->lte_frame_parms.N_RB_DL, 18, 4);
          ((DCI1A_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->ndi                  = 1;
          ((DCI1A_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->rv                   = 0;
          ((DCI1A_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->mcs                  = 2;
          ((DCI1A_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->harq_pid             = 0;
          ((DCI1A_5MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->TPC                  = 1;
          BCCH_pdu_size_bits  = sizeof_DCI1A_5MHz_TDD_1_6_t;
          BCCH_pdu_size_bytes = sizeof(DCI1A_5MHz_TDD_1_6_t);
          break;

        case 50:
          dci_length = sizeof_DCI1_10MHz_TDD_t;
          dci_length_bytes = sizeof(DCI1_10MHz_TDD_t);
          ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
          ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
          ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs              = mcs;
          ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
          ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi              = 1;
          ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv               = 0;
          ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
          ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;

          ((DCI1A_10MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->type                 = 1;
          ((DCI1A_10MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->vrb_type             = 0;
          ((DCI1A_10MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->rballoc              = computeRIV(PHY_vars_eNB_g[0]->lte_frame_parms.N_RB_DL, 30, 4);
          ((DCI1A_10MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->ndi                  = 1;
          ((DCI1A_10MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->rv                   = 0;
          ((DCI1A_10MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->mcs                  = 2;
          ((DCI1A_10MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->harq_pid             = 0;
          ((DCI1A_10MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->TPC                  = 1;
          BCCH_pdu_size_bits  = sizeof_DCI1A_10MHz_TDD_1_6_t;
          BCCH_pdu_size_bytes = sizeof(DCI1A_10MHz_TDD_1_6_t);
          break;

        case 100:
          dci_length = sizeof_DCI1_20MHz_TDD_t;
          dci_length_bytes = sizeof(DCI1_20MHz_TDD_t);
          ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
          ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
          ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs              = mcs;
          ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
          ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi              = 1;
          ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv               = 0;
          ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
          ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;

          ((DCI1A_20MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->type                 = 1;
          ((DCI1A_20MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->vrb_type             = 0;
          ((DCI1A_20MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->rballoc              = computeRIV(PHY_vars_eNB_g[0]->lte_frame_parms.N_RB_DL, 70, 4);
          ((DCI1A_20MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->ndi                  = 1;
          ((DCI1A_20MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->rv                   = 0;
          ((DCI1A_20MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->mcs                  = 2;
          ((DCI1A_20MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->harq_pid             = 0;
          ((DCI1A_20MHz_TDD_1_6_t*)&BCCH_alloc_pdu[0])->TPC                  = 1;
          BCCH_pdu_size_bits  = sizeof_DCI1A_20MHz_TDD_1_6_t;
          BCCH_pdu_size_bytes = sizeof(DCI1A_20MHz_TDD_1_6_t);
          break;
        }
      } else { //FDD
        switch (PHY_vars_eNB_g[0]->lte_frame_parms.N_RB_DL) {
        case 6:
          dci_length = sizeof_DCI1_1_5MHz_FDD_t;
          dci_length_bytes = sizeof(DCI1_1_5MHz_FDD_t);
          ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
          ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
          ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs              = mcs;
          ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
          ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi              = 1;
          ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv               = 0;
          ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;

          ((DCI1A_1_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->type                 = 1;
          ((DCI1A_1_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->vrb_type             = 0;
          ((DCI1A_1_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->rballoc              = computeRIV(PHY_vars_eNB_g[0]->lte_frame_parms.N_RB_DL, 0, 4);
          ((DCI1A_1_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->ndi                  = 1;
          ((DCI1A_1_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->rv                   = 0;
          ((DCI1A_1_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->mcs                  = 2;
          ((DCI1A_1_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->harq_pid             = 0;
          ((DCI1A_1_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->TPC                  = 1;
          BCCH_pdu_size_bits  = sizeof_DCI1A_1_5MHz_FDD_t;
          BCCH_pdu_size_bytes = sizeof(DCI1A_1_5MHz_FDD_t);
          break;

        case 25:
          dci_length = sizeof_DCI1_5MHz_FDD_t;
          dci_length_bytes = sizeof(DCI1_5MHz_FDD_t);
          ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
          ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
          ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs              = mcs;
          ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
          ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi              = 1;
          ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv               = 0;
          ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;

          ((DCI1A_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->type                 = 1;
          ((DCI1A_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->vrb_type             = 0;
          ((DCI1A_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->rballoc              = computeRIV(PHY_vars_eNB_g[0]->lte_frame_parms.N_RB_DL, 18, 4);
          ((DCI1A_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->ndi                  = 1;
          ((DCI1A_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->rv                   = 0;
          ((DCI1A_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->mcs                  = 2;
          ((DCI1A_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->harq_pid             = 0;
          ((DCI1A_5MHz_FDD_t*)&BCCH_alloc_pdu[0])->TPC                  = 1;
          BCCH_pdu_size_bits  = sizeof_DCI1A_5MHz_FDD_t;
          BCCH_pdu_size_bytes = sizeof(DCI1A_5MHz_FDD_t);
          break;

        case 50:
          dci_length = sizeof_DCI1_10MHz_FDD_t;
          dci_length_bytes = sizeof(DCI1_10MHz_FDD_t);
          ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
          ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
          ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs              = mcs;
          ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
          ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi              = 1;
          ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv               = 0;
          ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;

          ((DCI1A_10MHz_FDD_t*)&BCCH_alloc_pdu[0])->type                 = 1;
          ((DCI1A_10MHz_FDD_t*)&BCCH_alloc_pdu[0])->vrb_type             = 0;
          ((DCI1A_10MHz_FDD_t*)&BCCH_alloc_pdu[0])->rballoc              = computeRIV(PHY_vars_eNB_g[0]->lte_frame_parms.N_RB_DL, 30, 4);
          ((DCI1A_10MHz_FDD_t*)&BCCH_alloc_pdu[0])->ndi                  = 1;
          ((DCI1A_10MHz_FDD_t*)&BCCH_alloc_pdu[0])->rv                   = 0;
          ((DCI1A_10MHz_FDD_t*)&BCCH_alloc_pdu[0])->mcs                  = 2;
          ((DCI1A_10MHz_FDD_t*)&BCCH_alloc_pdu[0])->harq_pid             = 0;
          ((DCI1A_10MHz_FDD_t*)&BCCH_alloc_pdu[0])->TPC                  = 1;
          BCCH_pdu_size_bits  = sizeof_DCI1A_10MHz_FDD_t;
          BCCH_pdu_size_bytes = sizeof(DCI1A_10MHz_FDD_t);
          break;

        case 100:
          dci_length = sizeof_DCI1_20MHz_FDD_t;
          dci_length_bytes = sizeof(DCI1_20MHz_FDD_t);
          ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
          ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
          ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs              = mcs;
          ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
          ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi              = 1;
          ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv               = 0;
          ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;

          ((DCI1A_20MHz_FDD_t*)&BCCH_alloc_pdu[0])->type                 = 1;
          ((DCI1A_20MHz_FDD_t*)&BCCH_alloc_pdu[0])->vrb_type             = 0;
          ((DCI1A_20MHz_FDD_t*)&BCCH_alloc_pdu[0])->rballoc              = computeRIV(PHY_vars_eNB_g[0]->lte_frame_parms.N_RB_DL, 70, 4);
          ((DCI1A_20MHz_FDD_t*)&BCCH_alloc_pdu[0])->ndi                  = 1;
          ((DCI1A_20MHz_FDD_t*)&BCCH_alloc_pdu[0])->rv                   = 0;
          ((DCI1A_20MHz_FDD_t*)&BCCH_alloc_pdu[0])->mcs                  = 2;
          ((DCI1A_20MHz_FDD_t*)&BCCH_alloc_pdu[0])->harq_pid             = 0;
          ((DCI1A_20MHz_FDD_t*)&BCCH_alloc_pdu[0])->TPC                  = 1;
          BCCH_pdu_size_bits  = sizeof_DCI1A_20MHz_FDD_t;
          BCCH_pdu_size_bytes = sizeof(DCI1A_20MHz_FDD_t);
          break;
        }
      }

      break;

    default:
      printf("Unsupported Transmission Mode!!!");
      exit(-1);
      break;
    }

    // add common dci
    DCI_pdu.dci_alloc[0].dci_length   = BCCH_pdu_size_bits;
    DCI_pdu.dci_alloc[0].L            = 2;
    DCI_pdu.dci_alloc[0].rnti         = SI_RNTI;
    DCI_pdu.dci_alloc[0].format       = format1A;
    DCI_pdu.dci_alloc[0].ra_flag      = 0;
    DCI_pdu.dci_alloc[0].search_space = DCI_COMMON_SPACE;
    memcpy((void*)&DCI_pdu.dci_alloc[0].dci_pdu[0], &BCCH_alloc_pdu[0], BCCH_pdu_size_bytes);
    DCI_pdu.Num_dci++;

    // add ue specific dci
    DCI_pdu.dci_alloc[DCI_pdu.Num_dci].dci_length   = dci_length;
    DCI_pdu.dci_alloc[DCI_pdu.Num_dci].L            = 0;
    DCI_pdu.dci_alloc[DCI_pdu.Num_dci].rnti         = n_rnti+k;
    DCI_pdu.dci_alloc[DCI_pdu.Num_dci].format       = format1;
    DCI_pdu.dci_alloc[DCI_pdu.Num_dci].ra_flag      = 0;
    DCI_pdu.dci_alloc[DCI_pdu.Num_dci].search_space = DCI_UE_SPACE;
    memcpy((void*)&DCI_pdu.dci_alloc[DCI_pdu.Num_dci].dci_pdu[0], &DLSCH_alloc_pdu_1[k], dci_length_bytes);
    DCI_pdu.Num_dci++;

  }

  DCI_pdu.nCCE = 0;

  for (i=0; i<DCI_pdu.Num_dci; i++) {
    DCI_pdu.nCCE += (1<<(DCI_pdu.dci_alloc[i].L));
  }

  return(&DCI_pdu);
}

uint8_t DLSCH_pdu[768*8];

uint8_t *get_DLSCH_pdu(uint8_t Mod_id,uint16_t rnti,uint8_t abstraction_flag)
{
  int i;
  memset(DLSCH_pdu, 0, 768);

  for(i=0; i<768; i++)
    DLSCH_pdu[i] = (unsigned char)(0xff);

  return(DLSCH_pdu);
}

void do_OFDM_mod(mod_sym_t **txdataF, int32_t **txdata, uint16_t next_slot, LTE_DL_FRAME_PARMS *frame_parms)
{

  int aa, slot_offset, slot_offset_F;
  int nthreads,tid;

  slot_offset_F = (next_slot)*(frame_parms->ofdm_symbol_size)*((frame_parms->Ncp==1) ? 6 : 7);
  slot_offset = (next_slot)*(frame_parms->samples_per_tti>>1);

  for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
    //    printf("Thread %d starting ... aa %d (%llu)\n",omp_get_thread_num(),aa,rdtsc());

    if (frame_parms->Ncp == 1)
      PHY_ofdm_mod(&txdataF[aa][slot_offset_F],        // input
                   &txdata[aa][slot_offset],         // output
                   frame_parms->log2_symbol_size,                // log2_fft_size
                   6,                 // number of symbols
                   frame_parms->nb_prefix_samples,               // number of prefix samples
                   frame_parms->twiddle_ifft,  // IFFT twiddle factors
                   frame_parms->rev,           // bit-reversal permutation
                   CYCLIC_PREFIX);
    else {
      normal_prefix_mod(&txdataF[aa][slot_offset_F],
                        &txdata[aa][slot_offset],
                        7,
                        frame_parms);
    }


  }


}

void lte_param_init(  unsigned char transmission_mode,
                      unsigned char extended_prefix_flag,
                      uint16_t Nid_cell,
                      uint8_t N_RB_DL,
                      uint8_t osf,
                      uint8_t fdd_flag,
                      uint8_t tdd_config)
{

  unsigned int ind,i,j;
  LTE_DL_FRAME_PARMS *lte_frame_parms;
  int frame_type = (fdd_flag==1) ? 0 : 1;
  mac_xface = malloc(sizeof(MAC_xface));

  LOG_I(PHY,"init lte parms: Nid_cell %d, Frame_type %d, N_RB_DL %d\n", Nid_cell, frame_type, N_RB_DL);
  PHY_vars_eNB_g = malloc(sizeof(PHY_VARS_eNB*));
  PHY_vars_eNB_g[0] = malloc(sizeof(PHY_VARS_eNB));
  PHY_vars_UE_g = malloc(sizeof(PHY_VARS_UE*));
  PHY_vars_UE_g[0] = malloc(sizeof(PHY_VARS_UE));

  mac_xface->eNB_dlsch_ulsch_scheduler = eNB_scheduler;
  mac_xface->get_dci_sdu = get_dci;
  mac_xface->get_dlsch_sdu = get_DLSCH_pdu;

  randominit(0);
  set_taus_seed(0);

  lte_frame_parms = &(PHY_vars_eNB_g[0]->lte_frame_parms);

  lte_frame_parms->frame_type         = frame_type;
  lte_frame_parms->tdd_config         = tdd_config;
  lte_frame_parms->tdd_config_S       = 0;
  lte_frame_parms->N_RB_DL            = N_RB_DL;   //50 for 10MHz and 25 for 5 MHz
  lte_frame_parms->N_RB_UL            = N_RB_DL;
  lte_frame_parms->Ncp                = extended_prefix_flag;
  lte_frame_parms->Nid_cell           = Nid_cell;
  lte_frame_parms->nushift            = Nid_cell%6;
  lte_frame_parms->nb_antennas_tx     = (transmission_mode == 1) ? 1 : 2;
  lte_frame_parms->nb_antenna_ports_eNB = (transmission_mode == 1) ? 1 : 2;
  lte_frame_parms->nb_antennas_rx     = (transmission_mode == 1) ? 1 : 2;
  lte_frame_parms->mode1_flag = (transmission_mode == 1)? 1 : 0;
  lte_frame_parms->phich_config_common.phich_resource = oneSixth;
  lte_frame_parms->phich_config_common.phich_duration = normal;

  init_frame_parms(lte_frame_parms,osf);


  phy_init_top(lte_frame_parms); //allocation
  phy_init_lte_top(lte_frame_parms);

  lte_frame_parms->twiddle_fft      = twiddle_fft;
  lte_frame_parms->twiddle_ifft     = twiddle_ifft;
  lte_frame_parms->rev              = rev;

  memcpy(&(PHY_vars_UE_g[0]->lte_frame_parms), lte_frame_parms, sizeof(LTE_DL_FRAME_PARMS));

  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    for (j=0; j<2; j++) {
      PHY_vars_eNB_g[0]->dlsch_eNB[i][j] = new_eNB_dlsch(1,8,0);

      if (!PHY_vars_eNB_g[0]->dlsch_eNB[i][j]) {
        LOG_E(PHY,"Can't get eNb dlsch structures\n");
        exit(-1);
      } else {
        LOG_D(PHY,"dlsch_eNB[%d][%d] => %p\n",i,j,PHY_vars_eNB_g[0]->dlsch_eNB[i][j]);
        PHY_vars_eNB_g[0]->dlsch_eNB[i][j]->rnti=n_rnti+i;
      }
    }

    PHY_vars_eNB_g[0]->ulsch_eNB[1+i] = new_eNB_ulsch(8,0);

    if (!PHY_vars_eNB_g[0]->ulsch_eNB[1+i]) {
      LOG_E(PHY,"Can't get eNb ulsch structures\n");
      exit(-1);
    }

  }

  // ULSCH for RA

  PHY_vars_eNB_g[0]->ulsch_eNB[0] = new_eNB_ulsch(8,0);

  if (!PHY_vars_eNB_g[0]->ulsch_eNB[0]) {
    LOG_E(PHY,"Can't get eNb ulsch structures\n");
    exit(-1);
  }

  PHY_vars_eNB_g[0]->dlsch_eNB_SI  = new_eNB_dlsch(1,1,0);
  LOG_D(PHY,"[eNB %d] : SI %p\n",0,PHY_vars_eNB_g[0]->dlsch_eNB_SI);
  PHY_vars_eNB_g[0]->dlsch_eNB_ra  = new_eNB_dlsch(1,1,0);
  LOG_D(PHY,"[eNB %d] : RA %p\n",0,PHY_vars_eNB_g[0]->dlsch_eNB_ra);
  PHY_vars_eNB_g[0]->rx_total_gain_eNB_dB=150;


  phy_init_lte_ue(PHY_vars_UE_g[0],1,0);

  phy_init_lte_eNB(PHY_vars_eNB_g[0],0,0,0);

  LOG_I(PHY,"Done lte_param_init\n");

  PHY_vars_eNB_g[0]->transmission_mode[0] = transmission_mode;
  dump_frame_parms(lte_frame_parms);
}


int main(int argc, char **argv)
{

  char c;

  int i,l,aa,aarx,sector;

  unsigned char pbch_pdu[6];
  int result;
  int freq_offset;
  int subframe_offset;
  char fname[40], vname[40];

  uint8_t transmission_mode = 1;
  unsigned char eNB_id = 0;
  uint16_t Nid_cell=0;
  uint8_t tdd_config = 3;
  uint8_t fdd_flag = 0;
  uint8_t N_RB_DL=25,osf=1;
  double BW = 7.68;

  int n_frames=1;
  int frame;
  int8_t slot, next_slot, last_slot;

  uint32_t nsymb,tx_lev,tx_lev_dB;
  uint8_t extended_prefix_flag=0;

  LTE_DL_FRAME_PARMS *frame_parms;

  uint16_t amask=0;
  uint8_t frame_mod4,num_pdcch_symbols;
  uint16_t NB_RB=25;



  DCI_ALLOC_t dci_alloc[8];


  number_of_cards = 1;
  openair_daq_vars.rx_rf_mode = 1;

  logInit();
  g_log->log_component[PHY].level = LOG_DEBUG;

  while ((c = getopt (argc, argv, "hpFb:n:m:x:N:R:")) != -1) {
    switch (c) {
    case 'n':
      n_frames = atoi(optarg);
      break;

    case 'm':
      mcs = atoi(optarg);
      break;

    case 'p':
      extended_prefix_flag=1;
      break;

    case 'x':
      transmission_mode=atoi(optarg);

      if ((transmission_mode!=1) &&
          (transmission_mode!=2) &&
          (transmission_mode!=6)) {
        msg("Unsupported transmission mode %d\n",transmission_mode);
        exit(-1);
      }

      break;

    case 'N':
      Nid_cell = atoi(optarg);
      break;

    case 'R':
      N_RB_DL = atoi(optarg);
      break;

    case 'b':
      tdd_config = atoi(optarg);
      break;

    case 'F':
      fdd_flag = 1;
      break;

    default:
    case 'h':
      printf("%s -h(elp)  -F(FDD_mode) -b tdd_config -p(extended_prefix) -N cell_id -m mcs -n n_frames -x transmission_mode(1,2,5,6)\n",argv[0]);
      printf("-h This message\n");
      printf("-F TDD mode (default TDD mode)\n");
      printf("-b TDD configuration\n");
      printf("-m MCS\n");
      printf("-p Use extended prefix mode\n");
      printf("-n Number of frames to simulate\n");
      printf("-x Transmission mode (1,2,6 for the moment)\n");
      printf("-N Nid_cell\n");
      printf("-R N_RB_DL\n");
      exit (-1);
      break;
    }
  }

  switch (N_RB_DL) {
  case 6:
    DLSCH_RB_ALLOC = 0x20;
    BW = 1.92;
    num_pdcch_symbols = 3;
    break;

  case 25:
    DLSCH_RB_ALLOC = 0x1F00;
    BW = 7.68;
    break;

  case 50:
    DLSCH_RB_ALLOC = 0x1F000;
    BW = 15.36;
    break;

  case 100:
    DLSCH_RB_ALLOC = 0x1F00000;
    BW = 30.72;
    break;
  }

  lte_param_init(transmission_mode,extended_prefix_flag,Nid_cell,N_RB_DL,osf,fdd_flag,tdd_config);


  frame_parms = &PHY_vars_eNB_g[0]->lte_frame_parms;



  for (frame=0; frame<n_frames; frame++) {
    PHY_vars_eNB_g[0]->frame = frame;

    printf("**********Frame %d*********************\n",frame);

    for (slot=-1; slot<19; slot++) {

      last_slot = (slot - 1)%20;

      if (last_slot <0)
        last_slot+=20;

      next_slot = (slot + 1)%20;

      printf("Slot %d\n",next_slot);

      phy_procedures_eNB_lte(last_slot,next_slot,PHY_vars_eNB_g[0],0);

      // OFDM generation
      do_OFDM_mod(PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdataF[eNB_id],
                  PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[eNB_id],
                  next_slot,
                  frame_parms);

    }//slot

    char fname[64], vname[64];
    sprintf(fname, "txsig%d.m", frame);
    sprintf(vname, "txs%d", frame);
    write_output(fname, vname, PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][0], FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

    if (frame == 0)
      write_output("txsigF0.m", "txsF0", PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdataF[0][0], frame_parms->ofdm_symbol_size*14*10, 1, 1);
  }//frame
}
