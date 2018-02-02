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

/*! \file PHY_INTERFACE/defs.h
* \brief mac phy interface primitives
* \author Raymond Knopp and Navid Nikaein
* \date 2011
* \version 0.5
* \mail navid.nikaein@eurecom.fr or openair_tech@eurecom.fr
*/

#ifndef __MAC_PHY_PRIMITIVES_H__
#    define __MAC_PHY_PRIMITIVES_H__

#include "LAYER2/MAC/defs.h"


#define MAX_NUMBER_OF_MAC_INSTANCES 16

#define NULL_PDU 255
#define DCI 0
#define DLSCH 1
#define ULSCH 2

#define mac_exit_wrapper(sTRING)                                                            \
do {                                                                                        \
    char temp[300];                                                                         \
    snprintf(temp, sizeof(temp), "%s in file "__FILE__" at line %d\n", sTRING, __LINE__);   \
    mac_xface->macphy_exit(temp);                                                           \
} while(0)

/** @defgroup _phy_if MAC-PHY interface
 * @ingroup _oai2
 * @{
 */
/*! \brief MACPHY Interface */
/*
typedef struct {
  /// Pointer function that initializes L2
  int (*macphy_init)(int eMBMS_active, char *uecap_xer, uint8_t CBA_active,uint8_t HO_active);

  /// Pointer function that stops the low-level scheduler due an exit condition
  void (*macphy_exit)(const char *);

  // eNB functions
  /// Invoke dlsch/ulsch scheduling procedure for new subframe
  void (*eNB_dlsch_ulsch_scheduler)(module_id_t Mod_id,uint8_t cooperation_flag, frame_t frameP, sub_frame_t subframeP);//, int calibration_flag);

  /// Fill random access response sdu, passing timing advance
  uint16_t (*fill_rar)(module_id_t Mod_id,int CC_id,frame_t frameP,uint8_t *dlsch_buffer,uint16_t N_RB_UL, uint8_t input_buffer_length);

  /// Initiate the RA procedure upon reception (hypothetical) of a valid preamble
  void (*initiate_ra_proc)(module_id_t Mod_id,int CC_id,frame_t frameP,uint16_t preamble,int16_t timing_offset,uint8_t sect_id,sub_frame_t subframe,uint8_t f_id);

  /// cancel an ongoing RA procedure
  void (*cancel_ra_proc)(module_id_t Mod_id,int CC_id,frame_t frameP,uint16_t preamble);

  /// Inform MAC layer that an uplink is scheduled for Msg3 in given subframe.
  /// This is used so that the MAC scheduler marks as busy the RBs used by the Msg3.
  void (*set_msg3_subframe)(module_id_t Mod_id,
                            int CC_id,
                            int frame,
                            int subframe,
                            int rnti,
                            int Msg3_frame,
                            int Msg3_subframe);

  /// Get DCI for current subframe from MAC
  DCI_PDU* (*get_dci_sdu)(module_id_t Mod_id,int CC_id,frame_t frameP,sub_frame_t subframe);

  /// Get DLSCH sdu for particular RNTI and Transport block index
  uint8_t* (*get_dlsch_sdu)(module_id_t Mod_id,int CC_id,frame_t frameP,rnti_t rnti,uint8_t TB_index);

  /// Send ULSCH sdu to MAC for given rnti
  void (*rx_sdu)(module_id_t Mod_id,int CC_id,frame_t frameP, sub_frame_t sub_frameP,rnti_t rnti, uint8_t *sdu,uint16_t sdu_len, int harq_pid,uint8_t *msg3_flag);

  /// Indicate failure to synch to external source
  void (*mrbch_phy_sync_failure) (module_id_t Mod_id,frame_t frameP, uint8_t free_eNB_index);

  /// Indicate Scheduling Request from UE
  void (*SR_indication)(module_id_t Mod_id,int CC_id,frame_t frameP,rnti_t rnti,sub_frame_t subframe);
  /// Indicate UL Failure to eNodeB MAC
  void (*UL_failure_indication)(module_id_t Mod_id,int CC_id,frame_t frameP,rnti_t rnti,sub_frame_t subframe);

  /// Configure Common PHY parameters from SIB1
  void (*phy_config_mib_eNB)(module_id_t Mod_id,int CC_id,
			     int eutra_band,
			     int N_RB_DL,
			     PHICH_Config_t *phich_Config,
			     int Nid_cell,
			     int Ncp,
			     int p_eNB,
			     uint32_t      dl_CarrierFreq,
			     uint32_t      ul_CarrierFreq);

  /// Configure Common PHY parameters from SIB1
  void (*phy_config_sib1_eNB)(module_id_t Mod_id,int CC_id,
                              TDD_Config_t *tdd_config,
                              uint8_t SIwindowsize,
                              uint16_t SIperiod);

  /// Configure Common PHY parameters from SIB2
  void (*phy_config_sib2_eNB)(module_id_t Mod_id, int CC_id,
                              RadioResourceConfigCommonSIB_t *radioResourceConfigCommon,
                              ARFCN_ValueEUTRA_t *ul_CArrierFreq,
                              long *ul_Bandwidth,
                              AdditionalSpectrumEmission_t *additionalSpectrumEmission,
                              struct MBSFN_SubframeConfigList *mbsfn_SubframeConfigList);

#if defined(Rel10) || defined(Rel14)
  /// Configure Common PHY parameters from SIB13
  void (*phy_config_sib13_eNB)(module_id_t Mod_id,int CC_id, int mbsfn_Area_idx,
                               long mbsfn_AreaId_r9);

  void (*phy_config_dedicated_scell_eNB)(uint8_t Mod_id,
                                         uint16_t rnti,
                                         SCellToAddMod_r10_t *sCellToAddMod_r10,
                                         int CC_id);
#endif

  /// PHY-Config-Dedicated eNB
  void (*phy_config_dedicated_eNB)(module_id_t Mod_id,int CC_id,rnti_t rnti,
                                   struct PhysicalConfigDedicated *physicalConfigDedicated);

#if defined(Rel10) || defined(Rel14)
  /// Get MCH sdu and corresponding MCS for particular MBSFN subframe
  MCH_PDU* (*get_mch_sdu)(module_id_t Mod_id, int CC_id, frame_t frameP,sub_frame_t subframe);
#endif
  // configure the cba rnti at the physical layer
  void (*phy_config_cba_rnti)(module_id_t Mod_id,int CC_id,eNB_flag_t eNB_flag, uint8_t index, uint16_t cba_rnti, uint8_t cba_group_id, uint8_t num_active_cba_groups);
  /// get delta mcs for fast UL AMC
  int16_t (*estimate_ue_tx_power)(uint32_t tbs, uint32_t nb_rb, uint8_t control_only, lte_prefix_type_t ncp, uint8_t use_srs);

  int (*mac_phy_remove_ue)(module_id_t Mod_idP,rnti_t rntiP);
  /// UE functions

  /// reset the ue phy
  void (*phy_reset_ue)(module_id_t Mod_id,uint8_t CC_id,uint8_t eNB_index);

  /// Indicate loss of synchronization of PBCH for this eNB to MAC layer
  void (*out_of_sync_ind)(module_id_t Mod_id,frame_t frameP,uint16_t eNB_index);

  ///  Send a received SI sdu
  void (*ue_decode_si)(module_id_t Mod_id,int CC_id,frame_t frameP, uint8_t CH_index, void *pdu, uint16_t len);

  ///  Send a received Paging sdu
  void (*ue_decode_p)(module_id_t Mod_id,int CC_id,frame_t frameP, uint8_t CH_index, void *pdu, uint16_t len);

  /// Send a received DLSCH sdu to MAC
  void (*ue_send_sdu)(module_id_t Mod_id,uint8_t CC_id,frame_t frameP,sub_frame_t subframe,uint8_t *sdu,uint16_t sdu_len,uint8_t CH_index);

#if defined(Rel10) || defined(Rel14)
  /// Send a received MCH sdu to MAC
  void (*ue_send_mch_sdu)(module_id_t Mod_id,uint8_t CC_id, frame_t frameP,uint8_t *sdu,uint16_t sdu_len,uint8_t eNB_index,uint8_t sync_area);

  /// Function to check if UE PHY needs to decode MCH for MAC
  /// get the sync area id, and return MCS value if need to decode, otherwise -1
  int (*ue_query_mch)(module_id_t Mod_id, uint8_t CC_id,frame_t frameP,sub_frame_t subframe,uint8_t eNB_index,uint8_t *sync_area, uint8_t *mcch_active);
#endif

  /// Retrieve ULSCH sdu from MAC
  void (*ue_get_sdu)(module_id_t Mod_id,int CC_id,frame_t frameP,sub_frame_t subframe, uint8_t CH_index,uint8_t *ulsch_buffer,uint16_t buflen,uint8_t *access_mode);

  /// Retrieve RRCConnectionReq from MAC
  PRACH_RESOURCES_t* (*ue_get_rach)(module_id_t Mod_id,int CC_id,frame_t frameP,uint8_t Msg3_flag,sub_frame_t subframe);

  /// Process Random-Access Response
  uint16_t (*ue_process_rar)(module_id_t Mod_id,int CC_id,frame_t frameP, uint16_t ra_rnti, uint8_t *dlsch_buffer, uint16_t *t_crnti,uint8_t preamble_index, uint8_t* selected_rar_buffer);

  /// Get SR payload (0,1) from UE MAC
  uint32_t (*ue_get_SR)(module_id_t Mod_id,int CC_id,frame_t frameP,uint8_t eNB_id,rnti_t rnti,sub_frame_t subframe);

  /// Indicate synchronization with valid PBCH
  void (*dl_phy_sync_success) (module_id_t Mod_id,frame_t frameP, uint8_t CH_index,uint8_t first_sync);

  /// Only calls the PDCP for now
  UE_L2_STATE_t (*ue_scheduler)(module_id_t Mod_id, frame_t rxFrameP,sub_frame_t rxSubframe, frame_t txFrameP,sub_frame_t txSubframe, lte_subframe_t direction, uint8_t eNB_id, int CC_id);

  /// PHY-Config-Dedicated UE
  void (*phy_config_dedicated_ue)(module_id_t Mod_id,int CC_id,uint8_t CH_index,
                                  struct PhysicalConfigDedicated *physicalConfigDedicated);

  /// PHY-Config-harq UE
  void (*phy_config_harq_ue)(module_id_t Mod_id,int CC_id,uint8_t CH_index,
                             uint16_t max_harq_tx);
  /// Configure Common PHY parameters from SIB1
  void (*phy_config_sib1_ue)(module_id_t Mod_id,int CC_id,uint8_t CH_index,
                             TDD_Config_t *tdd_config,
                             uint8_t SIwindowsize,
                             uint16_t SIperiod);

  /// Configure Common PHY parameters from SIB2
  void (*phy_config_sib2_ue)(module_id_t Mod_id,int CC_id,uint8_t CH_index,
                             RadioResourceConfigCommonSIB_t *radioResourceConfigCommon,
                             ARFCN_ValueEUTRA_t *ul_CArrierFreq,
                             long *ul_Bandwidth,
                             AdditionalSpectrumEmission_t *additionalSpectrumEmission,
                             struct MBSFN_SubframeConfigList  *mbsfn_SubframeConfigList);

#if defined(Rel10) || defined(Rel14)
  /// Configure Common PHY parameters from SIB13
  void (*phy_config_sib13_ue)(uint8_t Mod_id,int CC_id, uint8_t eNB_index,int mbsfn_Area_idx,
                              long mbsfn_AreaId_r9);

  void (*phy_config_dedicated_scell_ue)(uint8_t Mod_id,
                                        uint8_t eNB_index,
                                        SCellToAddMod_r10_t *sCellToAddMod_r10,
                                        int CC_id);
#endif
  /// Configure Common PHY parameters from mobilityControlInfo
  void (*phy_config_afterHO_ue)(module_id_t Mod_id,uint8_t CC_id,uint8_t CH_index,
                                MobilityControlInfo_t *mobilityControlInfo,
                                uint8_t ho_failed);

  /// Function to indicate failure of contention resolution or RA procedure
  void (*ra_failed)(module_id_t Mod_id, uint8_t CC_id,uint8_t eNB_index);

  /// Function to indicate success of contention resolution or RA procedure
  void (*ra_succeeded)(module_id_t Mod_id,uint8_t CC_id, uint8_t eNB_index);

  /// Function to indicate the transmission of msg1/rach to MAC
  void (*Msg1_transmitted)(module_id_t Mod_id,uint8_t CC_id,frame_t frameP,uint8_t eNB_id);

  /// Function to indicate Msg3 transmission/retransmission which initiates/reset Contention Resolution Timer
  void (*Msg3_transmitted)(module_id_t Mod_id,uint8_t CC_id,frame_t frameP,uint8_t eNB_id);

  /// Function to pass inter-cell measurement parameters to PHY (cell Ids)
  void (*phy_config_meas_ue)(module_id_t Mod_id,uint8_t CC_id,uint8_t eNB_index,uint8_t n_adj_cells,uint32_t *adj_cell_id);

  // PHY Helper Functions

  /// RIV computation from PHY
  uint16_t (*computeRIV)(uint16_t N_RB_DL,uint16_t RBstart,uint16_t Lcrbs);

  /// Downlink TBS table lookup from PHY
  uint32_t (*get_TBS_DL)(uint8_t mcs, uint16_t nb_rb);

  /// Uplink TBS table lookup from PHY
  uint32_t (*get_TBS_UL)(uint8_t mcs, uint16_t nb_rb);

  /// Function to retrieve the HARQ round index for a particular UL/DLSCH and harq_pid
  int (*get_ue_active_harq_pid)(module_id_t Mod_id, uint8_t CC_id,rnti_t rnti, int frame, uint8_t subframe, uint8_t *harq_pid, uint8_t *round, uint8_t ul_flag);

  /// Function to retrieve number of CCE
  uint16_t (*get_nCCE_max)(module_id_t Mod_id,uint8_t  CC_id,int num_pdcch_symbols,int subframe);


  int (*get_nCCE_offset)(int *CCE_table,
                         const unsigned char L,
                         const int nCCE,
                         const int common_dci,
                         const unsigned short rnti,
                         const unsigned char subframe);

  /// Function to retrieve number of PRB in an rb_alloc
  uint32_t (*get_nb_rb)(uint8_t ra_header, uint32_t rb_alloc, int n_rb_dl);

  /// Function to convert VRB to PRB for distributed allocation
  uint32_t (*get_prb)(int N_RB_DL,int odd_slot,int vrb,int Ngap);

  /// Function to retrieve transmission mode for UE
  uint8_t (*get_transmission_mode)(module_id_t Mod_id,uint8_t CC_id,rnti_t rnti);

  /// Function to retrieve rb_alloc bitmap from dci rballoc field and VRB type
  uint32_t (*get_rballoc)(vrb_t vrb_type, uint16_t rb_alloc_dci);

  /// Function for UE MAC to retrieve current PHY connectivity mode (PRACH,RA_RESPONSE,PUSCH)
  UE_MODE_t (*get_ue_mode)(module_id_t Mod_id, uint8_t CC_id,uint8_t eNB_index);

  /// Function for UE MAC to retrieve measured Path Loss
  int16_t (*get_PL)(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index);

  /// Function for UE MAC to retrieve the rssi
  uint32_t (*get_RSSI)(uint8_t Mod_id,uint8_t CC_id);

  /// Function for UE MAC to retrieve the total gain
  uint32_t (*get_rx_total_gain_dB)(uint8_t Mod_id,uint8_t CC_id);

  /// Function for UE MAC to retrieve the number of adjustent cells
  uint8_t (*get_n_adj_cells)(uint8_t Mod_id,uint8_t CC_id);

  /// Function for UE MAC to retrieve RSRP/RSRQ measurements
  uint32_t (*get_RSRP)(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index);

  /// Function for UE MAC to retrieve RSRP/RSRQ measurements
  uint32_t (*get_RSRQ)(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index);

  /// Function for UE MAC to set the layer3 filtered RSRP/RSRQ measurements
  uint8_t (*set_RSRP_filtered)(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index,float rsrp);

  /// Function for UE MAC to set the layer3 filtered RSRP/RSRQ measurements
  uint8_t (*set_RSRQ_filtered)(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index,float rsrp);

  /// Function for UE/eNB MAC to retrieve number of PRACH in TDD
  uint8_t (*get_num_prach_tdd)(LTE_DL_FRAME_PARMS *frame_parms);

  /// Function for UE/eNB MAC to retrieve f_id of particular PRACH resource in TDD
  uint8_t (*get_fid_prach_tdd)(LTE_DL_FRAME_PARMS *frame_parms,uint8_t tdd_map_index);

  /// Function for eNB MAC to retrieve subframe direction
  lte_subframe_t (*get_subframe_direction)(module_id_t Mod_id, uint8_t CC_id, uint8_t subframe);

  // MAC Helper functions
  /// Function for UE/PHY to compute PUSCH transmit power in power-control procedure (Po_NOMINAL_PUSCH parameter)
  int8_t (*get_Po_NOMINAL_PUSCH)(module_id_t Mod_id,uint8_t  CC_id);

  /// Function for UE/PHY to compute PUSCH transmit power in power-control procedure (deltaP_rampup parameter)
  int8_t (*get_deltaP_rampup)(module_id_t Mod_id,uint8_t CC_id);

  /// Function for UE/PHY to compute PHR
  int8_t (*get_PHR)(module_id_t Mod_id, uint8_t CC_id,uint8_t eNB_index);

  /// Function for UE to process the timing advance command
  void (*process_timing_advance)(module_id_t Mod_id,uint8_t CC_id, int16_t timing_advance);
  
  /// Function for MAC to get the UE stats from the PHY   
  LTE_eNB_UE_stats* (*get_eNB_UE_stats)(module_id_t Mod_id, uint8_t CC_id, rnti_t rnti);

  /// get the frame parameters from the PHY
  LTE_DL_FRAME_PARMS* (*get_lte_frame_parms)(module_id_t Mod_id, uint8_t CC_id);
  
  /// get the Multiuser mimo mode
  MU_MIMO_mode* (*get_mu_mimo_mode) (module_id_t Mod_id, uint8_t CC_id, rnti_t rnti);

  /// get the delta TF for Uplink Power Control Calculation
  int16_t (*get_hundred_times_delta_TF) (module_id_t module_idP, uint8_t CC_id, rnti_t rnti, uint8_t harq_pid);
  /// get target PUSCH received power 
  int16_t (*get_target_pusch_rx_power) (module_id_t module_idP, uint8_t CC_id);
  /// get target PUSCH received power 
  int16_t (*get_target_pucch_rx_power) (module_id_t module_idP, uint8_t CC_id);

  unsigned char is_cluster_head;
  unsigned char is_primary_cluster_head;
  unsigned char is_secondary_cluster_head;
  unsigned char cluster_head_index;

  /// PHY Frame Configuration
  LTE_DL_FRAME_PARMS *frame_parms;

  uint8_t (*get_prach_prb_offset)(LTE_DL_FRAME_PARMS *frame_parms, uint8_t tdd_mapindex, uint16_t Nf); 

  int (*is_prach_subframe)(LTE_DL_FRAME_PARMS *frame_parms,frame_t frame, uint8_t subframe);

  /// ICIC algos
  uint8_t (*get_SB_size)(uint8_t n_rb_dl);
  ///end ALU's algo

} MAC_xface;

*/

#endif


/** @} */
