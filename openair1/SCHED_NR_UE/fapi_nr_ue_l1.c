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

/* \file fapi_nr_ue_l1.c
 * \brief functions for NR UE FAPI-like interface
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#include <stdio.h>

#include "fapi_nr_ue_interface.h"
#include "fapi_nr_ue_l1.h"
#include "harq_nr.h"
//#include "PHY/phy_vars_nr_ue.h"

#include "PHY/defs_nr_UE.h"
#include "PHY/impl_defs_nr.h"

extern PHY_VARS_NR_UE ***PHY_vars_UE_g;

const char *dl_pdu_type[]={"DCI", "DLSCH", "RA_DLSCH", "SI_DLSCH", "P_DLSCH"};
const char *ul_pdu_type[]={"PRACH", "PUCCH", "PUSCH", "SRS"};

int8_t nr_ue_scheduled_response(nr_scheduled_response_t *scheduled_response){

  bool found = false;
  if(scheduled_response != NULL){

    module_id_t module_id = scheduled_response->module_id;
    uint8_t cc_id = scheduled_response->CC_id, thread_id;
    uint32_t i;
    int slot = scheduled_response->slot;

    // Note: we have to handle the thread IDs for this. To be revisited completely.
    thread_id = scheduled_response->thread_id;
    NR_UE_DLSCH_t *dlsch0 = NULL;
    NR_UE_PDCCH *pdcch_vars = PHY_vars_UE_g[module_id][cc_id]->pdcch_vars[thread_id][0];
    NR_UE_ULSCH_t *ulsch0 = PHY_vars_UE_g[module_id][cc_id]->ulsch[thread_id][0][0];
    NR_UE_PUCCH *pucch_vars = PHY_vars_UE_g[module_id][cc_id]->pucch_vars[thread_id][0];

    if(scheduled_response->dl_config != NULL){
      fapi_nr_dl_config_request_t *dl_config = scheduled_response->dl_config;

      pdcch_vars->nb_search_space = 0;

      for (i = 0; i < dl_config->number_pdus; ++i){
        AssertFatal(dl_config->number_pdus < FAPI_NR_DL_CONFIG_LIST_NUM,"dl_config->number_pdus %d out of bounds\n",dl_config->number_pdus);
        AssertFatal(dl_config->dl_config_list[i].pdu_type<=FAPI_NR_DL_CONFIG_TYPES,"pdu_type %d > 2\n",dl_config->dl_config_list[i].pdu_type);
        LOG_D(PHY, "In %s: frame %d slot %d received 1 DL %s PDU of %d total DL PDUs:\n",
              __FUNCTION__, scheduled_response->frame, slot, dl_pdu_type[dl_config->dl_config_list[i].pdu_type - 1], dl_config->number_pdus);

        if (dl_config->dl_config_list[i].pdu_type == FAPI_NR_DL_CONFIG_TYPE_DCI) {

          fapi_nr_dl_config_dci_dl_pdu_rel15_t *pdcch_config = &dl_config->dl_config_list[i].dci_config_pdu.dci_config_rel15;
          memcpy((void*)&pdcch_vars->pdcch_config[pdcch_vars->nb_search_space],(void*)pdcch_config,sizeof(*pdcch_config));
          pdcch_vars->nb_search_space = pdcch_vars->nb_search_space + 1;
          pdcch_vars->sfn = scheduled_response->frame;
          pdcch_vars->slot = slot;
          LOG_D(PHY,"Number of DCI SearchSpaces %d\n",pdcch_vars->nb_search_space);

        } else {

          fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu = &dl_config->dl_config_list[i].dlsch_config_pdu.dlsch_config_rel15;
          uint8_t current_harq_pid = dlsch_config_pdu->harq_process_nbr;

          if (dl_config->dl_config_list[i].pdu_type == FAPI_NR_DL_CONFIG_TYPE_DLSCH){
            dlsch0 = PHY_vars_UE_g[module_id][cc_id]->dlsch[thread_id][0][0];
          }
          else if (dl_config->dl_config_list[i].pdu_type == FAPI_NR_DL_CONFIG_TYPE_RA_DLSCH){
            dlsch0 = PHY_vars_UE_g[module_id][cc_id]->dlsch_ra[0];
            dlsch0->rnti_type = _RA_RNTI_;
            dlsch0->harq_processes[current_harq_pid]->status = ACTIVE;
          }
          else if (dl_config->dl_config_list[i].pdu_type == FAPI_NR_DL_CONFIG_TYPE_SI_DLSCH){
            dlsch0 = PHY_vars_UE_g[module_id][cc_id]->dlsch_SI[0];
            dlsch0->rnti_type = _SI_RNTI_;
            dlsch0->harq_processes[current_harq_pid]->status = ACTIVE;
          }

          dlsch0->current_harq_pid = current_harq_pid;
          dlsch0->active = 1;
          dlsch0->rnti = dl_config->dl_config_list[i].dlsch_config_pdu.rnti;

          LOG_D(PHY,"slot %d current_harq_pid = %d\n",slot, current_harq_pid);

          NR_DL_UE_HARQ_t *dlsch0_harq = dlsch0->harq_processes[current_harq_pid];
          if (dlsch0_harq){

            dlsch0_harq->BWPStart = dlsch_config_pdu->BWPStart;
            dlsch0_harq->BWPSize = dlsch_config_pdu->BWPSize;
            dlsch0_harq->nb_rb = dlsch_config_pdu->number_rbs;
            dlsch0_harq->start_rb = dlsch_config_pdu->start_rb;
            dlsch0_harq->nb_symbols = dlsch_config_pdu->number_symbols;
            dlsch0_harq->start_symbol = dlsch_config_pdu->start_symbol;
            dlsch0_harq->dlDmrsSymbPos = dlsch_config_pdu->dlDmrsSymbPos;
            dlsch0_harq->dmrsConfigType = dlsch_config_pdu->dmrsConfigType;
            dlsch0_harq->n_dmrs_cdm_groups = dlsch_config_pdu->n_dmrs_cdm_groups;
            dlsch0_harq->mcs = dlsch_config_pdu->mcs;
            dlsch0_harq->rvidx = dlsch_config_pdu->rv;
            dlsch0->g_pucch = dlsch_config_pdu->accumulated_delta_PUCCH;
            //get nrOfLayers from DCI info
            uint8_t Nl = 0;
            for (i = 0; i < 4; i++) {
              if (dlsch_config_pdu->dmrs_ports[i] >= i) Nl += 1;
            }
            dlsch0_harq->Nl = Nl;
            dlsch0_harq->mcs_table=dlsch_config_pdu->mcs_table;
            downlink_harq_process(dlsch0_harq, dlsch0->current_harq_pid, dlsch_config_pdu->ndi, dlsch_config_pdu->rv, dlsch0->rnti_type);
            if (dlsch0_harq->status != ACTIVE) {
              // dlsch0_harq->status not ACTIVE due to false retransmission
              // Reset the following flag to skip PDSCH procedures in that case and retrasmit harq status
              dlsch0->active = 0;
              update_harq_status(module_id,dlsch0->current_harq_pid,dlsch0_harq->ack);
            }
            /* PTRS */
            dlsch0_harq->PTRSFreqDensity = dlsch_config_pdu->PTRSFreqDensity;
            dlsch0_harq->PTRSTimeDensity = dlsch_config_pdu->PTRSTimeDensity;
            dlsch0_harq->PTRSPortIndex = dlsch_config_pdu->PTRSPortIndex;
            dlsch0_harq->nEpreRatioOfPDSCHToPTRS = dlsch_config_pdu->nEpreRatioOfPDSCHToPTRS;
            dlsch0_harq->PTRSReOffset = dlsch_config_pdu->PTRSReOffset;
            dlsch0_harq->pduBitmap = dlsch_config_pdu->pduBitmap;
            LOG_D(MAC, ">>>> \tdlsch0->g_pucch = %d\tdlsch0_harq.mcs = %d\n", dlsch0->g_pucch, dlsch0_harq->mcs);
          }
        }
      }
      dl_config->number_pdus = 0;
    }

    if (scheduled_response->ul_config != NULL){

      fapi_nr_ul_config_request_t *ul_config = scheduled_response->ul_config;
      int pdu_done = 0;
      pthread_mutex_lock(&ul_config->mutex_ul_config);
      LOG_D(PHY, "%d.%d ul S ul_config %p pdu_done %d number_pdus %d\n", scheduled_response->frame, slot, ul_config, pdu_done, ul_config->number_pdus);

      for (i = 0; i < ul_config->number_pdus; ++i){

        AssertFatal(ul_config->ul_config_list[i].pdu_type <= FAPI_NR_UL_CONFIG_TYPES,"pdu_type %d out of bounds\n",ul_config->ul_config_list[i].pdu_type);
        LOG_D(PHY, "In %s: processing %s PDU of %d total UL PDUs (ul_config %p) \n", __FUNCTION__, ul_pdu_type[ul_config->ul_config_list[i].pdu_type - 1], ul_config->number_pdus, ul_config);

        uint8_t pdu_type = ul_config->ul_config_list[i].pdu_type, current_harq_pid, gNB_id = 0;
        /* PRACH */
        //NR_PRACH_RESOURCES_t *prach_resources;
        fapi_nr_ul_config_prach_pdu *prach_config_pdu;
        /* PUSCH */
        nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu;
        /* PUCCH */
        fapi_nr_ul_config_pucch_pdu *pucch_config_pdu;
        LOG_D(PHY, "%d.%d ul B ul_config %p t %d pdu_done %d number_pdus %d\n", scheduled_response->frame, slot, ul_config, pdu_type, pdu_done, ul_config->number_pdus);
        /* SRS */
        fapi_nr_ul_config_srs_pdu *srs_config_pdu;

        switch (pdu_type){

        case (FAPI_NR_UL_CONFIG_TYPE_PUSCH):
          // pusch config pdu
          pusch_config_pdu = &ul_config->ul_config_list[i].pusch_config_pdu;
          current_harq_pid = pusch_config_pdu->pusch_data.harq_process_id;
          NR_UL_UE_HARQ_t *harq_process_ul_ue = ulsch0->harq_processes[current_harq_pid];
          harq_process_ul_ue->status = 0;

          if (harq_process_ul_ue){

            nfapi_nr_ue_pusch_pdu_t *pusch_pdu = &harq_process_ul_ue->pusch_pdu;

            memcpy(pusch_pdu, pusch_config_pdu, sizeof(nfapi_nr_ue_pusch_pdu_t));

            ulsch0->f_pusch = pusch_config_pdu->absolute_delta_PUSCH;

            if (scheduled_response->tx_request) {
              for (int j=0; j<scheduled_response->tx_request->number_of_pdus; j++) {
                fapi_nr_tx_request_body_t *tx_req_body = &scheduled_response->tx_request->tx_request_body[j];
                if ((tx_req_body->pdu_index == i) && (tx_req_body->pdu_length > 0)) {
                  LOG_D(PHY,"%d.%d Copying %d bytes to harq_process_ul_ue->a (harq_pid %d)\n",scheduled_response->frame,slot,tx_req_body->pdu_length,current_harq_pid);
                  memcpy(harq_process_ul_ue->a, tx_req_body->pdu, tx_req_body->pdu_length);
                  harq_process_ul_ue->status = ACTIVE;
                  ul_config->ul_config_list[i].pdu_type = FAPI_NR_UL_CONFIG_TYPE_DONE; // not handle it any more
                  pdu_done++;
                  LOG_D(PHY, "%d.%d ul A ul_config %p t %d pdu_done %d number_pdus %d\n", scheduled_response->frame, slot, ul_config, pdu_type, pdu_done, ul_config->number_pdus);
                  break;
                }
              }
            }

          } else {

            LOG_E(PHY, "[phy_procedures_nrUE_TX] harq_process_ul_ue is NULL !!\n");
            return -1;

          }

        break;

        case (FAPI_NR_UL_CONFIG_TYPE_PUCCH):
          found = false;
          pucch_config_pdu = &ul_config->ul_config_list[i].pucch_config_pdu;
          for(int j=0; j<2; j++) {
            if(pucch_vars->active[j] == false) {
              LOG_D(PHY,"%d.%d Copying pucch pdu to UE PHY\n",scheduled_response->frame,slot);
              memcpy((void*)&(pucch_vars->pucch_pdu[j]), (void*)pucch_config_pdu, sizeof(fapi_nr_ul_config_pucch_pdu));
              pucch_vars->active[j] = true;
              found = true;
              ul_config->ul_config_list[i].pdu_type = FAPI_NR_UL_CONFIG_TYPE_DONE; // not handle it any more
              pdu_done++;
              LOG_D(PHY, "%d.%d ul A ul_config %p t %d pdu_done %d number_pdus %d\n", scheduled_response->frame, slot, ul_config, pdu_type, pdu_done, ul_config->number_pdus);
              break;
            }
          }
          if (!found)
            LOG_E(PHY, "Couldn't find allocation for PUCCH PDU in PUCCH VARS\n");
        break;

        case (FAPI_NR_UL_CONFIG_TYPE_PRACH):
          // prach config pdu
          prach_config_pdu = &ul_config->ul_config_list[i].prach_config_pdu;
          memcpy((void*)&(PHY_vars_UE_g[module_id][cc_id]->prach_vars[gNB_id]->prach_pdu), (void*)prach_config_pdu, sizeof(fapi_nr_ul_config_prach_pdu));
          ul_config->ul_config_list[i].pdu_type = FAPI_NR_UL_CONFIG_TYPE_DONE; // not handle it any more
          pdu_done++;
          LOG_D(PHY, "%d.%d ul A ul_config %p t %d pdu_done %d number_pdus %d\n", scheduled_response->frame, slot, ul_config, pdu_type, pdu_done, ul_config->number_pdus);
        break;

        case (FAPI_NR_UL_CONFIG_TYPE_DONE):
          pdu_done++; // count the no of pdu processed
          LOG_D(PHY, "%d.%d ul A ul_config %p t %d pdu_done %d number_pdus %d\n", scheduled_response->frame, slot, ul_config, pdu_type, pdu_done, ul_config->number_pdus);
        break;

        case (FAPI_NR_UL_CONFIG_TYPE_SRS):
          // srs config pdu
          srs_config_pdu = &ul_config->ul_config_list[i].srs_config_pdu;
          memcpy((void*)&(PHY_vars_UE_g[module_id][cc_id]->srs_vars[gNB_id]->srs_config_pdu), (void*)srs_config_pdu, sizeof(fapi_nr_ul_config_srs_pdu));
          PHY_vars_UE_g[module_id][cc_id]->srs_vars[gNB_id]->active = true;
        break;

        default:
          ul_config->ul_config_list[i].pdu_type = FAPI_NR_UL_CONFIG_TYPE_DONE; // not handle it any more
          pdu_done++; // count the no of pdu processed
          LOG_D(PHY, "%d.%d ul A ul_config %p t %d pdu_done %d number_pdus %d\n", scheduled_response->frame, slot, ul_config, pdu_type, pdu_done, ul_config->number_pdus);
        break;
        }
      }

      //Clear the fields when all the config pdu are done
      if (pdu_done == ul_config->number_pdus) {
        if (scheduled_response->tx_request)
          scheduled_response->tx_request->number_of_pdus = 0;
        ul_config->sfn = 0;
        ul_config->slot = 0;
        ul_config->number_pdus = 0;
        LOG_D(PHY, "%d.%d clear ul_config %p\n", scheduled_response->frame, slot, ul_config);
        memset(ul_config->ul_config_list, 0, sizeof(ul_config->ul_config_list));
      }
      pthread_mutex_unlock(&ul_config->mutex_ul_config);
    }
  }
  return 0;
}




int8_t nr_ue_phy_config_request(nr_phy_config_t *phy_config){

  fapi_nr_config_request_t *nrUE_config = &PHY_vars_UE_g[phy_config->Mod_id][phy_config->CC_id]->nrUE_config;

  if(phy_config != NULL) {
      memcpy(nrUE_config,&phy_config->config_req,sizeof(fapi_nr_config_request_t));
      if (PHY_vars_UE_g[phy_config->Mod_id][phy_config->CC_id]->UE_mode[0] == NOT_SYNCHED)
	      PHY_vars_UE_g[phy_config->Mod_id][phy_config->CC_id]->UE_mode[0] = PRACH;
  }
  return 0;
}


