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

/*! \file PHY/LTE_TRANSPORT/dci.c
* \brief Implements PDCCH physical channel TX/RX procedures (36.211) and DCI encoding/decoding (36.212/36.213). Current LTE compliance V8.6 2009-03.
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h"
#include "SCHED/sched_eNB.h"
#include "SIMULATION/TOOLS/sim.h" // for taus
#include "PHY/sse_intrin.h"
#include "transport_proto.h"
#include "transport_common_proto.h"
#include "assertions.h"
#include "T.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "PHY/LTE_TRANSPORT/transport_extern.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"
#include "targets/RT/USER/lte-softmodem.h"
//#define DEBUG_DCI_ENCODING 1
//#define DEBUG_DCI_DECODING 1
//#define DEBUG_PHY

//#undef ALL_AGGREGATION

//extern uint16_t phich_reg[MAX_NUM_PHICH_GROUPS][3];
//extern uint16_t pcfich_reg[4];



//static uint8_t d[3*(MAX_DCI_SIZE_BITS + 16) + 96];
//static uint8_t w[3*3*(MAX_DCI_SIZE_BITS+16)];

void dci_encoding(uint8_t *a,
                  uint8_t A,
                  uint16_t E,
                  uint8_t *e,
                  uint16_t rnti) {
  uint8_t D = (A + 16);
  uint32_t RCC;
  uint8_t d[3*(MAX_DCI_SIZE_BITS + 16) + 96];
  uint8_t w[3*3*(MAX_DCI_SIZE_BITS+16)];
#ifdef DEBUG_DCI_ENCODING
  int32_t i;
  printf("Doing DCI encoding for %d bits, e %p, rnti %x, E %d\n",A,e,rnti,E);
#endif
  // encode dci
  memset((void *)d,LTE_NULL,96);
  ccodelte_encode(A,2,a,d+96,rnti);
#ifdef DEBUG_DCI_ENCODING

  for (i=0; i<16+A; i++)
    printf("%d : (%d,%d,%d)\n",i,*(d+96+(3*i)),*(d+97+(3*i)),*(d+98+(3*i)));
  printf("Doing DCI interleaving for %d coded bits, e %p\n",D*3,e);
#endif

  RCC = sub_block_interleaving_cc(D,d+96,w);

#ifdef DEBUG_DCI_ENCODING
  if (E>1000) printf("Doing DCI rate matching for %d channel bits, RCC %d, e %p\n",E,RCC,e);
#endif

  lte_rate_matching_cc(RCC,E,w,e);
}


uint8_t *generate_dci0(uint8_t *dci,
                       uint8_t *e,
                       uint8_t DCI_LENGTH,
                       uint16_t coded_bits,
                       uint16_t rnti) {
  uint8_t dci_flip[8];
#ifdef DEBUG_DCI_ENCODING

  for (int i=0; i<1+((DCI_LENGTH+16)/8); i++)
    printf("i %d : %x\n",i,dci[i]);

#endif

  if (DCI_LENGTH<=32) {
    dci_flip[0] = dci[3];
    dci_flip[1] = dci[2];
    dci_flip[2] = dci[1];
    dci_flip[3] = dci[0];
#ifdef DEBUG_DCI_ENCODING
    printf("DCI => %x,%x,%x,%x\n",
           dci_flip[0],dci_flip[1],dci_flip[2],dci_flip[3]);
#endif
  } else {
    dci_flip[0] = dci[7];
    dci_flip[1] = dci[6];
    dci_flip[2] = dci[5];
    dci_flip[3] = dci[4];
    dci_flip[4] = dci[3];
    dci_flip[5] = dci[2];
    dci_flip[6] = dci[1];
    dci_flip[7] = dci[0];
#ifdef DEBUG_DCI_ENCODING
    printf("DCI => %x,%x,%x,%x,%x,%x,%x,%x\n",
           dci_flip[0],dci_flip[1],dci_flip[2],dci_flip[3],
           dci_flip[4],dci_flip[5],dci_flip[6],dci_flip[7]);
#endif
  }

  dci_encoding(dci_flip,DCI_LENGTH,coded_bits,e,rnti);
  return(e+coded_bits);
}

//uint32_t Y;






void pdcch_interleaving(LTE_DL_FRAME_PARMS *frame_parms,int32_t **z, int32_t **wbar,uint8_t n_symbols_pdcch,uint8_t mi) {
  int32_t *wptr,*wptr2,*zptr;
  uint32_t Mquad = get_nquad(n_symbols_pdcch,frame_parms,mi);
  uint32_t RCC = (Mquad>>5), ND;
  uint32_t row,col,Kpi,index;
  int32_t i,k,a;
#ifdef RM_DEBUG
  int32_t nulled=0;
#endif
  uint32_t Msymb=(DCI_BITS_MAX/2);
  int32_t wtemp[2][Msymb];

  //  printf("[PHY] PDCCH Interleaving Mquad %d (Nsymb %d)\n",Mquad,n_symbols_pdcch);
  if ((Mquad&0x1f) > 0)
    RCC++;

  Kpi = (RCC<<5);
  ND = Kpi - Mquad;
  k=0;

  for (col=0; col<32; col++) {
    index = bitrev_cc_dci[col];

    for (row=0; row<RCC; row++) {
      //printf("col %d, index %d, row %d\n",col,index,row);
      if (index>=ND) {
        for (a=0; a<frame_parms->nb_antenna_ports_eNB; a++) {
          //printf("a %d k %d\n",a,k);
          wptr = &wtemp[a][k<<2];
          zptr = &z[a][(index-ND)<<2];
          //printf("wptr=%p, zptr=%p\n",wptr,zptr);
          wptr[0] = zptr[0];
          wptr[1] = zptr[1];
          wptr[2] = zptr[2];
          wptr[3] = zptr[3];
        }

        k++;
      }

      index+=32;
    }
  }

  // permutation
  for (i=0; i<Mquad; i++) {
    for (a=0; a<frame_parms->nb_antenna_ports_eNB; a++) {
      //wptr  = &wtemp[a][i<<2];
      //wptr2 = &wbar[a][((i+frame_parms->Nid_cell)%Mquad)<<2];
      wptr = &wtemp[a][((i+frame_parms->Nid_cell)%Mquad)<<2];
      wptr2 = &wbar[a][i<<2];
      wptr2[0] = wptr[0];
      wptr2[1] = wptr[1];
      wptr2[2] = wptr[2];
      wptr2[3] = wptr[3];
    }
  }
}

void pdcch_scrambling(LTE_DL_FRAME_PARMS *frame_parms,
                      uint8_t subframe,
                      uint8_t *e,
                      uint32_t length) {
  int i;
  uint8_t reset;
  uint32_t x1, x2, s=0;
  //LOG_D(PHY, "%s(fp, subframe:%d, e, length:%d)\n", __FUNCTION__, subframe, length);
  reset = 1;
  // x1 is set in lte_gold_generic
  x2 = (subframe<<9) + frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.8.2

  for (i=0; i<length; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      //printf("lte_gold[%d]=%x\n",i,s);
      reset = 0;
    }

    //    printf("scrambling %d : e %d, c %d\n",i,e[i],((s>>(i&0x1f))&1));
    if (e[i] != 2) // <NIL> element is 2
      e[i] = (e[i]&1) ^ ((s>>(i&0x1f))&1);
  }
}

uint8_t generate_dci_top(uint8_t num_pdcch_symbols,
                         uint8_t num_dci,
                         DCI_ALLOC_t *dci_alloc,
                         uint32_t n_rnti,
                         int16_t amp,
                         LTE_DL_FRAME_PARMS *frame_parms,
                         int32_t **txdataF,
                         uint32_t subframe) {
  uint8_t *e_ptr;
  int8_t L;
  uint32_t i, lprime;
  uint32_t gain_lin_QPSK,kprime,kprime_mod12,mprime,nsymb,symbol_offset,tti_offset;
  int16_t re_offset;
  uint8_t mi = get_mi(frame_parms,subframe);
  uint8_t e[DCI_BITS_MAX];
  uint32_t Msymb=(DCI_BITS_MAX/2);
  int32_t yseq0[Msymb],yseq1[Msymb],wbar0[Msymb],wbar1[Msymb];
  int32_t *y[2];
  int32_t *wbar[2];
  int nushiftmod3 = frame_parms->nushift%3;
  int Msymb2;
  int split_flag=0;

  switch (frame_parms->N_RB_DL) {
    case 100:
      Msymb2 = Msymb;
      break;

    case 75:
      Msymb2 = 3*Msymb/4;
      break;

    case 50:
      Msymb2 = Msymb>>1;
      break;

    case 25:
      Msymb2 = Msymb>>2;
      break;

    case 15:
      Msymb2 = Msymb*15/100;
      break;

    case 6:
      Msymb2 = Msymb*6/100;
      break;

    default:
      Msymb2 = Msymb>>2;
      break;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GENERATE_PCFICH,1);
  generate_pcfich(num_pdcch_symbols,
                  amp,
                  frame_parms,
                  txdataF,
                  subframe);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GENERATE_PCFICH,0);
  wbar[0] = &wbar0[0];
  wbar[1] = &wbar1[0];
  y[0] = &yseq0[0];
  y[1] = &yseq1[0];

  /* reset all bits to <NIL>, here we set <NIL> elements as 2 */
  memset(e, 2, DCI_BITS_MAX);

  e_ptr = e;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GENERATE_DCI0,1);

  // generate DCIs in order of decreasing aggregation level, then common/ue spec
  // MAC is assumed to have ordered the UE spec DCI according to the RNTI-based randomization
  for (L=8; L>=1; L>>=1) {
    for (i=0; i<num_dci; i++) {
      if (dci_alloc[i].L == (uint8_t)L) {
        LOG_D(PHY,"Generating DCI %d/%d (nCCE %d) of length %d, aggregation %d (%x), rnti %x\n",
              i,num_dci,dci_alloc[i].firstCCE,dci_alloc[i].dci_length,dci_alloc[i].L,
              *(unsigned int *)dci_alloc[i].dci_pdu,
              dci_alloc[i].rnti);

        if (dci_alloc[i].firstCCE>=0) {
          e_ptr = generate_dci0(dci_alloc[i].dci_pdu,
                                e+(72*dci_alloc[i].firstCCE),
                                dci_alloc[i].dci_length,
                                72*dci_alloc[i].L,
                                dci_alloc[i].rnti);
        }
      }
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GENERATE_DCI0,0);
  // Scrambling
#ifdef DEBUG_DCI_ENCODING
  printf("pdcch scrambling\n");
#endif
  //LOG_D(PHY, "num_pdcch_symbols:%d mi:%d nquad:%d\n", num_pdcch_symbols, mi, get_nquad(num_pdcch_symbols, frame_parms, mi));
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCCH_SCRAMBLING,1);
  pdcch_scrambling(frame_parms,
                   subframe,
                   e,
                   8*get_nquad(num_pdcch_symbols, frame_parms, mi));
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCCH_SCRAMBLING,0);
  //72*get_nCCE(num_pdcch_symbols,frame_parms,mi));
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCCH_MODULATION,1);

  // Now do modulation
  if (frame_parms->nb_antenna_ports_eNB==1)
    gain_lin_QPSK = (int16_t)((amp*ONE_OVER_SQRT2_Q15)>>15);
  else
    gain_lin_QPSK = amp/2;

  e_ptr = e;
#ifdef DEBUG_DCI_ENCODING
  printf(" PDCCH Modulation, Msymb %d, Msymb2 %d,gain_lin_QPSK %d\n",Msymb,Msymb2,gain_lin_QPSK);
#endif

  //LOG_D(PHY,"%s() Msymb2:%d\n", __FUNCTION__, Msymb2);

  if (frame_parms->nb_antenna_ports_eNB==1) { //SISO
    for (i=0; i<Msymb2; i++) {
      //((int16_t*)(&(y[0][i])))[0] = (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      //((int16_t*)(&(y[1][i])))[0] = (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      ((int16_t *)(&(y[0][i])))[0] = (*e_ptr == 2) ? 0 : (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      ((int16_t *)(&(y[1][i])))[0] = (*e_ptr == 2) ? 0 : (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      e_ptr++;
      //((int16_t*)(&(y[0][i])))[1] = (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      //((int16_t*)(&(y[1][i])))[1] = (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      ((int16_t *)(&(y[0][i])))[1] = (*e_ptr == 2) ? 0 : (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      ((int16_t *)(&(y[1][i])))[1] = (*e_ptr == 2) ? 0 : (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      e_ptr++;
    }
  } else { //ALAMOUTI
    for (i=0; i<Msymb2; i+=2) {
#ifdef DEBUG_DCI_ENCODING
      printf(" PDCCH Modulation (TX diversity): REG %d\n",i>>2);
#endif
      // first antenna position n -> x0
      ((int16_t *)&y[0][i])[0] = (*e_ptr==2) ? 0 : (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      e_ptr++;
      ((int16_t *)&y[0][i])[1] = (*e_ptr==2) ? 0 : (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      e_ptr++;
      // second antenna position n -> -x1*
      ((int16_t *)&y[1][i])[0] = (*e_ptr==2) ? 0 : (*e_ptr == 1) ? gain_lin_QPSK : -gain_lin_QPSK;
      e_ptr++;
      ((int16_t *)&y[1][i])[1] = (*e_ptr==2) ? 0 : (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      e_ptr++;
      // fill in the rest of the ALAMOUTI precoding
      ((int16_t *)&y[0][i+1])[0] = -((int16_t *)&y[1][i])[0];
      ((int16_t *)&y[0][i+1])[1] = ((int16_t *)&y[1][i])[1];
      ((int16_t *)&y[1][i+1])[0] = ((int16_t *)&y[0][i])[0];
      ((int16_t *)&y[1][i+1])[1] = -((int16_t *)&y[0][i])[1];
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCCH_MODULATION,0);
#ifdef DEBUG_DCI_ENCODING
  printf(" PDCCH Interleaving\n");
#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCCH_INTERLEAVING,1);
  //  printf("y %p (%p,%p), wbar %p (%p,%p)\n",y,y[0],y[1],wbar,wbar[0],wbar[1]);
  // This is the interleaving procedure defined in 36-211, first part of Section 6.8.5
  pdcch_interleaving(frame_parms,&y[0],&wbar[0],num_pdcch_symbols,mi);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCCH_INTERLEAVING,0);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCCH_TX,1);
  mprime=0;
  nsymb = (frame_parms->Ncp==0) ? 14:12;
  re_offset = frame_parms->first_carrier_offset;
  // This is the REG allocation algorithm from 36-211, second part of Section 6.8.5
  //  printf("DCI (SF %d) : txdataF %p (0 %p)\n",subframe,&txdataF[0][512*14*subframe],&txdataF[0][0]);
#ifdef DEBUG_DCI_ENCODING
  printf("kprime loop - N_RB_DL:%d lprime:num_pdcch_symbols:%d Ncp:%d pcfich:%02x,%02x,%02x,%02x ofdm_symbol_size:%d first_carrier_offset:%d nb_antenna_ports_eNB:%d\n",
         frame_parms->N_RB_DL, num_pdcch_symbols,frame_parms->Ncp,
         frame_parms->pcfich_reg[0],
         frame_parms->pcfich_reg[1],
         frame_parms->pcfich_reg[2],
         frame_parms->pcfich_reg[3],
         frame_parms->ofdm_symbol_size,
         frame_parms->first_carrier_offset,
         frame_parms->nb_antenna_ports_eNB
        );
#endif

  for (kprime=0; kprime<frame_parms->N_RB_DL*12; kprime++) {
    for (lprime=0; lprime<num_pdcch_symbols; lprime++) {
      symbol_offset = (uint32_t)frame_parms->ofdm_symbol_size*(lprime+(subframe*nsymb));
      tti_offset = symbol_offset + re_offset;
      (re_offset==(frame_parms->ofdm_symbol_size-2)) ? (split_flag=1) : (split_flag=0);

      //            printf("kprime %d, lprime %d => REG %d (symbol %d)\n",kprime,lprime,(lprime==0)?(kprime/6) : (kprime>>2),symbol_offset);
      // if REG is allocated to PHICH, skip it
      if (check_phich_reg(frame_parms,kprime,lprime,mi) == 1) {
#ifdef DEBUG_DCI_ENCODING
        printf("generate_dci: skipping REG %d (kprime %d, lprime %d)\n",(lprime==0)?(kprime/6) : (kprime>>2),kprime,lprime);
#endif
      } else {
        // Copy REG to TX buffer
        if ((lprime == 0)||
            ((lprime==1)&&(frame_parms->nb_antenna_ports_eNB == 4))) {
          // first symbol, or second symbol+4 TX antennas skip pilots
          kprime_mod12 = kprime%12;

          if ((kprime_mod12 == 0) || (kprime_mod12 == 6)) {
            // kprime represents REG
            for (i=0; i<6; i++) {
              if ((i!=(nushiftmod3))&&(i!=(nushiftmod3+3))) {
                txdataF[0][tti_offset+i] = wbar[0][mprime];

                if (frame_parms->nb_antenna_ports_eNB > 1)
                  txdataF[1][tti_offset+i] = wbar[1][mprime];

#ifdef DEBUG_DCI_ENCODING
                printf(" PDCCH mapping mprime %d => %d (symbol %d re %d) -> (%d,%d)\n",mprime,tti_offset,symbol_offset,re_offset+i,*(short *)&wbar[0][mprime],*(1+(short *)&wbar[0][mprime]));
#endif
                mprime++;
              }
            }
          }
        } else { // no pilots in this symbol
          kprime_mod12 = kprime%12;

          if ((kprime_mod12 == 0) || (kprime_mod12 == 4) || (kprime_mod12 == 8)) {
            // kprime represents REG
            if (split_flag==0) {
              for (i=0; i<4; i++) {
                txdataF[0][tti_offset+i] = wbar[0][mprime];

                if (frame_parms->nb_antenna_ports_eNB > 1)
                  txdataF[1][tti_offset+i] = wbar[1][mprime];

#ifdef DEBUG_DCI_ENCODING
                LOG_I(PHY," PDCCH mapping mprime %d => %d (symbol %d re %d) -> (%d,%d)\n",mprime,tti_offset,symbol_offset,re_offset+i,*(short *)&wbar[0][mprime],*(1+(short *)&wbar[0][mprime]));
#endif
                mprime++;
              }
            } else {
              txdataF[0][tti_offset+0] = wbar[0][mprime];

              if (frame_parms->nb_antenna_ports_eNB > 1)
                txdataF[1][tti_offset+0] = wbar[1][mprime];

#ifdef DEBUG_DCI_ENCODING
              printf(" PDCCH mapping mprime %d => %d (symbol %d re %d) -> (%d,%d)\n",mprime,tti_offset,symbol_offset,re_offset,*(short *)&wbar[0][mprime],*(1+(short *)&wbar[0][mprime]));
#endif
              mprime++;
              txdataF[0][tti_offset+1] = wbar[0][mprime];

              if (frame_parms->nb_antenna_ports_eNB > 1)
                txdataF[1][tti_offset+1] = wbar[1][mprime];

#ifdef DEBUG_DCI_ENCODING
              printf("PDCCH mapping mprime %d => %d (symbol %d re %d) -> (%d,%d)\n",mprime,tti_offset,symbol_offset,re_offset+1,*(short *)&wbar[0][mprime],*(1+(short *)&wbar[0][mprime]));
#endif
              mprime++;
              txdataF[0][tti_offset-frame_parms->ofdm_symbol_size+3] = wbar[0][mprime];

              if (frame_parms->nb_antenna_ports_eNB > 1)
                txdataF[1][tti_offset-frame_parms->ofdm_symbol_size+3] = wbar[1][mprime];

#ifdef DEBUG_DCI_ENCODING
              printf(" PDCCH mapping mprime %d => %d (symbol %d re %d) -> (%d,%d)\n",mprime,tti_offset,symbol_offset,re_offset-frame_parms->ofdm_symbol_size+3,*(short *)&wbar[0][mprime],
                     *(1+(short *)&wbar[0][mprime]));
#endif
              mprime++;
              txdataF[0][tti_offset-frame_parms->ofdm_symbol_size+4] = wbar[0][mprime];

              if (frame_parms->nb_antenna_ports_eNB > 1)
                txdataF[1][tti_offset-frame_parms->ofdm_symbol_size+4] = wbar[1][mprime];

#ifdef DEBUG_DCI_ENCODING
              printf(" PDCCH mapping mprime %d => %d (symbol %d re %d) -> (%d,%d)\n",mprime,tti_offset,symbol_offset,re_offset-frame_parms->ofdm_symbol_size+4,*(short *)&wbar[0][mprime],
                     *(1+(short *)&wbar[0][mprime]));
#endif
              mprime++;
            }
          }
        }

        if (mprime>=Msymb2)
          return(num_pdcch_symbols);
      } // check_phich_reg
    } //lprime loop

    re_offset++;

    if (re_offset == (frame_parms->ofdm_symbol_size))
      re_offset = 1;
  } // kprime loop

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCCH_TX,0);
  return(num_pdcch_symbols);
}


