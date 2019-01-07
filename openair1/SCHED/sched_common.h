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

/*
  \author R. Knopp, F. Kaltenberger
  \company EURECOM
  \email knopp@eurecom.fr
*/

#ifndef __openair_SCHED_COMMON_H__
#define __openair_SCHED_COMMON_H__

#include "PHY/defs_eNB.h"
#include "PHY/defs_UE.h"
#include "LTE_TDD-Config.h"
/*! \brief Function to compute subframe Number(DL and S) as a function of Frame type and TDD Configuration
  @param frame_parms Pointer to DL frame parameter descriptor
  @returns Subframe Number (DL,S)
*/
int subframe_num(LTE_DL_FRAME_PARMS *frame_parms);

/*! \brief Function to compute subframe type as a function of Frame type and TDD Configuration (implements Table 4.2.2 from 36.211, p.11 from version 8.6) and subframe index.
  @param frame_parms Pointer to DL frame parameter descriptor
  @param subframe Subframe index
  @returns Subframe type (DL,UL,S)
*/


lte_subframe_t subframe_select(LTE_DL_FRAME_PARMS *frame_parms,uint8_t subframe);


/*! \brief Function to compute which type of DCIs to detect in the given subframe
  @param frame_parms Pointer to DL frame parameter descriptor
  @param subframe Subframe index
  @returns DCI detetion mode type (no DCIs, uplink DCIs, downlink DCIs, both uplink and downlink DCIs)
 */
dci_detect_mode_t dci_detect_mode_select(LTE_DL_FRAME_PARMS *frame_parms,uint8_t subframe);




/*! \brief Function to indicate PHICH transmission subframes.  Implements Table 9.1.2-1 for TDD.
  @param frame_parms Pointer to DL frame parameter descriptor
  @param subframe Subframe index
  @returns 1 if PHICH can be transmitted in subframe (always 1 for FDD)
*/
uint32_t is_phich_subframe(LTE_DL_FRAME_PARMS *frame_parms,uint8_t subframe);

/*! \brief Function to compute timing of Msg3 transmission on UL-SCH (first UE transmission in RA procedure). This implements the timing in paragraph a) from Section 6.1.1 in 36.213 (p. 17 in version 8.6).  Used by eNB upon transmission of random-access response (RA_RNTI) to program corresponding ULSCH reception procedure.  Used by UE upon reception of random-access response (RA_RNTI) to program corresponding ULSCH transmission procedure.  This does not support the UL_delay field in RAR (always assumed to be 0).
  @param frame_parms Pointer to DL frame parameter descriptor
  @param current_subframe Index of subframe where RA_RNTI was received
  @param current_frame Index of frame where RA_RNTI was received
  @param frame Frame index where Msg3 is to be transmitted (n+6 mod 10 for FDD, different for TDD)
  @param subframe subframe index where Msg3 is to be transmitted (n, n+1 or n+2)
*/
void get_Msg3_alloc(LTE_DL_FRAME_PARMS *frame_parms,
                    uint8_t current_subframe,
                    uint32_t current_frame,
                    uint32_t *frame,
                    uint8_t *subframe);

/*! \brief Function to compute timing of Msg3 retransmission on UL-SCH (first UE transmission in RA procedure).
  @param frame_parms Pointer to DL frame parameter descriptor
  @param current_subframe Index of subframe where RA_RNTI was received
  @param current_frame Index of frame where RA_RNTI was received
  @param frame Frame index where Msg3 is to be transmitted (n+6 mod 10 for FDD, different for TDD)
  @param subframe subframe index where Msg3 is to be transmitted (n, n+1 or n+2)
*/
void get_Msg3_alloc_ret(LTE_DL_FRAME_PARMS *frame_parms,
                        uint8_t current_subframe,
                        uint32_t current_frame,
                        uint32_t *frame,
                        uint8_t *subframe);

/*! \brief Get ULSCH harq_pid for Msg3 from RAR subframe.  This returns n+k mod 10 (k>6) and corresponds to the rule in Section 6.1.1 from 36.213
   @param frame_parms Pointer to DL Frame Parameters
   @param frame Frame index
   @param current_subframe subframe of RAR transmission
   @returns harq_pid (0 ... 7)
 */
uint8_t get_Msg3_harq_pid(LTE_DL_FRAME_PARMS *frame_parms,uint32_t frame,uint8_t current_subframe);

/*! \brief Get ULSCH harq_pid from PHICH subframe
   @param frame_parms Pointer to DL Frame Parameters
   @param subframe subframe of PHICH
   @returns harq_pid (0 ... 7)
 */

/*! \brief Function to indicate failure of contention resolution or RA procedure.  It places the UE back in PRACH mode.
    @param Mod_id Instance index of UE
    @param CC_id Component Carrier Index
    @param eNB_index Index of eNB
 */
void ra_failed(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index);

/*! \brief Function to indicate success of contention resolution or RA procedure.
    @param Mod_id Instance index of UE
    @param CC_id Component Carrier Index
    @param eNB_index Index of eNB
 */
void ra_succeeded(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index);

uint8_t phich_subframe_to_harq_pid(LTE_DL_FRAME_PARMS *frame_parms,uint32_t frame,uint8_t subframe);

/*! \brief Get PDSCH subframe (n+k) from PDCCH subframe n using relationship from Table 8-2 from 36.213
   @param frame_parms Pointer to DL Frame Parameters
   @param n subframe of PDCCH
   @returns PDSCH subframe (0 ... 7) (note: this is n+k from Table 8-2)
 */
uint8_t pdcch_alloc2ul_subframe(LTE_DL_FRAME_PARMS *frame_parms,uint8_t n);


/*! \brief Compute ACK/NACK information for PUSCH/PUCCH for UE transmission in subframe n. This function implements table 10.1-1 of 36.213, p. 69.
  @param frame_parms Pointer to DL frame parameter descriptor
  @param harq_ack Pointer to dlsch_ue harq_ack status descriptor
  @param subframe Subframe for UE transmission (n in 36.213)
  @param o_ACK Pointer to ACK/NAK payload for PUCCH/PUSCH
  @returns status indicator for PUCCH/PUSCH transmission
*/
uint8_t get_ack(LTE_DL_FRAME_PARMS *frame_parms,harq_status_t *harq_ack,uint8_t subframe_tx,uint8_t subframe_rx,uint8_t *o_ACK, uint8_t cw_idx);

/*! \brief Reset ACK/NACK information
  @param frame_parms Pointer to DL frame parameter descriptor
  @param harq_ack Pointer to dlsch_ue harq_ack status descriptor
  @param subframe Subframe for UE transmission (n in 36.213)
  @param o_ACK Pointer to ACK/NAK payload for PUCCH/PUSCH
  @returns status indicator for PUCCH/PUSCH transmission
*/
uint8_t reset_ack(LTE_DL_FRAME_PARMS *frame_parms,
                harq_status_t *harq_ack,
                unsigned char subframe_tx,
                unsigned char subframe_rx,
                unsigned char *o_ACK,
                uint8_t *pN_bundled,
                uint8_t cw_idx);

/*! \brief Compute UL ACK subframe from DL subframe. This is used to retrieve corresponding DLSCH HARQ pid at eNB upon reception of ACK/NAK information on PUCCH/PUSCH.  Derived from Table 10.1-1 in 36.213 (p. 69 in version 8.6)
  @param frame_parms Pointer to DL frame parameter descriptor
  @param subframe Subframe for UE transmission (n in 36.213)
  @param ACK_index TTI bundling index (0,1)
  @returns Subframe index for corresponding DL transmission
*/
uint8_t ul_ACK_subframe2_dl_subframe(LTE_DL_FRAME_PARMS *frame_parms,uint8_t subframe,uint8_t ACK_index);

/*! \brief Computes number of DL subframes represented by a particular ACK received on UL (M from Table 10.1-1 in 36.213, p. 69 in version 8.6)
  @param frame_parms Pointer to DL frame parameter descriptor
  @param subframe Subframe for UE transmission (n in 36.213)
  @returns Number of DL subframes (M)
*/
uint8_t ul_ACK_subframe2_M(LTE_DL_FRAME_PARMS *frame_parms,unsigned char subframe);

/*! \brief Indicates the SR TXOp in current subframe.  Implements Table 10.1-5 from 36.213.
  @param phy_vars_ue Pointer to UE variables
  @param proc Pointer to RXn_TXnp4 thread context
  @param eNB_id ID of eNB which is to receive the SR
  @returns 1 if TXOp is active.
*/
uint8_t is_SR_TXOp(PHY_VARS_UE *phy_vars_ue,UE_rxtx_proc_t *proc,uint8_t eNB_id);

/*! \brief Indicates the SR TXOp in current subframe for eNB and particular UE index.  Implements Table 10.1-5 from 36.213.
  @param phy_vars_eNB Pointer to eNB variables
  @param UE_id ID of UE which may be issuing the SR
  @returns 1 if TXOp is active.
*/
uint8_t is_SR_subframe(PHY_VARS_eNB *phy_vars_eNB,L1_rxtx_proc_t *proc,uint8_t UE_id);

/*! \brief Gives the UL subframe corresponding to a PDDCH order in subframe n
  @param frame_parms Pointer to DL frame parameters
  @param proc Pointer to RXn-TXnp4 proc information
  @param n subframe of PDCCH
  @returns UL subframe corresponding to pdcch order
*/
uint8_t pdcch_alloc2ul_subframe(LTE_DL_FRAME_PARMS *frame_parms,uint8_t n);

/*! \brief Gives the UL frame corresponding to a PDDCH order in subframe n
  @param frame_parms Pointer to DL frame parameters
  @param frame Frame of received PDCCH
  @param n subframe of PDCCH
  @returns UL frame corresponding to pdcch order
*/
uint32_t pdcch_alloc2ul_frame(LTE_DL_FRAME_PARMS *frame_parms,uint32_t frame, uint8_t n);


uint16_t get_Np(uint8_t N_RB_DL,uint8_t nCCE,uint8_t plus1);


int8_t find_ue_dlsch(uint16_t rnti, PHY_VARS_eNB *phy_vars_eNB);
int8_t find_ue_ulsch(uint16_t rnti, PHY_VARS_eNB *phy_vars_eNB);

//int32_t add_ue(int16_t rnti, PHY_VARS_eNB *phy_vars_eNB);
//int mac_phy_remove_ue(module_id_t Mod_idP,rnti_t rnti);

void process_timing_advance(module_id_t Mod_id,uint8_t CC_id,int16_t timing_advance);
void process_timing_advance_rar(PHY_VARS_UE *phy_vars_ue,UE_rxtx_proc_t *proc,uint16_t timing_advance);

unsigned int get_tx_amp(int power_dBm, int power_max_dBm, int N_RB_UL, int nb_rb);

void phy_reset_ue(module_id_t Mod_id,uint8_t CC_id,uint8_t eNB_index);

/*! \brief This function retrives the resource (n1_pucch) corresponding to a PDSCH transmission in
subframe n-4 which is acknowledged in subframe n (for FDD) according to n1_pucch = Ncce + N1_pucch.  For
TDD, this routine computes the complex procedure described in Section 10.1 of 36.213 (through tables 10.1-1,10.1-2)
@param phy_vars_ue Pointer to UE variables
@param proc Pointer to RXn-TXnp4 proc information
@param harq_ack Pointer to dlsch_ue harq_ack status descriptor
@param eNB_id Index of eNB
@param b Pointer to PUCCH payload (b[0],b[1])
@param SR 1 means there's a positive SR in parallel to ACK/NAK
@returns n1_pucch
*/
uint16_t get_n1_pucch(PHY_VARS_UE *phy_vars_ue,
		      UE_rxtx_proc_t *proc,
                      harq_status_t *harq_ack,
                      uint8_t eNB_id,
                      uint8_t *b,
                      uint8_t SR);

/*! \brief This function retrives the resource (n1_pucch) corresponding to a PDSCH transmission in
subframe n-4 which is acknowledged in subframe n (for FDD) according to n1_pucch = Ncce + N1_pucch.  For
TDD, this routine computes the procedure described in Section 10.1 of 36.213 (through tables 10.1-1,10.1-2)
@param phy_vars_eNB Pointer to eNB variables
@param proc Pointer to RXn-TXnp4 proc information
@param eNB_id Index of eNB
@param subframe Index of subframe
@param b Pointer to PUCCH payload (b[0],b[1])
@param n1_pucch0 Pointer to n1_pucch0
@param n1_pucch1 Pointer to n1_pucch1
@param n1_pucch2 Pointer to n1_pucch2
@param n1_pucch3 Pointer to n1_pucch3
*/
void get_n1_pucch_eNB(PHY_VARS_eNB *phy_vars_eNB,
		      L1_rxtx_proc_t *proc,
                      uint8_t UE_id,
                      int16_t *n1_pucch0,
                      int16_t *n1_pucch1,
                      int16_t *n1_pucch2,
                      int16_t *n1_pucch3);


/*! \brief This function retrieves the harq_pid of the corresponding DLSCH process and updates the error statistics of the DLSCH based on the received ACK info from UE along with the round index.  It also performs the fine-grain rate-adaptation based on the error statistics derived from the ACK/NAK process.
  @param UE_id Local UE index on which to act
  @param phy_vars_eNB Pointer to eNB variables on which to act
  @param proc Pointer to RXn-TXnp4 proc information
  @param pusch_flag Indication that feedback came from PUSCH
  @param pucch_payload Resulting payload from pucch
  @param pucch_sel Selection of n1_pucch0 or n1_pucch1 (TDD specific)
  @param SR_payload Indication of SR presence (TDD specific)
*/
void process_HARQ_feedback(uint8_t UE_id,
                           PHY_VARS_eNB *phy_vars_eNB,
			   L1_rxtx_proc_t *proc,
                           uint8_t pusch_flag,
                           uint8_t *pucch_payload,
                           uint8_t pucch_sel,
                           uint8_t SR_payload);

/*! \brief This function retrieves the PHY UE mode. It is used as a helper function for the UE MAC.
  @param Mod_id Local UE index on which to act
  @param CC_id Component Carrier Index
  @param eNB_index ID of eNB
  @returns UE mode
*/
UE_MODE_t get_ue_mode(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index);

/*! \brief This function implements the power control mechanism for PUCCH from 36.213.
    @param phy_vars_ue PHY variables
    @param proc Pointer to proc descriptor
    @param eNB_id Index of eNB
    @param pucch_fmt Format of PUCCH that is being transmitted
    @returns Transmit power
 */
int16_t pucch_power_cntl(PHY_VARS_UE *ue,UE_rxtx_proc_t *proc,uint8_t subframe,uint8_t eNB_id,PUCCH_FMT_t pucch_fmt);

/*! \brief This function implements the power control mechanism for PUCCH from 36.213.
    @param phy_vars_ue PHY variables
    @param proc Pointer to proc descriptor
    @param eNB_id Index of eNB
    @param j index of type of PUSCH (SPS, Normal, Msg3)
    @returns Transmit power
 */
void pusch_power_cntl(PHY_VARS_UE *phy_vars_ue,UE_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t j, uint8_t abstraction_flag);

/*! \brief This function implements the power control mechanism for SRS from 36.213.
    @param phy_vars_ue PHY variables
    @param proc Pointer to proc descriptor
    @param eNB_id Index of eNB
    @param j index of type of PUSCH (SPS, Normal, Msg3)
    @returns Transmit power
 */
void srs_power_cntl(PHY_VARS_UE *ue,UE_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t *pnb_rb_srs, uint8_t abstraction_flag);

void get_cqipmiri_params(PHY_VARS_UE *ue,uint8_t eNB_id);

int8_t get_PHR(uint8_t Mod_id, uint8_t CC_id, uint8_t eNB_index);

void schedule_response(Sched_Rsp_t *Sched_INFO);

LTE_eNB_UE_stats* get_UE_stats(uint8_t Mod_id, uint8_t CC_id,uint16_t rnti);



LTE_DL_FRAME_PARMS *get_lte_frame_parms(module_id_t Mod_id, uint8_t CC_id);

MU_MIMO_mode* get_mu_mimo_mode (module_id_t Mod_id, uint8_t CC_id, rnti_t rnti);

int16_t get_hundred_times_delta_IF(PHY_VARS_UE *phy_vars_ue,uint8_t eNB_id,uint8_t harq_pid);
int16_t get_hundred_times_delta_IF_eNB(PHY_VARS_eNB *phy_vars_eNB,uint16_t UE_id,uint8_t harq_pid, uint8_t bw_factor);
int16_t get_hundred_times_delta_IF_mac(module_id_t module_idP, uint8_t CC_id, rnti_t rnti, uint8_t harq_pid);

int16_t get_target_pusch_rx_power(module_id_t module_idP, uint8_t CC_id);
int16_t get_target_pucch_rx_power(module_id_t module_idP, uint8_t CC_id);

int get_ue_active_harq_pid(uint8_t Mod_id,uint8_t CC_id,uint16_t rnti,int frame, uint8_t subframe,uint8_t *harq_pid,uint8_t *round,uint8_t ul_flag);

void dump_dlsch(PHY_VARS_UE *phy_vars_ue,UE_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t subframe,uint8_t harq_pid);
void dump_dlsch_SI(PHY_VARS_UE *phy_vars_ue,UE_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t subframe);
void dump_dlsch_ra(PHY_VARS_UE *phy_vars_ue,UE_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t subframe);

void dump_dlsch2(PHY_VARS_UE *phy_vars_ue,uint8_t eNB_id,uint8_t subframe, unsigned int *coded_bits_per_codeword,int round, unsigned char harq_pid);


int is_srs_occasion_common(LTE_DL_FRAME_PARMS *frame_parms,int frame_tx,int subframe_tx);

void compute_srs_pos(lte_frame_type_t frameType,uint16_t isrs,uint16_t *psrsPeriodicity,uint16_t *psrsOffset);


/* from here: prototype added to remove compilation warnings, doc to be written by the author of the function */

int ul_ACK_subframe2_dl_frame(LTE_DL_FRAME_PARMS *frame_parms,int frame, unsigned char subframe,unsigned char subframe_tx);

void get_retransmission_timing(LTE_TDD_Config_t *tdd_Config, frame_t *frameP,  sub_frame_t *subframeP);
/*@}*/


#endif


