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

#include "SCHED/defs.h"
#include "SCHED/vars.h"
#include "LAYER2/MAC/vars.h"

#ifdef XFORMS
  #include "PHY/TOOLS/lte_phy_scope.h"
#endif

#include "unitary_defs.h"
#define N_TRIALS 100

PHY_VARS_eNB *eNB,*eNB1,*eNB2;
PHY_VARS_UE *UE;

#define UL_RB_ALLOC 0x1ff;
#define CCCH_RB_ALLOC computeRIV(eNB->frame_parms.N_RB_UL,0,2)
#define DLSCH_RB_ALLOC ((uint16_t)0x1fbf) // igore DC component,RB13

double cpuf;

DCI_PDU DCI_pdu;

DCI_PDU *get_dci(LTE_DL_FRAME_PARMS *lte_frame_parms,uint8_t log2L, uint8_t log2Lcommon, DCI_format_t format_selector[MAX_NUM_DCI], uint8_t num_dci, uint32_t rnti) {
  uint32_t BCCH_alloc_pdu[2];
  uint32_t DLSCH_alloc_pdu[2];
  uint32_t UL_alloc_pdu[2];
  int ind;
  int dci_length_bytes=0,dci_length=0;
  int BCCH_pdu_size_bits=0, BCCH_pdu_size_bytes=0;
  int UL_pdu_size_bits=0, UL_pdu_size_bytes=0;
  int mcs = 3;
  DCI_pdu.Num_dci = 0;

  if (lte_frame_parms->frame_type == TDD) {
    switch (lte_frame_parms->N_RB_DL) {
      case 6:
        dci_length = sizeof_DCI1_1_5MHz_TDD_t;
        dci_length_bytes = sizeof(DCI1_1_5MHz_TDD_t);
        ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->rah               = 0;
        ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->rballoc           = DLSCH_RB_ALLOC;
        ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->mcs               = mcs;
        ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->harq_pid          = 0;
        ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->ndi               = 1;
        ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->rv                = 0;
        ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->TPC               = 0;
        ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->dai               = 0;
        ((DCI1A_1_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->type           = 1;
        ((DCI1A_1_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->vrb_type       = 0;
        ((DCI1A_1_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->rballoc     = computeRIV(lte_frame_parms->N_RB_DL, 0, 4);
        ((DCI1A_1_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->ndi            = 1;
        ((DCI1A_1_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->rv             = 0;
        ((DCI1A_1_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->mcs            = 2;
        ((DCI1A_1_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->harq_pid       = 0;
        ((DCI1A_1_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->TPC            = 1;
        BCCH_pdu_size_bits  = sizeof_DCI1A_1_5MHz_TDD_1_6_t;
        BCCH_pdu_size_bytes = sizeof(DCI1A_1_5MHz_TDD_1_6_t);
        ((DCI0_1_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->type              = 0;
        ((DCI0_1_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->hopping           = 0;
        ((DCI0_1_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->rballoc        = DLSCH_RB_ALLOC;
        ((DCI0_1_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->mcs               = mcs;
        ((DCI0_1_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->ndi               = 1;
        ((DCI0_1_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->TPC               = 2;
        ((DCI0_1_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->cshift            = 3;
        ((DCI0_1_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->dai               = 1;
        ((DCI0_1_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->cqi_req           = 1;
        UL_pdu_size_bits  = sizeof_DCI0_1_5MHz_TDD_1_6_t;
        UL_pdu_size_bytes = sizeof(DCI0_1_5MHz_TDD_1_6_t);
        break;

      case 25:
        dci_length = sizeof_DCI1_5MHz_TDD_t;
        dci_length_bytes = sizeof(DCI1_5MHz_TDD_t);
        ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->rah                 = 0;
        ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->rballoc          = DLSCH_RB_ALLOC;
        ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->mcs                 = mcs;
        ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->harq_pid            = 0;
        ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->ndi                 = 1;
        ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->rv                  = 0;
        ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->TPC                 = 0;
        ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu[0])->dai                 = 0;
        ((DCI1A_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->type            = 1;
        ((DCI1A_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->vrb_type        = 0;
        ((DCI1A_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->rballoc      = computeRIV(lte_frame_parms->N_RB_DL, 18, 4);
        ((DCI1A_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->ndi             = 1;
        ((DCI1A_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->rv              = 0;
        ((DCI1A_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->mcs             = 2;
        ((DCI1A_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->harq_pid        = 0;
        ((DCI1A_5MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->TPC             = 1;
        BCCH_pdu_size_bits  = sizeof_DCI1A_5MHz_TDD_1_6_t;
        BCCH_pdu_size_bytes = sizeof(DCI1A_5MHz_TDD_1_6_t);
        ((DCI0_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->type              = 0;
        ((DCI0_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->hopping           = 0;
        ((DCI0_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->rballoc        = DLSCH_RB_ALLOC;
        ((DCI0_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->mcs               = mcs;
        ((DCI0_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->ndi               = 1;
        ((DCI0_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->TPC               = 2;
        ((DCI0_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->cshift            = 3;
        ((DCI0_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->dai               = 1;
        ((DCI0_5MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->cqi_req           = 1;
        UL_pdu_size_bits  = sizeof_DCI0_5MHz_TDD_1_6_t;
        UL_pdu_size_bytes = sizeof(DCI0_5MHz_TDD_1_6_t);
        break;

      case 50:
        dci_length = sizeof_DCI1_10MHz_TDD_t;
        dci_length_bytes = sizeof(DCI1_10MHz_TDD_t);
        ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu[0])->rah              = 0;
        ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu[0])->rballoc       = DLSCH_RB_ALLOC;
        ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu[0])->mcs              = mcs;
        ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu[0])->harq_pid         = 0;
        ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu[0])->ndi              = 1;
        ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu[0])->rv               = 0;
        ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu[0])->TPC              = 0;
        ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu[0])->dai              = 0;
        ((DCI1A_10MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->type          = 1;
        ((DCI1A_10MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->vrb_type      = 0;
        ((DCI1A_10MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->rballoc    = computeRIV(lte_frame_parms->N_RB_DL, 30, 4);
        ((DCI1A_10MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->ndi           = 1;
        ((DCI1A_10MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->rv            = 0;
        ((DCI1A_10MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->mcs           = 2;
        ((DCI1A_10MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->harq_pid      = 0;
        ((DCI1A_10MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->TPC           = 1;
        BCCH_pdu_size_bits  = sizeof_DCI1A_10MHz_TDD_1_6_t;
        BCCH_pdu_size_bytes = sizeof(DCI1A_10MHz_TDD_1_6_t);
        ((DCI0_10MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->type              = 0;
        ((DCI0_10MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->hopping           = 0;
        ((DCI0_10MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->rballoc        = DLSCH_RB_ALLOC;
        ((DCI0_10MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->mcs               = mcs;
        ((DCI0_10MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->ndi               = 1;
        ((DCI0_10MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->TPC               = 2;
        ((DCI0_10MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->cshift            = 3;
        ((DCI0_10MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->dai               = 1;
        ((DCI0_10MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->cqi_req           = 1;
        UL_pdu_size_bits  = sizeof_DCI0_10MHz_TDD_1_6_t;
        UL_pdu_size_bytes = sizeof(DCI0_10MHz_TDD_1_6_t);
        break;

      case 100:
        dci_length = sizeof_DCI1_20MHz_TDD_t;
        dci_length_bytes = sizeof(DCI1_20MHz_TDD_t);
        ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu[0])->rah                = 0;
        ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu[0])->rballoc         = DLSCH_RB_ALLOC;
        ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu[0])->mcs                = mcs;
        ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu[0])->harq_pid           = 0;
        ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu[0])->ndi                = 1;
        ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu[0])->rv                 = 0;
        ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu[0])->TPC                = 0;
        ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu[0])->dai                = 0;
        ((DCI1A_20MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->type            = 1;
        ((DCI1A_20MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->vrb_type        = 0;
        ((DCI1A_20MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->rballoc      = computeRIV(lte_frame_parms->N_RB_DL, 70, 4);
        ((DCI1A_20MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->ndi             = 1;
        ((DCI1A_20MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->rv              = 0;
        ((DCI1A_20MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->mcs             = 2;
        ((DCI1A_20MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->harq_pid        = 0;
        ((DCI1A_20MHz_TDD_1_6_t *)&BCCH_alloc_pdu[0])->TPC             = 1;
        BCCH_pdu_size_bits  = sizeof_DCI1A_20MHz_TDD_1_6_t;
        BCCH_pdu_size_bytes = sizeof(DCI1A_20MHz_TDD_1_6_t);
        ((DCI0_20MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->type              = 0;
        ((DCI0_20MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->hopping           = 0;
        ((DCI0_20MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->rballoc        = DLSCH_RB_ALLOC;
        ((DCI0_20MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->mcs               = mcs;
        ((DCI0_20MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->ndi               = 1;
        ((DCI0_20MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->TPC               = 2;
        ((DCI0_20MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->cshift            = 3;
        ((DCI0_20MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->dai               = 1;
        ((DCI0_20MHz_TDD_1_6_t *)&UL_alloc_pdu[0])->cqi_req           = 1;
        UL_pdu_size_bits  = sizeof_DCI0_20MHz_TDD_1_6_t;
        UL_pdu_size_bytes = sizeof(DCI0_20MHz_TDD_1_6_t);
        break;
    }
  } else { //FDD
    switch (lte_frame_parms->N_RB_DL) {
      case 6:
        dci_length = sizeof_DCI1_1_5MHz_FDD_t;
        dci_length_bytes = sizeof(DCI1_1_5MHz_FDD_t);
        ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu[0])->rah           = 0;
        ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu[0])->rballoc    = DLSCH_RB_ALLOC;
        ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu[0])->mcs           = mcs;
        ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu[0])->harq_pid      = 0;
        ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu[0])->ndi           = 1;
        ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu[0])->rv            = 0;
        ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu[0])->TPC           = 0;
        ((DCI1A_1_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->type           = 1;
        ((DCI1A_1_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->vrb_type       = 0;
        ((DCI1A_1_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->rballoc     = computeRIV(lte_frame_parms->N_RB_DL, 0, 4);
        ((DCI1A_1_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->ndi            = 1;
        ((DCI1A_1_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->rv             = 0;
        ((DCI1A_1_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->mcs            = 2;
        ((DCI1A_1_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->harq_pid       = 0;
        ((DCI1A_1_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->TPC            = 1;
        BCCH_pdu_size_bits  = sizeof_DCI1A_1_5MHz_FDD_t;
        BCCH_pdu_size_bytes = sizeof(DCI1A_1_5MHz_FDD_t);
        ((DCI0_1_5MHz_FDD_t *)&UL_alloc_pdu[0])->type              = 0;
        ((DCI0_1_5MHz_FDD_t *)&UL_alloc_pdu[0])->hopping           = 0;
        ((DCI0_1_5MHz_FDD_t *)&UL_alloc_pdu[0])->rballoc        = DLSCH_RB_ALLOC;
        ((DCI0_1_5MHz_FDD_t *)&UL_alloc_pdu[0])->mcs               = mcs;
        ((DCI0_1_5MHz_FDD_t *)&UL_alloc_pdu[0])->ndi               = 1;
        ((DCI0_1_5MHz_FDD_t *)&UL_alloc_pdu[0])->TPC               = 2;
        ((DCI0_1_5MHz_FDD_t *)&UL_alloc_pdu[0])->cshift            = 3;
        ((DCI0_1_5MHz_FDD_t *)&UL_alloc_pdu[0])->cqi_req           = 1;
        UL_pdu_size_bits  = sizeof_DCI0_1_5MHz_FDD_t;
        UL_pdu_size_bytes = sizeof(DCI0_1_5MHz_FDD_t);
        break;

      case 25:
        dci_length = sizeof_DCI1_5MHz_FDD_t;
        dci_length_bytes = sizeof(DCI1_5MHz_FDD_t);
        ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu[0])->rah           = 0;
        ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu[0])->rballoc    = DLSCH_RB_ALLOC;
        ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu[0])->mcs           = mcs;
        ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu[0])->harq_pid      = 0;
        ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu[0])->ndi           = 1;
        ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu[0])->rv            = 0;
        ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu[0])->TPC           = 0;
        ((DCI1A_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->type           = 1;
        ((DCI1A_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->vrb_type       = 0;
        ((DCI1A_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->rballoc     = computeRIV(lte_frame_parms->N_RB_DL, 18, 4);
        ((DCI1A_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->ndi            = 1;
        ((DCI1A_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->rv             = 0;
        ((DCI1A_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->mcs            = 2;
        ((DCI1A_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->harq_pid       = 0;
        ((DCI1A_5MHz_FDD_t *)&BCCH_alloc_pdu[0])->TPC            = 1;
        BCCH_pdu_size_bits  = sizeof_DCI1A_5MHz_FDD_t;
        BCCH_pdu_size_bytes = sizeof(DCI1A_5MHz_FDD_t);
        ((DCI0_5MHz_FDD_t *)&UL_alloc_pdu[0])->type              = 0;
        ((DCI0_5MHz_FDD_t *)&UL_alloc_pdu[0])->hopping           = 0;
        ((DCI0_5MHz_FDD_t *)&UL_alloc_pdu[0])->rballoc        = DLSCH_RB_ALLOC;
        ((DCI0_5MHz_FDD_t *)&UL_alloc_pdu[0])->mcs               = mcs;
        ((DCI0_5MHz_FDD_t *)&UL_alloc_pdu[0])->ndi               = 1;
        ((DCI0_5MHz_FDD_t *)&UL_alloc_pdu[0])->TPC               = 2;
        ((DCI0_5MHz_FDD_t *)&UL_alloc_pdu[0])->cshift            = 3;
        ((DCI0_5MHz_FDD_t *)&UL_alloc_pdu[0])->cqi_req           = 1;
        UL_pdu_size_bits  = sizeof_DCI0_5MHz_FDD_t;
        UL_pdu_size_bytes = sizeof(DCI0_5MHz_FDD_t);
        break;

      case 50:
        dci_length = sizeof_DCI1_10MHz_FDD_t;
        dci_length_bytes = sizeof(DCI1_10MHz_FDD_t);
        ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu[0])->rah           = 0;
        ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu[0])->rballoc    = DLSCH_RB_ALLOC;
        ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu[0])->mcs           = mcs;
        ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu[0])->harq_pid      = 0;
        ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu[0])->ndi           = 1;
        ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu[0])->rv            = 0;
        ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu[0])->TPC           = 0;
        ((DCI1A_10MHz_FDD_t *)&BCCH_alloc_pdu[0])->type           = 1;
        ((DCI1A_10MHz_FDD_t *)&BCCH_alloc_pdu[0])->vrb_type       = 0;
        ((DCI1A_10MHz_FDD_t *)&BCCH_alloc_pdu[0])->rballoc     = computeRIV(lte_frame_parms->N_RB_DL, 30, 4);
        ((DCI1A_10MHz_FDD_t *)&BCCH_alloc_pdu[0])->ndi            = 1;
        ((DCI1A_10MHz_FDD_t *)&BCCH_alloc_pdu[0])->rv             = 0;
        ((DCI1A_10MHz_FDD_t *)&BCCH_alloc_pdu[0])->mcs            = 2;
        ((DCI1A_10MHz_FDD_t *)&BCCH_alloc_pdu[0])->harq_pid       = 0;
        ((DCI1A_10MHz_FDD_t *)&BCCH_alloc_pdu[0])->TPC            = 1;
        BCCH_pdu_size_bits  = sizeof_DCI1A_10MHz_FDD_t;
        BCCH_pdu_size_bytes = sizeof(DCI1A_10MHz_FDD_t);
        ((DCI0_10MHz_FDD_t *)&UL_alloc_pdu[0])->type              = 0;
        ((DCI0_10MHz_FDD_t *)&UL_alloc_pdu[0])->hopping           = 0;
        ((DCI0_10MHz_FDD_t *)&UL_alloc_pdu[0])->rballoc        = DLSCH_RB_ALLOC;
        ((DCI0_10MHz_FDD_t *)&UL_alloc_pdu[0])->mcs               = mcs;
        ((DCI0_10MHz_FDD_t *)&UL_alloc_pdu[0])->ndi               = 1;
        ((DCI0_10MHz_FDD_t *)&UL_alloc_pdu[0])->TPC               = 2;
        ((DCI0_10MHz_FDD_t *)&UL_alloc_pdu[0])->cshift            = 3;
        ((DCI0_10MHz_FDD_t *)&UL_alloc_pdu[0])->cqi_req           = 1;
        UL_pdu_size_bits  = sizeof_DCI0_10MHz_FDD_t;
        UL_pdu_size_bytes = sizeof(DCI0_10MHz_FDD_t);
        break;

      case 100:
        dci_length = sizeof_DCI1_20MHz_FDD_t;
        dci_length_bytes = sizeof(DCI1_20MHz_FDD_t);
        ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu[0])->rah           = 0;
        ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu[0])->rballoc    = DLSCH_RB_ALLOC;
        ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu[0])->mcs           = mcs;
        ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu[0])->harq_pid      = 0;
        ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu[0])->ndi           = 1;
        ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu[0])->rv            = 0;
        ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu[0])->TPC           = 0;
        ((DCI1A_20MHz_FDD_t *)&BCCH_alloc_pdu[0])->type           = 1;
        ((DCI1A_20MHz_FDD_t *)&BCCH_alloc_pdu[0])->vrb_type       = 0;
        ((DCI1A_20MHz_FDD_t *)&BCCH_alloc_pdu[0])->rballoc     = computeRIV(lte_frame_parms->N_RB_DL, 70, 4);
        ((DCI1A_20MHz_FDD_t *)&BCCH_alloc_pdu[0])->ndi            = 1;
        ((DCI1A_20MHz_FDD_t *)&BCCH_alloc_pdu[0])->rv             = 0;
        ((DCI1A_20MHz_FDD_t *)&BCCH_alloc_pdu[0])->mcs            = 2;
        ((DCI1A_20MHz_FDD_t *)&BCCH_alloc_pdu[0])->harq_pid       = 0;
        ((DCI1A_20MHz_FDD_t *)&BCCH_alloc_pdu[0])->TPC            = 1;
        BCCH_pdu_size_bits  = sizeof_DCI1A_20MHz_FDD_t;
        BCCH_pdu_size_bytes = sizeof(DCI1A_20MHz_FDD_t);
        ((DCI0_20MHz_FDD_t *)&UL_alloc_pdu[0])->type              = 0;
        ((DCI0_20MHz_FDD_t *)&UL_alloc_pdu[0])->hopping           = 0;
        ((DCI0_20MHz_FDD_t *)&UL_alloc_pdu[0])->rballoc        = DLSCH_RB_ALLOC;
        ((DCI0_20MHz_FDD_t *)&UL_alloc_pdu[0])->mcs               = mcs;
        ((DCI0_20MHz_FDD_t *)&UL_alloc_pdu[0])->ndi               = 1;
        ((DCI0_20MHz_FDD_t *)&UL_alloc_pdu[0])->TPC               = 2;
        ((DCI0_20MHz_FDD_t *)&UL_alloc_pdu[0])->cshift            = 3;
        ((DCI0_20MHz_FDD_t *)&UL_alloc_pdu[0])->cqi_req           = 1;
        UL_pdu_size_bits  = sizeof_DCI0_20MHz_FDD_t;
        UL_pdu_size_bytes = sizeof(DCI0_20MHz_FDD_t);
        break;
    }
  }

  for (ind = 0; ind<num_dci; ind++) {
    if (format_selector[ind]==format1A) {
      // add common dci
      DCI_pdu.dci_alloc[ind].dci_length   = BCCH_pdu_size_bits;
      DCI_pdu.dci_alloc[ind].L            = log2Lcommon;
      DCI_pdu.dci_alloc[ind].rnti         = SI_RNTI;
      DCI_pdu.dci_alloc[ind].format       = format1A;
      DCI_pdu.dci_alloc[ind].ra_flag      = 0;
      DCI_pdu.dci_alloc[ind].search_space = DCI_COMMON_SPACE;
      memcpy((void *)&DCI_pdu.dci_alloc[ind].dci_pdu[0], &BCCH_alloc_pdu[0], BCCH_pdu_size_bytes);
      DCI_pdu.Num_dci++;
      printf("Added common dci (%d) for rnti %x\n",ind,SI_RNTI);
    }

    if (format_selector[ind]==format1) {
      DCI_pdu.dci_alloc[ind].dci_length   = dci_length;
      DCI_pdu.dci_alloc[ind].L            = log2L;
      DCI_pdu.dci_alloc[ind].rnti         = rnti;
      DCI_pdu.dci_alloc[ind].format       = format1;
      DCI_pdu.dci_alloc[ind].ra_flag      = 0;
      DCI_pdu.dci_alloc[ind].search_space = DCI_UE_SPACE;
      memcpy((void *)&DCI_pdu.dci_alloc[ind].dci_pdu[0], &DLSCH_alloc_pdu[0], dci_length_bytes);
      DCI_pdu.Num_dci++;
    }

    if (format_selector[ind]==format0) {
      DCI_pdu.dci_alloc[ind].dci_length   = UL_pdu_size_bits;
      DCI_pdu.dci_alloc[ind].L            = log2L;
      DCI_pdu.dci_alloc[ind].rnti         = rnti;
      DCI_pdu.dci_alloc[ind].format       = format0;
      DCI_pdu.dci_alloc[ind].ra_flag      = 0;
      DCI_pdu.dci_alloc[ind].search_space = DCI_UE_SPACE;
      memcpy((void *)&DCI_pdu.dci_alloc[ind].dci_pdu[0], &UL_alloc_pdu[0], UL_pdu_size_bytes);
      DCI_pdu.Num_dci++;
    }
  }

  return(&DCI_pdu);
}

extern int QPSK[4],QPSK2[4];

int main(int argc, char **argv) {
  char c;
  int i,l,aa;
  double sigma2, sigma2_dB=0,SNR,snr0=-2.0,snr1;
  int **txdata;
  double s_re[2][30720*2],s_im[2][30720*2],r_re[2][30720*2],r_im[2][30720*2];
  double iqim=0.0;
  //  int subframe_offset;
  uint8_t subframe=0;
#ifdef XFORMS
  FD_lte_phy_scope_ue *form_ue;
  char title[255];
#endif
  int trial, n_errors_common=0,n_errors_ul=0,n_errors_dl=0,n_errors_cfi=0,n_errors_hi=0;
  unsigned char eNb_id = 0;
  uint8_t awgn_flag=0;
  int n_frames=1;
  channel_desc_t *eNB2UE;
  uint32_t nsymb,tx_lev,tx_lev_dB=0,num_pdcch_symbols=3;
  uint8_t extended_prefix_flag=0,transmission_mode=1,n_tx=1,n_rx=1;
  uint16_t Nid_cell=0;
  //  int8_t interf1=-128,interf2=-128;
  uint8_t dci_cnt=0;
  LTE_DL_FRAME_PARMS *frame_parms;
  uint8_t log2L=2, log2Lcommon=2;
  DCI_format_t format_selector[MAX_NUM_DCI];
  uint8_t num_dci=0;
  uint8_t numCCE,common_active=0,ul_active=0,dl_active=0;
  uint32_t n_trials_common=0,n_trials_ul=0,n_trials_dl=0,false_detection_cnt=0;
  uint8_t common_rx,ul_rx,dl_rx;
  uint8_t tdd_config=3;
  FILE *input_fd=NULL;
  char input_val_str[50],input_val_str2[50];
  uint16_t n_rnti=0x1234;
  uint8_t osf=1,N_RB_DL=25;
  SCM_t channel_model=Rayleigh1_anticorr;
  DCI_ALLOC_t dci_alloc_rx[8];
  int ret;
  uint8_t harq_pid;
  uint8_t phich_ACK;
  uint8_t num_phich_interf = 0;
  frame_type_t frame_type=TDD;
  //  int re_offset;
  //  uint32_t *txptr;
  int aarx;
  int k;
  uint32_t perfect_ce = 0;
  int CCE_table[800];
  number_of_cards = 1;
  cpuf = get_cpu_freq_GHz();
  logInit();

  while ((c = getopt (argc, argv, "hapFg:R:c:n:s:x:y:z:L:M:N:I:f:i:S:P:Y")) != -1) {
    switch (c) {
      case 'a':
        printf("Running AWGN simulation\n");
        awgn_flag = 1;
        break;

      case 'R':
        N_RB_DL = atoi(optarg);
        break;

      case 'F':
        frame_type = FDD;
        break;

      case 'c':
        tdd_config=atoi(optarg);

        if (tdd_config>6) {
          printf("Illegal tdd_config %d (should be 0-6)\n",tdd_config);
          exit(-1);
        }

        break;

      case 'g':
        switch((char)*optarg) {
          case 'A':
            channel_model=SCM_A;
            break;

          case 'B':
            channel_model=SCM_B;
            break;

          case 'C':
            channel_model=SCM_C;
            break;

          case 'D':
            channel_model=SCM_D;
            break;

          case 'E':
            channel_model=EPA;
            break;

          case 'F':
            channel_model=EVA;
            break;

          case 'G':
            channel_model=ETU;
            break;

          default:
            printf("Unsupported channel model!\n");
            exit(-1);
        }

        break;

      /*
          case 'i':
      interf1=atoi(optarg);
      break;
          case 'j':
      interf2=atoi(optarg);
      break;
      */
      case 'n':
        n_frames = atoi(optarg);
        break;

      case 's':
        snr0 = atoi(optarg);
        break;

      case 'p':
        extended_prefix_flag=1;
        break;

      case 'x':
        transmission_mode=atoi(optarg);

        if ((transmission_mode!=1) &&
            (transmission_mode!=2) &&
            (transmission_mode!=6)) {
          printf("Unsupported transmission mode %d\n",transmission_mode);
          exit(-1);
        }

        break;

      case 'y':
        n_tx=atoi(optarg);

        if ((n_tx==0) || (n_tx>2)) {
          printf("Unsupported number of tx antennas %d\n",n_tx);
          exit(-1);
        }

        break;

      case 'z':
        n_rx=atoi(optarg);

        if ((n_rx==0) || (n_rx>2)) {
          printf("Unsupported number of rx antennas %d\n",n_rx);
          exit(-1);
        }

        break;

      case 'S':
        subframe=atoi(optarg);
        break;

      case 'L':
        log2L=atoi(optarg);

        if ((log2L!=0)&&
            (log2L!=1)&&
            (log2L!=2)&&
            (log2L!=3)) {
          printf("Unsupported DCI aggregation level %d (should be 0,1,2,3)\n",log2L);
          exit(-1);
        }

        break;

      case 'M':
        log2Lcommon=atoi(optarg);

        if ((log2Lcommon!=2)&&
            (log2Lcommon!=3)) {
          printf("Unsupported Common DCI aggregation level %d (should be 2 or 3)\n",log2Lcommon);
          exit(-1);
        }

        break;

      case 'N':
        format_selector[num_dci] = (DCI_format_t) atoi(optarg);

        if ((format_selector[num_dci]<format0) || (format_selector[num_dci] > format1A)) {
          printf("only formats 0, 1, and 1A supported for the moment\n");
          exit(-1);
        }

        if (format_selector[num_dci]==format0) ul_active=1;

        if (format_selector[num_dci]==format1A) common_active=1;

        if (format_selector[num_dci]==format1) dl_active=1;

        num_dci++;
        break;

      case 'O':
        osf = atoi(optarg);
        break;

      case 'I':
        Nid_cell = atoi(optarg);
        break;

      case 'f':
        input_fd = fopen(optarg,"r");

        if (input_fd==NULL) {
          printf("Problem with filename %s\n",optarg);
          exit(-1);
        }

        break;

      case 'i':
        n_rnti=atoi(optarg);
        break;

      case 'P':
        num_phich_interf=atoi(optarg);
        break;

      case 'Y':
        perfect_ce = 1;
        break;

      case 'h':
        printf("%s -h(elp) -a(wgn on) -c tdd_config -n n_frames -r RiceanFactor -s snr0 -t Delayspread -x transmission mode (1,2,6) -y TXant -z RXant -L AggregLevelUEspec -M AggregLevelCommonDCI -N DCIFormat\n\n",
               argv[0]);
        printf("-h This message\n");
        printf("-a Use AWGN channel and not multipath\n");
        printf("-c TDD config\n");
        printf("-S Subframe number (0..9)\n");
        printf("-R N_RB_DL\n");
        printf("-F use FDD frame\n");
        printf("-p Use extended prefix mode\n");
        printf("-n Number of frames to simulate\n");
        printf("-r Ricean factor (dB, 0 means Rayleigh, 100 is almost AWGN\n");
        printf("-s Starting SNR, runs from SNR to SNR + 5 dB.  If n_frames is 1 then just SNR is simulated\n");
        printf("-t Delay spread for multipath channel\n");
        printf("-x Transmission mode (1,2,6 for the moment)\n");
        printf("-y Number of TX antennas used in eNB\n");
        printf("-z Number of RX antennas used in UE\n");
        printf("-P Number of interfering PHICH\n");
        printf("-L log2 of Aggregation level for UE Specific DCI (0,1,2,3)\n");
        printf("-M log2 Aggregation level for Common DCI (4,8)\n");
        printf("-N Format for UE Spec DCI (0 - format0,\n");
        printf("                           1 - format1,\n");
        printf("                           2 - format1A,\n");
        printf("                           3 - format1B_2A,\n");
        printf("                           4 - format1B_4A,\n");
        printf("                           5 - format1C,\n");
        printf("                           6 - format1D_2A,\n");
        printf("                           7 - format1D_4A,\n");
        printf("                           8 - format2A_2A_L10PRB,\n");
        printf("                           9 - format2A_2A_M10PRB,\n");
        printf("                          10 - format2A_4A_L10PRB,\n");
        printf("                          11 - format2A_4A_M10PRB,\n");
        printf("                          12 - format2_2A_L10PRB,\n");
        printf("                          13 - format2_2A_M10PRB,\n");
        printf("                          14 - format2_4A_L10PRB,\n");
        printf("                          15 - format2_4A_M10PRB\n");
        printf("                          16 - format2_2D_M10PRB\n");
        printf("                          17 - format2_2D_L10PRB\n");
        printf("   can be called multiple times to add more than one DCI\n");
        printf("-O Oversampling factor\n");
        printf("-I Cell Id\n");
        printf("-F Input sample stream\n");
        exit(1);
        break;
    }
  }

  if ((transmission_mode>1) && (n_tx==1))
    n_tx=2;

  lte_param_init(n_tx,
                 n_tx,
                 n_rx,
                 transmission_mode,
                 extended_prefix_flag,
                 frame_type,
                 Nid_cell,
                 tdd_config,
                 N_RB_DL,
                 0,
                 osf,
                 perfect_ce);
#ifdef XFORMS
  fl_initialize (&argc, argv, NULL, 0, 0);
  form_ue = create_lte_phy_scope_ue();
  sprintf (title, "LTE PHY SCOPE UE");
  fl_show_form (form_ue->lte_phy_scope_ue, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);
#endif
  mac_xface->computeRIV = computeRIV;
  mac_xface->frame_parms = &eNB->frame_parms;
  //  init_transport_channels(transmission_mode);

  if (n_frames==1)
    snr1 = snr0+.1;
  else
    snr1 = snr0+8.0;

  printf("SNR0 %f, SNR1 %f\n",snr0,snr1);
  frame_parms = &eNB->frame_parms;
  printf("Getting %d dcis\n",num_dci);
  get_dci(frame_parms, log2L, log2Lcommon, format_selector, num_dci, n_rnti);
  txdata = eNB->common_vars.txdata[eNb_id];
  nsymb = (eNB->frame_parms.Ncp == 0) ? 14 : 12;
  printf("Subframe %d, FFT Size %d, Extended Prefix %d, Samples per subframe %d, Symbols per subframe %u\n",
         subframe,NUMBER_OF_OFDM_CARRIERS,
         eNB->frame_parms.Ncp,eNB->frame_parms.samples_per_tti,nsymb);
  eNB2UE = new_channel_desc_scm(eNB->frame_parms.nb_antennas_tx,
                                UE->frame_parms.nb_antennas_rx,
                                channel_model,
                                N_RB2sampling_rate(eNB->frame_parms.N_RB_DL),
                                N_RB2channel_bandwidth(eNB->frame_parms.N_RB_DL),
                                0,
                                0,
                                0, 0);
  L1_rxtx_proc_t *proc_rxtx = (subframe == 0)? &eNB->proc.L1_proc: &eNB->proc.L1_proc_tx;
  eNB->ulsch[0] = new_eNB_ulsch(MAX_TURBO_ITERATIONS,N_RB_DL,0);
  UE->ulsch[0]   = new_ue_ulsch(N_RB_DL,0);
  proc_rxtx->frame_tx    = 0;
  proc_rxtx->subframe_tx = subframe;

  if (input_fd==NULL) {
    printf("No input file, so starting TX\n");
  } else {
    i=0;

    while (!feof(input_fd)) {
      ret=fscanf(input_fd,"%49s %49s",input_val_str,input_val_str2);//&input_val1,&input_val2);

      if (ret != 2) {
        printf("%s:%d:%s: fscanf error, exiting\n", __FILE__, __LINE__, __FUNCTION__);
        exit(1);
      }

      if ((i%4)==0) {
        ((short *)txdata[0])[i/2] = (short)((1<<15)*strtod(input_val_str,NULL));
        ((short *)txdata[0])[(i/2)+1] = (short)((1<<15)*strtod(input_val_str2,NULL));

        if ((i/4)<100)
          printf("sample %d => %e + j%e (%d +j%d)\n",i/4,strtod(input_val_str,NULL),strtod(input_val_str2,NULL),((short *)txdata[0])[i/4],((short *)txdata[0])[(i/4)+1]); //1,input_val2,);
      }

      i++;

      if (i>(4*FRAME_LENGTH_SAMPLES))
        break;
    }

    printf("Read in %d samples\n",i/4);
    LOG_M("txsig0.m","txs0", txdata[0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
    //    LOG_M("txsig1.m","txs1", txdata[1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
    tx_lev = signal_energy(&txdata[0][0],
                           OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES);
    tx_lev_dB = (unsigned int) dB_fixed(tx_lev);
  }

  UE->UE_mode[0] = PUSCH;

  //  nCCE_max = get_nCCE(3,&eNB->frame_parms,get_mi(&eNB->frame_parms,0));
  //printf("nCCE_max %d\n",nCCE_max);

  //printf("num_phich interferers %d\n",num_phich_interf);
  for (SNR=snr0; SNR<snr1; SNR+=0.2) {
    n_errors_common = 0;
    n_errors_ul     = 0;
    n_errors_dl     = 0;
    n_errors_cfi    = 0;
    n_errors_hi     = 0;
    n_trials_common=0;
    n_trials_ul=0;
    n_trials_dl=0;

    for (trial=0; trial<n_frames; trial++) {
      //    printf("DCI (SF %d): txdataF %p (0 %p)\n",subframe,&eNB->common_vars.txdataF[eNb_id][aa][512*14*subframe],&eNB->common_vars.txdataF[eNb_id][aa][0]);
      for (aa=0; aa<eNB->frame_parms.nb_antennas_tx; aa++) {
        memset(&eNB->common_vars.txdataF[eNb_id][aa][0],0,FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX*sizeof(int32_t));
      }

      generate_pilots_slot(eNB,
                           eNB->common_vars.txdataF[eNb_id],
                           AMP,    //1024,
                           (subframe*2),
                           0);
      generate_pilots_slot(eNB,
                           eNB->common_vars.txdataF[eNb_id],
                           AMP,    //1024,
                           (subframe*2)+1,
                           0);

      if (input_fd == NULL) {
        numCCE=0;
        n_trials_common++;
        common_active = 1;

        if (eNB->frame_parms.N_RB_DL >= 50) {
          if (ul_active==1) {
            n_trials_ul++;
          }
        }

        if (eNB->frame_parms.N_RB_DL >= 25) {
          if (dl_active==1) {
            n_trials_dl++;
          }
        }

        num_pdcch_symbols = get_num_pdcch_symbols(DCI_pdu.Num_dci,
                            DCI_pdu.dci_alloc, frame_parms, subframe);
        numCCE = get_nCCE(num_pdcch_symbols,&eNB->frame_parms,get_mi(&eNB->frame_parms,subframe));

        if (n_frames==1) {
          printf("num_dci %d, num_pddch_symbols %u, nCCE %d\n",
                 DCI_pdu.Num_dci,
                 num_pdcch_symbols,numCCE);
        }

        // apply RNTI-based nCCE allocation
        memset(CCE_table,0,800*sizeof(int));

        for (i = 0; i < DCI_pdu.Num_dci; i++) {
          // SI RNTI
          if (DCI_pdu.dci_alloc[i].rnti == SI_RNTI) {
            DCI_pdu.dci_alloc[i].firstCCE = get_nCCE_offset_l1(CCE_table,
                                            1<<DCI_pdu.dci_alloc[i].L,
                                            numCCE,
                                            1,
                                            SI_RNTI,
                                            subframe);
          }
          // RA RNTI
          else if (DCI_pdu.dci_alloc[i].ra_flag == 1) {
            DCI_pdu.dci_alloc[i].firstCCE = get_nCCE_offset_l1(CCE_table,
                                            1<<DCI_pdu.dci_alloc[i].L,
                                            numCCE,
                                            1,
                                            DCI_pdu.dci_alloc[i].rnti,
                                            subframe);
          }
          // C RNTI
          else {
            DCI_pdu.dci_alloc[i].firstCCE = get_nCCE_offset_l1(CCE_table,
                                            1<<DCI_pdu.dci_alloc[i].L,
                                            numCCE,
                                            0,
                                            DCI_pdu.dci_alloc[i].rnti,
                                            subframe);
          }

          if (n_frames==1)
            printf("dci %d: rnti 0x%x, format %d, L %d (aggreg %d), nCCE %d/%d dci_length %d\n",i,DCI_pdu.dci_alloc[i].rnti, DCI_pdu.dci_alloc[i].format,
                   DCI_pdu.dci_alloc[i].L, 1<<DCI_pdu.dci_alloc[i].L, DCI_pdu.dci_alloc[i].firstCCE, numCCE, DCI_pdu.dci_alloc[i].dci_length);

          if (DCI_pdu.dci_alloc[i].firstCCE==-1)
            exit(-1);
        }

        num_pdcch_symbols = generate_dci_top(DCI_pdu.Num_dci,
                                             DCI_pdu.dci_alloc,
                                             0,
                                             AMP,
                                             &eNB->frame_parms,
                                             eNB->common_vars.txdataF[eNb_id],
                                             subframe);

        if (n_frames==1)
          printf("num_pdcch_symbols at TX %u\n",num_pdcch_symbols);

        if (is_phich_subframe(&eNB->frame_parms,subframe)) {
          if (n_frames==1)
            printf("generating PHICH\n");

          harq_pid = phich_subframe_to_harq_pid(&eNB->frame_parms, proc_rxtx->frame_tx, subframe);
          phich_ACK = taus()&1;
          eNB->ulsch[0]->harq_processes[harq_pid]->phich_active = 1;
          eNB->ulsch[0]->harq_processes[harq_pid]->first_rb     = 0;
          eNB->ulsch[0]->harq_processes[harq_pid]->n_DMRS       = 0;
          eNB->ulsch[0]->harq_processes[harq_pid]->phich_ACK    = phich_ACK;
          eNB->ulsch[0]->harq_processes[harq_pid]->dci_alloc    = 1;
          UE->ulsch[0]->harq_processes[harq_pid]->first_rb       = 0;
          UE->ulsch[0]->harq_processes[harq_pid]->n_DMRS         = 0;
          generate_phich_top(eNB,proc_rxtx,AMP,0);

          // generate 3 interfering PHICH
          if (num_phich_interf>0) {
            eNB->ulsch[0]->harq_processes[harq_pid]->first_rb = 4;
            generate_phich_top(eNB,proc_rxtx,1024,0);
          }

          if (num_phich_interf>1) {
            eNB->ulsch[0]->harq_processes[harq_pid]->first_rb = 8;
            eNB->ulsch[0]->harq_processes[harq_pid]->n_DMRS = 1;
            generate_phich_top(eNB,proc_rxtx,1024,0);
          }

          if (num_phich_interf>2) {
            eNB->ulsch[0]->harq_processes[harq_pid]->first_rb = 12;
            eNB->ulsch[0]->harq_processes[harq_pid]->n_DMRS = 1;
            generate_phich_top(eNB,proc_rxtx,1024,0);
          }

          eNB->ulsch[0]->harq_processes[harq_pid]->first_rb = 0;
        }

        //  LOG_M("pilotsF.m","rsF",txdataF[0],lte_eNB->frame_parms.ofdm_symbol_size,1,1);

        if (n_frames==1) {
          LOG_M("txsigF0.m","txsF0", eNB->common_vars.txdataF[eNb_id][0],4*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX,1,1);

          if (eNB->frame_parms.nb_antenna_ports_eNB > 1)
            LOG_M("txsigF1.m","txsF1", eNB->common_vars.txdataF[eNb_id][1],4*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX,1,1);
        }

        tx_lev = 0;

        for (aa=0; aa<eNB->frame_parms.nb_antenna_ports_eNB; aa++) {
          if (eNB->frame_parms.Ncp == 1)
            PHY_ofdm_mod(&eNB->common_vars.txdataF[eNb_id][aa][subframe*nsymb*eNB->frame_parms.ofdm_symbol_size],        // input,
                         &txdata[aa][subframe*eNB->frame_parms.samples_per_tti],         // output
                         eNB->frame_parms.ofdm_symbol_size,
                         2*nsymb,                 // number of symbols
                         eNB->frame_parms.nb_prefix_samples,               // number of prefix samples
                         CYCLIC_PREFIX);
          else {
            normal_prefix_mod(&eNB->common_vars.txdataF[eNb_id][aa][subframe*nsymb*eNB->frame_parms.ofdm_symbol_size],
                              &txdata[aa][subframe*eNB->frame_parms.samples_per_tti],
                              2*nsymb,
                              frame_parms);
          }

          tx_lev += signal_energy(&txdata[aa][subframe*eNB->frame_parms.samples_per_tti],
                                  eNB->frame_parms.ofdm_symbol_size);
        }

        tx_lev_dB = (unsigned int) dB_fixed(tx_lev);
      }

      for (i=0; i<2*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES; i++) {
        for (aa=0; aa<eNB->frame_parms.nb_antenna_ports_eNB; aa++) {
          if (awgn_flag == 0) {
            s_re[aa][i] = ((double)(((short *)txdata[aa]))[(2*subframe*UE->frame_parms.samples_per_tti) + (i<<1)]);
            s_im[aa][i] = ((double)(((short *)txdata[aa]))[(2*subframe*UE->frame_parms.samples_per_tti) + (i<<1)+1]);
          } else {
            for (aarx=0; aarx<UE->frame_parms.nb_antennas_rx; aarx++) {
              if (aa==0) {
                r_re[aarx][i] = ((double)(((short *)txdata[aa]))[(2*subframe*UE->frame_parms.samples_per_tti) + (i<<1)]);
                r_im[aarx][i] = ((double)(((short *)txdata[aa]))[(2*subframe*UE->frame_parms.samples_per_tti) + (i<<1)+1]);
              } else {
                r_re[aarx][i] += ((double)(((short *)txdata[aa]))[(2*subframe*UE->frame_parms.samples_per_tti) + (i<<1)]);
                r_im[aarx][i] += ((double)(((short *)txdata[aa]))[(2*subframe*UE->frame_parms.samples_per_tti) + (i<<1)+1]);
              }
            }
          }
        }
      }

      if (awgn_flag == 0) {
        multipath_channel(eNB2UE,s_re,s_im,r_re,r_im,
                          2*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES,0);
      }

      //LOG_M("channel0.m","chan0",ch[0],channel_length,1,8);
      // scale by path_loss = NOW - P_noise
      //sigma2       = pow(10,sigma2_dB/10);
      //N0W          = -95.87;
      sigma2_dB = (double)tx_lev_dB +10*log10((double)eNB->frame_parms.ofdm_symbol_size/(double)(12*eNB->frame_parms.N_RB_DL)) - SNR;

      if (n_frames==1)
        printf("sigma2_dB %f (SNR %f dB) tx_lev_dB %u\n",sigma2_dB,SNR,tx_lev_dB);

      //AWGN
      sigma2 = pow(10,sigma2_dB/10);

      //  printf("Sigma2 %f (sigma2_dB %f)\n",sigma2,sigma2_dB);
      for (i=0; i<2*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES; i++) {
        for (aa=0; aa<UE->frame_parms.nb_antennas_rx; aa++) {
          ((short *) UE->common_vars.rxdata[aa])[(2*subframe*UE->frame_parms.samples_per_tti) + 2*i] = (short) (.667*(r_re[aa][i] + sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
          ((short *) UE->common_vars.rxdata[aa])[(2*subframe*UE->frame_parms.samples_per_tti) + 2*i+1] = (short) (.667*(r_im[aa][i] + (iqim*r_re[aa][i]) + sqrt(sigma2/2)*gaussdouble(
                0.0,1.0)));
          /*
          ((short*)UE->common_vars.rxdata[aa])[(2*subframe*UE->frame_parms.samples_per_tti) + 2*i] =
            ((short*)txdata[aa])[(2*subframe*UE->frame_parms.samples_per_tti) + 2*i];
          ((short*)UE->common_vars.rxdata[aa])[(2*subframe*UE->frame_parms.samples_per_tti) + 2*i+1] =
            ((short*)txdata[aa])[(2*subframe*UE->frame_parms.samples_per_tti) + 2*i+1];
          */
        }
      }

      // UE receiver
      for (l=0; l<eNB->frame_parms.symbols_per_tti; l++) {
        //  subframe_offset = (l/eNB->frame_parms.symbols_per_tti)*eNB->frame_parms.samples_per_tti;
        //      printf("subframe_offset = %d\n",subframe_offset);
        slot_fep(UE,
                 l%(eNB->frame_parms.symbols_per_tti/2),
                 (2*subframe)+(l/(eNB->frame_parms.symbols_per_tti/2)),
                 0,
                 0,
                 0);

        if (UE->perfect_ce == 1) {
          if (awgn_flag==0) {
            // fill in perfect channel estimates
            freq_channel(eNB2UE,UE->frame_parms.N_RB_DL,12*UE->frame_parms.N_RB_DL + 1);

            //LOG_M("channel.m","ch",desc1->ch[0],desc1->channel_length,1,8);
            //LOG_M("channelF.m","chF",desc1->chF[0],nb_samples,1,8);
            for(k=0; k<NUMBER_OF_eNB_MAX; k++) {
              for(aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
                for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
                  for (i=0; i<frame_parms->N_RB_DL*12; i++) {
                    ((int16_t *) UE->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[k][(aa<<1)+aarx])[2*i+(l*frame_parms->ofdm_symbol_size+LTE_CE_FILTER_LENGTH)*2]=(int16_t)(
                          eNB2UE->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].x*AMP);
                    ((int16_t *) UE->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[k][(aa<<1)+aarx])[2*i+1+(l*frame_parms->ofdm_symbol_size+LTE_CE_FILTER_LENGTH)*2]=(int16_t)(
                          eNB2UE->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].y*AMP);
                  }
                }
              }
            }
          } else {
            for(aa=0; aa<frame_parms->nb_antenna_ports_eNB; aa++) {
              for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
                for (i=0; i<frame_parms->N_RB_DL*12; i++) {
                  ((int16_t *) UE->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[0][(aa<<1)+aarx])[2*i+(l*frame_parms->ofdm_symbol_size+LTE_CE_FILTER_LENGTH)*2]=(short)(AMP);
                  ((int16_t *) UE->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[0][(aa<<1)+aarx])[2*i+1+(l*frame_parms->ofdm_symbol_size+LTE_CE_FILTER_LENGTH)*2]=0/2;
                }
              }
            }
          }
        }

        if (l==((eNB->frame_parms.Ncp==0)?4:3)) {
          //      LOG_M("H00.m","h00",&(UE->common_vars.dl_ch_estimates[0][0][0]),((frame_parms->Ncp==0)?7:6)*(eNB->frame_parms.ofdm_symbol_size),1,1);
          // do PDCCH procedures here
          UE->pdcch_vars[0][0]->crnti = n_rnti;
          //    printf("Doing RX : num_pdcch_symbols at TX %d\n",num_pdcch_symbols);
          rx_pdcch(UE,
                   trial,
                   subframe,
                   0,
                   (UE->frame_parms.mode1_flag == 1) ? SISO : ALAMOUTI,
                   UE->high_speed_flag,
                   UE->is_secondary_ue);

          if (is_phich_subframe(&UE->frame_parms,subframe)) {
            UE->ulsch[0]->harq_processes[phich_subframe_to_harq_pid(&UE->frame_parms,0,subframe)]->status = ACTIVE;
            //UE->ulsch[0]->harq_processes[phich_subframe_to_harq_pid(&UE->frame_parms,0,subframe)]->Ndi = 1;
            rx_phich(UE,
                     &UE->proc.proc_rxtx[subframe&1],
                     subframe,
                     0);
          }

          //    if (UE->pdcch_vars[0]->num_pdcch_symbols != num_pdcch_symbols)
          //      break;
          dci_cnt = dci_decoding_procedure(UE,
                                           dci_alloc_rx,1,
                                           0,subframe);
          common_rx=0;
          ul_rx=0;
          dl_rx=0;

          if (n_frames==1)  {
            numCCE = get_nCCE(UE->pdcch_vars[0][0]->num_pdcch_symbols, &UE->frame_parms, get_mi(&UE->frame_parms,subframe));

            for (i = 0; i < dci_cnt; i++)
              printf("dci %d: rnti 0x%x, format %d, L %d, nCCE %d/%d dci_length %d\n",i, dci_alloc_rx[i].rnti, dci_alloc_rx[i].format,
                     dci_alloc_rx[i].L, dci_alloc_rx[i].firstCCE, numCCE, dci_alloc_rx[i].dci_length);
          }

          for (i=0; i<dci_cnt; i++) {
            if (dci_alloc_rx[i].rnti == SI_RNTI) {
              if (n_frames==1)
                dump_dci(&UE->frame_parms, &dci_alloc_rx[i]);

              common_rx=1;
            }

            if ((dci_alloc_rx[i].rnti == n_rnti) && (dci_alloc_rx[i].format == format0)) {
              if (n_frames==1)
                dump_dci(&UE->frame_parms, &dci_alloc_rx[i]);

              ul_rx=1;
            }

            if ((dci_alloc_rx[i].rnti == n_rnti) && ((dci_alloc_rx[i].format == format1))) {
              if (n_frames==1)
                dump_dci(&UE->frame_parms, &dci_alloc_rx[i]);

              dl_rx=1;
            }

            if ((dci_alloc_rx[i].rnti != n_rnti) && (dci_alloc_rx[i].rnti != SI_RNTI))
              false_detection_cnt++;
          }

          if (n_frames==1)
            printf("RX DCI Num %d (Common DCI %d, DL DCI %d, UL DCI %d)\n", dci_cnt, common_rx, dl_rx, ul_rx);

          if ((common_rx==0)&&(common_active==1))
            n_errors_common++;

          if ((ul_rx==0)&&(ul_active==1)) {
            n_errors_ul++;
            //     exit(-1);
          }

          if ((dl_rx==0)&&(dl_active==1)) {
            n_errors_dl++;
            //   exit(-1);
          }

          if (UE->pdcch_vars[0][0]->num_pdcch_symbols != num_pdcch_symbols)
            n_errors_cfi++;

          /*
           if (is_phich_subframe(&UE->frame_parms,subframe))
             if (UE->ulsch[0]->harq_processes[phich_subframe_to_harq_pid(&UE->frame_parms, UE->frame, subframe)]->Ndi != phich_ACK)
               n_errors_hi++;
          */

          if (n_errors_cfi > 10)
            break;
        }
      } // symbol loop

      if (n_errors_cfi > 100)
        break;

      if ((n_errors_ul>1000) && (n_errors_dl>1000) && (n_errors_common>1000))
        break;

#ifdef XFORMS
      phy_scope_UE(form_ue,
                   UE,
                   eNb_id,0,subframe);
#endif
    } //trials

    if (common_active) printf("SNR %f : n_errors_common = %d/%u (%e)\n", SNR,n_errors_common,n_trials_common,(double)n_errors_common/n_trials_common);

    if (ul_active==1) printf("SNR %f : n_errors_ul = %d/%u (%e)\n", SNR,n_errors_ul,n_trials_ul,(double)n_errors_ul/n_trials_ul);

    if (dl_active==1) printf("SNR %f : n_errors_dl = %d/%u (%e)\n", SNR,n_errors_dl,n_trials_dl,(double)n_errors_dl/n_trials_dl);

    printf("SNR %f : n_errors_cfi = %d/%d (%e)\n", SNR,n_errors_cfi,trial,(double)n_errors_cfi/trial);
    printf("SNR %f : n_errors_hi = %d/%d (%e)\n", SNR,n_errors_hi,trial,(double)n_errors_hi/trial);
  } // SNR

  if (n_frames==1) {
    LOG_M("txsig0.m","txs0", txdata[0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

    if (n_tx>1)
      LOG_M("txsig1.m","txs1", txdata[1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

    LOG_M("rxsig0.m","rxs0", UE->common_vars.rxdata[0],10*frame_parms->samples_per_tti,1,1);
    LOG_M("rxsigF0.m","rxsF0", UE->common_vars.common_vars_rx_data_per_thread[subframe&0x1].rxdataF[0],NUMBER_OF_OFDM_CARRIERS*2*((frame_parms->Ncp==0)?14:12),2,1);

    if (n_rx>1) {
      LOG_M("rxsig1.m","rxs1", UE->common_vars.rxdata[1],10*frame_parms->samples_per_tti,1,1);
      LOG_M("rxsigF1.m","rxsF1", UE->common_vars.common_vars_rx_data_per_thread[subframe&0x1].rxdataF[1],NUMBER_OF_OFDM_CARRIERS*2*((frame_parms->Ncp==0)?14:12),2,1);
    }

    LOG_M("H00.m","h00",&(UE->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[0][0][0]),((frame_parms->Ncp==0)?7:6)*(eNB->frame_parms.ofdm_symbol_size),1,1);

    if (n_tx==2)
      LOG_M("H10.m","h10",&(UE->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[0][2][0]),((frame_parms->Ncp==0)?7:6)*(eNB->frame_parms.ofdm_symbol_size),1,1);

    LOG_M("pdcch_rxF_ext0.m","pdcch_rxF_ext0",UE->pdcch_vars[0][eNb_id]->rxdataF_ext[0],3*12*UE->frame_parms.N_RB_DL,1,1);
    LOG_M("pdcch_rxF_comp0.m","pdcch0_rxF_comp0",UE->pdcch_vars[0][eNb_id]->rxdataF_comp[0],4*12*UE->frame_parms.N_RB_DL,1,1);
    LOG_M("pdcch_rxF_llr.m","pdcch_llr",UE->pdcch_vars[0][eNb_id]->llr,2400,1,4);
  }

  lte_sync_time_free();

  if ( input_fd != NULL)
    fclose(input_fd);

  return(n_errors_ul);
}



/*
    for (i=1;i<4;i++)
    memcpy((void *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[i*12*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX*2],
    (void *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[0],
    12*OFDM_SYMBOL_SIZE_SAMPLES_NO_PREFIX*2);
*/

