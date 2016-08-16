
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

/*! \file phy_procedures_lte_eNB.c
 * \brief Implementation of eNB procedures from 36.213 LTE specifications
 * \author R. Knopp, F. Kaltenberger, N. Nikaein
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr,navid.nikaein@eurecom.fr
 * \note
 * \warning
 */

#include "PHY/defs.h"
#include "PHY/extern.h"
#include "SCHED/defs.h"
#include "SCHED/extern.h"

#ifdef EMOS
#include "SCHED/phy_procedures_emos.h"
#endif

//#define DEBUG_PHY_PROC (Already defined in cmake)
//#define DEBUG_ULSCH

#include "LAYER2/MAC/extern.h"
#include "LAYER2/MAC/defs.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

#include "T.h"

#include "assertions.h"
#include "msc.h"

#if defined(ENABLE_ITTI)
#   include "intertask_interface.h"
#   if ENABLE_RAL
#     include "timer.h"
#   endif
#endif

//#define DIAG_PHY

#define NS_PER_SLOT 500000

#define PUCCH 1

extern int exit_openair;


unsigned char dlsch_input_buffer[2700] __attribute__ ((aligned(32)));
int eNB_sync_buffer0[640*6] __attribute__ ((aligned(32)));
int eNB_sync_buffer1[640*6] __attribute__ ((aligned(32)));
int *eNB_sync_buffer[2] = {eNB_sync_buffer0, eNB_sync_buffer1};

extern uint16_t hundred_times_log10_NPRB[100];

unsigned int max_peak_val;
int max_sync_pos;

//DCI_ALLOC_t dci_alloc[8];

#ifdef EMOS
fifo_dump_emos_eNB emos_dump_eNB;
#endif

#if defined(SMBV) && !defined(EXMIMO)
extern const char smbv_fname[];
extern unsigned short config_frames[4];
extern uint8_t smbv_frame_cnt;
#endif

#ifdef DIAG_PHY
extern int rx_sig_fifo;
#endif

uint8_t is_SR_subframe(PHY_VARS_eNB *phy_vars_eNB,uint8_t UE_id,uint8_t sched_subframe)
{

  const int subframe = phy_vars_eNB->proc[sched_subframe].subframe_rx;
  const int frame = phy_vars_eNB->proc[sched_subframe].frame_rx;

  LOG_D(PHY,"[eNB %d][SR %x] Frame %d subframe %d Checking for SR TXOp(sr_ConfigIndex %d)\n",
        phy_vars_eNB->Mod_id,phy_vars_eNB->ulsch_eNB[UE_id]->rnti,frame,subframe,
        phy_vars_eNB->scheduling_request_config[UE_id].sr_ConfigIndex);

  if (phy_vars_eNB->scheduling_request_config[UE_id].sr_ConfigIndex <= 4) {        // 5 ms SR period
    if ((subframe%5) == phy_vars_eNB->scheduling_request_config[UE_id].sr_ConfigIndex)
      return(1);
  } else if (phy_vars_eNB->scheduling_request_config[UE_id].sr_ConfigIndex <= 14) { // 10 ms SR period
    if (subframe==(phy_vars_eNB->scheduling_request_config[UE_id].sr_ConfigIndex-5))
      return(1);
  } else if (phy_vars_eNB->scheduling_request_config[UE_id].sr_ConfigIndex <= 34) { // 20 ms SR period
    if ((10*(frame&1)+subframe) == (phy_vars_eNB->scheduling_request_config[UE_id].sr_ConfigIndex-15))
      return(1);
  } else if (phy_vars_eNB->scheduling_request_config[UE_id].sr_ConfigIndex <= 74) { // 40 ms SR period
    if ((10*(frame&3)+subframe) == (phy_vars_eNB->scheduling_request_config[UE_id].sr_ConfigIndex-35))
      return(1);
  } else if (phy_vars_eNB->scheduling_request_config[UE_id].sr_ConfigIndex <= 154) { // 80 ms SR period
    if ((10*(frame&7)+subframe) == (phy_vars_eNB->scheduling_request_config[UE_id].sr_ConfigIndex-75))
      return(1);
  }

  return(0);
}

int32_t add_ue(int16_t rnti, PHY_VARS_eNB *phy_vars_eNB)
{
  uint8_t i;

#ifdef DEBUG_PHY_PROC
  LOG_I(PHY,"[eNB %d/%d] Adding UE with rnti %x\n",
        phy_vars_eNB->Mod_id,
        phy_vars_eNB->CC_id,
        (uint16_t)rnti);
#endif

  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    if ((phy_vars_eNB->dlsch_eNB[i]==NULL) || (phy_vars_eNB->ulsch_eNB[i]==NULL)) {
      MSC_LOG_EVENT(MSC_PHY_ENB, "0 Failed add ue %"PRIx16" (ENOMEM)", rnti);
      LOG_E(PHY,"Can't add UE, not enough memory allocated\n");
      return(-1);
    } else {
      if (phy_vars_eNB->eNB_UE_stats[i].crnti==0) {
        MSC_LOG_EVENT(MSC_PHY_ENB, "0 Add ue %"PRIx16" ", rnti);
        LOG_D(PHY,"UE_id %d associated with rnti %x\n",i, (uint16_t)rnti);
        phy_vars_eNB->dlsch_eNB[i][0]->rnti = rnti;
        phy_vars_eNB->ulsch_eNB[i]->rnti = rnti;
        phy_vars_eNB->eNB_UE_stats[i].crnti = rnti;

	phy_vars_eNB->eNB_UE_stats[i].Po_PUCCH1_below = 0;
	phy_vars_eNB->eNB_UE_stats[i].Po_PUCCH1_above = (int32_t)pow(10.0,.1*(phy_vars_eNB->lte_frame_parms.ul_power_control_config_common.p0_NominalPUCCH+phy_vars_eNB->rx_total_gain_eNB_dB));
	phy_vars_eNB->eNB_UE_stats[i].Po_PUCCH        = (int32_t)pow(10.0,.1*(phy_vars_eNB->lte_frame_parms.ul_power_control_config_common.p0_NominalPUCCH+phy_vars_eNB->rx_total_gain_eNB_dB));
	LOG_D(PHY,"Initializing Po_PUCCH: p0_NominalPUCCH %d, gain %d => %d\n",
	      phy_vars_eNB->lte_frame_parms.ul_power_control_config_common.p0_NominalPUCCH,
	      phy_vars_eNB->rx_total_gain_eNB_dB,
	      phy_vars_eNB->eNB_UE_stats[i].Po_PUCCH);
  
        return(i);
      }
    }
  }
  return(-1);
}

int mac_phy_remove_ue(module_id_t Mod_idP,rnti_t rntiP) {
  uint8_t i;
  int j,CC_id;
  PHY_VARS_eNB *phy_vars_eNB;

  for (CC_id=0;CC_id<MAX_NUM_CCs;CC_id++) {
    phy_vars_eNB = PHY_vars_eNB_g[Mod_idP][CC_id];
    for (i=0; i<NUMBER_OF_UE_MAX; i++) {
      if ((phy_vars_eNB->dlsch_eNB[i]==NULL) || (phy_vars_eNB->ulsch_eNB[i]==NULL)) {
	MSC_LOG_EVENT(MSC_PHY_ENB, "0 Failed remove ue %"PRIx16" (ENOMEM)", rnti);
	LOG_E(PHY,"Can't remove UE, not enough memory allocated\n");
	return(-1);
      } else {
	if (phy_vars_eNB->eNB_UE_stats[i].crnti==rntiP) {
	  MSC_LOG_EVENT(MSC_PHY_ENB, "0 Removed ue %"PRIx16" ", rntiP);
#ifdef DEBUG_PHY_PROC
	  LOG_I(PHY,"eNB %d removing UE %d with rnti %x\n",phy_vars_eNB->Mod_id,i,rnti);
#endif
	  //msg("[PHY] UE_id %d\n",i);
	  clean_eNb_dlsch(phy_vars_eNB->dlsch_eNB[i][0]);
	  clean_eNb_ulsch(phy_vars_eNB->ulsch_eNB[i]);
	  //phy_vars_eNB->eNB_UE_stats[i].crnti = 0;
	  memset(&phy_vars_eNB->eNB_UE_stats[i],0,sizeof(LTE_eNB_UE_stats));
	  //  mac_exit_wrapper("Removing UE");
	  
	  return(i);
	}
      }
    }
  }
  MSC_LOG_EVENT(MSC_PHY_ENB, "0 Failed remove ue %"PRIx16" (not found)", rntiP);
  return(-1);
}

int8_t find_next_ue_index(PHY_VARS_eNB *phy_vars_eNB)
{
  uint8_t i;

  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    if (phy_vars_eNB->eNB_UE_stats[i].crnti==0) {
      /*if ((phy_vars_eNB->dlsch_eNB[i]) &&
      (phy_vars_eNB->dlsch_eNB[i][0]) &&
      (phy_vars_eNB->dlsch_eNB[i][0]->rnti==0))*/
      LOG_D(PHY,"Next free UE id is %d\n",i);
      return(i);
    }
  }

  return(-1);
}

int get_ue_active_harq_pid(const uint8_t Mod_id,const uint8_t CC_id,const uint16_t rnti, const int frame, const uint8_t subframe,uint8_t *harq_pid,uint8_t *round,const uint8_t ul_flag)
{
  LTE_eNB_DLSCH_t *DLSCH_ptr;
  LTE_eNB_ULSCH_t *ULSCH_ptr;
  uint8_t ulsch_subframe,ulsch_frame;
  int i;
  int8_t UE_id = find_ue(rnti,PHY_vars_eNB_g[Mod_id][CC_id]);

  if (UE_id==-1) {
    LOG_D(PHY,"Cannot find UE with rnti %x (Mod_id %d, CC_id %d)\n",rnti, Mod_id, CC_id);
    *round=0;
    return(-1);
  }

  if (ul_flag == 0)  {// this is a DL request
    DLSCH_ptr = PHY_vars_eNB_g[Mod_id][CC_id]->dlsch_eNB[(uint32_t)UE_id][0];

    /* let's go synchronous for the moment - maybe we can change at some point */
    i = (frame * 10 + subframe) % 8;

    if (DLSCH_ptr->harq_processes[i]->status == ACTIVE) {
      *harq_pid = i;
      *round = DLSCH_ptr->harq_processes[i]->round;
    } else if (DLSCH_ptr->harq_processes[i]->status == SCH_IDLE) {
      *harq_pid = i;
      *round = 0;
    } else {
      printf("%s:%d: bad state for harq process - PLEASE REPORT!!\n", __FILE__, __LINE__);
      abort();
    }
  } else { // This is a UL request

    ULSCH_ptr = PHY_vars_eNB_g[Mod_id][CC_id]->ulsch_eNB[(uint32_t)UE_id];
    ulsch_subframe = pdcch_alloc2ul_subframe(&PHY_vars_eNB_g[Mod_id][CC_id]->lte_frame_parms,subframe);
    ulsch_frame    = pdcch_alloc2ul_frame(&PHY_vars_eNB_g[Mod_id][CC_id]->lte_frame_parms,frame,subframe);
    // Note this is for TDD configuration 3,4,5 only
    *harq_pid = subframe2harq_pid(&PHY_vars_eNB_g[Mod_id][CC_id]->lte_frame_parms,
                                  ulsch_frame,
                                  ulsch_subframe);
    *round    = ULSCH_ptr->harq_processes[*harq_pid]->round;
    LOG_T(PHY,"[eNB %d][PUSCH %d] Frame %d subframe %d Checking HARQ, round %d\n",Mod_id,*harq_pid,frame,subframe,*round);
  }

  return(0);
}

int16_t get_target_pusch_rx_power(const module_id_t module_idP, const uint8_t CC_id)
{
  //return PHY_vars_eNB_g[module_idP][CC_id]->PHY_measurements_eNB[0].n0_power_tot_dBm;
  return PHY_vars_eNB_g[module_idP][CC_id]->lte_frame_parms.ul_power_control_config_common.p0_NominalPUSCH;
}

int16_t get_target_pucch_rx_power(const module_id_t module_idP, const uint8_t CC_id)
{
  //return PHY_vars_eNB_g[module_idP][CC_id]->PHY_measurements_eNB[0].n0_power_tot_dBm;
  return PHY_vars_eNB_g[module_idP][CC_id]->lte_frame_parms.ul_power_control_config_common.p0_NominalPUCCH;
}

#ifdef EMOS
void phy_procedures_emos_eNB_TX(unsigned char subframe, PHY_VARS_eNB *phy_vars_eNB)
{

}
#endif



void phy_procedures_eNB_S_RX(unsigned char sched_subframe,PHY_VARS_eNB *phy_vars_eNB,uint8_t abstraction_flag,relaying_type_t r_type)
{
  UNUSED(r_type);

  int subframe = phy_vars_eNB->proc[sched_subframe].subframe_rx;

#ifdef DEBUG_PHY_PROC
  LOG_D(PHY,"[eNB %d] Frame %d: Doing phy_procedures_eNB_S_RX(%d)\n", phy_vars_eNB->Mod_id,phy_vars_eNB->proc[sched_subframe].frame_rx, subframe);
#endif


  if (abstraction_flag == 0) {
    lte_eNB_I0_measurements(phy_vars_eNB,
			    subframe,
                            0,
                            phy_vars_eNB->first_run_I0_measurements);
  }

#ifdef PHY_ABSTRACTION
  else {
    lte_eNB_I0_measurements_emul(phy_vars_eNB,
                                 0);
  }

#endif


}



#ifdef EMOS
void phy_procedures_emos_eNB_RX(unsigned char subframe,PHY_VARS_eNB *phy_vars_eNB)
{

  uint8_t aa;
  uint16_t last_subframe_emos;
  uint16_t pilot_pos1 = 3 - phy_vars_eNB->lte_frame_parms.Ncp, pilot_pos2 = 10 - 2*phy_vars_eNB->lte_frame_parms.Ncp;
  uint32_t bytes;

  last_subframe_emos=0;




#ifdef EMOS_CHANNEL

  //if (last_slot%2==1) // this is for all UL subframes
  if (subframe==3)
    for (aa=0; aa<phy_vars_eNB->lte_frame_parms.nb_antennas_rx; aa++) {
      memcpy(&emos_dump_eNB.channel[aa][last_subframe_emos*2*phy_vars_eNB->lte_frame_parms.N_RB_UL*12],
             &phy_vars_eNB->lte_eNB_pusch_vars[0]->drs_ch_estimates[0][aa][phy_vars_eNB->lte_frame_parms.N_RB_UL*12*pilot_pos1],
             phy_vars_eNB->lte_frame_parms.N_RB_UL*12*sizeof(int));
      memcpy(&emos_dump_eNB.channel[aa][(last_subframe_emos*2+1)*phy_vars_eNB->lte_frame_parms.N_RB_UL*12],
             &phy_vars_eNB->lte_eNB_pusch_vars[0]->drs_ch_estimates[0][aa][phy_vars_eNB->lte_frame_parms.N_RB_UL*12*pilot_pos2],
             phy_vars_eNB->lte_frame_parms.N_RB_UL*12*sizeof(int));
    }

#endif

  if (subframe==4) {
    emos_dump_eNB.timestamp = rt_get_time_ns();
    emos_dump_eNB.frame_tx = phy_vars_eNB->proc[subframe].frame_rx;
    emos_dump_eNB.rx_total_gain_dB = phy_vars_eNB->rx_total_gain_eNB_dB;
    emos_dump_eNB.mimo_mode = phy_vars_eNB->transmission_mode[0];
    memcpy(&emos_dump_eNB.PHY_measurements_eNB,
           &phy_vars_eNB->PHY_measurements_eNB[0],
           sizeof(PHY_MEASUREMENTS_eNB));
    memcpy(&emos_dump_eNB.eNB_UE_stats[0],&phy_vars_eNB->eNB_UE_stats[0],NUMBER_OF_UE_MAX*sizeof(LTE_eNB_UE_stats));

    bytes = rtf_put(CHANSOUNDER_FIFO_MINOR, &emos_dump_eNB, sizeof(fifo_dump_emos_eNB));

    //bytes = rtf_put(CHANSOUNDER_FIFO_MINOR, "test", sizeof("test"));
    if (bytes!=sizeof(fifo_dump_emos_eNB)) {
      LOG_W(PHY,"[eNB %d] Frame %d, subframe %d, Problem writing EMOS data to FIFO (bytes=%d, size=%d)\n",
            phy_vars_eNB->Mod_id,phy_vars_eNB->proc[(subframe+1)%10].frame_rx, subframe,bytes,sizeof(fifo_dump_emos_eNB));
    } else {
      if (phy_vars_eNB->proc[(subframe+1)%10].frame_tx%100==0) {
        LOG_I(PHY,"[eNB %d] Frame %d (%d), subframe %d, Writing %d bytes EMOS data to FIFO\n",
              phy_vars_eNB->Mod_id,phy_vars_eNB->proc[(subframe+1)%10].frame_rx, ((fifo_dump_emos_eNB*)&emos_dump_eNB)->frame_tx, subframe, bytes);
      }
    }
  }
}
#endif


#define AMP_OVER_SQRT2 ((AMP*ONE_OVER_SQRT2_Q15)>>15)
#define AMP_OVER_2 (AMP>>1)
int QPSK[4]= {AMP_OVER_SQRT2|(AMP_OVER_SQRT2<<16),AMP_OVER_SQRT2|((65536-AMP_OVER_SQRT2)<<16),((65536-AMP_OVER_SQRT2)<<16)|AMP_OVER_SQRT2,((65536-AMP_OVER_SQRT2)<<16)|(65536-AMP_OVER_SQRT2)};
int QPSK2[4]= {AMP_OVER_2|(AMP_OVER_2<<16),AMP_OVER_2|((65536-AMP_OVER_2)<<16),((65536-AMP_OVER_2)<<16)|AMP_OVER_2,((65536-AMP_OVER_2)<<16)|(65536-AMP_OVER_2)};


#if defined(ENABLE_ITTI)
#   if ENABLE_RAL
extern PHY_MEASUREMENTS PHY_measurements;

void phy_eNB_lte_measurement_thresholds_test_and_report(instance_t instanceP, ral_threshold_phy_t* threshold_phy_pP, uint16_t valP)
{
  MessageDef *message_p = NULL;

  if (
    (
      ((threshold_phy_pP->threshold.threshold_val <  valP) && (threshold_phy_pP->threshold.threshold_xdir == RAL_ABOVE_THRESHOLD)) ||
      ((threshold_phy_pP->threshold.threshold_val >  valP) && (threshold_phy_pP->threshold.threshold_xdir == RAL_BELOW_THRESHOLD))
    )  ||
    (threshold_phy_pP->threshold.threshold_xdir == RAL_NO_THRESHOLD)
  ) {
    message_p = itti_alloc_new_message(TASK_PHY_ENB , PHY_MEAS_REPORT_IND);
    memset(&PHY_MEAS_REPORT_IND(message_p), 0, sizeof(PHY_MEAS_REPORT_IND(message_p)));

    memcpy(&PHY_MEAS_REPORT_IND (message_p).threshold,
           &threshold_phy_pP->threshold,
           sizeof(PHY_MEAS_REPORT_IND (message_p).threshold));

    memcpy(&PHY_MEAS_REPORT_IND (message_p).link_param,
           &threshold_phy_pP->link_param,
           sizeof(PHY_MEAS_REPORT_IND (message_p).link_param));
    \

    switch (threshold_phy_pP->link_param.choice) {
    case RAL_LINK_PARAM_CHOICE_LINK_PARAM_VAL:
      PHY_MEAS_REPORT_IND (message_p).link_param._union.link_param_val = valP;
      break;

    case RAL_LINK_PARAM_CHOICE_QOS_PARAM_VAL:
      //PHY_MEAS_REPORT_IND (message_p).link_param._union.qos_param_val.
      AssertFatal (1 == 0, "TO DO RAL_LINK_PARAM_CHOICE_QOS_PARAM_VAL\n");
      break;
    }

    itti_send_msg_to_task(TASK_RRC_ENB, instanceP, message_p);
  }
}

void phy_eNB_lte_check_measurement_thresholds(instance_t instanceP, ral_threshold_phy_t* threshold_phy_pP)
{
  unsigned int  mod_id;

  mod_id = instanceP;

  switch (threshold_phy_pP->link_param.link_param_type.choice) {

  case RAL_LINK_PARAM_TYPE_CHOICE_GEN:
    switch (threshold_phy_pP->link_param.link_param_type._union.link_param_gen) {
    case RAL_LINK_PARAM_GEN_DATA_RATE:
      phy_eNB_lte_measurement_thresholds_test_and_report(instanceP, threshold_phy_pP, 0);
      break;

    case RAL_LINK_PARAM_GEN_SIGNAL_STRENGTH:
      phy_eNB_lte_measurement_thresholds_test_and_report(instanceP, threshold_phy_pP, 0);
      break;

    case RAL_LINK_PARAM_GEN_SINR:
      phy_eNB_lte_measurement_thresholds_test_and_report(instanceP, threshold_phy_pP, 0);
      break;

    case RAL_LINK_PARAM_GEN_THROUGHPUT:
      break;

    case RAL_LINK_PARAM_GEN_PACKET_ERROR_RATE:
      break;

    default:
      ;
    }

    break;

  case RAL_LINK_PARAM_TYPE_CHOICE_LTE:
    switch (threshold_phy_pP->link_param.link_param_type._union.link_param_gen) {
    case RAL_LINK_PARAM_LTE_UE_RSRP:
      break;

    case RAL_LINK_PARAM_LTE_UE_RSRQ:
      break;

    case RAL_LINK_PARAM_LTE_UE_CQI:
      break;

    case RAL_LINK_PARAM_LTE_AVAILABLE_BW:
      break;

    case RAL_LINK_PARAM_LTE_PACKET_DELAY:
      break;

    case RAL_LINK_PARAM_LTE_PACKET_LOSS_RATE:
      break;

    case RAL_LINK_PARAM_LTE_L2_BUFFER_STATUS:
      break;

    case RAL_LINK_PARAM_LTE_MOBILE_NODE_CAPABILITIES:
      break;

    case RAL_LINK_PARAM_LTE_EMBMS_CAPABILITY:
      break;

    case RAL_LINK_PARAM_LTE_JUMBO_FEASIBILITY:
      break;

    case RAL_LINK_PARAM_LTE_JUMBO_SETUP_STATUS:
      break;

    case RAL_LINK_PARAM_LTE_NUM_ACTIVE_EMBMS_RECEIVERS_PER_FLOW:
      break;

    default:
      ;
    }

    break;

  default:
    ;
  }
}
#   endif
#endif


unsigned int taus(void);
DCI_PDU DCI_pdu_tmp;

void phy_procedures_eNB_TX(unsigned char sched_subframe,PHY_VARS_eNB *phy_vars_eNB,uint8_t abstraction_flag,
                           relaying_type_t r_type,PHY_VARS_RN *phy_vars_rn)
{
  UNUSED(phy_vars_rn);
  uint8_t *pbch_pdu=&phy_vars_eNB->pbch_pdu[0];
  uint16_t input_buffer_length, re_allocated=0;
  uint32_t i,aa;
  uint8_t harq_pid;
  DCI_PDU *DCI_pdu;
  uint8_t *DLSCH_pdu=NULL;
  //DCI_PDU DCI_pdu_tmp;
  uint8_t DLSCH_pdu_tmp[768*8];
  int8_t UE_id;
  uint8_t num_pdcch_symbols=0;
  uint8_t ul_subframe;
  uint32_t ul_frame;
#ifdef Rel10
  MCH_PDU *mch_pduP;
  MCH_PDU  mch_pdu;
  //  uint8_t sync_area=255;
#endif
#if defined(SMBV) && !defined(EXMIMO)
  // counts number of allocations in subframe
  // there is at least one allocation for PDCCH
  uint8_t smbv_alloc_cnt = 1;
#endif
  int frame = phy_vars_eNB->proc[sched_subframe].frame_tx;
  int subframe = phy_vars_eNB->proc[sched_subframe].subframe_tx;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_TX,1);
  start_meas(&phy_vars_eNB->phy_proc_tx);

  T(T_ENB_PHY_DL_TICK, T_INT(phy_vars_eNB->Mod_id), T_INT(frame), T_INT(subframe));

  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    // If we've dropped the UE, go back to PRACH mode for this UE

    if (phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors == ULSCH_max_consecutive_errors) {
      LOG_W(PHY,"[eNB %d, CC %d] frame %d, subframe %d, UE %d: ULSCH consecutive error count reached %u, triggering UL Failure\n",
            phy_vars_eNB->Mod_id,phy_vars_eNB->CC_id,frame,subframe, i, phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors);
      phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors=0;
      mac_xface->UL_failure_indication(phy_vars_eNB->Mod_id,
				       phy_vars_eNB->CC_id,
				       frame,
				       phy_vars_eNB->eNB_UE_stats[i].crnti,
				       subframe);
				       
    }
	

  }


  // Get scheduling info for next subframe
  if (phy_vars_eNB->mac_enabled==1) {
    if (phy_vars_eNB->CC_id == 0) {
      mac_xface->eNB_dlsch_ulsch_scheduler(phy_vars_eNB->Mod_id,0,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe);//,1);
    }
  }

  if (abstraction_flag==0) {
    // clear the transmit data array for the current subframe
    for (aa=0; aa<phy_vars_eNB->lte_frame_parms.nb_antennas_tx_eNB; aa++) {

      memset(&phy_vars_eNB->lte_eNB_common_vars.txdataF[0][aa][subframe*phy_vars_eNB->lte_frame_parms.ofdm_symbol_size*(phy_vars_eNB->lte_frame_parms.symbols_per_tti)],
             0,phy_vars_eNB->lte_frame_parms.ofdm_symbol_size*(phy_vars_eNB->lte_frame_parms.symbols_per_tti)*sizeof(int32_t));
    }
  }


  if (is_pmch_subframe(phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,&phy_vars_eNB->lte_frame_parms)) {

    if (abstraction_flag==0) {
      // This is DL-Cell spec pilots in Control region
      generate_pilots_slot(phy_vars_eNB,
                           phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                           AMP,
                           subframe<<1,1);
    }

#ifdef Rel10
    // if mcch is active, send regardless of the node type: eNB or RN
    // when mcch is active, MAC sched does not allow MCCH and MTCH multiplexing
    mch_pduP = mac_xface->get_mch_sdu(phy_vars_eNB->Mod_id,
                                      phy_vars_eNB->CC_id,
                                      phy_vars_eNB->proc[sched_subframe].frame_tx,
                                      subframe);

    switch (r_type) {
    case no_relay:
      if ((mch_pduP->Pdu_size > 0) && (mch_pduP->sync_area == 0)) // TEST: only transmit mcch for sync area 0
        LOG_I(PHY,"[eNB%"PRIu8"] Frame %d subframe %d : Got MCH pdu for MBSFN (MCS %"PRIu8", TBS %d) \n",
              phy_vars_eNB->Mod_id,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,mch_pduP->mcs,
              phy_vars_eNB->dlsch_eNB_MCH->harq_processes[0]->TBS>>3);
      else {
        LOG_D(PHY,"[DeNB %"PRIu8"] Frame %d subframe %d : Do not transmit MCH pdu for MBSFN sync area %"PRIu8" (%s)\n",
              phy_vars_eNB->Mod_id,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,mch_pduP->sync_area,
              (mch_pduP->Pdu_size == 0)? "Empty MCH PDU":"Let RN transmit for the moment");
        mch_pduP = NULL;
      }

      break;

    case multicast_relay:
      if ((mch_pduP->Pdu_size > 0) && ((mch_pduP->mcch_active == 1) || mch_pduP->msi_active==1)) {
        LOG_I(PHY,"[RN %"PRIu8"] Frame %d subframe %d: Got the MCH PDU for MBSFN  sync area %"PRIu8" (MCS %"PRIu8", TBS %"PRIu16")\n",
              phy_vars_rn->Mod_id,phy_vars_rn->frame, subframe,
              mch_pduP->sync_area,mch_pduP->mcs,mch_pduP->Pdu_size);
      } else if (phy_vars_rn->mch_avtive[subframe%5] == 1) { // SF2 -> SF7, SF3 -> SF8
        mch_pduP= &mch_pdu;
        memcpy(&mch_pduP->payload, // could be a simple copy
               phy_vars_rn->dlsch_rn_MCH[subframe%5]->harq_processes[0]->b,
               phy_vars_rn->dlsch_rn_MCH[subframe%5]->harq_processes[0]->TBS>>3);
        mch_pduP->Pdu_size = (uint16_t) (phy_vars_rn->dlsch_rn_MCH[subframe%5]->harq_processes[0]->TBS>>3);
        mch_pduP->mcs = phy_vars_rn->dlsch_rn_MCH[subframe%5]->harq_processes[0]->mcs;
        LOG_I(PHY,"[RN %"PRIu8"] Frame %d subframe %d: Forward the MCH PDU for MBSFN received on SF %d sync area %"PRIu8" (MCS %"PRIu8", TBS %"PRIu16")\n",
              phy_vars_rn->Mod_id,phy_vars_rn->frame, subframe,subframe%5,
              phy_vars_rn->sync_area[subframe%5],mch_pduP->mcs,mch_pduP->Pdu_size);
      } else {
        mch_pduP=NULL;
      }

      phy_vars_rn->mch_avtive[subframe]=0;
      break;

    default:
      LOG_W(PHY,"[eNB %"PRIu8"] Frame %d subframe %d: unknown relaying type %d \n",
            phy_vars_eNB->Mod_id,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,r_type);
      mch_pduP=NULL;
      break;
    }// switch

    if (mch_pduP) {
      fill_eNB_dlsch_MCH(phy_vars_eNB,mch_pduP->mcs,1,0, abstraction_flag);
      // Generate PMCH
      generate_mch(phy_vars_eNB,sched_subframe,(uint8_t*)mch_pduP->payload,abstraction_flag);
#ifdef DEBUG_PHY

      for (i=0; i<mch_pduP->Pdu_size; i++)
        msg("%2"PRIx8".",(uint8_t)mch_pduP->payload[i]);

      msg("\n");
#endif
    } else {
      LOG_D(PHY,"[eNB/RN] Frame %d subframe %d: MCH not generated \n",phy_vars_eNB->proc[sched_subframe].frame_tx,subframe);
    }

#endif
  }

  else {
    // this is not a pmch subframe

    if (abstraction_flag==0) {
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_RS_TX,1);
      generate_pilots_slot(phy_vars_eNB,
                           phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                           AMP,
                           subframe<<1,0);
      if (subframe_select(&phy_vars_eNB->lte_frame_parms,subframe) == SF_DL)
	generate_pilots_slot(phy_vars_eNB,
			     phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
			     AMP,
			     (subframe<<1)+1,0);

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_RS_TX,0);

      // First half of PSS/SSS (FDD)
      if (subframe == 0) {
        if (phy_vars_eNB->lte_frame_parms.frame_type == FDD) {
          generate_pss(phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                       AMP,
                       &phy_vars_eNB->lte_frame_parms,
                       (phy_vars_eNB->lte_frame_parms.Ncp==NORMAL) ? 6 : 5,
                       0);
          generate_sss(phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                       AMP,
                       &phy_vars_eNB->lte_frame_parms,
                       (phy_vars_eNB->lte_frame_parms.Ncp==NORMAL) ? 5 : 4,
                       0);

        }
      }
    }
  }

  if (subframe == 0) {
    // generate PBCH (Physical Broadcast CHannel) info
    if ((phy_vars_eNB->proc[sched_subframe].frame_tx&3) == 0) {
      pbch_pdu[2] = 0;

      // FIXME setting pbch_pdu[2] to zero makes the switch statement easier: remove all the or-operators
      switch (phy_vars_eNB->lte_frame_parms.N_RB_DL) {
      case 6:
        pbch_pdu[2] = (pbch_pdu[2]&0x1f) | (0<<5);
        break;

      case 15:
        pbch_pdu[2] = (pbch_pdu[2]&0x1f) | (1<<5);
        break;

      case 25:
        pbch_pdu[2] = (pbch_pdu[2]&0x1f) | (2<<5);
        break;

      case 50:
        pbch_pdu[2] = (pbch_pdu[2]&0x1f) | (3<<5);
        break;

      case 75:
        pbch_pdu[2] = (pbch_pdu[2]&0x1f) | (4<<5);
        break;

      case 100:
        pbch_pdu[2] = (pbch_pdu[2]&0x1f) | (5<<5);
        break;

      default:
        // FIXME if we get here, this should be flagged as an error, right?
        pbch_pdu[2] = (pbch_pdu[2]&0x1f) | (2<<5);
        break;
      }

      pbch_pdu[2] = (pbch_pdu[2]&0xef) |
                    ((phy_vars_eNB->lte_frame_parms.phich_config_common.phich_duration << 4)&0x10);

      switch (phy_vars_eNB->lte_frame_parms.phich_config_common.phich_resource) {
      case oneSixth:
        pbch_pdu[2] = (pbch_pdu[2]&0xf3) | (0<<2);
        break;

      case half:
        pbch_pdu[2] = (pbch_pdu[2]&0xf3) | (1<<2);
        break;

      case one:
        pbch_pdu[2] = (pbch_pdu[2]&0xf3) | (2<<2);
        break;

      case two:
        pbch_pdu[2] = (pbch_pdu[2]&0xf3) | (3<<2);
        break;

      default:
        // unreachable
        break;
      }

      pbch_pdu[2] = (pbch_pdu[2]&0xfc) | ((phy_vars_eNB->proc[sched_subframe].frame_tx>>8)&0x3);
      pbch_pdu[1] = phy_vars_eNB->proc[sched_subframe].frame_tx&0xfc;
      pbch_pdu[0] = 0;
    }

    /// First half of SSS (TDD)
    if (abstraction_flag==0) {

      if (phy_vars_eNB->lte_frame_parms.frame_type == TDD) {
        generate_sss(phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                     AMP,
                     &phy_vars_eNB->lte_frame_parms,
                     (phy_vars_eNB->lte_frame_parms.Ncp==NORMAL) ? 6 : 5,
                     1);
      }
    }




    if (abstraction_flag==0) {

      generate_pbch(&phy_vars_eNB->lte_eNB_pbch,
                    phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                    AMP,
                    &phy_vars_eNB->lte_frame_parms,
                    pbch_pdu,
                    phy_vars_eNB->proc[sched_subframe].frame_tx&3);

    }

#ifdef PHY_ABSTRACTION
    else {
      generate_pbch_emul(phy_vars_eNB,pbch_pdu);
    }

#endif


  }

  if (subframe == 1) {

    if (abstraction_flag==0) {

      if (phy_vars_eNB->lte_frame_parms.frame_type == TDD) {
        generate_pss(phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                     AMP,
                     &phy_vars_eNB->lte_frame_parms,
                     2,
                     2);
      }
    }
  }

  // Second half of PSS/SSS (FDD)
  if (subframe == 5) {

    if (abstraction_flag==0) {

      if (phy_vars_eNB->lte_frame_parms.frame_type == FDD) {
        generate_pss(phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                     AMP,
                     &phy_vars_eNB->lte_frame_parms,
                     (phy_vars_eNB->lte_frame_parms.Ncp==NORMAL) ? 6 : 5,
                     10);
        generate_sss(phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                     AMP,
                     &phy_vars_eNB->lte_frame_parms,
                     (phy_vars_eNB->lte_frame_parms.Ncp==NORMAL) ? 5 : 4,
                     10);

      }
    }
  }

  //  Second-half of SSS (TDD)
  if (subframe == 5) {
    if (abstraction_flag==0) {

      if (phy_vars_eNB->lte_frame_parms.frame_type == TDD) {
        generate_sss(phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                     AMP,
                     &phy_vars_eNB->lte_frame_parms,
                     (phy_vars_eNB->lte_frame_parms.Ncp==NORMAL) ? 6 : 5,
                     11);
      }
    }
  }

  // Second half of PSS (TDD)
  if (subframe == 6) {

    if (abstraction_flag==0) {

      if (phy_vars_eNB->lte_frame_parms.frame_type == TDD) {
        generate_pss(phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                     AMP,
                     &phy_vars_eNB->lte_frame_parms,
                     2,
                     12);
      }
    }
  }



#if defined(SMBV) && !defined(EXMIMO)

  // PBCH takes one allocation
  if (smbv_is_config_frame(phy_vars_eNB->proc[sched_subframe].frame_tx) && (smbv_frame_cnt < 4)) {
    if (subframe==0)
      smbv_alloc_cnt++;
  }

#endif

  if (phy_vars_eNB->mac_enabled==1) {
    // Parse DCI received from MAC
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PDCCH_TX,1);
    DCI_pdu = mac_xface->get_dci_sdu(phy_vars_eNB->Mod_id,
				     phy_vars_eNB->CC_id,
				     phy_vars_eNB->proc[sched_subframe].frame_tx,
				     subframe);
  }
  else {
    DCI_pdu = &DCI_pdu_tmp;
#ifdef EMOS_CHANNEL
    fill_dci_emos(DCI_pdu,sched_subframe,phy_vars_eNB);
#else
    fill_dci(DCI_pdu,sched_subframe,phy_vars_eNB);
#endif
  }

  // clear existing ulsch dci allocations before applying info from MAC  (this is table
  ul_subframe = pdcch_alloc2ul_subframe(&phy_vars_eNB->lte_frame_parms,subframe);
  ul_frame = pdcch_alloc2ul_frame(&phy_vars_eNB->lte_frame_parms,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe);

  if ((subframe_select(&phy_vars_eNB->lte_frame_parms,ul_subframe)==SF_UL) ||
      (phy_vars_eNB->lte_frame_parms.frame_type == FDD)) {
    harq_pid = subframe2harq_pid(&phy_vars_eNB->lte_frame_parms,ul_frame,ul_subframe);

    for (i=0; i<NUMBER_OF_UE_MAX; i++)
      if (phy_vars_eNB->ulsch_eNB[i]) {
        phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->dci_alloc=0;
        phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->rar_alloc=0;
      }
  }

  // clear previous allocation information for all UEs
  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    phy_vars_eNB->dlsch_eNB[i][0]->subframe_tx[subframe] = 0;
  }


  num_pdcch_symbols = DCI_pdu->num_pdcch_symbols;

  LOG_D(PHY,"num_pdcch_symbols %"PRIu8",(dci common %"PRIu8", dci uespec %"PRIu8"\n",num_pdcch_symbols,
        DCI_pdu->Num_common_dci,DCI_pdu->Num_ue_spec_dci);

#if defined(SMBV) && !defined(EXMIMO)

  // Sets up PDCCH and DCI table
  if (smbv_is_config_frame(phy_vars_eNB->proc[sched_subframe].frame_tx) && (smbv_frame_cnt < 4) && ((DCI_pdu->Num_common_dci+DCI_pdu->Num_ue_spec_dci)>0)) {
    msg("[SMBV] Frame %3d, SF %d PDCCH, number of DCIs %d\n",phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,DCI_pdu->Num_common_dci+DCI_pdu->Num_ue_spec_dci);
    dump_dci(&phy_vars_eNB->lte_frame_parms,&DCI_pdu->dci_alloc[0]);
    smbv_configure_pdcch(smbv_fname,(smbv_frame_cnt*10) + (subframe),num_pdcch_symbols,DCI_pdu->Num_common_dci+DCI_pdu->Num_ue_spec_dci);
  }

#endif



  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DCI_INFO,DCI_pdu->num_pdcch_symbols);

  for (i=0; i<DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci ; i++) {
    LOG_D(PHY,"[eNB] Subframe %d: DCI %d/%d : rnti %x, CCEind %d\n",subframe,i,DCI_pdu->Num_common_dci+DCI_pdu->Num_ue_spec_dci,DCI_pdu->dci_alloc[i].rnti,DCI_pdu->dci_alloc[i].firstCCE);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DCI_INFO,DCI_pdu->dci_alloc[i].rnti);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DCI_INFO,DCI_pdu->dci_alloc[i].format);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DCI_INFO,DCI_pdu->dci_alloc[i].firstCCE);

    if (DCI_pdu->dci_alloc[i].rnti == SI_RNTI) {

      generate_eNB_dlsch_params_from_dci(frame,
					 subframe,
                                         &DCI_pdu->dci_alloc[i].dci_pdu[0],
                                         DCI_pdu->dci_alloc[i].rnti,
                                         DCI_pdu->dci_alloc[i].format,
                                         &phy_vars_eNB->dlsch_eNB_SI,
                                         &phy_vars_eNB->lte_frame_parms,
                                         phy_vars_eNB->pdsch_config_dedicated,
                                         SI_RNTI,
                                         0,
                                         P_RNTI,
                                         phy_vars_eNB->eNB_UE_stats[0].DL_pmi_single);


      phy_vars_eNB->dlsch_eNB_SI->nCCE[subframe] = DCI_pdu->dci_alloc[i].firstCCE;

      LOG_T(PHY,"[eNB %"PRIu8"] Frame %d subframe %d : CCE resource for common DCI (SI)  => %"PRIu8"/%u\n",phy_vars_eNB->Mod_id,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,
	    phy_vars_eNB->dlsch_eNB_SI->nCCE[subframe],DCI_pdu->dci_alloc[i].firstCCE);
      
#if defined(SMBV) && !defined(EXMIMO)

      // configure SI DCI
      if (smbv_is_config_frame(phy_vars_eNB->proc[sched_subframe].frame_tx) && (smbv_frame_cnt < 4)) {
	msg("[SMBV] Frame %3d, SI in SF %d DCI %"PRIu32"\n",phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,i);
	smbv_configure_common_dci(smbv_fname,(smbv_frame_cnt*10) + (subframe), "SI", &DCI_pdu->dci_alloc[i], i);
      }
      
#endif
      

    } else if (DCI_pdu->dci_alloc[i].ra_flag == 1) {

      generate_eNB_dlsch_params_from_dci(frame,
					 subframe,
                                         &DCI_pdu->dci_alloc[i].dci_pdu[0],
                                         DCI_pdu->dci_alloc[i].rnti,
                                         DCI_pdu->dci_alloc[i].format,
                                         &phy_vars_eNB->dlsch_eNB_ra,
                                         &phy_vars_eNB->lte_frame_parms,
                                         phy_vars_eNB->pdsch_config_dedicated,
                                         SI_RNTI,
                                         DCI_pdu->dci_alloc[i].rnti,
                                         P_RNTI,
                                         phy_vars_eNB->eNB_UE_stats[0].DL_pmi_single);


      phy_vars_eNB->dlsch_eNB_ra->nCCE[subframe] = DCI_pdu->dci_alloc[i].firstCCE;

      LOG_D(PHY,"[eNB %"PRIu8"] Frame %d subframe %d : CCE resource for common DCI (RA)  => %"PRIu8"/%u (num_pdcch_symbols %d)\n",phy_vars_eNB->Mod_id,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,
	    phy_vars_eNB->dlsch_eNB_ra->nCCE[subframe],get_nCCE_mac(phy_vars_eNB->Mod_id,phy_vars_eNB->CC_id,num_pdcch_symbols,subframe),num_pdcch_symbols);
#if defined(SMBV) && !defined(EXMIMO)

      // configure RA DCI
      if (smbv_is_config_frame(phy_vars_eNB->proc[sched_subframe].frame_tx) && (smbv_frame_cnt < 4)) {
	msg("[SMBV] Frame %3d, RA in SF %d DCI %"PRIu32"\n",phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,i);
	smbv_configure_common_dci(smbv_fname,(smbv_frame_cnt*10) + (subframe), "RA", &DCI_pdu->dci_alloc[i], i);
      }

#endif

    }

    else if (DCI_pdu->dci_alloc[i].format != format0) { // this is a normal DLSCH allocation

      if (phy_vars_eNB->mac_enabled==1)
	UE_id = find_ue((int16_t)DCI_pdu->dci_alloc[i].rnti,phy_vars_eNB);
      else
	UE_id = i;

      if (UE_id>=0) {
	if ((frame%100)==0) {
	  LOG_D(PHY,"Frame %3d, SF %d \n",frame,subframe); 
	  dump_dci(&phy_vars_eNB->lte_frame_parms,&DCI_pdu->dci_alloc[i]);
	}
#if defined(SMBV) && !defined(EXMIMO)
        // Configure this user
        if (smbv_is_config_frame(phy_vars_eNB->proc[sched_subframe].frame_tx) && (smbv_frame_cnt < 4)) {
          msg("[SMBV] Frame %3d, SF %d (SMBV SF %d) Configuring user %d with RNTI %"PRIu16" in TM%"PRIu8"\n",phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,(smbv_frame_cnt*10) + (subframe),UE_id+1,
              DCI_pdu->dci_alloc[i].rnti,phy_vars_eNB->transmission_mode[(uint8_t)UE_id]);
          smbv_configure_user(smbv_fname,UE_id+1,phy_vars_eNB->transmission_mode[(uint8_t)UE_id],DCI_pdu->dci_alloc[i].rnti);
        }

#endif

        generate_eNB_dlsch_params_from_dci(frame,
					   subframe,
                                           &DCI_pdu->dci_alloc[i].dci_pdu[0],
                                           DCI_pdu->dci_alloc[i].rnti,
                                           DCI_pdu->dci_alloc[i].format,
                                           phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id],
                                           &phy_vars_eNB->lte_frame_parms,
                                           phy_vars_eNB->pdsch_config_dedicated,
                                           SI_RNTI,
                                           0,
                                           P_RNTI,
                                           phy_vars_eNB->eNB_UE_stats[(uint8_t)UE_id].DL_pmi_single);
        LOG_D(PHY,"[eNB %"PRIu8"][PDSCH %"PRIx16"/%"PRIu8"] Frame %d subframe %d: Generated dlsch params\n",
              phy_vars_eNB->Mod_id,DCI_pdu->dci_alloc[i].rnti,phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->current_harq_pid,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe);


        T(T_ENB_PHY_DLSCH_UE_DCI, T_INT(phy_vars_eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(UE_id),
          T_INT(DCI_pdu->dci_alloc[i].rnti), T_INT(DCI_pdu->dci_alloc[i].format),
          T_INT(phy_vars_eNB->dlsch_eNB[(int)UE_id][0]->current_harq_pid));

        phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->nCCE[subframe] = DCI_pdu->dci_alloc[i].firstCCE;

	LOG_D(PHY,"[eNB %"PRIu8"] Frame %d subframe %d : CCE resource for ue DCI (PDSCH %"PRIx16")  => %"PRIu8"/%u\n",phy_vars_eNB->Mod_id,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,
	      DCI_pdu->dci_alloc[i].rnti,phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->nCCE[subframe],DCI_pdu->dci_alloc[i].firstCCE);

#if defined(SMBV) && !defined(EXMIMO)
	
	// configure UE-spec DCI
	if (smbv_is_config_frame(phy_vars_eNB->proc[sched_subframe].frame_tx) && (smbv_frame_cnt < 4)) {
	  msg("[SMBV] Frame %3d, PDSCH in SF %d DCI %"PRIu32"\n",phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,i);
	  smbv_configure_ue_spec_dci(smbv_fname,(smbv_frame_cnt*10) + (subframe), UE_id+1, &DCI_pdu->dci_alloc[i], i);
	}

#endif

        LOG_D(PHY,"[eNB %"PRIu8"][DCI][PDSCH %"PRIx16"] Frame %d subframe %d UE_id %"PRId8" Generated DCI format %d, aggregation %d\n",
              phy_vars_eNB->Mod_id, DCI_pdu->dci_alloc[i].rnti,
              phy_vars_eNB->proc[sched_subframe].frame_tx, subframe,UE_id,
              DCI_pdu->dci_alloc[i].format,
              1<<DCI_pdu->dci_alloc[i].L);
      } else {
        LOG_D(PHY,"[eNB %"PRIu8"][PDSCH] Frame %d : No UE_id with corresponding rnti %"PRIx16", dropping DLSCH\n",
              phy_vars_eNB->Mod_id,phy_vars_eNB->proc[sched_subframe].frame_tx,DCI_pdu->dci_alloc[i].rnti);
      }
    }

  }

  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DCI_INFO,(frame*10)+subframe);

  // Apply physicalConfigDedicated if needed
  phy_config_dedicated_eNB_step2(phy_vars_eNB);

  for (i=0; i<DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci ; i++) {
    if (DCI_pdu->dci_alloc[i].format == format0) {  // this is a ULSCH allocation

      harq_pid = subframe2harq_pid(&phy_vars_eNB->lte_frame_parms,
                                   pdcch_alloc2ul_frame(&phy_vars_eNB->lte_frame_parms,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe),
                                   pdcch_alloc2ul_subframe(&phy_vars_eNB->lte_frame_parms,subframe));

      if (harq_pid==255) {
        LOG_E(PHY,"[eNB %"PRIu8"] Frame %d: Bad harq_pid for ULSCH allocation\n",phy_vars_eNB->Mod_id,phy_vars_eNB->proc[sched_subframe].frame_tx);
        return; 
      }

      if (phy_vars_eNB->mac_enabled==1)
	UE_id = find_ue((int16_t)DCI_pdu->dci_alloc[i].rnti,phy_vars_eNB);
      else
	UE_id = i;

      T(T_ENB_PHY_ULSCH_UE_DCI, T_INT(phy_vars_eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(UE_id),
        T_INT(DCI_pdu->dci_alloc[i].rnti), T_INT(harq_pid));

      if (UE_id<0) {
        LOG_E(PHY,"[eNB %"PRIu8"] Frame %d: Unknown UE_id for rnti %"PRIx16"\n",phy_vars_eNB->Mod_id,phy_vars_eNB->proc[sched_subframe].frame_tx,DCI_pdu->dci_alloc[i].rnti);
        mac_exit_wrapper("Invalid UE id (< 0) detected");
        return; // not reached
      }

      LOG_D(PHY,
            "[eNB %"PRIu8"][PUSCH %"PRIu8"] Frame %d subframe %d UL Frame %"PRIu32", UL Subframe %"PRIu8", Generated ULSCH (format0) DCI (rnti %"PRIx16", dci %"PRIx8") (DCI pos %"PRIu32"/%d), aggregation %d\n",
            phy_vars_eNB->Mod_id,
            subframe2harq_pid(&phy_vars_eNB->lte_frame_parms,
                              pdcch_alloc2ul_frame(&phy_vars_eNB->lte_frame_parms,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe),
                              pdcch_alloc2ul_subframe(&phy_vars_eNB->lte_frame_parms,subframe)),
            phy_vars_eNB->proc[sched_subframe].frame_tx,
            subframe,
            pdcch_alloc2ul_frame(&phy_vars_eNB->lte_frame_parms,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe),
            pdcch_alloc2ul_subframe(&phy_vars_eNB->lte_frame_parms,subframe),
            DCI_pdu->dci_alloc[i].rnti,
            DCI_pdu->dci_alloc[i].dci_pdu[0],
            i,
            DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci,
            1<<DCI_pdu->dci_alloc[i].L);

      generate_eNB_ulsch_params_from_dci(&DCI_pdu->dci_alloc[i].dci_pdu[0],
                                         DCI_pdu->dci_alloc[i].rnti,
                                         sched_subframe,
                                         format0,
                                         UE_id,
                                         phy_vars_eNB,
                                         SI_RNTI,
                                         0,
                                         P_RNTI,
                                         CBA_RNTI,
                                         0);  // do_srs

      LOG_T(PHY,"[eNB %"PRIu8"] Frame %d subframe %d : CCE resources for UE spec DCI (PUSCH %"PRIx16") => %d/%u\n",
	    phy_vars_eNB->Mod_id,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,DCI_pdu->dci_alloc[i].rnti,
	    DCI_pdu->dci_alloc[i].firstCCE,DCI_pdu->dci_alloc[i].firstCCE);
      
#if defined(SMBV) && !defined(EXMIMO)

        // configure UE-spec DCI for UL Grant
      if (smbv_is_config_frame(phy_vars_eNB->proc[sched_subframe].frame_tx) && (smbv_frame_cnt < 4)) {
	msg("[SMBV] Frame %3d, SF %d UL DCI %"PRIu32"\n",phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,i);
	smbv_configure_ue_spec_dci(smbv_fname,(smbv_frame_cnt*10) + (subframe), UE_id+1, &DCI_pdu->dci_alloc[i], i);
      }
      
#endif

      

      if ((DCI_pdu->dci_alloc[i].rnti  >= CBA_RNTI) && (DCI_pdu->dci_alloc[i].rnti < P_RNTI))
        phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->harq_processes[harq_pid]->subframe_cba_scheduling_flag = 1;
      else
        phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->harq_processes[harq_pid]->subframe_scheduling_flag = 1;

    }
  }





  // if we have DCI to generate do it now
  if ((DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci)>0) {


  } else { // for emulation!!
    phy_vars_eNB->num_ue_spec_dci[(subframe)&1]=0;
    phy_vars_eNB->num_common_dci[(subframe)&1]=0;
  }

  if (abstraction_flag == 0) {

    if (DCI_pdu->Num_ue_spec_dci+DCI_pdu->Num_common_dci > 0)
      LOG_D(PHY,"[eNB %"PRIu8"] Frame %d, subframe %d: Calling generate_dci_top (pdcch) (common %"PRIu8",ue_spec %"PRIu8")\n",phy_vars_eNB->Mod_id,phy_vars_eNB->proc[sched_subframe].frame_tx, subframe,
            DCI_pdu->Num_common_dci,DCI_pdu->Num_ue_spec_dci);

    num_pdcch_symbols = generate_dci_top(DCI_pdu->Num_ue_spec_dci,
                                         DCI_pdu->Num_common_dci,
                                         DCI_pdu->dci_alloc,
                                         0,
                                         AMP,
                                         &phy_vars_eNB->lte_frame_parms,
                                         phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                                         subframe);

  }

#ifdef PHY_ABSTRACTION // FIXME this ifdef seems suspicious
  else {
    LOG_D(PHY,"[eNB %"PRIu8"] Frame %d, subframe %d: Calling generate_dci_top_emul\n",phy_vars_eNB->Mod_id,phy_vars_eNB->proc[sched_subframe].frame_tx, subframe);
    num_pdcch_symbols = generate_dci_top_emul(phy_vars_eNB,DCI_pdu->Num_ue_spec_dci,DCI_pdu->Num_common_dci,DCI_pdu->dci_alloc,subframe);
  }

#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PDCCH_TX,0);

  // Check for SI activity

  if (phy_vars_eNB->dlsch_eNB_SI->active == 1) {
    input_buffer_length = phy_vars_eNB->dlsch_eNB_SI->harq_processes[0]->TBS/8;


    if (phy_vars_eNB->mac_enabled==1) {
      DLSCH_pdu = mac_xface->get_dlsch_sdu(phy_vars_eNB->Mod_id,
					   phy_vars_eNB->CC_id,
					   phy_vars_eNB->proc[sched_subframe].frame_tx,
					   SI_RNTI,
					   0);
    }
    else {
      DLSCH_pdu = DLSCH_pdu_tmp;

      for (i=0; i<input_buffer_length; i++)
	DLSCH_pdu[i] = (unsigned char)(taus()&0xff);
    }

#if defined(SMBV) && !defined(EXMIMO)

    // Configures the data source of allocation (allocation is configured by DCI)
    if (smbv_is_config_frame(phy_vars_eNB->proc[sched_subframe].frame_tx) && (smbv_frame_cnt < 4)) {
      msg("[SMBV] Frame %3d, Configuring SI payload in SF %d alloc %"PRIu8"\n",phy_vars_eNB->proc[sched_subframe].frame_tx,(smbv_frame_cnt*10) + (subframe),smbv_alloc_cnt);
      smbv_configure_datalist_for_alloc(smbv_fname, smbv_alloc_cnt++, (smbv_frame_cnt*10) + (subframe), DLSCH_pdu, input_buffer_length);
    }

#endif

    if (abstraction_flag == 0) {

      start_meas(&phy_vars_eNB->dlsch_encoding_stats);
      dlsch_encoding(DLSCH_pdu,
                     &phy_vars_eNB->lte_frame_parms,
                     num_pdcch_symbols,
                     phy_vars_eNB->dlsch_eNB_SI,
                     phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,
                     &phy_vars_eNB->dlsch_rate_matching_stats,
                     &phy_vars_eNB->dlsch_turbo_encoding_stats,
                     &phy_vars_eNB->dlsch_interleaving_stats);
      stop_meas(&phy_vars_eNB->dlsch_encoding_stats);

      start_meas(&phy_vars_eNB->dlsch_scrambling_stats);
      dlsch_scrambling(&phy_vars_eNB->lte_frame_parms,
                       0,
                       phy_vars_eNB->dlsch_eNB_SI,
                       get_G(&phy_vars_eNB->lte_frame_parms,
                             phy_vars_eNB->dlsch_eNB_SI->harq_processes[0]->nb_rb,
                             phy_vars_eNB->dlsch_eNB_SI->harq_processes[0]->rb_alloc,
                             get_Qm(phy_vars_eNB->dlsch_eNB_SI->harq_processes[0]->mcs),
                             1,
                             num_pdcch_symbols,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe),
                       0,
                       subframe<<1);

      stop_meas(&phy_vars_eNB->dlsch_scrambling_stats);

      start_meas(&phy_vars_eNB->dlsch_modulation_stats);

      re_allocated = dlsch_modulation(phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                                      AMP,
                                      subframe,
                                      &phy_vars_eNB->lte_frame_parms,
                                      num_pdcch_symbols,
                                      phy_vars_eNB->dlsch_eNB_SI,
                                      (LTE_eNB_DLSCH_t *)NULL);
      stop_meas(&phy_vars_eNB->dlsch_modulation_stats);
    }

#ifdef PHY_ABSTRACTION
    else {
      start_meas(&phy_vars_eNB->dlsch_encoding_stats);
      dlsch_encoding_emul(phy_vars_eNB,
                          DLSCH_pdu,
                          phy_vars_eNB->dlsch_eNB_SI);
      stop_meas(&phy_vars_eNB->dlsch_encoding_stats);
    }

#endif
    phy_vars_eNB->dlsch_eNB_SI->active = 0;

  }

  // Check for RA activity
  if (phy_vars_eNB->dlsch_eNB_ra->active == 1) {

    input_buffer_length = phy_vars_eNB->dlsch_eNB_ra->harq_processes[0]->TBS/8;

    int16_t crnti = mac_xface->fill_rar(phy_vars_eNB->Mod_id,
                                        phy_vars_eNB->CC_id,
                                        phy_vars_eNB->proc[sched_subframe].frame_tx,
                                        dlsch_input_buffer,
                                        phy_vars_eNB->lte_frame_parms.N_RB_UL,
                                        input_buffer_length);
    if (crnti!=0) 
      UE_id = add_ue(crnti,phy_vars_eNB);
    else 
      UE_id = -1;

    if (UE_id==-1) {
      LOG_W(PHY,"[eNB] Max user count reached.\n");
      mac_xface->cancel_ra_proc(phy_vars_eNB->Mod_id,
                                phy_vars_eNB->CC_id,
                                phy_vars_eNB->proc[sched_subframe].frame_tx,
                                crnti);
    } else {
      phy_vars_eNB->eNB_UE_stats[(uint32_t)UE_id].mode = RA_RESPONSE;
      // Initialize indicator for first SR (to be cleared after ConnectionSetup is acknowledged)
      phy_vars_eNB->first_sr[(uint32_t)UE_id] = 1;

      generate_eNB_ulsch_params_from_rar(dlsch_input_buffer,
                                         phy_vars_eNB->proc[sched_subframe].frame_tx,
                                         (subframe),
                                         phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id],
                                         &phy_vars_eNB->lte_frame_parms);

      phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_active = 1;

      get_Msg3_alloc(&phy_vars_eNB->lte_frame_parms,
                     subframe,
                     phy_vars_eNB->proc[sched_subframe].frame_tx,
                     &phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_frame,
                     &phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_subframe);
      LOG_D(PHY,"[eNB][RAPROC] Frame %d subframe %d, Activated Msg3 demodulation for UE %"PRId8" in frame %"PRIu32", subframe %"PRIu8"\n",
            phy_vars_eNB->proc[sched_subframe].frame_tx,
            subframe,
            UE_id,
            phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_frame,
            phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_subframe);

#if defined(SMBV) && !defined(EXMIMO)

      // Configures the data source of allocation (allocation is configured by DCI)
      if (smbv_is_config_frame(phy_vars_eNB->proc[sched_subframe].frame_tx) && (smbv_frame_cnt < 4)) {
        msg("[SMBV] Frame %3d, Configuring RA payload in SF %d alloc %"PRIu8"\n",phy_vars_eNB->proc[sched_subframe].frame_tx,(smbv_frame_cnt*10) + (subframe),smbv_alloc_cnt);
        smbv_configure_datalist_for_alloc(smbv_fname, smbv_alloc_cnt++, (smbv_frame_cnt*10) + (subframe), dlsch_input_buffer, input_buffer_length);
      }

#endif


      LOG_D(PHY,"[eNB %"PRIu8"][RAPROC] Frame %d, subframe %d: Calling generate_dlsch (RA) with input size = %"PRIu16",Msg3 frame %"PRIu32", Msg3 subframe %"PRIu8"\n",
            phy_vars_eNB->Mod_id,
            phy_vars_eNB->proc[sched_subframe].frame_tx, subframe,input_buffer_length,
            phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_frame,
            phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_subframe);


      if (abstraction_flag == 0) {

        dlsch_encoding(dlsch_input_buffer,
                       &phy_vars_eNB->lte_frame_parms,
                       num_pdcch_symbols,
                       phy_vars_eNB->dlsch_eNB_ra,
                       phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,
                       &phy_vars_eNB->dlsch_rate_matching_stats,
                       &phy_vars_eNB->dlsch_turbo_encoding_stats,
                       &phy_vars_eNB->dlsch_interleaving_stats);

        //  phy_vars_eNB->dlsch_eNB_ra->rnti = RA_RNTI;
        dlsch_scrambling(&phy_vars_eNB->lte_frame_parms,
                         0,
                         phy_vars_eNB->dlsch_eNB_ra,
                         get_G(&phy_vars_eNB->lte_frame_parms,
                               phy_vars_eNB->dlsch_eNB_ra->harq_processes[0]->nb_rb,
                               phy_vars_eNB->dlsch_eNB_ra->harq_processes[0]->rb_alloc,
                               get_Qm(phy_vars_eNB->dlsch_eNB_ra->harq_processes[0]->mcs),
                               1,
                               num_pdcch_symbols,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe),
                         0,
                         subframe<<1);

        re_allocated = dlsch_modulation(phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                                        AMP,
                                        subframe,
                                        &phy_vars_eNB->lte_frame_parms,
                                        num_pdcch_symbols,
                                        phy_vars_eNB->dlsch_eNB_ra,
                                        (LTE_eNB_DLSCH_t *)NULL);
      }

#ifdef PHY_ABSTRACTION
      else {
        dlsch_encoding_emul(phy_vars_eNB,
                            dlsch_input_buffer,
                            phy_vars_eNB->dlsch_eNB_ra);
      }

#endif
      LOG_D(PHY,"[eNB %"PRIu8"][RAPROC] Frame %d subframe %d Deactivating DLSCH RA\n",phy_vars_eNB->Mod_id,
            phy_vars_eNB->proc[sched_subframe].frame_tx,subframe);

    } //max user count

    phy_vars_eNB->dlsch_eNB_ra->active = 0;
  }

  // Now scan UE specific DLSCH
  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++)
  {
    if ((phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0])&&
        (phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->rnti>0)&&
        (phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->active == 1)) {
      harq_pid = phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->current_harq_pid;
      input_buffer_length = phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->TBS/8;



      LOG_D(PHY,
            "[eNB %"PRIu8"][PDSCH %"PRIx16"/%"PRIu8"] Frame %d, subframe %d: Generating PDSCH/DLSCH with input size = %"PRIu16", G %d, nb_rb %"PRIu16", mcs %"PRIu8", pmi_alloc %"PRIx16", rv %"PRIu8" (round %"PRIu8")\n",
            phy_vars_eNB->Mod_id, phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->rnti,harq_pid,
            phy_vars_eNB->proc[sched_subframe].frame_tx, subframe, input_buffer_length,
            get_G(&phy_vars_eNB->lte_frame_parms,
                  phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->nb_rb,
                  phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->rb_alloc,
                  get_Qm(phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->mcs),
                  phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->Nl,
                  num_pdcch_symbols,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe),
            phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->nb_rb,
            phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->mcs,
            pmi2hex_2Ar1(phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->pmi_alloc),
            phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->rvidx,
            phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->round);

#if defined(MESSAGE_CHART_GENERATOR_PHY)
      MSC_LOG_TX_MESSAGE(
        MSC_PHY_ENB,MSC_PHY_UE,
        NULL,0,
        "%05u:%02u PDSCH/DLSCH input size = %"PRIu16", G %d, nb_rb %"PRIu16", mcs %"PRIu8", pmi_alloc %"PRIx16", rv %"PRIu8" (round %"PRIu8")",
        phy_vars_eNB->proc[sched_subframe].frame_tx, subframe,
        input_buffer_length,
        get_G(&phy_vars_eNB->lte_frame_parms,
        		phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->nb_rb,
        		phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->rb_alloc,
        		get_Qm(phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->mcs),
        		phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->Nl,
        		num_pdcch_symbols,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe),
        phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->nb_rb,
        phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->mcs,
        pmi2hex_2Ar1(phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->pmi_alloc),
        phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->rvidx,
        phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->round);
#endif

      phy_vars_eNB->eNB_UE_stats[(uint8_t)UE_id].dlsch_sliding_cnt++;

      if (phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->harq_processes[harq_pid]->round == 0) {

        phy_vars_eNB->eNB_UE_stats[(uint32_t)UE_id].dlsch_trials[harq_pid][0]++;

	if (phy_vars_eNB->mac_enabled==1) {
	  DLSCH_pdu = mac_xface->get_dlsch_sdu(phy_vars_eNB->Mod_id,
					       phy_vars_eNB->CC_id,
					       phy_vars_eNB->proc[sched_subframe].frame_tx,
					       phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->rnti,
					       0);
	  phy_vars_eNB->eNB_UE_stats[UE_id].total_TBS_MAC += phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->TBS;
	}
	else {
	  DLSCH_pdu = DLSCH_pdu_tmp;
	  
	  for (i=0; i<input_buffer_length; i++)
	    DLSCH_pdu[i] = (unsigned char)(taus()&0xff);
	}
	
#if defined(SMBV) && !defined(EXMIMO)

        // Configures the data source of allocation (allocation is configured by DCI)
        if (smbv_is_config_frame(phy_vars_eNB->proc[sched_subframe].frame_tx) && (smbv_frame_cnt < 4)) {
          msg("[SMBV] Frame %3d, Configuring PDSCH payload in SF %d alloc %"PRIu8"\n",phy_vars_eNB->proc[sched_subframe].frame_tx,(smbv_frame_cnt*10) + (subframe),smbv_alloc_cnt);
          smbv_configure_datalist_for_user(smbv_fname, UE_id+1, DLSCH_pdu, input_buffer_length);
        }

#endif


#ifdef DEBUG_PHY_PROC
#ifdef DEBUG_DLSCH
        LOG_T(PHY,"eNB DLSCH SDU: \n");

        for (i=0; i<phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->TBS>>3; i++)
          LOG_T(PHY,"%"PRIx8".",DLSCH_pdu[i]);

        LOG_T(PHY,"\n");
#endif
#endif
      } else {
        phy_vars_eNB->eNB_UE_stats[(uint32_t)UE_id].dlsch_trials[harq_pid][phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->round]++;
#ifdef DEBUG_PHY_PROC
#ifdef DEBUG_DLSCH
        LOG_D(PHY,"[eNB] This DLSCH is a retransmission\n");
#endif
#endif
      }

      if (abstraction_flag==0) {

        // 36-212
        start_meas(&phy_vars_eNB->dlsch_encoding_stats);
        dlsch_encoding(DLSCH_pdu,
                       &phy_vars_eNB->lte_frame_parms,
                       num_pdcch_symbols,
                       phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0],
                       phy_vars_eNB->proc[sched_subframe].frame_tx,subframe,
                       &phy_vars_eNB->dlsch_rate_matching_stats,
                       &phy_vars_eNB->dlsch_turbo_encoding_stats,
                       &phy_vars_eNB->dlsch_interleaving_stats);
        stop_meas(&phy_vars_eNB->dlsch_encoding_stats);
        // 36-211
        start_meas(&phy_vars_eNB->dlsch_scrambling_stats);
        dlsch_scrambling(&phy_vars_eNB->lte_frame_parms,
                         0,
                         phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0],
                         get_G(&phy_vars_eNB->lte_frame_parms,
                               phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->nb_rb,
                               phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->rb_alloc,
                               get_Qm(phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->mcs),
                               phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[harq_pid]->Nl,
                               num_pdcch_symbols,phy_vars_eNB->proc[sched_subframe].frame_tx,subframe),
                         0,
                         subframe<<1);
        stop_meas(&phy_vars_eNB->dlsch_scrambling_stats);
        start_meas(&phy_vars_eNB->dlsch_modulation_stats);


        re_allocated = dlsch_modulation(phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                                        AMP,
                                        subframe,
                                        &phy_vars_eNB->lte_frame_parms,
                                        num_pdcch_symbols,
                                        phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0],
                                        phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][1]);

        stop_meas(&phy_vars_eNB->dlsch_modulation_stats);
      }

#ifdef PHY_ABSTRACTION
      else {
        start_meas(&phy_vars_eNB->dlsch_encoding_stats);
        dlsch_encoding_emul(phy_vars_eNB,
                            DLSCH_pdu,
                            phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]);
        stop_meas(&phy_vars_eNB->dlsch_encoding_stats);
      }

#endif
      phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->active = 0;

    }

    else if ((phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0])&&
             (phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->rnti>0)&&
             (phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->active == 0)) {

      // clear subframe TX flag since UE is not scheduled for PDSCH in this subframe (so that we don't look for PUCCH later)
      phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->subframe_tx[subframe]=0;
    }
  }



  // if we have PHICH to generate

  if (is_phich_subframe(&phy_vars_eNB->lte_frame_parms,subframe))
  {
    generate_phich_top(phy_vars_eNB,
                       sched_subframe,
                       AMP,
                       0,
                       abstraction_flag);
  }



#ifdef EMOS
  phy_procedures_emos_eNB_TX(subframe, phy_vars_eNB);
#endif

#if !(defined(EXMIMO) || defined(OAI_USRP) || defined (CPRIGW))

  if (abstraction_flag==0)
  {
    start_meas(&phy_vars_eNB->ofdm_mod_stats);
    do_OFDM_mod(phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                phy_vars_eNB->lte_eNB_common_vars.txdata[0],
                phy_vars_eNB->proc[sched_subframe].frame_tx,subframe<<1,
                &phy_vars_eNB->lte_frame_parms);
    do_OFDM_mod(phy_vars_eNB->lte_eNB_common_vars.txdataF[0],
                phy_vars_eNB->lte_eNB_common_vars.txdata[0],
                phy_vars_eNB->proc[sched_subframe].frame_tx,1+(subframe<<1),
                &phy_vars_eNB->lte_frame_parms);
    stop_meas(&phy_vars_eNB->ofdm_mod_stats);
  }

#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_TX,0);
  stop_meas(&phy_vars_eNB->phy_proc_tx);


  (void)re_allocated; /* remove gcc warning "set but not used" */
}

void process_Msg3(PHY_VARS_eNB *phy_vars_eNB,uint8_t sched_subframe,uint8_t UE_id, uint8_t harq_pid)
{
  // this prepares the demodulation of the first PUSCH of a new user, containing Msg3

  int subframe = phy_vars_eNB->proc[sched_subframe].subframe_rx;
  int frame = phy_vars_eNB->proc[sched_subframe].frame_rx;

  LOG_D(PHY,"[eNB %d][RAPROC] frame %d : subframe %d : process_Msg3 UE_id %d (active %d, subframe %d, frame %d)\n",
        phy_vars_eNB->Mod_id,
        frame,subframe,
        UE_id,phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_active,
        phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_subframe,
        phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_frame);
  phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_flag = 0;

  if ((phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_active == 1) &&
      (phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_subframe == subframe) &&
      (phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_frame == (uint32_t)frame))   {

    //    harq_pid = 0;

    phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_active = 0;
    phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->Msg3_flag = 1;
    phy_vars_eNB->ulsch_eNB[(uint32_t)UE_id]->harq_processes[harq_pid]->subframe_scheduling_flag=1;
    LOG_D(PHY,"[eNB %d][RAPROC] frame %d, subframe %d: Setting subframe_scheduling_flag (Msg3) for UE %d\n",
          phy_vars_eNB->Mod_id,
          frame,subframe,UE_id);
  }
}


// This function retrieves the harq_pid of the corresponding DLSCH process
// and updates the error statistics of the DLSCH based on the received ACK
// info from UE along with the round index.  It also performs the fine-grain
// rate-adaptation based on the error statistics derived from the ACK/NAK process

void process_HARQ_feedback(uint8_t UE_id,
                           uint8_t sched_subframe,
                           PHY_VARS_eNB *phy_vars_eNB,
                           uint8_t pusch_flag,
                           uint8_t *pucch_payload,
                           uint8_t pucch_sel,
                           uint8_t SR_payload)
{

  uint8_t dl_harq_pid[8],dlsch_ACK[8],dl_subframe;
  LTE_eNB_DLSCH_t *dlsch             =  phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0];
  LTE_eNB_UE_stats *ue_stats         =  &phy_vars_eNB->eNB_UE_stats[(uint32_t)UE_id];
  LTE_DL_eNB_HARQ_t *dlsch_harq_proc;
  uint8_t subframe_m4,M,m;
  int mp;
  int all_ACKed=1,nb_alloc=0,nb_ACK=0;
  int frame = phy_vars_eNB->proc[sched_subframe].frame_rx;
  int subframe = phy_vars_eNB->proc[sched_subframe].subframe_rx;
  int harq_pid = subframe2harq_pid( &phy_vars_eNB->lte_frame_parms,frame,subframe);

  if (phy_vars_eNB->lte_frame_parms.frame_type == FDD) { //FDD
    subframe_m4 = (subframe<4) ? subframe+6 : subframe-4;

    dl_harq_pid[0] = dlsch->harq_ids[subframe_m4];
    M=1;

    if (pusch_flag == 1) {
      dlsch_ACK[0] = phy_vars_eNB->ulsch_eNB[(uint8_t)UE_id]->harq_processes[harq_pid]->o_ACK[0];
      if (dlsch->subframe_tx[subframe_m4]==1)
      LOG_D(PHY,"[eNB %d] Frame %d: Received ACK/NAK %d on PUSCH for subframe %d\n",phy_vars_eNB->Mod_id,
	    frame,dlsch_ACK[0],subframe_m4);
    }
    else {
      dlsch_ACK[0] = pucch_payload[0];
      LOG_D(PHY,"[eNB %d] Frame %d: Received ACK/NAK %d on PUCCH for subframe %d\n",phy_vars_eNB->Mod_id,
	    frame,dlsch_ACK[0],subframe_m4);
      /*
      if (dlsch_ACK[0]==0)
	AssertFatal(0,"Exiting on NAK on PUCCH\n");
      */
    }


#if defined(MESSAGE_CHART_GENERATOR_PHY)
    MSC_LOG_RX_MESSAGE(
      MSC_PHY_ENB,MSC_PHY_UE,
      NULL,0,
      "%05u:%02u %s received %s  rnti %x harq id %u  tx SF %u",
      frame,subframe,
      (pusch_flag == 1)?"PUSCH":"PUCCH",
      (dlsch_ACK[0])?"ACK":"NACK",
      dlsch->rnti,
      dl_harq_pid[0],
      subframe_m4
      );
#endif
  } else { // TDD Handle M=1,2 cases only

    M=ul_ACK_subframe2_M(&phy_vars_eNB->lte_frame_parms,
                         subframe);

    // Now derive ACK information for TDD
    if (pusch_flag == 1) { // Do PUSCH ACK/NAK first
      // detect missing DAI
      //FK: this code is just a guess
      //RK: not exactly, yes if scheduled from PHICH (i.e. no DCI format 0)
      //    otherwise, it depends on how many of the PDSCH in the set are scheduled, we can leave it like this,
      //    but we have to adapt the code below.  For example, if only one out of 2 are scheduled, only 1 bit o_ACK is used

      dlsch_ACK[0] = phy_vars_eNB->ulsch_eNB[(uint8_t)UE_id]->harq_processes[harq_pid]->o_ACK[0];
      dlsch_ACK[1] = (phy_vars_eNB->pucch_config_dedicated[UE_id].tdd_AckNackFeedbackMode == bundling)
                     ?phy_vars_eNB->ulsch_eNB[(uint8_t)UE_id]->harq_processes[harq_pid]->o_ACK[0]:phy_vars_eNB->ulsch_eNB[(uint8_t)UE_id]->harq_processes[harq_pid]->o_ACK[1];
      //      printf("UE %d: ACK %d,%d\n",UE_id,dlsch_ACK[0],dlsch_ACK[1]);
    }

    else {  // PUCCH ACK/NAK
      if ((SR_payload == 1)&&(pucch_sel!=2)) {  // decode Table 7.3 if multiplexing and SR=1
        nb_ACK = 0;

        if (M == 2) {
          if ((pucch_payload[0] == 1) && (pucch_payload[1] == 1)) // b[0],b[1]
            nb_ACK = 1;
          else if ((pucch_payload[0] == 1) && (pucch_payload[1] == 0))
            nb_ACK = 2;
        } else if (M == 3) {
          if ((pucch_payload[0] == 1) && (pucch_payload[1] == 1))
            nb_ACK = 1;
          else if ((pucch_payload[0] == 1) && (pucch_payload[1] == 0))
            nb_ACK = 2;
          else if ((pucch_payload[0] == 0) && (pucch_payload[1] == 1))
            nb_ACK = 3;
        }
      } else if (pucch_sel == 2) { // bundling or M=1
        //  printf("*** (%d,%d)\n",pucch_payload[0],pucch_payload[1]);
        dlsch_ACK[0] = pucch_payload[0];
        dlsch_ACK[1] = pucch_payload[0];
      } else { // multiplexing with no SR, this is table 10.1
        if (M==1)
          dlsch_ACK[0] = pucch_payload[0];
        else if (M==2) {
          if (((pucch_sel == 1) && (pucch_payload[0] == 1) && (pucch_payload[1] == 1)) ||
              ((pucch_sel == 0) && (pucch_payload[0] == 0) && (pucch_payload[1] == 1)))
            dlsch_ACK[0] = 1;
          else
            dlsch_ACK[0] = 0;

          if (((pucch_sel == 1) && (pucch_payload[0] == 1) && (pucch_payload[1] == 1)) ||
              ((pucch_sel == 1) && (pucch_payload[0] == 0) && (pucch_payload[1] == 0)))
            dlsch_ACK[1] = 1;
          else
            dlsch_ACK[1] = 0;
        }
      }
    }
  }

  // handle case where positive SR was transmitted with multiplexing
  if ((SR_payload == 1)&&(pucch_sel!=2)&&(pusch_flag == 0)) {
    nb_alloc = 0;

    for (m=0; m<M; m++) {
      dl_subframe = ul_ACK_subframe2_dl_subframe(&phy_vars_eNB->lte_frame_parms,
                    subframe,
                    m);

      if (dlsch->subframe_tx[dl_subframe]==1)
        nb_alloc++;
    }

    if (nb_alloc == nb_ACK)
      all_ACKed = 1;
    else
      all_ACKed = 0;

    //    printf("nb_alloc %d, all_ACKed %d\n",nb_alloc,all_ACKed);
  }


  for (m=0,mp=-1; m<M; m++) {

    dl_subframe = ul_ACK_subframe2_dl_subframe(&phy_vars_eNB->lte_frame_parms,
                  subframe,
                  m);

    if (dlsch->subframe_tx[dl_subframe]==1) {
      if (pusch_flag == 1)
        mp++;
      else
        mp = m;

      dl_harq_pid[m]     = dlsch->harq_ids[dl_subframe];

      if ((pucch_sel != 2)&&(pusch_flag == 0)) { // multiplexing
        if ((SR_payload == 1)&&(all_ACKed == 1))
          dlsch_ACK[m] = 1;
        else
          dlsch_ACK[m] = 0;
      }

      if (dl_harq_pid[m]<dlsch->Mdlharq) {
        dlsch_harq_proc = dlsch->harq_processes[dl_harq_pid[m]];
#ifdef DEBUG_PHY_PROC
        LOG_D(PHY,"[eNB %d][PDSCH %x/%d] subframe %d, status %d, round %d (mcs %d, rv %d, TBS %d)\n",phy_vars_eNB->Mod_id,
              dlsch->rnti,dl_harq_pid[m],dl_subframe,
              dlsch_harq_proc->status,dlsch_harq_proc->round,
              dlsch->harq_processes[dl_harq_pid[m]]->mcs,
              dlsch->harq_processes[dl_harq_pid[m]]->rvidx,
              dlsch->harq_processes[dl_harq_pid[m]]->TBS);

        if (dlsch_harq_proc->status==DISABLED)
          LOG_E(PHY,"dlsch_harq_proc is disabled? \n");

#endif

        if ((dl_harq_pid[m]<dlsch->Mdlharq) &&
            (dlsch_harq_proc->status == ACTIVE)) {
          // dl_harq_pid of DLSCH is still active

          //    msg("[PHY] eNB %d Process %d is active (%d)\n",phy_vars_eNB->Mod_id,dl_harq_pid[m],dlsch_ACK[m]);
          if ( dlsch_ACK[mp]==0) {
            // Received NAK
#ifdef DEBUG_PHY_PROC
            LOG_D(PHY,"[eNB %d][PDSCH %x/%d] M = %d, m= %d, mp=%d NAK Received in round %d, requesting retransmission\n",phy_vars_eNB->Mod_id,
                  dlsch->rnti,dl_harq_pid[m],M,m,mp,dlsch_harq_proc->round);
#endif

            T(T_ENB_PHY_DLSCH_UE_NACK, T_INT(phy_vars_eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(UE_id), T_INT(dlsch->rnti),
              T_INT(dl_harq_pid[m]));

            if (dlsch_harq_proc->round == 0)
              ue_stats->dlsch_NAK_round0++;

            ue_stats->dlsch_NAK[dl_harq_pid[m]][dlsch_harq_proc->round]++;


            // then Increment DLSCH round index
            dlsch_harq_proc->round++;


            if (dlsch_harq_proc->round == dlsch->Mlimit) {
              // This was the last round for DLSCH so reset round and increment l2_error counter
#ifdef DEBUG_PHY_PROC
              LOG_W(PHY,"[eNB %d][PDSCH %x/%d] DLSCH retransmissions exhausted, dropping packet\n",phy_vars_eNB->Mod_id,
                    dlsch->rnti,dl_harq_pid[m]);
#endif
#if defined(MESSAGE_CHART_GENERATOR_PHY)
              MSC_LOG_EVENT(MSC_PHY_ENB, "0 HARQ DLSCH Failed RNTI %"PRIx16" round %u",
                            dlsch->rnti,
                            dlsch_harq_proc->round);
#endif

              dlsch_harq_proc->round = 0;
              ue_stats->dlsch_l2_errors[dl_harq_pid[m]]++;
              dlsch_harq_proc->status = SCH_IDLE;
              dlsch->harq_ids[dl_subframe] = dlsch->Mdlharq;
            }
          } else {
#ifdef DEBUG_PHY_PROC
            LOG_D(PHY,"[eNB %d][PDSCH %x/%d] ACK Received in round %d, resetting process\n",phy_vars_eNB->Mod_id,
                  dlsch->rnti,dl_harq_pid[m],dlsch_harq_proc->round);
#endif

            T(T_ENB_PHY_DLSCH_UE_ACK, T_INT(phy_vars_eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(UE_id), T_INT(dlsch->rnti),
              T_INT(dl_harq_pid[m]));

            ue_stats->dlsch_ACK[dl_harq_pid[m]][dlsch_harq_proc->round]++;

            // Received ACK so set round to 0 and set dlsch_harq_pid IDLE
            dlsch_harq_proc->round  = 0;
            dlsch_harq_proc->status = SCH_IDLE;
            dlsch->harq_ids[dl_subframe] = dlsch->Mdlharq;

            ue_stats->total_TBS = ue_stats->total_TBS +
                                  phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[dl_harq_pid[m]]->TBS;
            /*
              ue_stats->total_transmitted_bits = ue_stats->total_transmitted_bits +
              phy_vars_eNB->dlsch_eNB[(uint8_t)UE_id][0]->harq_processes[dl_harq_pid[m]]->TBS;
            */
          }

          // Do fine-grain rate-adaptation for DLSCH
          if (ue_stats->dlsch_NAK_round0 > dlsch->error_threshold) {
            if (ue_stats->dlsch_mcs_offset == 1)
              ue_stats->dlsch_mcs_offset=0;
            else
              ue_stats->dlsch_mcs_offset=-1;
          }

#ifdef DEBUG_PHY_PROC
          LOG_D(PHY,"[process_HARQ_feedback] Frame %d Setting round to %d for pid %d (subframe %d)\n",frame,
                dlsch_harq_proc->round,dl_harq_pid[m],subframe);
#endif

          // Clear NAK stats and adjust mcs offset
          // after measurement window timer expires
          if (ue_stats->dlsch_sliding_cnt == dlsch->ra_window_size) {
            if ((ue_stats->dlsch_mcs_offset == 0) && (ue_stats->dlsch_NAK_round0 < 2))
              ue_stats->dlsch_mcs_offset = 1;

            if ((ue_stats->dlsch_mcs_offset == 1) && (ue_stats->dlsch_NAK_round0 > 2))
              ue_stats->dlsch_mcs_offset = 0;

            if ((ue_stats->dlsch_mcs_offset == 0) && (ue_stats->dlsch_NAK_round0 > 2))
              ue_stats->dlsch_mcs_offset = -1;

            if ((ue_stats->dlsch_mcs_offset == -1) && (ue_stats->dlsch_NAK_round0 < 2))
              ue_stats->dlsch_mcs_offset = 0;

            ue_stats->dlsch_NAK_round0 = 0;
            ue_stats->dlsch_sliding_cnt = 0;
          }


        }
      }
    }
  }
}

void get_n1_pucch_eNB(PHY_VARS_eNB *phy_vars_eNB,
                      uint8_t UE_id,
                      uint8_t sched_subframe,
                      int16_t *n1_pucch0,
                      int16_t *n1_pucch1,
                      int16_t *n1_pucch2,
                      int16_t *n1_pucch3)
{

  LTE_DL_FRAME_PARMS *frame_parms=&phy_vars_eNB->lte_frame_parms;
  uint8_t nCCE0,nCCE1;
  int sf;
  int frame = phy_vars_eNB->proc[sched_subframe].frame_rx;
  int subframe = phy_vars_eNB->proc[sched_subframe].subframe_rx;

  if (frame_parms->frame_type == FDD ) {
    sf = (subframe<4) ? (subframe+6) : (subframe-4);
    //    printf("n1_pucch_eNB: subframe %d, nCCE %d\n",sf,phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->nCCE[sf]);

    if (phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->subframe_tx[sf]>0) {
      *n1_pucch0 = frame_parms->pucch_config_common.n1PUCCH_AN + phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->nCCE[sf];
      *n1_pucch1 = -1;
    } else {
      *n1_pucch0 = -1;
      *n1_pucch1 = -1;
    }
  } else {

    switch (frame_parms->tdd_config) {
    case 1:  // DL:S:UL:UL:DL:DL:S:UL:UL:DL
      if (subframe == 2) {  // ACK subframes 5 and 6
        /*  if (phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->subframe_tx[6]>0) {
          nCCE1 = phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->nCCE[6];
          *n1_pucch1 = get_Np(frame_parms->N_RB_DL,nCCE1,1) + nCCE1 + frame_parms->pucch_config_common.n1PUCCH_AN;
          }
          else
          *n1_pucch1 = -1;*/

        if (phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->subframe_tx[5]>0) {
          nCCE0 = phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->nCCE[5];
          *n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0+ frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch0 = -1;

        *n1_pucch1 = -1;
      } else if (subframe == 3) { // ACK subframe 9

        if (phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->subframe_tx[9]>0) {
          nCCE0 = phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->nCCE[9];
          *n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0 +frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch0 = -1;

        *n1_pucch1 = -1;

      } else if (subframe == 7) { // ACK subframes 0 and 1
        //harq_ack[0].nCCE;
        //harq_ack[1].nCCE;
        if (phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->subframe_tx[0]>0) {
          nCCE0 = phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->nCCE[0];
          *n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0 + frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch0 = -1;

        *n1_pucch1 = -1;
      } else if (subframe == 8) { // ACK subframes 4
        //harq_ack[4].nCCE;
        if (phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->subframe_tx[4]>0) {
          nCCE0 = phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->nCCE[4];
          *n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0 + frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch0 = -1;

        *n1_pucch1 = -1;
      } else {
        LOG_D(PHY,"[eNB %d] frame %d: phy_procedures_lte.c: get_n1pucch, illegal subframe %d for tdd_config %d\n",
              phy_vars_eNB->Mod_id,
              frame,
              subframe,frame_parms->tdd_config);
        return;
      }

      break;

    case 3:  // DL:S:UL:UL:UL:DL:DL:DL:DL:DL
      if (subframe == 2) {  // ACK subframes 5,6 and 1 (S in frame-2), forget about n-11 for the moment (S-subframe)
        if (phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->subframe_tx[6]>0) {
          nCCE1 = phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->nCCE[6];
          *n1_pucch1 = get_Np(frame_parms->N_RB_DL,nCCE1,1) + nCCE1 + frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch1 = -1;

        if (phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->subframe_tx[5]>0) {
          nCCE0 = phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->nCCE[5];
          *n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0+ frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch0 = -1;
      } else if (subframe == 3) { // ACK subframes 7 and 8
        LOG_D(PHY,"get_n1_pucch_eNB : subframe 3, subframe_tx[7] %d, subframe_tx[8] %d\n",
              phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->subframe_tx[7],phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->subframe_tx[8]);

        if (phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->subframe_tx[8]>0) {
          nCCE1 = phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->nCCE[8];
          *n1_pucch1 = get_Np(frame_parms->N_RB_DL,nCCE1,1) + nCCE1 + frame_parms->pucch_config_common.n1PUCCH_AN;
          LOG_D(PHY,"nCCE1 %d, n1_pucch1 %d\n",nCCE1,*n1_pucch1);
        } else
          *n1_pucch1 = -1;

        if (phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->subframe_tx[7]>0) {
          nCCE0 = phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->nCCE[7];
          *n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0 +frame_parms->pucch_config_common.n1PUCCH_AN;
          LOG_D(PHY,"nCCE0 %d, n1_pucch0 %d\n",nCCE0,*n1_pucch0);
        } else
          *n1_pucch0 = -1;
      } else if (subframe == 4) { // ACK subframes 9 and 0
        if (phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->subframe_tx[0]>0) {
          nCCE1 = phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->nCCE[0];
          *n1_pucch1 = get_Np(frame_parms->N_RB_DL,nCCE1,1) + nCCE1 + frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch1 = -1;

        if (phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->subframe_tx[9]>0) {
          nCCE0 = phy_vars_eNB->dlsch_eNB[(uint32_t)UE_id][0]->nCCE[9];
          *n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0 +frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch0 = -1;
      } else {
        LOG_D(PHY,"[eNB %d] Frame %d: phy_procedures_lte.c: get_n1pucch, illegal subframe %d for tdd_config %d\n",
              phy_vars_eNB->Mod_id,frame,subframe,frame_parms->tdd_config);
        return;
      }

      break;
    }  // switch tdd_config

    // Don't handle the case M>2
    *n1_pucch2 = -1;
    *n1_pucch3 = -1;
  }
}

void prach_procedures(PHY_VARS_eNB *phy_vars_eNB,uint8_t sched_subframe,uint8_t abstraction_flag)
{

  uint16_t preamble_energy_list[64],preamble_delay_list[64];
  uint16_t preamble_max,preamble_energy_max;
  uint16_t i;
  int8_t UE_id;
  int subframe = phy_vars_eNB->proc[sched_subframe].subframe_rx;
  int frame = phy_vars_eNB->proc[sched_subframe].frame_rx;
  uint8_t CC_id = phy_vars_eNB->CC_id;

  memset(&preamble_energy_list[0],0,64*sizeof(uint16_t));
  memset(&preamble_delay_list[0],0,64*sizeof(uint16_t));

  if (abstraction_flag == 0) {
    LOG_D(PHY,"[eNB %d][RAPROC] Frame %d, Subframe %d : PRACH RX Signal Power : %d dBm\n",phy_vars_eNB->Mod_id, 
          frame,subframe,dB_fixed(signal_energy(&phy_vars_eNB->lte_eNB_common_vars.rxdata[0][0][subframe*phy_vars_eNB->lte_frame_parms.samples_per_tti],512)) - phy_vars_eNB->rx_total_gain_eNB_dB);


    rx_prach(phy_vars_eNB,
             subframe,
             preamble_energy_list,
             preamble_delay_list,
             frame,
             0);
  } else {
    for (UE_id=0; UE_id<NB_UE_INST; UE_id++) {

      LOG_D(PHY,"[RAPROC] UE_id %d (%p), generate_prach %d, UE RSI %d, eNB RSI %d preamble index %d\n",
            UE_id,PHY_vars_UE_g[UE_id][CC_id],PHY_vars_UE_g[UE_id][CC_id]->generate_prach,
            PHY_vars_UE_g[UE_id][CC_id]->lte_frame_parms.prach_config_common.rootSequenceIndex,
            phy_vars_eNB->lte_frame_parms.prach_config_common.rootSequenceIndex,
            PHY_vars_UE_g[UE_id][CC_id]->prach_PreambleIndex);

      if ((PHY_vars_UE_g[UE_id][CC_id]->generate_prach==1) &&
          (PHY_vars_UE_g[UE_id][CC_id]->lte_frame_parms.prach_config_common.rootSequenceIndex ==
           phy_vars_eNB->lte_frame_parms.prach_config_common.rootSequenceIndex) ) {
        preamble_energy_list[PHY_vars_UE_g[UE_id][CC_id]->prach_PreambleIndex] = 800;
        preamble_delay_list[PHY_vars_UE_g[UE_id][CC_id]->prach_PreambleIndex] = 5;

      }
    }
  }

  preamble_energy_max = preamble_energy_list[0];
  preamble_max = 0;

  for (i=1; i<64; i++) {
    if (preamble_energy_max < preamble_energy_list[i]) {
      preamble_energy_max = preamble_energy_list[i];
      preamble_max = i;
    }
  }

#ifdef DEBUG_PHY_PROC
  LOG_D(PHY,"[RAPROC] Most likely preamble %d, energy %d dB delay %d\n",
        preamble_max,
        preamble_energy_list[preamble_max],
        preamble_delay_list[preamble_max]);
#endif

  if (preamble_energy_list[preamble_max] > 580) {

    UE_id = find_next_ue_index(phy_vars_eNB);
 
    if (UE_id>=0) {
      phy_vars_eNB->eNB_UE_stats[(uint32_t)UE_id].UE_timing_offset = preamble_delay_list[preamble_max]&0x1FFF; //limit to 13 (=11+2) bits

      phy_vars_eNB->eNB_UE_stats[(uint32_t)UE_id].sector = 0;
      LOG_D(PHY,"[eNB %d/%d][RAPROC] Frame %d, subframe %d Initiating RA procedure (UE_id %d) with preamble %d, energy %d.%d dB, delay %d\n",
            phy_vars_eNB->Mod_id,
            phy_vars_eNB->CC_id,
            frame,
            subframe,
	    UE_id,
            preamble_max,
            preamble_energy_max/10,
            preamble_energy_max%10,
            preamble_delay_list[preamble_max]);

      if (phy_vars_eNB->mac_enabled==1) {
        uint8_t update_TA=4;

        switch (phy_vars_eNB->lte_frame_parms.N_RB_DL) {
        case 6:
          update_TA = 16;
          break;

        case 25:
          update_TA = 4;
          break;

        case 50:
          update_TA = 2;
          break;

        case 100:
          update_TA = 1;
          break;
        }

      mac_xface->initiate_ra_proc(phy_vars_eNB->Mod_id,
                                  phy_vars_eNB->CC_id,
                                  frame,
                                  preamble_max,
                                  preamble_delay_list[preamble_max]*update_TA,
				  0,subframe,0);
      }      

    } else {
      MSC_LOG_EVENT(MSC_PHY_ENB, "0 RA Failed add user, too many");
      LOG_I(PHY,"[eNB %d][RAPROC] frame %d, subframe %d: Unable to add user, max user count reached\n",
            phy_vars_eNB->Mod_id,frame, subframe);
    }
  }
}

void ulsch_decoding_procedures(unsigned char subframe, unsigned int i, PHY_VARS_eNB *phy_vars_eNB, unsigned char abstraction_flag)
{
  UNUSED(subframe);
  UNUSED(i);
  UNUSED(phy_vars_eNB);
  UNUSED(abstraction_flag);
  LOG_D(PHY,"ulsch_decoding_procedures not yet implemented. should not be called");
}


void pucch_procedures(const unsigned char sched_subframe,PHY_VARS_eNB *phy_vars_eNB,int UE_id,int harq_pid,const uint8_t abstraction_flag) {

  LTE_DL_FRAME_PARMS *frame_parms=&phy_vars_eNB->lte_frame_parms;
  uint8_t SR_payload = 0,*pucch_payload=NULL,pucch_payload0[2]= {0,0},pucch_payload1[2]= {0,0};
  int16_t n1_pucch0,n1_pucch1,n1_pucch2,n1_pucch3;
  uint8_t do_SR = 0;
  uint8_t pucch_sel = 0;
  int32_t metric0=0,metric1=0,metric0_SR=0;
  ANFBmode_t bundling_flag;
  PUCCH_FMT_t format;

  const int subframe = phy_vars_eNB->proc[sched_subframe].subframe_rx;
  const int frame = phy_vars_eNB->proc[sched_subframe].frame_rx;

  if ((phy_vars_eNB->dlsch_eNB[UE_id][0]) &&
      (phy_vars_eNB->dlsch_eNB[UE_id][0]->rnti>0) &&
      (phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->subframe_scheduling_flag==0)) { 

      // check SR availability
      do_SR = is_SR_subframe(phy_vars_eNB,UE_id,sched_subframe);
      //      do_SR = 0;

      // Now ACK/NAK
      // First check subframe_tx flag for earlier subframes
      get_n1_pucch_eNB(phy_vars_eNB,
                       UE_id,
                       sched_subframe,
                       &n1_pucch0,
                       &n1_pucch1,
                       &n1_pucch2,
                       &n1_pucch3);

      LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d, subframe %d Checking for PUCCH (%d,%d,%d,%d) SR %d\n",
            phy_vars_eNB->Mod_id,phy_vars_eNB->dlsch_eNB[UE_id][0]->rnti,
            frame,subframe,
            n1_pucch0,n1_pucch1,n1_pucch2,n1_pucch3,do_SR);

      if ((n1_pucch0==-1) && (n1_pucch1==-1) && (do_SR==0)) {  // no TX PDSCH that have to be checked and no SR for this UE_id
      } else {
        // otherwise we have some PUCCH detection to do

	// Null out PUCCH PRBs for noise measurement
	switch(phy_vars_eNB->lte_frame_parms.N_RB_UL) {
	case 6:
	  phy_vars_eNB->rb_mask_ul[0] |= (0x1 | (1<<5)); //position 5
	  break;
	case 15:
	  phy_vars_eNB->rb_mask_ul[0] |= (0x1 | (1<<14)); // position 14
	  break;
	case 25:
	  phy_vars_eNB->rb_mask_ul[0] |= (0x1 | (1<<24)); // position 24
	  break;
	case 50:
	  phy_vars_eNB->rb_mask_ul[0] |= 0x1;
	  phy_vars_eNB->rb_mask_ul[1] |= (1<<17); // position 49 (49-32)
	  break;
	case 75:
	  phy_vars_eNB->rb_mask_ul[0] |= 0x1;
	  phy_vars_eNB->rb_mask_ul[2] |= (1<<10); // position 74 (74-64)
	  break;
	case 100:
	  phy_vars_eNB->rb_mask_ul[0] |= 0x1;
	  phy_vars_eNB->rb_mask_ul[3] |= (1<<3); // position 99 (99-96)
	  break;
	default:
	  LOG_E(PHY,"Unknown number for N_RB_UL %d\n",phy_vars_eNB->lte_frame_parms.N_RB_UL);
	  break;
	}

        if (do_SR == 1) {
          phy_vars_eNB->eNB_UE_stats[UE_id].sr_total++;

          if (abstraction_flag == 0)
            metric0_SR = rx_pucch(phy_vars_eNB,
				  pucch_format1,
				  UE_id,
				  phy_vars_eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex,
				  0, // n2_pucch
				  0, // shortened format, should be use_srs flag, later
				  &SR_payload,
                                  frame,
				  subframe,
				  PUCCH1_THRES);

#ifdef PHY_ABSTRACTION
          else {
            metric0_SR = rx_pucch_emul(phy_vars_eNB,
				       UE_id,
				       pucch_format1,
				       0,
				       &SR_payload,
				       sched_subframe);
            LOG_D(PHY,"[eNB %d][SR %x] Frame %d subframe %d Checking SR (UE SR %d/%d)\n",phy_vars_eNB->Mod_id,
                  phy_vars_eNB->ulsch_eNB[UE_id]->rnti,frame,subframe,SR_payload,phy_vars_eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex);
          }

#endif

          if (SR_payload == 1) {
            LOG_D(PHY,"[eNB %d][SR %x] Frame %d subframe %d Got SR for PUSCH, transmitting to MAC\n",phy_vars_eNB->Mod_id,
                  phy_vars_eNB->ulsch_eNB[UE_id]->rnti,frame,subframe);
            phy_vars_eNB->eNB_UE_stats[UE_id].sr_received++;

            if (phy_vars_eNB->first_sr[UE_id] == 1) { // this is the first request for uplink after Connection Setup, so clear HARQ process 0 use for Msg4
              phy_vars_eNB->first_sr[UE_id] = 0;
              phy_vars_eNB->dlsch_eNB[UE_id][0]->harq_processes[0]->round=0;
              phy_vars_eNB->dlsch_eNB[UE_id][0]->harq_processes[0]->status=SCH_IDLE;
              LOG_D(PHY,"[eNB %d][SR %x] Frame %d subframe %d First SR\n",
                    phy_vars_eNB->Mod_id,
                    phy_vars_eNB->ulsch_eNB[UE_id]->rnti,frame,subframe);
            }

	    if (phy_vars_eNB->mac_enabled==1) {
	      mac_xface->SR_indication(phy_vars_eNB->Mod_id,
				       phy_vars_eNB->CC_id,
				       frame,
				       phy_vars_eNB->dlsch_eNB[UE_id][0]->rnti,subframe);
	    }
          }
        }// do_SR==1

        if ((n1_pucch0==-1) && (n1_pucch1==-1)) { // just check for SR
        } else if (phy_vars_eNB->lte_frame_parms.frame_type==FDD) { // FDD
          // if SR was detected, use the n1_pucch from SR, else use n1_pucch0
	  //          n1_pucch0 = (SR_payload==1) ? phy_vars_eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex:n1_pucch0;

	  LOG_D(PHY,"Demodulating PUCCH for ACK/NAK: n1_pucch0 %d (%d), SR_payload %d\n",n1_pucch0,phy_vars_eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex,SR_payload);

          if (abstraction_flag == 0) {



            metric0 = rx_pucch(phy_vars_eNB,
                               pucch_format1a,
                               UE_id,
                               (uint16_t)n1_pucch0,
                               0, //n2_pucch
                               0, // shortened format
                               pucch_payload0,
                               frame,
                               subframe,
                               PUCCH1a_THRES);

            if (metric0 < metric0_SR)
	      metric0=rx_pucch(phy_vars_eNB,
			       pucch_format1a,
			       UE_id,
			       phy_vars_eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex,
			       0, //n2_pucch
			       0, // shortened format
			       pucch_payload0,
                               frame,
			       subframe,
			       PUCCH1a_THRES);
	  }
          else {
#ifdef PHY_ABSTRACTION
            metric0 = rx_pucch_emul(phy_vars_eNB,UE_id,
                                    pucch_format1a,
                                    0,
                                    pucch_payload0,
                                    subframe);
#endif
          }

#ifdef DEBUG_PHY_PROC
          LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d subframe %d pucch1a (FDD) payload %d (metric %d)\n",
                phy_vars_eNB->Mod_id,
                phy_vars_eNB->dlsch_eNB[UE_id][0]->rnti,
                frame,subframe,
                pucch_payload0[0],metric0);
#endif

          process_HARQ_feedback(UE_id,sched_subframe,phy_vars_eNB,
                                0,// pusch_flag
                                pucch_payload0,
                                2,
                                SR_payload);

        } // FDD
        else {  //TDD

          bundling_flag = phy_vars_eNB->pucch_config_dedicated[UE_id].tdd_AckNackFeedbackMode;

          // fix later for 2 TB case and format1b

          if ((frame_parms->frame_type==FDD) ||
              (bundling_flag==bundling)    ||
              ((frame_parms->frame_type==TDD)&&(frame_parms->tdd_config==1)&&((subframe!=2)||(subframe!=7)))) {
            format = pucch_format1a;
            //      msg("PUCCH 1a\n");
          } else {
            format = pucch_format1b;
            //      msg("PUCCH 1b\n");
          }

          // if SR was detected, use the n1_pucch from SR
          if (SR_payload==1) {
#ifdef DEBUG_PHY_PROC
            LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d subframe %d Checking ACK/NAK (%d,%d,%d,%d) format %d with SR\n",phy_vars_eNB->Mod_id,
                  phy_vars_eNB->dlsch_eNB[UE_id][0]->rnti,
                  frame,subframe,
                  n1_pucch0,n1_pucch1,n1_pucch2,n1_pucch3,format);
#endif

            if (abstraction_flag == 0)
              metric0_SR = rx_pucch(phy_vars_eNB,
				    format,
				    UE_id,
				    phy_vars_eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex,
				    0, //n2_pucch
				    0, // shortened format
				    pucch_payload0,
                                    frame,
				    subframe,
				    PUCCH1a_THRES);
            else {
#ifdef PHY_ABSTRACTION
              metric0 = rx_pucch_emul(phy_vars_eNB,UE_id,
                                      format,
                                      0,
                                      pucch_payload0,
                                      subframe);
#endif
            }
          } else { //using n1_pucch0/n1_pucch1 resources
#ifdef DEBUG_PHY_PROC
            LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d subframe %d Checking ACK/NAK (%d,%d,%d,%d) format %d\n",phy_vars_eNB->Mod_id,
                  phy_vars_eNB->dlsch_eNB[UE_id][0]->rnti,
                  frame,subframe,
                  n1_pucch0,n1_pucch1,n1_pucch2,n1_pucch3,format);
#endif
            metric0=0;
            metric1=0;

            // Check n1_pucch0 metric
            if (n1_pucch0 != -1) {
              if (abstraction_flag == 0)
                metric0 = rx_pucch(phy_vars_eNB,
                                   format,
                                   UE_id,
                                   (uint16_t)n1_pucch0,
                                   0, // n2_pucch
                                   0, // shortened format
                                   pucch_payload0,
                                   frame,
                                   subframe,
                                   PUCCH1a_THRES);
              else {
#ifdef PHY_ABSTRACTION
                metric0 = rx_pucch_emul(phy_vars_eNB,UE_id,
                                        format,
                                        0,
                                        pucch_payload0,
                                        subframe);
#endif
              }
            }

            // Check n1_pucch1 metric
            if (n1_pucch1 != -1) {
              if (abstraction_flag == 0)
                metric1 = rx_pucch(phy_vars_eNB,
                                   format,
                                   UE_id,
                                   (uint16_t)n1_pucch1,
                                   0, //n2_pucch
                                   0, // shortened format
                                   pucch_payload1,
                                   frame,
                                   subframe,
                                   PUCCH1a_THRES);
              else {
#ifdef PHY_ABSTRACTION
                metric1 = rx_pucch_emul(phy_vars_eNB,UE_id,
                                        format,
                                        1,
                                        pucch_payload1,
                                        subframe);


#endif
              }
            }
          }

          if (SR_payload == 1) {
            pucch_payload = pucch_payload0;

            if (bundling_flag == bundling)
              pucch_sel = 2;
          } else if (bundling_flag == multiplexing) { // multiplexing + no SR
            pucch_payload = (metric1>metric0) ? pucch_payload1 : pucch_payload0;
            pucch_sel     = (metric1>metric0) ? 1 : 0;
          } else { // bundling + no SR
            if (n1_pucch1 != -1)
              pucch_payload = pucch_payload1;
            else if (n1_pucch0 != -1)
              pucch_payload = pucch_payload0;

            pucch_sel = 2;  // indicate that this is a bundled ACK/NAK
          }

#ifdef DEBUG_PHY_PROC
          LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d subframe %d ACK/NAK metric 0 %d, metric 1 %d, sel %d, (%d,%d)\n",phy_vars_eNB->Mod_id,
                phy_vars_eNB->dlsch_eNB[UE_id][0]->rnti,
                frame,subframe,
                metric0,metric1,pucch_sel,pucch_payload[0],pucch_payload[1]);
#endif
          process_HARQ_feedback(UE_id,sched_subframe,phy_vars_eNB,
                                0,// pusch_flag
                                pucch_payload,
                                pucch_sel,
                                SR_payload);
        }
      }

    }
}

void cba_procedures(const unsigned char sched_subframe,PHY_VARS_eNB *phy_vars_eNB,int UE_id,int harq_pid,const uint8_t abstraction_flag) {

  uint8_t access_mode;
  int num_active_cba_groups;
  const int subframe = phy_vars_eNB->proc[sched_subframe].subframe_rx;
  const int frame = phy_vars_eNB->proc[sched_subframe].frame_rx;
  uint16_t rnti=0;
  int ret=0;

  num_active_cba_groups = phy_vars_eNB->ulsch_eNB[UE_id]->num_active_cba_groups;
  
  if ((phy_vars_eNB->ulsch_eNB[UE_id]) &&
      (num_active_cba_groups > 0) &&
      (phy_vars_eNB->ulsch_eNB[UE_id]->cba_rnti[UE_id%num_active_cba_groups]>0) &&
      (phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->subframe_cba_scheduling_flag==1)) {
    rnti=0;
    
#ifdef DEBUG_PHY_PROC
    LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d Checking PUSCH/ULSCH CBA Reception for UE %d with cba rnti %x mode %s\n",
	  phy_vars_eNB->Mod_id,harq_pid,
	  frame,subframe,
	  UE_id, (uint16_t)phy_vars_eNB->ulsch_eNB[UE_id]->cba_rnti[UE_id%num_active_cba_groups],mode_string[phy_vars_eNB->eNB_UE_stats[UE_id].mode]);
#endif
    
    if (abstraction_flag==0) {
      rx_ulsch(phy_vars_eNB,
	       sched_subframe,
	       phy_vars_eNB->eNB_UE_stats[UE_id].sector,  // this is the effective sector id
	       UE_id,
	       phy_vars_eNB->ulsch_eNB,
	       0);
    }
    
#ifdef PHY_ABSTRACTION
    else {
      rx_ulsch_emul(phy_vars_eNB,
		    subframe,
		    phy_vars_eNB->eNB_UE_stats[UE_id].sector,  // this is the effective sector id
		    UE_id);
    }
    
#endif
    
    if (abstraction_flag == 0) {
      ret = ulsch_decoding(phy_vars_eNB,
			   UE_id,
			   sched_subframe,
			   0, // control_only_flag
			   phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->V_UL_DAI,
			   phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->nb_rb>20 ? 1 : 0);
    }
    
#ifdef PHY_ABSTRACTION
    else {
      ret = ulsch_decoding_emul(phy_vars_eNB,
				sched_subframe,
				UE_id,
				&rnti);
    }
    
#endif
    
    if (phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->cqi_crc_status == 1) {
#ifdef DEBUG_PHY_PROC
      
      print_CQI(phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->o,phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->uci_format,0,phy_vars_eNB->lte_frame_parms.N_RB_DL);
#endif
      access_mode = UNKNOWN_ACCESS;
      extract_CQI(phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->o,
		  phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->uci_format,
		  &phy_vars_eNB->eNB_UE_stats[UE_id],
		  phy_vars_eNB->lte_frame_parms.N_RB_DL,
		  &rnti, &access_mode);
      phy_vars_eNB->eNB_UE_stats[UE_id].rank = phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->o_RI[0];
    }
    
      phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->subframe_cba_scheduling_flag=0;
      phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->status= SCH_IDLE;
      
      if ((num_active_cba_groups > 0) &&
          (UE_id + num_active_cba_groups < NUMBER_OF_UE_MAX) &&
          (phy_vars_eNB->ulsch_eNB[UE_id+num_active_cba_groups]->cba_rnti[UE_id%num_active_cba_groups] > 0 ) &&
          (phy_vars_eNB->ulsch_eNB[UE_id+num_active_cba_groups]->num_active_cba_groups> 0)) {
#ifdef DEBUG_PHY_PROC
        LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d UE %d harq_pid %d resetting the subframe_scheduling_flag for Ue %d cba groups %d members\n",
              phy_vars_eNB->Mod_id,harq_pid,frame,subframe,UE_id,harq_pid,
              UE_id+num_active_cba_groups, UE_id%phy_vars_eNB->ulsch_eNB[UE_id]->num_active_cba_groups);
#endif
        phy_vars_eNB->ulsch_eNB[UE_id+num_active_cba_groups]->harq_processes[harq_pid]->subframe_cba_scheduling_flag=1;
        phy_vars_eNB->ulsch_eNB[UE_id+num_active_cba_groups]->harq_processes[harq_pid]->status= CBA_ACTIVE;
        phy_vars_eNB->ulsch_eNB[UE_id+num_active_cba_groups]->harq_processes[harq_pid]->TBS=phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->TBS;
      }

      if (ret == (1+MAX_TURBO_ITERATIONS)) {
        phy_vars_eNB->eNB_UE_stats[UE_id].ulsch_round_errors[harq_pid][phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->round]++;
        phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->phich_active = 1;
        phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->phich_ACK = 0;
        phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->round++;
      } // ulsch in error
      else {
        LOG_D(PHY,"[eNB %d][PUSCH %d] Frame %d subframe %d ULSCH received, setting round to 0, PHICH ACK\n",
              phy_vars_eNB->Mod_id,harq_pid,
              frame,subframe);

        phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->phich_active = 1;
        phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->phich_ACK = 1;
        phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->round = 0;
        phy_vars_eNB->eNB_UE_stats[UE_id].ulsch_consecutive_errors = 0;
#ifdef DEBUG_PHY_PROC
#ifdef DEBUG_ULSCH
        LOG_D(PHY,"[eNB] Frame %d, Subframe %d : ULSCH SDU (RX harq_pid %d) %d bytes:",
              frame,subframe,
              harq_pid,phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->TBS>>3);

        for (j=0; j<phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->TBS>>3; j++)
          LOG_T(PHY,"%x.",phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->b[j]);

        LOG_T(PHY,"\n");
#endif
#endif

        if (access_mode > UNKNOWN_ACCESS) {
          LOG_D(PHY,"[eNB %d] Frame %d, Subframe %d : received ULSCH SDU from CBA transmission, UE (%d,%x), CBA (group %d, rnti %x)\n",
                phy_vars_eNB->Mod_id, frame,subframe,
                UE_id, phy_vars_eNB->ulsch_eNB[UE_id]->rnti,
                UE_id % phy_vars_eNB->ulsch_eNB[UE_id]->num_active_cba_groups, phy_vars_eNB->ulsch_eNB[UE_id]->cba_rnti[UE_id%num_active_cba_groups]);

          // detect if there is a CBA collision
          if (phy_vars_eNB->cba_last_reception[UE_id%num_active_cba_groups] == 0 ) {
            mac_xface->rx_sdu(phy_vars_eNB->Mod_id,
                              phy_vars_eNB->CC_id,
                              frame,subframe,
                              phy_vars_eNB->ulsch_eNB[UE_id]->rnti,
                              phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->b,
                              phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->TBS>>3,
                              harq_pid,
                              NULL);

            phy_vars_eNB->cba_last_reception[UE_id%num_active_cba_groups]+=1;//(subframe);
          } else {
            if (phy_vars_eNB->cba_last_reception[UE_id%num_active_cba_groups] == 1 )
              LOG_N(PHY,"[eNB%d] Frame %d subframe %d : first CBA collision detected \n ",
                    phy_vars_eNB->Mod_id,frame,subframe);

            LOG_N(PHY,"[eNB%d] Frame %d subframe %d : CBA collision set SR for UE %d in group %d \n ",
                  phy_vars_eNB->Mod_id,frame,subframe,
                  phy_vars_eNB->cba_last_reception[UE_id%num_active_cba_groups],UE_id%num_active_cba_groups );

            phy_vars_eNB->cba_last_reception[UE_id%num_active_cba_groups]+=1;

            mac_xface->SR_indication(phy_vars_eNB->Mod_id,
                                     phy_vars_eNB->CC_id,
                                     frame,
                                     phy_vars_eNB->dlsch_eNB[UE_id][0]->rnti,subframe);
          }
        } // UNKNOWN_ACCESS
      } // ULSCH CBA not in error
  }

}

void phy_procedures_eNB_RX(const unsigned char sched_subframe,PHY_VARS_eNB *phy_vars_eNB,const uint8_t abstraction_flag,const relaying_type_t r_type)
{
  //RX processing
  UNUSED(r_type);
  uint32_t l, ret=0,i,j,k;
  uint32_t harq_pid, harq_idx, round;
  uint8_t nPRS;
  LTE_DL_FRAME_PARMS *frame_parms=&phy_vars_eNB->lte_frame_parms;
  int sync_pos;
  uint16_t rnti=0;
  uint8_t access_mode;

  const int subframe = phy_vars_eNB->proc[sched_subframe].subframe_rx;
  const int frame = phy_vars_eNB->proc[sched_subframe].frame_rx;

  AssertFatal(sched_subframe < NUM_ENB_THREADS, "Bad sched_subframe %d", sched_subframe);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_RX,1);
  start_meas(&phy_vars_eNB->phy_proc_rx);
#ifdef DEBUG_PHY_PROC
  LOG_D(PHY,"[eNB %d] Frame %d: Doing phy_procedures_eNB_RX(%d)\n",phy_vars_eNB->Mod_id,frame, subframe);
#endif

  T(T_ENB_PHY_UL_TICK, T_INT(phy_vars_eNB->Mod_id), T_INT(frame), T_INT(subframe));

  T(T_ENB_PHY_INPUT_SIGNAL, T_INT(phy_vars_eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(0),
    T_BUFFER(&phy_vars_eNB->lte_eNB_common_vars.rxdata[0][0][subframe*phy_vars_eNB->lte_frame_parms.samples_per_tti],
             phy_vars_eNB->lte_frame_parms.samples_per_tti * 4));

  phy_vars_eNB->rb_mask_ul[0]=0;
  phy_vars_eNB->rb_mask_ul[1]=0;
  phy_vars_eNB->rb_mask_ul[2]=0;
  phy_vars_eNB->rb_mask_ul[3]=0;

  if (abstraction_flag == 0) {
    remove_7_5_kHz(phy_vars_eNB,subframe<<1);
    remove_7_5_kHz(phy_vars_eNB,(subframe<<1)+1);
  }

  // check if we have to detect PRACH first
  if (is_prach_subframe(&phy_vars_eNB->lte_frame_parms,frame,subframe)>0) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PRACH_RX,1);
    prach_procedures(phy_vars_eNB,sched_subframe,abstraction_flag);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PRACH_RX,0);
  }

  if (abstraction_flag == 0) {
    start_meas(&phy_vars_eNB->ofdm_demod_stats);

    for (l=0; l<phy_vars_eNB->lte_frame_parms.symbols_per_tti/2; l++) {

      slot_fep_ul(&phy_vars_eNB->lte_frame_parms,
                  &phy_vars_eNB->lte_eNB_common_vars,
                  l,
                  subframe<<1,
                  0,
                  0
                 );
      slot_fep_ul(&phy_vars_eNB->lte_frame_parms,
                  &phy_vars_eNB->lte_eNB_common_vars,
                  l,
                  (subframe<<1)+1,
                  0,
                  0
                 );
    }

    stop_meas(&phy_vars_eNB->ofdm_demod_stats);
  }

  // Check for active processes in current subframe
  harq_pid = subframe2harq_pid(&phy_vars_eNB->lte_frame_parms,
                               frame,subframe);

  // reset the cba flag used for collision detection
  for (i=0; i < NUM_MAX_CBA_GROUP; i++) {
    phy_vars_eNB->cba_last_reception[i]=0;
  }

  // Do PUCCH processing first

  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    pucch_procedures(sched_subframe,phy_vars_eNB,i,harq_pid,abstraction_flag);
  }

  for (i=0; i<NUMBER_OF_UE_MAX; i++) {

    // check for Msg3
    if (phy_vars_eNB->mac_enabled==1) {
      if (phy_vars_eNB->eNB_UE_stats[i].mode == RA_RESPONSE) {
	process_Msg3(phy_vars_eNB,sched_subframe,i,harq_pid);
      }
    }


    phy_vars_eNB->pusch_stats_rb[i][(frame*10)+subframe] = -63;
    phy_vars_eNB->pusch_stats_round[i][(frame*10)+subframe] = 0;
    phy_vars_eNB->pusch_stats_mcs[i][(frame*10)+subframe] = -63;

    if ((phy_vars_eNB->ulsch_eNB[i]) &&
        (phy_vars_eNB->ulsch_eNB[i]->rnti>0) &&
        (phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->subframe_scheduling_flag==1)) {
      // UE is has ULSCH scheduling
      round = phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round;

      for (int rb=0;
           rb<=phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->nb_rb;
	   rb++) {
	int rb2 = rb+phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->first_rb;
	phy_vars_eNB->rb_mask_ul[rb2>>5] |= (1<<(rb2&31));
      }
#ifdef DEBUG_PHY_PROC
      LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d Scheduling PUSCH/ULSCH Reception for rnti %x (UE_id %d)\n",
            phy_vars_eNB->Mod_id,harq_pid,
            frame,subframe,phy_vars_eNB->ulsch_eNB[i]->rnti,i);
#endif

      if (phy_vars_eNB->ulsch_eNB[i]->Msg3_flag == 1) {
        LOG_D(PHY,"[eNB %d] frame %d, subframe %d: Scheduling ULSCH Reception for Msg3 in Sector %d\n",
              phy_vars_eNB->Mod_id,
              frame,
              subframe,
              phy_vars_eNB->eNB_UE_stats[i].sector);
	VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_ULSCH_MSG3,1);
      } else {
        LOG_D(PHY,"[eNB %d] frame %d, subframe %d: Scheduling ULSCH Reception for UE %d Mode %s\n",
              phy_vars_eNB->Mod_id,
              frame,
              subframe,
              i,
              mode_string[phy_vars_eNB->eNB_UE_stats[i].mode]);
      }


      nPRS = phy_vars_eNB->lte_frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.nPRS[subframe<<1];

      phy_vars_eNB->ulsch_eNB[i]->cyclicShift = (phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->n_DMRS2 + phy_vars_eNB->lte_frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift +
          nPRS)%12;

      if (frame_parms->frame_type == FDD ) {
        int sf = (subframe<4) ? (subframe+6) : (subframe-4);

        if (phy_vars_eNB->dlsch_eNB[i][0]->subframe_tx[sf]>0) { // we have downlink transmission
          phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->O_ACK = 1;
        } else {
          phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->O_ACK = 0;
        }
      }

      LOG_D(PHY,
            "[eNB %d][PUSCH %d] Frame %d Subframe %d Demodulating PUSCH: dci_alloc %d, rar_alloc %d, round %d, first_rb %d, nb_rb %d, mcs %d, TBS %d, rv %d, cyclic_shift %d (n_DMRS2 %d, cyclicShift_common %d, nprs %d), O_ACK %d \n",
            phy_vars_eNB->Mod_id,harq_pid,frame,subframe,
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->dci_alloc,
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->rar_alloc,
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round,
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->first_rb,
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->nb_rb,
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->mcs,
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->TBS,
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->rvidx,
            phy_vars_eNB->ulsch_eNB[i]->cyclicShift,
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->n_DMRS2,
            phy_vars_eNB->lte_frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift,
            nPRS,
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->O_ACK);
      phy_vars_eNB->pusch_stats_rb[i][(frame*10)+subframe] = phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->nb_rb;
      phy_vars_eNB->pusch_stats_round[i][(frame*10)+subframe] = phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round;
      phy_vars_eNB->pusch_stats_mcs[i][(frame*10)+subframe] = phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->mcs;
      start_meas(&phy_vars_eNB->ulsch_demodulation_stats);

      if (abstraction_flag==0) {
        rx_ulsch(phy_vars_eNB,
                 sched_subframe,
                 phy_vars_eNB->eNB_UE_stats[i].sector,  // this is the effective sector id
                 i,
                 phy_vars_eNB->ulsch_eNB,
                 0);
      }

#ifdef PHY_ABSTRACTION
      else {
        rx_ulsch_emul(phy_vars_eNB,
                      subframe,
                      phy_vars_eNB->eNB_UE_stats[i].sector,  // this is the effective sector id
                      i);
      }

#endif
      stop_meas(&phy_vars_eNB->ulsch_demodulation_stats);


      start_meas(&phy_vars_eNB->ulsch_decoding_stats);

      if (abstraction_flag == 0) {
        ret = ulsch_decoding(phy_vars_eNB,
                             i,
                             sched_subframe,
                             0, // control_only_flag
                             phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->V_UL_DAI,
			     phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->nb_rb>20 ? 1 : 0);
      }

#ifdef PHY_ABSTRACTION
      else {
        ret = ulsch_decoding_emul(phy_vars_eNB,
                                  sched_subframe,
                                  i,
                                  &rnti);
      }

#endif
      stop_meas(&phy_vars_eNB->ulsch_decoding_stats);

      LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d RNTI %x RX power (%d,%d) RSSI (%d,%d) N0 (%d,%d) dB ACK (%d,%d), decoding iter %d\n",
            phy_vars_eNB->Mod_id,harq_pid,
            frame,subframe,
            phy_vars_eNB->ulsch_eNB[i]->rnti,
            dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power[0]),
            dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power[1]),
            phy_vars_eNB->eNB_UE_stats[i].UL_rssi[0],
            phy_vars_eNB->eNB_UE_stats[i].UL_rssi[1],
            phy_vars_eNB->PHY_measurements_eNB->n0_power_dB[0],
            phy_vars_eNB->PHY_measurements_eNB->n0_power_dB[1],
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->o_ACK[0],
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->o_ACK[1],
            ret);


      //compute the expected ULSCH RX power (for the stats)
      phy_vars_eNB->ulsch_eNB[(uint32_t)i]->harq_processes[harq_pid]->delta_TF =
        get_hundred_times_delta_IF_eNB(phy_vars_eNB,i,harq_pid, 0); // 0 means bw_factor is not considered

      //dump_ulsch(phy_vars_eNB, sched_subframe, i);

      phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts[harq_pid][phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round]++;
#ifdef DEBUG_PHY_PROC
      LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d UE %d harq_pid %d Clearing subframe_scheduling_flag\n",
            phy_vars_eNB->Mod_id,harq_pid,frame,subframe,i,harq_pid);
#endif
      phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->subframe_scheduling_flag=0;

      if (phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->cqi_crc_status == 1) {
#ifdef DEBUG_PHY_PROC
        //if (((phy_vars_eNB->proc[sched_subframe].frame_tx%10) == 0) || (phy_vars_eNB->proc[sched_subframe].frame_tx < 50))
        print_CQI(phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->o,phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->uci_format,0,phy_vars_eNB->lte_frame_parms.N_RB_DL);
#endif
        extract_CQI(phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->o,
                    phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->uci_format,
                    &phy_vars_eNB->eNB_UE_stats[i],
                    phy_vars_eNB->lte_frame_parms.N_RB_DL,
                    &rnti, &access_mode);
        phy_vars_eNB->eNB_UE_stats[i].rank = phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->o_RI[0];

      }

      if (phy_vars_eNB->ulsch_eNB[i]->Msg3_flag == 1)
	VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_ULSCH_MSG3,0);

      if (ret == (1+MAX_TURBO_ITERATIONS)) {
        T(T_ENB_PHY_ULSCH_UE_NACK, T_INT(phy_vars_eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(i), T_INT(phy_vars_eNB->ulsch_eNB[i]->rnti),
          T_INT(harq_pid));

        phy_vars_eNB->eNB_UE_stats[i].ulsch_round_errors[harq_pid][phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round]++;
        phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_active = 1;
        phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_ACK = 0;
        phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round++;

        LOG_D(PHY,"[eNB][PUSCH %d] Increasing to round %d\n",harq_pid,phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round);

        if (phy_vars_eNB->ulsch_eNB[i]->Msg3_flag == 1) {
          LOG_D(PHY,"[eNB %d/%d][RAPROC] frame %d, subframe %d, UE %d: Error receiving ULSCH (Msg3), round %d/%d\n",
                phy_vars_eNB->Mod_id,
                phy_vars_eNB->CC_id,
                frame,subframe, i,
                phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round-1,
                phy_vars_eNB->lte_frame_parms.maxHARQ_Msg3Tx-1);

	  LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d RNTI %x RX power (%d,%d) RSSI (%d,%d) N0 (%d,%d) dB ACK (%d,%d), decoding iter %d\n",
		phy_vars_eNB->Mod_id,harq_pid,
		frame,subframe,
		phy_vars_eNB->ulsch_eNB[i]->rnti,
		dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power[0]),
		dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power[1]),
		phy_vars_eNB->eNB_UE_stats[i].UL_rssi[0],
		phy_vars_eNB->eNB_UE_stats[i].UL_rssi[1],
		phy_vars_eNB->PHY_measurements_eNB->n0_power_dB[0],
		phy_vars_eNB->PHY_measurements_eNB->n0_power_dB[1],
		phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->o_ACK[0],
		phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->o_ACK[1],
		ret);

          if (phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round ==
              phy_vars_eNB->lte_frame_parms.maxHARQ_Msg3Tx) {
            LOG_D(PHY,"[eNB %d][RAPROC] maxHARQ_Msg3Tx reached, abandoning RA procedure for UE %d\n",
                  phy_vars_eNB->Mod_id, i);
            phy_vars_eNB->eNB_UE_stats[i].mode = PRACH;
	    if (phy_vars_eNB->mac_enabled==1) {
	      mac_xface->cancel_ra_proc(phy_vars_eNB->Mod_id,
					phy_vars_eNB->CC_id,
					frame,
					phy_vars_eNB->eNB_UE_stats[i].crnti);
	    }
            mac_phy_remove_ue(phy_vars_eNB->Mod_id,phy_vars_eNB->eNB_UE_stats[i].crnti);

            phy_vars_eNB->ulsch_eNB[(uint32_t)i]->Msg3_active = 0;
            //phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_active = 0;

          } else {
            // activate retransmission for Msg3 (signalled to UE PHY by PHICH (not MAC/DCI)
            phy_vars_eNB->ulsch_eNB[(uint32_t)i]->Msg3_active = 1;

            get_Msg3_alloc_ret(&phy_vars_eNB->lte_frame_parms,
                               subframe,
                               frame,
                               &phy_vars_eNB->ulsch_eNB[i]->Msg3_frame,
                               &phy_vars_eNB->ulsch_eNB[i]->Msg3_subframe);
          }
          LOG_D(PHY,"[eNB] Frame %d, Subframe %d: Msg3 in error, i = %d \n", frame,subframe,i);
        } // This is Msg3 error

        else { //normal ULSCH
          LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d UE %d Error receiving ULSCH, round %d/%d (ACK %d,%d)\n",
                phy_vars_eNB->Mod_id,harq_pid,
                frame,subframe, i,
                phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round-1,
                phy_vars_eNB->ulsch_eNB[i]->Mlimit,
                phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->o_ACK[0],
                phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->o_ACK[1]);

#if defined(MESSAGE_CHART_GENERATOR_PHY)
          MSC_LOG_RX_DISCARDED_MESSAGE(
            MSC_PHY_ENB,MSC_PHY_UE,
            NULL,0,
            "%05u:%02u ULSCH received rnti %x harq id %u round %d",
            frame,subframe,
            phy_vars_eNB->ulsch_eNB[i]->rnti,harq_pid,
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round-1
            );
#endif

          if (phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round== phy_vars_eNB->ulsch_eNB[i]->Mlimit) {
            LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d UE %d ULSCH Mlimit %d reached\n",
                  phy_vars_eNB->Mod_id,harq_pid,
                  frame,subframe, i,
                  phy_vars_eNB->ulsch_eNB[i]->Mlimit);

            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round=0;
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_active=0;
            phy_vars_eNB->eNB_UE_stats[i].ulsch_errors[harq_pid]++;
            phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors++;

	    // indicate error to MAC
	    mac_xface->rx_sdu(phy_vars_eNB->Mod_id,
			      phy_vars_eNB->CC_id,
			      frame,subframe,
			      phy_vars_eNB->ulsch_eNB[i]->rnti,
			      NULL,
			      0,
			      harq_pid,
			      &phy_vars_eNB->ulsch_eNB[i]->Msg3_flag);
          }
        }
      }  // ulsch in error
      else {
        T(T_ENB_PHY_ULSCH_UE_ACK, T_INT(phy_vars_eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(i), T_INT(phy_vars_eNB->ulsch_eNB[i]->rnti),
          T_INT(harq_pid));

        if (phy_vars_eNB->ulsch_eNB[i]->Msg3_flag == 1) {
	  LOG_D(PHY,"[eNB %d][PUSCH %d] Frame %d subframe %d ULSCH received, setting round to 0, PHICH ACK\n",
		phy_vars_eNB->Mod_id,harq_pid,
		frame,subframe);
	  LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d RNTI %x RX power (%d,%d) RSSI (%d,%d) N0 (%d,%d) dB ACK (%d,%d), decoding iter %d\n",
		phy_vars_eNB->Mod_id,harq_pid,
		frame,subframe,
		phy_vars_eNB->ulsch_eNB[i]->rnti,
		dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power[0]),
		dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power[1]),
		phy_vars_eNB->eNB_UE_stats[i].UL_rssi[0],
		phy_vars_eNB->eNB_UE_stats[i].UL_rssi[1],
		phy_vars_eNB->PHY_measurements_eNB->n0_power_dB[0],
		phy_vars_eNB->PHY_measurements_eNB->n0_power_dB[1],
		phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->o_ACK[0],
		phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->o_ACK[1],
		ret);
	}
#if defined(MESSAGE_CHART_GENERATOR_PHY)
        MSC_LOG_RX_MESSAGE(
          MSC_PHY_ENB,MSC_PHY_UE,
          NULL,0,
          "%05u:%02u ULSCH received rnti %x harq id %u",
          frame,subframe,
          phy_vars_eNB->ulsch_eNB[i]->rnti,harq_pid
          );
#endif
        for (j=0; j<phy_vars_eNB->lte_frame_parms.nb_antennas_rx; j++)
          //this is the RSSI per RB
          phy_vars_eNB->eNB_UE_stats[i].UL_rssi[j] =
	    
            dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[i]->ulsch_power[j]*
                     (phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->nb_rb*12)/
                     phy_vars_eNB->lte_frame_parms.ofdm_symbol_size) -
            phy_vars_eNB->rx_total_gain_eNB_dB -
            hundred_times_log10_NPRB[phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->nb_rb-1]/100 -
            get_hundred_times_delta_IF_eNB(phy_vars_eNB,i,harq_pid, 0)/100;
	    
        phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_active = 1;
        phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_ACK = 1;
        phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round = 0;
        phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors = 0;

        if (phy_vars_eNB->ulsch_eNB[i]->Msg3_flag == 1) {
	  if (phy_vars_eNB->mac_enabled==1) {

	    LOG_I(PHY,"[eNB %d][RAPROC] Frame %d Terminating ra_proc for harq %d, UE %d\n",
		  phy_vars_eNB->Mod_id,
		  frame,harq_pid,i);
	    
	    mac_xface->rx_sdu(phy_vars_eNB->Mod_id,
			      phy_vars_eNB->CC_id,
			      frame,subframe,
			      phy_vars_eNB->ulsch_eNB[i]->rnti,
			      phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->b,
			      phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->TBS>>3,
			      harq_pid,
			      &phy_vars_eNB->ulsch_eNB[i]->Msg3_flag);
	    
	    // one-shot msg3 detection by MAC: empty PDU (e.g. CRNTI)
	    if (phy_vars_eNB->ulsch_eNB[i]->Msg3_flag == 0 ) {
	      phy_vars_eNB->eNB_UE_stats[i].mode = PRACH;
	      mac_xface->cancel_ra_proc(phy_vars_eNB->Mod_id,
					phy_vars_eNB->CC_id,
					frame,
					phy_vars_eNB->eNB_UE_stats[i].crnti);
	      mac_phy_remove_ue(phy_vars_eNB->Mod_id,phy_vars_eNB->eNB_UE_stats[i].crnti);
	      phy_vars_eNB->ulsch_eNB[(uint32_t)i]->Msg3_active = 0;
	    } // Msg3_flag == 0
	    
	  } // mac_enabled==1

          phy_vars_eNB->eNB_UE_stats[i].mode = PUSCH;
          phy_vars_eNB->ulsch_eNB[i]->Msg3_flag = 0;

	  LOG_D(PHY,"[eNB %d][RAPROC] Frame %d : RX Subframe %d Setting UE %d mode to PUSCH\n",phy_vars_eNB->Mod_id,frame,subframe,i);

          for (k=0; k<8; k++) { //harq_processes
            for (j=0; j<phy_vars_eNB->dlsch_eNB[i][0]->Mlimit; j++) {
              phy_vars_eNB->eNB_UE_stats[i].dlsch_NAK[k][j]=0;
              phy_vars_eNB->eNB_UE_stats[i].dlsch_ACK[k][j]=0;
              phy_vars_eNB->eNB_UE_stats[i].dlsch_trials[k][j]=0;
            }

            phy_vars_eNB->eNB_UE_stats[i].dlsch_l2_errors[k]=0;
            phy_vars_eNB->eNB_UE_stats[i].ulsch_errors[k]=0;
            phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors=0;

            for (j=0; j<phy_vars_eNB->ulsch_eNB[i]->Mlimit; j++) {
              phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts[k][j]=0;
              phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts_last[k][j]=0;
              phy_vars_eNB->eNB_UE_stats[i].ulsch_round_errors[k][j]=0;
              phy_vars_eNB->eNB_UE_stats[i].ulsch_round_fer[k][j]=0;
            }
          }

          phy_vars_eNB->eNB_UE_stats[i].dlsch_sliding_cnt=0;
          phy_vars_eNB->eNB_UE_stats[i].dlsch_NAK_round0=0;
          phy_vars_eNB->eNB_UE_stats[i].dlsch_mcs_offset=0;
        } // Msg3_flag==1
	else {  // Msg3_flag == 0

#ifdef DEBUG_PHY_PROC
#ifdef DEBUG_ULSCH
          LOG_D(PHY,"[eNB] Frame %d, Subframe %d : ULSCH SDU (RX harq_pid %d) %d bytes:",frame,subframe,
                harq_pid,phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->TBS>>3);

          for (j=0; j<phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->TBS>>3; j++)
            LOG_T(PHY,"%x.",phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->b[j]);

          LOG_T(PHY,"\n");
#endif
#endif

	  if (phy_vars_eNB->mac_enabled==1) {

	    mac_xface->rx_sdu(phy_vars_eNB->Mod_id,
			      phy_vars_eNB->CC_id,
			      frame,subframe,
			      phy_vars_eNB->ulsch_eNB[i]->rnti,
			      phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->b,
			      phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->TBS>>3,
			      harq_pid,
			      NULL);

#ifdef LOCALIZATION
	    start_meas(&phy_vars_eNB->localization_stats);
	    aggregate_eNB_UE_localization_stats(phy_vars_eNB,
						i,
						frame,
						subframe,
						get_hundred_times_delta_IF_eNB(phy_vars_eNB,i,harq_pid, 1)/100);
	    stop_meas(&phy_vars_eNB->localization_stats);
#endif
	    
	  } // mac_enabled==1
        } // Msg3_flag == 0

        // estimate timing advance for MAC
        if (abstraction_flag == 0) {
          sync_pos = lte_est_timing_advance_pusch(phy_vars_eNB,i,sched_subframe);
          phy_vars_eNB->eNB_UE_stats[i].timing_advance_update = sync_pos - phy_vars_eNB->lte_frame_parms.nb_prefix_samples/4; //to check
        }

#ifdef DEBUG_PHY_PROC
        LOG_D(PHY,"[eNB %d] frame %d, subframe %d: user %d: timing advance = %d\n",
              phy_vars_eNB->Mod_id,
              frame, subframe,
              i,
              phy_vars_eNB->eNB_UE_stats[i].timing_advance_update);
#endif


      }  // ulsch not in error

      // process HARQ feedback
#ifdef DEBUG_PHY_PROC
      LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d subframe %d, Processing HARQ feedback for UE %d (after PUSCH)\n",phy_vars_eNB->Mod_id,
            phy_vars_eNB->dlsch_eNB[i][0]->rnti,
            frame,subframe,
            i);
#endif
      process_HARQ_feedback(i,
                            sched_subframe,
                            phy_vars_eNB,
                            1, // pusch_flag
                            0,
                            0,
                            0);

#ifdef DEBUG_PHY_PROC
      LOG_D(PHY,"[eNB %d] Frame %d subframe %d, sect %d: received ULSCH harq_pid %d for UE %d, ret = %d, CQI CRC Status %d, ACK %d,%d, ulsch_errors %d/%d\n",
            phy_vars_eNB->Mod_id,frame,subframe,
            phy_vars_eNB->eNB_UE_stats[i].sector,
            harq_pid,
            i,
            ret,
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->cqi_crc_status,
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->o_ACK[0],
            phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->o_ACK[1],
            phy_vars_eNB->eNB_UE_stats[i].ulsch_errors[harq_pid],
            phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts[harq_pid][0]);
#endif
      
      // dump stats to VCD
      if (i==0) {
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_MCS0+harq_pid,phy_vars_eNB->pusch_stats_mcs[0][(frame*10)+subframe]);
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_RB0+harq_pid,phy_vars_eNB->pusch_stats_rb[0][(frame*10)+subframe]);
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_ROUND0+harq_pid,phy_vars_eNB->pusch_stats_round[0][(frame*10)+subframe]);
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_RSSI0+harq_pid,dB_fixed(phy_vars_eNB->lte_eNB_pusch_vars[0]->ulsch_power[0]));
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_RES0+harq_pid,ret);
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_SFN0+harq_pid,(frame*10)+subframe);
      }
    } // ulsch_eNB[0] && ulsch_eNB[0]->rnti>0 && ulsch_eNB[0]->subframe_scheduling_flag == 1


    // update ULSCH statistics for tracing
    if ((frame % 100 == 0) && (subframe == 4)) {
      for (harq_idx=0; harq_idx<8; harq_idx++) {
        for (round=0; round<phy_vars_eNB->ulsch_eNB[i]->Mlimit; round++) {
          if ((phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts[harq_idx][round] -
               phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts_last[harq_idx][round]) != 0) {
            phy_vars_eNB->eNB_UE_stats[i].ulsch_round_fer[harq_idx][round] =
              (100*(phy_vars_eNB->eNB_UE_stats[i].ulsch_round_errors[harq_idx][round] -
                    phy_vars_eNB->eNB_UE_stats[i].ulsch_round_errors_last[harq_idx][round]))/
              (phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts[harq_idx][round] -
               phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts_last[harq_idx][round]);
          } else {
            phy_vars_eNB->eNB_UE_stats[i].ulsch_round_fer[harq_idx][round] = 0;
          }

          phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts_last[harq_idx][round] =
            phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts[harq_idx][round];
          phy_vars_eNB->eNB_UE_stats[i].ulsch_round_errors_last[harq_idx][round] =
            phy_vars_eNB->eNB_UE_stats[i].ulsch_round_errors[harq_idx][round];
        }
      }
    }

    if ((frame % 100 == 0) && (subframe==4)) {
      phy_vars_eNB->eNB_UE_stats[i].dlsch_bitrate = (phy_vars_eNB->eNB_UE_stats[i].total_TBS -
          phy_vars_eNB->eNB_UE_stats[i].total_TBS_last);

      phy_vars_eNB->eNB_UE_stats[i].total_TBS_last = phy_vars_eNB->eNB_UE_stats[i].total_TBS;
    }

    // CBA (non-LTE)
    cba_procedures(sched_subframe,phy_vars_eNB,i,harq_pid,abstraction_flag);
  } // loop i=0 ... NUMBER_OF_UE_MAX-1

  if (abstraction_flag == 0) {
    lte_eNB_I0_measurements(phy_vars_eNB,
			    subframe,
			    0,
			    phy_vars_eNB->first_run_I0_measurements);
    phy_vars_eNB->first_run_I0_measurements = 0;
  }

#ifdef PHY_ABSTRACTION
    else {
      lte_eNB_I0_measurements_emul(phy_vars_eNB,
                                   0);
    }

#endif


    //}

#ifdef EMOS
  phy_procedures_emos_eNB_RX(subframe,phy_vars_eNB);
#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_RX,0);
  stop_meas(&phy_vars_eNB->phy_proc_rx);

}

#undef DEBUG_PHY_PROC

#ifdef Rel10
int phy_procedures_RN_eNB_TX(unsigned char last_slot, unsigned char next_slot, relaying_type_t r_type)
{

  int do_proc=0;// do nothing

  switch(r_type) {
  case no_relay:
    do_proc= no_relay; // perform the normal eNB operation
    break;

  case multicast_relay:
    if (((next_slot >>1) < 6) || ((next_slot >>1) > 8))
      do_proc = 0; // do nothing
    else // SF#6, SF#7 and SF#8
      do_proc = multicast_relay; // do PHY procedures eNB TX

    break;

  default: // should'not be here
    LOG_W(PHY,"Not supported relay type %d, do nothing\n", r_type);
    do_proc=0;
    break;
  }

  return do_proc;
}
#endif
void phy_procedures_eNB_lte(unsigned char subframe,PHY_VARS_eNB **phy_vars_eNB,uint8_t abstraction_flag,
                            relaying_type_t r_type, PHY_VARS_RN *phy_vars_rn)
{
#if defined(ENABLE_ITTI)
  MessageDef   *msg_p;
  const char   *msg_name;
  instance_t    instance;
  unsigned int  Mod_id;
  int           result;
#endif


  int CC_id=0;

  /*
    if (phy_vars_eNB->proc[sched_subframe].frame_tx >= 1000)
    mac_xface->macphy_exit("Exiting after 1000 Frames\n");
  */
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX_ENB, phy_vars_eNB[0]->proc[subframe].frame_tx);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_LTE,1);
  start_meas(&phy_vars_eNB[0]->phy_proc);

#if defined(ENABLE_ITTI)

  do {
    // Checks if a message has been sent to PHY sub-task
    itti_poll_msg (TASK_PHY_ENB, &msg_p);

    if (msg_p != NULL) {
      msg_name = ITTI_MSG_NAME (msg_p);
      instance = ITTI_MSG_INSTANCE (msg_p);
      Mod_id = instance;

      switch (ITTI_MSG_ID(msg_p)) {
#   if ENABLE_RAL

      case TIMER_HAS_EXPIRED:
        // check if it is a measurement timer
      {
        hashtable_rc_t       hashtable_rc;

        hashtable_rc = hashtable_is_key_exists(PHY_vars_eNB_g[Mod_id][0]->ral_thresholds_timed, (uint64_t)(TIMER_HAS_EXPIRED(msg_p).timer_id));

        if (hashtable_rc == HASH_TABLE_OK) {
          phy_eNB_lte_check_measurement_thresholds(instance, (ral_threshold_phy_t*)TIMER_HAS_EXPIRED(msg_p).arg);
        }
      }
      break;


      case PHY_MEAS_THRESHOLD_REQ:
#warning "TO DO LIST OF THRESHOLDS"
        LOG_D(PHY, "[ENB %d] Received %s\n", Mod_id, msg_name);
        {
          ral_threshold_phy_t* threshold_phy_p  = NULL;
          int                  index, res;
          long                 timer_id;
          hashtable_rc_t       hashtable_rc;

          switch (PHY_MEAS_THRESHOLD_REQ(msg_p).cfg_param.th_action) {

          case RAL_TH_ACTION_CANCEL_THRESHOLD:
            break;

          case RAL_TH_ACTION_SET_NORMAL_THRESHOLD:
          case RAL_TH_ACTION_SET_ONE_SHOT_THRESHOLD:
            for (index = 0; index < PHY_MEAS_THRESHOLD_REQ(msg_p).cfg_param.num_thresholds; index++) {
              threshold_phy_p                  = calloc(1, sizeof(ral_threshold_phy_t));
              threshold_phy_p->th_action       = PHY_MEAS_THRESHOLD_REQ(msg_p).cfg_param.th_action;
              memcpy(&threshold_phy_p->link_param.link_param_type,
                     &PHY_MEAS_THRESHOLD_REQ(msg_p).cfg_param.link_param_type,
                     sizeof(ral_link_param_type_t));

              memcpy(&threshold_phy_p->threshold,
                     &PHY_MEAS_THRESHOLD_REQ(msg_p).cfg_param.thresholds[index],
                     sizeof(ral_threshold_t));

              switch (PHY_MEAS_THRESHOLD_REQ(msg_p).cfg_param.union_choice) {

              case RAL_LINK_CFG_PARAM_CHOICE_TIMER_NULL:
                switch (PHY_MEAS_THRESHOLD_REQ(msg_p).cfg_param.link_param_type.choice) {
                case RAL_LINK_PARAM_TYPE_CHOICE_GEN:
                  SLIST_INSERT_HEAD(
                    &PHY_vars_eNB_g[Mod_id][0]->ral_thresholds_gen_polled[PHY_MEAS_THRESHOLD_REQ(msg_p).cfg_param.link_param_type._union.link_param_gen],
                    threshold_phy_p,
                    ral_thresholds);
                  break;

                case RAL_LINK_PARAM_TYPE_CHOICE_LTE:
                  SLIST_INSERT_HEAD(
                    &PHY_vars_eNB_g[Mod_id][0]->ral_thresholds_lte_polled[PHY_MEAS_THRESHOLD_REQ(msg_p).cfg_param.link_param_type._union.link_param_lte],
                    threshold_phy_p,
                    ral_thresholds);
                  break;

                default:
                  LOG_E(PHY, "[ENB %d] BAD PARAMETER cfg_param.link_param_type.choice %d in %s\n",
                        Mod_id, PHY_MEAS_THRESHOLD_REQ(msg_p).cfg_param.link_param_type.choice, msg_name);
                }

                break;

              case RAL_LINK_CFG_PARAM_CHOICE_TIMER:
                res = timer_setup(
                        (uint32_t)(PHY_MEAS_THRESHOLD_REQ(msg_p).cfg_param._union.timer_interval/1000),//uint32_t      interval_sec,
                        (uint32_t)(PHY_MEAS_THRESHOLD_REQ(msg_p).cfg_param._union.timer_interval%1000),//uint32_t      interval_us,
                        TASK_PHY_ENB,
                        instance,
                        TIMER_PERIODIC,
                        threshold_phy_p,
                        &timer_id);

                if (res == 0) {
                  hashtable_rc = hashtable_insert(PHY_vars_eNB_g[Mod_id][0]->ral_thresholds_timed, (uint64_t )timer_id, (void*)threshold_phy_p);

                  if (hashtable_rc == HASH_TABLE_OK) {
                    threshold_phy_p->timer_id = timer_id;
                  } else {
                    LOG_E(PHY, "[ENB %d]  %s: Error in hashtable. Could not configure threshold index %d \n",
                          Mod_id, msg_name, index);
                  }

                } else {
                  LOG_E(PHY, "[ENB %d]  %s: Could not configure threshold index %d because of timer initialization failure\n",
                        Mod_id, msg_name, index);
                }

                break;

              default: // already checked in RRC, should not happen here
                LOG_E(PHY, "[ENB %d] BAD PARAMETER cfg_param.union_choice %d in %s\n",
                      Mod_id, PHY_MEAS_THRESHOLD_REQ(msg_p).cfg_param.union_choice, msg_name);
              }
            }

            break;

          default:
            LOG_E(PHY, "[ENB %d] BAD PARAMETER th_action value %d in %s\n",
                  Mod_id, PHY_MEAS_THRESHOLD_REQ(msg_p).cfg_param.th_action, msg_name);
          }

        }
        break;
#   endif

        /* Messages from eNB app */
      case PHY_CONFIGURATION_REQ:
        LOG_I(PHY, "[eNB %d] Received %s\n", instance, msg_name);
        /* TODO */

        break;

      default:
        LOG_E(PHY, "[ENB %d] Received unexpected message %s\n", Mod_id, msg_name);
        break;
      }

      result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
      AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    }
  } while(msg_p != NULL);

#endif


  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    if ((((phy_vars_eNB[CC_id]->lte_frame_parms.frame_type == TDD)&&
          (subframe_select(&phy_vars_eNB[CC_id]->lte_frame_parms,phy_vars_eNB[CC_id]->proc[subframe].subframe_tx)==SF_DL))||
         (phy_vars_eNB[CC_id]->lte_frame_parms.frame_type == FDD))) {
#ifdef Rel10

      if (phy_procedures_RN_eNB_TX(phy_vars_eNB[CC_id]->proc[subframe].subframe_rx, phy_vars_eNB[CC_id]->proc[subframe].subframe_tx, r_type) != 0 )
#endif
        phy_procedures_eNB_TX(subframe,phy_vars_eNB[CC_id],abstraction_flag,r_type,phy_vars_rn);
    }

    if ((((phy_vars_eNB[CC_id]->lte_frame_parms.frame_type == TDD )&&
          (subframe_select(&phy_vars_eNB[CC_id]->lte_frame_parms,phy_vars_eNB[CC_id]->proc[subframe].subframe_rx)==SF_UL)) ||
         (phy_vars_eNB[CC_id]->lte_frame_parms.frame_type == FDD))) {
      phy_procedures_eNB_RX(subframe,phy_vars_eNB[CC_id],abstraction_flag,r_type);
    }

    if (subframe_select(&phy_vars_eNB[CC_id]->lte_frame_parms,phy_vars_eNB[CC_id]->proc[subframe].subframe_tx)==SF_S) {
#ifdef Rel10

      if (phy_procedures_RN_eNB_TX(subframe, subframe, r_type) != 0 )
#endif
        phy_procedures_eNB_TX(subframe,phy_vars_eNB[CC_id],abstraction_flag,r_type,phy_vars_rn);
    }

    if ((subframe_select(&phy_vars_eNB[CC_id]->lte_frame_parms,phy_vars_eNB[CC_id]->proc[subframe].subframe_rx)==SF_S)) {
      phy_procedures_eNB_S_RX(subframe,phy_vars_eNB[CC_id],abstraction_flag,r_type);
    }

    phy_vars_eNB[CC_id]->proc[subframe].frame_tx++;
    phy_vars_eNB[CC_id]->proc[subframe].frame_rx++;

    if (phy_vars_eNB[CC_id]->proc[subframe].frame_tx>=MAX_FRAME_NUMBER) // defined in impl_defs_top.h
      phy_vars_eNB[CC_id]->proc[subframe].frame_tx-=MAX_FRAME_NUMBER;

    if (phy_vars_eNB[CC_id]->proc[subframe].frame_rx>=MAX_FRAME_NUMBER)
      phy_vars_eNB[CC_id]->proc[subframe].frame_rx-=MAX_FRAME_NUMBER;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_LTE,0);
  stop_meas(&phy_vars_eNB[0]->phy_proc);
}

