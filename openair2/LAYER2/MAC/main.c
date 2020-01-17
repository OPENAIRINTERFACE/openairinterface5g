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

/*! \file main.c
 * \brief top init of Layer 2
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \version 1.0
 * \email: navid.nikaein@eurecom.fr
 * @ingroup _mac

 */

#include <dlfcn.h>
#include "mac.h"
#include "mac_proto.h"
#include "mac_extern.h"
#include "assertions.h"
#include "LAYER2/PDCP_v10.1.0/pdcp.h"
#include "RRC/LTE/rrc_defs.h"
#include "common/utils/LOG/log.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "common/ran_context.h"
#include "intertask_interface.h"

extern RAN_CONTEXT_t RC;

void init_UE_list(UE_list_t *UE_list)
{
  int list_el;
  UE_list->num_UEs = 0;
  UE_list->head = -1;
  UE_list->head_ul = -1;
  UE_list->avail = 0;
  for (list_el = 0; list_el < MAX_MOBILES_PER_ENB - 1; list_el++) {
    UE_list->next[list_el] = list_el + 1;
    UE_list->next_ul[list_el] = list_el + 1;
  }
  UE_list->next[list_el] = -1;
  UE_list->next_ul[list_el] = -1;
  memset(UE_list->DLSCH_pdu, 0, sizeof(UE_list->DLSCH_pdu));
  memset(UE_list->UE_template, 0, sizeof(UE_list->UE_template));
  memset(UE_list->eNB_UE_stats, 0, sizeof(UE_list->eNB_UE_stats));
  memset(UE_list->UE_sched_ctrl, 0, sizeof(UE_list->UE_sched_ctrl));
  memset(UE_list->active, 0, sizeof(UE_list->active));
  memset(UE_list->assoc_dl_slice_idx, 0, sizeof(UE_list->assoc_dl_slice_idx));
  memset(UE_list->assoc_ul_slice_idx, 0, sizeof(UE_list->assoc_ul_slice_idx));
}

void init_slice_info(slice_info_t *sli)
{
  sli->intraslice_share_active = 1;
  sli->interslice_share_active = 1;

  sli->n_dl = 1;
  memset(sli->dl, 0, sizeof(slice_sched_conf_dl_t) * MAX_NUM_SLICES);
  sli->dl[0].pct = 1.0;
  sli->dl[0].prio = 10;
  sli->dl[0].pos_high = N_RBG_MAX;
  sli->dl[0].maxmcs = 28;
  sli->dl[0].sorting = 0x012345;
  sli->dl[0].sched_name = "schedule_ue_spec";
  sli->dl[0].sched_cb = dlsym(NULL, sli->dl[0].sched_name);
  AssertFatal(sli->dl[0].sched_cb, "DLSCH scheduler callback is NULL\n");

  sli->n_ul = 1;
  memset(sli->ul, 0, sizeof(slice_sched_conf_ul_t) * MAX_NUM_SLICES);
  sli->ul[0].pct = 1.0;
  sli->ul[0].maxmcs = 20;
  sli->ul[0].sorting = 0x0123;
  sli->ul[0].sched_name = "schedule_ulsch_rnti";
  sli->ul[0].sched_cb = dlsym(NULL, sli->ul[0].sched_name);
  AssertFatal(sli->ul[0].sched_cb, "ULSCH scheduler callback is NULL\n");
}

void mac_top_init_eNB(void)
{
  module_id_t i, j;
  eNB_MAC_INST **mac;

  LOG_I(MAC, "[MAIN] Init function start:nb_macrlc_inst=%d\n",
        RC.nb_macrlc_inst);

  if (RC.nb_macrlc_inst <= 0) {
    RC.mac = NULL;
    return;
  }

  mac = malloc16(RC.nb_macrlc_inst * sizeof(eNB_MAC_INST *));
  AssertFatal(mac != NULL,
              "can't ALLOCATE %zu Bytes for %d eNB_MAC_INST with size %zu \n",
              RC.nb_macrlc_inst * sizeof(eNB_MAC_INST *),
              RC.nb_macrlc_inst, sizeof(eNB_MAC_INST));
  for (i = 0; i < RC.nb_macrlc_inst; i++) {
    mac[i] = malloc16(sizeof(eNB_MAC_INST));
    AssertFatal(mac[i] != NULL,
                "can't ALLOCATE %zu Bytes for %d eNB_MAC_INST with size %zu \n",
                RC.nb_macrlc_inst * sizeof(eNB_MAC_INST *),
                RC.nb_macrlc_inst, sizeof(eNB_MAC_INST));
    LOG_D(MAC,
          "[MAIN] ALLOCATE %zu Bytes for %d eNB_MAC_INST @ %p\n",
          sizeof(eNB_MAC_INST), RC.nb_macrlc_inst, mac);
    bzero(mac[i], sizeof(eNB_MAC_INST));
    mac[i]->Mod_id = i;
    for (j = 0; j < MAX_NUM_CCs; j++) {
      mac[i]->DL_req[j].dl_config_request_body.dl_config_pdu_list =
          mac[i]->dl_config_pdu_list[j];
      mac[i]->UL_req[j].ul_config_request_body.ul_config_pdu_list =
          mac[i]->ul_config_pdu_list[j];
      for (int k = 0; k < 10; k++)
        mac[i]->UL_req_tmp[j][k].ul_config_request_body.ul_config_pdu_list =
            mac[i]->ul_config_pdu_list_tmp[j][k];
      for(int sf=0;sf<10;sf++)
        mac[i]->HI_DCI0_req[j][sf].hi_dci0_request_body.hi_dci0_pdu_list =
            mac[i]->hi_dci0_pdu_list[j][sf];
      mac[i]->TX_req[j].tx_request_body.tx_pdu_list = mac[i]->tx_request_pdu[j];
      mac[i]->ul_handle = 0;
    }

    mac[i]->if_inst = IF_Module_init(i);

    init_UE_list(&mac[i]->UE_list);
    init_slice_info(&mac[i]->slice_info);
  }

  RC.mac = mac;

  AssertFatal(rlc_module_init() == 0,
      "Could not initialize RLC layer\n");

  // These should be out of here later
  pdcp_layer_init();

  rrc_init_global_param();
}

void mac_init_cell_params(int Mod_idP, int CC_idP)
{

    int j;
    UE_TEMPLATE *UE_template;

    LOG_D(MAC, "[MSC_NEW][FRAME 00000][MAC_eNB][MOD %02d][]\n", Mod_idP);
    //COMMON_channels_t *cc = &RC.mac[Mod_idP]->common_channels[CC_idP];

    memset(&RC.mac[Mod_idP]->eNB_stats, 0, sizeof(eNB_STATS));
    UE_template =
	(UE_TEMPLATE *) & RC.mac[Mod_idP]->UE_list.UE_template[CC_idP][0];

    for (j = 0; j < MAX_MOBILES_PER_ENB; j++) {
	UE_template[j].rnti = 0;
	// initiallize the eNB to UE statistics
	memset(&RC.mac[Mod_idP]->UE_list.eNB_UE_stats[CC_idP][j], 0,
	       sizeof(eNB_UE_STATS));
    }

}


int rlcmac_init_global_param(void)
{


    LOG_I(MAC, "[MAIN] CALLING RLC_MODULE_INIT...\n");

    if (rlc_module_init() != 0) {
	return (-1);
    }

    pdcp_layer_init();

    LOG_I(MAC, "[MAIN] Init Global Param Done\n");

    return 0;
}


void mac_top_cleanup(void)
{

    if (NB_UE_INST > 0) {
	free(UE_mac_inst);
    }

    if (RC.nb_macrlc_inst > 0) {
	free(RC.mac);
    }

}

int l2_init_eNB(void)
{



    LOG_I(MAC, "[MAIN] MAC_INIT_GLOBAL_PARAM IN...\n");

    rlcmac_init_global_param();

    LOG_D(MAC, "[MAIN] ALL INIT OK\n");


    return (1);
}

//-----------------------------------------------------------------------------
/*
 * Main loop of MAC itti message handling
 */
void *mac_enb_task(void *arg)
//-----------------------------------------------------------------------------
{
  MessageDef *received_msg = NULL;
  int         result;

  itti_mark_task_ready(TASK_MAC_ENB); // void function 10/2019
  LOG_I(MAC,"Starting main loop of MAC message task\n");

  while (1) {
    itti_receive_msg(TASK_MAC_ENB, &received_msg);

    switch (ITTI_MSG_ID(received_msg)) {
      case RRC_MAC_DRX_CONFIG_REQ:
        LOG_I(MAC, "MAC Task Received RRC_MAC_DRX_CONFIG_REQ\n");
        /* Set timers and thresholds values in local MAC context of UE */
        eNB_Config_Local_DRX(ITTI_MESSAGE_GET_INSTANCE(received_msg), &received_msg->ittiMsg.rrc_mac_drx_config_req);
        break;

      case TERMINATE_MESSAGE:
        LOG_W(MAC, " *** Exiting MAC thread\n");
        itti_exit_task();
        break;

      default:
        LOG_E(MAC, "MAC instance received unhandled message: %d:%s\n",
              ITTI_MSG_ID(received_msg), 
              ITTI_MSG_NAME(received_msg));
        break;  
    } // end switch

    result = itti_free(ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
    AssertFatal(result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    
    received_msg = NULL;
  } // end while

  return NULL;
}