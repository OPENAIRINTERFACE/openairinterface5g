#include "openair1/PHY/defs_eNB.h"
#include "openair2/NR_PHY_INTERFACE/NR_IF_Module.h"
#include "openair1/PHY/phy_extern.h"
#include "LAYER2/MAC/mac_extern.h"
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
  clear_nfapi_information(RC.mac[module_id],CC_id,
        UL_info->frame,UL_info->subframe);

  handle_rach(UL_info);

  handle_sr(UL_info);

  handle_cqi(UL_info);

  handle_harq(UL_info);

  // clear HI prior to handling ULSCH
  mac->HI_DCI0_req[CC_id].hi_dci0_request_body.number_of_hi                     = 0;
  
  handle_ulsch(UL_info);

  if (nfapi_mode != 1)
  {
    if (ifi->CC_mask == ((1<<MAX_NUM_CCs)-1)) {

      eNB_dlsch_ulsch_scheduler(module_id,
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
          (is_UL_sf(&mac->common_channels[CC_id],(sched_info->subframe+sf_ahead)%10)>0))
        sched_info->UL_req      = &mac->UL_req[CC_id];
      else
        sched_info->UL_req      = NULL;

      sched_info->TX_req      = &mac->TX_req[CC_id];

#ifdef DUMP_FAPI
      dump_dl(sched_info);
#endif

      if (ifi->schedule_response)
      {
        AssertFatal(ifi->schedule_response!=NULL,
            "schedule_response is null (mod %d, cc %d)\n",
            module_id,
            CC_id);
        ifi->schedule_response(sched_info);
      }

      LOG_D(PHY,"Schedule_response: SFN_SF:%d%d dl_pdus:%d\n",sched_info->frame,sched_info->subframe,sched_info->DL_req->dl_config_request_body.number_pdu);
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
    if_inst[Mod_id]->UL_indication = NR_UL_indication;

    AssertFatal(pthread_mutex_init(&if_inst[Mod_id]->if_mutex,NULL)==0,
        "allocation of if_inst[%d]->if_mutex fails\n",Mod_id);
  }
  return if_inst[Mod_id];
}