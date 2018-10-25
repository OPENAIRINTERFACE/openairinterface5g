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

/*! \file openair2/NR_PHY_INTERFACE/NR_IF_Module.c
* \brief data structures for PHY/MAC interface modules
* \author EURECOM/NTUST
* \date 2018
* \version 0.1
* \company Eurecom, NTUST
* \email: raymond.knopp@eurecom.fr, kroempa@gmail.com
* \note
* \warning
*/

#include "openair1/PHY/defs_eNB.h"
#include "openair1/PHY/phy_extern.h"
#include "openair1/SCHED_NR/fapi_nr_l1.h"
#include "openair2/NR_PHY_INTERFACE/NR_IF_Module.h"
#include "LAYER2/MAC/mac_extern.h"
#include "LAYER2/MAC/mac_proto.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "common/ran_context.h"

#define MAX_IF_MODULES 100

NR_IF_Module_t *if_inst[MAX_IF_MODULES];
NR_Sched_Rsp_t Sched_INFO[MAX_IF_MODULES][MAX_NUM_CCs];

extern int oai_nfapi_harq_indication(nfapi_harq_indication_t *harq_ind);
extern int oai_nfapi_crc_indication(nfapi_crc_indication_t *crc_ind);
extern int oai_nfapi_cqi_indication(nfapi_cqi_indication_t *cqi_ind);
extern int oai_nfapi_sr_indication(nfapi_sr_indication_t *ind);
extern int oai_nfapi_rx_ind(nfapi_rx_indication_t *ind);
extern uint8_t nfapi_mode;
extern uint16_t sf_ahead;

void handle_nr_rach(NR_UL_IND_t *UL_info) {
  int i;

  if (UL_info->rach_ind.rach_indication_body.number_of_preambles>0) {

    AssertFatal(UL_info->rach_ind.rach_indication_body.number_of_preambles==1,"More than 1 preamble not supported\n");
    UL_info->rach_ind.rach_indication_body.number_of_preambles=0;
    LOG_D(MAC,"UL_info[Frame %d, Subframe %d] Calling initiate_ra_proc RACH:SFN/SF:%d\n",UL_info->frame,UL_info->subframe, NFAPI_SFNSF2DEC(UL_info->rach_ind.sfn_sf));
    initiate_ra_proc(UL_info->module_id,
         UL_info->CC_id,
         NFAPI_SFNSF2SFN(UL_info->rach_ind.sfn_sf),
         NFAPI_SFNSF2SF(UL_info->rach_ind.sfn_sf),
         UL_info->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.preamble,
         UL_info->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.timing_advance,
         UL_info->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.rnti
#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
         ,0
#endif
         );
  }

#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  if (UL_info->rach_ind_br.rach_indication_body.number_of_preambles>0) {

    AssertFatal(UL_info->rach_ind_br.rach_indication_body.number_of_preambles<5,"More than 4 preambles not supported\n");
    for (i=0;i<UL_info->rach_ind_br.rach_indication_body.number_of_preambles;i++) {
      AssertFatal(UL_info->rach_ind_br.rach_indication_body.preamble_list[i].preamble_rel13.rach_resource_type>0,
      "Got regular PRACH preamble, not BL/CE\n");
      LOG_D(MAC,"Frame %d, Subframe %d Calling initiate_ra_proc (CE_level %d)\n",UL_info->frame,UL_info->subframe,
      UL_info->rach_ind_br.rach_indication_body.preamble_list[i].preamble_rel13.rach_resource_type-1);
      initiate_ra_proc(UL_info->module_id,
           UL_info->CC_id,
           UL_info->frame,
           UL_info->subframe,
           UL_info->rach_ind_br.rach_indication_body.preamble_list[i].preamble_rel8.preamble,
           UL_info->rach_ind_br.rach_indication_body.preamble_list[i].preamble_rel8.timing_advance,
           UL_info->rach_ind_br.rach_indication_body.preamble_list[i].preamble_rel8.rnti,
           UL_info->rach_ind_br.rach_indication_body.preamble_list[i].preamble_rel13.rach_resource_type);
    }
    UL_info->rach_ind_br.rach_indication_body.number_of_preambles=0;
  }
#endif
}

void handle_nr_sr(NR_UL_IND_t *UL_info) {

  int i;

  if (nfapi_mode == 1)  // PNF
  {
    if (UL_info->sr_ind.sr_indication_body.number_of_srs>0)
    {
      oai_nfapi_sr_indication(&UL_info->sr_ind);
    }
  }
  else
  {
    for (i=0;i<UL_info->sr_ind.sr_indication_body.number_of_srs;i++)
      SR_indication(UL_info->module_id,
          UL_info->CC_id,
          UL_info->frame,
          UL_info->subframe,
          UL_info->sr_ind.sr_indication_body.sr_pdu_list[i].rx_ue_information.rnti,
          UL_info->sr_ind.sr_indication_body.sr_pdu_list[i].ul_cqi_information.ul_cqi);
  }

  UL_info->sr_ind.sr_indication_body.number_of_srs=0;
}

void handle_nr_cqi(NR_UL_IND_t *UL_info) {

  int i;

  if (nfapi_mode == 1)
  {
    if (UL_info->cqi_ind.number_of_cqis>0)
    {
      LOG_D(PHY,"UL_info->cqi_ind.number_of_cqis:%d\n", UL_info->cqi_ind.number_of_cqis);
      nfapi_cqi_indication_t ind;

      ind.header.message_id = NFAPI_RX_CQI_INDICATION;
      ind.sfn_sf = UL_info->frame<<4 | UL_info->subframe;
      ind.cqi_indication_body = UL_info->cqi_ind;

      oai_nfapi_cqi_indication(&ind);

      UL_info->cqi_ind.number_of_cqis=0;
    }
  }
  else
  {
    for (i=0;i<UL_info->cqi_ind.number_of_cqis;i++) 
      cqi_indication(UL_info->module_id,
          UL_info->CC_id,
          UL_info->frame,
          UL_info->subframe,
          UL_info->cqi_ind.cqi_pdu_list[i].rx_ue_information.rnti,
          &UL_info->cqi_ind.cqi_pdu_list[i].cqi_indication_rel9,
          UL_info->cqi_ind.cqi_raw_pdu_list[i].pdu,
          &UL_info->cqi_ind.cqi_pdu_list[i].ul_cqi_information);

    UL_info->cqi_ind.number_of_cqis=0;
  }
}

void handle_nr_harq(NR_UL_IND_t *UL_info) {

  int i;

  if (nfapi_mode == 1 && UL_info->harq_ind.harq_indication_body.number_of_harqs>0) // PNF
  {
    //LOG_D(PHY, "UL_info->harq_ind.harq_indication_body.number_of_harqs:%d Send to VNF\n", UL_info->harq_ind.harq_indication_body.number_of_harqs);

    int retval = oai_nfapi_harq_indication(&UL_info->harq_ind);

    if (retval!=0)
    {
      LOG_E(PHY, "Failed to encode NFAPI HARQ_IND retval:%d\n", retval);
    }

    UL_info->harq_ind.harq_indication_body.number_of_harqs = 0;
  }
  else
  {
    for (i=0;i<UL_info->harq_ind.harq_indication_body.number_of_harqs;i++)
      harq_indication(UL_info->module_id,
          UL_info->CC_id,
          NFAPI_SFNSF2SFN(UL_info->harq_ind.sfn_sf),
          NFAPI_SFNSF2SF(UL_info->harq_ind.sfn_sf),
          &UL_info->harq_ind.harq_indication_body.harq_pdu_list[i]);

    UL_info->harq_ind.harq_indication_body.number_of_harqs=0;
  }
}

void handle_nr_ulsch(NR_UL_IND_t *UL_info) {

  int i,j;

  if(nfapi_mode == 1)
  {
    if (UL_info->crc_ind.crc_indication_body.number_of_crcs>0)
    {
      //LOG_D(PHY,"UL_info->crc_ind.crc_indication_body.number_of_crcs:%d CRC_IND:SFN/SF:%d\n", UL_info->crc_ind.crc_indication_body.number_of_crcs, NFAPI_SFNSF2DEC(UL_info->crc_ind.sfn_sf));

      oai_nfapi_crc_indication(&UL_info->crc_ind);

      UL_info->crc_ind.crc_indication_body.number_of_crcs = 0;
    }

    if (UL_info->rx_ind.rx_indication_body.number_of_pdus>0)
    {
      //LOG_D(PHY,"UL_info->rx_ind.number_of_pdus:%d RX_IND:SFN/SF:%d\n", UL_info->rx_ind.rx_indication_body.number_of_pdus, NFAPI_SFNSF2DEC(UL_info->rx_ind.sfn_sf));
      oai_nfapi_rx_ind(&UL_info->rx_ind);
      UL_info->rx_ind.rx_indication_body.number_of_pdus = 0;
    }
  }
  else
  {
    if (UL_info->rx_ind.rx_indication_body.number_of_pdus>0 && UL_info->crc_ind.crc_indication_body.number_of_crcs>0) {
      for (i=0;i<UL_info->rx_ind.rx_indication_body.number_of_pdus;i++) {
        for (j=0;j<UL_info->crc_ind.crc_indication_body.number_of_crcs;j++) {
          // find crc_indication j corresponding rx_indication i
          LOG_D(PHY,"UL_info->crc_ind.crc_indication_body.crc_pdu_list[%d].rx_ue_information.rnti:%04x UL_info->rx_ind.rx_indication_body.rx_pdu_list[%d].rx_ue_information.rnti:%04x\n", j, UL_info->crc_ind.crc_indication_body.crc_pdu_list[j].rx_ue_information.rnti, i, UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti);
          if (UL_info->crc_ind.crc_indication_body.crc_pdu_list[j].rx_ue_information.rnti ==
              UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti) {
            LOG_D(PHY, "UL_info->crc_ind.crc_indication_body.crc_pdu_list[%d].crc_indication_rel8.crc_flag:%d\n", j, UL_info->crc_ind.crc_indication_body.crc_pdu_list[j].crc_indication_rel8.crc_flag);
            if (UL_info->crc_ind.crc_indication_body.crc_pdu_list[j].crc_indication_rel8.crc_flag == 1) { // CRC error indication
              LOG_D(MAC,"Frame %d, Subframe %d Calling rx_sdu (CRC error) \n",UL_info->frame,UL_info->subframe);
              rx_sdu(UL_info->module_id,
                  UL_info->CC_id,
                  NFAPI_SFNSF2SFN(UL_info->rx_ind.sfn_sf), //UL_info->frame,
                  NFAPI_SFNSF2SF(UL_info->rx_ind.sfn_sf), //UL_info->subframe,
                  UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti,
                  (uint8_t *)NULL,
                  UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.length,
                  UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.timing_advance,
                  UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.ul_cqi);
            }
            else {
              LOG_D(MAC,"Frame %d, Subframe %d Calling rx_sdu (CRC ok) \n",UL_info->frame,UL_info->subframe);
              rx_sdu(UL_info->module_id,
                  UL_info->CC_id,
                  NFAPI_SFNSF2SFN(UL_info->rx_ind.sfn_sf), //UL_info->frame,
                  NFAPI_SFNSF2SF(UL_info->rx_ind.sfn_sf), //UL_info->subframe,
                  UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti,
                  UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].data,
                  UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.length,
                  UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.timing_advance,
                  UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.ul_cqi);
            }
            break;
          } //if (UL_info->crc_ind.crc_pdu_list[j].rx_ue_information.rnti ==
          //    UL_info->rx_ind.rx_pdu_list[i].rx_ue_information.rnti)
        } //    for (j=0;j<UL_info->crc_ind.crc_indication_body.number_of_crcs;j++)
      } //   for (i=0;i<UL_info->rx_ind.number_of_pdus;i++)
      UL_info->crc_ind.crc_indication_body.number_of_crcs=0;
      UL_info->rx_ind.rx_indication_body.number_of_pdus = 0;
    } // UL_info->rx_ind.rx_indication_body.number_of_pdus>0 && UL_info->subframe && UL_info->crc_ind.crc_indication_body.number_of_crcs>0
    else if (UL_info->rx_ind.rx_indication_body.number_of_pdus!=0 || UL_info->crc_ind.crc_indication_body.number_of_crcs!=0) {
      LOG_E(PHY,"hoping not to have mis-match between CRC ind and RX ind - hopefully the missing message is coming shortly rx_ind:%d(SFN/SF:%05d) crc_ind:%d(SFN/SF:%05d) UL_info(SFN/SF):%04d%d\n",
          UL_info->rx_ind.rx_indication_body.number_of_pdus, NFAPI_SFNSF2DEC(UL_info->rx_ind.sfn_sf),
          UL_info->crc_ind.crc_indication_body.number_of_crcs, NFAPI_SFNSF2DEC(UL_info->crc_ind.sfn_sf),
          UL_info->frame, UL_info->subframe);
    }
  }
}

void NR_UL_indication(NR_UL_IND_t *UL_info)
{

  AssertFatal(UL_info!=NULL,"UL_INFO is null\n");

#ifdef DUMP_FAPI
  dump_ul(UL_info);
#endif

  module_id_t  module_id   = UL_info->module_id;
  int          CC_id       = UL_info->CC_id;
  NR_Sched_Rsp_t  *sched_info = &Sched_INFO[module_id][CC_id];
  NR_IF_Module_t  *ifi        = if_inst[module_id];
  gNB_MAC_INST *mac        = RC.nrmac[module_id];

  LOG_D(PHY,"SFN/SF:%d%d module_id:%d CC_id:%d UL_info[rx_ind:%d harqs:%d crcs:%d cqis:%d preambles:%d sr_ind:%d]\n",
        UL_info->frame,UL_info->subframe,
        module_id,CC_id,
        UL_info->rx_ind.rx_indication_body.number_of_pdus, UL_info->harq_ind.harq_indication_body.number_of_harqs, UL_info->crc_ind.crc_indication_body.number_of_crcs, UL_info->cqi_ind.number_of_cqis, UL_info->rach_ind.rach_indication_body.number_of_preambles, UL_info->sr_ind.sr_indication_body.number_of_srs);

  if (nfapi_mode != 1)
  {
    if (ifi->CC_mask==0) {
      ifi->current_frame    = UL_info->frame;
      ifi->current_subframe = UL_info->subframe;
    }
    else {
      AssertFatal(UL_info->frame != ifi->current_frame,"CC_mask %x is not full and frame has changed\n",ifi->CC_mask);
      AssertFatal(UL_info->subframe != ifi->current_subframe,"CC_mask %x is not full and subframe has changed\n",ifi->CC_mask);
    }
    ifi->CC_mask |= (1<<CC_id);
  }


  // clear DL/UL info for new scheduling round
  clear_nr_nfapi_information(RC.nrmac[module_id],CC_id,UL_info->frame,UL_info->subframe);

  handle_nr_rach(UL_info);

  handle_nr_sr(UL_info);

  handle_nr_cqi(UL_info);

  handle_nr_harq(UL_info);

  // clear HI prior to handling ULSCH
  mac->HI_DCI0_req[CC_id].hi_dci0_request_body.number_of_hi                     = 0;
  
  handle_nr_ulsch(UL_info);

  if (nfapi_mode != 1)
  {
    if (ifi->CC_mask == ((1<<MAX_NUM_CCs)-1)) {
      /*
      eNB_dlsch_ulsch_scheduler(module_id,
          (UL_info->frame+((UL_info->subframe>(9-sf_ahead))?1:0)) % 1024,
          (UL_info->subframe+sf_ahead)%10);
      */
      
      gNB_dlsch_ulsch_scheduler(module_id,
          (UL_info->frame+((UL_info->subframe>(9-sf_ahead))?1:0)) % 1024,
          (UL_info->subframe+sf_ahead)%10);
      
      ifi->CC_mask            = 0;

      sched_info->module_id   = module_id;
      sched_info->CC_id       = CC_id;
      sched_info->frame       = (UL_info->frame + ((UL_info->subframe>(9-sf_ahead)) ? 1 : 0)) % 1024;
      sched_info->subframe    = (UL_info->subframe+sf_ahead)%10;
      sched_info->DL_req      = &mac->DL_req[CC_id];
      sched_info->HI_DCI0_req = &mac->HI_DCI0_req[CC_id];
      if ((mac->common_channels[CC_id].tdd_Config==NULL) ||
          (is_nr_UL_sf(&mac->common_channels[CC_id],(sched_info->subframe+sf_ahead)%10)>0))
        sched_info->UL_req      = &mac->UL_req[CC_id];
      else
        sched_info->UL_req      = NULL;

      sched_info->TX_req      = &mac->TX_req[CC_id];

#ifdef DUMP_FAPI
      dump_dl(sched_info);
#endif

      if (ifi->NR_Schedule_response)
      {
        AssertFatal(ifi->NR_Schedule_response!=NULL,
                    "nr_schedule_response is null (mod %d, cc %d)\n",
                    module_id,
                    CC_id);
        ifi->NR_Schedule_response(sched_info);
      }

      LOG_D(PHY,"NR_Schedule_response: SFN_SF:%d%d dl_pdus:%d\n",sched_info->frame,sched_info->subframe,sched_info->DL_req->dl_config_request_body.number_pdu);
    }
  }
}

NR_IF_Module_t *NR_IF_Module_init(int Mod_id){

  AssertFatal(Mod_id<MAX_MODULES,"Asking for Module %d > %d\n",Mod_id,MAX_IF_MODULES);

  LOG_D(PHY,"Installing callbacks for IF_Module - UL_indication\n");

  if (if_inst[Mod_id]==NULL) {
    if_inst[Mod_id] = (NR_IF_Module_t*)malloc(sizeof(NR_IF_Module_t));
    memset((void*)if_inst[Mod_id],0,sizeof(NR_IF_Module_t));

    if_inst[Mod_id]->CC_mask=0;
    if_inst[Mod_id]->NR_UL_indication = NR_UL_indication;

    AssertFatal(pthread_mutex_init(&if_inst[Mod_id]->if_mutex,NULL)==0,
        "allocation of if_inst[%d]->if_mutex fails\n",Mod_id);
  }
  return if_inst[Mod_id];
}