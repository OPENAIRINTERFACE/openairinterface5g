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
  \brief NR UE PHY functions prototypes
  \author R. Knopp, F. Kaltenberger
  \company EURECOM
  \email knopp@eurecom.fr
*/

#ifndef __openair_SCHED_H__
#define __openair_SCHED_H__

#include "PHY/defs_nr_UE.h"


/*enum THREAD_INDEX { OPENAIR_THREAD_INDEX = 0,
                    TOP_LEVEL_SCHEDULER_THREAD_INDEX,
                    DLC_SCHED_THREAD_INDEX,
                    openair_SCHED_NB_THREADS
                  };*/ // do not modify this line


#define OPENAIR_THREAD_PRIORITY        255


#define OPENAIR_THREAD_STACK_SIZE     PTHREAD_STACK_MIN //4096 //RTL_PTHREAD_STACK_MIN*6
//#define DLC_THREAD_STACK_SIZE        4096 //DLC stack size
//#define UE_SLOT_PARALLELISATION
//#define UE_DLSCH_PARALLELISATION

/*enum openair_SCHED_STATUS {
  openair_SCHED_STOPPED=1,
  openair_SCHED_STARTING,
  openair_SCHED_STARTED,
  openair_SCHED_STOPPING
};*/

/*enum openair_ERROR {
  // HARDWARE CAUSES
  openair_ERROR_HARDWARE_CLOCK_STOPPED= 1,

  // SCHEDULER CAUSE
  openair_ERROR_OPENAIR_RUNNING_LATE,
  openair_ERROR_OPENAIR_SCHEDULING_FAILED,

  // OTHERS
  openair_ERROR_OPENAIR_TIMING_OFFSET_OUT_OF_BOUNDS,
};*/

/*enum openair_SYNCH_STATUS {
  openair_NOT_SYNCHED=1,
  openair_SYNCHED,
  openair_SCHED_EXIT
};*/

/*enum openair_HARQ_TYPE {
  openair_harq_DL = 0,
  openair_harq_UL,
  openair_harq_RA
};*/

#define DAQ_AGC_ON 1
#define DAQ_AGC_OFF 0


typedef struct {
  uint8_t decoded_output[64];
  uint8_t xtra_byte;
} fapiPbch_t;

/** @addtogroup _PHY_PROCEDURES_
 * @{
 */


/*! \brief Top-level entry routine for UE procedures.  Called every slot by process scheduler. In even slots, it performs RX functions from previous subframe (if required).  On odd slots, it generate TX waveform for the following subframe.
  @param phy_vars_ue Pointer to UE variables on which to act
  @param eNB_id ID of eNB on which to act
  @param abstraction_flag Indicator of PHY abstraction
  @param mode calibration/debug mode
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
  @param *phy_vars_rn pointer to RN variables
*/
void phy_procedures_UE_lte(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t abstraction_flag,uint8_t do_pdcch_flag,runmode_t mode,relaying_type_t r_type);
/*! \brief Top-level entry routine for relay node procedures actinf as UE. This proc will make us of the existing UE procs.
  @param last_slot Index of last slot (0-19)
  @param next_slot Index of next_slot (0-19)
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
*/
int phy_procedures_RN_UE_RX(unsigned char last_slot, unsigned char next_slot, relaying_type_t r_type);

/*! \brief Scheduling for UE TX procedures in normal subframes.
  @param ue Pointer to UE variables on which to act
  @param proc Pointer to RXn-TXnp4 proc information
  @param eNB_id Local id of eNB on which to act
*/
void phy_procedures_nrUE_TX(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, uint8_t eNB_id);

/*! \brief Scheduling for UE RX procedures in normal subframes.
  @param ue                     Pointer to UE variables on which to act
  @param proc                   Pointer to proc information
  @param gNB_id                 Local id of eNB on which to act
  @param dlsch_parallel         use multithreaded dlsch processing
  @param phy_pdcch_config       PDCCH Config for this slot
  @param txFifo                 Result fifo if PDSCH is run in parallel
*/
int phy_procedures_nrUE_RX(PHY_VARS_NR_UE *ue,
                           UE_nr_rxtx_proc_t *proc,
                           uint8_t gNB_id,
                           uint8_t dlsch_parallel,
                           NR_UE_PDCCH_CONFIG *phy_pdcch_config,
                           notifiedFIFO_t *txFifo);

int phy_procedures_slot_parallelization_nrUE_RX(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, uint8_t eNB_id, uint8_t abstraction_flag, uint8_t do_pdcch_flag, relaying_type_t r_type);

void processSlotTX(void *arg);

#ifdef UE_SLOT_PARALLELISATION
  void *UE_thread_slot1_dl_processing(void *arg);
#endif

/*! \brief Scheduling for UE TX procedures in TDD S-subframes.
  @param phy_vars_ue Pointer to UE variables on which to act
  @param eNB_id Local id of eNB on which to act
  @param abstraction_flag Indicator of PHY abstraction
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
*/
//void phy_procedures_UE_S_TX(PHY_VARS_NR_UE *ue,uint8_t eNB_id,uint8_t abstraction_flag,relaying_type_t r_type);

/*! \brief Scheduling for UE RX procedures in TDD S-subframes.
  @param phy_vars_ue Pointer to UE variables on which to act
  @param eNB_id Local id of eNB on which to act
  @param abstraction_flag Indicator of PHY abstraction
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
*/
//void phy_procedures_UE_S_RX(PHY_VARS_NR_UE *ue,uint8_t eNB_id,uint8_t abstraction_flag, relaying_type_t r_type);


/*! \brief Function to compute subframe type as a function of Frame type and TDD Configuration (implements Table 4.2.2 from 36.211, p.11 from version 8.6) and subframe index.
  @param frame_parms Pointer to DL frame parameter descriptor
  @param subframe Subframe index
  @returns Subframe type (DL,UL,S)
*/


//nr_subframe_t nr_subframe_select(NR_DL_FRAME_PARMS *frame_parms,uint8_t subframe);


/*! \brief Function to compute subframe type as a function of Frame type and TDD Configuration (implements Table 4.2.2 from 36.211, p.11 from version 8.6) and subframe index.  Same as nr_subframe_select, except that it uses the Mod_id and is provided as a service to the MAC scheduler.
  @param Mod_id Index of eNB
  @param CC_id Component Carrier Index
  @param subframe Subframe index
  @returns Subframe type (DL,UL,S)
*/
nr_subframe_t nr_get_subframe_direction(uint8_t Mod_id, uint8_t CC_id,uint8_t subframe);

/*! \brief Function to compute timing of Msg3 transmission on UL-SCH (first UE transmission in RA procedure). This implements the timing in paragraph a) from Section 6.1.1 in 36.213 (p. 17 in version 8.6).  Used by eNB upon transmission of random-access response (RA_RNTI) to program corresponding ULSCH reception procedure.  Used by UE upon reception of random-access response (RA_RNTI) to program corresponding ULSCH transmission procedure.  This does not support the UL_delay field in RAR (always assumed to be 0).
  @param frame_parms Pointer to DL frame parameter descriptor
  @param current_subframe Index of subframe where RA_RNTI was received
  @param current_frame Index of frame where RA_RNTI was received
  @param frame Frame index where Msg3 is to be transmitted (n+6 mod 10 for FDD, different for TDD)
  @param subframe subframe index where Msg3 is to be transmitted (n, n+1 or n+2)
*/
void nr_get_Msg3_alloc(NR_DL_FRAME_PARMS *frame_parms,
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
void nr_get_Msg3_alloc_ret(NR_DL_FRAME_PARMS *frame_parms,
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
uint8_t nr_get_Msg3_harq_pid(NR_DL_FRAME_PARMS *frame_parms,uint32_t frame,uint8_t current_subframe);

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

/*! \brief UE PRACH procedures.
    @param
    @param
    @param
 */
void nr_ue_prach_procedures(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, uint8_t gNB_id);

int is_nr_prach_subframe(NR_DL_FRAME_PARMS *frame_parms, uint32_t frame, uint8_t subframe);

#if 0
/*! \brief Compute ACK/NACK information for PUSCH/PUCCH for UE transmission in subframe n. This function implements table 10.1-1 of 36.213, p. 69.
  @param frame_parms Pointer to DL frame parameter descriptor
  @param harq_ack Pointer to dlsch_ue harq_ack status descriptor
  @param subframe Subframe for UE transmission (n in 36.213)
  @param o_ACK Pointer to ACK/NAK payload for PUCCH/PUSCH
  @returns status indicator for PUCCH/PUSCH transmission
*/
uint8_t nr_get_ack(NR_DL_FRAME_PARMS *frame_parms,nr_harq_status_t *harq_ack,uint8_t subframe_tx,uint8_t subframe_rx,uint8_t *o_ACK, uint8_t cw_idx);

/*! \brief Reset ACK/NACK information
  @param frame_parms Pointer to DL frame parameter descriptor
  @param harq_ack Pointer to dlsch_ue harq_ack status descriptor
  @param subframe Subframe for UE transmission (n in 36.213)
  @param o_ACK Pointer to ACK/NAK payload for PUCCH/PUSCH
  @returns status indicator for PUCCH/PUSCH transmission
*/
uint8_t nr_reset_ack(NR_DL_FRAME_PARMS *frame_parms,
                     nr_harq_status_t *harq_ack,
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
uint8_t nr_ul_ACK_subframe2_dl_subframe(NR_DL_FRAME_PARMS *frame_parms,uint8_t subframe,uint8_t ACK_index);

/*! \brief Computes number of DL subframes represented by a particular ACK received on UL (M from Table 10.1-1 in 36.213, p. 69 in version 8.6)
  @param frame_parms Pointer to DL frame parameter descriptor
  @param subframe Subframe for UE transmission (n in 36.213)
  @returns Number of DL subframes (M)
*/
uint8_t nr_ul_ACK_subframe2_M(NR_DL_FRAME_PARMS *frame_parms,unsigned char subframe);

/*! \brief Indicates the SR TXOp in current subframe.  Implements Table 10.1-5 from 36.213.
  @param phy_vars_ue Pointer to UE variables
  @param proc Pointer to RXn_TXnp4 thread context
  @param eNB_id ID of eNB which is to receive the SR
  @returns 1 if TXOp is active.
*/
#endif
uint8_t nr_is_SR_TXOp(PHY_VARS_NR_UE *phy_vars_ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id);

/*! \brief Gives the UL subframe corresponding to a PDDCH order in subframe n
  @param frame_parms Pointer to DL frame parameters
  @param proc Pointer to RXn-TXnp4 proc information
  @param n subframe of PDCCH
  @returns UL subframe corresponding to pdcch order
*/
uint8_t nr_pdcch_alloc2ul_subframe(NR_DL_FRAME_PARMS *frame_parms,uint8_t n);

/*! \brief Gives the UL frame corresponding to a PDDCH order in subframe n
  @param frame_parms Pointer to DL frame parameters
  @param frame Frame of received PDCCH
  @param n subframe of PDCCH
  @returns UL frame corresponding to pdcch order
*/
uint32_t nr_pdcch_alloc2ul_frame(NR_DL_FRAME_PARMS *frame_parms,uint32_t frame, uint8_t n);


int8_t nr_find_ue(uint16_t rnti, PHY_VARS_eNB *phy_vars_eNB);

/*! \brief UL time alignment procedures for TA application
  @param PHY_VARS_NR_UE ue
  @param int slot_tx
  @param int frame_tx
*/
void ue_ta_procedures(PHY_VARS_NR_UE *ue, int slot_tx, int frame_tx);

unsigned int nr_get_tx_amp(int power_dBm, int power_max_dBm, int N_RB_UL, int nb_rb);

void phy_reset_ue(module_id_t Mod_id,uint8_t CC_id,uint8_t eNB_index);

#if 0
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
uint16_t nr_get_n1_pucch(PHY_VARS_NR_UE *phy_vars_ue,
                         UE_nr_rxtx_proc_t *proc,
                         nr_harq_status_t *harq_ack,
                         uint8_t eNB_id,
                         uint8_t *b,
                         uint8_t SR);

#endif

/*! \brief This function retrieves the PHY UE mode. It is used as a helper function for the UE MAC.
  @param Mod_id Local UE index on which to act
  @param CC_id Component Carrier Index
  @param gNB_index ID of gNB
  @returns UE mode
*/
UE_MODE_t get_nrUE_mode(uint8_t Mod_id,uint8_t CC_id,uint8_t gNB_index);

/*! \brief This function implements the power control mechanism for PUCCH from 36.213.
    @param phy_vars_ue PHY variables
    @param proc Pointer to proc descriptor
    @param eNB_id Index of eNB
    @param pucch_fmt Format of PUCCH that is being transmitted
    @returns Transmit power
 */
int16_t nr_pucch_power_cntl(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t subframe,uint8_t eNB_id,PUCCH_FMT_t pucch_fmt);

/*! \brief This function implements the power control mechanism for PUCCH from 36.213.
    @param phy_vars_ue PHY variables
    @param proc Pointer to proc descriptor
    @param eNB_id Index of eNB
    @param j index of type of PUSCH (SPS, Normal, Msg3)
    @returns Transmit power
 */
void nr_pusch_power_cntl(PHY_VARS_NR_UE *phy_vars_ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t j, uint8_t abstraction_flag);

void nr_get_cqipmiri_params(PHY_VARS_NR_UE *ue,uint8_t eNB_id);

void nr_dump_dlsch(PHY_VARS_NR_UE *phy_vars_ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t subframe,uint8_t harq_pid);
void nr_dump_dlsch_SI(PHY_VARS_NR_UE *phy_vars_ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t subframe);
void nr_dump_dlsch_ra(PHY_VARS_NR_UE *phy_vars_ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t subframe);

void set_tx_harq_id(NR_UE_ULSCH_t *ulsch, int harq_pid, int slot_tx);
int get_tx_harq_id(NR_UE_ULSCH_t *ulsch, int slot_tx);

int is_pbch_in_slot(fapi_nr_config_request_t *config, int frame, int slot, NR_DL_FRAME_PARMS *fp);
int is_ssb_in_slot(fapi_nr_config_request_t *config, int frame, int slot, NR_DL_FRAME_PARMS *fp);
bool is_csi_rs_in_symbol(fapi_nr_dl_config_csirs_pdu_rel15_t csirs_config_pdu, int symbol);

/*! \brief This function prepares the dl indication to pass to the MAC
    @param
    @param
    @param
    @param
 */
void nr_fill_dl_indication(nr_downlink_indication_t *dl_ind,
                           fapi_nr_dci_indication_t *dci_ind,
                           fapi_nr_rx_indication_t *rx_ind,
                           UE_nr_rxtx_proc_t *proc,
                           PHY_VARS_NR_UE *ue,
                           uint8_t gNB_id,
                           void *phy_data);

/*@}*/

/*! \brief This function prepares the dl rx indication
    @param
    @param
    @param
    @param
 */
void nr_fill_rx_indication(fapi_nr_rx_indication_t *rx_ind,
                           uint8_t pdu_type,
                           uint8_t gNB_id,
                           PHY_VARS_NR_UE *ue,
                           NR_UE_DLSCH_t *dlsch0,
                           NR_UE_DLSCH_t *dlsch1,
                           uint16_t n_pdus,
                           UE_nr_rxtx_proc_t *proc,
			   void * typeSpecific);

bool nr_ue_dlsch_procedures(PHY_VARS_NR_UE *ue,
                            UE_nr_rxtx_proc_t *proc,
                            int gNB_id,
                            PDSCH_t pdsch,
                            NR_UE_DLSCH_t *dlsch0,
                            NR_UE_DLSCH_t *dlsch1,
                            int *dlsch_errors);

int nr_ue_pdsch_procedures(PHY_VARS_NR_UE *ue,
                           UE_nr_rxtx_proc_t *proc,
                           int eNB_id, PDSCH_t pdsch,
                           NR_UE_DLSCH_t *dlsch0, NR_UE_DLSCH_t *dlsch1);

int nr_ue_pdcch_procedures(uint8_t gNB_id,
                           PHY_VARS_NR_UE *ue,
                           UE_nr_rxtx_proc_t *proc,
                           int32_t pdcch_est_size,
                           int32_t pdcch_dl_ch_estimates[][pdcch_est_size],
                           NR_UE_PDCCH_CONFIG *phy_pdcch_config,
                           int n_ss);

int nr_ue_csi_im_procedures(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, uint8_t gNB_id);

int nr_ue_csi_rs_procedures(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, uint8_t gNB_id);

#endif

