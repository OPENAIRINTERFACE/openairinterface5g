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

/*! \file PHY/LTE_TRANSPORT/pcfich.c
* \brief Top-level routines for generating and decoding  the PCFICH/CFI physical/transport channel V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/
#include "PHY/defs_UE.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"

//#define DEBUG_PCFICH
#include "openair1/PHY/LTE_TRANSPORT/transport_vars.h"

void pcfich_unscrambling(LTE_DL_FRAME_PARMS *frame_parms,
                         uint8_t subframe,
                         int16_t *d)
{

  uint8_t reset = 1;
  uint32_t x1 = 0; // x1 is set in lte_gold_generic
  uint32_t s = 0;

  uint32_t x2 = ((((2*frame_parms->Nid_cell)+1)*(1+subframe))<<9) + frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.7.1

  for (uint32_t i=0; i<32; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      //printf("lte_gold[%d]=%x\n",i,s);
      reset = 0;
    }

    if (((s>>(i&0x1f))&1) == 1)
      d[i]=-d[i];

  }
}

uint8_t rx_pcfich(LTE_DL_FRAME_PARMS *frame_parms,
                  uint8_t subframe,
                  LTE_UE_PDCCH *lte_ue_pdcch_vars,
                  MIMO_mode_t mimo_mode)
{

  uint8_t pcfich_quad;
  uint8_t i,j;
  uint16_t reg_offset;

  int32_t **rxdataF_comp = lte_ue_pdcch_vars->rxdataF_comp;
  int16_t pcfich_d[32],*pcfich_d_ptr;
  int32_t metric,old_metric=-16384;
  uint8_t num_pdcch_symbols=3;
  uint16_t *pcfich_reg = frame_parms->pcfich_reg;

  // demapping
  // loop over 4 quadruplets and lookup REGs
  //  m=0;
  pcfich_d_ptr = pcfich_d;

  for (pcfich_quad=0; pcfich_quad<4; pcfich_quad++) {
    reg_offset = (pcfich_reg[pcfich_quad]*4);

    for (i=0; i<4; i++) {

      pcfich_d_ptr[0] = ((int16_t*)&rxdataF_comp[0][reg_offset+i])[0]; // RE component
      pcfich_d_ptr[1] = ((int16_t*)&rxdataF_comp[0][reg_offset+i])[1]; // IM component
#ifdef DEBUG_PCFICH      
      printf("rx_pcfich: quad %d, i %d, offset %d =>  (%d,%d) => pcfich_d_ptr[0] %d \n",pcfich_quad,i,reg_offset+i,
             ((int16_t*)&rxdataF_comp[0][reg_offset+i])[0],
             ((int16_t*)&rxdataF_comp[0][reg_offset+i])[1],
             pcfich_d_ptr[0]);
#endif
      pcfich_d_ptr+=2;
    }

    /*
    }
    else { // ALAMOUTI
    for (i=0;i<4;i+=2) {
    pcfich_d_ptr[0] = 0;
    pcfich_d_ptr[1] = 0;
    pcfich_d_ptr[2] = 0;
    pcfich_d_ptr[3] = 0;
    for (j=0;j<frame_parms->nb_antennas_rx;j++) {

    pcfich_d_ptr[0] += (((int16_t*)&rxdataF_comp[j][reg_offset+i])[0]+
         ((int16_t*)&rxdataF_comp[j+2][reg_offset+i+1])[0]); // RE component
    pcfich_d_ptr[1] += (((int16_t*)&rxdataF_comp[j][reg_offset+i])[1] -
         ((int16_t*)&rxdataF_comp[j+2][reg_offset+i+1])[1]);// IM component

    pcfich_d_ptr[2] += (((int16_t*)&rxdataF_comp[j][reg_offset+i+1])[0]-
         ((int16_t*)&rxdataF_comp[j+2][reg_offset+i])[0]); // RE component
    pcfich_d_ptr[3] += (((int16_t*)&rxdataF_comp[j][reg_offset+i+1])[1] +
         ((int16_t*)&rxdataF_comp[j+2][reg_offset+i])[1]);// IM component


    }

    pcfich_d_ptr+=4;

    }
    */
  }

  // pcfhich unscrambling

  pcfich_unscrambling(frame_parms,subframe,pcfich_d);

  // pcfich detection

  for (i=0; i<3; i++) {
    metric = 0;

    for (j=0; j<32; j++) {
      //      printf("pcfich_b[%d][%d] %d => pcfich_d[%d] %d\n",i,j,pcfich_b[i][j],j,pcfich_d[j]);
      metric += (int32_t)(((pcfich_b[i][j]==0) ? (pcfich_d[j]) : (-pcfich_d[j])));
    }

#ifdef DEBUG_PCFICH
    printf("metric %d : %d\n",i,metric);
#endif

    if (metric > old_metric) {
      num_pdcch_symbols = 1+i;
      old_metric = metric;
    }
  }

#ifdef DEBUG_PCFICH
  printf("[PHY] PCFICH detected for %d PDCCH symbols\n",num_pdcch_symbols);
#endif
  return(num_pdcch_symbols);
}
