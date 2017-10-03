#include "openair1/PHY/defs.h"
#include "openair2/PHY_INTERFACE/IF_Module.h"
#include "openair1/PHY/extern.h"
#include "LAYER2/MAC/extern.h"
#include "LAYER2/MAC/proto.h"
#include "common/ran_context.h"

#define MAX_IF_MODULES 100

IF_Module_t *if_inst[MAX_IF_MODULES];
Sched_Rsp_t Sched_INFO[MAX_IF_MODULES][MAX_NUM_CCs];

void handle_rach(UL_IND_t *UL_info) {
  int i;

  if (UL_info->rach_ind.number_of_preambles>0) {

    AssertFatal(UL_info->rach_ind.number_of_preambles==1,"More than 1 preamble not supported\n");
    UL_info->rach_ind.number_of_preambles=0;
    LOG_D(MAC,"Frame %d, Subframe %d Calling initiate_ra_proc\n",UL_info->frame,UL_info->subframe);
    initiate_ra_proc(UL_info->module_id,
		     UL_info->CC_id,
		     UL_info->frame,
		     UL_info->subframe,
		     UL_info->rach_ind.preamble_list[0].preamble_rel8.preamble,
		     UL_info->rach_ind.preamble_list[0].preamble_rel8.timing_advance,
		     UL_info->rach_ind.preamble_list[0].preamble_rel8.rnti
#ifdef Rel14
		     ,0
#endif
		     );
  }

#ifdef Rel14
  if (UL_info->rach_ind_br.number_of_preambles>0) {

    AssertFatal(UL_info->rach_ind_br.number_of_preambles<5,"More than 4 preambles not supported\n");
    for (i=0;i<UL_info->rach_ind_br.number_of_preambles;i++) {
      AssertFatal(UL_info->rach_ind_br.preamble_list[i].preamble_rel13.rach_resource_type>0,
		  "Got regular PRACH preamble, not BL/CE\n");
      LOG_D(MAC,"Frame %d, Subframe %d Calling initiate_ra_proc (CE_level %d)\n",UL_info->frame,UL_info->subframe,
	    UL_info->rach_ind_br.preamble_list[i].preamble_rel13.rach_resource_type-1);
      initiate_ra_proc(UL_info->module_id,
		       UL_info->CC_id,
		       UL_info->frame,
		       UL_info->subframe,
		       UL_info->rach_ind_br.preamble_list[i].preamble_rel8.preamble,
		       UL_info->rach_ind_br.preamble_list[i].preamble_rel8.timing_advance,
		       UL_info->rach_ind_br.preamble_list[i].preamble_rel8.rnti,
		       UL_info->rach_ind_br.preamble_list[i].preamble_rel13.rach_resource_type);
    }
    UL_info->rach_ind.number_of_preambles=0;
  }
#endif
}

void handle_sr(UL_IND_t *UL_info) {

  int i;

  for (i=0;i<UL_info->sr_ind.number_of_srs;i++) 
    SR_indication(UL_info->module_id,
		  UL_info->CC_id,
		  UL_info->frame,
		  UL_info->subframe,
		  UL_info->sr_ind.sr_pdu_list[i].rx_ue_information.rnti,
		  UL_info->sr_ind.sr_pdu_list[i].ul_cqi_information.ul_cqi);

  UL_info->sr_ind.number_of_srs=0;
}

void handle_cqi(UL_IND_t *UL_info) {

  int i;

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

void handle_harq(UL_IND_t *UL_info) {

  int i;

  for (i=0;i<UL_info->harq_ind.number_of_harqs;i++) 
    harq_indication(UL_info->module_id,
		    UL_info->CC_id,
		    UL_info->frame,
		    UL_info->subframe,
		    &UL_info->harq_ind.harq_pdu_list[i]);

  UL_info->harq_ind.number_of_harqs=0;
}

void handle_ulsch(UL_IND_t *UL_info) {

  int i,j;

  for (i=0;i<UL_info->rx_ind.number_of_pdus;i++) {

    for (j=0;j<UL_info->crc_ind.number_of_crcs;j++) {
      // find crc_indication j corresponding rx_indication i
      if (UL_info->crc_ind.crc_pdu_list[j].rx_ue_information.rnti ==
	  UL_info->rx_ind.rx_pdu_list[i].rx_ue_information.rnti) {
	if (UL_info->crc_ind.crc_pdu_list[j].crc_indication_rel8.crc_flag == 1) { // CRC error indication
	  LOG_D(MAC,"Frame %d, Subframe %d Calling rx_sdu (CRC error) \n",UL_info->frame,UL_info->subframe);
	  rx_sdu(UL_info->module_id,
		 UL_info->CC_id,
		 UL_info->frame,
		 UL_info->subframe,
		 UL_info->rx_ind.rx_pdu_list[i].rx_ue_information.rnti,
		 (uint8_t *)NULL,
		 UL_info->rx_ind.rx_pdu_list[i].rx_indication_rel8.length,
		 UL_info->rx_ind.rx_pdu_list[i].rx_indication_rel8.timing_advance,
		 UL_info->rx_ind.rx_pdu_list[i].rx_indication_rel8.ul_cqi);
	}
	else {
	  LOG_D(MAC,"Frame %d, Subframe %d Calling rx_sdu (CRC ok) \n",UL_info->frame,UL_info->subframe);
	  rx_sdu(UL_info->module_id,
		 UL_info->CC_id,
		 UL_info->frame,
		 UL_info->subframe,
		 UL_info->rx_ind.rx_pdu_list[i].rx_ue_information.rnti,
		 UL_info->rx_ind.rx_pdu_list[i].data,
		 UL_info->rx_ind.rx_pdu_list[i].rx_indication_rel8.length,
		 UL_info->rx_ind.rx_pdu_list[i].rx_indication_rel8.timing_advance,
		 UL_info->rx_ind.rx_pdu_list[i].rx_indication_rel8.ul_cqi);
	}
	break;
      } //if (UL_info->crc_ind.crc_pdu_list[j].rx_ue_information.rnti ==
	//    UL_info->rx_ind.rx_pdu_list[i].rx_ue_information.rnti) {
    } //    for (j=0;j<UL_info->crc_ind.number_of_crcs;j++) {
    AssertFatal(j<UL_info->crc_ind.number_of_crcs,"Couldn't find matchin CRC indication\n");
  } //   for (i=0;i<UL_info->rx_ind.number_of_pdus;i++) {
    
  UL_info->rx_ind.number_of_pdus=0;
  UL_info->crc_ind.number_of_crcs=0;
}

void UL_indication(UL_IND_t *UL_info)
{

  AssertFatal(UL_info!=NULL,"UL_INFO is null\n");


  module_id_t  module_id   = UL_info->module_id;
  int          CC_id       = UL_info->CC_id;
  Sched_Rsp_t  *sched_info = &Sched_INFO[module_id][CC_id];
  IF_Module_t  *ifi        = if_inst[module_id];
  eNB_MAC_INST *mac        = RC.mac[module_id];

  LOG_D(PHY,"UL_Indication: frame %d, subframe %d, module_id %d, CC_id %d\n",
	UL_info->frame,UL_info->subframe,
	module_id,CC_id);

  if (ifi->CC_mask==0) {
    ifi->current_frame    = UL_info->frame;
    ifi->current_subframe = UL_info->subframe;
  }
  else {
    AssertFatal(UL_info->frame != ifi->current_frame,"CC_mask %x is not full and frame has changed\n",ifi->CC_mask);
    AssertFatal(UL_info->subframe != ifi->current_subframe,"CC_mask %x is not full and subframe has changed\n",ifi->CC_mask);
  }
  ifi->CC_mask |= (1<<CC_id);
 

  // clear DL/UL info for new scheduling round
  clear_nfapi_information(RC.mac[module_id],CC_id,
			  UL_info->frame,UL_info->subframe);


  handle_rach(UL_info);

  handle_sr(UL_info);

  handle_cqi(UL_info);

  handle_harq(UL_info);

  // clear HI prior to hanling ULSCH
  mac->HI_DCI0_req[CC_id].hi_dci0_request_body.number_of_hi                     = 0;
  
  handle_ulsch(UL_info);

  if (ifi->CC_mask == ((1<<MAX_NUM_CCs)-1)) {

    eNB_dlsch_ulsch_scheduler(module_id,
			      (UL_info->frame+((UL_info->subframe>5)?1:0)) % 1024,
			      (UL_info->subframe+4)%10);

    ifi->CC_mask            = 0;

    sched_info->module_id   = module_id;
    sched_info->CC_id       = CC_id;
    sched_info->frame       = (UL_info->frame + ((UL_info->subframe>5) ? 1 : 0)) % 1024;
    sched_info->subframe    = (UL_info->subframe+4)%10;
    sched_info->DL_req      = &mac->DL_req[CC_id];
    sched_info->HI_DCI0_req = &mac->HI_DCI0_req[CC_id];
    if ((mac->common_channels[CC_id].tdd_Config==NULL) ||
	(is_UL_sf(&mac->common_channels[CC_id],(sched_info->subframe+4)%10)>0)) 
      sched_info->UL_req      = &mac->UL_req[CC_id];
    else
      sched_info->UL_req      = NULL;

    sched_info->TX_req      = &mac->TX_req[CC_id];
    AssertFatal(ifi->schedule_response!=NULL,
		"UL_indication is null (mod %d, cc %d)\n",
		module_id,
		CC_id);
    ifi->schedule_response(sched_info);

    LOG_D(PHY,"Schedule_response: frame %d, subframe %d (dl_pdus %d / %p)\n",sched_info->frame,sched_info->subframe,sched_info->DL_req->dl_config_request_body.number_pdu,
	  &sched_info->DL_req->dl_config_request_body.number_pdu);
  }						 
}

IF_Module_t *IF_Module_init(int Mod_id){

  AssertFatal(Mod_id<MAX_MODULES,"Asking for Module %d > %d\n",Mod_id,MAX_IF_MODULES);

  if (if_inst[Mod_id]==NULL) {
    if_inst[Mod_id] = (IF_Module_t*)malloc(sizeof(IF_Module_t));
    memset((void*)if_inst[Mod_id],0,sizeof(IF_Module_t));
    
    if_inst[Mod_id]->CC_mask=0;
    if_inst[Mod_id]->UL_indication = UL_indication;

    AssertFatal(pthread_mutex_init(&if_inst[Mod_id]->if_mutex,NULL)==0,
		"allocation of if_inst[%d]->if_mutex fails\n",Mod_id);
  }
  return if_inst[Mod_id];
}

void IF_Module_kill(int Mod_id) {

  AssertFatal(Mod_id>MAX_MODULES,"Asking for Module %d > %d\n",Mod_id,MAX_IF_MODULES);
  if (if_inst[Mod_id]!=NULL) free(if_inst[Mod_id]);

}
