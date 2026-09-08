/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*!
 * \brief Routines for UE MAC-layer Random Access procedures (TS 38.321, Release 15)
 */

/* RRC */
#include "RRC/NR_UE/L2_interface_ue.h"

/* MAC */
#include "NR_MAC_COMMON/nr_mac.h"
#include "LAYER2/NR_MAC_UE/mac_proto.h"
#include <executables/softmodem-common.h>
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.h"

int16_t get_prach_tx_power(NR_UE_MAC_INST_t *mac)
{
  RA_config_t *ra = &mac->ra;
  int16_t pathloss = compute_nr_SSB_PL(mac);
  int16_t ra_preamble_rx_power = (int16_t)(ra->prach_resources.ra_preamble_rx_target_power + pathloss);
  return min(ra->prach_resources.Pc_max, ra_preamble_rx_power);
}

static void set_preambleTransMax(RA_config_t *ra, long preambleTransMax)
{
  switch (preambleTransMax) {
    case 0:
      ra->preambleTransMax = 3;
      break;
    case 1:
      ra->preambleTransMax = 4;
      break;
    case 2:
      ra->preambleTransMax = 5;
      break;
    case 3:
      ra->preambleTransMax = 6;
      break;
    case 4:
      ra->preambleTransMax = 7;
      break;
    case 5:
      ra->preambleTransMax = 8;
      break;
    case 6:
      ra->preambleTransMax = 10;
      break;
    case 7:
      ra->preambleTransMax = 20;
      break;
    case 8:
      ra->preambleTransMax = 50;
      break;
    case 9:
      ra->preambleTransMax = 100;
      break;
    case 10:
      ra->preambleTransMax = 200;
      break;
    default:
      AssertFatal(false, "Invalid preambleTransMax\n");
  }
}

static int get_Msg3SizeGroupA(long ra_Msg3SizeGroupA)
{
  int bits = 0;
  switch (ra_Msg3SizeGroupA) {
    case NR_RACH_ConfigCommon__groupBconfigured__ra_Msg3SizeGroupA_b56:
      bits = 56;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__ra_Msg3SizeGroupA_b144:
      bits = 144;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__ra_Msg3SizeGroupA_b208:
      bits = 208;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__ra_Msg3SizeGroupA_b256:
      bits = 256;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__ra_Msg3SizeGroupA_b282:
      bits = 282;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__ra_Msg3SizeGroupA_b480:
      bits = 480;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__ra_Msg3SizeGroupA_b640:
      bits = 640;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__ra_Msg3SizeGroupA_b800:
      bits = 800;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__ra_Msg3SizeGroupA_b1000:
      bits = 1000;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__ra_Msg3SizeGroupA_b72:
      bits = 72;
      break;
    default:
      AssertFatal(false, "Unknown ra-Msg3SizeGroupA %lu\n", ra_Msg3SizeGroupA);
  }
  return bits / 8; // returning bytes
}

static int get_messagePowerOffsetGroupB(long messagePowerOffsetGroupB)
{
  int pow_offset = 0;
  switch (messagePowerOffsetGroupB) {
    case NR_RACH_ConfigCommon__groupBconfigured__messagePowerOffsetGroupB_minusinfinity:
      pow_offset = INT_MIN;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__messagePowerOffsetGroupB_dB0:
      pow_offset = 0;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__messagePowerOffsetGroupB_dB5:
      pow_offset = 5;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__messagePowerOffsetGroupB_dB8:
      pow_offset = 8;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__messagePowerOffsetGroupB_dB10:
      pow_offset = 10;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__messagePowerOffsetGroupB_dB12:
      pow_offset = 12;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__messagePowerOffsetGroupB_dB15:
      pow_offset = 15;
      break;
    case NR_RACH_ConfigCommon__groupBconfigured__messagePowerOffsetGroupB_dB18:
      pow_offset = 18;
      break;
    default:
      AssertFatal(false, "Unknown messagePowerOffsetGroupB %lu\n", messagePowerOffsetGroupB);
  }
  return pow_offset;
}

static void select_preamble_group(NR_UE_MAC_INST_t *mac)
{
  RA_config_t *ra = &mac->ra;
  // TODO if the RA_TYPE is switched from 2-stepRA to 4-stepRA

  if (!ra->Msg3_buffer) { // if Msg3 buffer is empty
    NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon = mac->current_UL_BWP->rach_ConfigCommon;
    if (nr_rach_ConfigCommon && nr_rach_ConfigCommon->groupBconfigured) { // if Random Access Preambles group B is configured
      struct NR_RACH_ConfigCommon__groupBconfigured *groupB = nr_rach_ConfigCommon->groupBconfigured;
      // if the potential Msg3 size (UL data available for transmission plus MAC subheader(s) and,
      // where required, MAC CEs) is greater than ra-Msg3SizeGroupA
      // if the pathloss is less than PCMAX (of the Serving Cell performing the Random Access Procedure)
      // – preambleReceivedTargetPower – msg3-DeltaPreamble – messagePowerOffsetGroupB
      int groupB_pow_offset = get_messagePowerOffsetGroupB(groupB->messagePowerOffsetGroupB);
      int PLThreshold = ra->prach_resources.Pc_max - ra->preambleRxTargetPower - ra->msg3_deltaPreamble - groupB_pow_offset;
      int pathloss = compute_nr_SSB_PL(mac);
      // TODO if the Random Access procedure was initiated for the CCCH logical channel and the CCCH SDU size
      // plus MAC subheader is greater than ra-Msg3SizeGroupA
      if (ra->Msg3_size > get_Msg3SizeGroupA(groupB->ra_Msg3SizeGroupA) && pathloss < PLThreshold)
        ra->RA_GroupA = false;
      else
        ra->RA_GroupA = true;
    } else
      ra->RA_GroupA = true;
  }
  // else if Msg3 is being retransmitted, we keep what used in first transmission of Msg3
}

static ssb_ro_preambles_t get_ssb_ro_preambles_2step(struct NR_RACH_ConfigCommonTwoStepRA_r16__msgA_SSB_PerRACH_OccasionAndCB_PreamblesPerSSB_r16 *config)
{
  ssb_ro_preambles_t ret = {0};
  switch (config->present) {
    case NR_RACH_ConfigCommonTwoStepRA_r16__msgA_SSB_PerRACH_OccasionAndCB_PreamblesPerSSB_r16_PR_oneEighth :
      ret.ssb_per_ro = 0.125;
      ret.preambles_per_ssb = (config->choice.oneEighth + 1) << 2;
      break;
    case NR_RACH_ConfigCommonTwoStepRA_r16__msgA_SSB_PerRACH_OccasionAndCB_PreamblesPerSSB_r16_PR_oneFourth :
      ret.ssb_per_ro = 0.25;
      ret.preambles_per_ssb = (config->choice.oneFourth + 1) << 2;
      break;
    case NR_RACH_ConfigCommonTwoStepRA_r16__msgA_SSB_PerRACH_OccasionAndCB_PreamblesPerSSB_r16_PR_oneHalf :
      ret.ssb_per_ro = 0.5;
      ret.preambles_per_ssb = (config->choice.oneHalf + 1) << 2;
      break;
    case NR_RACH_ConfigCommonTwoStepRA_r16__msgA_SSB_PerRACH_OccasionAndCB_PreamblesPerSSB_r16_PR_one :
      ret.ssb_per_ro = 1;
      ret.preambles_per_ssb = (config->choice.one + 1) << 2;
      break;
    case NR_RACH_ConfigCommonTwoStepRA_r16__msgA_SSB_PerRACH_OccasionAndCB_PreamblesPerSSB_r16_PR_two :
      ret.ssb_per_ro = 2;
      ret.preambles_per_ssb = (config->choice.two + 1) << 2;
      break;
    case NR_RACH_ConfigCommonTwoStepRA_r16__msgA_SSB_PerRACH_OccasionAndCB_PreamblesPerSSB_r16_PR_four :
      ret.ssb_per_ro = 4;
      ret.preambles_per_ssb = config->choice.four;
      break;
    case NR_RACH_ConfigCommonTwoStepRA_r16__msgA_SSB_PerRACH_OccasionAndCB_PreamblesPerSSB_r16_PR_eight :
      ret.ssb_per_ro = 8;
      ret.preambles_per_ssb = config->choice.eight;
      break;
    case NR_RACH_ConfigCommonTwoStepRA_r16__msgA_SSB_PerRACH_OccasionAndCB_PreamblesPerSSB_r16_PR_sixteen :
      ret.ssb_per_ro = 16;
      ret.preambles_per_ssb = config->choice.sixteen;
      break;
    default :
      AssertFatal(false, "Invalid msgA_SSB_PerRACH_OccasionAndCB_PreamblesPerSSB_r16\n");
  }
  return ret;
}

static void config_preamble_index(NR_UE_MAC_INST_t *mac)
{
  RA_config_t *ra = &mac->ra;
  // Random seed generation
  unsigned int seed;
  if (IS_SOFTMODEM_IQPLAYER || IS_SOFTMODEM_IQRECORDER) {
    // Overwrite seed with non-random seed for IQ player/recorder
    seed = 1;
  } else {
    // & to truncate the int64_t and keep only the LSB bits, up to sizeof(int)
    seed = (unsigned int)(rdtsc_oai() & ~0);
  }

  NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon = mac->current_UL_BWP->rach_ConfigCommon;
  int nb_of_preambles = 64;
  bool groupBconfigured = false;
  int preamb_ga = 0;
  if (ra->ra_type == RA_4_STEP) {
    ra->ssb_ro_config = mac->ssb_ro_preambles;
    if (nr_rach_ConfigCommon->totalNumberOfRA_Preambles)
      nb_of_preambles = *nr_rach_ConfigCommon->totalNumberOfRA_Preambles;
    // Amongst the contention-based Random Access Preambles associated with an SSB the first numberOfRA-PreamblesGroupA
    // included in groupBconfigured Random Access Preambles belong to Random Access Preambles group A.
    // The remaining Random Access Preambles associated with the SSB belong to Random Access Preambles group B (if configured)
    select_preamble_group(mac);
    if (nr_rach_ConfigCommon->groupBconfigured) {
      groupBconfigured = true;
      preamb_ga = nr_rach_ConfigCommon->groupBconfigured->numberOfRA_PreamblesGroupA;
    }
  } else {
    NR_RACH_ConfigCommonTwoStepRA_r16_t *twostep = &mac->current_UL_BWP->msgA_ConfigCommon_r16->rach_ConfigCommonTwoStepRA_r16;
    AssertFatal(twostep->groupB_ConfiguredTwoStepRA_r16 == NULL, "GroupB preambles not supported for 2-step RA\n");
    ra->RA_GroupA = true;
    // The field is mandatory present if the 2-step random access type occasions are shared with 4-step random access type,
    // otherwise the field is not present
    bool sharedROs = twostep->msgA_CB_PreamblesPerSSB_PerSharedRO_r16 != NULL;
    AssertFatal(sharedROs == false, "Shared ROs between 2- and 4-step RA not supported\n");

    // For Type-2 random access procedure with separate configuration of PRACH occasions with Type-1 random access procedure
    // configuration by msgA-SSB-PerRACH-OccasionAndCB-PreamblesPerSSB when provided;
    // otherwise, by ssb-perRACH-OccasionAndCB-PreamblesPerSSB
    if (twostep->msgA_SSB_PerRACH_OccasionAndCB_PreamblesPerSSB_r16)
      ra->ssb_ro_config = get_ssb_ro_preambles_2step(twostep->msgA_SSB_PerRACH_OccasionAndCB_PreamblesPerSSB_r16);
    else
      ra->ssb_ro_config = mac->ssb_ro_preambles;
    if (twostep->msgA_TotalNumberOfRA_Preambles_r16)
      nb_of_preambles = *twostep->msgA_TotalNumberOfRA_Preambles_r16;
  }

  int groupOffset = 0;
  if (groupBconfigured) {
    AssertFatal(preamb_ga < nb_of_preambles, "Nb of preambles for groupA not compatible with total number of preambles\n");
    if (!ra->RA_GroupA) { // groupB
      groupOffset = preamb_ga;
      nb_of_preambles = nb_of_preambles - preamb_ga;
    } else {
      nb_of_preambles = preamb_ga;
    }
  }
  int pream_per_ssb = min(ra->ssb_ro_config.preambles_per_ssb, nb_of_preambles);
  int rand_preamb = rand_r(&seed) % pream_per_ssb;
  if (ra->ssb_ro_config.ssb_per_ro < 1)
    ra->ra_PreambleIndex = groupOffset + rand_preamb;
  else {
    int ssb_pr_idx = mac->ssb_list.nb_ssb_per_index[mac->mib_ssb] % (int)ra->ssb_ro_config.ssb_per_ro;
    ra->ra_PreambleIndex = groupOffset + (ssb_pr_idx * ra->ssb_ro_config.preambles_per_ssb) + rand_preamb;
  }
}

static void configure_ra_preamble(NR_UE_MAC_INST_t *mac)
{
  RA_config_t *ra = &mac->ra;
  int ssb = -1; // init as not selected
  ra->ro_mask_index = -1; // init as not selected
  NR_RACH_ConfigDedicated_t *rach_ConfigDedicated = ra->rach_ConfigDedicated;
  if (ra->ra_type == RA_4_STEP) {
    // TODO if the Random Access procedure was initiated for SpCell beam failure recovery
    // TODO if the Random Access procedure was initiated for SI request

    if (ra->pdcch_order.active && ra->pdcch_order.preamble_index != 0xb000000) {
      // set the PREAMBLE_INDEX to the signalled ra-PreambleIndex;
      ra->ra_PreambleIndex = ra->pdcch_order.preamble_index;
      // select the SSB signalled by PDCCH
      ssb = ra->pdcch_order.ssb_index;
      ra->ro_mask_index = ra->pdcch_order.prach_mask;
    } else if (rach_ConfigDedicated && rach_ConfigDedicated->cfra) {
      NR_CFRA_t *cfra = rach_ConfigDedicated->cfra;
      AssertFatal(cfra->occasions == NULL, "Dedicated PRACH occasions for CFRA not supported\n");
      AssertFatal(cfra->resources.present == NR_CFRA__resources_PR_ssb, "SSB-based CFRA supported only\n");
      struct NR_CFRA__resources__ssb *ssb_list = cfra->resources.choice.ssb;
      for (int i = 0; i < ssb_list->ssb_ResourceList.list.count; i++) {
        NR_CFRA_SSB_Resource_t *res = ssb_list->ssb_ResourceList.list.array[i];
        // TODO select an SSB with SS-RSRP above rsrp-ThresholdSSB amongst the associated SSBs
        if (res->ssb == mac->mib_ssb) {
          ssb = mac->mib_ssb;
          // set the PREAMBLE_INDEX to a ra-PreambleIndex corresponding to the selected SSB
          ra->ra_PreambleIndex = res->ra_PreambleIndex;
          ra->ro_mask_index = ssb_list->ra_ssb_OccasionMaskIndex;
          break;
        }
      }
    } else { // for the contention-based Random Access preamble selection
      // TODO if at least one of the SSBs with SS-RSRP above rsrp-ThresholdSSB is available
      // else select any SSB
      ssb = mac->mib_ssb;
      config_preamble_index(mac);
    }
  } else { // 2-step RA
    // if the contention-free 2-step RA type Resources associated with SSBs have been explicitly provided in rach-ConfigDedicated
    // TODO and at least one SSB with SS-RSRP above msgA-RSRP-ThresholdSSB amongst the associated SSBs is available
    if (ra->cfra) {
      AssertFatal(rach_ConfigDedicated->ext1 && rach_ConfigDedicated->ext1->cfra_TwoStep_r16,
                  "Two-step CFRA should be configured here\n");
      NR_CFRA_TwoStep_r16_t *cfra = rach_ConfigDedicated->ext1->cfra_TwoStep_r16;
      AssertFatal(cfra->occasionsTwoStepRA_r16 == NULL, "Dedicated PRACH occasions for CFRA not supported\n");
      for (int i = 0; i < cfra->resourcesTwoStep_r16.ssb_ResourceList.list.count; i++) {
        NR_CFRA_SSB_Resource_t *res = cfra->resourcesTwoStep_r16.ssb_ResourceList.list.array[i];
        if (res->ssb == mac->mib_ssb) {
          ssb = mac->mib_ssb;
          // set the PREAMBLE_INDEX to a ra-PreambleIndex corresponding to the selected SSB
          ra->ra_PreambleIndex = res->ra_PreambleIndex;
          ra->ro_mask_index = cfra->resourcesTwoStep_r16.ra_ssb_OccasionMaskIndex;
          break;
        }
      }
    } else { // for the contention-based Random Access Preamble selection
      // TODO if at least one of the SSBs with SS-RSRP above msgA-RSRP-ThresholdSSB is available
      // else select any SSB
      ssb = mac->mib_ssb;
      config_preamble_index(mac);
    }
  }
  AssertFatal(ssb >= 0, "Something wrong! RA resource selection didn't set any SSB\n");
  // setting the RA ssb value as the progressive number of SSB transmitted
  // non-transmitted SSBs are not taken into account
  int ssb_idx = mac->ssb_list.nb_ssb_per_index[ssb];
  ra->new_ssb = ssb != ssb_idx ? true : false;
  ra->ra_ssb = ssb_idx;

  // TODO not sure how to handle the RO mask when it is limiting the RO occasions
  AssertFatal(ra->ro_mask_index <= 0, "Handling of RACH occasion masking indication not implemented\n");
}

static bool check_mixed_slot_prach(frame_structure_t *fs, int slot, int start_prach, int end_prach)
{
  bool is_mixed = is_mixed_slot(slot, fs);
  if (is_mixed) {
    tdd_bitmap_t *bitmap = &fs->period_cfg.tdd_slot_bitmap[slot % fs->numb_slots_period];
    if (bitmap->num_ul_symbols == 0)
      return false;
    int ul_end = NR_SYMBOLS_PER_SLOT - 1;
    int ul_start = NR_SYMBOLS_PER_SLOT - bitmap->num_ul_symbols;
    if (start_prach < ul_start || end_prach > ul_end)
      return false;
  }
  return true;
}

static void select_prach_occasion(RA_config_t *ra,
                                  int nb_tx_ssb,
                                  int n,
                                  prach_occasion_info_t ra_occasions_period[n],
                                  int num_ra_occasions_period)
{
  unsigned int seed;
  if (IS_SOFTMODEM_IQPLAYER || IS_SOFTMODEM_IQRECORDER) {
    // Overwrite seed with non-random seed for IQ player/recorder
    seed = 1;
  } else {
    // & to truncate the int64_t and keep only the LSB bits, up to sizeof(int)
    seed = (unsigned int)(rdtsc_oai() & ~0);
  }

  int num_ros_per_ssb = 0;
  int idx_ssb = 0;
  int temp_idx = 0;
  if (ra->ssb_ro_config.ssb_per_ro < 1) {
    num_ros_per_ssb = (int)(1 / ra->ssb_ro_config.ssb_per_ro);
    idx_ssb = (rand_r(&seed) % num_ros_per_ssb);
    temp_idx = ra->ra_ssb * num_ros_per_ssb + idx_ssb;
  } else {
    int ssb_per_ro = nb_tx_ssb < ra->ssb_ro_config.ssb_per_ro ? nb_tx_ssb : ra->ssb_ro_config.ssb_per_ro;
    num_ros_per_ssb = ra->association_periods * ssb_per_ro * num_ra_occasions_period / nb_tx_ssb;
    idx_ssb = (rand_r(&seed) % num_ros_per_ssb);
    int eq_ssb = ra->ra_ssb + (idx_ssb * nb_tx_ssb);
    temp_idx = eq_ssb / ra->ssb_ro_config.ssb_per_ro;
  }
  int ro_index = temp_idx % num_ra_occasions_period;
  int ass_period_idx = temp_idx / num_ra_occasions_period;
  ra->sched_ro_info = ra_occasions_period[ro_index];
  ra->sched_ro_info.association_period_idx = ass_period_idx;
}

static void configure_prach_occasions(NR_UE_MAC_INST_t *mac, int scs)
{
  RA_config_t *ra = &mac->ra;
  int num_ra_occasions_period = 0;
  frame_structure_t *fs = &mac->frame_structure;
  nr_prach_info_t prach_info =  get_nr_prach_occasion_info_from_index(ra->ra_config_index, mac->frequency_range, fs->frame_type);
  int max_num_occasions = prach_info.N_RA_sfn * prach_info.N_t_slot * prach_info.N_RA_slot * ra->num_fd_occasions;
  prach_occasion_info_t ra_occasions_period[max_num_occasions];
  // Number of PRACH slots within a subframe (or 60kHz slot) to be taken into account only for 30 and 120kHz
  // as defined in 5.3.2 of 211
  int prach_slots_in_sf = (scs == 1 || scs == 3) ? prach_info.N_RA_slot : 1;
  uint64_t temp_s_map = prach_info.s_map;
  int n_frames = prach_info.y2 == -1 ? 1 : 2;
  for (int n = 0; n < n_frames; n++) {
    int sf = 0;
    for (int s = 0; s < prach_info.N_RA_sfn; s++) { // subframe/60kHz slot occasions in period
      while (((temp_s_map >> sf) & 0x01) == 0)
        sf++;
      int sl = scs == 1 || scs == 3 ? sf * 2 : sf;
      for (int i = 0; i < prach_slots_in_sf; i++) { // slot per subframe/60kHz slot
        int add_slot = i;
        if (scs == 1 || scs == 3) {
          // if only 1 slot per subframe (or 60kHz slot) in case of 30 or 120kHz it's the odd one
          // as defined in 5.3.2 of 211
          if (((prach_info.format & 0xff) > 3) && prach_slots_in_sf == 1)
            add_slot = 1;
        }
        int slot = sl + add_slot;
        if (!is_ul_slot(slot, fs))
          continue; // valid PRACH occasion only if slot is UL
        for (int t = 0; t < prach_info.N_t_slot; t++) { // td occasions within a slot
          // see 5.3.2 in 211
          int temp_format = prach_info.format >> 8; // B1, B2 or B3 if A1/B1, A2/B2 or A3/B3 formats in tables
          if (t != prach_info.N_t_slot - 1 || temp_format == 0xff)
            temp_format = prach_info.format & 0xff;
          int start_symbol = prach_info.start_symbol + t * prach_info.N_dur;
          int end_symbol = start_symbol + prach_info.N_dur;
          // valid occasion only if PRACH symbols are UL symbols in mixed slot
          if (fs->frame_type == TDD && !check_mixed_slot_prach(fs, slot, start_symbol, end_symbol))
            continue;
          for (int f = 0; f < ra->num_fd_occasions; f++) { // fd occasions
            ra_occasions_period[num_ra_occasions_period] =
                (prach_occasion_info_t){.slot = slot,
                                        .frame_info[0] = prach_info.x,
                                        .frame_info[1] = n == 0 ? prach_info.y : prach_info.y2,
                                        .start_symbol = start_symbol,
                                        .fdm = f,
                                        .format = temp_format};
            LOG_D(NR_MAC,
                  "RA occasion %d: slot %d start symbol %d fd occasion %d\n",
                  num_ra_occasions_period,
                  slot,
                  start_symbol,
                  f);
            num_ra_occasions_period++;
          }
        }
      }
      sf++;
    }
  }

  int config_period = prach_info.x; // configuration period
  ra->association_periods = 1;
  int nb_eq_ssb = mac->ssb_list.nb_tx_ssb;
  if (ra->ssb_ro_config.ssb_per_ro < 1)
    nb_eq_ssb *= (int)(1 / ra->ssb_ro_config.ssb_per_ro);
  int nb_eq_ro = num_ra_occasions_period;
  if (ra->ssb_ro_config.ssb_per_ro > 1)
    nb_eq_ro *= (int)ra->ssb_ro_config.ssb_per_ro;
  while (nb_eq_ssb > nb_eq_ro) {
    // not enough PRACH occasions -> need to increase association period
    ra->association_periods <<= 1;
    AssertFatal(ra->association_periods * config_period <= 16,
                "Cannot find an association period for %d SSB and %d RO with %f SSB per RO\n",
                mac->ssb_list.nb_tx_ssb,
                num_ra_occasions_period,
                ra->ssb_ro_config.ssb_per_ro);
    nb_eq_ro <<= 1; // doubling the association period -> doubling ROs
  }
  LOG_D(NR_MAC, "PRACH configuration period %d association period %d\n", config_period, ra->association_periods);

  select_prach_occasion(ra, mac->ssb_list.nb_tx_ssb, max_num_occasions, ra_occasions_period, num_ra_occasions_period);
  const prach_occasion_info_t *pi = &ra->sched_ro_info;
  LOG_I(NR_MAC,
        "[UE %d] selected PRACH occasion: start_symbol %d fdm %d slot %d format %d\n",
        mac->ue_id,
        pi->start_symbol,
        pi->fdm,
        pi->slot,
        pi->format);
}

/* TS 38.321 subclause 7.3 - return DELTA_PREAMBLE values in dB */
static int nr_get_delta_preamble(int scs, int prach_format)
{
  int delta = 0;
  switch (prach_format) {
    case 0:
    case 3:
      delta = 0;
      break;
    case 1:
      delta = -3;
      break;
    case 2:
      delta = -6;
      break;
    case 0xa1:
    case 0xb1:
      delta = 8 + 3 * scs;
      break;
    case 0xa2:
    case 0xb2:
    case 0xc2:
      delta = 5 + 3 * scs;
      break;
    case 0xa3:
    case 0xb3:
      delta = 3 + 3 * scs;
      break;
    case 0xb4:
      delta = 3 * scs;
      break;
    case 0xc0:
      delta = 11 + 3 * scs;
      break;
    default:
      AssertFatal(false, "Invalid preamble format %x\n", prach_format);
  }
  return delta;
}

static int get_ra_preamble_rx_target_power(RA_config_t *ra, int scs)
{
  NR_PRACH_RESOURCES_t *prach_resources = &ra->prach_resources;
  int delta_preamble = nr_get_delta_preamble(scs, ra->sched_ro_info.format);
  int tp = ra->preambleReceivedTargetPower_config + delta_preamble + prach_resources->power_offset_2step;
  tp += (prach_resources->preamble_power_ramping_cnt - 1) * prach_resources->preamble_power_ramping_step;
  return tp;
}

// 38.321 Sections 5.1.3 and .3a
static void ra_preamble_msga_transmission(RA_config_t *ra, int scs)
{
  NR_PRACH_RESOURCES_t *prach_resources = &ra->prach_resources;
  if (prach_resources->preamble_tx_counter > 1) {
    // TODO if the notification of suspending power ramping counter has not been received from lower layers (not implemented)
    //      this happens if the L1 does not transmit PRACH of if the spacial filter changed (UE transmitting in another beam)
    // TODO if LBT failure indication was not received from lower layers (LBT not implemented)
    // if SSB or CSI-RS selected is not changed from the selection in the last Random Access Preamble transmission
    if (!ra->new_ssb)
      prach_resources->preamble_power_ramping_cnt++;
  }


  prach_resources->ra_preamble_rx_target_power = get_ra_preamble_rx_target_power(ra, scs);

  // 3GPP TS 38.321 Section 5.1.3 says t_id for RA-RNTI depends on mu as specified in clause 5.3.2 in TS 38.211
  // so mu = 0 for prach format < 4.
  const int slot = ((ra->sched_ro_info.format & 0xff) < 4) ? ra->sched_ro_info.slot >> scs : ra->sched_ro_info.slot;
  ra->ra_rnti = nr_get_ra_rnti(ra->sched_ro_info.start_symbol, slot, ra->sched_ro_info.fdm, 0);
  if (ra->ra_type == RA_2_STEP)
    ra->MsgB_rnti = nr_get_MsgB_rnti(ra->sched_ro_info.start_symbol, slot, ra->sched_ro_info.fdm, 0);
}

// 38.321 Section 5.1.2 Random Access Resource selection
void ra_resource_selection(NR_UE_MAC_INST_t *mac)
{
  configure_ra_preamble(mac);
  const NR_UE_UL_BWP_t *current_UL_BWP = mac->current_UL_BWP;
  const int ul_mu = mac->current_UL_BWP->scs;
  const int mu = nr_get_prach_or_ul_mu(mac->current_UL_BWP->msgA_ConfigCommon_r16, current_UL_BWP->rach_ConfigCommon, ul_mu);
  configure_prach_occasions(mac, mu);
  ra_preamble_msga_transmission(&mac->ra, mu);
}

static int nr_get_RA_window_2Step_v17(long msgB_ResponseWindow)
{
  switch (msgB_ResponseWindow) {
    case NR_RACH_ConfigGenericTwoStepRA_r16__ext1__msgB_ResponseWindow_v1700_sl240:
      return 240;
      break;
    case NR_RACH_ConfigGenericTwoStepRA_r16__ext1__msgB_ResponseWindow_v1700_sl640:
      return 640;
      break;
    case NR_RACH_ConfigGenericTwoStepRA_r16__ext1__msgB_ResponseWindow_v1700_sl960:
      return 960;
      break;
    case NR_RACH_ConfigGenericTwoStepRA_r16__ext1__msgB_ResponseWindow_v1700_sl1280:
      return 1280;
      break;
    case NR_RACH_ConfigGenericTwoStepRA_r16__ext1__msgB_ResponseWindow_v1700_sl1920:
      return 1920;
      break;
    case NR_RACH_ConfigGenericTwoStepRA_r16__ext1__msgB_ResponseWindow_v1700_sl2560:
      return 2590;
    default:
      AssertFatal(false, "illegal msgB_responseWindow value %ld\n", msgB_ResponseWindow);
      break;
  }
  return 0;
}

static int nr_get_RA_window_2Step_v16(long msgB_ResponseWindow)
{
  switch (msgB_ResponseWindow) {
    case NR_RACH_ConfigGenericTwoStepRA_r16__msgB_ResponseWindow_r16_sl1:
      return 1;
      break;
    case NR_RACH_ConfigGenericTwoStepRA_r16__msgB_ResponseWindow_r16_sl2:
      return 2;
      break;
    case NR_RACH_ConfigGenericTwoStepRA_r16__msgB_ResponseWindow_r16_sl4:
      return 4;
      break;
    case NR_RACH_ConfigGenericTwoStepRA_r16__msgB_ResponseWindow_r16_sl8:
      return 8;
      break;
    case NR_RACH_ConfigGenericTwoStepRA_r16__msgB_ResponseWindow_r16_sl10:
      return 10;
      break;
    case NR_RACH_ConfigGenericTwoStepRA_r16__msgB_ResponseWindow_r16_sl20:
      return 20;
      break;
    case NR_RACH_ConfigGenericTwoStepRA_r16__msgB_ResponseWindow_r16_sl40:
      return 40;
      break;
    case NR_RACH_ConfigGenericTwoStepRA_r16__msgB_ResponseWindow_r16_sl80:
      return 80;
      break;
    case NR_RACH_ConfigGenericTwoStepRA_r16__msgB_ResponseWindow_r16_sl160:
      return 160;
      break;
    case NR_RACH_ConfigGenericTwoStepRA_r16__msgB_ResponseWindow_r16_sl320:
      return 360;
      break;
    default:
      AssertFatal(false, "illegal msgB_responseWindow value %ld\n", msgB_ResponseWindow);
      break;
  }
  return 0;
}

static int nr_get_RA_window_4Step_v16(long ra_ResponseWindow)
{
  switch (ra_ResponseWindow) {
    case NR_RACH_ConfigGeneric__ext1__ra_ResponseWindow_v1610_sl60:
      return 60;
    case NR_RACH_ConfigGeneric__ext1__ra_ResponseWindow_v1610_sl160:
      return 160;
    default:
      AssertFatal(false, "illegal ra_ResponseWindow value %ld\n", ra_ResponseWindow);
  }
  return 0;
}

static int nr_get_RA_window_4Step_v17(long ra_ResponseWindow)
{
  switch (ra_ResponseWindow) {
    case NR_RACH_ConfigGeneric__ext2__ra_ResponseWindow_v1700_sl240:
      return 240;
    case NR_RACH_ConfigGeneric__ext2__ra_ResponseWindow_v1700_sl320:
      return 320;
    case NR_RACH_ConfigGeneric__ext2__ra_ResponseWindow_v1700_sl640:
      return 640;
    case NR_RACH_ConfigGeneric__ext2__ra_ResponseWindow_v1700_sl960:
      return 960;
    case NR_RACH_ConfigGeneric__ext2__ra_ResponseWindow_v1700_sl1280:
      return 1280;
    case NR_RACH_ConfigGeneric__ext2__ra_ResponseWindow_v1700_sl1920:
      return 1920;
    case NR_RACH_ConfigGeneric__ext2__ra_ResponseWindow_v1700_sl2560:
      return 2590;
    default:
      AssertFatal(false, "illegal msgB_responseWindow value %ld\n", ra_ResponseWindow);
  }
  return 0;
}

static int nr_get_RA_window_4Step(long ra_ResponseWindow)
{
  switch (ra_ResponseWindow) {
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl1:
      return 1;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl2:
      return 2;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl4:
      return 4;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl8:
      return 8;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl10:
      return 10;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl20:
      return 20;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl40:
      return 40;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl80:
      return 80;
      break;
    default:
      AssertFatal(false, "illegal ra_ResponseWindow value %ld\n", ra_ResponseWindow);
      break;
  }
  return 0;
}

static void setup_ra_response_window(NR_UE_MAC_INST_t *mac,
                                     RA_config_t *ra,
                                     NR_RACH_ConfigGeneric_t *configGeneric,
                                     NR_RACH_ConfigGenericTwoStepRA_r16_t *twostep)
{
  const int slots_per_frame = mac->frame_structure.numb_slots_frame;
  int respwind_value = 0;
  if (ra->ra_type == RA_2_STEP) {
    long *msgB_ResponseWindow = twostep->msgB_ResponseWindow_r16;
    if (msgB_ResponseWindow)
      respwind_value = nr_get_RA_window_2Step_v16(*msgB_ResponseWindow);
    if (twostep->ext1 && twostep->ext1->msgB_ResponseWindow_v1700) {
      AssertFatal(msgB_ResponseWindow == NULL,
                  "The network does not configure msgB-ResponseWindow-r16 simultaneously with msgB-ResponseWindow-v1700\n");
      msgB_ResponseWindow = twostep->ext1->msgB_ResponseWindow_v1700;
      if (msgB_ResponseWindow)
        respwind_value = nr_get_RA_window_2Step_v17(*msgB_ResponseWindow);
    }
    // The network configures a value lower than or equal to 40ms
    int slots_40ms = 4 * slots_per_frame;
    AssertFatal(respwind_value != 0 && respwind_value <= slots_40ms, "Invalid MSGB response window value\n");
  } else {
    AssertFatal(ra->ra_type == RA_4_STEP, "Invalid RA type\n");
    long *response_window = NULL;
    if (configGeneric->ext1 && configGeneric->ext1->ra_ResponseWindow_v1610)
      respwind_value = nr_get_RA_window_4Step_v16(*configGeneric->ext1->ra_ResponseWindow_v1610);
    if (configGeneric->ext2 && configGeneric->ext2->ra_ResponseWindow_v1700) {
      AssertFatal(response_window == NULL, "ra_ResponseWindow_v1610 and ra_ResponseWindow_v1700 are mutually exclusive\n");
      respwind_value = nr_get_RA_window_4Step_v17(*configGeneric->ext2->ra_ResponseWindow_v1700);
    }
    if (!response_window)
      respwind_value = nr_get_RA_window_4Step(configGeneric->ra_ResponseWindow);
    // The network configures a value lower than or equal to 10ms
    AssertFatal(respwind_value != 0, "Invalid RAR response window value\n");
    if (respwind_value > slots_per_frame)
      LOG_E(NR_MAC, "RA-ResponseWindow need to be configured to a value lower than or equal to 10 ms\n");
  }

  const int ntn_ue_koffset = GET_NTN_UE_K_OFFSET(&mac->phy_config.config_req.ntn_config, mac->current_UL_BWP->scs);
  ra->response_window_setup_time = respwind_value + ntn_ue_koffset;
}

// Random Access procedure initialization as per 5.1.1 and initialization of variables specific
// to Random Access type as specified in clause 5.1.1a (3GPP TS 38.321 version 16.2.1 Release 16)
bool init_RA(NR_UE_MAC_INST_t *mac)
{
  RA_config_t *ra = &mac->ra;
  LOG_D(NR_MAC, "Initialization of RA\n");
  ra->ra_state = nrRA_GENERATE_PREAMBLE;

  // TODO this piece of code is required to compute MSG3_size that is used by ra_preambles_config function
  // Not a good implementation, it needs improvements
  if (!IS_SA_MODE(get_softmodem_params()))
    ra->Msg3_size = sizeof(uint16_t) + sizeof(NR_MAC_SUBHEADER_FIXED);

  // Random acces procedure initialization
  mac->state = UE_PERFORMING_RA;
  ra->RA_active = true;
  NR_PRACH_RESOURCES_t *prach_resources = &ra->prach_resources;
  fapi_nr_config_request_t *cfg = &mac->phy_config.config_req;
  // flush MSG3 buffer
  free_and_zero(ra->Msg3_buffer);
  // set the PREAMBLE_TRANSMISSION_COUNTER to 1
  prach_resources->preamble_tx_counter = 1;
  // set the PREAMBLE_POWER_RAMPING_COUNTER to 1
  prach_resources->preamble_power_ramping_cnt = 1;
  // set POWER_OFFSET_2STEP_RA to 0 dB
  prach_resources->power_offset_2step = 0;

  // perform the BWP operation as specified in clause 5.15
  // if PRACH occasions are not configured for the active UL BWP
  if (!mac->current_UL_BWP->rach_ConfigCommon) {
    // switch the active UL BWP to BWP indicated by initialUplinkBWP
    mac->current_UL_BWP = get_ul_bwp_structure(mac, 0, false);
    // if the Serving Cell is an SpCell
    // switch the active DL BWP to BWP indicated by initialDownlinkBWP
    mac->current_DL_BWP = get_dl_bwp_structure(mac, 0, false);
  } else {
    // if the active DL BWP does not have the same bwp-Id as the active UL BWP
    if (mac->current_UL_BWP->bwp_id != mac->current_DL_BWP->bwp_id) {
      // switch the active DL BWP to the DL BWP with the same bwp-Id as the active UL BWP
      mac->current_DL_BWP = get_dl_bwp_structure(mac, 0, false);
    }
  }

  const NR_UE_UL_BWP_t *current_UL_BWP = mac->current_UL_BWP;
  NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon = current_UL_BWP->rach_ConfigCommon;
  AssertFatal(nr_rach_ConfigCommon, "rach-ConfigCommon should be configured here\n");
  // stop the bwp-InactivityTimer associated with the active DL BWP of this Serving Cell, if running
  // TODO bwp-InactivityTimer not implemented

  // if the carrier to use for the Random Access procedure is explicitly signalled (always the case for us)
  // PRACH shall be as specified for QPSK modulated DFT-s-OFDM of equivalent RB allocation (38.101-1)
  NR_SubcarrierSpacing_t prach_scs;
  int scs_for_pcmax; // for long prach the UL BWP SCS is used for calculating RA_PCMAX
  NR_RACH_ConfigGeneric_t *rach_ConfigGeneric = &nr_rach_ConfigCommon->rach_ConfigGeneric;
  if (nr_rach_ConfigCommon->msg1_SubcarrierSpacing) {
    prach_scs = *nr_rach_ConfigCommon->msg1_SubcarrierSpacing;
    scs_for_pcmax = prach_scs;
  } else {
    const unsigned int index = rach_ConfigGeneric->prach_ConfigurationIndex;
    const unsigned int unpaired = mac->phy_config.config_req.cell_config.frame_duplex_type;
    const unsigned int format = get_format0(index, unpaired, mac->frequency_range);
    prach_scs = get_delta_f_RA_long(format);
    scs_for_pcmax = mac->current_UL_BWP->scs;
  }
  int n_prbs = get_N_RA_RB(prach_scs, current_UL_BWP->scs);
  int start_prb = rach_ConfigGeneric->msg1_FrequencyStart + current_UL_BWP->BWPStart;
  prach_resources->Pc_max = nr_get_Pcmax(mac->p_Max,
                                         mac->nr_band,
                                         mac->frame_structure.frame_type,
                                         mac->frequency_range,
                                         current_UL_BWP->channel_bandwidth,
                                         2,
                                         false,
                                         scs_for_pcmax,
                                         cfg->carrier_config.dl_grid_size[scs_for_pcmax],
                                         true,
                                         n_prbs,
                                         start_prb);


  // TODO if the Random Access procedure was initiated for SI request
  // and the Random Access Resources for SI request have been explicitly provided by RRC

  // TODO if the Random Access procedure was initiated for SpCell beam failure recovery
  // and if the contention-free Random Access Resources for beam failure recovery request for 4-step RA type
  // have been explicitly provided by RRC for the BWP selected for Random Access procedure

  NR_RACH_ConfigCommonTwoStepRA_r16_t *twostep_conf = NULL;
  NR_RACH_ConfigDedicated_t *rach_Dedicated = ra->rach_ConfigDedicated;

  // if the Random Access procedure is initiated by PDCCH order
  // and if the ra-PreambleIndex explicitly provided by PDCCH is not 0b000000
  bool pdcch_order = (ra->pdcch_order.active && ra->pdcch_order.preamble_index != 0xb000000) ? true : false;
  // or if the Random Access procedure was initiated for reconfiguration with sync and
  // if the contention-free Random Access Resources for 4-step RA type have been explicitly provided
  // in rach-ConfigDedicated for the BWP selected for Random Access procedure
  if ((rach_Dedicated && rach_Dedicated->cfra) || pdcch_order) {
    LOG_I(MAC, "Initialization of 4-Step CFRA procedure\n");
    ra->ra_type = RA_4_STEP;
    ra->cfra = true;
  } else {
    bool twostep_cfra = (rach_Dedicated && rach_Dedicated->ext1) ? (rach_Dedicated->ext1->cfra_TwoStep_r16 ? true : false) : false;
    if (twostep_cfra) {
      // if the Random Access procedure was initiated for reconfiguration with sync and
      // if the contention-free Random Access Resources for 2-step RA type have been explicitly provided in rach-ConfigDedicated
      LOG_I(MAC, "Initialization of 2-Step CFRA procedure\n");
      ra->ra_type = RA_2_STEP;
      ra->cfra = true;
    } else {
      bool twostep = false;
      if (current_UL_BWP->msgA_ConfigCommon_r16) {
        twostep_conf = &current_UL_BWP->msgA_ConfigCommon_r16->rach_ConfigCommonTwoStepRA_r16;
        if (nr_rach_ConfigCommon) {
          // if the BWP selected for Random Access procedure is configured with both 2- and 4-step RA type Random Access Resources
          // and the RSRP of the downlink pathloss reference is above msgA-RSRP-Threshold
          AssertFatal(twostep_conf->msgA_RSRP_Threshold_r16,
                      "msgA_RSRP_Threshold_r16 is mandatory present if both 2-step and 4-step random access types are configured\n");
          // For thresholds the RSRP value is (IE value – 156) dBm except for the IE 127 in which case the actual value is infinity
          int rsrp_msga_thr = *twostep_conf->msgA_RSRP_Threshold_r16 - 156;
          if (*twostep_conf->msgA_RSRP_Threshold_r16 != 127 && mac->ssb_measurements[mac->mib_ssb].ssb_rsrp_dBm > rsrp_msga_thr)
            twostep = true;
        } else {
          // if the BWP selected for Random Access procedure is only configured with 2-step RA type Random Access
          twostep = true;
        }
      }
      if (twostep) {
        LOG_I(MAC, "Initialization of 2-Step CBRA procedure\n");
        ra->ra_type = RA_2_STEP;
        ra->cfra = false;
      } else {
        LOG_I(MAC, "Initialization of 4-Step CBRA procedure\n");
        ra->ra_type = RA_4_STEP;
        ra->cfra = false;
      }
    }
  }

  // need to ask RRC to send the MSG3 data to transmit if needed (CBRA originating from upper layers)
  if (!mac->msg3_C_RNTI && ra->cfra == false)
    nr_mac_rrc_msg3_ind(mac->ue_id, 0, true);

  NR_RACH_ConfigGenericTwoStepRA_r16_t *twostep_generic = twostep_conf ? &twostep_conf->rach_ConfigGenericTwoStepRA_r16 : NULL;
  if (ra->ra_type == RA_2_STEP && twostep_generic && twostep_generic->msgA_PreamblePowerRampingStep_r16)
    prach_resources->preamble_power_ramping_step = *twostep_generic->msgA_PreamblePowerRampingStep_r16 << 1;
  else
    prach_resources->preamble_power_ramping_step = rach_ConfigGeneric->powerRampingStep << 1;

  ra->scaling_factor_bi = 1;

  if (ra->ra_type == RA_2_STEP && twostep_generic && twostep_generic->msgA_PreambleReceivedTargetPower_r16)
    ra->preambleReceivedTargetPower_config = *twostep_generic->msgA_PreambleReceivedTargetPower_r16;
  else
    ra->preambleReceivedTargetPower_config = rach_ConfigGeneric->preambleReceivedTargetPower;

  if (ra->ra_type == RA_2_STEP && twostep_generic && twostep_generic->preambleTransMax_r16)
    set_preambleTransMax(ra, *twostep_generic->preambleTransMax_r16);
  else
    set_preambleTransMax(ra, rach_ConfigGeneric->preambleTransMax);

  if (ra->ra_type == RA_2_STEP && twostep_generic && twostep_generic->msgA_ZeroCorrelationZoneConfig_r16)
    ra->zeroCorrelationZoneConfig = *twostep_generic->msgA_ZeroCorrelationZoneConfig_r16;
  else
    ra->zeroCorrelationZoneConfig = rach_ConfigGeneric->zeroCorrelationZoneConfig;
  if (ra->ra_type == RA_2_STEP && twostep_conf && twostep_conf->msgA_RestrictedSetConfig_r16)
    ra->restricted_set_config = *twostep_conf->msgA_RestrictedSetConfig_r16;
  else
    ra->restricted_set_config = nr_rach_ConfigCommon->restrictedSetConfig;

  // TODO msgA-TransMax not implemented
  // TODO ra-Prioritization not implemented

  if (ra->ra_type == RA_2_STEP && twostep_generic && twostep_generic->msgA_PreambleReceivedTargetPower_r16)
    ra->preambleRxTargetPower = *twostep_generic->msgA_PreambleReceivedTargetPower_r16;
  else
    ra->preambleRxTargetPower = rach_ConfigGeneric->preambleReceivedTargetPower;

  if (ra->ra_type == RA_2_STEP
      && current_UL_BWP->msgA_ConfigCommon_r16
      && current_UL_BWP->msgA_ConfigCommon_r16->msgA_PUSCH_Config_r16
      && current_UL_BWP->msgA_ConfigCommon_r16->msgA_PUSCH_Config_r16->msgA_DeltaPreamble_r16)
    ra->msg3_deltaPreamble = *current_UL_BWP->msgA_ConfigCommon_r16->msgA_PUSCH_Config_r16->msgA_DeltaPreamble_r16 << 1;
  else
    ra->msg3_deltaPreamble = mac->current_UL_BWP->msg3_DeltaPreamble ? *mac->current_UL_BWP->msg3_DeltaPreamble << 1 : 0;

  ra->new_ssb = false;

  if (ra->ra_type == RA_2_STEP && twostep_generic && twostep_generic->msgA_PRACH_ConfigurationIndex_r16)
    ra->ra_config_index = *twostep_generic->msgA_PRACH_ConfigurationIndex_r16;
  else {
    if (rach_ConfigGeneric->ext1 && rach_ConfigGeneric->ext1->prach_ConfigurationIndex_v1610)
      ra->ra_config_index = *rach_ConfigGeneric->ext1->prach_ConfigurationIndex_v1610;
    else
      ra->ra_config_index = rach_ConfigGeneric->prach_ConfigurationIndex;
  }

  if (ra->ra_type == RA_2_STEP && twostep_generic && twostep_generic->msgA_RO_FDM_r16)
    ra->num_fd_occasions = 1 << *twostep_generic->msgA_RO_FDM_r16;
  else
    ra->num_fd_occasions = 1 << rach_ConfigGeneric->msg1_FDM;

  setup_ra_response_window(mac, ra, rach_ConfigGeneric, twostep_generic);

  return true;
}

void nr_Msg3_transmitted(NR_UE_MAC_INST_t *mac)
{
  RA_config_t *ra = &mac->ra;
  NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon = mac->current_UL_BWP->rach_ConfigCommon;
  const int ntn_ue_koffset = GET_NTN_UE_K_OFFSET(&mac->phy_config.config_req.ntn_config, mac->current_UL_BWP->scs);
  const int slots_per_ms = mac->frame_structure.numb_slots_frame / 10;

  // start contention resolution timer
  const int RA_contention_resolution_timer_ms = (nr_rach_ConfigCommon->ra_ContentionResolutionTimer + 1) << 3;
  const int RA_contention_resolution_timer_slots = RA_contention_resolution_timer_ms * slots_per_ms;

  // timer step 1 slot and timer target given by ra_ContentionResolutionTimer + ta-Common-r17
  nr_timer_setup(&ra->contention_resolution_timer, RA_contention_resolution_timer_slots + ntn_ue_koffset, 1);
  nr_timer_start(&ra->contention_resolution_timer);

  ra->ra_state = nrRA_WAIT_CONTENTION_RESOLUTION;
}

static uint8_t *fill_msg3_crnti_pdu(RA_config_t *ra, uint8_t *pdu, uint16_t crnti)
{
  // RA triggered by UE MAC with C-RNTI in MAC CE
  LOG_D(NR_MAC, "Generating MAC CE with C-RNTI for MSG3 %x\n", crnti);
  *(NR_MAC_SUBHEADER_FIXED *)pdu = (NR_MAC_SUBHEADER_FIXED){.LCID = UL_SCH_LCID_C_RNTI, .R = 0};
  pdu += sizeof(NR_MAC_SUBHEADER_FIXED);

  // C-RNTI MAC CE (2 octets)
  uint16_t rnti_pdu = ((crnti & 0xFF) << 8) | ((crnti >> 8) & 0xFF);
  memcpy(pdu, &rnti_pdu, sizeof(rnti_pdu));
  pdu += sizeof(rnti_pdu);
  ra->t_crnti = crnti;
  return pdu;
}

static uint8_t *fill_msg3_pdu_from_rlc(NR_UE_MAC_INST_t *mac, uint8_t *pdu, int TBS_max)
{
  RA_config_t *ra = &mac->ra;
  // regular Msg3/MsgA_PUSCH with PDU coming from higher layers
  *(NR_MAC_SUBHEADER_FIXED *)pdu = (NR_MAC_SUBHEADER_FIXED){.LCID = UL_SCH_LCID_CCCH_48_BITS};
  pdu += sizeof(NR_MAC_SUBHEADER_FIXED);
  tbs_size_t len = nr_mac_rlc_data_req(mac->ue_id,
                                       mac->ue_id,
                                       false,
                                       0, // SRB0 for messages sent in MSG3
                                       TBS_max - sizeof(NR_MAC_SUBHEADER_FIXED), /* size of mac_ce above */
                                       (char *)pdu);
  AssertFatal(len > 0, "no data for Msg3/MsgA_PUSCH\n");
  // UE Contention Resolution Identity
  // Store the first 48 bits belonging to the uplink CCCH SDU within Msg3 to determine whether or not the
  // Random Access Procedure has been successful after reception of Msg4
  // We copy from persisted memory to another persisted memory
  memcpy(ra->cont_res_id, pdu, sizeof(uint8_t) * 6);
  pdu += len;
  return pdu;
}

void nr_get_Msg3_MsgA_PUSCH_payload(NR_UE_MAC_INST_t *mac, uint8_t *buf, int TBS_max)
{
  RA_config_t *ra = &mac->ra;

  // we already stored MSG3 in the buffer, we can use that
  if (ra->Msg3_buffer) {
    memcpy(buf, ra->Msg3_buffer, sizeof(uint8_t) * TBS_max);
    return;
  }

  uint8_t *pdu = buf;
  if (mac->msg3_C_RNTI)
    pdu = fill_msg3_crnti_pdu(ra, pdu, mac->crnti);
  else
    pdu = fill_msg3_pdu_from_rlc(mac, pdu, TBS_max);

  AssertFatal(TBS_max >= pdu - buf, "Allocated resources are not enough for Msg3/MsgA_PUSCH!\n");
  // Padding: fill remainder with 0
  LOG_D(NR_MAC, "Remaining %ld bytes, filling with padding\n", pdu - buf);
  while (pdu < buf + TBS_max - sizeof(NR_MAC_SUBHEADER_FIXED)) {
    *(NR_MAC_SUBHEADER_FIXED *)pdu = (NR_MAC_SUBHEADER_FIXED){.LCID = UL_SCH_LCID_PADDING};
    pdu += sizeof(NR_MAC_SUBHEADER_FIXED);
  }
  ra->Msg3_buffer = calloc_or_fail(TBS_max, sizeof(uint8_t));
  memcpy(ra->Msg3_buffer, buf, sizeof(uint8_t) * TBS_max);
}

// Handlig successful RA completion @ MAC layer
// according to section 5 of 3GPP TS 38.321 version 16.2.1 Release 16
// todo:
// - complete handling of received contention-based RA preamble
void nr_ra_succeeded(NR_UE_MAC_INST_t *mac, const frame_t frame, const int slot)
{
  RA_config_t *ra = &mac->ra;

  if (ra->cfra) {
    LOG_A(NR_MAC, "[UE %d][%d.%d][RAPROC] RA procedure succeeded. CFRA: RAR successfully received.\n", mac->ue_id, frame, slot);
  } else {
    LOG_A(NR_MAC,
          "[UE %d][%d.%d][RAPROC] %d-Step RA procedure succeeded. CBRA: Contention Resolution is successful.\n",
          mac->ue_id,
          frame,
          slot,
          ra->ra_type == RA_2_STEP ? 2 : 4);
    mac->crnti = ra->t_crnti;
    ra->t_crnti = 0;
    nr_timer_stop(&ra->contention_resolution_timer);
  }

  // rach-ConfigDedicated is one shot configuration to be used only for a specific instance of reconfiguration with sync
  if (ra->rach_ConfigDedicated)
    asn1cFreeStruc(asn_DEF_NR_RACH_ConfigDedicated, ra->rach_ConfigDedicated);

  ra->RA_active = false;
  mac->msg3_C_RNTI = false;
  ra->ra_state = nrRA_SUCCEEDED;
  mac->state = UE_CONNECTED;
  free_and_zero(ra->Msg3_buffer);
  nr_mac_rrc_ra_ind(mac->ue_id, true);
}

void nr_ra_backoff_setting(RA_config_t *ra)
{
  // select a random backoff time according to a uniform distribution
  // between 0 and the PREAMBLE_BACKOFF
  uint32_t seed = (unsigned int)(rdtsc_oai() & ~0);
  uint32_t random_backoff = ra->RA_backoff_limit ? rand_r(&seed) % ra->RA_backoff_limit : 0; // in slots
  nr_timer_setup(&ra->RA_backoff_timer, random_backoff, 1);
  nr_timer_start(&ra->RA_backoff_timer);
}

void nr_ra_contention_resolution_failed(NR_UE_MAC_INST_t *mac)
{
  LOG_W(MAC, "[UE %d] Contention resolution failed\n", mac->ue_id);
  RA_config_t *ra = &mac->ra;
  // discard the TEMPORARY_C-RNTI
  ra->t_crnti = 0;
  // flush MSG3 buffer
  free_and_zero(ra->Msg3_buffer);
  // MSG3 with C-RNTI is a L2 procedure, we shouldn't send any indication to RRC
  if (!mac->msg3_C_RNTI)
    nr_mac_rrc_msg3_ind(mac->ue_id, 0, true);
  NR_PRACH_RESOURCES_t *prach_resources = &ra->prach_resources;
  prach_resources->preamble_tx_counter++;
  if (prach_resources->preamble_tx_counter == ra->preambleTransMax + 1) {
    // indicate a Random Access problem to upper layers
    nr_mac_rrc_ra_ind(mac->ue_id, false);
  } else {
    // TODO handle msgA-TransMax (go back to 4-step if the threshold is reached)
    // starting backoff time
    nr_ra_backoff_setting(ra);
  }
}

void nr_rar_not_successful(NR_UE_MAC_INST_t *mac)
{
  LOG_W(MAC, "[UE %d] RAR reception failed\n", mac->ue_id);
  RA_config_t *ra = &mac->ra;
  NR_PRACH_RESOURCES_t *prach_resources = &ra->prach_resources;
  prach_resources->preamble_tx_counter++;
  bool ra_completed = false;
  if (prach_resources->preamble_tx_counter == ra->preambleTransMax + 1) {
    // if the Random Access Preamble is transmitted on the SpCell
    // TODO to be verified, this means SA if I'm not mistaken
    if (IS_SA_MODE(get_softmodem_params())) {
      // indicate a Random Access problem to upper layers
      nr_mac_rrc_ra_ind(mac->ue_id, false);
    } else {
      // if the Random Access Preamble is transmitted on an SCell:
      // consider the Random Access procedure unsuccessfully completed.
      ra_completed = true;
      ra->ra_state = nrRA_UE_IDLE;
    }
  }
  if (!ra_completed) {
    nr_ra_backoff_setting(ra);
  }
}

void trigger_MAC_UE_RA(NR_UE_MAC_INST_t *mac, dci_pdu_rel15_t *pdcch_order)
{
  LOG_W(NR_MAC, "Triggering new RA procedure for UE with RNTI %x\n", mac->crnti);
  mac->state = UE_PERFORMING_RA;
  reset_ra(mac, false);
  RA_config_t *ra = &mac->ra;
  mac->msg3_C_RNTI = true;
  if (pdcch_order) {
    ra->pdcch_order.active = true;
    ra->pdcch_order.preamble_index = pdcch_order->ra_preamble_index;
    ra->pdcch_order.ssb_index = pdcch_order->ss_pbch_index;
    ra->pdcch_order.prach_mask = pdcch_order->prach_mask_index;
  }
}

void prepare_msg4_msgb_feedback(NR_UE_MAC_INST_t *mac, int pid, int ack_nack)
{
  NR_UE_DL_HARQ_STATUS_t *current_harq = &mac->dl_harq_info[pid][0]; // single cw for MSG4
  int sched_slot = current_harq->ul_slot;
  int sched_frame = current_harq->ul_frame;
  PUCCH_sched_t pucch = {.n_CCE = current_harq->n_CCE,
                         .N_CCE = current_harq->N_CCE,
                         .ack_payload = ack_nack,
                         .n_harq = 1};
  current_harq->active = false;
  current_harq->ack_received = false;
  const NR_UE_UL_BWP_t *current_UL_BWP = mac->current_UL_BWP;
  configure_initial_pucch(&pucch, current_harq->pucch_resource_indicator, current_UL_BWP->pucch_ConfigCommon->pucch_ResourceCommon);

  RA_config_t *ra = &mac->ra;
  ra->ra_pucch = calloc_or_fail(1, sizeof(*ra->ra_pucch));
  ra->ra_pucch->sched_frame = sched_frame;
  ra->ra_pucch->sched_slot = sched_slot;
  int ret = nr_ue_configure_pucch(mac, sched_slot, sched_frame, ra->t_crnti, &pucch, &ra->ra_pucch->pucch_pdu);
  AssertFatal(ret == 0, "Couldn't configure PUCCH for MSG4\n");
}

void reset_ra(NR_UE_MAC_INST_t *nr_mac, bool free_prach)
{
  RA_config_t *ra = &nr_mac->ra;
  if (ra->rach_ConfigDedicated)
    asn1cFreeStruc(asn_DEF_NR_RACH_ConfigDedicated, ra->rach_ConfigDedicated);
  memset(ra, 0, sizeof(RA_config_t));

  if (!free_prach)
    return;
}
