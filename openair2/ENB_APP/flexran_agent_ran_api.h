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

/*! \file flexran_agent_ran_api.h
 * \brief FlexRAN RAN API abstraction header 
 * \author N. Nikaein, X. Foukas and S. SHARIAT BAGHERI
 * \date 2017
 * \version 0.1
 */

#include <stdio.h>
#include <time.h>

#include "flexran_agent_common.h"
#include "flexran_agent_common_internal.h"
#include "flexran_agent_extern.h"
#include "flexran_agent_defs.h"


#include "enb_config.h"
#include "LAYER2/RLC/rlc.h"
#include "SCHED/sched_eNB.h"
#include "pdcp.h"
#include "RRC/LTE/rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "RRC/LTE/rrc_eNB_UE_context.h"
#include "PHY/phy_extern.h"
#include "common/utils/LOG/log.h"

/****************************
 * get generic info from RAN
 ****************************/

uint32_t flexran_get_current_time_ms(mid_t mod_id, int subframe_flag);

/*Return the current frame number
 *Could be using implementation specific numbering of frames
 */
frame_t flexran_get_current_frame(mid_t mod_id);

/*Return the current SFN (0-1023)*/ 
frame_t flexran_get_current_system_frame_num(mid_t mod_id);

sub_frame_t flexran_get_current_subframe(mid_t mod_id);

/*Return the frame and subframe number in compact 16-bit format.
  Bits 0-3 subframe, rest for frame. Required by FlexRAN protocol*/
uint32_t flexran_get_sfn_sf(mid_t mod_id);

/* Return a future frame and subframe number that is ahead_of_time
   subframes later in compact 16-bit format. Bits 0-3 subframe,
   rest for frame */
uint16_t flexran_get_future_sfn_sf(mid_t mod_id, int ahead_of_time);

/* Return the number of attached UEs for the MAC */
int flexran_get_mac_num_ues(mid_t mod_id);

/* Get the number of logical channels per UE. This function does not consider
 * dedicated bearers yet */
int flexran_get_num_ue_lcs(mid_t mod_id, mid_t ue_id);

/* Get the rnti of a UE with id ue_id from MAC */
rnti_t flexran_get_mac_ue_crnti(mid_t mod_id, mid_t ue_id);

int flexran_get_mac_ue_id_rnti(mid_t mod_id, rnti_t rnti);

/* Return the UE id of attached UE as opposed to the index [0,NUM UEs] (i.e.,
 * the i'th active UE). Returns 0 if the i'th active UE could not be found. */
int flexran_get_mac_ue_id(mid_t mod_id, int i);

/* Get the RLC buffer status report in bytes of a ue for a designated
 * logical channel id */
int flexran_get_ue_bsr_ul_buffer_info(mid_t mod_id, mid_t ue_id, lcid_t lcid);

/* Get power headroom of UE with id ue_id */
int8_t flexran_get_ue_phr(mid_t mod_id, mid_t ue_id);

/* Get the UE wideband CQI */
uint8_t flexran_get_ue_wcqi(mid_t mod_id, mid_t ue_id);

/* Get the transmission queue size for a UE with a channel_id logical channel id */
rlc_buffer_occupancy_t flexran_get_tx_queue_size(mid_t mod_id, mid_t ue_id, logical_chan_id_t channel_id);

/*Get number of pdus in RLC buffer*/
rlc_buffer_occupancy_t flexran_get_num_pdus_buffer(mid_t mod_id, mid_t ue_id, logical_chan_id_t channel_id);

/* Get the head of line delay for a UE with a channel_id logical channel id */
frame_t flexran_get_hol_delay(mid_t mod_id, mid_t ue_id, logical_chan_id_t channel_id);

/* Check the status of the timing advance for a UE */
int32_t flexran_get_TA(mid_t mod_id, mid_t ue_id, uint8_t cc_id);

/* Update the timing advance status(find out whether a timing advance command is required) */
/* currently broken
void flexran_update_TA(mid_t mod_id, mid_t ue_id, uint8_t cc_id); */

/* Return timing advance MAC control element for a designated cell and UE */
/* this function is broken */
int flexran_get_MAC_CE_bitmap_TA(mid_t mod_id, mid_t ue_id, uint8_t cc_id);

/*Get number of mac SDU DL*/
uint32_t flexran_get_num_mac_sdu_tx(mid_t mod_id, mid_t ue_id, int cc_id);

/*Return the MAC sdu size got from logical channel lcid */
uint32_t flexran_get_mac_sdu_size(mid_t mod_id, mid_t ue_id, int cc_id, int lcid);

/*Return number of MAC SDUs obtained in MAC layer*/
uint32_t flexran_get_num_mac_sdu_tx(mid_t mod_id, mid_t ue_id, int cc_id);

/*Get mac sdu lcid index*/
unsigned char flexran_get_mac_sdu_lcid_index(mid_t mod_id, mid_t ue_id, int cc_id, int index);

/*Get MAC size sdus length dl*/
uint32_t flexran_get_size_dl_mac_sdus(mid_t mod_id, int cc_id);

/*Get MAC size sdus length ul */
uint32_t flexran_get_size_ul_mac_sdus(mid_t mod_id, int cc_id);

/*Get total size DL MAC SDUS*/
uint32_t flexran_get_total_size_dl_mac_sdus(mid_t mod_id, mid_t ue_id, int cc_id);

/*Get total size of UL mac SDUS*/
uint32_t flexran_get_total_size_ul_mac_sdus(mid_t mod_id, mid_t ue_id, int cc_id);

/*Get total number of PDU DL*/
uint32_t flexran_get_total_num_pdu_dl(mid_t mod_id, mid_t ue_id, int cc_id);

/*Get total number of PDU UL*/
uint32_t flexran_get_total_num_pdu_ul(mid_t mod_id, mid_t ue_id, int cc_id);

/*Get total PRB dl TODO Should be changed*/
uint32_t flexran_get_total_prb_dl_tx_per_ue(mid_t mod_id, mid_t ue_id, int cc_id);

/*Get total PRB ul TODO Should be changed*/
uint32_t flexran_get_total_prb_ul_rx_per_ue(mid_t mod_id, mid_t ue_id, int cc_id);

/*Get number of prb for tx per UE DL*/
uint16_t flexran_get_num_prb_dl_tx_per_ue(mid_t mod_id, mid_t ue_id, int cc_id);

/*Get number of prb for rx per UE UL*/
uint16_t flexran_get_num_prb_ul_rx_per_ue(mid_t mod_id, mid_t ue_id, int cc_id);

/*Get number of prb for retx per UE UL*/
uint32_t flexran_get_num_prb_retx_ul_per_ue(mid_t mod_id, mid_t ue_id, int cc_id);

/*Get number of prb for retx per UE*/
uint16_t flexran_get_num_prb_retx_dl_per_ue(mid_t mod_id, mid_t ue_id, int cc_id);

/*MCS before rate adaptation DL*/
uint8_t flexran_get_mcs1_dl(mid_t mod_id, mid_t ue_id, int cc_id);

/*MCS after rate adaptation DL*/
uint8_t flexran_get_mcs2_dl(mid_t mod_id, mid_t ue_id, int cc_id);

/*MCS before rate adaptation UL*/
uint8_t flexran_get_mcs1_ul(mid_t mod_id, mid_t ue_id, int cc_id);

/*MCS after rate adaptation UL*/
uint8_t flexran_get_mcs2_ul(mid_t mod_id, mid_t ue_id, int cc_id);

/*Get downlink TBS*/
uint32_t flexran_get_TBS_dl(mid_t mod_id, mid_t ue_id, int cc_id);

/*Get uplink TBS */
uint32_t flexran_get_TBS_ul(mid_t mod_id, mid_t ue_id, int cc_id);

/*Get total TBS DL*/
uint64_t flexran_get_total_TBS_dl(mid_t mod_id, mid_t ue_id, int cc_id);

/*Get total TBS DL*/
uint64_t flexran_get_total_TBS_ul(mid_t mod_id, mid_t ue_id, int cc_id);

/* Get the current HARQ round for UE ue_id */
int flexran_get_harq_round(mid_t mod_id, uint8_t cc_id, mid_t ue_id);

/* Get the number of active component carriers for a specific UE */
int flexran_get_active_CC(mid_t mod_id, mid_t ue_id);

/* Get the rank indicator for a designated cell and UE */
uint8_t flexran_get_current_RI(mid_t mod_id, mid_t ue_id, uint8_t cc_id);

/* See TS 36.213, section 10.1 */
uint16_t flexran_get_n1pucch_an(mid_t mod_id, uint8_t cc_id);

/* See TS 36.211, section 5.4 */
uint8_t flexran_get_nRB_CQI(mid_t mod_id, uint8_t cc_id);

/* See TS 36.211, section 5.4 */
uint8_t flexran_get_deltaPUCCH_Shift(mid_t mod_id, uint8_t cc_id);

/* See TS 36.211, section 5.7.1 */
uint8_t flexran_get_prach_ConfigIndex(mid_t mod_id, uint8_t cc_id);

/* See TS 36.211, section 5.7.1 */
uint8_t flexran_get_prach_FreqOffset(mid_t mod_id, uint8_t cc_id);

/* See TS 36.321 */
uint8_t flexran_get_maxHARQ_Msg3Tx(mid_t mod_id, uint8_t cc_id);

/* Get the length of the UL cyclic prefix */
Protocol__FlexUlCyclicPrefixLength flexran_get_ul_cyclic_prefix_length(mid_t mod_id, uint8_t cc_id);

/* Get the length of the DL cyclic prefix */
Protocol__FlexDlCyclicPrefixLength flexran_get_dl_cyclic_prefix_length(mid_t mod_id, uint8_t cc_id);

/* Get the physical cell id of a cell */
uint16_t flexran_get_cell_id(mid_t mod_id, uint8_t cc_id);

/* See TS 36.211, section 5.5.3.2 */
uint8_t flexran_get_srs_BandwidthConfig(mid_t mod_id, uint8_t cc_id);

/* See TS 36.211, table 5.5.3.3-1 and 2 */
uint8_t flexran_get_srs_SubframeConfig(mid_t mod_id, uint8_t cc_id);

/* Boolean value. See TS 36.211,
   section 5.5.3.2. TDD only */
uint8_t flexran_get_srs_MaxUpPts(mid_t mod_id, uint8_t cc_id);

/* Get number of DL resource blocks */
uint8_t flexran_get_N_RB_DL(mid_t mod_id, uint8_t cc_id);

/* Get number of UL resource blocks */
uint8_t flexran_get_N_RB_UL(mid_t mod_id, uint8_t cc_id);

/* Get number of resource block groups */
uint8_t flexran_get_N_RBG(mid_t mod_id, uint8_t cc_id);

/* Get DL/UL subframe assignment. TDD only */
uint8_t flexran_get_subframe_assignment(mid_t mod_id, uint8_t cc_id);

/* TDD only. See TS 36.211, table 4.2.1 */
uint8_t flexran_get_special_subframe_assignment(mid_t mod_id, uint8_t cc_id);

/* Get the duration of the random access response window in subframes */
long flexran_get_ra_ResponseWindowSize(mid_t mod_id, uint8_t cc_id);

/* Get timer used for random access */
long flexran_get_mac_ContentionResolutionTimer(mid_t mod_id, uint8_t cc_id);

/* Get type of duplex mode(FDD/TDD) */
Protocol__FlexDuplexMode flexran_get_duplex_mode(mid_t mod_id, uint8_t cc_id);

/* Get the SI window length */
long flexran_get_si_window_length(mid_t mod_id, uint8_t cc_id);

/* Get length of SystemInformationBlock1 */
uint8_t flexran_get_sib1_length(mid_t mod_id, uint8_t cc_id);

/* Get the number of PDCCH symbols configured for the cell */
uint8_t flexran_get_num_pdcch_symb(mid_t mod_id, uint8_t cc_id);

uint8_t flexran_get_antenna_ports(mid_t mod_id, uint8_t cc_id);

/* See TS 36.213, sec 5.1.1.1 */
int flexran_get_tpc(mid_t mod_id, mid_t ue_id, uint8_t cc_id);

uint8_t flexran_get_ue_wpmi(mid_t mod_id, mid_t ue_id, uint8_t cc_id);

/* Get the first available HARQ process for a specific cell and UE during 
   a designated frame and subframe. Returns 0 for success. The id and the 
   status of the HARQ process are stored in id and status respectively */
/* currently broken
int flexran_get_harq(mid_t mod_id, uint8_t cc_id, mid_t ue_id, frame_t frame,
                     sub_frame_t subframe, unsigned char *id, unsigned char *round,
                     uint8_t harq_flag); */

/* Uplink power control management*/
int32_t flexran_get_p0_pucch_dbm(mid_t mod_id, mid_t ue_id, uint8_t cc_id);

int8_t flexran_get_p0_nominal_pucch(mid_t mod_id, uint8_t cc_id);

int32_t flexran_get_p0_pucch_status(mid_t mod_id, mid_t ue_id, uint8_t cc_id);

int flexran_update_p0_pucch(mid_t mod_id, mid_t ue_id, uint8_t cc_id);

uint8_t flexran_get_threequarter_fs(mid_t mod_id, uint8_t cc_id);

Protocol__FlexHoppingMode flexran_get_hopping_mode(mid_t mod_id, uint8_t cc_id);

uint8_t flexran_get_hopping_offset(mid_t mod_id, uint8_t cc_id);

uint8_t flexran_get_n_SB(mid_t mod_id, uint8_t cc_id);

Protocol__FlexPhichResource flexran_get_phich_resource(mid_t mod_id, uint8_t cc_id);

Protocol__FlexQam flexran_get_enable64QAM(mid_t mod_id, uint8_t cc_id);

Protocol__FlexPhichDuration flexran_get_phich_duration(mid_t mod_id, uint8_t cc_id);

/*
 * ************************************
 * Get Messages for UE Configuration Reply
 * ************************************
 */
/* Get the number of attached UEs for the RRC */
int flexran_get_rrc_num_ues(mid_t mod_id);

/* Get the RNTI of UE at index 'index' in RRC list */
rnti_t flexran_get_rrc_rnti_nth_ue(mid_t mod_id, int index);

/* Get the list of RNTIs of up to max_list entries.  When max_list >=
 * flexran_get_rrc_num_ues(), gets a list of all UEs registered in the RRC. UE
 * RNTIs are saved in list, returns number of saved RNTIs */
int flexran_get_rrc_rnti_list(mid_t mod_id, rnti_t *list, int max_list);

/* Get timer in subframes. Controls the synchronization
   status of the UE, not the actual timing 
   advance procedure. See TS 36.321 */
LTE_TimeAlignmentTimer_t flexran_get_time_alignment_timer(mid_t mod_id, rnti_t rnti);

/* Get measurement gap configuration. See TS 36.133 */
Protocol__FlexMeasGapConfigPattern flexran_get_meas_gap_config(mid_t mod_id, rnti_t rnti);

/* Get measurement gap configuration offset if applicable */
long flexran_get_meas_gap_config_offset(mid_t mod_id, rnti_t rnti);

/* DL aggregated bit-rate of non-gbr bearer
   per UE. See TS 36.413 */
uint64_t flexran_get_ue_aggregated_max_bitrate_dl(mid_t mod_id, mid_t ue_id);

/* UL aggregated bit-rate of non-gbr bearer
   per UE. See TS 36.413 */
uint64_t flexran_get_ue_aggregated_max_bitrate_ul(mid_t mod_id, mid_t ue_id);

/* Only half-duplex support. FDD operation. Boolean value */
int flexran_get_half_duplex(mid_t mod_id, rnti_t rnti);

/* Support of intra-subframe hopping.  Boolean value */
int flexran_get_intra_sf_hopping(mid_t mod_id, rnti_t rnti);

/* UE support for type 2 hopping with n_sb>1 */
int flexran_get_type2_sb_1(mid_t mod_id, rnti_t rnti);

/* Get the UE category */
long flexran_get_ue_category(mid_t mod_id, rnti_t rnti);

/* UE support for resource allocation type 1 */
int flexran_get_res_alloc_type1(mid_t mod_id, rnti_t rnti);

/* Get UE transmission mode */
long flexran_get_ue_transmission_mode(mid_t mod_id, rnti_t rnti);

/* Boolean value. See TS 36.321 */
BOOLEAN_t flexran_get_tti_bundling(mid_t mod_id, rnti_t rnti);

/* The max HARQ retransmission for UL.
   See TS 36.321 */
long flexran_get_maxHARQ_TX(mid_t mod_id, rnti_t rnti);

/* See TS 36.213 */
long flexran_get_beta_offset_ack_index(mid_t mod_id, rnti_t rnti);

/* See TS 36.213 */
long flexran_get_beta_offset_ri_index(mid_t mod_id, rnti_t rnti);

/* See TS 36.213 */
long flexran_get_beta_offset_cqi_index(mid_t mod_id, rnti_t rnti);

/* Boolean. See TS36.213, Section 10.1 */
BOOLEAN_t flexran_get_simultaneous_ack_nack_cqi(mid_t mod_id, rnti_t rnti);

/* Boolean. See TS 36.213, Section 8.2 */
BOOLEAN_t flexran_get_ack_nack_simultaneous_trans(mid_t mod_id, uint8_t cc_id);

/* Get aperiodic CQI report mode */
Protocol__FlexAperiodicCqiReportMode flexran_get_aperiodic_cqi_rep_mode(mid_t mod_id, rnti_t rnti);

/* Get ACK/NACK feedback mode. TDD only */
long flexran_get_tdd_ack_nack_feedback_mode(mid_t mod_id, rnti_t rnti);

/* See TS36.213, section 10.1 */
long flexran_get_ack_nack_repetition_factor(mid_t mod_id, rnti_t rnti);

/* Boolean. Extended buffer status report size */
long flexran_get_extended_bsr_size(mid_t mod_id, rnti_t rnti);

/* Get number of UE transmission antennas */
int flexran_get_ue_transmission_antenna(mid_t mod_id, rnti_t rnti);

/* Get the IMSI of UE */
uint64_t flexran_get_ue_imsi(mid_t mod_id, rnti_t rnti);

/* Get logical channel group of a channel with id lc_id */
long flexran_get_lcg(mid_t mod_id, mid_t ue_id, mid_t lc_id);

/* Get direction of logical channel with id lc_id */
int flexran_get_direction(mid_t ue_id, mid_t lc_id);

/*Get downlink frequency*/
uint32_t flexran_agent_get_operating_dl_freq(mid_t mod_id, uint8_t cc_id);

/*Get uplink frequency*/
uint32_t flexran_agent_get_operating_ul_freq(mid_t mod_id, uint8_t cc_id);

/*Get eutra band*/
uint8_t flexran_agent_get_operating_eutra_band(mid_t mod_id, uint8_t cc_id);

/*Get downlink ref signal power*/
int8_t flexran_agent_get_operating_pdsch_refpower(mid_t mod_id, uint8_t cc_id);

/*Get uplink power*/
long flexran_agent_get_operating_pusch_p0(mid_t mod_id, uint8_t cc_id);

/*set the dl freq */
void flexran_agent_set_operating_dl_freq(mid_t mod_id, uint8_t cc_id, uint32_t dl_freq_mhz);

/* set the ul freq */
void flexran_agent_set_operating_ul_freq(mid_t mod_id, uint8_t cc_id, int32_t ul_freq_mhz_offset);

/*set the the band */
void flexran_agent_set_operating_eutra_band(mid_t mod_id, uint8_t cc_id, uint8_t eutra_band);

/* set the bandwidth (in RB) */
void flexran_agent_set_operating_bandwidth(mid_t mod_id, uint8_t cc_id, uint8_t N_RB);

/*set frame type*/
void flexran_agent_set_operating_frame_type(mid_t mod_id, uint8_t cc_id, lte_frame_type_t frame_type);

/*RRC status flexRAN*/
uint8_t flexran_get_rrc_status(mid_t mod_id, rnti_t rnti);


/***************************** PDCP ***********************/
/* PDCP uid obtained through the RNTI */
uint16_t flexran_get_pdcp_uid_from_rnti(mid_t mod_id, rnti_t rnti);

/*PDCP superframe numberflexRAN*/
uint32_t flexran_get_pdcp_sfn(mid_t mod_id);

/*PDCP pdcp tx stats window*/
void flexran_set_pdcp_tx_stat_window(mid_t mod_id, uint16_t uid, uint16_t obs_window);

/*PDCP pdcp rx stats window*/
void flexran_set_pdcp_rx_stat_window(mid_t mod_id, uint16_t uid, uint16_t obs_window);

/*PDCP num tx pdu status flexRAN*/
uint32_t flexran_get_pdcp_tx(mid_t mod_id, uint16_t uid, lcid_t lcid);

/*PDCP num tx bytes status flexRAN*/
uint32_t flexran_get_pdcp_tx_bytes(mid_t mod_id, uint16_t uid, lcid_t lcid);

/*PDCP number of transmit packet / second status flexRAN*/
uint32_t flexran_get_pdcp_tx_w(mid_t mod_id, uint16_t uid, lcid_t lcid);

/*PDCP pdcp tx bytes in a given window flexRAN*/
uint32_t flexran_get_pdcp_tx_bytes_w(mid_t mod_id, uint16_t uid, lcid_t lcid);

/*PDCP tx sequence number flexRAN*/
uint32_t flexran_get_pdcp_tx_sn(mid_t mod_id, uint16_t uid, lcid_t lcid);

/*PDCP tx aggregated packet arrival  flexRAN*/
uint32_t flexran_get_pdcp_tx_aiat(mid_t mod_id, uint16_t uid, lcid_t lcid);

/*PDCP tx aggregated packet arrival per second flexRAN*/
uint32_t flexran_get_pdcp_tx_aiat_w(mid_t mod_id, uint16_t uid, lcid_t lcid);


/*PDCP num rx pdu status flexRAN*/
uint32_t flexran_get_pdcp_rx(mid_t mod_id, uint16_t uid, lcid_t lcid);

/*PDCP num rx bytes status flexRAN*/
uint32_t flexran_get_pdcp_rx_bytes(mid_t mod_id, uint16_t uid, lcid_t lcid);

/*PDCP number of received packet / second  flexRAN*/
uint32_t flexran_get_pdcp_rx_w(mid_t mod_id, uint16_t uid, lcid_t lcid);

/*PDCP gootput (bit/s) status flexRAN*/
uint32_t flexran_get_pdcp_rx_bytes_w(mid_t mod_id, uint16_t uid, lcid_t lcid);

/*PDCP rx sequence number flexRAN*/
uint32_t flexran_get_pdcp_rx_sn(mid_t mod_id, uint16_t uid, lcid_t lcid);

/*PDCP rx aggregated packet arrival  flexRAN*/
uint32_t flexran_get_pdcp_rx_aiat(mid_t mod_id, uint16_t uid, lcid_t lcid);

/*PDCP rx aggregated packet arrival per second flexRAN*/
uint32_t flexran_get_pdcp_rx_aiat_w(mid_t mod_id, uint16_t uid, lcid_t lcid);

/*PDCP num of received outoforder pdu status flexRAN*/
uint32_t flexran_get_pdcp_rx_oo(mid_t mod_id, uint16_t uid, lcid_t lcid);

/*********************RRC**********************/
/* Call RRC Reconfiguration wrapper function */
int flexran_call_rrc_reconfiguration (mid_t mod_id, rnti_t rnti);

/* Call RRC to trigger handover wrapper function */
int flexran_call_rrc_trigger_handover (mid_t mod_id, rnti_t rnti, int target_cell_id);

/*Get primary cell measuremeant id flexRAN*/
LTE_MeasId_t flexran_get_rrc_pcell_measid(mid_t mod_id, rnti_t rnti);

/*Get primary cell RSRP measurement flexRAN*/  
float flexran_get_rrc_pcell_rsrp(mid_t mod_id, rnti_t rnti);

/*Get primary cell RSRQ measurement flexRAN*/
float flexran_get_rrc_pcell_rsrq(mid_t mod_id, rnti_t rnti);

/* Get RRC neighbouring measurement */
int flexran_get_rrc_num_ncell(mid_t mod_id, rnti_t rnti);

/* Get neighbouring physical cell id */
long flexran_get_rrc_neigh_phy_cell_id(mid_t mod_id, rnti_t rnti, long cell_id);

/* Get neighbouring cgi */
int flexran_get_rrc_neigh_cgi(mid_t mod_id, rnti_t rnti, long cell_id);

/* Get neighbouring cgi info cell id */
uint32_t flexran_get_rrc_neigh_cgi_cell_id(mid_t mod_id, rnti_t rnti, long cell_id);

/* Get neighbouring cgi info tac */
uint32_t flexran_get_rrc_neigh_cgi_tac(mid_t mod_id, rnti_t rnti, long cell_id);

/* Get the number of neighbouring cgi mnc */
int flexran_get_rrc_neigh_cgi_num_mnc(mid_t mod_id, rnti_t rnti, long cell_id);

/* Get the number of neighbouring cgi mcc */
int flexran_get_rrc_neigh_cgi_num_mcc(mid_t mod_id, rnti_t rnti, long cell_id);

/* Get neighbouring cgi mnc */
uint32_t flexran_get_rrc_neigh_cgi_mnc(mid_t mod_id, rnti_t rnti, long cell_id, int mnc_id);

/* Get neighbouring cgi mcc */
uint32_t flexran_get_rrc_neigh_cgi_mcc(mid_t mod_id, rnti_t rnti, long cell_id, int mcc_id);

/* Get RSRP of neighbouring Cell */
float flexran_get_rrc_neigh_rsrp(mid_t mod_id, rnti_t rnti, long cell_id);

/* Get RSRQ of neighbouring Cell */
float flexran_get_rrc_neigh_rsrq(mid_t mod_id, rnti_t rnti, long cell_id);

/* Get ofp offset */
long flexran_get_rrc_ofp(mid_t mod_id, rnti_t rnti);

/* Get ofn offset */
long flexran_get_rrc_ofn(mid_t mod_id, rnti_t rnti);

/* Get ocp offset */
long flexran_get_rrc_ocp(mid_t mod_id, rnti_t rnti);

/* Get ocn offset */
long flexran_get_rrc_ocn(mid_t mod_id, rnti_t rnti, long cell_id);

/* Get Periodic Event max reported cells */
long flexran_get_rrc_per_event_maxReportCells(mid_t mod_id, rnti_t rnti);

/* Get A3 Event hysteresis */
long flexran_get_rrc_a3_event_hysteresis(mid_t mod_id, rnti_t rnti);

/* Get A3 Event time to trigger */
long flexran_get_rrc_a3_event_timeToTrigger(mid_t mod_id, rnti_t rnti);

/* Get A3 Event max reported cells */
long flexran_get_rrc_a3_event_maxReportCells(mid_t mod_id, rnti_t rnti);

/* Get A3 Event a3 offset */
long flexran_get_rrc_a3_event_a3_offset(mid_t mod_id, rnti_t rnti);

/* Get A3 Event report on leave */
int flexran_get_rrc_a3_event_reportOnLeave(mid_t mod_id, rnti_t rnti);

/* Get filter coefficient for rsrp */
long flexran_get_filter_coeff_rsrp(mid_t mod_id, rnti_t rnti);

/* Get filter coefficient for rsrq */
long flexran_get_filter_coeff_rsrq(mid_t mod_id, rnti_t rnti);

/* Set ofp offset */
int flexran_set_rrc_ofp(mid_t mod_id, rnti_t rnti, long offsetFreq);

/* Set ofn offset */
int flexran_set_rrc_ofn(mid_t mod_id, rnti_t rnti, long offsetFreq);

/* Set ocp offset */
int flexran_set_rrc_ocp(mid_t mod_id, rnti_t rnti, long cellIndividualOffset);

/* Set ocn offset */
int flexran_set_rrc_ocn(mid_t mod_id, rnti_t rnti, long cell_id, long cellIndividualOffset);

/* Set Periodic Event max reported cells */
int flexran_set_rrc_per_event_maxReportCells(mid_t mod_id, rnti_t rnti, long maxReportCells);

/* Set A3 Event hysteresis */
int flexran_set_rrc_a3_event_hysteresis(mid_t mod_id, rnti_t rnti, long hysteresis);

/* Set A3 Event time to trigger */
int flexran_set_rrc_a3_event_timeToTrigger(mid_t mod_id, rnti_t rnti, long timeToTrigger);

/* Set A3 Event max reported cells */
int flexran_set_rrc_a3_event_maxReportCells(mid_t mod_id, rnti_t rnti, long maxReportCells);

/* Set A3 Event a3 offset */
int flexran_set_rrc_a3_event_a3_offset(mid_t mod_id, rnti_t rnti, long a3_offset);

/* Set A3 Event report on leave */
int flexran_set_rrc_a3_event_reportOnLeave(mid_t mod_id, rnti_t rnti, int reportOnLeave);

/* Set filter coefficient for rsrp */
int flexran_set_filter_coeff_rsrp(mid_t mod_id, rnti_t rnti, long filterCoefficientRSRP);

/* Set filter coefficient for rsrq */
int flexran_set_filter_coeff_rsrq(mid_t mod_id, rnti_t rnti, long filterCoefficientRSRQ);

/* Get number of PLMNs that is broadcasted in SIB1 */
uint8_t flexran_get_rrc_num_plmn_ids(mid_t mod_id);

/* Get index'th MCC broadcasted in SIB1 */
uint16_t flexran_get_rrc_mcc(mid_t mod_id, uint8_t index);

/* Get index'th MNC broadcasted in SIB1 */
uint16_t flexran_get_rrc_mnc(mid_t mod_id, uint8_t index);

/* Get index'th MNC's digit length broadcasted in SIB1 */
uint8_t flexran_get_rrc_mnc_digit_length(mid_t mod_id, uint8_t index);

/* Get X2 handover controlled by network */
int flexran_get_x2_ho_net_control(mid_t mod_id);

/* Set X2 handover controlled by network */
int flexran_set_x2_ho_net_control(mid_t mod_id, int x2_ho_net_control);

/* Get number of adjacent cells via X2 interface */
int flexran_get_rrc_num_adj_cells(mid_t mod_id);

/* Get the number of E-RABs for UE */
int flexran_agent_rrc_gtp_num_e_rab(mid_t mod_id, rnti_t rnti);

/* Get the e-RAB ID for UE */
int flexran_agent_rrc_gtp_get_e_rab_id(mid_t mod_id, rnti_t rnti, int index);

/* Get the TEID at the eNB for UE */
int flexran_agent_rrc_gtp_get_teid_enb(mid_t mod_id, rnti_t rnti, int index);

/* Get the TEID at the SGW for UE */
int flexran_agent_rrc_gtp_get_teid_sgw(mid_t mod_id, rnti_t rnti, int index);

/************************** Slice configuration **************************/

/* Get the DL slice ID for a UE */
int flexran_get_ue_dl_slice_id(mid_t mod_id, mid_t ue_id);

/* Set the DL slice index(!) for a UE */
void flexran_set_ue_dl_slice_idx(mid_t mod_id, mid_t ue_id, int slice_idx);

/* Get the UL slice ID for a UE */
int flexran_get_ue_ul_slice_id(mid_t mod_id, mid_t ue_id);

/* Set the UL slice index(!) for a UE */
void flexran_set_ue_ul_slice_idx(mid_t mod_id, mid_t ue_id, int slice_idx);

/* Whether intraslice sharing is active, return boolean */
int flexran_get_intraslice_sharing_active(mid_t mod_id);
/* Set whether intraslice sharing is active */
void flexran_set_intraslice_sharing_active(mid_t mod_id, int intraslice_active);

/* Whether intraslice sharing is active, return boolean */
int flexran_get_interslice_sharing_active(mid_t mod_id);
/* Set whether intraslice sharing is active */
void flexran_set_interslice_sharing_active(mid_t mod_id, int interslice_active);

/* Get the number of slices in DL */
int flexran_get_num_dl_slices(mid_t mod_id);

/* Query slice existence in DL. Return is boolean value */
int flexran_dl_slice_exists(mid_t mod_id, int slice_idx);

/* Create slice in DL, returns the new slice index */
int flexran_create_dl_slice(mid_t mod_id, slice_id_t slice_id);
/* Finds slice in DL with given slice_id and returns slice index */
int flexran_find_dl_slice(mid_t mod_id, slice_id_t slice_id);
/* Remove slice in DL, returns new number of slices or -1 on error */
int flexran_remove_dl_slice(mid_t mod_id, int slice_idx);

/* Get the ID of a slice in DL */
slice_id_t flexran_get_dl_slice_id(mid_t mod_id, int slice_idx);
/* Set the ID of a slice in DL */
void flexran_set_dl_slice_id(mid_t mod_id, int slice_idx, slice_id_t slice_id);

/* Get the RB share a slice in DL, value 0-100 */
int flexran_get_dl_slice_percentage(mid_t mod_id, int slice_idx);
/* Set the RB share a slice in DL, value 0-100 */
void flexran_set_dl_slice_percentage(mid_t mod_id, int slice_idx, int percentage);

/* Whether a slice in DL is isolated */
int flexran_get_dl_slice_isolation(mid_t mod_id, int slice_idx);
/* Set whether a slice in DL is isolated */
void flexran_set_dl_slice_isolation(mid_t mod_id, int slice_idx, int is_isolated);

/* Get the priority of a slice in DL */
int flexran_get_dl_slice_priority(mid_t mod_id, int slice_idx);
/* Get the priority of a slice in DL */
void flexran_set_dl_slice_priority(mid_t mod_id, int slice_idx, int priority);

/* Get the lower end of the frequency range for the slice positioning in DL */
int flexran_get_dl_slice_position_low(mid_t mod_id, int slice_idx);
/* Set the lower end of the frequency range for the slice positioning in DL */
void flexran_set_dl_slice_position_low(mid_t mod_id, int slice_idx, int poslow);

/* Get the higher end of the frequency range for the slice positioning in DL */
int flexran_get_dl_slice_position_high(mid_t mod_id, int slice_idx);
/* Set the higher end of the frequency range for the slice positioning in DL */
void flexran_set_dl_slice_position_high(mid_t mod_id, int slice_idx, int poshigh);

/* Get the maximum MCS for slice in DL */
int flexran_get_dl_slice_maxmcs(mid_t mod_id, int slice_idx);
/* Set the maximum MCS for slice in DL */
void flexran_set_dl_slice_maxmcs(mid_t mod_id, int slice_idx, int maxmcs);

/* Get the sorting order of a slice in DL, return value is number of elements
 * in sorting_list */
int flexran_get_dl_slice_sorting(mid_t mod_id, int slice_idx, Protocol__FlexDlSorting **sorting_list);
/* Set the sorting order of a slice in DL */
void flexran_set_dl_slice_sorting(mid_t mod_id, int slice_idx, Protocol__FlexDlSorting *sorting_list, int n);

/* Get the accounting policy for a slice in DL */
Protocol__FlexDlAccountingPolicy flexran_get_dl_slice_accounting_policy(mid_t mod_id, int slice_idx);
/* Set the accounting policy for a slice in DL */
void flexran_set_dl_slice_accounting_policy(mid_t mod_id, int slice_idx, Protocol__FlexDlAccountingPolicy accounting);

/* Get the scheduler name for a slice in DL */
char *flexran_get_dl_slice_scheduler(mid_t mod_id, int slice_idx);
/* Set the scheduler name for a slice in DL */
int flexran_set_dl_slice_scheduler(mid_t mod_id, int slice_idx, char *name);

/* Get the number of slices in UL */
int flexran_get_num_ul_slices(mid_t mod_id);

/* Query slice existence in UL. Return is boolean value */
int flexran_ul_slice_exists(mid_t mod_id, int slice_idx);

/* Create slice in UL, returns the new slice index */
int flexran_create_ul_slice(mid_t mod_id, slice_id_t slice_id);
/* Finds slice in UL with given slice_id and returns slice index */
int flexran_find_ul_slice(mid_t mod_id, slice_id_t slice_id);
/* Remove slice in UL */
int flexran_remove_ul_slice(mid_t mod_id, int slice_idx);

/* Get the ID of a slice in UL */
slice_id_t flexran_get_ul_slice_id(mid_t mod_id, int slice_idx);
/* Set the ID of a slice in UL */
void flexran_set_ul_slice_id(mid_t mod_id, int slice_idx, slice_id_t slice_id);

/* Get the RB share a slice in UL, value 0-100 */
int flexran_get_ul_slice_percentage(mid_t mod_id, int slice_idx);
/* Set the RB share a slice in UL, value 0-100 */
void flexran_set_ul_slice_percentage(mid_t mod_id, int slice_idx, int percentage);

/* TODO Whether a slice in UL is isolated */
/*int flexran_get_ul_slice_isolation(mid_t mod_id, int slice_idx);*/
/* TODO Set whether a slice in UL is isolated */
/*void flexran_set_ul_slice_isolation(mid_t mod_id, int slice_idx, int is_isolated);*/

/* TODO Get the priority of a slice in UL */
/*int flexran_get_ul_slice_priority(mid_t mod_id, int slice_idx);*/
/* TODO Set the priority of a slice in UL */
/*void flexran_set_ul_slice_priority(mid_t mod_id, int slice_idx, int priority);*/

/* Get the first RB for allocation in a slice in UL */
int flexran_get_ul_slice_first_rb(mid_t mod_id, int slice_idx);
/* Set the first RB for allocation in a slice in UL */
void flexran_set_ul_slice_first_rb(mid_t mod_id, int slice_idx, int first_rb);

/* TODO Get the number of RB for the allocation in a slice in UL */
/*int flexran_get_ul_slice_length_rb(mid_t mod_id, int slice_idx);*/
/* TODO Set the of number of RB for the allocation in a slice in UL */
/*void flexran_set_ul_slice_length_rb(mid_t mod_id, int slice_idx, int poshigh);*/

/* Get the maximum MCS for slice in UL */
int flexran_get_ul_slice_maxmcs(mid_t mod_id, int slice_idx);
/* Set the maximum MCS for slice in UL */
void flexran_set_ul_slice_maxmcs(mid_t mod_id, int slice_idx, int maxmcs);

/* TODO Get the sorting order of a slice in UL, return value is number of elements
 * in sorting_list */
/*int flexran_get_ul_slice_sorting(mid_t mod_id, int slice_idx, Protocol__FlexUlSorting **sorting_list);*/
/* TODO Set the sorting order of a slice in UL */
/*void flexran_set_ul_slice_sorting(mid_t mod_id, int slice_idx, Protocol__FlexUlSorting *sorting_list, int n);*/

/* TODO Get the accounting policy for a slice in UL */
/*Protocol__UlAccountingPolicy flexran_get_ul_slice_accounting_policy(mid_t mod_id, int slice_idx);*/
/* TODO Set the accounting policy for a slice in UL */
/*void flexran_get_ul_slice_accounting_policy(mid_t mod_id, int slice_idx, Protocol__UlAccountingPolicy accountin);*/

/* Get the scheduler name for a slice in UL */
char *flexran_get_ul_slice_scheduler(mid_t mod_id, int slice_idx);
/* Set the scheduler name for a slice in UL */
int flexran_set_ul_slice_scheduler(mid_t mod_id, int slice_idx, char *name);

/********************* general information *****************/
/* get an ID for this BS (or part of a BS) */
uint64_t flexran_get_bs_id(mid_t mod_id);

/* get the capabilities supported by the underlying network function,
 * returns the number and stores list of this length in caps. If there are zero
 * capabilities, *caps will be NULL */
size_t flexran_get_capabilities(mid_t mod_id, Protocol__FlexBsCapability **caps);

/* get the capabilities supported by the underlying network function as a bit
 * mask. */
uint16_t flexran_get_capabilities_mask(mid_t mod_id);
