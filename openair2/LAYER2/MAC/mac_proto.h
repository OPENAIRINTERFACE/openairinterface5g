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

/*! \file LAYER2/MAC/proto.h
 * \brief MAC functions prototypes for eNB and UE
 * \author Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email navid.nikaein@eurecom.fr
 * \version 1.0
 */
#ifndef __LAYER2_MAC_PROTO_H__
#define __LAYER2_MAC_PROTO_H__

#include "LAYER2/MAC/mac.h"
#include "PHY/defs_common.h" // for PRACH_RESOURCES_t and lte_subframe_t

/** \addtogroup _mac
 *  @{
 */

/** \fn void schedule_mib(module_id_t module_idP,frame_t frameP,sub_frame_t subframe);
\brief MIB scheduling for PBCH. This function requests the MIB from RRC and provides it to L1.
@param Mod_id Instance ID of eNB
@param frame Frame index
@param subframe Subframe number on which to act

*/

void schedule_mib(module_id_t module_idP,
		  frame_t frameP, sub_frame_t subframeP);

/** \fn void schedule_RA(module_id_t module_idP,frame_t frameP,sub_frame_t subframe);
\brief First stage of Random-Access Scheduling. Loops over the ras and checks if RAR, Msg3 or its retransmission are to be scheduled in the subframe.  It returns the total number of PRB used for RA SDUs.  For Msg3 it retrieves the L3msg from RRC and fills the appropriate buffers.  For the others it just computes the number of PRBs. Each DCI uses 3 PRBs (format 1A)
for the message.
@param Mod_id Instance ID of eNB
@param frame Frame index
@param subframe Subframe number on which to act
*/


void schedule_RA(module_id_t module_idP, frame_t frameP,
		 sub_frame_t subframe);

/** \brief First stage of SI Scheduling. Gets a SI SDU from RRC if available and computes the MCS required to transport it as a function of the SDU length.  It assumes a length less than or equal to 64 bytes (MCS 6, 3 PRBs).
@param Mod_id Instance ID of eNB
@param frame Frame index
@param subframe Subframe number on which to act
*/
void schedule_SI(module_id_t module_idP, frame_t frameP,
		 sub_frame_t subframeP);

/** \brief MBMS scheduling: Checking the position for MBSFN subframes. Create MSI, transfer MCCH from RRC to MAC, transfer MTCHs from RLC to MAC. Multiplexing MSI,MCCH&MTCHs. Return 1 if there are MBSFN data being allocated, otherwise return 0;
@param Mod_id Instance ID of eNB
@param frame Frame index
@param subframe Subframe number on which to act
*/
int schedule_MBMS(module_id_t module_idP, uint8_t CC_id, frame_t frameP,
		  sub_frame_t subframe);

/** \brief check the mapping between sf allocation and sync area, Currently only supports 1:1 mapping
@param Mod_id Instance ID of eNB
@param mbsfn_sync_area index of mbsfn sync area
@param[out] index of sf pattern
*/
int8_t get_mbsfn_sf_alloction(module_id_t module_idP, uint8_t CC_id,
			      uint8_t mbsfn_sync_area);

/** \brief check the mapping between sf allocation and sync area, Currently only supports 1:1 mapping
@param Mod_id Instance ID of eNB
@param mbsfn_sync_area index of mbsfn sync area
@param eNB_index index of eNB
@param[out] index of sf pattern
*/
int8_t ue_get_mbsfn_sf_alloction(module_id_t module_idP,
				 uint8_t mbsfn_sync_area,
				 unsigned char eNB_index);

/** \brief top ULSCH Scheduling for TDD (config 1-6).
@param Mod_id Instance ID of eNB
@param frame Frame index
@param subframe Subframe number on which to act
*/
void schedule_ulsch(module_id_t module_idP, frame_t frameP,
		    sub_frame_t subframe);

/** \brief ULSCH Scheduling per RNTI
@param Mod_id Instance ID of eNB
@param slice_idx Slice instance index for this eNB
@param frame Frame index
@param subframe Subframe number on which to act
@param sched_subframe Subframe number where PUSCH is transmitted (for DAI lookup)
*/
void schedule_ulsch_rnti(module_id_t module_idP, int slice_idx, frame_t frameP,
			 sub_frame_t subframe,
			 unsigned char sched_subframe,
			 uint16_t * first_rb);

void schedule_ulsch_rnti_emtc(module_id_t   module_idP,
			      frame_t       frameP,
			      sub_frame_t   subframeP,
			      unsigned char sched_subframeP,
			      int          *emtc_active);

/** \brief Second stage of DLSCH scheduling, after schedule_SI, schedule_RA and schedule_dlsch have been called.  This routine first allocates random frequency assignments for SI and RA SDUs using distributed VRB allocations and adds the corresponding DCI SDU to the DCI buffer for PHY.  It then loops over the UE specific DCIs previously allocated and fills in the remaining DCI fields related to frequency allocation.  It assumes localized allocation of type 0 (DCI.rah=0).  The allocation is done for tranmission modes 1,2,4.
@param Mod_id Instance of eNB
@param frame Frame index
@param subframe Index of subframe
@param mbsfn_flag Indicates that this subframe is for MCH/MCCH
*/
void fill_DLSCH_dci(module_id_t module_idP,frame_t frameP,sub_frame_t subframe,int *mbsfn_flag);

/** \brief UE specific DLSCH scheduling. Retrieves next ue to be schduled from round-robin scheduler and gets the appropriate harq_pid for the subframe from PHY. If the process is active and requires a retransmission, it schedules the retransmission with the same PRB count and MCS as the first transmission. Otherwise it consults RLC for DCCH/DTCH SDUs (status with maximum number of available PRBS), builds the MAC header (timing advance sent by default) and copies
@param Mod_id Instance ID of eNB
@param frame Frame index
@param subframe Subframe on which to act

@param mbsfn_flag  Indicates that MCH/MCCH is in this subframe
*/
void schedule_dlsch(module_id_t module_idP, frame_t frameP,
		      sub_frame_t subframe, int *mbsfn_flag);

void schedule_ue_spec(module_id_t module_idP, int slice_idxP,
		      frame_t frameP,sub_frame_t subframe, int *mbsfn_flag);
void schedule_ue_spec_br(module_id_t   module_idP,
			 frame_t       frameP,
			 sub_frame_t   subframeP);
void schedule_ue_spec_phy_test(module_id_t module_idP,frame_t frameP,sub_frame_t subframe,int *mbsfn_flag);
void schedule_ulsch_phy_test(module_id_t module_idP,frame_t frameP,sub_frame_t subframeP);


/** \brief Function for UE/PHY to compute PUSCH transmit power in power-control procedure.
    @param Mod_id Module id of UE
    @returns Po_NOMINAL_PUSCH (PREAMBLE_RECEIVED_TARGET_POWER+DELTA_PREAMBLE
*/
int8_t get_Po_NOMINAL_PUSCH(module_id_t module_idP, uint8_t CC_id);

/** \brief Function to compute DELTA_PREAMBLE from 36.321 (for RA power ramping procedure and Msg3 PUSCH power control policy)
    @param Mod_id Module id of UE
    @returns DELTA_PREAMBLE
*/
int8_t get_DELTA_PREAMBLE(module_id_t module_idP, int CC_id);

/** \brief Function for compute deltaP_rampup from 36.321 (for RA power ramping procedure and Msg3 PUSCH power control policy)
    @param Mod_id Module id of UE
    @param CC_id carrier component id of UE
    @returns deltaP_rampup
*/
int8_t get_deltaP_rampup(module_id_t module_idP, uint8_t CC_id);

uint16_t mac_computeRIV(uint16_t N_RB_DL, uint16_t RBstart,
			uint16_t Lcrbs);

void add_msg3(module_id_t module_idP, int CC_id, RA_t * ra, frame_t frameP,
	      sub_frame_t subframeP);

//main.c

void init_UE_list(UE_list_t *UE_list);

void init_slice_info(slice_info_t *sli);

int mac_top_init(int eMBMS_active, char *uecap_xer,
		 uint8_t cba_group_active, uint8_t HO_active);

void mac_top_init_eNB(void);

void mac_init_cell_params(int Mod_idP, int CC_idP);

char layer2_init_UE(module_id_t module_idP);

char layer2_init_eNB(module_id_t module_idP, uint8_t Free_ch_index);

void mac_switch_node_function(module_id_t module_idP);

int mac_init_global_param(void);

void mac_top_cleanup(void);

void mac_UE_out_of_sync_ind(module_id_t module_idP, frame_t frameP,
			    uint16_t eNB_index);

void clear_nfapi_information(eNB_MAC_INST * eNB, int CC_idP,
			     frame_t frameP, sub_frame_t subframeP);


// eNB functions
/* \brief This function assigns pre-available RBS to each UE in specified sub-bands before scheduling is done
@param Mod_id Instance ID of eNB
@param slice_idxP Slice instance index for the slice in which scheduling happens
@param frame Index of frame
@param subframe Index of current subframe
@param N_RBS Number of resource block groups
 */


void dlsch_scheduler_pre_processor(module_id_t module_idP,
                                   int slice_idxP,
                                   frame_t frameP,
                                   sub_frame_t subframe,
                                   int *mbsfn_flag,
                                   uint8_t rballoc_sub[NFAPI_CC_MAX][N_RBG_MAX]);

void dlsch_scheduler_pre_processor_reset(module_id_t module_idP,
                                         int slice_idx,
                                         frame_t frameP,
                                         sub_frame_t subframeP,
                                         int min_rb_unit[NFAPI_CC_MAX],
                                         uint16_t nb_rbs_required[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
                                         uint8_t rballoc_sub[NFAPI_CC_MAX][N_RBG_MAX],
                                         uint8_t MIMO_mode_indicator[NFAPI_CC_MAX][N_RBG_MAX],
                                         int *mbsfn_flag);

void dlsch_scheduler_pre_processor_partitioning(module_id_t Mod_id,
                                                int slice_idx,
                                                const uint8_t rbs_retx[NFAPI_CC_MAX]);

void dlsch_scheduler_pre_processor_accounting(module_id_t Mod_id,
                                              int slice_idx,
                                              frame_t frameP,
                                              sub_frame_t subframeP,
                                              int min_rb_unit[NFAPI_CC_MAX],
                                              uint16_t nb_rbs_required[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
                                              uint16_t nb_rbs_accounted[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB]);

void dlsch_scheduler_pre_processor_positioning(module_id_t Mod_id,
                                               int slice_idx,
                                               int min_rb_unit[NFAPI_CC_MAX],
                                               uint16_t nb_rbs_required[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
                                               uint16_t nb_rbs_accounted[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
                                               uint16_t nb_rbs_remaining[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
                                               uint8_t rballoc_sub[NFAPI_CC_MAX][N_RBG_MAX],
                                               uint8_t MIMO_mode_indicator[NFAPI_CC_MAX][N_RBG_MAX]);

void dlsch_scheduler_pre_processor_intraslice_sharing(module_id_t Mod_id,
                                                      int slice_idx,
                                                      int min_rb_unit[NFAPI_CC_MAX],
                                                      uint16_t nb_rbs_required[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
                                                      uint16_t nb_rbs_accounted[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
                                                      uint16_t nb_rbs_remaining[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
                                                      uint8_t rballoc_sub[NFAPI_CC_MAX][N_RBG_MAX],
                                                      uint8_t MIMO_mode_indicator[NFAPI_CC_MAX][N_RBG_MAX]);

void slice_priority_sort(module_id_t Mod_id, int slice_list[MAX_NUM_SLICES]);

void dlsch_scheduler_interslice_multiplexing(module_id_t Mod_id,
                                             int frameP,
                                             sub_frame_t subframeP,
                                             uint8_t rballoc_sub[NFAPI_CC_MAX][N_RBG_MAX]);

void dlsch_scheduler_qos_multiplexing(module_id_t Mod_id,
                                      int frameP,
                                      sub_frame_t subframeP);

void dlsch_scheduler_pre_processor_allocate(module_id_t Mod_id,
                                            int UE_id,
                                            uint8_t CC_id,
                                            int N_RBG,
                                            int min_rb_unit,
                                            uint16_t nb_rbs_required[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
                                            uint16_t nb_rbs_remaining[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
                                            uint8_t rballoc_sub[NFAPI_CC_MAX][N_RBG_MAX],
                                            uint8_t slice_allocation_mask[NFAPI_CC_MAX][N_RBG_MAX],
                                            uint8_t MIMO_mode_indicator[NFAPI_CC_MAX][N_RBG_MAX]);

/* \brief Function to trigger the eNB scheduling procedure.  It is called by PHY at the beginning of each subframe, \f$n$\f
   and generates all DLSCH allocations for subframe \f$n\f$ and ULSCH allocations for subframe \f$n+k$\f. 
@param Mod_id Instance ID of eNB
@param subframe Index of current subframe
@param calibration_flag Flag to indicate that eNB scheduler should schedule TDD auto-calibration PUSCH.
*/
void eNB_dlsch_ulsch_scheduler(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP);	//, int calibration_flag);

/* \brief Function to indicate a received preamble on PRACH.  It initiates the RA procedure.
@param Mod_id Instance ID of eNB
@param preamble_index index of the received RA request
@param timing_offset Offset in samples of the received PRACH w.r.t. eNB timing. This is used to
@param rnti RA rnti corresponding to this PRACH preamble
@param rach_resource type (0=non BL/CE,1 CE level 0,2 CE level 1, 3 CE level 2,4 CE level 3)
*/
void initiate_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP,
		      sub_frame_t subframeP, uint16_t preamble_index,
		      int16_t timing_offset, uint16_t rnti
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
		      , uint8_t rach_resource_type
#endif
    );

/* \brief Function in eNB to fill RAR pdu when requested by PHY.  This provides a single RAR SDU for the moment and returns the t-CRNTI.
@param Mod_id Instance ID of eNB
@param dlsch_buffer Pointer to DLSCH input buffer
@param N_RB_UL Number of UL resource blocks
@returns t_CRNTI
*/
unsigned short fill_rar(const module_id_t module_idP,
			const int CC_id,
			RA_t *ra,
			const frame_t frameP,
			uint8_t * const dlsch_buffer,
			const uint16_t N_RB_UL,
			const uint8_t input_buffer_length);

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
unsigned short fill_rar_br(eNB_MAC_INST * eNB,
			   int CC_id,
			   RA_t * ra,
			   const frame_t frameP,
			   const sub_frame_t subframeP,
			   uint8_t * const dlsch_buffer,
			   const uint8_t ce_level);
#endif

/* \brief Function to indicate a failed RA response.  It removes all temporary variables related to the initial connection of a UE
@param Mod_id Instance ID of eNB
@param preamble_index index of the received RA request.
*/
void cancel_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP,
		    uint16_t preamble_index);

/* \brief Function used by PHY to inform MAC that an uplink is scheduled
          for Msg3 in given subframe. This is used so that the MAC
          scheduler marks as busy the RBs used by the Msg3.
@param Mod_id        Instance ID of eNB
@param CC_id         CC ID of eNB
@param frame         current frame
@param subframe      current subframe
@param rnti          UE rnti concerned
@param Msg3_frame    frame where scheduling takes place
@param Msg3_subframe subframe where scheduling takes place
*/

void clear_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP);

void set_msg3_subframe(module_id_t Mod_id,
		       int CC_id,
		       int frame,
		       int subframe,
		       int rnti, int Msg3_frame, int Msg3_subframe);

/* \brief Function to indicate a received SDU on ULSCH.
@param Mod_id Instance ID of eNB
@param CC_id Component carrier index
@param rnti RNTI of UE transmitting the SDU
@param sdu Pointer to received SDU
@param sdu_len Length of SDU
@param timing_advance timing advance adjustment after this pdu
@param ul_cqi Uplink CQI estimate after this pdu (SNR quantized to 8 bits, -64 ... 63.5 dB in .5dB steps)
*/
void rx_sdu(const module_id_t enb_mod_idP,
	    const int CC_idP,
	    const frame_t frameP,
	    const sub_frame_t subframeP,
	    const rnti_t rntiP,
	    uint8_t * sduP,
	    const uint16_t sdu_lenP,
	    const uint16_t timing_advance, const uint8_t ul_cqi);


/* \brief Function to indicate a scheduled schduling request (SR) was received by eNB.
@param Mod_idP Instance ID of eNB
@param CC_idP CC_id of received SR
@param frameP of received SR
@param subframeP Index of subframe where SR was received
@param rnti RNTI of UE transmitting the SR
@param ul_cqi SNR measurement of PUCCH (SNR quantized to 8 bits, -64 ... 63.5 dB in .5dB steps)
*/
void SR_indication(module_id_t module_idP, int CC_id, frame_t frameP,
		   sub_frame_t subframe, rnti_t rnti, uint8_t ul_cqi);

/* \brief Function to indicate a UL failure was detected by eNB PHY.
@param Mod_id Instance ID of eNB
@param CC_id Component carrier
@param frameP Frame index
@param rnti RNTI of UE transmitting the SR
@param subframe Index of subframe where SR was received
*/
void UL_failure_indication(module_id_t Mod_id, int CC_id, frame_t frameP,
			   rnti_t rnti, sub_frame_t subframe);

/* \brief Function to indicate an HARQ ACK/NAK.
@param Mod_id Instance ID of eNB
@param CC_id Component carrier
@param frameP Frame index
@param subframeP subframe index
@param harq_pdu NFAPI HARQ PDU descriptor
*/
void harq_indication(module_id_t mod_idP, int CC_idP, frame_t frameP,
		     sub_frame_t subframeP,
		     nfapi_harq_indication_pdu_t * harq_pdu);

/* \brief Function to indicate a received CQI pdu
@param Mod_id Instance ID of eNB
@param CC_id Component carrier
@param frameP Frame index
@param subframeP subframe index
@param rntiP RNTI of incoming CQI information
@param ul_cqi_information NFAPI UL CQI measurement
*/
void cqi_indication(module_id_t mod_idP, int CC_idP, frame_t frameP,
		    sub_frame_t subframeP, rnti_t rntiP,
		    nfapi_cqi_indication_rel9_t * rel9, uint8_t * pdu,
		    nfapi_ul_cqi_information_t * ul_cqi_information);

uint8_t *get_dlsch_sdu(module_id_t module_idP, int CC_id, frame_t frameP,
		       rnti_t rnti, uint8_t TBindex);

/* \brief Function to retrieve MCH transport block and MCS used for MCH in this MBSFN subframe.  Returns null if no MCH is to be transmitted
@param Mod_id Instance ID of eNB
@param frame Index of frame
@param subframe Index of current subframe
@param mcs Pointer to mcs used by PHY (to be filled by MAC)
@returns Pointer to MCH transport block and mcs for subframe
*/
MCH_PDU *get_mch_sdu(module_id_t Mod_id, int CC_id, frame_t frame,
		     sub_frame_t subframe);





void ue_mac_reset(module_id_t module_idP, uint8_t eNB_index);
void ue_init_mac(module_id_t module_idP);
void init_ue_sched_info(void);
void add_ue_ulsch_info(module_id_t module_idP, int CC_id, int UE_id,
		       sub_frame_t subframe, UE_ULSCH_STATUS status);
void add_ue_dlsch_info(module_id_t module_idP, int CC_id, int UE_id,
		                   sub_frame_t subframe, UE_DLSCH_STATUS status, rnti_t rnti);
int find_UE_id(module_id_t module_idP, rnti_t rnti);
int find_RA_id(module_id_t mod_idP, int CC_idP, rnti_t rntiP);
rnti_t UE_RNTI(module_id_t module_idP, int UE_id);
int UE_PCCID(module_id_t module_idP, int UE_id);
uint8_t find_active_UEs(module_id_t module_idP);
boolean_t is_UE_active(module_id_t module_idP, int UE_id);
uint8_t get_aggregation(uint8_t bw_index, uint8_t cqi, uint8_t dci_fmt);

int8_t find_active_UEs_with_traffic(module_id_t module_idP);

void init_CCE_table(int *CCE_table);

int get_nCCE_offset(int *CCE_table,
		    const unsigned char L,
		    const int nCCE,
		    const int common_dci,
		    const unsigned short rnti,
		    const unsigned char subframe);

int allocate_CCEs(int module_idP, int CC_idP, frame_t frameP, sub_frame_t subframeP, int test_only);

boolean_t CCE_allocation_infeasible(int module_idP,
				    int CC_idP,
				    int common_flag,
				    int subframe,
				    int aggregation, int rnti);

void set_ue_dai(sub_frame_t subframeP,
		int UE_id,
		uint8_t CC_id, uint8_t tdd_config, UE_list_t * UE_list);

uint8_t frame_subframe2_dl_harq_pid(LTE_TDD_Config_t *tdd_Config, int abs_frameP, sub_frame_t subframeP);
/** \brief First stage of PCH Scheduling. Gets a PCH SDU from RRC if available and computes the MCS required to transport it as a function of the SDU length.  It assumes a length less than or equal to 64 bytes (MCS 6, 3 PRBs).
@param Mod_id Instance ID of eNB
@param frame Frame index
@param subframe Subframe number on which to act
@param paging_ue_index
*/
void schedule_PCH(module_id_t module_idP,frame_t frameP,sub_frame_t subframeP);

uint8_t find_num_active_UEs_in_cbagroup(module_id_t module_idP,
					unsigned char group_id);
uint8_t UE_is_to_be_scheduled(module_id_t module_idP, int CC_id,
			      uint8_t UE_id);
/** \brief Round-robin scheduler for ULSCH traffic.
@param Mod_id Instance ID for eNB
@param subframe Subframe number on which to act
@returns UE index that is to be scheduled if needed/room
*/
module_id_t schedule_next_ulue(module_id_t module_idP, int UE_id,
			       sub_frame_t subframe);

/** \brief Round-robin scheduler for DLSCH traffic.
@param Mod_id Instance ID for eNB
@param subframe Subframe number on which to act
@returns UE index that is to be scheduled if needed/room
*/
int schedule_next_dlue(module_id_t module_idP, int CC_id,
		       sub_frame_t subframe);

/* \brief Allocates a set of PRBS for a particular UE.  This is a simple function for the moment, later it should process frequency-domain CQI information and/or PMI information.  Currently it just returns the first PRBS that are available in the subframe based on the number requested.
@param UE_id Index of UE on which to act
@param nb_rb Number of PRBs allocated to UE by scheduler
@param N_RB_DL Number of PRBs on DL
@param rballoc Pointer to bit-map of current PRB allocation given to previous users/control channels.  This is updated for subsequent calls to the routine.
@returns an rballoc bitmap for resource type 0 allocation (DCI).
*/
uint32_t allocate_prbs(int UE_id, uint8_t nb_rb, int N_RB_DL,
		       uint32_t * rballoc);

/* \fn uint32_t req_new_ulsch(module_id_t module_idP)
\brief check for a new transmission in any drb
@param Mod_id Instance id of UE in machine
@returns 1 for new transmission, 0 for none
*/
uint32_t req_new_ulsch(module_id_t module_idP);

/* \brief Get SR payload (0,1) from UE MAC
@param Mod_id Instance id of UE in machine
@param CC_id Component Carrier index
@param eNB_id Index of eNB that UE is attached to
@param rnti C_RNTI of UE
@param subframe subframe number
@returns 0 for no SR, 1 for SR
*/
uint32_t ue_get_SR(module_id_t module_idP, int CC_id, frame_t frameP,
		   uint8_t eNB_id, rnti_t rnti, sub_frame_t subframe);

uint8_t get_ue_weight(module_id_t module_idP, int CC_id, int UE_id);

// UE functions
void mac_out_of_sync_ind(module_id_t module_idP, frame_t frameP,
			 uint16_t CH_index);

void ue_decode_si(module_id_t module_idP, int CC_id, frame_t frame,
		  uint8_t CH_index, void *pdu, uint16_t len);

void ue_decode_p(module_id_t module_idP, int CC_id, frame_t frame,
		 uint8_t CH_index, void *pdu, uint16_t len);


void ue_send_sdu(module_id_t module_idP, uint8_t CC_id, frame_t frame,
		 sub_frame_t subframe, uint8_t * sdu, uint16_t sdu_len,
		 uint8_t CH_index);

void ue_send_sl_sdu(module_id_t module_idP,
		    uint8_t CC_id,
		    frame_t frameP,
		    sub_frame_t subframeP,
		    uint8_t* sdu,
		    uint16_t sdu_len,
		    uint8_t eNB_index,
	       sl_discovery_flag_t sl_discovery_flag
		    );

#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
/* \brief Called by PHY to transfer MCH transport block to ue MAC.
@param Mod_id Index of module instance
@param frame Frame index
@param sdu Pointer to transport block
@param sdu_len Length of transport block
@param eNB_index Index of attached eNB
@param sync_area the index of MBSFN sync area
*/
void ue_send_mch_sdu(module_id_t module_idP, uint8_t CC_id, frame_t frameP,
		     uint8_t * sdu, uint16_t sdu_len, uint8_t eNB_index,
		     uint8_t sync_area);

/*\brief Function to check if UE PHY needs to decode MCH for MAC.
@param Mod_id Index of protocol instance
@param frame Index of frame
@param subframe Index of subframe
@param eNB_index index of eNB for this MCH
@param[out] sync_area return the sync area
@param[out] mcch_active flag indicating whether this MCCH is active in this SF
*/
int ue_query_mch(module_id_t Mod_id, uint8_t CC_id, uint32_t frame,
		 sub_frame_t subframe, uint8_t eNB_index,
		 uint8_t * sync_area, uint8_t * mcch_active);

#endif

/* \brief Called by PHY to get sdu for PUSCH transmission.  It performs the following operations: Checks BSR for DCCH, DCCH1 and DTCH corresponding to previous values computed either in SR or BSR procedures.  It gets rlc status indications on DCCH,DCCH1 and DTCH and forms BSR elements and PHR in MAC header.  CRNTI element is not supported yet.  It computes transport block for up to 3 SDUs and generates header and forms the complete MAC SDU.
@param Mod_id Instance id of UE in machine
@param eNB_id Index of eNB that UE is attached to
@param rnti C_RNTI of UE
@param subframe subframe number
*/
void ue_get_sdu(module_id_t module_idP, int CC_id, frame_t frameP,
		sub_frame_t subframe, uint8_t eNB_index,
		uint8_t * ulsch_buffer, uint16_t buflen,
		uint8_t * access_mode);

/* \brief Called by PHY to get sdu for PSBCH/SSS/PSS transmission.
@param Mod_id Instance id of UE in machine
@param frame_tx TX frame index
@param subframe_tx TX subframe index
@returns pointer to SLSS_t descriptor
*/
SLSS_t *ue_get_slss(module_id_t module_idP, int CC_id,frame_t frameP, sub_frame_t subframe);

/* \brief Called by PHY to get sdu for PSDCH transmission.
@param Mod_id Instance id of UE in machine
@param frame_tx TX frame index
@param subframe_tx TX subframe index
@returns pointer to SLDCH_t descriptor
*/
SLDCH_t *ue_get_sldch(module_id_t module_idP, int CC_id,frame_t frameP, sub_frame_t subframe);

/* \brief Called by PHY to get sdu for PSSCH transmission.
@param Mod_id Instance id of UE in machine
@param frame_tx TX frame index
@param subframe_tx TX subframe index
@returns pointer to SLSCH_t descriptor
*/
SLSCH_t *ue_get_slsch(module_id_t module_idP, int CC_id,frame_t frameP, sub_frame_t subframe);

/* \brief Function called by PHY to retrieve information to be transmitted using the RA procedure.  If the UE is not in PUSCH mode for a particular eNB index, this is assumed to be an Msg3 and MAC attempts to retrieves the CCCH message from RRC. If the UE is in PUSCH mode for a particular eNB index and PUCCH format 0 (Scheduling Request) is not activated, the MAC may use this resource for random-access to transmit a BSR along with the C-RNTI control element (see 5.1.4 from 36.321)
@param Mod_id Index of UE instance
@param Mod_id Component Carrier Index
@param New_Msg3 Flag to indicate this call is for a new Msg3
@param subframe Index of subframe for PRACH transmission (0 ... 9)
@returns A pointer to a PRACH_RESOURCES_t */
PRACH_RESOURCES_t *ue_get_rach(module_id_t module_idP, int CC_id,
			       frame_t frameP, uint8_t new_Msg3,
			       sub_frame_t subframe);

/* \brief Function called by PHY to process the received RAR.  It checks that the preamble matches what was sent by the eNB and provides the timing advance and t-CRNTI.
@param Mod_id Index of UE instance
@param CC_id Index to a component carrier
@param frame Frame index
@param ra_rnti RA_RNTI value
@param dlsch_buffer  Pointer to dlsch_buffer containing RAR PDU
@param t_crnti Pointer to PHY variable containing the T_CRNTI
@param preamble_index Preamble Index used by PHY to transmit the PRACH.  This should match the received RAR to trigger the rest of
random-access procedure
@param selected_rar_buffer the output buffer for storing the selected RAR header and RAR payload
@returns timing advance or 0xffff if preamble doesn't match
*/
uint16_t
ue_process_rar(const module_id_t module_idP,
	       const int CC_id,
	       const frame_t frameP,
	       const rnti_t ra_rnti,
	       uint8_t * const dlsch_buffer,
	       rnti_t * const t_crnti,
	       const uint8_t preamble_index,
	       uint8_t * selected_rar_buffer);


/* \brief Generate header for UL-SCH.  This function parses the desired control elements and sdus and generates the header as described
in 36-321 MAC layer specifications.  It returns the number of bytes used for the header to be used as an offset for the payload
in the ULSCH buffer.
@param mac_header Pointer to the first byte of the MAC header (UL-SCH buffer)
@param num_sdus Number of SDUs in the payload
@param short_padding Number of bytes for short padding (0,1,2)
@param sdu_lengths Pointer to array of SDU lengths
@param sdu_lcids Pointer to array of LCIDs (the order must be the same as the SDU length array)
@param power_headroom Pointer to power headroom command (NULL means not present in payload)
@param crnti Pointer to CRNTI command (NULL means not present in payload)
@param truncated_bsr Pointer to Truncated BSR command (NULL means not present in payload)
@param short_bsr Pointer to Short BSR command (NULL means not present in payload)
@param long_bsr Pointer to Long BSR command (NULL means not present in payload)
@param post_padding Number of bytes for padding at the end of MAC PDU
@returns Number of bytes used for header
*/
unsigned char generate_ulsch_header(uint8_t * mac_header,
				    uint8_t num_sdus,
				    uint8_t short_padding,
				    uint16_t * sdu_lengths,
				    uint8_t * sdu_lcids,
				    POWER_HEADROOM_CMD * power_headroom,
				    uint16_t * crnti,
				    BSR_SHORT * truncated_bsr,
				    BSR_SHORT * short_bsr,
				    BSR_LONG * long_bsr,
				    unsigned short post_padding);

/* \brief Parse header for UL-SCH.  This function parses the received UL-SCH header as described
in 36-321 MAC layer specifications.  It returns the number of bytes used for the header to be used as an offset for the payload
in the ULSCH buffer.
@param mac_header Pointer to the first byte of the MAC header (UL-SCH buffer)
@param num_ces Number of SDUs in the payload
@param num_sdu Number of SDUs in the payload
@param rx_ces Pointer to received CEs in the header
@param rx_lcids Pointer to array of LCIDs (the order must be the same as the SDU length array)
@param rx_lengths Pointer to array of SDU lengths
@returns Pointer to payload following header
*/
uint8_t *parse_ulsch_header(uint8_t * mac_header,
			    uint8_t * num_ce,
			    uint8_t * num_sdu,
			    uint8_t * rx_ces,
			    uint8_t * rx_lcids,
			    uint16_t * rx_lengths, uint16_t tx_lenght);

int to_prb(int);
int to_rbg(int);
int mac_init(void);
int add_new_ue(module_id_t Mod_id, int CC_id, rnti_t rnti, int harq_pid
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
	       , uint8_t rach_resource_type
#endif
    );
int rrc_mac_remove_ue(module_id_t Mod_id, rnti_t rntiP);

void store_dlsch_buffer(module_id_t Mod_id, int slice_idx, frame_t frameP, sub_frame_t subframeP);
void assign_rbs_required(module_id_t Mod_id, int slice_idx, frame_t frameP, sub_frame_t subframe, uint16_t nb_rbs_required[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB], int min_rb_unit[NFAPI_CC_MAX]);

void swap_UEs(UE_list_t * listP, int nodeiP, int nodejP, int ul_flag);
int prev(UE_list_t * listP, int nodeP, int ul_flag);
void dump_ue_list(UE_list_t * listP, int ul_flag);
int UE_num_active_CC(UE_list_t * listP, int ue_idP);
int UE_PCCID(module_id_t mod_idP, int ue_idP);
rnti_t UE_RNTI(module_id_t mod_idP, int ue_idP);

uint8_t find_rb_table_index(uint8_t average_rbs);

void set_ul_DAI(int module_idP,
                int UE_idP,
                int CC_idP,
                int frameP,
                int subframeP);

void ulsch_scheduler_pre_processor(module_id_t module_idP, int slice_idx, int frameP,
				   sub_frame_t subframeP,
                                   int sched_frameP,
                                   unsigned char sched_subframeP,
				   uint16_t * first_rb);
void store_ulsch_buffer(module_id_t module_idP, int frameP,
			sub_frame_t subframeP);
void assign_max_mcs_min_rb(module_id_t module_idP, int slice_idx, int frameP,
			   sub_frame_t subframeP, uint16_t * first_rb);
void adjust_bsr_info(int buffer_occupancy, uint16_t TBS,
		     UE_TEMPLATE * UE_template);

int phy_stats_exist(module_id_t Mod_id, int rnti);
void sort_UEs(module_id_t Mod_idP, int slice_idx, int frameP, sub_frame_t subframeP);

/*! \fn  UE_L2_state_t ue_scheduler(const module_id_t module_idP,const frame_t frameP, const sub_frame_t subframe, const lte_subframe_t direction,const uint8_t eNB_index)
   \brief UE scheduler where all the ue background tasks are done.  This function performs the following:  1) Trigger PDCP every 5ms 2) Call RRC for link status return to PHY3) Perform SR/BSR procedures for scheduling feedback 4) Perform PHR procedures.
\param[in] module_idP instance of the UE
\param[in] rxFrame the RX frame number
\param[in] rxSubframe the RX subframe number
\param[in] txFrame the TX frame number
\param[in] txSubframe the TX subframe number
\param[in] direction  subframe direction
\param[in] eNB_index  instance of eNB
@returns L2 state (CONNETION_OK or CONNECTION_LOST or PHY_RESYNCH)
*/
UE_L2_STATE_t ue_scheduler(const module_id_t module_idP,
			   const frame_t rxFrameP,
			   const sub_frame_t rxSubframe,
			   const frame_t txFrameP,
			   const sub_frame_t txSubframe,
			   const lte_subframe_t direction,
			   const uint8_t eNB_index, const int CC_id);

/*! \fn  int cba_access(module_id_t module_idP,frame_t frameP,sub_frame_t subframe, uint8_t eNB_index,uint16_t buflen);
\brief determine whether to use cba resource to transmit or not
\param[in] Mod_id instance of the UE
\param[in] frame the frame number
\param[in] subframe the subframe number
\param[in] eNB_index instance of eNB
\param[out] access(1) or postpone (0)
*/
int cba_access(module_id_t module_idP, frame_t frameP,
	       sub_frame_t subframe, uint8_t eNB_index, uint16_t buflen);

/*! \fn  BSR_SHORT *  get_bsr_short(module_id_t module_idP, uint8_t bsr_len)
\brief get short bsr level
\param[in] Mod_id instance of the UE
\param[in] bsr_len indicator for no, short, or long bsr
\param[out] bsr_s pointer to short bsr
*/
BSR_SHORT *get_bsr_short(module_id_t module_idP, uint8_t bsr_len);

/*! \fn  BSR_LONG * get_bsr_long(module_id_t module_idP, uint8_t bsr_len)
\brief get long bsr level
\param[in] Mod_id instance of the UE
\param[in] bsr_len indicator for no, short, or long bsr
\param[out] bsr_l pointer to long bsr
*/
BSR_LONG *get_bsr_long(module_id_t module_idP, uint8_t bsr_len);

/*! \fn  boolean_t update_bsr(module_id_t module_idP, frame_t frameP,sub_frame_t subframeP)
   \brief get the rlc stats and update the bsr level for each lcid
\param[in] Mod_id instance of the UE
\param[in] frame Frame index
*/
boolean_t update_bsr(module_id_t module_idP, frame_t frameP,
		     sub_frame_t subframeP, eNB_index_t eNB_index);

/*! \fn  locate_BsrIndexByBufferSize (int *table, int size, int value)
   \brief locate the BSR level in the table as defined in 36.321. This function requires that he values in table to be monotonic, either increasing or decreasing. The returned value is not less than 0, nor greater than n-1, where n is the size of table.
\param[in] *table Pointer to BSR table
\param[in] size Size of the table
\param[in] value Value of the buffer
\return the index in the BSR_LEVEL table
*/
uint8_t locate_BsrIndexByBufferSize(const uint32_t * table, int size,
				    int value);


/*! \fn  int get_sf_periodicBSRTimer(uint8_t periodicBSR_Timer)
   \brief get the number of subframe from the periodic BSR timer configured by the higher layers
\param[in] periodicBSR_Timer timer for periodic BSR
\return the number of subframe
*/
int get_sf_periodicBSRTimer(uint8_t bucketSize);

/*! \fn  int get_ms_bucketsizeduration(uint8_t bucketSize)
   \brief get the time in ms form the bucket size duration configured by the higher layer
\param[in]  bucketSize the bucket size duration
\return the time in ms
*/
int get_ms_bucketsizeduration(uint8_t bucketsizeduration);

/*! \fn  int get_sf_retxBSRTimer(uint8_t retxBSR_Timer)
   \brief get the number of subframe form the bucket size duration configured by the higher layer
\param[in]  retxBSR_Timer timer for regular BSR
\return the time in sf
*/
int get_sf_retxBSRTimer(uint8_t retxBSR_Timer);

/*! \fn  int get_sf_perioidicPHR_Timer(uint8_t perioidicPHR_Timer){
   \brief get the number of subframe form the periodic PHR timer configured by the higher layer
\param[in]  perioidicPHR_Timer timer for reguluar PHR
\return the time in sf
*/
int get_sf_perioidicPHR_Timer(uint8_t perioidicPHR_Timer);

/*! \fn  int get_sf_prohibitPHR_Timer(uint8_t prohibitPHR_Timer)
   \brief get the number of subframe form the prohibit PHR duration configured by the higher layer
\param[in]  prohibitPHR_Timer timer for  PHR
\return the time in sf
*/
int get_sf_prohibitPHR_Timer(uint8_t prohibitPHR_Timer);

/*! \fn  int get_db_dl_PathlossChange(uint8_t dl_PathlossChange)
   \brief get the db form the path loss change configured by the higher layer
\param[in]  dl_PathlossChange path loss for PHR
\return the pathloss in db
*/
int get_db_dl_PathlossChange(uint8_t dl_PathlossChange);

/*! \fn  uint8_t get_phr_mapping (module_id_t module_idP, int CC_id,uint8_t eNB_index)
   \brief get phr mapping as described in 36.313
\param[in]  Mod_id index of eNB
\param[in] CC_id Component Carrier Index
\return phr mapping
*/
uint8_t get_phr_mapping(module_id_t module_idP, int CC_id,
			uint8_t eNB_index);

/*! \fn  void update_phr (module_id_t module_idP)
   \brief update/reset the phr timers
\param[in]  Mod_id index of eNB
\param[in] CC_id Component carrier index
\return void
*/
void update_phr(module_id_t module_idP, int CC_id);

/*! \brief Function to indicate Msg3 transmission/retransmission which initiates/reset Contention Resolution Timer
\param[in] Mod_id Instance index of UE
\param[in] eNB_id Index of eNB
*/
void Msg3_tx(module_id_t module_idP, uint8_t CC_id, frame_t frameP,
	     uint8_t eNB_id);


/*! \brief Function to indicate the transmission of msg1/rach
\param[in] Mod_id Instance index of UE
\param[in] eNB_id Index of eNB
*/

void Msg1_tx(module_id_t module_idP, uint8_t CC_id, frame_t frameP,
	     uint8_t eNB_id);

void dl_phy_sync_success(module_id_t module_idP,
			 frame_t frameP,
			 unsigned char eNB_index, uint8_t first_sync);

int dump_eNB_l2_stats(char *buffer, int length);

double uniform_rngen(int min, int max);

/*
void add_common_dci(DCI_PDU *DCI_pdu,
                    void *pdu,
                    rnti_t rnti,
                    unsigned char dci_size_bytes,
                    unsigned char aggregation,
                    unsigned char dci_size_bits,
                    unsigned char dci_fmt,
                    uint8_t ra_flag);
*/

uint32_t allocate_prbs_sub(int nb_rb, int N_RB_DL, int N_RBG,
			   uint8_t * rballoc);

void update_ul_dci(module_id_t module_idP, uint8_t CC_id, rnti_t rnti,
		   uint8_t dai, sub_frame_t subframe);

int get_bw_index(module_id_t module_id, uint8_t CC_id);

int get_min_rb_unit(module_id_t module_idP, uint8_t CC_id);

/* \brief Generate header for DL-SCH.  This function parses the desired control elements and sdus and generates the header as described
in 36-321 MAC layer specifications.  It returns the number of bytes used for the header to be used as an offset for the payload
in the DLSCH buffer.
@param mac_header Pointer to the first byte of the MAC header (DL-SCH buffer)
@param num_sdus Number of SDUs in the payload
@param sdu_lengths Pointer to array of SDU lengths
@param sdu_lcids Pointer to array of LCIDs (the order must be the same as the SDU length array)
@param drx_cmd dicontinous reception command
@param timing_advancd_cmd timing advanced command
@param ue_cont_res_id Pointer to contention resolution identifier (NULL means not present in payload)
@param short_padding Number of bytes for short padding (0,1,2)
@param post_padding number of bytes for padding at the end of MAC PDU
@returns Number of bytes used for header
*/
int generate_dlsch_header(unsigned char *mac_header,
                          unsigned char num_sdus,
                          unsigned short *sdu_lengths,
                          unsigned char *sdu_lcids,
                          unsigned char drx_cmd,
                          unsigned short timing_advance_cmd,
                          unsigned char *ue_cont_res_id,
                          unsigned char short_padding,
                          unsigned short post_padding);

/** \brief RRC eNB Configuration primitive for PHY/MAC.  Allows configuration of PHY/MAC resources based on System Information (SI), RRCConnectionSetup and RRCConnectionReconfiguration messages.
@param Mod_id Instance ID of eNB
@param CC_id Component Carrier of the eNB
@param mib Pointer to MIB

@param radioResourceConfigCommon Structure from SIB2 for common radio parameters (if NULL keep existing configuration)
@param physicalConfigDedicated Structure from RRCConnectionSetup or RRCConnectionReconfiguration for dedicated PHY parameters (if NULL keep existing configuration)
@param measObj Structure from RRCConnectionReconfiguration for UE measurement procedures
@param mac_MainConfig Structure from RRCConnectionSetup or RRCConnectionReconfiguration for dedicated MAC parameters (if NULL keep existing configuration)
@param logicalChannelIdentity Logical channel identity index of corresponding logical channel config
@param logicalChannelConfig Pointer to logical channel configuration
@param measGapConfig Measurement Gap configuration for MAC (if NULL keep existing configuration)
@param tdd_Config TDD Configuration from SIB1 (if NULL keep existing configuration)
@param mobilityControlInfo mobility control info received for Handover
@param SchedInfoList SI Scheduling information
@param MBMS_Flag indicates MBMS transmission
@param mbsfn_SubframeConfigList pointer to mbsfn subframe configuration list from SIB2
@param mbsfn_AreaInfoList pointer to MBSFN Area Info list from SIB13
@param pmch_InfoList pointer to PMCH_InfoList from MBSFNAreaConfiguration Message (MCCH Message)
@param sib1_ext_r13 SI Scheduling information for SI-BR UEs         
*/

int rrc_mac_config_req_eNB(module_id_t module_idP,
			   int CC_id,
			   int physCellId,
			   int p_eNB,
			   int Ncp,
			   int eutra_band, uint32_t dl_CarrierFreq,
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
			   int pbch_repetition,
#endif
			   rnti_t rntiP,
			   LTE_BCCH_BCH_Message_t * mib,
			   LTE_RadioResourceConfigCommonSIB_t *
			   radioResourceConfigCommon,
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
			   LTE_RadioResourceConfigCommonSIB_t *
			   LTE_radioResourceConfigCommon_BR,
#endif
			   struct LTE_PhysicalConfigDedicated
			   *physicalConfigDedicated,
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
			   LTE_SCellToAddMod_r10_t * sCellToAddMod_r10,
			   //struct PhysicalConfigDedicatedSCell_r10 *physicalConfigDedicatedSCell_r10,
#endif
			   LTE_MeasObjectToAddMod_t ** measObj,
			   LTE_MAC_MainConfig_t * mac_MainConfig,
			   long logicalChannelIdentity,
			   LTE_LogicalChannelConfig_t * logicalChannelConfig,
			   LTE_MeasGapConfig_t * measGapConfig,
			   LTE_TDD_Config_t * tdd_Config,
			   LTE_MobilityControlInfo_t * mobilityControlInfo,
			   LTE_SchedulingInfoList_t * schedulingInfoList,
			   uint32_t ul_CarrierFreq,
			   long *ul_Bandwidth,
			   LTE_AdditionalSpectrumEmission_t *
			   additionalSpectrumEmission,
			   struct LTE_MBSFN_SubframeConfigList
			   *mbsfn_SubframeConfigList
#if (LTE_RRC_VERSION >= MAKE_VERSION(9, 0, 0))
			   ,
			   uint8_t MBMS_Flag,
			   LTE_MBSFN_AreaInfoList_r9_t * mbsfn_AreaInfoList,
			   LTE_PMCH_InfoList_r9_t * pmch_InfoList
#endif
#if (LTE_RRC_VERSION >= MAKE_VERSION(13, 0, 0))
			   ,
			   LTE_SystemInformationBlockType1_v1310_IEs_t *
			   sib1_ext_r13
#endif
    );

/** \brief RRC eNB Configuration primitive for PHY/MAC.  Allows configuration of PHY/MAC resources based on System Information (SI), RRCConnectionSetup and RRCConnectionReconfiguration messages.
@param Mod_id Instance ID of ue
@param CC_id Component Carrier of the ue
@param eNB_id Index of eNB
@param radioResourceConfigCommon Structure from SIB2 for common radio parameters (if NULL keep existing configuration)
@param physcialConfigDedicated Structure from RRCConnectionSetup or RRCConnectionReconfiguration for dedicated PHY parameters (if NULL keep existing configuration)
@param measObj Structure from RRCConnectionReconfiguration for UE measurement procedures
@param mac_MainConfig Structure from RRCConnectionSetup or RRCConnectionReconfiguration for dedicated MAC parameters (if NULL keep existing configuration)
@param logicalChannelIdentity Logical channel identity index of corresponding logical channel config
@param logicalChannelConfig Pointer to logical channel configuration
@param measGapConfig Measurement Gap configuration for MAC (if NULL keep existing configuration)
@param tdd_Config TDD Configuration from SIB1 (if NULL keep existing configuration)
@param mobilityControlInfo mobility control info received for Handover
@param SIwindowsize SI Windowsize from SIB1 (if NULL keep existing configuration)
@param SIperiod SI Period from SIB1 (if NULL keep existing configuration)
@param MBMS_Flag indicates MBMS transmission
@param mbsfn_SubframeConfigList pointer to mbsfn subframe configuration list from SIB2
@param mbsfn_AreaInfoList pointer to MBSFN Area Info list from SIB13
@param pmch_InfoList pointer to PMCH_InfoList from MBSFNAreaConfiguration Message (MCCH Message)
*/
int rrc_mac_config_req_ue(module_id_t module_idP,
			  int CC_id,
			  uint8_t eNB_index,
			  LTE_RadioResourceConfigCommonSIB_t *
			  radioResourceConfigCommon,
			  struct LTE_PhysicalConfigDedicated
			  *physicalConfigDedicated,
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
			  LTE_SCellToAddMod_r10_t * sCellToAddMod_r10,
			  //struct PhysicalConfigDedicatedSCell_r10 *physicalConfigDedicatedSCell_r10,
#endif
			  LTE_MeasObjectToAddMod_t ** measObj,
			  LTE_MAC_MainConfig_t * mac_MainConfig,
			  long logicalChannelIdentity,
			  LTE_LogicalChannelConfig_t * logicalChannelConfig,
			  LTE_MeasGapConfig_t * measGapConfig,
			  LTE_TDD_Config_t * tdd_Config,
			  LTE_MobilityControlInfo_t * mobilityControlInfo,
			  uint8_t * SIwindowsize,
			  uint16_t * SIperiod,
			  LTE_ARFCN_ValueEUTRA_t * ul_CarrierFreq,
			  long *ul_Bandwidth,
			  LTE_AdditionalSpectrumEmission_t *
			  additionalSpectrumEmission,
			  struct LTE_MBSFN_SubframeConfigList
			  *mbsfn_SubframeConfigList
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
			  ,
			  uint8_t MBMS_Flag,
			  LTE_MBSFN_AreaInfoList_r9_t * mbsfn_AreaInfoList,
			  LTE_PMCH_InfoList_r9_t * pmch_InfoList
#endif
#ifdef CBA
			  ,
			  uint8_t num_active_cba_groups, uint16_t cba_rnti
#endif
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
			  ,config_action_t config_action
			  ,const uint32_t * const sourceL2Id
			  ,const uint32_t * const destinationL2Id
#endif
			  );


uint16_t getRIV(uint16_t N_RB_DL, uint16_t RBstart, uint16_t Lcrbs);

int get_subbandsize(uint8_t dl_bandwidth);


void get_Msg3allocret(COMMON_channels_t * cc,
		      sub_frame_t current_subframe,
		      frame_t current_frame,
		      frame_t * frame, sub_frame_t * subframe);

void get_Msg3alloc(COMMON_channels_t * cc,
		   sub_frame_t current_subframe,
		   frame_t current_frame,
		   frame_t * frame, sub_frame_t * subframe);

uint16_t mac_computeRIV(uint16_t N_RB_DL, uint16_t RBstart,
			uint16_t Lcrbs);

int get_phich_resource_times6(COMMON_channels_t * cc);

uint8_t frame_subframe2_dl_harq_pid(LTE_TDD_Config_t *tdd_Config, int abs_frameP, sub_frame_t subframeP);

uint8_t ul_subframe2_k_phich(COMMON_channels_t * cc, sub_frame_t ul_subframe);

unsigned char ul_ACK_subframe2M(LTE_TDD_Config_t *tdd_Config,unsigned char subframe);

int to_rbg(int dl_Bandwidth);

int to_prb(int dl_Bandwidth);

uint8_t get_Msg3harqpid(COMMON_channels_t * cc,
			frame_t frame, sub_frame_t current_subframe);

uint32_t pdcchalloc2ulframe(COMMON_channels_t * ccP, uint32_t frame,
			    uint8_t n);

uint8_t pdcchalloc2ulsubframe(COMMON_channels_t * ccP, uint8_t n);

int is_UL_sf(COMMON_channels_t * ccP, sub_frame_t subframeP);

uint8_t getQm(uint8_t mcs);

uint8_t subframe2harqpid(COMMON_channels_t * cc, frame_t frame,
			 sub_frame_t subframe);

void get_srs_pos(COMMON_channels_t * cc, uint16_t isrs,
		 uint16_t * psrsPeriodicity, uint16_t * psrsOffset);

void get_csi_params(COMMON_channels_t * cc,
		    struct LTE_CQI_ReportPeriodic *cqi_PMI_ConfigIndex,
		    uint16_t * Npd, uint16_t * N_OFFSET_CQI, int *H);

uint8_t get_rel8_dl_cqi_pmi_size(UE_sched_ctrl * sched_ctl, int CC_idP,
				 COMMON_channels_t * cc, uint8_t tmode,
				 struct LTE_CQI_ReportPeriodic
				 *cqi_ReportPeriodic);

uint8_t get_dl_cqi_pmi_size_pusch(COMMON_channels_t * cc, uint8_t tmode,
				  uint8_t ri,
				  LTE_CQI_ReportModeAperiodic_t *
				  cqi_ReportModeAperiodic);
void extract_pucch_csi(module_id_t mod_idP, int CC_idP, int UE_id,
		       frame_t frameP, sub_frame_t subframeP,
		       uint8_t * pdu, uint8_t length);

void extract_pusch_csi(module_id_t mod_idP, int CC_idP, int UE_id,
		       frame_t frameP, sub_frame_t subframeP,
		       uint8_t * pdu, uint8_t length);

uint16_t fill_nfapi_tx_req(nfapi_tx_request_body_t * tx_req_body,
			   uint16_t absSF, uint16_t pdu_length,
			   int16_t pdu_index, uint8_t * pdu);

void fill_nfapi_ulsch_config_request_rel8(nfapi_ul_config_request_pdu_t *
					  ul_config_pdu, uint8_t cqi_req,
					  COMMON_channels_t * cc,
					  struct LTE_PhysicalConfigDedicated
					  *physicalConfigDedicated,
					  uint8_t tmode, uint32_t handle,
					  uint16_t rnti,
					  uint8_t resource_block_start,
					  uint8_t
					  number_of_resource_blocks,
					  uint8_t mcs,
					  uint8_t cyclic_shift_2_for_drms,
					  uint8_t
					  frequency_hopping_enabled_flag,
					  uint8_t frequency_hopping_bits,
					  uint8_t new_data_indication,
					  uint8_t redundancy_version,
					  uint8_t harq_process_number,
					  uint8_t ul_tx_mode,
					  uint8_t current_tx_nb,
					  uint8_t n_srs, uint16_t size);

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
void fill_nfapi_ulsch_config_request_emtc(nfapi_ul_config_request_pdu_t *
					  ul_config_pdu, uint8_t ue_type,
					  uint16_t
					  total_number_of_repetitions,
					  uint16_t repetition_number,
					  uint16_t
					  initial_transmission_sf_io);
#endif

void program_dlsch_acknak(module_id_t module_idP, int CC_idP, int UE_idP,
			  frame_t frameP, sub_frame_t subframeP,
			  uint8_t cce_idx);

void fill_nfapi_dlsch_config(eNB_MAC_INST * eNB,
			     nfapi_dl_config_request_body_t * dl_req,
			     uint16_t length, int16_t pdu_index,
			     uint16_t rnti,
			     uint8_t resource_allocation_type,
			     uint8_t
			     virtual_resource_block_assignment_flag,
			     uint16_t resource_block_coding,
			     uint8_t modulation,
			     uint8_t redundancy_version,
			     uint8_t transport_blocks,
			     uint8_t transport_block_to_codeword_swap_flag,
			     uint8_t transmission_scheme,
			     uint8_t number_of_layers,
			     uint8_t number_of_subbands,
			     //                      uint8_t codebook_index,
			     uint8_t ue_category_capacity,
			     uint8_t pa,
			     uint8_t delta_power_offset_index,
			     uint8_t ngap,
			     uint8_t nprb,
			     uint8_t transmission_mode,
			     uint8_t num_bf_prb_per_subband,
			     uint8_t num_bf_vector);

void fill_nfapi_harq_information(module_id_t module_idP,
				 int CC_idP,
				 uint16_t rntiP,
				 nfapi_ul_config_harq_information *
				 harq_information, uint8_t cce_idxP);

void fill_nfapi_ulsch_harq_information(module_id_t module_idP,
				       int CC_idP,
				       uint16_t rntiP,
				       nfapi_ul_config_ulsch_harq_information
				       * harq_information,
				       sub_frame_t subframeP);

uint16_t fill_nfapi_uci_acknak(module_id_t module_idP,
							int CC_idP,
							uint16_t rntiP,
							uint16_t absSFP,
							uint8_t cce_idxP);

void fill_nfapi_dl_dci_1A(nfapi_dl_config_request_pdu_t * dl_config_pdu,
			  uint8_t aggregation_level,
			  uint16_t rnti,
			  uint8_t rnti_type,
			  uint8_t harq_process,
			  uint8_t tpc,
			  uint16_t resource_block_coding,
			  uint8_t mcs,
			  uint8_t ndi, uint8_t rv, uint8_t vrb_flag);

nfapi_ul_config_request_pdu_t *has_ul_grant(module_id_t module_idP,
					    int CC_idP, uint16_t subframeP,
					    uint16_t rnti);

uint8_t get_tmode(module_id_t module_idP, int CC_idP, int UE_idP);

uint8_t get_ul_req_index(module_id_t module_idP, int CC_idP,
			 sub_frame_t subframeP);

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
int get_numnarrowbandbits(long dl_Bandwidth);

int mpdcch_sf_condition(eNB_MAC_INST * eNB, int CC_id, frame_t frameP,
			sub_frame_t subframeP, int rmax,
			MPDCCH_TYPES_t mpdcch_type, int UE_id);

int get_numnarrowbands(long dl_Bandwidth);

int narrowband_to_first_rb(COMMON_channels_t * cc, int nb_index);

#endif

int l2_init_eNB(void);

void Msg1_transmitted(module_id_t module_idP, uint8_t CC_id,
		      frame_t frameP, uint8_t eNB_id);
void Msg3_transmitted(module_id_t module_idP, uint8_t CC_id,
		      frame_t frameP, uint8_t eNB_id);
uint32_t from_earfcn(int eutra_bandP, uint32_t dl_earfcn);
int32_t get_uldl_offset(int eutra_bandP);
int l2_init_ue(int eMBMS_active, char *uecap_xer, uint8_t cba_group_active,
	       uint8_t HO_active);
#if defined(PRE_SCD_THREAD)
void pre_scd_nb_rbs_required(    module_id_t     module_idP,
                                 frame_t         frameP,
                                 sub_frame_t     subframeP,
                                 int             min_rb_unit[MAX_NUM_CCs],
                                 uint16_t        nb_rbs_required[MAX_NUM_CCs][NUMBER_OF_UE_MAX]);
#endif

/* Slice related functions */
uint16_t nb_rbs_allowed_slice(float rb_percentage, int total_rbs);
int ue_dl_slice_membership(module_id_t mod_id, int UE_id, int slice_idx);
int ue_ul_slice_membership(module_id_t mod_id, int UE_id, int slice_idx);

/* DRX Configuration */
/* Configure local DRX timers and thresholds in UE context, following the drx_configuration input */
void eNB_Config_Local_DRX(module_id_t Mod_id, rnti_t rnti, LTE_DRX_Config_t *drx_Configuration);

/* from here: prototypes to get rid of compilation warnings: doc to be written by function author */
uint8_t ul_subframe2_k_phich(COMMON_channels_t * cc, sub_frame_t ul_subframe);
#endif
/** @}*/
