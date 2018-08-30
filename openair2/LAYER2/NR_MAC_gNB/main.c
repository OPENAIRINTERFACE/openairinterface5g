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
 * \author  Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
 * \date 2010 - 2014, 2018
 * \version 1.0
 * \company Eurecom, NTUST
 * \email: navid.nikaein@eurecom.fr, kroempa@gmail.com
 * @ingroup _mac

 */

#include "mac_proto.h"
#include "LAYER2/MAC/mac_extern.h" //temporary
#include "assertions.h"

#include "LAYER2/PDCP_v10.1.0/pdcp.h"
#include "RRC/NR/nr_rrc_defs.h"
#include "common/utils/LOG/log.h"
//#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

#include "common/ran_context.h"

extern RAN_CONTEXT_t RC;


void mac_top_init_gNB(void)
{
  module_id_t     i,j;
  int             list_el;
  UE_list_t       *UE_list;
  gNB_MAC_INST    *nrmac;

  LOG_I(MAC, "[MAIN] Init function start:nb_nr_macrlc_inst=%d\n",RC.nb_nr_macrlc_inst);

  if (RC.nb_nr_macrlc_inst > 0) {
    
    RC.nrmac = (gNB_MAC_INST **) malloc16(RC.nb_nr_macrlc_inst *sizeof(gNB_MAC_INST *));
     
    AssertFatal(RC.nrmac != NULL,"can't ALLOCATE %zu Bytes for %d gNB_MAC_INST with size %zu \n",
                RC.nb_nr_macrlc_inst * sizeof(gNB_MAC_INST *),
                RC.nb_nr_macrlc_inst, sizeof(gNB_MAC_INST));
  
    for (i = 0; i < RC.nb_nr_macrlc_inst; i++) {
        RC.nrmac[i] = (gNB_MAC_INST *) malloc16(sizeof(gNB_MAC_INST));
        
        AssertFatal(RC.nrmac != NULL,"can't ALLOCATE %zu Bytes for %d gNB_MAC_INST with size %zu \n",
                    RC.nb_nr_macrlc_inst * sizeof(gNB_MAC_INST *),
                    RC.nb_nr_macrlc_inst, sizeof(gNB_MAC_INST));
        
        LOG_D(MAC,"[MAIN] ALLOCATE %zu Bytes for %d gNB_MAC_INST @ %p\n",sizeof(gNB_MAC_INST), RC.nb_nr_macrlc_inst, RC.mac);
       
        bzero(RC.nrmac[i], sizeof(gNB_MAC_INST));
        
        RC.nrmac[i]->Mod_id = i;
        
        
        for (j = 0; j < MAX_NUM_CCs; j++) {
          RC.nrmac[i]->DL_req[j].dl_config_request_body.dl_config_pdu_list = RC.nrmac[i]->dl_config_pdu_list[j];
          RC.nrmac[i]->UL_req[j].ul_config_request_body.ul_config_pdu_list = RC.nrmac[i]->ul_config_pdu_list[j];
          
          for (int k = 0; k < 10; k++)
            RC.nrmac[i]->UL_req_tmp[j][k].ul_config_request_body.ul_config_pdu_list =RC.nrmac[i]->ul_config_pdu_list_tmp[j][k];
        
        RC.nrmac[i]->HI_DCI0_req[j].hi_dci0_request_body.hi_dci0_pdu_list = RC.nrmac[i]->hi_dci0_pdu_list[j];
        RC.nrmac[i]->TX_req[j].tx_request_body.tx_pdu_list =                RC.nrmac[i]->tx_request_pdu[j];
        RC.nrmac[i]->ul_handle = 0;     
        }
        

    }//END for (i = 0; i < RC.nb_nr_macrlc_inst; i++)

  AssertFatal(rlc_module_init() == 0,"Could not initialize RLC layer\n");

  // These should be out of here later
  pdcp_layer_init();

  rrc_init_nr_global_param();

  }else {
    RC.nrmac = NULL;
  }

  // Initialize Linked-List for Active UEs
  for (i = 0; i < RC.nb_nr_macrlc_inst; i++) {
    
    nrmac = RC.nrmac[i];
    nrmac->if_inst = NR_IF_Module_init(i);

    UE_list = &nrmac->UE_list;
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
  }

}
