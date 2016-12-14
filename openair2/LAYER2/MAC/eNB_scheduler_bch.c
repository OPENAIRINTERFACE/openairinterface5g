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

/*! \file eNB_scheduler_bch.c
 * \brief procedures related to eNB for the BCH transport channel
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email: navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _mac

 */

#include "assertions.h"
#include "PHY/defs.h"
#include "PHY/extern.h"

#include "SCHED/defs.h"
#include "SCHED/extern.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/proto.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/LITE/extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1


//------------------------------------------------------------------------------
void
schedule_SI(
  module_id_t   module_idP,
  frame_t       frameP,
  sub_frame_t   subframeP)

//------------------------------------------------------------------------------
{



  int8_t bcch_sdu_length;
  int mcs = -1;
  void *BCCH_alloc_pdu;
  int CC_id;
  eNB_MAC_INST *eNB = &eNB_mac_inst[module_idP];
  uint8_t *vrb_map;
  int first_rb = -1;
  int rballoc[MAX_NUM_CCs];
  int sizeof1A_bytes,sizeof1A_bits = -1;
  DCI_PDU *DCI_pdu;

  start_meas(&eNB->schedule_si);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    
    BCCH_alloc_pdu  = (void*)&eNB->common_channels[CC_id].BCCH_alloc_pdu;
    DCI_pdu         = (void*)&eNB->common_channels[CC_id].DCI_pdu;
    vrb_map         = (void*)&eNB->common_channels[CC_id].vrb_map;

    bcch_sdu_length = mac_rrc_data_req(module_idP,
                                       CC_id,
                                       frameP,
                                       BCCH,1,
                                       &eNB->common_channels[CC_id].BCCH_pdu.payload[0],
                                       1,
                                       module_idP,
                                       0); // not used in this case

    if (bcch_sdu_length > 0) {
      LOG_D(MAC,"[eNB %d] Frame %d : BCCH->DLSCH CC_id %d, Received %d bytes \n",module_idP,frameP,CC_id,bcch_sdu_length);

      // Allocate 4 PRBs in a random location
      /*
      while (1) {
	first_rb = (unsigned char)(taus()%(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL-4));
	if ((vrb_map[first_rb] != 1) && 
	    (vrb_map[first_rb+1] != 1) && 
	    (vrb_map[first_rb+2] != 1) && 
	    (vrb_map[first_rb+3] != 1))
	  break;
      }
      */
      switch (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL) {
      case 6:
	first_rb = 0;
	break;
      case 15:
	first_rb = 6;
	break;
      case 25:
	first_rb = 11;
	break;
      case 50:
	first_rb = 23;
	break;
      case 100:
	first_rb = 48;
	break;
      }

      vrb_map[first_rb] = 1;
      vrb_map[first_rb+1] = 1;
      vrb_map[first_rb+2] = 1;
      vrb_map[first_rb+3] = 1;

      // Get MCS for length of SI
      if (bcch_sdu_length <= (mac_xface->get_TBS_DL(0,3))) {
        mcs=0;
      } else if (bcch_sdu_length <= (mac_xface->get_TBS_DL(1,3))) {
        mcs=1;
      } else if (bcch_sdu_length <= (mac_xface->get_TBS_DL(2,3))) {
        mcs=2;
      } else if (bcch_sdu_length <= (mac_xface->get_TBS_DL(3,3))) {
        mcs=3;
      } else if (bcch_sdu_length <= (mac_xface->get_TBS_DL(4,3))) {
        mcs=4;
      } else if (bcch_sdu_length <= (mac_xface->get_TBS_DL(5,3))) {
        mcs=5;
      } else if (bcch_sdu_length <= (mac_xface->get_TBS_DL(6,3))) {
        mcs=6;
      } else if (bcch_sdu_length <= (mac_xface->get_TBS_DL(7,3))) {
        mcs=7;
      } else if (bcch_sdu_length <= (mac_xface->get_TBS_DL(8,3))) {
        mcs=8;
      }



      if (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.frame_type == TDD) {
        switch (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL) {
        case 6:
          ((DCI1A_1_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->mcs = mcs;
          ((DCI1A_1_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
          ((DCI1A_1_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->type = 1;
          ((DCI1A_1_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->vrb_type = 0;
          ((DCI1A_1_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->ndi = 1;
          ((DCI1A_1_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->rv = 1;
          ((DCI1A_1_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->harq_pid = 0;
          ((DCI1A_1_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->TPC = 1;
          ((DCI1A_1_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->padding = 0;
          rballoc[CC_id] |= mac_xface->get_rballoc(0,((DCI1A_1_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->rballoc);
	  sizeof1A_bytes = sizeof(DCI1A_1_5MHz_TDD_1_6_t);
	  sizeof1A_bits = sizeof_DCI1A_1_5MHz_TDD_1_6_t;
          break;

        case 25:
          ((DCI1A_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->mcs = mcs;
          ((DCI1A_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
          ((DCI1A_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->type = 1;
          ((DCI1A_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->vrb_type = 0;
          ((DCI1A_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->ndi = 1;
          ((DCI1A_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->rv = 1;
          ((DCI1A_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->harq_pid = 0;
          ((DCI1A_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->TPC = 1;
          ((DCI1A_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->padding = 0;
          rballoc[CC_id] |= mac_xface->get_rballoc(0,((DCI1A_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->rballoc);
	  sizeof1A_bytes = sizeof(DCI1A_5MHz_TDD_1_6_t);
	  sizeof1A_bits = sizeof_DCI1A_5MHz_TDD_1_6_t;
	  break;

        case 50:
          ((DCI1A_10MHz_TDD_1_6_t*)BCCH_alloc_pdu)->mcs = mcs;
          ((DCI1A_10MHz_TDD_1_6_t*)BCCH_alloc_pdu)->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
          ((DCI1A_10MHz_TDD_1_6_t*)BCCH_alloc_pdu)->type = 1;
          ((DCI1A_10MHz_TDD_1_6_t*)BCCH_alloc_pdu)->vrb_type = 0;
          ((DCI1A_10MHz_TDD_1_6_t*)BCCH_alloc_pdu)->ndi = 1;
          ((DCI1A_10MHz_TDD_1_6_t*)BCCH_alloc_pdu)->rv = 1;
          ((DCI1A_10MHz_TDD_1_6_t*)BCCH_alloc_pdu)->harq_pid = 0;
          ((DCI1A_10MHz_TDD_1_6_t*)BCCH_alloc_pdu)->TPC = 1;
          ((DCI1A_10MHz_TDD_1_6_t*)BCCH_alloc_pdu)->padding = 0;
          rballoc[CC_id] |= mac_xface->get_rballoc(0,((DCI1A_10MHz_TDD_1_6_t*)BCCH_alloc_pdu)->rballoc);
	  sizeof1A_bytes = sizeof(DCI1A_10MHz_TDD_1_6_t);
	  sizeof1A_bits = sizeof_DCI1A_10MHz_TDD_1_6_t;
          break;

        case 100:
          ((DCI1A_20MHz_TDD_1_6_t*)BCCH_alloc_pdu)->mcs = mcs;
          ((DCI1A_20MHz_TDD_1_6_t*)BCCH_alloc_pdu)->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
          ((DCI1A_20MHz_TDD_1_6_t*)BCCH_alloc_pdu)->type = 1;
          ((DCI1A_20MHz_TDD_1_6_t*)BCCH_alloc_pdu)->vrb_type = 0;
          ((DCI1A_20MHz_TDD_1_6_t*)BCCH_alloc_pdu)->ndi = 1;
          ((DCI1A_20MHz_TDD_1_6_t*)BCCH_alloc_pdu)->rv = 1;
          ((DCI1A_20MHz_TDD_1_6_t*)BCCH_alloc_pdu)->harq_pid = 0;
          ((DCI1A_20MHz_TDD_1_6_t*)BCCH_alloc_pdu)->TPC = 1;
          ((DCI1A_20MHz_TDD_1_6_t*)BCCH_alloc_pdu)->padding = 0;
          rballoc[CC_id] |= mac_xface->get_rballoc(0,((DCI1A_20MHz_TDD_1_6_t*)BCCH_alloc_pdu)->rballoc);
	  sizeof1A_bytes = sizeof(DCI1A_20MHz_TDD_1_6_t);
	  sizeof1A_bits = sizeof_DCI1A_20MHz_TDD_1_6_t; 
         break;
        }

      } else {
        switch (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL) {
        case 6:
          ((DCI1A_1_5MHz_FDD_t*)BCCH_alloc_pdu)->mcs = mcs;
          ((DCI1A_1_5MHz_FDD_t*)BCCH_alloc_pdu)->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
          ((DCI1A_1_5MHz_FDD_t*)BCCH_alloc_pdu)->type = 1;
          ((DCI1A_1_5MHz_FDD_t*)BCCH_alloc_pdu)->vrb_type = 0;
          ((DCI1A_1_5MHz_FDD_t*)BCCH_alloc_pdu)->ndi = 1;
          ((DCI1A_1_5MHz_FDD_t*)BCCH_alloc_pdu)->rv = 1;
          ((DCI1A_1_5MHz_FDD_t*)BCCH_alloc_pdu)->harq_pid = 0;
          ((DCI1A_1_5MHz_FDD_t*)BCCH_alloc_pdu)->TPC = 1;
          ((DCI1A_1_5MHz_FDD_t*)BCCH_alloc_pdu)->padding = 0;

          rballoc[CC_id] |= mac_xface->get_rballoc(0,((DCI1A_1_5MHz_FDD_t*)BCCH_alloc_pdu)->rballoc);
	  sizeof1A_bytes = sizeof(DCI1A_1_5MHz_FDD_t);
	  sizeof1A_bits = sizeof_DCI1A_1_5MHz_FDD_t;
          break;

        case 25:
          ((DCI1A_5MHz_FDD_t*)BCCH_alloc_pdu)->mcs = mcs;
          ((DCI1A_5MHz_FDD_t*)BCCH_alloc_pdu)->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
          ((DCI1A_5MHz_FDD_t*)BCCH_alloc_pdu)->type = 1;
          ((DCI1A_5MHz_FDD_t*)BCCH_alloc_pdu)->vrb_type = 0;
          ((DCI1A_5MHz_FDD_t*)BCCH_alloc_pdu)->ndi = 1;
          ((DCI1A_5MHz_FDD_t*)BCCH_alloc_pdu)->rv = 1;
          ((DCI1A_5MHz_FDD_t*)BCCH_alloc_pdu)->harq_pid = 0;
          ((DCI1A_5MHz_FDD_t*)BCCH_alloc_pdu)->TPC = 1;
          ((DCI1A_5MHz_FDD_t*)BCCH_alloc_pdu)->padding = 0;

          rballoc[CC_id] |= mac_xface->get_rballoc(0,((DCI1A_5MHz_FDD_t*)BCCH_alloc_pdu)->rballoc);
	  sizeof1A_bytes = sizeof(DCI1A_5MHz_FDD_t);
	  sizeof1A_bits = sizeof_DCI1A_5MHz_FDD_t;
          break;

        case 50:
          ((DCI1A_10MHz_FDD_t*)BCCH_alloc_pdu)->mcs = mcs;
          ((DCI1A_10MHz_FDD_t*)BCCH_alloc_pdu)->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
          ((DCI1A_10MHz_FDD_t*)BCCH_alloc_pdu)->type = 1;
          ((DCI1A_10MHz_FDD_t*)BCCH_alloc_pdu)->vrb_type = 0;
          ((DCI1A_10MHz_FDD_t*)BCCH_alloc_pdu)->ndi = 1;
          ((DCI1A_10MHz_FDD_t*)BCCH_alloc_pdu)->rv = 1;
          ((DCI1A_10MHz_FDD_t*)BCCH_alloc_pdu)->harq_pid = 0;
          ((DCI1A_10MHz_FDD_t*)BCCH_alloc_pdu)->TPC = 1;
          ((DCI1A_10MHz_FDD_t*)BCCH_alloc_pdu)->padding = 0;

          rballoc[CC_id] |= mac_xface->get_rballoc(0,((DCI1A_10MHz_FDD_t*)BCCH_alloc_pdu)->rballoc);
	  sizeof1A_bytes = sizeof(DCI1A_10MHz_FDD_t);
	  sizeof1A_bits = sizeof_DCI1A_10MHz_FDD_t;
          break;

        case 100:
          ((DCI1A_20MHz_FDD_t*)BCCH_alloc_pdu)->mcs = mcs;
          ((DCI1A_20MHz_FDD_t*)BCCH_alloc_pdu)->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
          ((DCI1A_20MHz_FDD_t*)BCCH_alloc_pdu)->type = 1;
          ((DCI1A_20MHz_FDD_t*)BCCH_alloc_pdu)->vrb_type = 0;
          ((DCI1A_20MHz_FDD_t*)BCCH_alloc_pdu)->ndi = 1;
          ((DCI1A_20MHz_FDD_t*)BCCH_alloc_pdu)->rv = 1;
          ((DCI1A_20MHz_FDD_t*)BCCH_alloc_pdu)->harq_pid = 0;
          ((DCI1A_20MHz_FDD_t*)BCCH_alloc_pdu)->TPC = 1;
          ((DCI1A_20MHz_FDD_t*)BCCH_alloc_pdu)->padding = 0;

          rballoc[CC_id] |= mac_xface->get_rballoc(0,((DCI1A_20MHz_FDD_t*)BCCH_alloc_pdu)->rballoc);
 	  sizeof1A_bytes = sizeof(DCI1A_20MHz_FDD_t);
	  sizeof1A_bits = sizeof_DCI1A_20MHz_FDD_t;
	  break;

        }
      }

      if (!CCE_allocation_infeasible(module_idP,CC_id,1,subframeP,2,SI_RNTI)) {
	add_common_dci(DCI_pdu,
		       BCCH_alloc_pdu,
		       SI_RNTI,
		       sizeof1A_bytes,
		       2,
		       sizeof1A_bits,
		       format1A,0);
      }
      else {
	LOG_E(MAC,"[eNB %d] CCid %d Frame %d, subframe %d : Cannot add DCI 1A for SI\n",module_idP, CC_id,frameP,subframeP);
      }

      if (opt_enabled == 1) {
        trace_pdu(1,
                  &eNB->common_channels[CC_id].BCCH_pdu.payload[0],
                  bcch_sdu_length,
                  0xffff,
                  4,
                  0xffff,
                  eNB->frame,
                  eNB->subframe,
                  0,
                  0);
	LOG_D(OPT,"[eNB %d][BCH] Frame %d trace pdu for CC_id %d rnti %x with size %d\n",
	    module_idP, frameP, CC_id, 0xffff, bcch_sdu_length);
      }
      if (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.frame_type == TDD) {
        LOG_D(MAC,"[eNB] Frame %d : Scheduling BCCH->DLSCH (TDD) for CC_id %d SI %d bytes (mcs %d, rb 3, TBS %d)\n",
              frameP,
              CC_id,
              bcch_sdu_length,
              mcs,
              mac_xface->get_TBS_DL(mcs,3));
      } else {
        LOG_D(MAC,"[eNB] Frame %d : Scheduling BCCH->DLSCH (FDD) for CC_id %d SI %d bytes (mcs %d, rb 3, TBS %d)\n",
              frameP,
              CC_id,
              bcch_sdu_length,
              mcs,
              mac_xface->get_TBS_DL(mcs,3));
      }


      eNB->eNB_stats[CC_id].total_num_bcch_pdu+=1;
      eNB->eNB_stats[CC_id].bcch_buffer=bcch_sdu_length;
      eNB->eNB_stats[CC_id].total_bcch_buffer+=bcch_sdu_length;
      eNB->eNB_stats[CC_id].bcch_mcs=mcs;
    } else {

      //LOG_D(MAC,"[eNB %d] Frame %d : BCCH not active \n",Mod_id,frame);
    }
  }

  // this might be misleading when bcch is inactive
  stop_meas(&eNB->schedule_si);
  return;
}
