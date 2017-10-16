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

/*! \file eNB_scheduler_dlsch.c
 * \brief procedures related to eNB for the DLSCH transport channel
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

#include "SIMULATION/TOOLS/defs.h" // for taus

#include "T.h"

extern RAN_CONTEXT_t RC;

//------------------------------------------------------------------------------
void
schedule_ue_spec_phy_test(
  module_id_t   module_idP,
  frame_t       frameP,
  sub_frame_t   subframeP,
  int*          mbsfn_flag
)
//------------------------------------------------------------------------------
{
  uint8_t                        CC_id;
  int                            UE_id=0;
  uint16_t                       N_RB_DL;
  uint16_t                       TBS;
  uint16_t                       nb_rb;

  unsigned char                  harq_pid  = subframeP%5;
  uint16_t                       rnti      = 0x1235;
  uint32_t                       rb_alloc  = 0x1FFF;
  int32_t                        tpc       = 1;
  int32_t                        mcs       = 0;
  int32_t                        cqi       = 15;
  int32_t                        ndi       = subframeP/5;
  int32_t                        dai       = 0;

  eNB_MAC_INST                   *eNB      = RC.mac[module_idP];
  COMMON_channels_t              *cc       = eNB->common_channels;
  nfapi_dl_config_request_body_t *dl_req;
  nfapi_dl_config_request_pdu_t  *dl_config_pdu;

  N_RB_DL         = to_prb(cc->mib->message.dl_Bandwidth);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC, "doing schedule_ue_spec for CC_id %d\n",CC_id);

    dl_req        = &eNB->DL_req[CC_id].dl_config_request_body;

    if (mbsfn_flag[CC_id]>0)
      continue;

    nb_rb = conv_nprb(0,rb_alloc,N_RB_DL);
    TBS = get_TBS_DL(mcs,nb_rb);

    dl_config_pdu                                                         = &dl_req->dl_config_pdu_list[dl_req->number_pdu]; 
    memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
    dl_config_pdu->pdu_type                                               = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE; 
    dl_config_pdu->pdu_size                                               = (uint8_t)(2+sizeof(nfapi_dl_config_dci_dl_pdu));
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format                  = NFAPI_DL_DCI_FORMAT_1;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level           = get_aggregation(get_bw_index(module_idP,CC_id),cqi,format1);
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                        = rnti;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type                   = 1;    // CRNTI : see Table 4-10 from SCF082 - nFAPI specifications
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power          = 6000; // equal to RS power
    
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process                = harq_pid;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc                         = tpc; // dont adjust power when retransmitting
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1        = ndi;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = mcs;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1        = 0;
    //deactivate second codeword
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_2                       = 0;
    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_2        = 1;
    
    if (cc[CC_id].tdd_Config != NULL) { //TDD
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.downlink_assignment_index = dai;
      LOG_D(MAC,"[eNB %d] Initial transmission CC_id %d : harq_pid %d, dai %d, mcs %d\n",
	    module_idP,CC_id,harq_pid,dai,mcs);
    } else {
      LOG_D(MAC,"[eNB %d] Initial transmission CC_id %d : harq_pid %d, mcs %d\n",
	    module_idP,CC_id,harq_pid,mcs);
      
    }
    LOG_D(MAC,"Checking feasibility pdu %d (new sdu)\n",dl_req->number_pdu);
    if (!CCE_allocation_infeasible(module_idP,CC_id,1,subframeP,dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,rnti)) {
      
      
      //ue_sched_ctl->round[CC_id][harq_pid] = 0;
      dl_req->number_dci++;
      dl_req->number_pdu++;
      
      // Toggle NDI for next time
      /*
      LOG_D(MAC,"CC_id %d Frame %d, subframeP %d: Toggling Format1 NDI for UE %d (rnti %x/%d) oldNDI %d\n",
	    CC_id, frameP,subframeP,UE_id,
	    rnti,harq_pid,UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid]);
      
      UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid]=1-UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
      UE_list->UE_template[CC_id][UE_id].oldmcs1[harq_pid] = mcs;
      UE_list->UE_template[CC_id][UE_id].oldmcs2[harq_pid] = 0;
      AssertFatal(UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated!=NULL,"physicalConfigDedicated is NULL\n");
      AssertFatal(UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->pdsch_ConfigDedicated!=NULL,"physicalConfigDedicated->pdsch_ConfigDedicated is NULL\n");
      */


      
      fill_nfapi_dlsch_config(eNB,
			      dl_req,
			      TBS,
			      eNB->pdu_index[CC_id],
			      rnti,
			      0, // type 0 allocation from 7.1.6 in 36.213
			      0, // virtual_resource_block_assignment_flag
			      rb_alloc, // resource_block_coding
			      getQm(mcs),
			      0, // redundancy version
			      1, // transport blocks
			      0, // transport block to codeword swap flag
			      cc[CC_id].p_eNB == 1 ? 0 : 1, // transmission_scheme
			      1, // number of layers
			      1, // number of subbands
			      //			     uint8_t codebook_index,
			      4, // UE category capacity
			      0, /*UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->pdsch_ConfigDedicated->p_a,*/ 
			      0, // delta_power_offset for TM5
			      0, // ngap
			      0, // nprb
			      cc[CC_id].p_eNB == 1 ? 1 : 2, // transmission mode
			      0, //number of PRBs treated as one subband, not used here
			      0 // number of beamforming vectors, not used here
			      );  

      eNB->TX_req[CC_id].sfn_sf = fill_nfapi_tx_req(&eNB->TX_req[CC_id].tx_request_body,
						    (frameP*10)+subframeP,
						    TBS,
						    eNB->pdu_index[CC_id],
						    eNB->UE_list.DLSCH_pdu[CC_id][0][(unsigned char)UE_id].payload[0]);
    }
  }
}
