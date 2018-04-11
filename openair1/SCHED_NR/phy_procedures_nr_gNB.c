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

#include "PHY/extern.h"
#include "SCHED/defs.h"
#include "SCHED/extern.h"
#include "nfapi_interface.h"
#include "SCHED/fapi_l1.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

#include "T.h"

#include "assertions.h"
#include "msc.h"

#include <time.h>

#if defined(ENABLE_ITTI)
#   include "intertask_interface.h"
#endif

extern uint8_t nfapi_mode;

void nr_common_signal_procedures (PHY_VARS_gNB *gNB,int frame, int subframe) {

  NR_DL_FRAME_PARMS *fp=&gNB->frame_parms;
  nfapi_config_request_t *cfg = &gNB->gNB_config;
  int **txdataF = gNB->common_vars.txdataF;
  uint8_t *pbch_pdu=&gNB->pbch_pdu[0];

  LOG_D(PHY,"common_signal_procedures: frame %d, subframe %d\n",frame,subframe);

  int ssb_start_symbol = nr_get_ssb_start_symbol(cfg, fp);
  //nr_set_ssb_first_subcarrier(cfg);

  if (subframe == (cfg->sch_config.half_frame_index)? 0:5)
  {
    nr_generate_pss(gNB->d_pss, txdataF, AMP, ssb_start_symbol, cfg, fp);
    nr_generate_sss(gNB->d_sss, txdataF, AMP_OVER_2, ssb_start_symbol, cfg, fp);
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
  nfapi_config_request_t *cfg = &gNB->gNB_config;

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
    common_signal_procedures(gNB,frame, subframe);
  }
}
