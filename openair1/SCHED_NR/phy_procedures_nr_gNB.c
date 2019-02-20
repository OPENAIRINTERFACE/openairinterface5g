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

#include "PHY/phy_extern.h"
#include "PHY/defs_gNB.h"
#include "sched_nr.h"
#include "PHY/NR_TRANSPORT/nr_transport.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "SCHED/sched_eNB.h"
#include "SCHED/sched_common_extern.h"
#include "nfapi_interface.h"
#include "SCHED/fapi_l1.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "T.h"

#include "assertions.h"
#include "msc.h"

#include <time.h>

#if defined(ENABLE_ITTI)
#   include "intertask_interface.h"
#endif

extern uint8_t nfapi_mode;
/*
int return_ssb_type(nfapi_config_request_t *cfg)
{
  int mu = cfg->subframe_config.numerology_index_mu.value;
  nr_ssb_type_e ssb_type;

  switch(mu) {

  case NR_MU_0:
    ssb_type = nr_ssb_type_A;
    break;

  case NR_MU_1:
    ssb_type = nr_ssb_type_B;
    break;

  case NR_MU_3:
    ssb_type = nr_ssb_type_D;
    break;

  case NR_MU_4:
    ssb_type = nr_ssb_type_E;
    break;

  default:
    AssertFatal(0==1, "Invalid numerology index %d for the synchronization block\n", mu);
  }

  LOG_D(PHY, "SSB type %d\n", ssb_type);
  return ssb_type;

}*/

// First SSB starting symbol candidate is used and type B is chosen for 30kHz SCS
int nr_get_ssb_start_symbol(nfapi_nr_config_request_t *cfg, NR_DL_FRAME_PARMS *fp)
{
  int mu = cfg->subframe_config.numerology_index_mu.value;
  int symbol = 0;

  switch(mu) {

  case NR_MU_0:
    symbol = 2;
    break;

  case NR_MU_1: // case B
    symbol = 4;
    break;

  case NR_MU_3:
    symbol = 4;
    break;

  case NR_MU_4:
    symbol = 8;
    break;

  default:
    AssertFatal(0==1, "Invalid numerology index %d for the synchronization block\n", mu);
  }

  if (cfg->sch_config.half_frame_index.value)
    symbol += (5 * fp->symbols_per_slot * fp->slots_per_subframe);

  return symbol;
}

void nr_set_ssb_first_subcarrier(nfapi_nr_config_request_t *cfg, NR_DL_FRAME_PARMS *fp)
{
  fp->ssb_start_subcarrier = (12 * cfg->sch_config.n_ssb_crb.value + cfg->sch_config.ssb_subcarrier_offset.value)/(1<<cfg->subframe_config.numerology_index_mu.value);
  LOG_D(PHY, "SSB first subcarrier %d (%d,%d)\n", fp->ssb_start_subcarrier,cfg->sch_config.n_ssb_crb.value,cfg->sch_config.ssb_subcarrier_offset.value);
}

void nr_common_signal_procedures (PHY_VARS_gNB *gNB,int frame, int slot) {

  NR_DL_FRAME_PARMS *fp=&gNB->frame_parms;
  nfapi_nr_config_request_t *cfg = &gNB->gNB_config;
  int **txdataF = gNB->common_vars.txdataF;
  uint8_t *pbch_pdu=&gNB->pbch_pdu[0];
  int ss_slot = (cfg->sch_config.half_frame_index.value)? 10 : 0;
  uint8_t Lmax, ssb_index=0, n_hf=0;

  LOG_D(PHY,"common_signal_procedures: frame %d, slot %d\n",frame,slot);

  int ssb_start_symbol = nr_get_ssb_start_symbol(cfg, fp);
  nr_set_ssb_first_subcarrier(cfg, fp);
  Lmax = (fp->dl_CarrierFreq < 3e9)? 4:8;


  if (slot == ss_slot)
  {
    // Current implementation is based on SSB in first half frame, first candidate
    LOG_D(PHY,"SS TX: frame %d, slot %d, start_symbol %d\n",frame,slot, ssb_start_symbol);

    nr_generate_pss(gNB->d_pss, txdataF[0], AMP, ssb_start_symbol, cfg, fp);
    nr_generate_sss(gNB->d_sss, txdataF[0], AMP, ssb_start_symbol, cfg, fp);

    if (!(frame&7)){
      LOG_D(PHY,"%d.%d : pbch_configured %d\n",frame,slot,gNB->pbch_configured);
      if (gNB->pbch_configured != 1)return;
      gNB->pbch_configured = 0;
    }

    nr_generate_pbch_dmrs(gNB->nr_gold_pbch_dmrs[n_hf][ssb_index],txdataF[0], AMP, ssb_start_symbol, cfg, fp);

    nr_generate_pbch(&gNB->pbch,
                      gNB->nrPolar_params,
                      pbch_pdu,
                      gNB->nr_pbch_interleaver,
                      txdataF[0],
                      AMP,
                      ssb_start_symbol,
                      n_hf,Lmax,ssb_index,
                      frame, cfg, fp);
  }

}

void phy_procedures_gNB_TX(PHY_VARS_gNB *gNB,
			   gNB_L1_rxtx_proc_t *proc,
			   int do_meas)
{
  int aa;
  int frame=proc->frame_tx;

  int slot=proc->slot_tx;
  uint8_t num_dci=0,num_pdsch_rnti;

  NR_DL_FRAME_PARMS *fp=&gNB->frame_parms;
  nfapi_nr_config_request_t *cfg = &gNB->gNB_config;

  int offset = gNB->CC_id;

  if ((cfg->subframe_config.duplex_mode.value == TDD) && (nr_slot_select(cfg,slot)==SF_UL)) return;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_TX+offset,1);
  if (do_meas==1) start_meas(&gNB->phy_proc_tx);

  // clear the transmit data array for the current subframe
  for (aa=0; aa<1/*15*/; aa++) {      
    memset(gNB->common_vars.txdataF[aa],0,fp->samples_per_slot_wCP*sizeof(int32_t));
  }

  if (nfapi_mode == 0 || nfapi_mode == 1) {
    nr_common_signal_procedures(gNB,frame, slot);
    //if (frame == 9)
      //write_output("txdataF.m","txdataF",gNB->common_vars.txdataF[aa],fp->samples_per_frame_wCP, 1, 1);
  }

  num_dci = gNB->pdcch_vars.num_dci;

  num_pdsch_rnti = gNB->pdcch_vars.num_pdsch_rnti;
  if (num_dci) {
    LOG_I(PHY, "[gNB %d] Frame %d slot %d \
    Calling nr_generate_dci_top (number of DCI %d)\n", gNB->Mod_id, frame, slot, num_dci);

    if (nfapi_mode == 0 || nfapi_mode == 1){
      nr_generate_dci_top(gNB->pdcch_vars,
                          &gNB->nrPolar_params,
                          gNB->nr_gold_pdcch_dmrs[slot],
                          gNB->common_vars.txdataF[0],
                          AMP, *fp, *cfg);
      if (num_pdsch_rnti) {
        LOG_I(PHY, "PDSCH generation started (%d)\n", num_pdsch_rnti);
        nr_generate_pdsch(*gNB->dlsch[0][0],
                          gNB->pdcch_vars.dci_alloc[0],
                          gNB->nr_gold_pdsch_dmrs[slot],
                          gNB->common_vars.txdataF,
                          AMP, slot, *fp, *cfg);
      }
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_TX+offset,0);

}
