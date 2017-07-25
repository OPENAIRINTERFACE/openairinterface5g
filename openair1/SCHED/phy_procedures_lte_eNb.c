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

/*! \file phy_procedures_lte_eNB.c
 * \brief Implementation of eNB procedures from 36.213 LTE specifications
 * \author R. Knopp, F. Kaltenberger, N. Nikaein, X. Foukas
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr,navid.nikaein@eurecom.fr, x.foukas@sms.ed.ac.uk
 * \note
 * \warning
 */

#include "PHY/defs.h"
#include "PHY/extern.h"
#include "SCHED/defs.h"
#include "SCHED/extern.h"
#include "nfapi_interface.h"

#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"

//#define DEBUG_PHY_PROC (Already defined in cmake)
//#define DEBUG_ULSCH

#include "LAYER2/MAC/extern.h"
#include "LAYER2/MAC/defs.h"

#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

#include "T.h"

#include "assertions.h"
#include "msc.h"

#include <time.h>

#if defined(ENABLE_ITTI)
#   include "intertask_interface.h"
#endif


#if defined(FLEXRAN_AGENT_SB_IF)
//Agent-related headers
#include "ENB_APP/flexran_agent_extern.h"
#include "ENB_APP/CONTROL_MODULES/MAC/flexran_agent_mac.h"
#include "LAYER2/MAC/flexran_agent_mac_proto.h"
#endif


#define NS_PER_SLOT 500000

#define PUCCH 1

void exit_fun(const char* s);

extern int exit_openair;
struct timespec start_fh, start_fh_prev;
int start_fh_sf, start_fh_prev_sf;
// Fix per CC openair rf/if device update
// extern openair0_device openair0;

unsigned char dlsch_input_buffer[2700] __attribute__ ((aligned(32)));
int eNB_sync_buffer0[640*6] __attribute__ ((aligned(32)));
int eNB_sync_buffer1[640*6] __attribute__ ((aligned(32)));
int *eNB_sync_buffer[2] = {eNB_sync_buffer0, eNB_sync_buffer1};

extern uint16_t hundred_times_log10_NPRB[100];

unsigned int max_peak_val;
int max_sync_pos;

int harq_pid_updated[NUMBER_OF_UE_MAX][8] = {{0}};
int harq_pid_round[NUMBER_OF_UE_MAX][8] = {{0}};

//DCI_ALLOC_t dci_alloc[8];

uint8_t is_SR_subframe(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,uint8_t UE_id)
{

  const int subframe = proc->subframe_rx;
  const int frame = proc->frame_rx;

  LOG_D(PHY,"[eNB %d][SR %x] Frame %d subframe %d Checking for SR TXOp(sr_ConfigIndex %d)\n",
        eNB->Mod_id,eNB->ulsch[UE_id]->rnti,frame,subframe,
        eNB->scheduling_request_config[UE_id].sr_ConfigIndex);

  if (eNB->scheduling_request_config[UE_id].sr_ConfigIndex <= 4) {        // 5 ms SR period
    if ((subframe%5) == eNB->scheduling_request_config[UE_id].sr_ConfigIndex)
      return(1);
  } else if (eNB->scheduling_request_config[UE_id].sr_ConfigIndex <= 14) { // 10 ms SR period
    if (subframe==(eNB->scheduling_request_config[UE_id].sr_ConfigIndex-5))
      return(1);
  } else if (eNB->scheduling_request_config[UE_id].sr_ConfigIndex <= 34) { // 20 ms SR period
    if ((10*(frame&1)+subframe) == (eNB->scheduling_request_config[UE_id].sr_ConfigIndex-15))
      return(1);
  } else if (eNB->scheduling_request_config[UE_id].sr_ConfigIndex <= 74) { // 40 ms SR period
    if ((10*(frame&3)+subframe) == (eNB->scheduling_request_config[UE_id].sr_ConfigIndex-35))
      return(1);
  } else if (eNB->scheduling_request_config[UE_id].sr_ConfigIndex <= 154) { // 80 ms SR period
    if ((10*(frame&7)+subframe) == (eNB->scheduling_request_config[UE_id].sr_ConfigIndex-75))
      return(1);
  }

  return(0);
}


int32_t add_ue(int16_t rnti, PHY_VARS_eNB *eNB)
{
  uint8_t i;


  LOG_D(PHY,"[eNB %d/%d] Adding UE with rnti %x\n",
        eNB->Mod_id,
        eNB->CC_id,
        (uint16_t)rnti);

  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    if ((eNB->dlsch[i]==NULL) || (eNB->ulsch[i]==NULL)) {
      MSC_LOG_EVENT(MSC_PHY_ENB, "0 Failed add ue %"PRIx16" (ENOMEM)", rnti);
      LOG_E(PHY,"Can't add UE, not enough memory allocated\n");
      return(-1);
    } else {
      if (eNB->UE_stats[i].crnti==0) {
        MSC_LOG_EVENT(MSC_PHY_ENB, "0 Add ue %"PRIx16" ", rnti);
        LOG_D(PHY,"UE_id %d associated with rnti %x\n",i, (uint16_t)rnti);
        eNB->dlsch[i][0]->rnti = rnti;
        eNB->ulsch[i]->rnti = rnti;
        eNB->UE_stats[i].crnti = rnti;

	eNB->UE_stats[i].Po_PUCCH1_below = 0;
	eNB->UE_stats[i].Po_PUCCH1_above = (int32_t)pow(10.0,.1*(eNB->frame_parms.ul_power_control_config_common.p0_NominalPUCCH+eNB->rx_total_gain_dB));
	eNB->UE_stats[i].Po_PUCCH        = (int32_t)pow(10.0,.1*(eNB->frame_parms.ul_power_control_config_common.p0_NominalPUCCH+eNB->rx_total_gain_dB));
	LOG_D(PHY,"Initializing Po_PUCCH: p0_NominalPUCCH %d, gain %d => %d\n",
	      eNB->frame_parms.ul_power_control_config_common.p0_NominalPUCCH,
	      eNB->rx_total_gain_dB,
	      eNB->UE_stats[i].Po_PUCCH);
  
        return(i);
      }
    }
  }
  return(-1);
}

int mac_phy_remove_ue(module_id_t Mod_idP,rnti_t rntiP) {
  uint8_t i;
  int CC_id;
  PHY_VARS_eNB *eNB;

  for (CC_id=0;CC_id<MAX_NUM_CCs;CC_id++) {
    eNB = RC.eNB[Mod_idP][CC_id];
    for (i=0; i<NUMBER_OF_UE_MAX; i++) {
      if ((eNB->dlsch[i]==NULL) || (eNB->ulsch[i]==NULL)) {
	MSC_LOG_EVENT(MSC_PHY_ENB, "0 Failed remove ue %"PRIx16" (ENOMEM)", rntiP);
	LOG_E(PHY,"Can't remove UE, not enough memory allocated\n");
	return(-1);
      } else {
	if (eNB->UE_stats[i].crnti==rntiP) {
	  MSC_LOG_EVENT(MSC_PHY_ENB, "0 Removed ue %"PRIx16" ", rntiP);

	  LOG_D(PHY,"eNB %d removing UE %d with rnti %x\n",eNB->Mod_id,i,rntiP);

	  //LOG_D(PHY,("[PHY] UE_id %d\n",i);
	  clean_eNb_dlsch(eNB->dlsch[i][0]);
	  clean_eNb_ulsch(eNB->ulsch[i]);
	  //eNB->UE_stats[i].crnti = 0;
	  memset(&eNB->UE_stats[i],0,sizeof(LTE_eNB_UE_stats));
	  //  mac_exit_wrapper("Removing UE");
	  

	  return(i);
	}
      }
    }
  }
  MSC_LOG_EVENT(MSC_PHY_ENB, "0 Failed remove ue %"PRIx16" (not found)", rntiP);
  return(-1);
}

/*
int8_t find_next_ue_index(PHY_VARS_eNB *eNB)
{
  uint8_t i;

  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    if (eNB->UE_stats[i].crnti==0) {
	(eNB->dlsch[i][0]->rnti==0))
      LOG_D(PHY,"Next free UE id is %d\n",i);
      return(i);
    }
  }

  return(-1);
}
*/

/*
int get_ue_active_harq_pid(const uint8_t Mod_id,const uint8_t CC_id,const uint16_t rnti, const int frame, const uint8_t subframe,uint8_t *harq_pid,uint8_t *round,const uint8_t harq_flag)
{
  LTE_eNB_DLSCH_t *DLSCH_ptr;
  LTE_eNB_ULSCH_t *ULSCH_ptr;
  uint8_t ulsch_subframe,ulsch_frame;
  int i;
  int8_t UE_id = find_ue(rnti,RC.eNB[Mod_id][CC_id]);

  if (UE_id==-1) {
    LOG_D(PHY,"Cannot find UE with rnti %x (Mod_id %d, CC_id %d)\n",rnti, Mod_id, CC_id);
    *round=0;
    return(-1);
  }

  if ((harq_flag == openair_harq_DL) || (harq_flag == openair_harq_RA))  {// this is a DL request

    DLSCH_ptr = RC.eNB[Mod_id][CC_id]->dlsch[(uint32_t)UE_id][0];

    if (harq_flag == openair_harq_RA) {
      if (DLSCH_ptr->harq_processes[0] != NULL) {
	*harq_pid = 0;
	*round = DLSCH_ptr->harq_processes[0]->round;
	return 0;
      } else {
	return -1;
      }
    }

    // let's go synchronous for the moment - maybe we can change at some point
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

    ULSCH_ptr = RC.eNB[Mod_id][CC_id]->ulsch[(uint32_t)UE_id];
    ulsch_subframe = pdcch_alloc2ul_subframe(&RC.eNB[Mod_id][CC_id]->frame_parms,subframe);
    ulsch_frame    = pdcch_alloc2ul_frame(&RC.eNB[Mod_id][CC_id]->frame_parms,frame,subframe);
    // Note this is for TDD configuration 3,4,5 only
    *harq_pid = subframe2harq_pid(&RC.eNB[Mod_id][CC_id]->frame_parms,
                                  ulsch_frame,
                                  ulsch_subframe);
    *round    = ULSCH_ptr->harq_processes[*harq_pid]->round;
    LOG_T(PHY,"[eNB %d][PUSCH %d] Frame %d subframe %d Checking HARQ, round %d\n",Mod_id,*harq_pid,frame,subframe,*round);
  }

  return(0);
}
*/

int16_t get_target_pusch_rx_power(const module_id_t module_idP, const uint8_t CC_id)
{
  return RC.eNB[module_idP][CC_id]->frame_parms.ul_power_control_config_common.p0_NominalPUSCH;
}

int16_t get_target_pucch_rx_power(const module_id_t module_idP, const uint8_t CC_id)
{
  return RC.eNB[module_idP][CC_id]->frame_parms.ul_power_control_config_common.p0_NominalPUCCH;
}

void phy_procedures_eNB_S_RX(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,relaying_type_t r_type)
{
  UNUSED(r_type);
  int subframe = proc->subframe_rx;

#ifdef DEBUG_PHY_PROC
  LOG_D(PHY,"[eNB %d] Frame %d: Doing phy_procedures_eNB_S_RX(%d)\n", eNB->Mod_id,proc->frame_rx, subframe);
#endif


  lte_eNB_I0_measurements(eNB,
			  subframe,
			  0,
			  eNB->first_run_I0_measurements);

}



#define AMP_OVER_SQRT2 ((AMP*ONE_OVER_SQRT2_Q15)>>15)
#define AMP_OVER_2 (AMP>>1)
int QPSK[4]= {AMP_OVER_SQRT2|(AMP_OVER_SQRT2<<16),AMP_OVER_SQRT2|((65536-AMP_OVER_SQRT2)<<16),((65536-AMP_OVER_SQRT2)<<16)|AMP_OVER_SQRT2,((65536-AMP_OVER_SQRT2)<<16)|(65536-AMP_OVER_SQRT2)};
int QPSK2[4]= {AMP_OVER_2|(AMP_OVER_2<<16),AMP_OVER_2|((65536-AMP_OVER_2)<<16),((65536-AMP_OVER_2)<<16)|AMP_OVER_2,((65536-AMP_OVER_2)<<16)|(65536-AMP_OVER_2)};



unsigned int taus(void);


void pmch_procedures(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,PHY_VARS_RN *rn,relaying_type_t r_type) {


#if defined(Rel10) || defined(Rel14)
  MCH_PDU *mch_pduP;
  MCH_PDU  mch_pdu;
  //  uint8_t sync_area=255;
#endif

  int subframe = proc->subframe_tx;

  // This is DL-Cell spec pilots in Control region
  generate_pilots_slot(eNB,
		       eNB->common_vars.txdataF,
		       AMP,
		       subframe<<1,1);

  
#if defined(Rel10) || defined(Rel14)
  // if mcch is active, send regardless of the node type: eNB or RN
  // when mcch is active, MAC sched does not allow MCCH and MTCH multiplexing
  /*
  mch_pduP = mac_xface->get_mch_sdu(eNB->Mod_id,
				    eNB->CC_id,
				    proc->frame_tx,
				    subframe);
  */
  switch (r_type) {
  case no_relay:
    if ((mch_pduP->Pdu_size > 0) && (mch_pduP->sync_area == 0)) // TEST: only transmit mcch for sync area 0
      LOG_I(PHY,"[eNB%"PRIu8"] Frame %d subframe %d : Got MCH pdu for MBSFN (MCS %"PRIu8", TBS %d) \n",
	    eNB->Mod_id,proc->frame_tx,subframe,mch_pduP->mcs,
	    eNB->dlsch_MCH->harq_processes[0]->TBS>>3);
    else {
      LOG_D(PHY,"[DeNB %"PRIu8"] Frame %d subframe %d : Do not transmit MCH pdu for MBSFN sync area %"PRIu8" (%s)\n",
	    eNB->Mod_id,proc->frame_tx,subframe,mch_pduP->sync_area,
	    (mch_pduP->Pdu_size == 0)? "Empty MCH PDU":"Let RN transmit for the moment");
      mch_pduP = NULL;
    }
    
    break;
    
  case multicast_relay:
    if ((mch_pduP->Pdu_size > 0) && ((mch_pduP->mcch_active == 1) || mch_pduP->msi_active==1)) {
      LOG_I(PHY,"[RN %"PRIu8"] Frame %d subframe %d: Got the MCH PDU for MBSFN  sync area %"PRIu8" (MCS %"PRIu8", TBS %"PRIu16")\n",
	    rn->Mod_id,rn->frame, subframe,
	    mch_pduP->sync_area,mch_pduP->mcs,mch_pduP->Pdu_size);
    } else if (rn->mch_avtive[subframe%5] == 1) { // SF2 -> SF7, SF3 -> SF8
      mch_pduP= &mch_pdu;
      memcpy(&mch_pduP->payload, // could be a simple copy
	     rn->dlsch_rn_MCH[subframe%5]->harq_processes[0]->b,
	     rn->dlsch_rn_MCH[subframe%5]->harq_processes[0]->TBS>>3);
      mch_pduP->Pdu_size = (uint16_t) (rn->dlsch_rn_MCH[subframe%5]->harq_processes[0]->TBS>>3);
      mch_pduP->mcs = rn->dlsch_rn_MCH[subframe%5]->harq_processes[0]->mcs;
      LOG_I(PHY,"[RN %"PRIu8"] Frame %d subframe %d: Forward the MCH PDU for MBSFN received on SF %d sync area %"PRIu8" (MCS %"PRIu8", TBS %"PRIu16")\n",
	    rn->Mod_id,rn->frame, subframe,subframe%5,
	    rn->sync_area[subframe%5],mch_pduP->mcs,mch_pduP->Pdu_size);
    } else {
      mch_pduP=NULL;
    }
    
    rn->mch_avtive[subframe]=0;
    break;
    
  default:
    LOG_W(PHY,"[eNB %"PRIu8"] Frame %d subframe %d: unknown relaying type %d \n",
	  eNB->Mod_id,proc->frame_tx,subframe,r_type);
    mch_pduP=NULL;
    break;
  }// switch
  
  if (mch_pduP) {
    fill_eNB_dlsch_MCH(eNB,mch_pduP->mcs,1,0);
    // Generate PMCH
    generate_mch(eNB,proc,(uint8_t*)mch_pduP->payload);
  } else {
    LOG_D(PHY,"[eNB/RN] Frame %d subframe %d: MCH not generated \n",proc->frame_tx,subframe);
  }
  
#endif
}

void common_signal_procedures (PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc) {

  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int **txdataF = eNB->common_vars.txdataF;
  uint8_t *pbch_pdu=&eNB->pbch_pdu[0];
  int subframe = proc->subframe_tx;
  int frame = proc->frame_tx;

  LOG_D(PHY,"common_signal_procedures: frame %d, subframe %d\n",frame,subframe); 

  // generate Cell-Specific Reference Signals for both slots
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_RS_TX,1);
  generate_pilots_slot(eNB,
		       txdataF,
		       AMP,
		       subframe<<1,0);
  // check that 2nd slot is for DL
  if (subframe_select(fp,subframe) == SF_DL)
    generate_pilots_slot(eNB,
			 txdataF,
			 AMP,
			 (subframe<<1)+1,0);
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_RS_TX,0);
      

  // First half of PSS/SSS (FDD, slot 0)
  if (subframe == 0) {
    if (fp->frame_type == FDD) {
      generate_pss(txdataF,
		   AMP,
		   fp,
		   (fp->Ncp==NORMAL) ? 6 : 5,
		   0);
      generate_sss(txdataF,
		   AMP,
		   fp,
		   (fp->Ncp==NORMAL) ? 5 : 4,
		   0);
      
    }
    


      
    /// First half of SSS (TDD, slot 1)
    
    if (fp->frame_type == TDD) {
      generate_sss(txdataF,
		   AMP,
		   fp,
		   (fp->Ncp==NORMAL) ? 6 : 5,
		   1);
    }

    // generate PBCH (Physical Broadcast CHannel) info

    /// generate PBCH
    if ((frame&3)==0) {
      AssertFatal(eNB->pbch_configured==1,"PBCH was not configured by MAC\n");
      eNB->pbch_configured=0;
    }
    generate_pbch(&eNB->pbch,
		  txdataF,
		  AMP,
		  fp,
		  pbch_pdu,
		  frame&3);
  
  }
  else if ((subframe == 1) &&
	   (fp->frame_type == TDD)){
    generate_pss(txdataF,
		 AMP,
		 fp,
		 2,
		 2);
  }
  
  // Second half of PSS/SSS (FDD, slot 10)
  else if ((subframe == 5) && 
	   (fp->frame_type == FDD)) {
    generate_pss(txdataF,
		 AMP,
		 &eNB->frame_parms,
		 (fp->Ncp==NORMAL) ? 6 : 5,
		 10);
    generate_sss(txdataF,
		 AMP,
		 &eNB->frame_parms,
		 (fp->Ncp==NORMAL) ? 5 : 4,
		 10);

  }

  //  Second-half of SSS (TDD, slot 11)
  else if ((subframe == 5) &&
	   (fp->frame_type == TDD)) {
    generate_sss(txdataF,
		 AMP,
		 fp,
		 (fp->Ncp==NORMAL) ? 6 : 5,
		 11);
  }

  // Second half of PSS (TDD, slot 12)
  else if ((subframe == 6) &&
	   (fp->frame_type == TDD)) { 
    generate_pss(txdataF,
		 AMP,
		 fp,
		 2,
		 12);
  }

}

/*
void generate_eNB_dlsch_params(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,DCI_ALLOC_t *dci_alloc,const int UE_id) {

  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int frame = proc->frame_tx;
  int subframe = proc->subframe_tx;

  // if we have SI_RNTI, configure dlsch parameters and CCE index
  if (dci_alloc->rnti == SI_RNTI) {
    LOG_D(PHY,"Generating dlsch params for SI_RNTI\n");
    
    generate_eNB_dlsch_params_from_dci(frame,
				       subframe,
				       &dci_alloc->dci_pdu[0],
				       dci_alloc->rnti,
				       dci_alloc->format,
				       &eNB->dlsch_SI,
				       fp,
				       NULL,
				       SI_RNTI,
				       0,
				       P_RNTI,
				       eNB->UE_stats[0].DL_pmi_single,
				       0);
    LOG_I(PHY,"frame %d, subframe %d : SI with mcs %d\n",frame,subframe,eNB->dlsch_SI->harq_processes[0]->mcs);
    
    eNB->dlsch_SI->nCCE[subframe] = dci_alloc->firstCCE;
   
    LOG_I(PHY,"[eNB %"PRIu8"] Frame %d subframe %d : CCE resource for common DCI (SI)  => %"PRIu8", aggregation %d\n",eNB->Mod_id,frame,subframe,
	  eNB->dlsch_SI->nCCE[subframe],1<<dci_alloc->L);
    
  } else if (dci_alloc->ra_flag == 1) {  // This is format 1A allocation for RA
    // configure dlsch parameters and CCE index
    LOG_D(PHY,"Generating dlsch params for RA_RNTI\n");    
    
    generate_eNB_dlsch_params_from_dci(frame,
				       subframe,
				       &dci_alloc->dci_pdu[0],
				       dci_alloc->rnti,
				       dci_alloc->format,
				       &eNB->dlsch_ra,
				       fp,
				       NULL,
				       SI_RNTI,
				       dci_alloc->rnti,
				       P_RNTI,
				       eNB->UE_stats[0].DL_pmi_single,
				       0);
    
        
    eNB->dlsch_ra->nCCE[subframe] = dci_alloc->firstCCE;
    
    LOG_I(PHY,"[eNB %"PRIu8"] Frame %d subframe %d : CCE resource for common DCI (RA)  => %"PRIu8" length %d bits\n",eNB->Mod_id,frame,subframe,
	  eNB->dlsch_ra->nCCE[subframe],
	  dci_alloc->dci_length);
  }
  
  else if ((dci_alloc->format != format0)&&
	   (dci_alloc->format != format3)&&
	   (dci_alloc->format != format3A)&&
	   (dci_alloc->format != format4)){ // this is a normal DLSCH allocation
    

    
    if (UE_id>=0) {
      LOG_I(PHY,"Generating dlsch params for RNTI %x\n",dci_alloc->rnti);      
      
      generate_eNB_dlsch_params_from_dci(frame,
					 subframe,
					 &dci_alloc->dci_pdu[0],
					 dci_alloc->rnti,
					 dci_alloc->format,
					 eNB->dlsch[(uint8_t)UE_id],
					 fp,
					 &eNB->pdsch_config_dedicated[UE_id],
					 SI_RNTI,
					 0,
					 P_RNTI,
					 eNB->UE_stats[(uint8_t)UE_id].DL_pmi_single,
					 eNB->transmission_mode[(uint8_t)UE_id]<7?0:eNB->transmission_mode[(uint8_t)UE_id]);
      LOG_D(PHY,"[eNB %"PRIu8"][PDSCH %"PRIx16"/%"PRIu8"] Frame %d subframe %d: Generated dlsch params\n",
	    eNB->Mod_id,dci_alloc->rnti,eNB->dlsch[(uint8_t)UE_id][0]->current_harq_pid,frame,subframe);
      
      
      T(T_ENB_PHY_DLSCH_UE_DCI, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(UE_id),
        T_INT(dci_alloc->rnti), T_INT(dci_alloc->format),
        T_INT(dci_alloc->harq_pid),
        T_INT(eNB->dlsch[(int)UE_id][0]->harq_processes[dci_alloc->harq_pid]->mcs),
        T_INT(eNB->dlsch[(int)UE_id][0]->harq_processes[dci_alloc->harq_pid]->TBS));

      eNB->dlsch[(uint8_t)UE_id][0]->nCCE[subframe] = dci_alloc->firstCCE;
      
      LOG_I(PHY,"[eNB %"PRIu8"] Frame %d subframe %d : CCE resource for ue DCI (PDSCH %"PRIx16")  => %"PRIu8" length %d bits\n",eNB->Mod_id,frame,subframe,
	    dci_alloc->rnti,eNB->dlsch[(uint8_t)UE_id][0]->nCCE[subframe],dci_alloc->dci_length);
      
      LOG_D(PHY,"[eNB %"PRIu8"][DCI][PDSCH %"PRIx16"] Frame %d subframe %d UE_id %"PRId8" Generated DCI format %d, aggregation %d\n",
	    eNB->Mod_id, dci_alloc->rnti,
	    frame, subframe,UE_id,
	    dci_alloc->format,
	    1<<dci_alloc->L);
    } else {
      LOG_D(PHY,"[eNB %"PRIu8"][PDSCH] Frame %d : No UE_id with corresponding rnti %"PRIx16", dropping DLSCH\n",
	    eNB->Mod_id,frame,dci_alloc->rnti);
    }
  }
  
}

void generate_eNB_ulsch_params(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,DCI_ALLOC_t *dci_alloc,const int UE_id) {

  int harq_pid;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int frame = proc->frame_tx;
  int subframe = proc->subframe_tx;
  
//  uint16_t srsPeriodicity=0;
//  uint16_t srsOffset=0;
//  uint16_t srsConfigIndex=0;
//  uint16_t do_srs=0;
  
  uint16_t is_srs_pos=0;

  LOG_I(PHY,
	"[eNB %"PRIu8"][PUSCH %"PRIu8"] Frame %d subframe %d UL Frame %"PRIu32", UL Subframe %"PRIu8", Generated ULSCH (format0) DCI (rnti %"PRIx16", dci %"PRIx8"), aggregation %d\n",
	eNB->Mod_id,
	subframe2harq_pid(fp,
			  pdcch_alloc2ul_frame(fp,frame,subframe),
			  pdcch_alloc2ul_subframe(fp,subframe)),
	frame,
	subframe,
	pdcch_alloc2ul_frame(fp,frame,subframe),
	pdcch_alloc2ul_subframe(fp,subframe),
	dci_alloc->rnti,
	dci_alloc->dci_pdu[0],
	1<<dci_alloc->L);
  
  is_srs_pos = is_srs_occasion_common(fp,pdcch_alloc2ul_frame(fp,frame,subframe),pdcch_alloc2ul_subframe(fp,subframe));
  
//if (is_srs_pos && eNB->soundingrs_ul_config_dedicated[UE_id].srsConfigDedicatedSetup) {
//    srsConfigIndex = eNB->soundingrs_ul_config_dedicated[UE_id].srs_ConfigIndex;
//    compute_srs_pos(fp->frame_type, srsConfigIndex, &srsPeriodicity, &srsOffset);
//    if ((((10*pdcch_alloc2ul_frame(fp,frame,subframe)+pdcch_alloc2ul_subframe(fp,subframe)) % srsPeriodicity) == srsOffset)) {
//     do_srs = 1;
//    }
//  }
//      LOG_D(PHY,"frame %d (%d), subframe %d (%d), UE_id %d: is_srs_pos %d, do_SRS %d, index %d, period %d, offset %d \n",
//	    frame,pdcch_alloc2ul_frame(fp,frame,subframe),subframe,pdcch_alloc2ul_subframe(fp,subframe),
//	    UE_id,is_srs_pos,do_srs,srsConfigIndex,srsPeriodicity,srsOffset);
//  

  generate_eNB_ulsch_params_from_dci(eNB,
				     proc,
				     &dci_alloc->dci_pdu[0],
				     dci_alloc->rnti,
				     format0,
				     UE_id,
				     SI_RNTI,
				     0,
				     P_RNTI,
				     CBA_RNTI,
				     is_srs_pos);  
  LOG_T(PHY,"[eNB %"PRIu8"] Frame %d subframe %d : CCE resources for UE spec DCI (PUSCH %"PRIx16") => %d\n",
	eNB->Mod_id,frame,subframe,dci_alloc->rnti,
	dci_alloc->firstCCE);
  
  // get the hard_pid for this subframe 
  harq_pid = subframe2harq_pid(fp,
			       pdcch_alloc2ul_frame(fp,frame,subframe),
			       pdcch_alloc2ul_subframe(fp,subframe));
  
  AssertFatal(harq_pid!=255,"[eNB %"PRIu8"] Frame %d: Bad harq_pid for ULSCH allocation\n",eNB->Mod_id,frame);

  eNB->ulsch[(uint32_t)UE_id]->harq_processes[harq_pid]->subframe_scheduling_flag = 1;
  
  T(T_ENB_PHY_ULSCH_UE_DCI, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(UE_id),
    T_INT(dci_alloc->rnti), T_INT(harq_pid),
    T_INT(eNB->ulsch[(uint32_t)UE_id]->harq_processes[harq_pid]->mcs),
    T_INT(eNB->ulsch[(uint32_t)UE_id]->harq_processes[harq_pid]->round),
    T_INT(eNB->ulsch[(uint32_t)UE_id]->harq_processes[harq_pid]->first_rb),
    T_INT(eNB->ulsch[(uint32_t)UE_id]->harq_processes[harq_pid]->nb_rb),
    T_INT(eNB->ulsch[(uint32_t)UE_id]->harq_processes[harq_pid]->TBS),
    T_INT(dci_alloc->L),
    T_INT(dci_alloc->firstCCE));
}
*/

void pdsch_procedures(PHY_VARS_eNB *eNB,
		      eNB_rxtx_proc_t *proc,
		      int harq_pid,
		      LTE_eNB_DLSCH_t *dlsch, 
		      LTE_eNB_DLSCH_t *dlsch1,
		      LTE_eNB_UE_stats *ue_stats,
		      int ra_flag,
		      int num_pdcch_symbols) {

  int frame=proc->frame_tx;
  int subframe=proc->subframe_tx;
  LTE_DL_eNB_HARQ_t *dlsch_harq=dlsch->harq_processes[harq_pid];
  int input_buffer_length = dlsch_harq->TBS/8;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int i;

  LOG_I(PHY,
	"[eNB %"PRIu8"][PDSCH %"PRIx16"/%"PRIu8"] Frame %d, subframe %d: Generating PDSCH/DLSCH with input size = %"PRIu16", G %d, nb_rb %"PRIu16", mcs %"PRIu8", pmi_alloc %"PRIx64", rv %"PRIu8" (round %"PRIu8")\n",
	eNB->Mod_id, dlsch->rnti,harq_pid,
	frame, subframe, input_buffer_length,
	get_G(fp,
	      dlsch_harq->nb_rb,
	      dlsch_harq->rb_alloc,
	      get_Qm(dlsch_harq->mcs),
	      dlsch_harq->Nl,
	      num_pdcch_symbols,
	      frame,
	      subframe,
	      dlsch_harq->mimo_mode==TM7?7:0),
	dlsch_harq->nb_rb,
	dlsch_harq->mcs,
	pmi2hex_2Ar1(dlsch_harq->pmi_alloc),
	dlsch_harq->rvidx,
	dlsch_harq->round);

#if defined(MESSAGE_CHART_GENERATOR_PHY)
  MSC_LOG_TX_MESSAGE(
		     MSC_PHY_ENB,MSC_PHY_UE,
		     NULL,0,
		     "%05u:%02u PDSCH/DLSCH input size = %"PRIu16", G %d, nb_rb %"PRIu16", mcs %"PRIu8", pmi_alloc %"PRIx16", rv %"PRIu8" (round %"PRIu8")",
		     frame, subframe,
		     input_buffer_length,
		     get_G(fp,
			   dlsch_harq->nb_rb,
			   dlsch_harq->rb_alloc,
			   get_Qm(dlsch_harq->mcs),
			   dlsch_harq->Nl,
			   num_pdcch_symbols,
			   frame,
			   subframe,
			   dlsch_harq->mimo_mode==TM7?7:0),
		     dlsch_harq->nb_rb,
		     dlsch_harq->mcs,
		     pmi2hex_2Ar1(dlsch_harq->pmi_alloc),
		     dlsch_harq->rvidx,
		     dlsch_harq->round);
#endif

  if (ue_stats) ue_stats->dlsch_sliding_cnt++;

  if (dlsch_harq->round == 0) {

    if (ue_stats)
      ue_stats->dlsch_trials[harq_pid][0]++;


    if (eNB->mac_enabled==1) {


      /*
      //      if (ra_flag == 0) {
      DLSCH_pdu = get_dlsch_pdu(eNB->Mod_id,
				eNB->CC_id,
				dlsch->rnti);

      
      }

      else {
	int16_t crnti = mac_xface->fill_rar(eNB->Mod_id,
					    eNB->CC_id,
					    frame,
					    DLSCH_pdu_rar,
					    fp->N_RB_UL,
					    input_buffer_length);
	DLSCH_pdu = DLSCH_pdu_rar;

	int UE_id;

	if (crnti!=0) 
	  UE_id = add_ue(crnti,eNB);
	else 
	  UE_id = -1;
	    
	if (UE_id==-1) {
	  LOG_W(PHY,"[eNB] Max user count reached.\n");
	  mac_xface->cancel_ra_proc(eNB->Mod_id,
				    eNB->CC_id,
				    frame,
				    crnti);
	} else {
	  eNB->UE_stats[(uint32_t)UE_id].mode = RA_RESPONSE;
	  // Initialize indicator for first SR (to be cleared after ConnectionSetup is acknowledged)
	  eNB->first_sr[(uint32_t)UE_id] = 1;
	      
	  generate_eNB_ulsch_params_from_rar(DLSCH_pdu,
					     frame,
					     subframe,
					     eNB->ulsch[(uint32_t)UE_id],
					     fp);
	      
	  LOG_D(PHY,"[eNB][RAPROC] Frame %d subframe %d, Activated Msg3 demodulation for UE %"PRId8" in frame %"PRIu32", subframe %"PRIu8"\n",
		frame,
		subframe,
		UE_id,
		eNB->ulsch[(uint32_t)UE_id]->Msg3_frame,
		eNB->ulsch[(uint32_t)UE_id]->Msg3_subframe);


          LOG_D(PHY, "hack: set phich_active to 0 for UE %d fsf %d %d all HARQs\n", UE_id, frame, subframe);
          for (i = 0; i < 8; i++)
            eNB->ulsch[(uint32_t)UE_id]->harq_processes[i]->phich_active = 0;

          mac_xface->set_msg3_subframe(eNB->Mod_id, eNB->CC_id, frame, subframe, (uint16_t)crnti,
                                       eNB->ulsch[UE_id]->Msg3_frame, eNB->ulsch[UE_id]->Msg3_subframe);

          T(T_ENB_PHY_MSG3_ALLOCATION, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe),
            T_INT(UE_id), T_INT((uint16_t)crnti), T_INT(1 // 1 is for initial transmission
	    ),
            T_INT(eNB->ulsch[UE_id]->Msg3_frame), T_INT(eNB->ulsch[UE_id]->Msg3_subframe));
	}
	if (ue_stats) ue_stats->total_TBS_MAC += dlsch_harq->TBS;
      */
    }
    else {
      for (i=0; i<input_buffer_length; i++)
	dlsch_harq->pdu[i] = (unsigned char)(taus()&0xff);
    }
  } else {
    ue_stats->dlsch_trials[harq_pid][dlsch_harq->round]++;
#ifdef DEBUG_PHY_PROC
#ifdef DEBUG_DLSCH
    LOG_D(PHY,"[eNB] This DLSCH is a retransmission\n");
#endif
#endif
  }


  LOG_D(PHY,"Generating DLSCH/PDSCH %d\n",ra_flag);
  // 36-212
  start_meas(&eNB->dlsch_encoding_stats);
  eNB->te(eNB,
	  dlsch_harq->pdu,
	  num_pdcch_symbols,
	  dlsch,
	  frame,subframe,
	  &eNB->dlsch_rate_matching_stats,
	  &eNB->dlsch_turbo_encoding_stats,
	  &eNB->dlsch_interleaving_stats);
  stop_meas(&eNB->dlsch_encoding_stats);
  // 36-211
  start_meas(&eNB->dlsch_scrambling_stats);
  dlsch_scrambling(fp,
		   0,
		   dlsch,
		   harq_pid,
		   get_G(fp,
			 dlsch_harq->nb_rb,
			 dlsch_harq->rb_alloc,
			 get_Qm(dlsch_harq->mcs),
			 dlsch_harq->Nl,
			 num_pdcch_symbols,
			 frame,subframe,
			 0),
		   0,
		   subframe<<1);
  stop_meas(&eNB->dlsch_scrambling_stats);
  
  start_meas(&eNB->dlsch_modulation_stats);
  
  
  dlsch_modulation(eNB,
		   eNB->common_vars.txdataF,
		   AMP,
		   subframe,
		   num_pdcch_symbols,
		   dlsch,
		   dlsch1);
  
  stop_meas(&eNB->dlsch_modulation_stats);

  dlsch->active = 0;
}


void handle_nfapi_dci_dl_pdu(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,nfapi_dl_config_request_pdu_t *dl_config_pdu);
void handle_nfapi_dci_dl_pdu(PHY_VARS_eNB *eNB,  
			     eNB_rxtx_proc_t *proc,
			     nfapi_dl_config_request_pdu_t *dl_config_pdu) {

  int idx                         = proc->subframe_tx&1;  
  LTE_eNB_PDCCH *pdcch_vars       = &eNB->pdcch_vars[idx];
  nfapi_dl_config_dci_dl_pdu *pdu = &dl_config_pdu->dci_dl_pdu;

  LOG_I(PHY,"Frame %d, Subframe %d: DCI processing\n",proc->frame_tx,proc->subframe_tx);

  // copy dci configuration into eNB structure
  fill_dci_and_dlsch(eNB,proc,&pdcch_vars->dci_alloc[pdcch_vars->num_dci],pdu);
}

void handle_nfapi_mpdcch_pdu(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,nfapi_dl_config_request_pdu_t *dl_config_pdu);
void handle_nfapi_mpdcch_pdu(PHY_VARS_eNB *eNB,  
			     eNB_rxtx_proc_t *proc,
			     nfapi_dl_config_request_pdu_t *dl_config_pdu) {

  int idx                         = proc->subframe_tx&1;  
  LTE_eNB_MPDCCH *mpdcch_vars     = &eNB->mpdcch_vars[idx];
  nfapi_dl_config_mpdcch_pdu *pdu = &dl_config_pdu->mpdcch_pdu;

  LOG_I(PHY,"Frame %d, Subframe %d: MDCI processing\n",proc->frame_tx,proc->subframe_tx);

  // copy dci configuration into eNB structure
  fill_mdci_and_dlsch(eNB,proc,&mpdcch_vars->mdci_alloc[mpdcch_vars->num_dci],pdu);
}

void handle_nfapi_hi_dci0_dci_pdu(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,
				  nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu);

void handle_nfapi_hi_dci0_dci_pdu(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,  
				  nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu) {
  
  int idx                         = proc->subframe_tx&1;
  LTE_eNB_PDCCH *pdcch_vars       = &eNB->pdcch_vars[idx];
  nfapi_hi_dci0_dci_pdu *pdu      = &hi_dci0_config_pdu->dci_pdu;
  // copy dci configuration in to eNB structure
  fill_dci_and_ulsch(eNB,proc,&pdcch_vars->dci_alloc[pdcch_vars->num_dci],pdu);
}

void handle_nfapi_hi_dci0_hi_pdu(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,
				 nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu);


void handle_nfapi_hi_dci0_hi_pdu(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,  
				 nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu) {
  
  nfapi_hi_dci0_hi_pdu *pdu      = &hi_dci0_config_pdu->hi_pdu;
  // copy dci configuration in to eNB structure
  LOG_I(PHY,"Received HI PDU which value %d (rbstart %d,cshift %d)\n",
	hi_dci0_config_pdu->hi_pdu.hi_pdu_rel8.hi_value,
	hi_dci0_config_pdu->hi_pdu.hi_pdu_rel8.resource_block_start,
	hi_dci0_config_pdu->hi_pdu.hi_pdu_rel8.cyclic_shift_2_for_drms);
}

handle_nfapi_bch_pdu(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,  
		     nfapi_dl_config_request_pdu_t *dl_config_pdu,
		     uint8_t *sdu) {

  nfapi_dl_config_bch_pdu_rel8_t *rel8 = &dl_config_pdu->bch_pdu.bch_pdu_rel8;
  
  AssertFatal(rel8->length == 3, "BCH PDU has length %d != 3\n",rel8->length);

  LOG_I(PHY,"bch_pdu: %x,%x,%x\n",sdu[0],sdu[1],sdu[2]);
  eNB->pbch_pdu[0] = sdu[2];
  eNB->pbch_pdu[1] = sdu[1];
  eNB->pbch_pdu[2] = sdu[0];

  // adjust transmit amplitude here based on NFAPI info

}

#ifdef Rel14
extern uint32_t localRIV2alloc_LUT6[32];
extern uint32_t localRIV2alloc_LUT25[512];
extern uint32_t localRIV2alloc_LUT50_0[1600];
extern uint32_t localRIV2alloc_LUT50_1[1600];
extern uint32_t localRIV2alloc_LUT100_0[6000];
extern uint32_t localRIV2alloc_LUT100_1[6000];
extern uint32_t localRIV2alloc_LUT100_2[6000];
extern uint32_t localRIV2alloc_LUT100_3[6000];
#endif

handle_nfapi_dlsch_pdu(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,  
		       nfapi_dl_config_request_pdu_t *dl_config_pdu,
		       uint8_t codeword_index,
		       uint8_t *sdu) {

  nfapi_dl_config_dlsch_pdu_rel8_t *rel8 = &dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8;
#ifdef Rel14
  nfapi_dl_config_dlsch_pdu_rel13_t *rel13 = &dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13;
#endif
  LTE_eNB_DLSCH_t *dlsch0=NULL,*dlsch1=NULL;
  LTE_DL_eNB_HARQ_t *dlsch0_harq=NULL,*dlsch1_harq=NULL;
  int UE_id;
  int harq_pid;

  
  UE_id = find_dlsch(rel8->rnti,eNB,SEARCH_EXIST_OR_FREE);
  AssertFatal(UE_id!=-1,"no free or exiting dlsch_context\n");
  AssertFatal(UE_id<NUMBER_OF_UE_MAX,"returned UE_id %d >= %d(NUMBER_OF_UE_MAX)\n",UE_id,NUMBER_OF_UE_MAX);
 
  dlsch0 = eNB->dlsch[UE_id][0];
  dlsch1 = eNB->dlsch[UE_id][1];

#ifdef Rel14
  if ((rel13->pdsch_payload_type == 0) && (rel13->ue_type>0)) dlsch0->harq_ids[proc->subframe_tx] = 0;
#endif

  harq_pid        = dlsch0->harq_ids[proc->subframe_tx];
  AssertFatal((harq_pid>=0) && (harq_pid<8),"harq_pid %d not in 0...7\n",harq_pid);
  dlsch0_harq     = dlsch0->harq_processes[harq_pid];
  dlsch1_harq     = dlsch1->harq_processes[harq_pid];
  AssertFatal(dlsch0_harq!=NULL,"dlsch_harq is null\n");

  LOG_I(PHY,"NFAPI: frame %d, subframe %d: programming dlsch, rnti %x, UE_id %d, harq_pid %d\n",
	proc->frame_tx,proc->subframe_tx,rel8->rnti,UE_id,harq_pid);
  if (codeword_index == 0) dlsch0_harq->pdu                    = sdu;
  else                     dlsch1_harq->pdu                    = sdu;

#ifdef Rel14
  if ((rel13->pdsch_payload_type == 0) && (rel13->ue_type>0)) { // this is a BR/CE UE and SIB1-BR
    // configure PDSCH
    switch (eNB->frame_parms.N_RB_DL) {
    case 6:
      dlsch0_harq->rb_alloc[0]      = localRIV2alloc_LUT6[rel8->resource_block_coding];
      break;
    case 15:
      AssertFatal(1==0,"15 PRBs not supported for now\n");
      break;
    case 25:
      dlsch0_harq->rb_alloc[0]      = localRIV2alloc_LUT25[rel8->resource_block_coding];
      break;
    case 50:  
      dlsch0_harq->rb_alloc[0]      = localRIV2alloc_LUT50_0[rel8->resource_block_coding];
      dlsch0_harq->rb_alloc[1]      = localRIV2alloc_LUT50_1[rel8->resource_block_coding];
      break;
    case 75:
      AssertFatal(1==0,"75 PRBs not supported for now\n");
      break;
    case 100:
      dlsch0_harq->rb_alloc[0]      = localRIV2alloc_LUT100_0[rel8->resource_block_coding];
      dlsch0_harq->rb_alloc[1]      = localRIV2alloc_LUT100_1[rel8->resource_block_coding];
      dlsch0_harq->rb_alloc[2]      = localRIV2alloc_LUT100_2[rel8->resource_block_coding];
      dlsch0_harq->rb_alloc[3]      = localRIV2alloc_LUT100_3[rel8->resource_block_coding];
    }

    dlsch0->active                  = 1;

    dlsch0_harq->nb_rb              = 6;
    dlsch0_harq->vrb_type           = LOCALIZED;
    dlsch0_harq->rvidx              = 0;
    dlsch0_harq->Nl                 = 0;
    dlsch0_harq->mimo_mode          = (eNB->frame_parms.nb_antenna_ports_eNB == 1) ? SISO : ALAMOUTI;
    dlsch0_harq->dl_power_off       = 1;
    dlsch0_harq->round              = 0;
    dlsch0_harq->status             = ACTIVE;
    dlsch0_harq->TBS                = rel8->length;



  }
  else {

  }
#endif
}

handle_nfapi_ul_pdu(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,  
		    nfapi_ul_config_request_pdu_t *ul_config_pdu) {

  nfapi_ul_config_ulsch_pdu_rel8_t *rel8 = &ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8;
  int8_t UE_id;
  // check if we have received a dci for this ue and ulsch descriptor is configured
  
  AssertFatal((UE_id = find_ulsch(rel8->rnti,eNB,SEARCH_EXIST))>=0,
	      "No existing UE ULSCH for rnti %x\n",rel8->rnti);
  AssertFatal(eNB->ulsch[UE_id]->harq_mask > 0,
	      "ulsch for UE_id %d is not active\n",UE_id);
	      
}

void schedule_response(Sched_Rsp_t *Sched_INFO) {

  PHY_VARS_eNB *eNB;
  eNB_rxtx_proc_t *proc;
  // copy data from L2 interface into L1 structures
  module_id_t               Mod_id       = Sched_INFO->module_id;
  uint8_t                   CC_id        = Sched_INFO->CC_id;
  nfapi_dl_config_request_t *DL_req      = Sched_INFO->DL_req;
  nfapi_hi_dci0_request_t   *HI_DCI0_req = Sched_INFO->HI_DCI0_req;
  nfapi_ul_config_request_t *UL_req      = Sched_INFO->UL_req;
  nfapi_tx_request_t        *TX_req      = Sched_INFO->TX_req;
  frame_t                   frame        = Sched_INFO->frame;
  sub_frame_t               subframe     = Sched_INFO->subframe;
  LTE_DL_FRAME_PARMS        *fp;
  int                       ul_subframe;
  int                       ul_frame;
  int                       harq_pid;
  LTE_UL_eNB_HARQ_t         *ulsch_harq;

  AssertFatal(RC.eNB!=NULL,"RC.eNB is null\n");
  AssertFatal(RC.eNB[Mod_id]!=NULL,"RC.eNB[%d] is null\n",Mod_id);
  AssertFatal(RC.eNB[Mod_id][CC_id]!=NULL,"RC.eNB[%d][%d] is null\n",Mod_id,CC_id);

  eNB         = RC.eNB[Mod_id][CC_id];
  fp          = &eNB->frame_parms;
  proc        = &eNB->proc.proc_rxtx[0];
  ul_subframe = pdcch_alloc2ul_subframe(fp,subframe);
  ul_frame    = pdcch_alloc2ul_frame(fp,frame,subframe);

  AssertFatal(proc->subframe_tx == subframe, "Current subframe %d != NFAPI subframe %d\n",proc->subframe_tx,subframe);
  AssertFatal(proc->subframe_tx == subframe, "Current frame %d != NFAPI frame %d\n",proc->frame_tx,frame);

  int8_t UE_id;
  uint8_t number_dl_pdu             = DL_req->dl_config_request_body.number_pdu;
  uint8_t number_hi_dci0_pdu        = HI_DCI0_req->hi_dci0_request_body.number_of_dci+HI_DCI0_req->hi_dci0_request_body.number_of_hi;
  uint8_t number_ul_pdu             = UL_req->ul_config_request_body.number_of_pdus;
  uint8_t number_pdsch_rnti         = DL_req->dl_config_request_body.number_pdsch_rnti;
  uint8_t transmission_power_pcfich = DL_req->dl_config_request_body.transmission_power_pcfich;


  nfapi_dl_config_request_pdu_t *dl_config_pdu;
  nfapi_hi_dci0_request_pdu_t   *hi_dci0_req_pdu;
  nfapi_ul_config_request_pdu_t *ul_config_pdu;

  int i;

  eNB->pdcch_vars[subframe&1].num_pdcch_symbols = DL_req->dl_config_request_body.number_pdcch_ofdm_symbols;
  eNB->pdcch_vars[subframe&1].num_dci           = 0;


  LOG_I(PHY,"NFAPI: received %d dl_pdu, %d tx_req, %d hi_dci0_config_req, %d UL_config \n",
	number_dl_pdu,TX_req->tx_request_body.number_of_pdus,number_hi_dci0_pdu,number_ul_pdu);

  
  if ((subframe_select(fp,ul_subframe)==SF_UL) ||
      (fp->frame_type == FDD)) {
    harq_pid = subframe2harq_pid(fp,ul_frame,ul_subframe);

    // clear DCI allocation maps for new subframe
    
    for (i=0; i<NUMBER_OF_UE_MAX; i++) {
      if (eNB->ulsch[i]) {
	ulsch_harq = eNB->ulsch[i]->harq_processes[harq_pid];
        ulsch_harq->dci_alloc=0;
        ulsch_harq->rar_alloc=0;
      }
    }
  }
  for (i=0;i<number_dl_pdu;i++) {
    dl_config_pdu = &DL_req->dl_config_request_body.dl_config_pdu_list[i];
    LOG_I(PHY,"NFAPI: dl_pdu %d : type %d\n",i,dl_config_pdu->pdu_type);
    switch (dl_config_pdu->pdu_type) {
    case NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE:
      handle_nfapi_dci_dl_pdu(eNB,proc,dl_config_pdu);
      eNB->pdcch_vars[subframe&1].num_dci++;
      break;
    case NFAPI_DL_CONFIG_BCH_PDU_TYPE:
      AssertFatal(dl_config_pdu->bch_pdu.bch_pdu_rel8.pdu_index<TX_req->tx_request_body.number_of_pdus,
		  "bch_pdu_rel8.pdu_index>=TX_req->number_of_pdus (%d>%d)\n",
		  dl_config_pdu->bch_pdu.bch_pdu_rel8.pdu_index,
		  TX_req->tx_request_body.number_of_pdus);
      eNB->pbch_configured=1;
      handle_nfapi_bch_pdu(eNB,proc,dl_config_pdu,
			   TX_req->tx_request_body.tx_pdu_list[dl_config_pdu->bch_pdu.bch_pdu_rel8.pdu_index].segments[0].segment_data);
      break;
    case NFAPI_DL_CONFIG_MCH_PDU_TYPE:
      //      handle_nfapi_mch_dl_pdu(eNB,dl_config_pdu);
      break;
    case NFAPI_DL_CONFIG_DLSCH_PDU_TYPE:
      AssertFatal(dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index<TX_req->tx_request_body.number_of_pdus,
		  "dlsch_pdu_rel8.pdu_index>=TX_req->number_of_pdus (%d>%d)\n",
		  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index,
		  TX_req->tx_request_body.number_of_pdus);
      AssertFatal((dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks<3) &&
		  (dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks>0),
		  "dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks = %d not in [1,2]\n",
		  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks);
      handle_nfapi_dlsch_pdu(eNB,proc,dl_config_pdu,
			     dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks-1,
			     TX_req->tx_request_body.tx_pdu_list[dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index].segments[0].segment_data);
      if (dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti == eNB->prach_vars.preamble_list[0].preamble_rel8.rnti) {// is RAR pdu
	
	generate_eNB_ulsch_params_from_rar(eNB,
					   TX_req->tx_request_body.tx_pdu_list[dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index].segments[0].segment_data,
					   frame,
					   subframe);
					   
      }					   
      break;
    case NFAPI_DL_CONFIG_PCH_PDU_TYPE:
      //      handle_nfapi_pch_pdu(eNB,dl_config_pdu);
      break;
    case NFAPI_DL_CONFIG_PRS_PDU_TYPE:
      //      handle_nfapi_prs_pdu(eNB,dl_config_pdu);
      break;
    case NFAPI_DL_CONFIG_CSI_RS_PDU_TYPE:
      //      handle_nfapi_csi_rs_pdu(eNB,dl_config_pdu);
      break;
    case NFAPI_DL_CONFIG_EPDCCH_DL_PDU_TYPE:
      //      handle_nfapi_epdcch_pdu(eNB,dl_config_pdu);
      break;
    case NFAPI_DL_CONFIG_MPDCCH_PDU_TYPE:
      handle_nfapi_mpdcch_pdu(eNB,proc,dl_config_pdu);
      eNB->mpdcch_vars[subframe&1].num_dci++; 
      break;
    }
  }
  
  for (i=0;i<number_hi_dci0_pdu;i++) {
    hi_dci0_req_pdu = &HI_DCI0_req->hi_dci0_request_body.hi_dci0_pdu_list[i];
    switch (hi_dci0_req_pdu->pdu_type) {

    case NFAPI_HI_DCI0_DCI_PDU_TYPE:
      handle_nfapi_hi_dci0_dci_pdu(eNB,proc,hi_dci0_req_pdu);
      eNB->pdcch_vars[subframe&1].num_dci++; 
      break;

    case NFAPI_HI_DCI0_HI_PDU_TYPE:
      handle_nfapi_hi_dci0_hi_pdu(eNB,proc,hi_dci0_req_pdu);
      eNB->pdcch_vars[subframe&1].num_hi++; 

      break;
    }
  } 

  for (i=0;i<number_ul_pdu;i++) {
    ul_config_pdu = &UL_req->ul_config_request_body.ul_config_pdu_list[i];
    LOG_I(PHY,"NFAPI: ul_pdu %d : type %d\n",i,ul_config_pdu->pdu_type);
    AssertFatal(ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_PDU_TYPE,
		"Optional UL_PDU type %d not supported\n",ul_config_pdu->pdu_type);
    handle_nfapi_ul_pdu(eNB,proc,ul_config_pdu);
  }
}



void phy_procedures_eNB_TX(PHY_VARS_eNB *eNB,
			   eNB_rxtx_proc_t *proc,
                           relaying_type_t r_type,
			   PHY_VARS_RN *rn,
			   int do_meas)
{
  UNUSED(rn);
  int frame=proc->frame_tx;
  int subframe=proc->subframe_tx;
  uint32_t i,j,aa;
  uint8_t harq_pid;
  int8_t UE_id=0;
  uint8_t num_pdcch_symbols=0;
  uint8_t num_dci=0;
  uint8_t ul_subframe;
  uint32_t ul_frame;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  LTE_UL_eNB_HARQ_t *ulsch_harq;

  int offset = eNB->CC_id;//proc == &eNB->proc.proc_rxtx[0] ? 0 : 1;

  if ((fp->frame_type == TDD) && (subframe_select(fp,subframe)==SF_UL)) return;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_TX+offset,1);
  if (do_meas==1) start_meas(&eNB->phy_proc_tx);

  T(T_ENB_PHY_DL_TICK, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe));

  /*
  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    // If we've dropped the UE, go back to PRACH mode for this UE
    if ((frame==0)&&(subframe==0)) {
      if (eNB->UE_stats[i].crnti > 0) {
	LOG_I(PHY,"UE %d : rnti %x\n",i,eNB->UE_stats[i].crnti);
      }
    }
    if (eNB->UE_stats[i].ulsch_consecutive_errors == ULSCH_max_consecutive_errors) {
      LOG_W(PHY,"[eNB %d, CC %d] frame %d, subframe %d, UE %d: ULSCH consecutive error count reached %u, triggering UL Failure\n",
            eNB->Mod_id,eNB->CC_id,frame,subframe, i, eNB->UE_stats[i].ulsch_consecutive_errors);
      eNB->UE_stats[i].ulsch_consecutive_errors=0;
      mac_xface->UL_failure_indication(eNB->Mod_id,
				       eNB->CC_id,
				       frame,
				       eNB->UE_stats[i].crnti,
				       subframe);
				       
    }
	

  }


  // Get scheduling info for next subframe
  // This is called only for the CC_id = 0 and triggers scheduling for all CC_id's
  if (eNB->mac_enabled==1) {
    if (eNB->CC_id == 0) {
      mac_xface->eNB_dlsch_ulsch_scheduler(eNB->Mod_id,0,frame,subframe);//,1);
    }
  }
  */

  // clear the transmit data array for the current subframe
  for (aa=0; aa<fp->nb_antenna_ports_eNB; aa++) {      
    memset(&eNB->common_vars.txdataF[aa][subframe*fp->ofdm_symbol_size*(fp->symbols_per_tti)],
	   0,fp->ofdm_symbol_size*(fp->symbols_per_tti)*sizeof(int32_t));
  }
  

  if (is_pmch_subframe(frame,subframe,fp)) {
    pmch_procedures(eNB,proc,rn,r_type);
  }
  else {
    // this is not a pmch subframe, so generate PSS/SSS/PBCH
    common_signal_procedures(eNB,proc);
  }

  /*
  if (eNB->mac_enabled==1) {
    // Parse DCI received from MAC
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PDCCH_TX,1);
    DCI_pdu = mac_xface->get_dci_sdu(eNB->Mod_id,
				     eNB->CC_id,
				     frame,
				     subframe);
  }
  else {
    DCI_pdu = &DCI_pdu_tmp;
    fill_dci(DCI_pdu,eNB,proc);
    // clear previous allocation information for all UEs
    for (i=0; i<NUMBER_OF_UE_MAX; i++) {
      if (eNB->dlsch[i][0]){
        for (j=0; j<8; j++)
          eNB->dlsch[i][0]->harq_processes[j]->round = 0;
      }
    }
  }
*/
  // clear existing ulsch dci allocations before applying info from MAC  (this is table
  ul_subframe = pdcch_alloc2ul_subframe(fp,subframe);
  ul_frame = pdcch_alloc2ul_frame(fp,frame,subframe);



  // clear previous allocation information for all UEs
  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    if (eNB->dlsch[i][0])
      eNB->dlsch[i][0]->subframe_tx[subframe] = 0;
  }

  /* save old HARQ information needed for PHICH generation */
  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    harq_pid = subframe2harq_pid(fp,ul_frame,ul_subframe);
    if (eNB->ulsch[i]) {
      ulsch_harq = eNB->ulsch[i]->harq_processes[harq_pid];

      /* Store first_rb and n_DMRS for correct PHICH generation below.
       * For PHICH generation we need "old" values of last scheduling
       * for this HARQ process. 'generate_eNB_dlsch_params' below will
       * overwrite first_rb and n_DMRS and 'generate_phich_top', done
       * after 'generate_eNB_dlsch_params', would use the "new" values
       * instead of the "old" ones.
       *
       * This has been tested for FDD only, may be wrong for TDD.
       *
       * TODO: maybe we should restructure the code to be sure it
       *       is done correctly. The main concern is if the code
       *       changes and first_rb and n_DMRS are modified before
       *       we reach here, then the PHICH processing will be wrong,
       *       using wrong first_rb and n_DMRS values to compute
       *       ngroup_PHICH and nseq_PHICH.
       *
       * TODO: check if that works with TDD.
       */
      if ((subframe_select(fp,ul_subframe)==SF_UL) ||
          (fp->frame_type == FDD)) {
        ulsch_harq->previous_first_rb = ulsch_harq->first_rb;
        ulsch_harq->previous_n_DMRS   = ulsch_harq->n_DMRS;
      }
    }
  }


  //  num_pdcch_symbols = DCI_pdu->num_pdcch_symbols;
  num_pdcch_symbols = eNB->pdcch_vars[subframe&1].num_pdcch_symbols;
  num_dci           = eNB->pdcch_vars[subframe&1].num_dci;
  //  LOG_D(PHY,"num_pdcch_symbols %"PRIu8",(dci common %"PRIu8", dci uespec %"PRIu8"\n",num_pdcch_symbols,
  //        DCI_pdu->Num_common_dci,DCI_pdu->Num_ue_spec_dci);
  LOG_I(PHY,"num_pdcch_symbols %"PRIu8",(number dci %"PRIu8"\n",num_pdcch_symbols,
	num_dci);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DCI_INFO,num_pdcch_symbols);


  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DCI_INFO,(frame*10)+subframe);

  // Apply physicalConfigDedicated if needed
  // This is for UEs that have received this IE, which changes these DL and UL configuration, we apply after a delay for the eNodeB UL parameters
  phy_config_dedicated_eNB_step2(eNB);

  if (num_dci > 0)
    LOG_I(PHY,"[eNB %"PRIu8"] Frame %d, subframe %d: Calling generate_dci_top (pdcch) (num_dci %"PRIu8")\n",eNB->Mod_id,frame, subframe,
	  num_dci);
    
  generate_dci_top(num_pdcch_symbols,
		   num_dci,
		   &eNB->pdcch_vars[subframe&1].dci_alloc[0],
		   0,
		   AMP,
		   fp,
		   eNB->common_vars.txdataF,
		   subframe);
  

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PDCCH_TX,0);

  // Now scan UE specific DLSCH
  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++)
    {
      if ((eNB->dlsch[(uint8_t)UE_id][0])&&
	  (eNB->dlsch[(uint8_t)UE_id][0]->rnti>0)&&
	  (eNB->dlsch[(uint8_t)UE_id][0]->active == 1)) {

	// get harq_pid
	harq_pid = eNB->dlsch[(uint8_t)UE_id][0]->harq_ids[subframe];
	AssertFatal(harq_pid>=0,"harq_pid is negative\n");
	// generate pdsch
	pdsch_procedures(eNB,
			 proc,
			 harq_pid,
			 eNB->dlsch[(uint8_t)UE_id][0],
			 eNB->dlsch[(uint8_t)UE_id][1],
			 &eNB->UE_stats[(uint32_t)UE_id],
			 0,
			 num_pdcch_symbols);


      }

      else if ((eNB->dlsch[(uint8_t)UE_id][0])&&
	       (eNB->dlsch[(uint8_t)UE_id][0]->rnti>0)&&
	       (eNB->dlsch[(uint8_t)UE_id][0]->active == 0)) {

	// clear subframe TX flag since UE is not scheduled for PDSCH in this subframe (so that we don't look for PUCCH later)
	eNB->dlsch[(uint8_t)UE_id][0]->subframe_tx[subframe]=0;
      }
    }



  // if we have PHICH to generate

  if (is_phich_subframe(fp,subframe))
    {
      generate_phich_top(eNB,
			 proc,
			 AMP,
			 0);
    }

  /*
  if (frame>=10 && subframe>=9) {
    write_output("/tmp/txsigF0.m","txsF0", &eNB->common_vars.txdataF[0][0][0],120*eNB->frame_parms.ofdm_symbol_size,1,1);
    write_output("/tmp/txsigF1.m","txsF1", &eNB->common_vars.txdataF[0][0][0],120*eNB->frame_parms.ofdm_symbol_size,1,1);
    abort();
  }
  */

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_TX+offset,0);
  if (do_meas==1) stop_meas(&eNB->phy_proc_tx);
  
}

void process_Msg3(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,uint8_t UE_id, uint8_t harq_pid)
{
  // this prepares the demodulation of the first PUSCH of a new user, containing Msg3
  int subframe = proc->subframe_rx;
  int frame = proc->frame_rx;
  LTE_eNB_ULSCH_t *ulsch = eNB->ulsch[UE_id];
  LTE_UL_eNB_HARQ_t *ulsch_harq=ulsch->harq_processes[harq_pid];

  LOG_D(PHY,"[eNB %d][RAPROC] frame %d : subframe %d : process_Msg3 UE_id %d (active %d, subframe %d, frame %d)\n",
        eNB->Mod_id,
        frame,subframe,
        UE_id,ulsch->Msg3_active,
        ulsch_harq->subframe,
        ulsch_harq->frame);

  ulsch_harq->Msg3_flag = 0;

  if ((ulsch->Msg3_active == 1) &&
      (ulsch_harq->subframe == subframe) &&
      (ulsch_harq->frame == (uint32_t)frame))   {

    //    harq_pid = 0;

    ulsch->Msg3_active = 0;
    ulsch_harq->Msg3_flag = 1;
    LOG_D(PHY,"[eNB %d][RAPROC] frame %d, subframe %d: Setting subframe_scheduling_flag (Msg3) for UE %d\n",
          eNB->Mod_id,
          frame,subframe,UE_id);
  }
}


// This function retrieves the harq_pid of the corresponding DLSCH process
// and updates the error statistics of the DLSCH based on the received ACK
// info from UE along with the round index.  It also performs the fine-grain
// rate-adaptation based on the error statistics derived from the ACK/NAK process

void process_HARQ_feedback(uint8_t UE_id,
                           PHY_VARS_eNB *eNB,
			   eNB_rxtx_proc_t *proc,
                           uint8_t pusch_flag,
                           uint8_t *pucch_payload,
                           uint8_t pucch_sel,
                           uint8_t SR_payload)
{

  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  uint8_t dl_harq_pid[8],dlsch_ACK[8],dl_subframe;
  LTE_eNB_DLSCH_t *dlsch             =  eNB->dlsch[(uint32_t)UE_id][0];
  LTE_eNB_UE_stats *ue_stats         =  &eNB->UE_stats[(uint32_t)UE_id];
  LTE_DL_eNB_HARQ_t *dlsch_harq_proc;
  uint8_t subframe_m4,M,m;
  int mp;
  int all_ACKed=1,nb_alloc=0,nb_ACK=0;
  int frame = proc->frame_rx;
  int subframe = proc->subframe_rx;
  int harq_pid = subframe2harq_pid( fp,frame,subframe);

  if (fp->frame_type == FDD) { //FDD
    subframe_m4 = (subframe<4) ? subframe+6 : subframe-4;

    dl_harq_pid[0] = dlsch->harq_ids[subframe_m4];
    M=1;

    if (pusch_flag == 1) {
      dlsch_ACK[0] = eNB->ulsch[(uint8_t)UE_id]->harq_processes[harq_pid]->o_ACK[0];
      if (dlsch->subframe_tx[subframe_m4]==1)
	LOG_D(PHY,"[eNB %d] Frame %d: Received ACK/NAK %d on PUSCH for subframe %d\n",eNB->Mod_id,
	      frame,dlsch_ACK[0],subframe_m4);
    }
    else {
      dlsch_ACK[0] = pucch_payload[0];
      LOG_D(PHY,"[eNB %d] Frame %d: Received ACK/NAK %d on PUCCH for subframe %d\n",eNB->Mod_id,
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

    M=ul_ACK_subframe2_M(fp,
                         subframe);

    // Now derive ACK information for TDD
    if (pusch_flag == 1) { // Do PUSCH ACK/NAK first
      // detect missing DAI
      //FK: this code is just a guess
      //RK: not exactly, yes if scheduled from PHICH (i.e. no DCI format 0)
      //    otherwise, it depends on how many of the PDSCH in the set are scheduled, we can leave it like this,
      //    but we have to adapt the code below.  For example, if only one out of 2 are scheduled, only 1 bit o_ACK is used

      dlsch_ACK[0] = eNB->ulsch[(uint8_t)UE_id]->harq_processes[harq_pid]->o_ACK[0];
      dlsch_ACK[1] = (eNB->pucch_config_dedicated[UE_id].tdd_AckNackFeedbackMode == bundling)
	?eNB->ulsch[(uint8_t)UE_id]->harq_processes[harq_pid]->o_ACK[0]:eNB->ulsch[(uint8_t)UE_id]->harq_processes[harq_pid]->o_ACK[1];
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
      dl_subframe = ul_ACK_subframe2_dl_subframe(fp,
						 subframe,
						 m);

      if (dlsch->subframe_tx[dl_subframe]==1)
        nb_alloc++;
    }

    if (nb_alloc == nb_ACK)
      all_ACKed = 1;
    else
      all_ACKed = 0;
  }


  for (m=0,mp=-1; m<M; m++) {

    dl_subframe = ul_ACK_subframe2_dl_subframe(fp,
					       subframe,
					       m);

    if (dlsch->subframe_tx[dl_subframe]==1) {
      if (pusch_flag == 1)
        mp++;
      else
        mp = m;

      dl_harq_pid[m]     = dlsch->harq_ids[dl_subframe];
      harq_pid_updated[UE_id][dl_harq_pid[m]] = 1;

      if ((pucch_sel != 2)&&(pusch_flag == 0)) { // multiplexing
        if ((SR_payload == 1)&&(all_ACKed == 1))
          dlsch_ACK[m] = 1;
        else
          dlsch_ACK[m] = 0;
      }

      if (dl_harq_pid[m]<dlsch->Mdlharq) {
        dlsch_harq_proc = dlsch->harq_processes[dl_harq_pid[m]];
#ifdef DEBUG_PHY_PROC
        LOG_D(PHY,"[eNB %d][PDSCH %x/%d] subframe %d, status %d, round %d (mcs %d, rv %d, TBS %d)\n",eNB->Mod_id,
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

          if ( dlsch_ACK[mp]==0) {
            // Received NAK
#ifdef DEBUG_PHY_PROC
            LOG_D(PHY,"[eNB %d][PDSCH %x/%d] M = %d, m= %d, mp=%d NAK Received in round %d, requesting retransmission\n",eNB->Mod_id,
                  dlsch->rnti,dl_harq_pid[m],M,m,mp,dlsch_harq_proc->round);
#endif

            T(T_ENB_PHY_DLSCH_UE_NACK, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(UE_id), T_INT(dlsch->rnti),
              T_INT(dl_harq_pid[m]));

            if (dlsch_harq_proc->round == 0)
              ue_stats->dlsch_NAK_round0++;

            ue_stats->dlsch_NAK[dl_harq_pid[m]][dlsch_harq_proc->round]++;


            // then Increment DLSCH round index
            dlsch_harq_proc->round++;


            if (dlsch_harq_proc->round == dlsch->Mlimit) {
              // This was the last round for DLSCH so reset round and increment l2_error counter
#ifdef DEBUG_PHY_PROC
              LOG_W(PHY,"[eNB %d][PDSCH %x/%d] DLSCH retransmissions exhausted, dropping packet\n",eNB->Mod_id,
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
            LOG_D(PHY,"[eNB %d][PDSCH %x/%d] ACK Received in round %d, resetting process\n",eNB->Mod_id,
                  dlsch->rnti,dl_harq_pid[m],dlsch_harq_proc->round);
#endif

            T(T_ENB_PHY_DLSCH_UE_ACK, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(UE_id), T_INT(dlsch->rnti),
              T_INT(dl_harq_pid[m]));

            ue_stats->dlsch_ACK[dl_harq_pid[m]][dlsch_harq_proc->round]++;

            // Received ACK so set round to 0 and set dlsch_harq_pid IDLE
            dlsch_harq_proc->round  = 0;
            dlsch_harq_proc->status = SCH_IDLE;
            dlsch->harq_ids[dl_subframe] = dlsch->Mdlharq;

            ue_stats->total_TBS = ue_stats->total_TBS +
	      eNB->dlsch[(uint8_t)UE_id][0]->harq_processes[dl_harq_pid[m]]->TBS;
            /*
              ue_stats->total_transmitted_bits = ue_stats->total_transmitted_bits +
              eNB->dlsch[(uint8_t)UE_id][0]->harq_processes[dl_harq_pid[m]]->TBS;
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
	  harq_pid_round[UE_id][dl_harq_pid[m]] = dlsch_harq_proc->round;
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

void get_n1_pucch_eNB(PHY_VARS_eNB *eNB,
		      eNB_rxtx_proc_t *proc,
                      uint8_t UE_id,
                      int16_t *n1_pucch0,
                      int16_t *n1_pucch1,
                      int16_t *n1_pucch2,
                      int16_t *n1_pucch3)
{

  LTE_DL_FRAME_PARMS *frame_parms=&eNB->frame_parms;
  uint8_t nCCE0,nCCE1;
  int sf;
  int frame = proc->frame_rx;
  int subframe = proc->subframe_rx;

  if (frame_parms->frame_type == FDD ) {
    sf = (subframe<4) ? (subframe+6) : (subframe-4);

    if (eNB->dlsch[(uint32_t)UE_id][0]->subframe_tx[sf]>0) {
      *n1_pucch0 = frame_parms->pucch_config_common.n1PUCCH_AN + eNB->dlsch[(uint32_t)UE_id][0]->nCCE[sf];
      *n1_pucch1 = -1;
    } else {
      *n1_pucch0 = -1;
      *n1_pucch1 = -1;
    }
  } else {

    switch (frame_parms->tdd_config) {
    case 1:  // DL:S:UL:UL:DL:DL:S:UL:UL:DL
      if (subframe == 2) {  // ACK subframes 5 and 6
        /*  if (eNB->dlsch[(uint32_t)UE_id][0]->subframe_tx[6]>0) {
	    nCCE1 = eNB->dlsch[(uint32_t)UE_id][0]->nCCE[6];
	    *n1_pucch1 = get_Np(frame_parms->N_RB_DL,nCCE1,1) + nCCE1 + frame_parms->pucch_config_common.n1PUCCH_AN;
	    }
	    else
	    *n1_pucch1 = -1;*/

        if (eNB->dlsch[(uint32_t)UE_id][0]->subframe_tx[5]>0) {
          nCCE0 = eNB->dlsch[(uint32_t)UE_id][0]->nCCE[5];
          *n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0+ frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch0 = -1;

        *n1_pucch1 = -1;
      } else if (subframe == 3) { // ACK subframe 9

        if (eNB->dlsch[(uint32_t)UE_id][0]->subframe_tx[9]>0) {
          nCCE0 = eNB->dlsch[(uint32_t)UE_id][0]->nCCE[9];
          *n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0 +frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch0 = -1;

        *n1_pucch1 = -1;

      } else if (subframe == 7) { // ACK subframes 0 and 1
        //harq_ack[0].nCCE;
        //harq_ack[1].nCCE;
        if (eNB->dlsch[(uint32_t)UE_id][0]->subframe_tx[0]>0) {
          nCCE0 = eNB->dlsch[(uint32_t)UE_id][0]->nCCE[0];
          *n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0 + frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch0 = -1;

        *n1_pucch1 = -1;
      } else if (subframe == 8) { // ACK subframes 4
        //harq_ack[4].nCCE;
        if (eNB->dlsch[(uint32_t)UE_id][0]->subframe_tx[4]>0) {
          nCCE0 = eNB->dlsch[(uint32_t)UE_id][0]->nCCE[4];
          *n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0 + frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch0 = -1;

        *n1_pucch1 = -1;
      } else {
        LOG_D(PHY,"[eNB %d] frame %d: phy_procedures_lte.c: get_n1pucch, illegal subframe %d for tdd_config %d\n",
              eNB->Mod_id,
              frame,
              subframe,frame_parms->tdd_config);
        return;
      }

      break;

    case 3:  // DL:S:UL:UL:UL:DL:DL:DL:DL:DL
      if (subframe == 2) {  // ACK subframes 5,6 and 1 (S in frame-2), forget about n-11 for the moment (S-subframe)
        if (eNB->dlsch[(uint32_t)UE_id][0]->subframe_tx[6]>0) {
          nCCE1 = eNB->dlsch[(uint32_t)UE_id][0]->nCCE[6];
          *n1_pucch1 = get_Np(frame_parms->N_RB_DL,nCCE1,1) + nCCE1 + frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch1 = -1;

        if (eNB->dlsch[(uint32_t)UE_id][0]->subframe_tx[5]>0) {
          nCCE0 = eNB->dlsch[(uint32_t)UE_id][0]->nCCE[5];
          *n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0+ frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch0 = -1;
      } else if (subframe == 3) { // ACK subframes 7 and 8
        LOG_D(PHY,"get_n1_pucch_eNB : subframe 3, subframe_tx[7] %d, subframe_tx[8] %d\n",
              eNB->dlsch[(uint32_t)UE_id][0]->subframe_tx[7],eNB->dlsch[(uint32_t)UE_id][0]->subframe_tx[8]);

        if (eNB->dlsch[(uint32_t)UE_id][0]->subframe_tx[8]>0) {
          nCCE1 = eNB->dlsch[(uint32_t)UE_id][0]->nCCE[8];
          *n1_pucch1 = get_Np(frame_parms->N_RB_DL,nCCE1,1) + nCCE1 + frame_parms->pucch_config_common.n1PUCCH_AN;
          LOG_D(PHY,"nCCE1 %d, n1_pucch1 %d\n",nCCE1,*n1_pucch1);
        } else
          *n1_pucch1 = -1;

        if (eNB->dlsch[(uint32_t)UE_id][0]->subframe_tx[7]>0) {
          nCCE0 = eNB->dlsch[(uint32_t)UE_id][0]->nCCE[7];
          *n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0 +frame_parms->pucch_config_common.n1PUCCH_AN;
          LOG_D(PHY,"nCCE0 %d, n1_pucch0 %d\n",nCCE0,*n1_pucch0);
        } else
          *n1_pucch0 = -1;
      } else if (subframe == 4) { // ACK subframes 9 and 0
        if (eNB->dlsch[(uint32_t)UE_id][0]->subframe_tx[0]>0) {
          nCCE1 = eNB->dlsch[(uint32_t)UE_id][0]->nCCE[0];
          *n1_pucch1 = get_Np(frame_parms->N_RB_DL,nCCE1,1) + nCCE1 + frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch1 = -1;

        if (eNB->dlsch[(uint32_t)UE_id][0]->subframe_tx[9]>0) {
          nCCE0 = eNB->dlsch[(uint32_t)UE_id][0]->nCCE[9];
          *n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0 +frame_parms->pucch_config_common.n1PUCCH_AN;
        } else
          *n1_pucch0 = -1;
      } else {
        LOG_D(PHY,"[eNB %d] Frame %d: phy_procedures_lte.c: get_n1pucch, illegal subframe %d for tdd_config %d\n",
              eNB->Mod_id,frame,subframe,frame_parms->tdd_config);
        return;
      }

      break;
    }  // switch tdd_config

    // Don't handle the case M>2
    *n1_pucch2 = -1;
    *n1_pucch3 = -1;
  }
}

void prach_procedures(PHY_VARS_eNB *eNB) {

  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  uint16_t preamble_energy_list[64],preamble_delay_list[64];
  uint16_t preamble_max,preamble_energy_max;
  uint16_t i;
  int subframe = eNB->proc.subframe_prach;
  int frame = eNB->proc.frame_prach;
  uint8_t CC_id = eNB->CC_id;
  RU_t *ru;
  int aa=0;
  int ru_aa;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PRACH_RX,1);
  memset(&preamble_energy_list[0],0,64*sizeof(uint16_t));
  memset(&preamble_delay_list[0],0,64*sizeof(uint16_t));


  for (i=0;i<eNB->num_RU;i++) {
    ru=eNB->RU_list[i];
    for (ru_aa=0;ru_aa<ru->nb_rx;ru_aa++) eNB->prach_vars.rxsigF[aa++] = eNB->RU_list[i]->prach_rxsigF[ru_aa];
  }
  rx_prach(eNB,
	   eNB->RU_list[0],
	   preamble_energy_list,
	   preamble_delay_list,
	   frame,
	   0);
  preamble_energy_max = preamble_energy_list[0];
  preamble_max = 0;

  for (i=1; i<64; i++) {
    if (preamble_energy_max < preamble_energy_list[i]) {
      preamble_energy_max = preamble_energy_list[i];
      preamble_max = i;
    }
  }

  //#ifdef DEBUG_PHY_PROC
  LOG_I(PHY,"[RAPROC] Frame %d, subframe %d : Most likely preamble %d, energy %d dB delay %d\n",
        frame,subframe,
	preamble_max,
        preamble_energy_list[preamble_max],
        preamble_delay_list[preamble_max]);
  //#endif

  if (preamble_energy_list[preamble_max] > 580) {

    //    UE_id = find_next_ue_index(eNB);
 
    //    if (UE_id>=0) {
      //      eNB->UE_stats[(uint32_t)UE_id].UE_timing_offset = preamble_delay_list[preamble_max]&0x1FFF; //limit to 13 (=11+2) bits

      //      eNB->UE_stats[(uint32_t)UE_id].sector = 0;
    LOG_I(PHY,"[eNB %d/%d][RAPROC] Frame %d, subframe %d Initiating RA procedure with preamble %d, energy %d.%d dB, delay %d\n",
	  eNB->Mod_id,
	  eNB->CC_id,
	  frame,
	  subframe,
	  preamble_max,
	  preamble_energy_max/10,
	  preamble_energy_max%10,
	  preamble_delay_list[preamble_max]);
    
    T(T_ENB_PHY_INITIATE_RA_PROCEDURE, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe), 0,
      T_INT(preamble_max), T_INT(preamble_energy_max), T_INT(preamble_delay_list[preamble_max]));

    if (eNB->mac_enabled==1) {
      LTE_eNB_PRACH *prach_vars = &eNB->prach_vars;
      
      uint8_t update_TA  = 4;
      uint8_t update_TA2 = 1;
      switch (fp->N_RB_DL) {
      case 6:
	update_TA = 16;
	break;
	
      case 25:
	update_TA = 4;
	break;
	
      case 50:
	update_TA = 2;
	break;
	
      case 75:
	update_TA  = 3;
	update_TA2 = 2;
      case 100:
	update_TA  = 1;
	break;
      }
      pthread_mutex_lock(&eNB->UL_INFO_mutex);
      eNB->UL_INFO.rach_ind.number_of_preambles                        = 1;
      eNB->UL_INFO.rach_ind.preamble_list                              = prach_vars->preamble_list;
      
      prach_vars->preamble_list[0].preamble_rel8.timing_advance        = preamble_delay_list[preamble_max]*update_TA/update_TA2;
      prach_vars->preamble_list[0].preamble_rel8.preamble              = preamble_max;
      prach_vars->preamble_list[0].preamble_rel8.rnti                  = 1+subframe;  // note: fid is implicitly 0 here
      prach_vars->preamble_list[0].instance_length                     = 0; //don't know exactly what this is
      pthread_mutex_unlock(&eNB->UL_INFO_mutex);
      /*
	mac_xface->initiate_ra_proc(eNB->Mod_id,
	eNB->CC_id,
	frame,
	preamble_max,
	preamble_delay_list[preamble_max]*update_TA/update_TA2,
	0,subframe,0);*/
      
      // fill eNB->UL_info with prach information
    }      
    
    /*  } else {
    MSC_LOG_EVENT(MSC_PHY_ENB, "0 RA Failed add user, too many");
    LOG_I(PHY,"[eNB %d][RAPROC] frame %d, subframe %d: Unable to add user, max user count reached\n",
	  eNB->Mod_id,frame, subframe);
	  }*/
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PRACH_RX,0);
}

void pucch_procedures(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,int UE_id,int harq_pid,uint8_t do_srs)
{
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  uint8_t SR_payload = 0,*pucch_payload=NULL,pucch_payload0[2]= {0,0},pucch_payload1[2]= {0,0};
  int16_t n1_pucch0 = -1, n1_pucch1 = -1, n1_pucch2 = -1, n1_pucch3 = -1;
  uint8_t do_SR = 0;
  uint8_t pucch_sel = 0;
  int32_t metric0=0,metric1=0,metric0_SR=0;
  ANFBmode_t bundling_flag;
  PUCCH_FMT_t format;
  const int subframe = proc->subframe_rx;
  const int frame = proc->frame_rx;

  if ((eNB->dlsch[UE_id][0]) &&
      (eNB->dlsch[UE_id][0]->rnti>0) &&
      (eNB->ulsch[UE_id]->harq_processes[harq_pid]->frame != frame) &&
      (eNB->ulsch[UE_id]->harq_processes[harq_pid]->subframe != subframe)) {

    // check SR availability
    do_SR = is_SR_subframe(eNB,proc,UE_id);
    //      do_SR = 0;

    // Now ACK/NAK
    // First check subframe_tx flag for earlier subframes

    get_n1_pucch_eNB(eNB,
                     proc,
                     UE_id,
                     &n1_pucch0,
                     &n1_pucch1,
                     &n1_pucch2,
                     &n1_pucch3);

    LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d, subframe %d Checking for PUCCH (%d,%d,%d,%d) SR %d\n",
          eNB->Mod_id,eNB->dlsch[UE_id][0]->rnti,
          frame,subframe,
          n1_pucch0,n1_pucch1,n1_pucch2,n1_pucch3,do_SR);

    if ((n1_pucch0==-1) && (n1_pucch1==-1) && (do_SR==0)) {  // no TX PDSCH that have to be checked and no SR for this UE_id
    } else {
      // otherwise we have some PUCCH detection to do

      // Null out PUCCH PRBs for noise measurement
      switch(fp->N_RB_UL) {
      case 6:
        eNB->rb_mask_ul[0] |= (0x1 | (1<<5)); //position 5
        break;
      case 15:
        eNB->rb_mask_ul[0] |= (0x1 | (1<<14)); // position 14
        break;
      case 25:
        eNB->rb_mask_ul[0] |= (0x1 | (1<<24)); // position 24
        break;
      case 50:
        eNB->rb_mask_ul[0] |= 0x1;
        eNB->rb_mask_ul[1] |= (1<<17); // position 49 (49-32)
        break;
      case 75:
        eNB->rb_mask_ul[0] |= 0x1;
        eNB->rb_mask_ul[2] |= (1<<10); // position 74 (74-64)
        break;
      case 100:
        eNB->rb_mask_ul[0] |= 0x1;
        eNB->rb_mask_ul[3] |= (1<<3); // position 99 (99-96)
        break;
      default:
        LOG_E(PHY,"Unknown number for N_RB_UL %d\n",fp->N_RB_UL);
        break;
      }

      if (do_SR == 1) {
        eNB->UE_stats[UE_id].sr_total++;


	metric0_SR = rx_pucch(eNB,
			      pucch_format1,
			      UE_id,
			      eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex,
			      0, // n2_pucch
			      do_srs, // shortened format
			      &SR_payload,
			      frame,
			      subframe,
			      PUCCH1_THRES);
	LOG_D(PHY,"[eNB %d][SR %x] Frame %d subframe %d Checking SR is %d (SR n1pucch is %d)\n",
	      eNB->Mod_id,
	      eNB->ulsch[UE_id]->rnti,
	      frame,
	      subframe,
	      SR_payload,
	      eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex);
      }// do_SR==1

      if ((n1_pucch0==-1) && (n1_pucch1==-1)) { // just check for SR
      } else if (fp->frame_type==FDD) { // FDD
        // if SR was detected, use the n1_pucch from SR, else use n1_pucch0
        //          n1_pucch0 = (SR_payload==1) ? eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex:n1_pucch0;

        LOG_D(PHY,"Demodulating PUCCH for ACK/NAK: n1_pucch0 %d (%d), SR_payload %d\n",n1_pucch0,eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex,SR_payload);

	metric0 = rx_pucch(eNB,
			   pucch_format1a,
			   UE_id,
			   (uint16_t)n1_pucch0,
			   0, //n2_pucch
			   do_srs, // shortened format
			   pucch_payload0,
			   frame,
			   subframe,
			   PUCCH1a_THRES);
        

        /* cancel SR detection if reception on n1_pucch0 is better than on SR PUCCH resource index */
        if (do_SR && metric0 > metric0_SR) SR_payload = 0;

        if (do_SR && metric0 <= metric0_SR) {
          /* when transmitting ACK/NACK on SR PUCCH resource index, SR payload is always 1 */
          SR_payload = 1;

	  metric0=rx_pucch(eNB,
			   pucch_format1a,
			   UE_id,
			   eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex,
			   0, //n2_pucch
			   do_srs, // shortened format
			   pucch_payload0,
			   frame,
			   subframe,
			   PUCCH1a_THRES);
        }

#ifdef DEBUG_PHY_PROC
        LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d subframe %d pucch1a (FDD) payload %d (metric %d)\n",
            eNB->Mod_id,
            eNB->dlsch[UE_id][0]->rnti,
            frame,subframe,
            pucch_payload0[0],metric0);
#endif

        process_HARQ_feedback(UE_id,eNB,proc,
                            0,// pusch_flag
                            pucch_payload0,
                            2,
                            SR_payload);
      } // FDD
      else {  //TDD

        bundling_flag = eNB->pucch_config_dedicated[UE_id].tdd_AckNackFeedbackMode;

        // fix later for 2 TB case and format1b

        if ((fp->frame_type==FDD) ||
          (bundling_flag==bundling)    ||
          ((fp->frame_type==TDD)&&(fp->tdd_config==1)&&((subframe!=2)&&(subframe!=7)))) {
          format = pucch_format1a;
        } else {
          format = pucch_format1b;
        }

        // if SR was detected, use the n1_pucch from SR
        if (SR_payload==1) {
#ifdef DEBUG_PHY_PROC
          LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d subframe %d Checking ACK/NAK (%d,%d,%d,%d) format %d with SR\n",eNB->Mod_id,
                eNB->dlsch[UE_id][0]->rnti,
                frame,subframe,
                n1_pucch0,n1_pucch1,n1_pucch2,n1_pucch3,format);
#endif

	  metric0 = rx_pucch(eNB,
			     format,
			     UE_id,
			     eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex,
			     0, //n2_pucch
			     do_srs, // shortened format
			     pucch_payload0,
			     frame,
			     subframe,
			     PUCCH1a_THRES);
        } else { //using n1_pucch0/n1_pucch1 resources
#ifdef DEBUG_PHY_PROC
          LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d subframe %d Checking ACK/NAK (%d,%d,%d,%d) format %d\n",eNB->Mod_id,
                eNB->dlsch[UE_id][0]->rnti,
                frame,subframe,
                n1_pucch0,n1_pucch1,n1_pucch2,n1_pucch3,format);
#endif
          metric0=0;
          metric1=0;

          // Check n1_pucch0 metric
          if (n1_pucch0 != -1) {
	    metric0 = rx_pucch(eNB,
			       format,
			       UE_id,
			       (uint16_t)n1_pucch0,
			       0, // n2_pucch
			       do_srs, // shortened format
			       pucch_payload0,
			       frame,
			       subframe,
			       PUCCH1a_THRES);
          }

          // Check n1_pucch1 metric
          if (n1_pucch1 != -1) {
	    metric1 = rx_pucch(eNB,
			       format,
			       UE_id,
			       (uint16_t)n1_pucch1,
			       0, //n2_pucch
			       do_srs, // shortened format
			       pucch_payload1,
			       frame,
			       subframe,
			       PUCCH1a_THRES);
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
        LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d subframe %d ACK/NAK metric 0 %d, metric 1 %d, sel %d, (%d,%d)\n",eNB->Mod_id,
              eNB->dlsch[UE_id][0]->rnti,
              frame,subframe,
              metric0,metric1,pucch_sel,pucch_payload[0],pucch_payload[1]);
#endif
        process_HARQ_feedback(UE_id,eNB,proc,
                              0,// pusch_flag
                              pucch_payload,
                              pucch_sel,
                              SR_payload);
      } // TDD
    }

    if (SR_payload == 1) {
      LOG_D(PHY,"[eNB %d][SR %x] Frame %d subframe %d Got SR for PUSCH, transmitting to MAC\n",eNB->Mod_id,
            eNB->ulsch[UE_id]->rnti,frame,subframe);
      eNB->UE_stats[UE_id].sr_received++;

      if (eNB->first_sr[UE_id] == 1) { // this is the first request for uplink after Connection Setup, so clear HARQ process 0 use for Msg4
        eNB->first_sr[UE_id] = 0;
        eNB->dlsch[UE_id][0]->harq_processes[0]->round=0;
        eNB->dlsch[UE_id][0]->harq_processes[0]->status=SCH_IDLE;
        LOG_D(PHY,"[eNB %d][SR %x] Frame %d subframe %d First SR\n",
              eNB->Mod_id,
              eNB->ulsch[UE_id]->rnti,frame,subframe);
      }

      if (eNB->mac_enabled==1) {
	/*
        mac_xface->SR_indication(eNB->Mod_id,
                                 eNB->CC_id,
                                 frame,
                                 eNB->dlsch[UE_id][0]->rnti,subframe);
	*/

	// fill eNB->UL_info with SR indications
      }
    }
  }
}



extern int oai_exit;

extern void *td_thread(void*);

void init_td_thread(PHY_VARS_eNB *eNB,pthread_attr_t *attr_td) {

  eNB_proc_t *proc = &eNB->proc;

  proc->tdp.eNB = eNB;
  proc->instance_cnt_td         = -1;
    
  pthread_mutex_init( &proc->mutex_td, NULL);
  pthread_cond_init( &proc->cond_td, NULL);

  pthread_create(&proc->pthread_td, attr_td, td_thread, (void*)&proc->tdp);

}

extern void *te_thread(void*);

void init_te_thread(PHY_VARS_eNB *eNB,pthread_attr_t *attr_te) {

  eNB_proc_t *proc = &eNB->proc;

  proc->tep.eNB = eNB;
  proc->instance_cnt_te         = -1;
    
  pthread_mutex_init( &proc->mutex_te, NULL);
  pthread_cond_init( &proc->cond_te, NULL);

  printf("Creating te_thread\n");
  pthread_create(&proc->pthread_te, attr_te, te_thread, (void*)&proc->tep);

}



/*
void phy_procedures_eNB_common_RX(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc){


  //  eNB_proc_t *proc       = &eNB->proc;
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  const int subframe     = proc->subframe_rx;
  const int frame        = proc->frame_rx;
  int offset             = (eNB->single_thread_flag==1) ? 0 : (subframe&1);

  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_ENB+offset, proc->frame_rx );
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_RX0_ENB+offset, proc->subframe_rx );

  if ((fp->frame_type == TDD) && (subframe_select(fp,subframe)!=SF_UL)) {

    if (eNB->node_function == NGFI_RRU_IF4p5) {
      /// **** in TDD during DL send_IF4 of ULTICK to RCC **** ///
      send_IF4p5(eNB, proc->frame_rx, proc->subframe_rx, IF4p5_PULTICK, 0);
    }    
    return;
  }


  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_RX_COMMON+offset, 1 ); 
  start_meas(&eNB->phy_proc_rx);
  LOG_D(PHY,"[eNB %d] Frame %d: Doing phy_procedures_eNB_common_RX(%d)\n",eNB->Mod_id,frame,subframe);


  if (eNB->fep) eNB->fep(eNB,proc);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_RX_COMMON+offset, 0 );
}

*/

void fill_rx_indication(PHY_VARS_eNB *eNB,int UE_id,int frame,int subframe) {

  nfapi_rx_indication_pdu_t *pdu;

  int timing_advance_update;
  int sync_pos;

  uint32_t harq_pid = subframe2harq_pid(&eNB->frame_parms,
					frame,subframe);

  pthread_mutex_lock(&eNB->UL_INFO_mutex);
  pdu                                    = &eNB->UL_INFO.rx_ind.rx_pdu_list[eNB->UL_INFO.rx_ind.number_of_pdus];

  //  pdu->rx_ue_information.handle          = eNB->ulsch[UE_id]->handle;
  pdu->rx_ue_information.rnti            = eNB->ulsch[UE_id]->rnti;
  pdu->rx_indication_rel8.length         = eNB->ulsch[UE_id]->harq_processes[harq_pid]->TBS>>3;
  pdu->rx_indication_rel8.offset         = 0;  // filled in at the end of the UL_INFO formation
  pdu->data                              = eNB->ulsch[UE_id]->harq_processes[harq_pid]->b;  
  // estimate timing advance for MAC
  sync_pos                               = lte_est_timing_advance_pusch(eNB,UE_id);
  timing_advance_update                  = sync_pos - eNB->frame_parms.nb_prefix_samples/4; //to check
  switch (eNB->frame_parms.N_RB_DL) {
  case 6:
    pdu->rx_indication_rel8.timing_advance = timing_advance_update;  
    break;
  case 15:
    pdu->rx_indication_rel8.timing_advance = timing_advance_update/2;  
    break;
  case 25:
    pdu->rx_indication_rel8.timing_advance = timing_advance_update/4;  
    break;
  case 50:
    pdu->rx_indication_rel8.timing_advance = timing_advance_update/8;  
    break;
  case 75:
    pdu->rx_indication_rel8.timing_advance = timing_advance_update/12;  
    break;
  case 100:
    pdu->rx_indication_rel8.timing_advance = timing_advance_update/16;  
    break;
  }
  // put timing advance command in 0..63 range
  pdu->rx_indication_rel8.timing_advance += 31;
  if (pdu->rx_indication_rel8.timing_advance < 0)  pdu->rx_indication_rel8.timing_advance = 0;
  if (pdu->rx_indication_rel8.timing_advance > 63) pdu->rx_indication_rel8.timing_advance = 63;

  // estimate UL_CQI for MAC (from antenna port 0 only)
  int SNRtimes10 = dB_fixed_times10(eNB->pusch_vars[UE_id]->ulsch_power[0]) - 200;//(10*eNB->measurements.n0_power_dB[0]);

  if      (SNRtimes10 < -640) pdu->rx_indication_rel8.ul_cqi=0;
  else if (SNRtimes10 >  635) pdu->rx_indication_rel8.ul_cqi=255;
  else                        pdu->rx_indication_rel8.ul_cqi=(640+SNRtimes10)/5;

  eNB->UL_INFO.rx_ind.number_of_pdus++;
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);

}

void fill_crc_indication(PHY_VARS_eNB *eNB,int UE_id,int frame,int subframe,uint8_t crc_flag) {

  pthread_mutex_lock(&eNB->UL_INFO_mutex);
  nfapi_crc_indication_pdu_t *pdu =   &eNB->UL_INFO.crc_ind.crc_pdu_list[eNB->UL_INFO.crc_ind.number_of_crcs];

  pdu->instance_length                                = 0; // don't know what to do with this
  //  pdu->rx_ue_information.handle                       = handle;
  pdu->rx_ue_information.rnti                         = eNB->ulsch[UE_id]->rnti;
  pdu->crc_indication_rel8.crc_flag                   = crc_flag;
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);

  eNB->UL_INFO.crc_ind.number_of_crcs++;
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);
}

void update_harq_scheduling(LTE_DL_FRAME_PARMS *fp,LTE_UL_eNB_HARQ_t *ulsch_harq,int frame,int subframe) {
  if (fp->frame_type == FDD) {
    ulsch_harq->frame    += (ulsch_harq->subframe>1) ? 1 : 0;
    ulsch_harq->subframe  = (ulsch_harq->subframe + 8)%10;
  }
  else ulsch_harq->frame++;
}


void phy_procedures_eNB_uespec_RX(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,const relaying_type_t r_type)
{
  //RX processing for ue-specific resources (i
  UNUSED(r_type);
  uint32_t ret=0,i,j,k;
  uint32_t harq_pid, harq_idx, round;
  uint8_t nPRS;
  int sync_pos;
  uint16_t rnti=0;
  uint8_t access_mode;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  LTE_eNB_ULSCH_t *ulsch;
  LTE_UL_eNB_HARQ_t *ulsch_harq;

  const int subframe = proc->subframe_rx;
  const int frame    = proc->frame_rx;
  int offset         = eNB->CC_id;//(proc == &eNB->proc.proc_rxtx[0]) ? 0 : 1;

  uint16_t srsPeriodicity;
  uint16_t srsOffset;
  uint16_t do_srs=0;
  uint16_t is_srs_pos=0;

  T(T_ENB_PHY_UL_TICK, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe));

  T(T_ENB_PHY_INPUT_SIGNAL, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(0),
    T_BUFFER(&eNB->common_vars.rxdata[0][0][subframe*eNB->frame_parms.samples_per_tti],
             eNB->frame_parms.samples_per_tti * 4));

  if ((fp->frame_type == TDD) && (subframe_select(fp,subframe)!=SF_UL)) return;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_RX_UESPEC+offset, 1 );

#ifdef DEBUG_PHY_PROC
  LOG_D(PHY,"[eNB %d] Frame %d: Doing phy_procedures_eNB_uespec_RX(%d)\n",eNB->Mod_id,frame, subframe);
#endif


  eNB->rb_mask_ul[0]=0;
  eNB->rb_mask_ul[1]=0;
  eNB->rb_mask_ul[2]=0;
  eNB->rb_mask_ul[3]=0;

  eNB->UL_INFO.rx_ind.number_of_pdus  = 0;
  eNB->UL_INFO.crc_ind.number_of_crcs = 0;

  // Check for active processes in current subframe
  harq_pid = subframe2harq_pid(fp,
                               frame,subframe);

  is_srs_pos = is_srs_occasion_common(fp,frame,subframe);
  
  for (i=0; i<NUMBER_OF_UE_MAX; i++) {

    ulsch = eNB->ulsch[i];
    ulsch_harq = ulsch->harq_processes[harq_pid];

    // Do SRS processing 
    // check if there is SRS and we have to use shortened format
    // TODO: check for exceptions in transmission of SRS together with ACK/NACK
    do_srs=0;
    if (is_srs_pos && eNB->soundingrs_ul_config_dedicated[i].srsConfigDedicatedSetup ) {
      compute_srs_pos(fp->frame_type, eNB->soundingrs_ul_config_dedicated[i].srs_ConfigIndex, &srsPeriodicity, &srsOffset);
      if (((10*frame+subframe) % srsPeriodicity) == srsOffset) {
	do_srs = 1;
      }
    }

    if (do_srs==1) {
      if (lte_srs_channel_estimation(fp,
				     &eNB->common_vars,
				     &eNB->srs_vars[i],
				     &eNB->soundingrs_ul_config_dedicated[i],
				     subframe,
				     0/*eNB_id*/)) {
	LOG_E(PHY,"problem processing SRS\n");
      }
    }
    
    // Do PUCCH processing 

    pucch_procedures(eNB,proc,i,harq_pid, do_srs);


    // check for Msg3
    if (eNB->mac_enabled==1) {
      //      if (eNB->UE_stats[i].mode == RA_RESPONSE) {
	process_Msg3(eNB,proc,i,harq_pid);
	//      }
    }


    eNB->pusch_stats_rb[i][(frame*10)+subframe] = -63;
    eNB->pusch_stats_round[i][(frame*10)+subframe] = 0;
    eNB->pusch_stats_mcs[i][(frame*10)+subframe] = -63;

    if ((ulsch) &&
        (eNB->ulsch[i]->rnti>0) &&
        (ulsch_harq->status = ACTIVE))
      LOG_I(PHY,"Frame %d Subframe UE %d/%x active: scheduled for (%d,%d)\n",
	    frame,i,eNB->ulsch[i]->rnti,ulsch_harq->frame,ulsch_harq->subframe);
    if ((ulsch) &&
        (eNB->ulsch[i]->rnti>0) &&
        (ulsch_harq->status = ACTIVE) &&
        (ulsch_harq->subframe==subframe)&&
        (ulsch_harq->frame==frame)) {
      // UE is has ULSCH scheduling
      round = ulsch_harq->round;
 
      for (int rb=0;
           rb<=ulsch_harq->nb_rb;
	   rb++) {
	int rb2 = rb+ulsch_harq->first_rb;
	eNB->rb_mask_ul[rb2>>5] |= (1<<(rb2&31));
      }


      if (ulsch_harq->Msg3_flag == 1) {
        LOG_D(PHY,"[eNB %d] frame %d, subframe %d: Scheduling ULSCH Reception for Msg3 in Sector %d\n",
              eNB->Mod_id,
              frame,
              subframe,
              eNB->UE_stats[i].sector);
	VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_ULSCH_MSG3,1);
      } else {

        LOG_D(PHY,"[eNB %d] frame %d, subframe %d: Scheduling ULSCH Reception for UE %d Mode %s\n",
              eNB->Mod_id,
              frame,
              subframe,
              i,
              mode_string[eNB->UE_stats[i].mode]);
      }


      nPRS = fp->pusch_config_common.ul_ReferenceSignalsPUSCH.nPRS[subframe<<1];

      ulsch->cyclicShift = (ulsch_harq->n_DMRS2 + fp->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift +
				    nPRS)%12;

      if (fp->frame_type == FDD ) {
        int sf = (subframe<4) ? (subframe+6) : (subframe-4);

        if (eNB->dlsch[i][0]->subframe_tx[sf]>0) { // we have downlink transmission
          ulsch_harq->O_ACK = 1;
        } else {
          ulsch_harq->O_ACK = 0;
        }
      }

      LOG_I(PHY,
            "[eNB %d][PUSCH %d] Frame %d Subframe %d Demodulating PUSCH: dci_alloc %d, rar_alloc %d, round %d, first_rb %d, nb_rb %d, mcs %d, TBS %d, rv %d, cyclic_shift %d (n_DMRS2 %d, cyclicShift_common %d, nprs %d), O_ACK %d \n",
            eNB->Mod_id,harq_pid,frame,subframe,
            ulsch_harq->dci_alloc,
            ulsch_harq->rar_alloc,
            ulsch_harq->round,
            ulsch_harq->first_rb,
            ulsch_harq->nb_rb,
            ulsch_harq->mcs,
            ulsch_harq->TBS,
            ulsch_harq->rvidx,
            eNB->ulsch[i]->cyclicShift,
            ulsch_harq->n_DMRS2,
            fp->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift,
            nPRS,
            ulsch_harq->O_ACK);
      eNB->pusch_stats_rb[i][(frame*10)+subframe] = ulsch_harq->nb_rb;
      eNB->pusch_stats_round[i][(frame*10)+subframe] = ulsch_harq->round;
      eNB->pusch_stats_mcs[i][(frame*10)+subframe] = ulsch_harq->mcs;
      start_meas(&eNB->ulsch_demodulation_stats);

      rx_ulsch(eNB,proc,
	       i);
      

      stop_meas(&eNB->ulsch_demodulation_stats);


      start_meas(&eNB->ulsch_decoding_stats);

      ret = ulsch_decoding(eNB,proc,
			   i,
			   0, // control_only_flag
			   ulsch_harq->V_UL_DAI,
			   ulsch_harq->nb_rb>20 ? 1 : 0);
      


      stop_meas(&eNB->ulsch_decoding_stats);

      LOG_I(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d RNTI %x RX power (%d,%d) RSSI (%d,%d) N0 (%d,%d) dB ACK (%d,%d), decoding iter %d\n",
            eNB->Mod_id,harq_pid,
            frame,subframe,
            ulsch->rnti,
            dB_fixed(eNB->pusch_vars[i]->ulsch_power[0]),
            dB_fixed(eNB->pusch_vars[i]->ulsch_power[1]),
            eNB->UE_stats[i].UL_rssi[0],
            eNB->UE_stats[i].UL_rssi[1],
            20,//eNB->measurements.n0_power_dB[0],
            20,//eNB->measurements.n0_power_dB[1],
            ulsch_harq->o_ACK[0],
            ulsch_harq->o_ACK[1],
            ret);


      //compute the expected ULSCH RX power (for the stats)
      eNB->ulsch[(uint32_t)i]->harq_processes[harq_pid]->delta_TF =
        get_hundred_times_delta_IF_eNB(eNB,i,harq_pid, 0); // 0 means bw_factor is not considered

      eNB->UE_stats[i].ulsch_decoding_attempts[harq_pid][ulsch_harq->round]++;
#ifdef DEBUG_PHY_PROC
      LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d UE %d harq_pid %d Clearing subframe_scheduling_flag\n",
            eNB->Mod_id,harq_pid,frame,subframe,i,harq_pid);
#endif

      if (ulsch_harq->cqi_crc_status == 1) {
#ifdef DEBUG_PHY_PROC
        //if (((frame%10) == 0) || (frame < 50))
        print_CQI(ulsch_harq->o,ulsch_harq->uci_format,0,fp->N_RB_DL);
#endif
        extract_CQI(ulsch_harq->o,
                    ulsch_harq->uci_format,
                    &eNB->UE_stats[i],
                    fp->N_RB_DL,
                    &rnti, &access_mode);
        eNB->UE_stats[i].rank = ulsch_harq->o_RI[0];

      }

      if (ulsch_harq->Msg3_flag == 1)
	VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_ULSCH_MSG3,0);

      if (ret == (1+MAX_TURBO_ITERATIONS)) {
        T(T_ENB_PHY_ULSCH_UE_NACK, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(i), T_INT(ulsch->rnti),
          T_INT(harq_pid));

        eNB->UE_stats[i].ulsch_round_errors[harq_pid][ulsch_harq->round]++;
        ulsch_harq->phich_active = 1;
        ulsch_harq->phich_ACK = 0;
        ulsch_harq->round++;

	update_harq_scheduling(fp,ulsch_harq,frame,subframe);

	fill_crc_indication(eNB,i,frame,subframe,1); // indicate NAK to MAC

        LOG_D(PHY,"[eNB][PUSCH %d] Increasing to round %d\n",harq_pid,ulsch_harq->round);

        if (ulsch_harq->Msg3_flag == 1) {

          LOG_I(PHY,"[eNB %d/%d][RAPROC] frame %d, subframe %d, UE %d: Error receiving ULSCH (Msg3), round %d\n",
                eNB->Mod_id,
                eNB->CC_id,
                frame,subframe, i,
                ulsch_harq->round-1);

	  dump_ulsch(eNB,proc,i);
	  exit(-1);
	  
	  LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d RNTI %x RX power (%d,%d) RSSI (%d,%d) N0 (%d,%d) dB ACK (%d,%d), decoding iter %d\n",
		eNB->Mod_id,harq_pid,
		frame,subframe,
		ulsch->rnti,
		dB_fixed(eNB->pusch_vars[i]->ulsch_power[0]),
		dB_fixed(eNB->pusch_vars[i]->ulsch_power[1]),
		eNB->UE_stats[i].UL_rssi[0],
		eNB->UE_stats[i].UL_rssi[1],
		eNB->measurements.n0_power_dB[0],
		eNB->measurements.n0_power_dB[1],
		ulsch_harq->o_ACK[0],
		ulsch_harq->o_ACK[1],
		ret);
	  /*
          if (ulsch_harq->round ==
              fp->maxHARQ_Msg3Tx) {
            LOG_D(PHY,"[eNB %d][RAPROC] maxHARQ_Msg3Tx reached, abandoning RA procedure for UE %d\n",
                  eNB->Mod_id, i);
            eNB->UE_stats[i].mode = PRACH;
//	    if (eNB->mac_enabled==1) { 
//	      mac_xface->cancel_ra_proc(eNB->Mod_id,
//					eNB->CC_id,
//					frame,
//					eNB->UE_stats[i].crnti);
//				       
//	    }

	    //            mac_phy_remove_ue(eNB->Mod_id,eNB->UE_stats[i].crnti);

            eNB->ulsch[(uint32_t)i]->Msg3_active = 0;
            //ulsch_harq->phich_active = 0;

          } else {
            // activate retransmission for Msg3 (signalled to UE PHY by PHICH (not MAC/DCI)
            eNB->ulsch[(uint32_t)i]->Msg3_active = 1;

            get_Msg3_alloc_ret(fp,
                               subframe,
                               frame,
                               &ulsch_harq->frame,
                               &ulsch_harq->subframe);
	    
            //mac_xface->set_msg3_subframe(eNB->Mod_id, eNB->CC_id, frame, subframe, eNB->ulsch[i]->rnti,
            //                             eNB->ulsch[i]->Msg3_frame, eNB->ulsch[i]->Msg3_subframe);
	    

            T(T_ENB_PHY_MSG3_ALLOCATION, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe),
              T_INT(i), T_INT(ulsch->rnti), T_INT(0 
	      // 0 is for retransmission
	      ),
              T_INT(eNB->ulsch[i]->Msg3_frame), T_INT(eNB->ulsch[i]->Msg3_subframe));
          }
*/
          LOG_D(PHY,"[eNB] Frame %d, Subframe %d: Msg3 in error, i = %d \n", frame,subframe,i);
        } // This is Msg3 error

        else { //normal ULSCH
          LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d UE %d Error receiving ULSCH, round %d/%d (ACK %d,%d)\n",
                eNB->Mod_id,harq_pid,
                frame,subframe, i,
                ulsch_harq->round-1,
                ulsch->Mlimit,
                ulsch_harq->o_ACK[0],
                ulsch_harq->o_ACK[1]);

#if defined(MESSAGE_CHART_GENERATOR_PHY)
          MSC_LOG_RX_DISCARDED_MESSAGE(
				       MSC_PHY_ENB,MSC_PHY_UE,
				       NULL,0,
				       "%05u:%02u ULSCH received rnti %x harq id %u round %d",
				       frame,subframe,
				       ulsch->rnti,harq_pid,
				       ulsch_harq->round-1
				       );
#endif

          if (ulsch_harq->round== ulsch->Mlimit) {
            LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d UE %d ULSCH Mlimit %d reached\n",
                  eNB->Mod_id,harq_pid,
                  frame,subframe, i,
                  ulsch->Mlimit);

            ulsch_harq->round=0;
            ulsch_harq->phich_active=0;
            eNB->UE_stats[i].ulsch_errors[harq_pid]++;
            eNB->UE_stats[i].ulsch_consecutive_errors++;

	   /*if (ulsch_harq->nb_rb > 20) {
		dump_ulsch(eNB,proc,i);
	 	exit(-1);
           }*/
	    // indicate error to MAC
	    if (eNB->mac_enabled == 1) {
	      /*
	      mac_xface->rx_sdu(eNB->Mod_id,
				eNB->CC_id,
				frame,subframe,
				ulsch->rnti,
				NULL,
				0,
				harq_pid,
				&eNB->ulsch[i]->Msg3_flag);
	      */
	    }
          }
        }
      }  // ulsch in error
      else {


	fill_crc_indication(eNB,i,frame,subframe,0); // indicate ACK to MAC
	fill_rx_indication(eNB,i,frame,subframe);    // indicate SDU to MAC
        T(T_ENB_PHY_ULSCH_UE_ACK, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(i), T_INT(ulsch->rnti),
          T_INT(harq_pid));

        if (ulsch_harq->Msg3_flag == 1) {
	  LOG_D(PHY,"[eNB %d][PUSCH %d] Frame %d subframe %d ULSCH received, setting round to 0, PHICH ACK\n",
		eNB->Mod_id,harq_pid,
		frame,subframe);
	  LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d RNTI %x RX power (%d,%d) RSSI (%d,%d) N0 (%d,%d) dB ACK (%d,%d), decoding iter %d\n",
		eNB->Mod_id,harq_pid,
		frame,subframe,
		ulsch->rnti,
		dB_fixed(eNB->pusch_vars[i]->ulsch_power[0]),
		dB_fixed(eNB->pusch_vars[i]->ulsch_power[1]),
		eNB->UE_stats[i].UL_rssi[0],
		eNB->UE_stats[i].UL_rssi[1],
		eNB->measurements.n0_power_dB[0],
		eNB->measurements.n0_power_dB[1],
		ulsch_harq->o_ACK[0],
		ulsch_harq->o_ACK[1],
		ret);
	}
#if defined(MESSAGE_CHART_GENERATOR_PHY)
        MSC_LOG_RX_MESSAGE(
			   MSC_PHY_ENB,MSC_PHY_UE,
			   NULL,0,
			   "%05u:%02u ULSCH received rnti %x harq id %u",
			   frame,subframe,
			   ulsch->rnti,harq_pid
			   );
#endif
        for (j=0; j<fp->nb_antennas_rx; j++)
          //this is the RSSI per RB
          eNB->UE_stats[i].UL_rssi[j] =
	    
            dB_fixed(eNB->pusch_vars[i]->ulsch_power[j]*
                     (ulsch_harq->nb_rb*12)/
                     fp->ofdm_symbol_size) -
            eNB->rx_total_gain_dB -
            hundred_times_log10_NPRB[ulsch_harq->nb_rb-1]/100 -
            get_hundred_times_delta_IF_eNB(eNB,i,harq_pid, 0)/100;
	    
        ulsch_harq->phich_active = 1;
        ulsch_harq->phich_ACK = 1;
        ulsch_harq->round = 0;
        eNB->UE_stats[i].ulsch_consecutive_errors = 0;

        if (ulsch_harq->Msg3_flag == 1) {
	  if (eNB->mac_enabled==1) {

	    LOG_I(PHY,"[eNB %d][RAPROC] Frame %d Terminating ra_proc for harq %d, UE %d\n",
		  eNB->Mod_id,
		  frame,harq_pid,i);
	    if (eNB->mac_enabled) {
	      /*
	      mac_xface->rx_sdu(eNB->Mod_id,
				eNB->CC_id,
				frame,subframe,
				eNB->ulsch[i]->rnti,
				ulsch_harq->b,
				ulsch_harq->TBS>>3,
				harq_pid,
				&ulsch->Msg3_flag);
	      */
	      // Fill UL info
	    }
	    // one-shot msg3 detection by MAC: empty PDU (e.g. CRNTI)
	    if (ulsch_harq->Msg3_flag == 0 ) {
	      eNB->UE_stats[i].mode = PRACH;
	      /*
	      mac_xface->cancel_ra_proc(eNB->Mod_id,
					eNB->CC_id,
					frame,
					eNB->UE_stats[i].crnti);
	      mac_phy_remove_ue(eNB->Mod_id,eNB->UE_stats[i].crnti);
	      */
	      
	      eNB->ulsch[(uint32_t)i]->Msg3_active = 0;
	    } // Msg3_flag == 0
	    
	  } // mac_enabled==1

          eNB->UE_stats[i].mode = PUSCH;
          ulsch_harq->Msg3_flag = 0;

	  LOG_D(PHY,"[eNB %d][RAPROC] Frame %d : RX Subframe %d Setting UE %d mode to PUSCH\n",eNB->Mod_id,frame,subframe,i);

          for (k=0; k<8; k++) { //harq_processes
            for (j=0; j<eNB->dlsch[i][0]->Mlimit; j++) {
              eNB->UE_stats[i].dlsch_NAK[k][j]=0;
              eNB->UE_stats[i].dlsch_ACK[k][j]=0;
              eNB->UE_stats[i].dlsch_trials[k][j]=0;
            }

            eNB->UE_stats[i].dlsch_l2_errors[k]=0;
            eNB->UE_stats[i].ulsch_errors[k]=0;
            eNB->UE_stats[i].ulsch_consecutive_errors=0;

            for (j=0; j<ulsch->Mlimit; j++) {
              eNB->UE_stats[i].ulsch_decoding_attempts[k][j]=0;
              eNB->UE_stats[i].ulsch_decoding_attempts_last[k][j]=0;
              eNB->UE_stats[i].ulsch_round_errors[k][j]=0;
              eNB->UE_stats[i].ulsch_round_fer[k][j]=0;
            }
          }

          eNB->UE_stats[i].dlsch_sliding_cnt=0;
          eNB->UE_stats[i].dlsch_NAK_round0=0;
          eNB->UE_stats[i].dlsch_mcs_offset=0;
        } // Msg3_flag==1
	else {  // Msg3_flag == 0

#ifdef DEBUG_PHY_PROC
#ifdef DEBUG_ULSCH
          LOG_D(PHY,"[eNB] Frame %d, Subframe %d : ULSCH SDU (RX harq_pid %d) %d bytes:",frame,subframe,
                harq_pid,ulsch_harq->TBS>>3);

          for (j=0; j<ulsch_harq->TBS>>3; j++)
            LOG_T(PHY,"%x.",eNB->ulsch[i]->harq_processesyy[harq_pid]->b[j]);

          LOG_T(PHY,"\n");
#endif
#endif

	  if (eNB->mac_enabled==1) {
	    /*
	    mac_xface->rx_sdu(eNB->Mod_id,
			      eNB->CC_id,
			      frame,subframe,
			      ulsch->rnti,
			      eNB->ulsch[i]->harq_processes[harq_pid]->b,
			      eNB->ulsch[i]->harq_processes[harq_pid]->TBS>>3,
			      harq_pid,
			      NULL);*/

	    // Fill UL_INFO

	  } // mac_enabled==1
        } // Msg3_flag == 0

        

#ifdef DEBUG_PHY_PROC
        LOG_D(PHY,"[eNB %d] frame %d, subframe %d: user %d: timing advance = %d\n",
              eNB->Mod_id,
              frame, subframe,
              i,
              eNB->UE_stats[i].timing_advance_update);
#endif


      }  // ulsch not in error

      // process HARQ feedback
#ifdef DEBUG_PHY_PROC
      LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d subframe %d, Processing HARQ feedback for UE %d (after PUSCH)\n",eNB->Mod_id,
            eNB->dlsch[i][0]->rnti,
            frame,subframe,
            i);
#endif
      process_HARQ_feedback(i,
                            eNB,proc,
                            1, // pusch_flag
                            0,
                            0,
                            0);

#ifdef DEBUG_PHY_PROC
      LOG_D(PHY,"[eNB %d] Frame %d subframe %d, sect %d: received ULSCH harq_pid %d for UE %d, ret = %d, CQI CRC Status %d, ACK %d,%d, ulsch_errors %d/%d\n",
            eNB->Mod_id,frame,subframe,
            eNB->UE_stats[i].sector,
            harq_pid,
            i,
            ret,
            eNB->ulsch[i]->harq_processes[harq_pid]->cqi_crc_status,
            eNB->ulsch[i]->harq_processes[harq_pid]->o_ACK[0],
            eNB->ulsch[i]->harq_processes[harq_pid]->o_ACK[1],
            eNB->UE_stats[i].ulsch_errors[harq_pid],
            eNB->UE_stats[i].ulsch_decoding_attempts[harq_pid][0]);
#endif
      
      // dump stats to VCD
      if (i==0) {
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_MCS0+harq_pid,eNB->pusch_stats_mcs[0][(frame*10)+subframe]);
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_RB0+harq_pid,eNB->pusch_stats_rb[0][(frame*10)+subframe]);
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_ROUND0+harq_pid,eNB->pusch_stats_round[0][(frame*10)+subframe]);
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_RSSI0+harq_pid,dB_fixed(eNB->pusch_vars[0]->ulsch_power[0]));
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_RES0+harq_pid,ret);
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_SFN0+harq_pid,(frame*10)+subframe);
      }
    } // ulsch[0] && ulsch[0]->rnti>0 && ulsch[0]->subframe_scheduling_flag == 1


    // update ULSCH statistics for tracing
    if ((frame % 100 == 0) && (subframe == 4)) {
      for (harq_idx=0; harq_idx<8; harq_idx++) {
        for (round=0; round<ulsch->Mlimit; round++) {
          if ((eNB->UE_stats[i].ulsch_decoding_attempts[harq_idx][round] -
               eNB->UE_stats[i].ulsch_decoding_attempts_last[harq_idx][round]) != 0) {
            eNB->UE_stats[i].ulsch_round_fer[harq_idx][round] =
              (100*(eNB->UE_stats[i].ulsch_round_errors[harq_idx][round] -
                    eNB->UE_stats[i].ulsch_round_errors_last[harq_idx][round]))/
              (eNB->UE_stats[i].ulsch_decoding_attempts[harq_idx][round] -
               eNB->UE_stats[i].ulsch_decoding_attempts_last[harq_idx][round]);
          } else {
            eNB->UE_stats[i].ulsch_round_fer[harq_idx][round] = 0;
          }

          eNB->UE_stats[i].ulsch_decoding_attempts_last[harq_idx][round] =
            eNB->UE_stats[i].ulsch_decoding_attempts[harq_idx][round];
          eNB->UE_stats[i].ulsch_round_errors_last[harq_idx][round] =
            eNB->UE_stats[i].ulsch_round_errors[harq_idx][round];
        }
      }
    }

    if ((frame % 100 == 0) && (subframe==4)) {
      eNB->UE_stats[i].dlsch_bitrate = (eNB->UE_stats[i].total_TBS -
					eNB->UE_stats[i].total_TBS_last);

      eNB->UE_stats[i].total_TBS_last = eNB->UE_stats[i].total_TBS;
    }

  } // loop i=0 ... NUMBER_OF_UE_MAX-1

  lte_eNB_I0_measurements(eNB,
			  subframe,
			  0,
			  eNB->first_run_I0_measurements);
  eNB->first_run_I0_measurements = 0;
  


  //}

#if defined(FLEXRAN_AGENT_SB_IF)
#ifndef DISABLE_SF_TRIGGER
  //Send subframe trigger to the controller
  if (mac_agent_registered[eNB->Mod_id]) {
    agent_mac_xface[eNB->Mod_id]->flexran_agent_send_sf_trigger(eNB->Mod_id);
  }
#endif
#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_RX_UESPEC+offset, 0 );

  stop_meas(&eNB->phy_proc_rx);

}

#undef DEBUG_PHY_PROC

#if defined(Rel10) || defined(Rel14)
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

