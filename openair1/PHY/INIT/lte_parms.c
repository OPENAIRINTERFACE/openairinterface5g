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

#include "phy_init.h"
#include "common/utils/LOG/log.h"

uint16_t dl_S_table_normal[10]= {3,9,10,11,12,3,9,10,11,6};
uint16_t dl_S_table_extended[10]= {3,8,9,10,3,8,9,5,0,0};

void set_S_config(LTE_DL_FRAME_PARMS *fp) {
  int X = fp->srsX;
  fp->ul_symbols_in_S_subframe=(1+X);

  if ((fp->Ncp==EXTENDED) && (fp->tdd_config_S>7))
    AssertFatal(1==0,"Illegal S subframe configuration for Extended Prefix mode\n");

  fp->dl_symbols_in_S_subframe = (fp->Ncp==NORMAL)?dl_S_table_normal[fp->tdd_config_S] : dl_S_table_extended[fp->tdd_config_S];
}

int init_frame_parms(LTE_DL_FRAME_PARMS *frame_parms,uint8_t osf) {
  uint8_t log2_osf;
  LOG_I(PHY,"Initializing frame parms for N_RB_DL %d, Ncp %d, osf %d\n",frame_parms->N_RB_DL,frame_parms->Ncp,osf);

  if (frame_parms->Ncp==EXTENDED) {
    frame_parms->nb_prefix_samples0=512;
    frame_parms->nb_prefix_samples = 512;
    frame_parms->symbols_per_tti = 12;
  } else {
    frame_parms->nb_prefix_samples0 = 160;
    frame_parms->nb_prefix_samples = 144;
    frame_parms->symbols_per_tti = 14;
  }

  switch(osf) {
    case 1:
      log2_osf = 0;
      break;

    case 2:
      log2_osf = 1;
      break;

    case 4:
      log2_osf = 2;
      break;

    case 8:
      log2_osf = 3;
      break;

    case 16:
      log2_osf = 4;
      break;

    default:
      AssertFatal(1==0,"Illegal oversampling %d\n",osf);
  }

  switch (frame_parms->N_RB_DL) {
    case 100:
      AssertFatal(osf==1,"Illegal oversampling %d for N_RB_DL %d\n",osf,frame_parms->N_RB_DL);

      if (frame_parms->threequarter_fs) {
        frame_parms->ofdm_symbol_size = 1536;
        frame_parms->samples_per_tti = 23040;
        frame_parms->first_carrier_offset = 1536-600;
        frame_parms->nb_prefix_samples=(frame_parms->nb_prefix_samples*3)>>2;
        frame_parms->nb_prefix_samples0=(frame_parms->nb_prefix_samples0*3)>>2;
      } else {
        frame_parms->ofdm_symbol_size = 2048;
        frame_parms->samples_per_tti = 30720;
        frame_parms->first_carrier_offset = 2048-600;
      }

      frame_parms->N_RBGS = 4;
      frame_parms->N_RBG = 25;
      break;

    case 75:
      AssertFatal(osf==1,"Illegal oversampling %d for N_RB_DL %d\n",osf,frame_parms->N_RB_DL);
      frame_parms->ofdm_symbol_size = 1536;
      frame_parms->samples_per_tti = 23040;
      frame_parms->first_carrier_offset = 1536-450;
      frame_parms->nb_prefix_samples=(frame_parms->nb_prefix_samples*3)>>2;
      frame_parms->nb_prefix_samples0=(frame_parms->nb_prefix_samples0*3)>>2;
      frame_parms->N_RBGS = 4;
      frame_parms->N_RBG = 25;
      break;

    case 50:
      AssertFatal(osf==1,"Illegal oversampling %d for N_RB_DL %d\n",osf,frame_parms->N_RB_DL);
      frame_parms->ofdm_symbol_size = 1024*osf;
      frame_parms->samples_per_tti = 15360*osf;
      frame_parms->first_carrier_offset = frame_parms->ofdm_symbol_size - 300;
      frame_parms->nb_prefix_samples>>=(1-log2_osf);
      frame_parms->nb_prefix_samples0>>=(1-log2_osf);
      frame_parms->N_RBGS = 3;
      frame_parms->N_RBG = 17;
      break;

    case 25:
      AssertFatal(osf<=2,"Illegal oversampling %d for N_RB_DL %d\n",osf,frame_parms->N_RB_DL);
      frame_parms->ofdm_symbol_size = 512*osf;
      frame_parms->samples_per_tti = 7680*osf;
      frame_parms->first_carrier_offset = frame_parms->ofdm_symbol_size - 150;
      frame_parms->nb_prefix_samples>>=(2-log2_osf);
      frame_parms->nb_prefix_samples0>>=(2-log2_osf);
      frame_parms->N_RBGS = 2;
      frame_parms->N_RBG = 13;
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
      frame_parms->ofdm_symbol_size_khz_1dot25     = 6144*osf;
      frame_parms->first_carrier_offset_khz_1dot25 = frame_parms->ofdm_symbol_size_khz_1dot25 - 1800; //4344
      frame_parms->nb_prefix_samples_khz_1dot25>>=(2-log2_osf);
      frame_parms->nb_prefix_samples0_khz_1dot25>>=(2-log2_osf);
#endif
      break;

    case 15:
      frame_parms->ofdm_symbol_size = 256*osf;
      frame_parms->samples_per_tti = 3840*osf;
      frame_parms->first_carrier_offset = frame_parms->ofdm_symbol_size - 90;
      frame_parms->nb_prefix_samples>>=(3-log2_osf);
      frame_parms->nb_prefix_samples0>>=(3-log2_osf);
      frame_parms->N_RBGS = 2;
      frame_parms->N_RBG = 8;
      break;

    case 6:
      frame_parms->ofdm_symbol_size = 128*osf;
      frame_parms->samples_per_tti = 1920*osf;
      frame_parms->first_carrier_offset = frame_parms->ofdm_symbol_size - 36;
      frame_parms->nb_prefix_samples>>=(4-log2_osf);
      frame_parms->nb_prefix_samples0>>=(4-log2_osf);
      frame_parms->N_RBGS = 1;
      frame_parms->N_RBG = 6;
      break;

    default:
      AssertFatal(1==0,"Number of resource blocks (N_RB_DL %d) undefined, frame_parms = %p \n",frame_parms->N_RB_DL, frame_parms);
      break;
  }

  LOG_I(PHY,"lte_parms.c: Setting N_RB_DL to %d, ofdm_symbol_size %d\n",frame_parms->N_RB_DL, frame_parms->ofdm_symbol_size);

  if (frame_parms->frame_type == TDD) set_S_config(frame_parms);

  //  frame_parms->tdd_config=3;
  return(0);
}


void dump_frame_parms(LTE_DL_FRAME_PARMS *frame_parms) {
  LOG_I(PHY,"frame_parms->N_RB_DL=%d\n",frame_parms->N_RB_DL);
  LOG_I(PHY,"frame_parms->N_RB_UL=%d\n",frame_parms->N_RB_UL);
  LOG_I(PHY,"frame_parms->Nid_cell=%d\n",frame_parms->Nid_cell);
  LOG_I(PHY,"frame_parms->Ncp=%d\n",frame_parms->Ncp);
  LOG_I(PHY,"frame_parms->Ncp_UL=%d\n",frame_parms->Ncp_UL);
  LOG_I(PHY,"frame_parms->nushift=%d\n",frame_parms->nushift);
  LOG_I(PHY,"frame_parms->frame_type=%d\n",frame_parms->frame_type);
  LOG_I(PHY,"frame_parms->tdd_config=%d\n",frame_parms->tdd_config);
  LOG_I(PHY,"frame_parms->tdd_config_S=%d\n",frame_parms->tdd_config_S);
  LOG_I(PHY,"frame_parms->nb_antenna_ports_eNB=%d\n",frame_parms->nb_antenna_ports_eNB);
  LOG_I(PHY,"frame_parms->nb_antennas_tx=%d\n",frame_parms->nb_antennas_tx);
  LOG_I(PHY,"frame_parms->nb_antennas_rx=%d\n",frame_parms->nb_antennas_rx);
  LOG_I(PHY,"frame_parms->ofdm_symbol_size=%d\n",frame_parms->ofdm_symbol_size);
  LOG_I(PHY,"frame_parms->nb_prefix_samples=%d\n",frame_parms->nb_prefix_samples);
  LOG_I(PHY,"frame_parms->nb_prefix_samples0=%d\n",frame_parms->nb_prefix_samples0);
  LOG_I(PHY,"frame_parms->first_carrier_offset=%d\n",frame_parms->first_carrier_offset);
  LOG_I(PHY,"frame_parms->samples_per_tti=%d\n",frame_parms->samples_per_tti);
  LOG_I(PHY,"frame_parms->symbols_per_tti=%d\n",frame_parms->symbols_per_tti);
}
