/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/** \addtogroup _mac
 *  @{
 */

/** \fn void add_ue_spec_dci(DCI_PDU *DCI_pdu,void *pdu,rnti_t rnti,unsigned char dci_size_bytes,unsigned char aggregation,unsigned char dci_size_bits,unsigned char dci_fmt,uint8_t ra_flag);
\brief 

*/
void add_ue_spec_dci(DCI_PDU *DCI_pdu,void *pdu,rnti_t rnti,unsigned char dci_size_bytes,unsigned char aggregation,unsigned char dci_size_bits,unsigned char dci_fmt,uint8_t ra_flag);

//LG commented cause compilation error for RT eNB extern inline unsigned int taus(void);

/** \fn void schedule_RA(module_id_t module_idP,frame_t frameP,sub_frame_t subframe,uint8_t Msg3_subframe,unsigned int *nprb);
\brief First stage of Random-Access Scheduling. Loops over the RA_templates and checks if RAR, Msg3 or its retransmission are to be scheduled in the subframe.  It returns the total number of PRB used for RA SDUs.  For Msg3 it retrieves the L3msg from RRC and fills the appropriate buffers.  For the others it just computes the number of PRBs. Each DCI uses 3 PRBs (format 1A)
for the message.
@param Mod_id Instance ID of eNB
@param frame Frame index
@param subframe Subframe number on which to act

*/
void schedule_RA(module_id_t module_idP,frame_t frameP,sub_frame_t subframe,uint8_t Msg3_subframe);

/** \brief First stage of SI Scheduling. Gets a SI SDU from RRC if available and computes the MCS required to transport it as a function of the SDU length.  It assumes a length less than or equal to 64 bytes (MCS 6, 3 PRBs).
@param Mod_id Instance ID of eNB
@param frame Frame index
@param subframe Subframe number on which to act
@param Msg3_subframe Subframe where Msg3 will be transmitted
*/
void schedule_SI(module_id_t module_idP,frame_t frameP,sub_frame_t subframeP);

/** \brief MBMS scheduling: Checking the position for MBSFN subframes. Create MSI, transfer MCCH from RRC to MAC, transfer MTCHs from RLC to MAC. Multiplexing MSI,MCCH&MTCHs. Return 1 if there are MBSFN data being allocated, otherwise return 0;
@param Mod_id Instance ID of eNB
@param frame Frame index
@param subframe Subframe number on which to act
*/
int schedule_MBMS(module_id_t module_idP,uint8_t CC_id, frame_t frameP, sub_frame_t subframe);

/** \brief check the mapping between sf allocation and sync area, Currently only supports 1:1 mapping
@param Mod_id Instance ID of eNB
@param mbsfn_sync_area index of mbsfn sync area
@param[out] index of sf pattern
*/
int8_t get_mbsfn_sf_alloction (module_id_t module_idP, uint8_t CC_id, uint8_t mbsfn_sync_area);

/** \brief check the mapping between sf allocation and sync area, Currently only supports 1:1 mapping
@param Mod_id Instance ID of eNB
@param mbsfn_sync_area index of mbsfn sync area
@param eNB_index index of eNB
@param[out] index of sf pattern
*/
int8_t ue_get_mbsfn_sf_alloction (module_id_t module_idP, uint8_t mbsfn_sync_area, unsigned char eNB_index);

/** \brief top ULSCH Scheduling for TDD (config 1-6).
@param Mod_id Instance ID of eNB
@param frame Frame index
@param subframe Subframe number on which to act
@param sched_subframe Subframe number where PUSCH is transmitted (for DAI lookup)
*/
void schedule_ulsch(module_id_t module_idP,frame_t frameP,unsigned char cooperation_flag,sub_frame_t subframe,unsigned char sched_subframe);

/** \brief ULSCH Scheduling per RNTI
@param Mod_id Instance ID of eNB
@param frame Frame index
@param subframe Subframe number on which to act
@param sched_subframe Subframe number where PUSCH is transmitted (for DAI lookup)
*/
void schedule_ulsch_rnti(module_id_t module_idP, unsigned char cooperation_flag, frame_t frameP, sub_frame_t subframe, unsigned char sched_subframe, uint16_t *first_rb);

/** \brief ULSCH Scheduling for CBA  RNTI
@param Mod_id Instance ID of eNB
@param frame Frame index
@param subframe Subframe number on which to act
@param sched_subframe Subframe number where PUSCH is transmitted (for DAI lookup)
*/
void schedule_ulsch_cba_rnti(module_id_t module_idP, unsigned char cooperation_flag, frame_t frameP, sub_frame_t subframe, unsigned char sched_subframe, uint16_t *first_rb);

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
void schedule_ue_spec(module_id_t module_idP,frame_t frameP,sub_frame_t subframe,int *mbsfn_flag);


/** \brief Function for UE/PHY to compute PUSCH transmit power in power-control procedure.
    @param Mod_id Module id of UE
    @returns Po_NOMINAL_PUSCH (PREAMBLE_RECEIVED_TARGET_POWER+DELTA_PREAMBLE
*/
int8_t get_Po_NOMINAL_PUSCH(module_id_t module_idP,uint8_t CC_id);

/** \brief Function to compute DELTA_PREAMBLE from 36.321 (for RA power ramping procedure and Msg3 PUSCH power control policy)
    @param Mod_id Module id of UE
    @returns DELTA_PREAMBLE
*/
int8_t get_DELTA_PREAMBLE(module_id_t module_idP,int CC_id);

/** \brief Function for compute deltaP_rampup from 36.321 (for RA power ramping procedure and Msg3 PUSCH power control policy)
    @param Mod_id Module id of UE
    @param CC_id carrier component id of UE
    @returns deltaP_rampup
*/
int8_t get_deltaP_rampup(module_id_t module_idP,uint8_t CC_id);

//main.c

void chbch_phy_sync_success(module_id_t module_idP,frame_t frameP,uint8_t eNB_index);

void mrbch_phy_sync_failure(module_id_t module_idP, frame_t frameP,uint8_t free_eNB_index);

int mac_top_init(int eMBMS_active, char *uecap_xer,uint8_t cba_group_active, uint8_t HO_active);

char layer2_init_UE(module_id_t module_idP);

char layer2_init_eNB(module_id_t module_idP, uint8_t Free_ch_index);

void mac_switch_node_function(module_id_t module_idP);

int mac_init_global_param(void);

void mac_top_cleanup(void);

void mac_UE_out_of_sync_ind(module_id_t module_idP,frame_t frameP, uint16_t eNB_index);

void dlsch_scheduler_pre_processor_reset (int module_idP,int UE_id,
    uint8_t  CC_id,
    int frameP,
    int subframeP,
    int N_RBG,
    uint16_t nb_rbs_required[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
    uint16_t  nb_rbs_required_remaining[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
    unsigned char rballoc_sub[MAX_NUM_CCs][N_RBG_MAX],
    unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX]);

// eNB functions
/* \brief This function assigns pre-available RBS to each UE in specified sub-bands before scheduling is done
@param Mod_id Instance ID of eNB
@param frame Index of frame
@param subframe Index of current subframe
@param N_RBS Number of resource block groups
 */


void dlsch_scheduler_pre_processor (module_id_t module_idP,
                                    frame_t frameP,
                                    sub_frame_t subframe,
                                    int N_RBG[MAX_NUM_CCs],
                                    int *mbsfn_flag);


void dlsch_scheduler_pre_processor_allocate (module_id_t   Mod_id,
    int           UE_id,
    uint8_t       CC_id,
    int           N_RBG,
    int           transmission_mode,
    int           min_rb_unit,
    uint8_t       N_RB_DL,
    uint16_t      nb_rbs_required[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
    uint16_t      nb_rbs_required_remaining[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
    unsigned char rballoc_sub[MAX_NUM_CCs][N_RBG_MAX],
    unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX]);

/* \brief Function to trigger the eNB scheduling procedure.  It is called by PHY at the beginning of each subframe, \f$n$\f
   and generates all DLSCH allocations for subframe \f$n\f$ and ULSCH allocations for subframe \f$n+k$\f. The resultant DCI_PDU is
   ready after returning from this call.
@param Mod_id Instance ID of eNB
@param cooperation_flag Flag to indicated that this cell has cooperating nodes (i.e. that there are collaborative transport channels that
can be scheduled.
@param subframe Index of current subframe
@param calibration_flag Flag to indicate that eNB scheduler should schedule TDD auto-calibration PUSCH.
*/
void eNB_dlsch_ulsch_scheduler(module_id_t module_idP, uint8_t cooperation_flag, frame_t frameP, sub_frame_t subframeP);//, int calibration_flag);

/* \brief Function to retrieve result of scheduling (DCI) in current subframe.  Can be called an arbitrary numeber of times after eNB_dlsch_ulsch_scheduler
in a given subframe.
@param Mod_id Instance ID of eNB
@param CC_id Component Carrier Index
@param subframe Index of current subframe
@returns Pointer to generated DCI for subframe
*/
DCI_PDU *get_dci_sdu(module_id_t module_idP,int CC_id,frame_t frameP,sub_frame_t subframe);

/* \brief Function to indicate a received preamble on PRACH.  It initiates the RA procedure.
@param Mod_id Instance ID of eNB
@param preamble_index index of the received RA request
@param timing_offset Offset in samples of the received PRACH w.r.t. eNB timing. This is used to
*/
void initiate_ra_proc(module_id_t module_idP,int CC_id,frame_t frameP, uint16_t preamble_index,int16_t timing_offset,uint8_t sect_id,sub_frame_t subframe,uint8_t f_id);

/* \brief Function in eNB to fill RAR pdu when requested by PHY.  This provides a single RAR SDU for the moment and returns the t-CRNTI.
@param Mod_id Instance ID of eNB
@param dlsch_buffer Pointer to DLSCH input buffer
@param N_RB_UL Number of UL resource blocks
@returns t_CRNTI
*/
unsigned short fill_rar(
  const module_id_t module_idP,
  const int         CC_id,
  const frame_t     frameP,
  uint8_t   * const dlsch_buffer,
  const uint16_t    N_RB_UL,
  const uint8_t input_buffer_length
);

/* \brief Function to indicate a failed RA response.  It removes all temporary variables related to the initial connection of a UE
@param Mod_id Instance ID of eNB
@param preamble_index index of the received RA request.
*/
void cancel_ra_proc(module_id_t module_idP,int CC_id,frame_t frameP, uint16_t preamble_index);

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
void set_msg3_subframe(module_id_t Mod_id,
                       int CC_id,
                       int frame,
                       int subframe,
                       int rnti,
                       int Msg3_frame,
                       int Msg3_subframe);

/* \brief Function to indicate a received SDU on ULSCH.
@param Mod_id Instance ID of eNB
@param rnti RNTI of UE transmitting the SR
@param sdu Pointer to received SDU
@param harq_pid Index of harq process corresponding to this sdu
@param msg3_flag flag indicating that this sdu is msg3
*/
void rx_sdu(const module_id_t module_idP, const int CC_id,const frame_t frameP, const sub_frame_t subframeP, const rnti_t rnti, uint8_t *sdu, const uint16_t sdu_len, const int harq_pid,uint8_t *msg3_flag);

/* \brief Function to indicate a scheduled schduling request (SR) was received by eNB.
@param Mod_id Instance ID of eNB
@param rnti RNTI of UE transmitting the SR
@param subframe Index of subframe where SR was received
*/
void SR_indication(module_id_t module_idP,int CC_id,frame_t frameP,rnti_t rnti, sub_frame_t subframe);

/* \brief Function to indicate a UL failure was detected by eNB PHY.
@param Mod_id Instance ID of eNB
@param CC_id Component carrier
@param frameP Frame index
@param rnti RNTI of UE transmitting the SR
@param subframe Index of subframe where SR was received
*/
void UL_failure_indication(module_id_t Mod_id,int CC_id,frame_t frameP,rnti_t rnti,sub_frame_t subframe);

uint8_t *get_dlsch_sdu(module_id_t module_idP,int CC_id,frame_t frameP,rnti_t rnti,uint8_t TBindex);

/* \brief Function to retrieve MCH transport block and MCS used for MCH in this MBSFN subframe.  Returns null if no MCH is to be transmitted
@param Mod_id Instance ID of eNB
@param frame Index of frame
@param subframe Index of current subframe
@param mcs Pointer to mcs used by PHY (to be filled by MAC)
@returns Pointer to MCH transport block and mcs for subframe
*/
MCH_PDU *get_mch_sdu( module_id_t Mod_id, int CC_id, frame_t frame, sub_frame_t subframe);


//added for ALU icic purpose
uint32_t  Get_Cell_SBMap(module_id_t module_idP);
void UpdateSBnumber(module_id_t module_idP);
//end ALU's algo


void        ue_mac_reset      (module_id_t module_idP,uint8_t eNB_index);
void        ue_init_mac       (module_id_t module_idP);
void        init_ue_sched_info(void);
void        add_ue_ulsch_info (module_id_t module_idP, int CC_id, int UE_id, sub_frame_t subframe,UE_ULSCH_STATUS status);
void        add_ue_dlsch_info (module_id_t module_idP, int CC_id,int UE_id, sub_frame_t subframe,UE_DLSCH_STATUS status);
int         find_UE_id        (module_id_t module_idP, rnti_t rnti) ;
rnti_t      UE_RNTI           (module_id_t module_idP, int UE_id);
int         UE_PCCID          (module_id_t module_idP, int UE_id);
uint8_t     find_active_UEs   (module_id_t module_idP);
boolean_t   is_UE_active      (module_id_t module_idP, int UE_id);
uint8_t     process_ue_cqi    (module_id_t module_idP, int UE_id);

int8_t find_active_UEs_with_traffic(module_id_t module_idP);

void init_CCE_table(int module_idP,int CC_idP);

int get_nCCE_offset(int *CCE_table,
		    const unsigned char L, 
		    const int nCCE, 
		    const int common_dci, 
		    const unsigned short rnti, 
		    const unsigned char subframe);

int allocate_CCEs(int module_idP,
		  int CC_idP,
		  int subframe,
		  int test_only);

boolean_t CCE_allocation_infeasible(int module_idP,
				  int CC_idP,
				  int common_flag,
				  int subframe,
				  int aggregation,
				  int rnti);

void set_ue_dai(sub_frame_t   subframeP,
                uint8_t       tdd_config,
                int           UE_id,
                uint8_t       CC_id,
                UE_list_t     *UE_list);

uint8_t find_num_active_UEs_in_cbagroup(module_id_t module_idP, unsigned char group_id);
uint8_t UE_is_to_be_scheduled(module_id_t module_idP,int CC_id,uint8_t UE_id);
/** \brief Round-robin scheduler for ULSCH traffic.
@param Mod_id Instance ID for eNB
@param subframe Subframe number on which to act
@returns UE index that is to be scheduled if needed/room
*/
module_id_t schedule_next_ulue(module_id_t module_idP, int UE_id,sub_frame_t subframe);

/** \brief Round-robin scheduler for DLSCH traffic.
@param Mod_id Instance ID for eNB
@param subframe Subframe number on which to act
@returns UE index that is to be scheduled if needed/room
*/
int schedule_next_dlue(module_id_t module_idP, int CC_id, sub_frame_t subframe);

/* \brief Allocates a set of PRBS for a particular UE.  This is a simple function for the moment, later it should process frequency-domain CQI information and/or PMI information.  Currently it just returns the first PRBS that are available in the subframe based on the number requested.
@param UE_id Index of UE on which to act
@param nb_rb Number of PRBs allocated to UE by scheduler
@param rballoc Pointer to bit-map of current PRB allocation given to previous users/control channels.  This is updated for subsequent calls to the routine.
@returns an rballoc bitmap for resource type 0 allocation (DCI).
*/
uint32_t allocate_prbs(int UE_id,uint8_t nb_rb, uint32_t *rballoc);

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
uint32_t ue_get_SR(module_id_t module_idP, int CC_id,frame_t frameP, uint8_t eNB_id,rnti_t rnti,sub_frame_t subframe);

uint8_t get_ue_weight(module_id_t module_idP, int CC_id, int UE_id);

// UE functions
void mac_out_of_sync_ind(module_id_t module_idP, frame_t frameP, uint16_t CH_index);

void ue_decode_si(module_id_t module_idP, int CC_id,frame_t frame, uint8_t CH_index, void *pdu, uint16_t len);

void ue_decode_p(module_id_t module_idP, int CC_id,frame_t frame, uint8_t CH_index, void *pdu, uint16_t len);


void ue_send_sdu(module_id_t module_idP, uint8_t CC_id,frame_t frame, uint8_t *sdu,uint16_t sdu_len,uint8_t CH_index);


#ifdef Rel10
/* \brief Called by PHY to transfer MCH transport block to ue MAC.
@param Mod_id Index of module instance
@param frame Frame index
@param sdu Pointer to transport block
@param sdu_len Length of transport block
@param eNB_index Index of attached eNB
@param sync_area the index of MBSFN sync area
*/
void ue_send_mch_sdu(module_id_t module_idP,uint8_t CC_id, frame_t frameP,uint8_t *sdu,uint16_t sdu_len,uint8_t eNB_index,uint8_t sync_area) ;

/*\brief Function to check if UE PHY needs to decode MCH for MAC.
@param Mod_id Index of protocol instance
@param frame Index of frame
@param subframe Index of subframe
@param eNB_index index of eNB for this MCH
@param[out] sync_area return the sync area
@param[out] mcch_active flag indicating whether this MCCH is active in this SF
*/
int ue_query_mch(uint8_t Mod_id,uint8_t CC_id, uint32_t frame,sub_frame_t subframe, uint8_t eNB_index, uint8_t *sync_area, uint8_t *mcch_active);

#endif

/* \brief Called by PHY to get sdu for PUSCH transmission.  It performs the following operations: Checks BSR for DCCH, DCCH1 and DTCH corresponding to previous values computed either in SR or BSR procedures.  It gets rlc status indications on DCCH,DCCH1 and DTCH and forms BSR elements and PHR in MAC header.  CRNTI element is not supported yet.  It computes transport block for up to 3 SDUs and generates header and forms the complete MAC SDU.
@param Mod_id Instance id of UE in machine
@param eNB_id Index of eNB that UE is attached to
@param rnti C_RNTI of UE
@param subframe subframe number
@returns 0 for no SR, 1 for SR
*/
void ue_get_sdu(module_id_t module_idP, int CC_id,frame_t frameP, sub_frame_t subframe, uint8_t eNB_index,uint8_t *ulsch_buffer,uint16_t buflen,uint8_t *access_mode);

/* \brief Function called by PHY to retrieve information to be transmitted using the RA procedure.  If the UE is not in PUSCH mode for a particular eNB index, this is assumed to be an Msg3 and MAC attempts to retrieves the CCCH message from RRC. If the UE is in PUSCH mode for a particular eNB index and PUCCH format 0 (Scheduling Request) is not activated, the MAC may use this resource for random-access to transmit a BSR along with the C-RNTI control element (see 5.1.4 from 36.321)
@param Mod_id Index of UE instance
@param Mod_id Component Carrier Index
@param New_Msg3 Flag to indicate this call is for a new Msg3
@param subframe Index of subframe for PRACH transmission (0 ... 9)
@returns A pointer to a PRACH_RESOURCES_t */
PRACH_RESOURCES_t *ue_get_rach(module_id_t module_idP,int CC_id,frame_t frameP,uint8_t new_Msg3,sub_frame_t subframe);

/* \brief Function called by PHY to process the received RAR.  It checks that the preamble matches what was sent by the eNB and provides the timing advance and t-CRNTI.
@param Mod_id Index of UE instance
@param dlsch_buffer  Pointer to dlsch_buffer containing RAR PDU
@param t_crnti Pointer to PHY variable containing the T_CRNTI
@param preamble_index Preamble Index used by PHY to transmit the PRACH.  This should match the received RAR to trigger the rest of
random-access procedure
@returns timing advance or 0xffff if preamble doesn't match
*/
uint16_t
ue_process_rar(
  const module_id_t module_idP,
  const int CC_id,
  const frame_t frameP,
  uint8_t * const dlsch_buffer,
  rnti_t * const t_crnti,
  const uint8_t preamble_index
);


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
unsigned char generate_ulsch_header(uint8_t *mac_header,
                                    uint8_t num_sdus,
                                    uint8_t short_padding,
                                    uint16_t *sdu_lengths,
                                    uint8_t *sdu_lcids,
                                    POWER_HEADROOM_CMD *power_headroom,
                                    uint16_t *crnti,
                                    BSR_SHORT *truncated_bsr,
                                    BSR_SHORT *short_bsr,
                                    BSR_LONG *long_bsr,
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
uint8_t *parse_ulsch_header(uint8_t *mac_header,
                            uint8_t *num_ce,
                            uint8_t *num_sdu,
                            uint8_t *rx_ces,
                            uint8_t *rx_lcids,
                            uint16_t *rx_lengths,
                            uint16_t tx_lenght);


int l2_init(LTE_DL_FRAME_PARMS *frame_parms,int eMBMS_active, char *uecap_xer, uint8_t cba_group_active, uint8_t HO_active);
int mac_init(void);
int add_new_ue(module_id_t Mod_id, int CC_id, rnti_t rnti,int harq_pid);
int rrc_mac_remove_ue(module_id_t Mod_id, rnti_t rntiP);


int maxround(module_id_t Mod_id,uint16_t rnti,int frame,sub_frame_t subframe,uint8_t ul_flag);
void swap_UEs(UE_list_t *listP,int nodeiP, int nodejP, int ul_flag);
int prev(UE_list_t *listP, int nodeP, int ul_flag);
void dump_ue_list(UE_list_t *listP, int ul_flag);
int UE_num_active_CC(UE_list_t *listP,int ue_idP);
int UE_PCCID(module_id_t mod_idP,int ue_idP);
rnti_t UE_RNTI(module_id_t mod_idP, int ue_idP);


void ulsch_scheduler_pre_processor(module_id_t module_idP, int frameP, sub_frame_t subframeP, uint16_t *first_rb, uint8_t  aggregattion);
void store_ulsch_buffer(module_id_t module_idP, int frameP, sub_frame_t subframeP);
void sort_ue_ul (module_id_t module_idP,int frameP, sub_frame_t subframeP);
void assign_max_mcs_min_rb(module_id_t module_idP,int frameP, sub_frame_t subframeP,uint16_t *first_rb);
void adjust_bsr_info(int buffer_occupancy, uint16_t TBS, UE_TEMPLATE *UE_template);

/*! \fn  UE_L2_state_t ue_scheduler(const module_id_t module_idP,const frame_t frameP, const sub_frame_t subframe, const lte_subframe_t direction,const uint8_t eNB_index)
   \brief UE scheduler where all the ue background tasks are done.  This function performs the following:  1) Trigger PDCP every 5ms 2) Call RRC for link status return to PHY3) Perform SR/BSR procedures for scheduling feedback 4) Perform PHR procedures.
\param[in] module_idP instance of the UE
\param[in] subframe t the subframe number
\param[in] direction  subframe direction
\param[in] eNB_index  instance of eNB
@returns L2 state (CONNETION_OK or CONNECTION_LOST or PHY_RESYNCH)
*/
UE_L2_STATE_t ue_scheduler(
  const module_id_t module_idP,
  const frame_t frameP,
  const sub_frame_t subframe,
  const lte_subframe_t direction,
  const uint8_t eNB_index,
  const int CC_id);

/*! \fn  int cba_access(module_id_t module_idP,frame_t frameP,sub_frame_t subframe, uint8_t eNB_index,uint16_t buflen);
\brief determine whether to use cba resource to transmit or not
\param[in] Mod_id instance of the UE
\param[in] frame the frame number
\param[in] subframe the subframe number
\param[in] eNB_index instance of eNB
\param[out] access(1) or postpone (0)
*/
int cba_access(module_id_t module_idP,frame_t frameP,sub_frame_t subframe, uint8_t eNB_index,uint16_t buflen);

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
BSR_LONG * get_bsr_long(module_id_t module_idP, uint8_t bsr_len);

/*! \fn  boolean_t update_bsr(module_id_t module_idP, frame_t frameP,sub_frame_t subframeP)
   \brief get the rlc stats and update the bsr level for each lcid
\param[in] Mod_id instance of the UE
\param[in] frame Frame index
*/
boolean_t update_bsr(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP,eNB_index_t eNB_index);

/*! \fn  locate_BsrIndexByBufferSize (int *table, int size, int value)
   \brief locate the BSR level in the table as defined in 36.321. This function requires that he values in table to be monotonic, either increasing or decreasing. The returned value is not less than 0, nor greater than n-1, where n is the size of table.
\param[in] *table Pointer to BSR table
\param[in] size Size of the table
\param[in] value Value of the buffer
\return the index in the BSR_LEVEL table
*/
uint8_t locate_BsrIndexByBufferSize (const uint32_t *table, int size, int value);


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
uint8_t get_phr_mapping (module_id_t module_idP, int CC_id, uint8_t eNB_index);

/*! \fn  void update_phr (module_id_t module_idP)
   \brief update/reset the phr timers
\param[in]  Mod_id index of eNB
\param[in] CC_id Component carrier index
\return void
*/
void update_phr (module_id_t module_idP,int CC_id);

/*! \brief Function to indicate Msg3 transmission/retransmission which initiates/reset Contention Resolution Timer
\param[in] Mod_id Instance index of UE
\param[in] eNB_id Index of eNB
*/
void Msg3_tx(module_id_t module_idP,uint8_t CC_id,frame_t frameP,uint8_t eNB_id);


/*! \brief Function to indicate the transmission of msg1/rach
\param[in] Mod_id Instance index of UE
\param[in] eNB_id Index of eNB
*/

void Msg1_tx(module_id_t module_idP,uint8_t CC_id,frame_t frameP, uint8_t eNB_id);

void dl_phy_sync_success(module_id_t   module_idP,
                         frame_t       frameP,
                         unsigned char eNB_index,
                         uint8_t first_sync);

int dump_eNB_l2_stats(char *buffer, int length);

double uniform_rngen(int min, int max);


void add_common_dci(DCI_PDU *DCI_pdu,
                    void *pdu,
                    rnti_t rnti,
                    unsigned char dci_size_bytes,
                    unsigned char aggregation,
                    unsigned char dci_size_bits,
                    unsigned char dci_fmt,
                    uint8_t ra_flag);

uint32_t allocate_prbs_sub(int nb_rb, uint8_t *rballoc);

void update_ul_dci(module_id_t module_idP,uint8_t CC_id,rnti_t rnti,uint8_t dai);

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
unsigned char generate_dlsch_header(unsigned char *mac_header,
                                    unsigned char num_sdus,
                                    unsigned short *sdu_lengths,
                                    unsigned char *sdu_lcids,
                                    unsigned char drx_cmd,
                                    short timing_advance_cmd,
                                    unsigned char *ue_cont_res_id,
                                    unsigned char short_padding,
                                    unsigned short post_padding);

/** \brief RRC Configuration primitive for PHY/MAC.  Allows configuration of PHY/MAC resources based on System Information (SI), RRCConnectionSetup and RRCConnectionReconfiguration messages.
@param Mod_id Instance ID of eNB
@param CC_id Component Carrier of the eNB
@param eNB_flag Indicates if this is a eNB or UE configuration
@param rntiP id of UE if this is an eNB configuration
@param eNB_id Index of eNB if this is a UE configuration
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
int rrc_mac_config_req(module_id_t     module_idP,
                       int             CC_id,
                       eNB_flag_t eNB_flag,
                       rnti_t          rntiP,
                       uint8_t         eNB_index,
                       RadioResourceConfigCommonSIB_t *radioResourceConfigCommon,
                       struct PhysicalConfigDedicated *physicalConfigDedicated,
#ifdef Rel10
                       SCellToAddMod_r10_t *sCellToAddMod_r10,
                       //struct PhysicalConfigDedicatedSCell_r10 *physicalConfigDedicatedSCell_r10,
#endif
                       MeasObjectToAddMod_t **measObj,
                       MAC_MainConfig_t *mac_MainConfig,
                       long logicalChannelIdentity,
                       LogicalChannelConfig_t *logicalChannelConfig,
                       MeasGapConfig_t *measGapConfig,
                       TDD_Config_t *tdd_Config,
                       MobilityControlInfo_t *mobilityControlInfo,
                       uint8_t *SIwindowsize,
                       uint16_t *SIperiod,
                       ARFCN_ValueEUTRA_t *ul_CarrierFreq,
                       long *ul_Bandwidth,
                       AdditionalSpectrumEmission_t *additionalSpectrumEmission,
                       struct MBSFN_SubframeConfigList *mbsfn_SubframeConfigList
#ifdef Rel10
                       ,
                       uint8_t MBMS_Flag,
                       MBSFN_AreaInfoList_r9_t *mbsfn_AreaInfoList,
                       PMCH_InfoList_r9_t *pmch_InfoList

#endif
#ifdef CBA
                       ,
                       uint8_t num_active_cba_groups,
                       uint16_t cba_rnti
#endif
                      );

/** \brief get the estimated UE distance from the PHY->MAC layer.
@param Mod_id Instance ID of eNB
@param UE_id Index of UE if this is an eNB configuration
@param CC_id Component Carrier Index
@param loc_type localization type: time-based or power-based
@return the estimated distance in meters
 */
double
rrc_get_estimated_ue_distance(
  const protocol_ctxt_t * const ctxt_pP,
  const int         CC_idP,
  const uint8_t     loc_typeP);

void fill_dci(DCI_PDU *DCI_pdu, PHY_VARS_eNB *phy_vars_eNB,eNB_rxtx_proc_t *proc);

#endif
/** @}*/
