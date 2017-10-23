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

#ifdef USER_MODE
#include <string.h>
#endif
#include "defs.h"
#include "PHY/defs.h"
#include "SCHED/phy_procedures_emos.h"

// TODO: make channel estimation possible for multiple sectors (Gold sequences for pilots)

//#define DEBUG_CH
int lte_dl_channel_estimation_emos(int dl_ch_estimates_emos[NB_ANTENNAS_RX*NB_ANTENNAS_TX][N_RB_DL_EMOS*N_PILOTS_PER_RB*N_SLOTS_EMOS],
                                   int **rxdataF,
                                   LTE_DL_FRAME_PARMS *frame_parms,
                                   unsigned char Ns,
                                   unsigned char p,
                                   unsigned char l,
                                   unsigned char sector)
{

  int pilot[2][200] __attribute__((aligned(16)));
  unsigned char nu,aarx;
  unsigned short k;
  unsigned int rb,pilot_cnt;
  short ch[2],*pil,*rxF,*dl_ch,*dl_ch_prev; //*f,*f2,*fl,*f2l2,*fr,*f2r2;
  int ch_offset,symbol_offset;
  unsigned int n;
  int i;

  unsigned char symbol = l+((7-frame_parms->Ncp)*(Ns&1)); ///symbol within sub-frame

  if ((p==0) && (l==0) )
    nu = 0;
  else if ((p==0) && (l>0))
    nu = 3;
  else if ((p==1) && (l==0))
    nu = 3;
  else if ((p==1) && (l>0))
    nu = 0;
  else {
    msg("lte_dl_channel_estimation_emos: p %d, l %d -> ERROR\n",p,l);
    return(-1);
  }

  if (sector > 2) {
    msg("lte_dl_channel_estimation_emos: sector must be 0,1, or 2\n");
    return(-1);
  }

  switch (Ns) {
  case 0:
    ch_offset = ((l==0)?0:1)*2*frame_parms->N_RB_DL;
    break;

  case 1:
    ch_offset = ((l==0)?2:3)*2*frame_parms->N_RB_DL;
    break;

  case 12:
    ch_offset = ((l==0)?4:5)*2*frame_parms->N_RB_DL;
    break;

  case 13:
    ch_offset = ((l==0)?6:7)*2*frame_parms->N_RB_DL;
    break;

  default:
    msg("lte_dl_channel_estimation_emos: Ns must be  0, 1, 12, or 13\n");
    return(-1);
    break;

  }

  symbol_offset = symbol*frame_parms->ofdm_symbol_size; // offset within rxdataF

  k = nu + sector;

  if (k > 6)
    k -=6;

#ifdef DEBUG_CH
  printf("Channel Estimation : ch_offset %d, OFDM size %d, Ncp=%d, l=%d, Ns=%d, k=%d, symbol=%d\n",ch_offset,frame_parms->ofdm_symbol_size,frame_parms->Ncp,l,Ns,k,symbol);
#endif

  // generate pilot
  lte_dl_cell_spec_rx(&pilot[p][0],
                      frame_parms,
                      Ns,
                      (l==0)?0:1,
                      p);


  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

    pil   = (short *)&pilot[p][0];
    rxF   = (short *)&rxdataF[aarx][((symbol_offset+k+frame_parms->first_carrier_offset)<<1)];
    dl_ch = (short *)&dl_ch_estimates_emos[(p<<1)+aarx][ch_offset];
    memset(dl_ch,0,frame_parms->N_RB_DL);

    for (pilot_cnt=0; pilot_cnt<frame_parms->N_RB_DL; pilot_cnt++) {

      dl_ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
      dl_ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);

      pil+=2;    // Re Im
      rxF+=24;   // remember replicated format (Re0 Im0 Re0 Im0) !!!
      dl_ch+=2;

    }

    // printf("Second half\n");
    // Second half of RBs
    rxF   = (short *)&rxdataF[aarx][((symbol_offset+1+k)<<1)];

    for (pilot_cnt=0; pilot_cnt<frame_parms->N_RB_DL; pilot_cnt++) {


      dl_ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
      dl_ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);

      pil+=2;
      rxF+=24;
      dl_ch+=2;

    }

  }

  return(0);
}

