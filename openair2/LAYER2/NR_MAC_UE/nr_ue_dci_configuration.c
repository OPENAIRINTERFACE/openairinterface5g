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
#include "nfapi_nr_interface.h"

void fill_dci_search_candidates(const NR_SearchSpace_t *ss,
                                fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15,
                                const uint32_t Y)
{
  LOG_D(NR_MAC,"Filling search candidates for DCI\n");

  int i = 0;
  for (int maxL = 16; maxL > 0; maxL >>= 1) {
    uint8_t aggregation, max_number_of_candidates;
    find_aggregation_candidates(&aggregation,
                                &max_number_of_candidates,
                                ss,
                                maxL);
    if (max_number_of_candidates == 0)
      continue;
    LOG_D(NR_MAC,"L %d, max number of candidates %d, aggregation %d\n", maxL, max_number_of_candidates, aggregation);
    int N_cce_sym = 0; // nb of rbs of coreset per symbol
    for (int f = 0; f < 6; f++) {
      for (int t = 0; t < 8; t++) {
        N_cce_sym += ((rel15->coreset.frequency_domain_resource[f] >> t) & 1);
      }
    }
    int N_cces = N_cce_sym * rel15->coreset.duration;
    if (N_cces < aggregation)
      continue;
    for (int j = 0; j < max_number_of_candidates; j++) {
      int first_cce = aggregation * ((Y + ((j * N_cces) / (aggregation * max_number_of_candidates)) + 0) % (N_cces / aggregation));
      LOG_D(NR_MAC,"Candidate %d of %d first_cce %d (L %d N_cces %d Y %d)\n", j, max_number_of_candidates, first_cce, aggregation, N_cces, Y);
      // to avoid storing more than one candidate with the same aggregation and starting CCE (duplicated candidate)
      bool duplicated = false;
      for (int k = 0; k < i; k++) {
        if (rel15->CCE[k] == first_cce && rel15->L[k] == aggregation) {
          duplicated = true;
          LOG_D(NR_MAC, "Candidate %d of %d is duplicated\n", j, max_number_of_candidates);
        }
      }
      if (!duplicated) {
        rel15->CCE[i] = first_cce;
        rel15->L[i] = aggregation;
        i++;
      }
    }
  }
  rel15->number_of_candidates = i;
}

NR_ControlResourceSet_t *ue_get_coreset(const NR_BWP_PDCCH_t *config, const int coreset_id)
{
  if (config->commonControlResourceSet && coreset_id == config->commonControlResourceSet->controlResourceSetId)
    return config->commonControlResourceSet;
  NR_ControlResourceSet_t *coreset = NULL;
  for (int i = 0; i < config->list_Coreset.count; i++) {
    if (config->list_Coreset.array[i]->controlResourceSetId == coreset_id) {
      coreset = config->list_Coreset.array[i];
      break;
    }
  }
  AssertFatal(coreset, "Couldn't find coreset with id %d\n", coreset_id);
  return coreset;
}


void config_dci_pdu(NR_UE_MAC_INST_t *mac,
                    fapi_nr_dl_config_request_t *dl_config,
                    const int rnti_type,
                    const int slot,
                    const NR_SearchSpace_t *ss)
{
  const NR_UE_DL_BWP_t *current_DL_BWP = mac->current_DL_BWP;
  const NR_UE_UL_BWP_t *current_UL_BWP = mac->current_UL_BWP;
  NR_BWP_Id_t dl_bwp_id = current_DL_BWP ? current_DL_BWP->bwp_id : 0;
  NR_BWP_PDCCH_t *pdcch_config = &mac->config_BWP_PDCCH[dl_bwp_id];

  fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15 = &dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15;

  const int coreset_id = *ss->controlResourceSetId;
  NR_ControlResourceSet_t *coreset;
  if(coreset_id > 0) {
    coreset = ue_get_coreset(pdcch_config, coreset_id);
    rel15->coreset.CoreSetType = NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG;
  } else {
    coreset = pdcch_config->coreset0;
    rel15->coreset.CoreSetType = NFAPI_NR_CSET_CONFIG_MIB_SIB1;
  }

  rel15->coreset.duration = coreset->duration;

  for (int i = 0; i < 6; i++)
    rel15->coreset.frequency_domain_resource[i] = coreset->frequencyDomainResources.buf[i];
  rel15->coreset.CceRegMappingType = coreset->cce_REG_MappingType.present ==
    NR_ControlResourceSet__cce_REG_MappingType_PR_interleaved ? FAPI_NR_CCE_REG_MAPPING_TYPE_INTERLEAVED : FAPI_NR_CCE_REG_MAPPING_TYPE_NON_INTERLEAVED;
  if (rel15->coreset.CceRegMappingType == FAPI_NR_CCE_REG_MAPPING_TYPE_INTERLEAVED) {
    struct NR_ControlResourceSet__cce_REG_MappingType__interleaved *interleaved = coreset->cce_REG_MappingType.choice.interleaved;
    rel15->coreset.RegBundleSize = (interleaved->reg_BundleSize == NR_ControlResourceSet__cce_REG_MappingType__interleaved__reg_BundleSize_n6) ? 6 : (2 + interleaved->reg_BundleSize);
    rel15->coreset.InterleaverSize = (interleaved->interleaverSize == NR_ControlResourceSet__cce_REG_MappingType__interleaved__interleaverSize_n6) ? 6 : (2 + interleaved->interleaverSize);
    rel15->coreset.ShiftIndex = interleaved->shiftIndex != NULL ? *interleaved->shiftIndex : mac->physCellId;
  } else {
    rel15->coreset.RegBundleSize = 0;
    rel15->coreset.InterleaverSize = 0;
    rel15->coreset.ShiftIndex = 0;
  }

  rel15->coreset.precoder_granularity = coreset->precoderGranularity;

  // Scrambling RNTI
  rel15->coreset.scrambling_rnti = 0;
  if (coreset->pdcch_DMRS_ScramblingID) {
    rel15->coreset.pdcch_dmrs_scrambling_id = *coreset->pdcch_DMRS_ScramblingID;
    if (ss->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_ue_Specific)
      rel15->coreset.scrambling_rnti = mac->crnti;
  } else {
    rel15->coreset.pdcch_dmrs_scrambling_id = mac->physCellId;
  }

  rel15->num_dci_options = (mac->ra.ra_state == WAIT_RAR || rnti_type == TYPE_SI_RNTI_) ? 1 : 2;

  if (ss->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_ue_Specific) {
    if (ss->searchSpaceType->choice.ue_Specific->dci_Formats ==
        NR_SearchSpace__searchSpaceType__ue_Specific__dci_Formats_formats0_0_And_1_0) {
      rel15->dci_format_options[0] = NR_DL_DCI_FORMAT_1_0;
      rel15->dci_format_options[1] = NR_UL_DCI_FORMAT_0_0;
    }
    else {
      rel15->dci_format_options[0] = NR_DL_DCI_FORMAT_1_1;
      rel15->dci_format_options[1] = NR_UL_DCI_FORMAT_0_1;
    }
  }
  else { // common
    AssertFatal(ss->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0,
                "Only supporting format 10 and 00 for common SS\n");
    rel15->dci_format_options[0] = NR_DL_DCI_FORMAT_1_0;
    rel15->dci_format_options[1] = NR_UL_DCI_FORMAT_0_0;
  }

  NR_UE_ServingCell_Info_t *sc_info = &mac->sc_info;
  // loop over RNTI type and configure resource allocation for DCI
  for (int i = 0; i < rel15->num_dci_options; i++) {
    rel15->ss_type_options[i] = ss->searchSpaceType->present;
    const int dci_format = rel15->dci_format_options[i];
    uint16_t alt_size = 0;
    if(current_DL_BWP) {
      // computing alternative size for padding
      dci_pdu_rel15_t temp_pdu;
      if(dci_format == NR_DL_DCI_FORMAT_1_0)
        alt_size = nr_dci_size(current_DL_BWP,
                               current_UL_BWP,
                               sc_info,
                               mac->pdsch_HARQ_ACK_Codebook,
                               &temp_pdu,
                               NR_UL_DCI_FORMAT_0_0,
                               rnti_type,
                               coreset,
                               dl_bwp_id,
                               ss->searchSpaceType->present,
                               mac->type0_PDCCH_CSS_config.num_rbs,
                               0);
      if(dci_format == NR_UL_DCI_FORMAT_0_0)
        alt_size = nr_dci_size(current_DL_BWP,
                               current_UL_BWP,
                               sc_info,
                               mac->pdsch_HARQ_ACK_Codebook,
                               &temp_pdu,
                               NR_DL_DCI_FORMAT_1_0,
                               rnti_type,
                               coreset,
                               dl_bwp_id,
                               ss->searchSpaceType->present,
                               mac->type0_PDCCH_CSS_config.num_rbs,
                               0);
    }

    rel15->dci_length_options[i] = nr_dci_size(current_DL_BWP,
                                               current_UL_BWP,
                                               sc_info,
                                               mac->pdsch_HARQ_ACK_Codebook,
                                               &mac->def_dci_pdu_rel15[dl_config->slot][dci_format],
                                               dci_format,
                                               rnti_type,
                                               coreset,
                                               dl_bwp_id,
                                               ss->searchSpaceType->present,
                                               mac->type0_PDCCH_CSS_config.num_rbs,
                                               alt_size);
  }

  rel15->BWPStart = coreset_id == 0 ? mac->type0_PDCCH_CSS_config.cset_start_rb : current_DL_BWP->BWPStart;
  rel15->BWPSize = coreset_id == 0 ? mac->type0_PDCCH_CSS_config.num_rbs : current_DL_BWP->BWPSize;

  uint16_t monitoringSymbolsWithinSlot = 0;
  int sps = 0;

  switch(rnti_type) {
    case TYPE_C_RNTI_:
      // we use DL BWP dedicated
      sps = current_DL_BWP->cyclicprefix ? 12 : 14;
      // for SPS=14 8 MSBs in positions 13 down to 6
      monitoringSymbolsWithinSlot = (ss->monitoringSymbolsWithinSlot->buf[0]<<(sps-8)) | (ss->monitoringSymbolsWithinSlot->buf[1]>>(16-sps));
      rel15->rnti = mac->crnti;
      rel15->SubcarrierSpacing = current_DL_BWP->scs;
      break;
    case TYPE_RA_RNTI_:
      // we use the initial DL BWP
      sps = current_DL_BWP->cyclicprefix == NULL ? 14 : 12;
      monitoringSymbolsWithinSlot = (ss->monitoringSymbolsWithinSlot->buf[0]<<(sps-8)) | (ss->monitoringSymbolsWithinSlot->buf[1]>>(16-sps));
      rel15->rnti = mac->ra.ra_rnti;
      rel15->SubcarrierSpacing = current_DL_BWP->scs;
      break;
    case TYPE_P_RNTI_:
      break;
    case TYPE_CS_RNTI_:
      break;
    case TYPE_TC_RNTI_:
      // we use the initial DL BWP
      sps = current_DL_BWP->cyclicprefix == NULL ? 14 : 12;
      monitoringSymbolsWithinSlot = (ss->monitoringSymbolsWithinSlot->buf[0]<<(sps-8)) | (ss->monitoringSymbolsWithinSlot->buf[1]>>(16-sps));
      rel15->rnti = mac->ra.t_crnti;
      rel15->SubcarrierSpacing = current_DL_BWP->scs;
      break;
    case TYPE_SP_CSI_RNTI_:
      break;
    case TYPE_SI_RNTI_:
      sps = 14;
      // for SPS=14 8 MSBs in positions 13 down to 6
      monitoringSymbolsWithinSlot = (ss->monitoringSymbolsWithinSlot->buf[0]<<(sps-8)) | (ss->monitoringSymbolsWithinSlot->buf[1]>>(16-sps));
      rel15->rnti = SI_RNTI; // SI-RNTI - 3GPP TS 38.321 Table 7.1-1: RNTI values
      rel15->SubcarrierSpacing = mac->mib->subCarrierSpacingCommon;
      if(mac->frequency_range == FR2)
        rel15->SubcarrierSpacing = mac->mib->subCarrierSpacingCommon + 2;
      break;
    case TYPE_SFI_RNTI_:
      break;
    case TYPE_INT_RNTI_:
      break;
    case TYPE_TPC_PUSCH_RNTI_:
      break;
    case TYPE_TPC_PUCCH_RNTI_:
      break;
    case TYPE_TPC_SRS_RNTI_:
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
  uint32_t Y = 0;
  if (ss->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_ue_Specific)
    Y = get_Y(ss, slot, rel15->rnti);
  fill_dci_search_candidates(ss, rel15, Y);

  #ifdef DEBUG_DCI
    for (int i = 0; i < rel15->num_dci_options; i++) {
      LOG_D(MAC, "[DCI_CONFIG] Configure DCI PDU: rnti_type %d BWPSize %d BWPStart %d rel15->SubcarrierSpacing %d rel15->dci_format %d rel15->dci_length %d sps %d monitoringSymbolsWithinSlot %d \n",
            rnti_type,
            rel15->BWPSize,
            rel15->BWPStart,
            rel15->SubcarrierSpacing,
            rel15->dci_format_options[i],
            rel15->dci_length_options[i],
            sps,
            monitoringSymbolsWithinSlot);
    }
  #endif
  // add DCI
  dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DCI;
  dl_config->number_pdus += 1;
}


void get_monitoring_period_offset(const NR_SearchSpace_t *ss, int *period, int *offset)
{
  switch(ss->monitoringSlotPeriodicityAndOffset->present) {
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1:
      *period = 1;
      *offset = 0;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2:
      *period = 2;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl2;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl4:
      *period = 4;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl4;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl5:
      *period = 5;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl5;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl8:
      *period = 8;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl8;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl10:
      *period = 10;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl10;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl16:
      *period = 16;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl16;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl20:
      *period = 20;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl20;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl40:
      *period = 40;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl40;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl80:
      *period = 80;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl80;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl160:
      *period = 160;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl160;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl320:
      *period = 320;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl320;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl640:
      *period = 640;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl640;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1280:
      *period = 1280;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl1280;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2560:
      *period = 2560;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl2560;
      break;
  default:
    AssertFatal(1==0,"Invalid monitoring slot periodicity value\n");
    break;
  }
}

bool is_ss_monitor_occasion(const int frame, const int slot, const int slots_per_frame, const NR_SearchSpace_t *ss)
{
  const int duration = ss->duration ? *ss->duration : 1;
  bool monitor = false;
  int period, offset;
  get_monitoring_period_offset(ss, &period, &offset);
  // The UE monitors PDCCH candidates for search space set ss for 'duration' consecutive slots
  for (int i = 0; i < duration; i++) {
    if (((frame * slots_per_frame + slot - offset - i) % period) == 0) {
      monitor = true;
      break;
    }
  }
  return monitor;
}

bool monitior_dci_for_other_SI(NR_UE_MAC_INST_t *mac,
                               const NR_SearchSpace_t *ss,
                               const int slots_per_frame,
                               const int frame,
                               const int slot)
{
  const struct NR_SI_SchedulingInfo *si_SchedulingInfo = mac->si_SchedulingInfo;
  // 5.2.2.3.2 in 331
  if (!si_SchedulingInfo)
    return false;
  const int si_window_slots = 5 << si_SchedulingInfo->si_WindowLength;
  const int abs_slot = frame * slots_per_frame + slot;
  const int bwp_id = mac->current_DL_BWP->bwp_id;
  for (int n = 0; n < si_SchedulingInfo->schedulingInfoList.list.count; n++) {
    struct NR_SchedulingInfo *sched_Info = si_SchedulingInfo->schedulingInfoList.list.array[n];
    if(mac->si_window_start == -1) {
      int x = n * si_window_slots;
      int T = 8 << sched_Info->si_Periodicity; // radio frame periodicity
      if ((frame % T) == (x / slots_per_frame) &&
          (x % slots_per_frame == 0))
        mac->si_window_start = abs_slot; // in terms of absolute slot number
    }
    if (mac->si_window_start == -1) {
      // out of window
      return false;
    }
    else if (abs_slot > mac->si_window_start + si_window_slots) {
      // window expired
      mac->si_window_start = -1;
      return false;
    }
    else {
      const int duration = ss->duration ? *ss->duration : 1;
      int period, offset;
      get_monitoring_period_offset(ss, &period, &offset);
      for (int i = 0; i < duration; i++) {
        if (((frame * slots_per_frame + slot - offset - i) % period) == 0) {
          int N = mac->ssb_list[bwp_id].nb_tx_ssb;
          int K = 0; // k_th transmitted SSB
          for (int i = 0; i < mac->mib_ssb; i++) {
            if(mac->ssb_list[bwp_id].tx_ssb[i].transmitted)
              K++;
          }
          // numbering current frame and slot in terms of monitoring occasions in window
          int current_monitor_occasion = ((abs_slot - mac->si_window_start) % period) +
                                         (duration * (abs_slot - mac->si_window_start) / period);
          if (current_monitor_occasion % N == K)
            return true;
          else
           return false;
        }
      }
    }
  }
  return false;
}


void ue_dci_configuration(NR_UE_MAC_INST_t *mac, fapi_nr_dl_config_request_t *dl_config, const frame_t frame, const int slot)
{
  const NR_UE_DL_BWP_t *current_DL_BWP = mac->current_DL_BWP;
  NR_BWP_Id_t dl_bwp_id = current_DL_BWP ? current_DL_BWP->bwp_id : 0;
  NR_BWP_PDCCH_t *pdcch_config = &mac->config_BWP_PDCCH[dl_bwp_id];
  int scs = current_DL_BWP ? current_DL_BWP->scs : get_softmodem_params()->numerology;
  const int slots_per_frame = nr_slots_per_frame[scs];
  if (mac->get_sib1) {
    int ssb_sc_offset_norm;
    if (mac->ssb_subcarrier_offset < 24 && mac->frequency_range == FR1)
      ssb_sc_offset_norm = mac->ssb_subcarrier_offset >> scs;
    else
      ssb_sc_offset_norm = mac->ssb_subcarrier_offset;
    uint16_t ssb_offset_point_a = (mac->ssb_start_subcarrier - ssb_sc_offset_norm) / 12;
    int ssb_start_symbol = get_ssb_start_symbol(mac->nr_band, scs, mac->mib_ssb);
    get_type0_PDCCH_CSS_config_parameters(&mac->type0_PDCCH_CSS_config,
                                          mac->mib_frame,
                                          mac->mib,
                                          slots_per_frame,
                                          ssb_sc_offset_norm,
                                          ssb_start_symbol,
                                          scs,
                                          mac->frequency_range,
                                          mac->nr_band,
                                          mac->mib_ssb,
                                          1, // If the UE is not configured with a periodicity, the UE assumes a periodicity of a half frame
                                          ssb_offset_point_a);
    if (pdcch_config->search_space_zero == NULL)
      pdcch_config->search_space_zero = calloc(1, sizeof(*pdcch_config->search_space_zero));
    if (pdcch_config->coreset0 == NULL)
      pdcch_config->coreset0 = calloc(1, sizeof(*pdcch_config->coreset0));
    fill_coresetZero(pdcch_config->coreset0, &mac->type0_PDCCH_CSS_config);
    fill_searchSpaceZero(pdcch_config->search_space_zero, slots_per_frame, &mac->type0_PDCCH_CSS_config);
    if (is_ss_monitor_occasion(frame, slot, slots_per_frame, pdcch_config->search_space_zero)) {
      LOG_D(NR_MAC, "Monitoring DCI for SIB1 in frame %d slot %d\n", frame, slot);
      config_dci_pdu(mac, dl_config, TYPE_SI_RNTI_, slot, pdcch_config->search_space_zero);
    }
  }
  if (mac->get_otherSI) {
    // If searchSpaceOtherSystemInformation is set to zero,
    // PDCCH monitoring occasions for SI message reception in SI-window
    // are same as PDCCH monitoring occasions for SIB1
    const NR_SearchSpace_t *ss = pdcch_config->otherSI_SS ? pdcch_config->otherSI_SS : pdcch_config->search_space_zero;
    // TODO configure SI-window
    if (monitior_dci_for_other_SI(mac, ss, slots_per_frame, frame, slot)) {
      LOG_D(NR_MAC, "Monitoring DCI for other SIs in frame %d slot %d\n", frame, slot);
      config_dci_pdu(mac, dl_config, TYPE_SI_RNTI_, slot, ss);
    }
  }
  if (mac->state == UE_PERFORMING_RA &&
      mac->ra.ra_state >= WAIT_RAR) {
    // if RA is ongoing use RA search space
    if (is_ss_monitor_occasion(frame, slot, slots_per_frame, pdcch_config->ra_SS)) {
      int rnti_type = mac->ra.ra_state == WAIT_RAR ? TYPE_RA_RNTI_ : TYPE_TC_RNTI_;
      config_dci_pdu(mac, dl_config, rnti_type, slot, pdcch_config->ra_SS);
    }
  }
  else if (mac->state == UE_CONNECTED) {
    for (int i = 0; i < pdcch_config->list_SS.count; i++) {
      NR_SearchSpace_t *ss = pdcch_config->list_SS.array[i];
      if (is_ss_monitor_occasion(frame, slot, slots_per_frame, ss))
        config_dci_pdu(mac, dl_config, TYPE_C_RNTI_, slot, ss);
    }
    if (pdcch_config->list_SS.count == 0 && pdcch_config->ra_SS) {
      // If the UE has not been provided a Type3-PDCCH CSS set or a USS set and
      // the UE has received a C-RNTI and has been provided a Type1-PDCCH CSS set,
      // the UE monitors PDCCH candidates for DCI format 0_0 and DCI format 1_0
      // with CRC scrambled by the C-RNTI in the Type1-PDCCH CSS set
      if (is_ss_monitor_occasion(frame, slot, slots_per_frame, pdcch_config->ra_SS))
        config_dci_pdu(mac, dl_config, TYPE_C_RNTI_, slot, pdcch_config->ra_SS);
    }
  }
}
