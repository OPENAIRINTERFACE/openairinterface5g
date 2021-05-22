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

#ifndef __openair_SCHED_ENB_H__
#define __openair_SCHED_ENB_H__

#include "PHY/defs_eNB.h"
#include "PHY_INTERFACE/phy_interface.h"
#include "sched_common.h"

enum THREAD_INDEX { OPENAIR_THREAD_INDEX = 0,
                    TOP_LEVEL_SCHEDULER_THREAD_INDEX,
                    DLC_SCHED_THREAD_INDEX,
                    openair_SCHED_NB_THREADS
                  }; // do not modify this line


#define OPENAIR_THREAD_PRIORITY        255


#define OPENAIR_THREAD_STACK_SIZE     PTHREAD_STACK_MIN //4096 //RTL_PTHREAD_STACK_MIN*6
//#define DLC_THREAD_STACK_SIZE        4096 //DLC stack size
//#define UE_SLOT_PARALLELISATION

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
  openair_SYNCHED,
  openair_SCHED_EXIT
};

enum openair_HARQ_TYPE {
  openair_harq_DL = 0,
  openair_harq_UL,
  openair_harq_RA
};

#define DAQ_AGC_ON 1
#define DAQ_AGC_OFF 0


/** @addtogroup _PHY_PROCEDURES_
 * @{
 */



/*! \brief Scheduling for eNB TX procedures in normal subframes.
  @param phy_vars_eNB Pointer to eNB variables on which to act
  @param abstraction_flag Indicator of PHY abstraction
  @param do_meas Do inline timing measurement
*/
void phy_procedures_eNB_TX(PHY_VARS_eNB *phy_vars_eNB,L1_rxtx_proc_t *proc,int do_meas);

/*! \brief Scheduling for eNB RX UE-specific procedures in normal subframes.
  @param phy_vars_eNB Pointer to eNB variables on which to act
  @param proc Pointer to RXn-TXnp4 proc information
*/
void phy_procedures_eNB_uespec_RX(PHY_VARS_eNB *phy_vars_eNB,L1_rxtx_proc_t *proc);

/*! \brief Scheduling for eNB TX procedures in TDD S-subframes.
  @param phy_vars_eNB Pointer to eNB variables on which to act
  @param proc Pointer to RXn-TXnp4 proc information
  @param r_type indicates the relaying operation: 0: no_relaying, 1: unicast relaying type 1, 2: unicast relaying type 2, 3: multicast relaying
*/

/*! \brief Scheduling for eNB RX common procedures in normal subframes.
  @param phy_vars_eNB Pointer to eNB variables on which to act
  @param abstraction_flag Indicator of PHY abstraction
*/
void phy_procedures_eNB_common_RX(PHY_VARS_eNB *phy_vars_eNB,L1_rxtx_proc_t *proc);

/*! \brief Scheduling for eNB TX procedures in TDD S-subframes.
  @param phy_vars_eNB Pointer to eNB variables on which to act
*/

void phy_procedures_eNB_S_TX(PHY_VARS_eNB *phy_vars_eNB);

/*! \brief Scheduling for eNB RX procedures in TDD S-subframes.
  @param phy_vars_eNB Pointer to eNB variables on which to act
*/
void phy_procedures_eNB_S_RX(PHY_VARS_eNB *phy_vars_eNB,L1_rxtx_proc_t *proc);

/*! \brief Scheduling for eNB PRACH RX procedures
  @param phy_vars_eNB Pointer to eNB variables on which to act
  @param br_flag indicator for eMTC PRACH
*/

void prach_procedures(PHY_VARS_eNB *eNB, 
		      int br_flag);


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

/*! \brief Indicates the SR TXOp in current subframe for eNB and particular UE index.  Implements Table 10.1-5 from 36.213.
  @param phy_vars_eNB Pointer to eNB variables
  @param UE_id ID of UE which may be issuing the SR
  @returns 1 if TXOp is active.
*/
uint8_t is_SR_subframe(PHY_VARS_eNB *phy_vars_eNB,L1_rxtx_proc_t *proc,uint8_t UE_id);

int8_t find_ue_dlsch(uint16_t rnti, PHY_VARS_eNB *phy_vars_eNB);
int8_t find_ue_ulsch(uint16_t rnti, PHY_VARS_eNB *phy_vars_eNB);





void schedule_response(Sched_Rsp_t *Sched_INFO, void *proc);

LTE_eNB_UE_stats *get_UE_stats(uint8_t Mod_id, uint8_t CC_id,uint16_t rnti);

/*! \brief Function to compute subframe type as a function of Frame type and TDD Configuration (implements Table 4.2.2 from 36.211, p.11 from version 8.6) and subframe index.  Same as subframe_select, except that it uses the Mod_id and is provided as a service to the MAC scheduler.
  @param Mod_id Index of eNB
  @param CC_id Component Carrier Index
  @param subframe Subframe index
  @returns Subframe type (DL,UL,S)
*/
lte_subframe_t get_subframe_direction(uint8_t Mod_id,uint8_t CC_id,uint8_t subframe);


int16_t get_hundred_times_delta_IF_eNB(PHY_VARS_eNB *phy_vars_eNB,uint16_t UE_id,uint8_t harq_pid, uint8_t bw_factor);

int16_t get_hundred_times_delta_IF_mac(module_id_t module_idP, uint8_t CC_id, rnti_t rnti, uint8_t harq_pid);

int16_t get_target_pusch_rx_power(module_id_t module_idP, uint8_t CC_id);
int16_t get_target_pucch_rx_power(module_id_t module_idP, uint8_t CC_id);

int is_srs_occasion_common(LTE_DL_FRAME_PARMS *frame_parms,int frame_tx,int subframe_tx);

void compute_srs_pos(lte_frame_type_t frameType,uint16_t isrs,uint16_t *psrsPeriodicity,uint16_t *psrsOffset);

void release_rnti_of_phy(module_id_t mod_id);
/*@}*/


#endif


