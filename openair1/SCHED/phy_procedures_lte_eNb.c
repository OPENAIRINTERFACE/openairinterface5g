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

/*! \file phy_procedures_lte_eNb.c
 * \brief Implementation of eNB procedures from 36.213 LTE specifications / FeMBMS 36.231 LTE procedures v14.2
 * \author R. Knopp, F. Kaltenberger, N. Nikaein, X. Foukas, J. Morgade
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr,navid.nikaein@eurecom.fr, x.foukas@sms.ed.ac.uk, javier.morgade@ieee.org
 * \note
 * \warning
 */

#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h"
#include "SCHED/sched_eNB.h"
#include "SCHED/sched_common_extern.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "fapi_l1.h"
#include "common/utils/LOG/log.h"
#include <common/utils/system.h>
#include "common/utils/LOG/vcd_signal_dumper.h"
#include <nfapi/oai_integration/nfapi_pnf.h>

#include "assertions.h"

#include <time.h>

#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;


int16_t get_hundred_times_delta_IF_eNB(PHY_VARS_eNB *eNB,uint16_t ULSCH_id,uint8_t harq_pid, uint8_t bw_factor) {
  uint32_t Nre,sumKr,MPR_x100,Kr,r;
  uint16_t beta_offset_pusch;
  DevAssert( ULSCH_id < NUMBER_OF_ULSCH_MAX+1 );
  DevAssert( harq_pid < 8 );
  Nre = eNB->ulsch[ULSCH_id]->harq_processes[harq_pid]->Nsymb_initial *
        eNB->ulsch[ULSCH_id]->harq_processes[harq_pid]->nb_rb*12;
  sumKr = 0;

  for (r=0; r<eNB->ulsch[ULSCH_id]->harq_processes[harq_pid]->C; r++) {
    if (r<eNB->ulsch[ULSCH_id]->harq_processes[harq_pid]->Cminus)
      Kr = eNB->ulsch[ULSCH_id]->harq_processes[harq_pid]->Kminus;
    else
      Kr = eNB->ulsch[ULSCH_id]->harq_processes[harq_pid]->Kplus;

    sumKr += Kr;
  }

  if (Nre==0)
    return(0);

  MPR_x100 = 100*sumKr/Nre;
  // Note: MPR=is the effective spectral efficiency of the PUSCH
  // FK 20140908 sumKr is only set after the ulsch_encoding
  beta_offset_pusch = 8;
  //(eNB->ulsch[UE_id]->harq_processes[harq_pid]->control_only == 1) ? eNB->ulsch[UE_id]->beta_offset_cqi_times8:8;
  DevAssert( ULSCH_id < NUMBER_OF_ULSCH_MAX );
  //#warning "This condition happens sometimes. Need more investigation" // navid
  //DevAssert( MPR_x100/6 < 100 );

  if (1==1) { //eNB->ul_power_control_dedicated[UE_id].deltaMCS_Enabled == 1) {
    // This is the formula from Section 5.1.1.1 in 36.213 10*log10(deltaIF_PUSCH = (2^(MPR*Ks)-1)*beta_offset_pusch)
    if (bw_factor == 1) {
      uint8_t nb_rb = eNB->ulsch[ULSCH_id]->harq_processes[harq_pid]->nb_rb;
      return(hundred_times_delta_TF[MPR_x100/6]+10*dB_fixed_x10((beta_offset_pusch)>>3)) + hundred_times_log10_NPRB[nb_rb-1];
    } else
      return(hundred_times_delta_TF[MPR_x100/6]+10*dB_fixed_x10((beta_offset_pusch)>>3));
  } else {
    return(0);
  }
}


int16_t get_hundred_times_delta_IF_mac(module_id_t module_idP, uint8_t CC_id, rnti_t rnti, uint8_t harq_pid) {
  int8_t ULSCH_id;

  if ((RC.eNB == NULL) || (module_idP > RC.nb_inst) || (CC_id > RC.nb_CC[module_idP])) {
    LOG_E(PHY,"get_UE_stats: No eNB found (or not allocated) for Mod_id %d,CC_id %d\n",module_idP,CC_id);
    return -1;
  }

  ULSCH_id = find_ulsch( rnti, RC.eNB[module_idP][CC_id],SEARCH_EXIST);

  if (ULSCH_id == -1) {
    // not found
    return 0;
  }

  return get_hundred_times_delta_IF_eNB( RC.eNB[module_idP][CC_id], ULSCH_id, harq_pid, 0 );
}

int oai_nfapi_rach_ind(nfapi_rach_indication_t *rach_ind);


lte_subframe_t get_subframe_direction(uint8_t Mod_id,uint8_t CC_id,uint8_t subframe) {
  return(subframe_select(&RC.eNB[Mod_id][CC_id]->frame_parms,subframe));
}

void pmch_procedures(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc, int fembms_flag) {
  int subframe = proc->subframe_tx;
  // This is DL-Cell spec pilots in Control region
  if(!fembms_flag){
  	generate_pilots_slot (eNB, eNB->common_vars.txdataF, AMP, subframe << 1, 1);
 	generate_mbsfn_pilot(eNB,proc,
                       eNB->common_vars.txdataF,
                       AMP);
  }else
	generate_mbsfn_pilot_khz_1dot25(eNB,proc,
                       eNB->common_vars.txdataF,
                       AMP);

  if(eNB->dlsch_MCH->active==1){
	if(!fembms_flag){
  	  generate_mch (eNB, proc,NULL/*, eNB->dlsch_MCH->harq_processes[0]->pdu*/);
        }
        else{
 	  generate_mch_khz_1dot25 (eNB, proc,NULL/*, eNB->dlsch_MCH->harq_processes[0]->pdu*/);
        }

    	  LOG_D(PHY,"[eNB%"PRIu8"] Frame %d subframe %d : Got MCH pdu for MBSFN (TBS %d) fembms %d \n",
          eNB->Mod_id,proc->frame_tx,subframe,
          eNB->dlsch_MCH->harq_processes[0]->TBS>>3,fembms_flag);

  }
  eNB->dlsch_MCH->active = 0;
}

void common_signal_procedures_fembms (PHY_VARS_eNB *eNB,int frame, int subframe) {
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int **txdataF = eNB->common_vars.txdataF;
  uint8_t *pbch_pdu=&eNB->pbch_pdu[0];

  if((frame&3)!=0 /*&& subframe != 0*/)
	  return;

  LOG_I(PHY,"common_signal_procedures: frame %d, subframe %d fdd:%s dir:%s index:%d\n",frame,subframe,fp->frame_type == FDD?"FDD":"TDD", subframe_select(fp,subframe) == SF_DL?"DL":"UL?",(frame&15)/4);
  // generate Cell-Specific Reference Signals for both slots
  //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_RS_TX,1);


  if(subframe_select(fp,subframe) == SF_S)
    generate_pilots_slot(eNB,
                         txdataF,
                         AMP,
                         subframe<<1,1);
  else
    generate_pilots_slot(eNB,
                         txdataF,
                         AMP,
                         subframe<<1,0);

  // check that 2nd slot is for DL
  if (subframe_select (fp, subframe) == SF_DL)
    generate_pilots_slot (eNB, txdataF, AMP, (subframe << 1) + 1, 0);

  //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME (VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_RS_TX, 0);

  // First half of PSS/SSS (FDD, slot 0)
  if (subframe == 0) {
    //if (fp->frame_type == FDD) {
      generate_pss (txdataF, AMP, fp, (fp->Ncp == NORMAL) ? 6 : 5, 0);
      generate_sss (txdataF, AMP, fp, (fp->Ncp == NORMAL) ? 5 : 4, 0);
    //}

    /// First half of SSS (TDD, slot 1)

    //if (fp->frame_type == TDD) {
      generate_sss (txdataF, AMP, fp, (fp->Ncp == NORMAL) ? 6 : 5, 1);
    //}

    // generate PBCH (Physical Broadcast CHannel) info

    /// generate PBCH
    if ((frame&15)==0) {
      //AssertFatal(eNB->pbch_configured==1,"PBCH was not configured by MAC\n");
      if (eNB->pbch_configured!=1) return;

      eNB->pbch_configured=0;
    }

#if T_TRACER
    if (T_ACTIVE(T_ENB_PHY_MIB)) {
      /* MIB is stored in reverse in pbch_pdu, reverse it for properly logging */
      uint8_t mib[3];
      mib[0] = pbch_pdu[2];
      mib[1] = pbch_pdu[1];
      mib[2] = pbch_pdu[0];
      T(T_ENB_PHY_MIB, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe),
        T_BUFFER(mib, 3));
    }
#endif
    generate_pbch_fembms (&eNB->pbch, txdataF, AMP, fp, pbch_pdu, (frame & 15)/4);
  } //else if ((subframe == 1) && (fp->frame_type == TDD)) {
    //generate_pss (txdataF, AMP, fp, 2, 2);
 // }
  // Second half of PSS/SSS (FDD, slot 10)
  else if ((subframe == 5) && (fp->frame_type == FDD) && is_fembms_nonMBSFN_subframe(frame,subframe,fp)) {
     generate_pss (txdataF, AMP, &eNB->frame_parms, (fp->Ncp == NORMAL) ? 6 : 5, 10);
     generate_sss (txdataF, AMP, &eNB->frame_parms, (fp->Ncp == NORMAL) ? 5 : 4, 10);
  }
  //  Second-half of SSS (TDD, slot 11)
 // else if ((subframe == 5) && (fp->frame_type == TDD)) {
 //   generate_sss (txdataF, AMP, fp, (fp->Ncp == NORMAL) ? 6 : 5, 11);
 // }
  // Second half of PSS (TDD, slot 12)
 // else if ((subframe == 6) && (fp->frame_type == TDD)) {
 //   generate_pss (txdataF, AMP, fp, 2, 12);
 // }
}

void common_signal_procedures (PHY_VARS_eNB *eNB,int frame, int subframe) {
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int **txdataF = eNB->common_vars.txdataF;
  uint8_t *pbch_pdu=&eNB->pbch_pdu[0];
  //LOG_D(PHY,"common_signal_procedures: frame %d, subframe %d fdd:%s dir:%s\n",frame,subframe,fp->frame_type == FDD?"FDD":"TDD", subframe_select(fp,subframe) == SF_DL?"DL":"UL?");
  // generate Cell-Specific Reference Signals for both slots
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_RS_TX,1);

  if(subframe_select(fp,subframe) == SF_S)
    generate_pilots_slot(eNB,
                         txdataF,
                         AMP,
                         subframe<<1,1);
  else
    generate_pilots_slot(eNB,
                         txdataF,
                         AMP,
                         subframe<<1,0);

  // check that 2nd slot is for DL
  if (subframe_select (fp, subframe) == SF_DL)
    generate_pilots_slot (eNB, txdataF, AMP, (subframe << 1) + 1, 0);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME (VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_RS_TX, 0);

  // First half of PSS/SSS (FDD, slot 0)
  if (subframe == 0) {
    if (fp->frame_type == FDD) {
      generate_pss (txdataF, AMP, fp, (fp->Ncp == NORMAL) ? 6 : 5, 0);
      generate_sss (txdataF, AMP, fp, (fp->Ncp == NORMAL) ? 5 : 4, 0);
    }

    /// First half of SSS (TDD, slot 1)

    if (fp->frame_type == TDD) {
      generate_sss (txdataF, AMP, fp, (fp->Ncp == NORMAL) ? 6 : 5, 1);
    }

    // generate PBCH (Physical Broadcast CHannel) info

    /// generate PBCH
    if ((frame&3)==0) {
      //AssertFatal(eNB->pbch_configured==1,"PBCH was not configured by MAC\n");
      if (eNB->pbch_configured!=1) return;

      eNB->pbch_configured=0;
    }

#if T_TRACER
    if (T_ACTIVE(T_ENB_PHY_MIB)) {
      /* MIB is stored in reverse in pbch_pdu, reverse it for properly logging */
      uint8_t mib[3];
      mib[0] = pbch_pdu[2];
      mib[1] = pbch_pdu[1];
      mib[2] = pbch_pdu[0];
      T(T_ENB_PHY_MIB, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe),
        T_BUFFER(mib, 3));
    }
#endif
    generate_pbch (&eNB->pbch, txdataF, AMP, fp, pbch_pdu, frame & 3);
  } else if ((subframe == 1) && (fp->frame_type == TDD)) {
    generate_pss (txdataF, AMP, fp, 2, 2);
  }
  // Second half of PSS/SSS (FDD, slot 10)
  else if ((subframe == 5) && (fp->frame_type == FDD)) {
    generate_pss (txdataF, AMP, &eNB->frame_parms, (fp->Ncp == NORMAL) ? 6 : 5, 10);
    generate_sss (txdataF, AMP, &eNB->frame_parms, (fp->Ncp == NORMAL) ? 5 : 4, 10);
  }
  //  Second-half of SSS (TDD, slot 11)
  else if ((subframe == 5) && (fp->frame_type == TDD)) {
    generate_sss (txdataF, AMP, fp, (fp->Ncp == NORMAL) ? 6 : 5, 11);
  }
  // Second half of PSS (TDD, slot 12)
  else if ((subframe == 6) && (fp->frame_type == TDD)) {
    generate_pss (txdataF, AMP, fp, 2, 12);
  }
}



bool dlsch_procedures(PHY_VARS_eNB *eNB,
                      L1_rxtx_proc_t *proc,
                      int harq_pid,
                      LTE_eNB_DLSCH_t *dlsch,
                      LTE_eNB_UE_stats *ue_stats) {
  int frame=proc->frame_tx;
  int subframe=proc->subframe_tx;
  LTE_DL_eNB_HARQ_t *dlsch_harq=dlsch->harq_processes[harq_pid];
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;

  if (dlsch->rnti != 0xffff) {//frame < 200) {
    LOG_D(PHY,
          "[eNB %"PRIu8"][PDSCH %"PRIx16"/%"PRIu8"] Frame %d, subframe %d: Generating PDSCH/DLSCH (type %d) with input size = %"PRIu16", pdsch_start %d, G %d, nb_rb %"PRIu16", rb0 %x, rb1 %x, TBS %"PRIu16", pmi_alloc %"PRIx64", rv %"PRIu8" (round %"PRIu8")\n",
          eNB->Mod_id, dlsch->rnti,harq_pid,
          frame, subframe, dlsch->ue_type,dlsch_harq->TBS/8, dlsch_harq->pdsch_start,
          get_G(fp,
                dlsch_harq->nb_rb,
                dlsch_harq->rb_alloc,
                dlsch_harq->Qm,
                dlsch_harq->Nl,
                dlsch_harq->pdsch_start,
                frame,
                subframe,
                dlsch_harq->mimo_mode==TM7?7:0),
          dlsch_harq->nb_rb,
          dlsch_harq->rb_alloc[0],
          dlsch_harq->rb_alloc[1],
          dlsch_harq->TBS,
          pmi2hex_2Ar1(dlsch_harq->pmi_alloc),
          dlsch_harq->rvidx,
          dlsch_harq->round);
  }

  if (ue_stats) ue_stats->dlsch_sliding_cnt++;

  if (dlsch_harq->round == 0) {
    if (ue_stats)
      ue_stats->dlsch_trials[harq_pid][0]++;
  } else {
    ue_stats->dlsch_trials[harq_pid][dlsch_harq->round]++;
#ifdef DEBUG_PHY_PROC
#ifdef DEBUG_DLSCH
    LOG_D (PHY, "[eNB] This DLSCH is a retransmission\n");
#endif
#endif
  }

  if (dlsch->rnti!=0xffff)
    LOG_D(PHY,"Generating DLSCH/PDSCH pdu:%p pdsch_start:%d frame:%d subframe:%d nb_rb:%d rb_alloc:%d Qm:%d Nl:%d round:%d\n",
          dlsch_harq->pdu,dlsch_harq->pdsch_start,frame,subframe,dlsch_harq->nb_rb,dlsch_harq->rb_alloc[0],
          dlsch_harq->Qm,dlsch_harq->Nl,dlsch_harq->round);

  // 36-212
  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) { // monolthic OR PNF - do not need turbo encoding on VNF
    if (dlsch_harq->pdu==NULL) {
      LOG_E(PHY,"dlsch_harq->pdu == NULL SFN/SF:%04d%d dlsch[rnti:%x] dlsch_harq[pdu:%p pdsch_start:%d Qm:%d Nl:%d round:%d nb_rb:%d rb_alloc[0]:%d]\n", frame,subframe,dlsch->rnti, dlsch_harq->pdu,
            dlsch_harq->pdsch_start,dlsch_harq->Qm,dlsch_harq->Nl,dlsch_harq->round,dlsch_harq->nb_rb,dlsch_harq->rb_alloc[0]);
      return false;
    }

    start_meas(&eNB->dlsch_encoding_stats);
    dlsch_encoding(eNB, proc, dlsch_harq->pdu, dlsch_harq->pdsch_start, dlsch, frame, subframe, &eNB->dlsch_rate_matching_stats, &eNB->dlsch_turbo_encoding_stats, &eNB->dlsch_interleaving_stats);
    stop_meas(&eNB->dlsch_encoding_stats);

    if(eNB->dlsch_encoding_stats.p_time>500*3000 && opp_enabled == 1) {
      print_meas_now(&eNB->dlsch_encoding_stats,"total coding",stderr);
    }

#ifdef PHY_TX_THREAD
    dlsch->active[subframe] = 0;
#else
    dlsch->active = 0;
#endif
    dlsch_harq->round++;
    LOG_D(PHY,"Generated DLSCH dlsch_harq[round:%d]\n",dlsch_harq->round);
    return true;
  }

  return false;
}

void pdsch_procedures(PHY_VARS_eNB *eNB,
                      L1_rxtx_proc_t *proc,
                      int harq_pid,
                      LTE_eNB_DLSCH_t *dlsch,
                      LTE_eNB_DLSCH_t *dlsch1) {
  int frame=proc->frame_tx;
  int subframe=proc->subframe_tx;
  LTE_DL_eNB_HARQ_t *dlsch_harq=dlsch->harq_processes[harq_pid];
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  // 36-211
  start_meas(&eNB->dlsch_scrambling_stats);
  dlsch_scrambling(fp,
                   0,
                   dlsch,
                   harq_pid,
                   get_G(fp,
                         dlsch_harq->nb_rb,
                         dlsch_harq->rb_alloc,
                         dlsch_harq->Qm,
                         dlsch_harq->Nl,
                         dlsch_harq->pdsch_start,
                         frame,subframe,
                         0),
                   0,
                   frame,
                   subframe<<1);
  stop_meas(&eNB->dlsch_scrambling_stats);
  start_meas(&eNB->dlsch_modulation_stats);
  dlsch_modulation(eNB,
                   eNB->common_vars.txdataF,
                   AMP,
                   frame,
                   subframe,
                   dlsch_harq->pdsch_start,
                   dlsch,
                   dlsch->ue_type==0 ? dlsch1 : (LTE_eNB_DLSCH_t *)NULL);
  stop_meas(&eNB->dlsch_modulation_stats);
  LOG_D(PHY,"Generated PDSCH dlsch_harq[round:%d]\n",dlsch_harq->round);
}



void phy_procedures_eNB_TX(PHY_VARS_eNB *eNB,
                           L1_rxtx_proc_t *proc,
                           int do_meas) {
  int frame=proc->frame_tx;
  int subframe=proc->subframe_tx;
  uint8_t harq_pid;
  uint8_t num_pdcch_symbols=0;
  uint8_t num_dci=0;
  uint8_t         num_mdci = 0;
  uint8_t ul_subframe;
  uint32_t ul_frame;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  LTE_UL_eNB_HARQ_t *ulsch_harq;

  if ((fp->frame_type == TDD) && (subframe_select (fp, subframe) == SF_UL))
    return;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_TX+(eNB->CC_id),1);

  if (do_meas==1) start_meas(&eNB->phy_proc_tx);
  if (do_meas==1) start_meas(&eNB->dlsch_common_and_dci);

  // clear the transmit data array for the current subframe
  for (int aa = 0; aa < fp->nb_antenna_ports_eNB; aa++) {
    if (eNB->use_DTX==0) 
      memcpy(&eNB->common_vars.txdataF[aa][subframe * fp->ofdm_symbol_size * (fp->symbols_per_tti)],
	     &eNB->subframe_mask[aa][subframe*fp->ofdm_symbol_size*fp->symbols_per_tti],
	     fp->ofdm_symbol_size * (fp->symbols_per_tti) * sizeof (int32_t));
    else memset(&eNB->common_vars.txdataF[aa][subframe * fp->ofdm_symbol_size * (fp->symbols_per_tti)], 0, fp->ofdm_symbol_size * (fp->symbols_per_tti) * sizeof (int32_t));
  }

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    if (is_fembms_pmch_subframe(frame,subframe,fp)) {
      pmch_procedures(eNB,proc,1);
      LOG_D(MAC,"frame %d, subframe %d -> PMCH\n",frame,subframe);
      return;
    }else if(is_fembms_cas_subframe(frame,subframe,fp) || is_fembms_nonMBSFN_subframe(frame,subframe,fp)){
         LOG_D(MAC,"frame %d, subframe %d -> CAS\n",frame,subframe);
	common_signal_procedures_fembms(eNB,proc->frame_tx, proc->subframe_tx);
	//return;
    }

  if((!is_fembms_cas_subframe(frame,subframe,fp)) && (!is_fembms_nonMBSFN_subframe(frame,subframe,fp))){
    if (is_pmch_subframe(frame,subframe,fp)) {
      pmch_procedures(eNB,proc,0);
    } else {
      // this is not a pmch subframe, so generate PSS/SSS/PBCH
      common_signal_procedures(eNB,proc->frame_tx, proc->subframe_tx);
    }
   }
  }

  // clear existing ulsch dci allocations before applying info from MAC  (this is table
  ul_subframe = pdcch_alloc2ul_subframe (fp, subframe);
  ul_frame = pdcch_alloc2ul_frame (fp, frame, subframe);

  // clear previous allocation information for all UEs
  for (volatile int i = 0; i < NUMBER_OF_DLSCH_MAX; i++) {
    if (eNB->dlsch[i][0])
      eNB->dlsch[i][0]->subframe_tx[subframe] = 0;
  }

  /* TODO: check the following test - in the meantime it is put back as it was before */
  //if ((ul_subframe < 10)&&
  //    (subframe_select(fp,ul_subframe)==SF_UL)) { // This means that there is a potential UL subframe that will be scheduled here
  if (ul_subframe < 10) { // This means that there is a potential UL subframe that will be scheduled here
    for (volatile int i=0; i<NUMBER_OF_DLSCH_MAX; i++) {
      if (eNB->ulsch[i] && eNB->ulsch[i]->ue_type >0) harq_pid = 0;

      else
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
        ulsch_harq->previous_first_rb = ulsch_harq->first_rb;
        ulsch_harq->previous_n_DMRS = ulsch_harq->n_DMRS;
      }
    }
  }

  num_pdcch_symbols = eNB->pdcch_vars[subframe&1].num_pdcch_symbols;
  num_dci           = eNB->pdcch_vars[subframe&1].num_dci;
  LOG_D(PHY,"num_pdcch_symbols %"PRIu8",number dci %"PRIu8"\n",num_pdcch_symbols, num_dci);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DCI_INFO,num_pdcch_symbols);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME (VCD_SIGNAL_DUMPER_VARIABLES_DCI_INFO, (frame * 10) + subframe);

  if (num_pdcch_symbols == 0) {
    LOG_E(PHY,"[eNB %"PRIu8"] Frame %d, subframe %d: Calling generate_dci_top (pdcch) (num_dci %"PRIu8") num_pdcch_symbols:%d\n",eNB->Mod_id,frame, subframe, num_dci, num_pdcch_symbols);
    return;
  }

  if (num_dci > 0)
    LOG_D(PHY,"[eNB %"PRIu8"] Frame %d, subframe %d: Calling generate_dci_top (pdcch) (num_dci %"PRIu8") num_pdcch_symbols:%d\n",eNB->Mod_id,frame, subframe, num_dci, num_pdcch_symbols);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PDCCH_TX,1);

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    generate_dci_top(num_pdcch_symbols,
                     num_dci,
                     &eNB->pdcch_vars[subframe&1].dci_alloc[0],
                     0,
                     AMP,
                     fp,
                     eNB->common_vars.txdataF,
                     subframe);
    num_mdci = eNB->mpdcch_vars[subframe &1].num_dci;

    if (num_mdci > 0) {
      LOG_D (PHY, "[eNB %" PRIu8 "] Frame %d, subframe %d: Calling generate_mdci_top (mpdcch) (num_dci %" PRIu8 ")\n", eNB->Mod_id, frame, subframe, num_mdci);
      generate_mdci_top (eNB, frame, subframe, AMP, eNB->common_vars.txdataF);
    }
  }

  if (do_meas==1) stop_meas(&eNB->dlsch_common_and_dci);

  if (do_meas==1) start_meas(&eNB->dlsch_ue_specific);

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    if (is_fembms_pmch_subframe(frame,subframe,fp)) {
      pmch_procedures(eNB,proc,1);
      return;
    }else if(is_fembms_cas_subframe(frame,subframe,fp)){
	common_signal_procedures_fembms(eNB,proc->frame_tx, proc->subframe_tx);
    }
  }


  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PDCCH_TX,0);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GENERATE_DLSCH,1);
  // Now scan UE specific DLSCH
  LTE_eNB_DLSCH_t *dlsch0,*dlsch1;

  for (int DLSCH_id=0; DLSCH_id<NUMBER_OF_DLSCH_MAX; DLSCH_id++) {
    dlsch0 = eNB->dlsch[(uint8_t)DLSCH_id][0];
    dlsch1 = eNB->dlsch[(uint8_t)DLSCH_id][1];

    if ((dlsch0)&&(dlsch0->rnti>0)&&
#ifdef PHY_TX_THREAD
        (dlsch0->active[subframe] == 1)
#else
        (dlsch0->active == 1)
#endif
       ) {
      // get harq_pid
        harq_pid = dlsch0->harq_ids[frame%2][subframe];
      //AssertFatal(harq_pid>=0,"harq_pid is negative\n");

      if((harq_pid < 0) || (harq_pid >= dlsch0->Mdlharq)) {

        if (dlsch0->ue_type==0)
          LOG_E(PHY,"harq_pid:%d corrupt must be 0-7 DLSCH_id:%d frame:%d subframe:%d rnti:%x [ %1d.%1d.%1d.%1d.%1d.%1d.%1d.%1d\n", harq_pid,DLSCH_id,frame,subframe,dlsch0->rnti,
                dlsch0->harq_ids[frame%2][0],
                dlsch0->harq_ids[frame%2][1],
                dlsch0->harq_ids[frame%2][2],
                dlsch0->harq_ids[frame%2][3],
                dlsch0->harq_ids[frame%2][4],
                dlsch0->harq_ids[frame%2][5],
                dlsch0->harq_ids[frame%2][6],
                dlsch0->harq_ids[frame%2][7]);
      } else {
        if (dlsch_procedures(eNB,
                             proc,
                             harq_pid,
                             dlsch0,
                             &eNB->UE_stats[(uint32_t)DLSCH_id])) {
          // if we generate dlsch, we must generate pdsch
          pdsch_procedures(eNB,
                           proc,
                           harq_pid,
                           dlsch0,
                           dlsch1);
        }
      }
    } else if ((dlsch0)&&(dlsch0->rnti>0)&&
#ifdef PHY_TX_THREAD
               (dlsch0->active[subframe] == 0)
#else
               (dlsch0->active == 0)
#endif
              ) {
      // clear subframe TX flag since UE is not scheduled for PDSCH in this subframe (so that we don't look for PUCCH later)
      dlsch0->subframe_tx[subframe]=0;
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GENERATE_DLSCH,0);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GENERATE_PHICH,1);
  generate_phich_top(eNB,
                     proc,
                     AMP);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GENERATE_PHICH,0);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_TX+(eNB->CC_id),0);

  if (do_meas==1) stop_meas(&eNB->dlsch_ue_specific);

  if (do_meas==1) stop_meas(&eNB->phy_proc_tx);
}

/* This has to be updated with FAPI structures*/

void srs_procedures(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc) {
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  const int       subframe = proc->subframe_rx;
  const int       frame = proc->frame_rx;
  int             i;

  if (is_srs_occasion_common (fp, frame, subframe)) {
    // Do SRS processing
    // check if there is SRS and we have to use shortened format
    // TODO: check for exceptions in transmission of SRS together with ACK/NACK
    for (i = 0; i < NUMBER_OF_SRS_MAX; i++) {
      if (eNB->soundingrs_ul_config_dedicated[i].active == 1) {
        if (lte_srs_channel_estimation (fp, &eNB->common_vars, &eNB->srs_vars[i], &eNB->soundingrs_ul_config_dedicated[i], subframe, 0 /*eNB_id */ )) {
          LOG_E (PHY, "problem processing SRS\n");
        }

        eNB->soundingrs_ul_config_dedicated[i].active = 0;
      }
    }
  }
}

void fill_sr_indication(int UEid, PHY_VARS_eNB *eNB,uint16_t rnti,int frame,int subframe,uint32_t stat)
{
  pthread_mutex_lock(&eNB->UL_INFO_mutex);
  nfapi_sr_indication_t       *sr_ind =         &eNB->UL_INFO.sr_ind;
  nfapi_sr_indication_body_t  *sr_ind_body =    &sr_ind->sr_indication_body;
  assert(sr_ind_body->number_of_srs <= NFAPI_SR_IND_MAX_PDU);
  nfapi_sr_indication_pdu_t *pdu =   &sr_ind_body->sr_pdu_list[sr_ind_body->number_of_srs];
  sr_ind->sfn_sf = frame<<4|subframe;
  sr_ind->header.message_id = NFAPI_RX_SR_INDICATION;
  sr_ind_body->tl.tag = NFAPI_SR_INDICATION_BODY_TAG;
  pdu->instance_length                                = 0; // don't know what to do with this
  //  pdu->rx_ue_information.handle                       = handle;
  pdu->rx_ue_information.tl.tag                       = NFAPI_RX_UE_INFORMATION_TAG;
  pdu->rx_ue_information.rnti                         = rnti;
  int SNRtimes10 = dB_fixed_x10(stat) - 10 * eNB->measurements.n0_pucch_dB;
  LOG_D(PHY,"stat %d n0 %d, SNRtimes10 %d\n", stat, eNB->measurements.n0_subband_power_dB[0][0], SNRtimes10);
  pdu->ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;

  if      (SNRtimes10 < -640) pdu->ul_cqi_information.ul_cqi=0;
  else if (SNRtimes10 >  635) pdu->ul_cqi_information.ul_cqi=255;
  else                        pdu->ul_cqi_information.ul_cqi=(640+SNRtimes10)/5;

  pdu->ul_cqi_information.channel = 0;
  sr_ind_body->number_of_srs++;
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);
}

//-----------------------------------------------------------------------------
/*
 * Main handler of PUCCH received
 */
void
uci_procedures(PHY_VARS_eNB *eNB,
               L1_rxtx_proc_t *proc)
//-----------------------------------------------------------------------------
{
  uint8_t SR_payload = 0;
  uint8_t pucch_b0b1[4][2] = {{0,0},{0,0},{0,0},{0,0}};
  uint8_t harq_ack[4] = {0,0,0,0};
  uint16_t tdd_multiplexing_mask = 0;
  int32_t metric[4] = {0,0,0,0};
  int32_t metric_SR = 0;
  int32_t max_metric = 0;
  const int subframe = proc->subframe_rx;
  const int frame = proc->frame_rx;
  LTE_eNB_UCI *uci = NULL;
  LTE_DL_FRAME_PARMS *fp = &(eNB->frame_parms);

  calc_pucch_1x_interference(eNB, frame, subframe, 0);
  
  for (int i = 0; i < NUMBER_OF_UCI_MAX; i++) {
    uci = &(eNB->uci_vars[i]);

    if ((uci->active == 1) && (uci->frame == frame) && (uci->subframe == subframe)) {
      if (uci->ue_id > MAX_MOBILES_PER_ENB) {
        LOG_W(PHY, "UCI for UE %d and/or but is not active in MAC\n", uci->ue_id);
        continue;
      }
      LOG_D(PHY,"Frame %d, subframe %d: Running uci procedures (type %d) for %d \n",
            frame,
            subframe,
            uci->type,
            i);
      uci->active = 0;

      // Null out PUCCH PRBs for noise measurement
      switch (fp->N_RB_UL) {
        case 6:
          eNB->rb_mask_ul[0] |= (0x1 | (1 << 5)); // position 5
          break;

        case 15:
          eNB->rb_mask_ul[0] |= (0x1 | (1 << 14)); // position 14
          break;

        case 25:
          eNB->rb_mask_ul[0] |= (0x1 | (1 << 24)); // position 24
          break;

        case 50:
          eNB->rb_mask_ul[0] |= 0x1;
          eNB->rb_mask_ul[1] |= (1 << 17); // position 49 (49-32)
          break;

        case 75:
          eNB->rb_mask_ul[0] |= 0x1;
          eNB->rb_mask_ul[2] |= (1 << 10); // position 74 (74-64)
          break;

        case 100:
          eNB->rb_mask_ul[0] |= 0x1;
          eNB->rb_mask_ul[3] |= (1 << 3); // position 99 (99-96)
          break;

        default:
          LOG_E(PHY,"Unknown number for N_RB_UL %d\n", fp->N_RB_UL);
          break;
      }

      SR_payload = 0;

      switch (uci->type) {
        case SR:
        case HARQ_SR: {
          int pucch1_thres = (uci->ue_type == 0) ? eNB->pucch1_DTX_threshold : eNB->pucch1_DTX_threshold_emtc[0];
          metric_SR = rx_pucch(eNB,
                               uci->pucch_fmt,
                               i,//uci->ue_id,
                               uci->n_pucch_1_0_sr[0],
                               0, // n2_pucch
                               uci->srs_active, // shortened format
                               &SR_payload,
                               frame,
                               subframe,
                               pucch1_thres,
                               uci->ue_type
                              );
          LOG_D(PHY,"[eNB %d][SR %x] Frame %d subframe %d Checking SR is %d (uci.type %d SR n1pucch is %d)\n",
                eNB->Mod_id,
                uci->rnti,
                frame,
                subframe,
                SR_payload,
                uci->type,
                uci->n_pucch_1_0_sr[0]);

          if (uci->type == SR) {
            if (SR_payload == 1) {
              fill_sr_indication(i, eNB,uci->rnti,frame,subframe,metric_SR);
              break;
            } else {
              break;
            }
          }
        }

        case HARQ: {
          int pucch1ab_thres = (uci->ue_type == 0) ? eNB->pucch1ab_DTX_threshold : eNB->pucch1ab_DTX_threshold_emtc[0];

          if (fp->frame_type == FDD) {
            LOG_D(PHY,"Frame %d Subframe %d Demodulating PUCCH (UCI %d) for ACK/NAK (uci->pucch_fmt %d,uci->type %d.uci->frame %d, uci->subframe %d): n1_pucch0 %d SR_payload %d\n",
                  frame,subframe,i,
                  uci->pucch_fmt,uci->type,
                  uci->frame,uci->subframe,uci->n_pucch_1[0][0],
                  SR_payload);
            metric[0] = rx_pucch(eNB,
                                 uci->pucch_fmt,
                                 uci->ue_id,
                                 uci->n_pucch_1[0][0],
                                 0, //n2_pucch
                                 uci->srs_active, // shortened format
                                 pucch_b0b1[0],
                                 frame,
                                 subframe,
                                 pucch1ab_thres,
                                 uci->ue_type
                                );

            //dump_ulsch(eNB,frame,subframe,0,0); exit(-1);

            /* cancel SR detection if reception on n1_pucch0 is better than on SR PUCCH resource index, otherwise send it up to MAC */
            if (uci->type==HARQ_SR && metric[0] > metric_SR) SR_payload = 0;
            else if (SR_payload == 1) fill_sr_indication(i, eNB,uci->rnti,frame,subframe,metric_SR);

            if (uci->type==HARQ_SR && metric[0] <= metric_SR) {
              /* when transmitting ACK/NACK on SR PUCCH resource index, SR payload is always 1 */
              SR_payload = 1;
              metric[0]=rx_pucch(eNB,
                                 uci->pucch_fmt,
                                 uci->ue_id,
                                 uci->n_pucch_1_0_sr[0],
                                 0, //n2_pucch
                                 uci->srs_active, // shortened format
                                 pucch_b0b1[0],
                                 frame,
                                 subframe,
                                 pucch1ab_thres,
                                 uci->ue_type
                                );
            }

            LOG_D(PHY,"[eNB %d][PDSCH %x] Frame %d subframe %d pucch1a (FDD) payload %d (metric %d)\n",
                  eNB->Mod_id,
                  uci->rnti,
                  frame,subframe,
                  pucch_b0b1[0][0],metric[0]);
            uci->stat = metric[0];
            fill_uci_harq_indication(i, eNB,uci,frame,subframe,pucch_b0b1[0],0,0xffff);
          } else { // frame_type == TDD
            LOG_D(PHY,"Frame %d Subframe %d Demodulating PUCCH (UCI %d) for ACK/NAK (uci->pucch_fmt %d,uci->type %d.uci->frame %d, uci->subframe %d): n1_pucch0 %d SR_payload %d\n",
                  frame,subframe,i,
                  uci->pucch_fmt,uci->type,
                  uci->frame,uci->subframe,uci->n_pucch_1[0][0],
                  SR_payload);
#if 1
            metric[0] = rx_pucch(eNB,
                                 uci->pucch_fmt,
                                 uci->ue_id,
                                 uci->n_pucch_1[0][0],
                                 0, //n2_pucch
                                 uci->srs_active, // shortened format
                                 pucch_b0b1[0],
                                 frame,
                                 subframe,
                                 pucch1ab_thres,
                                 uci->ue_type
                                );

            if (uci->type==HARQ_SR && metric[0] > metric_SR) SR_payload = 0;
            else if (SR_payload == 1) fill_sr_indication(i, eNB,uci->rnti,frame,subframe,metric_SR);

            if (uci->type==HARQ_SR && metric[0] <= metric_SR) {
              SR_payload = 1;
              metric[0] = rx_pucch(eNB,
                                   pucch_format1b,
                                   uci->ue_id,
                                   uci->n_pucch_1_0_sr[0],
                                   0, //n2_pucch
                                   uci->srs_active, // shortened format
                                   pucch_b0b1[0],
                                   frame,
                                   subframe,
                                   pucch1ab_thres,
                                   uci->ue_type
                                  );
            }

#else

            // if SR was detected, use the n1_pucch from SR
            if (SR_payload==1) {
#ifdef DEBUG_PHY_PROC
              LOG_D (PHY, "[eNB %d][PDSCH %x] Frame %d subframe %d Checking ACK/NAK (%d,%d,%d,%d) format %d with SR\n", eNB->Mod_id,
                     eNB->dlsch[UE_id][0]->rnti, frame, subframe, n1_pucch0, n1_pucch1, n1_pucch2, n1_pucch3, format);
#endif
              metric[0] = rx_pucch (eNB, pucch_format1b, i, uci->n_pucch_1_0_sr[0], 0,    //n2_pucch
                                    uci->srs_active,      // shortened format
                                    pucch_b0b1[0], frame, subframe,
                                    pucch1ab_thres,
                                    uci->ue_type
                                   );
            } else {              //using assigned pucch resources
#ifdef DEBUG_PHY_PROC
              LOG_D (PHY, "[eNB %d][PDSCH %x] Frame %d subframe %d Checking ACK/NAK M=%d (%d,%d,%d,%d) format %d\n", eNB->Mod_id,
                     eNB->dlsch[UE_id][0]->rnti,
                     frame, subframe, uci->num_pucch_resources, uci->n_pucch_1[res][0], uci->n_pucch_1[res][1], uci->n_pucch_1[res][2], uci->n_pucch_1[res][3], uci->pucch_fmt);
#endif

              for (res = 0; res < uci->num_pucch_resources; res++)
                metric[res] = rx_pucch (eNB, uci->pucch_fmt, i, uci->n_pucch_1[res][0], 0,        // n2_pucch
                                        uci->srs_active,  // shortened format
                                        pucch_b0b1[res], frame, subframe,
                                        pucch1ab_thres,
                                        uci->ue_type
                                       );

              for (res=0; res<uci->num_pucch_resources; res++)
                metric[res] = rx_pucch(eNB,
                                       uci->pucch_fmt,
                                       i,
                                       uci->n_pucch_1[res][0],
                                       0, // n2_pucch
                                       uci->srs_active, // shortened format
                                       pucch_b0b1[res],
                                       frame,
                                       subframe,
                                       pucch1ab_thres,
                                       uci->ue_type
                                      );
            }

#ifdef DEBUG_PHY_PROC
            LOG_D(PHY,"RNTI %x type %d SR_payload %d  Frame %d Subframe %d  pucch_b0b1[0][0] %d pucch_b0b1[0][1] %d pucch_b0b1[1][0] %d pucch_b0b1[1][1] %d  \n",
                  uci->rnti,uci->type,SR_payload,frame,subframe,pucch_b0b1[0][0],pucch_b0b1[0][1],pucch_b0b1[1][0],pucch_b0b1[1][1]);
#endif
#endif

            if (SR_payload == 1) { // this implements Table 7.3.1 from 36.213
              if (pucch_b0b1[0][0] == 4) { // there isn't a likely transmission
                harq_ack[0] = 4; // DTX
              } else if (pucch_b0b1[0][0] == 1 && pucch_b0b1[0][1] == 1) { // 1/4/7 ACKs
                harq_ack[0] = 1;
              } else if (pucch_b0b1[0][0] == 1 && pucch_b0b1[0][1] != 1) { // 2/5/8 ACKs
                harq_ack[0] = 2;
              } else if (pucch_b0b1[0][0] != 1 && pucch_b0b1[0][1] == 1) { // 3/6/9 ACKs
                harq_ack[0] = 3;
              } else if (pucch_b0b1[0][0] != 1 && pucch_b0b1[0][1] != 1) { // 0 ACKs, or at least one DL assignment missed
                harq_ack[0] = 0;
              }

              uci->stat = metric[0];
              fill_uci_harq_indication(i, eNB,uci,frame,subframe,harq_ack,2,0xffff); // special_bundling mode
            } else if ((uci->tdd_bundling == 0) && (uci->num_pucch_resources==2)) { // multiplexing + no SR, implement Table 10.1.3-5 (Rel14) for multiplexing with M=2
              if (pucch_b0b1[0][0] == 4 ||
                  pucch_b0b1[1][0] == 4) { // there isn't a likely transmission
                harq_ack[0] = 4; // DTX
                harq_ack[1] = 6; // NACK/DTX
              } else {
                if (metric[1]>metric[0]) {
                  if (pucch_b0b1[1][0] == 1 && pucch_b0b1[1][1] != 1) {
                    harq_ack[0] = 1; // ACK
                    harq_ack[1] = 1; // ACK
                    tdd_multiplexing_mask = 0x3;
                  } else if (pucch_b0b1[1][0] != 1 && pucch_b0b1[1][1] == 1) {
                    harq_ack[0] = 6; // NACK/DTX
                    harq_ack[1] = 1; // ACK
                    tdd_multiplexing_mask = 0x2;
                  } else {
                    harq_ack[0] = 4; // DTX
                    harq_ack[1] = 4; // DTX
                  }
                } else {
                  if (pucch_b0b1[0][0] == 1 && pucch_b0b1[0][1] == 1) {
                    harq_ack[0] = 1; // ACK
                    harq_ack[1] = 6; // NACK/DTX
                    tdd_multiplexing_mask = 0x1;
                  } else if (pucch_b0b1[0][0] != 1 && pucch_b0b1[0][1] != 1) {
                    harq_ack[0] = 2; // NACK
                    harq_ack[1] = 6; // NACK/DTX
                  } else {
                    harq_ack[0] = 4; // DTX
                    harq_ack[1] = 4; // DTX
                  }
                }
              }

              uci->stat = max(metric[0],metric[1]);
              fill_uci_harq_indication(i, eNB,uci,frame,subframe,harq_ack,1,tdd_multiplexing_mask); // multiplexing mode
            } //else if ((uci->tdd_bundling == 0) && (res==2))
            else if ((uci->tdd_bundling == 0) && (uci->num_pucch_resources==3)) { // multiplexing + no SR, implement Table 10.1.3-6 (Rel14) for multiplexing with M=3
              if (harq_ack[0] == 4 ||
                  harq_ack[1] == 4 ||
                  harq_ack[2] == 4) { // there isn't a likely transmission
                harq_ack[0] = 4; // DTX
                harq_ack[1] = 6; // NACK/DTX
                harq_ack[2] = 6; // NACK/DTX
                max_metric = 0;
              } else {
                max_metric = max(metric[0],max(metric[1],metric[2]));

                if (metric[0]==max_metric) {
                  if (pucch_b0b1[0][0] == 1 && pucch_b0b1[0][1] == 1) {
                    harq_ack[0] = 1; // ACK
                    harq_ack[1] = 6; // NACK/DTX
                    harq_ack[2] = 6; // NACK/DTX
                    tdd_multiplexing_mask = 0x1;
                  } else if (pucch_b0b1[0][0] != 1 && pucch_b0b1[0][1] != 1) {
                    harq_ack[0] = 2; // NACK
                    harq_ack[1] = 6; // NACK/DTX
                    harq_ack[2] = 6; // NACK/DTX
                  } else {
                    harq_ack[0] = 4; // DTX
                    harq_ack[1] = 4; // DTX
                    harq_ack[2] = 4; // DTX
                  }
                } // if (metric[0]==max_metric) {
                else if (metric[1]==max_metric) {
                  if (pucch_b0b1[1][0] == 1 && pucch_b0b1[1][1] != 1) {
                    harq_ack[0] = 1; // ACK
                    harq_ack[1] = 1; // ACK
                    harq_ack[2] = 6; // NACK/DTX
                    tdd_multiplexing_mask = 0x3;
                  } else if (pucch_b0b1[1][0] != 1 && pucch_b0b1[1][1] == 1 ) {
                    harq_ack[0] = 6; // NACK/DTX
                    harq_ack[1] = 1; // ACK
                    harq_ack[2] = 6; // NACK/DTX
                    tdd_multiplexing_mask = 0x2;
                  } else {
                    harq_ack[0] = 4; // DTX
                    harq_ack[1] = 4; // DTX
                    harq_ack[2] = 4; // DTX
                  }
                } // if (metric[1]==max_metric) {
                else {
                  if (pucch_b0b1[2][0] == 1 && pucch_b0b1[2][1] == 1) {
                    harq_ack[0] = 1; // ACK
                    harq_ack[1] = 1; // ACK
                    harq_ack[2] = 1; // ACK
                    tdd_multiplexing_mask = 0x7;
                  } else if (pucch_b0b1[2][0] == 1 && pucch_b0b1[2][1] != 1 ) {
                    harq_ack[0] = 1; // ACK
                    harq_ack[1] = 6; // NACK/DTX
                    harq_ack[2] = 1; // ACK
                    tdd_multiplexing_mask = 0x5;
                  } else if (pucch_b0b1[2][0] != 1 && pucch_b0b1[2][1] == 1 ) {
                    harq_ack[0] = 6; // NACK/DTX
                    harq_ack[1] = 1; // ACK
                    harq_ack[2] = 1; // ACK
                    tdd_multiplexing_mask = 0x6;
                  } else if (pucch_b0b1[2][0] != 1 && pucch_b0b1[2][1] != 1 ) {
                    harq_ack[0] = 6; // NACK/DTX
                    harq_ack[1] = 6; // NACK/DTX
                    harq_ack[2] = 1; // ACK
                    tdd_multiplexing_mask = 0x4;
                  }
                }

                uci->stat = max_metric;
                fill_uci_harq_indication(i, eNB,uci,frame,subframe,harq_ack,1,tdd_multiplexing_mask); // multiplexing mode
              }
            } //else if ((uci->tdd_bundling == 0) && (res==3))
            else if ((uci->tdd_bundling == 0) && (uci->num_pucch_resources==4)) { // multiplexing + no SR, implement Table 10.1.3-7 (Rel14) for multiplexing with M=4
              if (pucch_b0b1[0][0] == 4 ||
                  pucch_b0b1[1][0] == 4 ||
                  pucch_b0b1[2][0] == 4 ||
                  pucch_b0b1[3][0] == 4) { // there isn't a likely transmission
                harq_ack[0] = 4; // DTX
                harq_ack[1] = 6; // NACK/DTX
                harq_ack[2] = 6; // NACK/DTX
                harq_ack[3] = 6; // NACK/DTX
                max_metric = 0;
              } else {
                max_metric = max(metric[0],max(metric[1],max(metric[2],metric[3])));

                if (metric[0]==max_metric) {
                  if (pucch_b0b1[0][0] == 1 && pucch_b0b1[0][1] != 1) {
                    harq_ack[0] = 2; // NACK
                    harq_ack[1] = 4; // DTX
                    harq_ack[2] = 4; // DTX
                    harq_ack[3] = 4; // DTX
                  } else if (pucch_b0b1[0][0] != 1 && pucch_b0b1[0][1] == 1) {
                    harq_ack[0] = 1; // ACK
                    harq_ack[1] = 6; // NACK/DTX
                    harq_ack[2] = 6; // NACK/DTX
                    harq_ack[3] = 1; // ACK
                    tdd_multiplexing_mask = 0x9;
                  } else if (pucch_b0b1[0][0] == 1 && pucch_b0b1[0][1] == 1) {
                    harq_ack[0] = 1; // ACK
                    harq_ack[1] = 6; // NACK/DTX
                    harq_ack[2] = 6; // NACK/DTX
                    harq_ack[3] = 6; // NACK/DTX
                    tdd_multiplexing_mask = 0x1;
                  } else if (pucch_b0b1[0][0] != 1 && pucch_b0b1[0][1] != 1) {
                    harq_ack[0] = 2; // NACK
                    harq_ack[1] = 6; // NACK/DTX
                    harq_ack[2] = 6; // NACK/DTX
                    harq_ack[3] = 6; // NACK/DTX
                  }
                } else if (metric[1]==max_metric) {
                  if (pucch_b0b1[1][0] == 1 && pucch_b0b1[1][1] == 1) {
                    harq_ack[0] = 1; // ACK
                    harq_ack[1] = 1; // ACK
                    harq_ack[2] = 1; // ACK
                    harq_ack[3] = 1; // ACK
                    tdd_multiplexing_mask = 0xF;
                  } else if (pucch_b0b1[1][0] == 1 && pucch_b0b1[1][1] != 1 ) {
                    harq_ack[0] = 1; // ACK
                    harq_ack[1] = 1; // ACK
                    harq_ack[2] = 6; // NACK/DTX
                    harq_ack[3] = 6; // NACK/DTX
                    tdd_multiplexing_mask = 0x3;
                  } else if (pucch_b0b1[1][0] != 1 && pucch_b0b1[1][1] != 1 ) {
                    harq_ack[0] = 6; // NACK/DTX
                    harq_ack[1] = 1; // ACK
                    harq_ack[2] = 1; // ACK
                    harq_ack[3] = 1; // ACK
                    tdd_multiplexing_mask = 0xE;
                  } else if (pucch_b0b1[1][0] != 1 && pucch_b0b1[1][1] == 1 ) {
                    harq_ack[0] = 6; // NACK/DTX
                    harq_ack[1] = 1; // ACK
                    harq_ack[2] = 6; // NACK/DTX
                    harq_ack[3] = 6; // NACK/DTX
                    tdd_multiplexing_mask = 0x2;
                  }
                } else if (metric[2]==max_metric) {
                  if (pucch_b0b1[2][0] == 1 && pucch_b0b1[2][1] == 1) {
                    harq_ack[0] = 1; // ACK
                    harq_ack[1] = 1; // ACK
                    harq_ack[2] = 1; // ACK
                    harq_ack[3] = 6; // NACK/DTX
                    tdd_multiplexing_mask = 0x7;
                  } else if (pucch_b0b1[2][0] == 1 && pucch_b0b1[2][1] != 1 ) {
                    harq_ack[0] = 1; // ACK
                    harq_ack[1] = 6; // NACK/DTX
                    harq_ack[2] = 1; // ACK
                    harq_ack[3] = 6; // NACK/DTX
                    tdd_multiplexing_mask = 0x5;
                  } else if (pucch_b0b1[2][0] != 1 && pucch_b0b1[2][1] == 1 ) {
                    harq_ack[0] = 4; // NACK/DTX
                    harq_ack[1] = 1; // ACK
                    harq_ack[2] = 1; // ACK
                    harq_ack[3] = 4; // NACK/DTX
                    tdd_multiplexing_mask = 0x6;
                  } else if (pucch_b0b1[2][0] != 1 && pucch_b0b1[2][1] != 1 ) {
                    harq_ack[0] = 4; // NACK/DTX
                    harq_ack[1] = 4; // NACK/DTX
                    harq_ack[2] = 1; // ACK
                    harq_ack[3] = 4; // NACK/DTX
                    tdd_multiplexing_mask = 0x4;
                  }
                } else { // max_metric[3]=max_metric
                  if (pucch_b0b1[2][0] == 1 && pucch_b0b1[2][1] == 1) {
                    harq_ack[0] = 1; // ACK
                    harq_ack[1] = 6; // NACK/DTX
                    harq_ack[2] = 1; // ACK
                    harq_ack[3] = 1; // ACK
                    tdd_multiplexing_mask = 0xD;
                  } else if (pucch_b0b1[2][0] == 1 && pucch_b0b1[2][1] != 1 ) {
                    harq_ack[0] = 6; // NACK/DTX
                    harq_ack[1] = 1; // ACK
                    harq_ack[2] = 6; // NACK/DTX
                    harq_ack[3] = 1; // ACK
                    tdd_multiplexing_mask = 0xA;
                  } else if (pucch_b0b1[2][0] != 1 && pucch_b0b1[2][1] == 1 ) {
                    harq_ack[0] = 6; // NACK/DTX
                    harq_ack[1] = 6; // NACK/DTX
                    harq_ack[2] = 1; // ACK
                    harq_ack[3] = 1; // ACK
                    tdd_multiplexing_mask = 0xC;
                  } else if (pucch_b0b1[2][0] != 1 && pucch_b0b1[2][1] != 1 ) {
                    harq_ack[0] = 6; // NACK/DTX
                    harq_ack[1] = 6; // NACK/DTX
                    harq_ack[2] = 6; // NACK/DTX
                    harq_ack[3] = 1; // ACK
                    tdd_multiplexing_mask = 0x8;
                  }
                }
              }

              uci->stat = max_metric;
              fill_uci_harq_indication(i, eNB,uci,frame,subframe,harq_ack,1,tdd_multiplexing_mask); // multiplexing mode
            } // else if ((uci->tdd_bundling == 0) && (res==4))
            else { // bundling
              harq_ack[0] = pucch_b0b1[0][0];
              harq_ack[1] = pucch_b0b1[0][1];
              uci->stat = metric[0];
              LOG_D(PHY,"bundling: (%d,%d), metric %d\n",harq_ack[0],harq_ack[1],uci->stat);
              fill_uci_harq_indication(i, eNB,uci,frame,subframe,harq_ack,0,0xffff); // special_bundling mode
            }

#ifdef DEBUG_PHY_PROC
            LOG_D (PHY, "[eNB %d][PDSCH %x] Frame %d subframe %d ACK/NAK metric 0 %d, metric 1 %d, (%d,%d)\n", eNB->Mod_id,
                   eNB->dlsch[DLSCH_id][0]->rnti, frame, subframe, metric0, metric1, pucch_b0b1[0], pucch_b0b1[1]);
#endif
          }

          break;

          default:
            AssertFatal (1 == 0, "Unsupported UCI type %d\n", uci->type);
            break;
          }

          if (SR_payload == 1) {
            LOG_D (PHY, "[eNB %d][SR %x] Frame %d subframe %d Got SR for PUSCH, transmitting to MAC\n", eNB->Mod_id, uci->rnti, frame, subframe);

            if (eNB->first_sr[uci->ue_id] == 1) {    // this is the first request for uplink after Connection Setup, so clear HARQ process 0 use for Msg4
              eNB->first_sr[uci->ue_id] = 0;
              eNB->dlsch[uci->ue_id][0]->harq_processes[0]->round = 0;
              eNB->dlsch[uci->ue_id][0]->harq_processes[0]->status = SCH_IDLE;
              LOG_D (PHY, "[eNB %d][SR %x] Frame %d subframe %d First SR\n", eNB->Mod_id, eNB->ulsch[uci->ue_id]->rnti, frame, subframe);
            }
          }
      }
    } // end if ((uci->active == 1) && (uci->frame == frame) && (uci->subframe == subframe)) {
  } // end loop for (int i = 0; i < NUMBER_OF_UCI_MAX; i++) {
}

void postDecode(L1_rxtx_proc_t *proc, notifiedFIFO_elt_t *req)
{
  turboDecode_t * rdata=(turboDecode_t *) NotifiedFifoData(req);

  LTE_eNB_ULSCH_t *ulsch = rdata->eNB->ulsch[rdata->UEid];
  LTE_UL_eNB_HARQ_t *ulsch_harq = rdata->ulsch_harq;
  PHY_VARS_eNB *eNB=rdata->eNB;
    
  bool decodeSucess=rdata->decodeIterations <= rdata->maxIterations;
  ulsch_harq->processedSegments++;
  LOG_D(PHY, "processing result of segment: %d, ue %d, processed %d/%d\n",
	rdata->segment_r, rdata->UEid, ulsch_harq->processedSegments, rdata->nbSegments);
  proc->nbDecode--;
  LOG_D(PHY,"remain to decoded in subframe: %d\n", proc->nbDecode);
  if (decodeSucess)  {
    int Fbytes=(rdata->segment_r==0) ? rdata->Fbits>>3 : 0;
    int sz=(rdata->Kr>>3) - Fbytes - ((ulsch_harq->C>1)?3:0);
    memcpy(ulsch_harq->decodedBytes + rdata->offset, rdata->decoded_bytes + Fbytes, sz);
  }

  // if this UE segments are all done
  if ( rdata->nbSegments == ulsch_harq->processedSegments) {
      //compute the expected ULSCH RX power (for the stats)
      int i=rdata->UEid;
      ulsch_harq->delta_TF = get_hundred_times_delta_IF_eNB(eNB,i,rdata->harq_pid, 0); // 0 means bw_factor is not considered
      if (RC.mac != NULL) { /* ulsim does not use RC.mac context. */
	if (ulsch_harq->cqi_crc_status == 1) {
	  fill_ulsch_cqi_indication(eNB,rdata->frame,rdata->subframe,ulsch_harq,ulsch->rnti);
	  RC.mac[eNB->Mod_id]->UE_info.UE_sched_ctrl[i].cqi_req_flag &= (~(1 << rdata->subframe));
	} else {
	  if(RC.mac[eNB->Mod_id]->UE_info.UE_sched_ctrl[i].cqi_req_flag & (1 << rdata->subframe) ) {
	    RC.mac[eNB->Mod_id]->UE_info.UE_sched_ctrl[i].cqi_req_flag &= (~(1 << rdata->subframe));
	    RC.mac[eNB->Mod_id]->UE_info.UE_sched_ctrl[i].cqi_req_timer=30;
	    LOG_D(PHY,"Frame %d,Subframe %d, We're supposed to get a cqi here. Set cqi_req_timer to 30.\n",rdata->frame,rdata->subframe);
	  }
	}
      }

      if (check_abort(&ulsch_harq->abort_decode)) {
        T(T_ENB_PHY_ULSCH_UE_NACK, T_INT(eNB->Mod_id), T_INT(rdata->frame), T_INT(rdata->subframe), T_INT(ulsch->rnti),
          T_INT(rdata->harq_pid));
	fill_crc_indication(eNB,i,rdata->frame,rdata->subframe,1); // indicate NAK to MAC
	fill_rx_indication(eNB,i,rdata->frame,rdata->subframe);  // indicate SDU to MAC
	LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d UE %d Error receiving ULSCH, round %d/%d (ACK %d,%d)\n",
	      eNB->Mod_id,rdata->harq_pid,
	      rdata->frame,rdata->subframe, i,
	      ulsch_harq->round,
	      ulsch->Mlimit,
	      ulsch_harq->o_ACK[0],
	      ulsch_harq->o_ACK[1]);
	  
	if (ulsch_harq->round >= 3)  {
	  ulsch_harq->status  = SCH_IDLE;
	  ulsch_harq->handled = 0;
	  ulsch->harq_mask   &= ~(1 << rdata->harq_pid);
	  ulsch_harq->round   = 0;
	}
	/* Mark the HARQ process to release it later if max transmission reached
	 * (see below).
	 * MAC does not send the max transmission count, we have to deal with it
	 * locally in PHY.
	 */
	ulsch_harq->handled = 1;
      } // ulsch in error
      else if (ulsch_harq->repetition_number == ulsch_harq->total_number_of_repetitions) {
        fill_crc_indication(eNB, i, rdata->frame, rdata->subframe, 0); // indicate ACK to MAC
        fill_rx_indication(eNB, i, rdata->frame, rdata->subframe); // indicate SDU to MAC
        ulsch_harq->status = SCH_IDLE;
        ulsch->harq_mask &= ~(1 << rdata->harq_pid);
        for (int j = 0; j < NUMBER_OF_ULSCH_MAX; j++)
          if (eNB->ulsch_stats[j].rnti == ulsch->rnti) {
            eNB->ulsch_stats[j].total_bytes_rx += ulsch_harq->TBS;
            for (int aa = 0; aa < eNB->frame_parms.nb_antennas_rx; aa++) {
              eNB->ulsch_stats[j].ulsch_power[aa] = dB_fixed_x10(eNB->pusch_vars[rdata->UEid]->ulsch_power[aa]);
              eNB->ulsch_stats[j].ulsch_noise_power[aa] = dB_fixed_x10(eNB->pusch_vars[rdata->UEid]->ulsch_noise_power[aa]);
            }
            break;
          }
        T (T_ENB_PHY_ULSCH_UE_ACK, T_INT(eNB->Mod_id), T_INT(rdata->frame), T_INT(rdata->subframe), T_INT(ulsch->rnti),
           T_INT(rdata->harq_pid));
      } // ulsch not in error

      if (ulsch_harq->O_ACK>0)
	fill_ulsch_harq_indication(eNB,ulsch_harq,ulsch->rnti,rdata->frame,rdata->subframe,ulsch->bundling);
  }
}

void pusch_procedures(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc) {
  uint32_t i;
  uint32_t harq_pid;
  uint8_t nPRS;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  LTE_eNB_ULSCH_t *ulsch;
  LTE_UL_eNB_HARQ_t *ulsch_harq;
  const int subframe = proc->subframe_rx;
  const int frame    = proc->frame_rx;
  uint32_t harq_pid0 = subframe2harq_pid(&eNB->frame_parms,frame,subframe);

  for (i = 0; i < NUMBER_OF_ULSCH_MAX; i++) {
    ulsch = eNB->ulsch[i];
    if (!ulsch) continue; 

    if (ulsch->ue_type > 0) harq_pid = 0;
    else harq_pid=harq_pid0;

    ulsch_harq = ulsch->harq_processes[harq_pid];

    if (ulsch->rnti>0) LOG_D(PHY,"eNB->ulsch[%d]->harq_processes[harq_pid:%d] SFN/SF:%04d%d: PUSCH procedures, UE %d/%x ulsch_harq[status:%d SFN/SF:%04d%d handled:%d]\n",
                               i, harq_pid, frame,subframe,i,ulsch->rnti,
                              ulsch_harq->status, ulsch_harq->frame, ulsch_harq->subframe, ulsch_harq->handled);

    if ((ulsch->rnti>0) &&
        (ulsch_harq->status == ACTIVE) &&
        ((ulsch_harq->frame == frame)	    || (ulsch_harq->repetition_number >1) ) &&
        ((ulsch_harq->subframe == subframe) || (ulsch_harq->repetition_number >1) ) &&
        (ulsch_harq->handled == 0)) {
      // UE has ULSCH scheduling
      for (int rb=0;
           rb<=ulsch_harq->nb_rb;
           rb++) {
        int rb2 = rb+ulsch_harq->first_rb;
        eNB->rb_mask_ul[rb2>>5] |= (1L<<(rb2&31));
      }

      LOG_D(PHY,"[eNB %d] frame %d, subframe %d: Scheduling ULSCH Reception for UE %d \n", eNB->Mod_id, frame, subframe, i);
      nPRS = fp->pusch_config_common.ul_ReferenceSignalsPUSCH.nPRS[subframe<<1];
      ulsch->cyclicShift = (ulsch_harq->n_DMRS2 +
                            fp->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift +
                            nPRS)%12;
      AssertFatal(ulsch_harq->TBS>0,"illegal TBS %d\n",ulsch_harq->TBS);
      LOG_D(PHY,
            "[eNB %d][PUSCH %d] Frame %d Subframe %d Demodulating PUSCH: dci_alloc %d, rar_alloc %d, round %d, first_rb %d, nb_rb %d, Qm %d, TBS %d, rv %d, cyclic_shift %d (n_DMRS2 %d, cyclicShift_common %d, ), O_ACK %d, beta_cqi %d \n",
            eNB->Mod_id,harq_pid,frame,subframe,
            ulsch_harq->dci_alloc,
            ulsch_harq->rar_alloc,
            ulsch_harq->round,
            ulsch_harq->first_rb,
            ulsch_harq->nb_rb,
            ulsch_harq->Qm,
            ulsch_harq->TBS,
            ulsch_harq->rvidx,
            ulsch->cyclicShift,
            ulsch_harq->n_DMRS2,
            fp->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift,
            ulsch_harq->O_ACK,
            ulsch->beta_offset_cqi_times8);
      start_meas(&eNB->ulsch_demodulation_stats);
      rx_ulsch(eNB,proc, i);
      stop_meas(&eNB->ulsch_demodulation_stats);
      start_meas(&eNB->ulsch_decoding_stats);
      ulsch_decoding(eNB,
                     proc,
                     i,
                     0, // control_only_flag
                     ulsch_harq->V_UL_DAI,
                     ulsch_harq->nb_rb > 20 ? 1 : 0);
    }
    else if ((ulsch) &&
             (ulsch->rnti>0) &&
             (ulsch_harq->status == ACTIVE) &&
             (ulsch_harq->frame == frame) &&
             (ulsch_harq->subframe == subframe) &&
             (ulsch_harq->handled == 1)) {
      // this harq process is stale, kill it, this 1024 frames later (10s), consider reducing that
      ulsch_harq->status = SCH_IDLE;
      ulsch_harq->handled = 0;
      ulsch->harq_mask &= ~(1 << harq_pid);
      LOG_W (PHY, "Removing stale ULSCH config for UE %x harq_pid %d (harq_mask is now 0x%2.2x)\n", ulsch->rnti, harq_pid, ulsch->harq_mask);
    }
  }   //   for (i=0; i<NUMBER_OF_ULSCH_MAX; i++)

  const bool decode = proc->nbDecode;
  while (proc->nbDecode > 0) {
    notifiedFIFO_elt_t *req=pullTpool(proc->respDecode, proc->threadPool);
    if (req == NULL)
      break; // Tpool has been stopped
    postDecode(proc, req);
    const time_stats_t ts = exec_time_stats_NotifiedFIFO(req);
    merge_meas(&eNB->ulsch_turbo_decoding_stats, &ts);
    delNotifiedFIFO_elt(req);
  }
  if (decode)
    stop_meas(&eNB->ulsch_decoding_stats);
}

extern int oai_exit;

void fill_rx_indication(PHY_VARS_eNB *eNB,
                        int ULSCH_id,
                        int frame,
                        int subframe) {
  nfapi_rx_indication_pdu_t *pdu;
  int             timing_advance_update;
  int             sync_pos;
  uint32_t        harq_pid;

  if (eNB->ulsch[ULSCH_id]->ue_type > 0) harq_pid = 0;
  else {
    harq_pid = subframe2harq_pid (&eNB->frame_parms,
                                  frame, subframe);
  }

  pthread_mutex_lock(&eNB->UL_INFO_mutex);
  eNB->UL_INFO.rx_ind.sfn_sf                    = frame<<4| subframe;
  eNB->UL_INFO.rx_ind.rx_indication_body.tl.tag = NFAPI_RX_INDICATION_BODY_TAG;
  assert(eNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus <= NFAPI_RX_IND_MAX_PDU);
  pdu                                    = &eNB->UL_INFO.rx_ind.rx_indication_body.rx_pdu_list[eNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus];
  //  pdu->rx_ue_information.handle          = eNB->ulsch[ULSCH_id]->handle;
  pdu->rx_ue_information.tl.tag          = NFAPI_RX_UE_INFORMATION_TAG;
  pdu->rx_ue_information.rnti            = eNB->ulsch[ULSCH_id]->rnti;
  pdu->rx_indication_rel8.tl.tag         = NFAPI_RX_INDICATION_REL8_TAG;
  pdu->rx_indication_rel8.length         = eNB->ulsch[ULSCH_id]->harq_processes[harq_pid]->TBS>>3;
  pdu->rx_indication_rel8.offset         = 1;   // DJP - I dont understand - but broken unless 1 ????  0;  // filled in at the end of the UL_INFO formation
  assert(pdu->rx_indication_rel8.length <= NFAPI_RX_IND_DATA_MAX);
  memcpy(pdu->rx_ind_data,
         eNB->ulsch[ULSCH_id]->harq_processes[harq_pid]->decodedBytes,
         pdu->rx_indication_rel8.length);

  // estimate timing advance for MAC
  sync_pos                               = lte_est_timing_advance_pusch(&eNB->frame_parms, eNB->pusch_vars[ULSCH_id]->drs_ch_estimates_time);
  timing_advance_update                  = sync_pos; 

  for (int i=0;i<NUMBER_OF_SCH_STATS_MAX;i++) 
      if (eNB->ulsch_stats[i].rnti == eNB->ulsch[ULSCH_id]->rnti) 
         eNB->ulsch_stats[i].timing_offset = sync_pos;
  //  if (timing_advance_update > 10) { dump_ulsch(eNB,frame,subframe,ULSCH_id); exit(-1);}
  //  if (timing_advance_update < -10) { dump_ulsch(eNB,frame,subframe,ULSCH_id); exit(-1);}
  switch (eNB->frame_parms.N_RB_DL) {
    case 6:                      /* nothing to do */
      break;

    case 15:
      timing_advance_update /= 2;
      break;

    case 25:
      timing_advance_update /= 4;
      break;

    case 50:
      timing_advance_update /= 8;
      break;

    case 75:
      timing_advance_update /= 12;
      break;

    case 100:
      timing_advance_update /= 16;
      break;

    default:
      abort ();
  }

  // put timing advance command in 0..63 range
  timing_advance_update += 31;

  if (timing_advance_update < 0)
    timing_advance_update = 0;

  if (timing_advance_update > 63)
    timing_advance_update = 63;

  pdu->rx_indication_rel8.timing_advance = timing_advance_update;
  // estimate UL_CQI for MAC 
  int total_power=0, avg_noise_power=0;  
  for (int i=0;i<eNB->frame_parms.nb_antennas_rx;i++) {
     total_power+=(eNB->pusch_vars[ULSCH_id]->ulsch_power[i]);
     avg_noise_power+=(eNB->pusch_vars[ULSCH_id]->ulsch_noise_power[i])/eNB->frame_parms.nb_antennas_rx;
  }
  int SNRtimes10 = dB_fixed_x10(total_power) - 
                   dB_fixed_x10(avg_noise_power);

  if (SNRtimes10 < -640)
    pdu->rx_indication_rel8.ul_cqi = 0;
  else if (SNRtimes10 > 635)
    pdu->rx_indication_rel8.ul_cqi = 255;
  else
    pdu->rx_indication_rel8.ul_cqi = (640 + SNRtimes10) / 5;

  LOG_D(PHY,"[PUSCH %d] Frame %d Subframe %d Filling RX_indication with SNR %d (%d,%d,%d), timing_advance %d (update %d,sync_pos %d)\n",
        harq_pid,frame,subframe,SNRtimes10,pdu->rx_indication_rel8.ul_cqi,dB_fixed_x10(total_power),dB_fixed_x10(avg_noise_power),pdu->rx_indication_rel8.timing_advance,timing_advance_update,sync_pos);
  for (int i=0;i<eNB->frame_parms.nb_antennas_rx;i++)
	LOG_D(PHY,"antenna %d: ulsch_power %d, noise_power %d\n",i,dB_fixed_x10(eNB->pusch_vars[ULSCH_id]->ulsch_power[i]),dB_fixed_x10(eNB->pusch_vars[ULSCH_id]->ulsch_noise_power[i]));

  eNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus++;
  eNB->UL_INFO.rx_ind.sfn_sf = frame<<4 | subframe;
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);
}

//-----------------------------------------------------------------------------
/*
 * Release the harq process if its round is >= 'after_rounds'
 */
static void do_release_harq(PHY_VARS_eNB *eNB,
                            int DLSCH_id,
                            int tb,
                            uint16_t frame,
                            uint8_t subframe,
                            uint16_t mask,
                            int after_rounds)
//-----------------------------------------------------------------------------
{
  LTE_eNB_DLSCH_t *dlsch0 = NULL;
  LTE_eNB_DLSCH_t *dlsch1 = NULL;
  LTE_DL_eNB_HARQ_t *dlsch0_harq = NULL;
  LTE_DL_eNB_HARQ_t *dlsch1_harq = NULL;
  int harq_pid;
  int subframe_tx;
  int frame_tx;
  AssertFatal(DLSCH_id != -1, "No existing dlsch context\n");
  AssertFatal(DLSCH_id < NUMBER_OF_DLSCH_MAX, "Returned DLSCH_id %d >= %d (NUMBER_OF_DLSCH_MAX)\n", DLSCH_id, NUMBER_OF_DLSCH_MAX);
  dlsch0 = eNB->dlsch[DLSCH_id][0];
  dlsch1 = eNB->dlsch[DLSCH_id][1];

  if (eNB->frame_parms.frame_type == FDD) {
    subframe_tx = (subframe + 6) % 10;
    frame_tx = ul_ACK_subframe2_dl_frame(&eNB->frame_parms,
                                         frame,
                                         subframe,
                                         subframe_tx);
    harq_pid = dlsch0->harq_ids[frame_tx%2][subframe_tx];

    if((harq_pid < 0) || (harq_pid >= dlsch0->Mdlharq)) {
      LOG_E(PHY,"illegal harq_pid %d %s:%d\n", harq_pid, __FILE__, __LINE__);
      return;
    }

    dlsch0_harq = dlsch0->harq_processes[harq_pid];
    dlsch1_harq = dlsch1->harq_processes[harq_pid];
    AssertFatal(dlsch0_harq != NULL, "dlsch0_harq is null\n");
#if T_TRACER

    if (after_rounds != -1) {
      T(T_ENB_PHY_DLSCH_UE_NACK,
        T_INT(0),
        T_INT(frame),
        T_INT(subframe),
        T_INT(dlsch0->rnti),
        T_INT(harq_pid));
    } else {
      T(T_ENB_PHY_DLSCH_UE_ACK,
        T_INT(0),
        T_INT(frame),
        T_INT(subframe),
        T_INT(dlsch0->rnti),
        T_INT(harq_pid));
    }

#endif

    if (dlsch0_harq->round >= after_rounds) {
      dlsch0_harq->status = SCH_IDLE;
      dlsch0->harq_mask &= ~(1 << harq_pid);
    }
  } else {
    /* Release all processes in the bundle that was acked, based on mask */
    /* This is at most 4 for multiplexing and 9 for bundling/special bundling */
    int M = ul_ACK_subframe2_M(&eNB->frame_parms, subframe);

    for (int m=0; m < M; m++) {
      subframe_tx = ul_ACK_subframe2_dl_subframe(&eNB->frame_parms,
                    subframe,
                    m);
      frame_tx = ul_ACK_subframe2_dl_frame(&eNB->frame_parms,
                                           frame,
                                           subframe,
                                           subframe_tx);

      if (((1 << m) & mask) > 0) {
        harq_pid = dlsch0->harq_ids[frame_tx%2][subframe_tx];

        if((harq_pid < 0) || (harq_pid >= dlsch0->Mdlharq)) {
          LOG_E(PHY,"illegal harq_pid %d %s:%d\n", harq_pid, __FILE__, __LINE__);
          return;
        }

        dlsch0_harq = dlsch0->harq_processes[harq_pid];
        dlsch1_harq = dlsch1->harq_processes[harq_pid];
        AssertFatal(dlsch0_harq != NULL, "Dlsch0_harq is null\n");
#if T_TRACER

        if (after_rounds != -1) {
          T(T_ENB_PHY_DLSCH_UE_NACK,
            T_INT(0),
            T_INT(frame),
            T_INT(subframe),
            T_INT(dlsch0->rnti),
            T_INT(harq_pid));
        } else {
          T(T_ENB_PHY_DLSCH_UE_ACK,
            T_INT(0),
            T_INT(frame),
            T_INT(subframe),
            T_INT(dlsch0->rnti),
            T_INT(harq_pid));
        }

#endif

        if (dlsch0_harq->round >= after_rounds) {
          dlsch0_harq->status = SCH_IDLE;

          if ((dlsch1_harq == NULL) || ((dlsch1_harq != NULL) && (dlsch1_harq->status == SCH_IDLE))) {
            dlsch0->harq_mask &= ~(1 << harq_pid);
          }
        }
      } // end if (((1 << m) & mask) > 0)
    } // end for (int m=0; m < M; m++)
  } // end if TDD
}

static void release_harq(PHY_VARS_eNB *eNB,int DLSCH_id,int tb,uint16_t frame,uint8_t subframe,uint16_t mask, int is_ack) {
  /*
   * Maximum number of DL transmissions = 4.
   * TODO: get the value from configuration.
   * If is_ack is true then we release immediately. The value -1 can be used for that.
   */
  do_release_harq(eNB, DLSCH_id, tb, frame, subframe, mask, is_ack ? -1 : 4);
}

int getM(PHY_VARS_eNB *eNB,int frame,int subframe) {
  int M,Mtx=0;
  LTE_eNB_DLSCH_t *dlsch0=NULL,*dlsch1=NULL;
  LTE_DL_eNB_HARQ_t *dlsch0_harq=NULL,*dlsch1_harq=NULL;
  int harq_pid;
  int subframe_tx,frame_tx;
  int m;
  M=ul_ACK_subframe2_M(&eNB->frame_parms,
                       subframe);

  for (m=0; m<M; m++) {
    subframe_tx = ul_ACK_subframe2_dl_subframe(&eNB->frame_parms,
                  subframe,
                  m);
    frame_tx =  ul_ACK_subframe2_dl_frame(&eNB->frame_parms,frame,
                                          subframe,subframe_tx);

    if (dlsch0 == NULL || dlsch1 == NULL) {
      LOG_E(PHY, "dlsch0 and/or dlsch1 NULL, getM frame %i, subframe %i\n",frame,subframe);
      return Mtx;
    }

    harq_pid = dlsch0->harq_ids[frame_tx%2][subframe_tx];

    if (harq_pid>=0 && harq_pid<dlsch0->Mdlharq) {
      dlsch0_harq     = dlsch0->harq_processes[harq_pid];
      dlsch1_harq     = dlsch1->harq_processes[harq_pid];
      AssertFatal(dlsch0_harq!=NULL,"dlsch0_harq is null\n");

      if (dlsch0_harq->status == ACTIVE||
          (dlsch1_harq!=NULL && dlsch1_harq->status == ACTIVE)) Mtx ++;
    }
  }

  return (Mtx);
}


void fill_ulsch_cqi_indication (PHY_VARS_eNB *eNB, uint16_t frame, uint8_t subframe, LTE_UL_eNB_HARQ_t *ulsch_harq, uint16_t rnti) {
  pthread_mutex_lock (&eNB->UL_INFO_mutex);
  assert(eNB->UL_INFO.cqi_ind.cqi_indication_body.number_of_cqis <= NFAPI_CQI_IND_MAX_PDU);
  nfapi_cqi_indication_pdu_t *pdu         = &eNB->UL_INFO.cqi_ind.cqi_indication_body.cqi_pdu_list[eNB->UL_INFO.cqi_ind.cqi_indication_body.number_of_cqis];
  nfapi_cqi_indication_raw_pdu_t *raw_pdu = &eNB->UL_INFO.cqi_ind.cqi_indication_body.cqi_raw_pdu_list[eNB->UL_INFO.cqi_ind.cqi_indication_body.number_of_cqis];
  pdu->instance_length = 0;
  pdu->rx_ue_information.tl.tag          = NFAPI_RX_UE_INFORMATION_TAG;
  pdu->rx_ue_information.rnti = rnti;

  if (ulsch_harq->cqi_crc_status != 1)
    pdu->cqi_indication_rel9.data_offset = 0;
  else
    pdu->cqi_indication_rel9.data_offset = 1;   // fill in after all cqi_indications have been generated when non-zero

  // by default set O to rank 1 value
  pdu->cqi_indication_rel9.tl.tag = NFAPI_CQI_INDICATION_REL9_TAG;
  pdu->cqi_indication_rel9.length = (ulsch_harq->Or1>>3) + ((ulsch_harq->Or1&7) > 0 ? 1 : 0);
  pdu->cqi_indication_rel9.ri[0]  = 0;

  // if we have RI bits, set them and if rank2 overwrite O
  if (ulsch_harq->O_RI > 0) {
    pdu->cqi_indication_rel9.ri[0] = ulsch_harq->o_RI[0];

    if (ulsch_harq->o_RI[0] == 2)
      pdu->cqi_indication_rel9.length = (ulsch_harq->Or2 >> 3) + ((ulsch_harq->Or2 & 7) > 0 ? 1 : 0);

    pdu->cqi_indication_rel9.timing_advance = 0;
  }

  pdu->cqi_indication_rel9.number_of_cc_reported = 1;
  pdu->ul_cqi_information.channel = 1;  // PUSCH
  pdu->ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;
  memcpy ((void *) raw_pdu->pdu, ulsch_harq->o, pdu->cqi_indication_rel9.length);
  eNB->UL_INFO.cqi_ind.cqi_indication_body.tl.tag = NFAPI_CQI_INDICATION_BODY_TAG;
  eNB->UL_INFO.cqi_ind.cqi_indication_body.number_of_cqis++;
  LOG_D(PHY,"eNB->UL_INFO.cqi_ind.cqi_indication_body.number_of_cqis:%d\n", eNB->UL_INFO.cqi_ind.cqi_indication_body.number_of_cqis);
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);
}

void fill_ulsch_harq_indication (PHY_VARS_eNB *eNB, LTE_UL_eNB_HARQ_t *ulsch_harq, uint16_t rnti, int frame, int subframe, int bundling) {
  int DLSCH_id = find_dlsch(rnti,eNB,SEARCH_EXIST);

  if( (DLSCH_id<0) || (DLSCH_id>=NUMBER_OF_DLSCH_MAX) ) {
    LOG_E(PHY,"illegal DLSCH_id found!!! rnti %04x DLSCH_id %d\n",rnti,DLSCH_id);
    return;
  }

  //AssertFatal(DLSCH_id>=0,"DLSCH_id doesn't exist\n");
  pthread_mutex_lock(&eNB->UL_INFO_mutex);
  assert(eNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs <= NFAPI_HARQ_IND_MAX_PDU);
  nfapi_harq_indication_pdu_t *pdu =   &eNB->UL_INFO.harq_ind.harq_indication_body.harq_pdu_list[eNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs];
  int M;
  int i;
  eNB->UL_INFO.harq_ind.header.message_id = NFAPI_HARQ_INDICATION;
  eNB->UL_INFO.harq_ind.sfn_sf = frame<<4|subframe;
  eNB->UL_INFO.harq_ind.harq_indication_body.tl.tag = NFAPI_HARQ_INDICATION_BODY_TAG;
  pdu->instance_length                                = 0; // don't know what to do with this
  //  pdu->rx_ue_information.handle                       = handle;
  pdu->rx_ue_information.tl.tag                       = NFAPI_RX_UE_INFORMATION_TAG;
  pdu->rx_ue_information.rnti                         = rnti;

  if (eNB->frame_parms.frame_type == FDD) {
    pdu->harq_indication_fdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_FDD_REL13_TAG;
    pdu->harq_indication_fdd_rel13.mode = 0;
    pdu->harq_indication_fdd_rel13.number_of_ack_nack = ulsch_harq->O_ACK;

    for (i = 0; i < ulsch_harq->O_ACK; i++) {
      AssertFatal (ulsch_harq->o_ACK[i] == 0 || ulsch_harq->o_ACK[i] == 1, "harq_ack[%d] is %d, should be 1,2 or 4\n", i, ulsch_harq->o_ACK[i]);
      pdu->harq_indication_fdd_rel13.harq_tb_n[i] = 2 - ulsch_harq->o_ACK[i];
      // release DLSCH if needed
      release_harq(eNB,DLSCH_id,i,frame,subframe,0xffff, ulsch_harq->o_ACK[i] == 1);
    }
  } else {                      // TDD
    M = ul_ACK_subframe2_M (&eNB->frame_parms, subframe);
    pdu->harq_indication_tdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL13_TAG;
    pdu->harq_indication_tdd_rel13.mode = 1-bundling;
    pdu->harq_indication_tdd_rel13.number_of_ack_nack = ulsch_harq->O_ACK;

    for (i = 0; i < ulsch_harq->O_ACK; i++) {
      AssertFatal (ulsch_harq->o_ACK[i] == 0 || ulsch_harq->o_ACK[i] == 1, "harq_ack[%d] is %d, should be 1,2 or 4\n", i, ulsch_harq->o_ACK[i]);
      pdu->harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = 2-ulsch_harq->o_ACK[i];
      // release DLSCH if needed
      /* TODO: review this code, it's most certainly wrong.
       * We have to release the proper HARQ in case of ACK or NACK if max retransmission reached.
       * Basically, call release_harq with 1 as last argument when ACK and 0 when NACK.
       */
      release_harq(eNB,DLSCH_id,i,frame,subframe,0xffff, ulsch_harq->o_ACK[i] == 1);

      if      (M==1 && ulsch_harq->O_ACK==1 && ulsch_harq->o_ACK[i] == 1) release_harq(eNB,DLSCH_id,0,frame,subframe,0xffff, ulsch_harq->o_ACK[i] == 1);
      else if (M==1 && ulsch_harq->O_ACK==2 && ulsch_harq->o_ACK[i] == 1) release_harq(eNB,DLSCH_id,i,frame,subframe,0xffff, ulsch_harq->o_ACK[i] == 1);
      else if (M>1 && ulsch_harq->o_ACK[i] == 1) {
        // spatial bundling
        release_harq(eNB,DLSCH_id,0,frame,subframe,1<<i, ulsch_harq->o_ACK[i] == 1);
        release_harq(eNB,DLSCH_id,1,frame,subframe,1<<i, ulsch_harq->o_ACK[i] == 1);
      }
    }
  }

  //LOG_E(PHY,"eNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs:%d\n", eNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs);
  eNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs++;
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);
}

#define packetError(ConD, fmt, args...) if (!(ConD)) { LOG_E(PHY, fmt, args); goodPacket=false; }

void fill_uci_harq_indication (int UEid, PHY_VARS_eNB *eNB, LTE_eNB_UCI *uci, int frame, int subframe, uint8_t *harq_ack, uint8_t tdd_mapping_mode, uint16_t tdd_multiplexing_mask)
{
  int DLSCH_id=find_dlsch(uci->rnti,eNB,SEARCH_EXIST);

  //AssertFatal(DLSCH_id>=0,"DLSCH_id doesn't exist rnti:%x\n", uci->rnti);
  if (DLSCH_id < 0) {
    LOG_E(PHY,"SFN/SF:%04d%d Unable to find rnti:%x do not send HARQ\n", frame, subframe, uci->rnti);
    return;
  }

  pthread_mutex_lock(&eNB->UL_INFO_mutex);
  bool goodPacket=true;
  nfapi_harq_indication_t *ind       = &eNB->UL_INFO.harq_ind;
  nfapi_harq_indication_body_t *body = &ind->harq_indication_body;
  assert(eNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs <= NFAPI_HARQ_IND_MAX_PDU);
  nfapi_harq_indication_pdu_t *pdu   = &body->harq_pdu_list[eNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs];
  ind->sfn_sf = frame<<4|subframe;
  ind->header.message_id = NFAPI_HARQ_INDICATION;
  body->tl.tag = NFAPI_HARQ_INDICATION_BODY_TAG;
  pdu->instance_length                                = 0; // don't know what to do with this
  //  pdu->rx_ue_information.handle                       = handle;
  pdu->rx_ue_information.tl.tag                       = NFAPI_RX_UE_INFORMATION_TAG;
  pdu->rx_ue_information.rnti                         = uci->rnti;
  // estimate UL_CQI for MAC (from antenna port 0 only)
  pdu->ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;
  int SNRtimes10 = dB_fixed_x10(uci->stat) - 10 * eNB->measurements.n0_pucch_dB;

  if (SNRtimes10 < -100)
    LOG_I (PHY, "uci->stat %d \n", uci->stat);

  if (SNRtimes10 < -640)
    pdu->ul_cqi_information.ul_cqi = 0;
  else if (SNRtimes10 > 635)
    pdu->ul_cqi_information.ul_cqi = 255;
  else
    pdu->ul_cqi_information.ul_cqi = (640 + SNRtimes10) / 5;

  pdu->ul_cqi_information.channel = 0;

  if (eNB->frame_parms.frame_type == FDD) {
    if (uci->pucch_fmt == pucch_format1a) {
      pdu->harq_indication_fdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_FDD_REL13_TAG;
      pdu->harq_indication_fdd_rel13.mode = 0;
      pdu->harq_indication_fdd_rel13.number_of_ack_nack = 1;
      packetError (harq_ack[0] == 1 || harq_ack[0] == 2 || harq_ack[0] == 4, "harq_ack[0] is %d, should be 1,2 or 4\n", harq_ack[0]);
      if (goodPacket) {
	pdu->harq_indication_fdd_rel13.harq_tb_n[0] = harq_ack[0];
	// release DLSCH if needed
	release_harq(eNB,DLSCH_id,0,frame,subframe,0xffff, harq_ack[0] == 1);
      }
    } else if (uci->pucch_fmt == pucch_format1b) {
      pdu->harq_indication_fdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_FDD_REL13_TAG;
      pdu->harq_indication_fdd_rel13.mode = 0;
      pdu->harq_indication_fdd_rel13.number_of_ack_nack = 2;
      packetError (harq_ack[0] == 1 || harq_ack[0] == 2 || harq_ack[1] == 4, "harq_ack[0] is %d, should be 0,1 or 4\n", harq_ack[0]);
      packetError (harq_ack[1] == 1 || harq_ack[1] == 2 || harq_ack[1] == 4, "harq_ack[1] is %d, should be 0,1 or 4\n", harq_ack[1]);
      if (goodPacket) {
	pdu->harq_indication_fdd_rel13.harq_tb_n[0] = harq_ack[0];
	pdu->harq_indication_fdd_rel13.harq_tb_n[1] = harq_ack[1];
	// release DLSCH if needed
	release_harq(eNB,DLSCH_id,0,frame,subframe,0xffff, harq_ack[0] == 1);
	release_harq(eNB,DLSCH_id,1,frame,subframe,0xffff, harq_ack[1] == 1);
      }
    } else
      packetError(1==0,"only format 1a/b for now, received %d\n",uci->pucch_fmt);
  } else { // TDD
    packetError (tdd_mapping_mode == 0 || tdd_mapping_mode == 1 || tdd_mapping_mode == 2, "Illegal tdd_mapping_mode %d\n", tdd_mapping_mode);
    if (goodPacket) {
      pdu->harq_indication_tdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL13_TAG;
      pdu->harq_indication_tdd_rel13.mode = tdd_mapping_mode;
      LOG_D(PHY,"%s(eNB, uci_harq format %d, rnti:%04x, frame:%d, subframe:%d, tdd_mapping_mode:%d) harq_ack[0]:%d harq_ack[1]:%d\n", __FUNCTION__, uci->pucch_fmt,uci->rnti, frame, subframe,
	    tdd_mapping_mode,harq_ack[0],harq_ack[1]);

      switch (tdd_mapping_mode) {
      case 0:                    // bundling
        if (uci->pucch_fmt == pucch_format1a) {
          pdu->harq_indication_tdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL13_TAG;
          pdu->harq_indication_tdd_rel13.number_of_ack_nack = 1;
          LOG_D(PHY,"bundling, pucch1a, number of ack nack %d\n",pdu->harq_indication_tdd_rel13.number_of_ack_nack);
          packetError(harq_ack[0] == 1 || harq_ack[0] == 2 || harq_ack[0] == 4, "harq_ack[0] is %d, should be 1,2 or 4\n",harq_ack[0]);
	  if (goodPacket) {
	    pdu->harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = harq_ack[0];
	    // release all bundled DLSCH if needed
	    release_harq(eNB,DLSCH_id,0,frame,subframe,0xffff, harq_ack[0] == 1);
	  }
        } else if (uci->pucch_fmt == pucch_format1b) {
          pdu->harq_indication_tdd_rel13.number_of_ack_nack = 2;
          packetError(harq_ack[0] == 1 || harq_ack[0] == 2 || harq_ack[1] == 4, "harq_ack[0] is %d, should be 0,1 or 4\n",harq_ack[0]);
          packetError(harq_ack[1] == 1 || harq_ack[1] == 2 || harq_ack[1] == 4, "harq_ack[1] is %d, should be 0,1 or 4\n",harq_ack[1]);
	  if (goodPacket) {
	    pdu->harq_indication_tdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL13_TAG;
	    pdu->harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = harq_ack[0];
	    pdu->harq_indication_tdd_rel13.harq_data[1].bundling.value_0 = harq_ack[1];
	    // release all DLSCH if needed
	    release_harq(eNB,DLSCH_id,0,frame,subframe,0xffff, harq_ack[0] == 1);
	    release_harq(eNB,DLSCH_id,1,frame,subframe,0xffff, harq_ack[1] == 1);
	  }
        }

        break;

      case 1:                    // multiplexing
        packetError (uci->pucch_fmt == pucch_format1b, "uci->pucch_format %d is not format1b\n", uci->pucch_fmt);
	if (goodPacket) {
	  if (uci->num_pucch_resources == 1 && uci->pucch_fmt == pucch_format1a) {
	    pdu->harq_indication_tdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL13_TAG;
	    pdu->harq_indication_tdd_rel13.number_of_ack_nack = 1;
	    packetError(harq_ack[0] == 1 || harq_ack[0] == 2 || harq_ack[0] == 4, "harq_ack[0] is %d, should be 1,2 or 4\n",harq_ack[0]);
	    if (goodPacket) {
	      pdu->harq_indication_tdd_rel13.harq_data[0].multiplex.value_0 = harq_ack[0];
	      // release all DLSCH if needed
	      release_harq(eNB,DLSCH_id,0,frame,subframe,0xffff, harq_ack[0] == 1);
	    }
        } else if (uci->num_pucch_resources == 1 && uci->pucch_fmt == pucch_format1b) {
	    pdu->harq_indication_tdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL13_TAG;
	    pdu->harq_indication_tdd_rel13.number_of_ack_nack = 2;
	    packetError(harq_ack[0] == 1 || harq_ack[0] == 2 || harq_ack[1] == 4, "harq_ack[0] is %d, should be 0,1 or 4\n",harq_ack[0]);
	    packetError(harq_ack[1] == 1 || harq_ack[1] == 2 || harq_ack[1] == 4, "harq_ack[1] is %d, should be 0,1 or 4\n",harq_ack[1]);
	    if (goodPacket) {
	      pdu->harq_indication_tdd_rel13.harq_data[0].multiplex.value_0 = harq_ack[0];
	      pdu->harq_indication_tdd_rel13.harq_data[1].multiplex.value_0 = harq_ack[1];
	      // release all DLSCH if needed
	      release_harq(eNB,DLSCH_id,0,frame,subframe,0xffff, harq_ack[0] == 1);
	      release_harq(eNB,DLSCH_id,1,frame,subframe,0xffff, harq_ack[1] == 1);
	    }
        } else { // num_pucch_resources (M) > 1
          pdu->harq_indication_tdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL13_TAG;
          pdu->harq_indication_tdd_rel13.number_of_ack_nack = uci->num_pucch_resources;
          pdu->harq_indication_tdd_rel13.harq_data[0].multiplex.value_0 = harq_ack[0];
          pdu->harq_indication_tdd_rel13.harq_data[1].multiplex.value_0 = harq_ack[1];

          if (uci->num_pucch_resources == 3)  pdu->harq_indication_tdd_rel13.harq_data[2].multiplex.value_0 = harq_ack[2];

          if (uci->num_pucch_resources == 4)  pdu->harq_indication_tdd_rel13.harq_data[3].multiplex.value_0 = harq_ack[3];

          // spatial-bundling in this case so release both HARQ if necessary
          release_harq(eNB,DLSCH_id,0,frame,subframe,tdd_multiplexing_mask, 1 /* force release? previous code was unconditional */);
          release_harq(eNB,DLSCH_id,1,frame,subframe,tdd_multiplexing_mask, 1 /* force release? previous code was unconditional */);
        }

	  break;

      case 2: // special bundling (SR collision)
        pdu->harq_indication_tdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL13_TAG;
        pdu->harq_indication_tdd_rel13.number_of_ack_nack = 1;
        pdu->harq_indication_tdd_rel13.mode = 0;
        int tdd_config5_sf2scheds=0;

        if (eNB->frame_parms.tdd_config==5) tdd_config5_sf2scheds = getM(eNB,frame,subframe);

        switch (harq_ack[0]) {
          case 0:
          case 4:
            pdu->harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = 0;
            /* TODO: release_harq here? this whole code looks suspicious */
            break;

          case 1: // check if M=1,4,7
            if (uci->num_pucch_resources == 1 || uci->num_pucch_resources == 4 ||
                tdd_config5_sf2scheds == 1 || tdd_config5_sf2scheds == 4 || tdd_config5_sf2scheds == 7) {
              pdu->harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = 1;
              release_harq(eNB,DLSCH_id,0,frame,subframe,0xffff, 1);
              release_harq(eNB,DLSCH_id,1,frame,subframe,0xffff, 1);
            } else {
              pdu->harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = 0;
            }

            break;

          case 2: // check if M=2,5,8
            if (uci->num_pucch_resources == 2 || tdd_config5_sf2scheds == 2 ||
                tdd_config5_sf2scheds == 5 || tdd_config5_sf2scheds == 8) {
              pdu->harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = 1;
              release_harq(eNB,DLSCH_id,0,frame,subframe,0xffff, 1);
              release_harq(eNB,DLSCH_id,1,frame,subframe,0xffff, 1);
            } else {
              pdu->harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = 0;
            }

            break;

          case 3: // check if M=3,6,9
            if (uci->num_pucch_resources == 3 || tdd_config5_sf2scheds == 3 ||
                tdd_config5_sf2scheds == 6 || tdd_config5_sf2scheds == 9) {
              pdu->harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = 1;
              release_harq(eNB,DLSCH_id,0,frame,subframe,0xffff, 1);
              release_harq(eNB,DLSCH_id,1,frame,subframe,0xffff, 1);
            } else {
              pdu->harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = 0;
            }
	    
            break;
        }

        break;
	}
      }
    }
  }                             //TDD
  
  if (goodPacket) {
    eNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs++;
    LOG_D(PHY,"Incremented eNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs:%d\n", eNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs);
  } else {
    LOG_W(PHY,"discarded a PUCCH because the decoded values are impossible\n");
  }
    
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);
}

void fill_crc_indication (PHY_VARS_eNB *eNB, int ULSCH_id, int frame, int subframe, uint8_t crc_flag) {
  pthread_mutex_lock(&eNB->UL_INFO_mutex);
  assert(eNB->UL_INFO.crc_ind.crc_indication_body.number_of_crcs <= NFAPI_CRC_IND_MAX_PDU);
  nfapi_crc_indication_pdu_t *pdu =   &eNB->UL_INFO.crc_ind.crc_indication_body.crc_pdu_list[eNB->UL_INFO.crc_ind.crc_indication_body.number_of_crcs];
  eNB->UL_INFO.crc_ind.sfn_sf                         = frame<<4 | subframe;
  eNB->UL_INFO.crc_ind.header.message_id              = NFAPI_CRC_INDICATION;
  eNB->UL_INFO.crc_ind.crc_indication_body.tl.tag     = NFAPI_CRC_INDICATION_BODY_TAG;
  pdu->instance_length = 0;     // don't know what to do with this
  //  pdu->rx_ue_information.handle                       = handle;
  pdu->rx_ue_information.tl.tag                       = NFAPI_RX_UE_INFORMATION_TAG;
  pdu->rx_ue_information.rnti                         = eNB->ulsch[ULSCH_id]->rnti;
  pdu->crc_indication_rel8.tl.tag                     = NFAPI_CRC_INDICATION_REL8_TAG;
  pdu->crc_indication_rel8.crc_flag                   = crc_flag;
  eNB->UL_INFO.crc_ind.crc_indication_body.number_of_crcs++;
  //LOG_D(PHY, "%s() rnti:%04x crcs:%d crc_flag:%d\n", __FUNCTION__, pdu->rx_ue_information.rnti, eNB->UL_INFO.crc_ind.crc_indication_body.number_of_crcs, crc_flag);
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);
}

void phy_procedures_eNB_uespec_RX(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc) {
  //RX processing for ue-specific resources (i
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  const int       subframe = proc->subframe_rx;
  const int       frame = proc->frame_rx;
  /* TODO: use correct rxdata */
  T (T_ENB_PHY_INPUT_SIGNAL, T_INT (eNB->Mod_id), T_INT (frame), T_INT (subframe), T_INT (0),
     T_BUFFER (&eNB->RU_list[0]->common.rxdata[0][subframe * eNB->frame_parms.samples_per_tti], eNB->frame_parms.samples_per_tti * 4));

  if ((fp->frame_type == TDD) && (subframe_select(fp,subframe)!=SF_UL)) return;

  T(T_ENB_PHY_UL_TICK, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe));
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_RX_UESPEC, 1 );
  LOG_D (PHY, "[eNB %d] Frame %d: Doing phy_procedures_eNB_uespec_RX(%d)\n", eNB->Mod_id, frame, subframe);
  eNB->rb_mask_ul[0] = 0;
  eNB->rb_mask_ul[1] = 0;
  eNB->rb_mask_ul[2] = 0;
  eNB->rb_mask_ul[3] = 0;
  // Fix me here, these should be locked
  eNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus  = 0;
  eNB->UL_INFO.crc_ind.crc_indication_body.number_of_crcs = 0;
  // Call SRS first since all others depend on presence of SRS or lack thereof
  srs_procedures (eNB, proc);
  eNB->first_run_I0_measurements = 0;
  uci_procedures (eNB, proc);

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) { // If PNF or monolithic
    pusch_procedures(eNB,proc);
  }

  lte_eNB_I0_measurements (eNB, subframe, 0, eNB->first_run_I0_measurements);

  // clear unused statistics after 2 seconds
  for (int i=0;i<NUMBER_OF_SCH_STATS_MAX;i++) {
     if (eNB->ulsch_stats[i].frame <= frame) {
         if ((eNB->ulsch_stats[i].frame + 200) < frame) memset(&eNB->ulsch_stats[i],0,sizeof(eNB->ulsch_stats[i]));
     }
     else {
         if (eNB->ulsch_stats[i].frame + 200 < (frame+1024)) memset(&eNB->ulsch_stats[i],0,sizeof(eNB->ulsch_stats[i]));
     }
     if (eNB->uci_stats[i].frame <= frame) {
         if ((eNB->uci_stats[i].frame + 200) < frame) memset(&eNB->uci_stats[i],0,sizeof(eNB->uci_stats[i]));
     }
     else {
         if (eNB->uci_stats[i].frame + 200 < (frame+1024)) memset(&eNB->uci_stats[i],0,sizeof(eNB->uci_stats[i]));
     }
  }


  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_RX_UESPEC, 0 );
}

void release_rnti_of_phy(module_id_t mod_id) {
  int i,j;
  int CC_id;
  rnti_t rnti;
  PHY_VARS_eNB *eNB_PHY = NULL;
  LTE_eNB_ULSCH_t *ulsch = NULL;
  LTE_eNB_DLSCH_t *dlsch = NULL;

  for(i = 0; i< release_rntis.number_of_TLVs; i++) {
    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
      eNB_PHY = RC.eNB[mod_id][CC_id];
      rnti = release_rntis.ue_release_request_TLVs_list[i].rnti;

      for (j=0; j<NUMBER_OF_DLSCH_MAX; j++) {

        dlsch = eNB_PHY->dlsch[j][0];

        if((dlsch != NULL) && (dlsch->rnti == rnti)) {
          LOG_I(PHY, "clean_eNb_dlsch dlsch[%d] UE %x \n", j, rnti);
          clean_eNb_dlsch(dlsch);
        }
      }

      for (j=0;j<NUMBER_OF_ULSCH_MAX; j++) {
        ulsch = eNB_PHY->ulsch[j];

        if((ulsch != NULL) && (ulsch->rnti == rnti)) {
          LOG_I(PHY, "clean_eNb_ulsch ulsch[%d] UE %x\n", j, rnti);
          clean_eNb_ulsch(ulsch);
          for (j=0;j<NUMBER_OF_ULSCH_MAX;j++)
             if (eNB_PHY->ulsch_stats[j].rnti == rnti) {eNB_PHY->ulsch_stats[j].rnti=0; break;}
        }
      }

      for(j=0; j<NUMBER_OF_UCI_MAX; j++) {
        if(eNB_PHY->uci_vars[j].rnti == rnti) {
          LOG_I(PHY, "clean eNb uci_vars[%d] UE %x \n",j, rnti);
          memset(&eNB_PHY->uci_vars[i],0,sizeof(LTE_eNB_UCI));
        }
      }
    }
  }

  memset(&release_rntis, 0, sizeof(nfapi_ue_release_request_body_t));
}
