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

/*! \file fapi_l1.c
 * \brief functions for FAPI L1 interface
 * \author R. Knopp
 * \date 2017
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */

#include "PHY/defs_eNB.h"
#include "PHY/LTE_TRANSPORT/transport_proto.h"
#include "SCHED/sched_eNB.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "nfapi_pnf_interface.h"
#include "fapi_l1.h"

int oai_nfapi_dl_config_req(nfapi_dl_config_request_t *dl_config_req);
int oai_nfapi_tx_req(nfapi_tx_request_t *tx_req);
int oai_nfapi_hi_dci0_req(nfapi_hi_dci0_request_t *hi_dci0_req);
int oai_nfapi_ul_config_req(nfapi_ul_config_request_t *ul_config_req);


void handle_nfapi_dci_dl_pdu(PHY_VARS_eNB *eNB,
                             int frame, int subframe,
                             L1_rxtx_proc_t *proc,
                             nfapi_dl_config_request_pdu_t *dl_config_pdu) {
  int idx                         = subframe&1;
  LTE_eNB_PDCCH *pdcch_vars       = &eNB->pdcch_vars[idx];
  nfapi_dl_config_dci_dl_pdu *pdu = &dl_config_pdu->dci_dl_pdu;

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  LOG_D(PHY,"Frame %d, Subframe %d: DCI processing - populating pdcch_vars->dci_alloc[%d] proc:subframe_tx:%d idx:%d pdcch_vars->num_dci:%d\n",frame,subframe, pdcch_vars->num_dci, proc->subframe_tx,
        idx, pdcch_vars->num_dci);
  // copy dci configuration into eNB structure
  fill_dci_and_dlsch(eNB,frame,subframe,proc,&pdcch_vars->dci_alloc[pdcch_vars->num_dci],pdu);
  LOG_D(PHY,"Frame %d, Subframe %d: DCI processing - populated pdcch_vars->dci_alloc[%d] proc:subframe_tx:%d idx:%d pdcch_vars->num_dci:%d\n",proc->frame_tx,proc->subframe_tx, pdcch_vars->num_dci,
        proc->subframe_tx, idx, pdcch_vars->num_dci);
}

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))

void handle_nfapi_mpdcch_pdu(PHY_VARS_eNB *eNB,
                             L1_rxtx_proc_t *proc,
                             nfapi_dl_config_request_pdu_t *dl_config_pdu) {
  int idx                         = proc->subframe_tx&1;
  LTE_eNB_MPDCCH *mpdcch_vars     = &eNB->mpdcch_vars[idx];
  nfapi_dl_config_mpdcch_pdu *pdu = &dl_config_pdu->mpdcch_pdu;

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  LOG_D(PHY,"Frame %d, Subframe %d: MDCI processing\n",proc->frame_tx,proc->subframe_tx);
  // copy dci configuration into eNB structure
  fill_mdci_and_dlsch(eNB,proc,&mpdcch_vars->mdci_alloc[mpdcch_vars->num_dci],pdu);
}

#endif

void handle_nfapi_hi_dci0_dci_pdu(PHY_VARS_eNB *eNB,int frame,int subframe,L1_rxtx_proc_t *proc,
                                  nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu) {
  int idx                         = subframe&1;
  LTE_eNB_PDCCH *pdcch_vars       = &eNB->pdcch_vars[idx];

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  //LOG_D(PHY,"%s() SFN/SF:%04d%d Before num_dci:%d\n", __FUNCTION__,frame,subframe,pdcch_vars->num_dci);
  // copy dci configuration in to eNB structure
  fill_dci0(eNB,frame,subframe,proc,&pdcch_vars->dci_alloc[pdcch_vars->num_dci], &hi_dci0_config_pdu->dci_pdu);
}



void handle_nfapi_hi_dci0_mpdcch_dci_pdu(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc,
    nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu) {
  int idx                         = proc->subframe_tx&1;
  LTE_eNB_MPDCCH *pdcch_vars      = &eNB->mpdcch_vars[idx];

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  // copy dci configuration in to eNB structure
  fill_mpdcch_dci0(eNB,proc,&pdcch_vars->mdci_alloc[pdcch_vars->num_dci], &hi_dci0_config_pdu->mpdcch_dci_pdu);
}


void handle_nfapi_hi_dci0_hi_pdu(PHY_VARS_eNB *eNB,int frame,int subframe,L1_rxtx_proc_t *proc,
                                 nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu) {
  LTE_eNB_PHICH *phich = &eNB->phich_vars[subframe&1];

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  // copy dci configuration in to eNB structure
  LOG_D(PHY,"Received HI PDU with value %d (rbstart %d,cshift %d)\n",
        hi_dci0_config_pdu->hi_pdu.hi_pdu_rel8.hi_value,
        hi_dci0_config_pdu->hi_pdu.hi_pdu_rel8.resource_block_start,
        hi_dci0_config_pdu->hi_pdu.hi_pdu_rel8.cyclic_shift_2_for_drms);
  // DJP - TODO FIXME - transmission power ignored
  phich->config[phich->num_hi].hi       = hi_dci0_config_pdu->hi_pdu.hi_pdu_rel8.hi_value;
  phich->config[phich->num_hi].first_rb = hi_dci0_config_pdu->hi_pdu.hi_pdu_rel8.resource_block_start;
  phich->config[phich->num_hi].n_DMRS   = hi_dci0_config_pdu->hi_pdu.hi_pdu_rel8.cyclic_shift_2_for_drms;
  phich->num_hi++;
  AssertFatal(phich->num_hi<32,"Maximum number of phich reached in subframe\n");
}

void handle_nfapi_bch_pdu(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc,
                          nfapi_dl_config_request_pdu_t *dl_config_pdu,
                          uint8_t *sdu) {
  nfapi_dl_config_bch_pdu_rel8_t *rel8 = &dl_config_pdu->bch_pdu.bch_pdu_rel8;

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  AssertFatal(rel8->length == 3, "BCH PDU has length %d != 3\n",rel8->length);
  //LOG_D(PHY,"bch_pdu: %x,%x,%x\n",sdu[0],sdu[1],sdu[2]);
  eNB->pbch_pdu[0] = sdu[2];
  eNB->pbch_pdu[1] = sdu[1];
  eNB->pbch_pdu[2] = sdu[0];
  // adjust transmit amplitude here based on NFAPI info
}

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  extern uint32_t localRIV2alloc_LUT6[32];
  extern uint32_t localRIV2alloc_LUT25[512];
  extern uint32_t localRIV2alloc_LUT50_0[1600];
  extern uint32_t localRIV2alloc_LUT50_1[1600];
  extern uint32_t localRIV2alloc_LUT100_0[6000];
  extern uint32_t localRIV2alloc_LUT100_1[6000];
  extern uint32_t localRIV2alloc_LUT100_2[6000];
  extern uint32_t localRIV2alloc_LUT100_3[6000];
#endif

void handle_nfapi_dlsch_pdu(PHY_VARS_eNB *eNB,int frame,int subframe,L1_rxtx_proc_t *proc,
                            nfapi_dl_config_request_pdu_t *dl_config_pdu,
                            uint8_t codeword_index,
                            uint8_t *sdu) {
  nfapi_dl_config_dlsch_pdu_rel8_t *rel8 = &dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8;
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
  nfapi_dl_config_dlsch_pdu_rel10_t *rel10 = &dl_config_pdu->dlsch_pdu.dlsch_pdu_rel10;
#endif
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  nfapi_dl_config_dlsch_pdu_rel13_t *rel13 = &dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13;
#endif
  LTE_eNB_DLSCH_t *dlsch0=NULL,*dlsch1=NULL;
  LTE_DL_eNB_HARQ_t *dlsch0_harq=NULL,*dlsch1_harq=NULL;
  int UE_id;
  int harq_pid;

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  UE_id = find_dlsch(rel8->rnti,eNB,SEARCH_EXIST_OR_FREE);

  if( (UE_id<0) || (UE_id>=NUMBER_OF_UE_MAX) ) {
    LOG_E(PHY,"illegal UE_id found!!! rnti %04x UE_id %d\n",rel8->rnti,UE_id);
    return;
  }

  //AssertFatal(UE_id!=-1,"no free or exiting dlsch_context\n");
  //AssertFatal(UE_id<NUMBER_OF_UE_MAX,"returned UE_id %d >= %d(NUMBER_OF_UE_MAX)\n",UE_id,NUMBER_OF_UE_MAX);
  dlsch0 = eNB->dlsch[UE_id][0];
  dlsch1 = eNB->dlsch[UE_id][1];
#if (LTE_RRC_VERSION >= MAKE_VERSION(13, 0, 0))

  if ((rel13->pdsch_payload_type < 2) && (rel13->ue_type>0)) dlsch0->harq_ids[proc->frame_tx%2][proc->subframe_tx] = 0;

#endif
  harq_pid        = dlsch0->harq_ids[proc->frame_tx%2][proc->subframe_tx];
  AssertFatal((harq_pid>=0) && (harq_pid<8),"harq_pid %d not in 0...7 frame:%d subframe:%d subframe(TX):%d rnti:%x UE_id:%d dlsch0[harq_ids:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d]\n",
              harq_pid,
              frame,subframe,
              proc->subframe_tx,rel8->rnti,UE_id,
              dlsch0->harq_ids[proc->frame_tx%2][0],
              dlsch0->harq_ids[proc->frame_tx%2][1],
              dlsch0->harq_ids[proc->frame_tx%2][2],
              dlsch0->harq_ids[proc->frame_tx%2][3],
              dlsch0->harq_ids[proc->frame_tx%2][4],
              dlsch0->harq_ids[proc->frame_tx%2][5],
              dlsch0->harq_ids[proc->frame_tx%2][6],
              dlsch0->harq_ids[proc->frame_tx%2][7],
              dlsch0->harq_ids[proc->frame_tx%2][8],
              dlsch0->harq_ids[proc->frame_tx%2][9]
             );
  dlsch0_harq     = dlsch0->harq_processes[harq_pid];
  dlsch1_harq     = dlsch1->harq_processes[harq_pid];
  AssertFatal(dlsch0_harq!=NULL,"dlsch_harq is null\n");
  // compute DL power control parameters
  eNB->pdsch_config_dedicated[UE_id].p_a = rel8->pa;
#ifdef PHY_TX_THREAD

  if (dlsch0->active[proc->subframe_tx]) {
# else

  if (dlsch0->active) {
#endif
    computeRhoA_eNB(rel8->pa, dlsch0,dlsch0_harq->dl_power_off, eNB->frame_parms.nb_antenna_ports_eNB);
    computeRhoB_eNB(rel8->pa,eNB->frame_parms.pdsch_config_common.p_b,eNB->frame_parms.nb_antenna_ports_eNB,dlsch0,dlsch0_harq->dl_power_off);
  }

#ifdef PHY_TX_THREAD

  if (dlsch1->active[proc->subframe_tx]) {
#else

  if (dlsch1->active) {
#endif
    computeRhoA_eNB(rel8->pa, dlsch1,dlsch1_harq->dl_power_off, eNB->frame_parms.nb_antenna_ports_eNB);
    computeRhoB_eNB(rel8->pa,eNB->frame_parms.pdsch_config_common.p_b,eNB->frame_parms.nb_antenna_ports_eNB,dlsch1,dlsch1_harq->dl_power_off);
  }

  dlsch0_harq->pdsch_start = eNB->pdcch_vars[proc->subframe_tx & 1].num_pdcch_symbols;

  if (dlsch0_harq->round==0) {  //get pointer to SDU if this a new SDU
    if(sdu == NULL) {
      LOG_E(PHY,
            "NFAPI: SFN/SF:%04d%d proc:TX:[frame %d subframe %d]: programming dlsch for round 0, rnti %x, UE_id %d, harq_pid %d : sdu is null for pdu_index %d dlsch0_harq[round:%d SFN/SF:%d%d pdu:%p mcs:%d ndi:%d pdschstart:%d]\n",
            frame,subframe,
            proc->frame_tx,proc->subframe_tx,rel8->rnti,UE_id,harq_pid,
            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index,dlsch0_harq->round,dlsch0_harq->frame,dlsch0_harq->subframe,dlsch0_harq->pdu,dlsch0_harq->mcs,dlsch0_harq->ndi,dlsch0_harq->pdsch_start);
      return;
    }

    //AssertFatal(sdu!=NULL,"NFAPI: SFN/SF:%04d%d proc:TX:[frame %d subframe %d]: programming dlsch for round 0, rnti %x, UE_id %d, harq_pid %d : sdu is null for pdu_index %d dlsch0_harq[round:%d SFN/SF:%d%d pdu:%p mcs:%d ndi:%d pdschstart:%d]\n",
    //            frame,subframe,
    //            proc->frame_tx,proc->subframe_tx,rel8->rnti,UE_id,harq_pid,
    //            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index,dlsch0_harq->round,dlsch0_harq->frame,dlsch0_harq->subframe,dlsch0_harq->pdu,dlsch0_harq->mcs,dlsch0_harq->ndi,dlsch0_harq->pdsch_start);
    if (rel8->rnti != 0xFFFF) LOG_D(PHY,"NFAPI: SFN/SF:%04d%d proc:TX:[frame %d, subframe %d]: programming dlsch for round 0, rnti %x, UE_id %d, harq_pid %d\n",
                                      frame,subframe,proc->frame_tx,proc->subframe_tx,rel8->rnti,UE_id,harq_pid);

    if (codeword_index == 0) dlsch0_harq->pdu                    = sdu;
    else                     dlsch1_harq->pdu                    = sdu;
  } else {
    if (rel8->rnti != 0xFFFF) LOG_D(PHY,"NFAPI: SFN/SF:%04d%d proc:TX:[frame %d, subframe %d]: programming dlsch for round %d, rnti %x, UE_id %d, harq_pid %d\n",
                                      frame,subframe,proc->frame_tx,proc->subframe_tx,dlsch0_harq->round,
                                      rel8->rnti,UE_id,harq_pid);
  }

#if (LTE_RRC_VERSION >= MAKE_VERSION(13, 0, 0))
#ifdef PHY_TX_THREAD
  dlsch0_harq->sib1_br_flag=0;
#else
  dlsch0->sib1_br_flag=0;
#endif

  if ((rel13->pdsch_payload_type <2) && (rel13->ue_type>0)) { // this is a BR/CE UE and SIB1-BR/SI-BR
    UE_id = find_dlsch(rel8->rnti,eNB,SEARCH_EXIST_OR_FREE);
    AssertFatal(UE_id!=-1,"no free or exiting dlsch_context\n");
    AssertFatal(UE_id<NUMBER_OF_UE_MAX,"returned UE_id %d >= %d(NUMBER_OF_UE_MAX)\n",UE_id,NUMBER_OF_UE_MAX);
    dlsch0 = eNB->dlsch[UE_id][0];
    dlsch0->harq_mask = 1;
    dlsch0_harq     = dlsch0->harq_processes[0];
    dlsch0_harq->pdu                    = sdu;

    LOG_D(PHY,"NFAPI: frame %d, subframe %d (TX %d.%d): Programming SI-BR (%d) => %d\n",frame,subframe,proc->frame_tx,proc->subframe_tx,rel13->pdsch_payload_type,UE_id);

    dlsch0->rnti             = 0xFFFF;
    dlsch0->Kmimo            = 1;
    dlsch0->Mdlharq          = 4;
    dlsch0->Nsoft            = 25344;
    dlsch0->i0               = rel13->initial_transmission_sf_io;
    dlsch0_harq->pdsch_start = rel10->pdsch_start;
    dlsch0->harq_ids[proc->frame_tx%2][proc->subframe_rx] = 0;
    dlsch0_harq->frame       = proc->frame_tx;
    dlsch0_harq->subframe    = proc->subframe_tx;
#ifdef PHY_TX_THREAD

    if (rel13->pdsch_payload_type == 0) dlsch0_harq->sib1_br_flag=1;

#else

    if (rel13->pdsch_payload_type == 0) dlsch0->sib1_br_flag=1;

#endif

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

#ifdef PHY_TX_THREAD
    dlsch0->active[proc->subframe_tx]= 1;
#else
    dlsch0->active                  = 1;
#endif
    dlsch0_harq->nb_rb              = 6;
    dlsch0_harq->vrb_type           = LOCALIZED;
    dlsch0_harq->rvidx              = rel8->redundancy_version;
    dlsch0_harq->Nl                 = 1;
    dlsch0_harq->mimo_mode          = (eNB->frame_parms.nb_antenna_ports_eNB == 1) ? SISO : ALAMOUTI;
    dlsch0_harq->dl_power_off       = 1;
    dlsch0_harq->round              = 0;
    dlsch0_harq->status             = ACTIVE;
    dlsch0_harq->TBS                = rel8->length<<3;
    dlsch0_harq->Qm                 = rel8->modulation;
    dlsch0_harq->codeword           = 0;
    dlsch0_harq->pdsch_start        = rel10->pdsch_start;
  } else
#endif
  {
    UE_id = find_dlsch(rel8->rnti,eNB,SEARCH_EXIST_OR_FREE);
    AssertFatal(UE_id!=-1,"no free or exiting dlsch_context\n");
    AssertFatal(UE_id<NUMBER_OF_UE_MAX,"returned UE_id %d >= %d(NUMBER_OF_UE_MAX)\n",UE_id,NUMBER_OF_UE_MAX);
    dlsch0 = eNB->dlsch[UE_id][0];
    dlsch1 = eNB->dlsch[UE_id][1];
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
    dlsch0->sib1_br_flag=0;
    dlsch0->i0               = 0xFFFF;
#endif
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
    LOG_D(PHY,"dlsch->i0:%04x dlsch0_harq[pdsch_start:%d nb_rb:%d vrb_type:%d rvidx:%d Nl:%d mimo_mode:%d dl_power_off:%d round:%d status:%d TBS:%d Qm:%d codeword:%d rb_alloc:%d] rel8[length:%d]\n",
#ifdef PHY_TX_THREAD
          dlsch0_harq->i0,
#else
          dlsch0->i0,
#endif
          dlsch0_harq->pdsch_start, dlsch0_harq->nb_rb, dlsch0_harq->vrb_type, dlsch0_harq->rvidx, dlsch0_harq->Nl, dlsch0_harq->mimo_mode, dlsch0_harq->dl_power_off, dlsch0_harq->round, dlsch0_harq->status,
          dlsch0_harq->TBS, dlsch0_harq->Qm, dlsch0_harq->codeword, dlsch0_harq->rb_alloc[0],
          rel8->length
         );
#else
    LOG_D(PHY,"dlsch0_harq[pdsch_start:%d nb_rb:%d vrb_type:%d rvidx:%d Nl:%d mimo_mode:%d dl_power_off:%d round:%d status:%d TBS:%d Qm:%d codeword:%d rb_alloc:%d] rel8[length:%d]\n",
          dlsch0_harq->pdsch_start, dlsch0_harq->nb_rb, dlsch0_harq->vrb_type, dlsch0_harq->rvidx, dlsch0_harq->Nl, dlsch0_harq->mimo_mode, dlsch0_harq->dl_power_off, dlsch0_harq->round, dlsch0_harq->status,
          dlsch0_harq->TBS, dlsch0_harq->Qm, dlsch0_harq->codeword, dlsch0_harq->rb_alloc[0],
          rel8->length
         );
#endif
    dlsch0->active = 1;
    harq_pid        = dlsch0->harq_ids[frame%2][proc->subframe_tx];
    dlsch0->harq_mask |= (1<<harq_pid);
    AssertFatal((harq_pid>=0) && (harq_pid<8),"subframe %d: harq_pid %d not in 0...7\n",proc->subframe_tx,harq_pid);
    dlsch0_harq     = dlsch0->harq_processes[harq_pid];
    dlsch1_harq     = dlsch1->harq_processes[harq_pid];
    AssertFatal(dlsch0_harq!=NULL,"dlsch_harq is null\n");

    // compute DL power control parameters

    if (dlsch0->active) {
      computeRhoA_eNB(rel8->pa,dlsch0,dlsch0_harq->dl_power_off, eNB->frame_parms.nb_antenna_ports_eNB);
      computeRhoB_eNB(rel8->pa,eNB->frame_parms.pdsch_config_common.p_b,eNB->frame_parms.nb_antenna_ports_eNB,dlsch0,dlsch0_harq->dl_power_off);
    }

    if (dlsch1->active) {
      computeRhoA_eNB(rel8->pa, dlsch1,dlsch1_harq->dl_power_off, eNB->frame_parms.nb_antenna_ports_eNB);
      computeRhoB_eNB(rel8->pa,eNB->frame_parms.pdsch_config_common.p_b,eNB->frame_parms.nb_antenna_ports_eNB,dlsch1,dlsch1_harq->dl_power_off);
    }

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))

    if (rel13->ue_type>0)
      dlsch0_harq->pdsch_start = rel10->pdsch_start;
    else
#endif
      dlsch0_harq->pdsch_start = eNB->pdcch_vars[proc->subframe_tx & 1].num_pdcch_symbols;

    if (dlsch0_harq->round==0) {  //get pointer to SDU if this a new SDU
      AssertFatal(sdu!=NULL,"NFAPI: frame %d, subframe %d: programming dlsch for round 0, rnti %x, UE_id %d, harq_pid %d : sdu is null for pdu_index %d\n",
                  proc->frame_tx,proc->subframe_tx,rel8->rnti,UE_id,harq_pid,
                  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index);

      if (rel8->rnti != 0xFFFF) LOG_D(PHY,"NFAPI: frame %d, subframe %d: programming dlsch for round 0, rnti %x, UE_id %d, harq_pid %d\n",
                                        proc->frame_tx,proc->subframe_tx,rel8->rnti,UE_id,harq_pid);

      if (codeword_index == 0) dlsch0_harq->pdu                    = sdu;
      else                     dlsch1_harq->pdu                    = sdu;
    } else {
      if (rel8->rnti != 0xFFFF) LOG_D(PHY,"NFAPI: frame %d, subframe %d: programming dlsch for round %d, rnti %x, UE_id %d, harq_pid %d\n",
                                        proc->frame_tx,proc->subframe_tx,dlsch0_harq->round,
                                        rel8->rnti,UE_id,harq_pid);
    }
  }
}

int16_t to_beta_offset_harqack[16]= {16,20,25,32,40,50,64,80,101,127,160,248,400,640,1008,8};

void handle_ulsch_harq_pdu(
  PHY_VARS_eNB                           *eNB,
  int                                     UE_id,
  nfapi_ul_config_request_pdu_t          *ul_config_pdu,
  nfapi_ul_config_ulsch_harq_information *harq_information,
  uint16_t                                frame,
  uint8_t                                 subframe) {
  nfapi_ul_config_ulsch_pdu_rel8_t *rel8 = &ul_config_pdu->ulsch_harq_pdu.ulsch_pdu.ulsch_pdu_rel8;
  LTE_eNB_ULSCH_t *ulsch=eNB->ulsch[UE_id];
  LTE_UL_eNB_HARQ_t *ulsch_harq;

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  int harq_pid = rel8->harq_process_number;
  ulsch_harq = ulsch->harq_processes[harq_pid];
  ulsch_harq->frame                      = frame;
  ulsch_harq->subframe                   = subframe;
  ulsch_harq->O_ACK                      = harq_information->harq_information_rel10.harq_size;
  ulsch->beta_offset_harqack_times8      = to_beta_offset_harqack[harq_information->harq_information_rel10.delta_offset_harq];

  if (eNB->frame_parms.frame_type == TDD) {
    if (harq_information->harq_information_rel10.ack_nack_mode==0) //bundling
      ulsch->bundling = 1;
  }
}

uint16_t to_beta_offset_ri[16]= {9,13,16,20,25,32,40,50,64,80,101,127,160,0,0,0};
uint16_t to_beta_offset_cqi[16]= {0,0,9,10,11,13,14,16,18,20,23,25,28,32,40,50};

void handle_ulsch_cqi_ri_pdu(PHY_VARS_eNB *eNB,int UE_id,nfapi_ul_config_request_pdu_t *ul_config_pdu,uint16_t frame,uint8_t subframe) {
  nfapi_ul_config_cqi_ri_information_rel9_t *rel9 = &ul_config_pdu->ulsch_cqi_ri_pdu.cqi_ri_information.cqi_ri_information_rel9;
  LTE_eNB_ULSCH_t *ulsch        = eNB->ulsch[UE_id];
  int harq_pid = ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.harq_process_number;
  LTE_UL_eNB_HARQ_t *ulsch_harq = ulsch->harq_processes[harq_pid];

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  ulsch_harq->frame                       = frame;
  ulsch_harq->subframe                    = subframe;
  ulsch_harq->O_RI                        = rel9->aperiodic_cqi_pmi_ri_report.cc[0].ri_size;
  ulsch_harq->Or1                         = rel9->aperiodic_cqi_pmi_ri_report.cc[0].dl_cqi_pmi_size[0];

  if (ulsch_harq->O_RI>1) ulsch_harq->Or2 = rel9->aperiodic_cqi_pmi_ri_report.cc[0].dl_cqi_pmi_size[1];

  ulsch->beta_offset_ri_times8            = to_beta_offset_ri[rel9->delta_offset_ri];
  ulsch->beta_offset_cqi_times8           = to_beta_offset_cqi[rel9->delta_offset_cqi];
  LOG_D(PHY,"Filling ulsch_cqi_ri information for frame %d, subframe %d : O_RI %d, Or1 %d, beta_offset_cqi_times8 %d (%d)\n",
        frame,subframe,ulsch_harq->O_RI,ulsch_harq->Or1,ulsch->beta_offset_cqi_times8,
        rel9->delta_offset_cqi);
}

void handle_ulsch_cqi_harq_ri_pdu(PHY_VARS_eNB *eNB,int UE_id,nfapi_ul_config_request_pdu_t *ul_config_pdu,uint16_t frame,uint8_t subframe) {
  nfapi_ul_config_cqi_ri_information_rel9_t *rel9 = &ul_config_pdu->ulsch_cqi_harq_ri_pdu.cqi_ri_information.cqi_ri_information_rel9;
  LTE_eNB_ULSCH_t *ulsch        = eNB->ulsch[UE_id];
  int harq_pid = ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.harq_process_number;
  LTE_UL_eNB_HARQ_t *ulsch_harq = ulsch->harq_processes[harq_pid];
  nfapi_ul_config_ulsch_harq_information *harq_information = &ul_config_pdu->ulsch_cqi_harq_ri_pdu.harq_information;

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  ulsch_harq->frame                       = frame;
  ulsch_harq->subframe                    = subframe;
  ulsch_harq->O_RI                        = rel9->aperiodic_cqi_pmi_ri_report.cc[0].ri_size;
  ulsch_harq->Or1                         = rel9->aperiodic_cqi_pmi_ri_report.cc[0].dl_cqi_pmi_size[0];
  ulsch_harq->O_ACK                       = harq_information->harq_information_rel10.harq_size;

  if (ulsch_harq->O_RI>1) ulsch_harq->Or2 = rel9->aperiodic_cqi_pmi_ri_report.cc[0].dl_cqi_pmi_size[1];

  ulsch->beta_offset_harqack_times8       = to_beta_offset_harqack[harq_information->harq_information_rel10.delta_offset_harq];
  ulsch->beta_offset_ri_times8            = to_beta_offset_ri[rel9->delta_offset_ri];
  ulsch->beta_offset_cqi_times8           = to_beta_offset_cqi[rel9->delta_offset_cqi];
}

void handle_uci_harq_information(PHY_VARS_eNB *eNB, LTE_eNB_UCI *uci,nfapi_ul_config_harq_information *harq_information) {
  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  if (eNB->frame_parms.frame_type == FDD) {
    uci->num_pucch_resources = harq_information->harq_information_rel9_fdd.number_of_pucch_resources;
    LOG_D(PHY,"Programming UCI HARQ mode %d : size %d in (%d,%d)\n",
          harq_information->harq_information_rel9_fdd.ack_nack_mode,
          harq_information->harq_information_rel9_fdd.harq_size,
          uci->frame,uci->subframe);

    if ((harq_information->harq_information_rel9_fdd.ack_nack_mode == 0) &&
        (harq_information->harq_information_rel9_fdd.harq_size == 1)) {
      uci->pucch_fmt =  pucch_format1a;
      uci->n_pucch_1[0][0] = harq_information->harq_information_rel9_fdd.n_pucch_1_0;
      uci->n_pucch_1[0][1] = harq_information->harq_information_rel11.n_pucch_2_0;
    } else if ((harq_information->harq_information_rel9_fdd.ack_nack_mode == 0) &&
               (harq_information->harq_information_rel9_fdd.harq_size == 2)) {
      uci->pucch_fmt =  pucch_format1b;
      uci->n_pucch_1[0][0] = harq_information->harq_information_rel9_fdd.n_pucch_1_0;
      uci->n_pucch_1[0][1] = harq_information->harq_information_rel11.n_pucch_2_0;
    } else if ((harq_information->harq_information_rel9_fdd.ack_nack_mode == 1) &&
               (harq_information->harq_information_rel9_fdd.harq_size == 2)) {
      uci->pucch_fmt =  pucch_format1b_csA2;
      uci->n_pucch_1[0][0] = harq_information->harq_information_rel9_fdd.n_pucch_1_0;
      uci->n_pucch_1[0][1] = harq_information->harq_information_rel11.n_pucch_2_0;
      uci->n_pucch_1[1][0] = harq_information->harq_information_rel9_fdd.n_pucch_1_1;
      uci->n_pucch_1[1][1] = harq_information->harq_information_rel11.n_pucch_2_1;
    } else if ((harq_information->harq_information_rel9_fdd.ack_nack_mode == 1) &&
               (harq_information->harq_information_rel9_fdd.harq_size == 3)) {
      uci->pucch_fmt =  pucch_format1b_csA3;
      uci->n_pucch_1[0][0] = harq_information->harq_information_rel9_fdd.n_pucch_1_0;
      uci->n_pucch_1[0][1] = harq_information->harq_information_rel11.n_pucch_2_0;
      uci->n_pucch_1[1][0] = harq_information->harq_information_rel9_fdd.n_pucch_1_1;
      uci->n_pucch_1[1][1] = harq_information->harq_information_rel11.n_pucch_2_1;
      uci->n_pucch_1[2][0] = harq_information->harq_information_rel9_fdd.n_pucch_1_2;
      uci->n_pucch_1[2][1] = harq_information->harq_information_rel11.n_pucch_2_2;
    } else if ((harq_information->harq_information_rel9_fdd.ack_nack_mode == 1) &&
               (harq_information->harq_information_rel9_fdd.harq_size == 4)) {
      uci->pucch_fmt =  pucch_format1b_csA4;
      uci->n_pucch_1[0][0] = harq_information->harq_information_rel9_fdd.n_pucch_1_0;
      uci->n_pucch_1[0][1] = harq_information->harq_information_rel11.n_pucch_2_0;
      uci->n_pucch_1[1][0] = harq_information->harq_information_rel9_fdd.n_pucch_1_1;
      uci->n_pucch_1[1][1] = harq_information->harq_information_rel11.n_pucch_2_1;
      uci->n_pucch_1[2][0] = harq_information->harq_information_rel9_fdd.n_pucch_1_2;
      uci->n_pucch_1[2][1] = harq_information->harq_information_rel11.n_pucch_2_2;
    } else if (harq_information->harq_information_rel9_fdd.ack_nack_mode == 2) {
      uci->pucch_fmt =  pucch_format3;
      uci->n_pucch_3[0] = harq_information->harq_information_rel9_fdd.n_pucch_1_0;
      uci->n_pucch_3[1] = harq_information->harq_information_rel11.n_pucch_2_0;
    } else AssertFatal(1==0,"unsupported FDD HARQ mode %d size %d\n",harq_information->harq_information_rel9_fdd.ack_nack_mode,harq_information->harq_information_rel9_fdd.harq_size);
  } else { // TDD
    uci->num_pucch_resources = harq_information->harq_information_rel10_tdd.number_of_pucch_resources;

    if (harq_information->harq_information_rel10_tdd.ack_nack_mode == 0) {//bundling
      uci->pucch_fmt =  harq_information->harq_information_rel10_tdd.harq_size==1 ? pucch_format1a : pucch_format1b;
      uci->tdd_bundling = 1;
      uci->n_pucch_1[0][0] = harq_information->harq_information_rel10_tdd.n_pucch_1_0;
      uci->n_pucch_1[0][1] = harq_information->harq_information_rel11.n_pucch_2_0;
    } else if ((harq_information->harq_information_rel10_tdd.ack_nack_mode == 1) && //multiplexing
               (uci->num_pucch_resources == 1)) {
      uci->pucch_fmt = harq_information->harq_information_rel10_tdd.harq_size==1 ? pucch_format1a : pucch_format1b;
      uci->tdd_bundling = 0;
      uci->n_pucch_1[0][0] = harq_information->harq_information_rel10_tdd.n_pucch_1_0;
      uci->n_pucch_1[0][1] = harq_information->harq_information_rel11.n_pucch_2_0;
    } else if ((harq_information->harq_information_rel10_tdd.ack_nack_mode == 1) && //multiplexing M>1
               (uci->num_pucch_resources > 1)) {
      uci->pucch_fmt = pucch_format1b;
      uci->tdd_bundling = 0;
      uci->n_pucch_1[0][0] = harq_information->harq_information_rel10_tdd.n_pucch_1_0;
      uci->n_pucch_1[0][1] = harq_information->harq_information_rel11.n_pucch_2_0;
      uci->n_pucch_1[1][0] = harq_information->harq_information_rel10_tdd.n_pucch_1_1;
      uci->n_pucch_1[1][1] = harq_information->harq_information_rel11.n_pucch_2_1;
      uci->n_pucch_1[2][0] = harq_information->harq_information_rel10_tdd.n_pucch_1_2;
      uci->n_pucch_1[2][1] = harq_information->harq_information_rel11.n_pucch_2_2;
      uci->n_pucch_1[3][0] = harq_information->harq_information_rel10_tdd.n_pucch_1_3;
      uci->n_pucch_1[3][1] = harq_information->harq_information_rel11.n_pucch_2_3;
    } else if (harq_information->harq_information_rel10_tdd.ack_nack_mode == 2) {
      uci->pucch_fmt =  pucch_format3;
      uci->n_pucch_3[0] = harq_information->harq_information_rel10_tdd.n_pucch_1_0;
      uci->n_pucch_3[1] = harq_information->harq_information_rel11.n_pucch_2_0;
    } else AssertFatal(1==0,"unsupported HARQ mode %d\n",harq_information->harq_information_rel10_tdd.ack_nack_mode);
  }
}

void handle_uci_sr_pdu(PHY_VARS_eNB *eNB,
                       int UE_id,
                       nfapi_ul_config_request_pdu_t *ul_config_pdu,
                       uint16_t frame,
                       uint8_t subframe,
                       uint8_t srs_active)
{
  LTE_eNB_UCI *uci = &eNB->uci_vars[UE_id];

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  uci->frame               = frame;
  uci->subframe            = subframe;
  uci->rnti                = ul_config_pdu->uci_sr_pdu.ue_information.ue_information_rel8.rnti;
  uci->type                = SR;
  uci->pucch_fmt           = pucch_format1;
  uci->num_antenna_ports   = 1;
  uci->num_pucch_resources = 1;
  uci->n_pucch_1_0_sr[0]   = ul_config_pdu->uci_sr_pdu.sr_information.sr_information_rel8.pucch_index;
  uci->srs_active          = srs_active;
  uci->active              = 1;
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  uci->ue_type                     = ul_config_pdu->uci_sr_pdu.ue_information.ue_information_rel13.ue_type;
  uci->empty_symbols               = ul_config_pdu->uci_sr_pdu.ue_information.ue_information_rel13.empty_symbols;
  uci->total_repetitions = ul_config_pdu->uci_sr_pdu.ue_information.ue_information_rel13.total_number_of_repetitions;
#endif
  LOG_D(PHY,"Programming UCI SR rnti %x, pucch1_0 %d for (%d,%d)\n",
        uci->rnti,
        uci->n_pucch_1_0_sr[0],
        frame,
        subframe);
}

void handle_uci_sr_harq_pdu(PHY_VARS_eNB *eNB,int UE_id,nfapi_ul_config_request_pdu_t *ul_config_pdu,uint16_t frame,uint8_t subframe,uint8_t srs_active) {
  LTE_eNB_UCI *uci = &eNB->uci_vars[UE_id];

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  uci->frame               = frame;
  uci->subframe            = subframe;
  uci->rnti                = ul_config_pdu->uci_sr_harq_pdu.ue_information.ue_information_rel8.rnti;
  uci->type                = HARQ_SR;
  uci->num_antenna_ports   = 1;
  uci->num_pucch_resources = 1;
  uci->n_pucch_1_0_sr[0]   = ul_config_pdu->uci_sr_harq_pdu.sr_information.sr_information_rel8.pucch_index;
  uci->srs_active          = srs_active;
  uci->active              = 1;
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  uci->ue_type                     = ul_config_pdu->uci_sr_harq_pdu.ue_information.ue_information_rel13.ue_type;
  uci->empty_symbols               = ul_config_pdu->uci_sr_harq_pdu.ue_information.ue_information_rel13.empty_symbols;
  uci->total_repetitions = ul_config_pdu->uci_sr_harq_pdu.ue_information.ue_information_rel13.total_number_of_repetitions;
#endif
  handle_uci_harq_information(eNB,uci,&ul_config_pdu->uci_sr_harq_pdu.harq_information);
}

void handle_uci_harq_pdu(PHY_VARS_eNB *eNB,int UE_id,nfapi_ul_config_request_pdu_t *ul_config_pdu,uint16_t frame,uint8_t subframe,uint8_t srs_active) {
  LTE_eNB_UCI *uci = &eNB->uci_vars[UE_id];

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  LOG_D(PHY,"Frame %d, Subframe %d: Programming UCI_HARQ process (type %d)\n",frame,subframe,HARQ);
  uci->frame             = frame;
  uci->subframe          = subframe;
  uci->rnti              = ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel8.rnti;
  uci->type              = HARQ;
  uci->srs_active        = srs_active;
  uci->num_antenna_ports = ul_config_pdu->uci_harq_pdu.harq_information.harq_information_rel11.num_ant_ports;
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  uci->ue_type                     = ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.ue_type;
  uci->empty_symbols               = ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.empty_symbols;
  uci->total_repetitions           = ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.total_number_of_repetitions;
#endif
  handle_uci_harq_information(eNB,uci,&ul_config_pdu->uci_harq_pdu.harq_information);
  uci->active=1;
}

void handle_srs_pdu(PHY_VARS_eNB *eNB,nfapi_ul_config_request_pdu_t *ul_config_pdu,uint16_t frame,uint8_t subframe) {
  int i;

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    if (eNB->soundingrs_ul_config_dedicated[i].active==1) continue;

    eNB->soundingrs_ul_config_dedicated[i].active               = 1;
    eNB->soundingrs_ul_config_dedicated[i].frame                = frame;
    eNB->soundingrs_ul_config_dedicated[i].subframe             = subframe;
    eNB->soundingrs_ul_config_dedicated[i].rnti                 = ul_config_pdu->srs_pdu.srs_pdu_rel8.rnti;
    eNB->soundingrs_ul_config_dedicated[i].srs_Bandwidth        = ul_config_pdu->srs_pdu.srs_pdu_rel8.srs_bandwidth;
    eNB->soundingrs_ul_config_dedicated[i].srs_HoppingBandwidth = ul_config_pdu->srs_pdu.srs_pdu_rel8.srs_hopping_bandwidth;
    eNB->soundingrs_ul_config_dedicated[i].freqDomainPosition   = ul_config_pdu->srs_pdu.srs_pdu_rel8.frequency_domain_position;
    eNB->soundingrs_ul_config_dedicated[i].transmissionComb     = ul_config_pdu->srs_pdu.srs_pdu_rel8.transmission_comb;
    eNB->soundingrs_ul_config_dedicated[i].srs_ConfigIndex      = ul_config_pdu->srs_pdu.srs_pdu_rel8.i_srs;
    eNB->soundingrs_ul_config_dedicated[i].cyclicShift          = ul_config_pdu->srs_pdu.srs_pdu_rel8.sounding_reference_cyclic_shift;
    break;
  }

  AssertFatal(i<NUMBER_OF_UE_MAX,"No room for SRS processing\n");
}

void handle_nfapi_ul_pdu(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc,
                         nfapi_ul_config_request_pdu_t *ul_config_pdu,
                         uint16_t frame,uint8_t subframe,uint8_t srs_present) {
  nfapi_ul_config_ulsch_pdu_rel8_t *rel8 = &ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8;
  int16_t UE_id;

  if (NFAPI_MODE==NFAPI_MODE_VNF) return;

  // check if we have received a dci for this ue and ulsch descriptor is configured

  if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_PDU_TYPE) {
    //if (UE_id == find_ulsch(ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.rnti,eNB,SEARCH_EXIST_OR_FREE)<0)
    //for (int i=0;i<16;i++) if (eNB->ulsch[i]->harq_mask>0) LOG_I(PHY,"rnti %x, mask %x\n",eNB->ulsch[i]->rnti,eNB->ulsch[i]->harq_mask >0);
    AssertFatal((UE_id = find_ulsch(ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.rnti,eNB,SEARCH_EXIST_OR_FREE))>=0,
                "No existing UE ULSCH for rnti %x\n",rel8->rnti);
    LOG_D(PHY,"Applying UL config for UE %d, rnti %x for frame %d, subframe %d, modulation %d, rvidx %d, first_rb %d, nb_rb %d\n", UE_id,rel8->rnti,frame,subframe,rel8->modulation_type,
          rel8->redundancy_version,
          rel8->resource_block_start,rel8->number_of_resource_blocks);
    fill_ulsch(eNB,UE_id,&ul_config_pdu->ulsch_pdu,frame,subframe);
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE) {
    AssertFatal((UE_id = find_ulsch(ul_config_pdu->ulsch_harq_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti,eNB,SEARCH_EXIST_OR_FREE))>=0,
                "No available UE ULSCH for rnti %x\n",ul_config_pdu->ulsch_harq_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti);
    fill_ulsch(eNB,UE_id,&ul_config_pdu->ulsch_harq_pdu.ulsch_pdu,frame,subframe);
    handle_ulsch_harq_pdu(eNB, UE_id, ul_config_pdu,
                          &ul_config_pdu->ulsch_harq_pdu.harq_information, frame, subframe);
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE) {
    AssertFatal((UE_id = find_ulsch(ul_config_pdu->ulsch_cqi_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti,
                                    eNB,SEARCH_EXIST_OR_FREE))>=0,
                "No available UE ULSCH for rnti %x\n",ul_config_pdu->ulsch_cqi_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti);
    fill_ulsch(eNB,UE_id,&ul_config_pdu->ulsch_cqi_ri_pdu.ulsch_pdu,frame,subframe);
    handle_ulsch_cqi_ri_pdu(eNB,UE_id,ul_config_pdu,frame,subframe);
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE) {
    AssertFatal((UE_id = find_ulsch(ul_config_pdu->ulsch_cqi_harq_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti,
                                    eNB,SEARCH_EXIST_OR_FREE))>=0,
                "No available UE ULSCH for rnti %x\n",ul_config_pdu->ulsch_cqi_harq_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti);
    fill_ulsch(eNB,UE_id,&ul_config_pdu->ulsch_cqi_harq_ri_pdu.ulsch_pdu,frame,subframe);
    handle_ulsch_cqi_harq_ri_pdu(eNB,UE_id,ul_config_pdu,frame,subframe);
    handle_ulsch_harq_pdu(eNB, UE_id, ul_config_pdu,
                          &ul_config_pdu->ulsch_cqi_harq_ri_pdu.harq_information, frame, subframe);
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE) {
    AssertFatal((UE_id = find_uci(ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel8.rnti,
                                  proc->frame_tx,proc->subframe_tx,eNB,SEARCH_EXIST_OR_FREE))>=0,
                "No available UE UCI for rnti %x\n",ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel8.rnti);
    LOG_D(PHY,"Applying UL UCI_HARQ config for UE %d, rnti %x for frame %d, subframe %d\n",
          UE_id,ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel8.rnti,frame,subframe);
    handle_uci_harq_pdu(eNB,UE_id,ul_config_pdu,frame,subframe,srs_present);
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE) {
    AssertFatal(1==0,"NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE not handled yet\n");
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_CQI_HARQ_PDU_TYPE) {
    AssertFatal(1==0,"NFAPI_UL_CONFIG_UCI_CQI_HARQ_PDU_TYPE not handled yet\n");
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_CQI_SR_PDU_TYPE) {
    AssertFatal(1==0,"NFAPI_UL_CONFIG_UCI_CQI_SR_PDU_TYPE not handled yet\n");
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE) {
    AssertFatal((UE_id = find_uci(ul_config_pdu->uci_sr_pdu.ue_information.ue_information_rel8.rnti,
                                  proc->frame_tx,proc->subframe_tx,eNB,SEARCH_EXIST_OR_FREE))>=0,
                "No available UE UCI for rnti %x\n",ul_config_pdu->uci_sr_pdu.ue_information.ue_information_rel8.rnti);
    handle_uci_sr_pdu(eNB,UE_id,ul_config_pdu,frame,subframe,srs_present);
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE) {
    AssertFatal((UE_id = find_uci(rel8->rnti,proc->frame_tx,proc->subframe_tx,eNB,SEARCH_EXIST_OR_FREE))>=0,
                "No available UE UCI for rnti %x\n",ul_config_pdu->uci_sr_harq_pdu.ue_information.ue_information_rel8.rnti);
    handle_uci_sr_harq_pdu(eNB,UE_id,ul_config_pdu,frame,subframe,srs_present);
  } else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_SRS_PDU_TYPE) {
    handle_srs_pdu(eNB,ul_config_pdu,frame,subframe);
  }
}

void schedule_response(Sched_Rsp_t *Sched_INFO) {
  PHY_VARS_eNB *eNB;
  L1_rxtx_proc_t *proc;
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
  uint8_t                   ul_subframe;
  int                       ul_frame;
  int                       harq_pid;
  LTE_UL_eNB_HARQ_t         *ulsch_harq;
  AssertFatal(RC.eNB!=NULL,"RC.eNB is null\n");
  AssertFatal(RC.eNB[Mod_id]!=NULL,"RC.eNB[%d] is null\n",Mod_id);
  AssertFatal(RC.eNB[Mod_id][CC_id]!=NULL,"RC.eNB[%d][%d] is null\n",Mod_id,CC_id);
  eNB         = RC.eNB[Mod_id][CC_id];
  fp          = &eNB->frame_parms;
  proc        = &eNB->proc.L1_proc;
  /* TODO: check that following line is correct - in the meantime it is disabled */
  //if ((fp->frame_type == TDD) && (subframe_select(fp,subframe)==SF_UL)) return;
  ul_subframe = pdcch_alloc2ul_subframe(fp,subframe);
  ul_frame    = pdcch_alloc2ul_frame(fp,frame,subframe);
  // DJP - subframe assert will fail - not sure why yet
  // DJP - AssertFatal(proc->subframe_tx == subframe, "Current subframe %d != NFAPI subframe %d\n",proc->subframe_tx,subframe);
  // DJP - AssertFatal(proc->subframe_tx == subframe, "Current frame %d != NFAPI frame %d\n",proc->frame_tx,frame);
  uint8_t number_pdcch_ofdm_symbols = DL_req->dl_config_request_body.number_pdcch_ofdm_symbols;
  uint8_t number_dl_pdu             = DL_req->dl_config_request_body.number_pdu;
  uint8_t number_hi_dci0_pdu        = HI_DCI0_req->hi_dci0_request_body.number_of_dci+HI_DCI0_req->hi_dci0_request_body.number_of_hi;
  uint8_t number_ul_pdu             = UL_req!=NULL ? UL_req->ul_config_request_body.number_of_pdus : 0;
  nfapi_dl_config_request_pdu_t *dl_config_pdu;
  nfapi_hi_dci0_request_pdu_t   *hi_dci0_req_pdu;
  nfapi_ul_config_request_pdu_t *ul_config_pdu;
  int i;
  eNB->pdcch_vars[subframe&1].num_pdcch_symbols = number_pdcch_ofdm_symbols;
  eNB->pdcch_vars[subframe&1].num_dci           = 0;
  eNB->phich_vars[subframe&1].num_hi            = 0;
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  eNB->mpdcch_vars[subframe&1].num_dci           = 0;
#endif
  LOG_D(PHY,"NFAPI: Sched_INFO:SFN/SF:%04d%d DL_req:SFN/SF:%04d%d:dl_pdu:%d tx_req:SFN/SF:%04d%d:pdus:%d\n",
        frame,subframe,
        NFAPI_SFNSF2SFN(DL_req->sfn_sf),NFAPI_SFNSF2SF(DL_req->sfn_sf),number_dl_pdu,
        NFAPI_SFNSF2SFN(TX_req->sfn_sf),NFAPI_SFNSF2SF(TX_req->sfn_sf),TX_req->tx_request_body.number_of_pdus
       );
  LOG_D(PHY,"NFAPI: hi_dci0:SFN/SF:%04d%d:pdus:%d\n",
        NFAPI_SFNSF2SFN(HI_DCI0_req->sfn_sf),NFAPI_SFNSF2SF(HI_DCI0_req->sfn_sf),number_hi_dci0_pdu
       );

  if(UL_req!=NULL)
    LOG_D(PHY,"NFAPI: ul_cfg:SFN/SF:%04d%d:pdus:%d num_pdcch_symbols:%d\n",
          NFAPI_SFNSF2SFN(UL_req->sfn_sf),NFAPI_SFNSF2SF(UL_req->sfn_sf),number_ul_pdu,
          eNB->pdcch_vars[subframe&1].num_pdcch_symbols);

  int do_oai =0;
  int dont_send =0;

  /* TODO: check the following test - in the meantime it is put back as it was before */
  //if ((ul_subframe<10)&&
  //    (subframe_select(fp,ul_subframe)==SF_UL)) { // This means that there is an ul_subframe that can be configured here
  if (ul_subframe<10) { // This means that there is an ul_subframe that can be configured here
    LOG_D(PHY,"NFAPI: Clearing dci allocations for potential UL subframe %d\n",ul_subframe);
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

  for (i=0; i<number_dl_pdu; i++) {
    dl_config_pdu = &DL_req->dl_config_request_body.dl_config_pdu_list[i];

    //LOG_D(PHY,"NFAPI: dl_pdu %d : type %d\n",i,dl_config_pdu->pdu_type);
    switch (dl_config_pdu->pdu_type) {
      case NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE:
        handle_nfapi_dci_dl_pdu(eNB,NFAPI_SFNSF2SFN(DL_req->sfn_sf),NFAPI_SFNSF2SF(DL_req->sfn_sf),proc,dl_config_pdu);
        eNB->pdcch_vars[NFAPI_SFNSF2SF(DL_req->sfn_sf)&1].num_dci++;
        //LOG_E(PHY,"Incremented num_dci:%d but already set??? dl_config:num_dci:%d\n", eNB->pdcch_vars[subframe&1].num_dci, number_dci);
        do_oai=1;
        break;

      case NFAPI_DL_CONFIG_BCH_PDU_TYPE:
        AssertFatal(dl_config_pdu->bch_pdu.bch_pdu_rel8.pdu_index<TX_req->tx_request_body.number_of_pdus,
                    "bch_pdu_rel8.pdu_index>=TX_req->number_of_pdus (%d>%d)\n",
                    dl_config_pdu->bch_pdu.bch_pdu_rel8.pdu_index,
                    TX_req->tx_request_body.number_of_pdus);
        eNB->pbch_configured=1;
        do_oai=1;
        //LOG_D(PHY,"%s() NFAPI_DL_CONFIG_BCH_PDU_TYPE TX:%d/%d RX:%d/%d TXREQ:%d/%d\n",
        //__FUNCTION__, proc->frame_tx, proc->subframe_tx, proc->frame_rx, proc->subframe_rx, NFAPI_SFNSF2SFN(TX_req->sfn_sf), NFAPI_SFNSF2SF(TX_req->sfn_sf));
        handle_nfapi_bch_pdu(eNB,proc,dl_config_pdu,
                             TX_req->tx_request_body.tx_pdu_list[dl_config_pdu->bch_pdu.bch_pdu_rel8.pdu_index].segments[0].segment_data);
        break;

      case NFAPI_DL_CONFIG_MCH_PDU_TYPE:
        //      handle_nfapi_mch_dl_pdu(eNB,dl_config_pdu);
        break;

      case NFAPI_DL_CONFIG_DLSCH_PDU_TYPE: {
        nfapi_dl_config_dlsch_pdu_rel8_t *dlsch_pdu_rel8 = &dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8;
        uint16_t pdu_index = dlsch_pdu_rel8->pdu_index;
        uint16_t tx_pdus = TX_req->tx_request_body.number_of_pdus;
        uint16_t invalid_pdu = pdu_index == -1;
        uint8_t *sdu = invalid_pdu ? NULL : pdu_index >= tx_pdus ? NULL : TX_req->tx_request_body.tx_pdu_list[pdu_index].segments[0].segment_data;
        LOG_D(PHY,"%s() [PDU:%d] NFAPI_DL_CONFIG_DLSCH_PDU_TYPE SFN/SF:%04d%d TX:%d/%d RX:%d/%d transport_blocks:%d pdu_index:%d sdu:%p\n",
              __FUNCTION__, i,
              NFAPI_SFNSF2SFN(DL_req->sfn_sf),NFAPI_SFNSF2SF(DL_req->sfn_sf),
              proc->frame_tx, proc->subframe_tx,
              proc->frame_rx, proc->subframe_rx,
              dlsch_pdu_rel8->transport_blocks, pdu_index, sdu);
        /*
        AssertFatal(dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index<TX_req->tx_request_body.number_of_pdus,
                    "dlsch_pdu_rel8.pdu_index>=TX_req->number_of_pdus (%d>%d)\n",
                    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index,
                    TX_req->tx_request_body.number_of_pdus);
        */
        AssertFatal((dlsch_pdu_rel8->transport_blocks<3) &&
                    (dlsch_pdu_rel8->transport_blocks>0),
                    "dlsch_pdu_rel8->transport_blocks = %d not in [1,2]\n",
                    dlsch_pdu_rel8->transport_blocks);

        if (1) { //sdu != NULL)
          handle_nfapi_dlsch_pdu(eNB,NFAPI_SFNSF2SFN(DL_req->sfn_sf),NFAPI_SFNSF2SF(DL_req->sfn_sf),proc,dl_config_pdu, dlsch_pdu_rel8->transport_blocks-1, sdu);
        } else {
          dont_send=1;
          LOG_E(MAC,"%s() NFAPI_DL_CONFIG_DLSCH_PDU_TYPE sdu is NULL DL_CFG:SFN/SF:%d:pdu_index:%d TX_REQ:SFN/SF:%d:pdus:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(DL_req->sfn_sf), pdu_index,
                NFAPI_SFNSF2DEC(TX_req->sfn_sf), tx_pdus);
        }

        // Send the data first so that the DL_CONFIG can just pluck it out of the buffer
        // DJP - OAI was here - moved to bottom
        do_oai=1;
        /*
        if (dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti == eNB->preamble_list[0].preamble_rel8.rnti) {// is RAR pdu

          LOG_D(PHY,"Frame %d, Subframe %d: Received LTE RAR pdu, programming based on UL Grant\n",frame,subframe);
          generate_eNB_ulsch_params_from_rar(eNB,
                                             TX_req->tx_request_body.tx_pdu_list[dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index].segments[0].segment_data,
                                             frame,
                                             subframe);

                                             }        */
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
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))

      case NFAPI_DL_CONFIG_MPDCCH_PDU_TYPE:
        handle_nfapi_mpdcch_pdu(eNB,proc,dl_config_pdu);
        eNB->mpdcch_vars[subframe&1].num_dci++;
        break;
#endif
    }
  }

  if ((NFAPI_MODE!=NFAPI_MONOLITHIC) && do_oai && !dont_send) {
    oai_nfapi_tx_req(Sched_INFO->TX_req);
    oai_nfapi_dl_config_req(Sched_INFO->DL_req); // DJP - .dl_config_request_body.dl_config_pdu_list[0]); // DJP - FIXME TODO - yuk - only copes with 1 pdu
  }

  if ((NFAPI_MODE!=NFAPI_MONOLITHIC) && number_hi_dci0_pdu!=0) {
    oai_nfapi_hi_dci0_req(HI_DCI0_req);
    eNB->pdcch_vars[NFAPI_SFNSF2SF(HI_DCI0_req->sfn_sf)&1].num_dci=0;
    eNB->pdcch_vars[NFAPI_SFNSF2SF(HI_DCI0_req->sfn_sf)&1].num_pdcch_symbols=0;
  }

  for (i=0; i<number_hi_dci0_pdu; i++) {
    hi_dci0_req_pdu = &HI_DCI0_req->hi_dci0_request_body.hi_dci0_pdu_list[i];
    LOG_D(PHY,"NFAPI: hi_dci0_pdu %d : type %d\n",i,hi_dci0_req_pdu->pdu_type);

    switch (hi_dci0_req_pdu->pdu_type) {
      case NFAPI_HI_DCI0_DCI_PDU_TYPE:
        handle_nfapi_hi_dci0_dci_pdu(eNB,NFAPI_SFNSF2SFN(HI_DCI0_req->sfn_sf),NFAPI_SFNSF2SF(HI_DCI0_req->sfn_sf),proc,hi_dci0_req_pdu);
        eNB->pdcch_vars[NFAPI_SFNSF2SF(HI_DCI0_req->sfn_sf)&1].num_dci++;
        break;

      case NFAPI_HI_DCI0_MPDCCH_DCI_PDU_TYPE:
        handle_nfapi_hi_dci0_mpdcch_dci_pdu(eNB,proc,hi_dci0_req_pdu);
        eNB->mpdcch_vars[subframe&1].num_dci++;
        break;

      case NFAPI_HI_DCI0_HI_PDU_TYPE:
        handle_nfapi_hi_dci0_hi_pdu(eNB,NFAPI_SFNSF2SFN(HI_DCI0_req->sfn_sf),NFAPI_SFNSF2SF(HI_DCI0_req->sfn_sf),proc,hi_dci0_req_pdu);
        break;
    }
  }

  if (NFAPI_MODE!=NFAPI_MONOLITHIC) {
    if (number_ul_pdu>0) {
      //LOG_D(PHY, "UL_CONFIG to send to PNF\n");
      oai_nfapi_ul_config_req(UL_req);
      UL_req->ul_config_request_body.number_of_pdus=0;
      number_ul_pdu=0;
    }
  } else {
    for (i=0; i<number_ul_pdu; i++) {
      ul_config_pdu = &UL_req->ul_config_request_body.ul_config_pdu_list[i];
      LOG_D(PHY,"NFAPI: ul_pdu %d : type %d\n",i,ul_config_pdu->pdu_type);
      AssertFatal(ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_PDU_TYPE ||
                  ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE ||
                  ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE ||
                  ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE ||
                  ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE ||
                  ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE ||
                  ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE
                  ,
                  "Optional UL_PDU type %d not supported\n",ul_config_pdu->pdu_type);
      handle_nfapi_ul_pdu(eNB,proc,ul_config_pdu,UL_req->sfn_sf>>4,UL_req->sfn_sf&0xf,UL_req->ul_config_request_body.srs_present);
    }
  }
}

/*Dummy functions*/

int memcpy_dl_config_req (nfapi_pnf_p7_config_t *pnf_p7, nfapi_dl_config_request_t *req) {
  return 0;
}

int memcpy_ul_config_req (nfapi_pnf_p7_config_t *pnf_p7, nfapi_ul_config_request_t *req) {
  return 0;
}

int memcpy_hi_dci0_req (nfapi_pnf_p7_config_t *pnf_p7, nfapi_hi_dci0_request_t *req) {
  return 0;
}

int memcpy_tx_req (nfapi_pnf_p7_config_t *pnf_p7, nfapi_tx_request_t *req) {
  return 0;
}
