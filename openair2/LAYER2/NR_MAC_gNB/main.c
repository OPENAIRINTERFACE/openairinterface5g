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
#include "executables/nr-softmodem.h"

extern RAN_CONTEXT_t RC;

void set_cset_offset(uint16_t offset_bits) {
  RC.nrmac[0]->coreset[0][1].frequency_domain_resources >>= offset_bits;
}

void nr_init_coreset(nfapi_nr_coreset_t *coreset) {

  coreset->coreset_id = 1;
  coreset->frequency_domain_resources = 0x1E0000000000;//0x1FFFE0000000; // 96 RB starting from CRB0
  coreset->duration = 2;
  coreset->cce_reg_mapping_type = NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED;
  coreset->reg_bundle_size = 6;
  coreset->interleaver_size = 2;
  coreset->precoder_granularity = NFAPI_NR_CSET_SAME_AS_REG_BUNDLE;
  coreset->tci_present_in_dci = 0;
  coreset->dmrs_scrambling_id = 0;
}

void nr_init_search_space(nfapi_nr_search_space_t *search_space)
{
  search_space->search_space_id = 1;
  search_space->coreset_id = 1;
  search_space->search_space_type = NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC;
  search_space->duration = 5;
  search_space->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL10;
  search_space->slot_monitoring_offset = 1;
  search_space->monitoring_symbols_in_slot = 0x3000; // 14 bits field
  search_space->css_formats_0_0_and_1_0 = 1;
  search_space->uss_dci_formats = 0; // enum to be defined-- formats 0.0 and 1.0
  for (int i=0; i<NFAPI_NR_MAX_NB_CCE_AGGREGATION_LEVELS; i++)
    search_space->number_of_candidates[i] = 4; // TODO
}

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

	  //FK changed UL_req to UL_tti_req, which does not contain a pointer to the pdu_list
          //RC.nrmac[i]->UL_req[j].ul_config_request_body.ul_config_pdu_list = RC.nrmac[i]->ul_config_pdu_list[j];
          
          //for (int k = 0; k < 10; k++)
          //  RC.nrmac[i]->UL_req_tmp[j][k].ul_config_request_body.ul_config_pdu_list =RC.nrmac[i]->ul_config_pdu_list_tmp[j][k];
        
	  RC.nrmac[i]->HI_DCI0_req[j].hi_dci0_request_body.hi_dci0_pdu_list = RC.nrmac[i]->hi_dci0_pdu_list[j];
	  RC.nrmac[i]->TX_req[j].tx_request_body.tx_pdu_list =                RC.nrmac[i]->tx_request_pdu[j];
	  RC.nrmac[i]->ul_handle = 0;

	  // Init PDCCH structures
	  nr_init_coreset(&RC.nrmac[i]->coreset[j][1]);
	  nr_init_search_space(&RC.nrmac[i]->search_space[j][1]);
        }


    }//END for (i = 0; i < RC.nb_nr_macrlc_inst; i++)

  AssertFatal(rlc_module_init(1) == 0,"Could not initialize RLC layer\n");

  // These should be out of here later
  pdcp_layer_init();

  if(IS_SOFTMODEM_NOS1)
	  nr_ip_over_LTE_DRB_preconfiguration();

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
    /*memset(UE_list->DLSCH_pdu, 0, sizeof(UE_list->DLSCH_pdu));
    memset(UE_list->UE_template, 0, sizeof(UE_list->UE_template));
    memset(UE_list->eNB_UE_stats, 0, sizeof(UE_list->eNB_UE_stats));
    memset(UE_list->UE_sched_ctrl, 0, sizeof(UE_list->UE_sched_ctrl));
    memset(UE_list->active, 0, sizeof(UE_list->active));*/
  }

}
