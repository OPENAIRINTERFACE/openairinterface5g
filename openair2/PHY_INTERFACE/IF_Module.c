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

  if (UL_info->rach_ind.number_of_preambles>0) {

    AssertFatal(UL_info->rach_ind.number_of_preambles==1,"More than 1 preamble not supported\n");
    UL_info->rach_ind.number_of_preambles=0;
    LOG_I(MAC,"Frame %d, Subframe %d Calling initiate_ra_proc\n",UL_info->frame,UL_info->subframe);
    initiate_ra_proc(UL_info->module_id,
		     UL_info->CC_id,
		     UL_info->frame,
		     UL_info->subframe,
		     UL_info->rach_ind.preamble_list[0].preamble_rel8.preamble,
		     UL_info->rach_ind.preamble_list[0].preamble_rel8.timing_advance,
		     UL_info->rach_ind.preamble_list[0].preamble_rel8.rnti);
  }
}

void handle_ulsch(UL_IND_t *UL_info) {

  int i;

  for (i=0;i<UL_info->rx_ind.number_of_pdus;i++) {

    LOG_I(MAC,"Frame %d, Subframe %d Calling rx_sdu \n",UL_info->frame,UL_info->subframe);
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
  UL_info->rx_ind.number_of_pdus=0;

  for (i=0;i<UL_info->crc_ind.number_of_crcs;i++) {

    if (UL_info->crc_ind.crc_pdu_list[i].crc_indication_rel8.crc_flag == 1) { // CRC error indication
      LOG_I(MAC,"Frame %d, Subframe %d Calling rx_sdu (CRC error) \n",UL_info->frame,UL_info->subframe);
      rx_sdu(UL_info->module_id,
	     UL_info->CC_id,
	     UL_info->frame,
	     UL_info->subframe,
	     UL_info->crc_ind.crc_pdu_list[i].rx_ue_information.rnti,
	     (uint8_t *)NULL,
	     0,
	     0,
	     0);
    }

 
  }
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

  LOG_I(PHY,"UL_Indication: frame %d, subframe %d, module_id %d, CC_id %d\n",
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
 


  handle_rach(UL_info);

  // clear HI prior to hanling ULSCH
  mac->HI_DCI0_req[CC_id].hi_dci0_request_body.number_of_hi                     = 0;
  
  handle_ulsch(UL_info);

  if (ifi->CC_mask == ((1<<MAX_NUM_CCs)-1)) {

    eNB_dlsch_ulsch_scheduler(module_id,
			      0,
			      UL_info->frame+((UL_info->subframe>5)?1:0),
			      (UL_info->subframe+4)%10);

    ifi->CC_mask            = 0;

    sched_info->module_id   = module_id;
    sched_info->CC_id       = CC_id;
    sched_info->frame       = UL_info->frame + ((UL_info->subframe>5) ? 1 : 0);
    sched_info->subframe    = (UL_info->subframe+4)%10;
    sched_info->DL_req      = &mac->DL_req[CC_id];
    sched_info->HI_DCI0_req = &mac->HI_DCI0_req[CC_id];
    sched_info->UL_req      = &mac->UL_req[CC_id];
    sched_info->TX_req      = &mac->TX_req[CC_id];
    AssertFatal(ifi->schedule_response!=NULL,
		"UL_indication is null (mod %d, cc %d)\n",
		module_id,
		CC_id);
    ifi->schedule_response(sched_info);
    LOG_I(PHY,"Schedule_response: frame %d, subframe %d (dl_pdus %d / %p)\n",sched_info->frame,sched_info->subframe,sched_info->DL_req->dl_config_request_body.number_pdu,
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
