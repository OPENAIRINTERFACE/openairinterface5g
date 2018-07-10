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


/// LUT for the number of symbols in the coreset indexed by SSB index
uint8_t nr_coreset_nsymb_pdcch_type_0[16] = {2,2,2,2,2,3,3,3,3,3,1,1,1,2,2,2};
/// LUT for the number of RBs in the coreset indexed by SSB index
uint8_t nr_coreset_rb_offset_pdcch_type_0[16] = {0,1,2,3,4,0,1,2,3,4,12,14,16,12,14,16};
/// LUT for monitoring occasions param O indexed by SSB index
uint8_t nr_ss_param_O_type_0_mux1_FR1[16] = {0,0,2,2,5,5,7,7,0,5,0,0,2,2,5,5};
/// LUT for number of SS sets per slot indexed by SSB index
uint8_t nr_ss_sets_per_slot_type_0_FR1[16] = {1,2,1,2,1,2,1,2,1,1,1,1,1,1,1,1};
/// LUT for monitoring occasions param M indexed by SSB index
uint8_t nr_ss_param_M_type_0_mux1_FR1[16] = {1,0.5,1,0.5,1,0.5,1,0.5,2,2,1,1,1,1,1,1};
/// LUT for SS first symbol index indexed by SSB index
uint8_t nr_ss_first_symb_idx_type_0_mux1_FR1[8] = {0,0,1,2,1,2,1,2};



nr_subframe_t nr_subframe_select(nfapi_nr_config_request_t *cfg,unsigned char subframe)
{
  if (cfg->subframe_config.duplex_mode.value == FDD)
    return(SF_DL);
}


void nr_get_coreset_and_ss_params(nr_pdcch_vars_t *pdcch_vars,
                          uint8_t mu,
                          uint8_t ssb_idx)
{
  nr_pdcch_coreset_params_t *coreset_params = &pdcch_vars->coreset_params;
  nr_pdcch_ss_params_t *ss_params = &pdcch_vars->ss_params;

  // the switch case below assumes that only the cases where the SSB and the PDCCH have the same SCS are supported along with type 1 PDCCH/ mux pattern 1 and FR1
  switch(mu) {

    case NR_MU_0:
      break;

    case NR_MU_1:
      coreset_params->mux_pattern = nr_pdcch_mux_pattern_type_1;
      coreset_params->n_rb = (ssb_idx < 10)? 24 : 48;
      coreset_params->n_symb = nr_coreset_nsymb_pdcch_type_0[ssb_idx];
      coreset_params->rb_offset =  nr_coreset_rb_offset_pdcch_type_0[ssb_idx];
      ss_params->ss_type = nr_pdcch_css_type_1;
      ss_params->param_O = nr_ss_param_O_type_0_mux1_FR1[ssb_idx];
      ss_params->nb_ss_sets_per_slot = nr_ss_sets_per_slot_type_0_FR1[ssb_idx];
      ss_params->param_M = nr_ss_param_M_type_0_mux1_FR1[ssb_idx];
      ss_params->first_symbol_idx = (ssb_idx < 8)? ( (ssb_idx&1)? coreset_params->n_symb : 0 ) : nr_ss_first_symb_idx_type_0_mux1_FR1[ssb_idx - 8];
      break;

    case NR_MU_2:
      break;

    case NR_MU_3:
      break;

  default:
    AssertFatal(1==0,"Invalid PDCCH numerology index %d\n", mu);

  }
}

void nr_get_pdcch_type_0_monitoring_period(nr_pdcch_vars_t *pdcch_vars,
                                           uint8_t mu,
                                           uint8_t nb_slots_per_frame,
                                           uint8_t ssb_idx)
{
  uint8_t O = pdcch_vars->ss_params.param_O, M = pdcch_vars->ss_params.param_M;

  if (pdcch_vars->coreset_params.mux_pattern == nr_pdcch_mux_pattern_type_1) {
    pdcch_vars->nb_slots = 2;
    pdcch_vars->sfn_mod2 = ((uint8_t)(floor( (O*pow(2, mu) + floor(ssb_idx*M)) / nb_slots_per_frame )) & 1)? 1 : 0;
    pdcch_vars->first_slot = (uint8_t)(O*pow(2, mu) + floor(ssb_idx*M)) % nb_slots_per_frame;
  }
  else { //nr_pdcch_mux_pattern_type_2, nr_pdcch_mux_pattern_type_3
    pdcch_vars->nb_slots = 1;
  }

}
