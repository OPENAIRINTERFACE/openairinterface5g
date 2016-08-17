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
#include "defs.h"
#include "log.h"

int init_frame_parms(LTE_DL_FRAME_PARMS *frame_parms,uint8_t osf)
{

  uint8_t log2_osf;

  LOG_I(PHY,"Initializing frame parms for N_RB_DL %d, Ncp %d, osf %d\n",frame_parms->N_RB_DL,frame_parms->Ncp,osf);

  if (frame_parms->Ncp==1) {
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
    printf("Illegal oversampling %d\n",osf);
    return(-1);
  }

  switch (frame_parms->N_RB_DL) {

  case 100:
    if (osf>1) {
      printf("Illegal oversampling %d for N_RB_DL %d\n",osf,frame_parms->N_RB_DL);
      return(-1);
    }

    if (frame_parms->threequarter_fs) {
      frame_parms->ofdm_symbol_size = 1536;
      frame_parms->samples_per_tti = 23040;
      frame_parms->first_carrier_offset = 1536-600;
      frame_parms->nb_prefix_samples=(frame_parms->nb_prefix_samples*3)>>2;
      frame_parms->nb_prefix_samples0=(frame_parms->nb_prefix_samples0*3)>>2;
    }
    else {
      frame_parms->ofdm_symbol_size = 2048;
      frame_parms->samples_per_tti = 30720;
      frame_parms->first_carrier_offset = 2048-600;
    }
    frame_parms->N_RBGS = 4;
    frame_parms->N_RBG = 25;
    break;

  case 75:
    if (osf>1) {
      printf("Illegal oversampling %d for N_RB_DL %d\n",osf,frame_parms->N_RB_DL);
      return(-1);
    }


    frame_parms->ofdm_symbol_size = 1536;
    frame_parms->samples_per_tti = 23040;
    frame_parms->first_carrier_offset = 1536-450;
    frame_parms->nb_prefix_samples=(frame_parms->nb_prefix_samples*3)>>2;
    frame_parms->nb_prefix_samples0=(frame_parms->nb_prefix_samples0*3)>>2;
    frame_parms->N_RBGS = 4;
    frame_parms->N_RBG = 25;
    break;

  case 50:
    if (osf>1) {
      printf("Illegal oversampling %d for N_RB_DL %d\n",osf,frame_parms->N_RB_DL);
      return(-1);
    }

    frame_parms->ofdm_symbol_size = 1024*osf;
    frame_parms->samples_per_tti = 15360*osf;
    frame_parms->first_carrier_offset = frame_parms->ofdm_symbol_size - 300;
    frame_parms->nb_prefix_samples>>=(1-log2_osf);
    frame_parms->nb_prefix_samples0>>=(1-log2_osf);
    frame_parms->N_RBGS = 3;
    frame_parms->N_RBG = 17;
    break;

  case 25:
    if (osf>2) {
      printf("Illegal oversampling %d for N_RB_DL %d\n",osf,frame_parms->N_RB_DL);
      return(-1);
    }

    frame_parms->ofdm_symbol_size = 512*osf;


    frame_parms->samples_per_tti = 7680*osf;
    frame_parms->first_carrier_offset = frame_parms->ofdm_symbol_size - 150;
    frame_parms->nb_prefix_samples>>=(2-log2_osf);
    frame_parms->nb_prefix_samples0>>=(2-log2_osf);
    frame_parms->N_RBGS = 2;
    frame_parms->N_RBG = 13;


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
    printf("init_frame_parms: Error: Number of resource blocks (N_RB_DL %d) undefined, frame_parms = %p \n",frame_parms->N_RB_DL, frame_parms);
    return(-1);
    break;
  }

  printf("lte_parms.c: Setting N_RB_DL to %d, ofdm_symbol_size %d\n",frame_parms->N_RB_DL, frame_parms->ofdm_symbol_size);

  //  frame_parms->tdd_config=3;
  return(0);
}


void dump_frame_parms(LTE_DL_FRAME_PARMS *frame_parms)
{
  printf("frame_parms->N_RB_DL=%d\n",frame_parms->N_RB_DL);
  printf("frame_parms->N_RB_UL=%d\n",frame_parms->N_RB_UL);
  printf("frame_parms->Nid_cell=%d\n",frame_parms->Nid_cell);
  printf("frame_parms->Ncp=%d\n",frame_parms->Ncp);
  printf("frame_parms->Ncp_UL=%d\n",frame_parms->Ncp_UL);
  printf("frame_parms->nushift=%d\n",frame_parms->nushift);
  printf("frame_parms->frame_type=%d\n",frame_parms->frame_type);
  printf("frame_parms->tdd_config=%d\n",frame_parms->tdd_config);
  printf("frame_parms->tdd_config_S=%d\n",frame_parms->tdd_config_S);
  printf("frame_parms->mode1_flag=%d\n",frame_parms->mode1_flag);
  printf("frame_parms->nb_antennas_tx_eNB(nb_antenna_ports)=%d\n",frame_parms->nb_antennas_tx_eNB);
  printf("frame_parms->nb_antennas_tx=%d\n",frame_parms->nb_antennas_tx);
  printf("frame_parms->nb_antennas_rx=%d\n",frame_parms->nb_antennas_rx);
  printf("frame_parms->ofdm_symbol_size=%d\n",frame_parms->ofdm_symbol_size);
  printf("frame_parms->nb_prefix_samples=%d\n",frame_parms->nb_prefix_samples);
  printf("frame_parms->nb_prefix_samples0=%d\n",frame_parms->nb_prefix_samples0);
  printf("frame_parms->first_carrier_offset=%d\n",frame_parms->first_carrier_offset);
  printf("frame_parms->samples_per_tti=%d\n",frame_parms->samples_per_tti);
  printf("frame_parms->symbols_per_tti=%d\n",frame_parms->symbols_per_tti);
}
