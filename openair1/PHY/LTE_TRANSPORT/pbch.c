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

/*! \file PHY/LTE_TRANSPORT/pbch.c
* \brief Top-level routines for generating and decoding  the PBCH/BCH physical/transport channel V8.6 2009-03, V14.1 (FeMBMS)
* \author R. Knopp, F. Kaltenberger, J. Morgade
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger.fr, javier.morgade@ieee.org
* \note
* \warning
*/
#include "PHY/defs_eNB.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "transport_eNB.h"
#include "transport_proto.h"
#include "PHY/phy_extern.h"
#include "PHY/sse_intrin.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"

//#define DEBUG_PBCH 1
//#define DEBUG_PBCH_ENCODING
//#define INTERFERENCE_MITIGATION 1


#define PBCH_A 24

int allocate_pbch_REs_in_RB(LTE_DL_FRAME_PARMS *frame_parms,
                            int32_t **txdataF,
                            uint32_t *jj,
                            uint16_t re_offset,
                            uint32_t symbol_offset,
                            uint8_t *x0,
                            uint8_t pilots,
                            int16_t amp,
                            uint32_t *re_allocated)
{

  MIMO_mode_t mimo_mode   = (frame_parms->nb_antenna_ports_eNB==1)?SISO:ALAMOUTI;


  uint32_t tti_offset;
  uint8_t re;
  int16_t gain_lin_QPSK;
  int16_t re_off=re_offset;

  uint8_t first_re,last_re;
  int32_t tmp_sample1,tmp_sample2;

  gain_lin_QPSK = (int16_t)((amp*ONE_OVER_SQRT2_Q15)>>15);

  first_re=0;
  last_re=12;


  for (re=first_re; re<last_re; re++) {

    tti_offset = symbol_offset + re_off + re;

    // check that RE is not from Cell-specific RS

    if (is_not_pilot(pilots,re,frame_parms->nushift,0)==1) {
      //      printf("re %d (jj %d)\n",re,*jj);
      if (mimo_mode == SISO) {  //SISO mapping
        *re_allocated = *re_allocated + 1;

        //    printf("%d(%d) : %d,%d => ",tti_offset,*jj,((int16_t*)&txdataF[0][tti_offset])[0],((int16_t*)&txdataF[0][tti_offset])[1]);
        ((int16_t*)&txdataF[0][tti_offset])[0] += (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //I //b_i

        *jj = *jj + 1;

        ((int16_t*)&txdataF[0][tti_offset])[1] += (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //Q //b_{i+1}

        *jj = *jj + 1;
      } else if (mimo_mode == ALAMOUTI) {
        *re_allocated = *re_allocated + 1;

        ((int16_t*)&tmp_sample1)[0] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
        *jj=*jj+1;
        ((int16_t*)&tmp_sample1)[1] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
        *jj=*jj+1;

        // second antenna position n -> -x1*

        ((int16_t*)&tmp_sample2)[0] = (x0[*jj]==1) ? (gain_lin_QPSK) : -gain_lin_QPSK;
        *jj=*jj+1;
        ((int16_t*)&tmp_sample2)[1] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
        *jj=*jj+1;

        // normalization for 2 tx antennas
        ((int16_t*)&txdataF[0][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample1)[0]*ONE_OVER_SQRT2_Q15)>>15);
        ((int16_t*)&txdataF[0][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample1)[1]*ONE_OVER_SQRT2_Q15)>>15);
        ((int16_t*)&txdataF[1][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample2)[0]*ONE_OVER_SQRT2_Q15)>>15);
        ((int16_t*)&txdataF[1][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample2)[1]*ONE_OVER_SQRT2_Q15)>>15);

        // fill in the rest of the ALAMOUTI precoding
        if (is_not_pilot(pilots,re + 1,frame_parms->nushift,0)==1) {
          ((int16_t *)&txdataF[0][tti_offset+1])[0] += -((int16_t *)&txdataF[1][tti_offset])[0]; //x1
          ((int16_t *)&txdataF[0][tti_offset+1])[1] += ((int16_t *)&txdataF[1][tti_offset])[1];
          ((int16_t *)&txdataF[1][tti_offset+1])[0] += ((int16_t *)&txdataF[0][tti_offset])[0];  //x0*
          ((int16_t *)&txdataF[1][tti_offset+1])[1] += -((int16_t *)&txdataF[0][tti_offset])[1];
        } else {
          ((int16_t *)&txdataF[0][tti_offset+2])[0] += -((int16_t *)&txdataF[1][tti_offset])[0]; //x1
          ((int16_t *)&txdataF[0][tti_offset+2])[1] += ((int16_t *)&txdataF[1][tti_offset])[1];
          ((int16_t *)&txdataF[1][tti_offset+2])[0] += ((int16_t *)&txdataF[0][tti_offset])[0];  //x0*
          ((int16_t *)&txdataF[1][tti_offset+2])[1] += -((int16_t *)&txdataF[0][tti_offset])[1];
        }

        re++;  // adjacent carriers are taken care of by precoding
        *re_allocated = *re_allocated + 1;

        if (is_not_pilot(pilots,re,frame_parms->nushift,0)==0) { // skip pilots
          re++;
          *re_allocated = *re_allocated + 1;
        }
      }
    }
  }

  return(0);
}

void pbch_scrambling_fembms(LTE_DL_FRAME_PARMS *frame_parms,
                     uint8_t *pbch_e,
                     uint32_t length)
{
  int i;
  uint8_t reset;
  uint32_t x1 = 0, x2 = 0, s = 0;

  reset = 1;
  // x1 is set in lte_gold_generic
  x2 = frame_parms->Nid_cell + (1<<9); //this is c_init for FeMBMS in 36.211 Sec 6.6.1
  //  msg("pbch_scrambling: Nid_cell = %d\n",x2);

  for (i=0; i<length; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      //      printf("lte_gold[%d]=%x\n",i,s);
      reset = 0;
    }

    pbch_e[i] = (pbch_e[i]&1) ^ ((s>>(i&0x1f))&1);

  }
}

int generate_pbch_fembms(LTE_eNB_PBCH *eNB_pbch,
                  int32_t **txdataF,
                  int amp,
                  LTE_DL_FRAME_PARMS *frame_parms,
                  uint8_t *pbch_pdu,
                  uint8_t frame_mod4)
{

  int i, l;

  uint32_t  pbch_D,pbch_E;//,pbch_coded_bytes;
  uint8_t pbch_a[PBCH_A>>3];
  uint8_t RCC;

  uint32_t nsymb = (frame_parms->Ncp==NORMAL) ? 14:12;
  uint32_t pilots;
#ifdef INTERFERENCE_MITIGATION
  uint32_t pilots_2;
#endif
  uint32_t second_pilot = (frame_parms->Ncp==NORMAL) ? 4 : 3;
  uint32_t jj=0;
  uint32_t re_allocated=0;
  uint32_t rb, re_offset, symbol_offset;
  uint16_t amask=0;

  pbch_D    = 16+PBCH_A;

  pbch_E  = (frame_parms->Ncp==NORMAL) ? 1920 : 1728; //RE/RB * #RB * bits/RB (QPSK)
  //  pbch_E_bytes = pbch_coded_bits>>3;

  LOG_D(PHY,"%s(eNB_pbch:%p txdataF:%p amp:%d frame_parms:%p pbch_pdu:%p frame_mod4:%d)\n", __FUNCTION__, eNB_pbch, txdataF, amp, frame_parms, pbch_pdu, frame_mod4==0);

  if (frame_mod4==0) {
    bzero(pbch_a,PBCH_A>>3);
    bzero(eNB_pbch->pbch_e,pbch_E);
    memset(eNB_pbch->pbch_d,LTE_NULL,96);
    // Encode data

    // CRC attachment
    //  crc = (uint16_t) (crc16(pbch_pdu, pbch_crc_bits-16) >> 16);

    /*
    // scramble crc with PBCH CRC mask (Table 5.3.1.1-1 of 3GPP 36.212-860)
    switch (frame_parms->nb_antenna_ports_eNB) {
    case 1:
    crc = crc ^ (uint16_t) 0;
    break;
    case 2:
    crc = crc ^ (uint16_t) 0xFFFF;
    break;
    case 4:
    crc = crc ^ (uint16_t) 0xAAAA;
    break;
    default:
    msg("[PBCH] Unknown number of TX antennas!\n");
    break;
    }
    */

    // Fix byte endian of PBCH (bit 23 goes in first)
    for (i=0; i<(PBCH_A>>3); i++)
      pbch_a[(PBCH_A>>3)-i-1] = pbch_pdu[i];

    //  pbch_data[i] = ((char*) &crc)[0];
    //  pbch_data[i+1] = ((char*) &crc)[1];
    //#ifdef DEBUG_PBCH

    for (i=0; i<(PBCH_A>>3); i++)
      LOG_D(PHY,"[PBCH] pbch_data[%d] = %x\n",i,pbch_a[i]);

    //#endif

    if (frame_parms->nb_antenna_ports_eNB == 1)
      amask = 0x0000;
    else {
      switch (frame_parms->nb_antenna_ports_eNB) {
      case 1:
        amask = 0x0000;
        break;

      case 2:
        amask = 0xffff;
        break;

      case 4:
        amask = 0x5555;
      }
    }

    ccodelte_encode(PBCH_A,2,pbch_a,eNB_pbch->pbch_d+96,amask);


#ifdef DEBUG_PBCH_ENCODING

    for (i=0; i<16+PBCH_A; i++)
      LOG_D(PHY,"%d : (%d,%d,%d)\n",i,*(eNB_pbch->pbch_d+96+(3*i)),*(eNB_pbch->pbch_d+97+(3*i)),*(eNB_pbch->pbch_d+98+(3*i)));

#endif //DEBUG_PBCH_ENCODING

    // Bit collection
    /*
      j2=0;
      for (j=0;j<pbch_crc_bits*3+12;j++) {
      if ((pbch_coded_data[j]&0x80) > 0) { // bit is to be transmitted
      pbch_coded_data2[j2++] = pbch_coded_data[j]&1;
      //Bit is repeated
      if ((pbch_coded_data[j]&0x40)>0)
      pbch_coded_data2[j2++] = pbch_coded_data[j]&1;
      }
      }

      #ifdef DEBUG_PBCH
      msg("[PBCH] rate matched bits=%d, pbch_coded_bits=%d, pbch_crc_bits=%d\n",j2,pbch_coded_bits,pbch_crc_bits);
      #endif

      #ifdef DEBUG_PBCH
      LOG_M"pbch_encoded_output2.m","pbch_encoded_out2",
      pbch_coded_data2,
      pbch_coded_bits,
      1,
      4);
      #endif //DEBUG_PBCH
    */
#ifdef DEBUG_PBCH_ENCODING
    LOG_D(PHY,"Doing PBCH interleaving for %d coded bits, e %p\n",pbch_D,eNB_pbch->pbch_e);
#endif
    RCC = sub_block_interleaving_cc(pbch_D,eNB_pbch->pbch_d+96,eNB_pbch->pbch_w);

    lte_rate_matching_cc(RCC,pbch_E,eNB_pbch->pbch_w,eNB_pbch->pbch_e);

#ifdef DEBUG_PBCH_ENCODING
    LOG_D(PHY,"PBCH_e:\n");

    for (i=0; i<pbch_E; i++)
      LOG_D(PHY,"%d %d\n",i,*(eNB_pbch->pbch_e+i));

    LOG_D(PHY,"\n");
#endif



#ifdef DEBUG_PBCH
    if (frame_mod4==0) {
      LOG_M("pbch_e.m","pbch_e",
                   eNB_pbch->pbch_e,
                   pbch_E,
                   1,
                   4);

      for (i=0; i<16; i++)
        printf("e[%d] %d\n",i,eNB_pbch->pbch_e[i]);
    }
#endif //DEBUG_PBCH
    // scrambling

    pbch_scrambling_fembms(frame_parms,
                    eNB_pbch->pbch_e,
                    pbch_E);
#ifdef DEBUG_PBCH
    if (frame_mod4==0) {
      LOG_M("pbch_e_s.m","pbch_e_s",
                   eNB_pbch->pbch_e,
                   pbch_E,
                   1,
                   4);

      for (i=0; i<16; i++)
        printf("e_s[%d] %d\n",i,eNB_pbch->pbch_e[i]);
    }
#endif //DEBUG_PBCH 
  } // frame_mod4==0

  // modulation and mapping (slot 1, symbols 0..3)
  for (l=(nsymb>>1); l<(nsymb>>1)+4; l++) {

    pilots=0;
#ifdef INTERFERENCE_MITIGATION
    pilots_2 = 0;
#endif

    if ((l==0) || (l==(nsymb>>1))) {
      pilots=1;
#ifdef INTERFERENCE_MITIGATION
      pilots_2=1;
#endif
    }

    if ((l==1) || (l==(nsymb>>1)+1)) {
      pilots=1;
    }

    if ((l==second_pilot)||(l==(second_pilot+(nsymb>>1)))) {
      pilots=1;
    }

#ifdef DEBUG_PBCH
    LOG_D(PHY,"[PBCH] l=%d, pilots=%d\n",l,pilots);
#endif


    re_offset = frame_parms->ofdm_symbol_size-3*12;
    symbol_offset = frame_parms->ofdm_symbol_size*l;

    for (rb=0; rb<6; rb++) {

#ifdef DEBUG_PBCH
      LOG_D(PHY,"RB %d, jj %d, re_offset %d, symbol_offset %d, pilots %d, nushift %d\n",rb,jj,re_offset, symbol_offset, pilots,frame_parms->nushift);
#endif
      allocate_pbch_REs_in_RB(frame_parms,
                              txdataF,
                              &jj,
                              re_offset,
                              symbol_offset,
                              &eNB_pbch->pbch_e[frame_mod4*(pbch_E>>2)],
                              pilots,
#ifdef INTERFERENCE_MITIGATION
                              (pilots_2==1)?(amp/3):amp,
#else
                              amp,
#endif
                              &re_allocated);

      re_offset+=12; // go to next RB

      // check if we crossed the symbol boundary and skip DC

      if (re_offset >= frame_parms->ofdm_symbol_size)
        re_offset=1;
    }

    //    }
  }

#ifdef DEBUG_PBCH
  printf("[PBCH] txdataF=\n");

  for (i=0; i<frame_parms->ofdm_symbol_size; i++) {
    printf("%d=>(%d,%d)",i,((short*)&txdataF[0][frame_parms->ofdm_symbol_size*(nsymb>>1)+i])[0],
           ((short*)&txdataF[0][frame_parms->ofdm_symbol_size*(nsymb>>1)+i])[1]);

    if (frame_parms->nb_antenna_ports_eNB!=1) {
      printf("(%d,%d)\n",((short*)&txdataF[1][frame_parms->ofdm_symbol_size*(nsymb>>1)+i])[0],
             ((short*)&txdataF[1][frame_parms->ofdm_symbol_size*(nsymb>>1)+i])[1]);
    } else {
      printf("\n");
    }
  }

#endif


  return(0);
}

void pbch_scrambling(LTE_DL_FRAME_PARMS *frame_parms,
                     uint8_t *pbch_e,
                     uint32_t length)
{
  int i;
  uint8_t reset;
  uint32_t x1 = 0, x2 = 0, s = 0;

  reset = 1;
  // x1 is set in lte_gold_generic
  x2 = frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.6.1
  //  msg("pbch_scrambling: Nid_cell = %d\n",x2);

  for (i=0; i<length; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      //      printf("lte_gold[%d]=%x\n",i,s);
      reset = 0;
    }

    pbch_e[i] = (pbch_e[i]&1) ^ ((s>>(i&0x1f))&1);

  }
}

//uint8_t pbch_d[96+(3*(16+PBCH_A))], pbch_w[3*3*(16+PBCH_A)],pbch_e[1920];  //one bit per byte
int generate_pbch(LTE_eNB_PBCH *eNB_pbch,
                  int32_t **txdataF,
                  int amp,
                  LTE_DL_FRAME_PARMS *frame_parms,
                  uint8_t *pbch_pdu,
                  uint8_t frame_mod4)
{

  int i, l;

  uint32_t  pbch_D,pbch_E;//,pbch_coded_bytes;
  uint8_t pbch_a[PBCH_A>>3];
  uint8_t RCC;

  uint32_t nsymb = (frame_parms->Ncp==NORMAL) ? 14:12;
  uint32_t pilots;
#ifdef INTERFERENCE_MITIGATION
  uint32_t pilots_2;
#endif
  uint32_t second_pilot = (frame_parms->Ncp==NORMAL) ? 4 : 3;
  uint32_t jj=0;
  uint32_t re_allocated=0;
  uint32_t rb, re_offset, symbol_offset;
  uint16_t amask=0;

  pbch_D    = 16+PBCH_A;

  pbch_E  = (frame_parms->Ncp==NORMAL) ? 1920 : 1728; //RE/RB * #RB * bits/RB (QPSK)
  //  pbch_E_bytes = pbch_coded_bits>>3;

  LOG_D(PHY,"%s(eNB_pbch:%p txdataF:%p amp:%d frame_parms:%p pbch_pdu:%p frame_mod4:%d)\n", __FUNCTION__, eNB_pbch, txdataF, amp, frame_parms, pbch_pdu, frame_mod4==0);

  if (frame_mod4==0) {
    bzero(pbch_a,PBCH_A>>3);
    bzero(eNB_pbch->pbch_e,pbch_E);
    memset(eNB_pbch->pbch_d,LTE_NULL,96);
    // Encode data

    // CRC attachment
    //  crc = (uint16_t) (crc16(pbch_pdu, pbch_crc_bits-16) >> 16);

    /*
    // scramble crc with PBCH CRC mask (Table 5.3.1.1-1 of 3GPP 36.212-860)
    switch (frame_parms->nb_antenna_ports_eNB) {
    case 1:
    crc = crc ^ (uint16_t) 0;
    break;
    case 2:
    crc = crc ^ (uint16_t) 0xFFFF;
    break;
    case 4:
    crc = crc ^ (uint16_t) 0xAAAA;
    break;
    default:
    msg("[PBCH] Unknown number of TX antennas!\n");
    break;
    }
    */

    // Fix byte endian of PBCH (bit 23 goes in first)
    for (i=0; i<(PBCH_A>>3); i++)
      pbch_a[(PBCH_A>>3)-i-1] = pbch_pdu[i];

    //  pbch_data[i] = ((char*) &crc)[0];
    //  pbch_data[i+1] = ((char*) &crc)[1];
    //#ifdef DEBUG_PBCH

    for (i=0; i<(PBCH_A>>3); i++)
      LOG_D(PHY,"[PBCH] pbch_data[%d] = %x\n",i,pbch_a[i]);

    //#endif

    if (frame_parms->nb_antenna_ports_eNB == 1)
      amask = 0x0000;
    else {
      switch (frame_parms->nb_antenna_ports_eNB) {
      case 1:
        amask = 0x0000;
        break;

      case 2:
        amask = 0xffff;
        break;

      case 4:
        amask = 0x5555;
      }
    }

    ccodelte_encode(PBCH_A,2,pbch_a,eNB_pbch->pbch_d+96,amask);


#ifdef DEBUG_PBCH_ENCODING

    for (i=0; i<16+PBCH_A; i++)
      LOG_D(PHY,"%d : (%d,%d,%d)\n",i,*(eNB_pbch->pbch_d+96+(3*i)),*(eNB_pbch->pbch_d+97+(3*i)),*(eNB_pbch->pbch_d+98+(3*i)));

#endif //DEBUG_PBCH_ENCODING

    // Bit collection
    /*
      j2=0;
      for (j=0;j<pbch_crc_bits*3+12;j++) {
      if ((pbch_coded_data[j]&0x80) > 0) { // bit is to be transmitted
      pbch_coded_data2[j2++] = pbch_coded_data[j]&1;
      //Bit is repeated
      if ((pbch_coded_data[j]&0x40)>0)
      pbch_coded_data2[j2++] = pbch_coded_data[j]&1;
      }
      }

      #ifdef DEBUG_PBCH
      msg("[PBCH] rate matched bits=%d, pbch_coded_bits=%d, pbch_crc_bits=%d\n",j2,pbch_coded_bits,pbch_crc_bits);
      #endif

      #ifdef DEBUG_PBCH
      LOG_M"pbch_encoded_output2.m","pbch_encoded_out2",
      pbch_coded_data2,
      pbch_coded_bits,
      1,
      4);
      #endif //DEBUG_PBCH
    */
#ifdef DEBUG_PBCH_ENCODING
    LOG_D(PHY,"Doing PBCH interleaving for %d coded bits, e %p\n",pbch_D,eNB_pbch->pbch_e);
#endif
    RCC = sub_block_interleaving_cc(pbch_D,eNB_pbch->pbch_d+96,eNB_pbch->pbch_w);

    lte_rate_matching_cc(RCC,pbch_E,eNB_pbch->pbch_w,eNB_pbch->pbch_e);

#ifdef DEBUG_PBCH_ENCODING
    LOG_D(PHY,"PBCH_e:\n");

    for (i=0; i<pbch_E; i++)
      LOG_D(PHY,"%d %d\n",i,*(eNB_pbch->pbch_e+i));

    LOG_D(PHY,"\n");
#endif



#ifdef DEBUG_PBCH
    if (frame_mod4==0) {
      LOG_M("pbch_e.m","pbch_e",
                   eNB_pbch->pbch_e,
                   pbch_E,
                   1,
                   4);

      for (i=0; i<16; i++)
        printf("e[%d] %d\n",i,eNB_pbch->pbch_e[i]);
    }
#endif //DEBUG_PBCH
    // scrambling

    pbch_scrambling(frame_parms,
                    eNB_pbch->pbch_e,
                    pbch_E);
#ifdef DEBUG_PBCH
    if (frame_mod4==0) {
      LOG_M("pbch_e_s.m","pbch_e_s",
                   eNB_pbch->pbch_e,
                   pbch_E,
                   1,
                   4);

      for (i=0; i<16; i++)
        printf("e_s[%d] %d\n",i,eNB_pbch->pbch_e[i]);
    }
#endif //DEBUG_PBCH 
  } // frame_mod4==0

  // modulation and mapping (slot 1, symbols 0..3)
  for (l=(nsymb>>1); l<(nsymb>>1)+4; l++) {

    pilots=0;
#ifdef INTERFERENCE_MITIGATION
    pilots_2 = 0;
#endif

    if ((l==0) || (l==(nsymb>>1))) {
      pilots=1;
#ifdef INTERFERENCE_MITIGATION
      pilots_2=1;
#endif
    }

    if ((l==1) || (l==(nsymb>>1)+1)) {
      pilots=1;
    }

    if ((l==second_pilot)||(l==(second_pilot+(nsymb>>1)))) {
      pilots=1;
    }

#ifdef DEBUG_PBCH
    LOG_D(PHY,"[PBCH] l=%d, pilots=%d\n",l,pilots);
#endif


    re_offset = frame_parms->ofdm_symbol_size-3*12;
    symbol_offset = frame_parms->ofdm_symbol_size*l;

    for (rb=0; rb<6; rb++) {

#ifdef DEBUG_PBCH
      LOG_D(PHY,"RB %d, jj %d, re_offset %d, symbol_offset %d, pilots %d, nushift %d\n",rb,jj,re_offset, symbol_offset, pilots,frame_parms->nushift);
#endif
      allocate_pbch_REs_in_RB(frame_parms,
                              txdataF,
                              &jj,
                              re_offset,
                              symbol_offset,
                              &eNB_pbch->pbch_e[frame_mod4*(pbch_E>>2)],
                              pilots,
#ifdef INTERFERENCE_MITIGATION
                              (pilots_2==1)?(amp/3):amp,
#else
                              amp,
#endif
                              &re_allocated);

      re_offset+=12; // go to next RB

      // check if we crossed the symbol boundary and skip DC

      if (re_offset >= frame_parms->ofdm_symbol_size)
        re_offset=1;
    }

    //    }
  }

#ifdef DEBUG_PBCH
  printf("[PBCH] txdataF=\n");

  for (i=0; i<frame_parms->ofdm_symbol_size; i++) {
    printf("%d=>(%d,%d)",i,((short*)&txdataF[0][frame_parms->ofdm_symbol_size*(nsymb>>1)+i])[0],
           ((short*)&txdataF[0][frame_parms->ofdm_symbol_size*(nsymb>>1)+i])[1]);

    if (frame_parms->nb_antenna_ports_eNB!=1) {
      printf("(%d,%d)\n",((short*)&txdataF[1][frame_parms->ofdm_symbol_size*(nsymb>>1)+i])[0],
             ((short*)&txdataF[1][frame_parms->ofdm_symbol_size*(nsymb>>1)+i])[1]);
    } else {
      printf("\n");
    }
  }

#endif


  return(0);
}



