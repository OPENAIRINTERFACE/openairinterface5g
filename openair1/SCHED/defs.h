/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
/*
  \author R. Knopp, F. Kaltenberger
  \company EURECOM
  \email knopp@eurecom.fr
*/

#ifndef __openair_SCHED_H__
#define __openair_SCHED_H__

#include "PHY/defs.h"


enum THREAD_INDEX { OPENAIR_THREAD_INDEX = 0,
                    TOP_LEVEL_SCHEDULER_THREAD_INDEX,
                    DLC_SCHED_THREAD_INDEX,
                    openair_SCHED_NB_THREADS
                  }; // do not modify this line


#define OPENAIR_THREAD_PRIORITY        255


#define OPENAIR_THREAD_STACK_SIZE     PTHREAD_STACK_MIN //4096 //RTL_PTHREAD_STACK_MIN*6
//#define DLC_THREAD_STACK_SIZE        4096 //DLC stack size

enum openair_SCHED_STATUS {
  openair_SCHED_STOPPED=1,
  openair_SCHED_STARTING,
  openair_SCHED_STARTED,
  openair_SCHED_STOPPING
};

enum openair_ERROR {
  // HARDWARE CAUSES
  openair_ERROR_HARDWARE_CLOCK_STOPPED= 1,

  // SCHEDULER CAUSE
  openair_ERROR_OPENAIR_RUNNING_LATE,
  openair_ERROR_OPENAIR_SCHEDULING_FAILED,

  // OTHERS
  openair_ERROR_OPENAIR_TIMING_OFFSET_OUT_OF_BOUNDS,
};

enum openair_SYNCH_STATUS {
  openair_NOT_SYNCHED=1,
#ifdef OPENAIR_LTE
  openair_SYNCHED,
#else
  openair_SYNCHED_TO_CHSCH,
  openair_SYNCHED_TO_MRSCH,
#endif
  openair_SCHED_EXIT
};


#define DAQ_AGC_ON 1
#define DAQ_AGC_OFF 0



typedef struct {
  boolean_t  is_eNB;
  uint8_t    mode;
  uint8_t    synch_source;
  uint32_t   slot_count;
  uint32_t   sched_cnt;
  uint32_t   synch_wait_cnt;
  uint32_t   sync_state;
  uint32_t   scheduler_interval_ns;
  uint32_t   last_adac_cnt;
  uint8_t    first_sync_call;
  int32_t    instance_cnt;
  uint8_t    one_shot_get_frame;
  uint8_t    do_synch;
  uint8_t    node_configured;  // &1..basic config, &3..ue config &5..eNb config
  uint8_t    node_running;
  uint8_t    tx_test;
  uint8_t    mac_registered;
  //uint8_t freq;
  uint32_t   freq;
  uint32_t   rx_gain_val;
  uint32_t   rx_gain_mode;
  uint32_t   tcxo_dac;
  uint32_t   auto_freq_correction;
  int32_t    freq_offset;
  uint32_t   tx_rx_switch_point;
  uint32_t   manual_timing_advance;  /// 1 to override automatic timing advance
  int32_t   timing_advance;
  uint32_t   dual_tx;                /// 1 for dual-antenna TX, 0 for single-antenna TX
  uint32_t   tdd;                    /// 1 for TDD mode, 0 for FDD mode
  uint32_t   rx_rf_mode;
  uint32_t   node_id;
  uint32_t   rach_detection_count;
  uint32_t   channel_vacant[4];
  uint32_t   target_ue_dl_mcs;
  uint32_t   target_ue_ul_mcs;
  uint32_t   ue_ul_nb_rb;
  uint32_t   ue_dl_rb_alloc;
  uint32_t   dlsch_rate_adaptation;
  uint32_t   dlsch_transmission_mode;
  uint32_t   ulsch_allocation_mode;
  uint32_t   rx_total_gain_dB;
  uint32_t   hw_frame;
  uint32_t   get_frame_done;
  uint32_t   use_ia_receiver;
} OPENAIR_DAQ_VARS;

#ifndef USER_MODE
int32_t openair_sched_init(void);
void openair_sched_cleanup(void);
void openair_sched_exit(char *);
void openair1_restart(void);
int32_t init_dlsch_threads(void);
void cleanup_dlsch_threads(void);
#endif //USER_MODE

#ifdef OPENAIR_LTE
/** @addtogroup _PHY_PROCEDURES_
 * @{
 */



/*! \brief Top-level entry routine for eNB procedures.  Called every slot by process scheduler. In even slots, it performs RX functions from previous subframe (if required).  On odd slots, it generate TX waveform for the following subframe.
  @param subframe Index of current subframe (0-9)
  @param phy_vars_eNB Pointer to eNB variables on which to act
  @param abstraction_flag Indicator of PHY abstraction
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
  @param *phy_vars_rn pointer to RN variables
*/
void phy_procedures_eNB_lte(uint8_t subframe,PHY_VARS_eNB **phy_vars_eNB,uint8_t abstraction_flag, relaying_type_t r_type, PHY_VARS_RN *phy_vars_rn);

/*! \brief Top-level entry routine for UE procedures.  Called every slot by process scheduler. In even slots, it performs RX functions from previous subframe (if required).  On odd slots, it generate TX waveform for the following subframe.
  @param last_slot Index of last slot (0-19)
  @param next_slot Index of next_slot (0-19)
  @param phy_vars_ue Pointer to UE variables on which to act
  @param eNB_id ID of eNB on which to act
  @param abstraction_flag Indicator of PHY abstraction
  @param mode calibration/debug mode
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
  @param *phy_vars_rn pointer to RN variables
*/
void phy_procedures_UE_lte(PHY_VARS_UE *phy_vars_ue,uint8_t eNB_id,uint8_t abstraction_flag,runmode_t mode,relaying_type_t r_type,PHY_VARS_RN *phy_vars_rn);

#ifdef Rel10
/*! \brief Top-level entry routine for relay node procedures when acting as eNB. This proc will make us of the existing eNB procs.
  @param last_slot Index of last slot (0-19)
  @param next_slot Index of next_slot (0-19)
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
*/
int phy_procedures_RN_eNB_TX(unsigned char last_slot, unsigned char next_slot, relaying_type_t r_type);
/*! \brief Top-level entry routine for relay node procedures actinf as UE. This proc will make us of the existing UE procs.
  @param last_slot Index of last slot (0-19)
  @param next_slot Index of next_slot (0-19)
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
*/
int phy_procedures_RN_UE_RX(unsigned char last_slot, unsigned char next_slot, relaying_type_t r_type);
#endif

/*! \brief Scheduling for UE TX procedures in normal subframes.
  @param phy_vars_ue Pointer to UE variables on which to act
  @param eNB_id Local id of eNB on which to act
  @param abstraction_flag Indicator of PHY abstraction
  @param mode calib/normal mode
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
*/
void phy_procedures_UE_TX(PHY_VARS_UE *phy_vars_ue,uint8_t eNB_id,uint8_t abstraction_flag,runmode_t mode,relaying_type_t r_type);
/*! \brief Scheduling for UE RX procedures in normal subframes.
  @param last_slot Index of last slot (0-19)
  @param phy_vars_ue Pointer to UE variables on which to act
  @param eNB_id Local id of eNB on which to act
  @param abstraction_flag Indicator of PHY abstraction
  @param mode calibration/debug mode
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
  @param phy_vars_rn pointer to RN variables
*/
int phy_procedures_UE_RX(PHY_VARS_UE *phy_vars_ue,uint8_t eNB_id,uint8_t abstraction_flag,runmode_t mode,relaying_type_t r_type,PHY_VARS_RN *phy_vars_rn);

/*! \brief Scheduling for UE TX procedures in TDD S-subframes.
  @param phy_vars_ue Pointer to UE variables on which to act
  @param eNB_id Local id of eNB on which to act
  @param abstraction_flag Indicator of PHY abstraction
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
*/
void phy_procedures_UE_S_TX(PHY_VARS_UE *phy_vars_ue,uint8_t eNB_id,uint8_t abstraction_flag,relaying_type_t r_type);

/*! \brief Scheduling for UE RX procedures in TDD S-subframes.
  @param last_slot Index of last slot (0-19)
  @param phy_vars_ue Pointer to UE variables on which to act
  @param eNB_id Local id of eNB on which to act
  @param abstraction_flag Indicator of PHY abstraction
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
*/
void phy_procedures_UE_S_RX(uint8_t last_slot,PHY_VARS_UE *phy_vars_ue,uint8_t eNB_id,uint8_t abstraction_flag, relaying_type_t r_type);

/*! \brief Scheduling for eNB TX procedures in normal subframes.
  @param next_slot Index of next slot (0-19)
  @param phy_vars_eNB Pointer to eNB variables on which to act
  @param abstraction_flag Indicator of PHY abstraction
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
  @param phy_vars_rn pointer to the RN variables
*/
void phy_procedures_eNB_TX(uint8_t next_slot,PHY_VARS_eNB *phy_vars_eNB,uint8_t abstraction_flag,relaying_type_t r_type,PHY_VARS_RN *phy_vars_rn);

/*! \brief Scheduling for eNB RX procedures in normal subframes.
  @param last_slot Index of last slot (0-19)
  @param phy_vars_eNB Pointer to eNB variables on which to act
  @param abstraction_flag Indicator of PHY abstraction
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
*/
void phy_procedures_eNB_RX(uint8_t last_slot,PHY_VARS_eNB *phy_vars_eNB,uint8_t abstraction_flag,relaying_type_t r_type);

/*! \brief Scheduling for eNB TX procedures in TDD S-subframes.
  @param next_slot Index of next slot (0-19)
  @param phy_vars_eNB Pointer to eNB variables on which to act
  @param abstraction_flag Indicator of PHY abstraction
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
*/
void phy_procedures_eNB_S_TX(uint8_t next_slot,PHY_VARS_eNB *phy_vars_eNB,uint8_t abstraction_flag,relaying_type_t r_type);

/*! \brief Scheduling for eNB RX procedures in TDD S-subframes.
  @param last_slot Index of next slot (0-19)
  @param phy_vars_eNB Pointer to eNB variables on which to act
  @param abstraction_flag Indicator of PHY abstraction
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
*/
void phy_procedures_eNB_S_RX(uint8_t last_slot,PHY_VARS_eNB *phy_vars_eNB,uint8_t abstraction_flag,relaying_type_t r_type);

/*! \brief Function to compute subframe type as a function of Frame type and TDD Configuration (implements Table 4.2.2 from 36.211, p.11 from version 8.6) and subframe index.
  @param frame_parms Pointer to DL frame parameter descriptor
  @param subframe Subframe index
  @returns Subframe type (DL,UL,S)
*/
lte_subframe_t subframe_select(LTE_DL_FRAME_PARMS *frame_parms,uint8_t subframe);

/*! \brief Function to compute subframe type as a function of Frame type and TDD Configuration (implements Table 4.2.2 from 36.211, p.11 from version 8.6) and subframe index.  Same as subframe_select, except that it uses the Mod_id and is provided as a service to the MAC scheduler.
  @param Mod_id Index of eNB
  @param CC_id Component Carrier Index
  @param subframe Subframe index
  @returns Subframe type (DL,UL,S)
*/
lte_subframe_t get_subframe_direction(uint8_t Mod_id, uint8_t CC_id,uint8_t subframe);

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
uint8_t get_ack(LTE_DL_FRAME_PARMS *frame_parms,harq_status_t *harq_ack,uint8_t subframe,uint8_t *o_ACK);

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
  @param eNB_id ID of eNB which is to receive the SR
  @param subframe index of next subframe
  @returns 1 if TXOp is active.
*/
uint8_t is_SR_TXOp(PHY_VARS_UE *phy_vars_ue,uint8_t eNB_id,uint8_t subframe);

/*! \brief Indicates the SR TXOp in current subframe for eNB and particular UE index.  Implements Table 10.1-5 from 36.213.
  @param phy_vars_eNB Pointer to eNB variables
  @param UE_id ID of UE which may be issuing the SR
  @param subframe index of last subframe
  @returns 1 if TXOp is active.
*/
uint8_t is_SR_subframe(PHY_VARS_eNB *phy_vars_eNB,uint8_t UE_id,uint8_t subframe);

/*! \brief Gives the UL subframe corresponding to a PDDCH order in subframe n
  @param frame_parms Pointer to DL frame parameters
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


int8_t find_ue(uint16_t rnti, PHY_VARS_eNB *phy_vars_eNB);
int32_t add_ue(int16_t rnti, PHY_VARS_eNB *phy_vars_eNB);
int mac_phy_remove_ue(module_id_t Mod_idP,rnti_t rnti);

void process_timing_advance(module_id_t Mod_id,uint8_t CC_id,int16_t timing_advance);
void process_timing_advance_rar(PHY_VARS_UE *phy_vars_ue,uint16_t timing_advance);

unsigned int get_tx_amp(int power_dBm, int power_max_dBm, int N_RB_UL, int nb_rb);

void phy_reset_ue(module_id_t Mod_id,uint8_t CC_id,uint8_t eNB_index);

/*! \brief This function retrives the resource (n1_pucch) corresponding to a PDSCH transmission in
subframe n-4 which is acknowledged in subframe n (for FDD) according to n1_pucch = Ncce + N1_pucch.  For
TDD, this routine computes the complex procedure described in Section 10.1 of 36.213 (through tables 10.1-1,10.1-2)
@param phy_vars_ue Pointer to UE variables
@param eNB_id Index of eNB
@param sched_subframe Index of subframe where procedures were scheduled
@param b Pointer to PUCCH payload (b[0],b[1])
@param SR 1 means there's a positive SR in parallel to ACK/NAK
@returns n1_pucch
*/
uint16_t get_n1_pucch(PHY_VARS_UE *phy_vars_ue,
                      uint8_t eNB_id,
                      uint8_t sched_subframe,
                      uint8_t *b,
                      uint8_t SR);

/*! \brief This function retrives the resource (n1_pucch) corresponding to a PDSCH transmission in
subframe n-4 which is acknowledged in subframe n (for FDD) according to n1_pucch = Ncce + N1_pucch.  For
TDD, this routine computes the procedure described in Section 10.1 of 36.213 (through tables 10.1-1,10.1-2)
@param phy_vars_eNB Pointer to UE variables
@param eNB_id Index of eNB
@param subframe Index of subframe
@param b Pointer to PUCCH payload (b[0],b[1])
@param n1_pucch0 Pointer to n1_pucch0
@param n1_pucch1 Pointer to n1_pucch1
@param n1_pucch2 Pointer to n1_pucch2
@param n1_pucch3 Pointer to n1_pucch3
*/
void get_n1_pucch_eNB(PHY_VARS_eNB *phy_vars_eNB,
                      uint8_t UE_id,
                      uint8_t subframe,
                      int16_t *n1_pucch0,
                      int16_t *n1_pucch1,
                      int16_t *n1_pucch2,
                      int16_t *n1_pucch3);


/*! \brief This function retrieves the harq_pid of the corresponding DLSCH process and updates the error statistics of the DLSCH based on the received ACK info from UE along with the round index.  It also performs the fine-grain rate-adaptation based on the error statistics derived from the ACK/NAK process.
  @param UE_id Local UE index on which to act
  @param subframe Index of subframe
  @param phy_vars_eNB Pointer to eNB variables on which to act
  @param pusch_flag Indication that feedback came from PUSCH
  @param pucch_payload Resulting payload from pucch
  @param pucch_sel Selection of n1_pucch0 or n1_pucch1 (TDD specific)
  @param SR_payload Indication of SR presence (TDD specific)
*/
void process_HARQ_feedback(uint8_t UE_id,
                           uint8_t subframe,
                           PHY_VARS_eNB *phy_vars_eNB,
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
    @param subframe Index of subframe
    @param eNB_id Index of eNB
    @param pucch_fmt Format of PUCCH that is being transmitted
    @returns Transmit power
 */
int8_t pucch_power_cntl(PHY_VARS_UE *phy_vars_ue,uint8_t subframe,uint8_t eNB_id,PUCCH_FMT_t pucch_fmt);

/*! \brief This function implements the power control mechanism for PUCCH from 36.213.
    @param phy_vars_ue PHY variables
    @param subframe Index of subframe
    @param eNB_id Index of eNB
    @param j index of type of PUSCH (SPS, Normal, Msg3)
    @returns Transmit power
 */
void pusch_power_cntl(PHY_VARS_UE *phy_vars_ue,uint8_t subframe,uint8_t eNB_id,uint8_t j, uint8_t abstraction_flag);

int8_t get_PHR(uint8_t Mod_id, uint8_t CC_id, uint8_t eNB_index);

#ifdef LOCALIZATION
/*! \brief This function collects eNB_UE stats and aggregate them in lists for localization
    @param phy_vars_ue PHY variables
    @param UE_id Index of UE
    @param frame Index of frame
    @param subframe Index of subframe
    @param UE_tx_power_dB estimated UE Tx power
    @returns -1 if updated list, 0 if calculated median
 */
double aggregate_eNB_UE_localization_stats(PHY_VARS_eNB *phy_vars_eNB, int8_t UE_id, frame_t frameP, sub_frame_t subframeP, int32_t UE_tx_power_dB);
#endif
LTE_eNB_UE_stats* get_eNB_UE_stats(uint8_t Mod_id, uint8_t CC_id,uint16_t rnti);

LTE_DL_FRAME_PARMS *get_lte_frame_parms(module_id_t Mod_id, uint8_t CC_id);

MU_MIMO_mode* get_mu_mimo_mode (module_id_t Mod_id, uint8_t CC_id, rnti_t rnti);

int16_t get_hundred_times_delta_IF(PHY_VARS_UE *phy_vars_ue,uint8_t eNB_id,uint8_t harq_pid);

int16_t get_hundred_times_delta_IF_eNB(PHY_VARS_eNB *phy_vars_eNB,uint8_t UE_id,uint8_t harq_pid, uint8_t bw_factor);

int16_t get_hundred_times_delta_IF_mac(module_id_t module_idP, uint8_t CC_id, rnti_t rnti, uint8_t harq_pid);

int16_t get_target_pusch_rx_power(module_id_t module_idP, uint8_t CC_id);
int16_t get_target_pucch_rx_power(module_id_t module_idP, uint8_t CC_id);

int get_ue_active_harq_pid(uint8_t Mod_id,uint8_t CC_id,uint16_t rnti,int frame, uint8_t subframe,uint8_t *harq_pid,uint8_t *round,uint8_t ul_flag);

void ulsch_decoding_procedures(unsigned char last_slot, unsigned int i, PHY_VARS_eNB *phy_vars_eNB, unsigned char abstraction_flag);

void dump_dlsch(PHY_VARS_UE *phy_vars_ue,uint8_t eNB_id,uint8_t subframe,uint8_t harq_pid);
void dump_dlsch_SI(PHY_VARS_UE *phy_vars_ue,uint8_t eNB_id,uint8_t subframe);
void dump_dlsch_ra(PHY_VARS_UE *phy_vars_ue,uint8_t eNB_id,uint8_t subframe);

/*@}*/

#endif //OPENAIR_LTE

extern int slot_irq_handler(int irq, void *cookie);

#endif


