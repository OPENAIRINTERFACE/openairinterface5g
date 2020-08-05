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

/* \file nr_ue_dci_configuration.c
 * \brief functions for generating dci search procedures based on RRC Serving Cell Group Configuration
 * \author R. Knopp, G. Casati
 * \date 2020
 * \version 0.2
 * \company Eurecom, Fraunhofer IIS
 * \email: knopp@eurecom.fr, guido.casati@iis.fraunhofer.de
 * \note
 * \warning
 */

#include "mac_proto.h"
#include "mac_defs.h"
#include "assertions.h"
#include "LAYER2/NR_MAC_UE/mac_extern.h"
#include "mac_defs.h"
#include "common/utils/nr/nr_common.h"
#include "executables/softmodem-common.h"
#include <stdio.h>

#ifdef NR_PDCCH_DCI_TOOLS_DEBUG
#define LOG_DCI_D(a...) printf("\t\t<-NR_PDCCH_DCI_TOOLS_DEBUG (nr_extract_dci_info) ->" a)
#else 
#define LOG_DCI_D(a...)
#endif
#define LOG_DCI_PARM(a...) LOG_D(PHY,"\t<-NR_PDCCH_DCI_TOOLS_DEBUG (nr_generate_ue_ul_dlsch_params_from_dci)" a)

// #define DEBUG_DCI

dci_pdu_rel15_t *def_dci_pdu_rel15;

void fill_dci_search_candidates(NR_SearchSpace_t *ss,fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15) {

  LOG_D(MAC,"Filling search candidates for DCI\n");
  
  rel15->number_of_candidates=3;
  rel15->CCE[0]=0;
  rel15->L[0]=4;
  rel15->CCE[1]=4;
  rel15->L[1]=4;
  rel15->CCE[2]=8;
  rel15->L[2]=4;

}

void config_dci_pdu(NR_UE_MAC_INST_t *mac, fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15, fapi_nr_dl_config_request_t *dl_config, int rnti_type, int ss_id, uint8_t dci_format){

  uint16_t monitoringSymbolsWithinSlot;
  uint8_t bwp_id = 1, coreset_id = 1;
  int sps;
  def_dci_pdu_rel15 = calloc(1,sizeof(dci_pdu_rel15_t));
  AssertFatal(mac->scc != NULL, "scc is null\n");
  NR_ServingCellConfigCommon_t *scc = mac->scc;
  NR_BWP_DownlinkCommon_t *bwp_Common = mac->DLbwp[bwp_id - 1]->bwp_Common;
  NR_BWP_DownlinkCommon_t *initialDownlinkBWP = scc->downlinkConfigCommon->initialDownlinkBWP;
  NR_SearchSpace_t *ss = mac->SSpace[bwp_id - 1][coreset_id - 1][ss_id];

  // DCI format configuration
  rel15->dci_format = dci_format;

  // CORESET configuration
  NR_ControlResourceSet_t *coreset = mac->coreset[bwp_id - 1][coreset_id - 1];
  rel15->coreset.duration = coreset->duration;
  for (int i = 0; i < 6; i++)
    rel15->coreset.frequency_domain_resource[i] = coreset->frequencyDomainResources.buf[i];
  rel15->coreset.CceRegMappingType = coreset->cce_REG_MappingType.present == NR_ControlResourceSet__cce_REG_MappingType_PR_interleaved ? FAPI_NR_CCE_REG_MAPPING_TYPE_INTERLEAVED : FAPI_NR_CCE_REG_MAPPING_TYPE_NON_INTERLEAVED;
  if (rel15->coreset.CceRegMappingType == FAPI_NR_CCE_REG_MAPPING_TYPE_INTERLEAVED) {
    struct NR_ControlResourceSet__cce_REG_MappingType__interleaved *interleaved = coreset->cce_REG_MappingType.choice.interleaved;
    rel15->coreset.RegBundleSize = (interleaved->reg_BundleSize == NR_ControlResourceSet__cce_REG_MappingType__interleaved__reg_BundleSize_n6) ? 6 : (2 + interleaved->reg_BundleSize);
    rel15->coreset.InterleaverSize = (interleaved->interleaverSize == NR_ControlResourceSet__cce_REG_MappingType__interleaved__interleaverSize_n6) ? 6 : (2 + interleaved->interleaverSize);
    AssertFatal(scc->physCellId != NULL, "mac->scc->physCellId is null\n");
    rel15->coreset.ShiftIndex = interleaved->shiftIndex != NULL ? *interleaved->shiftIndex : *scc->physCellId;
  } else {
    rel15->coreset.RegBundleSize = 0;
    rel15->coreset.InterleaverSize = 0;
    rel15->coreset.ShiftIndex = 0;
  }
  rel15->coreset.CoreSetType = 1;
  rel15->coreset.precoder_granularity = coreset->precoderGranularity;

  // Scrambling RNTI
  if (coreset->pdcch_DMRS_ScramblingID) {
    rel15->coreset.pdcch_dmrs_scrambling_id = *coreset->pdcch_DMRS_ScramblingID;
    rel15->coreset.scrambling_rnti = mac->t_crnti;
  } else {
    rel15->coreset.pdcch_dmrs_scrambling_id = *scc->physCellId;
    rel15->coreset.scrambling_rnti = 0;
  }

  #ifdef DEBUG_DCI
    LOG_D(MAC, "[DCI_CONFIG] Configure DCI PDU: ss_id %d bwp %p bwp_Id %d controlResourceSetId %d\n", ss_id, mac->DLbwp[bwp_id - 1], mac->DLbwp[bwp_id - 1]->bwp_Id, coreset->controlResourceSetId);
  #endif

  // loop over RNTI type and configure resource allocation for DCI
  switch(rnti_type) {
    case NR_RNTI_C:
    // we use DL BWP dedicated
    sps = bwp_Common->genericParameters.cyclicPrefix == NULL ? 14 : 12;
    // for SPS=14 8 MSBs in positions 13 down to 6
    monitoringSymbolsWithinSlot = (ss->monitoringSymbolsWithinSlot->buf[0]<<(sps-8)) | (ss->monitoringSymbolsWithinSlot->buf[1]>>(16-sps));
    rel15->rnti = mac->crnti;
    rel15->BWPSize = NRRIV2BW(bwp_Common->genericParameters.locationAndBandwidth, 275);
    rel15->BWPStart = NRRIV2PRBOFFSET(bwp_Common->genericParameters.locationAndBandwidth, 275);
    rel15->SubcarrierSpacing = bwp_Common->genericParameters.subcarrierSpacing;
    rel15->dci_length = nr_dci_size(scc, mac->scg, def_dci_pdu_rel15, rel15->dci_format, NR_RNTI_C, rel15->BWPSize, bwp_id);
    break;
    case NR_RNTI_RA:
    // we use the initial DL BWP
    sps = initialDownlinkBWP->genericParameters.cyclicPrefix == NULL ? 14 : 12;
    monitoringSymbolsWithinSlot = (ss->monitoringSymbolsWithinSlot->buf[0]<<(sps-8)) | (ss->monitoringSymbolsWithinSlot->buf[1]>>(16-sps));
    rel15->rnti = mac->ra_rnti;
    rel15->BWPSize = NRRIV2BW(initialDownlinkBWP->genericParameters.locationAndBandwidth, 275);
    rel15->BWPStart = NRRIV2PRBOFFSET(bwp_Common->genericParameters.locationAndBandwidth, 275); //NRRIV2PRBOFFSET(initialDownlinkBWP->genericParameters.locationAndBandwidth, 275);
    rel15->SubcarrierSpacing = initialDownlinkBWP->genericParameters.subcarrierSpacing;
    rel15->dci_length = nr_dci_size(scc, mac->scg, def_dci_pdu_rel15, rel15->dci_format, NR_RNTI_RA, rel15->BWPSize, bwp_id);
    break;
    case NR_RNTI_P:
    break;
    case NR_RNTI_CS:
    break;
    case NR_RNTI_TC:
    break;
    case NR_RNTI_SP_CSI:
    break;
    case NR_RNTI_SI:
    break;
    case NR_RNTI_SFI:
    break;
    case NR_RNTI_INT:
    break;
    case NR_RNTI_TPC_PUSCH:
    break;
    case NR_RNTI_TPC_PUCCH:
    break;
    case NR_RNTI_TPC_SRS:
    break;
    default:
    break;
  }
  for (int i = 0; i < sps; i++) {
    if ((monitoringSymbolsWithinSlot >> (sps - 1 - i)) & 1) {
      rel15->coreset.StartSymbolIndex = i;
      break;
    }
  }
  #ifdef DEBUG_DCI
    LOG_D(MAC, "[DCI_CONFIG] Configure DCI PDU: rnti_type %d BWPSize %d BWPStart %d rel15->SubcarrierSpacing %d rel15->dci_format %d rel15->dci_length %d sps %d monitoringSymbolsWithinSlot %d \n",
      rnti_type,
      rel15->BWPSize,
      rel15->BWPStart,
      rel15->SubcarrierSpacing,
      rel15->dci_format,
      rel15->dci_length,
      sps,
      monitoringSymbolsWithinSlot);
  #endif
  // add DCI
  dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DCI;
  dl_config->number_pdus = dl_config->number_pdus + 1;
}

void ue_dci_configuration(NR_UE_MAC_INST_t *mac, fapi_nr_dl_config_request_t *dl_config, frame_t frame, int slot) {

  int ss_id;
  uint8_t bwp_id = 1, coreset_id = 1;
  NR_ServingCellConfig_t *scd = mac->scg->spCellConfig->spCellConfigDedicated;
  NR_BWP_Downlink_t *bwp = mac->DLbwp[bwp_id - 1];

  #ifdef DEBUG_DCI
    LOG_D(MAC, "[DCI_CONFIG] ra_rnti %p (%x) crnti %p (%x) t_crnti %p (%x)\n", &mac->ra_rnti, mac->ra_rnti, &mac->crnti, mac->crnti, &mac->t_crnti, mac->t_crnti);
  #endif

  // loop over all available SS for BWP ID 1, CORESET ID 1
  for (ss_id = 0; ss_id < FAPI_NR_MAX_SS_PER_CORESET && mac->SSpace[bwp_id - 1][coreset_id - 1][ss_id] != NULL; ss_id++){
    NR_SearchSpace_t *ss = mac->SSpace[bwp_id - 1][coreset_id - 1][ss_id];
    fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15 = &dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15;
    NR_BWP_DownlinkCommon_t *bwp_Common = bwp->bwp_Common;
    NR_SetupRelease_PDCCH_ConfigCommon_t *pdcch_ConfigCommon = bwp_Common->pdcch_ConfigCommon;
    struct NR_PhysicalCellGroupConfig *phy_cgc = mac->scg->physicalCellGroupConfig;
    switch (ss->searchSpaceType->present){
      case NR_SearchSpace__searchSpaceType_PR_common:
      // this is for CSSs, we use BWP common and pdcch_ConfigCommon

      // Fetch configuration for searchSpaceZero
      // note: The search space with the SearchSpaceId = 0 identifies the search space configured via PBCH (MIB) and in ServingCellConfigCommon (searchSpaceZero).
      if (pdcch_ConfigCommon->choice.setup->searchSpaceZero){
        LOG_D(MAC, "[DCI_CONFIG] Configure SearchSpace#0 of the initial BWP\n");
        LOG_W(MAC, "[DCI_CONFIG] This should not be available yet...");
      }
      if (ss->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0){
        // check available SS IDs
        if (pdcch_ConfigCommon->choice.setup->ra_SearchSpace){
          if (ss->searchSpaceId == *pdcch_ConfigCommon->choice.setup->ra_SearchSpace){
            LOG_D(MAC, "[DCI_CONFIG] Configure monitoring of PDCCH candidates in Type1-PDCCH common random access search space\n");
            switch(mac->ra_state){
              case WAIT_RAR:
              config_dci_pdu(mac, rel15, dl_config, NR_RNTI_RA, ss_id, NR_DL_DCI_FORMAT_1_0);
              fill_dci_search_candidates(ss, rel15);
              break;
              case WAIT_CONTENTION_RESOLUTION:
              rel15->rnti = mac->t_crnti;
              break;
              default:
              break;
            }
          }
        }
        if (pdcch_ConfigCommon->choice.setup->searchSpaceSIB1){
          if (ss->searchSpaceId == *pdcch_ConfigCommon->choice.setup->searchSpaceSIB1){
            // Configure monitoring of PDCCH candidates in Type0-PDCCH common search space on the MCG
            LOG_W(MAC, "[DCI_CONFIG] This seach space should not be configured yet...");
          }
        }
        if (pdcch_ConfigCommon->choice.setup->searchSpaceOtherSystemInformation){
          if (ss->searchSpaceId == *pdcch_ConfigCommon->choice.setup->searchSpaceOtherSystemInformation){
            // Configure monitoring of PDCCH candidates in Type0-PDCCH common search space on the MCG
            LOG_W(MAC, "[DCI_CONFIG] This seach space should not be configured yet...");
          }
        }
        if (pdcch_ConfigCommon->choice.setup->pagingSearchSpace){
          if (ss->searchSpaceId == *pdcch_ConfigCommon->choice.setup->pagingSearchSpace){
            // Configure monitoring of PDCCH candidates in Type2-PDCCH common search space on the MCG
            LOG_W(MAC, "[DCI_CONFIG] This seach space should not be configured yet...");
          }
        }
        if (phy_cgc){
          if (phy_cgc->cs_RNTI){
            LOG_D(MAC, "[DCI_CONFIG] Configure monitoring of PDCCH candidates in Type3-PDCCH common search space for dci_Format0_0_AndFormat1_0 with CRC scrambled by CS-RNTI...\n");
            LOG_W(MAC, "[DCI_CONFIG] This RNTI should not be configured yet...");
          }
          if (phy_cgc->ext1){
            if (phy_cgc->ext1->mcs_C_RNTI){
            LOG_D(MAC, "[DCI_CONFIG] Configure monitoring of PDCCH candidates in user specific search space for dci_Format0_0_AndFormat1_0 with CRC scrambled by MCS-C-RNTI...\n");
            LOG_W(MAC, "[DCI_CONFIG] This RNTI should not be configured yet...");
            }
          }
        }
      } // end DCI 00 and 01
      // DCI 2_0
      if (ss->searchSpaceType->choice.common->dci_Format2_0){
        LOG_D(MAC, "[DCI_CONFIG] Configure monitoring of PDCCH candidates in Type3-PDCCH common search space for DCI format 2_0 with CRC scrambled by SFI-RNTI \n");
        LOG_W(MAC, "[DCI_CONFIG] This format should not be configured yet...");
      }
      // DCI 2_1
      if (ss->searchSpaceType->choice.common->dci_Format2_1){
        LOG_D(MAC, "[DCI_CONFIG] Configure monitoring of PDCCH candidates in Type3-PDCCH common search space for DCI format 2_1 with CRC scrambled by INT-RNTI \n");
        LOG_W(MAC, "[DCI_CONFIG] This format should not be configured yet...");
      }
      // DCI 2_2
      if (ss->searchSpaceType->choice.common->dci_Format2_2){
        LOG_D(MAC, "[DCI_CONFIG] Configure monitoring of PDCCH candidates in Type3-PDCCH common search space for DCI format 2_2 with CRC scrambled by TPC-RNTI \n");
        LOG_W(MAC, "[DCI_CONFIG] This format should not be configured yet...");
      }
      // DCI 2_3
      if (ss->searchSpaceType->choice.common->dci_Format2_3){
        LOG_D(MAC, "[DCI_CONFIG] Configure monitoring of PDCCH candidates in Type3-PDCCH common search space for DCI format 2_3 with CRC scrambled by TPC-SRS-RNTI \n");
        LOG_W(MAC, "[DCI_CONFIG] This format should not be configured yet...");
      }

      break;
      case NR_SearchSpace__searchSpaceType_PR_ue_Specific:
      // this is an USS
      if (ss->searchSpaceType->choice.ue_Specific){
        if(ss->searchSpaceType->choice.ue_Specific->dci_Formats == NR_SearchSpace__searchSpaceType__ue_Specific__dci_Formats_formats0_1_And_1_1){
          // Monitors DCI 01 and 11 scrambled with C-RNTI, or CS-RNTI(s), or SP-CSI-RNTI
          if (get_softmodem_params()->phy_test == 1 && mac->crnti > 0) {
            LOG_D(MAC, "[DCI_CONFIG] Configure monitoring of PDCCH candidates in the user specific search space\n");
            config_dci_pdu(mac, rel15, dl_config, NR_RNTI_C, ss_id, NR_DL_DCI_FORMAT_1_1);
            fill_dci_search_candidates(ss, rel15);

            #ifdef DEBUG_DCI
            LOG_D(MAC, "[DCI_CONFIG] ss %d ue_Specific %p searchSpaceType->present %d dci_Formats %d\n",
              ss_id,
              ss->searchSpaceType->choice.ue_Specific,
              ss->searchSpaceType->present,
              ss->searchSpaceType->choice.ue_Specific->dci_Formats);
            #endif
          }
          if (phy_cgc){
            if (phy_cgc->cs_RNTI){
              LOG_D(MAC, "[DCI_CONFIG] Configure monitoring of PDCCH candidates in user specific search space for dci_Format0_0_AndFormat1_0 with CRC scrambled by CS-RNTI...\n");
              LOG_W(MAC, "[DCI_CONFIG] This RNTI should not be configured yet...");
            }
            if (phy_cgc->sp_CSI_RNTI){
              LOG_D(MAC, "[DCI_CONFIG] Configure monitoring of PDCCH candidates in user specific search space for dci_Format0_0_AndFormat1_0 with CRC scrambled by SP-CSI-RNTI...\n");
              LOG_W(MAC, "[DCI_CONFIG] This RNTI should not be configured yet...");
            }
            if (phy_cgc->ext1){
              if (phy_cgc->ext1->mcs_C_RNTI){
                LOG_D(MAC, "[DCI_CONFIG] Configure monitoring of PDCCH candidates in user specific search space for dci_Format0_0_AndFormat1_0 with CRC scrambled by MCS-C-RNTI...\n");
                LOG_W(MAC, "[DCI_CONFIG] This RNTI should not be configured yet...");
              }
            }
          }
        }
      }
      break;
      default:
      AssertFatal(1 == 0, "[DCI_CONFIG] Unrecognized search space type...");
      break;
    }
  }
}
