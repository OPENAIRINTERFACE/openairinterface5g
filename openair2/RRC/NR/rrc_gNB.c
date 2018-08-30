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

/*! \file rrc_gNB.c
 * \brief rrc procedures for gNB
 * \author Navid Nikaein and  Raymond Knopp , WEI-TAI CHEN
 * \date 2011 - 2014 , 2018
 * \version 1.0
 * \company Eurecom, NTUST
 * \email: navid.nikaein@eurecom.fr and raymond.knopp@eurecom.fr, kroempa@gmail.com
 */
#define RRC_GNB_C
#define RRC_GNB_C

#include "nr_rrc_config.h"
#include "nr_rrc_defs.h"
#include "nr_rrc_extern.h"
#include "assertions.h"
#include "common/ran_context.h"
#include "asn1_conversions.h"

#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "LAYER2/RLC/rlc.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "common/utils/LOG/log.h"
#include "COMMON/mac_rrc_primitives.h"
#include "RRC/NR/MESSAGES/asn1_msg.h"

#include "NR_BCCH-BCH-Message.h"
#include "NR_UL-DCCH-Message.h"
#include "NR_DL-DCCH-Message.h"
#include "NR_CellGroupConfig.h"
#include "NR_MeasResults.h"

#include "rlc.h"
#include "rrc_eNB_UE_context.h"
#include "platform_types.h"
#include "msc.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "T.h"

//#if defined(Rel10) || defined(Rel14)
#include "MeasResults.h"
//#endif

#include "RRC/NAS/nas_config.h"
#include "RRC/NAS/rb_config.h"
#include "OCG.h"
#include "OCG_extern.h"

#if defined(ENABLE_SECURITY)
#   include "UTIL/OSA/osa_defs.h"
#endif

#if defined(ENABLE_USE_MME)
#   include "rrc_eNB_S1AP.h"
#   include "rrc_eNB_GTPV1U.h"
#   if defined(ENABLE_ITTI)
#   else
#      include "../../S1AP/s1ap_eNB.h"
#   endif
#endif

#include "pdcp.h"
#include "gtpv1u_eNB_task.h"

#if defined(ENABLE_ITTI)
#   include "intertask_interface.h"
#endif

#if ENABLE_RAL
#   include "rrc_eNB_ral.h"
#endif

#include "SIMULATION/TOOLS/sim.h" // for taus

//#define XER_PRINT


extern RAN_CONTEXT_t RC;

#ifdef PHY_EMUL
extern EMULATION_VARS              *Emul_vars;
#endif
//extern eNB_MAC_INST                *eNB_mac_inst;
//extern UE_MAC_INST                 *UE_mac_inst;
#ifdef BIGPHYSAREA
extern void*                        bigphys_malloc(int);
#endif

extern uint16_t                     two_tier_hexagonal_cellIds[7];

mui_t                               rrc_gNB_mui = 0;

///---------------------------------------------------------------------------------------------------------------///
///---------------------------------------------------------------------------------------------------------------///

void openair_nr_rrc_on(const protocol_ctxt_t* const ctxt_pP){
  
  int            CC_id;
  LOG_I(NR_RRC, PROTOCOL_NR_RRC_CTXT_FMT" gNB:OPENAIR NR RRC IN....\n",PROTOCOL_NR_RRC_CTXT_ARGS(ctxt_pP));

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    rrc_config_nr_buffer (&RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].SI, BCCH, 1);
    RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].SI.Active = 1;
    rrc_config_nr_buffer (&RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].Srb0, CCCH, 1);
    RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Active = 1;
  }

}

///---------------------------------------------------------------------------------------------------------------///
///---------------------------------------------------------------------------------------------------------------///

void rrc_gNB_process_SgNBAdditionRequest( 
     const protocol_ctxt_t  *const ctxt_pP,
     rrc_gNB_ue_context_t   *ue_context_pP 
     ){

  rrc_gNB_generate_SgNBAdditionRequestAcknowledge(ctxt_pP,ue_context_pP);
}

void rrc_gNB_generate_SgNBAdditionRequestAcknowledge( 
     const protocol_ctxt_t  *const ctxt_pP,
     rrc_gNB_ue_context_t   *const ue_context_pP
     ){
  
  uint8_t size;
  uint8_t buffer[100];
  int     CC_id = ue_context_pP->ue_context.primaryCC_id;
  OCTET_STRING_t                                      *secondaryCellGroup;
  NR_CellGroupConfig_t                                *cellGroupconfig;
  struct NR_CellGroupConfig__rlc_BearerToAddModList   *rlc_BearerToAddModList;
  struct NR_MAC_CellGroupConfig                       *mac_CellGroupConfig;
  struct NR_PhysicalCellGroupConfig                   *physicalCellGroupConfig;
  struct NR_SpCellConfig                              *spCellConfig;
  struct NR_CellGroupConfig__sCellToAddModList        *sCellToAddModList;

  cellGroupconfig                           = CALLOC(1,sizeof(NR_CellGroupConfig_t));
  cellGroupconfig->rlc_BearerToAddModList   = CALLOC(1,sizeof(struct NR_CellGroupConfig__rlc_BearerToAddModList));
  cellGroupconfig->mac_CellGroupConfig      = CALLOC(1,sizeof(struct NR_MAC_CellGroupConfig));
  cellGroupconfig->physicalCellGroupConfig  = CALLOC(1,sizeof(struct NR_PhysicalCellGroupConfig));
  cellGroupconfig->spCellConfig             = CALLOC(1,sizeof(struct NR_SpCellConfig));
  //cellGroupconfig->sCellToAddModList        = CALLOC(1,sizeof(struct NR_CellGroupConfig__sCellToAddModList));

  rlc_BearerToAddModList   = cellGroupconfig->rlc_BearerToAddModList;
  mac_CellGroupConfig      = cellGroupconfig->mac_CellGroupConfig;
  physicalCellGroupConfig  = cellGroupconfig->physicalCellGroupConfig;
  spCellConfig             = cellGroupconfig->spCellConfig;
  //sCellToAddModList        = cellGroupconfig->sCellToAddModList;
  
  rlc_bearer_config_t *rlc_config;
  rlc_config = CALLOC(1,sizeof(rlc_bearer_config_t));
  //Fill rlc_bearer config value
  rrc_config_rlc_bearer(ctxt_pP->module_id,
                        ue_context_pP->ue_context.primaryCC_id,
                        rlc_config
                       );
  //Fill rlc_bearer config to structure
  do_RLC_BEARER(ctxt_pP->module_id,
                ue_context_pP->ue_context.primaryCC_id,
                rlc_BearerToAddModList,
                rlc_config);

  mac_cellgroup_t *mac_cellgroup_config;
  mac_cellgroup_config = CALLOC(1,sizeof(mac_cellgroup_t));
  //Fill mac_cellgroup_config config value
  rrc_config_mac_cellgroup(ctxt_pP->module_id,
                           ue_context_pP->ue_context.primaryCC_id,
                           mac_cellgroup_config
                          );
  //Fill mac_cellgroup config to structure
  do_MAC_CELLGROUP(ctxt_pP->module_id,
                   ue_context_pP->ue_context.primaryCC_id,
                   mac_CellGroupConfig,
                   mac_cellgroup_config);

  physicalcellgroup_t *physicalcellgroup_config;
  physicalcellgroup_config = CALLOC(1,sizeof(physicalcellgroup_t));
  //Fill physicalcellgroup_config config value
  rrc_config_physicalcellgroup(ctxt_pP->module_id,
                               ue_context_pP->ue_context.primaryCC_id,
                               physicalcellgroup_config
                              );
  //Fill physicalcellgroup config to structure
  do_PHYSICALCELLGROUP(ctxt_pP->module_id,
                       ue_context_pP->ue_context.primaryCC_id,
                       physicalCellGroupConfig,
                       physicalcellgroup_config);


  do_SpCellConfig(ctxt_pP->module_id,
                  ue_context_pP->ue_context.primaryCC_id,
                  spCellConfig);


}

///---------------------------------------------------------------------------------------------------------------///
///---------------------------------------------------------------------------------------------------------------///

static void init_NR_SI(const protocol_ctxt_t* const ctxt_pP,
                       const int              CC_id
                       #if defined(ENABLE_ITTI)
                       ,
                       gNB_RrcConfigurationReq * configuration
                       #endif
                      ){
  //int                                 i;

  LOG_D(RRC,"%s()\n\n\n\n",__FUNCTION__);

  // copy basic parameters
  RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].physCellId      = configuration->Nid_cell[CC_id];
  RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].p_gNB           = configuration->nb_antenna_ports[CC_id];
  RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].Ncp             = configuration->DL_prefix_type[CC_id];
  RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].Ncp_UL          = configuration->UL_prefix_type[CC_id];
  RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].dl_CarrierFreq  = configuration->downlink_frequency[CC_id];
  RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].ul_CarrierFreq  = configuration->downlink_frequency[CC_id]+ configuration->uplink_frequency_offset[CC_id];
  
  ///MIB
  RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].sizeof_MIB      = 0;
  RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].MIB             = (uint8_t*) malloc16(4);
  RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].sizeof_MIB      = do_MIB_NR(&RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id],0,
                                                                            #ifdef ENABLE_ITTI
                                                                            configuration->MIB_ssb_SubcarrierOffset[CC_id],
                                                                            configuration->pdcch_ConfigSIB1[CC_id],
                                                                            configuration->MIB_subCarrierSpacingCommon[CC_id],
                                                                            configuration->MIB_dmrs_TypeA_Position[CC_id]
                                                                            #else
                                                                            0,0,15,2
                                                                            #endif
                                                                            );

  do_SERVINGCELLCONFIGCOMMON(ctxt_pP->module_id,
                             CC_id,
                             #if defined(ENABLE_ITTI)
                             configuration,
                             #endif
                             1
                             );
  
  LOG_I(NR_RRC,"Done init_NR_SI\n");
  
  
  rrc_mac_config_req_gNB(ctxt_pP->module_id,
                         CC_id,
                         RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].p_gNB,
                         configuration->eutra_band[CC_id],
                         RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].dl_CarrierFreq,
                         configuration->N_RB_DL[CC_id],
                         (NR_BCCH_BCH_Message_t *)&RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].mib,
                         (NR_ServingCellConfigCommon_t *)&RC.nrrrc[ctxt_pP->module_id]->carrier[CC_id].servingcellconfigcommon
                         );
}


char openair_rrc_gNB_configuration(const module_id_t gnb_mod_idP, gNB_RrcConfigurationReq* configuration){
  protocol_ctxt_t      ctxt;
  int                  CC_id;

  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, gnb_mod_idP, GNB_FLAG_YES, NOT_A_RNTI, 0, 0,gnb_mod_idP);
  LOG_I(NR_RRC,
        PROTOCOL_NR_RRC_CTXT_FMT" Init...\n",
        PROTOCOL_NR_RRC_CTXT_ARGS(&ctxt));

  #if OCP_FRAMEWORK
    while ( RC.nrrrc[gnb_mod_idP] == NULL ) {
      LOG_E(NR_RRC, "RC.nrrrc not yet initialized, waiting 1 second\n");
      sleep(1);
    }
  #endif 
    AssertFatal(RC.nrrrc[gnb_mod_idP] != NULL, "RC.nrrrc not initialized!");
    AssertFatal(NUMBER_OF_UE_MAX < (module_id_t)0xFFFFFFFFFFFFFFFF, " variable overflow");
  #ifdef ENABLE_ITTI
    AssertFatal(configuration!=NULL,"configuration input is null\n");
  #endif

  RC.nrrrc[ctxt.module_id]->Nb_ue = 0;

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    RC.nrrrc[ctxt.module_id]->carrier[CC_id].Srb0.Active = 0;
  }

  uid_linear_allocator_init(&RC.nrrrc[ctxt.module_id]->uid_allocator);
  RB_INIT(&RC.nrrrc[ctxt.module_id]->rrc_ue_head);

  RC.nrrrc[ctxt.module_id]->initial_id2_s1ap_ids = hashtable_create (NUMBER_OF_UE_MAX * 2, NULL, NULL);
  RC.nrrrc[ctxt.module_id]->s1ap_id2_s1ap_ids    = hashtable_create (NUMBER_OF_UE_MAX * 2, NULL, NULL);

  memcpy(&RC.nrrrc[ctxt.module_id]->configuration,configuration,sizeof(gNB_RrcConfigurationReq));

  /// System Information INIT

  LOG_I(NR_RRC, PROTOCOL_NR_RRC_CTXT_FMT" Checking release \n",PROTOCOL_NR_RRC_CTXT_ARGS(&ctxt));

  #ifdef CBA

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    for (j = 0; j < NUM_MAX_CBA_GROUP; j++) {
      RC.nrrrc[ctxt.module_id]->carrier[CC_id].cba_rnti[j] = CBA_OFFSET + j;
    }

    if (RC.nrrrc[ctxt.module_id]->carrier[CC_id].num_active_cba_groups > NUM_MAX_CBA_GROUP) {
      RC.nrrrc[ctxt.module_id]->carrier[CC_id].num_active_cba_groups = NUM_MAX_CBA_GROUP;
    }

    LOG_D(NR_RRC,
          PROTOCOL_NR_RRC_CTXT_FMT" Initialization of 4 cba_RNTI values (%x %x %x %x) num active groups %d\n",
          PROTOCOL_NR_RRC_CTXT_ARGS(&ctxt),
          gnb_mod_idP, RC.nrrrc[ctxt.module_id]->carrier[CC_id].cba_rnti[0],
          RC.nrrrc[ctxt.module_id]->carrier[CC_id].cba_rnti[1],
          RC.nrrrc[ctxt.module_id]->carrier[CC_id].cba_rnti[2],
          RC.nrrrc[ctxt.module_id]->carrier[CC_id].cba_rnti[3],
          RC.nrrrc[ctxt.module_id]->carrier[CC_id].num_active_cba_groups);
  }

  #endif

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    init_NR_SI(&ctxt,
              CC_id
              #if defined(ENABLE_ITTI)
              ,configuration
              #endif
              );
  }//END for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++)

  rrc_init_nr_global_param();


  openair_nr_rrc_on(&ctxt);

  return 0;  

}//END openair_rrc_gNB_configuration


///---------------------------------------------------------------------------------------------------------------///
///---------------------------------------------------------------------------------------------------------------///
#if defined(ENABLE_ITTI)

void* rrc_gnb_task(void* args_p){
  MessageDef                         *msg_p;
  const char                         *msg_name_p;
  instance_t                         instance;
  int                                result;
  //SRB_INFO                           *srb_info_p;
  //int                                CC_id;
  //protocol_ctxt_t                    ctxt;

  itti_mark_task_ready(TASK_RRC_GNB);
  LOG_I(NR_RRC,"Entering main loop of NR_RRC message task\n");

  while (1) {
  // Wait for a message
    itti_receive_msg(TASK_RRC_GNB, &msg_p);

    msg_name_p = ITTI_MSG_NAME(msg_p);
    instance = ITTI_MSG_INSTANCE(msg_p);
    LOG_I(NR_RRC,"Received message %s\n",msg_name_p);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      LOG_W(RRC, " *** Exiting NR_RRC thread\n");
      itti_exit_task();
      break;

    case MESSAGE_TEST:
      LOG_I(RRC, "[gNB %d] Received %s\n", instance, msg_name_p);
      break;

      /* Messages from MAC */

	    
      /* Messages from PDCP */

/*
#if defined(ENABLE_USE_MME)

      // Messages from S1AP 
    case S1AP_DOWNLINK_NAS:
      rrc_eNB_process_S1AP_DOWNLINK_NAS(msg_p, msg_name_p, instance, &rrc_gNB_mui);
      break;

    case S1AP_INITIAL_CONTEXT_SETUP_REQ:
      rrc_eNB_process_S1AP_INITIAL_CONTEXT_SETUP_REQ(msg_p, msg_name_p, instance);
      break;

    case S1AP_UE_CTXT_MODIFICATION_REQ:
      rrc_eNB_process_S1AP_UE_CTXT_MODIFICATION_REQ(msg_p, msg_name_p, instance);
      break;

    case S1AP_PAGING_IND:
      LOG_D(RRC, "[eNB %d] Received Paging message from S1AP: %s\n", instance, msg_name_p);
      rrc_eNB_process_PAGING_IND(msg_p, msg_name_p, instance);
      break;
  
    case S1AP_E_RAB_SETUP_REQ: 
      rrc_eNB_process_S1AP_E_RAB_SETUP_REQ(msg_p, msg_name_p, instance);
      LOG_D(RRC, "[eNB %d] Received the message %s\n", instance, msg_name_p);
      break;

    case S1AP_E_RAB_MODIFY_REQ:
      rrc_eNB_process_S1AP_E_RAB_MODIFY_REQ(msg_p, msg_name_p, instance);
      break;

    case S1AP_E_RAB_RELEASE_COMMAND:
      rrc_eNB_process_S1AP_E_RAB_RELEASE_COMMAND(msg_p, msg_name_p, instance);
      break;
    
    case S1AP_UE_CONTEXT_RELEASE_REQ:
      rrc_eNB_process_S1AP_UE_CONTEXT_RELEASE_REQ(msg_p, msg_name_p, instance);
      break;

    case S1AP_UE_CONTEXT_RELEASE_COMMAND:
      rrc_eNB_process_S1AP_UE_CONTEXT_RELEASE_COMMAND(msg_p, msg_name_p, instance);
      break;

    case GTPV1U_ENB_DELETE_TUNNEL_RESP:
      ///Nothing to do. Apparently everything is done in S1AP processing
      //LOG_I(RRC, "[eNB %d] Received message %s, not processed because procedure not synched\n",
      //instance, msg_name_p);
      if (rrc_eNB_get_ue_context(RC.nrrrc[instance], GTPV1U_ENB_DELETE_TUNNEL_RESP(msg_p).rnti)
          && rrc_eNB_get_ue_context(RC.nrrrc[instance], GTPV1U_ENB_DELETE_TUNNEL_RESP(msg_p).rnti)->ue_context.ue_release_timer_rrc > 0) {
        rrc_eNB_get_ue_context(RC.nrrrc[instance], GTPV1U_ENB_DELETE_TUNNEL_RESP(msg_p).rnti)->ue_context.ue_release_timer_rrc =
        rrc_eNB_get_ue_context(RC.nrrrc[instance], GTPV1U_ENB_DELETE_TUNNEL_RESP(msg_p).rnti)->ue_context.ue_release_timer_thres_rrc;
      }
      break;

#endif
*/
    /* Messages from gNB app */
    case NRRRC_CONFIGURATION_REQ:
      LOG_I(NR_RRC, "[gNB %d] Received %s : %p\n", instance, msg_name_p,&NRRRC_CONFIGURATION_REQ(msg_p));
      openair_rrc_gNB_configuration(GNB_INSTANCE_TO_MODULE_ID(instance), &NRRRC_CONFIGURATION_REQ(msg_p));
      break;

    default:
      LOG_E(NR_RRC, "[gNB %d] Received unexpected message %s\n", instance, msg_name_p);
      break;
    }

    result = itti_free(ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal(result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    msg_p = NULL;
  }
}

#endif //END #if defined(ENABLE_ITTI)
