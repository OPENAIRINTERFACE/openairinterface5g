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
#include "LAYER2/MAC/mac_extern.h"

#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/LTE/rrc_extern.h"
#include "RRC/NR/nr_rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#if defined(ENABLE_ITTI)
#include "intertask_interface.h"
#endif

#include "T.h"
#include "NR_PDCCH-ConfigCommon.h"
#include "NR_ControlResourceSet.h"
#include "NR_SearchSpace.h"

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

int is_nr_UL_slot(NR_COMMON_channels_t * ccP, int slot){

    return (0);
}

void nr_configure_css_dci_initial(nfapi_nr_dl_config_pdcch_parameters_rel15_t* pdcch_params,
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
  uint8_t O, M;
  uint8_t ss_idx = rmsi_pdcch_config&0xf;
  uint8_t cset_idx = (rmsi_pdcch_config>>4)&0xf;
  uint8_t mu = scs_common;
  uint8_t O_scale=0, M_scale=0; // used to decide if the values of O and M need to be divided by 2

  /// Coreset params
  switch(scs_common) {

    case kHz15:

      switch(pdcch_scs) {
        case kHz15:
          AssertFatal(cset_idx<15,"Coreset index %d reserved for scs kHz15/kHz15\n", cset_idx);
          pdcch_params->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
          pdcch_params->n_rb = (cset_idx < 6)? 24 : (cset_idx < 12)? 48 : 96;
          pdcch_params->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_15_15[cset_idx];
          pdcch_params->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_15_15[cset_idx];
        break;

        case kHz30:
          AssertFatal(cset_idx<14,"Coreset index %d reserved for scs kHz15/kHz30\n", cset_idx);
          pdcch_params->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
          pdcch_params->n_rb = (cset_idx < 8)? 24 : 48;
          pdcch_params->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_15_30[cset_idx];
          pdcch_params->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_15_15[cset_idx];
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
            pdcch_params->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
            pdcch_params->n_rb = (cset_idx < 10)? 48 : 96;
            pdcch_params->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_30_15_b40Mhz[cset_idx];
            pdcch_params->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_30_15_b40Mhz[cset_idx];
          break;

          case kHz30:
            pdcch_params->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
            pdcch_params->n_rb = (cset_idx < 6)? 24 : 48;
            pdcch_params->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_30_30_b40Mhz[cset_idx];
            pdcch_params->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_30_30_b40Mhz[cset_idx];
          break;

          default:
            AssertFatal(1==0,"Invalid scs_common/pdcch_scs combination %d/%d \n", scs_common, pdcch_scs);
        }
      }

      else { // above 40Mhz
        switch(pdcch_scs) {
          case kHz15:
            AssertFatal(cset_idx<9,"Coreset index %d reserved for scs kHz30/kHz15\n", cset_idx);
            pdcch_params->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
            pdcch_params->n_rb = (cset_idx < 3)? 48 : 96;
            pdcch_params->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_30_15_a40Mhz[cset_idx];
            pdcch_params->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_30_15_a40Mhz[cset_idx];
          break;

          case kHz30:
            AssertFatal(cset_idx<10,"Coreset index %d reserved for scs kHz30/kHz30\n", cset_idx);
            pdcch_params->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
            pdcch_params->n_rb = (cset_idx < 4)? 24 : 48;
            pdcch_params->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_30_30_a40Mhz[cset_idx];
            pdcch_params->rb_offset =  nr_coreset_rb_offset_pdcch_type_0_scs_30_30_a40Mhz[cset_idx];
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
          pdcch_params->mux_pattern = (cset_idx < 8)?NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1 : NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE2;
          pdcch_params->n_rb = (cset_idx < 6)? 48 : (cset_idx < 8)? 96 : (cset_idx < 10)? 48 : 96;
          pdcch_params->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_120_60[cset_idx];
          pdcch_params->rb_offset = (nr_coreset_rb_offset_pdcch_type_0_scs_120_60[cset_idx]>0)?nr_coreset_rb_offset_pdcch_type_0_scs_120_60[cset_idx] :
          (k_ssb == 0)? -41 : -42;
        break;

        case kHz120:
          AssertFatal(cset_idx<8,"Coreset index %d reserved for scs kHz120/kHz120\n", cset_idx);
          pdcch_params->mux_pattern = (cset_idx < 4)?NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1 : NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE3;
          pdcch_params->n_rb = (cset_idx < 2)? 24 : (cset_idx < 4)? 48 : (cset_idx < 6)? 24 : 48;
          pdcch_params->n_symb = (cset_idx == 2)? 1 : 2;
          pdcch_params->rb_offset = (nr_coreset_rb_offset_pdcch_type_0_scs_120_120[cset_idx]>0)? nr_coreset_rb_offset_pdcch_type_0_scs_120_120[cset_idx] :
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
        pdcch_params->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
        pdcch_params->n_rb = 96;
        pdcch_params->n_symb = (cset_idx < 2)? 1 : 2;
        pdcch_params->rb_offset = (cset_idx&1)? 16 : 0;
      break;

      case kHz120:
        AssertFatal(cset_idx<8,"Coreset index %d reserved for scs kHz240/kHz120\n", cset_idx);
        pdcch_params->mux_pattern = (cset_idx < 4)? NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1 : NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE2;
        pdcch_params->n_rb = (cset_idx < 4)? 48 : (cset_idx < 6)? 24 : 48;
        pdcch_params->n_symb = ((cset_idx==2)||(cset_idx==3))? 2 : 1;
        pdcch_params->rb_offset = (nr_coreset_rb_offset_pdcch_type_0_scs_240_120[cset_idx]>0)? nr_coreset_rb_offset_pdcch_type_0_scs_240_120[cset_idx] :
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
  switch(pdcch_params->mux_pattern) {

    case NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1:
      if (freq_range == nr_FR1) {
        O = nr_ss_param_O_type_0_mux1_FR1[ss_idx];
        pdcch_params->nb_ss_sets_per_slot = nr_ss_sets_per_slot_type_0_FR1[ss_idx];
        M = nr_ss_param_M_type_0_mux1_FR1[ss_idx];
        M_scale = nr_ss_scale_M_mux1_FR1[ss_idx];
        pdcch_params->first_symbol = (ss_idx < 8)? ( (ssb_idx&1)? pdcch_params->n_symb : 0 ) : nr_ss_first_symb_idx_type_0_mux1_FR1[ss_idx - 8];
      }

      else {
        AssertFatal(ss_idx<14 ,"Invalid search space index for multiplexing type 1 and FR2 %d\n", ss_idx);
        O = nr_ss_param_O_type_0_mux1_FR2[ss_idx];
        O_scale = nr_ss_scale_O_mux1_FR2[ss_idx];
        pdcch_params->nb_ss_sets_per_slot = nr_ss_sets_per_slot_type_0_FR2[ss_idx];
        M = nr_ss_param_M_type_0_mux1_FR2[ss_idx];
        M_scale = nr_ss_scale_M_mux1_FR2[ss_idx];
        pdcch_params->first_symbol = (ss_idx < 12)? ( (ss_idx&1)? 7 : 0 ) : 0;
      }
      pdcch_params->nb_slots = 2;
      pdcch_params->sfn_mod2 = (CEILIDIV( (((O<<mu)>>O_scale) + ((ssb_idx*M)>>M_scale)), nb_slots_per_frame ) & 1)? 1 : 0;
      pdcch_params->first_slot = (((O<<mu)>>O_scale) + ((ssb_idx*M)>>M_scale)) % nb_slots_per_frame;

    break;

    case NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE2:
      AssertFatal( ((scs_common==kHz120)&&(pdcch_scs==kHz60)) || ((scs_common==kHz240)&&(pdcch_scs==kHz120)),
      "Invalid scs_common/pdcch_scs combination %d/%d for Mux type 2\n", scs_common, pdcch_scs );
      AssertFatal(ss_idx==0, "Search space index %d reserved for scs_common/pdcch_scs combination %d/%d", ss_idx, scs_common, pdcch_scs);

      pdcch_params->nb_slots = 1;

      if ((scs_common==kHz120)&&(pdcch_scs==kHz60)) {
        pdcch_params->first_symbol = nr_ss_first_symb_idx_scs_120_60_mux2[ssb_idx&3];
        // Missing in pdcch_params sfn_C and n_C here and in else case
      }
      else {
        pdcch_params->first_symbol = ((ssb_idx&7)==4)?12 : ((ssb_idx&7)==4)?13 : nr_ss_first_symb_idx_scs_240_120_set1_mux2[ssb_idx&7]; //???
      }

    break;

    case NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE3:
      AssertFatal( (scs_common==kHz120)&&(pdcch_scs==kHz120),
      "Invalid scs_common/pdcch_scs combination %d/%d for Mux type 3\n", scs_common, pdcch_scs );
      AssertFatal(ss_idx==0, "Search space index %d reserved for scs_common/pdcch_scs combination %d/%d", ss_idx, scs_common, pdcch_scs);

      pdcch_params->first_symbol = nr_ss_first_symb_idx_scs_120_120_mux3[ssb_idx&3];

    break;

    default:
      AssertFatal(1==0, "Invalid SSB and coreset multiplexing pattern %d\n", pdcch_params->mux_pattern);
  }
  pdcch_params->config_type = NFAPI_NR_CSET_CONFIG_MIB_SIB1;
  pdcch_params->cr_mapping_type = NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED;
  pdcch_params->precoder_granularity = NFAPI_NR_CSET_SAME_AS_REG_BUNDLE;
  pdcch_params->reg_bundle_size = 6;
  pdcch_params->interleaver_size = 2;
  // set initial banwidth part to full bandwidth
  pdcch_params->n_RB_BWP = N_RB;


}

void nr_configure_dci_from_pdcch_config(nfapi_nr_dl_config_pdcch_parameters_rel15_t* pdcch_params,
					nfapi_nr_coreset_t* coreset,
					nfapi_nr_search_space_t* search_space,
					NR_BWP_Downlink_t *bwp) {			

  uint16_t N_RB=NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);

  /// coreset

  //ControlResourceSetId
  pdcch_params->config_type = NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG;
  
  //frequencyDomainResources
  uint8_t count=0, start=0, start_set=0;
  uint64_t bitmap = coreset->frequency_domain_resources;

  for (int i=0; i<45; i++)
    if ((bitmap>>(44-i))&1) {
      count++;
      if (!start_set) {
        start = i;
        start_set = 1;
      }
    }
  pdcch_params->rb_offset = 6*start;
  pdcch_params->n_rb = 6*count;

  //duration
  pdcch_params->n_symb = coreset->duration;

  //cce-REG-MappingType
  pdcch_params->cr_mapping_type = coreset->cce_reg_mapping_type;
  if (pdcch_params->cr_mapping_type == NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED) {
    pdcch_params->reg_bundle_size = coreset->reg_bundle_size;
    pdcch_params->interleaver_size = coreset->interleaver_size;
  }
  else {
    pdcch_params->reg_bundle_size = 0;
    pdcch_params->interleaver_size = 0;
  }

  //shift index
  pdcch_params->shift_index = coreset->shift_index;

  //precoderGranularity
  pdcch_params->precoder_granularity = coreset->precoder_granularity;

  //TCI states
  // PDCCH params does not yet include information about TCI and QCL (needed for DCI 1.1 and 0.1)

  //pdcch-DMRS-ScramblingID
  pdcch_params->scrambling_id = coreset->dmrs_scrambling_id;
  

/// SearchSpace

  // first symbol
  //AssertFatal(pdcch_scs==kHz15, "PDCCH SCS above 15kHz not allowed if a symbol above 2 is monitored");
  int sps = bwp->bwp_Common->genericParameters.cyclicPrefix == NULL ? 14 : 12;

  for (int i=0; i<sps; i++)
    if ((search_space->monitoring_symbols_in_slot>>(sps-1-i))&1) {
      pdcch_params->first_symbol=i;
      break;
    }

  //searchSpaceType
  pdcch_params->search_space_type = search_space->search_space_type;

  pdcch_params->n_RB_BWP = N_RB;
}

int nr_is_dci_opportunity(nfapi_nr_search_space_t search_space,
			  nfapi_nr_coreset_t coreset,
			  uint16_t frame,
			  uint16_t slot,
			  nfapi_nr_config_request_t cfg) {

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

int get_dlscs(nfapi_nr_config_request_t *cfg) {

  return(cfg->rf_config.dl_subcarrierspacing.value);
}


int get_ulscs(nfapi_nr_config_request_t *cfg) {

  return(cfg->rf_config.ul_subcarrierspacing.value);
} 

int get_spf(nfapi_nr_config_request_t *cfg) {

  int mu = cfg->rf_config.dl_subcarrierspacing.value;
  AssertFatal(mu>=0&&mu<4,"Illegal scs %d\n",mu);

  return(10 * (1<<mu));
} 

int to_absslot(nfapi_nr_config_request_t *cfg,int frame,int slot) {

  return(get_spf(cfg)*frame) + slot; 

}

int get_symbolsperslot(nfapi_nr_config_request_t *cfg) {

  return ((cfg->subframe_config.dl_cyclic_prefix_type.value==NFAPI_CP_EXTENDED)?12:14);

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
 
void fill_initialBWPDLtimeDomainAllocaion(nfapi_nr_config_request_t *cfg,int time_domain_assignment,int *k0,int *mappingType,int *start_symbol,int *length) {
  AssertFatal(time_domain_assignment < cfg->pdsch_config.num_PDSCHTimeDomainResourceAllocations.value,"DL time_domain_assignment %d >= %d\n",
	      time_domain_assignment,cfg->pdsch_config.num_PDSCHTimeDomainResourceAllocations.value);
  *k0           = cfg->pdsch_config.PDSCHTimeDomainResourceAllocation_k0[time_domain_assignment].value;
  *mappingType = cfg->pdsch_config.PDSCHTimeDomainResourceAllocation_mappingType[time_domain_assignment].value;
  *start_symbol = extract_startSymbol(cfg->pdsch_config.PDSCHTimeDomainResourceAllocation_startSymbolAndLength[time_domain_assignment].value);
  *length       = extract_length(cfg->pdsch_config.PDSCHTimeDomainResourceAllocation_startSymbolAndLength[time_domain_assignment].value);

}



void fill_initialBWPULtimeDomainAllocaion(nfapi_nr_config_request_t *cfg,int time_domain_assignment,int *k2, int *mappingType, int *start_symbol,int *length) {
  AssertFatal(time_domain_assignment < cfg->pusch_config.num_PUSCHTimeDomainResourceAllocations.value,"UL time_domain_assignment %d >= %d\n",
	      time_domain_assignment,cfg->pusch_config.num_PUSCHTimeDomainResourceAllocations.value);

  *k2           = cfg->pusch_config.PUSCHTimeDomainResourceAllocation_k2[time_domain_assignment].value;
  *mappingType = cfg->pusch_config.PUSCHTimeDomainResourceAllocation_mappingType[time_domain_assignment].value;
  *start_symbol = extract_startSymbol(cfg->pusch_config.PUSCHTimeDomainResourceAllocation_startSymbolAndLength[time_domain_assignment].value);
  *length       = extract_length(cfg->pusch_config.PUSCHTimeDomainResourceAllocation_startSymbolAndLength[time_domain_assignment].value);
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

int
find_nr_UE_id(module_id_t mod_idP,
           rnti_t rntiP)
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
 
int add_new_nr_ue(module_id_t mod_idP,
		  rnti_t rntiP){

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

void fill_nfapi_coresets_and_searchspaces(NR_CellGroupConfig_t *cg,
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

int16_t fill_dmrs_mask(NR_PDSCH_Config_t *pdsch_Config,int dmrs_TypeA_Position,int NrOfSymbols) {

  int mapping_Type = 0; // TypeA
  int l0;
  if (dmrs_TypeA_Position == NR_ServingCellConfigCommon__dmrs_TypeA_Position_pos2) l0=2;
  else if (dmrs_TypeA_Position == NR_ServingCellConfigCommon__dmrs_TypeA_Position_pos3) l0=3;
  else AssertFatal(1==0,"Illegal dmrs_TypeA_Position %d\n",(int)dmrs_TypeA_Position);
  if (pdsch_Config == NULL) { // Initial BWP
    return(1<<l0);
  }
  else {
    if (pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA &&
	pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA->present == NR_SetupRelease_DMRS_DownlinkConfig_PR_setup) {
      // Relative to start of slot
      NR_DMRS_DownlinkConfig_t *dmrs_config = (NR_DMRS_DownlinkConfig_t *)pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup;
      AssertFatal(NrOfSymbols>1 && NrOfSymbols < 15,"Illegal NrOfSymbols %d\n",NrOfSymbols);
      int pos2=0;
      if (dmrs_config->maxLength == NULL) {
	// this is Table 7.4.1.1.2-3: PDSCH DM-RS positions l for single-symbol DM-RS
	if (dmrs_config->dmrs_AdditionalPosition == NULL) pos2=1;
	else if (dmrs_config->dmrs_AdditionalPosition && *dmrs_config->dmrs_AdditionalPosition == NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos0 )
	  return(1<<l0);
	
	
	switch (NrOfSymbols) {
	case 2 :
	case 3 :
	case 4 :
	case 5 :
	case 6 :
	case 7 :
	  AssertFatal(1==0,"Incoompatible NrOfSymbols %d and dmrs_Additional_Position %d\n",
		      NrOfSymbols,(int)*dmrs_config->dmrs_AdditionalPosition);
	  break;
	case 8 :
	case 9:
	  return(1<<l0 | 1<<7);
	  break;
	case 10:
	case 11:
	  if (*dmrs_config->dmrs_AdditionalPosition==NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos1)
	    return(1<<l0 | 1<<9);
	  else
	    return(1<<l0 | 1<<6 | 1<<9);
	  break;
	case 12:
	  if (*dmrs_config->dmrs_AdditionalPosition==NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos1)
	    return(1<<l0 | 1<<9);
	  else if (pos2==1)
	    return(1<<l0 | 1<<6 | 1<<9);
	  else if (*dmrs_config->dmrs_AdditionalPosition==NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos3)
	    return(1<<l0 | 1<<5 | 1<<8 | 1<<11);
	  break;
	case 13:
	case 14:
	  if (*dmrs_config->dmrs_AdditionalPosition==NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos1)
	    return(1<<l0 | 1<<11);
	  else if (pos2==1)
	    return(1<<l0 | 1<<7 | 1<<11);
	  else if (*dmrs_config->dmrs_AdditionalPosition==NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos3)
	    return(1<<l0 | 1<<5 | 1<<8 | 1<<11);
	  break;
	}
      }
      else {
	// Table 7.4.1.1.2-4: PDSCH DM-RS positions l for double-symbol DM-RS.
	AssertFatal(NrOfSymbols>3,"Illegal NrOfSymbols %d for len2 DMRS\n",NrOfSymbols);
	if (NrOfSymbols < 10) return(1<<l0);
	if (NrOfSymbols < 13 && *dmrs_config->dmrs_AdditionalPosition==NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos0) return(1<<l0);
	if (NrOfSymbols < 13 && *dmrs_config->dmrs_AdditionalPosition!=NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos0) return(1<<l0 || 1<<8);
	if (*dmrs_config->dmrs_AdditionalPosition!=NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos0) return(1<<l0);
	if (*dmrs_config->dmrs_AdditionalPosition!=NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos1) return(1<<l0 || 1<<10);
      }
    }
  else if (pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeB &&
	   pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA->present == NR_SetupRelease_DMRS_DownlinkConfig_PR_setup) {
    // Relative to start of PDSCH resource
    AssertFatal(1==0,"TypeB DMRS not supported yet\n");
  }
  }
}
