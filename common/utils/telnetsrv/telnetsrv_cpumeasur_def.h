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

/*! \file common/utils/telnetsrv/telnetsrv_cpumeasur_def.h
 * \brief: definitions of macro used to initialize the telnet_ltemeasurdef_t
 * \        strucures arrays which are then used by the display functions
 * \        in telnetsrv_measurements.c.
 * \author Francois TABURET
 * \date 2021
 * \version 0.2
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */

#ifndef __TELNETSRV_CPUMEASUR_DEF__H__
#define __TELNETSRV_CPUMEASUR_DEF__H__

#define CPU_PHYENB_MEASURE \
{ \
  {"phy_proc_tx",            	  	 &(phyvars->phy_proc_tx),0,1},\
  {"phy_proc_rx",            	  	 &(phyvars->phy_proc_rx),0,1},\
  {"rx_prach",               	  	 &(phyvars->rx_prach),0,1},\
  {"ofdm_mod",         	  	         &(phyvars->ofdm_mod_stats),0,1},\
  {"dlsch_common_and_dci",   	  	 &(phyvars->dlsch_common_and_dci),0,1},\
  {"dlsch_ue_specific",      	  	 &(phyvars->dlsch_ue_specific),0,1},\
  {"dlsch_encoding",   	  	         &(phyvars->dlsch_encoding_stats),0,1},\
  {"dlsch_modulation", 	  	         &(phyvars->dlsch_modulation_stats),0,1},\
  {"dlsch_scrambling",                   &(phyvars->dlsch_scrambling_stats),0,1},\
  {"dlsch_rate_matching",                &(phyvars->dlsch_rate_matching_stats),0,1},\
  {"dlsch_turbo_encod_prep",             &(phyvars->dlsch_turbo_encoding_preperation_stats),0,1},\
  {"dlsch_turbo_encod_segm",             &(phyvars->dlsch_turbo_encoding_segmentation_stats),0,1},\
  {"dlsch_turbo_encod", 	         &(phyvars->dlsch_turbo_encoding_stats),0,1},\
  {"dlsch_turbo_encod_waiting",          &(phyvars->dlsch_turbo_encoding_waiting_stats),0,1},\
  {"dlsch_turbo_encod_signal",           &(phyvars->dlsch_turbo_encoding_signal_stats),0,1},\
  {"dlsch_turbo_encod_main",	         &(phyvars->dlsch_turbo_encoding_main_stats),0,1},\
  {"dlsch_turbo_encod_wakeup0",          &(phyvars->dlsch_turbo_encoding_wakeup_stats0),0,1},\
  {"dlsch_turbo_encod_wakeup1",          &(phyvars->dlsch_turbo_encoding_wakeup_stats1),0,1},\
  {"dlsch_interleaving",                 &(phyvars->dlsch_interleaving_stats),0,1},\
  {"rx_dft",                             &(phyvars->rx_dft_stats),0,1},\
  {"ulsch_channel_estimation",           &(phyvars->ulsch_channel_estimation_stats),0,1},\
  {"ulsch_freq_offset_estimation",       &(phyvars->ulsch_freq_offset_estimation_stats),0,1},\
  {"ulsch_decoding",                     &(phyvars->ulsch_decoding_stats),0,1},\
  {"ulsch_demodulation",                 &(phyvars->ulsch_demodulation_stats),0,1},\
  {"ulsch_rate_unmatching",              &(phyvars->ulsch_rate_unmatching_stats),0,1},\
  {"ulsch_turbo_decoding",               &(phyvars->ulsch_turbo_decoding_stats),0,1},\
  {"ulsch_deinterleaving",               &(phyvars->ulsch_deinterleaving_stats),0,1},\
  {"ulsch_demultiplexing",               &(phyvars->ulsch_demultiplexing_stats),0,1},\
  {"ulsch_llr",                          &(phyvars->ulsch_llr_stats),0,1},\
  {"ulsch_tc_init",                      &(phyvars->ulsch_tc_init_stats),0,1},\
  {"ulsch_tc_alpha",                     &(phyvars->ulsch_tc_alpha_stats),0,1},\
  {"ulsch_tc_beta",                      &(phyvars->ulsch_tc_beta_stats),0,1},\
  {"ulsch_tc_gamma",                     &(phyvars->ulsch_tc_gamma_stats),0,1},\
  {"ulsch_tc_ext",                       &(phyvars->ulsch_tc_ext_stats),0,1},\
  {"ulsch_tc_intl1",                     &(phyvars->ulsch_tc_intl1_stats),0,1},\
  {"ulsch_tc_intl2",                     &(phyvars->ulsch_tc_intl2_stats),0,1},\
}

#define CPU_MACENB_MEASURE \
{ \
  {"eNB_scheduler",	    &(macvars->eNB_scheduler),0,1},\
  {"schedule_si",	    &(macvars->schedule_si),0,1},\
  {"schedule_ra",	    &(macvars->schedule_ra),0,1},\
  {"schedule_ulsch",	    &(macvars->schedule_ulsch),0,1},\
  {"fill_DLSCH_dci",	    &(macvars->fill_DLSCH_dci),0,1},\
  {"schedule_dlsch_pre",    &(macvars->schedule_dlsch_preprocessor),0,1},\
  {"schedule_dlsch",	    &(macvars->schedule_dlsch),0,1},\
  {"schedule_mch",	    &(macvars->schedule_mch),0,1},\
  {"rx_ulsch_sdu",	    &(macvars->rx_ulsch_sdu),0,1},\
  {"schedule_pch",	    &(macvars->schedule_pch),0,1},\
}

#define CPU_PDCPENB_MEASURE \
{ \
  {"pdcp_run",               &(pdcpvars->pdcp_run),0,1},\
  {"data_req",               &(pdcpvars->data_req),0,1},\
  {"data_ind",               &(pdcpvars->data_ind),0,1},\
  {"apply_security",         &(pdcpvars->apply_security),0,1},\
  {"validate_security",      &(pdcpvars->validate_security),0,1},\
  {"pdcp_ip",                &(pdcpvars->pdcp_ip),0,1},\
  {"ip_pdcp",                &(pdcpvars->ip_pdcp),0,1},\
}

/* from openair1/PHY/defs_nr_UE.h */




//    {"ue_front_end_stat[RX_NB_TH]",       &(UE->
//    {"ue_front_end_per_slot_stat[RX_NB_TH][LTE_SLOTS_PER_SUBFRAME]",       &(UE->
//    {"pdcch_procedures_stat[RX_NB_TH]",       &(UE->
//    {"pdsch_procedures_stat[RX_NB_TH]",       &(UE->
//    {"pdsch_procedures_per_slot_stat[RX_NB_TH][LTE_SLOTS_PER_SUBFRAME]",       &(UE->
//    {"dlsch_procedures_stat[RX_NB_TH]",       &(UE->
//    {"dlsch_decoding_stats[2]",       &(UE->
//    {"dlsch_llr_stats_parallelization[RX_NB_TH][LTE_SLOTS_PER_SUBFRAME]",       &(UE->

#define CPU_PHYNRUE_MEASURE \
{ \
    {"phy_proc",          &(UE->phy_proc[0]),0,RX_NB_TH},\
    {"phy_proc_rx",       &(UE-> phy_proc_rx[0]),0,RX_NB_TH},\
    {"phy_proc_tx",       &(UE->phy_proc_tx),0,1},\
    {"ofdm_mod_stats",       &(UE->ofdm_mod_stats),0,1},\
    {"ulsch_encoding_stats",       &(UE->ulsch_encoding_stats),0,1},\
    {"ulsch_modulation_stats",       &(UE->ulsch_modulation_stats),0,1},\
    {"ulsch_segmentation_stats",       &(UE->ulsch_segmentation_stats),0,1},\
    {"ulsch_rate_matching_stats",       &(UE->ulsch_rate_matching_stats),0,1},\
    {"ulsch_turbo_encoding_stats",       &(UE->ulsch_turbo_encoding_stats),0,1},\
    {"ulsch_interleaving_stats",       &(UE->ulsch_interleaving_stats),0,1},\
    {"ulsch_multiplexing_stats",       &(UE->ulsch_multiplexing_stats),0,1},\
    {"generic_stat",       &(UE->generic_stat),0,1},\
    {"generic_stat_bis",       &(UE->generic_stat_bis[0][0]),0,RX_NB_TH,LTE_SLOTS_PER_SUBFRAME},\
    {"ofdm_demod_stats",       &(UE->ofdm_demod_stats),0,1},\
    {"dlsch_rx_pdcch_stats",       &(UE->dlsch_rx_pdcch_stats),0,1},\
    {"rx_dft_stats",       &(UE->rx_dft_stats),0,1},\
    {"dlsch_c...timation_stats",       &(UE->dlsch_channel_estimation_stats),0,1},\
    {"dlsch_f...timation_stats",       &(UE->dlsch_freq_offset_estimation_stats),0,1},\
    {"dlsch_demodulation_stats",       &(UE->dlsch_demodulation_stats),0,1},\
    {"dlsch_rate_unmatching_stats",       &(UE->dlsch_rate_unmatching_stats),0,1},\
    {"dlsch_turbo_decoding_stats",       &(UE->dlsch_turbo_decoding_stats),0,1},\
    {"dlsch_deinterleaving_stats",       &(UE->dlsch_deinterleaving_stats),0,1},\
    {"dlsch_llr_stats",       &(UE->dlsch_llr_stats),0,1},\
    {"dlsch_unscrambling_stats",       &(UE->dlsch_unscrambling_stats),0,1},\
    {"dlsch_rate_matching_stats",       &(UE->dlsch_rate_matching_stats),0,1},\
    {"dlsch_turbo_encoding_stats",       &(UE->dlsch_turbo_encoding_stats),0,1},\
    {"dlsch_interleaving_stats",       &(UE->dlsch_interleaving_stats),0,1},\
    {"dlsch_tc_init_stats",       &(UE->dlsch_tc_init_stats),0,1},\
    {"dlsch_tc_alpha_stats",       &(UE->dlsch_tc_alpha_stats),0,1},\
    {"dlsch_tc_beta_stats",       &(UE->dlsch_tc_beta_stats),0,1},\
    {"dlsch_tc_gamma_stats",       &(UE->dlsch_tc_gamma_stats),0,1},\
    {"dlsch_tc_ext_stats",       &(UE->dlsch_tc_ext_stats),0,1},\
    {"dlsch_tc_intl1_stats",       &(UE->dlsch_tc_intl1_stats),0,1},\
    {"dlsch_tc_intl2_stats",       &(UE->dlsch_tc_intl2_stats),0,1},\
    {"tx_prach",       &(UE->tx_prach),0,1},\
    {"dlsch_encoding_SIC_stats",       &(UE->dlsch_encoding_SIC_stats),0,1},\
    {"dlsch_scrambling_SIC_stats",       &(UE->dlsch_scrambling_SIC_stats),0,1},\
    {"dlsch_modulation_SIC_stats",       &(UE->dlsch_modulation_SIC_stats),0,1},\
    {"dlsch...ping_unit_SIC_stats",       &(UE->dlsch_llr_stripping_unit_SIC_stats),0,1},\
    {"dlsch_unscrambling_SIC_stats",       &(UE->dlsch_unscrambling_SIC_stats),0,1},\
}
#endif
