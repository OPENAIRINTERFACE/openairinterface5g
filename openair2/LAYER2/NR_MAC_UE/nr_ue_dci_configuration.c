/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*
 * \brief functions for generating dci search procedures based on RRC Serving Cell Group Configuration
 */

#include "mac_proto.h"
#include "mac_defs.h"
#include "assertions.h"
#include "mac_defs.h"
#include "common/utils/nr/nr_common.h"
#include "executables/softmodem-common.h"
#include "RRC/NR_UE/rrc_proto.h"
#include <stdio.h>

static void fill_dci_search_candidates(const NR_SearchSpace_t *ss, fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15, const uint32_t Y)
{
  LOG_T(NR_MAC_DCI, "Filling search candidates for DCI\n");

  int i = 0;
  for (int maxL = 16; maxL > 0; maxL >>= 1) {
    int aggregation, max_number_of_candidates;
    find_aggregation_candidates(&aggregation, &max_number_of_candidates, ss, maxL);
    if (max_number_of_candidates == 0)
      continue;
    LOG_T(NR_MAC_DCI, "L %d, max number of candidates %d, aggregation %d\n", maxL, max_number_of_candidates, aggregation);
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
      LOG_T(NR_MAC_DCI,
            "Candidate %d of %d first_cce %d (L %d N_cces %d Y %d)\n",
            j,
            max_number_of_candidates,
            first_cce,
            aggregation,
            N_cces,
            Y);
      // to avoid storing more than one candidate with the same aggregation and starting CCE (duplicated candidate)
      bool duplicated = false;
      for (int k = 0; k < i; k++) {
        if (rel15->CCE[k] == first_cce && rel15->L[k] == aggregation) {
          duplicated = true;
          LOG_T(NR_MAC_DCI, "Candidate %d of %d is duplicated\n", j, max_number_of_candidates);
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

static NR_ControlResourceSet_t *ue_get_coreset(const NR_BWP_PDCCH_t *config, const int coreset_id)
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

static void config_dci_pdu(NR_UE_MAC_INST_t *mac,
                           fapi_nr_dl_config_request_t *dl_config,
                           const int rnti_type,
                           const int slot,
                           const NR_SearchSpace_t *ss)
{
  if (!mac->mib) {
    LOG_E(MAC, "Incoherent call to dci configuration while mib decode not yet available\n");
    return;
  }
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
    if (coreset->ext1 && coreset->ext1->rb_Offset_r16)
      rel15->coreset.rb_offset = *coreset->ext1->rb_Offset_r16;
    else {
      // first common RB of the first group of 6 PRBs has common RB index equal to
      // 6 * ⌈BWP_start / 6⌉ if rb-Offset is not provided
      int start_common = (current_DL_BWP->BWPStart + 5) / 6 * 6;
      rel15->coreset.rb_offset = start_common - current_DL_BWP->BWPStart;
    }
  } else {
    coreset = mac->coreset0;
    rel15->coreset.CoreSetType = NFAPI_NR_CSET_CONFIG_MIB_SIB1;
    rel15->coreset.rb_offset = 0;
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

  int temp_num_dci_options = (mac->ra.ra_state == nrRA_WAIT_RAR || rnti_type == TYPE_SI_RNTI_) ? 1 : 2;
  int dci_format[2] = {0};
  if (ss->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_ue_Specific) {
    if (ss->searchSpaceType->choice.ue_Specific->dci_Formats ==
        NR_SearchSpace__searchSpaceType__ue_Specific__dci_Formats_formats0_0_And_1_0) {
      dci_format[0] = NR_DL_DCI_FORMAT_1_0;
      dci_format[1] = NR_UL_DCI_FORMAT_0_0;
    }
    else {
      dci_format[0] = NR_DL_DCI_FORMAT_1_1;
      dci_format[1] = NR_UL_DCI_FORMAT_0_1;
    }
  }
  else { // common
    AssertFatal(ss->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0,
                "Only supporting format 10 and 00 for common SS\n");
    dci_format[0] = NR_DL_DCI_FORMAT_1_0;
    dci_format[1] = NR_UL_DCI_FORMAT_0_0;
  }

  NR_UE_ServingCell_Info_t *sc_info = &mac->sc_info;
  // loop over DCI options and configure resource allocation
  // need to configure mac->def_dci_pdu_rel15 for all possible format options
  for (int i = 0; i < temp_num_dci_options; i++) {
    rel15->ss_type_options[i] = ss->searchSpaceType->present;
    if (dci_format[i] == NR_DL_DCI_FORMAT_1_0 || dci_format[i] == NR_UL_DCI_FORMAT_0_0)
      rel15->dci_format_options[i] = NFAPI_NR_FORMAT_0_0_AND_1_0;
    else
      rel15->dci_format_options[i] = NFAPI_NR_FORMAT_0_1_AND_1_1;
    uint16_t alt_size = 0;
    if(current_DL_BWP) {
      // computing alternative size for padding or truncation
      dci_pdu_rel15_t temp_pdu;
      if (dci_format[i] == NR_DL_DCI_FORMAT_1_0)
        alt_size = nr_dci_size(current_DL_BWP,
                               current_UL_BWP,
                               sc_info,
                               mac->pdsch_HARQ_ACK_Codebook,
                               &temp_pdu,
                               NR_UL_DCI_FORMAT_0_0,
                               rnti_type,
                               coreset,
                               ss->searchSpaceType->present,
                               mac->type0_PDCCH_CSS_config.num_rbs,
                               0);
      if (dci_format[i] == NR_UL_DCI_FORMAT_0_0)
        alt_size = nr_dci_size(current_DL_BWP,
                               current_UL_BWP,
                               sc_info,
                               mac->pdsch_HARQ_ACK_Codebook,
                               &temp_pdu,
                               NR_DL_DCI_FORMAT_1_0,
                               rnti_type,
                               coreset,
                               ss->searchSpaceType->present,
                               mac->type0_PDCCH_CSS_config.num_rbs,
                               0);
    }

    uint16_t dci_size = nr_dci_size(current_DL_BWP,
                                    current_UL_BWP,
                                    sc_info,
                                    mac->pdsch_HARQ_ACK_Codebook,
                                    &mac->def_dci_pdu_rel15[dl_config->slot][dci_format[i]],
                                    dci_format[i],
                                    rnti_type,
                                    coreset,
                                    ss->searchSpaceType->present,
                                    mac->type0_PDCCH_CSS_config.num_rbs,
                                    alt_size);
    if (dci_size == 0)
      return;
    rel15->dci_length_options[i] = dci_size;
  }

  // DCI 0_0 and 1_0 are same size, L1 just needs to look for 1 option
  // L2 decides format based on format indicator in payload
  if (rel15->dci_format_options[0] == NFAPI_NR_FORMAT_0_0_AND_1_0)
    rel15->num_dci_options = 1;
  else
    rel15->num_dci_options = 2;

  rel15->BWPStart = coreset_id == 0 ? mac->type0_PDCCH_CSS_config.cset_start_rb : current_DL_BWP->BWPStart;
  rel15->BWPSize = coreset_id == 0 ? mac->type0_PDCCH_CSS_config.num_rbs : current_DL_BWP->BWPSize;

  int sps = 0;
  const BIT_STRING_t *monitoringSymbols = ss->monitoringSymbolsWithinSlot;
  if (!monitoringSymbols || !monitoringSymbols->buf || monitoringSymbols->size < 2) {
    LOG_W(NR_MAC,
          "Invalid monitoringSymbolsWithinSlot for searchSpaceId %ld (ptr=%p size=%zu), skip DCI config\n",
          ss->searchSpaceId,
          monitoringSymbols,
          monitoringSymbols ? monitoringSymbols->size : 0);
    return;
  }

  switch(rnti_type) {
    case TYPE_C_RNTI_:
      // we use DL BWP dedicated
      sps = current_DL_BWP->cyclicprefix ? 12 : 14;
      rel15->rnti = mac->crnti;
      rel15->SubcarrierSpacing = current_DL_BWP->scs;
      break;
    case TYPE_RA_RNTI_:
      // we use the initial DL BWP
      sps = current_DL_BWP->cyclicprefix == NULL ? 14 : 12;
      rel15->rnti = mac->ra.ra_rnti;
      rel15->SubcarrierSpacing = current_DL_BWP->scs;
      break;
    case TYPE_MSGB_RNTI_:
      // we use the initial DL BWP
      sps = current_DL_BWP->cyclicprefix == NULL ? 14 : 12;
      rel15->rnti = mac->ra.MsgB_rnti;
      rel15->SubcarrierSpacing = current_DL_BWP->scs;
      break;
    case TYPE_P_RNTI_:
      sps = current_DL_BWP->cyclicprefix == NULL ? 14 : 12;
      rel15->rnti = P_RNTI;
      rel15->SubcarrierSpacing = current_DL_BWP->scs;
      break;
    case TYPE_CS_RNTI_:
      break;
    case TYPE_TC_RNTI_:
      // we use the initial DL BWP
      sps = current_DL_BWP->cyclicprefix == NULL ? 14 : 12;
      rel15->rnti = mac->ra.t_crnti;
      rel15->SubcarrierSpacing = current_DL_BWP->scs;
      break;
    case TYPE_SP_CSI_RNTI_:
      break;
    case TYPE_SI_RNTI_:
      sps = 14;
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

  rel15->coreset.StartSymbolBitmap = nr_pdcch_monitoring_symbols_mask(monitoringSymbols, sps);
  uint32_t Y = 0;
  if (ss->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_ue_Specific)
    Y = get_Y(ss, slot, rel15->rnti);
  fill_dci_search_candidates(ss, rel15, Y);
  // not scheduling DCI reception if there are no candidates
  if (rel15->number_of_candidates == 0) {
    LOG_E(NR_MAC, "No candidates in this DCI, not scheduling it");
    return;
  }

  #ifdef DEBUG_DCI
    for (int i = 0; i < rel15->num_dci_options; i++) {
      LOG_T(NR_MAC_DCI,
            "[DCI_CONFIG] Configure DCI PDU: rnti_type %d BWPSize %d BWPStart %d rel15->SubcarrierSpacing %d rel15->dci_format %d "
            "rel15->dci_length %d sps %d monitoringSymbolsWithinSlot %d \n",
            rnti_type,
            rel15->BWPSize,
            rel15->BWPStart,
            rel15->SubcarrierSpacing,
            rel15->dci_format_options[i],
            rel15->dci_length_options[i],
            sps,
            rel15->coreset.StartSymbolBitmap);
    }
  #endif
  // add DCI
  dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DCI;
  dl_config->number_pdus += 1;
}

bool is_ss_monitor_occasion(const int frame, const int slot, const int slots_per_frame, const NR_SearchSpace_t *ss)
{
  if (!ss)
    return false;
  const int duration = ss->duration ? *ss->duration : 1;
  bool monitor = false;
  int period, offset;
  get_monitoring_period_offset(ss, &period, &offset);
  // The UE monitors PDCCH candidates for search space set ss for 'duration' consecutive slots
  for (int i = 0; i < duration; i++) {
    if (is_periodic_occasion(frame, slot, slots_per_frame, period, offset + i)) {
      monitor = true;
      break;
    }
  }
  return monitor;
}

static bool search_space_monitoring_ocasion_other_si(NR_UE_MAC_INST_t *mac,
                                                     const NR_SearchSpace_t *ss,
                                                     const int abs_slot,
                                                     const int frame,
                                                     const int slot,
                                                     const int slots_per_frame)
{
  const int duration = ss->duration ? *ss->duration : 1;
  int period, offset;
  get_monitoring_period_offset(ss, &period, &offset);
  for (int i = 0; i < duration; i++) {
    if (is_periodic_occasion(frame, slot, slots_per_frame, period, offset + i)) {
      int N = mac->ssb_list.nb_tx_ssb;
      int K = mac->ssb_list.nb_ssb_per_index[mac->mib_ssb];
      // numbering current frame and slot in terms of monitoring occasions in window
      int rel_slot = abs_slot - mac->si_SchedInfo.si_window_start;
      int current_monitor_occasion = (rel_slot % period) + (duration * rel_slot / period);
      return current_monitor_occasion % N == K;
    }
  }

  return false;
}

static bool is_window_valid(NR_UE_MAC_INST_t *mac, int window_slots, int abs_slot, int slots_per_frame)
{
  if (mac->si_SchedInfo.si_window_start == -1) {
    // out of window
    return false;
  } else if ((abs_slot - mac->si_SchedInfo.si_window_start + MAX_FRAME_NUMBER * slots_per_frame)
                 % (MAX_FRAME_NUMBER * slots_per_frame)
             >= window_slots) {
    // window expired
    mac->si_SchedInfo.si_window_start = -1;
    return false;
  }
  return true;
}

static bool monitor_dci_for_other_SI(NR_UE_MAC_INST_t *mac,
                                     const NR_SearchSpace_t *ss,
                                     const int si_idx,
                                     const int slots_per_frame,
                                     const int frame,
                                     const int slot)
{
  // according to 5.2.2.3.2 in 331
  const int abs_slot = frame * slots_per_frame + slot;
  const si_schedinfo_config_t *config = mac->si_SchedInfo.si_SchedInfo_list.array[si_idx];
  const int window_slots = 5 << mac->si_SchedInfo.si_WindowLength;
  const int x = (config->si_WindowPosition - 1) * window_slots;
  const int T = 8 << config->si_Periodicity; // radio frame periodicity

  if (mac->si_SchedInfo.si_window_start == -1) {
    if ((frame % T) == (x / slots_per_frame) && (slot == x % slots_per_frame))
      mac->si_SchedInfo.si_window_start = abs_slot; // in terms of absolute slot number
  }
  if (is_window_valid(mac, window_slots, abs_slot, slots_per_frame))
    return search_space_monitoring_ocasion_other_si(mac, ss, abs_slot, frame, slot, slots_per_frame);

  return false;
}

/** @brief Per-TTI PF/PO check for paging DCI monitoring (TS 38.213 §10.1, TS 38.304 §7.1).
 * Returns true if (frame, slot) is a Paging Occasion for this UE on @p paging_ss, false otherwise. */
static bool monitor_paging_dci(const NR_SearchSpace_t *paging_ss,
                               const NR_UE_MAC_INST_t *mac,
                               const int frame,
                               const int slot,
                               const int slots_per_frame)
{
  DevAssert(paging_ss);
  DevAssert(mac);

  const nr_ue_paging_cfg_t *paging_cfg = &mac->paging_cfg;

  /* TS 38.213 §10.1: monitor on the SS occasions. For SearchSpaceId=0 this is equivalent to the
   * Type0/RMSI MO check (TS 38.304 §7.1: "same MOs as RMSI") */
  if (!is_ss_monitor_occasion(frame, slot, slots_per_frame, paging_ss))
    return false;

  const uint16_t ue_id = paging_cfg->ue_id;
  const uint16_t T = paging_cfg->T;
  const uint16_t N = paging_cfg->N;
  const uint8_t Ns = paging_cfg->Ns;
  const uint8_t PF_offset = paging_cfg->PF_offset;

  if (T == 0 || N == 0 || Ns == 0) {
    LOG_W(NR_MAC,
          "[UE %d] invalid paging config at %04d.%02d: T=%u N=%u Ns=%u, skipping P-RNTI monitoring\n",
          mac->ue_id,
          frame,
          slot,
          T,
          N,
          Ns);
    return false;
  }

  /* TS 38.304 §7.1 PF condition */
  if (!nr_pcch_sfn_is_pf((uint16_t)frame, PF_offset, T, N, ue_id))
    return false;

  /* TODO Short Messages support (TS 38.212 §7.3.1.2.1, TS 38.331 Table 6.5-1)
   * UE_ID = 0 is the spec-mandated fallback (no 5G-S-TMSI yet, or 5G-S-TMSI mod 1024 == 0; TS 38.304 §7.1):
   * UE-specific paging cannot succeed (no TMSI to match against pagingRecordList), the specs then expects
   * the UE to still receive broadcast Short Messages in DCI 1_0 with P-RNTI for systemInfoModification,
   * ETWS, CMAS and stopPagingMonitoring. */
  if (ue_id == 0) {
    LOG_W(NR_MAC,
          "[UE %d] PF match at %04d.%02d with UE_ID=0 (no 5G-S-TMSI yet, or 5G-S-TMSI mod 1024 == 0): skipping P-RNTI monitoring\n",
          mac->ue_id,
          frame,
          slot);
    return false;
  }

  /* TS 38.304 §7.1: i_s = floor(UE_ID / N) mod Ns; UE monitors the (i_s+1)-th PO within the PF. */
  const uint8_t i_s = nr_pcch_po_index(ue_id, N, Ns);
  const long ss_id = paging_ss->searchSpaceId;
  if (ss_id == 0) {
    /* TS 38.304 §7.1: pagingSearchSpace = 0 means MOs are same as RMSI. Ns is either 1 or 2.
     * Ns=1 means every Type0 MO is a PO, Ns=2 means first/second half-frame split into POs.
     * Ns=4 is invalid with SearchSpaceId 0 here. */
    AssertFatal(Ns <= 2, "TS 38.304 §7.1: Ns=%u invalid when pagingSearchSpace is SearchSpaceId 0 (must be 1 or 2)\n", Ns);
    if (Ns == 1) {
      LOG_D(NR_MAC_DCI, "[UE %d] PO (SS0, Ns=1) at %04d.%02d\n", mac->ue_id, frame, slot);
      return true;
    }
    const bool po = nr_pcch_ss0_po_half_frame(i_s, slot, slots_per_frame);
    if (po)
      LOG_D(NR_MAC_DCI, "[UE %d] PO (SS0, Ns=2, i_s=%u) at %04d.%02d\n", mac->ue_id, i_s, frame, slot);
    return po;
  }

  /* Type2 paging (SearchSpaceId != 0), TS 38.304 §7.1 + TS 38.213 §10.1: PO is S*X consecutive
   * PDCCH MOs in the PF starting at start_mo (= firstPDCCH-MonitoringOccasionOfPO[i_s] when present,
   * else i_s*S*X). */
  int period = 0;
  int offset = 0;
  get_monitoring_period_offset(paging_ss, &period, &offset);

  /* TODO TS 38.304 §7.1: S is defined as the number of transmitted SS/PBCH blocks (ssb-PositionsInBurst
   *  in SIB1). Currently uses the count of SSBs detected by the UE PHY. */
  const int S = max(1, mac->ssb_list.nb_tx_ssb);
  const int X = paging_cfg->X;
  int start_mo = i_s * S * X;
  if (i_s < paging_cfg->first_mo_of_po_count)
    start_mo = paging_cfg->first_mo_of_po[i_s];
  const int end_mo = start_mo + S * X - 1;

  return nr_pcch_type2_po_mo_in_range(frame, slot, slots_per_frame, period, offset, start_mo, end_mo);
}

void update_pdcch_config(NR_UE_MAC_INST_t *mac)
{
  int scs = mac->current_DL_BWP ? mac->current_DL_BWP->scs : mac->numerology;
  const int slots_per_frame = get_slots_per_frame_from_scs(scs);
  int ssb_sc_offset_norm;
  if (mac->ssb_subcarrier_offset < 24 && mac->frequency_range == FR1)
    ssb_sc_offset_norm = mac->ssb_subcarrier_offset >> scs;
  else
    ssb_sc_offset_norm = mac->ssb_subcarrier_offset;
  uint16_t ssb_offset_point_a = (mac->ssb_start_subcarrier - ssb_sc_offset_norm) / 12;
  int ssb_start_symbol = get_ssb_start_symbol(mac->nr_band, scs, mac->mib_ssb);
  if (mac->mib)
    get_type0_PDCCH_CSS_config_parameters(
        &mac->type0_PDCCH_CSS_config,
        mac->mib_frame,
        mac->mib,
        slots_per_frame,
        ssb_sc_offset_norm,
        ssb_start_symbol,
        scs,
        mac->frequency_range,
        mac->nr_band,
        273, // at this point UE in principle doesn't know the grid size (we assume the largest)
        mac->mib_ssb,
        1, // If the UE is not configured with a periodicity, the UE assumes a periodicity of a half frame
        ssb_offset_point_a);
  else
    LOG_E(MAC, "Call update_pdcch_config( but no mib\n");
  if (mac->search_space_zero == NULL)
    mac->search_space_zero = calloc(1, sizeof(*mac->search_space_zero));
  if (mac->coreset0 == NULL)
    mac->coreset0 = calloc(1, sizeof(*mac->coreset0));
  fill_coresetZero(mac->coreset0, &mac->type0_PDCCH_CSS_config);
  fill_searchSpaceZero(mac->search_space_zero, slots_per_frame, &mac->type0_PDCCH_CSS_config);
}

void ue_dci_configuration(NR_UE_MAC_INST_t *mac, fapi_nr_dl_config_request_t *dl_config, const frame_t frame, const int slot)
{
  const NR_UE_DL_BWP_t *current_DL_BWP = mac->current_DL_BWP;
  NR_BWP_Id_t dl_bwp_id = current_DL_BWP ? current_DL_BWP->bwp_id : 0;
  NR_BWP_PDCCH_t *pdcch_config = &mac->config_BWP_PDCCH[dl_bwp_id];
  int scs = current_DL_BWP ? current_DL_BWP->scs : mac->numerology;
  const int slots_per_frame = get_slots_per_frame_from_scs(scs);
  if (!mac->mib)
    return;
  if (mac->get_sib1 || mac->update_pdcch_config) {
    update_pdcch_config(mac);
    mac->update_pdcch_config = false;
  }
  if (mac->get_sib1) {
    bool is_occasion = is_ss_monitor_occasion(frame, slot, slots_per_frame, mac->search_space_zero);
    if (is_occasion) {
      LOG_D(NR_MAC_DCI, "Monitoring DCI for SIB1 in frame %d slot %d\n", frame, slot);
      config_dci_pdu(mac, dl_config, TYPE_SI_RNTI_, slot, mac->search_space_zero);
    }
  }

  for (int i = 0; i < MAX_SI_GROUPS; i++) {
    if (!mac->get_otherSI[i])
      continue;
    // If searchSpaceOtherSystemInformation is set to zero,
    // PDCCH monitoring occasions for SI message reception in SI-window
    // are same as PDCCH monitoring occasions for SIB1
    const NR_SearchSpace_t *ss = get_common_search_space(mac, pdcch_config->otherSI_SS_id == -1 ? 0 : pdcch_config->otherSI_SS_id);
    // TODO configure SI-window
    if (monitor_dci_for_other_SI(mac, ss, i, slots_per_frame, frame, slot)) {
      LOG_D(NR_MAC_DCI, "Monitoring DCI for other SIs in frame %d slot %d\n", frame, slot);
      config_dci_pdu(mac, dl_config, TYPE_SI_RNTI_, slot, ss);
    }
  }
  RA_config_t *ra = &mac->ra;
  if (mac->state == UE_PERFORMING_RA && ra->ra_state >= nrRA_WAIT_RAR) {
    const NR_SearchSpace_t *ra_SS = get_common_search_space(mac, pdcch_config->ra_SS_id);
    // if RA is ongoing use RA search space
    if (is_ss_monitor_occasion(frame, slot, slots_per_frame, ra_SS)) {
      nr_rnti_type_t rnti_type = 0;
      if (ra->ra_type == RA_4_STEP) {
        rnti_type = ra->ra_state == nrRA_WAIT_RAR ? TYPE_RA_RNTI_ : TYPE_TC_RNTI_;
      } else {
        rnti_type = TYPE_MSGB_RNTI_;
      }
      config_dci_pdu(mac, dl_config, rnti_type, slot, ra_SS);
    }
    // If Msg3 has C-RNTI MAC CE, also monitor dedicated search spaces during contention resolution
    // according to TS 38.321 section 5.1.5
    if (ra->ra_state == nrRA_WAIT_CONTENTION_RESOLUTION && mac->msg3_C_RNTI) {
      for (int i = 0; i < pdcch_config->list_SS.count; i++) {
        NR_SearchSpace_t *ss = pdcch_config->list_SS.array[i];
        if (is_ss_monitor_occasion(frame, slot, slots_per_frame, ss))
          config_dci_pdu(mac, dl_config, TYPE_C_RNTI_, slot, ss);
      }
      // the gNB may address the C-RNTI in a common search space
      bool ss0_listed = false;
      for (int i = 0; i < pdcch_config->list_common_SS.count; i++) {
        NR_SearchSpace_t *ss = pdcch_config->list_common_SS.array[i];
        ss0_listed |= ss->searchSpaceId == 0;
        if (is_ss_monitor_occasion(frame, slot, slots_per_frame, ss))
          config_dci_pdu(mac, dl_config, TYPE_C_RNTI_, slot, ss);
      }
      if (!ss0_listed && mac->search_space_zero && is_ss_monitor_occasion(frame, slot, slots_per_frame, mac->search_space_zero))
        config_dci_pdu(mac, dl_config, TYPE_C_RNTI_, slot, mac->search_space_zero);
    }
  } else if (mac->state == UE_CONNECTED) {
    /*
      In RRC Connected state, the UE monitors for fallback DCI with C-RNTI in common and UE-specific SS.
      First the UE check all UE specific SS. If no fallback DCI is configured, it configures one of following
      common SS with the shown priority
      1. RA SS
      2. Other SI SS
      3. Paging SS
      4. SIB1 SS

      There is a possibility that gNB could configure multiple UE-specific SS for fallback DCI or/and common SS,
      and in a given slot more than one SS become active. For the moment, we assume such configuration wouldn't happen
      and hence don't handle it for the sake of implementation simplicity.
    */
    bool configured_fallback_dci = false;
    for (int i = 0; i < pdcch_config->list_SS.count; i++) {
      NR_SearchSpace_t *ss = pdcch_config->list_SS.array[i];
      if (is_ss_monitor_occasion(frame, slot, slots_per_frame, ss)) {
        config_dci_pdu(mac, dl_config, TYPE_C_RNTI_, slot, ss);
        configured_fallback_dci |= (ss->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_common);
      }
    }
    const NR_SearchSpace_t *css = NULL;
    if (!configured_fallback_dci) {
#define CSS_VALID_CUR_SLOT(CSS) \
  ((CSS > -1) && is_ss_monitor_occasion(frame, slot, slots_per_frame, get_common_search_space(mac, CSS)))
      if (CSS_VALID_CUR_SLOT(pdcch_config->ra_SS_id))
        css = get_common_search_space(mac, pdcch_config->ra_SS_id);
      else if (CSS_VALID_CUR_SLOT(pdcch_config->otherSI_SS_id))
        css = get_common_search_space(mac, pdcch_config->otherSI_SS_id);
      else if (CSS_VALID_CUR_SLOT(pdcch_config->paging_SS_id))
        css = get_common_search_space(mac, pdcch_config->paging_SS_id);
#undef CSS_VALID_CUR_SLOT
      else
        css = mac->search_space_zero;
      AssertFatal(css, "Atleast one CSS should be present in connected state\n");
      config_dci_pdu(mac, dl_config, TYPE_C_RNTI_, slot, css);
    }
  } else if (mac->state == UE_IDLE) {
    /* UE_IDLE (RRC-triggered camped idle) monitors pagingSearchSpace per TS 38.213 §10.1 and TS 38.304 §7.1. */
    if (pdcch_config->paging_SS_id > -1) {
      const NR_SearchSpace_t *paging_ss = get_common_search_space(mac, pdcch_config->paging_SS_id);
      AssertFatal(paging_ss, "pagingSearchSpaceId=%ld not found in common SS list\n", pdcch_config->paging_SS_id);
      DevAssert(paging_ss->searchSpaceType && paging_ss->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_common);
      if (pdcch_config->paging_SS_id > 0) {
        DevAssert(paging_ss->monitoringSymbolsWithinSlot && paging_ss->monitoringSymbolsWithinSlot->buf
                  && paging_ss->monitoringSymbolsWithinSlot->size >= 2);
      }
      if (monitor_paging_dci(paging_ss, mac, frame, slot, slots_per_frame)) {
        LOG_D(NR_MAC_DCI,
              "[UE %d] Monitoring P-RNTI DCI at %04d.%02d (SearchSpaceId=%ld)\n",
              mac->ue_id,
              frame,
              slot,
              paging_ss->searchSpaceId);
        config_dci_pdu(mac, dl_config, TYPE_P_RNTI_, slot, paging_ss);
      }
    } else {
      LOG_D(NR_MAC,
            "[UE %d] pagingSearchSpace is not configured (paging_SS_id=%ld), skipping P-RNTI monitoring\n",
            mac->ue_id,
            pdcch_config->paging_SS_id);
    }
  }
}
