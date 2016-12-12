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

/*! \file eNB_scheduler_RA.c
 * \brief primitives used for random access
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email: navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _mac

 */

#include "assertions.h"
#include "platform_types.h"
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "msc.h"

#include "SCHED/defs.h"
#include "SCHED/extern.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"

#include "LAYER2/MAC/proto.h"
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

#include "SIMULATION/TOOLS/defs.h" // for taus

#include "T.h"

void schedule_RA(module_id_t module_idP,frame_t frameP, sub_frame_t subframeP,unsigned char Msg3_subframe)
{

  int CC_id;
  eNB_MAC_INST *eNB = &eNB_mac_inst[module_idP];


  RA_TEMPLATE *RA_template;
  unsigned char i,harq_pid,round;
  int16_t rrc_sdu_length;
  unsigned char lcid,offset;
  module_id_t UE_id= UE_INDEX_INVALID;
  unsigned short TBsize = -1;
  unsigned short msg4_padding,msg4_post_padding,msg4_header;
  uint8_t *vrb_map;
  int first_rb;
  int rballoc[MAX_NUM_CCs];
  DCI_PDU *DCI_pdu;

  start_meas(&eNB->schedule_ra);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {


    vrb_map = eNB->common_channels[CC_id].vrb_map;
    DCI_pdu = &eNB->common_channels[CC_id].DCI_pdu;

    for (i=0; i<NB_RA_PROC_MAX; i++) {

      RA_template = (RA_TEMPLATE *)&eNB->common_channels[CC_id].RA_template[i];

      if (RA_template->RA_active == TRUE) {

        LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d RA %d is active (generate RAR %d, generate_Msg4 %d, wait_ack_Msg4 %d, rnti %x)\n",
              module_idP,CC_id,i,RA_template->generate_rar,RA_template->generate_Msg4,RA_template->wait_ack_Msg4, RA_template->rnti);

        if (RA_template->generate_rar == 1) {

          LOG_D(MAC,"[eNB %d] CC_id %d Frame %d, subframeP %d: Generating RAR DCI (proc %d), RA_active %d format 1A (%d,%d))\n",
                module_idP, CC_id, frameP, subframeP,i,
                RA_template->RA_active,
                RA_template->RA_dci_fmt1,
                RA_template->RA_dci_size_bits1);

          first_rb = 0;
          vrb_map[first_rb] = 1;
          vrb_map[first_rb+1] = 1;
          vrb_map[first_rb+2] = 1;
          vrb_map[first_rb+3] = 1;

          if (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.frame_type == TDD) {
            switch(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL) {
            case 6:
              ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->type=1;
              ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type=0;
              ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->ndi=1;
              ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rv=0;
              ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->mcs=0;
              ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->harq_pid=0;
              ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->TPC=1;
              ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->padding=0;
              ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
              rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type,
                                ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rballoc);
              break;

            case 25:
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->type=1;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type=0;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->ndi=1;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rv=0;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->mcs=0;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->harq_pid=0;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->TPC=1;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->padding=0;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
              rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type,
                                ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rballoc);
              break;

            case 50:
              ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->type=1;
              ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type=0;
              ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->ndi=1;
              ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rv=0;
              ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->mcs=0;
              ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->harq_pid=0;
              ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->TPC=1;
              ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->padding=0;
              ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
              rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type,
                                ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rballoc);
              break;

            case 100:
              ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->type=1;
              ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type=0;
              ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->ndi=1;
              ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rv=0;
              ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->mcs=0;
              ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->harq_pid=0;
              ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->TPC=1;
              ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->padding=0;
              ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
              rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type,
                                ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rballoc);
              break;

            default:
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->type=1;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type=0;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->ndi=1;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rv=0;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->mcs=0;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->harq_pid=0;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->TPC=1;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->padding=0;
              ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
              rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type,
                                ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu1[0])->rballoc);
              break;
            }
          } else {
            switch(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL) {
            case 6:
              ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->type=1;
              ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type=0;
              ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->ndi=1;
              ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->rv=0;
              ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->mcs=0;
              ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->harq_pid=0;
              ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->TPC=1;
              ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->padding=0;
              ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
              rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type,
                                ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->rballoc);
              break;

            case 25:
              ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->type=1;
              ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type=0;
              ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->ndi=1;
              ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->rv=0;
              ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->mcs=0;
              ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->harq_pid=0;
              ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->TPC=1;
              ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->padding=0;
              ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_UL,first_rb,4);
              rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type,
                                ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->rballoc);
              break;

            case 50:
              ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->type=1;
              ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type=0;
              ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->ndi=1;
              ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->rv=0;
              ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->mcs=0;
              ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->harq_pid=0;
              ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->TPC=1;
              ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->padding=0;
              ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
              rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type,
                                ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->rballoc);
              break;

            case 100:
              ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->type=1;
              ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type=0;
              ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->ndi=1;
              ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->rv=0;
              ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->mcs=0;
              ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->harq_pid=0;
              ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->TPC=1;
              ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->padding=0;
              ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
              rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->vrb_type,
                                ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu1[0])->rballoc);
              break;

            default:
              break;
            }
          }

	  if (!CCE_allocation_infeasible(module_idP,CC_id,1,subframeP,2,RA_template->RA_rnti)) {
	    LOG_D(MAC,"Frame %d: Subframe %d : Adding common DCI for RA_RNTI %x\n",
                  frameP,subframeP,RA_template->RA_rnti);
	    add_common_dci(DCI_pdu,
			   (void*)&RA_template->RA_alloc_pdu1[0],
			   RA_template->RA_rnti,
			   RA_template->RA_dci_size_bytes1,
			   2,
			   RA_template->RA_dci_size_bits1,
			   RA_template->RA_dci_fmt1,
			   1);

            /* this will be updated when PHY calls set_msg3_subframe */
	    RA_template->Msg3_subframe = -1;
	  }
        } else if (RA_template->generate_Msg4 == 1) {

          // check for Msg4 Message
          UE_id = find_UE_id(module_idP,RA_template->rnti);

          if (Is_rrc_registered == 1) {

            // Get RRCConnectionSetup for Piggyback
            rrc_sdu_length = mac_rrc_data_req(module_idP,
                                              CC_id,
                                              frameP,
                                              CCCH,
                                              1, // 1 transport block
                                              &eNB->common_channels[CC_id].CCCH_pdu.payload[0],
                                              ENB_FLAG_YES,
                                              module_idP,
                                              0); // not used in this case

            if (rrc_sdu_length == -1) {
              mac_xface->macphy_exit("[MAC][eNB Scheduler] CCCH not allocated\n");
              return; // not reached
            } else {
              //msg("[MAC][eNB %d] Frame %d, subframeP %d: got %d bytes from RRC\n",module_idP,frameP, subframeP,rrc_sdu_length);
            }
          }

          LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: UE_id %d, Is_rrc_registered %d, rrc_sdu_length %d\n",
                module_idP, CC_id, frameP, subframeP,UE_id, Is_rrc_registered,rrc_sdu_length);

          if (rrc_sdu_length>0) {
            LOG_I(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Generating Msg4 with RRC Piggyback (RA proc %d, RNTI %x)\n",
                  module_idP, CC_id, frameP, subframeP,i,RA_template->rnti);

            //msg("[MAC][eNB %d][RAPROC] Frame %d, subframeP %d: Received %d bytes for Msg4: \n",module_idP,frameP,subframeP,rrc_sdu_length);
            //    for (j=0;j<rrc_sdu_length;j++)
            //      msg("%x ",(unsigned char)eNB_mac_inst[module_idP][CC_id].CCCH_pdu.payload[j]);
            //    msg("\n");
            //    msg("[MAC][eNB] Frame %d, subframeP %d: Generated DLSCH (Msg4) DCI, format 1A, for UE %d\n",frameP, subframeP,UE_id);
            // Schedule Reflection of Connection request

	    /*
	    // randomize frequency allocation for RA
	    while (1) {
	      first_rb = (unsigned char)(taus()%(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL-4));
	      
	      if ((vrb_map[first_rb] != 1) && (vrb_map[first_rb+3] != 1))
		break;
	    }
	    */
	    first_rb=0;

	    vrb_map[first_rb] = 1;
	    vrb_map[first_rb+1] = 1;
	    vrb_map[first_rb+2] = 1;
	    vrb_map[first_rb+3] = 1;


            // Compute MCS for 3 PRB
            msg4_header = 1+6+1;  // CR header, CR CE, SDU header

            if (mac_xface->frame_parms->frame_type == TDD) {

              switch (mac_xface->frame_parms->N_RB_DL) {
              case 6:
                ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->ndi=1;

                if ((rrc_sdu_length+msg4_header) <= 22) {
                  ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=4;
                  TBsize = 22;
                } else if ((rrc_sdu_length+msg4_header) <= 28) {
                  ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=5;
                  TBsize = 28;
                } else if ((rrc_sdu_length+msg4_header) <= 32) {
                  ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=6;
                  TBsize = 32;
                } else if ((rrc_sdu_length+msg4_header) <= 41) {
                  ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=7;
                  TBsize = 41;
                } else if ((rrc_sdu_length+msg4_header) <= 49) {
                  ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=8;
                  TBsize = 49;
                } else if ((rrc_sdu_length+msg4_header) <= 57) {
                  ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=9;
                  TBsize = 57;
                }
		((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->type=1;
		((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type=0;
		((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->rv=0;
		((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->harq_pid=0;
		((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->TPC=1;
		((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->padding=0;
		((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->rballoc= mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
		rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type,
							 ((DCI1A_1_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->rballoc);
		
                break;

              case 25:

                ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->ndi=1;

                if ((rrc_sdu_length+msg4_header) <= 22) {
                  ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=4;
                  TBsize = 22;
                } else if ((rrc_sdu_length+msg4_header) <= 28) {
                  ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=5;
                  TBsize = 28;
                } else if ((rrc_sdu_length+msg4_header) <= 32) {
                  ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=6;
                  TBsize = 32;
                } else if ((rrc_sdu_length+msg4_header) <= 41) {
                  ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=7;
                  TBsize = 41;
                } else if ((rrc_sdu_length+msg4_header) <= 49) {
                  ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=8;
                  TBsize = 49;
                } else if ((rrc_sdu_length+msg4_header) <= 57) {
                  ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=9;
                  TBsize = 57;
                }
		
		((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->type=1;
		((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type=0;
		((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->rv=0;
		((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->harq_pid=0;
		((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->TPC=1;
		((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->padding=0;
		((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->rballoc= mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
		rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type,
							 ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->rballoc);
                break;

              case 50:

                ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->ndi=1;

                if ((rrc_sdu_length+msg4_header) <= 22) {
                  ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=4;
                  TBsize = 22;
                } else if ((rrc_sdu_length+msg4_header) <= 28) {
                  ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=5;
                  TBsize = 28;
                } else if ((rrc_sdu_length+msg4_header) <= 32) {
                  ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=6;
                  TBsize = 32;
                } else if ((rrc_sdu_length+msg4_header) <= 41) {
                  ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=7;
                  TBsize = 41;
                } else if ((rrc_sdu_length+msg4_header) <= 49) {
                  ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=8;
                  TBsize = 49;
                } else if ((rrc_sdu_length+msg4_header) <= 57) {
                  ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=9;
                  TBsize = 57;
                }
		((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->type=1;
		((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type=0;
		((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->rv=0;
		((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->harq_pid=0;
		((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->TPC=1;
		((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->padding=0;
		((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->rballoc= mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
		rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type,
							 ((DCI1A_10MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->rballoc);

                break;

              case 100:

                ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->ndi=1;

                if ((rrc_sdu_length+msg4_header) <= 22) {
                  ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=4;
                  TBsize = 22;
                } else if ((rrc_sdu_length+msg4_header) <= 28) {
                  ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=5;
                  TBsize = 28;
                } else if ((rrc_sdu_length+msg4_header) <= 32) {
                  ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=6;
                  TBsize = 32;
                } else if ((rrc_sdu_length+msg4_header) <= 41) {
                  ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=7;
                  TBsize = 41;
                } else if ((rrc_sdu_length+msg4_header) <= 49) {
                  ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=8;
                  TBsize = 49;
                } else if ((rrc_sdu_length+msg4_header) <= 57) {
                  ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->mcs=9;
                  TBsize = 57;
                }

		((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->type=1;
		((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type=0;
		((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->rv=0;
		((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->harq_pid=0;
		((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->TPC=1;
		((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->rballoc= mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
		rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type,
							 ((DCI1A_20MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->rballoc);
                break;
              }
            } else { // FDD DCI
              switch (mac_xface->frame_parms->N_RB_DL) {
              case 6:
                ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->ndi=1;

                if ((rrc_sdu_length+msg4_header) <= 22) {
                  ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=4;
                  TBsize = 22;
                } else if ((rrc_sdu_length+msg4_header) <= 28) {
                  ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=5;
                  TBsize = 28;
                } else if ((rrc_sdu_length+msg4_header) <= 32) {
                  ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=6;
                  TBsize = 32;
                } else if ((rrc_sdu_length+msg4_header) <= 41) {
                  ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=7;
                  TBsize = 41;
                } else if ((rrc_sdu_length+msg4_header) <= 49) {
                  ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=8;
                  TBsize = 49;
                } else if ((rrc_sdu_length+msg4_header) <= 57) {
                  ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=9;
                  TBsize = 57;
                }

		((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->type=1;
		((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type=0;
		((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->rv=0;
		((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->harq_pid=0;
		((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->TPC=1;
		((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->padding=0;
		((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->rballoc= mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
		rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type,
							 ((DCI1A_1_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->rballoc);
                break;

              case 25:
                ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->ndi=1;

                if ((rrc_sdu_length+msg4_header) <= 22) {
                  ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=4;
                  TBsize = 22;
                } else if ((rrc_sdu_length+msg4_header) <= 28) {
                  ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=5;
                  TBsize = 28;
                } else if ((rrc_sdu_length+msg4_header) <= 32) {
                  ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=6;
                  TBsize = 32;
                } else if ((rrc_sdu_length+msg4_header) <= 41) {
                  ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=7;
                  TBsize = 41;
                } else if ((rrc_sdu_length+msg4_header) <= 49) {
                  ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=8;
                  TBsize = 49;
                } else if ((rrc_sdu_length+msg4_header) <= 57) {
                  ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=9;
                  TBsize = 57;
                }

		((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->type=1;
		((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type=0;
		((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->rv=0;
		((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->harq_pid=0;
		((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->TPC=1;
		((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->padding=0;
		((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->rballoc= mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
		rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type,
							 ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->rballoc);
                break;

              case 50:
                ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->ndi=1;

                if ((rrc_sdu_length+msg4_header) <= 22) {
                  ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=4;
                  TBsize = 22;
                } else if ((rrc_sdu_length+msg4_header) <= 28) {
                  ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=5;
                  TBsize = 28;
                } else if ((rrc_sdu_length+msg4_header) <= 32) {
                  ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=6;
                  TBsize = 32;
                } else if ((rrc_sdu_length+msg4_header) <= 41) {
                  ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=7;
                  TBsize = 41;
                } else if ((rrc_sdu_length+msg4_header) <= 49) {
                  ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=8;
                  TBsize = 49;
                } else if ((rrc_sdu_length+msg4_header) <= 57) {
                  ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=9;
                  TBsize = 57;
                }

		((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->type=1;
		((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type=0;
		((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->rv=0;
		((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->harq_pid=0;
		((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->TPC=1;
		((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->padding=0;
		((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->rballoc= mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
		rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type,
							 ((DCI1A_10MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->rballoc);
                break;

              case 100:
                ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->ndi=1;

                if ((rrc_sdu_length+msg4_header) <= 22) {
                  ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=4;
                  TBsize = 22;
                } else if ((rrc_sdu_length+msg4_header) <= 28) {
                  ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=5;
                  TBsize = 28;
                } else if ((rrc_sdu_length+msg4_header) <= 32) {
                  ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=6;
                  TBsize = 32;
                } else if ((rrc_sdu_length+msg4_header) <= 41) {
                  ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=7;
                  TBsize = 41;
                } else if ((rrc_sdu_length+msg4_header) <= 49) {
                  ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=8;
                  TBsize = 49;
                } else if ((rrc_sdu_length+msg4_header) <= 57) {
                  ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->mcs=9;
                  TBsize = 57;
                }
		((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->type=1;
		((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type=0;
		((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->rv=0;
		((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->harq_pid=0;
		((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->TPC=1;
		((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->padding=0;
		((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->rballoc= mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL,first_rb,4);
		rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type,
							 ((DCI1A_20MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->rballoc);
                break;
              }
            }

	    if (!CCE_allocation_infeasible(module_idP,CC_id,0,subframeP,2,RA_template->rnti)) {
	      add_ue_spec_dci(DCI_pdu,
			      (void*)&RA_template->RA_alloc_pdu2[0],
			      RA_template->rnti,
			      RA_template->RA_dci_size_bytes2,
			      2,
			      RA_template->RA_dci_size_bits2,
			      RA_template->RA_dci_fmt2,
			      0);
	      
	      RA_template->generate_Msg4=0;
	      RA_template->wait_ack_Msg4=1;
	      RA_template->RA_active = FALSE;
	      lcid=0;
	      
	      if ((TBsize - rrc_sdu_length - msg4_header) <= 2) {
		msg4_padding = TBsize - rrc_sdu_length - msg4_header;
		msg4_post_padding = 0;
	      } else {
		msg4_padding = 0;
		msg4_post_padding = TBsize - rrc_sdu_length - msg4_header -1;
	      }
	      
	      LOG_I(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d subframeP %d Msg4 : TBS %d, sdu_len %d, msg4_header %d, msg4_padding %d, msg4_post_padding %d\n",
		    module_idP,CC_id,frameP,subframeP,TBsize,rrc_sdu_length,msg4_header,msg4_padding,msg4_post_padding);
	      DevAssert( UE_id != UE_INDEX_INVALID ); // FIXME not sure how to gracefully return
	      offset = generate_dlsch_header((unsigned char*)eNB->UE_list.DLSCH_pdu[CC_id][0][(unsigned char)UE_id].payload[0],
					     1,                           //num_sdus
					     (unsigned short*)&rrc_sdu_length,             //
					     &lcid,                       // sdu_lcid
					     255,                         // no drx
					     0,                           // no timing advance
					     RA_template->cont_res_id,  // contention res id
					     msg4_padding,                // no padding
					     msg4_post_padding);
	      
	      memcpy((void*)&eNB->UE_list.DLSCH_pdu[CC_id][0][(unsigned char)UE_id].payload[0][(unsigned char)offset],
		     &eNB->common_channels[CC_id].CCCH_pdu.payload[0],
		     rrc_sdu_length);
	      
              T(T_ENB_MAC_UE_DL_PDU_WITH_DATA, T_INT(module_idP), T_INT(CC_id), T_INT(RA_template->rnti), T_INT(frameP), T_INT(subframeP),
                T_INT(0 /*harq_pid always 0?*/), T_BUFFER(&eNB->UE_list.DLSCH_pdu[CC_id][0][UE_id].payload[0], TBsize));

	      if (opt_enabled==1) {
		trace_pdu(1, (uint8_t *)eNB->UE_list.DLSCH_pdu[CC_id][0][(unsigned char)UE_id].payload[0],
			  rrc_sdu_length, UE_id, 3, UE_RNTI(module_idP, UE_id),
			  eNB->frame, eNB->subframe,0,0);
		LOG_D(OPT,"[eNB %d][DLSCH] CC_id %d Frame %d trace pdu for rnti %x with size %d\n",
		      module_idP, CC_id, frameP, UE_RNTI(module_idP,UE_id), rrc_sdu_length);
	      }
	      
	    }
	  }

          //try here
        }

      } else if (RA_template->wait_ack_Msg4==1) {
	// check HARQ status and retransmit if necessary
	LOG_I(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Checking if Msg4 was acknowledged: \n",
	      module_idP,CC_id,frameP,subframeP);
	// Get candidate harq_pid from PHY
	mac_xface->get_ue_active_harq_pid(module_idP,CC_id,RA_template->rnti,frameP,subframeP,&harq_pid,&round,openair_harq_RA);
	
	if (round>0) {
	  //RA_template->wait_ack_Msg4++;
	  // we have to schedule a retransmission
	  if (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.frame_type == TDD) {
	    ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->ndi=1;
	  } else {
	    ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->ndi=1;
	  }
	  
	  /*
	  // randomize frequency allocation for RA
	  while (1) {
	    first_rb = (unsigned char)(taus()%(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL-4));
	    
	    if ((vrb_map[first_rb] != 1) && (vrb_map[first_rb+3] != 1))
	      break;
	  }
	  */
	  first_rb=0;
	  vrb_map[first_rb] = 1;
	  vrb_map[first_rb+1] = 1;
	  vrb_map[first_rb+2] = 1;
	  vrb_map[first_rb+3] = 1;
	  
	  if (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.frame_type == TDD) {
	    ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_UL,first_rb,4);
	    rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type,
						     ((DCI1A_5MHz_TDD_1_6_t*)&RA_template->RA_alloc_pdu2[0])->rballoc);
	  } else {
	    ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->rballoc = mac_xface->computeRIV(PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_UL,first_rb,4);
	    rballoc[CC_id] |= mac_xface->get_rballoc(((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->vrb_type,
						     ((DCI1A_5MHz_FDD_t*)&RA_template->RA_alloc_pdu2[0])->rballoc);
	  }
	  
	  if (!CCE_allocation_infeasible(module_idP,CC_id,0,subframeP,2,RA_template->rnti)) {
	    add_ue_spec_dci(DCI_pdu,
			    (void*)&RA_template->RA_alloc_pdu2[0],
			    RA_template->rnti,
			    RA_template->RA_dci_size_bytes2,
			    2,
			    RA_template->RA_dci_size_bits2,
			    RA_template->RA_dci_fmt2,
			    0);
	  }
	  LOG_W(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Msg4 not acknowledged, adding ue specific dci (rnti %x) for RA (Msg4 Retransmission)\n",
		module_idP,CC_id,frameP,subframeP,RA_template->rnti);
	} else {
	  /*      msg4 not received
		  if ((round == 0) && (RA_template->wait_ack_Msg4>1){
		  remove UE instance across all the layers: mac_xface->cancel_RA();
		  }
	  */
	  LOG_I(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d : Msg4 acknowledged\n",module_idP,CC_id,frameP,subframeP);
	  RA_template->wait_ack_Msg4=0;
	  RA_template->RA_active=FALSE;
	  UE_id = find_UE_id(module_idP,RA_template->rnti);
	  DevAssert( UE_id != -1 );
	  eNB_mac_inst[module_idP].UE_list.UE_template[UE_PCCID(module_idP,UE_id)][UE_id].configured=TRUE;
	  
	}
      }
    } // for i=0 .. N_RA_PROC-1 
  } // CC_id

  stop_meas(&eNB->schedule_ra);
}

void initiate_ra_proc(module_id_t module_idP, int CC_id,frame_t frameP, uint16_t preamble_index,int16_t timing_offset,uint8_t sect_id,sub_frame_t subframeP,
                      uint8_t f_id)
{

  uint8_t i;
  RA_TEMPLATE *RA_template = (RA_TEMPLATE *)&eNB_mac_inst[module_idP].common_channels[CC_id].RA_template[0];

  LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d Initiating RA procedure for preamble index %d\n",module_idP,CC_id,frameP,preamble_index);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC,1);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC,0);

  for (i=0; i<NB_RA_PROC_MAX; i++) {
    if (RA_template[i].RA_active==FALSE) {
      RA_template[i].RA_active=TRUE;
      RA_template[i].generate_rar=1;
      RA_template[i].generate_Msg4=0;
      RA_template[i].wait_ack_Msg4=0;
      RA_template[i].timing_offset=timing_offset;
      // Put in random rnti (to be replaced with proper procedure!!)
      RA_template[i].rnti = taus();
      RA_template[i].RA_rnti = 1+subframeP+(10*f_id);
      RA_template[i].preamble_index = preamble_index;
      LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d Activating RAR generation for process %d, rnti %x, RA_active %d\n",
            module_idP,CC_id,frameP,i,RA_template[i].rnti,
            RA_template[i].RA_active);

      return;
    }
  }
}

void cancel_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP, rnti_t rnti)
{
  unsigned char i;
  RA_TEMPLATE *RA_template = (RA_TEMPLATE *)&eNB_mac_inst[module_idP].common_channels[CC_id].RA_template[0];

  MSC_LOG_EVENT(MSC_PHY_ENB, "RA Cancelling procedure ue %"PRIx16" ", rnti);
  LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d Cancelling RA procedure for UE rnti %x\n",module_idP,CC_id,frameP,rnti);

  for (i=0; i<NB_RA_PROC_MAX; i++) {
    if (rnti == RA_template[i].rnti) {
      RA_template[i].RA_active=FALSE;
      RA_template[i].generate_rar=0;
      RA_template[i].generate_Msg4=0;
      RA_template[i].wait_ack_Msg4=0;
      RA_template[i].timing_offset=0;
      RA_template[i].RRC_timer=20;
      RA_template[i].rnti = 0;
    }
  }
}

