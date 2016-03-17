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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/*! \file PHY/MODULATION/beamforming.c
 * \brief 
 * \author X. JIANG, F. Kaltenberger, R. KNOPP
 * \date 2016
 * \version 0.1
 * \company Eurecom
 * \email: xiwen.jiang@eurecom.fr,florian.kaltenberger@eurecom.fr,raymond.knopp@eurecom.fr
 * \note
 * \warning
 */
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "PHY/CODING/defs.h"
#include "PHY/CODING/extern.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/LTE_TRANSPORT/defs.h"
#include "defs.h"
#include "UTIL/LOG/vcd_signal_dumper.h"


// ue_spec_beamforming: perform beamforming for data in transmission_mode TM7-TM10
int ue_spec_beamforming(int32_t **txdataF,
	                int32_t **txdataF_BF,
                        LTE_DL_FRAME_PARMS *frame_parms,
	                int32_t ***ue_spec_bf_weights,
                        int slot,
                        int symbol)
{
  uint8_t p,aa;
  uint16_t re=0;
  int slot_offset_F;
  
  slot_offset_F = slot*(frame_parms->ofdm_symbol_size)*((frame_parms->Ncp==1) ? 6 : 7);

  // clear txdata_BF[aa][re] for each call of spec_spec_beamforming
  for(aa=0;aa<frame_parms->nb_antennas_tx;aa++)
    memset(txdataF_BF[aa],0,4*(frame_parms->ofdm_symbol_size));

  for (re=0;re<frame_parms->ofdm_symbol_size;re++) {
    if (txdataF[5][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re]!=0) { //that means this RE is actually using TM7 
      for (aa=0;aa<frame_parms->nb_antennas_tx;aa++) {
        ((int16_t*)&txdataF_BF[aa][re])[0] = (int16_t)((((int16_t*)&txdataF[5][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0]
                                              *((int16_t*)&ue_spec_bf_weights[0][aa][re])[0])>>15);
        ((int16_t*)&txdataF_BF[aa][re])[0] -= (int16_t)((((int16_t*)&txdataF[5][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1]
                                              *((int16_t*)&ue_spec_bf_weights[0][aa][re])[1])>>15);
        ((int16_t*)&txdataF_BF[aa][re])[1] = (int16_t)((((int16_t*)&txdataF[5][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0]
                                              *((int16_t*)&ue_spec_bf_weights[0][aa][re])[1])>>15);
        ((int16_t*)&txdataF_BF[aa][re])[1] += (int16_t)((((int16_t*)&txdataF[5][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1]
                                              *((int16_t*)&ue_spec_bf_weights[0][aa][re])[0])>>15);

/*        printf("beamforming.c:txdataF[5][%d]=%d+j%d, ue_bf_weights[0][%d][%d]=%d+j%d,txdata_BF[%d][%d]=%d+j%d\n",
               slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re,
               ((int16_t*)&txdataF[5][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0],
               ((int16_t*)&txdataF[5][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1],
               aa,re,
               ((int16_t*)&ue_spec_bf_weights[0][aa][re])[0],((int16_t*)&ue_spec_bf_weights[0][aa][re])[1],
               aa,re,
               ((int16_t*)&txdataF_BF[aa][re])[0],
               ((int16_t*)&txdataF_BF[aa][re])[1]);  */
      }
    }

    if (txdataF[7][re+symbol*frame_parms->ofdm_symbol_size]!=0) { //that means this RE is actually using TM8-10
      for (p=0;p<8;p++) {
        if (txdataF[p+7][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re]!=0) { //that means this RE is actually allocated
          for (aa=0;aa<frame_parms->nb_antennas_tx;aa++) {
            ((int16_t*)&txdataF_BF[aa][re])[0] = (int16_t)((((int16_t*)&txdataF[p+7][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0]
                                                  *((int16_t*)&ue_spec_bf_weights[p][aa][re])[0])>>15);
            ((int16_t*)&txdataF_BF[aa][re])[0] -= (int16_t)((((int16_t*)&txdataF[p+7][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1]
                                                  *((int16_t*)&ue_spec_bf_weights[p][aa][re])[1])>>15);
            ((int16_t*)&txdataF_BF[aa][re])[1] = (int16_t)((((int16_t*)&txdataF[p+7][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0]
                                                  *((int16_t*)&ue_spec_bf_weights[p][aa][re])[1])>>15);
            ((int16_t*)&txdataF_BF[aa][re])[1] += (int16_t)((((int16_t*)&txdataF[p+7][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1]
                                                  *((int16_t*)&ue_spec_bf_weights[p][aa][re])[0])>>15);
          }
        }
      }
    }  
  }
}

// cell_spec_beamforming: performed for all common data
int cell_spec_beamforming(int32_t **txdataF,
	                  int32_t **txdataF_BF,
                          LTE_DL_FRAME_PARMS *frame_parms,
	                  int32_t ***cell_spec_bf_weights,
                          int slot,
                          int symbol)
{
  uint8_t p,aa;
  uint16_t re=0;
  int slot_offset_F;

  slot_offset_F = slot*(frame_parms->ofdm_symbol_size)*((frame_parms->Ncp==1) ? 6 : 7);
  
  // clear txdata_BF[aa][re] for each call of cell_spec_beamforming
  for(aa=0;aa<frame_parms->nb_antennas_tx;aa++)
    memset(txdataF_BF[aa],0,4*(frame_parms->ofdm_symbol_size));

  for (re=0;re<frame_parms->ofdm_symbol_size;re++) {
    for (p=0;p<frame_parms->nb_antenna_ports_eNB;p++) {
      if (txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re]!=0) { //that means this RE is actually allocated
        for (aa=0;aa<frame_parms->nb_antennas_tx;aa++) {
          //printf("cell_spec_beamforming():txdata_BF[%d][%d]=%d+j%d\n",aa,re,((int16_t*)&txdataF_BF[aa][re])[0],((int16_t*)&txdataF_BF[aa][re])[1]);
        
          ((int16_t*)&txdataF_BF[aa][re])[0] = (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0]
                                                *((int16_t*)&cell_spec_bf_weights[p][aa][re])[0])>>15);
          ((int16_t*)&txdataF_BF[aa][re])[0] -= (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1]
                                                *((int16_t*)&cell_spec_bf_weights[p][aa][re])[1])>>15);
          ((int16_t*)&txdataF_BF[aa][re])[1] = (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0]
                                                *((int16_t*)&cell_spec_bf_weights[p][aa][re])[1])>>15);
          ((int16_t*)&txdataF_BF[aa][re])[1] += (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1]
                                                *((int16_t*)&cell_spec_bf_weights[p][aa][re])[0])>>15);
         // printf("beamforming.c:txdata[%d][%d]=%d+j%d, cell_bf_weights[%d][%d][%d]=%d+j%d,txdata_BF[%d][%d]=%d+j%d\n",
         //        p,slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re,
         //        ((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0],
         //        ((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1],
         //        p,aa,re,
         //        ((int16_t*)&cell_spec_bf_weights[p][aa][re])[0],((int16_t*)&cell_spec_bf_weights[p][aa][re])[1],
         //        aa,re,
         //        ((int16_t*)&txdataF_BF[aa][re])[0],
         //        ((int16_t*)&txdataF_BF[aa][re])[1]);
        }
      }
    }
  }
}
/*
int ue_spec_beamforming(int32_t **txdataF,
	                int32_t **txdataF_BF,
                        LTE_DL_FRAME_PARMS *frame_parms,
                        LTE_eNB_DLSCH_t *dlsch0,
                        LTE_eNB_DLSCH_t *dlsch1,
                        int symbol)
{
  uint8_t p,aa;
  uint16_t re=0;

  uint8_t harq_pid = dlsch0->current_harq_pid;
  LTE_DL_eNB_HARQ_t *dlsch0_harq = dlsch0->harq_processes[harq_pid];
  LTE_DL_eNB_HARQ_t *dlsch1_harq; //= dlsch1->harq_processes[harq_pid];
  MIMO_mode_t mimo_mode = dlsch0_harq->mimo_mode;
  int first_layer0        = dlsch0_harq->first_layer;
  int Nlayers0            = dlsch0_harq->Nlayers;

  // Fill these in later for TM8-10
  int Nlayers1;
  int first_layer1;

  if (dlsch1_harq) {
    Nlayers1       = dlsch1_harq->Nlayers;
    first_layer1   = dlsch1_harq->first_layer;
  }
 
  //temp
  if(mimo_mode==TM7){
    first_layer0 = 5;
    Nlayers0=1;
  }
  
  for (re=0;re<N_RB_DL*12;re++) {
    //for (p=5,7..14) // this depends on the mimo_mode, but also the used ports for each UE
    for (p=first_layer0;p<first_layer0+Nlayers0;p++) {
      if (txdataF[p][re+symbol*N_RB_DL*12]!=0) { //that means this RE is actually allocated
        for (aa=0;aa<frame_aprms->nb_antennas_tx;aa++) {
          txdataF_BF[aa][re] += txdataF[p][re]*dlsch0->uespec_bf_weights[p][aa][re];
        } 
      } 
    }

    if(dlsch1_harq) {
      for (p=first_layer1;p<first_layer1+Nlayers1;p++) {
        if (txdataF[p][re+symbol*N_RB_DL*12]!=0) { //that means this RE is actually allocated
          for (aa=0;aa<frame_aprms->nb_antennas_tx;aa++) {
            txdataF_BF[aa][re] += txdataF[p][re]*dlsch1->uespec_bf_weights[p][aa][re];
          } 
        } 
      }
    }

  }
} */
