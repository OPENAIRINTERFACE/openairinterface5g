/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <stdbool.h>
#include "ue_capability_handler.h"
#include "common/utils/assertions.h"
#include "common/utils/LOG/log.h"

// Attempts to resolve DL/UL feature-set-per-CC ids for one specific (featureSetCombination, pos) candidate
static bool get_ids_from_fs_combination(const NR_UE_NR_Capability_t *cap, long featureSetCombination, int pos, NR_feature_set_ids_t *ids)
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
    if (idx < 0 || idx >= cap->featureSets->featureSetsDownlink->list.count) {
      LOG_E(NR_MAC, "Invalid downlinkSetNR %d\n", res.dlset_id);
      return false;
    }
    const NR_FeatureSetDownlink_t *fsd = cap->featureSets->featureSetsDownlink->list.array[idx];
    if (fsd->featureSetListPerDownlinkCC.list.count != 1) {
      LOG_W(NR_MAC,
            "Multiple DL carriers (%d) bundled in this FeatureSetDownlink, skipping candidate\n",
            fsd->featureSetListPerDownlinkCC.list.count);
      return false;
    }
    res.dl_feature_set_percc_id = *(fsd->featureSetListPerDownlinkCC.list.array[0]);
  }
  if (res.ulset_id > 0) {
    long idx = res.ulset_id - 1;
    if (idx < 0 || idx >= cap->featureSets->featureSetsUplink->list.count) {
      LOG_E(NR_MAC, "Invalid uplinkSetNR %d\n", res.ulset_id);
      return false;
    }
    const NR_FeatureSetUplink_t *fsu = cap->featureSets->featureSetsUplink->list.array[idx];
    if (fsu->featureSetListPerUplinkCC.list.count != 1) {
      LOG_W(NR_MAC,
            "Multiple UL carriers (%d) bundled in this FeatureSetUplink, skipping candidate\n",
            fsu->featureSetListPerUplinkCC.list.count);
      return false;
    }
    res.ul_feature_set_percc_id = *(fsu->featureSetListPerUplinkCC.list.array[0]);
  }
  *ids = res;
  return true;
}

NR_feature_set_ids_t get_feature_set_ids(const NR_UE_NR_Capability_t *cap, int band, nr_rat_type_t type)
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
          if (get_ids_from_fs_combination(cap, bc->featureSetCombination, j, &ids))
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
          if (match_pos >= 0 && get_ids_from_fs_combination(cap, bc->featureSetCombination, match_pos, &ids))
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
