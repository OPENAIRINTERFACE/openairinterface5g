/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file gNB_scheduler_phytest.c
 * \brief gNB scheduling procedures in phy_test mode
 * \author  Guy De Souza
 * \date 07/2018
 * \email: desouza@eurecom.fr
 * \version 1.0
 * @ingroup _mac
 */

#include "nr_mac_gNB.h"
#include "SCHED_NR/sched_nr.h"

extern RAN_CONTEXT_t RC;

/*Scheduling of DLSCH with associated DCI in common search space
 * current version has only a DCI for type 1 PDCCH for RA-RNTI*/
void nr_schedule_css_dlsch_phytest(module_id_t   module_idP,
                                   frame_t       frameP,
                                   sub_frame_t   subframeP)
{
  uint8_t  CC_id;

  gNB_MAC_INST                        *nr_mac      = RC.nrmac[module_idP];
  //NR_COMMON_channels_t                *cc           = nr_mac->common_channels;
  nfapi_nr_dl_config_request_body_t   *dl_req;
  nfapi_nr_dl_config_request_pdu_t  *dl_config_pdu;
  nfapi_tx_request_pdu_t            *TX_req;
  uint16_t sfn_sf = frameP << 4 | subframeP;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    LOG_I(MAC, "Scheduling common search space DCI type 1 for CC_id %d\n",CC_id);

    PHY_VARS_gNB                *gNB      = RC.gNB[module_idP][CC_id];
    nfapi_nr_config_request_t     *cfg   = &gNB->gNB_config;
    NR_DL_FRAME_PARMS             *fp    = &gNB->frame_parms;

    dl_req = &nr_mac->DL_req[CC_id].dl_config_request_body;
    dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
    memset((void*)dl_config_pdu,0,sizeof(nfapi_nr_dl_config_request_pdu_t));
    dl_config_pdu->pdu_type = NFAPI_NR_DL_CONFIG_DCI_DL_PDU_TYPE;
    dl_config_pdu->pdu_size = (uint8_t)(2+sizeof(nfapi_nr_dl_config_dci_dl_pdu));

    nfapi_nr_dl_config_dci_dl_pdu_rel15_t *pdu_rel15 = &dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel15;
    nfapi_nr_dl_config_pdcch_parameters_rel15_t *params_rel15 = &dl_config_pdu->dci_dl_pdu.pdcch_params_rel15;

    nr_configure_css_dci_from_mib(&gNB->pdcch_type0_params,
                               kHz30, kHz30, nr_FR1, 0, 0,
                               fp->slots_per_frame,
                               cfg->rf_config.dl_carrier_bandwidth.value);
    memcpy((void*)params_rel15, (void*)&gNB->pdcch_type0_params, sizeof(nfapi_nr_dl_config_pdcch_parameters_rel15_t));

    pdu_rel15->frequency_domain_assignment = 5;
    pdu_rel15->time_domain_assignment = 2;
    pdu_rel15->vrb_to_prb_mapping = 0;
    pdu_rel15->mcs = 12;
    pdu_rel15->tb_scaling = 1;
    LOG_I(MAC, "[gNB scheduler phytest] DCI type 1 payload: freq_alloc %d, time_alloc %d, vrb to prb %d, mcs %d tb_scaling %d\n",
                pdu_rel15->frequency_domain_assignment,
                pdu_rel15->time_domain_assignment,
                pdu_rel15->vrb_to_prb_mapping,
                pdu_rel15->mcs,
                pdu_rel15->tb_scaling);

    params_rel15->rnti = 0x03;
    params_rel15->rnti_type = NFAPI_NR_RNTI_RA;
    params_rel15->dci_format = NFAPI_NR_DL_DCI_FORMAT_1_0;
    //params_rel15->aggregation_level = 1;
    LOG_I(MAC, "DCI type 1 params: rmsi_pdcch_config %d, rnti %d, rnti_type %d, dci_format %d\n \
                coreset params: mux_pattern %d, n_rb %d, n_symb %d, rb_offset %d  \n \
                ss params : nb_ss_sets_per_slot %d, first symb %d, nb_slots %d, sfn_mod2 %d, first slot %d\n",
                0,
                params_rel15->rnti,
                params_rel15->rnti_type,
                params_rel15->dci_format,
                params_rel15->mux_pattern,
                params_rel15->n_rb,
                params_rel15->n_symb,
                params_rel15->rb_offset,
                params_rel15->nb_ss_sets_per_slot,
                params_rel15->first_symbol,
                params_rel15->nb_slots,
                params_rel15->sfn_mod2,
                params_rel15->first_slot);
  dl_req->number_dci++;
  dl_req->number_pdu++;

  TX_req = &nr_mac->TX_req[CC_id].tx_request_body.tx_pdu_list[nr_mac->TX_req[CC_id].tx_request_body.number_of_pdus];
  TX_req->pdu_length = 6;
  TX_req->pdu_index = nr_mac->pdu_index[CC_id]++;
  TX_req->num_segments = 1;
  TX_req->segments[0].segment_length = 8;
  //TX_req->segments[0].segment_data = (uint8_t*)pdu_rel15;
  nr_mac->TX_req[CC_id].tx_request_body.number_of_pdus++;
  nr_mac->TX_req[CC_id].sfn_sf = sfn_sf;
  nr_mac->TX_req[CC_id].tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
  nr_mac->TX_req[CC_id].header.message_id = NFAPI_TX_REQUEST;
    
  }
}
