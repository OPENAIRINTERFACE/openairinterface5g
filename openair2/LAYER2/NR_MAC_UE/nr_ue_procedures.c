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

/* \file ue_procedures.c
 * \brief procedures related to UE
 * \author R. Knopp, K.H. HSU, G. Casati
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr, guido.casati@iis.fraunhofer.de
 * \note
 * \warning
 */


#include <stdio.h>
#include <math.h>

/* exe */
#include "executables/nr-softmodem.h"

/* RRC*/
#include "RRC/NR_UE/rrc_proto.h"
#include "NR_RACH-ConfigCommon.h"
#include "NR_RACH-ConfigGeneric.h"
#include "NR_FrequencyInfoDL.h"
#include "NR_PDCCH-ConfigCommon.h"

/* MAC */
#include "mac_defs.h"
#include "NR_MAC_COMMON/nr_mac.h"
#include "NR_MAC_UE/mac_proto.h"
#include "NR_MAC_UE/mac_extern.h"
#include "common/utils/nr/nr_common.h"

/* PHY */
#include "SCHED_NR_UE/defs.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/NR_TRANSPORT/nr_dci.h"

/*Openair Packet Tracer */
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "executables/softmodem-common.h"

/* utils */
#include "assertions.h"

/* Tools */
#include "asn1_conversions.h"
#include "SIMULATION/TOOLS/sim.h" // for taus

#include "common/utils/LOG/log.h"
#include "SIMULATION/TOOLS/sim.h" // for taus
#include "openair2/LAYER2/NR_MAC_COMMON/nr_mac.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include <stdio.h>
#include <math.h>

// ================================================
// SSB to RO mapping private defines and structures
// ================================================

#define MAX_NB_PRACH_CONF_PERIOD_IN_ASSOCIATION_PERIOD (16) // Maximum association period is 16
#define MAX_NB_PRACH_CONF_PERIOD_IN_ASSOCIATION_PATTERN_PERIOD (16) // Max association pattern period is 160ms and minimum PRACH configuration period is 10ms
#define MAX_NB_ASSOCIATION_PERIOD_IN_ASSOCIATION_PATTERN_PERIOD (16) // Max nb of association periods in an association pattern period of 160ms
#define MAX_NB_FRAME_IN_PRACH_CONF_PERIOD (16) // Max PRACH configuration period is 160ms and frame is 10ms
#define MAX_NB_SLOT_IN_FRAME (160) // Max number of slots in a frame (@ SCS 240kHz = 160)
#define MAX_NB_FRAME_IN_ASSOCIATION_PATTERN_PERIOD (16) // Maximum number of frames in the maximum association pattern period
#define MAX_NB_SSB (64) // Maximum number of possible SSB indexes
#define MAX_RO_PER_SSB (8) // Maximum number of consecutive ROs that can be mapped to an SSB according to the ssb_per_RACH config

// Maximum number of ROs that can be mapped to an SSB in an association pattern
// This is to reserve enough elements in the SSBs list for each mapped ROs for a single SSB
// An arbitrary maximum number is chosen to be safe: maximum number of slots in an association pattern * maximum number of ROs in a slot
#define MAX_NB_RO_PER_SSB_IN_ASSOCIATION_PATTERN (MAX_TDM*MAX_FDM*MAX_NB_SLOT_IN_FRAME*MAX_NB_FRAME_IN_ASSOCIATION_PATTERN_PERIOD)

// The PRACH Config period is a series of selected slots in one or multiple frames
typedef struct prach_conf_period {
  prach_occasion_slot_t prach_occasion_slot_map[MAX_NB_FRAME_IN_PRACH_CONF_PERIOD][MAX_NB_SLOT_IN_FRAME];
  uint16_t nb_of_prach_occasion; // Total number of PRACH occasions in the PRACH Config period
  uint8_t nb_of_frame; // Size of the PRACH Config period in number of 10ms frames
  uint8_t nb_of_slot; // Nb of slots in each frame
} prach_conf_period_t;

// The association period is a series of PRACH Config periods
typedef struct prach_association_period {
  prach_conf_period_t *prach_conf_period_list[MAX_NB_PRACH_CONF_PERIOD_IN_ASSOCIATION_PERIOD];
  uint8_t nb_of_prach_conf_period; // Nb of PRACH configuration periods within the association period
  uint8_t nb_of_frame; // Total number of frames included in the association period
} prach_association_period_t;

// The association pattern is a series of Association periods
typedef struct prach_association_pattern {
  prach_association_period_t prach_association_period_list[MAX_NB_ASSOCIATION_PERIOD_IN_ASSOCIATION_PATTERN_PERIOD];
  prach_conf_period_t prach_conf_period_list[MAX_NB_PRACH_CONF_PERIOD_IN_ASSOCIATION_PATTERN_PERIOD];
  uint8_t nb_of_assoc_period; // Nb of association periods within the association pattern
  uint8_t nb_of_prach_conf_period_in_max_period; // Nb of PRACH configuration periods within the maximum association pattern period (according to the size of the configured PRACH
  uint8_t nb_of_frame; // Total number of frames included in the association pattern period (after mapping the SSBs and determining the real association pattern length)
} prach_association_pattern_t;

// SSB details
typedef struct ssb_info {
  boolean_t transmitted; // True if the SSB index is transmitted according to the SSB positions map configuration
  prach_occasion_info_t *mapped_ro[MAX_NB_RO_PER_SSB_IN_ASSOCIATION_PATTERN]; // List of mapped RACH Occasions to this SSB index
  uint16_t nb_mapped_ro; // Total number of mapped ROs to this SSB index
} ssb_info_t;

// List of all the possible SSBs and their details
typedef struct ssb_list_info {
  ssb_info_t tx_ssb[MAX_NB_SSB];
  uint8_t   nb_tx_ssb;
} ssb_list_info_t;

static prach_association_pattern_t prach_assoc_pattern;
static ssb_list_info_t ssb_list;

// ===============================================
// ===============================================

//#define ENABLE_MAC_PAYLOAD_DEBUG 1
#define DEBUG_EXTRACT_DCI 1

extern int bwp_id;
extern dci_pdu_rel15_t *def_dci_pdu_rel15;

extern void mac_rlc_data_ind     (
				  const module_id_t         module_idP,
				  const rnti_t              rntiP,
				  const eNB_index_t         eNB_index,
				  const frame_t             frameP,
				  const eNB_flag_t          enb_flagP,
				  const MBMS_flag_t         MBMS_flagP,
				  const logical_chan_id_t   channel_idP,
				  char                     *buffer_pP,
				  const tb_size_t           tb_sizeP,
				  num_tb_t                  num_tbP,
				  crc_t                    *crcs_pP);

// Build the list of all the valid RACH occasions in the maximum association pattern period according to the PRACH config
static void build_ro_list(NR_ServingCellConfigCommon_t *scc, uint8_t unpaired) {

  int x,y; // PRACH Configuration Index table variables used to compute the valid frame numbers
  int y2;  // PRACH Configuration Index table additional variable used to compute the valid frame numbers
  uint8_t slot_shift_for_map;
  uint8_t map_shift;
  boolean_t even_slot_invalid;
  int64_t s_map;
  uint8_t prach_conf_start_symbol; // Starting symbol of the PRACH occasions in the PRACH slot
  uint8_t N_t_slot; // Number of PRACH occasions in a 14-symbols PRACH slot
  uint8_t N_dur; // Duration of a PRACH occasion (nb of symbols)
  uint8_t frame; // Maximum is NB_FRAMES_IN_MAX_ASSOCIATION_PATTERN_PERIOD
  uint8_t slot; // Maximum is the number of slots in a frame @ SCS 240kHz
  uint16_t format = 0xffff;
  uint8_t format2 = 0xff;
  int nb_fdm;

  uint8_t config_index, mu;
  uint32_t pointa;
  int msg1_FDM;

  uint8_t prach_conf_period_idx;
  uint8_t nb_of_frames_per_prach_conf_period;
  uint8_t prach_conf_period_frame_idx;
  int64_t *prach_config_info_p;

  NR_RACH_ConfigCommon_t *setup = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  NR_FrequencyInfoDL_t *frequencyInfoDL = scc->downlinkConfigCommon->frequencyInfoDL;
  NR_RACH_ConfigGeneric_t *rach_ConfigGeneric = &setup->rach_ConfigGeneric;

  config_index = rach_ConfigGeneric->prach_ConfigurationIndex;

  if (setup->msg1_SubcarrierSpacing)
    mu = *setup->msg1_SubcarrierSpacing;
  else
    mu = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;

  pointa = frequencyInfoDL->absoluteFrequencyPointA;
  msg1_FDM = rach_ConfigGeneric->msg1_FDM;

  switch (msg1_FDM){
    case 0:
    case 1:
    case 2:
    case 3:
      nb_fdm = 1 << msg1_FDM;
      break;
    default:
      AssertFatal(1 == 0, "Unknown msg1_FDM from rach_ConfigGeneric %d\n", msg1_FDM);
  }

  // Create the PRACH occasions map
  // ==============================
  // WIP: For now assume no rejected PRACH occasions because of conflict with SSB or TDD_UL_DL_ConfigurationCommon schedule

  // Identify the proper PRACH Configuration Index table according to the operating frequency
  LOG_D(MAC,"Pointa %u, mu = %u, PRACH config index  = %u, unpaired = %u\n", pointa, mu, config_index, unpaired);

  prach_config_info_p = get_prach_config_info(pointa, config_index, unpaired);

  if (pointa > 2016666) { //FR2

    x = prach_config_info_p[2];
    y = prach_config_info_p[3];
    y2 = prach_config_info_p[4];

    s_map = prach_config_info_p[5];

    prach_conf_start_symbol = prach_config_info_p[6];
    N_t_slot = prach_config_info_p[8];
    N_dur = prach_config_info_p[9];
    if (prach_config_info_p[1] != -1)
      format2 = (uint8_t) prach_config_info_p[1];
    format = ((uint8_t) prach_config_info_p[0]) | (format2<<8);

    slot_shift_for_map = mu-2;
    if ( (mu == 3) && (prach_config_info_p[7] == 1) )
      even_slot_invalid = true;
    else
      even_slot_invalid = false;
  }
  else { // FR1
    x = prach_config_info_p[2];
    y = prach_config_info_p[3];
    y2 = y;

    s_map = prach_config_info_p[4];

    prach_conf_start_symbol = prach_config_info_p[5];
    N_t_slot = prach_config_info_p[7];
    N_dur = prach_config_info_p[8];
    if (prach_config_info_p[1] != -1)
      format2 = (uint8_t) prach_config_info_p[1];
    format = ((uint8_t) prach_config_info_p[0]) | (format2<<8);

    slot_shift_for_map = mu;
    if ( (mu == 1) && (prach_config_info_p[6] <= 1) )
      // no prach in even slots @ 30kHz for 1 prach per subframe
      even_slot_invalid = true;
    else
      even_slot_invalid = false;
  } // FR2 / FR1

  prach_assoc_pattern.nb_of_prach_conf_period_in_max_period = MAX_NB_PRACH_CONF_PERIOD_IN_ASSOCIATION_PATTERN_PERIOD / x;
  nb_of_frames_per_prach_conf_period = x;

  LOG_D(MAC,"nb_of_prach_conf_period_in_max_period %d\n", prach_assoc_pattern.nb_of_prach_conf_period_in_max_period);

  // Fill in the PRACH occasions table for every slot in every frame in every PRACH configuration periods in the maximum association pattern period
  // ----------------------------------------------------------------------------------------------------------------------------------------------
  // ----------------------------------------------------------------------------------------------------------------------------------------------
  // For every PRACH configuration periods
  // -------------------------------------
  for (prach_conf_period_idx=0; prach_conf_period_idx<prach_assoc_pattern.nb_of_prach_conf_period_in_max_period; prach_conf_period_idx++) {
    prach_assoc_pattern.prach_conf_period_list[prach_conf_period_idx].nb_of_prach_occasion = 0;
    prach_assoc_pattern.prach_conf_period_list[prach_conf_period_idx].nb_of_frame = nb_of_frames_per_prach_conf_period;
    prach_assoc_pattern.prach_conf_period_list[prach_conf_period_idx].nb_of_slot = nr_slots_per_frame[mu];

    LOG_D(MAC,"PRACH Conf Period Idx %d\n", prach_conf_period_idx);

    // For every frames in a PRACH configuration period
    // ------------------------------------------------
    for (prach_conf_period_frame_idx=0; prach_conf_period_frame_idx<nb_of_frames_per_prach_conf_period; prach_conf_period_frame_idx++) {
      frame = (prach_conf_period_idx * nb_of_frames_per_prach_conf_period) + prach_conf_period_frame_idx;

      LOG_D(MAC,"PRACH Conf Period Frame Idx %d - Frame %d\n", prach_conf_period_frame_idx, frame);
      // Is it a valid frame for this PRACH configuration index? (n_sfn mod x = y)
      if ( (frame%x)==y || (frame%x)==y2 ) {

        // For every slot in a frame
        // -------------------------
        for (slot=0; slot<nr_slots_per_frame[mu]; slot++) {
          // Is it a valid slot?
          map_shift = slot >> slot_shift_for_map; // in PRACH configuration index table slots are numbered wrt 60kHz
          if ( (s_map>>map_shift)&0x01 ) {
            // Valid slot

            // Additionally, for 30kHz/120kHz, we must check for the n_RA_Slot param also
            if ( even_slot_invalid && (slot%2 == 0) )
                continue; // no prach in even slots @ 30kHz/120kHz for 1 prach per 60khz slot/subframe

            // We're good: valid frame and valid slot
            // Compute all the PRACH occasions in the slot

            uint8_t n_prach_occ_in_time;
            uint8_t n_prach_occ_in_freq;

            prach_assoc_pattern.prach_conf_period_list[prach_conf_period_idx].prach_occasion_slot_map[prach_conf_period_frame_idx][slot].nb_of_prach_occasion_in_time = N_t_slot;
            prach_assoc_pattern.prach_conf_period_list[prach_conf_period_idx].prach_occasion_slot_map[prach_conf_period_frame_idx][slot].nb_of_prach_occasion_in_freq = nb_fdm;

            for (n_prach_occ_in_time=0; n_prach_occ_in_time<N_t_slot; n_prach_occ_in_time++) {
              uint8_t start_symbol = prach_conf_start_symbol + n_prach_occ_in_time * N_dur;
              LOG_D(MAC,"PRACH Occ in time %d\n", n_prach_occ_in_time);

              for (n_prach_occ_in_freq=0; n_prach_occ_in_freq<nb_fdm; n_prach_occ_in_freq++) {
                prach_occasion_info_t *prach_occasion_p = &prach_assoc_pattern.prach_conf_period_list[prach_conf_period_idx].prach_occasion_slot_map[prach_conf_period_frame_idx][slot].prach_occasion[n_prach_occ_in_time][n_prach_occ_in_freq];

                prach_occasion_p->start_symbol = start_symbol;
                prach_occasion_p->fdm = n_prach_occ_in_freq;
                prach_occasion_p->frame = frame;
                prach_occasion_p->slot = slot;
                prach_occasion_p->format = format;
                prach_assoc_pattern.prach_conf_period_list[prach_conf_period_idx].nb_of_prach_occasion++;

                LOG_D(MAC,"Adding a PRACH occasion: frame %u, slot-symbol %d-%d, occ_in_time-occ_in-freq %d-%d, nb ROs in conf period %d, for this slot: RO# in time %d, RO# in freq %d\n",
                    frame, slot, start_symbol, n_prach_occ_in_time, n_prach_occ_in_freq, prach_assoc_pattern.prach_conf_period_list[prach_conf_period_idx].nb_of_prach_occasion,
                    prach_assoc_pattern.prach_conf_period_list[prach_conf_period_idx].prach_occasion_slot_map[prach_conf_period_frame_idx][slot].nb_of_prach_occasion_in_time,
                    prach_assoc_pattern.prach_conf_period_list[prach_conf_period_idx].prach_occasion_slot_map[prach_conf_period_frame_idx][slot].nb_of_prach_occasion_in_freq);
              } // For every freq in the slot
            } // For every time occasions in the slot
          } // Valid slot?
        } // For every slots in a frame
      } // Valid frame?
    } // For every frames in a prach configuration period
  } // For every prach configuration periods in the maximum association pattern period (160ms)
}

// Build the list of all the valid/transmitted SSBs according to the config
static void build_ssb_list(NR_ServingCellConfigCommon_t *scc) {

  // Create the list of transmitted SSBs
  // ===================================
  BIT_STRING_t *ssb_bitmap;
  uint64_t ssb_positionsInBurst;
  uint8_t ssb_idx = 0;

  switch (scc->ssb_PositionsInBurst->present) {
    case NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_shortBitmap:
      ssb_bitmap = &scc->ssb_PositionsInBurst->choice.shortBitmap;

      ssb_positionsInBurst = BIT_STRING_to_uint8(ssb_bitmap);
      LOG_D(MAC,"SSB config: SSB_positions_in_burst 0x%lx\n", ssb_positionsInBurst);

      for (uint8_t bit_nb=3; bit_nb<=3; bit_nb--) {
        // If SSB is transmitted
        if ((ssb_positionsInBurst>>bit_nb) & 0x01) {
          ssb_list.nb_tx_ssb++;
          ssb_list.tx_ssb[ssb_idx].transmitted = true;
          LOG_D(MAC,"SSB idx %d transmitted\n", ssb_idx);
        }
        ssb_idx++;
      }
      break;
    case NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_mediumBitmap:
      ssb_bitmap = &scc->ssb_PositionsInBurst->choice.mediumBitmap;

      ssb_positionsInBurst = BIT_STRING_to_uint8(ssb_bitmap);
      LOG_D(MAC,"SSB config: SSB_positions_in_burst 0x%lx\n", ssb_positionsInBurst);

      for (uint8_t bit_nb=7; bit_nb<=7; bit_nb--) {
        // If SSB is transmitted
        if ((ssb_positionsInBurst>>bit_nb) & 0x01) {
          ssb_list.nb_tx_ssb++;
          ssb_list.tx_ssb[ssb_idx].transmitted = true;
          LOG_D(MAC,"SSB idx %d transmitted\n", ssb_idx);
        }
        ssb_idx++;
      }
      break;
    case NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_longBitmap:
      ssb_bitmap = &scc->ssb_PositionsInBurst->choice.longBitmap;

      ssb_positionsInBurst = BIT_STRING_to_uint64(ssb_bitmap);
      LOG_D(MAC,"SSB config: SSB_positions_in_burst 0x%lx\n", ssb_positionsInBurst);

      for (uint8_t bit_nb=63; bit_nb<=63; bit_nb--) {
        // If SSB is transmitted
        if ((ssb_positionsInBurst>>bit_nb) & 0x01) {
          ssb_list.nb_tx_ssb++;
          ssb_list.tx_ssb[ssb_idx].transmitted = true;
          LOG_D(MAC,"SSB idx %d transmitted\n", ssb_idx);
        }
        ssb_idx++;
      }
      break;
    default:
      AssertFatal(false,"ssb_PositionsInBurst not present\n");
      break;
  }
}

// Map the transmitted SSBs to the ROs and create the association pattern according to the config
static void map_ssb_to_ro(NR_ServingCellConfigCommon_t *scc) {

  // Map SSBs to PRACH occasions
  // ===========================
  // WIP: Assumption: No PRACH occasion is rejected because of a conflict with SSBs or TDD_UL_DL_ConfigurationCommon schedule
  NR_RACH_ConfigCommon_t *setup = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR ssb_perRACH_config = setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present;

  boolean_t multiple_ssb_per_ro; // true if more than one or exactly one SSB per RACH occasion, false if more than one RO per SSB
  uint8_t ssb_rach_ratio; // Nb of SSBs per RACH or RACHs per SSB
  uint16_t required_nb_of_prach_occasion; // Nb of RACH occasions required to map all the SSBs
  uint8_t required_nb_of_prach_conf_period; // Nb of PRACH configuration periods required to map all the SSBs

  // Determine the SSB to RACH mapping ratio
  // =======================================
  switch (ssb_perRACH_config){
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneEighth:
      multiple_ssb_per_ro = false;
      ssb_rach_ratio = 8;
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneFourth:
      multiple_ssb_per_ro = false;
      ssb_rach_ratio = 4;
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneHalf:
      multiple_ssb_per_ro = false;
      ssb_rach_ratio = 2;
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_one:
      multiple_ssb_per_ro = true;
      ssb_rach_ratio = 1;
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_two:
      multiple_ssb_per_ro = true;
      ssb_rach_ratio = 2;
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_four:
      multiple_ssb_per_ro = true;
      ssb_rach_ratio = 4;
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_eight:
      multiple_ssb_per_ro = true;
      ssb_rach_ratio = 8;
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_sixteen:
      multiple_ssb_per_ro = true;
      ssb_rach_ratio = 16;
      break;
    default:
      AssertFatal(1 == 0, "Unsupported ssb_perRACH_config %d\n", ssb_perRACH_config);
      break;
  }
  LOG_D(MAC,"SSB rach ratio %d, Multiple SSB per RO %d\n", ssb_rach_ratio, multiple_ssb_per_ro);

  // Evaluate the number of PRACH configuration periods required to map all the SSBs and set the association period
  // ==============================================================================================================
  // WIP: Assumption for now is that all the PRACH configuration periods within a maximum association pattern period have the same number of PRACH occasions
  //      (No PRACH occasions are conflicting with SSBs nor TDD_UL_DL_ConfigurationCommon schedule)
  //      There is only one possible association period which can contain up to 16 PRACH configuration periods
  LOG_D(MAC,"Evaluate the number of PRACH configuration periods required to map all the SSBs and set the association period\n");
  if (true == multiple_ssb_per_ro) {
    required_nb_of_prach_occasion = ((ssb_list.nb_tx_ssb-1) + ssb_rach_ratio) / ssb_rach_ratio;
  }
  else {
    required_nb_of_prach_occasion = ssb_list.nb_tx_ssb * ssb_rach_ratio;
  }

  required_nb_of_prach_conf_period = ((required_nb_of_prach_occasion-1) + prach_assoc_pattern.prach_conf_period_list[0].nb_of_prach_occasion) / prach_assoc_pattern.prach_conf_period_list[0].nb_of_prach_occasion;

  if (required_nb_of_prach_conf_period == 1) {
    prach_assoc_pattern.prach_association_period_list[0].nb_of_prach_conf_period = 1;
  }
  else if (required_nb_of_prach_conf_period == 2) {
    prach_assoc_pattern.prach_association_period_list[0].nb_of_prach_conf_period = 2;
  }
  else if (required_nb_of_prach_conf_period <= 4) {
    prach_assoc_pattern.prach_association_period_list[0].nb_of_prach_conf_period = 4;
  }
  else if (required_nb_of_prach_conf_period <= 8) {
    prach_assoc_pattern.prach_association_period_list[0].nb_of_prach_conf_period = 8;
  }
  else if (required_nb_of_prach_conf_period <= 16) {
    prach_assoc_pattern.prach_association_period_list[0].nb_of_prach_conf_period = 16;
  }
  else {
    AssertFatal(1 == 0, "Invalid number of PRACH config periods within an association period %d\n", required_nb_of_prach_conf_period);
  }

  prach_assoc_pattern.nb_of_assoc_period = 1; // WIP: only one possible association period
  prach_assoc_pattern.prach_association_period_list[0].nb_of_frame = prach_assoc_pattern.prach_association_period_list[0].nb_of_prach_conf_period * prach_assoc_pattern.prach_conf_period_list[0].nb_of_frame;
  prach_assoc_pattern.nb_of_frame = prach_assoc_pattern.prach_association_period_list[0].nb_of_frame;

  LOG_D(MAC,"Assoc period %d, Nb of frames in assoc period %d\n",
        prach_assoc_pattern.prach_association_period_list[0].nb_of_prach_conf_period,
        prach_assoc_pattern.prach_association_period_list[0].nb_of_frame);

  // Proceed to the SSB to RO mapping
  // ================================
  uint8_t association_period_idx; // Association period index within the association pattern
  uint8_t ssb_idx = 0;
  uint8_t prach_configuration_period_idx; // PRACH Configuration period index within the association pattern
  prach_conf_period_t *prach_conf_period_p;

  // Map all the association periods within the association pattern period
  LOG_D(MAC,"Proceed to the SSB to RO mapping\n");
  for (association_period_idx=0; association_period_idx<prach_assoc_pattern.nb_of_assoc_period; association_period_idx++) {
    uint8_t n_prach_conf=0; // PRACH Configuration period index within the association period
    uint8_t frame=0;
    uint8_t slot=0;
    uint8_t ro_in_time=0;
    uint8_t ro_in_freq=0;

    // Set the starting PRACH Configuration period index in the association_pattern map for this particular association period
    prach_configuration_period_idx = 0;  // WIP: only one possible association period so the starting PRACH configuration period is automatically 0

    // Check if we need to map multiple SSBs per RO or multiple ROs per SSB
    if (true == multiple_ssb_per_ro) {
      // --------------------
      // --------------------
      // Multiple SSBs per RO
      // --------------------
      // --------------------

      // WIP: For the moment, only map each SSB idx once per association period if configuration is multiple SSBs per RO
      //      this is true if no PRACH occasions are conflicting with SSBs nor TDD_UL_DL_ConfigurationCommon schedule
      ssb_idx = 0;

      // Go through the list of PRACH config periods within this association period
      for (n_prach_conf=0; n_prach_conf<prach_assoc_pattern.prach_association_period_list[association_period_idx].nb_of_prach_conf_period; n_prach_conf++, prach_configuration_period_idx++) {
        // Build the association period with its association PRACH Configuration indexes
        prach_conf_period_p = &prach_assoc_pattern.prach_conf_period_list[prach_configuration_period_idx];
        prach_assoc_pattern.prach_association_period_list[association_period_idx].prach_conf_period_list[n_prach_conf] = prach_conf_period_p;

        // Go through all the ROs within the PRACH config period
        for (frame=0; frame<prach_conf_period_p->nb_of_frame; frame++) {
          for (slot=0; slot<prach_conf_period_p->nb_of_slot; slot++) {
            for (ro_in_time=0; ro_in_time<prach_conf_period_p->prach_occasion_slot_map[frame][slot].nb_of_prach_occasion_in_time; ro_in_time++) {
              for (ro_in_freq=0; ro_in_freq<prach_conf_period_p->prach_occasion_slot_map[frame][slot].nb_of_prach_occasion_in_freq; ro_in_freq++) {
                prach_occasion_info_t *ro_p = &prach_conf_period_p->prach_occasion_slot_map[frame][slot].prach_occasion[ro_in_time][ro_in_freq];

                // Go through the list of transmitted SSBs and map the required amount of SSBs to this RO
                // WIP: For the moment, only map each SSB idx once per association period if configuration is multiple SSBs per RO
                //      this is true if no PRACH occasions are conflicting with SSBs nor TDD_UL_DL_ConfigurationCommon schedule
                for (; ssb_idx<MAX_NB_SSB; ssb_idx++) {
                  // Map only the transmitted ssb_idx
                  if (true == ssb_list.tx_ssb[ssb_idx].transmitted) {
                    ro_p->mapped_ssb_idx[ro_p->nb_mapped_ssb] = ssb_idx;
                    ro_p->nb_mapped_ssb++;
                    ssb_list.tx_ssb[ssb_idx].mapped_ro[ssb_list.tx_ssb[ssb_idx].nb_mapped_ro] = ro_p;
                    ssb_list.tx_ssb[ssb_idx].nb_mapped_ro++;
                    AssertFatal(MAX_NB_RO_PER_SSB_IN_ASSOCIATION_PATTERN > ssb_list.tx_ssb[ssb_idx].nb_mapped_ro,"Too many mapped ROs (%d) to a single SSB\n", ssb_list.tx_ssb[ssb_idx].nb_mapped_ro);

                    LOG_D(MAC,"Mapped ssb_idx %u to RO slot-symbol %u-%u, %u-%u-%u/%u\n", ssb_idx, ro_p->slot, ro_p->start_symbol, slot, ro_in_time, ro_in_freq, prach_conf_period_p->prach_occasion_slot_map[frame][slot].nb_of_prach_occasion_in_freq);
                    LOG_D(MAC,"Nb mapped ROs for this ssb idx: in the association period only %u\n", ssb_list.tx_ssb[ssb_idx].nb_mapped_ro);

                    // If all the required SSBs are mapped to this RO, exit the loop of SSBs
                    if (ro_p->nb_mapped_ssb == ssb_rach_ratio) {
                      ssb_idx++;
                      break;
                    }
                  } // if ssb_idx is transmitted
                } // for ssb_idx

                // Exit the loop of ROs if there is no more SSB to map
                if (MAX_NB_SSB == ssb_idx) break;
              } // for ro_in_freq

              // Exit the loop of ROs if there is no more SSB to map
              if (MAX_NB_SSB == ssb_idx) break;
            } // for ro_in_time

            // Exit the loop of slots if there is no more SSB to map
            if (MAX_NB_SSB == ssb_idx) break;
          } // for slot

          // Exit the loop frames if there is no more SSB to map
          if (MAX_NB_SSB == ssb_idx) break;
        } // for frame

        // Exit the loop of PRACH configurations if there is no more SSB to map
        if (MAX_NB_SSB == ssb_idx) break;
      } // for n_prach_conf

      // WIP: note that there is no re-mapping of the SSBs within the association period since there is no invalid ROs in the PRACH config periods that would create this situation

    } // if multiple_ssbs_per_ro

    else {
      // --------------------
      // --------------------
      // Multiple ROs per SSB
      // --------------------
      // --------------------

      n_prach_conf = 0;

      // Go through the list of transmitted SSBs
      for (ssb_idx=0; ssb_idx<MAX_NB_SSB; ssb_idx++) {
        uint8_t nb_mapped_ro_in_association_period=0; // Reset the nb of mapped ROs for the new SSB index

        // Map only the transmitted ssb_idx
        if (true == ssb_list.tx_ssb[ssb_idx].transmitted) {

          // Map all the required ROs to this SSB
          // Go through the list of PRACH config periods within this association period
          for (; n_prach_conf<prach_assoc_pattern.prach_association_period_list[association_period_idx].nb_of_prach_conf_period; n_prach_conf++, prach_configuration_period_idx++) {

            // Build the association period with its association PRACH Configuration indexes
            prach_conf_period_p = &prach_assoc_pattern.prach_conf_period_list[prach_configuration_period_idx];
            prach_assoc_pattern.prach_association_period_list[association_period_idx].prach_conf_period_list[n_prach_conf] = prach_conf_period_p;

            for (; frame<prach_conf_period_p->nb_of_frame; frame++) {
              for (; slot<prach_conf_period_p->nb_of_slot; slot++) {
                for (; ro_in_time<prach_conf_period_p->prach_occasion_slot_map[frame][slot].nb_of_prach_occasion_in_time; ro_in_time++) {
                  for (; ro_in_freq<prach_conf_period_p->prach_occasion_slot_map[frame][slot].nb_of_prach_occasion_in_freq; ro_in_freq++) {
                    prach_occasion_info_t *ro_p = &prach_conf_period_p->prach_occasion_slot_map[frame][slot].prach_occasion[ro_in_time][ro_in_freq];

                    ro_p->mapped_ssb_idx[0] = ssb_idx;
                    ro_p->nb_mapped_ssb = 1;
                    ssb_list.tx_ssb[ssb_idx].mapped_ro[ssb_list.tx_ssb[ssb_idx].nb_mapped_ro] = ro_p;
                    ssb_list.tx_ssb[ssb_idx].nb_mapped_ro++;
                    AssertFatal(MAX_NB_RO_PER_SSB_IN_ASSOCIATION_PATTERN > ssb_list.tx_ssb[ssb_idx].nb_mapped_ro,"Too many mapped ROs (%d) to a single SSB\n", ssb_list.tx_ssb[ssb_idx].nb_mapped_ro);
                    nb_mapped_ro_in_association_period++;

                    LOG_D(MAC,"Mapped ssb_idx %u to RO slot-symbol %u-%u, %u-%u-%u/%u\n", ssb_idx, ro_p->slot, ro_p->start_symbol, slot, ro_in_time, ro_in_freq, prach_conf_period_p->prach_occasion_slot_map[frame][slot].nb_of_prach_occasion_in_freq);
                    LOG_D(MAC,"Nb mapped ROs for this ssb idx: in the association period only %u / total %u\n", ssb_list.tx_ssb[ssb_idx].nb_mapped_ro, nb_mapped_ro_in_association_period);

                    // Exit the loop if this SSB has been mapped to all the required ROs
                    // WIP: Assuming that ssb_rach_ratio equals the maximum nb of times a given ssb_idx is mapped within an association period:
                    //      this is true if no PRACH occasions are conflicting with SSBs nor TDD_UL_DL_ConfigurationCommon schedule
                    if (nb_mapped_ro_in_association_period == ssb_rach_ratio) {
                      ro_in_freq++;
                      break;
                    }
                  } // for ro_in_freq

                  // Exit the loop if this SSB has been mapped to all the required ROs
                  if (nb_mapped_ro_in_association_period == ssb_rach_ratio) {
                    break;
                  }
                  else ro_in_freq = 0; // else go to the next time symbol in that slot and reset the freq index
                } // for ro_in_time

                // Exit the loop if this SSB has been mapped to all the required ROs
                if (nb_mapped_ro_in_association_period == ssb_rach_ratio) {
                  break;
                }
                else ro_in_time = 0; // else go to the next slot in that PRACH config period and reset the symbol index
              } // for slot

              // Exit the loop if this SSB has been mapped to all the required ROs
              if (nb_mapped_ro_in_association_period == ssb_rach_ratio) {
                break;
              }
              else slot = 0; // else go to the next frame in that PRACH config period and reset the slot index
            } // for frame

            // Exit the loop if this SSB has been mapped to all the required ROs
            if (nb_mapped_ro_in_association_period == ssb_rach_ratio) {
              break;
            }
            else frame = 0; // else go to the next PRACH config period in that association period and reset the frame index
          } // for n_prach_conf

        } // if ssb_idx is transmitted
      } // for ssb_idx
    } // else if multiple_ssbs_per_ro

  } // for association_period_index
}

// Returns a RACH occasion if any matches the SSB idx, the frame and the slot
static int get_nr_prach_info_from_ssb_index(uint8_t ssb_idx,
                                            int frame,
                                            int slot,
                                            prach_occasion_info_t **prach_occasion_info_pp) {

  ssb_info_t *ssb_info_p;
  prach_occasion_slot_t *prach_occasion_slot_p = NULL;

  *prach_occasion_info_pp = NULL;

  // Search for a matching RO slot in the SSB_to_RO map
  // A valid RO slot will match:
  //      - ssb_idx mapped to one of the ROs in that RO slot
  //      - exact slot number
  //      - frame offset
  ssb_info_p = &ssb_list.tx_ssb[ssb_idx];
  for (uint8_t n_mapped_ro=0; n_mapped_ro<ssb_info_p->nb_mapped_ro; n_mapped_ro++) {
    if ((slot == ssb_info_p->mapped_ro[n_mapped_ro]->slot) &&
        (ssb_info_p->mapped_ro[n_mapped_ro]->frame == (frame % prach_assoc_pattern.nb_of_frame))) {

      uint8_t prach_config_period_nb = ssb_info_p->mapped_ro[n_mapped_ro]->frame / prach_assoc_pattern.prach_conf_period_list[0].nb_of_frame;
      uint8_t frame_nb_in_prach_config_period = ssb_info_p->mapped_ro[n_mapped_ro]->frame % prach_assoc_pattern.prach_conf_period_list[0].nb_of_frame;
      prach_occasion_slot_p = &prach_assoc_pattern.prach_conf_period_list[prach_config_period_nb].prach_occasion_slot_map[frame_nb_in_prach_config_period][slot];
    }
  }

  // If there is a matching RO slot in the SSB_to_RO map
  if (NULL != prach_occasion_slot_p)
  {
    // A random RO mapped to the SSB index should be selected in the slot

    // First count the number of times the SSB index is found in that RO
    uint8_t nb_mapped_ssb = 0;

    for (int ro_in_time=0; ro_in_time < prach_occasion_slot_p->nb_of_prach_occasion_in_time; ro_in_time++) {
      for (int ro_in_freq=0; ro_in_freq < prach_occasion_slot_p->nb_of_prach_occasion_in_freq; ro_in_freq++) {
        prach_occasion_info_t *prach_occasion_info_p = &prach_occasion_slot_p->prach_occasion[ro_in_time][ro_in_freq];

        for (uint8_t ssb_nb=0; ssb_nb<prach_occasion_info_p->nb_mapped_ssb; ssb_nb++) {
          if (prach_occasion_info_p->mapped_ssb_idx[ssb_nb] == ssb_idx) {
            nb_mapped_ssb++;
          }
        }
      }
    }

    // Choose a random SSB nb
    uint8_t random_ssb_nb = 0;

    random_ssb_nb = ((taus()) % nb_mapped_ssb);

    // Select the RO according to the chosen random SSB nb
    nb_mapped_ssb=0;
    for (int ro_in_time=0; ro_in_time < prach_occasion_slot_p->nb_of_prach_occasion_in_time; ro_in_time++) {
      for (int ro_in_freq=0; ro_in_freq < prach_occasion_slot_p->nb_of_prach_occasion_in_freq; ro_in_freq++) {
        prach_occasion_info_t *prach_occasion_info_p = &prach_occasion_slot_p->prach_occasion[ro_in_time][ro_in_freq];

        for (uint8_t ssb_nb=0; ssb_nb<prach_occasion_info_p->nb_mapped_ssb; ssb_nb++) {
          if (prach_occasion_info_p->mapped_ssb_idx[ssb_nb] == ssb_idx) {
            if (nb_mapped_ssb == random_ssb_nb) {
              *prach_occasion_info_pp = prach_occasion_info_p;
              return 1;
            }
            else {
              nb_mapped_ssb++;
            }
          }
        }
      }
    }
  }

  return 0;
}

// Build the SSB to RO mapping upon RRC configuration update
void build_ssb_to_ro_map(NR_ServingCellConfigCommon_t *scc, uint8_t unpaired){

  // Clear all the lists and maps
  memset(&prach_assoc_pattern, 0, sizeof(prach_association_pattern_t));
  memset(&ssb_list, 0, sizeof(ssb_list_info_t));

  // Build the list of all the valid RACH occasions in the maximum association pattern period according to the PRACH config
  LOG_D(MAC,"Build RO list\n");
  build_ro_list(scc, unpaired);

  // Build the list of all the valid/transmitted SSBs according to the config
  LOG_D(MAC,"Build SSB list\n");
  build_ssb_list(scc);

  // Map the transmitted SSBs to the ROs and create the association pattern according to the config
  LOG_D(MAC,"Map SSB to RO\n");
  map_ssb_to_ro(scc);
  LOG_D(MAC,"Map SSB to RO done\n");
}

void fill_scheduled_response(nr_scheduled_response_t *scheduled_response,
                             fapi_nr_dl_config_request_t *dl_config,
                             fapi_nr_ul_config_request_t *ul_config,
                             fapi_nr_tx_request_t *tx_request,
                             module_id_t mod_id,
                             int cc_id,
                             frame_t frame,
                             int slot,
                             int thread_id){

  scheduled_response->dl_config  = dl_config;
  scheduled_response->ul_config  = ul_config;
  scheduled_response->tx_request = tx_request;
  scheduled_response->module_id  = mod_id;
  scheduled_response->CC_id      = cc_id;
  scheduled_response->frame      = frame;
  scheduled_response->slot       = slot;
  scheduled_response->thread_id  = thread_id;
}

uint32_t get_ssb_slot(uint32_t ssb_index){
  //  this function now only support f <= 3GHz
  return ssb_index & 0x3 ;

  //  return first_symbol(case, freq, ssb_index) / 14
}

uint8_t table_9_2_2_1[16][8]={
  {0,12,2, 0, 0,3,0,0},
  {0,12,2, 0, 0,4,8,0},
  {0,12,2, 3, 0,4,8,0},
  {1,10,4, 0, 0,6,0,0},
  {1,10,4, 0, 0,3,6,9},
  {1,10,4, 2, 0,3,6,9},
  {1,10,4, 4, 0,3,6,9},
  {1,4, 10,0, 0,6,0,0},
  {1,4, 10,0, 0,3,6,9},
  {1,4, 10,2, 0,3,6,9},
  {1,4, 10,4, 0,3,6,9},
  {1,0, 14,0, 0,6,0,0},
  {1,0, 14,0, 0,3,6,9},
  {1,0, 14,2, 0,3,6,9},
  {1,0, 14,4, 0,3,6,9},
  {1,0, 14,26,0,3,0,0}
};

#if 0 // Not in use. In case of use, please adapt the schedule_response (no longer in MAC)
int8_t nr_ue_process_dlsch(module_id_t module_id,
			   int cc_id,
			   uint8_t gNB_index,
			   fapi_nr_dci_indication_t *dci_ind,
			   void *pduP,
			   uint32_t pdu_len)
{
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  fapi_nr_ul_config_request_t *ul_config = &mac->ul_config_request;
  //fapi_nr_dl_config_request_t *dl_config = &mac->dl_config_request;
  nr_phy_config_t *phy_config = &mac->phy_config;

  //ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.rnti = rnti;
  // First we need to verify if DCI ind contains a ul-sch to be perfomred. If it does, we will handle a PUSCH in the UL_CONFIG_REQ.
  ul_config->ul_config_list[ul_config->number_pdus].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUCCH;
  for (int i=0; i<10; i++) {
    if(dci_ind!=NULL){
      if (dci_ind->dci_list[i].dci_format < 2) ul_config->ul_config_list[ul_config->number_pdus].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUSCH;
    }
  }
  if (ul_config->ul_config_list[ul_config->number_pdus].pdu_type == FAPI_NR_UL_CONFIG_TYPE_PUSCH) {
    // fill in the elements in config request inside P5 message
    //ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.bandwidth_part_ind = 0; //FIXME
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.rb_size = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.rb_start = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.nr_of_symbols = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.start_symbol_index = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.frequency_hopping = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.mcs_index = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.pusch_data.new_data_indicator = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.pusch_data.rv_index = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.pusch_data.harq_process_id = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.absolute_delta_PUSCH = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.nrOfLayers = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.transform_precoding = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.num_dmrs_cdm_grps_no_data = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.dmrs_ports = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.dmrs_config_type = 0;
    //ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.n_front_load_symb = 0; //FIXME
    //ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.srs_config = 0; //FIXME
    //ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.csi_reportTriggerSize = 0; //FIXME
    //ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.maxCodeBlockGroupsPerTransportBlock = 0; //FIXME
    //ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.ptrs_dmrs_association_port = 0; FIXME
    //ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.beta_offset_ind = 0; //FIXME
  } else { // If DCI ind is not format 0_0 or 0_1, we will handle a PUCCH in the UL_CONFIG_REQ
    ul_config->ul_config_list[ul_config->number_pdus].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUCCH;
    // If we handle PUCCH common
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.format              = table_9_2_2_1[phy_config->config_req.ul_bwp_common.pucch_config_common.pucch_resource_common][0];              /* format   0    1    2    3    4    */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.initialCyclicShift  = table_9_2_2_1[phy_config->config_req.ul_bwp_common.pucch_config_common.pucch_resource_common][4];  /*          x    x                   */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofSymbols         = table_9_2_2_1[phy_config->config_req.ul_bwp_common.pucch_config_common.pucch_resource_common][2];         /*          x    x    x    x    x    */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.startingSymbolIndex = table_9_2_2_1[phy_config->config_req.ul_bwp_common.pucch_config_common.pucch_resource_common][1]; /*          x    x    x    x    x    */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.timeDomainOCC = 0;       /*               x                   */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofPRBs = 0;            /*                    x    x         */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.startingPRB         = table_9_2_2_1[phy_config->config_req.ul_bwp_common.pucch_config_common.pucch_resource_common][3];         /*                                     maxNrofPhysicalResourceBlocks  = 275 */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.occ_length = 0;          /*                              x    */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.occ_Index = 0;           /*                              x    */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.intraSlotFrequencyHopping = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.secondHopPRB = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.interslotFrequencyHopping = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.additionalDMRS = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.maxCodeRate = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofSlots = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.pi2PBSK = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.simultaneousHARQ_ACK_CSI = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.pucch_GroupHopping  = phy_config->config_req.ul_bwp_common.pucch_config_common.pucch_group_hopping;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.hoppingId           = phy_config->config_req.ul_bwp_common.pucch_config_common.hopping_id;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.p0_nominal          = phy_config->config_req.ul_bwp_common.pucch_config_common.p0_nominal;
    for (int i=0;i<NUMBER_PUCCH_FORMAT_NR;i++) ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.deltaF_PUCCH_f[i] = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.p0_PUCCH_Id = 0;     /* INTEGER (1..8)     */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.p0_PUCCH_Value = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.twoPUCCH_PC_AdjustmentStates = 0;
    // If we handle PUCCH dedicated
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.format              = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].format;              /* format   0    1    2    3    4    */
    switch (ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.format){
    case pucch_format1_nr:
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.interslotFrequencyHopping = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format1.inter_slot_frequency_hopping;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.additionalDMRS            = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format1.additional_dmrs;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.maxCodeRate               = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format1.max_code_rate;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofSlots                 = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format1.number_of_slots;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.pi2PBSK                   = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format1.pi2bpsk;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.simultaneousHARQ_ACK_CSI  = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format1.simultaneous_harq_ack_csi;
      break;
    case pucch_format2_nr:
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.interslotFrequencyHopping = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format2.inter_slot_frequency_hopping;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.additionalDMRS            = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format2.additional_dmrs;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.maxCodeRate               = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format2.max_code_rate;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofSlots                 = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format2.number_of_slots;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.pi2PBSK                   = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format2.pi2bpsk;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.simultaneousHARQ_ACK_CSI  = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format2.simultaneous_harq_ack_csi;
      break;
    case pucch_format3_nr:
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.interslotFrequencyHopping = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format3.inter_slot_frequency_hopping;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.additionalDMRS            = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format3.additional_dmrs;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.maxCodeRate               = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format3.max_code_rate;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofSlots                 = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format3.number_of_slots;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.pi2PBSK                   = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format3.pi2bpsk;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.simultaneousHARQ_ACK_CSI  = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format3.simultaneous_harq_ack_csi;
      break;
    case pucch_format4_nr:
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.interslotFrequencyHopping = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format4.inter_slot_frequency_hopping;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.additionalDMRS            = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format4.additional_dmrs;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.maxCodeRate               = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format4.max_code_rate;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofSlots                 = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format4.number_of_slots;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.pi2PBSK                   = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format4.pi2bpsk;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.simultaneousHARQ_ACK_CSI  = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format4.simultaneous_harq_ack_csi;
      break;
    default:
      break;
    }
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.initialCyclicShift        = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].initial_cyclic_shift;  /*          x    x                   */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofSymbols               = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].number_of_symbols;         /*          x    x    x    x    x    */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.startingSymbolIndex       = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].starting_symbol_index; /*          x    x    x    x    x    */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.timeDomainOCC             = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].time_domain_occ;       /*               x                   */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofPRBs                  = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].number_of_prbs;            /*                    x    x         */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.startingPRB               = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].starting_prb;         /*                                     maxNrofPhysicalResourceBlocks  = 275 */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.occ_length                = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].occ_length;          /*                              x    */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.occ_Index                 = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].occ_index;           /*                              x    */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.intraSlotFrequencyHopping = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].intra_slot_frequency_hopping;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.secondHopPRB              = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].second_hop_prb;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.pucch_GroupHopping        = phy_config->config_req.ul_bwp_common.pucch_config_common.pucch_group_hopping;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.hoppingId                 = phy_config->config_req.ul_bwp_common.pucch_config_common.hopping_id;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.p0_nominal                = phy_config->config_req.ul_bwp_common.pucch_config_common.p0_nominal;
    for (int i=0;i<NUMBER_PUCCH_FORMAT_NR; i++) ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.deltaF_PUCCH_f[i] = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.p0_PUCCH_Id = 0;     /* INTEGER (1..8)     */
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.p0_PUCCH_Value = 0;
    ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.twoPUCCH_PC_AdjustmentStates = 0;

  }
  if(mac->if_module != NULL && mac->if_module->scheduled_response != NULL){
    mac->if_module->scheduled_response(&mac->scheduled_response);
  }
  return 0;
}
#endif

int8_t nr_ue_decode_mib(module_id_t module_id,
                        int cc_id,
                        uint8_t gNB_index,
                        uint8_t extra_bits,	//	8bits 38.212 c7.1.1
                        uint32_t ssb_length,
                        uint32_t ssb_index,
                        void *pduP,
                        uint16_t cell_id)
{
  LOG_I(MAC,"[L2][MAC] decode mib\n");

  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

  nr_mac_rrc_data_ind_ue( module_id, cc_id, gNB_index, NR_BCCH_BCH, (uint8_t *) pduP, 3 );    //  fixed 3 bytes MIB PDU
    
  AssertFatal(mac->mib != NULL, "nr_ue_decode_mib() mac->mib == NULL\n");
  //if(mac->mib != NULL){
  uint16_t frame = (mac->mib->systemFrameNumber.buf[0] >> mac->mib->systemFrameNumber.bits_unused);
  uint16_t frame_number_4lsb = 0;
  for (int i=0; i<4; i++)
    frame_number_4lsb |= ((extra_bits>>i)&1)<<(3-i);
  uint8_t half_frame_bit = ( extra_bits >> 4 ) & 0x1;               //	extra bits[4]
  uint8_t ssb_subcarrier_offset_msb = ( extra_bits >> 5 ) & 0x1;    //	extra bits[5]
  uint8_t ssb_subcarrier_offset = (uint8_t)mac->mib->ssb_SubcarrierOffset;

  frame = frame << 4;
  frame = frame | frame_number_4lsb;
  if(ssb_length == 64){
    for (int i=0; i<3; i++)
      ssb_index += (((extra_bits>>(7-i))&0x01)<<(3+i));
  }else{
    if(ssb_subcarrier_offset_msb){
      ssb_subcarrier_offset = ssb_subcarrier_offset | 0x10;
    }
  }

  LOG_D(MAC,"system frame number(6 MSB bits): %d\n",  mac->mib->systemFrameNumber.buf[0]);
  LOG_D(MAC,"system frame number(with LSB): %d\n", (int)frame);
  LOG_D(MAC,"subcarrier spacing (0=15or60, 1=30or120): %d\n", (int)mac->mib->subCarrierSpacingCommon);
  LOG_D(MAC,"ssb carrier offset(with MSB):  %d\n", (int)ssb_subcarrier_offset);
  LOG_D(MAC,"dmrs type A position (0=pos2,1=pos3): %d\n", (int)mac->mib->dmrs_TypeA_Position);
  LOG_D(MAC,"cell barred (0=barred,1=notBarred): %d\n", (int)mac->mib->cellBarred);
  LOG_D(MAC,"intra frequency reselection (0=allowed,1=notAllowed): %d\n", (int)mac->mib->intraFreqReselection);
  LOG_D(MAC,"half frame bit(extra bits):    %d\n", (int)half_frame_bit);
  LOG_D(MAC,"ssb index(extra bits):         %d\n", (int)ssb_index);

  subcarrier_spacing_t scs_ssb = scs_30kHz;      //  default for 
  //const uint32_t scs_index = 0;
  const uint32_t num_slot_per_frame = 20;
  subcarrier_spacing_t scs_pdcch;

  //  assume carrier frequency < 6GHz
  if(mac->mib->subCarrierSpacingCommon == NR_MIB__subCarrierSpacingCommon_scs15or60){
    scs_pdcch = scs_15kHz;
  }else{  //NR_MIB__subCarrierSpacingCommon_scs30or120
    scs_pdcch = scs_30kHz;
  }

  channel_bandwidth_t min_channel_bw = bw_10MHz;  //  deafult for testing
	    
  uint32_t is_condition_A = (ssb_subcarrier_offset == 0);   //  38.213 ch.13
  frequency_range_t frequency_range = FR1;
  uint32_t index_4msb = (mac->mib->pdcch_ConfigSIB1.controlResourceSetZero);
  uint32_t index_4lsb = (mac->mib->pdcch_ConfigSIB1.searchSpaceZero);
  int32_t num_rbs = -1;
  int32_t num_symbols = -1;
  int32_t rb_offset = -1;
  LOG_D(MAC,"<<<<<<<<<configSIB1: controlResourceSetZero %d searchSpaceZero %d scs_ssb %d scs_pdcch %d switch %d ",
        index_4msb,index_4lsb,scs_ssb,scs_pdcch, (scs_ssb << 5)|scs_pdcch);

  //  type0-pdcch coreset
  switch( (scs_ssb << 5)|scs_pdcch ){
  case (scs_15kHz << 5) | scs_15kHz :
    AssertFatal(index_4msb < 15, "38.213 Table 13-1 4 MSB out of range\n");
    mac->type0_pdcch_ss_mux_pattern = 1;
    num_rbs     = table_38213_13_1_c2[index_4msb];
    num_symbols = table_38213_13_1_c3[index_4msb];
    rb_offset   = table_38213_13_1_c4[index_4msb];
    break;

  case (scs_15kHz << 5) | scs_30kHz:
    AssertFatal(index_4msb < 14, "38.213 Table 13-2 4 MSB out of range\n");
    mac->type0_pdcch_ss_mux_pattern = 1;
    num_rbs     = table_38213_13_2_c2[index_4msb];
    num_symbols = table_38213_13_2_c3[index_4msb];
    rb_offset   = table_38213_13_2_c4[index_4msb];
    break;

  case (scs_30kHz << 5) | scs_15kHz:
    if((min_channel_bw & bw_5MHz) | (min_channel_bw & bw_10MHz)){
      AssertFatal(index_4msb < 9, "38.213 Table 13-3 4 MSB out of range\n");
      mac->type0_pdcch_ss_mux_pattern = 1;
      num_rbs     = table_38213_13_3_c2[index_4msb];
      num_symbols = table_38213_13_3_c3[index_4msb];
      rb_offset   = table_38213_13_3_c4[index_4msb];
    }else if(min_channel_bw & bw_40MHz){
      AssertFatal(index_4msb < 9, "38.213 Table 13-5 4 MSB out of range\n");
      mac->type0_pdcch_ss_mux_pattern = 1;
      num_rbs     = table_38213_13_5_c2[index_4msb];
      num_symbols = table_38213_13_5_c3[index_4msb];
      rb_offset   = table_38213_13_5_c4[index_4msb];
    }else{ ; }

    break;

  case (scs_30kHz << 5) | scs_30kHz:
    if((min_channel_bw & bw_5MHz) | (min_channel_bw & bw_10MHz)){
      mac->type0_pdcch_ss_mux_pattern = 1;
      num_rbs     = table_38213_13_4_c2[index_4msb];
      num_symbols = table_38213_13_4_c3[index_4msb];
      rb_offset   = table_38213_13_4_c4[index_4msb];
      LOG_I(MAC,"<<<<<<<<<index_4msb %d num_rbs %d num_symb %d rb_offset %d\n",index_4msb,num_rbs,num_symbols,rb_offset );
    }else if(min_channel_bw & bw_40MHz){
      AssertFatal(index_4msb < 10, "38.213 Table 13-6 4 MSB out of range\n");
      mac->type0_pdcch_ss_mux_pattern = 1;
      num_rbs     = table_38213_13_6_c2[index_4msb];
      num_symbols = table_38213_13_6_c3[index_4msb];
      rb_offset   = table_38213_13_6_c4[index_4msb];
    }else{ ; }
    break;

  case (scs_120kHz << 5) | scs_60kHz:
    AssertFatal(index_4msb < 12, "38.213 Table 13-7 4 MSB out of range\n");
    if(index_4msb & 0x7){
      mac->type0_pdcch_ss_mux_pattern = 1;
    }else if(index_4msb & 0x18){
      mac->type0_pdcch_ss_mux_pattern = 2;
    }else{ ; }

    num_rbs     = table_38213_13_7_c2[index_4msb];
    num_symbols = table_38213_13_7_c3[index_4msb];
    if(!is_condition_A && (index_4msb == 8 || index_4msb == 10)){
      rb_offset   = table_38213_13_7_c4[index_4msb] - 1;
    }else{
      rb_offset   = table_38213_13_7_c4[index_4msb];
    }
    break;

  case (scs_120kHz << 5) | scs_120kHz:
    AssertFatal(index_4msb < 8, "38.213 Table 13-8 4 MSB out of range\n");
    if(index_4msb & 0x3){
      mac->type0_pdcch_ss_mux_pattern = 1;
    }else if(index_4msb & 0x0c){
      mac->type0_pdcch_ss_mux_pattern = 3;
    }

    num_rbs     = table_38213_13_8_c2[index_4msb];
    num_symbols = table_38213_13_8_c3[index_4msb];
    if(!is_condition_A && (index_4msb == 4 || index_4msb == 6)){
      rb_offset   = table_38213_13_8_c4[index_4msb] - 1;
    }else{
      rb_offset   = table_38213_13_8_c4[index_4msb];
    }
    break;

  case (scs_240kHz << 5) | scs_60kHz:
    AssertFatal(index_4msb < 4, "38.213 Table 13-9 4 MSB out of range\n");
    mac->type0_pdcch_ss_mux_pattern = 1;
    num_rbs     = table_38213_13_9_c2[index_4msb];
    num_symbols = table_38213_13_9_c3[index_4msb];
    rb_offset   = table_38213_13_9_c4[index_4msb];
    break;

  case (scs_240kHz << 5) | scs_120kHz:
    AssertFatal(index_4msb < 8, "38.213 Table 13-10 4 MSB out of range\n");
    if(index_4msb & 0x3){
      mac->type0_pdcch_ss_mux_pattern = 1;
    }else if(index_4msb & 0x0c){
      mac->type0_pdcch_ss_mux_pattern = 2;
    }
    num_rbs     = table_38213_13_10_c2[index_4msb];
    num_symbols = table_38213_13_10_c3[index_4msb];
    if(!is_condition_A && (index_4msb == 4 || index_4msb == 6)){
      rb_offset   = table_38213_13_10_c4[index_4msb]-1;
    }else{
      rb_offset   = table_38213_13_10_c4[index_4msb];
    }
                
    break;

  default:
    break;
  }

  AssertFatal(num_rbs != -1, "Type0 PDCCH coreset num_rbs undefined");
  AssertFatal(num_symbols != -1, "Type0 PDCCH coreset num_symbols undefined");
  AssertFatal(rb_offset != -1, "Type0 PDCCH coreset rb_offset undefined");
        
  //uint32_t cell_id = 0;   //  obtain from L1 later

  //mac->type0_pdcch_dci_config.coreset.rb_start = rb_offset;
  //mac->type0_pdcch_dci_config.coreset.rb_end = rb_offset + num_rbs - 1;
  uint64_t mask = 0x0;
  uint8_t i;
  for(i=0; i<(num_rbs/6); ++i){   //  38.331 Each bit corresponds a group of 6 RBs
    mask = mask >> 1;
    mask = mask | 0x100000000000;
  }
  //LOG_I(MAC,">>>>>>>>mask %x num_rbs %d rb_offset %d\n", mask, num_rbs, rb_offset);
  /*
    mac->type0_pdcch_dci_config.coreset.frequency_domain_resource = mask;
    mac->type0_pdcch_dci_config.coreset.rb_offset = rb_offset;  //  additional parameter other than coreset

    //mac->type0_pdcch_dci_config.type0_pdcch_coreset.duration = num_symbols;
    mac->type0_pdcch_dci_config.coreset.cce_reg_mapping_type = CCE_REG_MAPPING_TYPE_INTERLEAVED;
    mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_reg_bundle_size = 6;   //  L 38.211 7.3.2.2
    mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_interleaver_size = 2;  //  R 38.211 7.3.2.2
    mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_shift_index = cell_id;
    mac->type0_pdcch_dci_config.coreset.precoder_granularity = PRECODER_GRANULARITY_SAME_AS_REG_BUNDLE;
    mac->type0_pdcch_dci_config.coreset.pdcch_dmrs_scrambling_id = cell_id;
  */


  // type0-pdcch search space
  float big_o;
  float big_m;
  uint32_t temp;
  SFN_C_TYPE sfn_c=SFN_C_IMPOSSIBLE;   //  only valid for mux=1
  uint32_t n_c=UINT_MAX;
  uint32_t number_of_search_space_per_slot=UINT_MAX;
//  uint32_t first_symbol_index=UINT_MAX;
//  uint32_t search_space_duration;  //  element of search space
  //  38.213 table 10.1-1

  /// MUX PATTERN 1
  if(mac->type0_pdcch_ss_mux_pattern == 1 && frequency_range == FR1){
    big_o = table_38213_13_11_c1[index_4lsb];
    number_of_search_space_per_slot = table_38213_13_11_c2[index_4lsb];
    big_m = table_38213_13_11_c3[index_4lsb];

    temp = (uint32_t)(big_o*pow(2, scs_pdcch)) + (uint32_t)(ssb_index*big_m);
    n_c = temp / num_slot_per_frame;
    if((temp/num_slot_per_frame) & 0x1){
      sfn_c = SFN_C_MOD_2_EQ_1;
    }else{
      sfn_c = SFN_C_MOD_2_EQ_0;
    }

//    if((index_4lsb == 1 || index_4lsb == 3 || index_4lsb == 5 || index_4lsb == 7) && (ssb_index&1)){
//      first_symbol_index = num_symbols;
//    }else{
//      first_symbol_index = table_38213_13_11_c4[index_4lsb];
//    }
    //  38.213 chapter 13: over two consecutive slots
//    search_space_duration = 2;
  }

  if(mac->type0_pdcch_ss_mux_pattern == 1 && frequency_range == FR2){
    big_o = table_38213_13_12_c1[index_4lsb];
    number_of_search_space_per_slot = table_38213_13_11_c2[index_4lsb];
    big_m = table_38213_13_12_c3[index_4lsb];

//    if((index_4lsb == 1 || index_4lsb == 3 || index_4lsb == 5 || index_4lsb == 10) && (ssb_index&1)){
//      first_symbol_index = 7;
//    }else if((index_4lsb == 6 || index_4lsb == 7 || index_4lsb == 8 || index_4lsb == 11) && (ssb_index&1)){
//      first_symbol_index = num_symbols;
//    }else{
//      first_symbol_index = 0;
//    }
    //  38.213 chapter 13: over two consecutive slots
    //search_space_duration = 2;
  }

  /// MUX PATTERN 2
  if(mac->type0_pdcch_ss_mux_pattern == 2){
            
    if((scs_ssb == scs_120kHz) && (scs_pdcch == scs_60kHz)){
      //  38.213 Table 13-13
      AssertFatal(index_4lsb == 0, "38.213 Table 13-13 4 LSB out of range\n");
      //  PDCCH monitoring occasions (SFN and slot number) same as SSB frame-slot
      //                sfn_c = SFN_C_EQ_SFN_SSB;
      n_c = get_ssb_slot(ssb_index);
//      switch(ssb_index & 0x3){    //  ssb_index(i) mod 4
//      case 0:
//	first_symbol_index = 0;
//	break;
//      case 1:
//	first_symbol_index = 1;
//	break;
//      case 2:
//	first_symbol_index = 6;
//	break;
//      case 3:
//	first_symbol_index = 7;
//	break;
//      default: break;
//      }
                
    }else if((scs_ssb == scs_240kHz) && (scs_pdcch == scs_120kHz)){
      //  38.213 Table 13-14
      AssertFatal(index_4lsb == 0, "38.213 Table 13-14 4 LSB out of range\n");
      //  PDCCH monitoring occasions (SFN and slot number) same as SSB frame-slot
      //                sfn_c = SFN_C_EQ_SFN_SSB;
      n_c = get_ssb_slot(ssb_index);
      switch(ssb_index & 0x7){    //  ssb_index(i) mod 8
      case 0: 
//	first_symbol_index = 0;
	break;
      case 1: 
//	first_symbol_index = 1;
	break;
      case 2: 
//	first_symbol_index = 2;
	break;
      case 3: 
//	first_symbol_index = 3;
	break;
      case 4: 
//	first_symbol_index = 12;
	n_c = get_ssb_slot(ssb_index) - 1;
	break;
      case 5: 
//	first_symbol_index = 13;
	n_c = get_ssb_slot(ssb_index) - 1;
	break;
      case 6: 
//	first_symbol_index = 0;
	break;
      case 7: 
//	first_symbol_index = 1;
	break;
      default: break; 
      }
    }else{ ; }
    //  38.213 chapter 13: over one slot
//    search_space_duration = 1;
  }

  /// MUX PATTERN 3
  if(mac->type0_pdcch_ss_mux_pattern == 3){
    if((scs_ssb == scs_120kHz) && (scs_pdcch == scs_120kHz)){
      //  38.213 Table 13-15
      AssertFatal(index_4lsb == 0, "38.213 Table 13-15 4 LSB out of range\n");
      //  PDCCH monitoring occasions (SFN and slot number) same as SSB frame-slot
      //                sfn_c = SFN_C_EQ_SFN_SSB;
      n_c = get_ssb_slot(ssb_index);
//      switch(ssb_index & 0x3){    //  ssb_index(i) mod 4
//      case 0:
//	first_symbol_index = 4;
//	break;
//      case 1:
//	first_symbol_index = 8;
//	break;
//      case 2:
//	first_symbol_index = 2;
//	break;
//      case 3:
//	first_symbol_index = 6;
//	break;
//      default: break;
//      }
    }else{ ; }
    //  38.213 chapter 13: over one slot
//    search_space_duration = 1;
  }

  AssertFatal(number_of_search_space_per_slot!=UINT_MAX,"");
  /*
  uint32_t coreset_duration = num_symbols * number_of_search_space_per_slot;
    mac->type0_pdcch_dci_config.number_of_candidates[0] = table_38213_10_1_1_c2[0];
    mac->type0_pdcch_dci_config.number_of_candidates[1] = table_38213_10_1_1_c2[1];
    mac->type0_pdcch_dci_config.number_of_candidates[2] = table_38213_10_1_1_c2[2];   //  CCE aggregation level = 4
    mac->type0_pdcch_dci_config.number_of_candidates[3] = table_38213_10_1_1_c2[3];   //  CCE aggregation level = 8
    mac->type0_pdcch_dci_config.number_of_candidates[4] = table_38213_10_1_1_c2[4];   //  CCE aggregation level = 16
    mac->type0_pdcch_dci_config.duration = search_space_duration;
    mac->type0_pdcch_dci_config.coreset.duration = coreset_duration;   //  coreset
    AssertFatal(first_symbol_index!=UINT_MAX,"");
    mac->type0_pdcch_dci_config.monitoring_symbols_within_slot = (0x3fff << first_symbol_index) & (0x3fff >> (14-coreset_duration-first_symbol_index)) & 0x3fff;
  */
  AssertFatal(sfn_c!=SFN_C_IMPOSSIBLE,"");
  AssertFatal(n_c!=UINT_MAX,"");
  mac->type0_pdcch_ss_sfn_c = sfn_c;
  mac->type0_pdcch_ss_n_c = n_c;
        
  // fill in the elements in config request inside P5 message
  mac->phy_config.Mod_id = module_id;
  mac->phy_config.CC_id = cc_id;

  mac->dl_config_request.sfn = frame;
  mac->dl_config_request.slot = (ssb_index>>1) + ((ssb_index>>4)<<1); // not valid for 240kHz SCS 

  //}
  return 0;

}


//  TODO: change to UE parameter, scs: 15KHz, slot duration: 1ms
uint32_t get_ssb_frame(uint32_t test){
  return test;
}

/*
 * This function returns the slot offset K2 corresponding to a given time domain
 * indication value from RRC configuration.
 */
long get_k2(NR_UE_MAC_INST_t *mac, uint8_t time_domain_ind) {
  long k2 = -1;
  // Get K2 from RRC configuration
  NR_PUSCH_Config_t *pusch_config=mac->ULbwp[0]->bwp_Dedicated->pusch_Config->choice.setup;
  NR_PUSCH_TimeDomainResourceAllocationList_t *pusch_TimeDomainAllocationList = NULL;
  if (pusch_config->pusch_TimeDomainAllocationList) {
    pusch_TimeDomainAllocationList = pusch_config->pusch_TimeDomainAllocationList->choice.setup;
  }
  else if (mac->ULbwp[0]->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList) {
    pusch_TimeDomainAllocationList = mac->ULbwp[0]->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
  }
  if (pusch_TimeDomainAllocationList) {
    if (time_domain_ind >= pusch_TimeDomainAllocationList->list.count) {
      LOG_E(MAC, "time_domain_ind %d >= pusch->TimeDomainAllocationList->list.count %d\n",
            time_domain_ind, pusch_TimeDomainAllocationList->list.count);
      return -1;
    }
    k2 = *pusch_TimeDomainAllocationList->list.array[time_domain_ind]->k2;
  }
  
  LOG_D(MAC, "get_k2(): k2 is %ld\n", k2);
  return k2;
}

/*
 * This function returns the UL config corresponding to a given UL slot
 * from MAC instance .
 */
fapi_nr_ul_config_request_t *get_ul_config_request(NR_UE_MAC_INST_t *mac, int slot) {
  //Check if request to access ul_config is for a UL slot
  if (is_nr_UL_slot(mac->scc, slot) == 0) {
    LOG_W(MAC, "Slot %d is not a UL slot. get_ul_config_request() called for wrong slot!!!\n", slot);
    return NULL;
  }
  
  // Calculate the index of the UL slot in mac->ul_config_request list. This is
  // based on the TDD pattern (slot configuration period) and number of UL+mixed
  // slots in the period. TS 38.213 Sec 11.1
  int mu = mac->ULbwp[0]->bwp_Common->genericParameters.subcarrierSpacing;
  NR_TDD_UL_DL_Pattern_t *tdd_pattern = &mac->scc->tdd_UL_DL_ConfigurationCommon->pattern1;
  const int num_slots_per_tdd = nr_slots_per_frame[mu] >> (7 - tdd_pattern->dl_UL_TransmissionPeriodicity);
  const int num_slots_ul = tdd_pattern->nrofUplinkSlots + (tdd_pattern->nrofUplinkSymbols!=0);
  int index = (slot + num_slots_ul - num_slots_per_tdd) % num_slots_per_tdd;
  LOG_D(MAC, "nr_ue_procedures: get_ul_config_request() slots per tdd %d, num_slots_ul %d, index %d\n", 
                num_slots_per_tdd,
                num_slots_ul,
                index);

  return &mac->ul_config_request[index];
}

// Performs :
// 1. TODO: Call RRC for link status return to PHY
// 2. TODO: Perform SR/BSR procedures for scheduling feedback
// 3. TODO: Perform PHR procedures
NR_UE_L2_STATE_t nr_ue_scheduler(nr_downlink_indication_t *dl_info, nr_uplink_indication_t *ul_info){

  uint32_t search_space_mask = 0;

  if (dl_info){

    module_id_t mod_id    = dl_info->module_id;
    uint32_t gNB_index    = dl_info->gNB_index;
    int cc_id             = dl_info->cc_id;
    frame_t rx_frame      = dl_info->frame;
    slot_t rx_slot        = dl_info->slot;
    NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);

    fapi_nr_dl_config_request_t *dl_config = &mac->dl_config_request;
    nr_scheduled_response_t scheduled_response;
    nr_dcireq_t dcireq;

    // check type0 from 38.213 13 if we have no CellGroupConfig
    // TODO: implementation to be completed
    if (mac->scg == NULL) {
      if(dl_info->ssb_index != -1){

        if(mac->type0_pdcch_ss_mux_pattern == 1){
          //  38.213 chapter 13
          if((mac->type0_pdcch_ss_sfn_c == SFN_C_MOD_2_EQ_0) && !(rx_frame & 0x1) && (rx_slot == mac->type0_pdcch_ss_n_c)){
            search_space_mask = search_space_mask | type0_pdcch;
            mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_dci_config.coreset.duration;
          }
          if((mac->type0_pdcch_ss_sfn_c == SFN_C_MOD_2_EQ_1) && (rx_frame & 0x1) && (rx_slot == mac->type0_pdcch_ss_n_c)){
            search_space_mask = search_space_mask | type0_pdcch;
            mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_dci_config.coreset.duration;
          }
        }
        if(mac->type0_pdcch_ss_mux_pattern == 2){
          //  38.213 Table 13-13, 13-14
          if((rx_frame == get_ssb_frame(rx_frame)) && (rx_slot == mac->type0_pdcch_ss_n_c)){
            search_space_mask = search_space_mask | type0_pdcch;
            mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_dci_config.coreset.duration;
          }
        }
        if(mac->type0_pdcch_ss_mux_pattern == 3){
          //  38.213 Table 13-15
          if((rx_frame == get_ssb_frame(rx_frame)) && (rx_slot == mac->type0_pdcch_ss_n_c)){
            search_space_mask = search_space_mask | type0_pdcch;
            mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_dci_config.coreset.duration;
          }
        }
      } // ssb_index != -1

      // Type0 PDCCH search space
      if((search_space_mask & type0_pdcch) || ( mac->type0_pdcch_consecutive_slots != 0 )){
        mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_consecutive_slots - 1;

        dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15 = mac->type0_pdcch_dci_config;
        dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DCI;

        /*
        dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.rnti = 0xaaaa;	//	to be set
        dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.N_RB_BWP = 106;	//	to be set

        LOG_I(MAC,"nr_ue_scheduler Type0 PDCCH with rnti %x, BWP %d\n",
        dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.rnti,
        dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.N_RB_BWP);
        */
        dl_config->number_pdus = dl_config->number_pdus + 1;

        fill_scheduled_response(&scheduled_response, dl_config, NULL, NULL, mod_id, cc_id, rx_frame, rx_slot, dl_info->thread_id);
        if(mac->if_module != NULL && mac->if_module->scheduled_response != NULL)
          mac->if_module->scheduled_response(&scheduled_response);
      }
    } else { // we have an scg

      dcireq.module_id = mod_id;
      dcireq.gNB_index = gNB_index;
      dcireq.cc_id     = cc_id;
      dcireq.frame     = rx_frame;
      dcireq.slot      = rx_slot;
      dcireq.dl_config_req.number_pdus = 0;
      nr_ue_dcireq(&dcireq); //to be replaced with function pointer later

      fill_scheduled_response(&scheduled_response, &dcireq.dl_config_req, NULL, NULL, mod_id, cc_id, rx_frame, rx_slot, dl_info->thread_id);
      if(mac->if_module != NULL && mac->if_module->scheduled_response != NULL){
        mac->if_module->scheduled_response(&scheduled_response);
      }

      /*
        if(search_space_mask & type0a_pdcch){
        }
        
        if(search_space_mask & type1_pdcch){
        }

        if(search_space_mask & type2_pdcch){
        }

        if(search_space_mask & type3_pdcch){
        }
      */
    }
  } else if (ul_info) {

    module_id_t mod_id    = ul_info->module_id;
    NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);

    if (mac->ra_state == RA_SUCCEEDED || get_softmodem_params()->phy_test) {

      uint8_t nb_dmrs_re_per_rb;
      uint8_t ulsch_input_buffer[MAX_ULSCH_PAYLOAD_BYTES];
      uint8_t data_existing = 0;
      uint16_t TBS_bytes;
      uint32_t TBS;
      int i, N_PRB_oh;

      uint32_t gNB_index    = ul_info->gNB_index;
      int cc_id             = ul_info->cc_id;
      frame_t rx_frame      = ul_info->frame_rx;
      slot_t rx_slot        = ul_info->slot_rx;
      frame_t frame_tx      = ul_info->frame_tx;
      slot_t slot_tx        = ul_info->slot_tx;
      uint8_t access_mode   = SCHEDULED_ACCESS;

      fapi_nr_ul_config_request_t *ul_config_req = get_ul_config_request(mac, slot_tx);

      // Schedule ULSCH only if the current frame and slot match those in ul_config_req
      // AND if a UL DCI has been received (as indicated by num_pdus).
      if ((frame_tx == ul_config_req->sfn && slot_tx == ul_config_req->slot) &&
          ul_config_req->number_pdus > 0) {
      
        // program PUSCH with UL DCI parameters
        nr_scheduled_response_t scheduled_response;
        fapi_nr_tx_request_t tx_req;
        nfapi_nr_ue_ptrs_ports_t ptrs_ports_list;

        for (int j = 0; j < ul_config_req->number_pdus; j++) {
          fapi_nr_ul_config_request_pdu_t *ulcfg_pdu = &ul_config_req->ul_config_list[j];

          if (ulcfg_pdu->pdu_type == FAPI_NR_UL_CONFIG_TYPE_PUSCH) {

            // These should come from RRC config!!!
            uint8_t  ptrs_mcs1          = 2;
            uint8_t  ptrs_mcs2          = 4;
            uint8_t  ptrs_mcs3          = 10;
            uint16_t n_rb0              = 25;
            uint16_t n_rb1              = 75;
            uint8_t  ptrs_time_density  = get_L_ptrs(ptrs_mcs1, ptrs_mcs2, ptrs_mcs3, ulcfg_pdu->pusch_config_pdu.mcs_index, ulcfg_pdu->pusch_config_pdu.mcs_table);
            uint8_t  ptrs_freq_density  = get_K_ptrs(n_rb0, n_rb1, ulcfg_pdu->pusch_config_pdu.rb_size);
            uint16_t l_prime_mask       = get_l_prime(ulcfg_pdu->pusch_config_pdu.nr_of_symbols, typeB, pusch_dmrs_pos0, pusch_len1);
            uint16_t ul_dmrs_symb_pos   = l_prime_mask << ulcfg_pdu->pusch_config_pdu.start_symbol_index;

            uint8_t  dmrs_config_type   = 0;
            uint16_t number_dmrs_symbols = 0;

            // PTRS ports configuration
            // TbD: ptrs_dmrs_port and ptrs_port_index are not initialised!
            ptrs_ports_list.ptrs_re_offset = 0;

            // Num PRB Overhead from PUSCH-ServingCellConfig
            if (mac->scg->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup->xOverhead == NULL)
              N_PRB_oh = 0;
            else
              N_PRB_oh = *mac->scg->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup->xOverhead;

            ulcfg_pdu->pusch_config_pdu.ul_dmrs_symb_pos = ul_dmrs_symb_pos;
            ulcfg_pdu->pusch_config_pdu.dmrs_config_type = dmrs_config_type;
            ulcfg_pdu->pusch_config_pdu.num_dmrs_cdm_grps_no_data = 1;
            ulcfg_pdu->pusch_config_pdu.nrOfLayers = 1;
            ulcfg_pdu->pusch_config_pdu.pusch_data.new_data_indicator = 0;
            ulcfg_pdu->pusch_config_pdu.pdu_bit_map = PUSCH_PDU_BITMAP_PUSCH_DATA;
            ulcfg_pdu->pusch_config_pdu.pusch_ptrs.ptrs_time_density = ptrs_time_density;
            ulcfg_pdu->pusch_config_pdu.pusch_ptrs.ptrs_freq_density = ptrs_freq_density;
            ulcfg_pdu->pusch_config_pdu.pusch_ptrs.ptrs_ports_list   = &ptrs_ports_list;
            //ulcfg_pdu->pusch_config_pdu.target_code_rate = nr_get_code_rate_ul(ulcfg_pdu->pusch_config_pdu.mcs_index, ulcfg_pdu->pusch_config_pdu.mcs_table);
            //ulcfg_pdu->pusch_config_pdu.qam_mod_order = nr_get_Qm_ul(ulcfg_pdu->pusch_config_pdu.mcs_index, ulcfg_pdu->pusch_config_pdu.mcs_table);

            if (1 << ulcfg_pdu->pusch_config_pdu.pusch_ptrs.ptrs_time_density >= ulcfg_pdu->pusch_config_pdu.nr_of_symbols) {
              ulcfg_pdu->pusch_config_pdu.pdu_bit_map &= ~PUSCH_PDU_BITMAP_PUSCH_PTRS; // disable PUSCH PTRS
            }

            get_num_re_dmrs(&ulcfg_pdu->pusch_config_pdu,
                            &nb_dmrs_re_per_rb,
                            &number_dmrs_symbols);

            TBS = nr_compute_tbs(ulcfg_pdu->pusch_config_pdu.qam_mod_order,
                                ulcfg_pdu->pusch_config_pdu.target_code_rate,
                                ulcfg_pdu->pusch_config_pdu.rb_size,
                                ulcfg_pdu->pusch_config_pdu.nr_of_symbols,
                                nb_dmrs_re_per_rb*number_dmrs_symbols,
                                N_PRB_oh,
                                0,
                                ulcfg_pdu->pusch_config_pdu.nrOfLayers);
            TBS_bytes = TBS/8;
            ulcfg_pdu->pusch_config_pdu.pusch_data.tb_size = TBS_bytes;

            if (IS_SOFTMODEM_NOS1){
              // Getting IP traffic to be transmitted
              data_existing = nr_ue_get_sdu(mod_id,
                                            cc_id,
                                            frame_tx,
                                            slot_tx,
                                            0,
                                            ulsch_input_buffer,
                                            TBS_bytes,
                                            &access_mode);
            }

            //Random traffic to be transmitted if there is no IP traffic available for this Tx opportunity
            if (!IS_SOFTMODEM_NOS1 || !data_existing) {
              //Use zeros for the header bytes in noS1 mode, in order to make sure that the LCID is not valid
              //and block this traffic from being forwarded to the upper layers at the gNB
              LOG_D(PHY, "Random data to be tranmsitted: \n");

              //Give the first byte a dummy value (a value not corresponding to any valid LCID based on 38.321, Table 6.2.1-2)
              //in order to distinguish the PHY random packets at the MAC layer of the gNB receiver from the normal packets that should
              //have a valid LCID (nr_process_mac_pdu function)
              ulsch_input_buffer[0] = 0x31;

              for (i = 1; i < TBS_bytes; i++) {
                ulsch_input_buffer[i] = (unsigned char) rand();
                //printf(" input encoder a[%d]=0x%02x\n",i,harq_process_ul_ue->a[i]);
              }
            }

    #ifdef DEBUG_MAC_PDU

            LOG_D(PHY, "Is data existing ?: %d \n", data_existing);
            LOG_I(PHY, "Printing MAC PDU to be encoded, TBS is: %d \n", TBS_bytes);
            for (i = 0; i < TBS_bytes; i++) {
              printf("%02x", ulsch_input_buffer[i]);
            }
            printf("\n");

    #endif

            // Config UL TX PDU
            tx_req.slot = slot_tx;
            tx_req.sfn = frame_tx;
            // tx_req->tx_config // TbD
            tx_req.number_of_pdus++;
            tx_req.tx_request_body[j].pdu_length = TBS_bytes;
            tx_req.tx_request_body[j].pdu_index = j;
            tx_req.tx_request_body[j].pdu = ulsch_input_buffer;
          }
        }

        fill_scheduled_response(&scheduled_response, NULL, ul_config_req, &tx_req, mod_id, cc_id, rx_frame, rx_slot, ul_info->thread_id);
        if(mac->if_module != NULL && mac->if_module->scheduled_response != NULL){
          mac->if_module->scheduled_response(&scheduled_response);
        }

        // TODO: expand
        // Note: Contention resolution is currently not active
        if (mac->RA_contention_resolution_timer_active == 1)
          ue_contention_resolution(mod_id, gNB_index, cc_id, ul_info->frame_tx);

      }

    } else if (get_softmodem_params()->do_ra){

      NR_UE_MAC_INST_t *mac = get_mac_inst(ul_info->module_id);

      if (mac->RA_active && ul_info->slot_tx == mac->msg3_slot && ul_info->frame_tx == mac->msg3_frame){

        uint8_t ulsch_input_buffer[MAX_ULSCH_PAYLOAD_BYTES];
        nr_scheduled_response_t scheduled_response;
        fapi_nr_tx_request_t tx_req;
        //fapi_nr_ul_config_request_t *ul_config = get_ul_config_request(mac, ul_info->slot_tx);
        fapi_nr_ul_config_request_t *ul_config = &mac->ul_config_request[0];
        fapi_nr_ul_config_request_pdu_t *ul_config_list = &ul_config->ul_config_list[ul_config->number_pdus];
        uint16_t TBS_bytes = ul_config_list->pusch_config_pdu.pusch_data.tb_size;

        //if (IS_SOFTMODEM_NOS1){
        //  // Getting IP traffic to be transmitted
        //  data_existing = nr_ue_get_sdu(mod_id,
        //                                cc_id,
        //                                frame_tx,
        //                                slot_tx,
        //                                0,
        //                                ulsch_input_buffer,
        //                                TBS_bytes,
        //                                &access_mode);
        //}

        //Random traffic to be transmitted if there is no IP traffic available for this Tx opportunity
        //if (!IS_SOFTMODEM_NOS1 || !data_existing) {
          //Use zeros for the header bytes in noS1 mode, in order to make sure that the LCID is not valid
          //and block this traffic from being forwarded to the upper layers at the gNB
          LOG_D(MAC, "Random data to be tranmsitted (TBS_bytes %d): \n", TBS_bytes);
          //Give the first byte a dummy value (a value not corresponding to any valid LCID based on 38.321, Table 6.2.1-2)
          //in order to distinguish the PHY random packets at the MAC layer of the gNB receiver from the normal packets that should
          //have a valid LCID (nr_process_mac_pdu function)
          ulsch_input_buffer[0] = 0x31;
          for (int i = 1; i < TBS_bytes; i++) {
            ulsch_input_buffer[i] = (unsigned char) rand();
            //printf(" input encoder a[%d]=0x%02x\n",i,harq_process_ul_ue->a[i]);
          }
        //}

        LOG_D(MAC, "[UE %d] Frame %d, Subframe %d Adding Msg3 UL Config Request for rnti: %x\n",
          ul_info->module_id,
          ul_info->frame_tx,
          ul_info->slot_tx,
          mac->t_crnti);

        // Config UL TX PDU
        tx_req.slot = ul_info->slot_tx;
        tx_req.sfn = ul_info->frame_tx;
        tx_req.number_of_pdus = 1;
        tx_req.tx_request_body[0].pdu_length = TBS_bytes;
        tx_req.tx_request_body[0].pdu_index = 0;
        tx_req.tx_request_body[0].pdu = ulsch_input_buffer;
        ul_config_list->pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUSCH;
        ul_config->number_pdus++;
        // scheduled_response
        fill_scheduled_response(&scheduled_response, NULL, ul_config, &tx_req, ul_info->module_id, ul_info->cc_id, ul_info->frame_rx, ul_info->slot_rx, ul_info->thread_id);
        if(mac->if_module != NULL && mac->if_module->scheduled_response != NULL){
          mac->if_module->scheduled_response(&scheduled_response);
        }
      }
    }
  }
  return UE_CONNECTION_OK;
}

// PUSCH Msg3 scheduled by RAR UL grant according to 8.3 of TS 38.213
// Note: Msg3 tx in the uplink symbols of mixed slot
void nr_ue_msg3_scheduler(NR_UE_MAC_INST_t *mac,
                          frame_t current_frame,
                          sub_frame_t current_slot,
                          uint8_t Msg3_tda_id){

  int delta = 0;
  NR_BWP_Uplink_t *ubwp = mac->ULbwp[0];
  int mu = ubwp->bwp_Common->genericParameters.subcarrierSpacing;
  struct NR_PUSCH_TimeDomainResourceAllocationList *pusch_TimeDomainAllocationList = ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
  // k2 as per 3GPP TS 38.214 version 15.9.0 Release 15 ch 6.1.2.1.1
  // PUSCH time domain resource allocation is higher layer configured from uschTimeDomainAllocationList in either pusch-ConfigCommon
  uint8_t k2 = *pusch_TimeDomainAllocationList->list.array[Msg3_tda_id]->k2;

  switch (mu) {
    case 0:
      delta = 2;
      break;
    case 1:
      delta = 3;
      break;
    case 2:
      delta = 4;
      break;
    case 3:
      delta = 6;
      break;
  }

  mac->msg3_slot = (current_slot + k2 + delta) % nr_slots_per_frame[mu];
  if (current_slot + k2 + delta > nr_slots_per_frame[mu])
    mac->msg3_frame = (current_frame + 1) % 1024;
  else
    mac->msg3_frame = current_frame;

  #ifdef DEBUG_MSG3
  LOG_D(MAC, "[DEBUG_MSG3] current_slot %d k2 %d delta %d temp_slot %d mac->msg3_frame %d mac->msg3_slot %d \n", current_slot, k2, delta, current_slot + k2 + delta, mac->msg3_frame, mac->msg3_slot);
  #endif
}

// This function schedules the PRACH according to prach_ConfigurationIndex and TS 38.211, tables 6.3.3.2.x
// PRACH formats 9, 10, 11 are corresponding to dual PRACH format configurations A1/B1, A2/B2, A3/B3.
// - todo:
// - Partial configuration is actually already stored in (fapi_nr_prach_config_t) &mac->phy_config.config_req->prach_config
void nr_ue_prach_scheduler(module_id_t module_idP, frame_t frameP, sub_frame_t slotP, int thread_id) {

  uint16_t format, format0, format1, ncs;
  int is_nr_prach_slot;
  prach_occasion_info_t *prach_occasion_info_p;

  NR_UE_MAC_INST_t *mac = get_mac_inst(module_idP);

  //fapi_nr_ul_config_request_t *ul_config = get_ul_config_request(mac, slotP);
  fapi_nr_ul_config_request_t *ul_config = &mac->ul_config_request[0];
  fapi_nr_ul_config_prach_pdu *prach_config_pdu;
  fapi_nr_config_request_t *cfg = &mac->phy_config.config_req;
  fapi_nr_prach_config_t *prach_config = &cfg->prach_config;
  nr_scheduled_response_t scheduled_response;

  NR_ServingCellConfigCommon_t *scc = mac->scc;
  NR_RACH_ConfigCommon_t *setup = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  NR_RACH_ConfigGeneric_t *rach_ConfigGeneric = &setup->rach_ConfigGeneric;

  mac->RA_offset = 2; // to compensate the rx frame offset at the gNB
  mac->generate_nr_prach = 0; // Reset flag for PRACH generation

  if (is_nr_UL_slot(scc, slotP)) {

    // WIP Need to get the proper selected ssb_idx
    //     Initial beam selection functionality is not available yet
    uint8_t selected_gnb_ssb_idx = 0;

    // Get any valid PRACH occasion in the current slot for the selected SSB index
    is_nr_prach_slot = get_nr_prach_info_from_ssb_index(selected_gnb_ssb_idx,
                                                       (int)frameP,
                                                       (int)slotP,
                                                        &prach_occasion_info_p);

    if (is_nr_prach_slot && mac->ra_state == RA_UE_IDLE) {
      AssertFatal(NULL != prach_occasion_info_p,"PRACH Occasion Info not returned in a valid NR Prach Slot\n");

      mac->generate_nr_prach = 1;

      format = prach_occasion_info_p->format;
      format0 = format & 0xff;        // single PRACH format
      format1 = (format >> 8) & 0xff; // dual PRACH format

      ul_config->sfn = frameP;
      ul_config->slot = slotP;

      ul_config->ul_config_list[ul_config->number_pdus].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PRACH;
      prach_config_pdu = &ul_config->ul_config_list[ul_config->number_pdus].prach_config_pdu;
      memset(prach_config_pdu, 0, sizeof(fapi_nr_ul_config_prach_pdu));
      ul_config->number_pdus += 1;

      ncs = get_NCS(rach_ConfigGeneric->zeroCorrelationZoneConfig, format0, setup->restrictedSetConfig);

      prach_config_pdu->phys_cell_id = *scc->physCellId;
      prach_config_pdu->num_prach_ocas = 1;
      prach_config_pdu->prach_slot = prach_occasion_info_p->slot;
      prach_config_pdu->prach_start_symbol = prach_occasion_info_p->start_symbol;
      prach_config_pdu->num_ra = prach_occasion_info_p->fdm;

      prach_config_pdu->num_cs = ncs;
      prach_config_pdu->root_seq_id = prach_config->num_prach_fd_occasions_list[prach_occasion_info_p->fdm].prach_root_sequence_index;
      prach_config_pdu->restricted_set = prach_config->restricted_set_config;
      prach_config_pdu->freq_msg1 = prach_config->num_prach_fd_occasions_list[prach_occasion_info_p->fdm].k1;

      LOG_D(MAC,"Selected RO Frame %u, Slot %u, Symbol %u, Fdm %u\n", frameP, prach_config_pdu->prach_slot, prach_config_pdu->prach_start_symbol, prach_config_pdu->num_ra);

      // Search which SSB is mapped in the RO (among all the SSBs mapped to this RO)
      for (prach_config_pdu->ssb_nb_in_ro=0; prach_config_pdu->ssb_nb_in_ro<prach_occasion_info_p->nb_mapped_ssb; prach_config_pdu->ssb_nb_in_ro++) {
        if (prach_occasion_info_p->mapped_ssb_idx[prach_config_pdu->ssb_nb_in_ro] == selected_gnb_ssb_idx)
          break;
      }
      AssertFatal(prach_config_pdu->ssb_nb_in_ro<prach_occasion_info_p->nb_mapped_ssb, "%u not found in the mapped SSBs to the PRACH occasion", selected_gnb_ssb_idx);

      if (format1 != 0xff) {
        switch(format0) { // dual PRACH format
          case 0xa1:
            prach_config_pdu->prach_format = 11;
            break;
          case 0xa2:
            prach_config_pdu->prach_format = 12;
            break;
          case 0xa3:
            prach_config_pdu->prach_format = 13;
            break;
        default:
          AssertFatal(1 == 0, "Only formats A1/B1 A2/B2 A3/B3 are valid for dual format");
        }
      } else {
        switch(format0) { // single PRACH format
          case 0:
            prach_config_pdu->prach_format = 0;
            break;
          case 1:
            prach_config_pdu->prach_format = 1;
            break;
          case 2:
            prach_config_pdu->prach_format = 2;
            break;
          case 3:
            prach_config_pdu->prach_format = 3;
            break;
          case 0xa1:
            prach_config_pdu->prach_format = 4;
            break;
          case 0xa2:
            prach_config_pdu->prach_format = 5;
            break;
          case 0xa3:
            prach_config_pdu->prach_format = 6;
            break;
          case 0xb1:
            prach_config_pdu->prach_format = 7;
            break;
          case 0xb4:
            prach_config_pdu->prach_format = 8;
            break;
          case 0xc0:
            prach_config_pdu->prach_format = 9;
            break;
          case 0xc2:
            prach_config_pdu->prach_format = 10;
            break;
          default:
            AssertFatal(1 == 0, "Invalid PRACH format");
        }
      } // if format1
    } // is_nr_prach_slot

    fill_scheduled_response(&scheduled_response, NULL, ul_config, NULL, module_idP, 0 /*TBR fix*/, frameP, slotP, thread_id);
    if(mac->if_module != NULL && mac->if_module->scheduled_response != NULL)
      mac->if_module->scheduled_response(&scheduled_response);
  } // if is_nr_UL_slot
}

////////////////////////////////////////////////////////////////////////////
/////////* Random Access Contention Resolution (5.1.35 TS 38.321) */////////
////////////////////////////////////////////////////////////////////////////
// Handling contention resolution timer
// WIP todo:
// - beam failure recovery
// - RA completed

void ue_contention_resolution(module_id_t module_id, uint8_t gNB_index, int cc_id, frame_t tx_frame){
  
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  NR_ServingCellConfigCommon_t *scc = mac->scc;
  NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;

  if (mac->RA_contention_resolution_timer_active == 1) {
    if (nr_rach_ConfigCommon){
      LOG_I(MAC, "Frame %d: Contention resolution timer %d/%ld\n",
        tx_frame,
        mac->RA_contention_resolution_cnt,
        ((1 + nr_rach_ConfigCommon->ra_ContentionResolutionTimer) << 3));
        mac->RA_contention_resolution_cnt++;

      if (mac->RA_contention_resolution_cnt == ((1 + nr_rach_ConfigCommon->ra_ContentionResolutionTimer) << 3)) {
        mac->t_crnti = 0;
        mac->RA_active = 0;
        mac->RA_contention_resolution_timer_active = 0;
        // Signal PHY to quit RA procedure
        LOG_E(MAC, "[UE %u] [RAPROC] Contention resolution timer expired, RA failed, discarded TC-RNTI\n", module_id);
        nr_ra_failed(module_id, cc_id, gNB_index);
      }
    }
  }
}

#if 0
uint16_t nr_dci_format_size (PHY_VARS_NR_UE *ue,
                             uint8_t slot,
                             int p,
                             crc_scrambled_t crc_scrambled,
                             uint8_t dci_fields_sizes[NBR_NR_DCI_FIELDS][NBR_NR_FORMATS],
                             uint8_t format) {
  LOG_DDD("crc_scrambled=%d, n_RB_ULBWP=%d, n_RB_DLBWP=%d\n",crc_scrambled,n_RB_ULBWP,n_RB_DLBWP);
  /*
   * function nr_dci_format_size calculates and returns the size in bits of a determined format
   * it also returns an bi-dimensional array 'dci_fields_sizes' with x rows and y columns, where:
   * x is the number of fields defined in TS 38.212 subclause 7.3.1 (Each field is mapped in the order in which it appears in the description in the specification)
   * y is the number of formats
   *   e.g.: dci_fields_sizes[10][0] contains the size in bits of the field FREQ_DOM_RESOURCE_ASSIGNMENT_UL for format 0_0
   */
  // pdsch_config contains the PDSCH-Config IE is used to configure the UE specific PDSCH parameters (TS 38.331)
  PDSCH_Config_t pdsch_config       = ue->PDSCH_Config;
  // pusch_config contains the PUSCH-Config IE is used to configure the UE specific PUSCH parameters (TS 38.331)
  PUSCH_Config_t pusch_config       = ue->pusch_config;
  PUCCH_Config_t pucch_config_dedicated       = ue->pucch_config_dedicated_nr[eNB_id];
  crossCarrierSchedulingConfig_t crossCarrierSchedulingConfig = ue->crossCarrierSchedulingConfig;
  dmrs_UplinkConfig_t dmrs_UplinkConfig = ue->dmrs_UplinkConfig;
  dmrs_DownlinkConfig_t dmrs_DownlinkConfig = ue->dmrs_DownlinkConfig;
  csi_MeasConfig_t csi_MeasConfig = ue->csi_MeasConfig;
  PUSCH_ServingCellConfig_t PUSCH_ServingCellConfig= ue->PUSCH_ServingCellConfig;
  PDSCH_ServingCellConfig_t PDSCH_ServingCellConfig= ue->PDSCH_ServingCellConfig;
  NR_UE_PDCCH *pdcch_vars2 = ue->pdcch_vars[proc->thread_id][eNB_id];
  // 1  CARRIER_IN
  // crossCarrierSchedulingConfig from higher layers, variable crossCarrierSchedulingConfig indicates if 'cross carrier scheduling' is enabled or not:
  //      if No cross carrier scheduling: number of bits for CARRIER_IND is 0
  //      if Cross carrier scheduling: number of bits for CARRIER_IND is 3
  // The IE CrossCarrierSchedulingConfig is used to specify the configuration when the cross-carrier scheduling is used in a cell
  uint8_t crossCarrierSchedulingConfig_ind = 0;

  if (crossCarrierSchedulingConfig.schedulingCellInfo.other.cif_InSchedulingCell !=0 ) crossCarrierSchedulingConfig_ind=1;

  // 2  SUL_IND_0_1, // 40 SRS_REQUEST, // 50 SUL_IND_0_0
  // UL/SUL indicator (TS 38.331, supplementary uplink is indicated in higher layer parameter ServCellAdd-SUL from IE ServingCellConfig and ServingCellConfigCommon):
  // 0 bit for UEs not configured with SUL in the cell or UEs configured with SUL in the cell but only PUCCH carrier in the cell is configured for PUSCH transmission
  // 1 bit for UEs configured with SUL in the cell as defined in Table 7.3.1.1.1-1
  // sul_ind indicates whether SUL is configured in cell or not
  uint8_t sul_ind=ue->supplementaryUplink.supplementaryUplink; // this value will be 0 or 1 depending on higher layer parameter ServCellAdd-SUL. FIXME!!!
  // 7  BANDWIDTH_PART_IND
  // number of UL BWPs configured by higher layers
  uint8_t n_UL_BWP_RRC=1; // initialized to 1 but it has to be initialized by higher layers FIXME!!!
  n_UL_BWP_RRC = ((n_UL_BWP_RRC > 3)?n_UL_BWP_RRC:(n_UL_BWP_RRC+1));
  // number of DL BWPs configured by higher layers
  uint8_t n_DL_BWP_RRC=1; // initialized to 1 but it has to be initialized by higher layers FIXME!!!
  n_DL_BWP_RRC = ((n_DL_BWP_RRC > 3)?n_DL_BWP_RRC:(n_DL_BWP_RRC+1));
  // 10 FREQ_DOM_RESOURCE_ASSIGNMENT_UL
  // if format0_0, only resource allocation type 1 is allowed
  // if format0_1, then resource allocation type 0 can be configured and N_RBG is defined in TS 38.214 subclause 6.1.2.2.1
  // for PUSCH hopping with resource allocation type 1
  //      n_UL_hopping = 1 if the higher layer parameter frequencyHoppingOffsetLists contains two  offset values
  //      n_UL_hopping = 2 if the higher layer parameter frequencyHoppingOffsetLists contains four offset values
  uint8_t n_UL_hopping=pusch_config.n_frequencyHoppingOffsetLists;

  if (n_UL_hopping == 2) {
    n_UL_hopping = 1;
  } else if (n_UL_hopping == 4) {
    n_UL_hopping = 2;
  } else {
    n_UL_hopping = 0;
  }

  ul_resourceAllocation_t ul_resource_allocation_type = pusch_config.ul_resourceAllocation;
  uint8_t ul_res_alloc_type_0 = 0;
  uint8_t ul_res_alloc_type_1 = 0;

  if (ul_resource_allocation_type == ul_resourceAllocationType0) ul_res_alloc_type_0 = 1;

  if (ul_resource_allocation_type == ul_resourceAllocationType1) ul_res_alloc_type_1 = 1;

  if (ul_resource_allocation_type == ul_dynamicSwitch) {
    ul_res_alloc_type_0 = 1;
    ul_res_alloc_type_1 = 1;
  }

  uint8_t n_bits_freq_dom_res_assign_ul=0,n_ul_RGB_tmp;

  if (ul_res_alloc_type_0 == 1) { // implementation of Table 6.1.2.2.1-1 TC 38.214 subclause 6.1.2.2.1
    // config1: PUSCH-Config IE contains rbg-Size ENUMERATED {config1 config2}
    ul_rgb_Size_t config = pusch_config.ul_rgbSize;
    uint8_t nominal_RBG_P               = (config==ul_rgb_config1?2:4);

    if (n_RB_ULBWP > 36)  nominal_RBG_P = (config==ul_rgb_config1?4:8);

    if (n_RB_ULBWP > 72)  nominal_RBG_P = (config==ul_rgb_config1?8:16);

    if (n_RB_ULBWP > 144) nominal_RBG_P = 16;

    n_bits_freq_dom_res_assign_ul = (uint8_t)ceil((n_RB_ULBWP+(0%nominal_RBG_P))/nominal_RBG_P);                                   //FIXME!!! what is 0???
    n_ul_RGB_tmp = n_bits_freq_dom_res_assign_ul;
  }

  if (ul_res_alloc_type_1 == 1) n_bits_freq_dom_res_assign_ul = (uint8_t)(ceil(log2(n_RB_ULBWP*(n_RB_ULBWP+1)/2)))-n_UL_hopping;

  if ((ul_res_alloc_type_0 == 1) && (ul_res_alloc_type_1 == 1))
    n_bits_freq_dom_res_assign_ul = ((n_bits_freq_dom_res_assign_ul>n_ul_RGB_tmp)?(n_bits_freq_dom_res_assign_ul+1):(n_ul_RGB_tmp+1));

  // 11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL
  // if format1_0, only resource allocation type 1 is allowed
  // if format1_1, then resource allocation type 0 can be configured and N_RBG is defined in TS 38.214 subclause 5.1.2.2.1
  dl_resourceAllocation_t dl_resource_allocation_type = pdsch_config.dl_resourceAllocation;
  uint8_t dl_res_alloc_type_0 = 0;
  uint8_t dl_res_alloc_type_1 = 0;

  if (dl_resource_allocation_type == dl_resourceAllocationType0) dl_res_alloc_type_0 = 1;

  if (dl_resource_allocation_type == dl_resourceAllocationType1) dl_res_alloc_type_1 = 1;

  if (dl_resource_allocation_type == dl_dynamicSwitch) {
    dl_res_alloc_type_0 = 1;
    dl_res_alloc_type_1 = 1;
  }

  uint8_t n_bits_freq_dom_res_assign_dl=0,n_dl_RGB_tmp;

  if (dl_res_alloc_type_0 == 1) { // implementation of Table 5.1.2.2.1-1 TC 38.214 subclause 6.1.2.2.1
    // config1: PDSCH-Config IE contains rbg-Size ENUMERATED {config1, config2}
    dl_rgb_Size_t config = pdsch_config.dl_rgbSize;
    uint8_t nominal_RBG_P               = (config==dl_rgb_config1?2:4);

    if (n_RB_DLBWP > 36)  nominal_RBG_P = (config==dl_rgb_config1?4:8);

    if (n_RB_DLBWP > 72)  nominal_RBG_P = (config==dl_rgb_config1?8:16);

    if (n_RB_DLBWP > 144) nominal_RBG_P = 16;

    n_bits_freq_dom_res_assign_dl = (uint8_t)ceil((n_RB_DLBWP+(0%nominal_RBG_P))/nominal_RBG_P);                                     //FIXME!!! what is 0???
    n_dl_RGB_tmp = n_bits_freq_dom_res_assign_dl;
  }

  if (dl_res_alloc_type_1 == 1) n_bits_freq_dom_res_assign_dl = (uint8_t)(ceil(log2(n_RB_DLBWP*(n_RB_DLBWP+1)/2)));

  if ((dl_res_alloc_type_0 == 1) && (dl_res_alloc_type_1 == 1))
    n_bits_freq_dom_res_assign_dl = ((n_bits_freq_dom_res_assign_dl>n_dl_RGB_tmp)?(n_bits_freq_dom_res_assign_dl+1):(n_dl_RGB_tmp+1));

  // 12 TIME_DOM_RESOURCE_ASSIGNMENT
  uint8_t pusch_alloc_list = pusch_config.n_push_alloc_list;
  uint8_t pdsch_alloc_list = pdsch_config.n_pdsh_alloc_list;
  // 14 PRB_BUNDLING_SIZE_IND:0 bit if the higher layer parameter PRB_bundling is not configured or is set to 'static', or 1 bit if the higher layer parameter PRB_bundling is set to 'dynamic' according to Subclause 5.1.2.3 of [6, TS 38.214]
  static_bundleSize_t static_prb_BundlingType = pdsch_config.prbBundleType.staticBundling;
  bundleSizeSet1_t dynamic_prb_BundlingType1  = pdsch_config.prbBundleType.dynamicBundlig.bundleSizeSet1;
  bundleSizeSet2_t dynamic_prb_BundlingType2  = pdsch_config.prbBundleType.dynamicBundlig.bundleSizeSet2;
  uint8_t prb_BundlingType_size=0;

  if ((static_prb_BundlingType==st_n4)||(static_prb_BundlingType==st_wideband)) prb_BundlingType_size=0;

  if ((dynamic_prb_BundlingType1==dy_1_n4)||(dynamic_prb_BundlingType1==dy_1_wideband)||(dynamic_prb_BundlingType1==dy_1_n2_wideband)||(dynamic_prb_BundlingType1==dy_1_n4_wideband)||
      (dynamic_prb_BundlingType2==dy_2_n4)||(dynamic_prb_BundlingType2==dy_2_wideband)) prb_BundlingType_size=1;

  // 15 RATE_MATCHING_IND FIXME!!!
  // according to TS 38.212: Rate matching indicator  E0, 1, or 2 bits according to higher layer parameter rateMatchPattern
  uint8_t rateMatching_bits = pdsch_config.n_rateMatchPatterns;
  // 16 ZP_CSI_RS_TRIGGER FIXME!!!
  // 0, 1, or 2 bits as defined in Subclause 5.1.4.2 of [6, TS 38.214].
  // is the number of ZP CSI-RS resource sets in the higher layer parameter zp-CSI-RS-Resource
  uint8_t n_zp_bits = pdsch_config.n_zp_CSI_RS_ResourceId;
  // 17 FREQ_HOPPING_FLAG
  // freqHopping is defined by higher layer parameter frequencyHopping from IE PUSCH-Config. Values are ENUMERATED{mode1, mode2}
  frequencyHopping_t f_hopping = pusch_config.frequencyHopping;
  uint8_t freqHopping = 0;

  if ((f_hopping==f_hop_mode1)||(f_hopping==f_hop_mode2)) freqHopping = 1;

  // 28 DAI
  pdsch_HARQ_ACK_Codebook_t pdsch_HARQ_ACK_Codebook = pdsch_config.pdsch_HARQ_ACK_Codebook;
  uint8_t n_dai = 0;
  uint8_t n_serving_cell_dl = 1; // this is hardcoded to 1 as we need to get this value from RRC higher layers parameters. FIXME!!!

  if ((pdsch_HARQ_ACK_Codebook == dynamic) && (n_serving_cell_dl == 1)) n_dai = 2;

  if ((pdsch_HARQ_ACK_Codebook == dynamic) && (n_serving_cell_dl > 1))  n_dai = 4;

  // 29 FIRST_DAI
  uint8_t codebook_HARQ_ACK = 0;           // We need to get this value to calculate number of bits of fields 1st DAI and 2nd DAI.

  if (pdsch_HARQ_ACK_Codebook == semiStatic) codebook_HARQ_ACK = 1;

  if (pdsch_HARQ_ACK_Codebook == dynamic) codebook_HARQ_ACK = 2;

  // 30 SECOND_DAI
  uint8_t n_HARQ_ACK_sub_codebooks = 0;   // We need to get this value to calculate number of bits of fields 1st DAI and 2nd DAI. FIXME!!!
  // 35 PDSCH_TO_HARQ_FEEDBACK_TIME_IND
  uint8_t pdsch_harq_t_ind = (uint8_t)ceil(log2(pucch_config_dedicated.dl_DataToUL_ACK[0]));
  // 36 SRS_RESOURCE_IND
  // n_SRS is the number of configured SRS resources in the SRS resource set associated with the higher layer parameter usage of value 'codeBook' or 'nonCodeBook'
  // from SRS_ResourceSet_t type we should get the information of the usage parameter (with possible values beamManagement, codebook, nonCodebook, antennaSwitching)
  // at frame_parms->srs_nr->p_SRS_ResourceSetList[]->usage
  uint8_t n_SRS = ue->srs.number_srs_Resource_Set;
  // 37 PRECOD_NBR_LAYERS
  // 38 ANTENNA_PORTS
  txConfig_t txConfig = pusch_config.txConfig;
  transformPrecoder_t transformPrecoder = pusch_config.transformPrecoder;
  codebookSubset_t codebookSubset = pusch_config.codebookSubset;
  uint8_t maxRank = pusch_config.maxRank;
  uint8_t num_antenna_ports = 1; // this is hardcoded. We need to get the real value FIXME!!!
  uint8_t precond_nbr_layers_bits = 0;
  uint8_t antenna_ports_bits_ul = 0;

  // searching number of bits at tables 7.3.1.1.2-2/3/4/5 from TS 38.212 subclause 7.3.1.1.2
  if (txConfig == txConfig_codebook) {
    if (num_antenna_ports == 4) {
      if ((transformPrecoder == transformPrecoder_disabled) && ((maxRank == 2)||(maxRank == 3)||(maxRank == 4))) { // Table 7.3.1.1.2-2
        if (codebookSubset == codebookSubset_fullyAndPartialAndNonCoherent) precond_nbr_layers_bits=6;

        if (codebookSubset == codebookSubset_partialAndNonCoherent) precond_nbr_layers_bits=5;

        if (codebookSubset == codebookSubset_nonCoherent) precond_nbr_layers_bits=4;
      }

      if (((transformPrecoder == transformPrecoder_enabled)||(transformPrecoder == transformPrecoder_disabled)) && (maxRank == 1)) { // Table 7.3.1.1.2-3
        if (codebookSubset == codebookSubset_fullyAndPartialAndNonCoherent) precond_nbr_layers_bits=5;

        if (codebookSubset == codebookSubset_partialAndNonCoherent) precond_nbr_layers_bits=4;

        if (codebookSubset == codebookSubset_nonCoherent) precond_nbr_layers_bits=2;
      }
    }

    if (num_antenna_ports == 2) {
      if ((transformPrecoder == transformPrecoder_disabled) && (maxRank == 2)) { // Table 7.3.1.1.2-4
        if (codebookSubset == codebookSubset_fullyAndPartialAndNonCoherent) precond_nbr_layers_bits=4;

        if (codebookSubset == codebookSubset_nonCoherent) precond_nbr_layers_bits=2;
      }

      if (((transformPrecoder == transformPrecoder_enabled)||(transformPrecoder == transformPrecoder_disabled)) && (maxRank == 1)) { // Table 7.3.1.1.2-5
        if (codebookSubset == codebookSubset_fullyAndPartialAndNonCoherent) precond_nbr_layers_bits=3;

        if (codebookSubset == codebookSubset_nonCoherent) precond_nbr_layers_bits=1;
      }
    }
  }

  if (txConfig == txConfig_nonCodebook) {
  }

  // searching number of bits at tables 7.3.1.1.2-6/7/8/9/10/11/12/13/14/15/16/17/18/19
  if((dmrs_UplinkConfig.pusch_dmrs_type == pusch_dmrs_type1)) {
    if ((transformPrecoder == transformPrecoder_enabled) && (dmrs_UplinkConfig.pusch_maxLength == pusch_len1)) antenna_ports_bits_ul = 2;

    if ((transformPrecoder == transformPrecoder_enabled) && (dmrs_UplinkConfig.pusch_maxLength == pusch_len2)) antenna_ports_bits_ul = 4;

    if ((transformPrecoder == transformPrecoder_disabled) && (dmrs_UplinkConfig.pusch_maxLength == pusch_len1)) antenna_ports_bits_ul = 3;

    if ((transformPrecoder == transformPrecoder_disabled) && (dmrs_UplinkConfig.pusch_maxLength == pusch_len2)) antenna_ports_bits_ul = 4;
  }

  if((dmrs_UplinkConfig.pusch_dmrs_type == pusch_dmrs_type2)) {
    if ((transformPrecoder == transformPrecoder_disabled) && (dmrs_UplinkConfig.pusch_maxLength == pusch_len1)) antenna_ports_bits_ul = 4;

    if ((transformPrecoder == transformPrecoder_disabled) && (dmrs_UplinkConfig.pusch_maxLength == pusch_len2)) antenna_ports_bits_ul = 5;
  }

  // for format 1_1 number of bits as defined by Tables 7.3.1.2.2-1/2/3/4
  uint8_t antenna_ports_bits_dl = 0;

  if((dmrs_DownlinkConfig.pdsch_dmrs_type == pdsch_dmrs_type1) && (dmrs_DownlinkConfig.pdsch_maxLength == pdsch_len1)) antenna_ports_bits_dl = 4; // Table 7.3.1.2.2-1

  if((dmrs_DownlinkConfig.pdsch_dmrs_type == pdsch_dmrs_type1) && (dmrs_DownlinkConfig.pdsch_maxLength == pdsch_len2)) antenna_ports_bits_dl = 5; // Table 7.3.1.2.2-2

  if((dmrs_DownlinkConfig.pdsch_dmrs_type == pdsch_dmrs_type2) && (dmrs_DownlinkConfig.pdsch_maxLength == pdsch_len1)) antenna_ports_bits_dl = 5; // Table 7.3.1.2.2-3

  if((dmrs_DownlinkConfig.pdsch_dmrs_type == pdsch_dmrs_type2) && (dmrs_DownlinkConfig.pdsch_maxLength == pdsch_len2)) antenna_ports_bits_dl = 6; // Table 7.3.1.2.2-4

  // 39 TCI
  uint8_t tci_bits=0;

  if (pdcch_vars2->coreset[p].tciPresentInDCI == tciPresentInDCI_enabled) tci_bits=3;

  // 42 CSI_REQUEST
  // reportTriggerSize is defined in the CSI-MeasConfig IE (TS 38.331).
  // Size of CSI request field in DCI (bits). Corresponds to L1 parameter 'ReportTriggerSize' (see 38.214, section 5.2)
  uint8_t reportTriggerSize = csi_MeasConfig.reportTriggerSize; // value from 0..6
  // 43 CBGTI
  // for format 0_1
  uint8_t maxCodeBlockGroupsPerTransportBlock = 0;

  if (PUSCH_ServingCellConfig.maxCodeBlockGroupsPerTransportBlock != 0)
    maxCodeBlockGroupsPerTransportBlock = (uint8_t)PUSCH_ServingCellConfig.maxCodeBlockGroupsPerTransportBlock;

  // for format 1_1, as defined in Subclause 5.1.7 of [6, TS38.214]
  uint8_t maxCodeBlockGroupsPerTransportBlock_dl = 0;

  if (PDSCH_ServingCellConfig.maxCodeBlockGroupsPerTransportBlock_dl != 0)
    maxCodeBlockGroupsPerTransportBlock_dl = pdsch_config.maxNrofCodeWordsScheduledByDCI; // FIXME!!!

  // 44 CBGFI
  uint8_t cbgfi_bit = PDSCH_ServingCellConfig.codeBlockGroupFlushIndicator;
  // 45 PTRS_DMRS
  // 0 bit if PTRS-UplinkConfig is not configured and transformPrecoder=disabled, or if transformPrecoder=enabled, or if maxRank=1
  // 2 bits otherwise
  uint8_t ptrs_dmrs_bits=0; //FIXME!!!
  // 46 BETA_OFFSET_IND
  // at IE PUSCH-Config, beta_offset indicator  E0 if the higher layer parameter betaOffsets = semiStatic; otherwise 2 bits
  // uci-OnPUSCH
  // Selection between and configuration of dynamic and semi-static beta-offset. If the field is absent or released, the UE applies the value 'semiStatic' and the BetaOffsets
  uint8_t betaOffsets = 0;

  if (pusch_config.uci_onPusch.betaOffset_type == betaOffset_semiStatic);

  if (pusch_config.uci_onPusch.betaOffset_type == betaOffset_dynamic) betaOffsets = 2;

  // 47 DMRS_SEQ_INI
  uint8_t dmrs_seq_ini_bits_ul = 0;
  uint8_t dmrs_seq_ini_bits_dl = 0;

  //1 bit if both scramblingID0 and scramblingID1 are configured in DMRS-UplinkConfig
  if ((transformPrecoder == transformPrecoder_disabled) && (dmrs_UplinkConfig.scramblingID0 != 0) && (dmrs_UplinkConfig.scramblingID1 != 0)) dmrs_seq_ini_bits_ul = 1;

  //1 bit if both scramblingID0 and scramblingID1 are configured in DMRS-DownlinkConfig
  if ((dmrs_DownlinkConfig.scramblingID0 != 0) && (dmrs_DownlinkConfig.scramblingID0 != 0)) dmrs_seq_ini_bits_dl = 1;

  /*
   * For format 2_2
   *
   * This format supports power control commands for semi-persistent scheduling.
   * As we can already support power control commands dynamically with formats 0_0/0_1 (TPC PUSCH) and 1_0/1_1 (TPC PUCCH)
   *
   * This format will be implemented in the future FIXME!!!
   *
   */
  // 5  BLOCK_NUMBER: The parameter tpc-PUSCH or tpc-PUCCH provided by higher layers determines the index to the block number for an UL of a cell
  // The following fields are defined for each block: Closed loop indicator and TPC command
  // 6  CLOSE_LOOP_IND
  // 41 TPC_CMD
  uint8_t tpc_cmd_bit_2_2 = 2;
  /*
   * For format 2_3
   *
   * This format is used for power control of uplink sounding reference signals for devices which have not coupled SRS power control to the PUSCH power control
   * either because independent control is desirable or because the device is configured without PUCCH and PUSCH
   *
   * This format will be implemented in the future FIXME!!!
   *
   */
  // 40 SRS_REQUEST
  // 41 TPC_CMD
  uint8_t tpc_cmd_bit_2_3 = 0;
  uint8_t dci_field_size_table [NBR_NR_DCI_FIELDS][NBR_NR_FORMATS] = { // This table contains the number of bits for each field (row) contained in each dci format (column).
    // The values of the variables indicate field sizes in number of bits
    //Format0_0                     Format0_1                      Format1_0                      Format1_1             Formats2_0/1/2/3
    {
      1,                             1,                             (((crc_scrambled == _p_rnti) || (crc_scrambled == _si_rnti) || (crc_scrambled == _ra_rnti)) ? 0:1),
      1,                             0,0,0,0
    }, // 0  IDENTIFIER_DCI_FORMATS:
    {
      0,                             ((crossCarrierSchedulingConfig_ind == 0) ? 0:3),
      0,                             ((crossCarrierSchedulingConfig_ind == 0) ? 0:3),
      0,0,0,0
    }, // 1  CARRIER_IND: 0 or 3 bits, as defined in Subclause x.x of [5, TS38.213]
    {0,                             (sul_ind == 0)?0:1,            0,                             0,                             0,0,0,0}, // 2  SUL_IND_0_1:
    {0,                             0,                             0,                             0,                             1,0,0,0}, // 3  SLOT_FORMAT_IND: size of DCI format 2_0 is configurable by higher layers up to 128 bits, according to Subclause 11.1.1 of [5, TS 38.213]
    {0,                             0,                             0,                             0,                             0,1,0,0}, // 4  PRE_EMPTION_IND: size of DCI format 2_1 is configurable by higher layers up to 126 bits, according to Subclause 11.2 of [5, TS 38.213]. Each pre-emption indication is 14 bits
    {0,                             0,                             0,                             0,                             0,0,0,0}, // 5  BLOCK_NUMBER: starting position of a block is determined by the parameter startingBitOfFormat2_3
    {0,                             0,                             0,                             0,                             0,0,1,0}, // 6  CLOSE_LOOP_IND
    {
      0,                             (uint8_t)ceil(log2(n_UL_BWP_RRC)),
      0,                             (uint8_t)ceil(log2(n_DL_BWP_RRC)),
      0,0,0,0
    }, // 7  BANDWIDTH_PART_IND:
    {
      0,                             0,                             ((crc_scrambled == _p_rnti) ? 2:0),
      0,                             0,0,0,0
    }, // 8  SHORT_MESSAGE_IND 2 bits if crc scrambled with P-RNTI
    {
      0,                             0,                             ((crc_scrambled == _p_rnti) ? 8:0),
      0,                             0,0,0,0
    }, // 9  SHORT_MESSAGES 8 bit8 if crc scrambled with P-RNTI
    {
      (uint8_t)(ceil(log2(n_RB_ULBWP*(n_RB_ULBWP+1)/2)))-n_UL_hopping,
      n_bits_freq_dom_res_assign_ul,
      0,                             0,                             0,0,0,0
    }, // 10 FREQ_DOM_RESOURCE_ASSIGNMENT_UL: PUSCH hopping with resource allocation type 1 not considered
    //    (NOTE 1) If DCI format 0_0 is monitored in common search space
    //    and if the number of information bits in the DCI format 0_0 prior to padding
    //    is larger than the payload size of the DCI format 1_0 monitored in common search space
    //    the bitwidth of the frequency domain resource allocation field in the DCI format 0_0
    //    is reduced such that the size of DCI format 0_0 equals to the size of the DCI format 1_0
    {
      0,                             0,                             (uint8_t)ceil(log2(n_RB_DLBWP*(n_RB_DLBWP+1)/2)),
      n_bits_freq_dom_res_assign_dl,
      0,0,0,0
    }, // 11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
    {
      4,                             (uint8_t)log2(pusch_alloc_list),
      4,                             (uint8_t)log2(pdsch_alloc_list),
      0,0,0,0
    }, // 12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 6.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
    //    where I the number of entries in the higher layer parameter pusch-AllocationList
    {
      0,                             0,                             1,                             (((dl_res_alloc_type_0==1) &&(dl_res_alloc_type_1==0))?0:1),
      0,0,0,0
    }, // 13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
    {0,                             0,                             0,                             prb_BundlingType_size,         0,0,0,0}, // 14 PRB_BUNDLING_SIZE_IND:0 bit if the higher layer parameter PRB_bundling is not configured or is set to 'static', or 1 bit if the higher layer parameter PRB_bundling is set to 'dynamic' according to Subclause 5.1.2.3 of [6, TS 38.214]
    {0,                             0,                             0,                             rateMatching_bits,             0,0,0,0}, // 15 RATE_MATCHING_IND: 0, 1, or 2 bits according to higher layer parameter rate-match-PDSCH-resource-set
    {0,                             0,                             0,                             n_zp_bits,                     0,0,0,0}, // 16 ZP_CSI_RS_TRIGGER:
    {
      1,                             (((ul_res_alloc_type_0==1) &&(ul_res_alloc_type_1==0))||(freqHopping == 0))?0:1,
      0,                             0,                             0,0,0,0
    }, // 17 FREQ_HOPPING_FLAG: 0 bit if only resource allocation type 0
    {0,                             0,                             0,                             5,                             0,0,0,0}, // 18 TB1_MCS:
    {0,                             0,                             0,                             1,                             0,0,0,0}, // 19 TB1_NDI:
    {0,                             0,                             0,                             2,                             0,0,0,0}, // 20 TB1_RV:
    {0,                             0,                             0,                             5,                             0,0,0,0}, // 21 TB2_MCS:
    {0,                             0,                             0,                             1,                             0,0,0,0}, // 22 TB2_NDI:
    {0,                             0,                             0,                             2,                             0,0,0,0}, // 23 TB2_RV:
    {5,                             5,                             5,                             0,                             0,0,0,0}, // 24 MCS:
    {1,                             1,                             (crc_scrambled == _c_rnti)?1:0,0,                             0,0,0,0}, // 25 NDI:
    {
      2,                             2,                             (((crc_scrambled == _c_rnti) || (crc_scrambled == _si_rnti)) ? 2:0),
      0,                             0,0,0,0
    }, // 26 RV:
    {4,                             4,                             (crc_scrambled == _c_rnti)?4:0,4,                             0,0,0,0}, // 27 HARQ_PROCESS_NUMBER:
    {0,                             0,                             (crc_scrambled == _c_rnti)?2:0,n_dai,                         0,0,0,0}, // 28 DAI: For format1_1: 4 if more than one serving cell are configured in the DL and the higher layer parameter HARQ-ACK-codebook=dynamic, where the 2 MSB bits are the counter DAI and the 2 LSB bits are the total DAI
    //    2 if one serving cell is configured in the DL and the higher layer parameter HARQ-ACK-codebook=dynamic, where the 2 bits are the counter DAI
    //    0 otherwise
    {0,                             codebook_HARQ_ACK,             0,                             0,                             0,0,0,0}, // 29 FIRST_DAI: (1 or 2 bits) 1 bit for semi-static HARQ-ACK // 2 bits for dynamic HARQ-ACK codebook with single HARQ-ACK codebook
    {
      0,                             (((codebook_HARQ_ACK == 2) &&(n_HARQ_ACK_sub_codebooks==2))?2:0),
      0,                             0,                             0,0,0,0
    }, // 30 SECOND_DAI: (0 or 2 bits) 2 bits for dynamic HARQ-ACK codebook with two HARQ-ACK sub-codebooks // 0 bits otherwise
    {
      0,                             0,                             (((crc_scrambled == _p_rnti) || (crc_scrambled == _ra_rnti)) ? 2:0),
      0,                             0,0,0,0
    }, // 31 TB_SCALING
    {2,                             2,                             0,                             0,                             0,0,0,0}, // 32 TPC_PUSCH:
    {0,                             0,                             (crc_scrambled == _c_rnti)?2:0,2,                             0,0,0,0}, // 33 TPC_PUCCH:
    {0,                             0,                             (crc_scrambled == _c_rnti)?3:0,3,                             0,0,0,0}, // 34 PUCCH_RESOURCE_IND:
    {0,                             0,                             (crc_scrambled == _c_rnti)?3:0,pdsch_harq_t_ind,              0,0,0,0}, // 35 PDSCH_TO_HARQ_FEEDBACK_TIME_IND:
    {0,                             (uint8_t)log2(n_SRS),          0,                             0,                             0,0,0,0}, // 36 SRS_RESOURCE_IND:
    {0,                             precond_nbr_layers_bits,       0,                             0,                             0,0,0,0}, // 37 PRECOD_NBR_LAYERS:
    {0,                             antenna_ports_bits_ul,         0,                             antenna_ports_bits_dl,         0,0,0,0}, // 38 ANTENNA_PORTS:
    {0,                             0,                             0,                             tci_bits,                      0,0,0,0}, // 39 TCI: 0 bit if higher layer parameter tci-PresentInDCI is not enabled; otherwise 3 bits
    {0,                             (sul_ind == 0)?2:3,            0,                             (sul_ind == 0)?2:3,            0,0,0,2}, // 40 SRS_REQUEST:
    {
      0,                             0,                             0,                             0,                             0,0,tpc_cmd_bit_2_2,
      tpc_cmd_bit_2_3
    },
    // 41 TPC_CMD:
    {0,                             reportTriggerSize,             0,                             0,                             0,0,0,0}, // 42 CSI_REQUEST:
    {
      0,                             maxCodeBlockGroupsPerTransportBlock,
      0,                             maxCodeBlockGroupsPerTransportBlock_dl,
      0,0,0,0
    }, // 43 CBGTI: 0, 2, 4, 6, or 8 bits determined by higher layer parameter maxCodeBlockGroupsPerTransportBlock for the PDSCH
    {0,                             0,                             0,                             cbgfi_bit,                     0,0,0,0}, // 44 CBGFI: 0 or 1 bit determined by higher layer parameter codeBlockGroupFlushIndicator
    {0,                             ptrs_dmrs_bits,                0,                             0,                             0,0,0,0}, // 45 PTRS_DMRS:
    {0,                             betaOffsets,                   0,                             0,                             0,0,0,0}, // 46 BETA_OFFSET_IND:
    {0,                             dmrs_seq_ini_bits_ul,          0,                             dmrs_seq_ini_bits_dl,          0,0,0,0}, // 47 DMRS_SEQ_INI: 1 bit if the cell has two ULs and the number of bits for DCI format 1_0 before padding
    //    is larger than the number of bits for DCI format 0_0 before padding; 0 bit otherwise
    {0,                             1,                             0,                             0,                             0,0,0,0}, // 48 UL_SCH_IND: value of "1" indicates UL-SCH shall be transmitted on the PUSCH and a value of "0" indicates UL-SCH shall not be transmitted on the PUSCH
    {0,                             0,                             0,                             0,                             0,0,0,0}, // 49 PADDING_NR_DCI:
    //    (NOTE 2) If DCI format 0_0 is monitored in common search space
    //    and if the number of information bits in the DCI format 0_0 prior to padding
    //    is less than the payload size of the DCI format 1_0 monitored in common search space
    //    zeros shall be appended to the DCI format 0_0
    //    until the payload size equals that of the DCI format 1_0
    {(sul_ind == 0)?0:1,            0,                             0,                             0,                             0,0,0,0}, // 50 SUL_IND_0_0:
    {0,                             0,                             0,                             0,                             0,0,0,0}, // 51 RA_PREAMBLE_INDEX (random access procedure initiated by a PDCCH order not implemented, FIXME!!!)
    {0,                             0,                             0,                             0,                             0,0,0,0}, // 52 SUL_IND_1_0 (random access procedure initiated by a PDCCH order not implemented, FIXME!!!)
    {0,                             0,                             0,                             0,                             0,0,0,0}, // 53 SS_PBCH_INDEX (random access procedure initiated by a PDCCH order not implemented, FIXME!!!)
    {0,                             0,                             0,                             0,                             0,0,0,0}, // 54 PRACH_MASK_INDEX (random access procedure initiated by a PDCCH order not implemented, FIXME!!!)
    {
      0,                             0,                             ((crc_scrambled == _p_rnti)?6:(((crc_scrambled == _si_rnti) || (crc_scrambled == _ra_rnti))?16:0)),
      0,                             0,0,0,0
    }  // 55 RESERVED_NR_DCI
  };
  // NOTE 1: adjustments in freq_dom_resource_assignment_UL to be done if necessary
  // NOTE 2: adjustments in padding to be done if necessary
  uint8_t dci_size [8] = {0,0,0,0,0,0,0,0}; // will contain size for each format

  for (int i=0 ; i<NBR_NR_FORMATS ; i++) {
    //#ifdef NR_PDCCH_DCI_DEBUG
    //  LOG_DDD("i=%d, j=%d\n", i, j);
    //#endif
    for (int j=0; j<NBR_NR_DCI_FIELDS; j++) {
      dci_size [i] = dci_size [i] + dci_field_size_table[j][i]; // dci_size[i] contains the size in bits of the dci pdu format i
      //if (i==(int)format-15) {                                  // (int)format-15 indicates the position of each format in the table (e.g. format1_0=17 -> position in table is 2)
      dci_fields_sizes[j][i] = dci_field_size_table[j][i];       // dci_fields_sizes[j] contains the sizes of each field (j) for a determined format i
      //}
    }

    LOG_DDD("(nr_dci_format_size) dci_size[%d]=%d for n_RB_ULBWP=%d\n",
	    i,dci_size[i],n_RB_ULBWP);
  }

  LOG_DDD("(nr_dci_format_size) dci_fields_sizes[][] = { \n");

#ifdef NR_PDCCH_DCI_DEBUG
  for (int j=0; j<NBR_NR_DCI_FIELDS; j++) {
    printf("\t\t");

    for (int i=0; i<NBR_NR_FORMATS ; i++) printf("%d\t",dci_fields_sizes[j][i]);

    printf("\n");
  }

  printf(" }\n");
#endif
  LOG_DNL("(nr_dci_format_size) dci_size[0_0]=%d, dci_size[0_1]=%d, dci_size[1_0]=%d, dci_size[1_1]=%d,\n",dci_size[0],dci_size[1],dci_size[2],dci_size[3]);

  //UL/SUL indicator format0_0 (TS 38.212 subclause 7.3.1.1.1)
  // - 1 bit if the cell has two ULs and the number of bits for DCI format 1_0 before padding is larger than the number of bits for DCI format 0_0 before padding;
  // - 0 bit otherwise.
  // The UL/SUL indicator, if present, locates in the last bit position of DCI format 0_0, after the padding bit(s)
  if ((dci_field_size_table[SUL_IND_0_0][0] == 1) && (dci_size[0] > dci_size[2])) {
    dci_field_size_table[SUL_IND_0_0][0] = 0;
    dci_size[0]=dci_size[0]-1;
  }

  //  if ((format == format0_0) || (format == format1_0)) {
  // According to Section 7.3.1.1.1 in TS 38.212
  // If DCI format 0_0 is monitored in common search space and if the number of information bits in the DCI format 0_0 prior to padding
  // is less than the payload size of the DCI format 1_0 monitored in common search space for scheduling the same serving cell,
  // zeros shall be appended to the DCI format 0_0 until the payload size equals that of the DCI format 1_0.
  if (dci_size[0] < dci_size[2]) { // '0' corresponding to index for format0_0 and '2' corresponding to index of format1_0
    //if (format == format0_0) {
    dci_fields_sizes[PADDING_NR_DCI][0] = dci_size[2] - dci_size[0];
    dci_size[0] = dci_size[2];
    LOG_DDD("(nr_dci_format_size) new dci_size[format0_0]=%d\n",dci_size[0]);
    //}
  }

  // If DCI format 0_0 is monitored in common search space and if the number of information bits in the DCI format 0_0 prior to padding
  // is larger than the payload size of the DCI format 1_0 monitored in common search space for scheduling the same serving cell,
  // the bitwidth of the frequency domain resource allocation field in the DCI format 0_0 is reduced
  // such that the size of DCI format 0_0 equals to the size of the DCI format 1_0..
  if (dci_size[0] > dci_size[2]) {
    //if (format == format0_0) {
    dci_fields_sizes[FREQ_DOM_RESOURCE_ASSIGNMENT_UL][0] -= (dci_size[0] - dci_size[2]);
    dci_size[0] = dci_size[2];
    LOG_DDD("(nr_dci_format_size) new dci_size[format0_0]=%d\n",dci_size[0]);
    //}
  }

  /*
   * TS 38.212 subclause 7.3.1.1.2
   * For a UE configured with SUL in a cell:
   * if PUSCH is configured to be transmitted on both the SUL and the non-SUL of the cell and
   *              if the number of information bits in format 0_1 for the SUL
   * is not equal to the number of information bits in format 0_1 for the non-SUL,
   * zeros shall be appended to smaller format 0_1 until the payload size equals that of the larger format 0_1
   *
   * Not implemented. FIXME!!!
   *
   */
  //  }
  LOG_DDD("(nr_dci_format_size) dci_fields_sizes[][] = { \n");

#ifdef NR_PDCCH_DCI_DEBUG
  for (int j=0; j<NBR_NR_DCI_FIELDS; j++) {
    printf("\t\t");

    for (int i=0; i<NBR_NR_FORMATS ; i++) printf("%d\t",dci_fields_sizes[j][i]);

    printf("\n");
  }

  printf(" }\n");
#endif
  return dci_size[format];
}

#endif

//////////////
/*
 * This code contains all the functions needed to process all dci fields.
 * These tables and functions are going to be called by function nr_ue_process_dci
 */
// table_7_3_1_1_2_2_3_4_5 contains values for number of layers and precoding information for tables 7.3.1.1.2-2/3/4/5 from TS 38.212 subclause 7.3.1.1.2
// the first 6 columns contain table 7.3.1.1.2-2: Precoding information and number of layers, for 4 antenna ports, if transformPrecoder=disabled and maxRank = 2 or 3 or 4
// next six columns contain table 7.3.1.1.2-3: Precoding information and number of layers for 4 antenna ports, if transformPrecoder= enabled, or if transformPrecoder=disabled and maxRank = 1
// next four columns contain table 7.3.1.1.2-4: Precoding information and number of layers, for 2 antenna ports, if transformPrecoder=disabled and maxRank = 2
// next four columns contain table 7.3.1.1.2-5: Precoding information and number of layers, for 2 antenna ports, if transformPrecoder= enabled, or if transformPrecoder= disabled and maxRank = 1
uint8_t table_7_3_1_1_2_2_3_4_5[64][20] = {
  {1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0},
  {1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1},
  {1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  2,  0,  2,  0,  1,  2,  0,  0},
  {1,  3,  1,  3,  1,  3,  1,  3,  1,  3,  1,  3,  1,  2,  0,  0,  1,  3,  0,  0},
  {2,  0,  2,  0,  2,  0,  1,  4,  1,  4,  0,  0,  1,  3,  0,  0,  1,  4,  0,  0},
  {2,  1,  2,  1,  2,  1,  1,  5,  1,  5,  0,  0,  1,  4,  0,  0,  1,  5,  0,  0},
  {2,  2,  2,  2,  2,  2,  1,  6,  1,  6,  0,  0,  1,  5,  0,  0,  0,  0,  0,  0},
  {2,  3,  2,  3,  2,  3,  1,  7,  1,  7,  0,  0,  2,  1,  0,  0,  0,  0,  0,  0},
  {2,  4,  2,  4,  2,  4,  1,  8,  1,  8,  0,  0,  2,  2,  0,  0,  0,  0,  0,  0},
  {2,  5,  2,  5,  2,  5,  1,  9,  1,  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  0,  3,  0,  3,  0,  1,  10, 1,  10, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  0,  4,  0,  4,  0,  1,  11, 1,  11, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  4,  1,  4,  0,  0,  1,  12, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  5,  1,  5,  0,  0,  1,  13, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  6,  1,  6,  0,  0,  1,  14, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  7,  1,  7,  0,  0,  1,  15, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  8,  1,  8,  0,  0,  1,  16, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  9,  1,  9,  0,  0,  1,  17, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  10, 1,  10, 0,  0,  1,  18, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  11, 1,  11, 0,  0,  1,  19, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  6,  2,  6,  0,  0,  1,  20, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  7,  2,  7,  0,  0,  1,  21, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  8,  2,  8,  0,  0,  1,  22, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  9,  2,  9,  0,  0,  1,  23, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  10, 2,  10, 0,  0,  1,  24, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  11, 2,  11, 0,  0,  1,  25, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  12, 2,  12, 0,  0,  1,  26, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  13, 2,  13, 0,  0,  1,  27, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  1,  3,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  2,  3,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  1,  4,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  2,  4,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  12, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  13, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  14, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  15, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  16, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  17, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  18, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  19, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  20, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  21, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  22, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  23, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  24, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  25, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  26, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  27, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  14, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  15, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  16, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  17, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  18, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  19, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  20, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  21, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}
};
uint8_t table_7_3_1_1_2_12[14][3] = {
  {1,0,1},
  {1,1,1},
  {2,0,1},
  {2,1,1},
  {2,2,1},
  {2,3,1},
  {2,0,2},
  {2,1,2},
  {2,2,2},
  {2,3,2},
  {2,4,2},
  {2,5,2},
  {2,6,2},
  {2,7,2}
};
uint8_t table_7_3_1_1_2_13[10][4] = {
  {1,0,1,1},
  {2,0,1,1},
  {2,2,3,1},
  {2,0,2,1},
  {2,0,1,2},
  {2,2,3,2},
  {2,4,5,2},
  {2,6,7,2},
  {2,0,4,2},
  {2,2,6,2}
};
uint8_t table_7_3_1_1_2_14[3][5] = {
  {2,0,1,2,1},
  {2,0,1,4,2},
  {2,2,3,6,2}
};
uint8_t table_7_3_1_1_2_15[4][6] = {
  {2,0,1,2,3,1},
  {2,0,1,4,5,2},
  {2,2,3,6,7,2},
  {2,0,2,4,6,2}
};
uint8_t table_7_3_1_1_2_16[12][2] = {
  {1,0},
  {1,1},
  {2,0},
  {2,1},
  {2,2},
  {2,3},
  {3,0},
  {3,1},
  {3,2},
  {3,3},
  {3,4},
  {3,5}
};
uint8_t table_7_3_1_1_2_17[7][3] = {
  {1,0,1},
  {2,0,1},
  {2,2,3},
  {3,0,1},
  {3,2,3},
  {3,4,5},
  {2,0,2}
};
uint8_t table_7_3_1_1_2_18[3][4] = {
  {2,0,1,2},
  {3,0,1,2},
  {3,3,4,5}
};
uint8_t table_7_3_1_1_2_19[2][5] = {
  {2,0,1,2,3},
  {3,0,1,2,3}
};
uint8_t table_7_3_1_1_2_20[28][3] = {
  {1,0,1},
  {1,1,1},
  {2,0,1},
  {2,1,1},
  {2,2,1},
  {2,3,1},
  {3,0,1},
  {3,1,1},
  {3,2,1},
  {3,3,1},
  {3,4,1},
  {3,5,1},
  {3,0,2},
  {3,1,2},
  {3,2,2},
  {3,3,2},
  {3,4,2},
  {3,5,2},
  {3,6,2},
  {3,7,2},
  {3,8,2},
  {3,9,2},
  {3,10,2},
  {3,11,2},
  {1,0,2},
  {1,1,2},
  {1,6,2},
  {1,7,2}
};
uint8_t table_7_3_1_1_2_21[19][4] = {
  {1,0,1,1},
  {2,0,1,1},
  {2,2,3,1},
  {3,0,1,1},
  {3,2,3,1},
  {3,4,5,1},
  {2,0,2,1},
  {3,0,1,2},
  {3,2,3,2},
  {3,4,5,2},
  {3,6,7,2},
  {3,8,9,2},
  {3,10,11,2},
  {1,0,1,2},
  {1,6,7,2},
  {2,0,1,2},
  {2,2,3,2},
  {2,6,7,2},
  {2,8,9,2}
};
uint8_t table_7_3_1_1_2_22[6][5] = {
  {2,0,1,2,1},
  {3,0,1,2,1},
  {3,3,4,5,1},
  {3,0,1,6,2},
  {3,2,3,8,2},
  {3,4,5,10,2}
};
uint8_t table_7_3_1_1_2_23[5][6] = {
  {2,0,1,2,3,1},
  {3,0,1,2,3,1},
  {3,0,1,6,7,2},
  {3,2,3,8,9,2},
  {3,4,5,10,11,2}
};
uint8_t table_7_3_2_3_3_1[12][5] = {
  {1,0,0,0,0},
  {1,1,0,0,0},
  {1,0,1,0,0},
  {2,0,0,0,0},
  {2,1,0,0,0},
  {2,2,0,0,0},
  {2,3,0,0,0},
  {2,0,1,0,0},
  {2,2,3,0,0},
  {2,0,1,2,0},
  {2,0,1,2,3},
  {2,0,2,0,0}
};
uint8_t table_7_3_2_3_3_2_oneCodeword[31][6] = {
  {1,0,0,0,0,1},
  {1,1,0,0,0,1},
  {1,0,1,0,0,1},
  {2,0,0,0,0,1},
  {2,1,0,0,0,1},
  {2,2,0,0,0,1},
  {2,3,0,0,0,1},
  {2,0,1,0,0,1},
  {2,2,3,0,0,1},
  {2,0,1,2,0,1},
  {2,0,1,2,3,1},
  {2,0,2,0,0,1},
  {2,0,0,0,0,2},
  {2,1,0,0,0,2},
  {2,2,0,0,0,2},
  {2,3,0,0,0,2},
  {2,4,0,0,0,2},
  {2,5,0,0,0,2},
  {2,6,0,0,0,2},
  {2,7,0,0,0,2},
  {2,0,1,0,0,2},
  {2,2,3,0,0,2},
  {2,4,5,0,0,2},
  {2,6,7,0,0,2},
  {2,0,4,0,0,2},
  {2,2,6,0,0,2},
  {2,0,1,4,0,2},
  {2,2,3,6,0,2},
  {2,0,1,4,5,2},
  {2,2,3,6,7,2},
  {2,0,2,4,6,2}
};
uint8_t table_7_3_2_3_3_2_twoCodeword[4][10] = {
  {2,0,1,2,3,4,0,0,0,2},
  {2,0,1,2,3,4,6,0,0,2},
  {2,0,1,2,3,4,5,6,0,2},
  {2,0,1,2,3,4,5,6,7,2}
};
uint8_t table_7_3_2_3_3_3_oneCodeword[24][5] = {
  {1,0,0,0,0},
  {1,1,0,0,0},
  {1,0,1,0,0},
  {2,0,0,0,0},
  {2,1,0,0,0},
  {2,2,0,0,0},
  {2,3,0,0,0},
  {2,0,1,0,0},
  {2,2,3,0,0},
  {2,0,1,2,0},
  {2,0,1,2,3},
  {3,0,0,0,0},
  {3,1,0,0,0},
  {3,2,0,0,0},
  {3,3,0,0,0},
  {3,4,0,0,0},
  {3,5,0,0,0},
  {3,0,1,0,0},
  {3,2,3,0,0},
  {3,4,5,0,0},
  {3,0,1,2,0},
  {3,3,4,5,0},
  {3,0,1,2,3},
  {2,0,2,0,0}
};
uint8_t table_7_3_2_3_3_3_twoCodeword[2][7] = {
  {3,0,1,2,3,4,0},
  {3,0,1,2,3,4,5}
};
uint8_t table_7_3_2_3_3_4_oneCodeword[58][6] = {
  {1,0,0,0,0,1},
  {1,1,0,0,0,1},
  {1,0,1,0,0,1},
  {2,0,0,0,0,1},
  {2,1,0,0,0,1},
  {2,2,0,0,0,1},
  {2,3,0,0,0,1},
  {2,0,1,0,0,1},
  {2,2,3,0,0,1},
  {2,0,1,2,0,1},
  {2,0,1,2,3,1},
  {3,0,0,0,0,1},
  {3,1,0,0,0,1},
  {3,2,0,0,0,1},
  {3,3,0,0,0,1},
  {3,4,0,0,0,1},
  {3,5,0,0,0,1},
  {3,0,1,0,0,1},
  {3,2,3,0,0,1},
  {3,4,5,0,0,1},
  {3,0,1,2,0,1},
  {3,3,4,5,0,1},
  {3,0,1,2,3,1},
  {2,0,2,0,0,1},
  {3,0,0,0,0,2},
  {3,1,0,0,0,2},
  {3,2,0,0,0,2},
  {3,3,0,0,0,2},
  {3,4,0,0,0,2},
  {3,5,0,0,0,2},
  {3,6,0,0,0,2},
  {3,7,0,0,0,2},
  {3,8,0,0,0,2},
  {3,9,0,0,0,2},
  {3,10,0,0,0,2},
  {3,11,0,0,0,2},
  {3,0,1,0,0,2},
  {3,2,3,0,0,2},
  {3,4,5,0,0,2},
  {3,6,7,0,0,2},
  {3,8,9,0,0,2},
  {3,10,11,0,0,2},
  {3,0,1,6,0,2},
  {3,2,3,8,0,2},
  {3,4,5,10,0,2},
  {3,0,1,6,7,2},
  {3,2,3,8,9,2},
  {3,4,5,10,11,2},
  {1,0,0,0,0,2},
  {1,1,0,0,0,2},
  {1,6,0,0,0,2},
  {1,7,0,0,0,2},
  {1,0,1,0,0,2},
  {1,6,7,0,0,2},
  {2,0,1,0,0,2},
  {2,2,3,0,0,2},
  {2,6,7,0,0,2},
  {2,8,9,0,0,2}
};
uint8_t table_7_3_2_3_3_4_twoCodeword[6][10] = {
  {3,0,1,2,3,4,0,0,0,1},
  {3,0,1,2,3,4,5,0,0,1},
  {2,0,1,2,3,6,0,0,0,2},
  {2,0,1,2,3,6,8,0,0,2},
  {2,0,1,2,3,6,7,8,0,2},
  {2,0,1,2,3,6,7,8,9,2}
};
int8_t nr_ue_process_dci_freq_dom_resource_assignment(nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu,
						      fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu,
						      uint16_t n_RB_ULBWP,
						      uint16_t n_RB_DLBWP,
						      uint16_t riv
						      ){

  /*
   * TS 38.214 subclause 5.1.2.2 Resource allocation in frequency domain (downlink)
   * when the scheduling grant is received with DCI format 1_0, then downlink resource allocation type 1 is used
   */
  if(dlsch_config_pdu != NULL){

    /*
     * TS 38.214 subclause 5.1.2.2.1 Downlink resource allocation type 0
     */
    /*
     * TS 38.214 subclause 5.1.2.2.2 Downlink resource allocation type 1
     */
    dlsch_config_pdu->number_rbs = NRRIV2BW(riv,n_RB_DLBWP);
    dlsch_config_pdu->start_rb   = NRRIV2PRBOFFSET(riv,n_RB_DLBWP);

    // Sanity check in case a false or erroneous DCI is received
    if ((dlsch_config_pdu->number_rbs < 1 ) || (dlsch_config_pdu->number_rbs > n_RB_DLBWP - dlsch_config_pdu->start_rb)) {
      // DCI is invalid!
      LOG_W(MAC, "Frequency domain assignment values are invalid! #RBs: %d, Start RB: %d, n_RB_DLBWP: %d \n", dlsch_config_pdu->number_rbs, dlsch_config_pdu->start_rb, n_RB_DLBWP);
      return -1;
    }

  }
  if(pusch_config_pdu != NULL){
    /*
     * TS 38.214 subclause 6.1.2.2 Resource allocation in frequency domain (uplink)
     */
    /*
     * TS 38.214 subclause 6.1.2.2.1 Uplink resource allocation type 0
     */
    /*
     * TS 38.214 subclause 6.1.2.2.2 Uplink resource allocation type 1
     */

    pusch_config_pdu->rb_size  = NRRIV2BW(riv,n_RB_ULBWP);
    pusch_config_pdu->rb_start = NRRIV2PRBOFFSET(riv,n_RB_ULBWP);

    // Sanity check in case a false or erroneous DCI is received
    if ((pusch_config_pdu->rb_size < 1) || (pusch_config_pdu->rb_size > n_RB_ULBWP - pusch_config_pdu->rb_start)) {
      // DCI is invalid!
      LOG_W(MAC, "Frequency domain assignment values are invalid! #RBs: %d, Start RB: %d, n_RB_ULBWP: %d \n",pusch_config_pdu->rb_size, pusch_config_pdu->rb_start, n_RB_ULBWP);
      return -1;
    }

  }
  return 0;
}

int8_t nr_ue_process_dci_time_dom_resource_assignment(NR_UE_MAC_INST_t *mac,
						      nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu,
						      fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu,
						      uint8_t time_domain_ind
						      ){
  int dmrs_typeA_pos = mac->scc->dmrs_TypeA_Position;
//  uint8_t k_offset=0;
  uint8_t sliv_S=0;
  uint8_t sliv_L=0;
  uint8_t table_5_1_2_1_1_2_time_dom_res_alloc_A[16][3]={ // for PDSCH from TS 38.214 subclause 5.1.2.1.1
    {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?12:11}, // row index 1
    {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?10:9},  // row index 2
    {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?9:8},   // row index 3
    {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?7:6},   // row index 4
    {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?5:4},   // row index 5
    {0,(dmrs_typeA_pos == 0)?9:10,(dmrs_typeA_pos == 0)?4:4},   // row index 6
    {0,(dmrs_typeA_pos == 0)?4:6, (dmrs_typeA_pos == 0)?4:4},   // row index 7
    {0,5,7},  // row index 8
    {0,5,2},  // row index 9
    {0,9,2},  // row index 10
    {0,12,2}, // row index 11
    {0,1,13}, // row index 12
    {0,1,6},  // row index 13
    {0,2,4},  // row index 14
    {0,4,7},  // row index 15
    {0,8,4}   // row index 16
  };
  /*uint8_t table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP[16][3]={ // for PDSCH from TS 38.214 subclause 5.1.2.1.1
    {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?6:5},   // row index 1
    {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?10:9},  // row index 2
    {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?9:8},   // row index 3
    {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?7:6},   // row index 4
    {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?5:4},   // row index 5
    {0,(dmrs_typeA_pos == 0)?6:8, (dmrs_typeA_pos == 0)?4:2},   // row index 6
    {0,(dmrs_typeA_pos == 0)?4:6, (dmrs_typeA_pos == 0)?4:4},   // row index 7
    {0,5,6},  // row index 8
    {0,5,2},  // row index 9
    {0,9,2},  // row index 10
    {0,10,2}, // row index 11
    {0,1,11}, // row index 12
    {0,1,6},  // row index 13
    {0,2,4},  // row index 14
    {0,4,6},  // row index 15
    {0,8,4}   // row index 16
    };*/
  /*uint8_t table_5_1_2_1_1_4_time_dom_res_alloc_B[16][3]={ // for PDSCH from TS 38.214 subclause 5.1.2.1.1
    {0,2,2},  // row index 1
    {0,4,2},  // row index 2
    {0,6,2},  // row index 3
    {0,8,2},  // row index 4
    {0,10,2}, // row index 5
    {1,2,2},  // row index 6
    {1,4,2},  // row index 7
    {0,2,4},  // row index 8
    {0,4,4},  // row index 9
    {0,6,4},  // row index 10
    {0,8,4},  // row index 11
    {0,10,4}, // row index 12
    {0,2,7},  // row index 13
    {0,(dmrs_typeA_pos == 0)?2:3,(dmrs_typeA_pos == 0)?12:11},  // row index 14
    {1,2,4},  // row index 15
    {0,0,0}   // row index 16
    };*/
  /*uint8_t table_5_1_2_1_1_5_time_dom_res_alloc_C[16][3]={ // for PDSCH from TS 38.214 subclause 5.1.2.1.1
    {0,2,2},  // row index 1
    {0,4,2},  // row index 2
    {0,6,2},  // row index 3
    {0,8,2},  // row index 4
    {0,10,2}, // row index 5
    {0,0,0},  // row index 6
    {0,0,0},  // row index 7
    {0,2,4},  // row index 8
    {0,4,4},  // row index 9
    {0,6,4},  // row index 10
    {0,8,4},  // row index 11
    {0,10,4}, // row index 12
    {0,2,7},  // row index 13
    {0,(dmrs_typeA_pos == 0)?2:3,(dmrs_typeA_pos == 0)?12:11},  // row index 14
    {0,0,6},  // row index 15
    {0,2,6}   // row index 16
    };*/
  uint8_t mu_pusch = 1;
  // definition table j Table 6.1.2.1.1-4
  uint8_t j = (mu_pusch==3)?3:(mu_pusch==2)?2:1;
  uint8_t table_6_1_2_1_1_2_time_dom_res_alloc_A[16][3]={ // for PUSCH from TS 38.214 subclause 6.1.2.1.1
    {j,  0,14}, // row index 1
    {j,  0,12}, // row index 2
    {j,  0,10}, // row index 3
    {j,  2,10}, // row index 4
    {j,  4,10}, // row index 5
    {j,  4,8},  // row index 6
    {j,  4,6},  // row index 7
    {j+1,0,14}, // row index 8
    {j+1,0,12}, // row index 9
    {j+1,0,10}, // row index 10
    {j+2,0,14}, // row index 11
    {j+2,0,12}, // row index 12
    {j+2,0,10}, // row index 13
    {j,  8,6},  // row index 14
    {j+3,0,14}, // row index 15
    {j+3,0,10}  // row index 16
  };
  /*uint8_t table_6_1_2_1_1_3_time_dom_res_alloc_A_extCP[16][3]={ // for PUSCH from TS 38.214 subclause 6.1.2.1.1
    {j,  0,8},  // row index 1
    {j,  0,12}, // row index 2
    {j,  0,10}, // row index 3
    {j,  2,10}, // row index 4
    {j,  4,4},  // row index 5
    {j,  4,8},  // row index 6
    {j,  4,6},  // row index 7
    {j+1,0,8},  // row index 8
    {j+1,0,12}, // row index 9
    {j+1,0,10}, // row index 10
    {j+2,0,6},  // row index 11
    {j+2,0,12}, // row index 12
    {j+2,0,10}, // row index 13
    {j,  8,4},  // row index 14
    {j+3,0,8},  // row index 15
    {j+3,0,10}  // row index 16
    };*/

  /*
   * TS 38.214 subclause 5.1.2.1 Resource allocation in time domain (downlink)
   */
  if(dlsch_config_pdu != NULL){
    NR_PDSCH_TimeDomainResourceAllocationList_t *pdsch_TimeDomainAllocationList = NULL;
    if (mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->pdsch_TimeDomainAllocationList)
      pdsch_TimeDomainAllocationList = mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->pdsch_TimeDomainAllocationList->choice.setup;
    else if (mac->DLbwp[0]->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList)
      pdsch_TimeDomainAllocationList = mac->DLbwp[0]->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
    if (pdsch_TimeDomainAllocationList) {

      if (time_domain_ind >= pdsch_TimeDomainAllocationList->list.count) {
        LOG_E(MAC, "time_domain_ind %d >= pdsch->TimeDomainAllocationList->list.count %d\n",
              time_domain_ind, pdsch_TimeDomainAllocationList->list.count);
        dlsch_config_pdu->start_symbol   = 0;
        dlsch_config_pdu->number_symbols = 0;
        return -1;
      }

      int startSymbolAndLength = pdsch_TimeDomainAllocationList->list.array[time_domain_ind]->startSymbolAndLength;
      int S,L;
      SLIV2SL(startSymbolAndLength,&S,&L);
      dlsch_config_pdu->start_symbol=S;
      dlsch_config_pdu->number_symbols=L;
    }
    else {// Default configuration from tables
//      k_offset = table_5_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind-1][0];
      sliv_S   = table_5_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind-1][1];
      sliv_L   = table_5_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind-1][2];
      // k_offset = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][0];
      // sliv_S   = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][1];
      // sliv_L   = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][2];
      // k_offset = table_5_1_2_1_1_4_time_dom_res_alloc_B[nr_pdci_info_extracted->time_dom_resource_assignment][0];
      // sliv_S   = table_5_1_2_1_1_4_time_dom_res_alloc_B[nr_pdci_info_extracted->time_dom_resource_assignment][1];
      // sliv_L   = table_5_1_2_1_1_4_time_dom_res_alloc_B[nr_pdci_info_extracted->time_dom_resource_assignment][2];
      // k_offset = table_5_1_2_1_1_5_time_dom_res_alloc_C[nr_pdci_info_extracted->time_dom_resource_assignment][0];
      // sliv_S   = table_5_1_2_1_1_5_time_dom_res_alloc_C[nr_pdci_info_extracted->time_dom_resource_assignment][1];
      // sliv_L   = table_5_1_2_1_1_5_time_dom_res_alloc_C[nr_pdci_info_extracted->time_dom_resource_assignment][2];
      dlsch_config_pdu->number_symbols = sliv_L;
      dlsch_config_pdu->start_symbol = sliv_S;
    }
  }	/*
	 * TS 38.214 subclause 6.1.2.1 Resource allocation in time domain (uplink)
	 */
  if(pusch_config_pdu != NULL){
    NR_PUSCH_TimeDomainResourceAllocationList_t *pusch_TimeDomainAllocationList = NULL;
    if (mac->ULbwp[0]->bwp_Dedicated->pusch_Config->choice.setup->pusch_TimeDomainAllocationList) {
      pusch_TimeDomainAllocationList = mac->ULbwp[0]->bwp_Dedicated->pusch_Config->choice.setup->pusch_TimeDomainAllocationList->choice.setup;
    }
    else if (mac->ULbwp[0]->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList) {
      pusch_TimeDomainAllocationList = mac->ULbwp[0]->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
    }
    	
    if (pusch_TimeDomainAllocationList) {
      if (time_domain_ind >= pusch_TimeDomainAllocationList->list.count) {
        LOG_E(MAC, "time_domain_ind %d >= pusch->TimeDomainAllocationList->list.count %d\n",
              time_domain_ind, pusch_TimeDomainAllocationList->list.count);
        pusch_config_pdu->start_symbol_index=0;
        pusch_config_pdu->nr_of_symbols=0;
        return -1;
      }
      
      int startSymbolAndLength = pusch_TimeDomainAllocationList->list.array[time_domain_ind]->startSymbolAndLength;
      int S,L;
      SLIV2SL(startSymbolAndLength,&S,&L);
      pusch_config_pdu->start_symbol_index=S;
      pusch_config_pdu->nr_of_symbols=L;
    }
    else {
//      k_offset = table_6_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind-1][0];
      sliv_S   = table_6_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind-1][1];
      sliv_L   = table_6_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind-1][2];
      // k_offset = table_6_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][0];
      // sliv_S   = table_6_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][1];
      // sliv_L   = table_6_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][2];
      pusch_config_pdu->nr_of_symbols = sliv_L;
      pusch_config_pdu->start_symbol_index = sliv_S;
    }
  }
  return 0;
}
//////////////
int nr_ue_process_dci_indication_pdu(module_id_t module_id,int cc_id, int gNB_index, frame_t frame, int slot, fapi_nr_dci_indication_pdu_t *dci) {

  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

  LOG_D(MAC,"Received dci indication (rnti %x,dci format %d,n_CCE %d,payloadSize %d,payload %llx)\n",
	dci->rnti,dci->dci_format,dci->n_CCE,dci->payloadSize,*(unsigned long long*)dci->payloadBits);

  int dci_format = nr_extract_dci_info(mac,dci->dci_format,dci->payloadSize,dci->rnti,(uint64_t *)dci->payloadBits,def_dci_pdu_rel15);
  return (nr_ue_process_dci(module_id, cc_id, gNB_index, frame, slot, def_dci_pdu_rel15, dci->rnti, dci_format));
}

int8_t nr_ue_process_dci(module_id_t module_id, int cc_id, uint8_t gNB_index, frame_t frame, int slot, dci_pdu_rel15_t *dci, uint16_t rnti, uint32_t dci_format){

  int bwp_id = 1;
  int mu = 0;
  long k2 = 0;
  int pucch_res_set_cnt = 0, valid = 0;
  uint16_t frame_tx = 0, slot_tx = 0;
  bool valid_ptrs_setup = 0;
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  fapi_nr_dl_config_request_t *dl_config = &mac->dl_config_request;
  fapi_nr_ul_config_request_t *ul_config = NULL;
    
  //const uint16_t n_RB_DLBWP = dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.N_RB_BWP; //make sure this has been set
  AssertFatal(mac->DLbwp[0]!=NULL,"DLbwp[0] should not be zero here!\n");
  AssertFatal(mac->ULbwp[0]!=NULL,"DLbwp[0] should not be zero here!\n");

  const uint16_t n_RB_DLBWP = (mac->ra_state == WAIT_RAR) ? NRRIV2BW(mac->scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, 275) : NRRIV2BW(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth,275);
  const uint16_t n_RB_ULBWP = NRRIV2BW(mac->ULbwp[0]->bwp_Common->genericParameters.locationAndBandwidth,275);

  LOG_D(MAC,"nr_ue_process_dci at MAC layer with dci_format=%d (DL BWP %d, UL BWP %d)\n",dci_format,n_RB_DLBWP,n_RB_ULBWP);

  NR_PDSCH_Config_t *pdsch_config=mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup;
  NR_PUSCH_Config_t *pusch_config=mac->ULbwp[0]->bwp_Dedicated->pusch_Config->choice.setup;

  switch(dci_format){
  case NR_UL_DCI_FORMAT_0_0:
    /*
     *  with CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI
     *    0  IDENTIFIER_DCI_FORMATS:
     *    10 FREQ_DOM_RESOURCE_ASSIGNMENT_UL: PUSCH hopping with resource allocation type 1 not considered
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 6.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    17 FREQ_HOPPING_FLAG: 0 bit if only resource allocation type 0
     *    24 MCS:
     *    25 NDI:
     *    26 RV:
     *    27 HARQ_PROCESS_NUMBER:
     *    32 TPC_PUSCH:
     *    49 PADDING_NR_DCI: (Note 2) If DCI format 0_0 is monitored in common search space
     *    50 SUL_IND_0_0:
     */
    // Calculate the slot in which ULSCH should be scheduled. This is current slot + K2,
    // where K2 is the offset between the slot in which UL DCI is received and the slot
    // in which ULSCH should be scheduled. K2 is configured in RRC configuration.  
    
    // Get the numerology to calculate the Tx frame and slot
    mu = mac->ULbwp[0]->bwp_Common->genericParameters.subcarrierSpacing;
    // Get slot offset K2 which will be used to calculate TX slot
    k2 = get_k2(mac, dci->time_domain_assignment.val);
    if (k2 < 0)           // This can happen when a false DCI is received
      return -1;
    // Calculate TX slot and frame
    slot_tx = (slot + k2) % nr_slots_per_frame[mu];
    frame_tx = ((slot + k2) > nr_slots_per_frame[mu]) ? (frame + 1) % 1024 : frame;
    
    // Get UL config request corresponding slot_tx 
    ul_config = get_ul_config_request(mac, slot_tx);
    //AssertFatal(ul_config != NULL, "nr_ue_process_dci(): ul_config is NULL\n");
    if (!ul_config) {
      LOG_W(MAC, "nr_ue_process_dci(): ul_config request is NULL. Probably due to unexpected UL DCI in frame.slot %d.%d. Ignoring DCI!\n",frame,slot);
      return -1;
    }

    ul_config->ul_config_list[ul_config->number_pdus].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUSCH;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.rnti = rnti;
    nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu_0_0 = &ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu;
    /* IDENTIFIER_DCI_FORMATS */
    /* FREQ_DOM_RESOURCE_ASSIGNMENT_UL */
    if (nr_ue_process_dci_freq_dom_resource_assignment(pusch_config_pdu_0_0,NULL,n_RB_ULBWP,0,dci->frequency_domain_assignment.val) < 0)
      return -1;
    /* TIME_DOM_RESOURCE_ASSIGNMENT */
    if (nr_ue_process_dci_time_dom_resource_assignment(mac,pusch_config_pdu_0_0,NULL,dci->time_domain_assignment.val) < 0)
      return -1;
    /* FREQ_HOPPING_FLAG */
    if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.resource_allocation != 0) &&
	(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.frequency_hopping !=0))
      pusch_config_pdu_0_0->frequency_hopping = dci->frequency_hopping_flag.val;

    /* MCS */
    pusch_config_pdu_0_0->mcs_index = dci->mcs;

    /* MCS TABLE */
    if (mac->scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder == NULL)
      pusch_config_pdu_0_0->transform_precoding = 1;
    else
      pusch_config_pdu_0_0->transform_precoding = 0;
      
    if (pusch_config_pdu_0_0->transform_precoding == transform_precoder_disabled) 
      pusch_config_pdu_0_0->mcs_table = get_pusch_mcs_table(pusch_config->mcs_Table, 0,
                                                 dci_format, NR_RNTI_TC, NR_SearchSpace__searchSpaceType_PR_common, false);
    else
      pusch_config_pdu_0_0->mcs_table = get_pusch_mcs_table(pusch_config->mcs_TableTransformPrecoder, 1,
                                                 dci_format, NR_RNTI_TC, NR_SearchSpace__searchSpaceType_PR_common, false);
    
    pusch_config_pdu_0_0->target_code_rate = nr_get_code_rate_ul(pusch_config_pdu_0_0->mcs_index, pusch_config_pdu_0_0->mcs_table);
    pusch_config_pdu_0_0->qam_mod_order = nr_get_Qm_ul(pusch_config_pdu_0_0->mcs_index, pusch_config_pdu_0_0->mcs_table);
    if (pusch_config_pdu_0_0->target_code_rate == 0 || pusch_config_pdu_0_0->qam_mod_order == 0) {
      LOG_W(MAC, "Invalid code rate or Mod order, likely due to unexpected UL DCI. Ignoring DCI! \n");
      return -1;
    }

    /* NDI */
    pusch_config_pdu_0_0->pusch_data.new_data_indicator = dci->ndi;
    /* RV */
    pusch_config_pdu_0_0->pusch_data.rv_index = dci->rv;
    /* HARQ_PROCESS_NUMBER */
    pusch_config_pdu_0_0->pusch_data.harq_process_id = dci->harq_pid;
    /* TPC_PUSCH */
    // according to TS 38.213 Table Table 7.1.1-1
    if (dci->tpc == 0) {
      pusch_config_pdu_0_0->absolute_delta_PUSCH = -4;
    }
    if (dci->tpc == 1) {
      pusch_config_pdu_0_0->absolute_delta_PUSCH = -1;
    }
    if (dci->tpc == 2) {
      pusch_config_pdu_0_0->absolute_delta_PUSCH = 1;
    }
    if (dci->tpc == 3) {
      pusch_config_pdu_0_0->absolute_delta_PUSCH = 4;
    }
    /* SUL_IND_0_0 */ // To be implemented, FIXME!!!

    ul_config->slot = slot_tx;
    ul_config->sfn = frame_tx;
    ul_config->number_pdus = ul_config->number_pdus + 1;
    LOG_D(MAC, "nr_ue_process_dci(): Calculated frame and slot for pusch Tx: %d.%d, number of pdus %d\n", ul_config->sfn, ul_config->slot, ul_config->number_pdus);
    break;

  case NR_UL_DCI_FORMAT_0_1:
    /*
     *  with CRC scrambled by C-RNTI or CS-RNTI or SP-CSI-RNTI or new-RNTI
     *    0  IDENTIFIER_DCI_FORMATS:
     *    1  CARRIER_IND
     *    2  SUL_IND_0_1
     *    7  BANDWIDTH_PART_IND
     *    10 FREQ_DOM_RESOURCE_ASSIGNMENT_UL: PUSCH hopping with resource allocation type 1 not considered
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 6.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    17 FREQ_HOPPING_FLAG: 0 bit if only resource allocation type 0
     *    24 MCS:
     *    25 NDI:
     *    26 RV:
     *    27 HARQ_PROCESS_NUMBER:
     *    29 FIRST_DAI
     *    30 SECOND_DAI
     *    32 TPC_PUSCH:
     *    36 SRS_RESOURCE_IND:
     *    37 PRECOD_NBR_LAYERS:
     *    38 ANTENNA_PORTS:
     *    40 SRS_REQUEST:
     *    42 CSI_REQUEST:
     *    43 CBGTI
     *    45 PTRS_DMRS
     *    46 BETA_OFFSET_IND
     *    47 DMRS_SEQ_INI
     *    48 UL_SCH_IND
     *    49 PADDING_NR_DCI: (Note 2) If DCI format 0_0 is monitored in common search space
     */
    // Calculate the slot in which ULSCH should be scheduled. This is current slot + K2,
    // where K2 is the offset between the slot in which UL DCI is received and the slot
    // in which ULSCH should be scheduled. K2 is configured in RRC configuration.  
    
    // Get the numerology to calculate the Tx frame and slot
    mu = mac->ULbwp[0]->bwp_Common->genericParameters.subcarrierSpacing;
    // Get slot offset K2 which will be used to calculate TX slot
    k2 = get_k2(mac, dci->time_domain_assignment.val);
    if (k2 < 0)           // This can happen when a false DCI is received
      return -1;
    // Calculate TX slot and frame
    slot_tx = (slot + k2) % nr_slots_per_frame[mu];
    frame_tx = ((slot + k2) > nr_slots_per_frame[mu]) ? (frame + 1) % 1024 : frame;
    
    // Get UL config request corresponding slot_tx 
    ul_config = get_ul_config_request(mac, slot_tx);
    //AssertFatal(ul_config != NULL, "nr_ue_process_dci(): ul_config is NULL\n");
    if (!ul_config) {
      LOG_W(MAC, "nr_ue_process_dci(): ul_config request is NULL. Probably due to unexpected UL DCI in frame.slot %d.%d. Ignoring DCI!\n",frame,slot);
      return -1;
    }

    ul_config->ul_config_list[ul_config->number_pdus].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUSCH;
    ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu.rnti = rnti;
    nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu_0_1 = &ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu;
    /* IDENTIFIER_DCI_FORMATS */
    /* CARRIER_IND */
    /* SUL_IND_0_1 */
    /* BANDWIDTH_PART_IND */
    //pusch_config_pdu_0_1->bandwidth_part_ind = dci->bwp_indicator.val;
    /* FREQ_DOM_RESOURCE_ASSIGNMENT_UL */
    if (nr_ue_process_dci_freq_dom_resource_assignment(pusch_config_pdu_0_1,NULL,n_RB_ULBWP,0,dci->frequency_domain_assignment.val) < 0)
      return -1;
    /* TIME_DOM_RESOURCE_ASSIGNMENT */
    if (nr_ue_process_dci_time_dom_resource_assignment(mac,pusch_config_pdu_0_1,NULL,dci->time_domain_assignment.val) < 0)
      return -1;
    /* FREQ_HOPPING_FLAG */
    if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.resource_allocation != 0) &&
	(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.frequency_hopping !=0))
      pusch_config_pdu_0_1->frequency_hopping = dci->frequency_hopping_flag.val;
    /* MCS */
    pusch_config_pdu_0_1->mcs_index = dci->mcs;
    /* MCS TABLE */
    if (pusch_config->transformPrecoder == NULL) {
      if (mac->scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder == NULL)
        pusch_config_pdu_0_1->transform_precoding = 1;
      else
        pusch_config_pdu_0_1->transform_precoding = 0;
    }
    else
      pusch_config_pdu_0_1->transform_precoding = *pusch_config->transformPrecoder;
      
    if (pusch_config_pdu_0_1->transform_precoding == transform_precoder_disabled) 
      pusch_config_pdu_0_1->mcs_table = get_pusch_mcs_table(pusch_config->mcs_Table, 0,
                                                 dci_format, NR_RNTI_C, NR_SearchSpace__searchSpaceType_PR_ue_Specific, false);
    else
      pusch_config_pdu_0_1->mcs_table = get_pusch_mcs_table(pusch_config->mcs_TableTransformPrecoder, 1,
                                                 dci_format, NR_RNTI_C, NR_SearchSpace__searchSpaceType_PR_ue_Specific, false);
    
    pusch_config_pdu_0_1->target_code_rate = nr_get_code_rate_ul(pusch_config_pdu_0_1->mcs_index, pusch_config_pdu_0_1->mcs_table);
    pusch_config_pdu_0_1->qam_mod_order = nr_get_Qm_ul(pusch_config_pdu_0_1->mcs_index, pusch_config_pdu_0_1->mcs_table);
    if (pusch_config_pdu_0_1->target_code_rate == 0 || pusch_config_pdu_0_1->qam_mod_order == 0) {
      LOG_W(MAC, "Invalid code rate or Mod order, likely due to unexpected UL DCI. Ignoring DCI! \n");
      return -1;
    }

    /* NDI */
    pusch_config_pdu_0_1->pusch_data.new_data_indicator = dci->ndi;
    /* RV */
    pusch_config_pdu_0_1->pusch_data.rv_index = dci->rv;
    /* HARQ_PROCESS_NUMBER */
    pusch_config_pdu_0_1->pusch_data.harq_process_id = dci->harq_pid;
    /* FIRST_DAI */ //To be implemented, FIXME!!!
    /* SECOND_DAI */ //To be implemented, FIXME!!!
    /* TPC_PUSCH */
    // according to TS 38.213 Table Table 7.1.1-1
    if (dci->tpc == 0) {
      pusch_config_pdu_0_1->absolute_delta_PUSCH = -4;
    }
    if (dci->tpc == 1) {
      pusch_config_pdu_0_1->absolute_delta_PUSCH = -1;
    }
    if (dci->tpc == 2) {
      pusch_config_pdu_0_1->absolute_delta_PUSCH = 1;
    }
    if (dci->tpc == 3) {
      pusch_config_pdu_0_1->absolute_delta_PUSCH = 4;
    }
    /* SRS_RESOURCE_IND */
    //FIXME!!
    /* PRECOD_NBR_LAYERS */
    if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.tx_config == tx_config_nonCodebook));
    // 0 bits if the higher layer parameter txConfig = nonCodeBook
    if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.tx_config == tx_config_codebook)){
      uint8_t n_antenna_port = 0; //FIXME!!!
      if (n_antenna_port == 1); // 1 antenna port and the higher layer parameter txConfig = codebook 0 bits
      if (n_antenna_port == 4){ // 4 antenna port and the higher layer parameter txConfig = codebook
	// Table 7.3.1.1.2-2: transformPrecoder=disabled and maxRank = 2 or 3 or 4
	if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled)
	    && ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.max_rank == 2) ||
		(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.max_rank == 3) ||
		(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.max_rank == 4))){
	  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_fullyAndPartialAndNonCoherent) {
	    pusch_config_pdu_0_1->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][0];
	    pusch_config_pdu_0_1->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][1];
	  }
	  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_partialAndNonCoherent){
	    pusch_config_pdu_0_1->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][2];
	    pusch_config_pdu_0_1->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][3];
	  }
	  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_nonCoherent){
	    pusch_config_pdu_0_1->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][4];
	    pusch_config_pdu_0_1->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][5];
	  }
	}
	// Table 7.3.1.1.2-3: transformPrecoder= enabled, or transformPrecoder=disabled and maxRank = 1
	if (((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_enabled)
	     || (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled))
	    && (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.max_rank == 1)){
	  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_fullyAndPartialAndNonCoherent) {
	    pusch_config_pdu_0_1->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][6];
	    pusch_config_pdu_0_1->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][7];
	  }
	  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_partialAndNonCoherent){
	    pusch_config_pdu_0_1->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][8];
	    pusch_config_pdu_0_1->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][9];
	  }
	  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_nonCoherent){
	    pusch_config_pdu_0_1->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][10];
	    pusch_config_pdu_0_1->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][11];
	  }
	}
      }
      if (n_antenna_port == 4){ // 2 antenna port and the higher layer parameter txConfig = codebook
	// Table 7.3.1.1.2-4: transformPrecoder=disabled and maxRank = 2
	if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled)
	    && (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.max_rank == 2)){
	  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_fullyAndPartialAndNonCoherent) {
	    pusch_config_pdu_0_1->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][12];
	    pusch_config_pdu_0_1->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][13];
	  }
	  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_nonCoherent){
	    pusch_config_pdu_0_1->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][14];
	    pusch_config_pdu_0_1->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][15];
	  }
	}
	// Table 7.3.1.1.2-5: transformPrecoder= enabled, or transformPrecoder= disabled and maxRank = 1
	if (((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_enabled)
	     || (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled))
	    && (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.max_rank == 1)){
	  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_fullyAndPartialAndNonCoherent) {
	    pusch_config_pdu_0_1->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][16];
	    pusch_config_pdu_0_1->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][17];
	  }
	  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_nonCoherent){
	    pusch_config_pdu_0_1->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][18];
	    pusch_config_pdu_0_1->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][19];
	  }
	}
      }
    }
    /* ANTENNA_PORTS */
    uint8_t rank=0; // We need to initialize rank FIXME!!!
    if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_enabled) &&
	(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 1) &&
	(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.max_length == 1)) { // tables 7.3.1.1.2-6
      pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = 2; //TBC
      pusch_config_pdu_0_1->dmrs_ports = dci->antenna_ports.val; //TBC
    }
    if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_enabled) &&
	(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 1) &&
	(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.max_length == 2)) { // tables 7.3.1.1.2-7
      pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = 2; //TBC
      pusch_config_pdu_0_1->dmrs_ports = (dci->antenna_ports.val > 3)?(dci->antenna_ports.val-4):(dci->antenna_ports.val); //TBC
      //pusch_config_pdu_0_1->n_front_load_symb = (dci->antenna_ports > 3)?2:1; //FIXME
    }
    if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled) &&
	(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 1) &&
	(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.max_length == 1)) { // tables 7.3.1.1.2-8/9/10/11
      if (rank == 1){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = (dci->antenna_ports.val > 1)?2:1; //TBC
	pusch_config_pdu_0_1->dmrs_ports = (dci->antenna_ports.val > 1)?(dci->antenna_ports.val-2):(dci->antenna_ports.val); //TBC
      }
      if (rank == 2){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = (dci->antenna_ports.val > 0)?2:1; //TBC
	pusch_config_pdu_0_1->dmrs_ports = 0; //FIXME
	//pusch_config_pdu_0_1->dmrs_ports[0] = (dci->antenna_ports > 1)?(dci->antenna_ports > 2 ?0:2):0;
	//pusch_config_pdu_0_1->dmrs_ports[1] = (dci->antenna_ports > 1)?(dci->antenna_ports > 2 ?2:3):1;
      }
      if (rank == 3){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = 2; //TBC
	pusch_config_pdu_0_1->dmrs_ports = 0; //FIXME
	//pusch_config_pdu_0_1->dmrs_ports[0] = 0;
	//pusch_config_pdu_0_1->dmrs_ports[1] = 1;
	//pusch_config_pdu_0_1->dmrs_ports[2] = 2;
      }
      if (rank == 4){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = 2; //TBC
	pusch_config_pdu_0_1->dmrs_ports = 0; //FIXME
	//pusch_config_pdu_0_1->dmrs_ports[0] = 0;
	//pusch_config_pdu_0_1->dmrs_ports[1] = 1;
	//pusch_config_pdu_0_1->dmrs_ports[2] = 2;
	//pusch_config_pdu_0_1->dmrs_ports[3] = 3;
      }
    }
    if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled) &&
	(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 1) &&
	(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.max_length == 2)) { // tables 7.3.1.1.2-12/13/14/15
      if (rank == 1){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = (dci->antenna_ports.val > 1)?2:1; //TBC
	pusch_config_pdu_0_1->dmrs_ports = (dci->antenna_ports.val > 1)?(dci->antenna_ports.val > 5 ?(dci->antenna_ports.val-6):(dci->antenna_ports.val-2)):dci->antenna_ports.val; //TBC
	//pusch_config_pdu_0_1->n_front_load_symb = (dci->antenna_ports.val > 6)?2:1; //FIXME
      }
      if (rank == 2){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = (dci->antenna_ports.val > 0)?2:1; //TBC
	pusch_config_pdu_0_1->dmrs_ports = 0; //FIXME
	//pusch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_13[dci->antenna_ports.val][1];
	//pusch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_13[dci->antenna_ports.val][2];
	//pusch_config_pdu_0_1->n_front_load_symb = (dci->antenna_ports.val > 3)?2:1; // FIXME
      }
      if (rank == 3){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = 2; //TBC
	pusch_config_pdu_0_1->dmrs_ports = 0; //FIXME
	//pusch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_14[dci->antenna_ports.val][1];
	//pusch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_14[dci->antenna_ports.val][2];
	//pusch_config_pdu_0_1->dmrs_ports[2] = table_7_3_1_1_2_14[dci->antenna_ports.val][3];
	//pusch_config_pdu_0_1->n_front_load_symb = (dci->antenna_ports.val > 1)?2:1; //FIXME
      }
      if (rank == 4){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = 2; //TBC
	pusch_config_pdu_0_1->dmrs_ports = 0; //FIXME
	//pusch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_15[dci->antenna_ports.val][1];
	//pusch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_15[dci->antenna_ports.val][2];
	//pusch_config_pdu_0_1->dmrs_ports[2] = table_7_3_1_1_2_15[dci->antenna_ports.val][3];
	//pusch_config_pdu_0_1->dmrs_ports[3] = table_7_3_1_1_2_15[dci->antenna_ports.val][4];
	//pusch_config_pdu_0_1->n_front_load_symb = (dci->antenna_ports.val > 1)?2:1; //FIXME
      }
    }
    if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled) &&
	(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 2) &&
	(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.max_length == 1)) { // tables 7.3.1.1.2-16/17/18/19
      if (rank == 1){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = (dci->antenna_ports.val > 1)?((dci->antenna_ports.val > 5)?3:2):1; //TBC
	pusch_config_pdu_0_1->dmrs_ports = (dci->antenna_ports.val > 1)?(dci->antenna_ports.val > 5 ?(dci->antenna_ports.val-6):(dci->antenna_ports.val-2)):dci->antenna_ports.val; //TBC
      }
      if (rank == 2){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = (dci->antenna_ports.val > 0)?((dci->antenna_ports.val > 2)?3:2):1; //TBC
	pusch_config_pdu_0_1->dmrs_ports = 0; //FIXME
	//pusch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_17[dci->antenna_ports.val][1];
	//pusch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_17[dci->antenna_ports.val][2];
      }
      if (rank == 3){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = (dci->antenna_ports.val > 0)?3:2; //TBC
	pusch_config_pdu_0_1->dmrs_ports = 0; //FIXME
	//pusch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_18[dci->antenna_ports.val][1];
	//pusch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_18[dci->antenna_ports.val][2];
	//pusch_config_pdu_0_1->dmrs_ports[2] = table_7_3_1_1_2_18[dci->antenna_ports.val][3];
      }
      if (rank == 4){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = dci->antenna_ports.val + 2; //TBC
	pusch_config_pdu_0_1->dmrs_ports = 0; //FIXME
	//pusch_config_pdu_0_1->dmrs_ports[0] = 0;
	//pusch_config_pdu_0_1->dmrs_ports[1] = 1;
	//pusch_config_pdu_0_1->dmrs_ports[2] = 2;
	//pusch_config_pdu_0_1->dmrs_ports[3] = 3;
      }
    }
    if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled) &&
	(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 2) &&
	(mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.max_length == 2)) { // tables 7.3.1.1.2-20/21/22/23
      if (rank == 1){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = table_7_3_1_1_2_20[dci->antenna_ports.val][0]; //TBC
	pusch_config_pdu_0_1->dmrs_ports = table_7_3_1_1_2_20[dci->antenna_ports.val][1]; //TBC
	//pusch_config_pdu_0_1->n_front_load_symb = table_7_3_1_1_2_20[dci->antenna_ports.val][2]; //FIXME
      }
      if (rank == 2){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = table_7_3_1_1_2_21[dci->antenna_ports.val][0]; //TBC
	pusch_config_pdu_0_1->dmrs_ports = 0; //FIXME
	//pusch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_21[dci->antenna_ports.val][1];
	//pusch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_21[dci->antenna_ports.val][2];
	//pusch_config_pdu_0_1->n_front_load_symb = table_7_3_1_1_2_21[dci->antenna_ports.val][3]; //FIXME
      }
      if (rank == 3){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = table_7_3_1_1_2_22[dci->antenna_ports.val][0]; //TBC
	pusch_config_pdu_0_1->dmrs_ports = 0; //FIXME
	//pusch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_22[dci->antenna_ports.val][1];
	//pusch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_22[dci->antenna_ports.val][2];
	//pusch_config_pdu_0_1->dmrs_ports[2] = table_7_3_1_1_2_22[dci->antenna_ports.val][3];
	//pusch_config_pdu_0_1->n_front_load_symb = table_7_3_1_1_2_22[dci->antenna_ports.val][4]; //FIXME
      }
      if (rank == 4){
	pusch_config_pdu_0_1->num_dmrs_cdm_grps_no_data = table_7_3_1_1_2_23[dci->antenna_ports.val][0]; //TBC
	pusch_config_pdu_0_1->dmrs_ports = 0; //FIXME
	//pusch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_23[dci->antenna_ports.val][1];
	//pusch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_23[dci->antenna_ports.val][2];
	//pusch_config_pdu_0_1->dmrs_ports[2] = table_7_3_1_1_2_23[dci->antenna_ports.val][3];
	//pusch_config_pdu_0_1->dmrs_ports[3] = table_7_3_1_1_2_23[dci->antenna_ports.val][4];
	//pusch_config_pdu_0_1->n_front_load_symb = table_7_3_1_1_2_23[dci->antenna_ports.val][5]; //FIXME
      }
    }
    /* SRS_REQUEST */
    // if SUL is supported in the cell, there is an additional bit in thsi field and the value of this bit represents table 7.1.1.1-1 TS 38.212 FIXME!!!
    //pusch_config_pdu_0_1->srs_config.aperiodicSRS_ResourceTrigger = (dci->srs_request.val & 0x11); // as per Table 7.3.1.1.2-24 TS 38.212 //FIXME
    /* CSI_REQUEST */
    //pusch_config_pdu_0_1->csi_reportTriggerSize = dci->csi_request.val; //FIXME
    /* CBGTI */
    //pusch_config_pdu_0_1->maxCodeBlockGroupsPerTransportBlock = dci->cbgti.val; //FIXME
    /* PTRS_DMRS */
    if (((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled) &&
	 (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.ptrs_uplink_config == 0)) ||
	((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_enabled) &&
	 (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.max_rank == 1))){
    } else {
      //pusch_config_pdu_0_1->ptrs_dmrs_association_port = dci->ptrs_dmrs_association.val; //FIXME
    }
    /* BETA_OFFSET_IND */
    // Table 9.3-3 in [5, TS 38.213]
    //pusch_config_pdu_0_1->beta_offset_ind = dci->beta_offset_indicator.val; //FIXME
    /* DMRS_SEQ_INI */
    // FIXME!!
    /* UL_SCH_IND */
    // A value of "1" indicates UL-SCH shall be transmitted on the PUSCH and
    // a value of "0" indicates UL-SCH shall not be transmitted on the PUSCH
    
    ul_config->slot = slot_tx;
    ul_config->sfn = frame_tx;
    ul_config->number_pdus = ul_config->number_pdus + 1;
    LOG_D(MAC, "nr_ue_process_dci(): Calculated frame and slot for pusch Tx: %d.%d, number of pdus %d\n", ul_config->sfn, ul_config->slot, ul_config->number_pdus);
    break;

  case NR_DL_DCI_FORMAT_1_0:
    /*
     *  with CRC scrambled by C-RNTI or CS-RNTI or new-RNTI
     *    0  IDENTIFIER_DCI_FORMATS:
     *    11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 5.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
     *    24 MCS:
     *    25 NDI:
     *    26 RV:
     *    27 HARQ_PROCESS_NUMBER:
     *    28 DAI_: For format1_1: 4 if more than one serving cell are configured in the DL and the higher layer parameter HARQ-ACK-codebook=dynamic, where the 2 MSB bits are the counter DAI and the 2 LSB bits are the total DAI
     *    33 TPC_PUCCH:
     *    34 PUCCH_RESOURCE_IND:
     *    35 PDSCH_TO_HARQ_FEEDBACK_TIME_IND:
     *    55 RESERVED_NR_DCI
     *  with CRC scrambled by P-RNTI
     *    8  SHORT_MESSAGE_IND
     *    9  SHORT_MESSAGES
     *    11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 5.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
     *    24 MCS:
     *    31 TB_SCALING
     *    55 RESERVED_NR_DCI
     *  with CRC scrambled by SI-RNTI
     *    11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 5.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
     *    24 MCS:
     *    26 RV:
     *    55 RESERVED_NR_DCI
     *  with CRC scrambled by RA-RNTI
     *    11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 5.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
     *    24 MCS:
     *    31 TB_SCALING
     *    55 RESERVED_NR_DCI
     *  with CRC scrambled by TC-RNTI
     *    0  IDENTIFIER_DCI_FORMATS:
     *    11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 5.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
     *    24 MCS:
     *    25 NDI:
     *    26 RV:
     *    27 HARQ_PROCESS_NUMBER:
     *    28 DAI_: For format1_1: 4 if more than one serving cell are configured in the DL and the higher layer parameter HARQ-ACK-codebook=dynamic, where the 2 MSB bits are the counter DAI and the 2 LSB bits are the total DAI
     *    33 TPC_PUCCH:
     */

    dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.rnti = rnti;
    //fapi_nr_dl_config_dlsch_pdu_rel15_t dlsch_config_pdu_1_0 = dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.dlsch_config_rel15;
    fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu_1_0 = &dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.dlsch_config_rel15;
    dlsch_config_pdu_1_0->BWPSize = NRRIV2BW(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth,275);
    dlsch_config_pdu_1_0->BWPStart = NRRIV2PRBOFFSET(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth,275);
    dlsch_config_pdu_1_0->SubcarrierSpacing = mac->DLbwp[0]->bwp_Common->genericParameters.subcarrierSpacing;
    /* IDENTIFIER_DCI_FORMATS */
    /* FREQ_DOM_RESOURCE_ASSIGNMENT_DL */
    if (nr_ue_process_dci_freq_dom_resource_assignment(NULL,dlsch_config_pdu_1_0,0,n_RB_DLBWP,dci->frequency_domain_assignment.val) < 0)
      return -1;
    /* TIME_DOM_RESOURCE_ASSIGNMENT */
    if (nr_ue_process_dci_time_dom_resource_assignment(mac,NULL,dlsch_config_pdu_1_0,dci->time_domain_assignment.val) < 0)
      return -1;
    /* dmrs symbol positions*/
    dlsch_config_pdu_1_0->dlDmrsSymbPos = fill_dmrs_mask(pdsch_config,
							 mac->scc->dmrs_TypeA_Position,
							 dlsch_config_pdu_1_0->number_symbols);
    dlsch_config_pdu_1_0->dmrsConfigType = mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type == NULL ? 0 : 1;
    /* number of DM-RS CDM groups without data according to subclause 5.1.6.2 of 3GPP TS 38.214 version 15.9.0 Release 15 */
    if (dlsch_config_pdu_1_0->number_symbols == 2)
      dlsch_config_pdu_1_0->n_dmrs_cdm_groups = 1;
    else
      dlsch_config_pdu_1_0->n_dmrs_cdm_groups = 2;
    /* VRB_TO_PRB_MAPPING */
    dlsch_config_pdu_1_0->vrb_to_prb_mapping = (dci->vrb_to_prb_mapping.val == 0) ? vrb_to_prb_mapping_non_interleaved:vrb_to_prb_mapping_interleaved;
    /* MCS TABLE INDEX */
    dlsch_config_pdu_1_0->mcs_table = (pdsch_config->mcs_Table) ? (*pdsch_config->mcs_Table + 1) : 0;
    /* MCS */
    dlsch_config_pdu_1_0->mcs = dci->mcs;
    // Basic sanity check for MCS value to check for a false or erroneous DCI
    if (dlsch_config_pdu_1_0->mcs > 28) {
      LOG_W(MAC, "MCS value % d out of bounds! Possibly due to false DCI. Ignoring DCI!!\n", dlsch_config_pdu_1_0->mcs);
      return -1;
    }
    /* NDI (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI)*/
    dlsch_config_pdu_1_0->ndi = dci->ndi;
    /* RV (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI)*/
    dlsch_config_pdu_1_0->rv = dci->rv;
    /* HARQ_PROCESS_NUMBER (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI)*/
    dlsch_config_pdu_1_0->harq_process_nbr = dci->harq_pid;
    /* DAI (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI)*/
    dlsch_config_pdu_1_0->dai = dci->dai[0].val;
    /* TB_SCALING (only if CRC scrambled by P-RNTI or RA-RNTI) */
    // according to TS 38.214 Table 5.1.3.2-3
    if (dci->tb_scaling == 0) dlsch_config_pdu_1_0->scaling_factor_S = 1;
    if (dci->tb_scaling == 1) dlsch_config_pdu_1_0->scaling_factor_S = 0.5;
    if (dci->tb_scaling == 2) dlsch_config_pdu_1_0->scaling_factor_S = 0.25;
    if (dci->tb_scaling == 3) dlsch_config_pdu_1_0->scaling_factor_S = 0; // value not defined in table
    /* TPC_PUCCH (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI)*/
    // according to TS 38.213 Table 7.2.1-1
    if (dci->tpc == 0) dlsch_config_pdu_1_0->accumulated_delta_PUCCH = -1;
    if (dci->tpc == 1) dlsch_config_pdu_1_0->accumulated_delta_PUCCH = 0;
    if (dci->tpc == 2) dlsch_config_pdu_1_0->accumulated_delta_PUCCH = 1;
    if (dci->tpc == 3) dlsch_config_pdu_1_0->accumulated_delta_PUCCH = 3;
    /* PUCCH_RESOURCE_IND (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI)*/
    //if (dci->pucch_resource_indicator == 0) dlsch_config_pdu_1_0->pucch_resource_id = 1; //pucch-ResourceId obtained from the 1st value of resourceList FIXME!!!
    //if (dci->pucch_resource_indicator == 1) dlsch_config_pdu_1_0->pucch_resource_id = 2; //pucch-ResourceId obtained from the 2nd value of resourceList FIXME!!
    //if (dci->pucch_resource_indicator == 2) dlsch_config_pdu_1_0->pucch_resource_id = 3; //pucch-ResourceId obtained from the 3rd value of resourceList FIXME!!
    //if (dci->pucch_resource_indicator == 3) dlsch_config_pdu_1_0->pucch_resource_id = 4; //pucch-ResourceId obtained from the 4th value of resourceList FIXME!!
    //if (dci->pucch_resource_indicator == 4) dlsch_config_pdu_1_0->pucch_resource_id = 5; //pucch-ResourceId obtained from the 5th value of resourceList FIXME!!
    //if (dci->pucch_resource_indicator == 5) dlsch_config_pdu_1_0->pucch_resource_id = 6; //pucch-ResourceId obtained from the 6th value of resourceList FIXME!!
    //if (dci->pucch_resource_indicator == 6) dlsch_config_pdu_1_0->pucch_resource_id = 7; //pucch-ResourceId obtained from the 7th value of resourceList FIXME!!
    //if (dci->pucch_resource_indicator == 7) dlsch_config_pdu_1_0->pucch_resource_id = 8; //pucch-ResourceId obtained from the 8th value of resourceList FIXME!!
    dlsch_config_pdu_1_0->pucch_resource_id = dci->pucch_resource_indicator;
    // Sanity check for pucch_resource_indicator value received to check for false DCI.
    valid = 0;
    pucch_res_set_cnt = mac->ULbwp[0]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList->list.count;
    for (int id = 0; id < pucch_res_set_cnt; id++) {
      if (dlsch_config_pdu_1_0->pucch_resource_id < mac->ULbwp[0]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList->list.array[id]->resourceList.list.count) {
        valid = 1;
        break;
      }
    }
    if (!valid) {
      LOG_W(MAC, "pucch_resource_indicator value %d is out of bounds. Possibly due to false DCI. Ignoring DCI!\n", dlsch_config_pdu_1_0->pucch_resource_id);
      return -1;
    }

    /* PDSCH_TO_HARQ_FEEDBACK_TIME_IND (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI)*/
    dlsch_config_pdu_1_0->pdsch_to_harq_feedback_time_ind = dci->pdsch_to_harq_feedback_timing_indicator.val;

    LOG_D(MAC,"(nr_ue_procedures.c) rnti = %x dl_config->number_pdus = %d\n",
	  dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.rnti,
	  dl_config->number_pdus);
    LOG_D(MAC,"(nr_ue_procedures.c) frequency_domain_resource_assignment=%d \t number_rbs=%d \t start_rb=%d\n",
	  dci->frequency_domain_assignment.val,
	  dlsch_config_pdu_1_0->number_rbs,
	  dlsch_config_pdu_1_0->start_rb);
    LOG_D(MAC,"(nr_ue_procedures.c) time_domain_resource_assignment=%d \t number_symbols=%d \t start_symbol=%d\n",
	  dci->time_domain_assignment.val,
	  dlsch_config_pdu_1_0->number_symbols,
	  dlsch_config_pdu_1_0->start_symbol);
    LOG_D(MAC,"(nr_ue_procedures.c) vrb_to_prb_mapping=%d \n>>> mcs=%d\n>>> ndi=%d\n>>> rv=%d\n>>> harq_process_nbr=%d\n>>> dai=%d\n>>> scaling_factor_S=%f\n>>> tpc_pucch=%d\n>>> pucch_res_ind=%d\n>>> pdsch_to_harq_feedback_time_ind=%d\n",
	  dlsch_config_pdu_1_0->vrb_to_prb_mapping,
	  dlsch_config_pdu_1_0->mcs,
	  dlsch_config_pdu_1_0->ndi,
	  dlsch_config_pdu_1_0->rv,
	  dlsch_config_pdu_1_0->harq_process_nbr,
	  dlsch_config_pdu_1_0->dai,
	  dlsch_config_pdu_1_0->scaling_factor_S,
	  dlsch_config_pdu_1_0->accumulated_delta_PUCCH,
	  dlsch_config_pdu_1_0->pucch_resource_id,
	  dlsch_config_pdu_1_0->pdsch_to_harq_feedback_time_ind);

    if (mac->RA_window_cnt >= 0 && rnti == mac->ra_rnti){
      dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_RA_DLSCH;
    } else {
      dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DLSCH;
    }

    //	    dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.N_RB_BWP = n_RB_DLBWP;
	    
    LOG_D(MAC,"(nr_ue_procedures.c) pdu_type=%d\n\n",dl_config->dl_config_list[dl_config->number_pdus].pdu_type);
            
    dl_config->number_pdus = dl_config->number_pdus + 1;
    break;

  case NR_DL_DCI_FORMAT_1_1:        
    /*
     *  with CRC scrambled by C-RNTI or CS-RNTI or new-RNTI
     *    0  IDENTIFIER_DCI_FORMATS:
     *    1  CARRIER_IND:
     *    7  BANDWIDTH_PART_IND:
     *    11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 5.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
     *    14 PRB_BUNDLING_SIZE_IND:
     *    15 RATE_MATCHING_IND:
     *    16 ZP_CSI_RS_TRIGGER:
     *    18 TB1_MCS:
     *    19 TB1_NDI:
     *    20 TB1_RV:
     *    21 TB2_MCS:
     *    22 TB2_NDI:
     *    23 TB2_RV:
     *    27 HARQ_PROCESS_NUMBER:
     *    28 DAI_: For format1_1: 4 if more than one serving cell are configured in the DL and the higher layer parameter HARQ-ACK-codebook=dynamic, where the 2 MSB bits are the counter DAI and the 2 LSB bits are the total DAI
     *    33 TPC_PUCCH:
     *    34 PUCCH_RESOURCE_IND:
     *    35 PDSCH_TO_HARQ_FEEDBACK_TIME_IND:
     *    38 ANTENNA_PORTS:
     *    39 TCI:
     *    40 SRS_REQUEST:
     *    43 CBGTI:
     *    44 CBGFI:
     *    47 DMRS_SEQ_INI:
     */
    dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DLSCH;
    dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.rnti = rnti;
    fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu_1_1 = &dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.dlsch_config_rel15;
    /* IDENTIFIER_DCI_FORMATS */
    /* CARRIER_IND */
    /* BANDWIDTH_PART_IND */
    //    dlsch_config_pdu_1_1->bandwidth_part_ind = dci->bandwidth_part_ind;
    /* FREQ_DOM_RESOURCE_ASSIGNMENT_DL */
    if (nr_ue_process_dci_freq_dom_resource_assignment(NULL,dlsch_config_pdu_1_1,0,n_RB_DLBWP,dci->frequency_domain_assignment.val) < 0)
      return -1;
    /* TIME_DOM_RESOURCE_ASSIGNMENT */
    if (nr_ue_process_dci_time_dom_resource_assignment(mac,NULL,dlsch_config_pdu_1_1,dci->time_domain_assignment.val) < 0)
      return -1;
    /* dmrs symbol positions*/
    dlsch_config_pdu_1_1->dlDmrsSymbPos = fill_dmrs_mask(pdsch_config,
							 mac->scc->dmrs_TypeA_Position,
							 dlsch_config_pdu_1_1->number_symbols);
    dlsch_config_pdu_1_1->dmrsConfigType = mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type == NULL ? 0 : 1;
    /* TODO: fix number of DM-RS CDM groups without data according to subclause 5.1.6.2 of 3GPP TS 38.214,
             using tables 7.3.1.2.2-1, 7.3.1.2.2-2, 7.3.1.2.2-3, 7.3.1.2.2-4 of 3GPP TS 38.212 */
    dlsch_config_pdu_1_1->n_dmrs_cdm_groups = 1;
    /* VRB_TO_PRB_MAPPING */
    if (mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.resource_allocation != 0)
      dlsch_config_pdu_1_1->vrb_to_prb_mapping = (dci->vrb_to_prb_mapping.val == 0) ? vrb_to_prb_mapping_non_interleaved:vrb_to_prb_mapping_interleaved;
    /* PRB_BUNDLING_SIZE_IND */
    dlsch_config_pdu_1_1->prb_bundling_size_ind = dci->prb_bundling_size_indicator.val;
    /* RATE_MATCHING_IND */
    dlsch_config_pdu_1_1->rate_matching_ind = dci->rate_matching_indicator.val;
    /* ZP_CSI_RS_TRIGGER */
    dlsch_config_pdu_1_1->zp_csi_rs_trigger = dci->zp_csi_rs_trigger.val;
    /* MCS (for transport block 1)*/
    dlsch_config_pdu_1_1->mcs = dci->mcs;
    // Basic sanity check for MCS value to check for a false or erroneous DCI
    if (dlsch_config_pdu_1_1->mcs > 28) {
      LOG_W(MAC, "MCS value % d out of bounds! Possibly due to false DCI. Ignoring DCI!!\n", dlsch_config_pdu_1_1->mcs);
      return -1;
    }
    /* NDI (for transport block 1)*/
    dlsch_config_pdu_1_1->ndi = dci->ndi;
    /* RV (for transport block 1)*/
    dlsch_config_pdu_1_1->rv = dci->rv;
    /* MCS (for transport block 2)*/
    dlsch_config_pdu_1_1->tb2_mcs = dci->mcs2.val;
    // Basic sanity check for MCS value to check for a false or erroneous DCI
    if (dlsch_config_pdu_1_1->tb2_mcs > 28) {
      LOG_W(MAC, "MCS value % d out of bounds! Possibly due to false DCI. Ignoring DCI!!\n", dlsch_config_pdu_1_1->tb2_mcs);
      return -1;
    }
    /* NDI (for transport block 2)*/
    dlsch_config_pdu_1_1->tb2_ndi = dci->ndi2.val;
    /* RV (for transport block 2)*/
    dlsch_config_pdu_1_1->tb2_rv = dci->rv2.val;
    /* HARQ_PROCESS_NUMBER */
    dlsch_config_pdu_1_1->harq_process_nbr = dci->harq_pid;
    /* DAI */
    dlsch_config_pdu_1_1->dai = dci->dai[0].val;
    /* TPC_PUCCH */
    // according to TS 38.213 Table 7.2.1-1
    if (dci->tpc == 0) dlsch_config_pdu_1_1->accumulated_delta_PUCCH = -1;
    if (dci->tpc == 1) dlsch_config_pdu_1_1->accumulated_delta_PUCCH = 0;
    if (dci->tpc == 2) dlsch_config_pdu_1_1->accumulated_delta_PUCCH = 1;
    if (dci->tpc == 3) dlsch_config_pdu_1_1->accumulated_delta_PUCCH = 3;
    /* PUCCH_RESOURCE_IND */
    dlsch_config_pdu_1_1->pucch_resource_id = dci->pucch_resource_indicator;
    // Sanity check for pucch_resource_indicator value received to check for false DCI.
    valid = 0;
    pucch_res_set_cnt = mac->ULbwp[0]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList->list.count;
    for (int id = 0; id < pucch_res_set_cnt; id++) {
      if (dlsch_config_pdu_1_1->pucch_resource_id < mac->ULbwp[0]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList->list.array[id]->resourceList.list.count) {
        valid = 1;
        break;
      }
    }
    if (!valid) {
      LOG_W(MAC, "pucch_resource_indicator value %d is out of bounds. Possibly due to false DCI. Ignoring DCI!\n", dlsch_config_pdu_1_1->pucch_resource_id);
      return -1;
    }

    /* PDSCH_TO_HARQ_FEEDBACK_TIME_IND */
    // according to TS 38.213 Table 9.2.3-1
    dlsch_config_pdu_1_1->pdsch_to_harq_feedback_time_ind = mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->dl_DataToUL_ACK->list.array[dci->pdsch_to_harq_feedback_timing_indicator.val][0];
    /* ANTENNA_PORTS */
    uint8_t n_codewords = 1; // FIXME!!!
    if ((mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.dmrs_type == 1) &&
	(mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.max_length == 1)){
      // Table 7.3.1.2.2-1: Antenna port(s) (1000 + DMRS port), dmrs-Type=1, maxLength=1
      dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_1[dci->antenna_ports.val][0];
      dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_1[dci->antenna_ports.val][1];
      dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_1[dci->antenna_ports.val][2];
      dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_1[dci->antenna_ports.val][3];
      dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_1[dci->antenna_ports.val][4];
    }
    if ((mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.dmrs_type == 1) &&
	(mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.max_length == 2)){
      // Table 7.3.1.2.2-2: Antenna port(s) (1000 + DMRS port), dmrs-Type=1, maxLength=2
      if (n_codewords == 1) {
	dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports.val][0];
	dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports.val][1];
	dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports.val][2];
	dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports.val][3];
	dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports.val][4];
	dlsch_config_pdu_1_1->n_front_load_symb = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports.val][5];
      }
      if (n_codewords == 1) {
	dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][0];
	dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][1];
	dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][2];
	dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][3];
	dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][4];
	dlsch_config_pdu_1_1->dmrs_ports[4]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][5];
	dlsch_config_pdu_1_1->dmrs_ports[5]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][6];
	dlsch_config_pdu_1_1->dmrs_ports[6]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][7];
	dlsch_config_pdu_1_1->dmrs_ports[7]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][8];
	dlsch_config_pdu_1_1->n_front_load_symb = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][9];
      }
    }
    if ((mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.dmrs_type == 2) &&
	(mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.max_length == 1)){
      // Table 7.3.1.2.2-3: Antenna port(s) (1000 + DMRS port), dmrs-Type=2, maxLength=1
      if (n_codewords == 1) {
	dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports.val][0];
	dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports.val][1];
	dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports.val][2];
	dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports.val][3];
	dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports.val][4];
      }
      if (n_codewords == 1) {
	dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports.val][0];
	dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports.val][1];
	dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports.val][2];
	dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports.val][3];
	dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports.val][4];
	dlsch_config_pdu_1_1->dmrs_ports[4]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports.val][5];
	dlsch_config_pdu_1_1->dmrs_ports[5]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports.val][6];
      }
    }
    if ((mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.dmrs_type == 2) &&
	(mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.max_length == 2)){
      // Table 7.3.1.2.2-4: Antenna port(s) (1000 + DMRS port), dmrs-Type=2, maxLength=2
      if (n_codewords == 1) {
	dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports.val][0];
	dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports.val][1];
	dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports.val][2];
	dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports.val][3];
	dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports.val][4];
	dlsch_config_pdu_1_1->n_front_load_symb = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports.val][5];
      }
      if (n_codewords == 1) {
	dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][0];
	dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][1];
	dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][2];
	dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][3];
	dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][4];
	dlsch_config_pdu_1_1->dmrs_ports[4]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][5];
	dlsch_config_pdu_1_1->dmrs_ports[5]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][6];
	dlsch_config_pdu_1_1->dmrs_ports[6]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][7];
	dlsch_config_pdu_1_1->dmrs_ports[7]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][8];
	dlsch_config_pdu_1_1->n_front_load_symb = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][9];
      }
    }
    /* TCI */
    if (mac->dl_config_request.dl_config_list[0].dci_config_pdu.dci_config_rel15.coreset.tci_present_in_dci == 1){
      // 0 bit if higher layer parameter tci-PresentInDCI is not enabled
      // otherwise 3 bits as defined in Subclause 5.1.5 of [6, TS38.214]
      dlsch_config_pdu_1_1->tci_state = dci->transmission_configuration_indication.val;
    }
    /* SRS_REQUEST */
    // if SUL is supported in the cell, there is an additional bit in this field and the value of this bit represents table 7.1.1.1-1 TS 38.212 FIXME!!!
    dlsch_config_pdu_1_1->srs_config.aperiodicSRS_ResourceTrigger = (dci->srs_request.val & 0x11); // as per Table 7.3.1.1.2-24 TS 38.212
    /* CBGTI */
    dlsch_config_pdu_1_1->cbgti = dci->cbgti.val;
    /* CBGFI */
    dlsch_config_pdu_1_1->codeBlockGroupFlushIndicator = dci->cbgfi.val;
    /* DMRS_SEQ_INI */
    //FIXME!!!

    //	    dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.N_RB_BWP = n_RB_DLBWP;
	    
    dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DLSCH;
    LOG_D(MAC,"(nr_ue_procedures.c) pdu_type=%d\n\n",dl_config->dl_config_list[dl_config->number_pdus].pdu_type);
            
    dl_config->number_pdus = dl_config->number_pdus + 1;
    /* TODO same calculation for MCS table as done in UL */
    dlsch_config_pdu_1_1->mcs_table = (pdsch_config->mcs_Table) ? (*pdsch_config->mcs_Table + 1) : 0;
    /*PTRS configuration */
    if(mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS != NULL) {
      valid_ptrs_setup = set_dl_ptrs_values(mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup,
                                            dlsch_config_pdu_1_1->number_rbs, dlsch_config_pdu_1_1->mcs, dlsch_config_pdu_1_1->mcs_table,
                                            &dlsch_config_pdu_1_1->PTRSFreqDensity,&dlsch_config_pdu_1_1->PTRSTimeDensity,
                                            &dlsch_config_pdu_1_1->PTRSPortIndex,&dlsch_config_pdu_1_1->nEpreRatioOfPDSCHToPTRS,
                                            &dlsch_config_pdu_1_1->PTRSReOffset, dlsch_config_pdu_1_1->number_symbols);
      if(valid_ptrs_setup==true) {
        dlsch_config_pdu_1_1->pduBitmap |= 0x1;
      }
    }

    break;

  case NR_DL_DCI_FORMAT_2_0:
    break;

  case NR_DL_DCI_FORMAT_2_1:        
    break;

  case NR_DL_DCI_FORMAT_2_2:        
    break;

  case NR_DL_DCI_FORMAT_2_3:
    break;

  default: 
    break;
  }


  if(rnti == SI_RNTI){

    //    }else if(rnti == mac->ra_rnti){

  }else if(rnti == P_RNTI){

  }else{  //  c-rnti

    ///  check if this is pdcch order 
    //dci->random_access_preamble_index;
    //dci->ss_pbch_index;
    //dci->prach_mask_index;

    ///  else normal DL-SCH grant
  }
  return 0;
}

int8_t nr_ue_get_SR(module_id_t module_idP, int CC_id, frame_t frameP, uint8_t eNB_id, uint16_t rnti, sub_frame_t subframe){

  return 0;
}

void nr_ue_send_sdu(module_id_t module_idP,
                    uint8_t CC_id,
                    frame_t frameP,
                    int slotP,
                    uint8_t * pdu, uint16_t pdu_len, uint8_t gNB_index,
                    NR_UL_TIME_ALIGNMENT_t *ul_time_alignment){

  LOG_D(MAC, "Handling PDU frame %d slot %d\n", frameP, slotP);

  uint8_t * pduP = pdu;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_SDU, VCD_FUNCTION_IN);

  //LOG_D(MAC,"sdu: %x.%x.%x\n",sdu[0],sdu[1],sdu[2]);

  /*
  #ifdef DEBUG_HEADER_PARSING
    LOG_D(MAC, "[UE %d] ue_send_sdu : Frame %d gNB_index %d : num_ce %d num_sdu %d\n",
      module_idP, frameP, gNB_index, num_ce, num_sdu);
  #endif
  */

  #if defined(ENABLE_MAC_PAYLOAD_DEBUG)
    LOG_T(MAC, "[UE %d] First 32 bytes of DLSCH : \n", module_idP);
    for (i = 0; i < 32; i++) {
      LOG_T(MAC, "%x.", sdu[i]);
    }
    LOG_T(MAC, "\n");
  #endif

  // Processing MAC PDU
  // it parses MAC CEs subheaders, MAC CEs, SDU subheaderds and SDUs
  if (pduP != NULL)
    nr_ue_process_mac_pdu(module_idP, CC_id, frameP, pduP, pdu_len, gNB_index, ul_time_alignment);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_SDU, VCD_FUNCTION_OUT);

}

// N_RB configuration according to 7.3.1.0 (DCI size alignment) of TS 38.212
int get_n_rb(NR_UE_MAC_INST_t *mac, int rnti_type){

  int N_RB = 0, start_RB;
  switch(rnti_type) {
    case NR_RNTI_RA:
    case NR_RNTI_TC:
    case NR_RNTI_P:
    case NR_RNTI_SI:
      if (mac->DLbwp[0]->bwp_Common->pdcch_ConfigCommon->choice.setup->controlResourceSetZero) {
        uint8_t bwp_id = 1;
        uint8_t coreset_id = 0; // assuming controlResourceSetId is 0 for controlResourceSetZero
        NR_ControlResourceSet_t *coreset = mac->coreset[bwp_id - 1][coreset_id];
        get_coreset_rballoc(coreset->frequencyDomainResources.buf,&N_RB,&start_RB);
      } else {
        N_RB = NRRIV2BW(mac->scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, 275);
      }
      break;
    case NR_RNTI_C:
    N_RB = NRRIV2BW(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth, 275);
    break;
  }
  return N_RB;

}

int nr_extract_dci_info(NR_UE_MAC_INST_t *mac,
			 int dci_format,
			 uint8_t dci_size,
			 uint16_t rnti,
			 uint64_t *dci_pdu,
			 dci_pdu_rel15_t *dci_pdu_rel15) {
  int rnti_type=-1;

  if       (rnti == mac->ra_rnti) rnti_type = NR_RNTI_RA;
  else if (rnti == mac->crnti)    rnti_type = NR_RNTI_C;
  else if (rnti == mac->t_crnti)  rnti_type = NR_RNTI_TC;
  else if (rnti == 0xFFFE)        rnti_type = NR_RNTI_P;
  else if (rnti == 0xFFFF)        rnti_type = NR_RNTI_SI;

  AssertFatal(rnti_type!=-1,"no identified/handled rnti\n");
  AssertFatal(mac->DLbwp[0] != NULL, "DLbwp[0] shouldn't be null here!\n");
  AssertFatal(mac->ULbwp[0] != NULL, "ULbwp[0] shouldn't be null here!\n");
  int N_RB = get_n_rb(mac, rnti_type);
  int N_RB_UL = (mac->scg != NULL) ? 
    NRRIV2BW(mac->ULbwp[0]->bwp_Common->genericParameters.locationAndBandwidth,275) :
    NRRIV2BW(mac->scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth,275);

  int pos=0;
  int fsize=0;

  if (rnti_type == NR_RNTI_C) {
    // First find out the DCI format from the first bit (UE performed blind decoding)
    pos++;
    dci_pdu_rel15->format_indicator = (*dci_pdu>>(dci_size-pos))&1;
#ifdef DEBUG_EXTRACT_DCI
    LOG_D(MAC,"Format indicator %d (%d bits) N_RB_BWP %d => %d (0x%lx)\n",dci_pdu_rel15->format_indicator,1,N_RB,dci_size-pos,*dci_pdu);
#endif

    if (dci_format == NR_UL_DCI_FORMAT_0_0 || dci_format == NR_DL_DCI_FORMAT_1_0) {
      if (dci_pdu_rel15->format_indicator == 0) 
        dci_format = NR_UL_DCI_FORMAT_0_0;
      else
        dci_format = NR_DL_DCI_FORMAT_1_0;
    }
    else if (dci_format == NR_UL_DCI_FORMAT_0_1 || dci_format == NR_DL_DCI_FORMAT_1_1) {
      // In case the sizes of formats 0_1 and 1_1 happen to be the same
      if (dci_pdu_rel15->format_indicator == 0) 
        dci_format = NR_UL_DCI_FORMAT_0_1;
      else
        dci_format = NR_DL_DCI_FORMAT_1_1;
    }
  }
#ifdef DEBUG_EXTRACT_DCI
  LOG_D(MAC, "DCI format is %d\n", dci_format);
#endif
  
  switch(dci_format) {

  case NR_DL_DCI_FORMAT_1_0:
    switch(rnti_type) {
    case NR_RNTI_RA:
      // Freq domain assignment
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      pos=fsize;
      dci_pdu_rel15->frequency_domain_assignment.val = *dci_pdu>>(dci_size-pos)&((1<<fsize)-1);
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"frequency-domain assignment %d (%d bits) N_RB_BWP %d=> %d (0x%lx)\n",dci_pdu_rel15->frequency_domain_assignment.val,fsize,N_RB,dci_size-pos,*dci_pdu);
#endif
      // Time domain assignment
      pos+=4;
      dci_pdu_rel15->time_domain_assignment.val = (*dci_pdu >> (dci_size-pos))&0xf;
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"time-domain assignment %d  (4 bits)=> %d (0x%lx)\n",dci_pdu_rel15->time_domain_assignment.val,dci_size-pos,*dci_pdu);
#endif
      // VRB to PRB mapping
	
      pos++;
      dci_pdu_rel15->vrb_to_prb_mapping.val = (*dci_pdu>>(dci_size-pos))&0x1;
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"vrb to prb mapping %d  (1 bits)=> %d (0x%lx)\n",dci_pdu_rel15->vrb_to_prb_mapping.val,dci_size-pos,*dci_pdu);
#endif
      // MCS
      pos+=5;
      dci_pdu_rel15->mcs = (*dci_pdu>>(dci_size-pos))&0x1f;
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"mcs %d  (5 bits)=> %d (0x%lx)\n",dci_pdu_rel15->mcs,dci_size-pos,*dci_pdu);
#endif
      // TB scaling
      pos+=2;
      dci_pdu_rel15->tb_scaling = (*dci_pdu>>(dci_size-pos))&0x3;
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"tb_scaling %d  (2 bits)=> %d (0x%lx)\n",dci_pdu_rel15->tb_scaling,dci_size-pos,*dci_pdu);
#endif
      break;

    case NR_RNTI_C:
	
      // Freq domain assignment (275rb >> fsize = 16)
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      pos+=fsize;
      dci_pdu_rel15->frequency_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&((1<<fsize)-1);
  	
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"Freq domain assignment %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->frequency_domain_assignment.val,fsize,dci_size-pos,*dci_pdu);
#endif
    	
      uint16_t is_ra = 1;
      for (int i=0; i<fsize; i++)
	if (!((dci_pdu_rel15->frequency_domain_assignment.val>>i)&1)) {
	  is_ra = 0;
	  break;
	}
      if (is_ra) //fsize are all 1  38.212 p86
	{
	  // ra_preamble_index 6 bits
	  pos+=6;
	  dci_pdu_rel15->ra_preamble_index = (*dci_pdu>>(dci_size-pos))&0x3f;
	    
	  // UL/SUL indicator  1 bit
	  pos++;
	  dci_pdu_rel15->ul_sul_indicator.val = (*dci_pdu>>(dci_size-pos))&1;
	    
	  // SS/PBCH index  6 bits
	  pos+=6;
	  dci_pdu_rel15->ss_pbch_index = (*dci_pdu>>(dci_size-pos))&0x3f;
	    
	  //  prach_mask_index  4 bits
	  pos+=4;
	  dci_pdu_rel15->prach_mask_index = (*dci_pdu>>(dci_size-pos))&0xf;
	    
	}  //end if
      else {
	  
	// Time domain assignment 4bit
		  
	pos+=4;
	dci_pdu_rel15->time_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&0xf;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"Time domain assignment %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->time_domain_assignment.val,4,dci_size-pos,*dci_pdu);
#endif
	  
	// VRB to PRB mapping  1bit
	pos++;
	dci_pdu_rel15->vrb_to_prb_mapping.val = (*dci_pdu>>(dci_size-pos))&1;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"VRB to PRB %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->vrb_to_prb_mapping.val,1,dci_size-pos,*dci_pdu);
#endif
	
	// MCS 5bit  //bit over 32, so dci_pdu ++
	pos+=5;
	dci_pdu_rel15->mcs = (*dci_pdu>>(dci_size-pos))&0x1f;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"MCS %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->mcs,5,dci_size-pos,*dci_pdu);
#endif
	  
	// New data indicator 1bit
	pos++;
	dci_pdu_rel15->ndi = (*dci_pdu>>(dci_size-pos))&1;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"NDI %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->ndi,1,dci_size-pos,*dci_pdu);
#endif      
	  
	// Redundancy version  2bit
	pos+=2;
	dci_pdu_rel15->rv = (*dci_pdu>>(dci_size-pos))&0x3;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"RV %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->rv,2,dci_size-pos,*dci_pdu);
#endif
	  
	// HARQ process number  4bit
	pos+=4;
	dci_pdu_rel15->harq_pid  = (*dci_pdu>>(dci_size-pos))&0xf;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"HARQ_PID %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->harq_pid,4,dci_size-pos,*dci_pdu);
#endif
	  
	// Downlink assignment index  2bit
	pos+=2;
	dci_pdu_rel15->dai[0].val = (*dci_pdu>>(dci_size-pos))&3;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"DAI %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->dai[0].val,2,dci_size-pos,*dci_pdu);
#endif
	  
	// TPC command for scheduled PUCCH  2bit
	pos+=2;
	dci_pdu_rel15->tpc = (*dci_pdu>>(dci_size-pos))&3;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"TPC %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->tpc,2,dci_size-pos,*dci_pdu);
#endif
	  
	// PUCCH resource indicator  3bit
	pos+=3;
	dci_pdu_rel15->pucch_resource_indicator = (*dci_pdu>>(dci_size-pos))&0x7;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"PUCCH RI %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->pucch_resource_indicator,3,dci_size-pos,*dci_pdu);
#endif
	  
	// PDSCH-to-HARQ_feedback timing indicator 3bit
	pos+=3;
	dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val = (*dci_pdu>>(dci_size-pos))&0x7;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"PDSCH to HARQ TI %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val,3,dci_size-pos,*dci_pdu);
#endif
	  
      } //end else
      break;
    	
    case NR_RNTI_P:
      /*
      // Short Messages Indicator  E2 bits
      for (int i=0; i<2; i++)
      dci_pdu |= (((uint64_t)dci_pdu_rel15->short_messages_indicator>>(1-i))&1)<<(dci_size-pos++);
      // Short Messages  E8 bits
      for (int i=0; i<8; i++)
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->short_messages>>(7-i))&1)<<(dci_size-pos++);
      // Freq domain assignment 0-16 bit
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      for (int i=0; i<fsize; i++)
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<(dci_size-pos++);
      // Time domain assignment 4 bit
      for (int i=0; i<4; i++)
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->time_domain_assignment>>(3-i))&1)<<(dci_size-pos++);
      // VRB to PRB mapping 1 bit
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->vrb_to_prb_mapping.val&1)<<(dci_size-pos++);
      // MCS 5 bit
      for (int i=0; i<5; i++)
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->mcs>>(4-i))&1)<<(dci_size-pos++);
	
      // TB scaling 2 bit
      for (int i=0; i<2; i++)
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->tb_scaling>>(1-i))&1)<<(dci_size-pos++);
      */	
	
      break;
  	
    case NR_RNTI_SI:
      /*
      // Freq domain assignment 0-16 bit
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      for (int i=0; i<fsize; i++)
      *dci_pdu |= ((dci_pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<(dci_size-pos++);
      // Time domain assignment 4 bit
      for (int i=0; i<4; i++)
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->time_domain_assignment>>(3-i))&1)<<(dci_size-pos++);
      // VRB to PRB mapping 1 bit
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->vrb_to_prb_mapping.val&1)<<(dci_size-pos++);
      // MCS 5bit  //bit over 32, so dci_pdu ++
      for (int i=0; i<5; i++)
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->mcs>>(4-i))&1)<<(dci_size-pos++);
      // Redundancy version  2bit
      for (int i=0; i<2; i++)
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->rv>>(1-i))&1)<<(dci_size-pos++);
      */	
      break;
	
    case NR_RNTI_TC:
      // indicating a DL DCI format 1bit
      pos++;
      dci_pdu_rel15->format_indicator = (*dci_pdu>>(dci_size-pos))&1;
      // Freq domain assignment 0-16 bit
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      pos+=fsize;
      dci_pdu_rel15->frequency_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&((1<<fsize)-1);
      // Time domain assignment 4 bit
      pos+=4;
      dci_pdu_rel15->time_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&0xf;
      // VRB to PRB mapping 1 bit
      dci_pdu_rel15->vrb_to_prb_mapping.val = (*dci_pdu>>(dci_size-pos))&1;
      // MCS 5bit  //bit over 32, so dci_pdu ++
      pos+=5;
      dci_pdu_rel15->mcs = (*dci_pdu>>(dci_size-pos))&0x1f;
      // New data indicator 1bit
      dci_pdu_rel15->ndi = (*dci_pdu>>(dci_size-pos))&1;
      // Redundancy version  2bit
      pos+=2;
      dci_pdu_rel15->rv = (*dci_pdu>>(dci_size-pos))&3;
      // HARQ process number  4bit
      pos+=4;
      dci_pdu_rel15->harq_pid = (*dci_pdu>>(dci_size-pos))&0xf;
      // Downlink assignment index  E2 bits
      pos+=2;
      dci_pdu_rel15->dai[0].val = (*dci_pdu>>(dci_size-pos))&3;
      // TPC command for scheduled PUCCH  E2 bits
      pos+=2;
      dci_pdu_rel15->tpc  = (*dci_pdu>>(dci_size-pos))&3;
      // PDSCH-to-HARQ_feedback timing indicator  E3 bits
      pos+=3;
      dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val = (*dci_pdu>>(dci_size-pos))&7;
       
      break;
    }
    break;
  
  case NR_UL_DCI_FORMAT_0_0:
    switch(rnti_type)
      {
      case NR_RNTI_C:
	fsize = (int)ceil( log2( (N_RB_UL*(N_RB_UL+1))>>1 ) );
	pos+=fsize;
	dci_pdu_rel15->frequency_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&((1<<fsize)-1);
	// Time domain assignment 4bit
	pos+=4;
	dci_pdu_rel15->time_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&0xf;
	// Frequency hopping flag  E1 bit
	pos++;
	dci_pdu_rel15->frequency_hopping_flag.val= (*dci_pdu>>(dci_size-pos))&1;
	// MCS  5 bit
	pos+=5;
	dci_pdu_rel15->mcs= (*dci_pdu>>(dci_size-pos))&0x1f;
	// New data indicator 1bit
	pos++;
	dci_pdu_rel15->ndi= (*dci_pdu>>(dci_size-pos))&1;
	// Redundancy version  2bit
	pos+=2;
	dci_pdu_rel15->rv= (*dci_pdu>>(dci_size-pos))&3;
	// HARQ process number  4bit
	pos+=4;
	dci_pdu_rel15->harq_pid = (*dci_pdu>>(dci_size-pos))&0xf;
	// TPC command for scheduled PUSCH  E2 bits
	pos+=2;
	dci_pdu_rel15->tpc = (*dci_pdu>>(dci_size-pos))&3;
	// UL/SUL indicator  E1 bit
	/* commented for now (RK): need to get this from BWP descriptor
	   if (cfg->pucch_config.pucch_GroupHopping.value)
	   dci_pdu->= ((uint64_t)*dci_pdu>>(dci_size-pos)ul_sul_indicator&1)<<(dci_size-pos++);
	*/
	break;
	
      case NR_RNTI_TC:
	/*	
	// indicating a DL DCI format 1bit
	dci_pdu->= (*dci_pdu>>(dci_size-pos)format_indicator&1)<<(dci_size-pos++);
	// Freq domain assignment  max 16 bit
	fsize = (int)ceil( log2( (N_RB_UL*(N_RB_UL+1))>>1 ) );
	for (int i=0; i<fsize; i++)
	dci_pdu->= ((*dci_pdu>>(dci_size-pos)frequency_domain_assignment>>(fsize-i-1))&1)<<(dci_size-pos++);
	// Time domain assignment 4bit
	for (int i=0; i<4; i++)
	dci_pdu->= (((uint64_t)*dci_pdu>>(dci_size-pos)time_domain_assignment>>(3-i))&1)<<(dci_size-pos++);
	// Frequency hopping flag  E1 bit
	dci_pdu->= ((uint64_t)*dci_pdu>>(dci_size-pos)frequency_hopping_flag&1)<<(dci_size-pos++);
	// MCS  5 bit
	for (int i=0; i<5; i++)
	dci_pdu->= (((uint64_t)*dci_pdu>>(dci_size-pos)mcs>>(4-i))&1)<<(dci_size-pos++);
	// New data indicator 1bit
	dci_pdu->= ((uint64_t)*dci_pdu>>(dci_size-pos)ndi&1)<<(dci_size-pos++);
	// Redundancy version  2bit
	for (int i=0; i<2; i++)
	dci_pdu->= (((uint64_t)*dci_pdu>>(dci_size-pos)rv>>(1-i))&1)<<(dci_size-pos++);
	// HARQ process number  4bit
	for (int i=0; i<4; i++)
	*dci_pdu  |= (((uint64_t)*dci_pdu>>(dci_size-pos)harq_pid>>(3-i))&1)<<(dci_size-pos++);
	
	// TPC command for scheduled PUSCH  E2 bits
	for (int i=0; i<2; i++)
	dci_pdu->= (((uint64_t)*dci_pdu>>(dci_size-pos)tpc>>(1-i))&1)<<(dci_size-pos++);
	*/	
	// UL/SUL indicator  E1 bit
	/*
	  commented for now (RK): need to get this information from BWP descriptor
	  if (cfg->pucch_config.pucch_GroupHopping.value)
	  dci_pdu->= ((uint64_t)dci_pdu_rel15->ul_sul_indicator&1)<<(dci_size-pos++);
	*/
	break;
	
      }
    break;

  case NR_DL_DCI_FORMAT_1_1:
  switch(rnti_type)
    {
      case NR_RNTI_C:
        // Carrier indicator
        pos+=dci_pdu_rel15->carrier_indicator.nbits;
        dci_pdu_rel15->carrier_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->carrier_indicator.nbits)-1);
        // BWP Indicator
        pos+=dci_pdu_rel15->bwp_indicator.nbits;
        dci_pdu_rel15->bwp_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->bwp_indicator.nbits)-1);
        // Frequency domain resource assignment
        pos+=dci_pdu_rel15->frequency_domain_assignment.nbits;
        dci_pdu_rel15->frequency_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->frequency_domain_assignment.nbits)-1);
        // Time domain resource assignment
        pos+=dci_pdu_rel15->time_domain_assignment.nbits;
        dci_pdu_rel15->time_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->time_domain_assignment.nbits)-1);
        // VRB-to-PRB mapping
        pos+=dci_pdu_rel15->vrb_to_prb_mapping.nbits;
        dci_pdu_rel15->vrb_to_prb_mapping.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->vrb_to_prb_mapping.nbits)-1);
        // PRB bundling size indicator
        pos+=dci_pdu_rel15->prb_bundling_size_indicator.nbits;
        dci_pdu_rel15->prb_bundling_size_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->prb_bundling_size_indicator.nbits)-1);
        // Rate matching indicator
        pos+=dci_pdu_rel15->rate_matching_indicator.nbits;
        dci_pdu_rel15->rate_matching_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->rate_matching_indicator.nbits)-1);
        // ZP CSI-RS trigger
        pos+=dci_pdu_rel15->zp_csi_rs_trigger.nbits;
        dci_pdu_rel15->zp_csi_rs_trigger.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->zp_csi_rs_trigger.nbits)-1);
        //TB1
        // MCS 5bit
        pos+=5;
        dci_pdu_rel15->mcs = (*dci_pdu>>(dci_size-pos))&0x1f;
        // New data indicator 1bit
        pos+=1;
        dci_pdu_rel15->ndi = (*dci_pdu>>(dci_size-pos))&0x1;
        // Redundancy version  2bit
        pos+=2;
        dci_pdu_rel15->rv = (*dci_pdu>>(dci_size-pos))&0x3;
        //TB2
        // MCS 5bit
        pos+=dci_pdu_rel15->mcs2.nbits;
        dci_pdu_rel15->mcs2.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->mcs2.nbits)-1);
        // New data indicator 1bit
        pos+=dci_pdu_rel15->ndi2.nbits;
        dci_pdu_rel15->ndi2.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->ndi2.nbits)-1);
        // Redundancy version  2bit
        pos+=dci_pdu_rel15->rv2.nbits;
        dci_pdu_rel15->rv2.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->rv2.nbits)-1);
        // HARQ process number  4bit
        pos+=4;
        dci_pdu_rel15->harq_pid = (*dci_pdu>>(dci_size-pos))&0xf;
        // Downlink assignment index
        pos+=dci_pdu_rel15->dai[0].nbits;
        dci_pdu_rel15->dai[0].val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->dai[0].nbits)-1);
        // TPC command for scheduled PUCCH  2bit
        pos+=2;
        dci_pdu_rel15->tpc = (*dci_pdu>>(dci_size-pos))&0x3;
        // PUCCH resource indicator  3bit
        pos+=3;
        dci_pdu_rel15->pucch_resource_indicator = (*dci_pdu>>(dci_size-pos))&0x3;
        // PDSCH-to-HARQ_feedback timing indicator
        pos+=dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.nbits;
        dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.nbits)-1);
        // Antenna ports
        pos+=dci_pdu_rel15->antenna_ports.nbits;
        dci_pdu_rel15->antenna_ports.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->antenna_ports.nbits)-1);
        // TCI
        pos+=dci_pdu_rel15->transmission_configuration_indication.nbits;
        dci_pdu_rel15->transmission_configuration_indication.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->transmission_configuration_indication.nbits)-1);
        // SRS request
        pos+=dci_pdu_rel15->srs_request.nbits;
        dci_pdu_rel15->srs_request.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->srs_request.nbits)-1);
        // CBG transmission information
        pos+=dci_pdu_rel15->cbgti.nbits;
        dci_pdu_rel15->cbgti.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->cbgti.nbits)-1);
        // CBG flushing out information
        pos+=dci_pdu_rel15->cbgfi.nbits;
        dci_pdu_rel15->cbgfi.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->cbgfi.nbits)-1);
        // DMRS sequence init
        pos+=1;
        dci_pdu_rel15->dmrs_sequence_initialization.val = (*dci_pdu>>(dci_size-pos))&0x1;
        break;
      }
      break;

  case NR_UL_DCI_FORMAT_0_1:
    switch(rnti_type)
      {
      case NR_RNTI_C:
        // Carrier indicator
        pos+=dci_pdu_rel15->carrier_indicator.nbits;
        dci_pdu_rel15->carrier_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->carrier_indicator.nbits)-1);
        
        // UL/SUL Indicator
        pos+=dci_pdu_rel15->ul_sul_indicator.nbits;
        dci_pdu_rel15->ul_sul_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->ul_sul_indicator.nbits)-1);
        
        // BWP Indicator
        pos+=dci_pdu_rel15->bwp_indicator.nbits;
        dci_pdu_rel15->bwp_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->bwp_indicator.nbits)-1);
        
        // Freq domain assignment  max 16 bit
        fsize = (int)ceil( log2( (N_RB_UL*(N_RB_UL+1))>>1 ) );
        //pos+=dci_pdu_rel15->frequency_domain_assignment.nbits;
        pos+=fsize;
        
        //pos+=dci_pdu_rel15->frequency_domain_assignment.nbits;
        dci_pdu_rel15->frequency_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&((1<<fsize)-1);
        
        // Time domain assignment 4bit
        //pos+=4;
        pos+=dci_pdu_rel15->time_domain_assignment.nbits;
        dci_pdu_rel15->time_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&0x3;
        
        // Not supported yet - skip for now
        // Frequency hopping flag – 1 bit 
        //pos++;
        //dci_pdu_rel15->frequency_hopping_flag.val= (*dci_pdu>>(dci_size-pos))&1;

        // MCS  5 bit
        pos+=5;
        dci_pdu_rel15->mcs= (*dci_pdu>>(dci_size-pos))&0x1f;

        // New data indicator 1bit
        pos++;
        dci_pdu_rel15->ndi= (*dci_pdu>>(dci_size-pos))&1;

        // Redundancy version  2bit
        pos+=2;
        dci_pdu_rel15->rv= (*dci_pdu>>(dci_size-pos))&3;

        // HARQ process number  4bit
        pos+=4;
        dci_pdu_rel15->harq_pid = (*dci_pdu>>(dci_size-pos))&0xf;

        // 1st Downlink assignment index
        pos+=dci_pdu_rel15->dai[0].nbits;
        dci_pdu_rel15->dai[0].val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->dai[0].nbits)-1);

        // 2nd Downlink assignment index
        pos+=dci_pdu_rel15->dai[1].nbits;
        dci_pdu_rel15->dai[1].val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->dai[1].nbits)-1);

        // TPC command for scheduled PUSCH – 2 bits
        pos+=2;
        dci_pdu_rel15->tpc = (*dci_pdu>>(dci_size-pos))&3;

        // SRS resource indicator
        pos+=dci_pdu_rel15->srs_resource_indicator.nbits;
        dci_pdu_rel15->srs_resource_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->srs_resource_indicator.nbits)-1);

        // Precoding info and n. of layers
        pos+=dci_pdu_rel15->precoding_information.nbits;
        dci_pdu_rel15->precoding_information.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->precoding_information.nbits)-1);

        // Antenna ports
        pos+=dci_pdu_rel15->antenna_ports.nbits;
        dci_pdu_rel15->antenna_ports.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->antenna_ports.nbits)-1);

        // SRS request
        pos+=dci_pdu_rel15->srs_request.nbits;
        dci_pdu_rel15->srs_request.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->srs_request.nbits)-1);

        // CSI request
        pos+=dci_pdu_rel15->csi_request.nbits;
        dci_pdu_rel15->csi_request.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->csi_request.nbits)-1);

        // CBG transmission information
        pos+=dci_pdu_rel15->cbgti.nbits;
        dci_pdu_rel15->cbgti.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->cbgti.nbits)-1);

        // PTRS DMRS association
        pos+=dci_pdu_rel15->ptrs_dmrs_association.nbits;
        dci_pdu_rel15->ptrs_dmrs_association.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->ptrs_dmrs_association.nbits)-1);

        // Beta offset indicator
        pos+=dci_pdu_rel15->beta_offset_indicator.nbits;
        dci_pdu_rel15->beta_offset_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->beta_offset_indicator.nbits)-1);

        // DMRS sequence initialization
        pos+=dci_pdu_rel15->dmrs_sequence_initialization.nbits;
        dci_pdu_rel15->dmrs_sequence_initialization.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->dmrs_sequence_initialization.nbits)-1);

        // UL-SCH indicator
        pos+=1;
        dci_pdu_rel15->ulsch_indicator = (*dci_pdu>>(dci_size-pos))&0x1;

        // UL/SUL indicator – 1 bit
        /* commented for now (RK): need to get this from BWP descriptor
          if (cfg->pucch_config.pucch_GroupHopping.value)
          dci_pdu->= ((uint64_t)*dci_pdu>>(dci_size-pos)ul_sul_indicator&1)<<(dci_size-pos++);
        */
        break;
      }
    break;
       }
    
    return dci_format;
}


void nr_ue_process_mac_pdu(module_id_t module_idP,
                           uint8_t CC_id,
                           frame_t frameP,
                           uint8_t *pduP, 
                           uint16_t mac_pdu_len,
                           uint8_t gNB_index,
                           NR_UL_TIME_ALIGNMENT_t *ul_time_alignment){

    // This function is adapting code from the old
    // parse_header(...) and ue_send_sdu(...) functions of OAI LTE

    uint8_t *pdu_ptr = pduP, rx_lcid, done = 0;
    int pdu_len = mac_pdu_len;
    uint16_t mac_ce_len, mac_subheader_len, mac_sdu_len;
    NR_UE_MAC_INST_t *mac = get_mac_inst(module_idP);

    //NR_UE_MAC_INST_t *UE_mac_inst = get_mac_inst(module_idP);
    //uint8_t scs = UE_mac_inst->mib->subCarrierSpacingCommon;
    //uint16_t bwp_ul_NB_RB = UE_mac_inst->initial_bwp_ul.N_RB;

    //  For both DL/UL-SCH
    //  Except:
    //   - UL/DL-SCH: fixed-size MAC CE(known by LCID)
    //   - UL/DL-SCH: padding
    //   - UL-SCH:    MSG3 48-bits
    //  |0|1|2|3|4|5|6|7|  bit-wise
    //  |R|F|   LCID    |
    //  |       L       |
    //  |0|1|2|3|4|5|6|7|  bit-wise
    //  |R|F|   LCID    |
    //  |       L       |
    //  |       L       |

    //  For both DL/UL-SCH
    //  For:
    //   - UL/DL-SCH: fixed-size MAC CE(known by LCID)
    //   - UL/DL-SCH: padding, for single/multiple 1-oct padding CE(s)
    //   - UL-SCH:    MSG3 48-bits
    //  |0|1|2|3|4|5|6|7|  bit-wise
    //  |R|R|   LCID    |
    //  LCID: The Logical Channel ID field identifies the logical channel instance of the corresponding MAC SDU or the type of the corresponding MAC CE or padding as described in Tables 6.2.1-1 and 6.2.1-2 for the DL-SCH and UL-SCH respectively. There is one LCID field per MAC subheader. The LCID field size is 6 bits;
    //  L: The Length field indicates the length of the corresponding MAC SDU or variable-sized MAC CE in bytes. There is one L field per MAC subheader except for subheaders corresponding to fixed-sized MAC CEs and padding. The size of the L field is indicated by the F field;
    //  F: lenght of L is 0:8 or 1:16 bits wide
    //  R: Reserved bit, set to zero.
    while (!done && pdu_len > 0){
        mac_ce_len = 0x0000;
        mac_subheader_len = 0x0001; //  default to fixed-length subheader = 1-oct
        mac_sdu_len = 0x0000;
        rx_lcid = ((NR_MAC_SUBHEADER_FIXED *)pdu_ptr)->LCID;
	  //#ifdef DEBUG_HEADER_PARSING
              LOG_D(MAC, "[UE] LCID %d, PDU length %d\n", ((NR_MAC_SUBHEADER_FIXED *)pdu_ptr)->LCID, pdu_len);
	      //#endif

        LOG_D(MAC, "[UE] LCID %d, PDU length %d\n", rx_lcid, pdu_len);
        switch(rx_lcid){
            //  MAC CE

            case DL_SCH_LCID_CCCH:
                //  MSG4 RRC Connection Setup 38.331
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }

                break;

            case DL_SCH_LCID_TCI_STATE_ACT_UE_SPEC_PDSCH:

                //  38.321 Ch6.1.3.14
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            case DL_SCH_LCID_APERIODIC_CSI_TRI_STATE_SUBSEL:
                //  38.321 Ch6.1.3.13
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            case DL_SCH_LCID_SP_CSI_RS_CSI_IM_RES_SET_ACT:
                //  38.321 Ch6.1.3.12
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            case DL_SCH_LCID_SP_SRS_ACTIVATION:
                //  38.321 Ch6.1.3.17
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            
            case DL_SCH_LCID_RECOMMENDED_BITRATE:
                //  38.321 Ch6.1.3.20
                mac_ce_len = 2;
                break;
            case DL_SCH_LCID_SP_ZP_CSI_RS_RES_SET_ACT:
                //  38.321 Ch6.1.3.19
                mac_ce_len = 2;
                break;
            case DL_SCH_LCID_PUCCH_SPATIAL_RELATION_ACT:
                //  38.321 Ch6.1.3.18
                mac_ce_len = 3;
                break;
            case DL_SCH_LCID_SP_CSI_REP_PUCCH_ACT:
                //  38.321 Ch6.1.3.16
                mac_ce_len = 2;
                break;
            case DL_SCH_LCID_TCI_STATE_IND_UE_SPEC_PDCCH:
                //  38.321 Ch6.1.3.15
                mac_ce_len = 2;
                break;
            case DL_SCH_LCID_DUPLICATION_ACT:
                //  38.321 Ch6.1.3.11
                mac_ce_len = 1;
                break;
            case DL_SCH_LCID_SCell_ACT_4_OCT:
                //  38.321 Ch6.1.3.10
                mac_ce_len = 4;
                break;
            case DL_SCH_LCID_SCell_ACT_1_OCT:
                //  38.321 Ch6.1.3.10
                mac_ce_len = 1;
                break;
            case DL_SCH_LCID_L_DRX:
                //  38.321 Ch6.1.3.6
                //  fixed length but not yet specify.
                mac_ce_len = 0;
                break;
            case DL_SCH_LCID_DRX:
                //  38.321 Ch6.1.3.5
                //  fixed length but not yet specify.
                mac_ce_len = 0;
                break;
            case DL_SCH_LCID_TA_COMMAND:
                //  38.321 Ch6.1.3.4
                mac_ce_len = 1;

                /*uint8_t ta_command = ((NR_MAC_CE_TA *)pdu_ptr)[1].TA_COMMAND;
                uint8_t tag_id = ((NR_MAC_CE_TA *)pdu_ptr)[1].TAGID;*/

                ul_time_alignment->apply_ta = 1;
                ul_time_alignment->ta_command = ((NR_MAC_CE_TA *)pdu_ptr)[1].TA_COMMAND;
                ul_time_alignment->tag_id = ((NR_MAC_CE_TA *)pdu_ptr)[1].TAGID;

                /*
                #ifdef DEBUG_HEADER_PARSING
                LOG_D(MAC, "[UE] CE %d : UE Timing Advance : %d\n", i, pdu_ptr[1]);
                #endif
                */

                LOG_D(MAC, "Received TA_COMMAND %u TAGID %u CC_id %d\n", ul_time_alignment->ta_command, ul_time_alignment->tag_id, CC_id);

                break;
            case DL_SCH_LCID_CON_RES_ID:
                //  38.321 Ch6.1.3.3
                // WIP todo: handle CCCH_pdu
                mac_ce_len = 6;
                
                LOG_I(MAC, "[UE %d][RAPROC] Frame %d : received contention resolution msg: %x.%x.%x.%x.%x.%x, Terminating RA procedure\n", module_idP, frameP, pdu_ptr[0], pdu_ptr[1], pdu_ptr[2], pdu_ptr[3], pdu_ptr[4], pdu_ptr[5]);

                if (mac->RA_active == 1) {
                  LOG_I(MAC, "[UE %d][RAPROC] Frame %d : Clearing RA_active flag\n", module_idP, frameP);
                  mac->RA_active = 0;
                   // // check if RA procedure has finished completely (no contention)
                   // tx_sdu = &mac->CCCH_pdu.payload[3];
                   // //Note: 3 assumes sizeof(SCH_SUBHEADER_SHORT) + PADDING CE, which is when UL-Grant has TBS >= 9 (64 bits)
                   // // (other possibility is 1 for TBS=7 (SCH_SUBHEADER_FIXED), or 2 for TBS=8 (SCH_SUBHEADER_FIXED+PADDING or //  SCH_SUBHEADER_SHORT)
                   // for (i = 0; i < 6; i++)
                   //   if (tx_sdu[i] != payload_ptr[i]) {
                   //     LOG_E(MAC, "[UE %d][RAPROC] Contention detected, RA failed\n", module_idP);
                   //     nr_ra_failed(module_idP, CC_id, eNB_index);
                   //     mac->RA_contention_resolution_timer_active = 0;
                   //     return;
                   //   }
                  LOG_I(MAC, "[UE %d][RAPROC] Frame %d : Cleared contention resolution timer. Set C-RNTI to TC-RNTI\n",
                    module_idP,
                    frameP);
                  mac->RA_contention_resolution_timer_active = 0;
                  nr_ra_succeeded(module_idP, CC_id, gNB_index);
                  mac->crnti = mac->t_crnti;
                  mac->t_crnti = 0;
                  mac->ra_state = RA_SUCCEEDED;
                }
                break;
            case DL_SCH_LCID_PADDING:
                done = 1;
                //  end of MAC PDU, can ignore the rest.
                break;

            //  MAC SDU

            case DL_SCH_LCID_DCCH:
                //  check if LCID is valid at current time.

            case DL_SCH_LCID_DCCH1:
                //  check if LCID is valid at current time.

            default:
                //  check if LCID is valid at current time.
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    //mac_sdu_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                    mac_sdu_len = ((uint16_t)(((NR_MAC_SUBHEADER_LONG *) pdu_ptr)->L1 & 0x7f) << 8)
                    | ((uint16_t)((NR_MAC_SUBHEADER_LONG *) pdu_ptr)->L2 & 0xff);

                } else {
                  mac_sdu_len = (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                  mac_subheader_len = 2;
                }

                LOG_D(MAC, "[UE %d] Frame %d : DLSCH -> DL-DTCH %d (gNB %d, %d bytes)\n", module_idP, frameP, rx_lcid, gNB_index, mac_sdu_len);

                #if defined(ENABLE_MAC_PAYLOAD_DEBUG)
                    LOG_T(MAC, "[UE %d] First 32 bytes of DLSCH : \n", module_idP);

                    for (i = 0; i < 32; i++)
                      LOG_T(MAC, "%x.", (pdu_ptr + mac_subheader_len)[i]);

                    LOG_T(MAC, "\n");
                #endif

                if (IS_SOFTMODEM_NOS1){
                  if (rx_lcid < NB_RB_MAX && rx_lcid >= DL_SCH_LCID_DTCH) {

                    mac_rlc_data_ind(module_idP,
                                     mac->crnti,
                                     gNB_index,
                                     frameP,
                                     ENB_FLAG_NO,
                                     MBMS_FLAG_NO,
                                     rx_lcid,
                                     (char *) (pdu_ptr + mac_subheader_len),
                                     mac_sdu_len,
                                     1,
                                     NULL);
                  } else {
                    LOG_E(MAC, "[UE %d] Frame %d : unknown LCID %d (gNB %d)\n", module_idP, frameP, rx_lcid, gNB_index);
                  }
                }

            break;
        }
        pdu_ptr += ( mac_subheader_len + mac_ce_len + mac_sdu_len );
        pdu_len -= ( mac_subheader_len + mac_ce_len + mac_sdu_len );
        if (pdu_len < 0)
          LOG_E(MAC, "[MAC] nr_ue_process_mac_pdu, residual mac pdu length %d < 0!\n", pdu_len);
    }
}

////////////////////////////////////////////////////////
/////* ULSCH MAC PDU generation (6.1.2 TS 38.321) */////
////////////////////////////////////////////////////////

uint16_t nr_generate_ulsch_pdu(uint8_t *sdus_payload,
                                    uint8_t *pdu,
                                    uint8_t num_sdus,
                                    uint16_t *sdu_lengths,
                                    uint8_t *sdu_lcids,
                                    uint8_t power_headroom,
                                    uint16_t crnti,
                                    uint16_t truncated_bsr,
                                    uint16_t short_bsr,
                                    uint16_t long_bsr,
                                    unsigned short post_padding,
                                    uint16_t buflen) {

  NR_MAC_SUBHEADER_FIXED *mac_pdu_ptr = (NR_MAC_SUBHEADER_FIXED *) pdu;
  unsigned char last_size = 0, i, mac_header_control_elements[16], *ce_ptr, bsr = 0;
  int mac_ce_size;
  uint16_t offset = 0;

  LOG_D(MAC, "[UE] Generating ULSCH PDU : num_sdus %d\n", num_sdus);

  #ifdef DEBUG_HEADER_PARSING

    for (i = 0; i < num_sdus; i++)
      LOG_D(MAC, "[UE] MAC subPDU %d (lcid %d length %d bytes \n", i, sdu_lcids[i], sdu_lengths[i]);

  #endif

  // Generating UL MAC subPDUs including MAC SDU and subheader

  for (i = 0; i < num_sdus; i++) {
    LOG_D(MAC, "[UE] Generating UL MAC subPDUs for SDU with lenght %d ( num_sdus %d )\n", sdu_lengths[i], num_sdus);

    if (sdu_lcids[i] != UL_SCH_LCID_CCCH){
      if (sdu_lengths[i] < 128) {
        ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->R = 0;
        ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->F = 0;
        ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->LCID = sdu_lcids[i];
        ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->L = (unsigned char) sdu_lengths[i];
        last_size = 2;
      } else {
        ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->R = 0;
        ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->F = 1;
        ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->LCID = sdu_lcids[i];
        ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->L1 = ((unsigned short) sdu_lengths[i] >> 8) & 0x7f;
        ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->L2 = (unsigned short) sdu_lengths[i] & 0xff;
        last_size = 3;
      }
    } else { // UL CCCH SDU
      mac_pdu_ptr->R = 0;
      mac_pdu_ptr->LCID = sdu_lcids[i];
    }

    mac_pdu_ptr += last_size;

    // cycle through SDUs, compute each relevant and place ulsch_buffer in
    memcpy((void *) mac_pdu_ptr, (void *) sdus_payload, sdu_lengths[i]);
    sdus_payload += sdu_lengths[i]; 
    mac_pdu_ptr  += sdu_lengths[i];
  }

  // Generating UL MAC subPDUs including MAC CEs (MAC CE and subheader)

  ce_ptr = &mac_header_control_elements[0];

  if (power_headroom) {
    // MAC CE fixed subheader
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = UL_SCH_LCID_SINGLE_ENTRY_PHR;
    mac_pdu_ptr++;

    // PHR MAC CE (1 octet)
    ((NR_SINGLE_ENTRY_PHR_MAC_CE *) ce_ptr)->PH = power_headroom;
    ((NR_SINGLE_ENTRY_PHR_MAC_CE *) ce_ptr)->R1 = 0;
    ((NR_SINGLE_ENTRY_PHR_MAC_CE *) ce_ptr)->PCMAX = 0; // todo
    ((NR_SINGLE_ENTRY_PHR_MAC_CE *) ce_ptr)->R2 = 0;

    mac_ce_size = sizeof(NR_SINGLE_ENTRY_PHR_MAC_CE);

    // Copying bytes for PHR MAC CEs to the mac pdu pointer
    memcpy((void *) mac_pdu_ptr, (void *) ce_ptr, mac_ce_size);
    ce_ptr += mac_ce_size;
    mac_pdu_ptr += (unsigned char) mac_ce_size;
  }

  if (crnti) {
    // MAC CE fixed subheader
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = CRNTI;
    mac_pdu_ptr++;

    // C-RNTI MAC CE (2 octets)
    * (uint16_t *) ce_ptr = crnti;
    mac_ce_size = sizeof(uint16_t);

    // Copying bytes for CRNTI MAC CE to the mac pdu pointer
    memcpy((void *) mac_pdu_ptr, (void *) ce_ptr, mac_ce_size);
    ce_ptr += mac_ce_size;
    mac_pdu_ptr += (unsigned char) mac_ce_size;
  }

  if (truncated_bsr) {
    // MAC CE fixed subheader
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = UL_SCH_LCID_S_TRUNCATED_BSR;
    mac_pdu_ptr++;

    // Short truncated BSR MAC CE (1 octet)
    ((NR_BSR_SHORT_TRUNCATED *) ce_ptr)-> Buffer_size = truncated_bsr;
    ((NR_BSR_SHORT_TRUNCATED *) ce_ptr)-> LcgID = 0; // todo
    mac_ce_size = sizeof(NR_BSR_SHORT_TRUNCATED);

    bsr = 1 ;
  } else if (short_bsr) {
    // MAC CE fixed subheader
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = UL_SCH_LCID_S_BSR;
    mac_pdu_ptr++;

    // Short truncated BSR MAC CE (1 octet)
    ((NR_BSR_SHORT *) ce_ptr)->Buffer_size = short_bsr;
    ((NR_BSR_SHORT *) ce_ptr)->LcgID = 0; // todo
    mac_ce_size = sizeof(NR_BSR_SHORT);

    bsr = 1 ;
  } else if (long_bsr) {
    // MAC CE variable subheader
    // todo ch 6.1.3.1. TS 38.321
    // ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->R = 0;
    // ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->F = 0;
    // ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->LCID = UL_SCH_LCID_L_BSR;
    // ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->L = 0;
    // last_size = 2;
    // mac_pdu_ptr += last_size;

    // Short truncated BSR MAC CE (1 octet)
    // ((NR_BSR_LONG *) ce_ptr)->Buffer_size0 = short_bsr;
    // ((NR_BSR_LONG *) ce_ptr)->LCGID0 = 0;
    // mac_ce_size = sizeof(NR_BSR_LONG); // size is variable
  }

  if (bsr){
    // Copying bytes for BSR MAC CE to the mac pdu pointer
    memcpy((void *) mac_pdu_ptr, (void *) ce_ptr, mac_ce_size);
    ce_ptr += mac_ce_size;
    mac_pdu_ptr += (unsigned char) mac_ce_size;
  }

  // compute offset before adding padding (if necessary)
  offset = ((unsigned char *) mac_pdu_ptr - pdu);
  uint16_t padding_bytes = 0; 

  if(buflen > 0) // If the buflen is provided
    padding_bytes = buflen - offset;

  // Compute final offset for padding
  if (post_padding > 0 || padding_bytes>0) {
    ((NR_MAC_SUBHEADER_FIXED *) mac_pdu_ptr)->R = 0;
    ((NR_MAC_SUBHEADER_FIXED *) mac_pdu_ptr)->LCID = UL_SCH_LCID_PADDING;
    mac_pdu_ptr++;
  } else {            
    // no MAC subPDU with padding
  }

  // compute final offset
  offset = ((unsigned char *) mac_pdu_ptr - pdu);

  //printf("Offset %d \n", ((unsigned char *) mac_pdu_ptr - pdu));

  return offset;
}

uint8_t
nr_ue_get_sdu(module_id_t module_idP, int CC_id, frame_t frameP,
           sub_frame_t subframe, uint8_t eNB_index,
           uint8_t *ulsch_buffer, uint16_t buflen, uint8_t *access_mode) {
  uint8_t total_rlc_pdu_header_len = 0;
  int16_t buflen_remain = 0;
  uint8_t lcid = 0;
  uint16_t sdu_lengths[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  uint8_t sdu_lcids[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  uint16_t payload_offset = 0, num_sdus = 0;
  uint8_t ulsch_sdus[MAX_ULSCH_PAYLOAD_BYTES];
  uint16_t sdu_length_total = 0;
  //unsigned short post_padding = 0;
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_idP);

  rlc_buffer_occupancy_t lcid_buffer_occupancy_old =
    0, lcid_buffer_occupancy_new = 0;
  LOG_D(MAC,
        "[UE %d] MAC PROCESS UL TRANSPORT BLOCK at frame%d subframe %d TBS=%d\n",
        module_idP, frameP, subframe, buflen);
  AssertFatal(CC_id == 0,
              "Transmission on secondary CCs is not supported yet\n");
#if UE_TIMING_TRACE
  start_meas(&UE_mac_inst[module_idP].tx_ulsch_sdu);
#endif

  // Check for DCCH first
  // TO DO: Multiplex in the order defined by the logical channel prioritization
  for (lcid = UL_SCH_LCID_SRB1;
       lcid < NR_MAX_NUM_LCID; lcid++) {

      lcid_buffer_occupancy_old = mac_rlc_get_buffer_occupancy_ind(module_idP, mac->crnti, eNB_index, frameP, subframe, ENB_FLAG_NO, lcid);
      lcid_buffer_occupancy_new = lcid_buffer_occupancy_old;

      if(lcid_buffer_occupancy_new){

        buflen_remain =
          buflen - (total_rlc_pdu_header_len + sdu_length_total + MAX_RLC_SDU_SUBHEADER_SIZE);
        LOG_D(MAC,
              "[UE %d] Frame %d : UL-DXCH -> ULSCH, RLC %d has %d bytes to "
              "send (Transport Block size %d SDU Length Total %d , mac header len %d, buflen_remain %d )\n", //BSR byte before Tx=%d
              module_idP, frameP, lcid, lcid_buffer_occupancy_new,
              buflen, sdu_length_total,
              total_rlc_pdu_header_len, buflen_remain); // ,nr_ue_mac_inst->scheduling_info.BSR_bytes[nr_ue_mac_inst->scheduling_info.LCGID[lcid]]

        while(buflen_remain > 0 && lcid_buffer_occupancy_new){

        sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP,
                                mac->crnti,
                                eNB_index,
                                frameP,
                                ENB_FLAG_NO,
                                MBMS_FLAG_NO,
                                lcid,
                                buflen_remain,
                                (char *)&ulsch_sdus[sdu_length_total],0,
                                0);

        AssertFatal(buflen_remain >= sdu_lengths[num_sdus],
                    "LCID=%d RLC has segmented %d bytes but MAC has max=%d\n",
                    lcid, sdu_lengths[num_sdus], buflen_remain);

        if (sdu_lengths[num_sdus]) {
          sdu_length_total += sdu_lengths[num_sdus];
          sdu_lcids[num_sdus] = lcid;

          total_rlc_pdu_header_len += MAX_RLC_SDU_SUBHEADER_SIZE; //rlc_pdu_header_len_last;

          //Update number of SDU
          num_sdus++;
        }

        /* Get updated BO after multiplexing this PDU */
        lcid_buffer_occupancy_new = mac_rlc_get_buffer_occupancy_ind(module_idP,
                                                                     mac->crnti,
                                                                     eNB_index,
                                                                     frameP,
                                                                     subframe,
                                                                     ENB_FLAG_NO,
                                                                     lcid);
        buflen_remain =
                  buflen - (total_rlc_pdu_header_len + sdu_length_total + MAX_RLC_SDU_SUBHEADER_SIZE);
        }
  }

}

  // Generate ULSCH PDU
  if (num_sdus>0) {
  payload_offset = nr_generate_ulsch_pdu(ulsch_sdus,
                                         ulsch_buffer,  // mac header
                                         num_sdus,  // num sdus
                                         sdu_lengths, // sdu length
                                         sdu_lcids, // sdu lcid
                                         0, // power_headroom
                                         mac->crnti, // crnti
                                         0, // truncated_bsr
                                         0, // short_bsr
                                         0, // long_bsr
                                         0, // post_padding 
                                         buflen);  // TBS in bytes
  }
  else
	  return 0;

  // Padding: fill remainder of ULSCH with 0
  if (buflen - payload_offset > 0){
  	  for (int j = payload_offset; j < buflen; j++)
  		  ulsch_buffer[j] = 0;
  }

#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
  LOG_I(MAC, "Printing UL MAC payload UE side, payload_offset: %d \n", payload_offset);
  for (int i = 0; i < buflen ; i++) {
	  //harq_process_ul_ue->a[i] = (unsigned char) rand();
	  //printf("a[%d]=0x%02x\n",i,harq_process_ul_ue->a[i]);
	  printf("%02x ",(unsigned char)ulsch_buffer[i]);
  }
  printf("\n");
#endif

  return 1;
}
