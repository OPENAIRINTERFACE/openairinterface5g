/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

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
  unsigned int* nprbP,
  unsigned int* nCCEP
)
//------------------------------------------------------------------------------
{



  int8_t bcch_sdu_length;
  int mcs = -1;
  void *BCCH_alloc_pdu;
  int CC_id;
  eNB_MAC_INST *eNB = &eNB_mac_inst[module_idP];

  start_meas(&eNB->schedule_si);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

    BCCH_alloc_pdu=(void*)&eNB->common_channels[CC_id].BCCH_alloc_pdu;

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

      if (PHY_vars_eNB_g[module_idP][CC_id]->lte_frame_parms.frame_type == TDD) {
        switch (PHY_vars_eNB_g[module_idP][CC_id]->lte_frame_parms.N_RB_DL) {
        case 6:
          ((DCI1A_1_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->mcs = mcs;
          break;

        case 25:
          ((DCI1A_5MHz_TDD_1_6_t*)BCCH_alloc_pdu)->mcs = mcs;
          break;

        case 50:
          ((DCI1A_10MHz_TDD_1_6_t*)BCCH_alloc_pdu)->mcs = mcs;
          break;

        case 100:
          ((DCI1A_20MHz_TDD_1_6_t*)BCCH_alloc_pdu)->mcs = mcs;
          break;
        }

      } else {
        switch (PHY_vars_eNB_g[module_idP][CC_id]->lte_frame_parms.N_RB_DL) {
        case 6:
          ((DCI1A_1_5MHz_FDD_t*)BCCH_alloc_pdu)->mcs = mcs;
          break;

        case 25:
          ((DCI1A_5MHz_FDD_t*)BCCH_alloc_pdu)->mcs = mcs;
          break;

        case 50:
          ((DCI1A_10MHz_FDD_t*)BCCH_alloc_pdu)->mcs = mcs;
          break;

        case 100:
          ((DCI1A_20MHz_FDD_t*)BCCH_alloc_pdu)->mcs = mcs;
          break;

        }
      }

      if (opt_enabled == 1) {
        trace_pdu(1,
                  &eNB->common_channels[CC_id].BCCH_pdu.payload[0],
                  bcch_sdu_length,
                  0xffff,
                  4,
                  0xffff,
                  eNB->subframe,
                  0,
                  0);
	LOG_D(OPT,"[eNB %d][BCH] Frame %d trace pdu for CC_id %d rnti %x with size %d\n",
	    module_idP, frameP, CC_id, 0xffff, bcch_sdu_length);
      }
      if (PHY_vars_eNB_g[module_idP][CC_id]->lte_frame_parms.frame_type == TDD) {
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

      eNB->common_channels[CC_id].bcch_active=1;
      nprbP[CC_id]=3;
      nCCEP[CC_id]=4;
      eNB->eNB_stats[CC_id].total_num_bcch_pdu+=1;
      eNB->eNB_stats[CC_id].bcch_buffer=bcch_sdu_length;
      eNB->eNB_stats[CC_id].total_bcch_buffer+=bcch_sdu_length;
      eNB->eNB_stats[CC_id].bcch_mcs=mcs;
    } else {
      eNB->common_channels[CC_id].bcch_active=0;
      nprbP[CC_id]=0;
      nCCEP[CC_id]=0;
      //LOG_D(MAC,"[eNB %d] Frame %d : BCCH not active \n",Mod_id,frame);
    }
  }

  // this might be misleading when bcch is inactive
  stop_meas(&eNB->schedule_si);
  return;
}
