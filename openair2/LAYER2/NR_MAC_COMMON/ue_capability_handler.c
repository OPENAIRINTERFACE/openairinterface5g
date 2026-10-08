/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <stdbool.h>
#include "ue_capability_handler.h"
#include "nr_mac_common.h"
#include "common/utils/assertions.h"
#include "common/utils/LOG/log.h"

// numeric value of a SupportedBandwidth; follows the ASN.1 enumeration order
static int supported_bw_to_mhz(const NR_SupportedBandwidth_t *bw)
{
  static const int fr1_mhz[] = {5, 10, 15, 20, 25, 30, 40, 50, 60, 80, 100};
  static const int fr2_mhz[] = {50, 100, 200, 400};
  switch (bw->present) {
    case NR_SupportedBandwidth_PR_fr1:
      AssertFatal(bw->choice.fr1 >= 0 && bw->choice.fr1 < sizeofArray(fr1_mhz), "Invalid FR1 supported band\n");
      return fr1_mhz[bw->choice.fr1];
    case NR_SupportedBandwidth_PR_fr2:
      AssertFatal(bw->choice.fr2 >= 0 && bw->choice.fr2 < sizeofArray(fr2_mhz), "Invalid FR2 supported band\n");
      return fr2_mhz[bw->choice.fr2];
    default:
      AssertFatal(false, "Invalid BW type\n");
  }
}

// TS 38.306: supportedBandwidthDL/UL is the maximum channel bandwidth the UE supports for the SCS
// of the per-CC entry, so any smaller bandwidth fits. 90 MHz is not in the enumeration and is
// validated with channelBW-90mhz instead
static bool supported_bw_comparison(int bw_mhz, const NR_SupportedBandwidth_t *supported_BW, const long *support_90mhz)
{
  if (bw_mhz == 90)
    return support_90mhz != NULL;
  return bw_mhz <= supported_bw_to_mhz(supported_BW);
}

// Pick the per-CC id to use for a cell with the given SCS/BW. Any id of the list is valid for any
// carrier (TS 38.306). The SCS must match, since it is what the network validates to know whether
// the UE supports the numerology. The bandwidth only ranks the ids: supportedBandwidth is a maximum,
// and the spec does not tie the maximum of a CA entry to single-carrier operation. Among the ids
// covering the cell bandwidth the one with most layers wins; if none covers it, the first
// id matching the SCS is used.
static int select_dl_percc_id(const NR_FeatureSets_t *fs, const NR_FeatureSetDownlink_t *fsd, int scs, int bw_mhz)
{
  int best = 0;
  bool best_fits_bw = false;
  long best_layers = -1;
  if (!fs->featureSetsDownlinkPerCC)
    return 0;
  for (int k = 0; k < fsd->featureSetListPerDownlinkCC.list.count; k++) {
    long id = *fsd->featureSetListPerDownlinkCC.list.array[k];
    if (id < 1 || id > fs->featureSetsDownlinkPerCC->list.count)
      continue;
    const NR_FeatureSetDownlinkPerCC_t *cc = fs->featureSetsDownlinkPerCC->list.array[id - 1];
    if (cc->supportedSubcarrierSpacingDL != scs)
      continue;
    bool fits_bw = supported_bw_comparison(bw_mhz, &cc->supportedBandwidthDL, cc->channelBW_90mhz);
    long layers = cc->maxNumberMIMO_LayersPDSCH ? 2 << *cc->maxNumberMIMO_LayersPDSCH : 0;
    if (best == 0 || (fits_bw && (!best_fits_bw || layers > best_layers))) {
      best = id;
      best_fits_bw = fits_bw;
      best_layers = layers;
    }
  }
  if (best && !best_fits_bw)
    LOG_W(NR_MAC, "No FeatureSetDownlinkPerCC-Id covers %d MHz, using id %d\n", bw_mhz, best);
  return best;
}

static int select_ul_percc_id(const NR_FeatureSets_t *fs, const NR_FeatureSetUplink_t *fsu, int scs, int bw_mhz)
{
  int best = 0;
  bool best_fits_bw = false;
  long best_layers = -1;
  if (!fs->featureSetsUplinkPerCC)
    return 0;
  for (int k = 0; k < fsu->featureSetListPerUplinkCC.list.count; k++) {
    long id = *fsu->featureSetListPerUplinkCC.list.array[k];
    if (id < 1 || id > fs->featureSetsUplinkPerCC->list.count)
      continue;
    const NR_FeatureSetUplinkPerCC_t *cc = fs->featureSetsUplinkPerCC->list.array[id - 1];
    if (cc->supportedSubcarrierSpacingUL != scs)
      continue;
    bool fits_bw = supported_bw_comparison(bw_mhz, &cc->supportedBandwidthUL, cc->channelBW_90mhz);
    long layers = 0;
    if (cc->mimo_CB_PUSCH && cc->mimo_CB_PUSCH->maxNumberMIMO_LayersCB_PUSCH)
      layers = 1 << *cc->mimo_CB_PUSCH->maxNumberMIMO_LayersCB_PUSCH;
    if (cc->maxNumberMIMO_LayersNonCB_PUSCH) {
      long nocb = 1 << *cc->maxNumberMIMO_LayersNonCB_PUSCH;
      if (nocb > layers)
        layers = nocb;
    }
    if (best == 0 || (fits_bw && (!best_fits_bw || layers > best_layers))) {
      best = id;
      best_fits_bw = fits_bw;
      best_layers = layers;
    }
  }
  if (best && !best_fits_bw)
    LOG_W(NR_MAC, "No FeatureSetUplinkPerCC-Id covers %d MHz, using id %d\n", bw_mhz, best);
  return best;
}

// Attempts to resolve DL/UL feature-set-per-CC ids for one specific (featureSetCombination, pos) candidate.
// *ids is written only on success, so a rejected candidate never leaks partial results to the caller.
static bool get_ids_from_fs_combination(const NR_UE_NR_Capability_t *cap,
                                        long featureSetCombination,
                                        int pos,
                                        int scs,
                                        int bw_mhz,
                                        NR_feature_set_ids_t *ids)
{
  if (featureSetCombination < 0 || featureSetCombination >= cap->featureSetCombinations->list.count) {
    LOG_E(NR_MAC, "Invalid featureSetCombination index %ld\n", featureSetCombination);
    return false;
  }
  const NR_FeatureSetCombination_t *fsc = cap->featureSetCombinations->list.array[featureSetCombination];
  if (pos < 0 || pos >= fsc->list.count) {
    LOG_E(NR_MAC, "Invalid FeatureSetsPerBand index %d\n", pos);
    return false;
  }
  const NR_FeatureSetsPerBand_t *fspb = fsc->list.array[pos];
  if (fspb->list.count != 1) {
    LOG_E(NR_MAC, "Cannot handle more than 1 FeatureSet alternative\n");
    return false;
  }
  const NR_FeatureSet_t *fs = fspb->list.array[0];
  if (fs->present != NR_FeatureSet_PR_nr || !fs->choice.nr) {
    LOG_E(NR_MAC, "FeatureSet is not NR\n");
    return false;
  }

  NR_feature_set_ids_t res = {0};
  res.dlset_id = fs->choice.nr->downlinkSetNR; /* 0 = no DL carrier here, per 38.306 */
  res.ulset_id = fs->choice.nr->uplinkSetNR;   /* 0 = no UL carrier here, per 38.306 */
  if (res.dlset_id > 0) {
    long idx = res.dlset_id - 1;
    if (!cap->featureSets->featureSetsDownlink || idx >= cap->featureSets->featureSetsDownlink->list.count) {
      LOG_E(NR_MAC, "Invalid downlinkSetNR %d\n", res.dlset_id);
      return false;
    }
    const NR_FeatureSetDownlink_t *fsd = cap->featureSets->featureSetsDownlink->list.array[idx];
    res.dl_feature_set_percc_id = select_dl_percc_id(cap->featureSets, fsd, scs, bw_mhz);
    if (res.dl_feature_set_percc_id == 0) {
      LOG_W(NR_MAC, "No FeatureSetDownlinkPerCC-Id fits DL SCS %d\n", scs);
      return false;
    }
  }
  if (res.ulset_id > 0) {
    long idx = res.ulset_id - 1;
    if (!cap->featureSets->featureSetsUplink || idx >= cap->featureSets->featureSetsUplink->list.count) {
      LOG_E(NR_MAC, "Invalid uplinkSetNR %d\n", res.ulset_id);
      return false;
    }
    const NR_FeatureSetUplink_t *fsu = cap->featureSets->featureSetsUplink->list.array[idx];
    res.ul_feature_set_percc_id = select_ul_percc_id(cap->featureSets, fsu, scs, bw_mhz);
    if (res.ul_feature_set_percc_id == 0) {
      LOG_W(NR_MAC, "No FeatureSetUplinkPerCC-Id fits UL SCS %d\n", scs);
      return false;
    }
  }
  *ids = res;
  return true;
}

NR_feature_set_ids_t feature_set_ids_handler(const NR_UE_NR_Capability_t *cap,
                                             const NR_FrequencyInfoDL_t *frequencyInfoDL,
                                             nr_rat_type_t type)
{
  // Assuming information in DL and UL is identical
  AssertFatal(frequencyInfoDL, "Cannot handle feature set IDs without frequencyInfoDL\n");
  NR_FreqBandIndicatorNR_t nr_band = *frequencyInfoDL->frequencyBandList.list.array[0];
  frequency_range_t frequency_range = get_freq_range_from_band(nr_band);
  int scs = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
  int bw = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
  int bw_index = get_supported_band_index(scs, frequency_range, bw);
  AssertFatal(bw_index >= 0, "Unsupported carrier BW\n");
  int bw_mhz = get_supported_bw_mhz(frequency_range, bw_index);
  return get_feature_set_ids(cap, nr_band, scs, bw_mhz, type);
}

NR_feature_set_ids_t get_feature_set_ids(const NR_UE_NR_Capability_t *cap, int band, int scs, int bw_mhz, nr_rat_type_t type)
{
  NR_feature_set_ids_t ids = {0};
  if (!cap) {
    LOG_W(NR_MAC, "Cannot handle feature set IDs without UE capabilities\n");
    return ids;
  }
  NR_BandCombinationList_t *bcl = cap->rf_Parameters.supportedBandCombinationList;
  if (!bcl || !cap->featureSetCombinations || !cap->featureSets)
    return ids;

  for (int i = 0; i < bcl->list.count; i++) {
    const NR_BandCombination_t *bc = bcl->list.array[i];
    int count = bc->bandList.list.count;
    switch (type) {
      case NR_SA:
        for (int j = 0; j < count; j++) {
          const NR_BandParameters_t *bp = bc->bandList.list.array[j];
          if (bp->present != NR_BandParameters_PR_nr || !bp->choice.nr || bp->choice.nr->bandNR != band)
            continue;
          bool dl_present_here = (bp->choice.nr->ca_BandwidthClassDL_NR != NULL);
          bool ul_present_here = (bp->choice.nr->ca_BandwidthClassUL_NR != NULL);
          if (!dl_present_here || !ul_present_here)
            continue; /* needs both DL UL on this CC; this position doesn't have it */
          if (get_ids_from_fs_combination(cap, bc->featureSetCombination, j, scs, bw_mhz, &ids))
            return ids;
        }
        break;
      case EN_DC:
      case NR_DC:
        if (count == 2) {
          int match_pos = -1;
          for (int j = 0; j < 2; j++) {
            const NR_BandParameters_t *bp = bc->bandList.list.array[j];
            if (bp->present == NR_BandParameters_PR_nr && bp->choice.nr && bp->choice.nr->bandNR == band) {
              if (match_pos >= 0) {
                match_pos = -1; // looking for a configuration with 2 different bands NR+NR or NR+EUTRA
                break;
              }
              match_pos = j;
            }
          }
          if (match_pos >= 0 && get_ids_from_fs_combination(cap, bc->featureSetCombination, match_pos, scs, bw_mhz, &ids))
            return ids;
        }
        break;
      default:
        AssertFatal(false, "Unsupported NR RAT type\n");
    }
  }
  LOG_E(NR_MAC, "No band combination matched the input band %d\n", band);
  return (NR_feature_set_ids_t){0};
}
