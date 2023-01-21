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

/*! \file PHY/LTE_TRANSPORT/sss.c
* \brief Top-level routines for generating and decoding the secondary synchronization signal (SSS) V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/
#include "PHY/defs_eNB.h"
#include "transport_eNB.h"
#include "PHY/phy_extern.h"
#include "openair1/PHY/LTE_TRANSPORT/transport_vars.h"

//#define DEBUG_SSS


int generate_sss(int32_t **txdataF,
                 int16_t amp,
                 LTE_DL_FRAME_PARMS *frame_parms,
                 uint16_t symbol,
                 uint16_t slot_offset)
{

  uint8_t i,aa,Nsymb;
  int16_t *d,k;
  uint8_t Nid2;
  uint16_t Nid1;
  int16_t a;

  Nid2 = frame_parms->Nid_cell % 3;
  Nid1 = frame_parms->Nid_cell/3;

  if (slot_offset < 3)
    d = &d0_sss[62*(Nid2 + (Nid1*3))];
  else
    d = &d5_sss[62*(Nid2 + (Nid1*3))];

  Nsymb = (frame_parms->Ncp==NORMAL)?14:12;
  k = frame_parms->ofdm_symbol_size-3*12+5;
  a = (frame_parms->nb_antenna_ports_eNB == 1) ? amp : (amp*ONE_OVER_SQRT2_Q15)>>15;

  for (i=0; i<62; i++) {
    for (aa=0; aa<frame_parms->nb_antenna_ports_eNB; aa++) {

      ((int16_t*)txdataF[aa])[2*(slot_offset*Nsymb/2*frame_parms->ofdm_symbol_size +
                                 symbol*frame_parms->ofdm_symbol_size + k)] =
                                   (a * d[i]);
      ((int16_t*)txdataF[aa])[2*(slot_offset*Nsymb/2*frame_parms->ofdm_symbol_size +
                                 symbol*frame_parms->ofdm_symbol_size + k)+1] = 0;
    }

    k+=1;

    if (k >= frame_parms->ofdm_symbol_size) {
      k++;
      k-=frame_parms->ofdm_symbol_size;
    }
  }

  return(0);
}

