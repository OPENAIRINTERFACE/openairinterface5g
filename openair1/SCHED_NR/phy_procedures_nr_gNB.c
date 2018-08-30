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
  nr_numerology_index_e ssb_type;

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
  int start_rb = cfg->sch_config.n_ssb_crb.value / pow(2,cfg->subframe_config.numerology_index_mu.value);
  fp->ssb_start_subcarrier = 12 * start_rb + cfg->sch_config.ssb_subcarrier_offset.value;
  LOG_D(PHY, "SSB first subcarrier %d\n", fp->ssb_start_subcarrier);
}

void nr_common_signal_procedures (PHY_VARS_gNB *gNB,int frame, int subframe) {

  NR_DL_FRAME_PARMS *fp=&gNB->frame_parms;
  nfapi_nr_config_request_t *cfg = &gNB->gNB_config;
  int **txdataF = gNB->common_vars.txdataF;
  uint8_t *pbch_pdu=&gNB->pbch_pdu[0];
  int ss_subframe = (cfg->sch_config.half_frame_index.value)? 5 : 0;
  uint8_t Lmax, ssb_index=0, n_hf=0;

  LOG_D(PHY,"common_signal_procedures: frame %d, subframe %d\n",frame,subframe);

  int ssb_start_symbol = nr_get_ssb_start_symbol(cfg, fp);
  nr_set_ssb_first_subcarrier(cfg, fp);
  Lmax = (fp->dl_CarrierFreq < 3e9)? 4:8;


  if (subframe == ss_subframe)
  {
    // Current implementation is based on SSB in first half frame, first candidate
    LOG_I(PHY,"SS TX: frame %d, subframe %d, start_symbol %d\n",frame,subframe, ssb_start_symbol);

    nr_generate_pss(gNB->d_pss, txdataF, AMP, ssb_start_symbol, cfg, fp);
    nr_generate_sss(gNB->d_sss, txdataF, AMP_OVER_2, ssb_start_symbol, cfg, fp);

    /*if ((frame_mod8) == 0){
      if (gNB->pbch_configured != 1)return;
      gNB->pbch_configured = 0;
    }*/
    nr_generate_pbch_dmrs(gNB->nr_gold_pbch_dmrs[n_hf][ssb_index],txdataF, AMP_OVER_2, ssb_start_symbol, cfg, fp);
    nr_generate_pbch(&gNB->pbch, pbch_pdu, txdataF, AMP_OVER_2, ssb_start_symbol, n_hf, Lmax, ssb_index, frame, cfg, fp);
  }

}

void phy_procedures_gNB_TX(PHY_VARS_gNB *gNB,
			   gNB_rxtx_proc_t *proc,
			   int do_meas)
{
  int aa;
  int frame=proc->frame_tx;
  int subframe=proc->subframe_tx;

  NR_DL_FRAME_PARMS *fp=&gNB->frame_parms;
  nfapi_nr_config_request_t *cfg = &gNB->gNB_config;

  int offset = gNB->CC_id;

  if ((cfg->subframe_config.duplex_mode.value == TDD) && (nr_subframe_select(cfg,subframe)==SF_UL)) return;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_TX+offset,1);
  if (do_meas==1) start_meas(&gNB->phy_proc_tx);

  // clear the transmit data array for the current subframe
  for (aa=0; aa<cfg->rf_config.tx_antenna_ports.value; aa++) {      
    memset(&gNB->common_vars.txdataF[aa][subframe*fp->samples_per_subframe_wCP],
	   0,fp->samples_per_subframe_wCP*sizeof(int32_t));
  }

  if (nfapi_mode == 0 || nfapi_mode == 1) {
    nr_common_signal_procedures(gNB,frame, subframe);
    //if (frame == 9)
      //write_output("txdataF.m","txdataF",gNB->common_vars.txdataF[aa],fp->samples_per_frame_wCP, 1, 1);
  }
}
