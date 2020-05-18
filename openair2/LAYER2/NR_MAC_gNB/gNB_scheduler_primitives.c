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

/*! \file gNB_scheduler_primitives.c
 * \brief primitives used by gNB for BCH, RACH, ULSCH, DLSCH scheduling
 * \author  Raymond Knopp, Guy De Souza
 * \date 2018, 2019
 * \email: knopp@eurecom.fr, desouza@eurecom.fr
 * \version 1.0
 * \company Eurecom
 * @ingroup _mac

 */

#include "assertions.h"

#include "LAYER2/MAC/mac.h"
#include "LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"

#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/nr/nr_common.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/LTE/rrc_extern.h"
#include "RRC/NR/nr_rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#include "intertask_interface.h"

#include "T.h"
#include "NR_PDCCH-ConfigCommon.h"
#include "NR_ControlResourceSet.h"
#include "NR_SearchSpace.h"

#include "nfapi_nr_interface.h"

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_gNB_SCHEDULER 1


#include "common/ran_context.h"

extern RAN_CONTEXT_t RC;

extern int n_active_slices;

  // Note the 2 scs values in the table names represent resp. scs_common and pdcch_scs
/// LUT for the number of symbols in the coreset indexed by coreset index (4 MSB rmsi_pdcch_config)
uint8_t nr_coreset_nsymb_pdcch_type_0_scs_15_15[15] = {2,2,2,3,3,3,1,1,2,2,3,3,1,2,3};
uint8_t nr_coreset_nsymb_pdcch_type_0_scs_15_30[14] = {2,2,2,2,3,3,3,3,1,1,2,2,3,3};
uint8_t nr_coreset_nsymb_pdcch_type_0_scs_30_15_b40Mhz[9] = {1,1,2,2,3,3,1,2,3};
uint8_t nr_coreset_nsymb_pdcch_type_0_scs_30_15_a40Mhz[9] = {1,2,3,1,1,2,2,3,3};
uint8_t nr_coreset_nsymb_pdcch_type_0_scs_30_30_b40Mhz[16] = {2,2,2,2,2,3,3,3,3,3,1,1,1,2,2,2}; // below 40Mhz bw
uint8_t nr_coreset_nsymb_pdcch_type_0_scs_30_30_a40Mhz[10] = {2,2,3,3,1,1,2,2,3,3}; // above 40Mhz bw
uint8_t nr_coreset_nsymb_pdcch_type_0_scs_120_60[12] = {1,1,2,2,3,3,1,2,1,1,1,1};

/// LUT for the number of RBs in the coreset indexed by coreset index
uint8_t nr_coreset_rb_offset_pdcch_type_0_scs_15_15[15] = {0,2,4,0,2,4,12,16,12,16,12,16,38,38,38};
uint8_t nr_coreset_rb_offset_pdcch_type_0_scs_15_30[14] = {5,6,7,8,5,6,7,8,18,20,18,20,18,20};
uint8_t nr_coreset_rb_offset_pdcch_type_0_scs_30_15_b40Mhz[9] = {2,6,2,6,2,6,28,28,28};
uint8_t nr_coreset_rb_offset_pdcch_type_0_scs_30_15_a40Mhz[9] = {4,4,4,0,56,0,56,0,56};
uint8_t nr_coreset_rb_offset_pdcch_type_0_scs_30_30_b40Mhz[16] = {0,1,2,3,4,0,1,2,3,4,12,14,16,12,14,16};
uint8_t nr_coreset_rb_offset_pdcch_type_0_scs_30_30_a40Mhz[10] = {0,4,0,4,0,28,0,28,0,28};
int8_t  nr_coreset_rb_offset_pdcch_type_0_scs_120_60[12] = {0,8,0,8,0,8,28,28,-1,49,-1,97};
int8_t  nr_coreset_rb_offset_pdcch_type_0_scs_120_120[8] = {0,4,14,14,-1,24,-1,48};
int8_t  nr_coreset_rb_offset_pdcch_type_0_scs_240_120[8] = {0,8,0,8,-1,25,-1,49};

/// LUT for monitoring occasions param O indexed by ss index (4 LSB rmsi_pdcch_config)
  // Note: scaling is used to avoid decimal values for O and M, original values commented
uint8_t nr_ss_param_O_type_0_mux1_FR1[16] = {0,0,2,2,5,5,7,7,0,5,0,0,2,2,5,5};
uint8_t nr_ss_param_O_type_0_mux1_FR2[14] = {0,0,5,5,5,5,0,5,5,15,15,15,0,5}; //{0,0,2.5,2.5,5,5,0,2.5,5,7.5,7.5,7.5,0,5}
uint8_t nr_ss_scale_O_mux1_FR2[14] = {0,0,1,1,0,0,0,1,0,1,1,1,0,0};

/// LUT for number of SS sets per slot indexed by ss index
uint8_t nr_ss_sets_per_slot_type_0_FR1[16] = {1,2,1,2,1,2,1,2,1,1,1,1,1,1,1,1};
uint8_t nr_ss_sets_per_slot_type_0_FR2[14] = {1,2,1,2,1,2,2,2,2,1,2,2,1,1};

/// LUT for monitoring occasions param M indexed by ss index
uint8_t nr_ss_param_M_type_0_mux1_FR1[16] = {1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1}; //{1,0.5,1,0.5,1,0.5,1,0.5,2,2,1,1,1,1,1,1}
uint8_t nr_ss_scale_M_mux1_FR1[16] = {0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0};
uint8_t nr_ss_param_M_type_0_mux1_FR2[14] = {1,1,1,1,1,1,1,1,1,1,1,1,2,2}; //{1,0.5,1,0.5,1,0.5,0.5,0.5,0.5,1,0.5,0.5,2,2}
uint8_t nr_ss_scale_M_mux1_FR2[14] = {0,1,0,1,0,1,1,1,1,0,1,1,0,0};

/// LUT for SS first symbol index indexed by ss index
uint8_t nr_ss_first_symb_idx_type_0_mux1_FR1[8] = {0,0,1,2,1,2,1,2};
  // Mux pattern type 2
uint8_t nr_ss_first_symb_idx_scs_120_60_mux2[4] = {0,1,6,7};
uint8_t nr_ss_first_symb_idx_scs_240_120_set1_mux2[6] = {0,1,2,3,0,1};
  // Mux pattern type 3
uint8_t nr_ss_first_symb_idx_scs_120_120_mux3[4] = {4,8,2,6};

/// Search space max values indexed by scs
uint8_t nr_max_number_of_candidates_per_slot[4] = {44, 36, 22, 20};
uint8_t nr_max_number_of_cces_per_slot[4] = {56, 56, 48, 32};

static inline uint8_t get_max_candidates(uint8_t scs) {
  AssertFatal(scs<4, "Invalid PDCCH subcarrier spacing %d\n", scs);
  return (nr_max_number_of_candidates_per_slot[scs]);
}

static inline uint8_t get_max_cces(uint8_t scs) {
  AssertFatal(scs<4, "Invalid PDCCH subcarrier spacing %d\n", scs);
  return (nr_max_number_of_cces_per_slot[scs]);
} 

int allocate_nr_CCEs(gNB_MAC_INST *nr_mac,
		     int bwp_id,
		     int coreset_id,
		     int aggregation,
		     int search_space, // 0 common, 1 ue-specific
		     int UE_id,
		     int m
		     ) {
  // uncomment these when we allocate for common search space
  //  NR_COMMON_channels_t                *cc      = nr_mac->common_channels;
  //  NR_ServingCellConfigCommon_t        *scc     = cc->ServingCellConfigCommon;

  NR_UE_list_t *UE_list = &nr_mac->UE_list;

  NR_BWP_Downlink_t *bwp;
  NR_CellGroupConfig_t *secondaryCellGroup;

  NR_ControlResourceSet_t *coreset;

  if (search_space == 1) {
    AssertFatal(UE_list->active[UE_id] >=0,"UE_id %d is not active\n",UE_id);
    secondaryCellGroup = UE_list->secondaryCellGroup[UE_id];
    bwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1];
    coreset = bwp->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList->list.array[coreset_id];
  }
  else {
    AssertFatal(1==0,"Add code for common search space\n");
  }

  int *cce_list = nr_mac->cce_list[bwp_id][coreset_id];


  int n_rb=0;
  for (int i=0;i<6;i++)
    for (int j=0;j<8;j++) {
      n_rb+=((coreset->frequencyDomainResources.buf[i]>>j)&1);
    }
  n_rb*=6;

  uint16_t N_reg = n_rb * coreset->duration;
  uint16_t Y=0, N_cce, M_s_max, n_CI=0;
  uint16_t n_RNTI = search_space == 1 ? UE_list->rnti[UE_id]:0;
  uint32_t A[3]={39827,39829,39839};

  N_cce = N_reg / NR_NB_REG_PER_CCE;

  M_s_max = (aggregation==4)?4:(aggregation==8)?2:1;

  if (search_space == 1) {
    Y = (A[0]*n_RNTI)%65537; // Candidate 0, antenna port 0
  }
  int first_cce = aggregation * (( Y + (m*N_cce)/(aggregation*M_s_max) + n_CI ) % CEILIDIV(N_cce,aggregation));

  for (int i=0;i<aggregation;i++) 
    if (cce_list[first_cce+i] != 0) return(-1);
  
  for (int i=0;i<aggregation;i++) cce_list[first_cce+i] = 1;

  return(first_cce);

}

void nr_configure_css_dci_initial(nfapi_nr_dl_tti_pdcch_pdu_rel15_t* pdcch_pdu,
				  nr_scs_e scs_common,
				  nr_scs_e pdcch_scs,
				  nr_frequency_range_e freq_range,
				  uint8_t rmsi_pdcch_config,
				  uint8_t ssb_idx,
				  uint8_t k_ssb,
				  uint16_t sfn_ssb,
				  uint8_t n_ssb, /*slot index overlapping the corresponding SSB index*/
				  uint16_t nb_slots_per_frame,
				  uint16_t N_RB)
{
  //  uint8_t O, M;
  //  uint8_t ss_idx = rmsi_pdcch_config&0xf;
  //  uint8_t cset_idx = (rmsi_pdcch_config>>4)&0xf;
  //  uint8_t mu = scs_common;
  //  uint8_t O_scale=0, M_scale=0; // used to decide if the values of O and M need to be divided by 2

  AssertFatal(1==0,"todo\n");
  /*
  /// Coreset params
  switch(scs_common) {

    case kHz15:

      switch(pdcch_scs) {
        case kHz15:
          AssertFatal(cset_idx<15,"Coreset index %d reserved for scs kHz15/kHz15\n", cset_idx);
          pdcch_pdu->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
          pdcch_pdu->n_rb = (cset_idx < 6)? 24 : (cset_idx < 12)? 48 : 96;
          pdcch_pdu->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_15_15[cset_idx];
          pdcch_pdu->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_15_15[cset_idx];
        break;

        case kHz30:
          AssertFatal(cset_idx<14,"Coreset index %d reserved for scs kHz15/kHz30\n", cset_idx);
          pdcch_pdu->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
          pdcch_pdu->n_rb = (cset_idx < 8)? 24 : 48;
          pdcch_pdu->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_15_30[cset_idx];
          pdcch_pdu->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_15_15[cset_idx];
        break;

        default:
            AssertFatal(1==0,"Invalid scs_common/pdcch_scs combination %d/%d \n", scs_common, pdcch_scs);

      }
      break;

    case kHz30:

      if (N_RB < 106) { // Minimum 40Mhz bandwidth not satisfied
        switch(pdcch_scs) {
          case kHz15:
            AssertFatal(cset_idx<9,"Coreset index %d reserved for scs kHz30/kHz15\n", cset_idx);
            pdcch_pdu->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
            pdcch_pdu->n_rb = (cset_idx < 10)? 48 : 96;
            pdcch_pdu->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_30_15_b40Mhz[cset_idx];
            pdcch_pdu->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_30_15_b40Mhz[cset_idx];
          break;

          case kHz30:
            pdcch_pdu->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
            pdcch_pdu->n_rb = (cset_idx < 6)? 24 : 48;
            pdcch_pdu->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_30_30_b40Mhz[cset_idx];
            pdcch_pdu->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_30_30_b40Mhz[cset_idx];
          break;

          default:
            AssertFatal(1==0,"Invalid scs_common/pdcch_scs combination %d/%d \n", scs_common, pdcch_scs);
        }
      }

      else { // above 40Mhz
        switch(pdcch_scs) {
          case kHz15:
            AssertFatal(cset_idx<9,"Coreset index %d reserved for scs kHz30/kHz15\n", cset_idx);
            pdcch_pdu->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
            pdcch_pdu->n_rb = (cset_idx < 3)? 48 : 96;
            pdcch_pdu->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_30_15_a40Mhz[cset_idx];
            pdcch_pdu->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_30_15_a40Mhz[cset_idx];
          break;

          case kHz30:
            AssertFatal(cset_idx<10,"Coreset index %d reserved for scs kHz30/kHz30\n", cset_idx);
            pdcch_pdu->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
            pdcch_pdu->n_rb = (cset_idx < 4)? 24 : 48;
            pdcch_pdu->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_30_30_a40Mhz[cset_idx];
            pdcch_pdu->rb_offset =  nr_coreset_rb_offset_pdcch_type_0_scs_30_30_a40Mhz[cset_idx];
          break;

          default:
            AssertFatal(1==0,"Invalid scs_common/pdcch_scs combination %d/%d \n", scs_common, pdcch_scs);
        }
      }
      break;

    case kHz120:
      switch(pdcch_scs) {
        case kHz60:
          AssertFatal(cset_idx<12,"Coreset index %d reserved for scs kHz120/kHz60\n", cset_idx);
          pdcch_pdu->mux_pattern = (cset_idx < 8)?NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1 : NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE2;
          pdcch_pdu->n_rb = (cset_idx < 6)? 48 : (cset_idx < 8)? 96 : (cset_idx < 10)? 48 : 96;
          pdcch_pdu->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_120_60[cset_idx];
          pdcch_pdu->rb_offset = (nr_coreset_rb_offset_pdcch_type_0_scs_120_60[cset_idx]>0)?nr_coreset_rb_offset_pdcch_type_0_scs_120_60[cset_idx] :
          (k_ssb == 0)? -41 : -42;
        break;

        case kHz120:
          AssertFatal(cset_idx<8,"Coreset index %d reserved for scs kHz120/kHz120\n", cset_idx);
          pdcch_pdu->mux_pattern = (cset_idx < 4)?NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1 : NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE3;
          pdcch_pdu->n_rb = (cset_idx < 2)? 24 : (cset_idx < 4)? 48 : (cset_idx < 6)? 24 : 48;
          pdcch_pdu->n_symb = (cset_idx == 2)? 1 : 2;
          pdcch_pdu->rb_offset = (nr_coreset_rb_offset_pdcch_type_0_scs_120_120[cset_idx]>0)? nr_coreset_rb_offset_pdcch_type_0_scs_120_120[cset_idx] :
          (k_ssb == 0)? -20 : -21;
        break;

        default:
            AssertFatal(1==0,"Invalid scs_common/pdcch_scs combination %d/%d \n", scs_common, pdcch_scs);
      }
    break;

    case kHz240:
    switch(pdcch_scs) {
      case kHz60:
        AssertFatal(cset_idx<4,"Coreset index %d reserved for scs kHz240/kHz60\n", cset_idx);
        pdcch_pdu->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
        pdcch_pdu->n_rb = 96;
        pdcch_pdu->n_symb = (cset_idx < 2)? 1 : 2;
        pdcch_pdu->rb_offset = (cset_idx&1)? 16 : 0;
      break;

      case kHz120:
        AssertFatal(cset_idx<8,"Coreset index %d reserved for scs kHz240/kHz120\n", cset_idx);
        pdcch_pdu->mux_pattern = (cset_idx < 4)? NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1 : NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE2;
        pdcch_pdu->n_rb = (cset_idx < 4)? 48 : (cset_idx < 6)? 24 : 48;
        pdcch_pdu->n_symb = ((cset_idx==2)||(cset_idx==3))? 2 : 1;
        pdcch_pdu->rb_offset = (nr_coreset_rb_offset_pdcch_type_0_scs_240_120[cset_idx]>0)? nr_coreset_rb_offset_pdcch_type_0_scs_240_120[cset_idx] :
        (k_ssb == 0)? -41 : -42;
      break;

      default:
          AssertFatal(1==0,"Invalid scs_common/pdcch_scs combination %d/%d \n", scs_common, pdcch_scs);
    }
    break;

  default:
    AssertFatal(1==0,"Invalid common subcarrier spacing %d\n", scs_common);

  }

  /// Search space params
  switch(pdcch_pdu->mux_pattern) {

    case NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1:
      if (freq_range == nr_FR1) {
        O = nr_ss_param_O_type_0_mux1_FR1[ss_idx];
        pdcch_pdu->nb_ss_sets_per_slot = nr_ss_sets_per_slot_type_0_FR1[ss_idx];
        M = nr_ss_param_M_type_0_mux1_FR1[ss_idx];
        M_scale = nr_ss_scale_M_mux1_FR1[ss_idx];
        pdcch_pdu->first_symbol = (ss_idx < 8)? ( (ssb_idx&1)? pdcch_pdu->n_symb : 0 ) : nr_ss_first_symb_idx_type_0_mux1_FR1[ss_idx - 8];
      }

      else {
        AssertFatal(ss_idx<14 ,"Invalid search space index for multiplexing type 1 and FR2 %d\n", ss_idx);
        O = nr_ss_param_O_type_0_mux1_FR2[ss_idx];
        O_scale = nr_ss_scale_O_mux1_FR2[ss_idx];
        pdcch_pdu->nb_ss_sets_per_slot = nr_ss_sets_per_slot_type_0_FR2[ss_idx];
        M = nr_ss_param_M_type_0_mux1_FR2[ss_idx];
        M_scale = nr_ss_scale_M_mux1_FR2[ss_idx];
        pdcch_pdu->first_symbol = (ss_idx < 12)? ( (ss_idx&1)? 7 : 0 ) : 0;
      }
      pdcch_pdu->nb_slots = 2;
      pdcch_pdu->sfn_mod2 = (CEILIDIV( (((O<<mu)>>O_scale) + ((ssb_idx*M)>>M_scale)), nb_slots_per_frame ) & 1)? 1 : 0;
      pdcch_pdu->first_slot = (((O<<mu)>>O_scale) + ((ssb_idx*M)>>M_scale)) % nb_slots_per_frame;

    break;

    case NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE2:
      AssertFatal( ((scs_common==kHz120)&&(pdcch_scs==kHz60)) || ((scs_common==kHz240)&&(pdcch_scs==kHz120)),
      "Invalid scs_common/pdcch_scs combination %d/%d for Mux type 2\n", scs_common, pdcch_scs );
      AssertFatal(ss_idx==0, "Search space index %d reserved for scs_common/pdcch_scs combination %d/%d", ss_idx, scs_common, pdcch_scs);

      pdcch_pdu->nb_slots = 1;

      if ((scs_common==kHz120)&&(pdcch_scs==kHz60)) {
        pdcch_pdu->first_symbol = nr_ss_first_symb_idx_scs_120_60_mux2[ssb_idx&3];
        // Missing in pdcch_pdu sfn_C and n_C here and in else case
      }
      else {
        pdcch_pdu->first_symbol = ((ssb_idx&7)==4)?12 : ((ssb_idx&7)==4)?13 : nr_ss_first_symb_idx_scs_240_120_set1_mux2[ssb_idx&7]; //???
      }

    break;

    case NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE3:
      AssertFatal( (scs_common==kHz120)&&(pdcch_scs==kHz120),
      "Invalid scs_common/pdcch_scs combination %d/%d for Mux type 3\n", scs_common, pdcch_scs );
      AssertFatal(ss_idx==0, "Search space index %d reserved for scs_common/pdcch_scs combination %d/%d", ss_idx, scs_common, pdcch_scs);

      pdcch_pdu->first_symbol = nr_ss_first_symb_idx_scs_120_120_mux3[ssb_idx&3];

    break;

    default:
      AssertFatal(1==0, "Invalid SSB and coreset multiplexing pattern %d\n", pdcch_pdu->mux_pattern);
  }
  pdcch_pdu->config_type = NFAPI_NR_CSET_CONFIG_MIB_SIB1;
  pdcch_pdu->cr_mapping_type = NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED;
  pdcch_pdu->precoder_granularity = NFAPI_NR_CSET_SAME_AS_REG_BUNDLE;
  pdcch_pdu->reg_bundle_size = 6;
  pdcch_pdu->interleaver_size = 2;
  // set initial banwidth part to full bandwidth
  pdcch_pdu->n_RB_BWP = N_RB;

  */

}

void nr_configure_pdcch(nfapi_nr_dl_tti_pdcch_pdu_rel15_t* pdcch_pdu,
			int ss_type,
			NR_ServingCellConfigCommon_t *scc,
			NR_BWP_Downlink_t *bwp){
  
  if (bwp) { // This is not the InitialBWP
    /// coreset
    
    //ControlResourceSetId
    //  pdcch_pdu->config_type = NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG;
    AssertFatal(bwp->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList!=NULL,
		"controlResourceSetToAddModList is null\n");
    AssertFatal(bwp->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList->list.count>0,
		"controlResourceSetToAddModList is empty\n");
    NR_ControlResourceSet_t *coreset0 = bwp->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList->list.array[0];
    
    
    pdcch_pdu->BWPSize  = NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);
    pdcch_pdu->BWPStart = NRRIV2PRBOFFSET(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);
    pdcch_pdu->SubcarrierSpacing = bwp->bwp_Common->genericParameters.subcarrierSpacing;
    pdcch_pdu->CyclicPrefix = (bwp->bwp_Common->genericParameters.cyclicPrefix==NULL) ? 0 : *bwp->bwp_Common->genericParameters.cyclicPrefix;
    
    pdcch_pdu->DurationSymbols  = coreset0->duration;
    
    //frequencyDomainResources
    /*    uint8_t count=0, start=0, start_set=0;
    // find coreset descriptor
    
    uint64_t bitmap = (((uint64_t)coreset0->frequencyDomainResources.buf[0])<<37)|
	(((uint64_t)coreset0->frequencyDomainResources.buf[1])<<29)|
	(((uint64_t)coreset0->frequencyDomainResources.buf[2])<<21)|
	(((uint64_t)coreset0->frequencyDomainResources.buf[3])<<13)|
	(((uint64_t)coreset0->frequencyDomainResources.buf[4])<<5)|
	(((uint64_t)coreset0->frequencyDomainResources.buf[5])>>3);
	
	for (int i=0; i<45; i++)
	if ((bitmap>>(44-i))&1) {
	count++;
	if (!start_set) {
        start = i;
        start_set = 1;
	}
	}
	pdcch_pdu->rb_offset = 6*start;
	pdcch_pdu->n_rb = 6*count;
    */
    for (int i=0;i<6;i++)
      pdcch_pdu->FreqDomainResource[i] = coreset0->frequencyDomainResources.buf[i];
    //duration
    
    
    //cce-REG-MappingType
    pdcch_pdu->CceRegMappingType = coreset0->cce_REG_MappingType.present == NR_ControlResourceSet__cce_REG_MappingType_PR_interleaved?
      NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED : NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED;
    if (pdcch_pdu->CceRegMappingType == NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED) {
      pdcch_pdu->RegBundleSize = (coreset0->cce_REG_MappingType.choice.interleaved->reg_BundleSize == NR_ControlResourceSet__cce_REG_MappingType__interleaved__reg_BundleSize_n6) ? 6 : (2+coreset0->cce_REG_MappingType.choice.interleaved->reg_BundleSize);
      pdcch_pdu->InterleaverSize = (coreset0->cce_REG_MappingType.choice.interleaved->interleaverSize==NR_ControlResourceSet__cce_REG_MappingType__interleaved__interleaverSize_n6) ? 6 : (2+coreset0->cce_REG_MappingType.choice.interleaved->interleaverSize);
      AssertFatal(scc->physCellId != NULL,"scc->physCellId is null\n");
      pdcch_pdu->ShiftIndex = coreset0->cce_REG_MappingType.choice.interleaved->shiftIndex != NULL ? *coreset0->cce_REG_MappingType.choice.interleaved->shiftIndex : *scc->physCellId;
    }
    else {
      pdcch_pdu->RegBundleSize = 0;
      pdcch_pdu->InterleaverSize = 0;
      pdcch_pdu->ShiftIndex = 0;
    }

    pdcch_pdu->CoreSetType = 1; 
    
    //precoderGranularity
    pdcch_pdu->precoderGranularity = coreset0->precoderGranularity;
    
    //TCI states
    // 
    /*
    //TCI present
    if (coreset0->tci_PresentInDCI != NULL) {
    AssertFatal(coreset0->tci_StatesPDCCH_ToAddList != NULL,"tci_StatesPDCCH_ToAddList is null\n");
    AssertFatal(coreset0->tci_StatesPDCCH_ToAddList->list.count>0,"TCI state list is empty\n");
    for (int i=0;i<coreset0->tci_StatesPDCCH_ToAddList->list.count;i++) {
    
    }
    */
    
    for (int i=0;i<pdcch_pdu->numDlDci;i++) {
      //pdcch-DMRS-ScramblingID
      AssertFatal(coreset0->pdcch_DMRS_ScramblingID != NULL,"coreset0->pdcch_DMRS_ScramblingID is null\n");
      pdcch_pdu->ScramblingId[i] = *coreset0->pdcch_DMRS_ScramblingID;
    }    
    
    /// SearchSpace
    
    // first symbol
    //AssertFatal(pdcch_scs==kHz15, "PDCCH SCS above 15kHz not allowed if a symbol above 2 is monitored");
    int sps = bwp->bwp_Common->genericParameters.cyclicPrefix == NULL ? 14 : 12;
    
    AssertFatal(bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList!=NULL,"searchPsacesToAddModList is null\n");
    AssertFatal(bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.count>0,
		"searchPsacesToAddModList is empty\n");
    NR_SearchSpace_t *ss=NULL;
    int found=0;
    int target_ss = NR_SearchSpace__searchSpaceType_PR_common;
    if (ss_type == 1) { 
      target_ss = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
    } 
    
    for (int i=0;i<bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.count;i++) {
      ss=bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.array[i];
      AssertFatal(ss->controlResourceSetId != NULL,"ss->controlResourceSetId is null\n");
      AssertFatal(ss->searchSpaceType != NULL,"ss->searchSpaceType is null\n");
      if (*ss->controlResourceSetId == coreset0->controlResourceSetId && 
	  ss->searchSpaceType->present == target_ss) {
	found=1;
	break;
      }
    }
    AssertFatal(found==1,"Couldn't find a searchspace corresponding to coreset0\n");
    AssertFatal(ss->monitoringSymbolsWithinSlot!=NULL,"ss->monitoringSymbolsWithinSlot is null\n");
    AssertFatal(ss->monitoringSymbolsWithinSlot->buf!=NULL,"ss->monitoringSymbolsWithinSlot->buf is null\n");
    
    // for SPS=14 8 MSBs in positions 13 downto 6,  
    uint16_t monitoringSymbolsWithinSlot = (ss->monitoringSymbolsWithinSlot->buf[0]<<(sps-8)) | 
      (ss->monitoringSymbolsWithinSlot->buf[1]>>(16-sps));
    
    for (int i=0; i<sps; i++)
      if ((monitoringSymbolsWithinSlot>>(sps-1-i))&1) {
	pdcch_pdu->StartSymbolIndex=i;
	break;
      }
  }

  else { // this is for InitialBWP
    AssertFatal(1==0,"Fill in InitialBWP PDCCH configuration\n");
  }
}


// This function configures pucch pdu fapi structure
void nr_configure_pucch(nfapi_nr_pucch_pdu_t* pucch_pdu,
			NR_ServingCellConfigCommon_t *scc,
			NR_BWP_Uplink_t *bwp,
                        uint8_t pucch_resource,
                        uint16_t O_uci,
                        uint16_t O_ack,
                        uint8_t SR_flag) {

  NR_PUCCH_Config_t *pucch_Config;
  NR_PUCCH_Resource_t *pucchres;
  NR_PUCCH_ResourceSet_t *pucchresset;
  NR_PUCCH_FormatConfig_t *pucchfmt;
  NR_PUCCH_ResourceId_t *resource_id = NULL;

  long *id0 = NULL;
  int n_list, n_set;
  uint16_t N2,N3;
  int res_found = 0;

  pucch_pdu->bit_len_harq = O_ack;

  if (bwp) { // This is not the InitialBWP

    NR_PUSCH_Config_t *pusch_Config = bwp->bwp_Dedicated->pusch_Config->choice.setup;
    long *pusch_id = pusch_Config->dataScramblingIdentityPUSCH;

    if (pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA != NULL)
      id0 = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA->choice.setup->transformPrecodingDisabled->scramblingID0;
    if (pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB != NULL)
      id0 = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->transformPrecodingDisabled->scramblingID0;

    // hop flags and hopping id are valid for any BWP
    switch (bwp->bwp_Common->pucch_ConfigCommon->choice.setup->pucch_GroupHopping){
      case 0 :
        // if neither, both disabled
        pucch_pdu->group_hop_flag = 0;
        pucch_pdu->sequence_hop_flag = 0;
        break;
      case 1 :
        // if enable, group enabled
        pucch_pdu->group_hop_flag = 1;
        pucch_pdu->sequence_hop_flag = 0;
        break;
      case 2 :
        // if disable, sequence disabled
        pucch_pdu->group_hop_flag = 0;
        pucch_pdu->sequence_hop_flag = 1;
        break;
      default:
        AssertFatal(1==0,"Group hopping flag %ld undefined (0,1,2) \n", bwp->bwp_Common->pucch_ConfigCommon->choice.setup->pucch_GroupHopping);
    }

    if (bwp->bwp_Common->pucch_ConfigCommon->choice.setup->hoppingId != NULL)
      pucch_pdu->hopping_id = *bwp->bwp_Common->pucch_ConfigCommon->choice.setup->hoppingId;
    else
      pucch_pdu->hopping_id = *scc->physCellId;

    pucch_pdu->bwp_size  = NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);
    pucch_pdu->bwp_start = NRRIV2PRBOFFSET(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);
    pucch_pdu->subcarrier_spacing = bwp->bwp_Common->genericParameters.subcarrierSpacing;
    pucch_pdu->cyclic_prefix = (bwp->bwp_Common->genericParameters.cyclicPrefix==NULL) ? 0 : *bwp->bwp_Common->genericParameters.cyclicPrefix;

    pucch_Config = bwp->bwp_Dedicated->pucch_Config->choice.setup;

    AssertFatal(pucch_Config->resourceSetToAddModList!=NULL,
		"PUCCH resourceSetToAddModList is null\n");

    n_set = pucch_Config->resourceSetToAddModList->list.count; 
    AssertFatal(n_set>0,"PUCCH resourceSetToAddModList is empty\n");

    N2 = 2;
    // procedure to select pucch resource id from resource sets according to 
    // number of uci bits and pucch resource indicator pucch_resource
    // ( see table 9.2.3.2 in 38.213)
    for (int i=0; i<n_set; i++) {
      pucchresset = pucch_Config->resourceSetToAddModList->list.array[i];
      n_list = pucchresset->resourceList.list.count;
      if (pucchresset->pucch_ResourceSetId == 0 && O_uci<3) {
        if (pucch_resource < n_list)
          resource_id = pucchresset->resourceList.list.array[pucch_resource];
        else 
          AssertFatal(1==0,"Couldn't fine pucch resource indicator %d in PUCCH resource set %d for %d UCI bits",pucch_resource,i,O_uci);
      }
      else {
        N3 = pucchresset->maxPayloadMinus1!= NULL ?  *pucchresset->maxPayloadMinus1 : 1706;
        if (N2<O_uci && N3>O_uci) {
          if (pucch_resource < n_list)
            resource_id = pucchresset->resourceList.list.array[pucch_resource];
          else 
            AssertFatal(1==0,"Couldn't fine pucch resource indicator %d in PUCCH resource set %d for %d UCI bits",pucch_resource,i,O_uci);
        }
        else N2 = N3;
      }
    }

    AssertFatal(resource_id!=NULL,"Couldn-t find any matching PUCCH resource in the PUCCH resource sets");

    AssertFatal(pucch_Config->resourceToAddModList!=NULL,
		"PUCCH resourceToAddModList is null\n");

    n_list = pucch_Config->resourceToAddModList->list.count; 
    AssertFatal(n_list>0,"PUCCH resourceToAddModList is empty\n");

    // going through the list of PUCCH resources to find the one indexed by resource_id
    for (int i=0; i<n_list; i++) {
      pucchres = pucch_Config->resourceToAddModList->list.array[i];
      if (pucchres->pucch_ResourceId == *resource_id) {
        res_found = 1;
        pucch_pdu->prb_start = pucchres->startingPRB;
        // FIXME why there is only one frequency hopping flag
        // what about inter slot frequency hopping?
        pucch_pdu->freq_hop_flag = pucchres->intraSlotFrequencyHopping!= NULL ?  1 : 0;
        pucch_pdu->second_hop_prb = pucchres->secondHopPRB!= NULL ?  *pucchres->secondHopPRB : 0;
        switch(pucchres->format.present) {
          case NR_PUCCH_Resource__format_PR_format0 :
            pucch_pdu->format_type = 0;
            pucch_pdu->initial_cyclic_shift = pucchres->format.choice.format0->initialCyclicShift;
            pucch_pdu->nr_of_symbols = pucchres->format.choice.format0->nrofSymbols;
            pucch_pdu->start_symbol_index = pucchres->format.choice.format0->startingSymbolIndex;
            pucch_pdu->sr_flag = SR_flag;
            break;
          case NR_PUCCH_Resource__format_PR_format1 :
            pucch_pdu->format_type = 1;
            pucch_pdu->initial_cyclic_shift = pucchres->format.choice.format1->initialCyclicShift;
            pucch_pdu->nr_of_symbols = pucchres->format.choice.format1->nrofSymbols;
            pucch_pdu->start_symbol_index = pucchres->format.choice.format1->startingSymbolIndex;
            pucch_pdu->time_domain_occ_idx = pucchres->format.choice.format1->timeDomainOCC;
            pucch_pdu->sr_flag = SR_flag;
            break;
          case NR_PUCCH_Resource__format_PR_format2 :
            pucch_pdu->format_type = 2;
            pucch_pdu->nr_of_symbols = pucchres->format.choice.format2->nrofSymbols;
            pucch_pdu->start_symbol_index = pucchres->format.choice.format2->startingSymbolIndex;
            pucch_pdu->prb_size = pucchres->format.choice.format2->nrofPRBs;
            pucch_pdu->data_scrambling_id = pusch_id!= NULL ? *pusch_id : *scc->physCellId;
            pucch_pdu->dmrs_scrambling_id = id0!= NULL ? *id0 : *scc->physCellId;
            break;
          case NR_PUCCH_Resource__format_PR_format3 :
            pucch_pdu->format_type = 3;
            pucch_pdu->nr_of_symbols = pucchres->format.choice.format3->nrofSymbols;
            pucch_pdu->start_symbol_index = pucchres->format.choice.format3->startingSymbolIndex;
            pucch_pdu->prb_size = pucchres->format.choice.format3->nrofPRBs;
            pucch_pdu->data_scrambling_id = pusch_id!= NULL ? *pusch_id : *scc->physCellId;
            if (pucch_Config->format3 == NULL) {
              pucch_pdu->pi_2bpsk = 0;
              pucch_pdu->add_dmrs_flag = 0;
            }
            else {
              pucchfmt = pucch_Config->format3->choice.setup;
              pucch_pdu->pi_2bpsk = pucchfmt->pi2BPSK!= NULL ?  1 : 0;
              pucch_pdu->add_dmrs_flag = pucchfmt->additionalDMRS!= NULL ?  1 : 0;
            }
            break;
          case NR_PUCCH_Resource__format_PR_format4 :
            pucch_pdu->format_type = 4;
            pucch_pdu->nr_of_symbols = pucchres->format.choice.format4->nrofSymbols;
            pucch_pdu->start_symbol_index = pucchres->format.choice.format4->startingSymbolIndex;
            pucch_pdu->pre_dft_occ_len = pucchres->format.choice.format4->occ_Length;
            pucch_pdu->pre_dft_occ_idx = pucchres->format.choice.format4->occ_Index;
            pucch_pdu->data_scrambling_id = pusch_id!= NULL ? *pusch_id : *scc->physCellId;
            if (pucch_Config->format3 == NULL) {
              pucch_pdu->pi_2bpsk = 0;
              pucch_pdu->add_dmrs_flag = 0;
            }
            else {
              pucchfmt = pucch_Config->format3->choice.setup;
              pucch_pdu->pi_2bpsk = pucchfmt->pi2BPSK!= NULL ?  1 : 0;
              pucch_pdu->add_dmrs_flag = pucchfmt->additionalDMRS!= NULL ?  1 : 0;
            }
            break;
          default :
            AssertFatal(1==0,"Undefined PUCCH format \n");
        }
      }
    }
    AssertFatal(res_found==1,"No PUCCH resource found corresponding to id %ld\n",*resource_id);
  }  
  else { // this is for InitialBWP
    AssertFatal(1==0,"Fill in InitialBWP PUCCH configuration\n");
  }

}



void fill_dci_pdu_rel15(nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15,
			dci_pdu_rel15_t *dci_pdu_rel15,
			int *dci_formats,
			int *rnti_types) {
  
  uint16_t N_RB = pdcch_pdu_rel15->BWPSize;
  uint8_t fsize=0, pos=0;

  for (int d=0;d<pdcch_pdu_rel15->numDlDci;d++) {

    uint64_t *dci_pdu = (uint64_t *)pdcch_pdu_rel15->Payload[d];
    AssertFatal(pdcch_pdu_rel15->PayloadSizeBits[d]<=64, "DCI sizes above 64 bits not yet supported");

    int dci_size = pdcch_pdu_rel15->PayloadSizeBits[d];
    
    /// Payload generation
    switch(dci_formats[d]) {
    case NR_DL_DCI_FORMAT_1_0:
      switch(rnti_types[d]) {
      case NR_RNTI_RA:
	// Freq domain assignment
	fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
	pos=fsize;
	*dci_pdu |= ((dci_pdu_rel15->frequency_domain_assignment&((1<<fsize)-1)) << (dci_size-pos));
	LOG_D(MAC,"frequency-domain assignment %d (%d bits) N_RB_BWP %d=> %d (0x%lx)\n",dci_pdu_rel15->frequency_domain_assignment,fsize,N_RB,dci_size-pos,*dci_pdu);
	// Time domain assignment
	pos+=4;
	*dci_pdu |= (((uint64_t)dci_pdu_rel15->time_domain_assignment&0xf) << (dci_size-pos));
	LOG_D(MAC,"time-domain assignment %d  (3 bits)=> %d (0x%lx)\n",dci_pdu_rel15->time_domain_assignment,dci_size-pos,*dci_pdu);
	// VRB to PRB mapping
	
	pos++;
	*dci_pdu |= ((uint64_t)dci_pdu_rel15->vrb_to_prb_mapping&0x1)<<(dci_size-pos);
	LOG_D(MAC,"vrb to prb mapping %d  (1 bits)=> %d (0x%lx)\n",dci_pdu_rel15->vrb_to_prb_mapping,dci_size-pos,*dci_pdu);
	// MCS
	pos+=5;
	*dci_pdu |= ((uint64_t)dci_pdu_rel15->mcs&0x1f)<<(dci_size-pos);
	LOG_D(MAC,"mcs %d  (5 bits)=> %d (0x%lx)\n",dci_pdu_rel15->mcs,dci_size-pos,*dci_pdu);
	// TB scaling
	pos+=2;
	*dci_pdu |= ((uint64_t)dci_pdu_rel15->tb_scaling&0x3)<<(dci_size-pos);
	LOG_D(MAC,"tb_scaling %d  (2 bits)=> %d (0x%lx)\n",dci_pdu_rel15->tb_scaling,dci_size-pos,*dci_pdu);
	break;
	
      case NR_RNTI_C:
	
	// indicating a DL DCI format 1bit
	pos++;
	*dci_pdu |= ((uint64_t)dci_pdu_rel15->format_indicator&1)<<(dci_size-pos);
	LOG_D(MAC,"Format indicator %d (%d bits) N_RB_BWP %d => %d (0x%lx)\n",dci_pdu_rel15->format_indicator,1,N_RB,dci_size-pos,*dci_pdu);
	
	// Freq domain assignment (275rb >> fsize = 16)
	fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
	pos+=fsize;
	*dci_pdu |= (((uint64_t)dci_pdu_rel15->frequency_domain_assignment&((1<<fsize)-1)) << (dci_size-pos));
	
	LOG_D(MAC,"Freq domain assignment %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->frequency_domain_assignment,fsize,dci_size-pos,*dci_pdu);
	
	uint16_t is_ra = 1;
	for (int i=0; i<fsize; i++)
	  if (!((dci_pdu_rel15->frequency_domain_assignment>>i)&1)) {
	    is_ra = 0;
	    break;
	  }
	if (is_ra) //fsize are all 1  38.212 p86
	  {
	    // ra_preamble_index 6 bits
	    pos+=6;
	    *dci_pdu |= ((dci_pdu_rel15->ra_preamble_index&0x3f)<<(dci_size-pos));
	    
	    // UL/SUL indicator  1 bit
	    pos++;
	    *dci_pdu |= (dci_pdu_rel15->ul_sul_indicator&1)<<(dci_size-pos);
	    
	    // SS/PBCH index  6 bits
	    pos+=6;
	    *dci_pdu |= ((dci_pdu_rel15->ss_pbch_index&0x3f)<<(dci_size-pos));
	    
	    //  prach_mask_index  4 bits
	    pos+=4;
	    *dci_pdu |= ((dci_pdu_rel15->prach_mask_index&0xf)<<(dci_size-pos));
	    
	  }  //end if
	
	else {
	  
	  // Time domain assignment 4bit
	  
	  pos+=4;
	  *dci_pdu |= ((dci_pdu_rel15->time_domain_assignment&0xf) << (dci_size-pos));
	  LOG_D(MAC,"Time domain assignment %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->time_domain_assignment,4,dci_size-pos,*dci_pdu);
	  
	  // VRB to PRB mapping  1bit
	  pos++;
	  *dci_pdu |= (dci_pdu_rel15->vrb_to_prb_mapping&1)<<(dci_size-pos);
	  LOG_D(MAC,"VRB to PRB %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->vrb_to_prb_mapping,1,dci_size-pos,*dci_pdu);
	  
	  // MCS 5bit  //bit over 32, so dci_pdu ++
	  pos+=5;
	  *dci_pdu |= (dci_pdu_rel15->mcs&0x1f)<<(dci_size-pos);
	  LOG_D(MAC,"MCS %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->mcs,5,dci_size-pos,*dci_pdu);
	  
	  // New data indicator 1bit
	  pos++;
	  *dci_pdu |= (dci_pdu_rel15->ndi&1)<<(dci_size-pos);
	  LOG_D(MAC,"NDI %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->ndi,1,dci_size-pos,*dci_pdu);
	  
	  // Redundancy version  2bit
	  pos+=2;
	  *dci_pdu |= (dci_pdu_rel15->rv&0x3)<<(dci_size-pos);
	  LOG_D(MAC,"RV %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->rv,2,dci_size-pos,*dci_pdu);
	  
	  // HARQ process number  4bit
	  pos+=4;
	  *dci_pdu  |= ((dci_pdu_rel15->harq_pid&0xf)<<(dci_size-pos));
	  LOG_D(MAC,"HARQ_PID %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->harq_pid,4,dci_size-pos,*dci_pdu);
	  
	  // Downlink assignment index  2bit
	  pos+=2;
	  *dci_pdu |= ((dci_pdu_rel15->dai&3)<<(dci_size-pos));
	  LOG_D(MAC,"DAI %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->dai,2,dci_size-pos,*dci_pdu);
	  
	  // TPC command for scheduled PUCCH  2bit
	  pos+=2;
	  *dci_pdu |= ((dci_pdu_rel15->tpc&3)<<(dci_size-pos));
	  LOG_D(MAC,"TPC %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->tpc,2,dci_size-pos,*dci_pdu);
	  
	  // PUCCH resource indicator  3bit
	  pos+=3;
	  *dci_pdu |= ((dci_pdu_rel15->pucch_resource_indicator&0x7)<<(dci_size-pos));
	  LOG_D(MAC,"PUCCH RI %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->pucch_resource_indicator,3,dci_size-pos,*dci_pdu);
	  
	  // PDSCH-to-HARQ_feedback timing indicator 3bit
	  pos+=3;
	  *dci_pdu |= ((dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator&0x7)<<(dci_size-pos));
	  LOG_D(MAC,"PDSCH to HARQ TI %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator,3,dci_size-pos,*dci_pdu);
	  
	} //end else
	break;
	
      case NR_RNTI_P:
	
	// Short Messages Indicator – 2 bits
	for (int i=0; i<2; i++)
	  *dci_pdu |= (((uint64_t)dci_pdu_rel15->short_messages_indicator>>(1-i))&1)<<(dci_size-pos++);
	// Short Messages – 8 bits
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
	*dci_pdu |= ((uint64_t)dci_pdu_rel15->vrb_to_prb_mapping&1)<<(dci_size-pos++);
	// MCS 5 bit
	for (int i=0; i<5; i++)
	  *dci_pdu |= (((uint64_t)dci_pdu_rel15->mcs>>(4-i))&1)<<(dci_size-pos++);
	
	// TB scaling 2 bit
	for (int i=0; i<2; i++)
	  *dci_pdu |= (((uint64_t)dci_pdu_rel15->tb_scaling>>(1-i))&1)<<(dci_size-pos++);
	
	
	break;
	
      case NR_RNTI_SI:
	// Freq domain assignment 0-16 bit
	fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
	for (int i=0; i<fsize; i++)
	  *dci_pdu |= ((dci_pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<(dci_size-pos++);
	// Time domain assignment 4 bit
	for (int i=0; i<4; i++)
	  *dci_pdu |= (((uint64_t)dci_pdu_rel15->time_domain_assignment>>(3-i))&1)<<(dci_size-pos++);
	// VRB to PRB mapping 1 bit
	*dci_pdu |= ((uint64_t)dci_pdu_rel15->vrb_to_prb_mapping&1)<<(dci_size-pos++);
	// MCS 5bit  //bit over 32, so dci_pdu ++
	for (int i=0; i<5; i++)
	  *dci_pdu |= (((uint64_t)dci_pdu_rel15->mcs>>(4-i))&1)<<(dci_size-pos++);
	// Redundancy version  2bit
	for (int i=0; i<2; i++)
	  *dci_pdu |= (((uint64_t)dci_pdu_rel15->rv>>(1-i))&1)<<(dci_size-pos++);
	
	break;
	
      case NR_RNTI_TC:
	// indicating a DL DCI format 1bit
	*dci_pdu |= ((uint64_t)dci_pdu_rel15->format_indicator&1)<<(dci_size-pos++);
	// Freq domain assignment 0-16 bit
	fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
	for (int i=0; i<fsize; i++)
	  *dci_pdu |= (((uint64_t)dci_pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<(dci_size-pos++);
	// Time domain assignment 4 bit
	for (int i=0; i<4; i++)
	  *dci_pdu |= (((uint64_t)dci_pdu_rel15->time_domain_assignment>>(3-i))&1)<<(dci_size-pos++);
	// VRB to PRB mapping 1 bit
	*dci_pdu |= ((uint64_t)dci_pdu_rel15->vrb_to_prb_mapping&1)<<(dci_size-pos++);
	// MCS 5bit  //bit over 32, so dci_pdu ++
	for (int i=0; i<5; i++)
	  *dci_pdu |= (((uint64_t)dci_pdu_rel15->mcs>>(4-i))&1)<<(dci_size-pos++);
	// New data indicator 1bit
	*dci_pdu |= ((uint64_t)dci_pdu_rel15->ndi&1)<<(dci_size-pos++);
	// Redundancy version  2bit
	for (int i=0; i<2; i++)
	  *dci_pdu |= (((uint64_t)dci_pdu_rel15->rv>>(1-i))&1)<<(dci_size-pos++);
	// HARQ process number  4bit
	for (int i=0; i<4; i++)
	  *dci_pdu  |= (((uint64_t)dci_pdu_rel15->harq_pid>>(3-i))&1)<<(dci_size-pos++);
	
	// Downlink assignment index – 2 bits
	for (int i=0; i<2; i++)
	  *dci_pdu  |= (((uint64_t)dci_pdu_rel15->dai>>(1-i))&1)<<(dci_size-pos++);
	
	// TPC command for scheduled PUCCH – 2 bits
	for (int i=0; i<2; i++)
	  *dci_pdu  |= (((uint64_t)dci_pdu_rel15->tpc>>(1-i))&1)<<(dci_size-pos++);
	
	
	//      LOG_D(MAC, "DCI PDU: [0]->0x%08llx \t [1]->0x%08llx \t [2]->0x%08llx \t [3]->0x%08llx\n",
	//	    dci_pdu[0], dci_pdu[1], dci_pdu[2], dci_pdu[3]);
	
	
	// PDSCH-to-HARQ_feedback timing indicator – 3 bits
	for (int i=0; i<3; i++)
	  *dci_pdu  |= (((uint64_t)dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator>>(2-i))&1)<<(dci_size-pos++);
	
	break;
      }
      break;
      
    case NR_UL_DCI_FORMAT_0_0:
      switch(rnti_types[d])
	{
	case NR_RNTI_C:
	  // indicating a DL DCI format 1bit
	  *dci_pdu |= ((uint64_t)dci_pdu_rel15->format_indicator&1)<<(dci_size-pos++);
	  // Freq domain assignment  max 16 bit
	  fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
	  for (int i=0; i<fsize; i++)
	    *dci_pdu |= ((dci_pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<(dci_size-pos++);
	  // Time domain assignment 4bit
	  for (int i=0; i<4; i++)
	    *dci_pdu |= (((uint64_t)dci_pdu_rel15->time_domain_assignment>>(3-i))&1)<<(dci_size-pos++);
	  // Frequency hopping flag – 1 bit
	  *dci_pdu |= ((uint64_t)dci_pdu_rel15->frequency_hopping_flag&1)<<(dci_size-pos++);
	  // MCS  5 bit
	  for (int i=0; i<5; i++)
	    *dci_pdu |= (((uint64_t)dci_pdu_rel15->mcs>>(4-i))&1)<<(dci_size-pos++);
	  // New data indicator 1bit
	  *dci_pdu |= ((uint64_t)dci_pdu_rel15->ndi&1)<<(dci_size-pos++);
	  // Redundancy version  2bit
	  for (int i=0; i<2; i++)
	    *dci_pdu |= (((uint64_t)dci_pdu_rel15->rv>>(1-i))&1)<<(dci_size-pos++);
	  // HARQ process number  4bit
	  for (int i=0; i<4; i++)
	    *dci_pdu  |= (((uint64_t)dci_pdu_rel15->harq_pid>>(3-i))&1)<<(dci_size-pos++);
	  
	  // TPC command for scheduled PUSCH – 2 bits
	  for (int i=0; i<2; i++)
	    *dci_pdu |= (((uint64_t)dci_pdu_rel15->tpc>>(1-i))&1)<<(dci_size-pos++);
	  
	  // Padding bits
	  for(int a = pos;a<32;a++)
	    *dci_pdu |= ((uint64_t)dci_pdu_rel15->padding&1)<<(dci_size-pos++);
	  
	  // UL/SUL indicator – 1 bit
	  /* commented for now (RK): need to get this from BWP descriptor
	  if (cfg->pucch_config.pucch_GroupHopping.value)
	    *dci_pdu |= ((uint64_t)dci_pdu_rel15->ul_sul_indicator&1)<<(dci_size-pos++);
	    */
	  break;
	  
	case NFAPI_NR_RNTI_TC:
	  
	  // indicating a DL DCI format 1bit
	  *dci_pdu |= (dci_pdu_rel15->format_indicator&1)<<(dci_size-pos++);
	  // Freq domain assignment  max 16 bit
	  fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
	  for (int i=0; i<fsize; i++)
	    *dci_pdu |= ((dci_pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<(dci_size-pos++);
	  // Time domain assignment 4bit
	  for (int i=0; i<4; i++)
	    *dci_pdu |= (((uint64_t)dci_pdu_rel15->time_domain_assignment>>(3-i))&1)<<(dci_size-pos++);
	  // Frequency hopping flag – 1 bit
	  *dci_pdu |= ((uint64_t)dci_pdu_rel15->frequency_hopping_flag&1)<<(dci_size-pos++);
	  // MCS  5 bit
	  for (int i=0; i<5; i++)
	    *dci_pdu |= (((uint64_t)dci_pdu_rel15->mcs>>(4-i))&1)<<(dci_size-pos++);
	  // New data indicator 1bit
	  *dci_pdu |= ((uint64_t)dci_pdu_rel15->ndi&1)<<(dci_size-pos++);
	  // Redundancy version  2bit
	  for (int i=0; i<2; i++)
	    *dci_pdu |= (((uint64_t)dci_pdu_rel15->rv>>(1-i))&1)<<(dci_size-pos++);
	  // HARQ process number  4bit
	  for (int i=0; i<4; i++)
	    *dci_pdu  |= (((uint64_t)dci_pdu_rel15->harq_pid>>(3-i))&1)<<(dci_size-pos++);
	  
	  // TPC command for scheduled PUSCH – 2 bits
	  for (int i=0; i<2; i++)
	    *dci_pdu |= (((uint64_t)dci_pdu_rel15->tpc>>(1-i))&1)<<(dci_size-pos++);
	  
	  // Padding bits
	  for(int a = pos;a<32;a++)
	    *dci_pdu |= ((uint64_t)dci_pdu_rel15->padding&1)<<(dci_size-pos++);
	  
	  // UL/SUL indicator – 1 bit
	  /*
	    commented for now (RK): need to get this information from BWP descriptor
	    if (cfg->pucch_config.pucch_GroupHopping.value)
	    *dci_pdu |= ((uint64_t)dci_pdu_rel15->ul_sul_indicator&1)<<(dci_size-pos++);
	    */
	  break;
	  
	    }
      break;
    }
  }
}

  
    /*
      int nr_is_dci_opportunity(nfapi_nr_search_space_t search_space,
      nfapi_nr_coreset_t coreset,
      uint16_t frame,
      uint16_t slot,
      nfapi_nr_config_request_scf_t cfg) {
      
      AssertFatal(search_space.coreset_id==coreset.coreset_id, "Invalid association of coreset(%d) and search space(%d)\n",
      search_space.search_space_id, coreset.coreset_id);
      
      uint8_t is_dci_opportunity=0;
      uint16_t Ks=search_space.slot_monitoring_periodicity;
      uint16_t Os=search_space.slot_monitoring_offset;
      uint8_t Ts=search_space.duration;
      
      if (((frame*get_spf(&cfg) + slot - Os)%Ks)<Ts)
    is_dci_opportunity=1;

  return is_dci_opportunity;
}
*/

int get_spf(nfapi_nr_config_request_scf_t *cfg) {

  int mu = cfg->ssb_config.scs_common.value;
  AssertFatal(mu>=0&&mu<4,"Illegal scs %d\n",mu);

  return(10 * (1<<mu));
} 

int to_absslot(nfapi_nr_config_request_scf_t *cfg,int frame,int slot) {

  return(get_spf(cfg)*frame) + slot; 

}


int extract_startSymbol(int startSymbolAndLength) {
  int tmp = startSymbolAndLength/14;
  int tmp2 = startSymbolAndLength%14;

  if (tmp > 0 && tmp < (14-tmp2)) return(tmp2);
  else                            return(13-tmp2);
}

int extract_length(int startSymbolAndLength) {
  int tmp = startSymbolAndLength/14;
  int tmp2 = startSymbolAndLength%14;

  if (tmp > 0 && tmp < (14-tmp2)) return(tmp);
  else                            return(15-tmp2);
}

/*
 * Dump the UL or DL UE_list into LOG_T(MAC)
 */
void
dump_nr_ue_list(NR_UE_list_t *listP,
             int ul_flag)
//------------------------------------------------------------------------------
{
  if (ul_flag == 0) {
    for (int j = listP->head; j >= 0; j = listP->next[j]) {
      LOG_T(MAC, "DL list node %d => %d\n",
            j,
            listP->next[j]);
    }
  } else {
    for (int j = listP->head_ul; j >= 0; j = listP->next_ul[j]) {
      LOG_T(MAC, "UL list node %d => %d\n",
            j,
            listP->next_ul[j]);
    }
  }

  return;
}

int find_nr_UE_id(module_id_t mod_idP, rnti_t rntiP)
//------------------------------------------------------------------------------
{
  int UE_id;
  NR_UE_list_t *UE_list = &RC.nrmac[mod_idP]->UE_list;

  for (UE_id = 0; UE_id < MAX_MOBILES_PER_GNB; UE_id++) {
    if (UE_list->active[UE_id] == TRUE) {
      if (UE_list->rnti[UE_id] == rntiP) {
        return UE_id;
      }
    }
  }

  return -1;
}

int add_new_nr_ue(module_id_t mod_idP, rnti_t rntiP){

  int UE_id;
  int i;
  NR_UE_list_t *UE_list = &RC.nrmac[mod_idP]->UE_list;
  LOG_I(MAC, "[gNB %d] Adding UE with rnti %x (next avail %d, num_UEs %d)\n",
        mod_idP,
        rntiP,
        UE_list->avail,
        UE_list->num_UEs);
  dump_nr_ue_list(UE_list, 0);

  for (i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    if (UE_list->active[i] == TRUE)
      continue;

    UE_id = i;
    UE_list->num_UEs++;
    UE_list->active[UE_id] = TRUE;
    UE_list->rnti[UE_id] = rntiP;
    memset((void *) &UE_list->UE_sched_ctrl[UE_id],
           0,
           sizeof(NR_UE_sched_ctrl_t));
    LOG_I(MAC, "gNB %d] Add NR UE_id %d : rnti %x\n",
          mod_idP,
          UE_id,
          rntiP);
    dump_nr_ue_list(UE_list,
		    0);
    return (UE_id);
  }

  // printf("MAC: cannot add new UE for rnti %x\n", rntiP);
  LOG_E(MAC, "error in add_new_ue(), could not find space in UE_list, Dumping UE list\n");
  dump_nr_ue_list(UE_list,
		  0);
  return -1;
}


void get_pdsch_to_harq_feedback(int Mod_idP,
                                int UE_id,
                                NR_SearchSpace__searchSpaceType_PR ss_type,
                                uint8_t *pdsch_to_harq_feedback) {

  int bwp_id=1;
  NR_UE_list_t *UE_list = &RC.nrmac[Mod_idP]->UE_list;
  NR_CellGroupConfig_t *secondaryCellGroup = UE_list->secondaryCellGroup[UE_id];
  NR_BWP_Downlink_t *bwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1];
  NR_BWP_Uplink_t *ubwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[bwp_id-1];

  NR_SearchSpace_t *ss;

  // common search type uses DCI format 1_0
  if (ss_type == NR_SearchSpace__searchSpaceType_PR_common) {
    for (int i=0; i<8; i++)
      pdsch_to_harq_feedback[i] = i+1;
  }
  else {
    // searching for a ue specific search space
    int found=0;

    for (int i=0;i<bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.count;i++) {
      ss=bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.array[i];
      AssertFatal(ss->controlResourceSetId != NULL,"ss->controlResourceSetId is null\n");
      AssertFatal(ss->searchSpaceType != NULL,"ss->searchSpaceType is null\n");
      if (ss->searchSpaceType->present == ss_type) {
	found=1;
	break;
      }
    }
    AssertFatal(found==1,"Couldn't find a ue specific searchspace\n");
    if (ss->searchSpaceType->choice.ue_Specific->dci_Formats == NR_SearchSpace__searchSpaceType__ue_Specific__dci_Formats_formats0_0_And_1_0) {
      for (int i=0; i<8; i++)
        pdsch_to_harq_feedback[i] = i+1;
    }
    else {
      if(ubwp->bwp_Dedicated->pucch_Config->choice.setup->dl_DataToUL_ACK != NULL)
        pdsch_to_harq_feedback = (uint8_t *)ubwp->bwp_Dedicated->pucch_Config->choice.setup->dl_DataToUL_ACK;
      else
        AssertFatal(found==1,"There is no allocated dl_DataToUL_ACK for pdsch to harq feedback\n");
    }
  }
}


// function to update pucch scheduling parameters in UE list when a USS DL is scheduled
void nr_update_pucch_scheduling(int Mod_idP,
                                int UE_id,
                                frame_t frameP,
                                sub_frame_t slotP,
                                int slots_per_tdd,
                                NR_sched_pucch *sched_pucch) {

  NR_ServingCellConfigCommon_t *scc = RC.nrmac[Mod_idP]->common_channels->ServingCellConfigCommon;
  NR_UE_list_t *UE_list = &RC.nrmac[Mod_idP]->UE_list;
  int first_ul_slot_tdd,k;
  NR_sched_pucch *curr_pucch;
  uint8_t pdsch_to_harq_feedback[8];
  int found = 0;
  int i = 0;
  int nr_ulmix_slots = scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots;
  if (scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols!=0)
    nr_ulmix_slots++;

  // this is hardcoded for now as ue specific
  NR_SearchSpace__searchSpaceType_PR ss_type = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
  get_pdsch_to_harq_feedback(Mod_idP,UE_id,ss_type,pdsch_to_harq_feedback);

  // if the list of pucch to be scheduled is empty
  if (UE_list->UE_sched_ctrl[UE_id].sched_pucch == NULL) {
    sched_pucch->frame = frameP;
    sched_pucch->next_sched_pucch = NULL;
    sched_pucch->dai_c = 1;
    sched_pucch->resource_indicator = 0; // in phytest with only 1 UE we are using just the 1st resource
    if ( nr_ulmix_slots > 0 ) {
      // first pucch occasion in first UL or MIXED slot
      first_ul_slot_tdd = scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots;
      for (k=0; k<nr_ulmix_slots; k++) { // for each possible UL or mixed slot
        while (i<8 && found == 0)  {  // look if timing indicator is among allowed values
          if (pdsch_to_harq_feedback[i]==(first_ul_slot_tdd+k)-(slotP % slots_per_tdd))
            found = 1;
          if (found == 0) i++;
        }
        if (found == 1) break;
      }
      if (found == 1) {
        // computing slot in which pucch is scheduled
        sched_pucch->ul_slot = first_ul_slot_tdd + k + (slotP - (slotP % slots_per_tdd));
        sched_pucch->timing_indicator = pdsch_to_harq_feedback[i];
      }
      else
        AssertFatal(1==0,"No Uplink slot available in accordance to allowed timing indicator\n");
    }
    else
      AssertFatal(1==0,"No Uplink Slots in this Frame\n");

    UE_list->UE_sched_ctrl[UE_id].sched_pucch = sched_pucch;
  }
  else {  // to be tested
    curr_pucch = UE_list->UE_sched_ctrl[UE_id].sched_pucch;
    if (curr_pucch->dai_c<MAX_ACK_BITS) {     // we are scheduling at most MAX_UCI_BITS harq-ack in the same pucch
      while (i<8 && found == 0)  {  // look if timing indicator is among allowed values for current pucch
        if (pdsch_to_harq_feedback[i]==(curr_pucch->ul_slot % slots_per_tdd)-(slotP % slots_per_tdd))
          found = 1;
        if (found == 0) i++;
      }
      if (found == 1) {  // scheduling this harq-ack in current pucch
        sched_pucch = curr_pucch;
        sched_pucch->dai_c = 1 + sched_pucch->dai_c;
        sched_pucch->timing_indicator = pdsch_to_harq_feedback[i];
      }
    }
    if (curr_pucch->dai_c==MAX_ACK_BITS || found == 0) { // if current pucch is full or no timing indicator allowed
      // look for pucch occasions in other UL of mixed slots
      for (k=scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots; k<slots_per_tdd; k++) { // for each possible UL or mixed slot
        if (k!=(curr_pucch->ul_slot % slots_per_tdd)) { // skip current scheduled slot (already checked)
          i = 0;
          while (i<8 && found == 0)  {  // look if timing indicator is among allowed values
            if (pdsch_to_harq_feedback[i]==k-(slotP % slots_per_tdd))
              found = 1;
            if (found == 0) i++;
          }
          if (found == 1) {
            if (k<(curr_pucch->ul_slot % slots_per_tdd)) { // we need to add a pucch occasion before current pucch
              sched_pucch->frame = frameP;
              sched_pucch->ul_slot =  k + (slotP - (slotP % slots_per_tdd));
              sched_pucch->next_sched_pucch = curr_pucch;
              sched_pucch->dai_c = 1;
              sched_pucch->resource_indicator = 0; // in phytest with only 1 UE we are using just the 1st resource
              sched_pucch->timing_indicator = pdsch_to_harq_feedback[i];
              UE_list->UE_sched_ctrl[UE_id].sched_pucch = sched_pucch;
            }
            else {
              while (curr_pucch->next_sched_pucch != NULL && k!=(curr_pucch->ul_slot % slots_per_tdd))
                curr_pucch = curr_pucch->next_sched_pucch;
              if (curr_pucch == NULL) {  // creating a new item in the list
                sched_pucch->frame = frameP;
                sched_pucch->next_sched_pucch = NULL;
                sched_pucch->dai_c = 1;
                sched_pucch->timing_indicator = pdsch_to_harq_feedback[i];
                sched_pucch->resource_indicator = 0; // in phytest with only 1 UE we are using just the 1st resource
                sched_pucch->ul_slot = k + (slotP - (slotP % slots_per_tdd));
                curr_pucch->next_sched_pucch = (NR_sched_pucch*) malloc(sizeof(NR_sched_pucch));
                curr_pucch->next_sched_pucch = sched_pucch;
              }
              else {
                if (curr_pucch->dai_c==MAX_ACK_BITS)
                  found = 0; // if pucch at index k is already full we have to find a new one in a following occasion
                else { // scheduling this harq-ack in current pucch
                  sched_pucch = curr_pucch;
                  sched_pucch->dai_c = 1 + sched_pucch->dai_c;
                  sched_pucch->timing_indicator = pdsch_to_harq_feedback[i];
                }
              }
            }
          }
        }
      }
    }
  }
}


/*void fill_nfapi_coresets_and_searchspaces(NR_CellGroupConfig_t *cg,
					  nfapi_nr_coreset_t *coreset,
					  nfapi_nr_search_space_t *search_space) {

  nfapi_nr_coreset_t *cs;
  nfapi_nr_search_space_t *ss;
  NR_ServingCellConfigCommon_t *scc=cg->spCellConfig->reconfigurationWithSync->spCellConfigCommon;
  AssertFatal(cg->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count == 1,
	      "downlinkBWP_ToAddModList has %d BWP!\n",
	      cg->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count);

  NR_BWP_Downlink_t *bwp=cg->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[0];
  struct NR_PDCCH_Config__controlResourceSetToAddModList *coreset_list = bwp->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList;
  AssertFatal(coreset_list->list.count>0,
	      "cs list has 0 elements\n");
  for (int i=0;i<coreset_list->list.count;i++) {
    NR_ControlResourceSet_t *coreset_i=coreset_list->list.array[i];
    cs = coreset + coreset_i->controlResourceSetId;
      
    cs->coreset_id = coreset_i->controlResourceSetId;
    AssertFatal(coreset_i->frequencyDomainResources.size <=8 && coreset_i->frequencyDomainResources.size>0,
		"coreset_i->frequencyDomainResources.size=%d\n",
		(int)coreset_i->frequencyDomainResources.size);
  
    for (int f=0;f<coreset_i->frequencyDomainResources.size;f++)
      ((uint8_t*)&cs->frequency_domain_resources)[coreset_i->frequencyDomainResources.size-1-f]=coreset_i->frequencyDomainResources.buf[f];
    
    cs->frequency_domain_resources>>=coreset_i->frequencyDomainResources.bits_unused;
    
    cs->duration = coreset_i->duration;
    // Need to add information about TCI_StateIDs

    if (coreset_i->cce_REG_MappingType.present == NR_ControlResourceSet__cce_REG_MappingType_PR_nonInterleaved)
      cs->cce_reg_mapping_type = NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED;
    else {
      cs->cce_reg_mapping_type = NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED;

      if (coreset_i->cce_REG_MappingType.choice.interleaved->reg_BundleSize==NR_ControlResourceSet__cce_REG_MappingType__interleaved__reg_BundleSize_n6)
	cs->reg_bundle_size = 6;
      else cs->reg_bundle_size = 2+coreset_i->cce_REG_MappingType.choice.interleaved->reg_BundleSize;

      if (coreset_i->cce_REG_MappingType.choice.interleaved->interleaverSize==NR_ControlResourceSet__cce_REG_MappingType__interleaved__interleaverSize_n6)
	cs->interleaver_size = 6;
      else cs->interleaver_size = 2+coreset_i->cce_REG_MappingType.choice.interleaved->interleaverSize;

      if (coreset_i->cce_REG_MappingType.choice.interleaved->shiftIndex)
	cs->shift_index = *coreset_i->cce_REG_MappingType.choice.interleaved->shiftIndex;
      else cs->shift_index = 0;
    }
    
    if (coreset_i->precoderGranularity == NR_ControlResourceSet__precoderGranularity_sameAsREG_bundle)
      cs->precoder_granularity = NFAPI_NR_CSET_SAME_AS_REG_BUNDLE;
    else cs->precoder_granularity = NFAPI_NR_CSET_ALL_CONTIGUOUS_RBS;
    if (coreset_i->tci_PresentInDCI == NULL) cs->tci_present_in_dci = 0;
    else                                     cs->tci_present_in_dci = 1;

    if (coreset_i->tci_PresentInDCI == NULL) cs->dmrs_scrambling_id = 0;
    else                                     cs->dmrs_scrambling_id = *coreset_i->tci_PresentInDCI;
  }

  struct NR_PDCCH_ConfigCommon__commonSearchSpaceList *commonSearchSpaceList = bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList;
  AssertFatal(commonSearchSpaceList->list.count>0,
	      "common SearchSpace list has 0 elements\n");
  // Common searchspace list
  for (int i=0;i<commonSearchSpaceList->list.count;i++) {
    NR_SearchSpace_t *searchSpace_i=commonSearchSpaceList->list.array[i];  
    ss=search_space + searchSpace_i->searchSpaceId;
    if (searchSpace_i->controlResourceSetId) ss->coreset_id = *searchSpace_i->controlResourceSetId;
    switch(searchSpace_i->monitoringSlotPeriodicityAndOffset->present) {
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL1;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL2;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl2;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl4:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL4;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl4;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl5:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL5;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl5;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl8:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL8;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl8;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl10:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL10;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl10;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl16:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL16;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl16;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl20:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL20;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl20;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl40:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL40;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl40;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl80:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL80;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl80;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl160:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL160;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl160;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl320:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL320;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl320;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl640:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL640;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl640;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1280:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL1280;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl1280;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2560:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL2560;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl2560;
      break;
    default:
      AssertFatal(1==0,"Shouldn't get here\n");
      break;    
    }
    if (searchSpace_i->duration) ss->duration = *searchSpace_i->duration;
    else                         ss->duration = 1;


    AssertFatal(searchSpace_i->monitoringSymbolsWithinSlot->size == 2,
		"ss_i->monitoringSymbolsWithinSlot = %d != 2\n",
		(int)searchSpace_i->monitoringSymbolsWithinSlot->size);
    ((uint8_t*)&ss->monitoring_symbols_in_slot)[1] = searchSpace_i->monitoringSymbolsWithinSlot->buf[0];
    ((uint8_t*)&ss->monitoring_symbols_in_slot)[0] = searchSpace_i->monitoringSymbolsWithinSlot->buf[1];

    AssertFatal(searchSpace_i->nrofCandidates!=NULL,"searchSpace_%d->nrofCandidates is null\n",(int)searchSpace_i->searchSpaceId);
    if (searchSpace_i->nrofCandidates->aggregationLevel1 == NR_SearchSpace__nrofCandidates__aggregationLevel1_n8)
      ss->number_of_candidates[0] = 8;
    else ss->number_of_candidates[0] = searchSpace_i->nrofCandidates->aggregationLevel1;
    if (searchSpace_i->nrofCandidates->aggregationLevel2 == NR_SearchSpace__nrofCandidates__aggregationLevel2_n8)
      ss->number_of_candidates[1] = 8;
    else ss->number_of_candidates[1] = searchSpace_i->nrofCandidates->aggregationLevel2;
    if (searchSpace_i->nrofCandidates->aggregationLevel4 == NR_SearchSpace__nrofCandidates__aggregationLevel4_n8)
      ss->number_of_candidates[2] = 8;
    else ss->number_of_candidates[2] = searchSpace_i->nrofCandidates->aggregationLevel4;
    if (searchSpace_i->nrofCandidates->aggregationLevel8 == NR_SearchSpace__nrofCandidates__aggregationLevel8_n8)
      ss->number_of_candidates[3] = 8;
    else ss->number_of_candidates[3] = searchSpace_i->nrofCandidates->aggregationLevel8;
    if (searchSpace_i->nrofCandidates->aggregationLevel16 == NR_SearchSpace__nrofCandidates__aggregationLevel16_n8)
      ss->number_of_candidates[4] = 8;
    else ss->number_of_candidates[4] = searchSpace_i->nrofCandidates->aggregationLevel16;      

    AssertFatal(searchSpace_i->searchSpaceType->present==NR_SearchSpace__searchSpaceType_PR_common,
		"searchspace %d is not common\n",(int)searchSpace_i->searchSpaceId);
    AssertFatal(searchSpace_i->searchSpaceType->choice.common!=NULL,
		"searchspace %d common is null\n",(int)searchSpace_i->searchSpaceId);
    ss->search_space_type = NFAPI_NR_SEARCH_SPACE_TYPE_COMMON;
    if (searchSpace_i->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0)
      ss->css_formats_0_0_and_1_0 = 1;
    if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_0) {
      ss->css_format_2_0 = 1;
      // add aggregation info
    }
    if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_1)
      ss->css_format_2_1 = 1;
    if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_2)
      ss->css_format_2_2 = 1;
    if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_3)
      ss->css_format_2_3 = 1;
  }

  struct NR_PDCCH_Config__searchSpacesToAddModList *dedicatedSearchSpaceList = bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList;
  AssertFatal(dedicatedSearchSpaceList->list.count>0,
	      "Dedicated Search Space list has 0 elements\n");
  // Dedicated searchspace list
  for (int i=0;i<dedicatedSearchSpaceList->list.count;i++) {
    NR_SearchSpace_t *searchSpace_i=dedicatedSearchSpaceList->list.array[i];  
    ss=search_space + searchSpace_i->searchSpaceId;
    ss->search_space_id = searchSpace_i->searchSpaceId;
    if (searchSpace_i->controlResourceSetId) ss->coreset_id = *searchSpace_i->controlResourceSetId;
    switch(searchSpace_i->monitoringSlotPeriodicityAndOffset->present) {
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL1;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL2;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl2;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl4:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL4;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl4;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl5:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL5;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl5;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl8:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL8;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl8;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl10:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL10;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl10;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl16:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL16;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl16;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl20:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL20;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl20;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl40:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL40;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl40;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl80:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL80;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl80;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl160:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL160;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl160;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl320:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL320;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl320;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl640:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL640;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl640;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1280:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL1280;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl1280;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2560:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL2560;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl2560;
      break;
    default:
      AssertFatal(1==0,"Shouldn't get here\n");
      break;    
    }
    if (searchSpace_i->duration) ss->duration = *searchSpace_i->duration;
    else                         ss->duration = 1;
    
    
    AssertFatal(searchSpace_i->monitoringSymbolsWithinSlot->size == 2,
		"ss_i->monitoringSymbolsWithinSlot = %d != 2\n",
		(int)searchSpace_i->monitoringSymbolsWithinSlot->size);
    ((uint8_t*)&ss->monitoring_symbols_in_slot)[1] = searchSpace_i->monitoringSymbolsWithinSlot->buf[0];
    ((uint8_t*)&ss->monitoring_symbols_in_slot)[0] = searchSpace_i->monitoringSymbolsWithinSlot->buf[1];
    
    AssertFatal(searchSpace_i->nrofCandidates!=NULL,"searchSpace_%d->nrofCandidates is null\n",(int)searchSpace_i->searchSpaceId);
    if (searchSpace_i->nrofCandidates->aggregationLevel1 == NR_SearchSpace__nrofCandidates__aggregationLevel1_n8)
      ss->number_of_candidates[0] = 8;
    else ss->number_of_candidates[0] = searchSpace_i->nrofCandidates->aggregationLevel1;
    if (searchSpace_i->nrofCandidates->aggregationLevel2 == NR_SearchSpace__nrofCandidates__aggregationLevel2_n8)
      ss->number_of_candidates[1] = 8;
    else ss->number_of_candidates[1] = searchSpace_i->nrofCandidates->aggregationLevel2;
    if (searchSpace_i->nrofCandidates->aggregationLevel4 == NR_SearchSpace__nrofCandidates__aggregationLevel4_n8)
      ss->number_of_candidates[2] = 8;
    else ss->number_of_candidates[2] = searchSpace_i->nrofCandidates->aggregationLevel4;
    if (searchSpace_i->nrofCandidates->aggregationLevel8 == NR_SearchSpace__nrofCandidates__aggregationLevel8_n8)
      ss->number_of_candidates[3] = 8;
    else ss->number_of_candidates[3] = searchSpace_i->nrofCandidates->aggregationLevel8;
    if (searchSpace_i->nrofCandidates->aggregationLevel16 == NR_SearchSpace__nrofCandidates__aggregationLevel16_n8)
      ss->number_of_candidates[4] = 8;
    else ss->number_of_candidates[4] = searchSpace_i->nrofCandidates->aggregationLevel16;      
    
    if (searchSpace_i->searchSpaceType->present==NR_SearchSpace__searchSpaceType_PR_ue_Specific && searchSpace_i->searchSpaceType->choice.ue_Specific!=NULL) {
      
      ss->search_space_type = NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC;
      
      ss->uss_dci_formats = searchSpace_i->searchSpaceType->choice.ue_Specific-> dci_Formats;
      
    } else if (searchSpace_i->searchSpaceType->present==NR_SearchSpace__searchSpaceType_PR_common && searchSpace_i->searchSpaceType->choice.common!=NULL) {
      ss->search_space_type = NFAPI_NR_SEARCH_SPACE_TYPE_COMMON;
      
      if (searchSpace_i->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0)
	ss->css_formats_0_0_and_1_0 = 1;
      if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_0) {
	ss->css_format_2_0 = 1;
	// add aggregation info
      }
      if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_1)
	ss->css_format_2_1 = 1;
      if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_2)
	ss->css_format_2_2 = 1;
      if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_3)
	ss->css_format_2_3 = 1;
    }
  }
}
*/
