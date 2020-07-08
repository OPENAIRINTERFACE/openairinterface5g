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

void ue_dci_configuration(NR_UE_MAC_INST_t *mac, fapi_nr_dl_config_request_t *dl_config, frame_t frame, int slot) {

  def_dci_pdu_rel15 = calloc(1,sizeof(dci_pdu_rel15_t));

  NR_BWP_Downlink_t *bwp;
  NR_BWP_DownlinkCommon_t *bwp_Common;
  NR_BWP_DownlinkDedicated_t *dl_bwp_Dedicated;
  NR_SetupRelease_PDCCH_Config_t *pdcch_Config;
  NR_SetupRelease_PDCCH_ConfigCommon_t *pdcch_ConfigCommon;
  struct NR_PDCCH_Config__controlResourceSetToAddModList *controlResourceSetToAddModList;
  struct NR_PDCCH_Config__searchSpacesToAddModList *searchSpacesToAddModList;
  struct NR_UplinkConfig__uplinkBWP_ToAddModList *uplinkBWP_ToAddModList;
  NR_SearchSpace_t *NR_SearchSpace;
  NR_ControlResourceSet_t *coreset;

  fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15;
  NR_ServingCellConfig_t *scd = mac->scg->spCellConfig->spCellConfigDedicated;

  uint8_t bwp_id = 1;
  int ss_id, sps;

  // get PDCCH configuration(s) (Coreset, SearchSpace, etc...) from BWP Dedicated in serving cell config dedicated (scd)
  // get BWP 1, Coreset ID 0, SearchSpace ID 0 (i.e. configured in MIB and in ServingCellConfigCommon)

  // Check dedicated DL BWP
  if (mac->DLbwp[0] == NULL) {
    NR_SearchSpace_t *ss;
    int ss_id = 0;

    AssertFatal(scd->downlinkBWP_ToAddModList != NULL, "downlinkBWP_ToAddModList is null\n");
    AssertFatal(scd->downlinkBWP_ToAddModList->list.count == 1, "downlinkBWP_ToAddModList->list->count is %d\n", scd->downlinkBWP_ToAddModList->list.count);
    mac->DLbwp[0] = scd->downlinkBWP_ToAddModList->list.array[bwp_id - 1];

    dl_bwp_Dedicated = mac->DLbwp[0]->bwp_Dedicated;
    AssertFatal(dl_bwp_Dedicated != NULL, "dl_bwp_Dedicated is null\n");

    bwp_Common = mac->DLbwp[0]->bwp_Common;
    AssertFatal(bwp_Common != NULL, "bwp_Common is null\n");

    pdcch_Config = dl_bwp_Dedicated->pdcch_Config;
    AssertFatal(pdcch_Config != NULL, "pdcch_Config is null\n");

    pdcch_ConfigCommon = bwp_Common->pdcch_ConfigCommon;
    AssertFatal(pdcch_ConfigCommon != NULL, "pdcch_ConfigCommon is null\n");
    AssertFatal(pdcch_ConfigCommon->choice.setup->ra_SearchSpace != NULL, "ra_SearchSpace must be available in DL BWP\n");

    controlResourceSetToAddModList = pdcch_Config->choice.setup->controlResourceSetToAddModList;
    AssertFatal(controlResourceSetToAddModList != NULL, "controlResourceSetToAddModList is null\n");

    AssertFatal(controlResourceSetToAddModList->list.count == 1, "controlResourceSetToAddModList->list.count=%d\n", controlResourceSetToAddModList->list.count);

    mac->coreset[0][0] = controlResourceSetToAddModList->list.array[0];
    coreset = mac->coreset[0][0];
    AssertFatal(coreset != NULL, "coreset[0][0] is null\n");
    
    searchSpacesToAddModList = pdcch_Config->choice.setup->searchSpacesToAddModList;
    AssertFatal(searchSpacesToAddModList != NULL, "searchSpacesToAddModList is null\n");
    AssertFatal(searchSpacesToAddModList->list.count > 0, "searchSpacesToAddModList is empty\n");
    AssertFatal(searchSpacesToAddModList->list.count < FAPI_NR_MAX_SS_PER_CORESET, "too many searchpaces per coreset %d\n", searchSpacesToAddModList->list.count);

    for (int i = 0; i < searchSpacesToAddModList->list.count; i++) {
      ss = searchSpacesToAddModList->list.array[i];
      AssertFatal(ss->controlResourceSetId != NULL, "ss->controlResourceSetId is null\n");
      AssertFatal(ss->searchSpaceType != NULL, "ss->searchSpaceType is null\n");
      AssertFatal(*ss->controlResourceSetId == coreset->controlResourceSetId, "ss->controlResourceSetId is unknown\n");
      mac->SSpace[0][0][ss_id] = ss;
      ss_id++;
    }
  }

  // Check dedicated UL BWP
  if (mac->ULbwp[0] == NULL) {
    uplinkBWP_ToAddModList = scd->uplinkConfig->uplinkBWP_ToAddModList;
    AssertFatal(uplinkBWP_ToAddModList != NULL, "uplinkBWP_ToAddModList is null\n");
    AssertFatal(uplinkBWP_ToAddModList->list.count == 1, "uplinkBWP_ToAddModList->list->count is %d\n", uplinkBWP_ToAddModList->list.count);
    mac->ULbwp[0] = uplinkBWP_ToAddModList->list.array[bwp_id - 1];
    AssertFatal(mac->ULbwp[0]->bwp_Dedicated != NULL, "UL bwp_Dedicated is null\n");
  }

  bwp = mac->DLbwp[0];
  bwp_Common = bwp->bwp_Common;
  pdcch_ConfigCommon = bwp_Common->pdcch_ConfigCommon;
  dl_bwp_Dedicated = bwp->bwp_Dedicated;
  pdcch_Config = dl_bwp_Dedicated->pdcch_Config;
  searchSpacesToAddModList = pdcch_Config->choice.setup->searchSpacesToAddModList;

  // check USSs
  ss_id = 0;
  NR_SearchSpace = mac->SSpace[0][0][ss_id];
  while(NR_SearchSpace != NULL && ss_id < searchSpacesToAddModList->list.count) {
    AssertFatal(NR_SearchSpace->monitoringSymbolsWithinSlot != NULL, "NR_SearchSpace->monitoringSymbolsWithinSlot is null\n");
    AssertFatal(NR_SearchSpace->monitoringSymbolsWithinSlot->buf != NULL, "NR_SearchSpace->monitoringSymbolsWithinSlot->buf is null\n");
    ss_id++;
  }

  if (mac->crnti > 0) {

    NR_SearchSpace_t *css;
    NR_SearchSpace_t *uss = NULL;
    NR_ServingCellConfigCommon_t *scc;
    NR_SearchSpaceId_t ra_SearchSpaceId;
    rel15 = &dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15;
    uint16_t monitoringSymbolsWithinSlot;
    uint8_t add_dci = 1;

    AssertFatal(mac->scc != NULL, "scc is null\n");

    scc = mac->scc;
    rel15->dci_format = NR_DL_DCI_FORMAT_1_1;

    // CoReSet configuration
    coreset = mac->coreset[0][0];
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

    if (mac->ra_state == WAIT_RAR){

       NR_BWP_DownlinkCommon_t *initialDownlinkBWP = scc->downlinkConfigCommon->initialDownlinkBWP;
       struct NR_PDCCH_ConfigCommon__commonSearchSpaceList *commonSearchSpaceList = pdcch_ConfigCommon->choice.setup->commonSearchSpaceList;
       AssertFatal(commonSearchSpaceList->list.count > 0, "PDCCH CSS list has 0 elements\n");

       ra_SearchSpaceId = *pdcch_ConfigCommon->choice.setup->ra_SearchSpace;

       // fetch the CSS for RA from the CSS list
       for (int i = 0; i < commonSearchSpaceList->list.count; i++) {
         css = commonSearchSpaceList->list.array[i];
         if(css->searchSpaceId == ra_SearchSpaceId)
           break;
       }

       // check CSSs
       if(css != NULL) {
         AssertFatal(css->monitoringSymbolsWithinSlot != NULL, "css->monitoringSymbolsWithinSlot is null\n");
         AssertFatal(css->monitoringSymbolsWithinSlot->buf != NULL, "css->monitoringSymbolsWithinSlot->buf is null\n");
       }

      if (frame == mac->msg2_rx_frame && slot == mac->msg2_rx_slot){
        sps = initialDownlinkBWP->genericParameters.cyclicPrefix == NULL ? 14 : 12;
        monitoringSymbolsWithinSlot = (css->monitoringSymbolsWithinSlot->buf[0]<<(sps-8)) | (css->monitoringSymbolsWithinSlot->buf[1]>>(16-sps));
        rel15->rnti = mac->ra_rnti;
        rel15->BWPSize = NRRIV2BW(initialDownlinkBWP->genericParameters.locationAndBandwidth, 275);
        rel15->BWPStart = NRRIV2PRBOFFSET(bwp_Common->genericParameters.locationAndBandwidth, 275); // NRRIV2PRBOFFSET(initialDownlinkBWP->genericParameters.locationAndBandwidth, 275);
        rel15->SubcarrierSpacing = initialDownlinkBWP->genericParameters.subcarrierSpacing;
        rel15->dci_length = nr_dci_size(scc,mac->scg,def_dci_pdu_rel15,rel15->dci_format,NR_RNTI_C,rel15->BWPSize,bwp_id);
        for (int i = 0; i < sps; i++)
        if ((monitoringSymbolsWithinSlot >> (sps - 1 - i)) & 1) {
          rel15->coreset.StartSymbolIndex = i;
          break;
        }
        fill_dci_search_candidates(css, rel15);
      } else {
        add_dci = 0;
      }
    } else if (mac->ra_state == WAIT_CONTENTION_RESOLUTION){

      rel15->rnti = mac->t_crnti;

    } else {

      rel15->rnti = mac->crnti;
      rel15->BWPSize = NRRIV2BW(bwp_Common->genericParameters.locationAndBandwidth, 275);
      rel15->BWPStart = NRRIV2PRBOFFSET(bwp_Common->genericParameters.locationAndBandwidth, 275);
      rel15->SubcarrierSpacing = bwp_Common->genericParameters.subcarrierSpacing;
      rel15->dci_length = nr_dci_size(scc,mac->scg,def_dci_pdu_rel15,rel15->dci_format,NR_RNTI_C,rel15->BWPSize,bwp_id);
      // get UE-specific search space
      for (ss_id = 0; ss_id < FAPI_NR_MAX_SS_PER_CORESET && mac->SSpace[0][0][ss_id] != NULL; ss_id++){
        uss = mac->SSpace[0][0][ss_id];
        if (uss->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_ue_Specific) break;
      }
      AssertFatal(ss_id < FAPI_NR_MAX_SS_PER_CORESET, "couldn't find a UE-specific SS\n");
      sps = bwp_Common->genericParameters.cyclicPrefix == NULL ? 14 : 12;
      // for SPS=14 8 MSBs in positions 13 down to 6
      monitoringSymbolsWithinSlot = (uss->monitoringSymbolsWithinSlot->buf[0]<<(sps-8)) | (uss->monitoringSymbolsWithinSlot->buf[1]>>(16-sps));
      for (int i = 0; i < sps; i++)
        if ((monitoringSymbolsWithinSlot >> (sps - 1 - i)) & 1) {
          rel15->coreset.StartSymbolIndex = i;
          break;
        }
      fill_dci_search_candidates(uss, rel15);
    }

    // Scrambling RNTI
    if (coreset->pdcch_DMRS_ScramblingID) {
      rel15->coreset.pdcch_dmrs_scrambling_id = *coreset->pdcch_DMRS_ScramblingID;
      rel15->coreset.scrambling_rnti = mac->t_crnti;
    } else {
      rel15->coreset.pdcch_dmrs_scrambling_id = *scc->physCellId;
      rel15->coreset.scrambling_rnti = 0;
    }

    if (add_dci){
      dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DCI;
      dl_config->number_pdus = dl_config->number_pdus + 1;
    }
  }
}
