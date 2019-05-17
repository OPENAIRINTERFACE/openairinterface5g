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

/*! \file PHY/LTE_TRANSPORT/pss.c
* \brief Top-level routines for generating primary synchronization signal (PSS) V8.6 2009-03
* \author F. Kaltenberger, O. Tonelli, R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: florian.kaltenberger@eurecom.fr, oscar.tonelli@yahoo.it,knopp@eurecom.fr
* \note
* \warning
*/
/* file: pss.c
   purpose: generate the primary synchronization signals of LTE
   author: florian.kaltenberger@eurecom.fr, oscar.tonelli@yahoo.it
   date: 21.10.2009
*/

//#include "defs.h"
#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h"
#include "targets/RT/USER/lte-softmodem.h"

int generate_pss(int32_t **txdataF,
                 short amp,
                 LTE_DL_FRAME_PARMS *frame_parms,
                 unsigned short symbol,
                 unsigned short slot_offset) {
  unsigned int Nsymb;
  unsigned short k,m,aa,a;
  uint8_t Nid2;
  short *primary_sync;
  Nid2 = frame_parms->Nid_cell % 3;

  switch (Nid2) {
    case 0:
      primary_sync = primary_synch0;
      break;

    case 1:
      primary_sync = primary_synch1;
      break;

    case 2:
      primary_sync = primary_synch2;
      break;

    default:
      LOG_E(PHY,"[PSS] eNb_id has to be 0,1,2\n");
      return(-1);
  }

  a = (frame_parms->nb_antenna_ports_eNB == 1) ? amp: (amp*ONE_OVER_SQRT2_Q15)>>15;
  //printf("[PSS] amp=%d, a=%d\n",amp,a);

  if (IS_SOFTMODEM_BASICSIM)
    /* a hack to remove at some point (the UE doesn't synch with 100 RBs) */
    a = (frame_parms->nb_antenna_ports_eNB == 1) ? 4*amp: (amp*ONE_OVER_SQRT2_Q15)>>15;

  Nsymb = (frame_parms->Ncp==NORMAL)?14:12;

  for (aa=0; aa<frame_parms->nb_antenna_ports_eNB; aa++) {
    //  aa = 0;
    // The PSS occupies the inner 6 RBs, which start at
    k = frame_parms->ofdm_symbol_size-3*12+5;

    //printf("[PSS] k = %d\n",k);
    for (m=5; m<67; m++) {
      ((short *)txdataF[aa])[2*(slot_offset*Nsymb/2*frame_parms->ofdm_symbol_size +
                                symbol*frame_parms->ofdm_symbol_size + k)] =
                                  (a * primary_sync[2*m]) >> 15;
      ((short *)txdataF[aa])[2*(slot_offset*Nsymb/2*frame_parms->ofdm_symbol_size +
                                symbol*frame_parms->ofdm_symbol_size + k) + 1] =
                                  (a * primary_sync[2*m+1]) >> 15;
      k+=1;

      if (k >= frame_parms->ofdm_symbol_size) {
        k++; //skip DC
        k-=frame_parms->ofdm_symbol_size;
      }
    }
  }

  return(0);
}

