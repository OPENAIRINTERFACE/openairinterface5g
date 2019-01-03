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

/*! \file phy_procedures_lte_eNB.c
* \brief Implementation of common utilities for eNB/UE procedures from 36.213 LTE specifications
* \author R. Knopp, F. Kaltenberger
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
* \note
* \warning
*/

#include "sched_nr.h"


/// LUT for the number of symbols in the coreset indexed by coreset index (4 MSB rmsi_pdcch_config)
uint8_t nr_coreset_nsymb_pdcch_type_0_b40Mhz[16] = {2,2,2,2,2,3,3,3,3,3,1,1,1,2,2,2}; // below 40Mhz bw
uint8_t nr_coreset_nsymb_pdcch_type_0_a40Mhz[10] = {2,2,3,3,1,1,2,2,3,3}; // above 40Mhz bw
/// LUT for the number of RBs in the coreset indexed by coreset index
uint8_t nr_coreset_rb_offset_pdcch_type_0_b40Mhz[16] = {0,1,2,3,4,0,1,2,3,4,12,14,16,12,14,16};
uint8_t nr_coreset_rb_offset_pdcch_type_0_a40Mhz[10] = {0,4,0,4,0,28,0,28,0,28};
/// LUT for monitoring occasions param O indexed by ss index (4 LSB rmsi_pdcch_config)
uint8_t nr_ss_param_O_type_0_mux1_FR1[16] = {0,0,2,2,5,5,7,7,0,5,0,0,2,2,5,5};
uint8_t nr_ss_param_O_type_0_mux1_FR2[14] = {0,0,2.5,2.5,5,5,0,2.5,5,7.5,7.5,7.5,0,5};
/// LUT for number of SS sets per slot indexed by ss index
uint8_t nr_ss_sets_per_slot_type_0_FR1[16] = {1,2,1,2,1,2,1,2,1,1,1,1,1,1,1,1};
uint8_t nr_ss_sets_per_slot_type_0_FR2[14] = {1,2,1,2,1,2,2,2,2,1,2,2,1,1};
/// LUT for monitoring occasions param M indexed by ss index
uint8_t nr_ss_param_M_type_0_mux1_FR1[16] = {1,0.5,1,0.5,1,0.5,1,0.5,2,2,1,1,1,1,1,1};
uint8_t nr_ss_param_M_type_0_mux1_FR2[14] = {1,0.5,1,0.5,1,0.5,0.5,0.5,0.5,1,0.5,0.5,2,2};
/// LUT for SS first symbol index indexed by ss index
uint8_t nr_ss_first_symb_idx_type_0_mux1_FR1[8] = {0,0,1,2,1,2,1,2};



nr_subframe_t nr_subframe_select(nfapi_nr_config_request_t *cfg,unsigned char subframe)
{
  if (cfg->subframe_config.duplex_mode.value == FDD)
    return(SF_DL);
}


void nr_configure_css_dci_from_mib(nfapi_nr_dl_config_pdcch_parameters_rel15_t* pdcch_params,
                               nr_scs_e scs_common,
                               nr_scs_e pdcch_scs,
                               nr_frequency_range_e freq_range,
                               uint8_t rmsi_pdcch_config,
                               uint8_t ssb_idx,
                               uint16_t nb_slots_per_frame,
                               uint16_t N_RB)
{
  uint8_t O, M;
  uint8_t ss_idx = rmsi_pdcch_config&0xf;
  uint8_t cset_idx = (rmsi_pdcch_config>>4)&0xf;
  uint8_t mu;

  /// Coreset params
  switch(scs_common) {

    case kHz15:
      mu = 0;
      break;

    case kHz30:
      mu = 1;

      if (N_RB < 106) { // Minimum 40Mhz bandwidth not satisfied
        switch(pdcch_scs) {
          case kHz15:
            break;

          case kHz30:
            pdcch_params->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
            pdcch_params->n_rb = (cset_idx < 10)? 24 : 48;
            pdcch_params->n_symb = nr_coreset_nsymb_pdcch_type_0_b40Mhz[cset_idx];
            pdcch_params->rb_offset =  nr_coreset_rb_offset_pdcch_type_0_b40Mhz[cset_idx];
            break;

          default:
            AssertFatal(1==0,"Invalid scs_common/pdcch_scs combination %d/%d \n", scs_common, pdcch_scs);
        }
      }

      else {
        AssertFatal(ss_idx<10 ,"Invalid scs_common/pdcch_scs combination %d/%d \n", scs_common, pdcch_scs);
        switch(pdcch_scs) {
          case kHz15:
            break;

          case kHz30:
            pdcch_params->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
            pdcch_params->n_rb = (cset_idx < 4)? 24 : 48;
            pdcch_params->n_symb = nr_coreset_nsymb_pdcch_type_0_b40Mhz[cset_idx];
            pdcch_params->rb_offset =  nr_coreset_rb_offset_pdcch_type_0_b40Mhz[cset_idx];
            break;

          default:
            AssertFatal(1==0,"Invalid scs_common/pdcch_scs combination %d/%d \n", scs_common, pdcch_scs);
        }
      }

    case kHz60:
      mu = 2;
      break;

    case kHz120:
    mu = 3;
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
        pdcch_params->first_symbol = (ss_idx < 8)? ( (ss_idx&1)? pdcch_params->n_symb : 0 ) : nr_ss_first_symb_idx_type_0_mux1_FR1[ss_idx - 8];
      }

      else {
        AssertFatal(ss_idx<14 ,"Invalid search space index for multiplexing type 1 and FR2 %d\n", ss_idx);
        O = nr_ss_param_O_type_0_mux1_FR2[ss_idx];
        pdcch_params->nb_ss_sets_per_slot = nr_ss_sets_per_slot_type_0_FR2[ss_idx];
        M = nr_ss_param_M_type_0_mux1_FR2[ss_idx];
        pdcch_params->first_symbol = (ss_idx < 12)? ( (ss_idx&1)? 7 : 0 ) : 0;
      }
      pdcch_params->nb_slots = 2;
      pdcch_params->sfn_mod2 = ((uint8_t)(floor( (O*pow(2, mu) + floor(ssb_idx*M)) / nb_slots_per_frame )) & 1)? 1 : 0;
      pdcch_params->first_slot = (uint8_t)(O*pow(2, mu) + floor(ssb_idx*M)) % nb_slots_per_frame;

      break;

    case NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE2:
      break;

    case NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE3:
      break;

    default:
      AssertFatal(1==0, "Invalid SSB and coreset multiplexing pattern %d\n", pdcch_params->mux_pattern);
  }
  pdcch_params->config_type = NFAPI_NR_CSET_CONFIG_MIB_SIB1;
  pdcch_params->cr_mapping_type = NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED;
  pdcch_params->precoder_granularity = NFAPI_NR_CSET_SAME_AS_REG_BUNDLE;
  pdcch_params->reg_bundle_size = 6;
  pdcch_params->interleaver_size = 2;
}

void nr_configure_css_dci_from_pdcch_config(nfapi_nr_dl_config_pdcch_parameters_rel15_t* pdcch_params,
                                            nfapi_nr_coreset_t* coreset,
                                            nfapi_nr_search_space_t* search_space) {
//coreset

  //ControlResourceSetId
  pdcch_params->config_type = coreset->coreset_id;
  
  //frequencyDomainResources
  pdcch_params->n_rb = coreset->frequency_domain_resources;

  //duration
  pdcch_params->n_symb = coreset->duration;

  //cce-REG-MappingType
  switch (coreset->cce_reg_mapping_type) {
    case NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED: //Non_interleaved
      pdcch_params->reg_bundle_size = 0;
      pdcch_params->interleaver_size = 0;
      pdcch_params->shift_index = 0;
      break;
    case NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED: //interleaved
      pdcch_params->reg_bundle_size = coreset->reg_bundle_size;
      pdcch_params->interleaver_size = coreset->interleaver_size;
      pdcch_params->shift_index = coreset->shift_index;
      break;
  }
  //precoderGranularity
  pdcch_params->precoder_granularity = coreset->precoder_granularity;
/*
  if (tci_present_in_dci == tciPresentInDCI_t->tciPresentInDCI_enabled){
    //not sure information
  }
*/
  //pdcch-DMRS-ScramblingID
  pdcch_params->scrambling_id = coreset->dmrs_scrambling_id;

  
  

//SearchSpace 38.213, Reference defs_nr_UE.h - 755-840 code 
  //searchSpaceId
  search_space->search_space_id = 0;

  //controlResourceSetId
  if(coreset->coreset_id != NULL){ 
    search_space->coreset_id = coreset->coreset_id; //Value 0 or Value 1
  }

  //monitoringSlotPeriodicityAndOffset
  //pdcch_params->nb_slots = search_space->slot_monitoring_offset; //output not sure.
  pdcch_params->nb_slots = search_space->slot_monitoring_periodicity; //nr_sl1=1,nr_sl2=2,nr_sl4=4,nr_sl5=5,nr_sl8=8,nr_sl10=10,nr_sl16=16,nr_sl20=20,nr_sl40=40,nr_sl80=80,nr_sl160=160,nr_sl320=320,nr_sl640=640,nr_sl1280=1280,nr_sl2560=2560
  
  //duration
  search_space->duration = search_space->slot_monitoring_periodicity-1; //The maximum valid duration is periodicity-1


  //monitoringSymbolsWithinSlot
  pdcch_params->first_symbol = search_space->monitoring_symbols_in_slot; //38.213 page.69

  //nrofCandidates
  //NFAPI_NR_MAX_NB_CCE_AGGREGATION_LEVELS = 5 {1,2,4,8,16}
  pdcch_params->aggregation_level = search_space->number_of_candidates[NFAPI_NR_MAX_NB_CCE_AGGREGATION_LEVELS]; 

  if (search_space->search_space_type == 0){
    //Common_CSS
    pdcch_params->search_space_type = search_space->search_space_type;
    if(search_space->css_formats_0_0_and_1_0 == 0){ //Type0-"SI-RNTI, Type0A-SI-RNTI, Type1-RA-RNTI or TC-RNTI, Type2-P-RNTI, Type3-C-RNTI or CS-RNTI"
      pdcch_params->dci_format = search_space->css_formats_0_0_and_1_0;
    }
    if(search_space->css_format_2_0 == 2){ //Type3-"SFI-RNTI"
      pdcch_params->dci_format = search_space->css_format_2_0;
      //Not SFI-Aggregation-LeveL
      //pdcch_params->aggregation_level = search_space->number_of_candidates[NFAPI_NR_MAX_NB_CCE_AGGREGATION_LEVELS];
    }
    if(search_space->css_format_2_1 == 3){ //Type3-"INT-RNTI"
      pdcch_params->dci_format = search_space->css_format_2_1;
    }
    if(search_space->css_format_2_2 == 4){ //Type3-"TPC-PUSCH_RNTI or TPC-PUCCH-RNTI"
      pdcch_params->dci_format = search_space->css_format_2_2;
    }
    if(search_space->css_format_2_3 == 5){ //Type3-"TPC-SRS-RNTI"
      pdcch_params->dci_format = search_space->css_format_2_3;
      //Monitoring periodicity of SRS PDCCH in number of slots for DCI format 2-3
      pdcch_params->nb_slots = search_space->srs_monitoring_periodicity; //mp1=1,mp2=2,mp4=4,mp5=5,mp8=8,mp10=10,mp16=16,mp20=20
	}
  }
  if (search_space->search_space_type == 1){
    //Ue_Specific_USS
    pdcch_params->search_space_type = search_space->search_space_type; 
    if(search_space->uss_dci_formats == 0){ //DCI formats 0-0 and 1-0
      pdcch_params->dci_format = search_space->uss_dci_formats;
    }
    if(search_space->uss_dci_formats == 1){ //DCI formats 0-1 and 1-1
      pdcch_params->dci_format = search_space->uss_dci_formats;
    }
  }

  
}
