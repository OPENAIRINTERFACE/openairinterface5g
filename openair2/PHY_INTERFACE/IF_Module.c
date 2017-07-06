#include "openair1/PHY/defs.h"
#include "openair2/PHY_INTERFACE/IF_Module.h"
#include "openair1/PHY/extern.h"
#include "LAYER2/MAC/extern.h"
#include "LAYER2/MAC/proto.h"
#include "common/ran_context.h"

#define MAX_IF_MODULES 100

IF_Module_t *if_inst[MAX_IF_MODULES];
Sched_Rsp_t Sched_INFO[MAX_IF_MODULES][MAX_NUM_CCs];

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
 


  // Call uplink indication function
  AssertFatal(ifi->UL_indication!=NULL,"UL_indication is null (mod %d, cc %d)\n",
	      module_id,
	      CC_id);

  //  ifi->UL_indication(UL_info);
    
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
    LOG_I(PHY,"Schedule_response: frame %d, subframe %d (dl_pdus %d / %p)\n",sched_info->frame,sched_info->subframe,sched_info->DL_req->number_pdu,
	  &sched_info->DL_req->number_pdu);
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
