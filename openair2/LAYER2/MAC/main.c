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

#include "defs.h"
#include "proto.h"
#include "extern.h"
#include "assertions.h"
#include "PHY_INTERFACE/extern.h"
#include "PHY/defs.h"
#include "SCHED/defs.h"
#include "LAYER2/PDCP_v10.1.0/pdcp.h"
#include "RRC/LITE/defs.h"
#include "UTIL/LOG/log.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

#include "SCHED/defs.h"


#include "common/ran_context.h"

extern RAN_CONTEXT_t RC;


void mac_top_init_eNB(void)
{

    module_id_t i, j;
    int list_el;
    UE_list_t *UE_list;
    eNB_MAC_INST *mac;

    LOG_I(MAC, "[MAIN] Init function start:nb_macrlc_inst=%d\n",
	  RC.nb_macrlc_inst);

    if (RC.nb_macrlc_inst > 0) {
	RC.mac =
	    (eNB_MAC_INST **) malloc16(RC.nb_macrlc_inst *
				       sizeof(eNB_MAC_INST *));
	AssertFatal(RC.mac != NULL,
		    "can't ALLOCATE %zu Bytes for %d eNB_MAC_INST with size %zu \n",
		    RC.nb_macrlc_inst * sizeof(eNB_MAC_INST *),
		    RC.nb_macrlc_inst, sizeof(eNB_MAC_INST));
	for (i = 0; i < RC.nb_macrlc_inst; i++) {
	    RC.mac[i] = (eNB_MAC_INST *) malloc16(sizeof(eNB_MAC_INST));
	    AssertFatal(RC.mac != NULL,
			"can't ALLOCATE %zu Bytes for %d eNB_MAC_INST with size %zu \n",
			RC.nb_macrlc_inst * sizeof(eNB_MAC_INST *),
			RC.nb_macrlc_inst, sizeof(eNB_MAC_INST));
	    LOG_D(MAC,
		  "[MAIN] ALLOCATE %zu Bytes for %d eNB_MAC_INST @ %p\n",
		  sizeof(eNB_MAC_INST), RC.nb_macrlc_inst, RC.mac);
	    bzero(RC.mac[i], sizeof(eNB_MAC_INST));
	    RC.mac[i]->Mod_id = i;
	    for (j = 0; j < MAX_NUM_CCs; j++) {
		RC.mac[i]->DL_req[j].dl_config_request_body.
		    dl_config_pdu_list = RC.mac[i]->dl_config_pdu_list[j];
		RC.mac[i]->UL_req[j].ul_config_request_body.
		    ul_config_pdu_list = RC.mac[i]->ul_config_pdu_list[j];
		for (int k = 0; k < 10; k++)
		    RC.mac[i]->UL_req_tmp[j][k].
			ul_config_request_body.ul_config_pdu_list =
			RC.mac[i]->ul_config_pdu_list_tmp[j][k];
		RC.mac[i]->HI_DCI0_req[j].
		    hi_dci0_request_body.hi_dci0_pdu_list =
		    RC.mac[i]->hi_dci0_pdu_list[j];
		RC.mac[i]->TX_req[j].tx_request_body.tx_pdu_list =
		    RC.mac[i]->tx_request_pdu[j];
		RC.mac[i]->ul_handle = 0;
	    }
	}

	AssertFatal(rlc_module_init() == 0,
		    "Could not initialize RLC layer\n");

	// These should be out of here later
	pdcp_layer_init();

	rrc_init_global_param();

    } else {
	RC.mac = NULL;
    }

    // Initialize Linked-List for Active UEs
    for (i = 0; i < RC.nb_macrlc_inst; i++) {
	mac = RC.mac[i];


	mac->if_inst = IF_Module_init(i);

	UE_list = &mac->UE_list;

	UE_list->num_UEs = 0;
	UE_list->head = -1;
	UE_list->head_ul = -1;
	UE_list->avail = 0;

	for (list_el = 0; list_el < NUMBER_OF_UE_MAX - 1; list_el++) {
	    UE_list->next[list_el] = list_el + 1;
	    UE_list->next_ul[list_el] = list_el + 1;
	}

	UE_list->next[list_el] = -1;
	UE_list->next_ul[list_el] = -1;
    }

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

    for (j = 0; j < NUMBER_OF_UE_MAX; j++) {
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
