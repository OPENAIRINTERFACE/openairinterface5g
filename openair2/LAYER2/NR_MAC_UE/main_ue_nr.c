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

/* \file main_ue_nr.c
 * \brief top init of Layer 2
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

//#include "defs.h"
#include "mac_proto.h"
#include "radio/COMMON/common_lib.h"
//#undef MALLOC
#include "assertions.h"
#include "executables/softmodem-common.h"
#include "openair2/LAYER2/nr_pdcp/nr_pdcp.h"
#include "openair2/LAYER2/nr_pdcp/nr_pdcp_oai_api.h"
#include "nr_rlc/nr_rlc_oai_api.h"
#include "RRC/NR/MESSAGES/asn1_msg.h"
#include "RRC/NR_UE/rrc_proto.h"
#include <pthread.h>

static NR_UE_MAC_INST_t *nr_ue_mac_inst; 

static void send_srb0_rrc(int rnti, const uint8_t *sdu, sdu_size_t sdu_len, void *data)
{
  AssertFatal(sdu_len > 0 && sdu_len < CCCH_SDU_SIZE, "invalid CCCH SDU size %d\n", sdu_len);

  MessageDef *message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_CCCH_DATA_IND);
  memset(NR_RRC_MAC_CCCH_DATA_IND(message_p).sdu, 0, sdu_len);
  memcpy(NR_RRC_MAC_CCCH_DATA_IND(message_p).sdu, sdu, sdu_len);
  NR_RRC_MAC_CCCH_DATA_IND(message_p).sdu_size = sdu_len;
  NR_RRC_MAC_CCCH_DATA_IND(message_p).rnti = rnti;
  itti_send_msg_to_task(TASK_RRC_NRUE, 0, message_p);
}

void send_msg3_rrc_request(module_id_t mod_id, int rnti)
{
  nr_rlc_activate_srb0(rnti, NULL, send_srb0_rrc);
  nr_mac_rrc_msg3_ind(mod_id, rnti);
}

NR_UE_MAC_INST_t * nr_l2_init_ue(NR_UE_RRC_INST_t* rrc_inst) {

    //LOG_I(MAC, "[MAIN] MAC_INIT_GLOBAL_PARAM IN...\n");

    //LOG_I(MAC, "[MAIN] init UE MAC functions \n");
    
    //init mac here
    nr_ue_mac_inst = (NR_UE_MAC_INST_t *)calloc(NB_NR_UE_MAC_INST, sizeof(NR_UE_MAC_INST_t));

    for (int j = 0; j < NB_NR_UE_MAC_INST; j++)
      nr_ue_init_mac(j);

    int scs = get_softmodem_params()->sa ?
              get_softmodem_params()->numerology :
              rrc_inst ?
              *rrc_inst->scell_group_config->spCellConfig->reconfigurationWithSync->spCellConfigCommon->ssbSubcarrierSpacing :
              - 1;
    if (scs > -1)
      ue_init_config_request(nr_ue_mac_inst, scs);

    if (rrc_inst && rrc_inst->scell_group_config) {

      nr_rrc_mac_config_req_scg(0, 0, rrc_inst->scell_group_config);
      int rc = rlc_module_init(0);
      AssertFatal(rc == 0, "%s: Could not initialize RLC layer\n", __FUNCTION__);
      nr_rlc_activate_srb0(nr_ue_mac_inst->crnti, NULL, send_srb0_rrc);
      if (IS_SOFTMODEM_NOS1){
        // get default noS1 configuration
        NR_RadioBearerConfig_t *rbconfig = NULL;
        NR_RLC_BearerConfig_t *rlc_rbconfig = NULL;
        fill_nr_noS1_bearer_config(&rbconfig, &rlc_rbconfig);

        // set up PDCP, RLC, MAC
        nr_pdcp_layer_init(false);
        nr_pdcp_add_drbs(ENB_FLAG_NO, nr_ue_mac_inst->crnti, rbconfig->drb_ToAddModList, 0, NULL, NULL);
        nr_rlc_add_drb(nr_ue_mac_inst->crnti, rbconfig->drb_ToAddModList->list.array[0]->drb_Identity, rlc_rbconfig);
        struct NR_CellGroupConfig__rlc_BearerToAddModList rlc_toadd_list;
        rlc_toadd_list.list.count = 1;
        rlc_toadd_list.list.array = calloc(1, sizeof(NR_RLC_BearerConfig_t));
        rlc_toadd_list.list.array[0] = rlc_rbconfig;
        nr_rrc_mac_config_req_ue_logicalChannelBearer(0, &rlc_toadd_list, NULL);

        // free memory
        free_nr_noS1_bearer_config(&rbconfig, &rlc_rbconfig);
      }
    }
    else {
      LOG_I(MAC,"Running without CellGroupConfig\n");
      if(get_softmodem_params()->sa == 1) {
        int rc = rlc_module_init(0);
        AssertFatal(rc == 0, "%s: Could not initialize RLC layer\n", __FUNCTION__);
      }
    }

    return (nr_ue_mac_inst);
}

NR_UE_MAC_INST_t *get_mac_inst(module_id_t module_id){
    return &nr_ue_mac_inst[(int)module_id];
}
