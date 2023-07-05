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

/*! \file PHY/LTE_TRANSPORT/dlsch_llr_computation.c
 * \brief Top-level routines for LLR computation of the PDSCH physical channel from 36-211, V8.6 2009-03
 * \author R. Knopp, F. Kaltenberger,A. Bhamri, S. Aubert, S. Wagner, X Jiang
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr,ankit.bhamri@eurecom.fr,sebastien.aubert@eurecom.fr, sebastian.wagner@eurecom.fr
 * \note
 * \warning
 */

#include "PHY/defs_UE.h"
#include "PHY/TOOLS/tools_defs.h"
#include "PHY/phy_extern_ue.h"
#include "transport_ue.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"
#include "transport_proto_ue.h"
#include "PHY/sse_intrin.h"

//#define DEBUG_LLR_SIC

const int16_t zeros[8] __attribute__((aligned(16))) = {0, 0, 0, 0, 0, 0, 0, 0};
const int16_t ones[8] __attribute__((aligned(16))) = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
//==============================================================================================
// Auxiliary Makros

// calculates psi_a = psi_r*a_r + psi_i*a_i
#define prodsum_psi_a_epi16(psi_r, a_r, psi_i, a_i, psi_a) \
  tmp_result = simde_mm_mulhi_epi16(psi_r, a_r);           \
  tmp_result = simde_mm_slli_epi16(tmp_result, 1);         \
  tmp_result2 = simde_mm_mulhi_epi16(psi_i, a_i);          \
  tmp_result2 = simde_mm_slli_epi16(tmp_result2, 1);       \
  simde__m128i psi_a = simde_mm_adds_epi16(tmp_result, tmp_result2);

// calculate interference magnitude
#define interference_abs_epi16(psi, int_ch_mag, int_mag, c1, c2)             \
  tmp_result = simde_mm_cmplt_epi16(psi, int_ch_mag);                        \
  tmp_result2 = simde_mm_xor_si128(tmp_result, (*(simde__m128i *)&ones[0])); \
  tmp_result = simde_mm_and_si128(tmp_result, c1);                           \
  tmp_result2 = simde_mm_and_si128(tmp_result2, c2);                         \
  simde__m128i int_mag = simde_mm_or_si128(tmp_result, tmp_result2);

// calculate interference magnitude
// tmp_result = ones in shorts corr. to interval 2<=x<=4, tmp_result2 interval < 2, tmp_result3 interval 4<x<6 and tmp_result4 interval x>6
#define interference_abs_64qam_epi16(psi, int_ch_mag, int_two_ch_mag, int_three_ch_mag, a, c1, c3, c5, c7) \
  tmp_result = simde_mm_cmplt_epi16(psi, int_two_ch_mag);                                                  \
  tmp_result3 = simde_mm_xor_si128(tmp_result, (*(simde__m128i *)&ones[0]));                               \
  tmp_result2 = simde_mm_cmplt_epi16(psi, int_ch_mag);                                                     \
  tmp_result = simde_mm_xor_si128(tmp_result, tmp_result2);                                                \
  tmp_result4 = simde_mm_cmpgt_epi16(psi, int_three_ch_mag);                                               \
  tmp_result3 = simde_mm_xor_si128(tmp_result3, tmp_result4);                                              \
  tmp_result = simde_mm_and_si128(tmp_result, c3);                                                         \
  tmp_result2 = simde_mm_and_si128(tmp_result2, c1);                                                       \
  tmp_result3 = simde_mm_and_si128(tmp_result3, c5);                                                       \
  tmp_result4 = simde_mm_and_si128(tmp_result4, c7);                                                       \
  tmp_result = simde_mm_or_si128(tmp_result, tmp_result2);                                                 \
  tmp_result3 = simde_mm_or_si128(tmp_result3, tmp_result4);                                               \
  simde__m128i a = simde_mm_or_si128(tmp_result, tmp_result3);

// calculates a_sq = int_ch_mag*(a_r^2 + a_i^2)*scale_factor
#define square_a_epi16(a_r, a_i, int_ch_mag, scale_factor, a_sq) \
  tmp_result = simde_mm_mulhi_epi16(a_r, a_r);                   \
  tmp_result = simde_mm_slli_epi16(tmp_result, 1);               \
  tmp_result = simde_mm_mulhi_epi16(tmp_result, scale_factor);   \
  tmp_result = simde_mm_slli_epi16(tmp_result, 1);               \
  tmp_result = simde_mm_mulhi_epi16(tmp_result, int_ch_mag);     \
  tmp_result = simde_mm_slli_epi16(tmp_result, 1);               \
  tmp_result2 = simde_mm_mulhi_epi16(a_i, a_i);                  \
  tmp_result2 = simde_mm_slli_epi16(tmp_result2, 1);             \
  tmp_result2 = simde_mm_mulhi_epi16(tmp_result2, scale_factor); \
  tmp_result2 = simde_mm_slli_epi16(tmp_result2, 1);             \
  tmp_result2 = simde_mm_mulhi_epi16(tmp_result2, int_ch_mag);   \
  tmp_result2 = simde_mm_slli_epi16(tmp_result2, 1);             \
  simde__m128i a_sq = simde_mm_adds_epi16(tmp_result, tmp_result2);

// calculates a_sq = int_ch_mag*(a_r^2 + a_i^2)*scale_factor for 64-QAM
#define square_a_64qam_epi16(a_r, a_i, int_ch_mag, scale_factor, a_sq) \
  tmp_result = simde_mm_mulhi_epi16(a_r, a_r);                         \
  tmp_result = simde_mm_slli_epi16(tmp_result, 1);                     \
  tmp_result = simde_mm_mulhi_epi16(tmp_result, scale_factor);         \
  tmp_result = simde_mm_slli_epi16(tmp_result, 3);                     \
  tmp_result = simde_mm_mulhi_epi16(tmp_result, int_ch_mag);           \
  tmp_result = simde_mm_slli_epi16(tmp_result, 1);                     \
  tmp_result2 = simde_mm_mulhi_epi16(a_i, a_i);                        \
  tmp_result2 = simde_mm_slli_epi16(tmp_result2, 1);                   \
  tmp_result2 = simde_mm_mulhi_epi16(tmp_result2, scale_factor);       \
  tmp_result2 = simde_mm_slli_epi16(tmp_result2, 3);                   \
  tmp_result2 = simde_mm_mulhi_epi16(tmp_result2, int_ch_mag);         \
  tmp_result2 = simde_mm_slli_epi16(tmp_result2, 1);                   \
  simde__m128i a_sq = simde_mm_adds_epi16(tmp_result, tmp_result2);

//==============================================================================================
// SINGLE-STREAM
//==============================================================================================

//----------------------------------------------------------------------------------------------
// QPSK
//----------------------------------------------------------------------------------------------

int dlsch_qpsk_llr(LTE_DL_FRAME_PARMS *frame_parms,
                   int32_t **rxdataF_comp,
                   int16_t *dlsch_llr,
                   uint8_t symbol,
                   uint8_t first_symbol_flag,
                   uint16_t nb_rb,
                   uint16_t pbch_pss_sss_adjust,
                   uint8_t beamforming_mode)
{

  uint32_t *rxF = (uint32_t*)&rxdataF_comp[0][((int32_t)symbol*frame_parms->N_RB_DL*12)];
  uint32_t *llr32;
  int len;
  uint8_t symbol_mod = (symbol >= (7-frame_parms->Ncp))? (symbol-(7-frame_parms->Ncp)) : symbol;

  /*
  if (first_symbol_flag==1) {
    llr32 = (uint32_t*)dlsch_llr;
  } else {
    llr32 = (uint32_t*)(*llr32p);
  }*/

  llr32 = (uint32_t*)dlsch_llr;
  if (!llr32) {
    LOG_E(PHY,"dlsch_qpsk_llr: llr is null, symbol %d, llr32=%p\n",symbol, llr32);
    return(-1);
  }


  if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
    if (frame_parms->nb_antenna_ports_eNB!=1)
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
    else
      len = (nb_rb*10) - (5*pbch_pss_sss_adjust/6);
  } else if((beamforming_mode==7) && (frame_parms->Ncp==0) && (symbol==3 || symbol==6 || symbol==9 || symbol==12)){
      len = (nb_rb*9) - (3*pbch_pss_sss_adjust/4);
  } else if((beamforming_mode==7) && (frame_parms->Ncp==1) && (symbol==4 || symbol==7 || symbol==10)){
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
  } else {
    len = (nb_rb*12) - pbch_pss_sss_adjust;
  }


  LOG_D(PHY,"[p %d : symb %d / FirstSym %d / Length %d]: @LLR Buff %p, @LLR Buff(symb) %p \n",
	frame_parms->nb_antenna_ports_eNB,
	symbol,
	first_symbol_flag,
	len,
	dlsch_llr,
	llr32);


  qpsk_llr((short *)rxF,
           (short *)llr32,
           len);

  return(0);
}

void qpsk_llr(int16_t *stream0_in,
              int16_t *stream0_out,
              int length)
{
  int i;
  for (i=0; i<2*length; i++) {
    *stream0_out = *stream0_in;
    //printf("llr %d : (%d,%d)\n",i,((int16_t*)stream0_out)[0],((int16_t*)stream0_out)[1]);
    stream0_in++;
    stream0_out++;
  }

}

int32_t dlsch_qpsk_llr_SIC(LTE_DL_FRAME_PARMS *frame_parms,
                           int32_t **rxdataF_comp,
                           int32_t **sic_buffer,  //Q15
                           int32_t **rho_i,
                           short *dlsch_llr,
                           uint8_t num_pdcch_symbols,
                           uint16_t nb_rb,
                           uint8_t subframe,
                           uint16_t mod_order_0,
                           uint32_t rb_alloc)
{

  int16_t rho_amp_x0[2*frame_parms->N_RB_DL*12];
  int16_t rho_rho_amp_x0[2*frame_parms->N_RB_DL*12];
  uint16_t amp_tmp;
  uint16_t *llr16=(uint16_t*)dlsch_llr;
  int i, len,  nsymb;
  uint8_t symbol, symbol_mod;
  int len_acc=0;
  uint16_t *sic_data;
  uint16_t pbch_pss_sss_adjust;

  nsymb = (frame_parms->Ncp==0) ? 14:12;

  for (symbol=num_pdcch_symbols; symbol<nsymb; symbol++) {
    uint16_t *rxF = (uint16_t*)(&rxdataF_comp[0][((int16_t)symbol*frame_parms->N_RB_DL*12)]);
    int16_t *rho_1=(int16_t*)(&rho_i[0][((int16_t)symbol*frame_parms->N_RB_DL*12)]);
    sic_data = (uint16_t*)&sic_buffer[0][((int16_t)len_acc)];

    symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;




    if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp))) //pilots=1
      amp_tmp=0x1fff;//dlsch0->sqrt_rho_b; already taken into account
    else //pilots=0
      amp_tmp=0x1fff;//1.5*dlsch0->sqrt_rho_a; already taken into account

    if (mod_order_0==6)
      amp_tmp=amp_tmp<<1; // to compensate for >> 1 shift in modulation


    pbch_pss_sss_adjust=adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL ,&rb_alloc,2,subframe,symbol);

    if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
      if (frame_parms->nb_antenna_ports_eNB!=1)
        len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
      else
        len = (nb_rb*10) - (5*pbch_pss_sss_adjust/6);
    } else {
      len = (nb_rb*12) - pbch_pss_sss_adjust;
    }

   //  printf("dlsch_qpsk_llr: symbol %d,nb_rb %d, len %d,pbch_pss_sss_adjust %d\n",symbol,nb_rb,len,pbch_pss_sss_adjust);

    len_acc+=len; //accumulated length; this is done because in sic_buffer we have only data symbols

    multadd_complex_vector_real_scalar((int16_t *)sic_data,
                                       amp_tmp,
                                       (int16_t *)rho_amp_x0, //this is in Q13
                                       1,
                                       len);

    mult_cpx_vector((int16_t *)rho_1, //Q15
                    (int16_t *)rho_amp_x0, //Q13
                    (int16_t*)rho_rho_amp_x0,
                    len,
                    13);

#ifdef DEBUG_LLR_SIC
    LOG_M("rho_for_multipl.m","rho_for_m", rho_1,len,1,
     symbol==num_pdcch_symbols ? 15 :
     symbol==nsymb-1 ? 14 : 13);

    LOG_M("rho_rho_in_llr.m","rho2", rho_rho_amp_x0,len,1,
     symbol==num_pdcch_symbols ? 15 :
     symbol==nsymb-1 ? 14 : 13);
#endif

    sub_cpx_vector16((int16_t *)rxF,
                     (int16_t *)rho_rho_amp_x0,
                     //(int16_t *)clean_x1,
                     (int16_t *)rxF,
                     len*2);

#ifdef DEBUG_LLR_SIC
    LOG_M("rxFdata_comp1_after.m","rxF_a", rxF,len,1,1);
    LOG_M("rxF_comp1.m","rxF_1_comp", rxF,len,1,
                 symbol==num_pdcch_symbols ? 15 :
                 symbol==nsymb-1 ? 14 : 13);
#endif

    //this is for QPSK only!!!
    for (i=0; i<len*2; i++) {
      *llr16 =rxF[i];
      //printf("llr %d : (%d,%d)\n",i,((int16_t*)llr32)[0],((int16_t*)llr32)[1]);
      llr16++;
    }

  }

 // printf("dlsch_qpsk_llr_SIC: acc_len=%d\n",len_acc);

  return(0);
}


//----------------------------------------------------------------------------------------------
// 16-QAM
//----------------------------------------------------------------------------------------------

void dlsch_16qam_llr(LTE_DL_FRAME_PARMS *frame_parms,
                     int32_t **rxdataF_comp,
                     int16_t *dlsch_llr,
                     int32_t **dl_ch_mag,
                     uint8_t symbol,
                     uint8_t first_symbol_flag,
                     uint16_t nb_rb,
                     uint16_t pbch_pss_sss_adjust,
                     int16_t **llr32p,
                     uint8_t beamforming_mode)
{
  int32_t *rxF = (int32_t*)&rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  int32_t *ch_mag = (int32_t*)&dl_ch_mag[0][(symbol*frame_parms->N_RB_DL*12)];
  int32_t *llr32;

  int len;
  unsigned char symbol_mod,len_mod4=0;

  if (first_symbol_flag==1) {
    llr32 = (int32_t*)dlsch_llr;
  } else {
    llr32 = (int32_t*)*llr32p;
  }

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
    if (frame_parms->nb_antenna_ports_eNB!=1)
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
    else
      len = (nb_rb*10) - (5*pbch_pss_sss_adjust/6);
  } else if((beamforming_mode==7) && (frame_parms->Ncp==0) && (symbol==3 || symbol==6 || symbol==9 || symbol==12)){
      len = (nb_rb*9) - (3*pbch_pss_sss_adjust/4);
  } else if((beamforming_mode==7) && (frame_parms->Ncp==1) && (symbol==4 || symbol==7 || symbol==10)){
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
  } else {
    len = (nb_rb*12) - pbch_pss_sss_adjust;
  }

  // update output pointer according to number of REs in this symbol (<<2 because 4 bits per RE)
  if (first_symbol_flag == 1)
    *llr32p = dlsch_llr + (len<<2);
  else
    *llr32p += (len<<2);

 // printf("len=%d\n", len);
  len_mod4 = len&3;
 // printf("len_mod4=%d\n", len_mod4);
  len>>=2;  // length in quad words (4 REs)
 // printf("len>>=2=%d\n", len);
  len+=(len_mod4==0 ? 0 : 1);

  qam16_llr((short *)rxF,
            (short *)ch_mag,
            (short *)llr32,
            len);
 // printf ("This line in qam16_llr is %d.\n", __LINE__);

}

void qam16_llr(int16_t *stream0_in,
               int16_t *chan_magn,
               int16_t *llr,
               int length)
{
  int i;
  simde__m128i *rxF_128 = (simde__m128i*)stream0_in;
  simde__m128i *ch_mag_128 = (simde__m128i*)chan_magn;
  simde__m128i llr128[2];
  int32_t *llr32 = (int32_t*) llr;

 // printf ("This line in qam16_llr is %d.\n", __LINE__);

  for (i=0; i<length; i++) {
    simde__m128i xmm0 = simde_mm_abs_epi16(rxF_128[i]);
    xmm0 = simde_mm_subs_epi16(ch_mag_128[i], xmm0);

    // lambda_1=y_R, lambda_2=|y_R|-|h|^2, lamda_3=y_I, lambda_4=|y_I|-|h|^2
    llr128[0] = simde_mm_unpacklo_epi32(rxF_128[i],xmm0);
    llr128[1] = simde_mm_unpackhi_epi32(rxF_128[i],xmm0);
    llr32[0] = simde_mm_extract_epi32(llr128[0],0); //((uint32_t *)&llr128[0])[0];
    llr32[1] = simde_mm_extract_epi32(llr128[0],1); //((uint32_t *)&llr128[0])[0];
    llr32[2] = simde_mm_extract_epi32(llr128[0],2); //((uint32_t *)&llr128[0])[2];
    llr32[3] = simde_mm_extract_epi32(llr128[0],3); //((uint32_t *)&llr128[0])[3];
    llr32[4] = simde_mm_extract_epi32(llr128[1],0); //((uint32_t *)&llr128[1])[0];
    llr32[5] = simde_mm_extract_epi32(llr128[1],1); //((uint32_t *)&llr128[1])[1];
    llr32[6] = simde_mm_extract_epi32(llr128[1],2); //((uint32_t *)&llr128[1])[2];
    llr32[7] = simde_mm_extract_epi32(llr128[1],3); //((uint32_t *)&llr128[1])[3];
    llr32+=8;
  }
}

void dlsch_16qam_llr_SIC (LTE_DL_FRAME_PARMS *frame_parms,
                          int32_t **rxdataF_comp,
                          int32_t **sic_buffer,  //Q15
                          int32_t **rho_i,
                          int16_t *dlsch_llr,
                          uint8_t num_pdcch_symbols,
                          int32_t **dl_ch_mag,
                          uint16_t nb_rb,
                          uint8_t subframe,
                          uint16_t mod_order_0,
                          uint32_t rb_alloc)
{
  int16_t rho_amp_x0[2*frame_parms->N_RB_DL*12];
  int16_t rho_rho_amp_x0[2*frame_parms->N_RB_DL*12];
  uint16_t amp_tmp;
  uint32_t *llr32=(uint32_t*)dlsch_llr;
  int i, len,  nsymb;
  uint8_t symbol, symbol_mod;
  int len_acc=0;
  uint16_t *sic_data;
  uint16_t pbch_pss_sss_adjust;
  unsigned char len_mod4=0;
  simde__m128i llr128[2];
  simde__m128i *ch_mag;
  nsymb = (frame_parms->Ncp==0) ? 14:12;

  for (symbol=num_pdcch_symbols; symbol<nsymb; symbol++) {
    uint16_t *rxF = (uint16_t*)(&rxdataF_comp[0][((int16_t)symbol*frame_parms->N_RB_DL*12)]);
    int16_t *rho_1=(int16_t*)(&rho_i[0][((int16_t)symbol*frame_parms->N_RB_DL*12)]);
    ch_mag = (simde__m128i*)(&dl_ch_mag[0][((int16_t)symbol*frame_parms->N_RB_DL*12)]);
    sic_data = (uint16_t*)(&sic_buffer[0][((int16_t)len_acc)]);

    symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

    pbch_pss_sss_adjust=adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,&rb_alloc,4,subframe,symbol);

    if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
      amp_tmp=0x1fff;//dlsch0->sqrt_rho_b; already taken into account
      if (frame_parms->nb_antenna_ports_eNB!=1)
        len = nb_rb*8 - (2*pbch_pss_sss_adjust/3);
      else
        len = nb_rb*10 - (5*pbch_pss_sss_adjust/6);
    } else {
      amp_tmp=0x1fff;;//dlsch0->sqrt_rho_a; already taken into account
      len = nb_rb*12 - pbch_pss_sss_adjust;
    }

    if (mod_order_0==6)
      amp_tmp=amp_tmp<<1; // to compensate for >> 1 shift in modulation

    len_acc+=len;

    multadd_complex_vector_real_scalar((int16_t *)sic_data,
                                       amp_tmp,
                                       (int16_t *)rho_amp_x0, //this is in Q13
                                       1,
                                       len);

     mult_cpx_vector((int16_t *)rho_1, //Q15
                    (int16_t *)rho_amp_x0, //Q13
                    (int16_t*)rho_rho_amp_x0,
                    len,
                    13);

     sub_cpx_vector16((int16_t *)rxF,
                      (int16_t *)rho_rho_amp_x0,
                      //(int16_t *)clean_x1,
                      (int16_t *)rxF,
                      len*2);

    len_mod4 = len&3;
    len>>=2;  // length in quad words (4 REs)
    len+=(len_mod4==0 ? 0 : 1);

    for (i=0; i<len; i++) {


      simde__m128i *x1 = (simde__m128i*)rxF;//clean_x1;
//printf("%p %p %p\n", clean_x1, &clean_x1, &clean_x1[0]);
//int *a = malloc(10*sizeof(int));
//printf("%p %p\n", a, &a);
//exit(0);
      simde__m128i xmm0 = simde_mm_abs_epi16(x1[i]);
      xmm0 = simde_mm_subs_epi16(ch_mag[i],xmm0);

    // lambda_1=y_R, lambda_2=|y_R|-|h|^2, lamda_3=y_I, lambda_4=|y_I|-|h|^2
      llr128[0] = simde_mm_unpacklo_epi32(x1[i],xmm0);
      llr128[1] = simde_mm_unpackhi_epi32(x1[i],xmm0);
      llr32[0] = simde_mm_extract_epi32(llr128[0],0); //((uint32_t *)&llr128[0])[0];
      llr32[1] = simde_mm_extract_epi32(llr128[0],1); //((uint32_t *)&llr128[0])[1];
      llr32[2] = simde_mm_extract_epi32(llr128[0],2); //((uint32_t *)&llr128[0])[2];
      llr32[3] = simde_mm_extract_epi32(llr128[0],3); //((uint32_t *)&llr128[0])[3];
      llr32[4] = simde_mm_extract_epi32(llr128[1],0); //((uint32_t *)&llr128[1])[0];
      llr32[5] = simde_mm_extract_epi32(llr128[1],1); //((uint32_t *)&llr128[1])[1];
      llr32[6] = simde_mm_extract_epi32(llr128[1],2); //((uint32_t *)&llr128[1])[2];
      llr32[7] = simde_mm_extract_epi32(llr128[1],3); //((uint32_t *)&llr128[1])[3];
      llr32+=8;

    }
  }
}


//----------------------------------------------------------------------------------------------
// 64-QAM
//----------------------------------------------------------------------------------------------

void dlsch_64qam_llr(LTE_DL_FRAME_PARMS *frame_parms,
                     int32_t **rxdataF_comp,
                     int16_t *dlsch_llr,
                     int32_t **dl_ch_mag,
                     int32_t **dl_ch_magb,
                     uint8_t symbol,
                     uint8_t first_symbol_flag,
                     uint16_t nb_rb,
                     uint16_t pbch_pss_sss_adjust,
                     //int16_t **llr_save,
                     uint32_t llr_offset,
                     uint8_t beamforming_mode)
{
  int32_t *rxF = (int32_t*)&rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  int32_t *ch_mag = (int32_t*)&dl_ch_mag[0][(symbol*frame_parms->N_RB_DL*12)];
  int32_t *ch_magb = (int32_t*)&dl_ch_magb[0][(symbol*frame_parms->N_RB_DL*12)];
  int len,len2;
  unsigned char symbol_mod,len_mod4;
  short *llr;
  int16_t *llr2;

  /*
  if (first_symbol_flag==1)
    llr = dlsch_llr;
  else
    llr = *llr_save;
  */
  llr = dlsch_llr;

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
    if (frame_parms->nb_antenna_ports_eNB!=1)
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
    else
      len = (nb_rb*10) - (5*pbch_pss_sss_adjust/6);
  } else if((beamforming_mode==7) && (frame_parms->Ncp==0) && (symbol==3 || symbol==6 || symbol==9 || symbol==12)){
      len = (nb_rb*9) - (3*pbch_pss_sss_adjust/4);
  } else if((beamforming_mode==7) && (frame_parms->Ncp==1) && (symbol==4 || symbol==7 || symbol==10)){
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
  } else {
    len = (nb_rb*12) - pbch_pss_sss_adjust;
  }

  llr2 = llr;
  llr += (len*6);

  len_mod4 =len&3;
  len2=len>>2;  // length in quad words (4 REs)
  len2+=((len_mod4==0)?0:1);

  qam64_llr((short *)rxF,
           (short *)ch_mag,
           (short *)ch_magb,
           llr2,
           len2);
}


void qam64_llr(int16_t *stream0_in,
               int16_t *chan_magn,
               int16_t *chan_magn_b,
               int16_t *llr,
               int length)
{

  simde__m128i *rxF_128 = (simde__m128i*)stream0_in;
  simde__m128i *ch_mag_128 = (simde__m128i*)chan_magn;
  simde__m128i *ch_magb_128 = (simde__m128i*)chan_magn_b;
  int i;
  //int16_t *llr2;
  //llr2 = llr;

  for (i=0; i<length; i++) {
    simde__m128i xmm1 = simde_mm_abs_epi16(rxF_128[i]);
    xmm1 = simde_mm_subs_epi16(ch_mag_128[i],xmm1);
    simde__m128i xmm2 = simde_mm_abs_epi16(xmm1);
    xmm2 = simde_mm_subs_epi16(ch_magb_128[i],xmm2);
    // loop over all LLRs in quad word (24 coded bits)
    /*
      for (j=0;j<8;j+=2) {
      llr2[0] = ((short *)&rxF[i])[j];
      llr2[1] = ((short *)&rxF[i])[j+1];
      llr2[2] = ((short *)&xmm1)[j];
      llr2[3] = ((short *)&xmm1)[j+1];
      llr2[4] = ((short *)&xmm2)[j];
      llr2[5] = ((short *)&xmm2)[j+1];

     llr2+=6;
      }
    */
    llr[0] = ((short *)&rxF_128[i])[0];
    llr[1] = ((short *)&rxF_128[i])[1];
    llr[2] = simde_mm_extract_epi16(xmm1,0);
    llr[3] = simde_mm_extract_epi16(xmm1,1);//((short *)&xmm1)[j+1];
    llr[4] = simde_mm_extract_epi16(xmm2,0);//((short *)&xmm2)[j];
    llr[5] = simde_mm_extract_epi16(xmm2,1);//((short *)&xmm2)[j+1];
    llr+=6;
    llr[0] = ((short *)&rxF_128[i])[2];
    llr[1] = ((short *)&rxF_128[i])[3];
    llr[2] = simde_mm_extract_epi16(xmm1,2);
    llr[3] = simde_mm_extract_epi16(xmm1,3);//((short *)&xmm1)[j+1];
    llr[4] = simde_mm_extract_epi16(xmm2,2);//((short *)&xmm2)[j];
    llr[5] = simde_mm_extract_epi16(xmm2,3);//((short *)&xmm2)[j+1];
    llr+=6;
    llr[0] = ((short *)&rxF_128[i])[4];
    llr[1] = ((short *)&rxF_128[i])[5];
    llr[2] = simde_mm_extract_epi16(xmm1,4);
    llr[3] = simde_mm_extract_epi16(xmm1,5);//((short *)&xmm1)[j+1];
    llr[4] = simde_mm_extract_epi16(xmm2,4);//((short *)&xmm2)[j];
    llr[5] = simde_mm_extract_epi16(xmm2,5);//((short *)&xmm2)[j+1];
    llr+=6;
    llr[0] = ((short *)&rxF_128[i])[6];
    llr[1] = ((short *)&rxF_128[i])[7];
    llr[2] = simde_mm_extract_epi16(xmm1,6);
    llr[3] = simde_mm_extract_epi16(xmm1,7);//((short *)&xmm1)[j+1];
    llr[4] = simde_mm_extract_epi16(xmm2,6);//((short *)&xmm2)[j];
    llr[5] = simde_mm_extract_epi16(xmm2,7);//((short *)&xmm2)[j+1];
    llr += 6;
  }
}

void dlsch_64qam_llr_SIC(LTE_DL_FRAME_PARMS *frame_parms,
                         int32_t **rxdataF_comp,
                         int32_t **sic_buffer,  //Q15
                         int32_t **rho_i,
                         int16_t *dlsch_llr,
                         uint8_t num_pdcch_symbols,
                         int32_t **dl_ch_mag,
                         int32_t **dl_ch_magb,
                         uint16_t nb_rb,
                         uint8_t subframe,
                         uint16_t mod_order_0,
                         uint32_t rb_alloc)
{
  int16_t rho_amp_x0[2*frame_parms->N_RB_DL*12];
  int16_t rho_rho_amp_x0[2*frame_parms->N_RB_DL*12];
  uint16_t amp_tmp;
  uint16_t *llr32=(uint16_t*)dlsch_llr;
  int i, len,  nsymb, len2;
  uint8_t symbol, symbol_mod;
  int len_acc=0;
  uint16_t *sic_data;
  uint16_t pbch_pss_sss_adjust;
  unsigned char len_mod4=0;
  uint16_t *llr2;
  simde__m128i *ch_mag,*ch_magb;

  nsymb = (frame_parms->Ncp==0) ? 14:12;

  for (symbol=num_pdcch_symbols; symbol<nsymb; symbol++) {
    uint16_t *rxF = (uint16_t*)(&rxdataF_comp[0][((int16_t)symbol*frame_parms->N_RB_DL*12)]);
    int16_t *rho_1=(int16_t*)(&rho_i[0][((int16_t)symbol*frame_parms->N_RB_DL*12)]);
    ch_mag = (simde__m128i*)(&dl_ch_mag[0][((int16_t)symbol*frame_parms->N_RB_DL*12)]);
    ch_magb = (simde__m128i*)(&dl_ch_magb[0][((int16_t)symbol*frame_parms->N_RB_DL*12)]);
    sic_data = (uint16_t*)(&sic_buffer[0][((int16_t)len_acc)]);

    symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

    pbch_pss_sss_adjust=adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,&rb_alloc,6,subframe,symbol);

    if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
      amp_tmp = 0x1fff;//dlsch0->sqrt_rho_b; already taken into account
      if (frame_parms->nb_antenna_ports_eNB!=1)
        len = nb_rb*8 - (2*pbch_pss_sss_adjust/3);
      else
        len = nb_rb*10 - (5*pbch_pss_sss_adjust/6);
      } else {
        amp_tmp = 0x1fff; //dlsch0->sqrt_rho_a; already taken into account
        len = nb_rb*12 - pbch_pss_sss_adjust;
      }

    if (mod_order_0==6)
      amp_tmp=amp_tmp<<1; // to compensate for >> 1 shift in modulation

    len_acc+=len;

    multadd_complex_vector_real_scalar((int16_t *)sic_data,
                                        amp_tmp,
                                        (int16_t *)rho_amp_x0, //this is in Q13
                                        1,
                                        len);

    mult_cpx_vector((int16_t *)rho_1, //Q15
                    (int16_t *)rho_amp_x0, //Q13
                    (int16_t*)rho_rho_amp_x0,
                    len,
                    13);

    sub_cpx_vector16((int16_t *)rxF,
                      (int16_t *)rho_rho_amp_x0,
                      //(int16_t *)clean_x1,
                      (int16_t *)rxF,
                      len*2);

    llr2 = llr32;
    llr32 += (len*6);

    len_mod4 =len&3;
    len2=len>>2;  // length in quad words (4 REs)
    len2+=(len_mod4?0:1);



    for (i=0; i<len2; i++) {

      simde__m128i *x1 = (simde__m128i*)rxF;
      simde__m128i xmm1 = simde_mm_abs_epi16(x1[i]);
      xmm1 = simde_mm_subs_epi16(ch_mag[i],xmm1);
      simde__m128i xmm2 = simde_mm_abs_epi16(xmm1);
      xmm2 = simde_mm_subs_epi16(ch_magb[i],xmm2);

      // loop over all LLRs in quad word (24 coded bits)
      /*
        for (j=0;j<8;j+=2) {
        llr2[0] = ((short *)&rxF[i])[j];
        llr2[1] = ((short *)&rxF[i])[j+1];
        llr2[2] = ((short *)&xmm1)[j];
        llr2[3] = ((short *)&xmm1)[j+1];
        llr2[4] = ((short *)&xmm2)[j];
        llr2[5] = ((short *)&xmm2)[j+1];

       llr2+=6;
        }
      */
      llr2[0] = ((short *)&x1[i])[0];
      llr2[1] = ((short *)&x1[i])[1];
      llr2[2] = simde_mm_extract_epi16(xmm1,0);
      llr2[3] = simde_mm_extract_epi16(xmm1,1);//((short *)&xmm1)[j+1];
      llr2[4] = simde_mm_extract_epi16(xmm2,0);//((short *)&xmm2)[j];
      llr2[5] = simde_mm_extract_epi16(xmm2,1);//((short *)&xmm2)[j+1];


      llr2+=6;
      llr2[0] = ((short *)&x1[i])[2];
      llr2[1] = ((short *)&x1[i])[3];

      llr2[2] = simde_mm_extract_epi16(xmm1,2);
      llr2[3] = simde_mm_extract_epi16(xmm1,3);//((short *)&xmm1)[j+1];
      llr2[4] = simde_mm_extract_epi16(xmm2,2);//((short *)&xmm2)[j];
      llr2[5] = simde_mm_extract_epi16(xmm2,3);//((short *)&xmm2)[j+1];

      llr2+=6;
      llr2[0] = ((short *)&x1[i])[4];
      llr2[1] = ((short *)&x1[i])[5];

      llr2[2] = simde_mm_extract_epi16(xmm1,4);
      llr2[3] = simde_mm_extract_epi16(xmm1,5);//((short *)&xmm1)[j+1];
      llr2[4] = simde_mm_extract_epi16(xmm2,4);//((short *)&xmm2)[j];
      llr2[5] = simde_mm_extract_epi16(xmm2,5);//((short *)&xmm2)[j+1];

      llr2+=6;
      llr2[0] = ((short *)&x1[i])[6];
      llr2[1] = ((short *)&x1[i])[7];

      llr2[2] = simde_mm_extract_epi16(xmm1,6);
      llr2[3] = simde_mm_extract_epi16(xmm1,7);//((short *)&xmm1)[j+1];
      llr2[4] = simde_mm_extract_epi16(xmm2,6);//((short *)&xmm2)[j];
      llr2[5] = simde_mm_extract_epi16(xmm2,7);//((short *)&xmm2)[j+1];

      llr2 += 6;
    }
  }
}
//#endif
//==============================================================================================
// DUAL-STREAM
//==============================================================================================

//----------------------------------------------------------------------------------------------
// QPSK
//----------------------------------------------------------------------------------------------

int dlsch_qpsk_qpsk_llr(LTE_DL_FRAME_PARMS *frame_parms,
                        int **rxdataF_comp,
                        int **rxdataF_comp_i,
                        int **rho_i,
                        short *dlsch_llr,
                        unsigned char symbol,
                        unsigned char first_symbol_flag,
                        unsigned short nb_rb,
                        uint16_t pbch_pss_sss_adjust,
                        short **llr16p)
{

  int16_t *rxF=(int16_t*)&rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rxF_i=(int16_t*)&rxdataF_comp_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rho=(int16_t*)&rho_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *llr16;
  int len;
  uint8_t symbol_mod = (symbol >= (7-frame_parms->Ncp))? (symbol-(7-frame_parms->Ncp)) : symbol;

  if (first_symbol_flag == 1) {
    llr16 = (int16_t*)dlsch_llr;
  } else {
    llr16 = (int16_t*)(*llr16p);
  }

  AssertFatal(llr16!=NULL,"dlsch_qpsk_qpsk_llr: llr is null, symbol %d\n",symbol);

  if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
    // if symbol has pilots
    if (frame_parms->nb_antenna_ports_eNB!=1)
      // in 2 antenna ports we have 8 REs per symbol per RB
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
    else
      // for 1 antenna port we have 10 REs per symbol per RB
      len = (nb_rb*10) - (5*pbch_pss_sss_adjust/6);
  } else {
    // symbol has no pilots
    len = (nb_rb*12) - pbch_pss_sss_adjust;
  }

  // printf("dlsch_qpsk_qpsk_llr: symbol %d,nb_rb %d, len %d,pbch_pss_sss_adjust %d\n",symbol,nb_rb,len,pbch_pss_sss_adjust);
  //    printf("qpsk_qpsk: len %d, llr16 %p\n",len,llr16);
  qpsk_qpsk((short *)rxF,
            (short *)rxF_i,
            (short *)llr16,
            (short *)rho,
            len);

  llr16 += (len<<1);
  *llr16p = (short *)llr16;

  return(0);
}

void qpsk_qpsk(short *stream0_in,
               short *stream1_in,
               short *stream0_out,
               short *rho01,
               int length
         )
{

  /*
    This function computes the LLRs of stream 0 (s_0) in presence of the interfering stream 1 (s_1) assuming that both symbols are QPSK. It can be used for both MU-MIMO interference-aware receiver or for SU-MIMO receivers.

    Parameters:
    stream0_in = Matched filter output y0' = (h0*g0)*y0
    stream1_in = Matched filter output y1' = (h0*g1)*y0
    stream0_out = LLRs
    rho01 = Correlation between the two effective channels \rho_{10} = (h1*g1)*(h0*g0)
    length = number of resource elements
  */

  simde__m128i *rho01_128i = (simde__m128i *)rho01;
  simde__m128i *stream0_128i_in = (simde__m128i *)stream0_in;
  simde__m128i *stream1_128i_in = (simde__m128i *)stream1_in;
  simde__m128i *stream0_128i_out = (simde__m128i *)stream0_out;
  simde__m128i ONE_OVER_SQRT_8 = simde_mm_set1_epi16(23170); //round(2^16/sqrt(8))

  int i;


  for (i=0; i<length>>2; i+=2) {
    // in each iteration, we take 8 complex samples
    simde__m128i xmm0 = rho01_128i[i]; // 4 symbols
    simde__m128i xmm1 = rho01_128i[i + 1];

    // put (rho_r + rho_i)/2sqrt2 in rho_rpi
    // put (rho_r - rho_i)/2sqrt2 in rho_rmi

    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i xmm2 = simde_mm_unpacklo_epi64(xmm0, xmm1); // Re(rho)
    simde__m128i xmm3 = simde_mm_unpackhi_epi64(xmm0, xmm1); // Im(rho)
    simde__m128i rho_rpi = simde_mm_adds_epi16(xmm2, xmm3); // rho = Re(rho) + Im(rho)
    simde__m128i rho_rmi = simde_mm_subs_epi16(xmm2, xmm3); // rho* = Re(rho) - Im(rho)

    // divide by sqrt(8), no shift needed ONE_OVER_SQRT_8 = Q1.16
    rho_rpi = simde_mm_mulhi_epi16(rho_rpi,ONE_OVER_SQRT_8);
    rho_rmi = simde_mm_mulhi_epi16(rho_rmi,ONE_OVER_SQRT_8);
    // Compute LLR for first bit of stream 0

    // Compute real and imaginary parts of MF output for stream 0
    xmm0 = stream0_128i_in[i];
    xmm1 = stream0_128i_in[i+1];

    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y0r = simde_mm_unpacklo_epi64(xmm0, xmm1); // = [y0r(1),y0r(2),y0r(3),y0r(4)]
    simde__m128i y0i = simde_mm_unpackhi_epi64(xmm0, xmm1);

    simde__m128i y0r_over2 = simde_mm_srai_epi16(y0r, 1); // divide by 2
    simde__m128i y0i_over2 = simde_mm_srai_epi16(y0i, 1); // divide by 2
    // Compute real and imaginary parts of MF output for stream 1
    xmm0 = stream1_128i_in[i];
    xmm1 = stream1_128i_in[i+1];

    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y1r = simde_mm_unpacklo_epi64(xmm0, xmm1); //[y1r(1),y1r(2),y1r(3),y1r(4)]
    simde__m128i y1i = simde_mm_unpackhi_epi64(xmm0, xmm1); //[y1i(1),y1i(2),y1i(3),y1i(4)]

    simde__m128i y1r_over2 = simde_mm_srai_epi16(y1r, 1); // divide by 2
    simde__m128i y1i_over2 = simde_mm_srai_epi16(y1i, 1); // divide by 2

    // Compute the terms for the LLR of first bit

    xmm0 = simde_mm_setzero_si128(); // ZERO

    // 1 term for numerator of LLR
    xmm3 = simde_mm_subs_epi16(y1r_over2,rho_rpi);
    simde__m128i A = simde_mm_abs_epi16(xmm3); // A = |y1r/2 - rho/sqrt(8)|
    xmm2 = simde_mm_adds_epi16(A,y0i_over2); // = |y1r/2 - rho/sqrt(8)| + y0i/2
    xmm3 = simde_mm_subs_epi16(y1i_over2,rho_rmi);
    simde__m128i B = simde_mm_abs_epi16(xmm3); // B = |y1i/2 - rho*/sqrt(8)|
    simde__m128i logmax_num_re0 = simde_mm_adds_epi16(B, xmm2); // = |y1r/2 - rho/sqrt(8)|+|y1i/2 - rho*/sqrt(8)| + y0i/2

    // 2 term for numerator of LLR
    xmm3 = simde_mm_subs_epi16(y1r_over2,rho_rmi);
    simde__m128i C = simde_mm_abs_epi16(xmm3); // C = |y1r/2 - rho*/4|
    xmm2 = simde_mm_subs_epi16(C,y0i_over2); // = |y1r/2 - rho*/4| - y0i/2
    xmm3 = simde_mm_adds_epi16(y1i_over2,rho_rpi);
    simde__m128i D = simde_mm_abs_epi16(xmm3); // D = |y1i/2 + rho/4|
    xmm2 = simde_mm_adds_epi16(xmm2,D); // |y1r/2 - rho*/4| + |y1i/2 + rho/4| - y0i/2
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0,xmm2); // max, numerator done

    // 1 term for denominator of LLR
    xmm3 = simde_mm_adds_epi16(y1r_over2,rho_rmi);
    simde__m128i E = simde_mm_abs_epi16(xmm3); // E = |y1r/2 + rho*/4|
    xmm2 = simde_mm_adds_epi16(E,y0i_over2); // = |y1r/2 + rho*/4| + y0i/2
    xmm3 = simde_mm_subs_epi16(y1i_over2,rho_rpi);
    simde__m128i F = simde_mm_abs_epi16(xmm3); // F = |y1i/2 - rho/4|
    simde__m128i logmax_den_re0 = simde_mm_adds_epi16(F, xmm2); // = |y1r/2 + rho*/4| + |y1i/2 - rho/4| + y0i/2

    // 2 term for denominator of LLR
    xmm3 = simde_mm_adds_epi16(y1r_over2,rho_rpi);
    simde__m128i G = simde_mm_abs_epi16(xmm3); // G = |y1r/2 + rho/4|
    xmm2 = simde_mm_subs_epi16(G,y0i_over2); // = |y1r/2 + rho/4| - y0i/2
    xmm3 = simde_mm_adds_epi16(y1i_over2,rho_rmi);
    simde__m128i H = simde_mm_abs_epi16(xmm3); // H = |y1i/2 + rho*/4|
    xmm2 = simde_mm_adds_epi16(xmm2,H); // = |y1r/2 + rho/4| + |y1i/2 + rho*/4| - y0i/2
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0,xmm2); // max, denominator done

    // Compute the terms for the LLR of first bit

    // 1 term for nominator of LLR
    xmm2 = simde_mm_adds_epi16(A,y0r_over2);
    simde__m128i logmax_num_im0 = simde_mm_adds_epi16(B, xmm2); // = |y1r/2 - rho/4| + |y1i/2 - rho*/4| + y0r/2

    // 2 term for nominator of LLR
    xmm2 = simde_mm_subs_epi16(E,y0r_over2);
    xmm2 = simde_mm_adds_epi16(xmm2,F); // = |y1r/2 + rho*/4| + |y1i/2 - rho/4| - y0r/2

    logmax_num_im0 = simde_mm_max_epi16(logmax_num_im0,xmm2); // max, nominator done

    // 1 term for denominator of LLR
    xmm2 = simde_mm_adds_epi16(C,y0r_over2);
    simde__m128i logmax_den_im0 = simde_mm_adds_epi16(D, xmm2); // = |y1r/2 - rho*/4| + |y1i/2 + rho/4| - y0r/2

    xmm2 = simde_mm_subs_epi16(G,y0r_over2);
    xmm2 = simde_mm_adds_epi16(xmm2,H); // = |y1r/2 + rho/4| + |y1i/2 + rho*/4| - y0r/2

    logmax_den_im0 = simde_mm_max_epi16(logmax_den_im0,xmm2); // max, denominator done

    // LLR of first bit [L1(1), L1(2), L1(3), L1(4)]
    y0r = simde_mm_adds_epi16(y0r,logmax_num_re0);
    y0r = simde_mm_subs_epi16(y0r,logmax_den_re0);

    // LLR of second bit [L2(1), L2(2), L2(3), L2(4)]
    y0i = simde_mm_adds_epi16(y0i,logmax_num_im0);
    y0i = simde_mm_subs_epi16(y0i,logmax_den_im0);

    simde_mm_storeu_si128(&stream0_128i_out[i],simde_mm_unpacklo_epi16(y0r,y0i)); // = [L1(1), L2(1), L1(2), L2(2)]

    if (i<((length>>1) - 1)) // false if only 2 REs remain
      simde_mm_storeu_si128(&stream0_128i_out[i + 1], simde_mm_unpackhi_epi16(y0r, y0i));
  }
}

int dlsch_qpsk_16qam_llr(LTE_DL_FRAME_PARMS *frame_parms,
                         int32_t **rxdataF_comp,
                         int32_t **rxdataF_comp_i,
                         int32_t **dl_ch_mag_i, //|h_1|^2*(2/sqrt{10})
                         int32_t **rho_i,
                         int16_t *dlsch_llr,
                         uint8_t symbol,
                         uint8_t first_symbol_flag,
                         uint16_t nb_rb,
                         uint16_t pbch_pss_sss_adjust,
                         int16_t **llr16p)
{

  int16_t *rxF=(int16_t*)&rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rxF_i=(int16_t*)&rxdataF_comp_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *ch_mag_i = (int16_t*)&dl_ch_mag_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rho=(int16_t*)&rho_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *llr16;
  int len;
  uint8_t symbol_mod = (symbol >= (7-frame_parms->Ncp))? (symbol-(7-frame_parms->Ncp)) : symbol;

  if (first_symbol_flag == 1) {
    llr16 = (int16_t*)dlsch_llr;
  } else {
    llr16 = (int16_t*)(*llr16p);
  }

  AssertFatal(llr16!=NULL,"dlsch_qpsk_qpsk_llr: llr is null, symbol %d\n",symbol);


  if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
    // if symbol has pilots
    if (frame_parms->nb_antenna_ports_eNB!=1)
      // in 2 antenna ports we have 8 REs per symbol per RB
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
    else
      // for 1 antenna port we have 10 REs per symbol per RB
      len = (nb_rb*10) - (5*pbch_pss_sss_adjust/6);
  } else {
    // symbol has no pilots
    len = (nb_rb*12) - pbch_pss_sss_adjust;
  }

  qpsk_qam16((short *)rxF,
             (short *)rxF_i,
             (short *)ch_mag_i,
             (short *)llr16,
             (short *)rho,
             len);

  llr16 += (len<<1);
  *llr16p = (short *)llr16;

  return(0);
}

void qpsk_qam16(int16_t *stream0_in,
                int16_t *stream1_in,
                int16_t *ch_mag_i,
                int16_t *stream0_out,
                int16_t *rho01,
                int32_t length
    )
{
  /*
    This function computes the LLRs of stream 0 (s_0) in presence of the interfering stream 1 (s_1) assuming that both symbols are QPSK. It can be used for both MU-MIMO interference-aware receiver or for SU-MIMO receivers.

    Parameters:
    stream0_in = Matched filter output y0' = (h0*g0)*y0
    stream1_in = Matched filter output y1' = (h0*g1)*y0
    stream0_out = LLRs
    rho01 = Correlation between the two effective channels \rho_{10} = (h1*g1)*(h0*g0)
    length = number of resource elements
  */

  simde__m128i *rho01_128i = (simde__m128i *)rho01;
  simde__m128i *stream0_128i_in = (simde__m128i *)stream0_in;
  simde__m128i *stream1_128i_in = (simde__m128i *)stream1_in;
  simde__m128i *stream0_128i_out = (simde__m128i *)stream0_out;
  simde__m128i *ch_mag_128i_i    = (simde__m128i *)ch_mag_i;
  simde__m128i ONE_OVER_SQRT_2 = simde_mm_set1_epi16(23170); // round(1/sqrt(2)*2^15)
  simde__m128i ONE_OVER_SQRT_10_Q15 = simde_mm_set1_epi16(10362); // round(1/sqrt(10)*2^15)
  simde__m128i THREE_OVER_SQRT_10 = simde_mm_set1_epi16(31086); // round(3/sqrt(10)*2^15)
  simde__m128i SQRT_10_OVER_FOUR = simde_mm_set1_epi16(25905); // round(sqrt(10)/4*2^15)
  simde__m128i ch_mag_int;

#ifdef DEBUG_LLR
  print_shorts2("rho01_128i:\n",rho01_128i);
#endif

  int i;


  for (i=0; i<length>>2; i+=2) {
    // in each iteration, we take 8 complex samples
    simde__m128i xmm0 = rho01_128i[i]; // 4 symbols
    simde__m128i xmm1 = rho01_128i[i + 1];

    // put (rho_r + rho_i)/2sqrt2 in rho_rpi
    // put (rho_r - rho_i)/2sqrt2 in rho_rmi
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i xmm2 = simde_mm_unpacklo_epi64(xmm0, xmm1); // Re(rho)
    simde__m128i xmm3 = simde_mm_unpackhi_epi64(xmm0, xmm1); // Im(rho)
    simde__m128i rho_rpi = simde_mm_adds_epi16(xmm2, xmm3); // rho = Re(rho) + Im(rho)
    simde__m128i rho_rmi = simde_mm_subs_epi16(xmm2, xmm3); // rho* = Re(rho) - Im(rho)

    // divide by sqrt(2)
    rho_rpi = simde_mm_mulhi_epi16(rho_rpi, ONE_OVER_SQRT_2);
    rho_rmi = simde_mm_mulhi_epi16(rho_rmi, ONE_OVER_SQRT_2);
    rho_rpi = simde_mm_slli_epi16(rho_rpi,1);
    rho_rmi = simde_mm_slli_epi16(rho_rmi,1);

    // Compute LLR for first bit of stream 0

    // Compute real and imaginary parts of MF output for stream 0
    xmm0 = stream0_128i_in[i];
    xmm1 = stream0_128i_in[i+1];

    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y0r = simde_mm_unpacklo_epi64(xmm0, xmm1); // = [y0r(1),y0r(2),y0r(3),y0r(4)]
    simde__m128i y0i = simde_mm_unpackhi_epi64(xmm0, xmm1);

    // divide by sqrt(2)
    simde__m128i y0r_over2 = simde_mm_mulhi_epi16(y0r, ONE_OVER_SQRT_2);
    simde__m128i y0i_over2 = simde_mm_mulhi_epi16(y0i, ONE_OVER_SQRT_2);
    y0r_over2  = simde_mm_slli_epi16(y0r,1);
    y0i_over2  = simde_mm_slli_epi16(y0i,1);

    simde__m128i y0_p_1_1 = simde_mm_adds_epi16(y0r_over2, y0i_over2);
    simde__m128i y0_m_1_1 = simde_mm_subs_epi16(y0r_over2, y0i_over2);

    // Compute real and imaginary parts of MF output for stream 1
    xmm0 = stream1_128i_in[i];
    xmm1 = stream1_128i_in[i+1];

    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y1r = simde_mm_unpacklo_epi64(xmm0, xmm1); //[y1r(1),y1r(2),y1r(3),y1r(4)]
    simde__m128i y1i = simde_mm_unpackhi_epi64(xmm0, xmm1); //[y1i(1),y1i(2),y1i(3),y1i(4)]

    xmm0 = simde_mm_setzero_si128(); // ZERO

    // compute psi
    xmm3 = simde_mm_subs_epi16(y1r, rho_rpi);
    simde__m128i psi_r_p1_p1 = simde_mm_abs_epi16(xmm3);
    xmm3 = simde_mm_subs_epi16(y1i, rho_rmi);
    simde__m128i psi_i_p1_p1 = simde_mm_abs_epi16(xmm3);
    xmm3 = simde_mm_subs_epi16(y1r, rho_rmi);
    simde__m128i psi_r_p1_m1 = simde_mm_abs_epi16(xmm3);
    xmm3 = simde_mm_adds_epi16(y1i, rho_rpi);
    simde__m128i psi_i_p1_m1 = simde_mm_abs_epi16(xmm3);
    xmm3 = simde_mm_adds_epi16(y1r, rho_rmi);
    simde__m128i psi_r_m1_p1 = simde_mm_abs_epi16(xmm3);
    xmm3 = simde_mm_subs_epi16(y1i, rho_rpi);
    simde__m128i psi_i_m1_p1 = simde_mm_abs_epi16(xmm3);
    xmm3 = simde_mm_adds_epi16(y1r, rho_rpi);
    simde__m128i psi_r_m1_m1 = simde_mm_abs_epi16(xmm3);
    xmm3 = simde_mm_adds_epi16(y1i, rho_rmi);
    simde__m128i psi_i_m1_m1 = simde_mm_abs_epi16(xmm3);

    // Rearrange interfering channel magnitudes
    xmm2 = ch_mag_128i_i[i];
    xmm3 = ch_mag_128i_i[i+1];

    xmm2 = simde_mm_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));

    ch_mag_int = simde_mm_unpacklo_epi64(xmm2, xmm3);
    simde__m128i tmp_result, tmp_result2;
    // calculate optimal interference amplitudes
    interference_abs_epi16(psi_r_p1_p1 , ch_mag_int, a_r_p1_p1 , ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_p1 , ch_mag_int, a_i_p1_p1 , ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_m1 , ch_mag_int, a_r_p1_m1 , ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_m1 , ch_mag_int, a_i_p1_m1 , ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_p1 , ch_mag_int, a_r_m1_p1 , ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_p1 , ch_mag_int, a_i_m1_p1 , ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_m1 , ch_mag_int, a_r_m1_m1 , ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_m1 , ch_mag_int, a_i_m1_m1 , ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);

    // prodsum
    prodsum_psi_a_epi16(psi_r_p1_p1, a_r_p1_p1, psi_i_p1_p1, a_i_p1_p1, psi_a_p1_p1);
    prodsum_psi_a_epi16(psi_r_p1_m1, a_r_p1_m1, psi_i_p1_m1, a_i_p1_m1, psi_a_p1_m1);
    prodsum_psi_a_epi16(psi_r_m1_p1, a_r_m1_p1, psi_i_m1_p1, a_i_m1_p1, psi_a_m1_p1);
    prodsum_psi_a_epi16(psi_r_m1_m1, a_r_m1_m1, psi_i_m1_m1, a_i_m1_m1, psi_a_m1_m1);

    // squares
    square_a_epi16(a_r_p1_p1, a_i_p1_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_p1);
    square_a_epi16(a_r_p1_m1, a_i_p1_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_m1);
    square_a_epi16(a_r_m1_p1, a_i_m1_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_p1);
    square_a_epi16(a_r_m1_m1, a_i_m1_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_m1);

    // Computing Metrics
    xmm0 = simde_mm_subs_epi16(psi_a_p1_p1, a_sq_p1_p1);
    simde__m128i bit_met_p1_p1 = simde_mm_adds_epi16(xmm0, y0_p_1_1);

    xmm0 = simde_mm_subs_epi16(psi_a_p1_m1, a_sq_p1_m1);
    simde__m128i bit_met_p1_m1 = simde_mm_adds_epi16(xmm0, y0_m_1_1);

    xmm0 = simde_mm_subs_epi16(psi_a_m1_p1, a_sq_m1_p1);
    simde__m128i bit_met_m1_p1 = simde_mm_subs_epi16(xmm0, y0_m_1_1);

    xmm0 = simde_mm_subs_epi16(psi_a_m1_m1, a_sq_m1_m1);
    simde__m128i bit_met_m1_m1 = simde_mm_subs_epi16(xmm0, y0_p_1_1);

    // MSB
    simde__m128i logmax_num_re0 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p1_m1); // bit=0
    simde__m128i logmax_den_re0 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m1_m1); // bit=1

    y0r = simde_mm_subs_epi16(logmax_num_re0,logmax_den_re0);

    // LSB
    simde__m128i logmax_num_im0 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_m1_p1); // bit=0
    simde__m128i logmax_den_im0 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_m1_m1); // bit=1

    y0i = simde_mm_subs_epi16(logmax_num_im0,logmax_den_im0);

    stream0_128i_out[i] = simde_mm_unpacklo_epi16(y0r,y0i); // = [L1(1), L2(1), L1(2), L2(2)]

    if (i<((length>>1) - 1)) // false if only 2 REs remain
      stream0_128i_out[i+1] = simde_mm_unpackhi_epi16(y0r,y0i);
  }
}

int dlsch_qpsk_64qam_llr(LTE_DL_FRAME_PARMS *frame_parms,
                         int32_t **rxdataF_comp,
                         int32_t **rxdataF_comp_i,
                         int32_t **dl_ch_mag_i, //|h_1|^2*(2/sqrt{10})
                         int32_t **rho_i,
                         int16_t *dlsch_llr,
                         uint8_t symbol,
                         uint8_t first_symbol_flag,
                         uint16_t nb_rb,
                         uint16_t pbch_pss_sss_adjust,
                         int16_t **llr16p)
{

  int16_t *rxF=(int16_t*)&rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rxF_i=(int16_t*)&rxdataF_comp_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *ch_mag_i = (int16_t*)&dl_ch_mag_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rho=(int16_t*)&rho_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *llr16;
  int len;
  uint8_t symbol_mod = (symbol >= (7-frame_parms->Ncp))? (symbol-(7-frame_parms->Ncp)) : symbol;


  if (first_symbol_flag == 1) {
    llr16 = (int16_t*)dlsch_llr;
  } else {
    llr16 = (int16_t*)(*llr16p);
  }

  AssertFatal(llr16!=NULL,"dlsch_qpsk_qam64_llr: llr is null, symbol %d\n",symbol);

  if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
    // if symbol has pilots
    if (frame_parms->nb_antenna_ports_eNB!=1)
      // in 2 antenna ports we have 8 REs per symbol per RB
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
    else
      // for 1 antenna port we have 10 REs per symbol per RB
      len = (nb_rb*10) - (5*pbch_pss_sss_adjust/6);
  } else {
    // symbol has no pilots
    len = (nb_rb*12) - pbch_pss_sss_adjust;
  }

  qpsk_qam64((short *)rxF,
             (short *)rxF_i,
             (short *)ch_mag_i,
             (short *)llr16,
             (short *)rho,
             len);

  llr16 += (len<<1);
  *llr16p = (short *)llr16;

  return(0);
}
void qpsk_qam64(short *stream0_in,
                short *stream1_in,
                short *ch_mag_i,
                short *stream0_out,
                short *rho01,
                int length
    )
{

  /*
    This function computes the LLRs of stream 0 (s_0) in presence of the interfering stream 1 (s_1) assuming that both symbols are QPSK. It can be used for both MU-MIMO interference-aware receiver or for SU-MIMO receivers.

    Parameters:
    stream0_in = Matched filter output y0' = (h0*g0)*y0
    stream1_in = Matched filter output y1' = (h0*g1)*y0
    stream0_out = LLRs
    rho01 = Correlation between the two effective channels \rho_{10} = (h1*g1)*(h0*g0)
    length = number of resource elements
  */

  simde__m128i *rho01_128i = (simde__m128i *)rho01;
  simde__m128i *stream0_128i_in = (simde__m128i *)stream0_in;
  simde__m128i *stream1_128i_in = (simde__m128i *)stream1_in;
  simde__m128i *stream0_128i_out = (simde__m128i *)stream0_out;
  simde__m128i *ch_mag_128i_i    = (simde__m128i *)ch_mag_i;
  simde__m128i ONE_OVER_SQRT_2 = simde_mm_set1_epi16(23170); // round(1/sqrt(2)*2^15)
  simde__m128i ONE_OVER_SQRT_2_42 = simde_mm_set1_epi16(3575); // round(1/sqrt(2*42)*2^15)
  simde__m128i THREE_OVER_SQRT_2_42 = simde_mm_set1_epi16(10726); // round(3/sqrt(2*42)*2^15)
  simde__m128i FIVE_OVER_SQRT_2_42 = simde_mm_set1_epi16(17876); // round(5/sqrt(2*42)*2^15)
  simde__m128i SEVEN_OVER_SQRT_2_42 = simde_mm_set1_epi16(25027); // round(7/sqrt(2*42)*2^15)
  simde__m128i SQRT_42_OVER_FOUR = simde_mm_set1_epi16(13272); // round(sqrt(42)/4*2^13), Q3.1
  simde__m128i ch_mag_int;
  simde__m128i ch_mag_int_with_sigma2;
  simde__m128i two_ch_mag_int_with_sigma2;
  simde__m128i three_ch_mag_int_with_sigma2;


#ifdef DEBUG_LLR
  print_shorts2("rho01_128i:\n",rho01_128i);
#endif

  int i;


  for (i=0; i<length>>2; i+=2) {
    // in each iteration, we take 8 complex samples


    simde__m128i xmm0 = rho01_128i[i]; // 4 symbols
    simde__m128i xmm1 = rho01_128i[i + 1];

    // put (rho_r + rho_i)/sqrt2 in rho_rpi
    // put (rho_r - rho_i)/sqrt2 in rho_rmi
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i xmm2 = simde_mm_unpacklo_epi64(xmm0, xmm1); // Re(rho)
    simde__m128i xmm3 = simde_mm_unpackhi_epi64(xmm0, xmm1); // Im(rho)
    simde__m128i rho_rpi = simde_mm_adds_epi16(xmm2, xmm3); // rho = Re(rho) + Im(rho)
    simde__m128i rho_rmi = simde_mm_subs_epi16(xmm2, xmm3); // rho* = Re(rho) - Im(rho)

    // divide by sqrt(2)
    rho_rpi = simde_mm_mulhi_epi16(rho_rpi, ONE_OVER_SQRT_2);
    rho_rmi = simde_mm_mulhi_epi16(rho_rmi, ONE_OVER_SQRT_2);
    rho_rpi = simde_mm_slli_epi16(rho_rpi,1);
    rho_rmi = simde_mm_slli_epi16(rho_rmi,1);

    // Compute LLR for first bit of stream 0

    // Compute real and imaginary parts of MF output for stream 0
    xmm0 = stream0_128i_in[i];
    xmm1 = stream0_128i_in[i+1];

    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y0r = simde_mm_unpacklo_epi64(xmm0, xmm1); // = [y0r(1),y0r(2),y0r(3),y0r(4)]
    simde__m128i y0i = simde_mm_unpackhi_epi64(xmm0, xmm1);

    // divide by sqrt(2)
    simde__m128i y0r_over2 = simde_mm_mulhi_epi16(y0r, ONE_OVER_SQRT_2);
    simde__m128i y0i_over2 = simde_mm_mulhi_epi16(y0i, ONE_OVER_SQRT_2);
    y0r_over2  = simde_mm_slli_epi16(y0r,1);
    y0i_over2  = simde_mm_slli_epi16(y0i,1);

    simde__m128i y0_p_1_1 = simde_mm_adds_epi16(y0r_over2, y0i_over2);
    simde__m128i y0_m_1_1 = simde_mm_subs_epi16(y0r_over2, y0i_over2);

    // Compute real and imaginary parts of MF output for stream 1
    xmm0 = stream1_128i_in[i];
    xmm1 = stream1_128i_in[i+1];

    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y1r = simde_mm_unpacklo_epi64(xmm0, xmm1); //[y1r(1),y1r(2),y1r(3),y1r(4)]
    simde__m128i y1i = simde_mm_unpackhi_epi64(xmm0, xmm1); //[y1i(1),y1i(2),y1i(3),y1i(4)]

    xmm0 = simde_mm_setzero_si128(); // ZERO

    // compute psi
    xmm3 = simde_mm_subs_epi16(y1r, rho_rpi);
    simde__m128i psi_r_p1_p1 = simde_mm_abs_epi16(xmm3);
    xmm3 = simde_mm_subs_epi16(y1i, rho_rmi);
    simde__m128i psi_i_p1_p1 = simde_mm_abs_epi16(xmm3);
    xmm3 = simde_mm_subs_epi16(y1r, rho_rmi);
    simde__m128i psi_r_p1_m1 = simde_mm_abs_epi16(xmm3);
    xmm3 = simde_mm_adds_epi16(y1i, rho_rpi);
    simde__m128i psi_i_p1_m1 = simde_mm_abs_epi16(xmm3);
    xmm3 = simde_mm_adds_epi16(y1r, rho_rmi);
    simde__m128i psi_r_m1_p1 = simde_mm_abs_epi16(xmm3);
    xmm3 = simde_mm_subs_epi16(y1i, rho_rpi);
    simde__m128i psi_i_m1_p1 = simde_mm_abs_epi16(xmm3);
    xmm3 = simde_mm_adds_epi16(y1r, rho_rpi);
    simde__m128i psi_r_m1_m1 = simde_mm_abs_epi16(xmm3);
    xmm3 = simde_mm_adds_epi16(y1i, rho_rmi);
    simde__m128i psi_i_m1_m1 = simde_mm_abs_epi16(xmm3);

    // Rearrange interfering channel magnitudes
    xmm2 = ch_mag_128i_i[i];
    xmm3 = ch_mag_128i_i[i+1];

    xmm2 = simde_mm_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));

    ch_mag_int = simde_mm_unpacklo_epi64(xmm2,xmm3);
    ch_mag_int_with_sigma2       = simde_mm_srai_epi16(ch_mag_int, 1); // *2
    two_ch_mag_int_with_sigma2   = ch_mag_int; // *4
    three_ch_mag_int_with_sigma2 = simde_mm_adds_epi16(ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2); // *6
    simde__m128i tmp_result, tmp_result2, tmp_result3, tmp_result4;
    interference_abs_64qam_epi16(psi_r_p1_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);

    // prodsum
    prodsum_psi_a_epi16(psi_r_p1_p1, a_r_p1_p1, psi_i_p1_p1, a_i_p1_p1, psi_a_p1_p1);
    prodsum_psi_a_epi16(psi_r_p1_m1, a_r_p1_m1, psi_i_p1_m1, a_i_p1_m1, psi_a_p1_m1);
    prodsum_psi_a_epi16(psi_r_m1_p1, a_r_m1_p1, psi_i_m1_p1, a_i_m1_p1, psi_a_m1_p1);
    prodsum_psi_a_epi16(psi_r_m1_m1, a_r_m1_m1, psi_i_m1_m1, a_i_m1_m1, psi_a_m1_m1);

    // Multiply by sqrt(2)
    psi_a_p1_p1 = simde_mm_mulhi_epi16(psi_a_p1_p1, ONE_OVER_SQRT_2);
    psi_a_p1_p1 = simde_mm_slli_epi16(psi_a_p1_p1, 2);
    psi_a_p1_m1 = simde_mm_mulhi_epi16(psi_a_p1_m1, ONE_OVER_SQRT_2);
    psi_a_p1_m1 = simde_mm_slli_epi16(psi_a_p1_m1, 2);
    psi_a_m1_p1 = simde_mm_mulhi_epi16(psi_a_m1_p1, ONE_OVER_SQRT_2);
    psi_a_m1_p1 = simde_mm_slli_epi16(psi_a_m1_p1, 2);
    psi_a_m1_m1 = simde_mm_mulhi_epi16(psi_a_m1_m1, ONE_OVER_SQRT_2);
    psi_a_m1_m1 = simde_mm_slli_epi16(psi_a_m1_m1, 2);

    square_a_64qam_epi16(a_r_p1_p1, a_i_p1_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_p1);
    square_a_64qam_epi16(a_r_p1_m1, a_i_p1_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_m1);
    square_a_64qam_epi16(a_r_m1_p1, a_i_m1_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_p1);
    square_a_64qam_epi16(a_r_m1_m1, a_i_m1_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_m1);

    // Computing Metrics
    xmm0 = simde_mm_subs_epi16(psi_a_p1_p1, a_sq_p1_p1);
    simde__m128i bit_met_p1_p1 = simde_mm_adds_epi16(xmm0, y0_p_1_1);

    xmm0 = simde_mm_subs_epi16(psi_a_p1_m1, a_sq_p1_m1);
    simde__m128i bit_met_p1_m1 = simde_mm_adds_epi16(xmm0, y0_m_1_1);

    xmm0 = simde_mm_subs_epi16(psi_a_m1_p1, a_sq_m1_p1);
    simde__m128i bit_met_m1_p1 = simde_mm_subs_epi16(xmm0, y0_m_1_1);

    xmm0 = simde_mm_subs_epi16(psi_a_m1_m1, a_sq_m1_m1);
    simde__m128i bit_met_m1_m1 = simde_mm_subs_epi16(xmm0, y0_p_1_1);

    // MSB
    simde__m128i logmax_num_re0 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p1_m1); // bit=0
    simde__m128i logmax_den_re0 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m1_m1); // bit=1

    y0r = simde_mm_subs_epi16(logmax_num_re0,logmax_den_re0);

    // LSB
    simde__m128i logmax_num_im0 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_m1_p1); // bit=0
    simde__m128i logmax_den_im0 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_m1_m1); // bit=1

    y0i = simde_mm_subs_epi16(logmax_num_im0,logmax_den_im0);

    stream0_128i_out[i] = simde_mm_unpacklo_epi16(y0r,y0i); // = [L1(1), L2(1), L1(2), L2(2)]

    if (i<((length>>1) - 1)) // false if only 2 REs remain
      stream0_128i_out[i+1] = simde_mm_unpackhi_epi16(y0r,y0i);
  }
}


//----------------------------------------------------------------------------------------------
// 16-QAM
//----------------------------------------------------------------------------------------------

void qam16_qpsk(short *stream0_in,
                short *stream1_in,
                short *ch_mag,
                short *stream0_out,
                short *rho01,
                int length
    )
{

  /*
    Author: Sebastian Wagner
    Date: 2012-06-04

    Input:
    stream0_in:  MF filter for 1st stream, i.e., y0=h0'*y
    stream!_in:  MF filter for 2nd stream, i.e., y1=h1'*y
    ch_mag:      2*h0/sqrt(00), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    ch_mag_i:    2*h1/sqrt(00), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    rho01:       Channel cross correlation, i.e., h1'*h0

    Output:
    stream0_out: output LLRs for 1st stream
  */

  simde__m128i *rho01_128i       = (simde__m128i *)rho01;
  simde__m128i *stream0_128i_in  = (simde__m128i *)stream0_in;
  simde__m128i *stream1_128i_in  = (simde__m128i *)stream1_in;
  simde__m128i *stream0_128i_out = (simde__m128i *)stream0_out;
  simde__m128i *ch_mag_128i      = (simde__m128i *)ch_mag;
  simde__m128i ONE_OVER_SQRT_2 = simde_mm_set1_epi16(23170); // round(1/sqrt(2)*2^15)
  simde__m128i ONE_OVER_SQRT_10 = simde_mm_set1_epi16(20724); // round(1/sqrt(10)*2^16)
  simde__m128i THREE_OVER_SQRT_10 = simde_mm_set1_epi16(31086); // round(3/sqrt(10)*2^15)
  simde__m128i SQRT_10_OVER_FOUR = simde_mm_set1_epi16(25905); // round(sqrt(10)/4*2^15)
  simde__m128i ONE_OVER_TWO_SQRT_10 = simde_mm_set1_epi16(10362); // round(1/2/sqrt(10)*2^16)
  simde__m128i NINE_OVER_TWO_SQRT_10 = simde_mm_set1_epi16(23315); // round(9/2/sqrt(10)*2^14)
  simde__m128i  y0r_over_sqrt10;
  simde__m128i  y0i_over_sqrt10;
  simde__m128i  y0r_three_over_sqrt10;
  simde__m128i  y0i_three_over_sqrt10;

  simde__m128i ch_mag_des;
  simde__m128i ch_mag_over_10;
  simde__m128i ch_mag_over_2;
  simde__m128i ch_mag_9_over_10;

  int i;


  for (i=0; i<length>>2; i+=2) {
    // In one iteration, we deal with 8 REs

    // Get rho
    simde__m128i xmm0 = rho01_128i[i];
    simde__m128i xmm1 = rho01_128i[i + 1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i xmm2 = simde_mm_unpacklo_epi64(xmm0, xmm1); // Re(rho)
    simde__m128i xmm3 = simde_mm_unpackhi_epi64(xmm0, xmm1); // Im(rho)
    simde__m128i rho_rpi = simde_mm_adds_epi16(xmm2, xmm3); // rho = Re(rho) + Im(rho)
    simde__m128i rho_rmi = simde_mm_subs_epi16(xmm2, xmm3); // rho* = Re(rho) - Im(rho)

    // Compute the different rhos
    simde__m128i rho_rpi_1_1 = simde_mm_mulhi_epi16(rho_rpi, ONE_OVER_SQRT_10);
    simde__m128i rho_rmi_1_1 = simde_mm_mulhi_epi16(rho_rmi, ONE_OVER_SQRT_10);
    simde__m128i rho_rpi_3_3 = simde_mm_mulhi_epi16(rho_rpi, THREE_OVER_SQRT_10);
    simde__m128i rho_rmi_3_3 = simde_mm_mulhi_epi16(rho_rmi, THREE_OVER_SQRT_10);
    rho_rpi_3_3 = simde_mm_slli_epi16(rho_rpi_3_3, 1);
    rho_rmi_3_3 = simde_mm_slli_epi16(rho_rmi_3_3, 1);

    simde__m128i xmm4 = simde_mm_mulhi_epi16(xmm2, ONE_OVER_SQRT_10); // Re(rho)
    simde__m128i xmm5 = simde_mm_mulhi_epi16(xmm3, THREE_OVER_SQRT_10); // Im(rho)
    xmm5 = simde_mm_slli_epi16(xmm5, 1);

    simde__m128i rho_rpi_1_3 = simde_mm_adds_epi16(xmm4, xmm5);
    simde__m128i rho_rmi_1_3 = simde_mm_subs_epi16(xmm4, xmm5);

    simde__m128i xmm6 = simde_mm_mulhi_epi16(xmm2, THREE_OVER_SQRT_10); // Re(rho)
    simde__m128i xmm7 = simde_mm_mulhi_epi16(xmm3, ONE_OVER_SQRT_10); // Im(rho)
    xmm6 = simde_mm_slli_epi16(xmm6, 1);

    simde__m128i rho_rpi_3_1 = simde_mm_adds_epi16(xmm6, xmm7);
    simde__m128i rho_rmi_3_1 = simde_mm_subs_epi16(xmm6, xmm7);

    // Rearrange interfering MF output
    xmm0 = stream1_128i_in[i];
    xmm1 = stream1_128i_in[i+1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y1r = simde_mm_unpacklo_epi64(xmm0, xmm1); //[y1r(1),y1r(2),y1r(3),y1r(4)]
    simde__m128i y1i = simde_mm_unpackhi_epi64(xmm0, xmm1); //[y1i(1),y1i(2),y1i(3),y1i(4)]

    xmm0 = simde_mm_setzero_si128(); // ZERO
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_1, y1r); // = [Re(rho)+ Im(rho)]/sqrt(10) - y1r
    simde__m128i psi_r_p1_p1 = simde_mm_abs_epi16(xmm2); // = |[Re(rho)+ Im(rho)]/sqrt(10) - y1r|

    xmm2 = simde_mm_subs_epi16(rho_rmi_1_1, y1r);
    simde__m128i psi_r_p1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_1, y1i);
    simde__m128i psi_i_p1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_3, y1r);
    simde__m128i psi_r_p1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_3, y1r);
    simde__m128i psi_r_p1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_1, y1i);
    simde__m128i psi_i_p1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_1, y1r);
    simde__m128i psi_r_p3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_1, y1r);
    simde__m128i psi_r_p3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_3, y1i);
    simde__m128i psi_i_p3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_3, y1r);
    simde__m128i psi_r_p3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_3, y1r);
    simde__m128i psi_r_p3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_3, y1i);
    simde__m128i psi_i_p3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_1, y1i);
    simde__m128i psi_i_m1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_1, y1i);
    simde__m128i psi_i_m1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_3, y1i);
    simde__m128i psi_i_m3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_3, y1i);
    simde__m128i psi_i_m3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_1, y1i);
    simde__m128i psi_i_p1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_1, y1i);
    simde__m128i psi_i_p1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_3, y1i);
    simde__m128i psi_i_p3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_3, y1i);
    simde__m128i psi_i_p3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_1, y1r);
    simde__m128i psi_r_m1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_3, y1r);
    simde__m128i psi_r_m1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_1, y1r);
    simde__m128i psi_r_m3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_3, y1r);
    simde__m128i psi_r_m3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1r, rho_rmi_1_1);
    simde__m128i psi_r_m1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1r, rho_rmi_1_3);
    simde__m128i psi_r_m1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1i, rho_rmi_1_1);
    simde__m128i psi_i_m1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1i, rho_rmi_3_1);
    simde__m128i psi_i_m1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1r, rho_rmi_3_1);
    simde__m128i psi_r_m3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1r, rho_rmi_3_3);
    simde__m128i psi_r_m3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1i, rho_rmi_1_3);
    simde__m128i psi_i_m3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1i, rho_rmi_3_3);
    simde__m128i psi_i_m3_m3 = simde_mm_abs_epi16(xmm2);

    // Rearrange desired MF output
    xmm0 = stream0_128i_in[i];
    xmm1 = stream0_128i_in[i+1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y0r = simde_mm_unpacklo_epi64(xmm0, xmm1); // = [y0r(1),y0r(2),y0r(3),y0r(4)]
    simde__m128i y0i = simde_mm_unpackhi_epi64(xmm0, xmm1);

    // Rearrange desired channel magnitudes
    xmm2 = ch_mag_128i[i]; // = [|h|^2(1),|h|^2(1),|h|^2(2),|h|^2(2)]*(2/sqrt(10))
    xmm3 = ch_mag_128i[i+1]; // = [|h|^2(3),|h|^2(3),|h|^2(4),|h|^2(4)]*(2/sqrt(10))
    xmm2 = simde_mm_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));

    ch_mag_des = simde_mm_unpacklo_epi64(xmm2,xmm3); // = [|h|^2(1),|h|^2(2),|h|^2(3),|h|^2(4)]*(2/sqrt(10))

    // Scale MF output of desired signal
    y0r_over_sqrt10 = simde_mm_mulhi_epi16(y0r,ONE_OVER_SQRT_10);
    y0i_over_sqrt10 = simde_mm_mulhi_epi16(y0i,ONE_OVER_SQRT_10);
    y0r_three_over_sqrt10 = simde_mm_mulhi_epi16(y0r,THREE_OVER_SQRT_10);
    y0i_three_over_sqrt10 = simde_mm_mulhi_epi16(y0i,THREE_OVER_SQRT_10);
    y0r_three_over_sqrt10 = simde_mm_slli_epi16(y0r_three_over_sqrt10,1);
    y0i_three_over_sqrt10 = simde_mm_slli_epi16(y0i_three_over_sqrt10,1);

    // Compute necessary combination of required terms
    simde__m128i y0_p_1_1 = simde_mm_adds_epi16(y0r_over_sqrt10, y0i_over_sqrt10);
    simde__m128i y0_m_1_1 = simde_mm_subs_epi16(y0r_over_sqrt10, y0i_over_sqrt10);

    simde__m128i y0_p_1_3 = simde_mm_adds_epi16(y0r_over_sqrt10, y0i_three_over_sqrt10);
    simde__m128i y0_m_1_3 = simde_mm_subs_epi16(y0r_over_sqrt10, y0i_three_over_sqrt10);

    simde__m128i y0_p_3_1 = simde_mm_adds_epi16(y0r_three_over_sqrt10, y0i_over_sqrt10);
    simde__m128i y0_m_3_1 = simde_mm_subs_epi16(y0r_three_over_sqrt10, y0i_over_sqrt10);

    simde__m128i y0_p_3_3 = simde_mm_adds_epi16(y0r_three_over_sqrt10, y0i_three_over_sqrt10);
    simde__m128i y0_m_3_3 = simde_mm_subs_epi16(y0r_three_over_sqrt10, y0i_three_over_sqrt10);

    // Add psi
    simde__m128i psi_a_p1_p1 = simde_mm_adds_epi16(psi_r_p1_p1, psi_i_p1_p1);
    simde__m128i psi_a_p1_p3 = simde_mm_adds_epi16(psi_r_p1_p3, psi_i_p1_p3);
    simde__m128i psi_a_p3_p1 = simde_mm_adds_epi16(psi_r_p3_p1, psi_i_p3_p1);
    simde__m128i psi_a_p3_p3 = simde_mm_adds_epi16(psi_r_p3_p3, psi_i_p3_p3);
    simde__m128i psi_a_p1_m1 = simde_mm_adds_epi16(psi_r_p1_m1, psi_i_p1_m1);
    simde__m128i psi_a_p1_m3 = simde_mm_adds_epi16(psi_r_p1_m3, psi_i_p1_m3);
    simde__m128i psi_a_p3_m1 = simde_mm_adds_epi16(psi_r_p3_m1, psi_i_p3_m1);
    simde__m128i psi_a_p3_m3 = simde_mm_adds_epi16(psi_r_p3_m3, psi_i_p3_m3);
    simde__m128i psi_a_m1_p1 = simde_mm_adds_epi16(psi_r_m1_p1, psi_i_m1_p1);
    simde__m128i psi_a_m1_p3 = simde_mm_adds_epi16(psi_r_m1_p3, psi_i_m1_p3);
    simde__m128i psi_a_m3_p1 = simde_mm_adds_epi16(psi_r_m3_p1, psi_i_m3_p1);
    simde__m128i psi_a_m3_p3 = simde_mm_adds_epi16(psi_r_m3_p3, psi_i_m3_p3);
    simde__m128i psi_a_m1_m1 = simde_mm_adds_epi16(psi_r_m1_m1, psi_i_m1_m1);
    simde__m128i psi_a_m1_m3 = simde_mm_adds_epi16(psi_r_m1_m3, psi_i_m1_m3);
    simde__m128i psi_a_m3_m1 = simde_mm_adds_epi16(psi_r_m3_m1, psi_i_m3_m1);
    simde__m128i psi_a_m3_m3 = simde_mm_adds_epi16(psi_r_m3_m3, psi_i_m3_m3);

    // scale by sqrt(2)
    psi_a_p1_p1 = simde_mm_mulhi_epi16(psi_a_p1_p1,ONE_OVER_SQRT_2);
    psi_a_p1_p1 = simde_mm_slli_epi16(psi_a_p1_p1,1);
    psi_a_p1_p3 = simde_mm_mulhi_epi16(psi_a_p1_p3,ONE_OVER_SQRT_2);
    psi_a_p1_p3 = simde_mm_slli_epi16(psi_a_p1_p3,1);
    psi_a_p3_p1 = simde_mm_mulhi_epi16(psi_a_p3_p1,ONE_OVER_SQRT_2);
    psi_a_p3_p1 = simde_mm_slli_epi16(psi_a_p3_p1,1);
    psi_a_p3_p3 = simde_mm_mulhi_epi16(psi_a_p3_p3,ONE_OVER_SQRT_2);
    psi_a_p3_p3 = simde_mm_slli_epi16(psi_a_p3_p3,1);

    psi_a_p1_m1 = simde_mm_mulhi_epi16(psi_a_p1_m1,ONE_OVER_SQRT_2);
    psi_a_p1_m1 = simde_mm_slli_epi16(psi_a_p1_m1,1);
    psi_a_p1_m3 = simde_mm_mulhi_epi16(psi_a_p1_m3,ONE_OVER_SQRT_2);
    psi_a_p1_m3 = simde_mm_slli_epi16(psi_a_p1_m3,1);
    psi_a_p3_m1 = simde_mm_mulhi_epi16(psi_a_p3_m1,ONE_OVER_SQRT_2);
    psi_a_p3_m1 = simde_mm_slli_epi16(psi_a_p3_m1,1);
    psi_a_p3_m3 = simde_mm_mulhi_epi16(psi_a_p3_m3,ONE_OVER_SQRT_2);
    psi_a_p3_m3 = simde_mm_slli_epi16(psi_a_p3_m3,1);

    psi_a_m1_p1 = simde_mm_mulhi_epi16(psi_a_m1_p1,ONE_OVER_SQRT_2);
    psi_a_m1_p1 = simde_mm_slli_epi16(psi_a_m1_p1,1);
    psi_a_m1_p3 = simde_mm_mulhi_epi16(psi_a_m1_p3,ONE_OVER_SQRT_2);
    psi_a_m1_p3 = simde_mm_slli_epi16(psi_a_m1_p3,1);
    psi_a_m3_p1 = simde_mm_mulhi_epi16(psi_a_m3_p1,ONE_OVER_SQRT_2);
    psi_a_m3_p1 = simde_mm_slli_epi16(psi_a_m3_p1,1);
    psi_a_m3_p3 = simde_mm_mulhi_epi16(psi_a_m3_p3,ONE_OVER_SQRT_2);
    psi_a_m3_p3 = simde_mm_slli_epi16(psi_a_m3_p3,1);

    psi_a_m1_m1 = simde_mm_mulhi_epi16(psi_a_m1_m1,ONE_OVER_SQRT_2);
    psi_a_m1_m1 = simde_mm_slli_epi16(psi_a_m1_m1,1);
    psi_a_m1_m3 = simde_mm_mulhi_epi16(psi_a_m1_m3,ONE_OVER_SQRT_2);
    psi_a_m1_m3 = simde_mm_slli_epi16(psi_a_m1_m3,1);
    psi_a_m3_m1 = simde_mm_mulhi_epi16(psi_a_m3_m1,ONE_OVER_SQRT_2);
    psi_a_m3_m1 = simde_mm_slli_epi16(psi_a_m3_m1,1);
    psi_a_m3_m3 = simde_mm_mulhi_epi16(psi_a_m3_m3,ONE_OVER_SQRT_2);
    psi_a_m3_m3 = simde_mm_slli_epi16(psi_a_m3_m3,1);

    // Computing different multiples of channel norms
    ch_mag_over_10   = simde_mm_mulhi_epi16(ch_mag_des, ONE_OVER_TWO_SQRT_10);
    ch_mag_over_2    = simde_mm_mulhi_epi16(ch_mag_des, SQRT_10_OVER_FOUR);
    ch_mag_over_2    = simde_mm_slli_epi16(ch_mag_over_2, 1);
    ch_mag_9_over_10 = simde_mm_mulhi_epi16(ch_mag_des, NINE_OVER_TWO_SQRT_10);
    ch_mag_9_over_10 = simde_mm_slli_epi16(ch_mag_9_over_10, 2);

    // Computing Metrics
    xmm1 = simde_mm_adds_epi16(psi_a_p1_p1, y0_p_1_1);
    simde__m128i bit_met_p1_p1 = simde_mm_subs_epi16(xmm1, ch_mag_over_10);

    xmm1 = simde_mm_adds_epi16(psi_a_p1_p3, y0_p_1_3);
    simde__m128i bit_met_p1_p3 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm1 = simde_mm_adds_epi16(psi_a_p1_m1, y0_m_1_1);
    simde__m128i bit_met_p1_m1 = simde_mm_subs_epi16(xmm1, ch_mag_over_10);

    xmm1 = simde_mm_adds_epi16(psi_a_p1_m3, y0_m_1_3);
    simde__m128i bit_met_p1_m3 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm1 = simde_mm_adds_epi16(psi_a_p3_p1, y0_p_3_1);
    simde__m128i bit_met_p3_p1 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm1 = simde_mm_adds_epi16(psi_a_p3_p3, y0_p_3_3);
    simde__m128i bit_met_p3_p3 = simde_mm_subs_epi16(xmm1, ch_mag_9_over_10);

    xmm1 = simde_mm_adds_epi16(psi_a_p3_m1, y0_m_3_1);
    simde__m128i bit_met_p3_m1 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm1 = simde_mm_adds_epi16(psi_a_p3_m3, y0_m_3_3);
    simde__m128i bit_met_p3_m3 = simde_mm_subs_epi16(xmm1, ch_mag_9_over_10);

    xmm1 = simde_mm_subs_epi16(psi_a_m1_p1, y0_m_1_1);
    simde__m128i bit_met_m1_p1 = simde_mm_subs_epi16(xmm1, ch_mag_over_10);

    xmm1 = simde_mm_subs_epi16(psi_a_m1_p3, y0_m_1_3);
    simde__m128i bit_met_m1_p3 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm1 = simde_mm_subs_epi16(psi_a_m1_m1, y0_p_1_1);
    simde__m128i bit_met_m1_m1 = simde_mm_subs_epi16(xmm1, ch_mag_over_10);

    xmm1 = simde_mm_subs_epi16(psi_a_m1_m3, y0_p_1_3);
    simde__m128i bit_met_m1_m3 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm1 = simde_mm_subs_epi16(psi_a_m3_p1, y0_m_3_1);
    simde__m128i bit_met_m3_p1 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm1 = simde_mm_subs_epi16(psi_a_m3_p3, y0_m_3_3);
    simde__m128i bit_met_m3_p3 = simde_mm_subs_epi16(xmm1, ch_mag_9_over_10);

    xmm1 = simde_mm_subs_epi16(psi_a_m3_m1, y0_p_3_1);
    simde__m128i bit_met_m3_m1 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm1 = simde_mm_subs_epi16(psi_a_m3_m3, y0_p_3_3);
    simde__m128i bit_met_m3_m3 = simde_mm_subs_epi16(xmm1, ch_mag_9_over_10);

    // LLR of the first bit
    // Bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m3_m1, bit_met_m3_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);

    // Bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm1 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p3_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);

    // LLR of first bit [L1(1), L1(2), L1(3), L1(4), L1(5), L1(6), L1(7), L1(8)]
    y0r = simde_mm_subs_epi16(logmax_den_re0,logmax_num_re0);

    // LLR of the second bit
    // Bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p3_m1);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_m3, bit_met_p3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_re1 = simde_mm_max_epi16(xmm4, xmm5);

    // Bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p3_p1);
    xmm1 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p3, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_re1 = simde_mm_max_epi16(xmm4, xmm5);

    // LLR of second bit [L2(1), L2(2), L2(3), L2(4)]
    y1r = simde_mm_subs_epi16(logmax_den_re1,logmax_num_re1);

    // LLR of the third bit
    // Bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm1 = simde_mm_max_epi16(bit_met_m3_m1, bit_met_m3_m3);
    xmm2 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p3_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_im0 = simde_mm_max_epi16(xmm4, xmm5);

    // Bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p1_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_im0 = simde_mm_max_epi16(xmm4, xmm5);

    // LLR of third bit [L3(1), L3(2), L3(3), L3(4)]
    y0i = simde_mm_subs_epi16(logmax_den_im0,logmax_num_im0);

    // LLR of the fourth bit
    // Bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_p1_m3, bit_met_p3_m3);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p3, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_im1 = simde_mm_max_epi16(xmm4, xmm5);

    // Bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p3_m1);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p3_p1);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_im1 = simde_mm_max_epi16(xmm4, xmm5);

    // LLR of fourth bit [L4(1), L4(2), L4(3), L4(4)]
    y1i = simde_mm_subs_epi16(logmax_den_im1,logmax_num_im1);

    // Pack LLRs in output
    // [L1(1), L2(1), L1(2), L2(2), L1(3), L2(3), L1(4), L2(4)]
    xmm0 = simde_mm_unpacklo_epi16(y0r,y1r);
    // [L1(5), L2(5), L1(6), L2(6), L1(7), L2(7), L1(8), L2(8)]
    xmm1 = simde_mm_unpackhi_epi16(y0r,y1r);
    // [L3(1), L4(1), L3(2), L4(2), L3(3), L4(3), L3(4), L4(4)]
    xmm2 = simde_mm_unpacklo_epi16(y0i,y1i);
    // [L3(5), L4(5), L3(6), L4(6), L3(7), L4(7), L3(8), L4(8)]
    xmm3 = simde_mm_unpackhi_epi16(y0i,y1i);

    stream0_128i_out[2*i+0] = simde_mm_unpacklo_epi32(xmm0,xmm2); // 8LLRs, 2REs
    stream0_128i_out[2*i+1] = simde_mm_unpackhi_epi32(xmm0,xmm2);
    stream0_128i_out[2*i+2] = simde_mm_unpacklo_epi32(xmm1,xmm3);
    stream0_128i_out[2*i+3] = simde_mm_unpackhi_epi32(xmm1,xmm3);
  }
}

int dlsch_16qam_qpsk_llr(LTE_DL_FRAME_PARMS *frame_parms,
                         int32_t **rxdataF_comp,
                         int32_t **rxdataF_comp_i,
                         int32_t **dl_ch_mag,   //|h_0|^2*(2/sqrt{10})
                         int32_t **rho_i,
                         int16_t *dlsch_llr,
                         uint8_t symbol,
                         uint8_t first_symbol_flag,
                         uint16_t nb_rb,
                         uint16_t pbch_pss_sss_adjust,
                         int16_t **llr16p)
{

  int16_t *rxF      = (int16_t*)&rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rxF_i    = (int16_t*)&rxdataF_comp_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *ch_mag   = (int16_t*)&dl_ch_mag[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rho      = (int16_t*)&rho_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *llr16;
  int len;
  uint8_t symbol_mod = (symbol >= (7-frame_parms->Ncp))? (symbol-(7-frame_parms->Ncp)) : symbol;

  // first symbol has different structure due to more pilots
  if (first_symbol_flag == 1) {
    llr16 = (int16_t*)dlsch_llr;
  } else {
    llr16 = (int16_t*)(*llr16p);
  }

  AssertFatal(llr16!=NULL,"dlsch_16qam_qpsk_llr: llr is null, symbol %d\n",symbol);

  if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
    // if symbol has pilots
    if (frame_parms->nb_antenna_ports_eNB!=1)
      // in 2 antenna ports we have 8 REs per symbol per RB
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
    else
      // for 1 antenna port we have 10 REs per symbol per RB
      len = (nb_rb*10) - (5*pbch_pss_sss_adjust/6);
  } else {
    // symbol has no pilots
    len = (nb_rb*12) - pbch_pss_sss_adjust;
  }

  // printf("symbol %d: qam16_llr, len %d (llr16 %p)\n",symbol,len,llr16);

  qam16_qpsk((short *)rxF,
             (short *)rxF_i,
             (short *)ch_mag,
             (short *)llr16,
             (short *)rho,
             len);

  llr16 += (len<<2);
  *llr16p = (short *)llr16;

  return(0);
}

void qam16_qam16(short *stream0_in,
                 short *stream1_in,
                 short *ch_mag,
                 short *ch_mag_i,
                 short *stream0_out,
                 short *rho01,
                 int length
     )
{

  /*
    Author: Sebastian Wagner
    Date: 2012-06-04

    Input:
    stream0_in:  MF filter for 1st stream, i.e., y0=h0'*y
    stream!_in:  MF filter for 2nd stream, i.e., y1=h1'*y
    ch_mag:      2*h0/sqrt(00), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    ch_mag_i:    2*h1/sqrt(00), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    rho01:       Channel cross correlation, i.e., h1'*h0

    Output:
    stream0_out: output LLRs for 1st stream
  */
  simde__m128i *rho01_128i       = (simde__m128i *)rho01;
  simde__m128i *stream0_128i_in  = (simde__m128i *)stream0_in;
  simde__m128i *stream1_128i_in  = (simde__m128i *)stream1_in;
  simde__m128i *stream0_128i_out = (simde__m128i *)stream0_out;
  simde__m128i *ch_mag_128i      = (simde__m128i *)ch_mag;
  simde__m128i *ch_mag_128i_i    = (simde__m128i *)ch_mag_i;



  simde__m128i ONE_OVER_SQRT_10 = simde_mm_set1_epi16(20724); // round(1/sqrt(10)*2^16)
  simde__m128i ONE_OVER_SQRT_10_Q15 = simde_mm_set1_epi16(10362); // round(1/sqrt(10)*2^15)
  simde__m128i THREE_OVER_SQRT_10 = simde_mm_set1_epi16(31086); // round(3/sqrt(10)*2^15)
  simde__m128i SQRT_10_OVER_FOUR = simde_mm_set1_epi16(25905); // round(sqrt(10)/4*2^15)
  simde__m128i ONE_OVER_TWO_SQRT_10 = simde_mm_set1_epi16(10362); // round(1/2/sqrt(10)*2^16)
  simde__m128i NINE_OVER_TWO_SQRT_10 = simde_mm_set1_epi16(23315); // round(9/2/sqrt(10)*2^14)
  simde__m128i ch_mag_des,ch_mag_int;
  simde__m128i  y0r_over_sqrt10;
  simde__m128i  y0i_over_sqrt10;
  simde__m128i  y0r_three_over_sqrt10;
  simde__m128i  y0i_three_over_sqrt10;
  simde__m128i ch_mag_over_10;
  simde__m128i ch_mag_over_2;
  simde__m128i ch_mag_9_over_10;

  int i;

  for (i=0; i<length>>2; i+=2) {
    // In one iteration, we deal with 8 REs

    // Get rho
    simde__m128i xmm0 = rho01_128i[i];
    simde__m128i xmm1 = rho01_128i[i + 1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i xmm2 = simde_mm_unpacklo_epi64(xmm0, xmm1); // Re(rho)
    simde__m128i xmm3 = simde_mm_unpackhi_epi64(xmm0, xmm1); // Im(rho)
    simde__m128i rho_rpi = simde_mm_adds_epi16(xmm2, xmm3); // rho = Re(rho) + Im(rho)
    simde__m128i rho_rmi = simde_mm_subs_epi16(xmm2, xmm3); // rho* = Re(rho) - Im(rho)

    // Compute the different rhos
    simde__m128i rho_rpi_1_1 = simde_mm_mulhi_epi16(rho_rpi, ONE_OVER_SQRT_10);
    simde__m128i rho_rmi_1_1 = simde_mm_mulhi_epi16(rho_rmi, ONE_OVER_SQRT_10);
    simde__m128i rho_rpi_3_3 = simde_mm_mulhi_epi16(rho_rpi, THREE_OVER_SQRT_10);
    simde__m128i rho_rmi_3_3 = simde_mm_mulhi_epi16(rho_rmi, THREE_OVER_SQRT_10);
    rho_rpi_3_3 = simde_mm_slli_epi16(rho_rpi_3_3, 1);
    rho_rmi_3_3 = simde_mm_slli_epi16(rho_rmi_3_3, 1);

    simde__m128i xmm4 = simde_mm_mulhi_epi16(xmm2, ONE_OVER_SQRT_10); // Re(rho)
    simde__m128i xmm5 = simde_mm_mulhi_epi16(xmm3, THREE_OVER_SQRT_10); // Im(rho)
    xmm5 = simde_mm_slli_epi16(xmm5, 1);

    simde__m128i rho_rpi_1_3 = simde_mm_adds_epi16(xmm4, xmm5);
    simde__m128i rho_rmi_1_3 = simde_mm_subs_epi16(xmm4, xmm5);

    simde__m128i xmm6 = simde_mm_mulhi_epi16(xmm2, THREE_OVER_SQRT_10); // Re(rho)
    simde__m128i xmm7 = simde_mm_mulhi_epi16(xmm3, ONE_OVER_SQRT_10); // Im(rho)
    xmm6 = simde_mm_slli_epi16(xmm6, 1);

    simde__m128i rho_rpi_3_1 = simde_mm_adds_epi16(xmm6, xmm7);
    simde__m128i rho_rmi_3_1 = simde_mm_subs_epi16(xmm6, xmm7);

    // Rearrange interfering MF output
    xmm0 = stream1_128i_in[i];
    xmm1 = stream1_128i_in[i+1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y1r = simde_mm_unpacklo_epi64(xmm0, xmm1); //[y1r(1),y1r(2),y1r(3),y1r(4)]
    simde__m128i y1i = simde_mm_unpackhi_epi64(xmm0, xmm1); //[y1i(1),y1i(2),y1i(3),y1i(4)]

    xmm0 = simde_mm_setzero_si128(); // ZERO
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_1, y1r); // = [Re(rho)+ Im(rho)]/sqrt(10) - y1r
    simde__m128i psi_r_p1_p1 = simde_mm_abs_epi16(xmm2); // = |[Re(rho)+ Im(rho)]/sqrt(10) - y1r|

    xmm2 = simde_mm_subs_epi16(rho_rmi_1_1, y1r);
    simde__m128i psi_r_p1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_1, y1i);
    simde__m128i psi_i_p1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_3, y1r);
    simde__m128i psi_r_p1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_3, y1r);
    simde__m128i psi_r_p1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_1, y1i);
    simde__m128i psi_i_p1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_1, y1r);
    simde__m128i psi_r_p3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_1, y1r);
    simde__m128i psi_r_p3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_3, y1i);
    simde__m128i psi_i_p3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_3, y1r);
    simde__m128i psi_r_p3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_3, y1r);
    simde__m128i psi_r_p3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_3, y1i);
    simde__m128i psi_i_p3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_1, y1i);
    simde__m128i psi_i_m1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_1, y1i);
    simde__m128i psi_i_m1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_3, y1i);
    simde__m128i psi_i_m3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_3, y1i);
    simde__m128i psi_i_m3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_1, y1i);
    simde__m128i psi_i_p1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_1, y1i);
    simde__m128i psi_i_p1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_3, y1i);
    simde__m128i psi_i_p3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_3, y1i);
    simde__m128i psi_i_p3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_1, y1r);
    simde__m128i psi_r_m1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_3, y1r);
    simde__m128i psi_r_m1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_1, y1r);
    simde__m128i psi_r_m3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_3, y1r);
    simde__m128i psi_r_m3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1r, rho_rmi_1_1);
    simde__m128i psi_r_m1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1r, rho_rmi_1_3);
    simde__m128i psi_r_m1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1i, rho_rmi_1_1);
    simde__m128i psi_i_m1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1i, rho_rmi_3_1);
    simde__m128i psi_i_m1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1r, rho_rmi_3_1);
    simde__m128i psi_r_m3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1r, rho_rmi_3_3);
    simde__m128i psi_r_m3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1i, rho_rmi_1_3);
    simde__m128i psi_i_m3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1i, rho_rmi_3_3);
    simde__m128i psi_i_m3_m3 = simde_mm_abs_epi16(xmm2);

    // Rearrange desired MF output
    xmm0 = stream0_128i_in[i];
    xmm1 = stream0_128i_in[i+1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y0r = simde_mm_unpacklo_epi64(xmm0, xmm1); // = [y0r(1),y0r(2),y0r(3),y0r(4)]
    simde__m128i y0i = simde_mm_unpackhi_epi64(xmm0, xmm1);

    // Rearrange desired channel magnitudes
    xmm2 = ch_mag_128i[i]; // = [|h|^2(1),|h|^2(1),|h|^2(2),|h|^2(2)]*(2/sqrt(10))
    xmm3 = ch_mag_128i[i+1]; // = [|h|^2(3),|h|^2(3),|h|^2(4),|h|^2(4)]*(2/sqrt(10))
    xmm2 = simde_mm_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));

    ch_mag_des = simde_mm_unpacklo_epi64(xmm2,xmm3); // = [|h|^2(1),|h|^2(2),|h|^2(3),|h|^2(4)]*(2/sqrt(10))

    // Rearrange interfering channel magnitudes
    xmm2 = ch_mag_128i_i[i];
    xmm3 = ch_mag_128i_i[i+1];

    xmm2 = simde_mm_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));

    ch_mag_int  = simde_mm_unpacklo_epi64(xmm2,xmm3);

    // Scale MF output of desired signal
    y0r_over_sqrt10 = simde_mm_mulhi_epi16(y0r,ONE_OVER_SQRT_10);
    y0i_over_sqrt10 = simde_mm_mulhi_epi16(y0i,ONE_OVER_SQRT_10);
    y0r_three_over_sqrt10 = simde_mm_mulhi_epi16(y0r,THREE_OVER_SQRT_10);
    y0i_three_over_sqrt10 = simde_mm_mulhi_epi16(y0i,THREE_OVER_SQRT_10);
    y0r_three_over_sqrt10 = simde_mm_slli_epi16(y0r_three_over_sqrt10,1);
    y0i_three_over_sqrt10 = simde_mm_slli_epi16(y0i_three_over_sqrt10,1);

    // Compute necessary combination of required terms
    simde__m128i y0_p_1_1 = simde_mm_adds_epi16(y0r_over_sqrt10, y0i_over_sqrt10);
    simde__m128i y0_m_1_1 = simde_mm_subs_epi16(y0r_over_sqrt10, y0i_over_sqrt10);

    simde__m128i y0_p_1_3 = simde_mm_adds_epi16(y0r_over_sqrt10, y0i_three_over_sqrt10);
    simde__m128i y0_m_1_3 = simde_mm_subs_epi16(y0r_over_sqrt10, y0i_three_over_sqrt10);

    simde__m128i y0_p_3_1 = simde_mm_adds_epi16(y0r_three_over_sqrt10, y0i_over_sqrt10);
    simde__m128i y0_m_3_1 = simde_mm_subs_epi16(y0r_three_over_sqrt10, y0i_over_sqrt10);

    simde__m128i y0_p_3_3 = simde_mm_adds_epi16(y0r_three_over_sqrt10, y0i_three_over_sqrt10);
    simde__m128i y0_m_3_3 = simde_mm_subs_epi16(y0r_three_over_sqrt10, y0i_three_over_sqrt10);
    simde__m128i tmp_result, tmp_result2;
    // Compute optimal interfering symbol magnitude
    interference_abs_epi16(psi_r_p1_p1 ,ch_mag_int,a_r_p1_p1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_p1 ,ch_mag_int,a_i_p1_p1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_p3 ,ch_mag_int,a_r_p1_p3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_p3 ,ch_mag_int,a_i_p1_p3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_m1 ,ch_mag_int,a_r_p1_m1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_m1 ,ch_mag_int,a_i_p1_m1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_m3 ,ch_mag_int,a_r_p1_m3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_m3 ,ch_mag_int,a_i_p1_m3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_p1 ,ch_mag_int,a_r_p3_p1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_p1 ,ch_mag_int,a_i_p3_p1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_p3 ,ch_mag_int,a_r_p3_p3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_p3 ,ch_mag_int,a_i_p3_p3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_m1 ,ch_mag_int,a_r_p3_m1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_m1 ,ch_mag_int,a_i_p3_m1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_m3 ,ch_mag_int,a_r_p3_m3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_m3 ,ch_mag_int,a_i_p3_m3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_p1 ,ch_mag_int,a_r_m1_p1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_p1 ,ch_mag_int,a_i_m1_p1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_p3 ,ch_mag_int,a_r_m1_p3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_p3 ,ch_mag_int,a_i_m1_p3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_m1 ,ch_mag_int,a_r_m1_m1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_m1 ,ch_mag_int,a_i_m1_m1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_m3 ,ch_mag_int,a_r_m1_m3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_m3 ,ch_mag_int,a_i_m1_m3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_p1 ,ch_mag_int,a_r_m3_p1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_p1 ,ch_mag_int,a_i_m3_p1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_p3 ,ch_mag_int,a_r_m3_p3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_p3 ,ch_mag_int,a_i_m3_p3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_m1 ,ch_mag_int,a_r_m3_m1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_m1 ,ch_mag_int,a_i_m3_m1,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_m3 ,ch_mag_int,a_r_m3_m3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_m3 ,ch_mag_int,a_i_m3_m3,ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);

    // Calculation of groups of two terms in the bit metric involving product of psi and interference magnitude
    prodsum_psi_a_epi16(psi_r_p1_p1,a_r_p1_p1,psi_i_p1_p1,a_i_p1_p1,psi_a_p1_p1);
    prodsum_psi_a_epi16(psi_r_p1_p3,a_r_p1_p3,psi_i_p1_p3,a_i_p1_p3,psi_a_p1_p3);
    prodsum_psi_a_epi16(psi_r_p3_p1,a_r_p3_p1,psi_i_p3_p1,a_i_p3_p1,psi_a_p3_p1);
    prodsum_psi_a_epi16(psi_r_p3_p3,a_r_p3_p3,psi_i_p3_p3,a_i_p3_p3,psi_a_p3_p3);
    prodsum_psi_a_epi16(psi_r_p1_m1,a_r_p1_m1,psi_i_p1_m1,a_i_p1_m1,psi_a_p1_m1);
    prodsum_psi_a_epi16(psi_r_p1_m3,a_r_p1_m3,psi_i_p1_m3,a_i_p1_m3,psi_a_p1_m3);
    prodsum_psi_a_epi16(psi_r_p3_m1,a_r_p3_m1,psi_i_p3_m1,a_i_p3_m1,psi_a_p3_m1);
    prodsum_psi_a_epi16(psi_r_p3_m3,a_r_p3_m3,psi_i_p3_m3,a_i_p3_m3,psi_a_p3_m3);
    prodsum_psi_a_epi16(psi_r_m1_p1,a_r_m1_p1,psi_i_m1_p1,a_i_m1_p1,psi_a_m1_p1);
    prodsum_psi_a_epi16(psi_r_m1_p3,a_r_m1_p3,psi_i_m1_p3,a_i_m1_p3,psi_a_m1_p3);
    prodsum_psi_a_epi16(psi_r_m3_p1,a_r_m3_p1,psi_i_m3_p1,a_i_m3_p1,psi_a_m3_p1);
    prodsum_psi_a_epi16(psi_r_m3_p3,a_r_m3_p3,psi_i_m3_p3,a_i_m3_p3,psi_a_m3_p3);
    prodsum_psi_a_epi16(psi_r_m1_m1,a_r_m1_m1,psi_i_m1_m1,a_i_m1_m1,psi_a_m1_m1);
    prodsum_psi_a_epi16(psi_r_m1_m3,a_r_m1_m3,psi_i_m1_m3,a_i_m1_m3,psi_a_m1_m3);
    prodsum_psi_a_epi16(psi_r_m3_m1,a_r_m3_m1,psi_i_m3_m1,a_i_m3_m1,psi_a_m3_m1);
    prodsum_psi_a_epi16(psi_r_m3_m3,a_r_m3_m3,psi_i_m3_m3,a_i_m3_m3,psi_a_m3_m3);


    // squared interference magnitude times int. ch. power
    square_a_epi16(a_r_p1_p1,a_i_p1_p1,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_p1_p1);
    square_a_epi16(a_r_p1_p3,a_i_p1_p3,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_p1_p3);
    square_a_epi16(a_r_p3_p1,a_i_p3_p1,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_p3_p1);
    square_a_epi16(a_r_p3_p3,a_i_p3_p3,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_p3_p3);
    square_a_epi16(a_r_p1_m1,a_i_p1_m1,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_p1_m1);
    square_a_epi16(a_r_p1_m3,a_i_p1_m3,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_p1_m3);
    square_a_epi16(a_r_p3_m1,a_i_p3_m1,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_p3_m1);
    square_a_epi16(a_r_p3_m3,a_i_p3_m3,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_p3_m3);
    square_a_epi16(a_r_m1_p1,a_i_m1_p1,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_m1_p1);
    square_a_epi16(a_r_m1_p3,a_i_m1_p3,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_m1_p3);
    square_a_epi16(a_r_m3_p1,a_i_m3_p1,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_m3_p1);
    square_a_epi16(a_r_m3_p3,a_i_m3_p3,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_m3_p3);
    square_a_epi16(a_r_m1_m1,a_i_m1_m1,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_m1_m1);
    square_a_epi16(a_r_m1_m3,a_i_m1_m3,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_m1_m3);
    square_a_epi16(a_r_m3_m1,a_i_m3_m1,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_m3_m1);
    square_a_epi16(a_r_m3_m3,a_i_m3_m3,ch_mag_int,SQRT_10_OVER_FOUR,a_sq_m3_m3);

    // Computing different multiples of channel norms
    ch_mag_over_10   = simde_mm_mulhi_epi16(ch_mag_des, ONE_OVER_TWO_SQRT_10);
    ch_mag_over_2    = simde_mm_mulhi_epi16(ch_mag_des, SQRT_10_OVER_FOUR);
    ch_mag_over_2    = simde_mm_slli_epi16(ch_mag_over_2, 1);
    ch_mag_9_over_10 = simde_mm_mulhi_epi16(ch_mag_des, NINE_OVER_TWO_SQRT_10);
    ch_mag_9_over_10 = simde_mm_slli_epi16(ch_mag_9_over_10, 2);

    // Computing Metrics
    xmm0 = simde_mm_subs_epi16(psi_a_p1_p1, a_sq_p1_p1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_1_1);
    simde__m128i bit_met_p1_p1 = simde_mm_subs_epi16(xmm1, ch_mag_over_10);

    xmm0 = simde_mm_subs_epi16(psi_a_p1_p3, a_sq_p1_p3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_1_3);
    simde__m128i bit_met_p1_p3 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_p1_m1, a_sq_p1_m1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_1_1);
    simde__m128i bit_met_p1_m1 = simde_mm_subs_epi16(xmm1, ch_mag_over_10);

    xmm0 = simde_mm_subs_epi16(psi_a_p1_m3, a_sq_p1_m3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_1_3);
    simde__m128i bit_met_p1_m3 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_p3_p1, a_sq_p3_p1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_3_1);
    simde__m128i bit_met_p3_p1 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_p3_p3, a_sq_p3_p3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_3_3);
    simde__m128i bit_met_p3_p3 = simde_mm_subs_epi16(xmm1, ch_mag_9_over_10);

    xmm0 = simde_mm_subs_epi16(psi_a_p3_m1, a_sq_p3_m1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_3_1);
    simde__m128i bit_met_p3_m1 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_p3_m3, a_sq_p3_m3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_3_3);
    simde__m128i bit_met_p3_m3 = simde_mm_subs_epi16(xmm1, ch_mag_9_over_10);

    xmm0 = simde_mm_subs_epi16(psi_a_m1_p1, a_sq_m1_p1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_1_1);
    simde__m128i bit_met_m1_p1 = simde_mm_subs_epi16(xmm1, ch_mag_over_10);

    xmm0 = simde_mm_subs_epi16(psi_a_m1_p3, a_sq_m1_p3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_1_3);
    simde__m128i bit_met_m1_p3 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_m1_m1, a_sq_m1_m1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_1_1);
    simde__m128i bit_met_m1_m1 = simde_mm_subs_epi16(xmm1, ch_mag_over_10);

    xmm0 = simde_mm_subs_epi16(psi_a_m1_m3, a_sq_m1_m3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_1_3);
    simde__m128i bit_met_m1_m3 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_m3_p1, a_sq_m3_p1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_3_1);
    simde__m128i bit_met_m3_p1 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_m3_p3, a_sq_m3_p3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_3_3);
    simde__m128i bit_met_m3_p3 = simde_mm_subs_epi16(xmm1, ch_mag_9_over_10);

    xmm0 = simde_mm_subs_epi16(psi_a_m3_m1, a_sq_m3_m1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_3_1);
    simde__m128i bit_met_m3_m1 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_m3_m3, a_sq_m3_m3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_3_3);
    simde__m128i bit_met_m3_m3 = simde_mm_subs_epi16(xmm1, ch_mag_9_over_10);

    // LLR of the first bit
    // Bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m3_m1, bit_met_m3_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);

    // Bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm1 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p3_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);

    // LLR of first bit [L1(1), L1(2), L1(3), L1(4), L1(5), L1(6), L1(7), L1(8)]
    y0r = simde_mm_subs_epi16(logmax_den_re0,logmax_num_re0);

    // LLR of the second bit
    // Bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p3_m1);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_m3, bit_met_p3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_re1 = simde_mm_max_epi16(xmm4, xmm5);

    // Bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p3_p1);
    xmm1 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p3, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_re1 = simde_mm_max_epi16(xmm4, xmm5);

    // LLR of second bit [L2(1), L2(2), L2(3), L2(4)]
    y1r = simde_mm_subs_epi16(logmax_den_re1,logmax_num_re1);

    // LLR of the third bit
    // Bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm1 = simde_mm_max_epi16(bit_met_m3_m1, bit_met_m3_m3);
    xmm2 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p3_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_im0 = simde_mm_max_epi16(xmm4, xmm5);

    // Bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p1_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_im0 = simde_mm_max_epi16(xmm4, xmm5);

    // LLR of third bit [L3(1), L3(2), L3(3), L3(4)]
    y0i = simde_mm_subs_epi16(logmax_den_im0,logmax_num_im0);

    // LLR of the fourth bit
    // Bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_p1_m3, bit_met_p3_m3);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p3, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_im1 = simde_mm_max_epi16(xmm4, xmm5);

    // Bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p3_m1);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p3_p1);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_im1 = simde_mm_max_epi16(xmm4, xmm5);

    // LLR of fourth bit [L4(1), L4(2), L4(3), L4(4)]
    y1i = simde_mm_subs_epi16(logmax_den_im1,logmax_num_im1);

    // Pack LLRs in output
    // [L1(1), L2(1), L1(2), L2(2), L1(3), L2(3), L1(4), L2(4)]
    xmm0 = simde_mm_unpacklo_epi16(y0r,y1r);
    // [L1(5), L2(5), L1(6), L2(6), L1(7), L2(7), L1(8), L2(8)]
    xmm1 = simde_mm_unpackhi_epi16(y0r,y1r);
    // [L3(1), L4(1), L3(2), L4(2), L3(3), L4(3), L3(4), L4(4)]
    xmm2 = simde_mm_unpacklo_epi16(y0i,y1i);
    // [L3(5), L4(5), L3(6), L4(6), L3(7), L4(7), L3(8), L4(8)]
    xmm3 = simde_mm_unpackhi_epi16(y0i,y1i);

    stream0_128i_out[2*i+0] = simde_mm_unpacklo_epi32(xmm0,xmm2); // 8LLRs, 2REs
    stream0_128i_out[2*i+1] = simde_mm_unpackhi_epi32(xmm0,xmm2);
    stream0_128i_out[2*i+2] = simde_mm_unpacklo_epi32(xmm1,xmm3);
    stream0_128i_out[2*i+3] = simde_mm_unpackhi_epi32(xmm1,xmm3);
  }
}

int dlsch_16qam_16qam_llr(LTE_DL_FRAME_PARMS *frame_parms,
                          int32_t **rxdataF_comp,
                          int32_t **rxdataF_comp_i,
                          int32_t **dl_ch_mag,   //|h_0|^2*(2/sqrt{10})
                          int32_t **dl_ch_mag_i, //|h_1|^2*(2/sqrt{10})
                          int32_t **rho_i,
                          int16_t *dlsch_llr,
                          uint8_t symbol,
                          uint8_t first_symbol_flag,
                          uint16_t nb_rb,
                          uint16_t pbch_pss_sss_adjust,
                          int16_t **llr16p)
{

  int16_t *rxF      = (int16_t*)&rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rxF_i    = (int16_t*)&rxdataF_comp_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *ch_mag   = (int16_t*)&dl_ch_mag[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *ch_mag_i = (int16_t*)&dl_ch_mag_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rho      = (int16_t*)&rho_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *llr16;
  int len;
  uint8_t symbol_mod = (symbol >= (7-frame_parms->Ncp))? (symbol-(7-frame_parms->Ncp)) : symbol;

  // first symbol has different structure due to more pilots
  if (first_symbol_flag == 1) {
    llr16 = (int16_t*)dlsch_llr;
  } else {
    llr16 = (int16_t*)(*llr16p);
  }


  AssertFatal(llr16!=NULL,"dlsch_16qam_16qam_llr: llr is null, symbol %d\n",symbol);

  if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
    // if symbol has pilots
    if (frame_parms->nb_antenna_ports_eNB!=1)
      // in 2 antenna ports we have 8 REs per symbol per RB
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
    else
      // for 1 antenna port we have 10 REs per symbol per RB
      len = (nb_rb*10) - (5*pbch_pss_sss_adjust/6);
  } else {
    // symbol has no pilots
    len = (nb_rb*12) - pbch_pss_sss_adjust;
  }

  // printf("symbol %d: qam16_llr, len %d (llr16 %p)\n",symbol,len,llr16);

  qam16_qam16((short *)rxF,
              (short *)rxF_i,
              (short *)ch_mag,
              (short *)ch_mag_i,
              (short *)llr16,
              (short *)rho,
              len);

  llr16 += (len<<2);
  *llr16p = (short *)llr16;

  return(0);
}

void qam16_qam64(int16_t *stream0_in,
                 int16_t *stream1_in,
                 int16_t *ch_mag,
                 int16_t *ch_mag_i,
                 int16_t *stream0_out,
                 int16_t *rho01,
                 int32_t length
     )
{

  /*
    Author: Sebastian Wagner
    Date: 2012-06-04

    Input:
    stream0_in:  MF filter for 1st stream, i.e., y0=h0'*y
    stream!_in:  MF filter for 2nd stream, i.e., y1=h1'*y
    ch_mag:      2*h0/sqrt(00), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    ch_mag_i:    2*h1/sqrt(00), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    rho01:       Channel cross correlation, i.e., h1'*h0

    Output:
    stream0_out: output LLRs for 1st stream
  */

  simde__m128i *rho01_128i       = (simde__m128i *)rho01;
  simde__m128i *stream0_128i_in  = (simde__m128i *)stream0_in;
  simde__m128i *stream1_128i_in  = (simde__m128i *)stream1_in;
  simde__m128i *stream0_128i_out = (simde__m128i *)stream0_out;
  simde__m128i *ch_mag_128i      = (simde__m128i *)ch_mag;
  simde__m128i *ch_mag_128i_i    = (simde__m128i *)ch_mag_i;


  simde__m128i ONE_OVER_SQRT_2 = simde_mm_set1_epi16(23170); // round(1/sqrt(2)*2^15)
  simde__m128i ONE_OVER_SQRT_10 = simde_mm_set1_epi16(20724); // round(1/sqrt(10)*2^16)
  simde__m128i THREE_OVER_SQRT_10 = simde_mm_set1_epi16(31086); // round(3/sqrt(10)*2^15)
  simde__m128i SQRT_10_OVER_FOUR = simde_mm_set1_epi16(25905); // round(sqrt(10)/4*2^15)
  simde__m128i ONE_OVER_TWO_SQRT_10 = simde_mm_set1_epi16(10362); // round(1/2/sqrt(10)*2^16)
  simde__m128i NINE_OVER_TWO_SQRT_10 = simde_mm_set1_epi16(23315); // round(9/2/sqrt(10)*2^14)
  simde__m128i ONE_OVER_SQRT_2_42 = simde_mm_set1_epi16(3575); // round(1/sqrt(2*42)*2^15)
  simde__m128i THREE_OVER_SQRT_2_42 = simde_mm_set1_epi16(10726); // round(3/sqrt(2*42)*2^15)
  simde__m128i FIVE_OVER_SQRT_2_42 = simde_mm_set1_epi16(17876); // round(5/sqrt(2*42)*2^15)
  simde__m128i SEVEN_OVER_SQRT_2_42 = simde_mm_set1_epi16(25027); // round(7/sqrt(2*42)*2^15)
  simde__m128i SQRT_42_OVER_FOUR = simde_mm_set1_epi16(13272); // round(sqrt(42)/4*2^13), Q3.
  simde__m128i ch_mag_des,ch_mag_int;
  simde__m128i  y0r_over_sqrt10;
  simde__m128i  y0i_over_sqrt10;
  simde__m128i  y0r_three_over_sqrt10;
  simde__m128i  y0i_three_over_sqrt10;
  simde__m128i ch_mag_over_10;
  simde__m128i ch_mag_over_2;
  simde__m128i ch_mag_9_over_10;
  simde__m128i ch_mag_int_with_sigma2;
  simde__m128i two_ch_mag_int_with_sigma2;
  simde__m128i three_ch_mag_int_with_sigma2;

  int i;

  for (i=0; i<length>>2; i+=2) {
    // In one iteration, we deal with 8 REs

    // Get rho
    simde__m128i xmm0 = rho01_128i[i];
    simde__m128i xmm1 = rho01_128i[i + 1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i xmm2 = simde_mm_unpacklo_epi64(xmm0, xmm1); // Re(rho)
    simde__m128i xmm3 = simde_mm_unpackhi_epi64(xmm0, xmm1); // Im(rho)
    simde__m128i rho_rpi = simde_mm_adds_epi16(xmm2, xmm3); // rho = Re(rho) + Im(rho)
    simde__m128i rho_rmi = simde_mm_subs_epi16(xmm2, xmm3); // rho* = Re(rho) - Im(rho)

    // Compute the different rhos
    simde__m128i rho_rpi_1_1 = simde_mm_mulhi_epi16(rho_rpi, ONE_OVER_SQRT_10);
    simde__m128i rho_rmi_1_1 = simde_mm_mulhi_epi16(rho_rmi, ONE_OVER_SQRT_10);
    simde__m128i rho_rpi_3_3 = simde_mm_mulhi_epi16(rho_rpi, THREE_OVER_SQRT_10);
    simde__m128i rho_rmi_3_3 = simde_mm_mulhi_epi16(rho_rmi, THREE_OVER_SQRT_10);
    rho_rpi_3_3 = simde_mm_slli_epi16(rho_rpi_3_3, 1);
    rho_rmi_3_3 = simde_mm_slli_epi16(rho_rmi_3_3, 1);

    simde__m128i xmm4 = simde_mm_mulhi_epi16(xmm2, ONE_OVER_SQRT_10); // Re(rho)
    simde__m128i xmm5 = simde_mm_mulhi_epi16(xmm3, THREE_OVER_SQRT_10); // Im(rho)
    xmm5 = simde_mm_slli_epi16(xmm5, 1);

    simde__m128i rho_rpi_1_3 = simde_mm_adds_epi16(xmm4, xmm5);
    simde__m128i rho_rmi_1_3 = simde_mm_subs_epi16(xmm4, xmm5);

    simde__m128i xmm6 = simde_mm_mulhi_epi16(xmm2, THREE_OVER_SQRT_10); // Re(rho)
    simde__m128i xmm7 = simde_mm_mulhi_epi16(xmm3, ONE_OVER_SQRT_10); // Im(rho)
    xmm6 = simde_mm_slli_epi16(xmm6, 1);

    simde__m128i rho_rpi_3_1 = simde_mm_adds_epi16(xmm6, xmm7);
    simde__m128i rho_rmi_3_1 = simde_mm_subs_epi16(xmm6, xmm7);

    // Rearrange interfering MF output
    xmm0 = stream1_128i_in[i];
    xmm1 = stream1_128i_in[i+1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y1r = simde_mm_unpacklo_epi64(xmm0, xmm1); //[y1r(1),y1r(2),y1r(3),y1r(4)]
    simde__m128i y1i = simde_mm_unpackhi_epi64(xmm0, xmm1); //[y1i(1),y1i(2),y1i(3),y1i(4)]

    xmm0 = simde_mm_setzero_si128(); // ZERO
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_1, y1r); // = [Re(rho)+ Im(rho)]/sqrt(10) - y1r
    simde__m128i psi_r_p1_p1 = simde_mm_abs_epi16(xmm2); // = |[Re(rho)+ Im(rho)]/sqrt(10) - y1r|

    xmm2 = simde_mm_subs_epi16(rho_rmi_1_1, y1r);
    simde__m128i psi_r_p1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_1, y1i);
    simde__m128i psi_i_p1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_3, y1r);
    simde__m128i psi_r_p1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_3, y1r);
    simde__m128i psi_r_p1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_1, y1i);
    simde__m128i psi_i_p1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_1, y1r);
    simde__m128i psi_r_p3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_1, y1r);
    simde__m128i psi_r_p3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_3, y1i);
    simde__m128i psi_i_p3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_3, y1r);
    simde__m128i psi_r_p3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_3, y1r);
    simde__m128i psi_r_p3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_3, y1i);
    simde__m128i psi_i_p3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_1, y1i);
    simde__m128i psi_i_m1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_1, y1i);
    simde__m128i psi_i_m1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_3, y1i);
    simde__m128i psi_i_m3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_3, y1i);
    simde__m128i psi_i_m3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_1, y1i);
    simde__m128i psi_i_p1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_1, y1i);
    simde__m128i psi_i_p1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_3, y1i);
    simde__m128i psi_i_p3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_3, y1i);
    simde__m128i psi_i_p3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_1, y1r);
    simde__m128i psi_r_m1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_3, y1r);
    simde__m128i psi_r_m1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_1, y1r);
    simde__m128i psi_r_m3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_3, y1r);
    simde__m128i psi_r_m3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1r, rho_rmi_1_1);
    simde__m128i psi_r_m1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1r, rho_rmi_1_3);
    simde__m128i psi_r_m1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1i, rho_rmi_1_1);
    simde__m128i psi_i_m1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1i, rho_rmi_3_1);
    simde__m128i psi_i_m1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1r, rho_rmi_3_1);
    simde__m128i psi_r_m3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1r, rho_rmi_3_3);
    simde__m128i psi_r_m3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1i, rho_rmi_1_3);
    simde__m128i psi_i_m3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(y1i, rho_rmi_3_3);
    simde__m128i psi_i_m3_m3 = simde_mm_abs_epi16(xmm2);

    // Rearrange desired MF output
    xmm0 = stream0_128i_in[i];
    xmm1 = stream0_128i_in[i+1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y0r = simde_mm_unpacklo_epi64(xmm0, xmm1); // = [y0r(1),y0r(2),y0r(3),y0r(4)]
    simde__m128i y0i = simde_mm_unpackhi_epi64(xmm0, xmm1);

    // Rearrange desired channel magnitudes
    xmm2 = ch_mag_128i[i]; // = [|h|^2(1),|h|^2(1),|h|^2(2),|h|^2(2)]*(2/sqrt(10))
    xmm3 = ch_mag_128i[i+1]; // = [|h|^2(3),|h|^2(3),|h|^2(4),|h|^2(4)]*(2/sqrt(10))
    xmm2 = simde_mm_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));

    ch_mag_des = simde_mm_unpacklo_epi64(xmm2,xmm3); // = [|h|^2(1),|h|^2(2),|h|^2(3),|h|^2(4)]*(2/sqrt(10))

    // Rearrange interfering channel magnitudes
    xmm2 = ch_mag_128i_i[i];
    xmm3 = ch_mag_128i_i[i+1];

    xmm2 = simde_mm_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));

    ch_mag_int  = simde_mm_unpacklo_epi64(xmm2,xmm3);

    // Scale MF output of desired signal
    y0r_over_sqrt10 = simde_mm_mulhi_epi16(y0r,ONE_OVER_SQRT_10);
    y0i_over_sqrt10 = simde_mm_mulhi_epi16(y0i,ONE_OVER_SQRT_10);
    y0r_three_over_sqrt10 = simde_mm_mulhi_epi16(y0r,THREE_OVER_SQRT_10);
    y0i_three_over_sqrt10 = simde_mm_mulhi_epi16(y0i,THREE_OVER_SQRT_10);
    y0r_three_over_sqrt10 = simde_mm_slli_epi16(y0r_three_over_sqrt10,1);
    y0i_three_over_sqrt10 = simde_mm_slli_epi16(y0i_three_over_sqrt10,1);

    // Compute necessary combination of required terms
    simde__m128i y0_p_1_1 = simde_mm_adds_epi16(y0r_over_sqrt10, y0i_over_sqrt10);
    simde__m128i y0_m_1_1 = simde_mm_subs_epi16(y0r_over_sqrt10, y0i_over_sqrt10);

    simde__m128i y0_p_1_3 = simde_mm_adds_epi16(y0r_over_sqrt10, y0i_three_over_sqrt10);
    simde__m128i y0_m_1_3 = simde_mm_subs_epi16(y0r_over_sqrt10, y0i_three_over_sqrt10);

    simde__m128i y0_p_3_1 = simde_mm_adds_epi16(y0r_three_over_sqrt10, y0i_over_sqrt10);
    simde__m128i y0_m_3_1 = simde_mm_subs_epi16(y0r_three_over_sqrt10, y0i_over_sqrt10);

    simde__m128i y0_p_3_3 = simde_mm_adds_epi16(y0r_three_over_sqrt10, y0i_three_over_sqrt10);
    simde__m128i y0_m_3_3 = simde_mm_subs_epi16(y0r_three_over_sqrt10, y0i_three_over_sqrt10);

    // Compute optimal interfering symbol magnitude
    ch_mag_int_with_sigma2       = simde_mm_srai_epi16(ch_mag_int, 1); // *2
    two_ch_mag_int_with_sigma2   = ch_mag_int; // *4
    simde__m128i tmp_result, tmp_result2, tmp_result3, tmp_result4;
    three_ch_mag_int_with_sigma2 = simde_mm_adds_epi16(ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2); // *6

    interference_abs_64qam_epi16(psi_r_p1_p1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_p1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_p1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_p1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_p3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_p3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_p3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_p3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_m1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_m1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_m1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_m1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_m3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_m3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_m3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_m3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_p1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_p1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_p1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_p1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_p3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_p3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_p3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_p3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_m1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_m1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_m1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_m1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_m3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_m3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_m3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_m3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_p1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_p1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_p1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_p1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_p3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_p3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_p3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_p3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_m1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_m1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_m1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_m1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_m3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_m3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_m3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_m3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_p1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_p1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_p1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_p1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_p3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_p3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_p3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_p3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_m1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_m1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_m1 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_m1,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_m3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_m3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_m3 ,ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_m3,ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42,FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);

    // Calculation of groups of two terms in the bit metric involving product of psi and interference magnitude
    prodsum_psi_a_epi16(psi_r_p1_p1,a_r_p1_p1,psi_i_p1_p1,a_i_p1_p1,psi_a_p1_p1);
    prodsum_psi_a_epi16(psi_r_p1_p3,a_r_p1_p3,psi_i_p1_p3,a_i_p1_p3,psi_a_p1_p3);
    prodsum_psi_a_epi16(psi_r_p3_p1,a_r_p3_p1,psi_i_p3_p1,a_i_p3_p1,psi_a_p3_p1);
    prodsum_psi_a_epi16(psi_r_p3_p3,a_r_p3_p3,psi_i_p3_p3,a_i_p3_p3,psi_a_p3_p3);
    prodsum_psi_a_epi16(psi_r_p1_m1,a_r_p1_m1,psi_i_p1_m1,a_i_p1_m1,psi_a_p1_m1);
    prodsum_psi_a_epi16(psi_r_p1_m3,a_r_p1_m3,psi_i_p1_m3,a_i_p1_m3,psi_a_p1_m3);
    prodsum_psi_a_epi16(psi_r_p3_m1,a_r_p3_m1,psi_i_p3_m1,a_i_p3_m1,psi_a_p3_m1);
    prodsum_psi_a_epi16(psi_r_p3_m3,a_r_p3_m3,psi_i_p3_m3,a_i_p3_m3,psi_a_p3_m3);
    prodsum_psi_a_epi16(psi_r_m1_p1,a_r_m1_p1,psi_i_m1_p1,a_i_m1_p1,psi_a_m1_p1);
    prodsum_psi_a_epi16(psi_r_m1_p3,a_r_m1_p3,psi_i_m1_p3,a_i_m1_p3,psi_a_m1_p3);
    prodsum_psi_a_epi16(psi_r_m3_p1,a_r_m3_p1,psi_i_m3_p1,a_i_m3_p1,psi_a_m3_p1);
    prodsum_psi_a_epi16(psi_r_m3_p3,a_r_m3_p3,psi_i_m3_p3,a_i_m3_p3,psi_a_m3_p3);
    prodsum_psi_a_epi16(psi_r_m1_m1,a_r_m1_m1,psi_i_m1_m1,a_i_m1_m1,psi_a_m1_m1);
    prodsum_psi_a_epi16(psi_r_m1_m3,a_r_m1_m3,psi_i_m1_m3,a_i_m1_m3,psi_a_m1_m3);
    prodsum_psi_a_epi16(psi_r_m3_m1,a_r_m3_m1,psi_i_m3_m1,a_i_m3_m1,psi_a_m3_m1);
    prodsum_psi_a_epi16(psi_r_m3_m3,a_r_m3_m3,psi_i_m3_m3,a_i_m3_m3,psi_a_m3_m3);

    // Multiply by sqrt(2)
    psi_a_p1_p1 = simde_mm_mulhi_epi16(psi_a_p1_p1, ONE_OVER_SQRT_2);
    psi_a_p1_p1 = simde_mm_slli_epi16(psi_a_p1_p1, 2);
    psi_a_p1_p3 = simde_mm_mulhi_epi16(psi_a_p1_p3, ONE_OVER_SQRT_2);
    psi_a_p1_p3 = simde_mm_slli_epi16(psi_a_p1_p3, 2);
    psi_a_p3_p1 = simde_mm_mulhi_epi16(psi_a_p3_p1, ONE_OVER_SQRT_2);
    psi_a_p3_p1 = simde_mm_slli_epi16(psi_a_p3_p1, 2);
    psi_a_p3_p3 = simde_mm_mulhi_epi16(psi_a_p3_p3, ONE_OVER_SQRT_2);
    psi_a_p3_p3 = simde_mm_slli_epi16(psi_a_p3_p3, 2);
    psi_a_p1_m1 = simde_mm_mulhi_epi16(psi_a_p1_m1, ONE_OVER_SQRT_2);
    psi_a_p1_m1 = simde_mm_slli_epi16(psi_a_p1_m1, 2);
    psi_a_p1_m3 = simde_mm_mulhi_epi16(psi_a_p1_m3, ONE_OVER_SQRT_2);
    psi_a_p1_m3 = simde_mm_slli_epi16(psi_a_p1_m3, 2);
    psi_a_p3_m1 = simde_mm_mulhi_epi16(psi_a_p3_m1, ONE_OVER_SQRT_2);
    psi_a_p3_m1 = simde_mm_slli_epi16(psi_a_p3_m1, 2);
    psi_a_p3_m3 = simde_mm_mulhi_epi16(psi_a_p3_m3, ONE_OVER_SQRT_2);
    psi_a_p3_m3 = simde_mm_slli_epi16(psi_a_p3_m3, 2);
    psi_a_m1_p1 = simde_mm_mulhi_epi16(psi_a_m1_p1, ONE_OVER_SQRT_2);
    psi_a_m1_p1 = simde_mm_slli_epi16(psi_a_m1_p1, 2);
    psi_a_m1_p3 = simde_mm_mulhi_epi16(psi_a_m1_p3, ONE_OVER_SQRT_2);
    psi_a_m1_p3 = simde_mm_slli_epi16(psi_a_m1_p3, 2);
    psi_a_m3_p1 = simde_mm_mulhi_epi16(psi_a_m3_p1, ONE_OVER_SQRT_2);
    psi_a_m3_p1 = simde_mm_slli_epi16(psi_a_m3_p1, 2);
    psi_a_m3_p3 = simde_mm_mulhi_epi16(psi_a_m3_p3, ONE_OVER_SQRT_2);
    psi_a_m3_p3 = simde_mm_slli_epi16(psi_a_m3_p3, 2);
    psi_a_m1_m1 = simde_mm_mulhi_epi16(psi_a_m1_m1, ONE_OVER_SQRT_2);
    psi_a_m1_m1 = simde_mm_slli_epi16(psi_a_m1_m1, 2);
    psi_a_m1_m3 = simde_mm_mulhi_epi16(psi_a_m1_m3, ONE_OVER_SQRT_2);
    psi_a_m1_m3 = simde_mm_slli_epi16(psi_a_m1_m3, 2);
    psi_a_m3_m1 = simde_mm_mulhi_epi16(psi_a_m3_m1, ONE_OVER_SQRT_2);
    psi_a_m3_m1 = simde_mm_slli_epi16(psi_a_m3_m1, 2);
    psi_a_m3_m3 = simde_mm_mulhi_epi16(psi_a_m3_m3, ONE_OVER_SQRT_2);
    psi_a_m3_m3 = simde_mm_slli_epi16(psi_a_m3_m3, 2);

    // squared interference magnitude times int. ch. power
    square_a_64qam_epi16(a_r_p1_p1,a_i_p1_p1,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_p1_p1);
    square_a_64qam_epi16(a_r_p1_p3,a_i_p1_p3,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_p1_p3);
    square_a_64qam_epi16(a_r_p3_p1,a_i_p3_p1,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_p3_p1);
    square_a_64qam_epi16(a_r_p3_p3,a_i_p3_p3,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_p3_p3);
    square_a_64qam_epi16(a_r_p1_m1,a_i_p1_m1,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_p1_m1);
    square_a_64qam_epi16(a_r_p1_m3,a_i_p1_m3,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_p1_m3);
    square_a_64qam_epi16(a_r_p3_m1,a_i_p3_m1,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_p3_m1);
    square_a_64qam_epi16(a_r_p3_m3,a_i_p3_m3,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_p3_m3);
    square_a_64qam_epi16(a_r_m1_p1,a_i_m1_p1,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_m1_p1);
    square_a_64qam_epi16(a_r_m1_p3,a_i_m1_p3,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_m1_p3);
    square_a_64qam_epi16(a_r_m3_p1,a_i_m3_p1,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_m3_p1);
    square_a_64qam_epi16(a_r_m3_p3,a_i_m3_p3,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_m3_p3);
    square_a_64qam_epi16(a_r_m1_m1,a_i_m1_m1,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_m1_m1);
    square_a_64qam_epi16(a_r_m1_m3,a_i_m1_m3,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_m1_m3);
    square_a_64qam_epi16(a_r_m3_m1,a_i_m3_m1,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_m3_m1);
    square_a_64qam_epi16(a_r_m3_m3,a_i_m3_m3,ch_mag_int,SQRT_42_OVER_FOUR,a_sq_m3_m3);

    // Computing different multiples of channel norms
    ch_mag_over_10   = simde_mm_mulhi_epi16(ch_mag_des, ONE_OVER_TWO_SQRT_10);
    ch_mag_over_2    = simde_mm_mulhi_epi16(ch_mag_des, SQRT_10_OVER_FOUR);
    ch_mag_over_2    = simde_mm_slli_epi16(ch_mag_over_2, 1);
    ch_mag_9_over_10 = simde_mm_mulhi_epi16(ch_mag_des, NINE_OVER_TWO_SQRT_10);
    ch_mag_9_over_10 = simde_mm_slli_epi16(ch_mag_9_over_10, 2);

    // Computing Metrics
    xmm0 = simde_mm_subs_epi16(psi_a_p1_p1, a_sq_p1_p1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_1_1);
    simde__m128i bit_met_p1_p1 = simde_mm_subs_epi16(xmm1, ch_mag_over_10);

    xmm0 = simde_mm_subs_epi16(psi_a_p1_p3, a_sq_p1_p3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_1_3);
    simde__m128i bit_met_p1_p3 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_p1_m1, a_sq_p1_m1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_1_1);
    simde__m128i bit_met_p1_m1 = simde_mm_subs_epi16(xmm1, ch_mag_over_10);

    xmm0 = simde_mm_subs_epi16(psi_a_p1_m3, a_sq_p1_m3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_1_3);
    simde__m128i bit_met_p1_m3 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_p3_p1, a_sq_p3_p1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_3_1);
    simde__m128i bit_met_p3_p1 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_p3_p3, a_sq_p3_p3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_3_3);
    simde__m128i bit_met_p3_p3 = simde_mm_subs_epi16(xmm1, ch_mag_9_over_10);

    xmm0 = simde_mm_subs_epi16(psi_a_p3_m1, a_sq_p3_m1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_3_1);
    simde__m128i bit_met_p3_m1 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_p3_m3, a_sq_p3_m3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_3_3);
    simde__m128i bit_met_p3_m3 = simde_mm_subs_epi16(xmm1, ch_mag_9_over_10);

    xmm0 = simde_mm_subs_epi16(psi_a_m1_p1, a_sq_m1_p1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_1_1);
    simde__m128i bit_met_m1_p1 = simde_mm_subs_epi16(xmm1, ch_mag_over_10);

    xmm0 = simde_mm_subs_epi16(psi_a_m1_p3, a_sq_m1_p3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_1_3);
    simde__m128i bit_met_m1_p3 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_m1_m1, a_sq_m1_m1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_1_1);
    simde__m128i bit_met_m1_m1 = simde_mm_subs_epi16(xmm1, ch_mag_over_10);

    xmm0 = simde_mm_subs_epi16(psi_a_m1_m3, a_sq_m1_m3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_1_3);
    simde__m128i bit_met_m1_m3 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_m3_p1, a_sq_m3_p1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_3_1);
    simde__m128i bit_met_m3_p1 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_m3_p3, a_sq_m3_p3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_3_3);
    simde__m128i bit_met_m3_p3 = simde_mm_subs_epi16(xmm1, ch_mag_9_over_10);

    xmm0 = simde_mm_subs_epi16(psi_a_m3_m1, a_sq_m3_m1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_3_1);
    simde__m128i bit_met_m3_m1 = simde_mm_subs_epi16(xmm1, ch_mag_over_2);

    xmm0 = simde_mm_subs_epi16(psi_a_m3_m3, a_sq_m3_m3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_3_3);
    simde__m128i bit_met_m3_m3 = simde_mm_subs_epi16(xmm1, ch_mag_9_over_10);

    // LLR of the first bit
    // Bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m3_m1, bit_met_m3_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);

    // Bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm1 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p3_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);

    // LLR of first bit [L1(1), L1(2), L1(3), L1(4), L1(5), L1(6), L1(7), L1(8)]
    y0r = simde_mm_subs_epi16(logmax_den_re0,logmax_num_re0);

    // LLR of the second bit
    // Bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p3_m1);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_m3, bit_met_p3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_re1 = simde_mm_max_epi16(xmm4, xmm5);

    // Bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p3_p1);
    xmm1 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p3, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_re1 = simde_mm_max_epi16(xmm4, xmm5);

    // LLR of second bit [L2(1), L2(2), L2(3), L2(4)]
    y1r = simde_mm_subs_epi16(logmax_den_re1,logmax_num_re1);

    // LLR of the third bit
    // Bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm1 = simde_mm_max_epi16(bit_met_m3_m1, bit_met_m3_m3);
    xmm2 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p3_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_im0 = simde_mm_max_epi16(xmm4, xmm5);

    // Bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p1_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_im0 = simde_mm_max_epi16(xmm4, xmm5);

    // LLR of third bit [L3(1), L3(2), L3(3), L3(4)]
    y0i = simde_mm_subs_epi16(logmax_den_im0,logmax_num_im0);

    // LLR of the fourth bit
    // Bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_p1_m3, bit_met_p3_m3);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p3, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_im1 = simde_mm_max_epi16(xmm4, xmm5);

    // Bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p3_m1);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p3_p1);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_im1 = simde_mm_max_epi16(xmm4, xmm5);

    // LLR of fourth bit [L4(1), L4(2), L4(3), L4(4)]
    y1i = simde_mm_subs_epi16(logmax_den_im1,logmax_num_im1);

    // Pack LLRs in output
    // [L1(1), L2(1), L1(2), L2(2), L1(3), L2(3), L1(4), L2(4)]
    xmm0 = simde_mm_unpacklo_epi16(y0r,y1r);
    // [L1(5), L2(5), L1(6), L2(6), L1(7), L2(7), L1(8), L2(8)]
    xmm1 = simde_mm_unpackhi_epi16(y0r,y1r);
    // [L3(1), L4(1), L3(2), L4(2), L3(3), L4(3), L3(4), L4(4)]
    xmm2 = simde_mm_unpacklo_epi16(y0i,y1i);
    // [L3(5), L4(5), L3(6), L4(6), L3(7), L4(7), L3(8), L4(8)]
    xmm3 = simde_mm_unpackhi_epi16(y0i,y1i);

    stream0_128i_out[2*i+0] = simde_mm_unpacklo_epi32(xmm0,xmm2); // 8LLRs, 2REs
    stream0_128i_out[2*i+1] = simde_mm_unpackhi_epi32(xmm0,xmm2);
    stream0_128i_out[2*i+2] = simde_mm_unpacklo_epi32(xmm1,xmm3);
    stream0_128i_out[2*i+3] = simde_mm_unpackhi_epi32(xmm1,xmm3);
  }
}

int dlsch_16qam_64qam_llr(LTE_DL_FRAME_PARMS *frame_parms,
                          int32_t **rxdataF_comp,
                          int32_t **rxdataF_comp_i,
                          int32_t **dl_ch_mag,   //|h_0|^2*(2/sqrt{10})
                          int32_t **dl_ch_mag_i, //|h_1|^2*(2/sqrt{10})
                          int32_t **rho_i,
                          int16_t *dlsch_llr,
                          uint8_t symbol,
                          uint8_t first_symbol_flag,
                          uint16_t nb_rb,
                          uint16_t pbch_pss_sss_adjust,
                          int16_t **llr16p)
{

  int16_t *rxF      = (int16_t*)&rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rxF_i    = (int16_t*)&rxdataF_comp_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *ch_mag   = (int16_t*)&dl_ch_mag[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *ch_mag_i = (int16_t*)&dl_ch_mag_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rho      = (int16_t*)&rho_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *llr16;
  int len;
  uint8_t symbol_mod = (symbol >= (7-frame_parms->Ncp))? (symbol-(7-frame_parms->Ncp)) : symbol;

  // first symbol has different structure due to more pilots
  if (first_symbol_flag == 1) {
    llr16 = (int16_t*)dlsch_llr;
  } else {
    llr16 = (int16_t*)(*llr16p);
  }


  AssertFatal(llr16!=NULL,"dlsch_16qam_64qam_llr:llr is null, symbol %d\n",symbol);


  if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
    // if symbol has pilots
    if (frame_parms->nb_antenna_ports_eNB!=1)
      // in 2 antenna ports we have 8 REs per symbol per RB
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
    else
      // for 1 antenna port we have 10 REs per symbol per RB
      len = (nb_rb*10) - (5*pbch_pss_sss_adjust/6);
  } else {
    // symbol has no pilots
    len = (nb_rb*12) - pbch_pss_sss_adjust;
  }

  // printf("symbol %d: qam16_llr, len %d (llr16 %p)\n",symbol,len,llr16);

  qam16_qam64((short *)rxF,
              (short *)rxF_i,
              (short *)ch_mag,
              (short *)ch_mag_i,
              (short *)llr16,
              (short *)rho,
              len);

  llr16 += (len<<2);
  *llr16p = (short *)llr16;

  return(0);
}

//----------------------------------------------------------------------------------------------
// 64-QAM
//----------------------------------------------------------------------------------------------

void qam64_qpsk(int16_t *stream0_in,
                int16_t *stream1_in,
                int16_t *ch_mag,
                int16_t *stream0_out,
                int16_t *rho01,
                int32_t length
    )
{

  /*
    Author: S. Wagner
    Date: 31-07-12

    Input:
    stream0_in:  MF filter for 1st stream, i.e., y0=h0'*y
    stream1_in:  MF filter for 2nd stream, i.e., y1=h1'*y
    ch_mag:      4*h0/sqrt(42), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    ch_mag_i:    4*h1/sqrt(42), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    rho01:       Channel cross correlation, i.e., h1'*h0

    Output:
    stream0_out: output LLRs for 1st stream
  */

  simde__m128i *rho01_128i      = (simde__m128i *)rho01;
  simde__m128i *stream0_128i_in = (simde__m128i *)stream0_in;
  simde__m128i *stream1_128i_in = (simde__m128i *)stream1_in;
  simde__m128i *ch_mag_128i     = (simde__m128i *)ch_mag;


  simde__m128i ONE_OVER_SQRT_42 = simde_mm_set1_epi16(10112); // round(1/sqrt(42)*2^16)
  simde__m128i THREE_OVER_SQRT_42 = simde_mm_set1_epi16(30337); // round(3/sqrt(42)*2^16)
  simde__m128i FIVE_OVER_SQRT_42 = simde_mm_set1_epi16(25281); // round(5/sqrt(42)*2^15)
  simde__m128i SEVEN_OVER_SQRT_42 = simde_mm_set1_epi16(17697); // round(5/sqrt(42)*2^15)
  simde__m128i ONE_OVER_SQRT_2 = simde_mm_set1_epi16(23170); // round(1/sqrt(2)*2^15)
  simde__m128i FORTYNINE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(30969); // round(49/(4*sqrt(42))*2^14), Q2.14
  simde__m128i THIRTYSEVEN_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(23385); // round(37/(4*sqrt(42))*2^14), Q2.14
  simde__m128i TWENTYFIVE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(31601); // round(25/(4*sqrt(42))*2^15)
  simde__m128i TWENTYNINE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(18329); // round(29/(4*sqrt(42))*2^15), Q2.14
  simde__m128i SEVENTEEN_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(21489); // round(17/(4*sqrt(42))*2^15)
  simde__m128i NINE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(11376); // round(9/(4*sqrt(42))*2^15)
  simde__m128i THIRTEEN_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(16433); // round(13/(4*sqrt(42))*2^15)
  simde__m128i FIVE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(6320); // round(5/(4*sqrt(42))*2^15)
  simde__m128i ONE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(1264); // round(1/(4*sqrt(42))*2^15)


  simde__m128i ch_mag_des;
  simde__m128i ch_mag_98_over_42_with_sigma2;
  simde__m128i ch_mag_74_over_42_with_sigma2;
  simde__m128i ch_mag_58_over_42_with_sigma2;
  simde__m128i ch_mag_50_over_42_with_sigma2;
  simde__m128i ch_mag_34_over_42_with_sigma2;
  simde__m128i ch_mag_18_over_42_with_sigma2;
  simde__m128i ch_mag_26_over_42_with_sigma2;
  simde__m128i ch_mag_10_over_42_with_sigma2;
  simde__m128i ch_mag_2_over_42_with_sigma2;
  simde__m128i  y0r_one_over_sqrt_21;
  simde__m128i  y0r_three_over_sqrt_21;
  simde__m128i  y0r_five_over_sqrt_21;
  simde__m128i  y0r_seven_over_sqrt_21;
  simde__m128i  y0i_one_over_sqrt_21;
  simde__m128i  y0i_three_over_sqrt_21;
  simde__m128i  y0i_five_over_sqrt_21;
  simde__m128i  y0i_seven_over_sqrt_21;

  int i,j;

  for (i=0; i<length>>2; i+=2) {

    // Get rho
    simde__m128i xmm0 = rho01_128i[i];
    simde__m128i xmm1 = rho01_128i[i + 1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i xmm2 = simde_mm_unpacklo_epi64(xmm0, xmm1); // Re(rho)
    simde__m128i xmm3 = simde_mm_unpackhi_epi64(xmm0, xmm1); // Im(rho)
    simde__m128i rho_rpi = simde_mm_adds_epi16(xmm2, xmm3); // rho = Re(rho) + Im(rho)
    simde__m128i rho_rmi = simde_mm_subs_epi16(xmm2, xmm3); // rho* = Re(rho) - Im(rho)

    // Compute the different rhos
    simde__m128i rho_rpi_1_1 = simde_mm_mulhi_epi16(rho_rpi, ONE_OVER_SQRT_42);
    simde__m128i rho_rmi_1_1 = simde_mm_mulhi_epi16(rho_rmi, ONE_OVER_SQRT_42);
    simde__m128i rho_rpi_3_3 = simde_mm_mulhi_epi16(rho_rpi, THREE_OVER_SQRT_42);
    simde__m128i rho_rmi_3_3 = simde_mm_mulhi_epi16(rho_rmi, THREE_OVER_SQRT_42);
    simde__m128i rho_rpi_5_5 = simde_mm_mulhi_epi16(rho_rpi, FIVE_OVER_SQRT_42);
    simde__m128i rho_rmi_5_5 = simde_mm_mulhi_epi16(rho_rmi, FIVE_OVER_SQRT_42);
    simde__m128i rho_rpi_7_7 = simde_mm_mulhi_epi16(rho_rpi, SEVEN_OVER_SQRT_42);
    simde__m128i rho_rmi_7_7 = simde_mm_mulhi_epi16(rho_rmi, SEVEN_OVER_SQRT_42);

    rho_rpi_5_5 = simde_mm_slli_epi16(rho_rpi_5_5, 1);
    rho_rmi_5_5 = simde_mm_slli_epi16(rho_rmi_5_5, 1);
    rho_rpi_7_7 = simde_mm_slli_epi16(rho_rpi_7_7, 2);
    rho_rmi_7_7 = simde_mm_slli_epi16(rho_rmi_7_7, 2);

    simde__m128i xmm4 = simde_mm_mulhi_epi16(xmm2, ONE_OVER_SQRT_42);
    simde__m128i xmm5 = simde_mm_mulhi_epi16(xmm3, ONE_OVER_SQRT_42);
    simde__m128i xmm6 = simde_mm_mulhi_epi16(xmm3, THREE_OVER_SQRT_42);
    simde__m128i xmm7 = simde_mm_mulhi_epi16(xmm3, FIVE_OVER_SQRT_42);
    simde__m128i xmm8 = simde_mm_mulhi_epi16(xmm3, SEVEN_OVER_SQRT_42);
    xmm7 = simde_mm_slli_epi16(xmm7, 1);
    xmm8 = simde_mm_slli_epi16(xmm8, 2);

    simde__m128i rho_rpi_1_3 = simde_mm_adds_epi16(xmm4, xmm6);
    simde__m128i rho_rmi_1_3 = simde_mm_subs_epi16(xmm4, xmm6);
    simde__m128i rho_rpi_1_5 = simde_mm_adds_epi16(xmm4, xmm7);
    simde__m128i rho_rmi_1_5 = simde_mm_subs_epi16(xmm4, xmm7);
    simde__m128i rho_rpi_1_7 = simde_mm_adds_epi16(xmm4, xmm8);
    simde__m128i rho_rmi_1_7 = simde_mm_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm_mulhi_epi16(xmm2, THREE_OVER_SQRT_42);
    simde__m128i rho_rpi_3_1 = simde_mm_adds_epi16(xmm4, xmm5);
    simde__m128i rho_rmi_3_1 = simde_mm_subs_epi16(xmm4, xmm5);
    simde__m128i rho_rpi_3_5 = simde_mm_adds_epi16(xmm4, xmm7);
    simde__m128i rho_rmi_3_5 = simde_mm_subs_epi16(xmm4, xmm7);
    simde__m128i rho_rpi_3_7 = simde_mm_adds_epi16(xmm4, xmm8);
    simde__m128i rho_rmi_3_7 = simde_mm_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm_mulhi_epi16(xmm2, FIVE_OVER_SQRT_42);
    xmm4 = simde_mm_slli_epi16(xmm4, 1);
    simde__m128i rho_rpi_5_1 = simde_mm_adds_epi16(xmm4, xmm5);
    simde__m128i rho_rmi_5_1 = simde_mm_subs_epi16(xmm4, xmm5);
    simde__m128i rho_rpi_5_3 = simde_mm_adds_epi16(xmm4, xmm6);
    simde__m128i rho_rmi_5_3 = simde_mm_subs_epi16(xmm4, xmm6);
    simde__m128i rho_rpi_5_7 = simde_mm_adds_epi16(xmm4, xmm8);
    simde__m128i rho_rmi_5_7 = simde_mm_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm_mulhi_epi16(xmm2, SEVEN_OVER_SQRT_42);
    xmm4 = simde_mm_slli_epi16(xmm4, 2);
    simde__m128i rho_rpi_7_1 = simde_mm_adds_epi16(xmm4, xmm5);
    simde__m128i rho_rmi_7_1 = simde_mm_subs_epi16(xmm4, xmm5);
    simde__m128i rho_rpi_7_3 = simde_mm_adds_epi16(xmm4, xmm6);
    simde__m128i rho_rmi_7_3 = simde_mm_subs_epi16(xmm4, xmm6);
    simde__m128i rho_rpi_7_5 = simde_mm_adds_epi16(xmm4, xmm7);
    simde__m128i rho_rmi_7_5 = simde_mm_subs_epi16(xmm4, xmm7);

    // Rearrange interfering MF output
    xmm0 = stream1_128i_in[i];
    xmm1 = stream1_128i_in[i+1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y1r = simde_mm_unpacklo_epi64(xmm0, xmm1); //[y1r(1),y1r(2),y1r(3),y1r(4)]
    simde__m128i y1i = simde_mm_unpackhi_epi64(xmm0, xmm1); //[y1i(1),y1i(2),y1i(3),y1i(4)]

    // Psi_r calculation from rho_rpi or rho_rmi
    xmm0 = simde_mm_setzero_si128(); // ZERO for abs_pi16
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_7, y1r);
    simde__m128i psi_r_p7_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_5, y1r);
    simde__m128i psi_r_p7_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_3, y1r);
    simde__m128i psi_r_p7_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_1, y1r);
    simde__m128i psi_r_p7_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_1, y1r);
    simde__m128i psi_r_p7_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_3, y1r);
    simde__m128i psi_r_p7_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_5, y1r);
    simde__m128i psi_r_p7_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_7, y1r);
    simde__m128i psi_r_p7_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_7, y1r);
    simde__m128i psi_r_p5_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_5, y1r);
    simde__m128i psi_r_p5_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_3, y1r);
    simde__m128i psi_r_p5_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_1, y1r);
    simde__m128i psi_r_p5_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_1, y1r);
    simde__m128i psi_r_p5_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_3, y1r);
    simde__m128i psi_r_p5_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_5, y1r);
    simde__m128i psi_r_p5_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_7, y1r);
    simde__m128i psi_r_p5_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_7, y1r);
    simde__m128i psi_r_p3_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_5, y1r);
    simde__m128i psi_r_p3_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_3, y1r);
    simde__m128i psi_r_p3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_1, y1r);
    simde__m128i psi_r_p3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_1, y1r);
    simde__m128i psi_r_p3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_3, y1r);
    simde__m128i psi_r_p3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_5, y1r);
    simde__m128i psi_r_p3_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_7, y1r);
    simde__m128i psi_r_p3_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_7, y1r);
    simde__m128i psi_r_p1_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_5, y1r);
    simde__m128i psi_r_p1_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_3, y1r);
    simde__m128i psi_r_p1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_1, y1r);
    simde__m128i psi_r_p1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_1, y1r);
    simde__m128i psi_r_p1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_3, y1r);
    simde__m128i psi_r_p1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_5, y1r);
    simde__m128i psi_r_p1_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_7, y1r);
    simde__m128i psi_r_p1_m7 = simde_mm_abs_epi16(xmm2);

    xmm2 = simde_mm_adds_epi16(rho_rmi_1_7, y1r);
    simde__m128i psi_r_m1_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_5, y1r);
    simde__m128i psi_r_m1_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_3, y1r);
    simde__m128i psi_r_m1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_1, y1r);
    simde__m128i psi_r_m1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_1, y1r);
    simde__m128i psi_r_m1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_3, y1r);
    simde__m128i psi_r_m1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_5, y1r);
    simde__m128i psi_r_m1_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_7, y1r);
    simde__m128i psi_r_m1_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_7, y1r);
    simde__m128i psi_r_m3_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_5, y1r);
    simde__m128i psi_r_m3_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_3, y1r);
    simde__m128i psi_r_m3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_1, y1r);
    simde__m128i psi_r_m3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_1, y1r);
    simde__m128i psi_r_m3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_3, y1r);
    simde__m128i psi_r_m3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_5, y1r);
    simde__m128i psi_r_m3_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_7, y1r);
    simde__m128i psi_r_m3_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_7, y1r);
    simde__m128i psi_r_m5_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_5, y1r);
    simde__m128i psi_r_m5_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_3, y1r);
    simde__m128i psi_r_m5_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_1, y1r);
    simde__m128i psi_r_m5_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_1, y1r);
    simde__m128i psi_r_m5_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_3, y1r);
    simde__m128i psi_r_m5_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_5, y1r);
    simde__m128i psi_r_m5_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_7, y1r);
    simde__m128i psi_r_m5_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_7, y1r);
    simde__m128i psi_r_m7_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_5, y1r);
    simde__m128i psi_r_m7_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_3, y1r);
    simde__m128i psi_r_m7_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_1, y1r);
    simde__m128i psi_r_m7_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_1, y1r);
    simde__m128i psi_r_m7_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_3, y1r);
    simde__m128i psi_r_m7_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_5, y1r);
    simde__m128i psi_r_m7_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_7, y1r);
    simde__m128i psi_r_m7_m7 = simde_mm_abs_epi16(xmm2);

    // Simde__M128i Psi_i calculation from rho_rpi or rho_rmi
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_7, y1i);
    simde__m128i psi_i_p7_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_7, y1i);
    simde__m128i psi_i_p7_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_7, y1i);
    simde__m128i psi_i_p7_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_7, y1i);
    simde__m128i psi_i_p7_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_7, y1i);
    simde__m128i psi_i_p7_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_7, y1i);
    simde__m128i psi_i_p7_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_7, y1i);
    simde__m128i psi_i_p7_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_7, y1i);
    simde__m128i psi_i_p7_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_5, y1i);
    simde__m128i psi_i_p5_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_5, y1i);
    simde__m128i psi_i_p5_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_5, y1i);
    simde__m128i psi_i_p5_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_5, y1i);
    simde__m128i psi_i_p5_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_5, y1i);
    simde__m128i psi_i_p5_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_5, y1i);
    simde__m128i psi_i_p5_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_5, y1i);
    simde__m128i psi_i_p5_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_5, y1i);
    simde__m128i psi_i_p5_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_3, y1i);
    simde__m128i psi_i_p3_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_3, y1i);
    simde__m128i psi_i_p3_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_3, y1i);
    simde__m128i psi_i_p3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_3, y1i);
    simde__m128i psi_i_p3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_3, y1i);
    simde__m128i psi_i_p3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_3, y1i);
    simde__m128i psi_i_p3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_3, y1i);
    simde__m128i psi_i_p3_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_3, y1i);
    simde__m128i psi_i_p3_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_1, y1i);
    simde__m128i psi_i_p1_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_1, y1i);
    simde__m128i psi_i_p1_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_1, y1i);
    simde__m128i psi_i_p1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_1, y1i);
    simde__m128i psi_i_p1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_1, y1i);
    simde__m128i psi_i_p1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_1, y1i);
    simde__m128i psi_i_p1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_1, y1i);
    simde__m128i psi_i_p1_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_1, y1i);
    simde__m128i psi_i_p1_m7 = simde_mm_abs_epi16(xmm2);

    xmm2 = simde_mm_subs_epi16(rho_rpi_7_1, y1i);
    simde__m128i psi_i_m1_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_1, y1i);
    simde__m128i psi_i_m1_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_1, y1i);
    simde__m128i psi_i_m1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_1, y1i);
    simde__m128i psi_i_m1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_1, y1i);
    simde__m128i psi_i_m1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_1, y1i);
    simde__m128i psi_i_m1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_1, y1i);
    simde__m128i psi_i_m1_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_1, y1i);
    simde__m128i psi_i_m1_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_3, y1i);
    simde__m128i psi_i_m3_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_3, y1i);
    simde__m128i psi_i_m3_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_3, y1i);
    simde__m128i psi_i_m3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_3, y1i);
    simde__m128i psi_i_m3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_3, y1i);
    simde__m128i psi_i_m3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_3, y1i);
    simde__m128i psi_i_m3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_3, y1i);
    simde__m128i psi_i_m3_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_3, y1i);
    simde__m128i psi_i_m3_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_5, y1i);
    simde__m128i psi_i_m5_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_5, y1i);
    simde__m128i psi_i_m5_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_5, y1i);
    simde__m128i psi_i_m5_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_5, y1i);
    simde__m128i psi_i_m5_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_5, y1i);
    simde__m128i psi_i_m5_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_5, y1i);
    simde__m128i psi_i_m5_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_5, y1i);
    simde__m128i psi_i_m5_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_5, y1i);
    simde__m128i psi_i_m5_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_7, y1i);
    simde__m128i psi_i_m7_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_7, y1i);
    simde__m128i psi_i_m7_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_7, y1i);
    simde__m128i psi_i_m7_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_7, y1i);
    simde__m128i psi_i_m7_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_7, y1i);
    simde__m128i psi_i_m7_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_7, y1i);
    simde__m128i psi_i_m7_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_7, y1i);
    simde__m128i psi_i_m7_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_7, y1i);
    simde__m128i psi_i_m7_m7 = simde_mm_abs_epi16(xmm2);

    // Rearrange desired MF output
    xmm0 = stream0_128i_in[i];
    xmm1 = stream0_128i_in[i+1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y0r = simde_mm_unpacklo_epi64(xmm0, xmm1); // = [y0r(1),y0r(2),y0r(3),y0r(4)]
    simde__m128i y0i = simde_mm_unpackhi_epi64(xmm0, xmm1);

    // Rearrange desired channel magnitudes
    xmm2 = ch_mag_128i[i]; // = [|h|^2(1),|h|^2(1),|h|^2(2),|h|^2(2)]*(2/sqrt(10))
    xmm3 = ch_mag_128i[i+1]; // = [|h|^2(3),|h|^2(3),|h|^2(4),|h|^2(4)]*(2/sqrt(10))
    xmm2 = simde_mm_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    ch_mag_des = simde_mm_unpacklo_epi64(xmm2,xmm3);

    y0r_one_over_sqrt_21   = simde_mm_mulhi_epi16(y0r, ONE_OVER_SQRT_42);
    y0r_three_over_sqrt_21 = simde_mm_mulhi_epi16(y0r, THREE_OVER_SQRT_42);
    y0r_five_over_sqrt_21  = simde_mm_mulhi_epi16(y0r, FIVE_OVER_SQRT_42);
    y0r_five_over_sqrt_21  = simde_mm_slli_epi16(y0r_five_over_sqrt_21, 1);
    y0r_seven_over_sqrt_21 = simde_mm_mulhi_epi16(y0r, SEVEN_OVER_SQRT_42);
    y0r_seven_over_sqrt_21 = simde_mm_slli_epi16(y0r_seven_over_sqrt_21, 2); // Q2.14

    y0i_one_over_sqrt_21   = simde_mm_mulhi_epi16(y0i, ONE_OVER_SQRT_42);
    y0i_three_over_sqrt_21 = simde_mm_mulhi_epi16(y0i, THREE_OVER_SQRT_42);
    y0i_five_over_sqrt_21  = simde_mm_mulhi_epi16(y0i, FIVE_OVER_SQRT_42);
    y0i_five_over_sqrt_21  = simde_mm_slli_epi16(y0i_five_over_sqrt_21, 1);
    y0i_seven_over_sqrt_21 = simde_mm_mulhi_epi16(y0i, SEVEN_OVER_SQRT_42);
    y0i_seven_over_sqrt_21 = simde_mm_slli_epi16(y0i_seven_over_sqrt_21, 2); // Q2.14

    simde__m128i y0_p_7_1 = simde_mm_adds_epi16(y0r_seven_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_p_7_3 = simde_mm_adds_epi16(y0r_seven_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_p_7_5 = simde_mm_adds_epi16(y0r_seven_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_p_7_7 = simde_mm_adds_epi16(y0r_seven_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_p_5_1 = simde_mm_adds_epi16(y0r_five_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_p_5_3 = simde_mm_adds_epi16(y0r_five_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_p_5_5 = simde_mm_adds_epi16(y0r_five_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_p_5_7 = simde_mm_adds_epi16(y0r_five_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_p_3_1 = simde_mm_adds_epi16(y0r_three_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_p_3_3 = simde_mm_adds_epi16(y0r_three_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_p_3_5 = simde_mm_adds_epi16(y0r_three_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_p_3_7 = simde_mm_adds_epi16(y0r_three_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_p_1_1 = simde_mm_adds_epi16(y0r_one_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_p_1_3 = simde_mm_adds_epi16(y0r_one_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_p_1_5 = simde_mm_adds_epi16(y0r_one_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_p_1_7 = simde_mm_adds_epi16(y0r_one_over_sqrt_21, y0i_seven_over_sqrt_21);

    simde__m128i y0_m_1_1 = simde_mm_subs_epi16(y0r_one_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_m_1_3 = simde_mm_subs_epi16(y0r_one_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_m_1_5 = simde_mm_subs_epi16(y0r_one_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_m_1_7 = simde_mm_subs_epi16(y0r_one_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_m_3_1 = simde_mm_subs_epi16(y0r_three_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_m_3_3 = simde_mm_subs_epi16(y0r_three_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_m_3_5 = simde_mm_subs_epi16(y0r_three_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_m_3_7 = simde_mm_subs_epi16(y0r_three_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_m_5_1 = simde_mm_subs_epi16(y0r_five_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_m_5_3 = simde_mm_subs_epi16(y0r_five_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_m_5_5 = simde_mm_subs_epi16(y0r_five_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_m_5_7 = simde_mm_subs_epi16(y0r_five_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_m_7_1 = simde_mm_subs_epi16(y0r_seven_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_m_7_3 = simde_mm_subs_epi16(y0r_seven_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_m_7_5 = simde_mm_subs_epi16(y0r_seven_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_m_7_7 = simde_mm_subs_epi16(y0r_seven_over_sqrt_21, y0i_seven_over_sqrt_21);

    // divide by sqrt(2)
    psi_r_p7_p7 = simde_mm_mulhi_epi16(psi_r_p7_p7, ONE_OVER_SQRT_2);
    psi_r_p7_p7 = simde_mm_slli_epi16(psi_r_p7_p7, 1);
    psi_r_p7_p5 = simde_mm_mulhi_epi16(psi_r_p7_p5, ONE_OVER_SQRT_2);
    psi_r_p7_p5 = simde_mm_slli_epi16(psi_r_p7_p5, 1);
    psi_r_p7_p3 = simde_mm_mulhi_epi16(psi_r_p7_p3, ONE_OVER_SQRT_2);
    psi_r_p7_p3 = simde_mm_slli_epi16(psi_r_p7_p3, 1);
    psi_r_p7_p1 = simde_mm_mulhi_epi16(psi_r_p7_p1, ONE_OVER_SQRT_2);
    psi_r_p7_p1 = simde_mm_slli_epi16(psi_r_p7_p1, 1);
    psi_r_p7_m1 = simde_mm_mulhi_epi16(psi_r_p7_m1, ONE_OVER_SQRT_2);
    psi_r_p7_m1 = simde_mm_slli_epi16(psi_r_p7_m1, 1);
    psi_r_p7_m3 = simde_mm_mulhi_epi16(psi_r_p7_m3, ONE_OVER_SQRT_2);
    psi_r_p7_m3 = simde_mm_slli_epi16(psi_r_p7_m3, 1);
    psi_r_p7_m5 = simde_mm_mulhi_epi16(psi_r_p7_m5, ONE_OVER_SQRT_2);
    psi_r_p7_m5 = simde_mm_slli_epi16(psi_r_p7_m5, 1);
    psi_r_p7_m7 = simde_mm_mulhi_epi16(psi_r_p7_m7, ONE_OVER_SQRT_2);
    psi_r_p7_m7 = simde_mm_slli_epi16(psi_r_p7_m7, 1);
    psi_r_p5_p7 = simde_mm_mulhi_epi16(psi_r_p5_p7, ONE_OVER_SQRT_2);
    psi_r_p5_p7 = simde_mm_slli_epi16(psi_r_p5_p7, 1);
    psi_r_p5_p5 = simde_mm_mulhi_epi16(psi_r_p5_p5, ONE_OVER_SQRT_2);
    psi_r_p5_p5 = simde_mm_slli_epi16(psi_r_p5_p5, 1);
    psi_r_p5_p3 = simde_mm_mulhi_epi16(psi_r_p5_p3, ONE_OVER_SQRT_2);
    psi_r_p5_p3 = simde_mm_slli_epi16(psi_r_p5_p3, 1);
    psi_r_p5_p1 = simde_mm_mulhi_epi16(psi_r_p5_p1, ONE_OVER_SQRT_2);
    psi_r_p5_p1 = simde_mm_slli_epi16(psi_r_p5_p1, 1);
    psi_r_p5_m1 = simde_mm_mulhi_epi16(psi_r_p5_m1, ONE_OVER_SQRT_2);
    psi_r_p5_m1 = simde_mm_slli_epi16(psi_r_p5_m1, 1);
    psi_r_p5_m3 = simde_mm_mulhi_epi16(psi_r_p5_m3, ONE_OVER_SQRT_2);
    psi_r_p5_m3 = simde_mm_slli_epi16(psi_r_p5_m3, 1);
    psi_r_p5_m5 = simde_mm_mulhi_epi16(psi_r_p5_m5, ONE_OVER_SQRT_2);
    psi_r_p5_m5 = simde_mm_slli_epi16(psi_r_p5_m5, 1);
    psi_r_p5_m7 = simde_mm_mulhi_epi16(psi_r_p5_m7, ONE_OVER_SQRT_2);
    psi_r_p5_m7 = simde_mm_slli_epi16(psi_r_p5_m7, 1);
    psi_r_p3_p7 = simde_mm_mulhi_epi16(psi_r_p3_p7, ONE_OVER_SQRT_2);
    psi_r_p3_p7 = simde_mm_slli_epi16(psi_r_p3_p7, 1);
    psi_r_p3_p5 = simde_mm_mulhi_epi16(psi_r_p3_p5, ONE_OVER_SQRT_2);
    psi_r_p3_p5 = simde_mm_slli_epi16(psi_r_p3_p5, 1);
    psi_r_p3_p3 = simde_mm_mulhi_epi16(psi_r_p3_p3, ONE_OVER_SQRT_2);
    psi_r_p3_p3 = simde_mm_slli_epi16(psi_r_p3_p3, 1);
    psi_r_p3_p1 = simde_mm_mulhi_epi16(psi_r_p3_p1, ONE_OVER_SQRT_2);
    psi_r_p3_p1 = simde_mm_slli_epi16(psi_r_p3_p1, 1);
    psi_r_p3_m1 = simde_mm_mulhi_epi16(psi_r_p3_m1, ONE_OVER_SQRT_2);
    psi_r_p3_m1 = simde_mm_slli_epi16(psi_r_p3_m1, 1);
    psi_r_p3_m3 = simde_mm_mulhi_epi16(psi_r_p3_m3, ONE_OVER_SQRT_2);
    psi_r_p3_m3 = simde_mm_slli_epi16(psi_r_p3_m3, 1);
    psi_r_p3_m5 = simde_mm_mulhi_epi16(psi_r_p3_m5, ONE_OVER_SQRT_2);
    psi_r_p3_m5 = simde_mm_slli_epi16(psi_r_p3_m5, 1);
    psi_r_p3_m7 = simde_mm_mulhi_epi16(psi_r_p3_m7, ONE_OVER_SQRT_2);
    psi_r_p3_m7 = simde_mm_slli_epi16(psi_r_p3_m7, 1);
    psi_r_p1_p7 = simde_mm_mulhi_epi16(psi_r_p1_p7, ONE_OVER_SQRT_2);
    psi_r_p1_p7 = simde_mm_slli_epi16(psi_r_p1_p7, 1);
    psi_r_p1_p5 = simde_mm_mulhi_epi16(psi_r_p1_p5, ONE_OVER_SQRT_2);
    psi_r_p1_p5 = simde_mm_slli_epi16(psi_r_p1_p5, 1);
    psi_r_p1_p3 = simde_mm_mulhi_epi16(psi_r_p1_p3, ONE_OVER_SQRT_2);
    psi_r_p1_p3 = simde_mm_slli_epi16(psi_r_p1_p3, 1);
    psi_r_p1_p1 = simde_mm_mulhi_epi16(psi_r_p1_p1, ONE_OVER_SQRT_2);
    psi_r_p1_p1 = simde_mm_slli_epi16(psi_r_p1_p1, 1);
    psi_r_p1_m1 = simde_mm_mulhi_epi16(psi_r_p1_m1, ONE_OVER_SQRT_2);
    psi_r_p1_m1 = simde_mm_slli_epi16(psi_r_p1_m1, 1);
    psi_r_p1_m3 = simde_mm_mulhi_epi16(psi_r_p1_m3, ONE_OVER_SQRT_2);
    psi_r_p1_m3 = simde_mm_slli_epi16(psi_r_p1_m3, 1);
    psi_r_p1_m5 = simde_mm_mulhi_epi16(psi_r_p1_m5, ONE_OVER_SQRT_2);
    psi_r_p1_m5 = simde_mm_slli_epi16(psi_r_p1_m5, 1);
    psi_r_p1_m7 = simde_mm_mulhi_epi16(psi_r_p1_m7, ONE_OVER_SQRT_2);
    psi_r_p1_m7 = simde_mm_slli_epi16(psi_r_p1_m7, 1);
    psi_r_m1_p7 = simde_mm_mulhi_epi16(psi_r_m1_p7, ONE_OVER_SQRT_2);
    psi_r_m1_p7 = simde_mm_slli_epi16(psi_r_m1_p7, 1);
    psi_r_m1_p5 = simde_mm_mulhi_epi16(psi_r_m1_p5, ONE_OVER_SQRT_2);
    psi_r_m1_p5 = simde_mm_slli_epi16(psi_r_m1_p5, 1);
    psi_r_m1_p3 = simde_mm_mulhi_epi16(psi_r_m1_p3, ONE_OVER_SQRT_2);
    psi_r_m1_p3 = simde_mm_slli_epi16(psi_r_m1_p3, 1);
    psi_r_m1_p1 = simde_mm_mulhi_epi16(psi_r_m1_p1, ONE_OVER_SQRT_2);
    psi_r_m1_p1 = simde_mm_slli_epi16(psi_r_m1_p1, 1);
    psi_r_m1_m1 = simde_mm_mulhi_epi16(psi_r_m1_m1, ONE_OVER_SQRT_2);
    psi_r_m1_m1 = simde_mm_slli_epi16(psi_r_m1_m1, 1);
    psi_r_m1_m3 = simde_mm_mulhi_epi16(psi_r_m1_m3, ONE_OVER_SQRT_2);
    psi_r_m1_m3 = simde_mm_slli_epi16(psi_r_m1_m3, 1);
    psi_r_m1_m5 = simde_mm_mulhi_epi16(psi_r_m1_m5, ONE_OVER_SQRT_2);
    psi_r_m1_m5 = simde_mm_slli_epi16(psi_r_m1_m5, 1);
    psi_r_m1_m7 = simde_mm_mulhi_epi16(psi_r_m1_m7, ONE_OVER_SQRT_2);
    psi_r_m1_m7 = simde_mm_slli_epi16(psi_r_m1_m7, 1);
    psi_r_m3_p7 = simde_mm_mulhi_epi16(psi_r_m3_p7, ONE_OVER_SQRT_2);
    psi_r_m3_p7 = simde_mm_slli_epi16(psi_r_m3_p7, 1);
    psi_r_m3_p5 = simde_mm_mulhi_epi16(psi_r_m3_p5, ONE_OVER_SQRT_2);
    psi_r_m3_p5 = simde_mm_slli_epi16(psi_r_m3_p5, 1);
    psi_r_m3_p3 = simde_mm_mulhi_epi16(psi_r_m3_p3, ONE_OVER_SQRT_2);
    psi_r_m3_p3 = simde_mm_slli_epi16(psi_r_m3_p3, 1);
    psi_r_m3_p1 = simde_mm_mulhi_epi16(psi_r_m3_p1, ONE_OVER_SQRT_2);
    psi_r_m3_p1 = simde_mm_slli_epi16(psi_r_m3_p1, 1);
    psi_r_m3_m1 = simde_mm_mulhi_epi16(psi_r_m3_m1, ONE_OVER_SQRT_2);
    psi_r_m3_m1 = simde_mm_slli_epi16(psi_r_m3_m1, 1);
    psi_r_m3_m3 = simde_mm_mulhi_epi16(psi_r_m3_m3, ONE_OVER_SQRT_2);
    psi_r_m3_m3 = simde_mm_slli_epi16(psi_r_m3_m3, 1);
    psi_r_m3_m5 = simde_mm_mulhi_epi16(psi_r_m3_m5, ONE_OVER_SQRT_2);
    psi_r_m3_m5 = simde_mm_slli_epi16(psi_r_m3_m5, 1);
    psi_r_m3_m7 = simde_mm_mulhi_epi16(psi_r_m3_m7, ONE_OVER_SQRT_2);
    psi_r_m3_m7 = simde_mm_slli_epi16(psi_r_m3_m7, 1);
    psi_r_m5_p7 = simde_mm_mulhi_epi16(psi_r_m5_p7, ONE_OVER_SQRT_2);
    psi_r_m5_p7 = simde_mm_slli_epi16(psi_r_m5_p7, 1);
    psi_r_m5_p5 = simde_mm_mulhi_epi16(psi_r_m5_p5, ONE_OVER_SQRT_2);
    psi_r_m5_p5 = simde_mm_slli_epi16(psi_r_m5_p5, 1);
    psi_r_m5_p3 = simde_mm_mulhi_epi16(psi_r_m5_p3, ONE_OVER_SQRT_2);
    psi_r_m5_p3 = simde_mm_slli_epi16(psi_r_m5_p3, 1);
    psi_r_m5_p1 = simde_mm_mulhi_epi16(psi_r_m5_p1, ONE_OVER_SQRT_2);
    psi_r_m5_p1 = simde_mm_slli_epi16(psi_r_m5_p1, 1);
    psi_r_m5_m1 = simde_mm_mulhi_epi16(psi_r_m5_m1, ONE_OVER_SQRT_2);
    psi_r_m5_m1 = simde_mm_slli_epi16(psi_r_m5_m1, 1);
    psi_r_m5_m3 = simde_mm_mulhi_epi16(psi_r_m5_m3, ONE_OVER_SQRT_2);
    psi_r_m5_m3 = simde_mm_slli_epi16(psi_r_m5_m3, 1);
    psi_r_m5_m5 = simde_mm_mulhi_epi16(psi_r_m5_m5, ONE_OVER_SQRT_2);
    psi_r_m5_m5 = simde_mm_slli_epi16(psi_r_m5_m5, 1);
    psi_r_m5_m7 = simde_mm_mulhi_epi16(psi_r_m5_m7, ONE_OVER_SQRT_2);
    psi_r_m5_m7 = simde_mm_slli_epi16(psi_r_m5_m7, 1);
    psi_r_m7_p7 = simde_mm_mulhi_epi16(psi_r_m7_p7, ONE_OVER_SQRT_2);
    psi_r_m7_p7 = simde_mm_slli_epi16(psi_r_m7_p7, 1);
    psi_r_m7_p5 = simde_mm_mulhi_epi16(psi_r_m7_p5, ONE_OVER_SQRT_2);
    psi_r_m7_p5 = simde_mm_slli_epi16(psi_r_m7_p5, 1);
    psi_r_m7_p3 = simde_mm_mulhi_epi16(psi_r_m7_p3, ONE_OVER_SQRT_2);
    psi_r_m7_p3 = simde_mm_slli_epi16(psi_r_m7_p3, 1);
    psi_r_m7_p1 = simde_mm_mulhi_epi16(psi_r_m7_p1, ONE_OVER_SQRT_2);
    psi_r_m7_p1 = simde_mm_slli_epi16(psi_r_m7_p1, 1);
    psi_r_m7_m1 = simde_mm_mulhi_epi16(psi_r_m7_m1, ONE_OVER_SQRT_2);
    psi_r_m7_m1 = simde_mm_slli_epi16(psi_r_m7_m1, 1);
    psi_r_m7_m3 = simde_mm_mulhi_epi16(psi_r_m7_m3, ONE_OVER_SQRT_2);
    psi_r_m7_m3 = simde_mm_slli_epi16(psi_r_m7_m3, 1);
    psi_r_m7_m5 = simde_mm_mulhi_epi16(psi_r_m7_m5, ONE_OVER_SQRT_2);
    psi_r_m7_m5 = simde_mm_slli_epi16(psi_r_m7_m5, 1);
    psi_r_m7_m7 = simde_mm_mulhi_epi16(psi_r_m7_m7, ONE_OVER_SQRT_2);
    psi_r_m7_m7 = simde_mm_slli_epi16(psi_r_m7_m7, 1);

    psi_i_p7_p7 = simde_mm_mulhi_epi16(psi_i_p7_p7, ONE_OVER_SQRT_2);
    psi_i_p7_p7 = simde_mm_slli_epi16(psi_i_p7_p7, 1);
    psi_i_p7_p5 = simde_mm_mulhi_epi16(psi_i_p7_p5, ONE_OVER_SQRT_2);
    psi_i_p7_p5 = simde_mm_slli_epi16(psi_i_p7_p5, 1);
    psi_i_p7_p3 = simde_mm_mulhi_epi16(psi_i_p7_p3, ONE_OVER_SQRT_2);
    psi_i_p7_p3 = simde_mm_slli_epi16(psi_i_p7_p3, 1);
    psi_i_p7_p1 = simde_mm_mulhi_epi16(psi_i_p7_p1, ONE_OVER_SQRT_2);
    psi_i_p7_p1 = simde_mm_slli_epi16(psi_i_p7_p1, 1);
    psi_i_p7_m1 = simde_mm_mulhi_epi16(psi_i_p7_m1, ONE_OVER_SQRT_2);
    psi_i_p7_m1 = simde_mm_slli_epi16(psi_i_p7_m1, 1);
    psi_i_p7_m3 = simde_mm_mulhi_epi16(psi_i_p7_m3, ONE_OVER_SQRT_2);
    psi_i_p7_m3 = simde_mm_slli_epi16(psi_i_p7_m3, 1);
    psi_i_p7_m5 = simde_mm_mulhi_epi16(psi_i_p7_m5, ONE_OVER_SQRT_2);
    psi_i_p7_m5 = simde_mm_slli_epi16(psi_i_p7_m5, 1);
    psi_i_p7_m7 = simde_mm_mulhi_epi16(psi_i_p7_m7, ONE_OVER_SQRT_2);
    psi_i_p7_m7 = simde_mm_slli_epi16(psi_i_p7_m7, 1);
    psi_i_p5_p7 = simde_mm_mulhi_epi16(psi_i_p5_p7, ONE_OVER_SQRT_2);
    psi_i_p5_p7 = simde_mm_slli_epi16(psi_i_p5_p7, 1);
    psi_i_p5_p5 = simde_mm_mulhi_epi16(psi_i_p5_p5, ONE_OVER_SQRT_2);
    psi_i_p5_p5 = simde_mm_slli_epi16(psi_i_p5_p5, 1);
    psi_i_p5_p3 = simde_mm_mulhi_epi16(psi_i_p5_p3, ONE_OVER_SQRT_2);
    psi_i_p5_p3 = simde_mm_slli_epi16(psi_i_p5_p3, 1);
    psi_i_p5_p1 = simde_mm_mulhi_epi16(psi_i_p5_p1, ONE_OVER_SQRT_2);
    psi_i_p5_p1 = simde_mm_slli_epi16(psi_i_p5_p1, 1);
    psi_i_p5_m1 = simde_mm_mulhi_epi16(psi_i_p5_m1, ONE_OVER_SQRT_2);
    psi_i_p5_m1 = simde_mm_slli_epi16(psi_i_p5_m1, 1);
    psi_i_p5_m3 = simde_mm_mulhi_epi16(psi_i_p5_m3, ONE_OVER_SQRT_2);
    psi_i_p5_m3 = simde_mm_slli_epi16(psi_i_p5_m3, 1);
    psi_i_p5_m5 = simde_mm_mulhi_epi16(psi_i_p5_m5, ONE_OVER_SQRT_2);
    psi_i_p5_m5 = simde_mm_slli_epi16(psi_i_p5_m5, 1);
    psi_i_p5_m7 = simde_mm_mulhi_epi16(psi_i_p5_m7, ONE_OVER_SQRT_2);
    psi_i_p5_m7 = simde_mm_slli_epi16(psi_i_p5_m7, 1);
    psi_i_p3_p7 = simde_mm_mulhi_epi16(psi_i_p3_p7, ONE_OVER_SQRT_2);
    psi_i_p3_p7 = simde_mm_slli_epi16(psi_i_p3_p7, 1);
    psi_i_p3_p5 = simde_mm_mulhi_epi16(psi_i_p3_p5, ONE_OVER_SQRT_2);
    psi_i_p3_p5 = simde_mm_slli_epi16(psi_i_p3_p5, 1);
    psi_i_p3_p3 = simde_mm_mulhi_epi16(psi_i_p3_p3, ONE_OVER_SQRT_2);
    psi_i_p3_p3 = simde_mm_slli_epi16(psi_i_p3_p3, 1);
    psi_i_p3_p1 = simde_mm_mulhi_epi16(psi_i_p3_p1, ONE_OVER_SQRT_2);
    psi_i_p3_p1 = simde_mm_slli_epi16(psi_i_p3_p1, 1);
    psi_i_p3_m1 = simde_mm_mulhi_epi16(psi_i_p3_m1, ONE_OVER_SQRT_2);
    psi_i_p3_m1 = simde_mm_slli_epi16(psi_i_p3_m1, 1);
    psi_i_p3_m3 = simde_mm_mulhi_epi16(psi_i_p3_m3, ONE_OVER_SQRT_2);
    psi_i_p3_m3 = simde_mm_slli_epi16(psi_i_p3_m3, 1);
    psi_i_p3_m5 = simde_mm_mulhi_epi16(psi_i_p3_m5, ONE_OVER_SQRT_2);
    psi_i_p3_m5 = simde_mm_slli_epi16(psi_i_p3_m5, 1);
    psi_i_p3_m7 = simde_mm_mulhi_epi16(psi_i_p3_m7, ONE_OVER_SQRT_2);
    psi_i_p3_m7 = simde_mm_slli_epi16(psi_i_p3_m7, 1);
    psi_i_p1_p7 = simde_mm_mulhi_epi16(psi_i_p1_p7, ONE_OVER_SQRT_2);
    psi_i_p1_p7 = simde_mm_slli_epi16(psi_i_p1_p7, 1);
    psi_i_p1_p5 = simde_mm_mulhi_epi16(psi_i_p1_p5, ONE_OVER_SQRT_2);
    psi_i_p1_p5 = simde_mm_slli_epi16(psi_i_p1_p5, 1);
    psi_i_p1_p3 = simde_mm_mulhi_epi16(psi_i_p1_p3, ONE_OVER_SQRT_2);
    psi_i_p1_p3 = simde_mm_slli_epi16(psi_i_p1_p3, 1);
    psi_i_p1_p1 = simde_mm_mulhi_epi16(psi_i_p1_p1, ONE_OVER_SQRT_2);
    psi_i_p1_p1 = simde_mm_slli_epi16(psi_i_p1_p1, 1);
    psi_i_p1_m1 = simde_mm_mulhi_epi16(psi_i_p1_m1, ONE_OVER_SQRT_2);
    psi_i_p1_m1 = simde_mm_slli_epi16(psi_i_p1_m1, 1);
    psi_i_p1_m3 = simde_mm_mulhi_epi16(psi_i_p1_m3, ONE_OVER_SQRT_2);
    psi_i_p1_m3 = simde_mm_slli_epi16(psi_i_p1_m3, 1);
    psi_i_p1_m5 = simde_mm_mulhi_epi16(psi_i_p1_m5, ONE_OVER_SQRT_2);
    psi_i_p1_m5 = simde_mm_slli_epi16(psi_i_p1_m5, 1);
    psi_i_p1_m7 = simde_mm_mulhi_epi16(psi_i_p1_m7, ONE_OVER_SQRT_2);
    psi_i_p1_m7 = simde_mm_slli_epi16(psi_i_p1_m7, 1);
    psi_i_m1_p7 = simde_mm_mulhi_epi16(psi_i_m1_p7, ONE_OVER_SQRT_2);
    psi_i_m1_p7 = simde_mm_slli_epi16(psi_i_m1_p7, 1);
    psi_i_m1_p5 = simde_mm_mulhi_epi16(psi_i_m1_p5, ONE_OVER_SQRT_2);
    psi_i_m1_p5 = simde_mm_slli_epi16(psi_i_m1_p5, 1);
    psi_i_m1_p3 = simde_mm_mulhi_epi16(psi_i_m1_p3, ONE_OVER_SQRT_2);
    psi_i_m1_p3 = simde_mm_slli_epi16(psi_i_m1_p3, 1);
    psi_i_m1_p1 = simde_mm_mulhi_epi16(psi_i_m1_p1, ONE_OVER_SQRT_2);
    psi_i_m1_p1 = simde_mm_slli_epi16(psi_i_m1_p1, 1);
    psi_i_m1_m1 = simde_mm_mulhi_epi16(psi_i_m1_m1, ONE_OVER_SQRT_2);
    psi_i_m1_m1 = simde_mm_slli_epi16(psi_i_m1_m1, 1);
    psi_i_m1_m3 = simde_mm_mulhi_epi16(psi_i_m1_m3, ONE_OVER_SQRT_2);
    psi_i_m1_m3 = simde_mm_slli_epi16(psi_i_m1_m3, 1);
    psi_i_m1_m5 = simde_mm_mulhi_epi16(psi_i_m1_m5, ONE_OVER_SQRT_2);
    psi_i_m1_m5 = simde_mm_slli_epi16(psi_i_m1_m5, 1);
    psi_i_m1_m7 = simde_mm_mulhi_epi16(psi_i_m1_m7, ONE_OVER_SQRT_2);
    psi_i_m1_m7 = simde_mm_slli_epi16(psi_i_m1_m7, 1);
    psi_i_m3_p7 = simde_mm_mulhi_epi16(psi_i_m3_p7, ONE_OVER_SQRT_2);
    psi_i_m3_p7 = simde_mm_slli_epi16(psi_i_m3_p7, 1);
    psi_i_m3_p5 = simde_mm_mulhi_epi16(psi_i_m3_p5, ONE_OVER_SQRT_2);
    psi_i_m3_p5 = simde_mm_slli_epi16(psi_i_m3_p5, 1);
    psi_i_m3_p3 = simde_mm_mulhi_epi16(psi_i_m3_p3, ONE_OVER_SQRT_2);
    psi_i_m3_p3 = simde_mm_slli_epi16(psi_i_m3_p3, 1);
    psi_i_m3_p1 = simde_mm_mulhi_epi16(psi_i_m3_p1, ONE_OVER_SQRT_2);
    psi_i_m3_p1 = simde_mm_slli_epi16(psi_i_m3_p1, 1);
    psi_i_m3_m1 = simde_mm_mulhi_epi16(psi_i_m3_m1, ONE_OVER_SQRT_2);
    psi_i_m3_m1 = simde_mm_slli_epi16(psi_i_m3_m1, 1);
    psi_i_m3_m3 = simde_mm_mulhi_epi16(psi_i_m3_m3, ONE_OVER_SQRT_2);
    psi_i_m3_m3 = simde_mm_slli_epi16(psi_i_m3_m3, 1);
    psi_i_m3_m5 = simde_mm_mulhi_epi16(psi_i_m3_m5, ONE_OVER_SQRT_2);
    psi_i_m3_m5 = simde_mm_slli_epi16(psi_i_m3_m5, 1);
    psi_i_m3_m7 = simde_mm_mulhi_epi16(psi_i_m3_m7, ONE_OVER_SQRT_2);
    psi_i_m3_m7 = simde_mm_slli_epi16(psi_i_m3_m7, 1);
    psi_i_m5_p7 = simde_mm_mulhi_epi16(psi_i_m5_p7, ONE_OVER_SQRT_2);
    psi_i_m5_p7 = simde_mm_slli_epi16(psi_i_m5_p7, 1);
    psi_i_m5_p5 = simde_mm_mulhi_epi16(psi_i_m5_p5, ONE_OVER_SQRT_2);
    psi_i_m5_p5 = simde_mm_slli_epi16(psi_i_m5_p5, 1);
    psi_i_m5_p3 = simde_mm_mulhi_epi16(psi_i_m5_p3, ONE_OVER_SQRT_2);
    psi_i_m5_p3 = simde_mm_slli_epi16(psi_i_m5_p3, 1);
    psi_i_m5_p1 = simde_mm_mulhi_epi16(psi_i_m5_p1, ONE_OVER_SQRT_2);
    psi_i_m5_p1 = simde_mm_slli_epi16(psi_i_m5_p1, 1);
    psi_i_m5_m1 = simde_mm_mulhi_epi16(psi_i_m5_m1, ONE_OVER_SQRT_2);
    psi_i_m5_m1 = simde_mm_slli_epi16(psi_i_m5_m1, 1);
    psi_i_m5_m3 = simde_mm_mulhi_epi16(psi_i_m5_m3, ONE_OVER_SQRT_2);
    psi_i_m5_m3 = simde_mm_slli_epi16(psi_i_m5_m3, 1);
    psi_i_m5_m5 = simde_mm_mulhi_epi16(psi_i_m5_m5, ONE_OVER_SQRT_2);
    psi_i_m5_m5 = simde_mm_slli_epi16(psi_i_m5_m5, 1);
    psi_i_m5_m7 = simde_mm_mulhi_epi16(psi_i_m5_m7, ONE_OVER_SQRT_2);
    psi_i_m5_m7 = simde_mm_slli_epi16(psi_i_m5_m7, 1);
    psi_i_m7_p7 = simde_mm_mulhi_epi16(psi_i_m7_p7, ONE_OVER_SQRT_2);
    psi_i_m7_p7 = simde_mm_slli_epi16(psi_i_m7_p7, 1);
    psi_i_m7_p5 = simde_mm_mulhi_epi16(psi_i_m7_p5, ONE_OVER_SQRT_2);
    psi_i_m7_p5 = simde_mm_slli_epi16(psi_i_m7_p5, 1);
    psi_i_m7_p3 = simde_mm_mulhi_epi16(psi_i_m7_p3, ONE_OVER_SQRT_2);
    psi_i_m7_p3 = simde_mm_slli_epi16(psi_i_m7_p3, 1);
    psi_i_m7_p1 = simde_mm_mulhi_epi16(psi_i_m7_p1, ONE_OVER_SQRT_2);
    psi_i_m7_p1 = simde_mm_slli_epi16(psi_i_m7_p1, 1);
    psi_i_m7_m1 = simde_mm_mulhi_epi16(psi_i_m7_m1, ONE_OVER_SQRT_2);
    psi_i_m7_m1 = simde_mm_slli_epi16(psi_i_m7_m1, 1);
    psi_i_m7_m3 = simde_mm_mulhi_epi16(psi_i_m7_m3, ONE_OVER_SQRT_2);
    psi_i_m7_m3 = simde_mm_slli_epi16(psi_i_m7_m3, 1);
    psi_i_m7_m5 = simde_mm_mulhi_epi16(psi_i_m7_m5, ONE_OVER_SQRT_2);
    psi_i_m7_m5 = simde_mm_slli_epi16(psi_i_m7_m5, 1);
    psi_i_m7_m7 = simde_mm_mulhi_epi16(psi_i_m7_m7, ONE_OVER_SQRT_2);
    psi_i_m7_m7 = simde_mm_slli_epi16(psi_i_m7_m7, 1);

    simde__m128i psi_a_p7_p7 = simde_mm_adds_epi16(psi_r_p7_p7, psi_i_p7_p7);
    simde__m128i psi_a_p7_p5 = simde_mm_adds_epi16(psi_r_p7_p5, psi_i_p7_p5);
    simde__m128i psi_a_p7_p3 = simde_mm_adds_epi16(psi_r_p7_p3, psi_i_p7_p3);
    simde__m128i psi_a_p7_p1 = simde_mm_adds_epi16(psi_r_p7_p1, psi_i_p7_p1);
    simde__m128i psi_a_p7_m1 = simde_mm_adds_epi16(psi_r_p7_m1, psi_i_p7_m1);
    simde__m128i psi_a_p7_m3 = simde_mm_adds_epi16(psi_r_p7_m3, psi_i_p7_m3);
    simde__m128i psi_a_p7_m5 = simde_mm_adds_epi16(psi_r_p7_m5, psi_i_p7_m5);
    simde__m128i psi_a_p7_m7 = simde_mm_adds_epi16(psi_r_p7_m7, psi_i_p7_m7);
    simde__m128i psi_a_p5_p7 = simde_mm_adds_epi16(psi_r_p5_p7, psi_i_p5_p7);
    simde__m128i psi_a_p5_p5 = simde_mm_adds_epi16(psi_r_p5_p5, psi_i_p5_p5);
    simde__m128i psi_a_p5_p3 = simde_mm_adds_epi16(psi_r_p5_p3, psi_i_p5_p3);
    simde__m128i psi_a_p5_p1 = simde_mm_adds_epi16(psi_r_p5_p1, psi_i_p5_p1);
    simde__m128i psi_a_p5_m1 = simde_mm_adds_epi16(psi_r_p5_m1, psi_i_p5_m1);
    simde__m128i psi_a_p5_m3 = simde_mm_adds_epi16(psi_r_p5_m3, psi_i_p5_m3);
    simde__m128i psi_a_p5_m5 = simde_mm_adds_epi16(psi_r_p5_m5, psi_i_p5_m5);
    simde__m128i psi_a_p5_m7 = simde_mm_adds_epi16(psi_r_p5_m7, psi_i_p5_m7);
    simde__m128i psi_a_p3_p7 = simde_mm_adds_epi16(psi_r_p3_p7, psi_i_p3_p7);
    simde__m128i psi_a_p3_p5 = simde_mm_adds_epi16(psi_r_p3_p5, psi_i_p3_p5);
    simde__m128i psi_a_p3_p3 = simde_mm_adds_epi16(psi_r_p3_p3, psi_i_p3_p3);
    simde__m128i psi_a_p3_p1 = simde_mm_adds_epi16(psi_r_p3_p1, psi_i_p3_p1);
    simde__m128i psi_a_p3_m1 = simde_mm_adds_epi16(psi_r_p3_m1, psi_i_p3_m1);
    simde__m128i psi_a_p3_m3 = simde_mm_adds_epi16(psi_r_p3_m3, psi_i_p3_m3);
    simde__m128i psi_a_p3_m5 = simde_mm_adds_epi16(psi_r_p3_m5, psi_i_p3_m5);
    simde__m128i psi_a_p3_m7 = simde_mm_adds_epi16(psi_r_p3_m7, psi_i_p3_m7);
    simde__m128i psi_a_p1_p7 = simde_mm_adds_epi16(psi_r_p1_p7, psi_i_p1_p7);
    simde__m128i psi_a_p1_p5 = simde_mm_adds_epi16(psi_r_p1_p5, psi_i_p1_p5);
    simde__m128i psi_a_p1_p3 = simde_mm_adds_epi16(psi_r_p1_p3, psi_i_p1_p3);
    simde__m128i psi_a_p1_p1 = simde_mm_adds_epi16(psi_r_p1_p1, psi_i_p1_p1);
    simde__m128i psi_a_p1_m1 = simde_mm_adds_epi16(psi_r_p1_m1, psi_i_p1_m1);
    simde__m128i psi_a_p1_m3 = simde_mm_adds_epi16(psi_r_p1_m3, psi_i_p1_m3);
    simde__m128i psi_a_p1_m5 = simde_mm_adds_epi16(psi_r_p1_m5, psi_i_p1_m5);
    simde__m128i psi_a_p1_m7 = simde_mm_adds_epi16(psi_r_p1_m7, psi_i_p1_m7);
    simde__m128i psi_a_m1_p7 = simde_mm_adds_epi16(psi_r_m1_p7, psi_i_m1_p7);
    simde__m128i psi_a_m1_p5 = simde_mm_adds_epi16(psi_r_m1_p5, psi_i_m1_p5);
    simde__m128i psi_a_m1_p3 = simde_mm_adds_epi16(psi_r_m1_p3, psi_i_m1_p3);
    simde__m128i psi_a_m1_p1 = simde_mm_adds_epi16(psi_r_m1_p1, psi_i_m1_p1);
    simde__m128i psi_a_m1_m1 = simde_mm_adds_epi16(psi_r_m1_m1, psi_i_m1_m1);
    simde__m128i psi_a_m1_m3 = simde_mm_adds_epi16(psi_r_m1_m3, psi_i_m1_m3);
    simde__m128i psi_a_m1_m5 = simde_mm_adds_epi16(psi_r_m1_m5, psi_i_m1_m5);
    simde__m128i psi_a_m1_m7 = simde_mm_adds_epi16(psi_r_m1_m7, psi_i_m1_m7);
    simde__m128i psi_a_m3_p7 = simde_mm_adds_epi16(psi_r_m3_p7, psi_i_m3_p7);
    simde__m128i psi_a_m3_p5 = simde_mm_adds_epi16(psi_r_m3_p5, psi_i_m3_p5);
    simde__m128i psi_a_m3_p3 = simde_mm_adds_epi16(psi_r_m3_p3, psi_i_m3_p3);
    simde__m128i psi_a_m3_p1 = simde_mm_adds_epi16(psi_r_m3_p1, psi_i_m3_p1);
    simde__m128i psi_a_m3_m1 = simde_mm_adds_epi16(psi_r_m3_m1, psi_i_m3_m1);
    simde__m128i psi_a_m3_m3 = simde_mm_adds_epi16(psi_r_m3_m3, psi_i_m3_m3);
    simde__m128i psi_a_m3_m5 = simde_mm_adds_epi16(psi_r_m3_m5, psi_i_m3_m5);
    simde__m128i psi_a_m3_m7 = simde_mm_adds_epi16(psi_r_m3_m7, psi_i_m3_m7);
    simde__m128i psi_a_m5_p7 = simde_mm_adds_epi16(psi_r_m5_p7, psi_i_m5_p7);
    simde__m128i psi_a_m5_p5 = simde_mm_adds_epi16(psi_r_m5_p5, psi_i_m5_p5);
    simde__m128i psi_a_m5_p3 = simde_mm_adds_epi16(psi_r_m5_p3, psi_i_m5_p3);
    simde__m128i psi_a_m5_p1 = simde_mm_adds_epi16(psi_r_m5_p1, psi_i_m5_p1);
    simde__m128i psi_a_m5_m1 = simde_mm_adds_epi16(psi_r_m5_m1, psi_i_m5_m1);
    simde__m128i psi_a_m5_m3 = simde_mm_adds_epi16(psi_r_m5_m3, psi_i_m5_m3);
    simde__m128i psi_a_m5_m5 = simde_mm_adds_epi16(psi_r_m5_m5, psi_i_m5_m5);
    simde__m128i psi_a_m5_m7 = simde_mm_adds_epi16(psi_r_m5_m7, psi_i_m5_m7);
    simde__m128i psi_a_m7_p7 = simde_mm_adds_epi16(psi_r_m7_p7, psi_i_m7_p7);
    simde__m128i psi_a_m7_p5 = simde_mm_adds_epi16(psi_r_m7_p5, psi_i_m7_p5);
    simde__m128i psi_a_m7_p3 = simde_mm_adds_epi16(psi_r_m7_p3, psi_i_m7_p3);
    simde__m128i psi_a_m7_p1 = simde_mm_adds_epi16(psi_r_m7_p1, psi_i_m7_p1);
    simde__m128i psi_a_m7_m1 = simde_mm_adds_epi16(psi_r_m7_m1, psi_i_m7_m1);
    simde__m128i psi_a_m7_m3 = simde_mm_adds_epi16(psi_r_m7_m3, psi_i_m7_m3);
    simde__m128i psi_a_m7_m5 = simde_mm_adds_epi16(psi_r_m7_m5, psi_i_m7_m5);
    simde__m128i psi_a_m7_m7 = simde_mm_adds_epi16(psi_r_m7_m7, psi_i_m7_m7);

    // Computing different multiples of ||h0||^2
    // x=1, y=1
    ch_mag_2_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,ONE_OVER_FOUR_SQRT_42);
    ch_mag_2_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_2_over_42_with_sigma2,1);
    // x=1, y=3
    ch_mag_10_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,FIVE_OVER_FOUR_SQRT_42);
    ch_mag_10_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_10_over_42_with_sigma2,1);
    // x=1, x=5
    ch_mag_26_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,THIRTEEN_OVER_FOUR_SQRT_42);
    ch_mag_26_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_26_over_42_with_sigma2,1);
    // x=1, y=7
    ch_mag_50_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,TWENTYFIVE_OVER_FOUR_SQRT_42);
    ch_mag_50_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_50_over_42_with_sigma2,1);
    // x=3, y=3
    ch_mag_18_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,NINE_OVER_FOUR_SQRT_42);
    ch_mag_18_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_18_over_42_with_sigma2,1);
    // x=3, y=5
    ch_mag_34_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,SEVENTEEN_OVER_FOUR_SQRT_42);
    ch_mag_34_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_34_over_42_with_sigma2,1);
    // x=3, y=7
    ch_mag_58_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,TWENTYNINE_OVER_FOUR_SQRT_42);
    ch_mag_58_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_58_over_42_with_sigma2,2);
    // x=5, y=5
    ch_mag_50_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,TWENTYFIVE_OVER_FOUR_SQRT_42);
    ch_mag_50_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_50_over_42_with_sigma2,1);
    // x=5, y=7
    ch_mag_74_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,THIRTYSEVEN_OVER_FOUR_SQRT_42);
    ch_mag_74_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_74_over_42_with_sigma2,2);
    // x=7, y=7
    ch_mag_98_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,FORTYNINE_OVER_FOUR_SQRT_42);
    ch_mag_98_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_98_over_42_with_sigma2,2);

    // Computing Metrics
    xmm1 = simde_mm_adds_epi16(psi_a_p7_p7, y0_p_7_7);
    simde__m128i bit_met_p7_p7 = simde_mm_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p7_p5, y0_p_7_5);
    simde__m128i bit_met_p7_p5 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p7_p3, y0_p_7_3);
    simde__m128i bit_met_p7_p3 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p7_p1, y0_p_7_1);
    simde__m128i bit_met_p7_p1 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p7_m1, y0_m_7_1);
    simde__m128i bit_met_p7_m1 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p7_m3, y0_m_7_3);
    simde__m128i bit_met_p7_m3 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p7_m5, y0_m_7_5);
    simde__m128i bit_met_p7_m5 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p7_m7, y0_m_7_7);
    simde__m128i bit_met_p7_m7 = simde_mm_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p5_p7, y0_p_5_7);
    simde__m128i bit_met_p5_p7 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p5_p5, y0_p_5_5);
    simde__m128i bit_met_p5_p5 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p5_p3, y0_p_5_3);
    simde__m128i bit_met_p5_p3 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p5_p1, y0_p_5_1);
    simde__m128i bit_met_p5_p1 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p5_m1, y0_m_5_1);
    simde__m128i bit_met_p5_m1 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p5_m3, y0_m_5_3);
    simde__m128i bit_met_p5_m3 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p5_m5, y0_m_5_5);
    simde__m128i bit_met_p5_m5 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p5_m7, y0_m_5_7);
    simde__m128i bit_met_p5_m7 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p3_p7, y0_p_3_7);
    simde__m128i bit_met_p3_p7 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p3_p5, y0_p_3_5);
    simde__m128i bit_met_p3_p5 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p3_p3, y0_p_3_3);
    simde__m128i bit_met_p3_p3 = simde_mm_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p3_p1, y0_p_3_1);
    simde__m128i bit_met_p3_p1 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p3_m1, y0_m_3_1);
    simde__m128i bit_met_p3_m1 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p3_m3, y0_m_3_3);
    simde__m128i bit_met_p3_m3 = simde_mm_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p3_m5, y0_m_3_5);
    simde__m128i bit_met_p3_m5 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p3_m7, y0_m_3_7);
    simde__m128i bit_met_p3_m7 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p1_p7, y0_p_1_7);
    simde__m128i bit_met_p1_p7 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p1_p5, y0_p_1_5);
    simde__m128i bit_met_p1_p5 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p1_p3, y0_p_1_3);
    simde__m128i bit_met_p1_p3 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p1_p1, y0_p_1_1);
    simde__m128i bit_met_p1_p1 = simde_mm_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p1_m1, y0_m_1_1);
    simde__m128i bit_met_p1_m1 = simde_mm_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p1_m3, y0_m_1_3);
    simde__m128i bit_met_p1_m3 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p1_m5, y0_m_1_5);
    simde__m128i bit_met_p1_m5 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm1 = simde_mm_adds_epi16(psi_a_p1_m7, y0_m_1_7);
    simde__m128i bit_met_p1_m7 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);

    xmm1 = simde_mm_subs_epi16(psi_a_m1_p7, y0_m_1_7);
    simde__m128i bit_met_m1_p7 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m1_p5, y0_m_1_5);
    simde__m128i bit_met_m1_p5 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m1_p3, y0_m_1_3);
    simde__m128i bit_met_m1_p3 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m1_p1, y0_m_1_1);
    simde__m128i bit_met_m1_p1 = simde_mm_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m1_m1, y0_p_1_1);
    simde__m128i bit_met_m1_m1 = simde_mm_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m1_m3, y0_p_1_3);
    simde__m128i bit_met_m1_m3 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m1_m5, y0_p_1_5);
    simde__m128i bit_met_m1_m5 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m1_m7, y0_p_1_7);
    simde__m128i bit_met_m1_m7 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m3_p7, y0_m_3_7);
    simde__m128i bit_met_m3_p7 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m3_p5, y0_m_3_5);
    simde__m128i bit_met_m3_p5 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m3_p3, y0_m_3_3);
    simde__m128i bit_met_m3_p3 = simde_mm_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m3_p1, y0_m_3_1);
    simde__m128i bit_met_m3_p1 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m3_m1, y0_p_3_1);
    simde__m128i bit_met_m3_m1 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m3_m3, y0_p_3_3);
    simde__m128i bit_met_m3_m3 = simde_mm_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m3_m5, y0_p_3_5);
    simde__m128i bit_met_m3_m5 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m3_m7, y0_p_3_7);
    simde__m128i bit_met_m3_m7 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m5_p7, y0_m_5_7);
    simde__m128i bit_met_m5_p7 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m5_p5, y0_m_5_5);
    simde__m128i bit_met_m5_p5 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m5_p3, y0_m_5_3);
    simde__m128i bit_met_m5_p3 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m5_p1, y0_m_5_1);
    simde__m128i bit_met_m5_p1 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m5_m1, y0_p_5_1);
    simde__m128i bit_met_m5_m1 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m5_m3, y0_p_5_3);
    simde__m128i bit_met_m5_m3 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m5_m5, y0_p_5_5);
    simde__m128i bit_met_m5_m5 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m5_m7, y0_p_5_7);
    simde__m128i bit_met_m5_m7 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m7_p7, y0_m_7_7);
    simde__m128i bit_met_m7_p7 = simde_mm_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m7_p5, y0_m_7_5);
    simde__m128i bit_met_m7_p5 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m7_p3, y0_m_7_3);
    simde__m128i bit_met_m7_p3 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m7_p1, y0_m_7_1);
    simde__m128i bit_met_m7_p1 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m7_m1, y0_p_7_1);
    simde__m128i bit_met_m7_m1 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m7_m3, y0_p_7_3);
    simde__m128i bit_met_m7_m3 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m7_m5, y0_p_7_5);
    simde__m128i bit_met_m7_m5 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm1 = simde_mm_subs_epi16(psi_a_m7_m7, y0_p_7_7);
    simde__m128i bit_met_m7_m7 = simde_mm_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);

    // Detection for 1st bit (LTE mapping)
    // bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_m7_p7, bit_met_m7_p5);
    xmm1 = simde_mm_max_epi16(bit_met_m7_p3, bit_met_m7_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m7_m1, bit_met_m7_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m7_m5, bit_met_m7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m5_p7, bit_met_m5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_m5_p3, bit_met_m5_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m5_m1, bit_met_m5_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m5, bit_met_m5_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m3_p7, bit_met_m3_p5);
    xmm1 = simde_mm_max_epi16(bit_met_m3_p3, bit_met_m3_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m3_m1, bit_met_m3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m3_m5, bit_met_m3_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m1_p7, bit_met_m1_p5);
    xmm1 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m1_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_m5, bit_met_m1_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    // bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p7_p7, bit_met_p7_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p7_p3, bit_met_p7_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p7_m1, bit_met_p7_m3);
    xmm3 = simde_mm_max_epi16(bit_met_p7_m5, bit_met_p7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p5_p7, bit_met_p5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p5_p3, bit_met_p5_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p5_m1, bit_met_p5_m3);
    xmm3 = simde_mm_max_epi16(bit_met_p5_m5, bit_met_p5_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p3_p7, bit_met_p3_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p3, bit_met_p3_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_m5, bit_met_p3_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p1_p7, bit_met_p1_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p1_p3, bit_met_p1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p1_m3);
    xmm3 = simde_mm_max_epi16(bit_met_p1_m5, bit_met_p1_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    y0r = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 2nd bit (LTE mapping)
    // bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    // bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m7_p1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m7_p5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    y1r = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 3rd bit (LTE mapping)
    xmm0 = simde_mm_max_epi16(bit_met_m7_m7, bit_met_m7_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m7_m3, bit_met_m7_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m7_p1, bit_met_m7_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m7_p5, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m5_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m5_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m5_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p5_m7, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p5_m3, bit_met_p5_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p5_p1, bit_met_p5_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p5_p5, bit_met_p5_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p7_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p7_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p7_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm_max_epi16(bit_met_m3_m7, bit_met_m3_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m3_m3, bit_met_m3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m3_p5, bit_met_m3_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m1_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m1_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p1_m7, bit_met_p1_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p1_m3, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p1_p5, bit_met_p1_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p3_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p3_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    simde__m128i y2r = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 4th bit (LTE mapping)
    xmm0 = simde_mm_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m5_p5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m7_p1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    y0i = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);


    // Detection for 5th bit (LTE mapping)
    xmm0 = simde_mm_max_epi16(bit_met_m7_m7, bit_met_m7_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m7_m3, bit_met_m7_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m7_p1, bit_met_m7_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m7_p5, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m1_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m1_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p1_m7, bit_met_p1_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p1_m3, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p1_p5, bit_met_p1_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p7_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p7_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p7_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m5_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m5_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m5_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m3_m7, bit_met_m3_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m3_m3, bit_met_m3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m3_p5, bit_met_m3_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p3_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p3_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p5_m7, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p5_m3, bit_met_p5_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p5_p1, bit_met_p5_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p5_p5, bit_met_p5_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    y1i = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 6th bit (LTE mapping)
    xmm0 = simde_mm_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m5_p1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m7_p5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    simde__m128i y2i = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // map to output stream, difficult to do in SIMD since we have 6 16bit LLRs
    // RE 1
    j = 24*i;
    stream0_out[j + 0] = ((short *)&y0r)[0];
    stream0_out[j + 1] = ((short *)&y1r)[0];
    stream0_out[j + 2] = ((short *)&y2r)[0];
    stream0_out[j + 3] = ((short *)&y0i)[0];
    stream0_out[j + 4] = ((short *)&y1i)[0];
    stream0_out[j + 5] = ((short *)&y2i)[0];
    // RE 2
    stream0_out[j + 6] = ((short *)&y0r)[1];
    stream0_out[j + 7] = ((short *)&y1r)[1];
    stream0_out[j + 8] = ((short *)&y2r)[1];
    stream0_out[j + 9] = ((short *)&y0i)[1];
    stream0_out[j + 10] = ((short *)&y1i)[1];
    stream0_out[j + 11] = ((short *)&y2i)[1];
    // RE 3
    stream0_out[j + 12] = ((short *)&y0r)[2];
    stream0_out[j + 13] = ((short *)&y1r)[2];
    stream0_out[j + 14] = ((short *)&y2r)[2];
    stream0_out[j + 15] = ((short *)&y0i)[2];
    stream0_out[j + 16] = ((short *)&y1i)[2];
    stream0_out[j + 17] = ((short *)&y2i)[2];
    // RE 4
    stream0_out[j + 18] = ((short *)&y0r)[3];
    stream0_out[j + 19] = ((short *)&y1r)[3];
    stream0_out[j + 20] = ((short *)&y2r)[3];
    stream0_out[j + 21] = ((short *)&y0i)[3];
    stream0_out[j + 22] = ((short *)&y1i)[3];
    stream0_out[j + 23] = ((short *)&y2i)[3];
    // RE 5
    stream0_out[j + 24] = ((short *)&y0r)[4];
    stream0_out[j + 25] = ((short *)&y1r)[4];
    stream0_out[j + 26] = ((short *)&y2r)[4];
    stream0_out[j + 27] = ((short *)&y0i)[4];
    stream0_out[j + 28] = ((short *)&y1i)[4];
    stream0_out[j + 29] = ((short *)&y2i)[4];
    // RE 6
    stream0_out[j + 30] = ((short *)&y0r)[5];
    stream0_out[j + 31] = ((short *)&y1r)[5];
    stream0_out[j + 32] = ((short *)&y2r)[5];
    stream0_out[j + 33] = ((short *)&y0i)[5];
    stream0_out[j + 34] = ((short *)&y1i)[5];
    stream0_out[j + 35] = ((short *)&y2i)[5];
    // RE 7
    stream0_out[j + 36] = ((short *)&y0r)[6];
    stream0_out[j + 37] = ((short *)&y1r)[6];
    stream0_out[j + 38] = ((short *)&y2r)[6];
    stream0_out[j + 39] = ((short *)&y0i)[6];
    stream0_out[j + 40] = ((short *)&y1i)[6];
    stream0_out[j + 41] = ((short *)&y2i)[6];
    // RE 8
    stream0_out[j + 42] = ((short *)&y0r)[7];
    stream0_out[j + 43] = ((short *)&y1r)[7];
    stream0_out[j + 44] = ((short *)&y2r)[7];
    stream0_out[j + 45] = ((short *)&y0i)[7];
    stream0_out[j + 46] = ((short *)&y1i)[7];
    stream0_out[j + 47] = ((short *)&y2i)[7];
  }
}


int dlsch_64qam_qpsk_llr(LTE_DL_FRAME_PARMS *frame_parms,
                         int32_t **rxdataF_comp,
                         int32_t **rxdataF_comp_i,
                         int32_t **dl_ch_mag,
                         int32_t **rho_i,
                         int16_t *dlsch_llr,
                         uint8_t symbol,
                         uint8_t first_symbol_flag,
                         uint16_t nb_rb,
                         uint16_t pbch_pss_sss_adjust,
                         int16_t **llr16p)
{

  int16_t *rxF      = (int16_t*)&rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rxF_i    = (int16_t*)&rxdataF_comp_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *ch_mag   = (int16_t*)&dl_ch_mag[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rho      = (int16_t*)&rho_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *llr16;
  int len;
  uint8_t symbol_mod = (symbol >= (7-frame_parms->Ncp))? (symbol-(7-frame_parms->Ncp)) : symbol;

  //first symbol has different structure due to more pilots
  if (first_symbol_flag == 1) {
    llr16 = (int16_t*)dlsch_llr;
  } else {
    llr16 = (int16_t*)(*llr16p);
  }

  AssertFatal(llr16!=NULL,"dlsch_16qam_64qam_llr:llr is null, symbol %d\n",symbol);

  if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
    // if symbol has pilots
    if (frame_parms->nb_antenna_ports_eNB!=1)
      // in 2 antenna ports we have 8 REs per symbol per RB
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
    else
      // for 1 antenna port we have 10 REs per symbol per RB
      len = (nb_rb*10) - (5*pbch_pss_sss_adjust/6);
  } else {
    // symbol has no pilots
    len = (nb_rb*12) - pbch_pss_sss_adjust;
  }

  qam64_qpsk((short *)rxF,
             (short *)rxF_i,
             (short *)ch_mag,
             (short *)llr16,
             (short *)rho,
             len);

  llr16 += (6*len);
  *llr16p = (short *)llr16;
  return(0);
}



void qam64_qam16(short *stream0_in,
                 short *stream1_in,
                 short *ch_mag,
                 short *ch_mag_i,
                 short *stream0_out,
                 short *rho01,
                 int length
     )
{

  /*
    Author: S. Wagner
    Date: 31-07-12

    Input:
    stream0_in:  MF filter for 1st stream, i.e., y0=h0'*y
    stream1_in:  MF filter for 2nd stream, i.e., y1=h1'*y
    ch_mag:      4*h0/sqrt(42), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    ch_mag_i:    4*h1/sqrt(42), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    rho01:       Channel cross correlation, i.e., h1'*h0

    Output:
    stream0_out: output LLRs for 1st stream
  */


  simde__m128i *rho01_128i      = (simde__m128i *)rho01;
  simde__m128i *stream0_128i_in = (simde__m128i *)stream0_in;
  simde__m128i *stream1_128i_in = (simde__m128i *)stream1_in;
  simde__m128i *ch_mag_128i     = (simde__m128i *)ch_mag;
  simde__m128i *ch_mag_128i_i   = (simde__m128i *)ch_mag_i;

  simde__m128i ONE_OVER_SQRT_42 = simde_mm_set1_epi16(10112); // round(1/sqrt(42)*2^16)
  simde__m128i THREE_OVER_SQRT_42 = simde_mm_set1_epi16(30337); // round(3/sqrt(42)*2^16)
  simde__m128i FIVE_OVER_SQRT_42 = simde_mm_set1_epi16(25281); // round(5/sqrt(42)*2^15)
  simde__m128i SEVEN_OVER_SQRT_42 = simde_mm_set1_epi16(17697); // round(5/sqrt(42)*2^15)
  simde__m128i FORTYNINE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(30969); // round(49/(4*sqrt(42))*2^14), Q2.14
  simde__m128i THIRTYSEVEN_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(23385); // round(37/(4*sqrt(42))*2^14), Q2.14
  simde__m128i TWENTYFIVE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(31601); // round(25/(4*sqrt(42))*2^15)
  simde__m128i TWENTYNINE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(18329); // round(29/(4*sqrt(42))*2^15), Q2.14
  simde__m128i SEVENTEEN_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(21489); // round(17/(4*sqrt(42))*2^15)
  simde__m128i NINE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(11376); // round(9/(4*sqrt(42))*2^15)
  simde__m128i THIRTEEN_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(16433); // round(13/(4*sqrt(42))*2^15)
  simde__m128i FIVE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(6320); // round(5/(4*sqrt(42))*2^15)
  simde__m128i ONE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(1264); // round(1/(4*sqrt(42))*2^15)
  simde__m128i ONE_OVER_SQRT_10_Q15 = simde_mm_set1_epi16(10362); // round(1/sqrt(10)*2^15)
  simde__m128i THREE_OVER_SQRT_10 = simde_mm_set1_epi16(31086); // round(3/sqrt(10)*2^15)
  simde__m128i SQRT_10_OVER_FOUR = simde_mm_set1_epi16(25905); // round(sqrt(10)/4*2^15)


  simde__m128i ch_mag_int;
  simde__m128i ch_mag_des;
  simde__m128i ch_mag_98_over_42_with_sigma2;
  simde__m128i ch_mag_74_over_42_with_sigma2;
  simde__m128i ch_mag_58_over_42_with_sigma2;
  simde__m128i ch_mag_50_over_42_with_sigma2;
  simde__m128i ch_mag_34_over_42_with_sigma2;
  simde__m128i ch_mag_18_over_42_with_sigma2;
  simde__m128i ch_mag_26_over_42_with_sigma2;
  simde__m128i ch_mag_10_over_42_with_sigma2;
  simde__m128i ch_mag_2_over_42_with_sigma2;
  simde__m128i  y0r_one_over_sqrt_21;
  simde__m128i  y0r_three_over_sqrt_21;
  simde__m128i  y0r_five_over_sqrt_21;
  simde__m128i  y0r_seven_over_sqrt_21;
  simde__m128i  y0i_one_over_sqrt_21;
  simde__m128i  y0i_three_over_sqrt_21;
  simde__m128i  y0i_five_over_sqrt_21;
  simde__m128i  y0i_seven_over_sqrt_21;

  int i,j;



  for (i=0; i<length>>2; i+=2) {

    // Get rho
    simde__m128i xmm0 = rho01_128i[i];
    simde__m128i xmm1 = rho01_128i[i + 1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i xmm2 = simde_mm_unpacklo_epi64(xmm0, xmm1); // Re(rho)
    simde__m128i xmm3 = simde_mm_unpackhi_epi64(xmm0, xmm1); // Im(rho)
    simde__m128i rho_rpi = simde_mm_adds_epi16(xmm2, xmm3); // rho = Re(rho) + Im(rho)
    simde__m128i rho_rmi = simde_mm_subs_epi16(xmm2, xmm3); // rho* = Re(rho) - Im(rho)

    // Compute the different rhos
    simde__m128i rho_rpi_1_1 = simde_mm_mulhi_epi16(rho_rpi, ONE_OVER_SQRT_42);
    simde__m128i rho_rmi_1_1 = simde_mm_mulhi_epi16(rho_rmi, ONE_OVER_SQRT_42);
    simde__m128i rho_rpi_3_3 = simde_mm_mulhi_epi16(rho_rpi, THREE_OVER_SQRT_42);
    simde__m128i rho_rmi_3_3 = simde_mm_mulhi_epi16(rho_rmi, THREE_OVER_SQRT_42);
    simde__m128i rho_rpi_5_5 = simde_mm_mulhi_epi16(rho_rpi, FIVE_OVER_SQRT_42);
    simde__m128i rho_rmi_5_5 = simde_mm_mulhi_epi16(rho_rmi, FIVE_OVER_SQRT_42);
    simde__m128i rho_rpi_7_7 = simde_mm_mulhi_epi16(rho_rpi, SEVEN_OVER_SQRT_42);
    simde__m128i rho_rmi_7_7 = simde_mm_mulhi_epi16(rho_rmi, SEVEN_OVER_SQRT_42);

    rho_rpi_5_5 = simde_mm_slli_epi16(rho_rpi_5_5, 1);
    rho_rmi_5_5 = simde_mm_slli_epi16(rho_rmi_5_5, 1);
    rho_rpi_7_7 = simde_mm_slli_epi16(rho_rpi_7_7, 2);
    rho_rmi_7_7 = simde_mm_slli_epi16(rho_rmi_7_7, 2);

    simde__m128i xmm4 = simde_mm_mulhi_epi16(xmm2, ONE_OVER_SQRT_42);
    simde__m128i xmm5 = simde_mm_mulhi_epi16(xmm3, ONE_OVER_SQRT_42);
    simde__m128i xmm6 = simde_mm_mulhi_epi16(xmm3, THREE_OVER_SQRT_42);
    simde__m128i xmm7 = simde_mm_mulhi_epi16(xmm3, FIVE_OVER_SQRT_42);
    simde__m128i xmm8 = simde_mm_mulhi_epi16(xmm3, SEVEN_OVER_SQRT_42);
    xmm7 = simde_mm_slli_epi16(xmm7, 1);
    xmm8 = simde_mm_slli_epi16(xmm8, 2);

    simde__m128i rho_rpi_1_3 = simde_mm_adds_epi16(xmm4, xmm6);
    simde__m128i rho_rmi_1_3 = simde_mm_subs_epi16(xmm4, xmm6);
    simde__m128i rho_rpi_1_5 = simde_mm_adds_epi16(xmm4, xmm7);
    simde__m128i rho_rmi_1_5 = simde_mm_subs_epi16(xmm4, xmm7);
    simde__m128i rho_rpi_1_7 = simde_mm_adds_epi16(xmm4, xmm8);
    simde__m128i rho_rmi_1_7 = simde_mm_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm_mulhi_epi16(xmm2, THREE_OVER_SQRT_42);
    simde__m128i rho_rpi_3_1 = simde_mm_adds_epi16(xmm4, xmm5);
    simde__m128i rho_rmi_3_1 = simde_mm_subs_epi16(xmm4, xmm5);
    simde__m128i rho_rpi_3_5 = simde_mm_adds_epi16(xmm4, xmm7);
    simde__m128i rho_rmi_3_5 = simde_mm_subs_epi16(xmm4, xmm7);
    simde__m128i rho_rpi_3_7 = simde_mm_adds_epi16(xmm4, xmm8);
    simde__m128i rho_rmi_3_7 = simde_mm_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm_mulhi_epi16(xmm2, FIVE_OVER_SQRT_42);
    xmm4 = simde_mm_slli_epi16(xmm4, 1);
    simde__m128i rho_rpi_5_1 = simde_mm_adds_epi16(xmm4, xmm5);
    simde__m128i rho_rmi_5_1 = simde_mm_subs_epi16(xmm4, xmm5);
    simde__m128i rho_rpi_5_3 = simde_mm_adds_epi16(xmm4, xmm6);
    simde__m128i rho_rmi_5_3 = simde_mm_subs_epi16(xmm4, xmm6);
    simde__m128i rho_rpi_5_7 = simde_mm_adds_epi16(xmm4, xmm8);
    simde__m128i rho_rmi_5_7 = simde_mm_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm_mulhi_epi16(xmm2, SEVEN_OVER_SQRT_42);
    xmm4 = simde_mm_slli_epi16(xmm4, 2);
    simde__m128i rho_rpi_7_1 = simde_mm_adds_epi16(xmm4, xmm5);
    simde__m128i rho_rmi_7_1 = simde_mm_subs_epi16(xmm4, xmm5);
    simde__m128i rho_rpi_7_3 = simde_mm_adds_epi16(xmm4, xmm6);
    simde__m128i rho_rmi_7_3 = simde_mm_subs_epi16(xmm4, xmm6);
    simde__m128i rho_rpi_7_5 = simde_mm_adds_epi16(xmm4, xmm7);
    simde__m128i rho_rmi_7_5 = simde_mm_subs_epi16(xmm4, xmm7);

    // Rearrange interfering MF output
    xmm0 = stream1_128i_in[i];
    xmm1 = stream1_128i_in[i+1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y1r = simde_mm_unpacklo_epi64(xmm0, xmm1); //[y1r(1),y1r(2),y1r(3),y1r(4)]
    simde__m128i y1i = simde_mm_unpackhi_epi64(xmm0, xmm1); //[y1i(1),y1i(2),y1i(3),y1i(4)]

    // Psi_r calculation from rho_rpi or rho_rmi
    xmm0 = simde_mm_setzero_si128(); // ZERO for abs_pi16
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_7, y1r);
    simde__m128i psi_r_p7_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_5, y1r);
    simde__m128i psi_r_p7_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_3, y1r);
    simde__m128i psi_r_p7_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_1, y1r);
    simde__m128i psi_r_p7_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_1, y1r);
    simde__m128i psi_r_p7_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_3, y1r);
    simde__m128i psi_r_p7_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_5, y1r);
    simde__m128i psi_r_p7_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_7, y1r);
    simde__m128i psi_r_p7_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_7, y1r);
    simde__m128i psi_r_p5_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_5, y1r);
    simde__m128i psi_r_p5_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_3, y1r);
    simde__m128i psi_r_p5_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_1, y1r);
    simde__m128i psi_r_p5_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_1, y1r);
    simde__m128i psi_r_p5_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_3, y1r);
    simde__m128i psi_r_p5_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_5, y1r);
    simde__m128i psi_r_p5_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_7, y1r);
    simde__m128i psi_r_p5_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_7, y1r);
    simde__m128i psi_r_p3_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_5, y1r);
    simde__m128i psi_r_p3_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_3, y1r);
    simde__m128i psi_r_p3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_1, y1r);
    simde__m128i psi_r_p3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_1, y1r);
    simde__m128i psi_r_p3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_3, y1r);
    simde__m128i psi_r_p3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_5, y1r);
    simde__m128i psi_r_p3_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_7, y1r);
    simde__m128i psi_r_p3_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_7, y1r);
    simde__m128i psi_r_p1_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_5, y1r);
    simde__m128i psi_r_p1_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_3, y1r);
    simde__m128i psi_r_p1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_1, y1r);
    simde__m128i psi_r_p1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_1, y1r);
    simde__m128i psi_r_p1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_3, y1r);
    simde__m128i psi_r_p1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_5, y1r);
    simde__m128i psi_r_p1_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_7, y1r);
    simde__m128i psi_r_p1_m7 = simde_mm_abs_epi16(xmm2);

    xmm2 = simde_mm_adds_epi16(rho_rmi_1_7, y1r);
    simde__m128i psi_r_m1_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_5, y1r);
    simde__m128i psi_r_m1_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_3, y1r);
    simde__m128i psi_r_m1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_1, y1r);
    simde__m128i psi_r_m1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_1, y1r);
    simde__m128i psi_r_m1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_3, y1r);
    simde__m128i psi_r_m1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_5, y1r);
    simde__m128i psi_r_m1_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_7, y1r);
    simde__m128i psi_r_m1_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_7, y1r);
    simde__m128i psi_r_m3_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_5, y1r);
    simde__m128i psi_r_m3_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_3, y1r);
    simde__m128i psi_r_m3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_1, y1r);
    simde__m128i psi_r_m3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_1, y1r);
    simde__m128i psi_r_m3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_3, y1r);
    simde__m128i psi_r_m3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_5, y1r);
    simde__m128i psi_r_m3_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_7, y1r);
    simde__m128i psi_r_m3_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_7, y1r);
    simde__m128i psi_r_m5_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_5, y1r);
    simde__m128i psi_r_m5_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_3, y1r);
    simde__m128i psi_r_m5_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_1, y1r);
    simde__m128i psi_r_m5_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_1, y1r);
    simde__m128i psi_r_m5_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_3, y1r);
    simde__m128i psi_r_m5_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_5, y1r);
    simde__m128i psi_r_m5_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_7, y1r);
    simde__m128i psi_r_m5_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_7, y1r);
    simde__m128i psi_r_m7_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_5, y1r);
    simde__m128i psi_r_m7_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_3, y1r);
    simde__m128i psi_r_m7_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_1, y1r);
    simde__m128i psi_r_m7_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_1, y1r);
    simde__m128i psi_r_m7_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_3, y1r);
    simde__m128i psi_r_m7_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_5, y1r);
    simde__m128i psi_r_m7_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_7, y1r);
    simde__m128i psi_r_m7_m7 = simde_mm_abs_epi16(xmm2);

    // Simde__M128i Psi_i calculation from rho_rpi or rho_rmi
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_7, y1i);
    simde__m128i psi_i_p7_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_7, y1i);
    simde__m128i psi_i_p7_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_7, y1i);
    simde__m128i psi_i_p7_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_7, y1i);
    simde__m128i psi_i_p7_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_7, y1i);
    simde__m128i psi_i_p7_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_7, y1i);
    simde__m128i psi_i_p7_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_7, y1i);
    simde__m128i psi_i_p7_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_7, y1i);
    simde__m128i psi_i_p7_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_5, y1i);
    simde__m128i psi_i_p5_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_5, y1i);
    simde__m128i psi_i_p5_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_5, y1i);
    simde__m128i psi_i_p5_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_5, y1i);
    simde__m128i psi_i_p5_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_5, y1i);
    simde__m128i psi_i_p5_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_5, y1i);
    simde__m128i psi_i_p5_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_5, y1i);
    simde__m128i psi_i_p5_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_5, y1i);
    simde__m128i psi_i_p5_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_3, y1i);
    simde__m128i psi_i_p3_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_3, y1i);
    simde__m128i psi_i_p3_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_3, y1i);
    simde__m128i psi_i_p3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_3, y1i);
    simde__m128i psi_i_p3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_3, y1i);
    simde__m128i psi_i_p3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_3, y1i);
    simde__m128i psi_i_p3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_3, y1i);
    simde__m128i psi_i_p3_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_3, y1i);
    simde__m128i psi_i_p3_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_1, y1i);
    simde__m128i psi_i_p1_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_1, y1i);
    simde__m128i psi_i_p1_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_1, y1i);
    simde__m128i psi_i_p1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_1, y1i);
    simde__m128i psi_i_p1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_1, y1i);
    simde__m128i psi_i_p1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_1, y1i);
    simde__m128i psi_i_p1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_1, y1i);
    simde__m128i psi_i_p1_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_1, y1i);
    simde__m128i psi_i_p1_m7 = simde_mm_abs_epi16(xmm2);

    xmm2 = simde_mm_subs_epi16(rho_rpi_7_1, y1i);
    simde__m128i psi_i_m1_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_1, y1i);
    simde__m128i psi_i_m1_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_1, y1i);
    simde__m128i psi_i_m1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_1, y1i);
    simde__m128i psi_i_m1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_1, y1i);
    simde__m128i psi_i_m1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_1, y1i);
    simde__m128i psi_i_m1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_1, y1i);
    simde__m128i psi_i_m1_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_1, y1i);
    simde__m128i psi_i_m1_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_3, y1i);
    simde__m128i psi_i_m3_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_3, y1i);
    simde__m128i psi_i_m3_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_3, y1i);
    simde__m128i psi_i_m3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_3, y1i);
    simde__m128i psi_i_m3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_3, y1i);
    simde__m128i psi_i_m3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_3, y1i);
    simde__m128i psi_i_m3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_3, y1i);
    simde__m128i psi_i_m3_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_3, y1i);
    simde__m128i psi_i_m3_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_5, y1i);
    simde__m128i psi_i_m5_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_5, y1i);
    simde__m128i psi_i_m5_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_5, y1i);
    simde__m128i psi_i_m5_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_5, y1i);
    simde__m128i psi_i_m5_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_5, y1i);
    simde__m128i psi_i_m5_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_5, y1i);
    simde__m128i psi_i_m5_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_5, y1i);
    simde__m128i psi_i_m5_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_5, y1i);
    simde__m128i psi_i_m5_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_7, y1i);
    simde__m128i psi_i_m7_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_7, y1i);
    simde__m128i psi_i_m7_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_7, y1i);
    simde__m128i psi_i_m7_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_7, y1i);
    simde__m128i psi_i_m7_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_7, y1i);
    simde__m128i psi_i_m7_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_7, y1i);
    simde__m128i psi_i_m7_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_7, y1i);
    simde__m128i psi_i_m7_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_7, y1i);
    simde__m128i psi_i_m7_m7 = simde_mm_abs_epi16(xmm2);

    // Rearrange desired MF output
    xmm0 = stream0_128i_in[i];
    xmm1 = stream0_128i_in[i+1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y0r = simde_mm_unpacklo_epi64(xmm0, xmm1); // = [y0r(1),y0r(2),y0r(3),y0r(4)]
    simde__m128i y0i = simde_mm_unpackhi_epi64(xmm0, xmm1);

    // Rearrange desired channel magnitudes
    xmm2 = ch_mag_128i[i]; // = [|h|^2(1),|h|^2(1),|h|^2(2),|h|^2(2)]*(2/sqrt(10))
    xmm3 = ch_mag_128i[i+1]; // = [|h|^2(3),|h|^2(3),|h|^2(4),|h|^2(4)]*(2/sqrt(10))
    xmm2 = simde_mm_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    ch_mag_des = simde_mm_unpacklo_epi64(xmm2,xmm3);

    // Rearrange interfering channel magnitudes
    xmm2 = ch_mag_128i_i[i];
    xmm3 = ch_mag_128i_i[i+1];
    xmm2 = simde_mm_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    ch_mag_int  = simde_mm_unpacklo_epi64(xmm2,xmm3);

    y0r_one_over_sqrt_21   = simde_mm_mulhi_epi16(y0r, ONE_OVER_SQRT_42);
    y0r_three_over_sqrt_21 = simde_mm_mulhi_epi16(y0r, THREE_OVER_SQRT_42);
    y0r_five_over_sqrt_21  = simde_mm_mulhi_epi16(y0r, FIVE_OVER_SQRT_42);
    y0r_five_over_sqrt_21  = simde_mm_slli_epi16(y0r_five_over_sqrt_21, 1);
    y0r_seven_over_sqrt_21 = simde_mm_mulhi_epi16(y0r, SEVEN_OVER_SQRT_42);
    y0r_seven_over_sqrt_21 = simde_mm_slli_epi16(y0r_seven_over_sqrt_21, 2); // Q2.14

    y0i_one_over_sqrt_21   = simde_mm_mulhi_epi16(y0i, ONE_OVER_SQRT_42);
    y0i_three_over_sqrt_21 = simde_mm_mulhi_epi16(y0i, THREE_OVER_SQRT_42);
    y0i_five_over_sqrt_21  = simde_mm_mulhi_epi16(y0i, FIVE_OVER_SQRT_42);
    y0i_five_over_sqrt_21  = simde_mm_slli_epi16(y0i_five_over_sqrt_21, 1);
    y0i_seven_over_sqrt_21 = simde_mm_mulhi_epi16(y0i, SEVEN_OVER_SQRT_42);
    y0i_seven_over_sqrt_21 = simde_mm_slli_epi16(y0i_seven_over_sqrt_21, 2); // Q2.14

    simde__m128i y0_p_7_1 = simde_mm_adds_epi16(y0r_seven_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_p_7_3 = simde_mm_adds_epi16(y0r_seven_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_p_7_5 = simde_mm_adds_epi16(y0r_seven_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_p_7_7 = simde_mm_adds_epi16(y0r_seven_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_p_5_1 = simde_mm_adds_epi16(y0r_five_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_p_5_3 = simde_mm_adds_epi16(y0r_five_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_p_5_5 = simde_mm_adds_epi16(y0r_five_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_p_5_7 = simde_mm_adds_epi16(y0r_five_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_p_3_1 = simde_mm_adds_epi16(y0r_three_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_p_3_3 = simde_mm_adds_epi16(y0r_three_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_p_3_5 = simde_mm_adds_epi16(y0r_three_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_p_3_7 = simde_mm_adds_epi16(y0r_three_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_p_1_1 = simde_mm_adds_epi16(y0r_one_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_p_1_3 = simde_mm_adds_epi16(y0r_one_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_p_1_5 = simde_mm_adds_epi16(y0r_one_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_p_1_7 = simde_mm_adds_epi16(y0r_one_over_sqrt_21, y0i_seven_over_sqrt_21);

    simde__m128i y0_m_1_1 = simde_mm_subs_epi16(y0r_one_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_m_1_3 = simde_mm_subs_epi16(y0r_one_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_m_1_5 = simde_mm_subs_epi16(y0r_one_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_m_1_7 = simde_mm_subs_epi16(y0r_one_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_m_3_1 = simde_mm_subs_epi16(y0r_three_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_m_3_3 = simde_mm_subs_epi16(y0r_three_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_m_3_5 = simde_mm_subs_epi16(y0r_three_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_m_3_7 = simde_mm_subs_epi16(y0r_three_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_m_5_1 = simde_mm_subs_epi16(y0r_five_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_m_5_3 = simde_mm_subs_epi16(y0r_five_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_m_5_5 = simde_mm_subs_epi16(y0r_five_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_m_5_7 = simde_mm_subs_epi16(y0r_five_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_m_7_1 = simde_mm_subs_epi16(y0r_seven_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_m_7_3 = simde_mm_subs_epi16(y0r_seven_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_m_7_5 = simde_mm_subs_epi16(y0r_seven_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_m_7_7 = simde_mm_subs_epi16(y0r_seven_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i tmp_result, tmp_result2;
    interference_abs_epi16(psi_r_p7_p7, ch_mag_int, a_r_p7_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p7_p5, ch_mag_int, a_r_p7_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p7_p3, ch_mag_int, a_r_p7_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p7_p1, ch_mag_int, a_r_p7_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p7_m1, ch_mag_int, a_r_p7_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p7_m3, ch_mag_int, a_r_p7_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p7_m5, ch_mag_int, a_r_p7_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p7_m7, ch_mag_int, a_r_p7_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_p7, ch_mag_int, a_r_p5_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_p5, ch_mag_int, a_r_p5_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_p3, ch_mag_int, a_r_p5_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_p1, ch_mag_int, a_r_p5_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_m1, ch_mag_int, a_r_p5_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_m3, ch_mag_int, a_r_p5_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_m5, ch_mag_int, a_r_p5_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_m7, ch_mag_int, a_r_p5_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_p7, ch_mag_int, a_r_p3_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_p5, ch_mag_int, a_r_p3_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_p3, ch_mag_int, a_r_p3_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_p1, ch_mag_int, a_r_p3_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_m1, ch_mag_int, a_r_p3_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_m3, ch_mag_int, a_r_p3_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_m5, ch_mag_int, a_r_p3_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_m7, ch_mag_int, a_r_p3_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_p7, ch_mag_int, a_r_p1_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_p5, ch_mag_int, a_r_p1_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_p3, ch_mag_int, a_r_p1_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_p1, ch_mag_int, a_r_p1_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_m1, ch_mag_int, a_r_p1_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_m3, ch_mag_int, a_r_p1_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_m5, ch_mag_int, a_r_p1_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_m7, ch_mag_int, a_r_p1_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_p7, ch_mag_int, a_r_m1_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_p5, ch_mag_int, a_r_m1_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_p3, ch_mag_int, a_r_m1_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_p1, ch_mag_int, a_r_m1_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_m1, ch_mag_int, a_r_m1_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_m3, ch_mag_int, a_r_m1_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_m5, ch_mag_int, a_r_m1_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_m7, ch_mag_int, a_r_m1_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_p7, ch_mag_int, a_r_m3_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_p5, ch_mag_int, a_r_m3_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_p3, ch_mag_int, a_r_m3_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_p1, ch_mag_int, a_r_m3_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_m1, ch_mag_int, a_r_m3_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_m3, ch_mag_int, a_r_m3_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_m5, ch_mag_int, a_r_m3_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_m7, ch_mag_int, a_r_m3_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_p7, ch_mag_int, a_r_m5_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_p5, ch_mag_int, a_r_m5_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_p3, ch_mag_int, a_r_m5_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_p1, ch_mag_int, a_r_m5_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_m1, ch_mag_int, a_r_m5_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_m3, ch_mag_int, a_r_m5_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_m5, ch_mag_int, a_r_m5_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_m7, ch_mag_int, a_r_m5_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_p7, ch_mag_int, a_r_m7_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_p5, ch_mag_int, a_r_m7_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_p3, ch_mag_int, a_r_m7_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_p1, ch_mag_int, a_r_m7_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_m1, ch_mag_int, a_r_m7_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_m3, ch_mag_int, a_r_m7_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_m5, ch_mag_int, a_r_m7_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_m7, ch_mag_int, a_r_m7_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);

    interference_abs_epi16(psi_i_p7_p7, ch_mag_int, a_i_p7_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p7_p5, ch_mag_int, a_i_p7_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p7_p3, ch_mag_int, a_i_p7_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p7_p1, ch_mag_int, a_i_p7_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p7_m1, ch_mag_int, a_i_p7_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p7_m3, ch_mag_int, a_i_p7_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p7_m5, ch_mag_int, a_i_p7_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p7_m7, ch_mag_int, a_i_p7_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_p7, ch_mag_int, a_i_p5_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_p5, ch_mag_int, a_i_p5_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_p3, ch_mag_int, a_i_p5_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_p1, ch_mag_int, a_i_p5_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_m1, ch_mag_int, a_i_p5_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_m3, ch_mag_int, a_i_p5_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_m5, ch_mag_int, a_i_p5_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_m7, ch_mag_int, a_i_p5_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_p7, ch_mag_int, a_i_p3_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_p5, ch_mag_int, a_i_p3_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_p3, ch_mag_int, a_i_p3_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_p1, ch_mag_int, a_i_p3_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_m1, ch_mag_int, a_i_p3_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_m3, ch_mag_int, a_i_p3_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_m5, ch_mag_int, a_i_p3_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_m7, ch_mag_int, a_i_p3_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_p7, ch_mag_int, a_i_p1_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_p5, ch_mag_int, a_i_p1_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_p3, ch_mag_int, a_i_p1_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_p1, ch_mag_int, a_i_p1_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_m1, ch_mag_int, a_i_p1_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_m3, ch_mag_int, a_i_p1_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_m5, ch_mag_int, a_i_p1_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_m7, ch_mag_int, a_i_p1_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_p7, ch_mag_int, a_i_m1_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_p5, ch_mag_int, a_i_m1_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_p3, ch_mag_int, a_i_m1_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_p1, ch_mag_int, a_i_m1_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_m1, ch_mag_int, a_i_m1_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_m3, ch_mag_int, a_i_m1_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_m5, ch_mag_int, a_i_m1_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_m7, ch_mag_int, a_i_m1_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_p7, ch_mag_int, a_i_m3_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_p5, ch_mag_int, a_i_m3_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_p3, ch_mag_int, a_i_m3_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_p1, ch_mag_int, a_i_m3_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_m1, ch_mag_int, a_i_m3_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_m3, ch_mag_int, a_i_m3_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_m5, ch_mag_int, a_i_m3_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_m7, ch_mag_int, a_i_m3_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_p7, ch_mag_int, a_i_m5_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_p5, ch_mag_int, a_i_m5_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_p3, ch_mag_int, a_i_m5_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_p1, ch_mag_int, a_i_m5_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_m1, ch_mag_int, a_i_m5_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_m3, ch_mag_int, a_i_m5_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_m5, ch_mag_int, a_i_m5_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_m7, ch_mag_int, a_i_m5_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_p7, ch_mag_int, a_i_m7_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_p5, ch_mag_int, a_i_m7_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_p3, ch_mag_int, a_i_m7_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_p1, ch_mag_int, a_i_m7_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_m1, ch_mag_int, a_i_m7_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_m3, ch_mag_int, a_i_m7_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_m5, ch_mag_int, a_i_m7_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_m7, ch_mag_int, a_i_m7_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);

    // Calculation of a group of two terms in the bit metric involving product of psi and interference
    prodsum_psi_a_epi16(psi_r_p7_p7, a_r_p7_p7, psi_i_p7_p7, a_i_p7_p7, psi_a_p7_p7);
    prodsum_psi_a_epi16(psi_r_p7_p5, a_r_p7_p5, psi_i_p7_p5, a_i_p7_p5, psi_a_p7_p5);
    prodsum_psi_a_epi16(psi_r_p7_p3, a_r_p7_p3, psi_i_p7_p3, a_i_p7_p3, psi_a_p7_p3);
    prodsum_psi_a_epi16(psi_r_p7_p1, a_r_p7_p1, psi_i_p7_p1, a_i_p7_p1, psi_a_p7_p1);
    prodsum_psi_a_epi16(psi_r_p7_m1, a_r_p7_m1, psi_i_p7_m1, a_i_p7_m1, psi_a_p7_m1);
    prodsum_psi_a_epi16(psi_r_p7_m3, a_r_p7_m3, psi_i_p7_m3, a_i_p7_m3, psi_a_p7_m3);
    prodsum_psi_a_epi16(psi_r_p7_m5, a_r_p7_m5, psi_i_p7_m5, a_i_p7_m5, psi_a_p7_m5);
    prodsum_psi_a_epi16(psi_r_p7_m7, a_r_p7_m7, psi_i_p7_m7, a_i_p7_m7, psi_a_p7_m7);
    prodsum_psi_a_epi16(psi_r_p5_p7, a_r_p5_p7, psi_i_p5_p7, a_i_p5_p7, psi_a_p5_p7);
    prodsum_psi_a_epi16(psi_r_p5_p5, a_r_p5_p5, psi_i_p5_p5, a_i_p5_p5, psi_a_p5_p5);
    prodsum_psi_a_epi16(psi_r_p5_p3, a_r_p5_p3, psi_i_p5_p3, a_i_p5_p3, psi_a_p5_p3);
    prodsum_psi_a_epi16(psi_r_p5_p1, a_r_p5_p1, psi_i_p5_p1, a_i_p5_p1, psi_a_p5_p1);
    prodsum_psi_a_epi16(psi_r_p5_m1, a_r_p5_m1, psi_i_p5_m1, a_i_p5_m1, psi_a_p5_m1);
    prodsum_psi_a_epi16(psi_r_p5_m3, a_r_p5_m3, psi_i_p5_m3, a_i_p5_m3, psi_a_p5_m3);
    prodsum_psi_a_epi16(psi_r_p5_m5, a_r_p5_m5, psi_i_p5_m5, a_i_p5_m5, psi_a_p5_m5);
    prodsum_psi_a_epi16(psi_r_p5_m7, a_r_p5_m7, psi_i_p5_m7, a_i_p5_m7, psi_a_p5_m7);
    prodsum_psi_a_epi16(psi_r_p3_p7, a_r_p3_p7, psi_i_p3_p7, a_i_p3_p7, psi_a_p3_p7);
    prodsum_psi_a_epi16(psi_r_p3_p5, a_r_p3_p5, psi_i_p3_p5, a_i_p3_p5, psi_a_p3_p5);
    prodsum_psi_a_epi16(psi_r_p3_p3, a_r_p3_p3, psi_i_p3_p3, a_i_p3_p3, psi_a_p3_p3);
    prodsum_psi_a_epi16(psi_r_p3_p1, a_r_p3_p1, psi_i_p3_p1, a_i_p3_p1, psi_a_p3_p1);
    prodsum_psi_a_epi16(psi_r_p3_m1, a_r_p3_m1, psi_i_p3_m1, a_i_p3_m1, psi_a_p3_m1);
    prodsum_psi_a_epi16(psi_r_p3_m3, a_r_p3_m3, psi_i_p3_m3, a_i_p3_m3, psi_a_p3_m3);
    prodsum_psi_a_epi16(psi_r_p3_m5, a_r_p3_m5, psi_i_p3_m5, a_i_p3_m5, psi_a_p3_m5);
    prodsum_psi_a_epi16(psi_r_p3_m7, a_r_p3_m7, psi_i_p3_m7, a_i_p3_m7, psi_a_p3_m7);
    prodsum_psi_a_epi16(psi_r_p1_p7, a_r_p1_p7, psi_i_p1_p7, a_i_p1_p7, psi_a_p1_p7);
    prodsum_psi_a_epi16(psi_r_p1_p5, a_r_p1_p5, psi_i_p1_p5, a_i_p1_p5, psi_a_p1_p5);
    prodsum_psi_a_epi16(psi_r_p1_p3, a_r_p1_p3, psi_i_p1_p3, a_i_p1_p3, psi_a_p1_p3);
    prodsum_psi_a_epi16(psi_r_p1_p1, a_r_p1_p1, psi_i_p1_p1, a_i_p1_p1, psi_a_p1_p1);
    prodsum_psi_a_epi16(psi_r_p1_m1, a_r_p1_m1, psi_i_p1_m1, a_i_p1_m1, psi_a_p1_m1);
    prodsum_psi_a_epi16(psi_r_p1_m3, a_r_p1_m3, psi_i_p1_m3, a_i_p1_m3, psi_a_p1_m3);
    prodsum_psi_a_epi16(psi_r_p1_m5, a_r_p1_m5, psi_i_p1_m5, a_i_p1_m5, psi_a_p1_m5);
    prodsum_psi_a_epi16(psi_r_p1_m7, a_r_p1_m7, psi_i_p1_m7, a_i_p1_m7, psi_a_p1_m7);
    prodsum_psi_a_epi16(psi_r_m1_p7, a_r_m1_p7, psi_i_m1_p7, a_i_m1_p7, psi_a_m1_p7);
    prodsum_psi_a_epi16(psi_r_m1_p5, a_r_m1_p5, psi_i_m1_p5, a_i_m1_p5, psi_a_m1_p5);
    prodsum_psi_a_epi16(psi_r_m1_p3, a_r_m1_p3, psi_i_m1_p3, a_i_m1_p3, psi_a_m1_p3);
    prodsum_psi_a_epi16(psi_r_m1_p1, a_r_m1_p1, psi_i_m1_p1, a_i_m1_p1, psi_a_m1_p1);
    prodsum_psi_a_epi16(psi_r_m1_m1, a_r_m1_m1, psi_i_m1_m1, a_i_m1_m1, psi_a_m1_m1);
    prodsum_psi_a_epi16(psi_r_m1_m3, a_r_m1_m3, psi_i_m1_m3, a_i_m1_m3, psi_a_m1_m3);
    prodsum_psi_a_epi16(psi_r_m1_m5, a_r_m1_m5, psi_i_m1_m5, a_i_m1_m5, psi_a_m1_m5);
    prodsum_psi_a_epi16(psi_r_m1_m7, a_r_m1_m7, psi_i_m1_m7, a_i_m1_m7, psi_a_m1_m7);
    prodsum_psi_a_epi16(psi_r_m3_p7, a_r_m3_p7, psi_i_m3_p7, a_i_m3_p7, psi_a_m3_p7);
    prodsum_psi_a_epi16(psi_r_m3_p5, a_r_m3_p5, psi_i_m3_p5, a_i_m3_p5, psi_a_m3_p5);
    prodsum_psi_a_epi16(psi_r_m3_p3, a_r_m3_p3, psi_i_m3_p3, a_i_m3_p3, psi_a_m3_p3);
    prodsum_psi_a_epi16(psi_r_m3_p1, a_r_m3_p1, psi_i_m3_p1, a_i_m3_p1, psi_a_m3_p1);
    prodsum_psi_a_epi16(psi_r_m3_m1, a_r_m3_m1, psi_i_m3_m1, a_i_m3_m1, psi_a_m3_m1);
    prodsum_psi_a_epi16(psi_r_m3_m3, a_r_m3_m3, psi_i_m3_m3, a_i_m3_m3, psi_a_m3_m3);
    prodsum_psi_a_epi16(psi_r_m3_m5, a_r_m3_m5, psi_i_m3_m5, a_i_m3_m5, psi_a_m3_m5);
    prodsum_psi_a_epi16(psi_r_m3_m7, a_r_m3_m7, psi_i_m3_m7, a_i_m3_m7, psi_a_m3_m7);
    prodsum_psi_a_epi16(psi_r_m5_p7, a_r_m5_p7, psi_i_m5_p7, a_i_m5_p7, psi_a_m5_p7);
    prodsum_psi_a_epi16(psi_r_m5_p5, a_r_m5_p5, psi_i_m5_p5, a_i_m5_p5, psi_a_m5_p5);
    prodsum_psi_a_epi16(psi_r_m5_p3, a_r_m5_p3, psi_i_m5_p3, a_i_m5_p3, psi_a_m5_p3);
    prodsum_psi_a_epi16(psi_r_m5_p1, a_r_m5_p1, psi_i_m5_p1, a_i_m5_p1, psi_a_m5_p1);
    prodsum_psi_a_epi16(psi_r_m5_m1, a_r_m5_m1, psi_i_m5_m1, a_i_m5_m1, psi_a_m5_m1);
    prodsum_psi_a_epi16(psi_r_m5_m3, a_r_m5_m3, psi_i_m5_m3, a_i_m5_m3, psi_a_m5_m3);
    prodsum_psi_a_epi16(psi_r_m5_m5, a_r_m5_m5, psi_i_m5_m5, a_i_m5_m5, psi_a_m5_m5);
    prodsum_psi_a_epi16(psi_r_m5_m7, a_r_m5_m7, psi_i_m5_m7, a_i_m5_m7, psi_a_m5_m7);
    prodsum_psi_a_epi16(psi_r_m7_p7, a_r_m7_p7, psi_i_m7_p7, a_i_m7_p7, psi_a_m7_p7);
    prodsum_psi_a_epi16(psi_r_m7_p5, a_r_m7_p5, psi_i_m7_p5, a_i_m7_p5, psi_a_m7_p5);
    prodsum_psi_a_epi16(psi_r_m7_p3, a_r_m7_p3, psi_i_m7_p3, a_i_m7_p3, psi_a_m7_p3);
    prodsum_psi_a_epi16(psi_r_m7_p1, a_r_m7_p1, psi_i_m7_p1, a_i_m7_p1, psi_a_m7_p1);
    prodsum_psi_a_epi16(psi_r_m7_m1, a_r_m7_m1, psi_i_m7_m1, a_i_m7_m1, psi_a_m7_m1);
    prodsum_psi_a_epi16(psi_r_m7_m3, a_r_m7_m3, psi_i_m7_m3, a_i_m7_m3, psi_a_m7_m3);
    prodsum_psi_a_epi16(psi_r_m7_m5, a_r_m7_m5, psi_i_m7_m5, a_i_m7_m5, psi_a_m7_m5);
    prodsum_psi_a_epi16(psi_r_m7_m7, a_r_m7_m7, psi_i_m7_m7, a_i_m7_m7, psi_a_m7_m7);

    // Calculation of a group of two terms in the bit metric involving squares of interference
    square_a_epi16(a_r_p7_p7, a_i_p7_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_p7);
    square_a_epi16(a_r_p7_p5, a_i_p7_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_p5);
    square_a_epi16(a_r_p7_p3, a_i_p7_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_p3);
    square_a_epi16(a_r_p7_p1, a_i_p7_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_p1);
    square_a_epi16(a_r_p7_m1, a_i_p7_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_m1);
    square_a_epi16(a_r_p7_m3, a_i_p7_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_m3);
    square_a_epi16(a_r_p7_m5, a_i_p7_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_m5);
    square_a_epi16(a_r_p7_m7, a_i_p7_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_m7);
    square_a_epi16(a_r_p5_p7, a_i_p5_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_p7);
    square_a_epi16(a_r_p5_p5, a_i_p5_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_p5);
    square_a_epi16(a_r_p5_p3, a_i_p5_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_p3);
    square_a_epi16(a_r_p5_p1, a_i_p5_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_p1);
    square_a_epi16(a_r_p5_m1, a_i_p5_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_m1);
    square_a_epi16(a_r_p5_m3, a_i_p5_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_m3);
    square_a_epi16(a_r_p5_m5, a_i_p5_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_m5);
    square_a_epi16(a_r_p5_m7, a_i_p5_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_m7);
    square_a_epi16(a_r_p3_p7, a_i_p3_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_p7);
    square_a_epi16(a_r_p3_p5, a_i_p3_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_p5);
    square_a_epi16(a_r_p3_p3, a_i_p3_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_p3);
    square_a_epi16(a_r_p3_p1, a_i_p3_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_p1);
    square_a_epi16(a_r_p3_m1, a_i_p3_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_m1);
    square_a_epi16(a_r_p3_m3, a_i_p3_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_m3);
    square_a_epi16(a_r_p3_m5, a_i_p3_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_m5);
    square_a_epi16(a_r_p3_m7, a_i_p3_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_m7);
    square_a_epi16(a_r_p1_p7, a_i_p1_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_p7);
    square_a_epi16(a_r_p1_p5, a_i_p1_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_p5);
    square_a_epi16(a_r_p1_p3, a_i_p1_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_p3);
    square_a_epi16(a_r_p1_p1, a_i_p1_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_p1);
    square_a_epi16(a_r_p1_m1, a_i_p1_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_m1);
    square_a_epi16(a_r_p1_m3, a_i_p1_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_m3);
    square_a_epi16(a_r_p1_m5, a_i_p1_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_m5);
    square_a_epi16(a_r_p1_m7, a_i_p1_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_m7);
    square_a_epi16(a_r_m1_p7, a_i_m1_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_p7);
    square_a_epi16(a_r_m1_p5, a_i_m1_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_p5);
    square_a_epi16(a_r_m1_p3, a_i_m1_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_p3);
    square_a_epi16(a_r_m1_p1, a_i_m1_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_p1);
    square_a_epi16(a_r_m1_m1, a_i_m1_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_m1);
    square_a_epi16(a_r_m1_m3, a_i_m1_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_m3);
    square_a_epi16(a_r_m1_m5, a_i_m1_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_m5);
    square_a_epi16(a_r_m1_m7, a_i_m1_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_m7);
    square_a_epi16(a_r_m3_p7, a_i_m3_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_p7);
    square_a_epi16(a_r_m3_p5, a_i_m3_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_p5);
    square_a_epi16(a_r_m3_p3, a_i_m3_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_p3);
    square_a_epi16(a_r_m3_p1, a_i_m3_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_p1);
    square_a_epi16(a_r_m3_m1, a_i_m3_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_m1);
    square_a_epi16(a_r_m3_m3, a_i_m3_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_m3);
    square_a_epi16(a_r_m3_m5, a_i_m3_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_m5);
    square_a_epi16(a_r_m3_m7, a_i_m3_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_m7);
    square_a_epi16(a_r_m5_p7, a_i_m5_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_p7);
    square_a_epi16(a_r_m5_p5, a_i_m5_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_p5);
    square_a_epi16(a_r_m5_p3, a_i_m5_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_p3);
    square_a_epi16(a_r_m5_p1, a_i_m5_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_p1);
    square_a_epi16(a_r_m5_m1, a_i_m5_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_m1);
    square_a_epi16(a_r_m5_m3, a_i_m5_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_m3);
    square_a_epi16(a_r_m5_m5, a_i_m5_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_m5);
    square_a_epi16(a_r_m5_m7, a_i_m5_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_m7);
    square_a_epi16(a_r_m7_p7, a_i_m7_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_p7);
    square_a_epi16(a_r_m7_p5, a_i_m7_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_p5);
    square_a_epi16(a_r_m7_p3, a_i_m7_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_p3);
    square_a_epi16(a_r_m7_p1, a_i_m7_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_p1);
    square_a_epi16(a_r_m7_m1, a_i_m7_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_m1);
    square_a_epi16(a_r_m7_m3, a_i_m7_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_m3);
    square_a_epi16(a_r_m7_m5, a_i_m7_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_m5);
    square_a_epi16(a_r_m7_m7, a_i_m7_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_m7);

    // Computing different multiples of ||h0||^2
    // x=1, y=1
    ch_mag_2_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,ONE_OVER_FOUR_SQRT_42);
    ch_mag_2_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_2_over_42_with_sigma2,1);
    // x=1, y=3
    ch_mag_10_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,FIVE_OVER_FOUR_SQRT_42);
    ch_mag_10_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_10_over_42_with_sigma2,1);
    // x=1, x=5
    ch_mag_26_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,THIRTEEN_OVER_FOUR_SQRT_42);
    ch_mag_26_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_26_over_42_with_sigma2,1);
    // x=1, y=7
    ch_mag_50_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,TWENTYFIVE_OVER_FOUR_SQRT_42);
    ch_mag_50_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_50_over_42_with_sigma2,1);
    // x=3, y=3
    ch_mag_18_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,NINE_OVER_FOUR_SQRT_42);
    ch_mag_18_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_18_over_42_with_sigma2,1);
    // x=3, y=5
    ch_mag_34_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,SEVENTEEN_OVER_FOUR_SQRT_42);
    ch_mag_34_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_34_over_42_with_sigma2,1);
    // x=3, y=7
    ch_mag_58_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,TWENTYNINE_OVER_FOUR_SQRT_42);
    ch_mag_58_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_58_over_42_with_sigma2,2);
    // x=5, y=5
    ch_mag_50_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,TWENTYFIVE_OVER_FOUR_SQRT_42);
    ch_mag_50_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_50_over_42_with_sigma2,1);
    // x=5, y=7
    ch_mag_74_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,THIRTYSEVEN_OVER_FOUR_SQRT_42);
    ch_mag_74_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_74_over_42_with_sigma2,2);
    // x=7, y=7
    ch_mag_98_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,FORTYNINE_OVER_FOUR_SQRT_42);
    ch_mag_98_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_98_over_42_with_sigma2,2);

    // Computing Metrics
    xmm0 = simde_mm_subs_epi16(psi_a_p7_p7, a_sq_p7_p7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_7_7);
    simde__m128i bit_met_p7_p7 = simde_mm_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p7_p5, a_sq_p7_p5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_7_5);
    simde__m128i bit_met_p7_p5 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p7_p3, a_sq_p7_p3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_7_3);
    simde__m128i bit_met_p7_p3 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p7_p1, a_sq_p7_p1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_7_1);
    simde__m128i bit_met_p7_p1 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p7_m1, a_sq_p7_m1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_7_1);
    simde__m128i bit_met_p7_m1 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p7_m3, a_sq_p7_m3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_7_3);
    simde__m128i bit_met_p7_m3 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p7_m5, a_sq_p7_m5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_7_5);
    simde__m128i bit_met_p7_m5 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p7_m7, a_sq_p7_m7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_7_7);
    simde__m128i bit_met_p7_m7 = simde_mm_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_p7, a_sq_p5_p7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_5_7);
    simde__m128i bit_met_p5_p7 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_p5, a_sq_p5_p5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_5_5);
    simde__m128i bit_met_p5_p5 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_p3, a_sq_p5_p3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_5_3);
    simde__m128i bit_met_p5_p3 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_p1, a_sq_p5_p1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_5_1);
    simde__m128i bit_met_p5_p1 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_m1, a_sq_p5_m1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_5_1);
    simde__m128i bit_met_p5_m1 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_m3, a_sq_p5_m3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_5_3);
    simde__m128i bit_met_p5_m3 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_m5, a_sq_p5_m5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_5_5);
    simde__m128i bit_met_p5_m5 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_m7, a_sq_p5_m7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_5_7);
    simde__m128i bit_met_p5_m7 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_p7, a_sq_p3_p7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_3_7);
    simde__m128i bit_met_p3_p7 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_p5, a_sq_p3_p5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_3_5);
    simde__m128i bit_met_p3_p5 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_p3, a_sq_p3_p3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_3_3);
    simde__m128i bit_met_p3_p3 = simde_mm_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_p1, a_sq_p3_p1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_3_1);
    simde__m128i bit_met_p3_p1 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_m1, a_sq_p3_m1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_3_1);
    simde__m128i bit_met_p3_m1 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_m3, a_sq_p3_m3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_3_3);
    simde__m128i bit_met_p3_m3 = simde_mm_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_m5, a_sq_p3_m5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_3_5);
    simde__m128i bit_met_p3_m5 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_m7, a_sq_p3_m7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_3_7);
    simde__m128i bit_met_p3_m7 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_p7, a_sq_p1_p7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_1_7);
    simde__m128i bit_met_p1_p7 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_p5, a_sq_p1_p5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_1_5);
    simde__m128i bit_met_p1_p5 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_p3, a_sq_p1_p3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_1_3);
    simde__m128i bit_met_p1_p3 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_p1, a_sq_p1_p1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_1_1);
    simde__m128i bit_met_p1_p1 = simde_mm_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_m1, a_sq_p1_m1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_1_1);
    simde__m128i bit_met_p1_m1 = simde_mm_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_m3, a_sq_p1_m3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_1_3);
    simde__m128i bit_met_p1_m3 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_m5, a_sq_p1_m5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_1_5);
    simde__m128i bit_met_p1_m5 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_m7, a_sq_p1_m7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_1_7);
    simde__m128i bit_met_p1_m7 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);

    xmm0 = simde_mm_subs_epi16(psi_a_m1_p7, a_sq_m1_p7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_1_7);
    simde__m128i bit_met_m1_p7 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m1_p5, a_sq_m1_p5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_1_5);
    simde__m128i bit_met_m1_p5 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m1_p3, a_sq_m1_p3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_1_3);
    simde__m128i bit_met_m1_p3 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m1_p1, a_sq_m1_p1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_1_1);
    simde__m128i bit_met_m1_p1 = simde_mm_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m1_m1, a_sq_m1_m1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_1_1);
    simde__m128i bit_met_m1_m1 = simde_mm_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m1_m3, a_sq_m1_m3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_1_3);
    simde__m128i bit_met_m1_m3 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m1_m5, a_sq_m1_m5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_1_5);
    simde__m128i bit_met_m1_m5 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m1_m7, a_sq_m1_m7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_1_7);
    simde__m128i bit_met_m1_m7 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_p7, a_sq_m3_p7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_3_7);
    simde__m128i bit_met_m3_p7 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_p5, a_sq_m3_p5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_3_5);
    simde__m128i bit_met_m3_p5 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_p3, a_sq_m3_p3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_3_3);
    simde__m128i bit_met_m3_p3 = simde_mm_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_p1, a_sq_m3_p1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_3_1);
    simde__m128i bit_met_m3_p1 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_m1, a_sq_m3_m1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_3_1);
    simde__m128i bit_met_m3_m1 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_m3, a_sq_m3_m3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_3_3);
    simde__m128i bit_met_m3_m3 = simde_mm_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_m5, a_sq_m3_m5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_3_5);
    simde__m128i bit_met_m3_m5 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_m7, a_sq_m3_m7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_3_7);
    simde__m128i bit_met_m3_m7 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_p7, a_sq_m5_p7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_5_7);
    simde__m128i bit_met_m5_p7 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_p5, a_sq_m5_p5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_5_5);
    simde__m128i bit_met_m5_p5 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_p3, a_sq_m5_p3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_5_3);
    simde__m128i bit_met_m5_p3 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_p1, a_sq_m5_p1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_5_1);
    simde__m128i bit_met_m5_p1 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_m1, a_sq_m5_m1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_5_1);
    simde__m128i bit_met_m5_m1 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_m3, a_sq_m5_m3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_5_3);
    simde__m128i bit_met_m5_m3 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_m5, a_sq_m5_m5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_5_5);
    simde__m128i bit_met_m5_m5 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_m7, a_sq_m5_m7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_5_7);
    simde__m128i bit_met_m5_m7 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_p7, a_sq_m7_p7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_7_7);
    simde__m128i bit_met_m7_p7 = simde_mm_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_p5, a_sq_m7_p5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_7_5);
    simde__m128i bit_met_m7_p5 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_p3, a_sq_m7_p3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_7_3);
    simde__m128i bit_met_m7_p3 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_p1, a_sq_m7_p1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_7_1);
    simde__m128i bit_met_m7_p1 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_m1, a_sq_m7_m1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_7_1);
    simde__m128i bit_met_m7_m1 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_m3, a_sq_m7_m3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_7_3);
    simde__m128i bit_met_m7_m3 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_m5, a_sq_m7_m5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_7_5);
    simde__m128i bit_met_m7_m5 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_m7, a_sq_m7_m7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_7_7);
    simde__m128i bit_met_m7_m7 = simde_mm_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);

    // Detection for 1st bit (LTE mapping)
    // bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_m7_p7, bit_met_m7_p5);
    xmm1 = simde_mm_max_epi16(bit_met_m7_p3, bit_met_m7_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m7_m1, bit_met_m7_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m7_m5, bit_met_m7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m5_p7, bit_met_m5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_m5_p3, bit_met_m5_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m5_m1, bit_met_m5_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m5, bit_met_m5_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m3_p7, bit_met_m3_p5);
    xmm1 = simde_mm_max_epi16(bit_met_m3_p3, bit_met_m3_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m3_m1, bit_met_m3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m3_m5, bit_met_m3_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m1_p7, bit_met_m1_p5);
    xmm1 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m1_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_m5, bit_met_m1_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    // bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p7_p7, bit_met_p7_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p7_p3, bit_met_p7_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p7_m1, bit_met_p7_m3);
    xmm3 = simde_mm_max_epi16(bit_met_p7_m5, bit_met_p7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p5_p7, bit_met_p5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p5_p3, bit_met_p5_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p5_m1, bit_met_p5_m3);
    xmm3 = simde_mm_max_epi16(bit_met_p5_m5, bit_met_p5_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p3_p7, bit_met_p3_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p3, bit_met_p3_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_m5, bit_met_p3_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p1_p7, bit_met_p1_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p1_p3, bit_met_p1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p1_m3);
    xmm3 = simde_mm_max_epi16(bit_met_p1_m5, bit_met_p1_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    y0r = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 2nd bit (LTE mapping)
    // bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    // bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m7_p1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m7_p5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    y1r = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 3rd bit (LTE mapping)
    xmm0 = simde_mm_max_epi16(bit_met_m7_m7, bit_met_m7_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m7_m3, bit_met_m7_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m7_p1, bit_met_m7_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m7_p5, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m5_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m5_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m5_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p5_m7, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p5_m3, bit_met_p5_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p5_p1, bit_met_p5_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p5_p5, bit_met_p5_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p7_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p7_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p7_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm_max_epi16(bit_met_m3_m7, bit_met_m3_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m3_m3, bit_met_m3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m3_p5, bit_met_m3_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m1_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m1_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p1_m7, bit_met_p1_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p1_m3, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p1_p5, bit_met_p1_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p3_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p3_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    simde__m128i y2r = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 4th bit (LTE mapping)
    xmm0 = simde_mm_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m5_p5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m7_p1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    y0i = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);


    // Detection for 5th bit (LTE mapping)
    xmm0 = simde_mm_max_epi16(bit_met_m7_m7, bit_met_m7_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m7_m3, bit_met_m7_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m7_p1, bit_met_m7_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m7_p5, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m1_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m1_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p1_m7, bit_met_p1_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p1_m3, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p1_p5, bit_met_p1_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p7_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p7_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p7_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m5_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m5_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m5_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m3_m7, bit_met_m3_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m3_m3, bit_met_m3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m3_p5, bit_met_m3_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p3_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p3_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p5_m7, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p5_m3, bit_met_p5_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p5_p1, bit_met_p5_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p5_p5, bit_met_p5_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    y1i = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 6th bit (LTE mapping)
    xmm0 = simde_mm_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m5_p1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m7_p5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    simde__m128i y2i = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // map to output stream, difficult to do in SIMD since we have 6 16bit LLRs
    // RE 1
    j = 24*i;
    stream0_out[j + 0] = ((short *)&y0r)[0];
    stream0_out[j + 1] = ((short *)&y1r)[0];
    stream0_out[j + 2] = ((short *)&y2r)[0];
    stream0_out[j + 3] = ((short *)&y0i)[0];
    stream0_out[j + 4] = ((short *)&y1i)[0];
    stream0_out[j + 5] = ((short *)&y2i)[0];
    // RE 2
    stream0_out[j + 6] = ((short *)&y0r)[1];
    stream0_out[j + 7] = ((short *)&y1r)[1];
    stream0_out[j + 8] = ((short *)&y2r)[1];
    stream0_out[j + 9] = ((short *)&y0i)[1];
    stream0_out[j + 10] = ((short *)&y1i)[1];
    stream0_out[j + 11] = ((short *)&y2i)[1];
    // RE 3
    stream0_out[j + 12] = ((short *)&y0r)[2];
    stream0_out[j + 13] = ((short *)&y1r)[2];
    stream0_out[j + 14] = ((short *)&y2r)[2];
    stream0_out[j + 15] = ((short *)&y0i)[2];
    stream0_out[j + 16] = ((short *)&y1i)[2];
    stream0_out[j + 17] = ((short *)&y2i)[2];
    // RE 4
    stream0_out[j + 18] = ((short *)&y0r)[3];
    stream0_out[j + 19] = ((short *)&y1r)[3];
    stream0_out[j + 20] = ((short *)&y2r)[3];
    stream0_out[j + 21] = ((short *)&y0i)[3];
    stream0_out[j + 22] = ((short *)&y1i)[3];
    stream0_out[j + 23] = ((short *)&y2i)[3];
    // RE 5
    stream0_out[j + 24] = ((short *)&y0r)[4];
    stream0_out[j + 25] = ((short *)&y1r)[4];
    stream0_out[j + 26] = ((short *)&y2r)[4];
    stream0_out[j + 27] = ((short *)&y0i)[4];
    stream0_out[j + 28] = ((short *)&y1i)[4];
    stream0_out[j + 29] = ((short *)&y2i)[4];
    // RE 6
    stream0_out[j + 30] = ((short *)&y0r)[5];
    stream0_out[j + 31] = ((short *)&y1r)[5];
    stream0_out[j + 32] = ((short *)&y2r)[5];
    stream0_out[j + 33] = ((short *)&y0i)[5];
    stream0_out[j + 34] = ((short *)&y1i)[5];
    stream0_out[j + 35] = ((short *)&y2i)[5];
    // RE 7
    stream0_out[j + 36] = ((short *)&y0r)[6];
    stream0_out[j + 37] = ((short *)&y1r)[6];
    stream0_out[j + 38] = ((short *)&y2r)[6];
    stream0_out[j + 39] = ((short *)&y0i)[6];
    stream0_out[j + 40] = ((short *)&y1i)[6];
    stream0_out[j + 41] = ((short *)&y2i)[6];
    // RE 8
    stream0_out[j + 42] = ((short *)&y0r)[7];
    stream0_out[j + 43] = ((short *)&y1r)[7];
    stream0_out[j + 44] = ((short *)&y2r)[7];
    stream0_out[j + 45] = ((short *)&y0i)[7];
    stream0_out[j + 46] = ((short *)&y1i)[7];
    stream0_out[j + 47] = ((short *)&y2i)[7];
  }
}


int dlsch_64qam_16qam_llr(LTE_DL_FRAME_PARMS *frame_parms,
                          int32_t **rxdataF_comp,
                          int32_t **rxdataF_comp_i,
                          int32_t **dl_ch_mag,
                          int32_t **dl_ch_mag_i,
                          int32_t **rho_i,
                          int16_t *dlsch_llr,
                          uint8_t symbol,
                          uint8_t first_symbol_flag,
                          uint16_t nb_rb,
                          uint16_t pbch_pss_sss_adjust,
                          int16_t **llr16p)
{

  int16_t *rxF      = (int16_t*)&rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rxF_i    = (int16_t*)&rxdataF_comp_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *ch_mag   = (int16_t*)&dl_ch_mag[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *ch_mag_i = (int16_t*)&dl_ch_mag_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rho      = (int16_t*)&rho_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *llr16;
  int len;
  uint8_t symbol_mod = (symbol >= (7-frame_parms->Ncp))? (symbol-(7-frame_parms->Ncp)) : symbol;

  //first symbol has different structure due to more pilots
  if (first_symbol_flag == 1) {
    llr16 = (int16_t*)dlsch_llr;
  } else {
    llr16 = (int16_t*)(*llr16p);
  }

  AssertFatal(llr16!=NULL,"dlsch_16qam_64qam_llr:llr is null, symbol %d\n",symbol);

  if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
    // if symbol has pilots
    if (frame_parms->nb_antenna_ports_eNB!=1)
      // in 2 antenna ports we have 8 REs per symbol per RB
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
    else
      // for 1 antenna port we have 10 REs per symbol per RB
      len = (nb_rb*10) - (5*pbch_pss_sss_adjust/6);
  } else {
    // symbol has no pilots
    len = (nb_rb*12) - pbch_pss_sss_adjust;
  }

  qam64_qam16((short *)rxF,
              (short *)rxF_i,
              (short *)ch_mag,
              (short *)ch_mag_i,
              (short *)llr16,
              (short *)rho,
              len);

  llr16 += (6*len);
  *llr16p = (short *)llr16;
  return(0);
}

void qam64_qam64(short *stream0_in,
                 short *stream1_in,
                 short *ch_mag,
                 short *ch_mag_i,
                 short *stream0_out,
                 short *rho01,
                 int length
     )
{

  /*
    Author: S. Wagner
    Date: 31-07-12

    Input:
    stream0_in:  MF filter for 1st stream, i.e., y0=h0'*y
    stream1_in:  MF filter for 2nd stream, i.e., y1=h1'*y
    ch_mag:      4*h0/sqrt(42), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    ch_mag_i:    4*h1/sqrt(42), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    rho01:       Channel cross correlation, i.e., h1'*h0

    Output:
    stream0_out: output LLRs for 1st stream
  */

  simde__m128i *rho01_128i      = (simde__m128i *)rho01;
  simde__m128i *stream0_128i_in = (simde__m128i *)stream0_in;
  simde__m128i *stream1_128i_in = (simde__m128i *)stream1_in;
  simde__m128i *ch_mag_128i     = (simde__m128i *)ch_mag;
  simde__m128i *ch_mag_128i_i   = (simde__m128i *)ch_mag_i;

  simde__m128i ONE_OVER_SQRT_42 = simde_mm_set1_epi16(10112); // round(1/sqrt(42)*2^16)
  simde__m128i THREE_OVER_SQRT_42 = simde_mm_set1_epi16(30337); // round(3/sqrt(42)*2^16)
  simde__m128i FIVE_OVER_SQRT_42 = simde_mm_set1_epi16(25281); // round(5/sqrt(42)*2^15)
  simde__m128i SEVEN_OVER_SQRT_42 = simde_mm_set1_epi16(17697); // round(7/sqrt(42)*2^14) Q2.14
  simde__m128i ONE_OVER_SQRT_2 = simde_mm_set1_epi16(23170); // round(1/sqrt(2)*2^15)
  simde__m128i ONE_OVER_SQRT_2_42 = simde_mm_set1_epi16(3575); // round(1/sqrt(2*42)*2^15)
  simde__m128i THREE_OVER_SQRT_2_42 = simde_mm_set1_epi16(10726); // round(3/sqrt(2*42)*2^15)
  simde__m128i FIVE_OVER_SQRT_2_42 = simde_mm_set1_epi16(17876); // round(5/sqrt(2*42)*2^15)
  simde__m128i SEVEN_OVER_SQRT_2_42 = simde_mm_set1_epi16(25027); // round(7/sqrt(2*42)*2^15)
  simde__m128i FORTYNINE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(30969); // round(49/(4*sqrt(42))*2^14), Q2.14
  simde__m128i THIRTYSEVEN_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(23385); // round(37/(4*sqrt(42))*2^14), Q2.14
  simde__m128i TWENTYFIVE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(31601); // round(25/(4*sqrt(42))*2^15)
  simde__m128i TWENTYNINE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(18329); // round(29/(4*sqrt(42))*2^15), Q2.14
  simde__m128i SEVENTEEN_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(21489); // round(17/(4*sqrt(42))*2^15)
  simde__m128i NINE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(11376); // round(9/(4*sqrt(42))*2^15)
  simde__m128i THIRTEEN_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(16433); // round(13/(4*sqrt(42))*2^15)
  simde__m128i FIVE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(6320); // round(5/(4*sqrt(42))*2^15)
  simde__m128i ONE_OVER_FOUR_SQRT_42 = simde_mm_set1_epi16(1264); // round(1/(4*sqrt(42))*2^15)
  simde__m128i SQRT_42_OVER_FOUR = simde_mm_set1_epi16(13272); // round(sqrt(42)/4*2^13), Q3.12

  simde__m128i ch_mag_des;
  simde__m128i ch_mag_int;
  simde__m128i ch_mag_98_over_42_with_sigma2;
  simde__m128i ch_mag_74_over_42_with_sigma2;
  simde__m128i ch_mag_58_over_42_with_sigma2;
  simde__m128i ch_mag_50_over_42_with_sigma2;
  simde__m128i ch_mag_34_over_42_with_sigma2;
  simde__m128i ch_mag_18_over_42_with_sigma2;
  simde__m128i ch_mag_26_over_42_with_sigma2;
  simde__m128i ch_mag_10_over_42_with_sigma2;
  simde__m128i ch_mag_2_over_42_with_sigma2;
  simde__m128i  y0r_one_over_sqrt_21;
  simde__m128i  y0r_three_over_sqrt_21;
  simde__m128i  y0r_five_over_sqrt_21;
  simde__m128i  y0r_seven_over_sqrt_21;
  simde__m128i  y0i_one_over_sqrt_21;
  simde__m128i  y0i_three_over_sqrt_21;
  simde__m128i  y0i_five_over_sqrt_21;
  simde__m128i  y0i_seven_over_sqrt_21;
  simde__m128i ch_mag_int_with_sigma2;
  simde__m128i two_ch_mag_int_with_sigma2;
  simde__m128i three_ch_mag_int_with_sigma2;

  int i,j;


  for (i=0; i<length>>2; i+=2) {

    // Get rho
    simde__m128i xmm0 = rho01_128i[i];
    simde__m128i xmm1 = rho01_128i[i + 1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i xmm2 = simde_mm_unpacklo_epi64(xmm0, xmm1); // Re(rho)
    simde__m128i xmm3 = simde_mm_unpackhi_epi64(xmm0, xmm1); // Im(rho)
    simde__m128i rho_rpi = simde_mm_adds_epi16(xmm2, xmm3); // rho = Re(rho) + Im(rho)
    simde__m128i rho_rmi = simde_mm_subs_epi16(xmm2, xmm3); // rho* = Re(rho) - Im(rho)

    // Compute the different rhos
    simde__m128i rho_rpi_1_1 = simde_mm_mulhi_epi16(rho_rpi, ONE_OVER_SQRT_42);
    simde__m128i rho_rmi_1_1 = simde_mm_mulhi_epi16(rho_rmi, ONE_OVER_SQRT_42);
    simde__m128i rho_rpi_3_3 = simde_mm_mulhi_epi16(rho_rpi, THREE_OVER_SQRT_42);
    simde__m128i rho_rmi_3_3 = simde_mm_mulhi_epi16(rho_rmi, THREE_OVER_SQRT_42);
    simde__m128i rho_rpi_5_5 = simde_mm_mulhi_epi16(rho_rpi, FIVE_OVER_SQRT_42);
    simde__m128i rho_rmi_5_5 = simde_mm_mulhi_epi16(rho_rmi, FIVE_OVER_SQRT_42);
    simde__m128i rho_rpi_7_7 = simde_mm_mulhi_epi16(rho_rpi, SEVEN_OVER_SQRT_42);
    simde__m128i rho_rmi_7_7 = simde_mm_mulhi_epi16(rho_rmi, SEVEN_OVER_SQRT_42);

    rho_rpi_5_5 = simde_mm_slli_epi16(rho_rpi_5_5, 1);
    rho_rmi_5_5 = simde_mm_slli_epi16(rho_rmi_5_5, 1);
    rho_rpi_7_7 = simde_mm_slli_epi16(rho_rpi_7_7, 2);
    rho_rmi_7_7 = simde_mm_slli_epi16(rho_rmi_7_7, 2);

    simde__m128i xmm4 = simde_mm_mulhi_epi16(xmm2, ONE_OVER_SQRT_42);
    simde__m128i xmm5 = simde_mm_mulhi_epi16(xmm3, ONE_OVER_SQRT_42);
    simde__m128i xmm6 = simde_mm_mulhi_epi16(xmm3, THREE_OVER_SQRT_42);
    simde__m128i xmm7 = simde_mm_mulhi_epi16(xmm3, FIVE_OVER_SQRT_42);
    simde__m128i xmm8 = simde_mm_mulhi_epi16(xmm3, SEVEN_OVER_SQRT_42);
    xmm7 = simde_mm_slli_epi16(xmm7, 1);
    xmm8 = simde_mm_slli_epi16(xmm8, 2);

    simde__m128i rho_rpi_1_3 = simde_mm_adds_epi16(xmm4, xmm6);
    simde__m128i rho_rmi_1_3 = simde_mm_subs_epi16(xmm4, xmm6);
    simde__m128i rho_rpi_1_5 = simde_mm_adds_epi16(xmm4, xmm7);
    simde__m128i rho_rmi_1_5 = simde_mm_subs_epi16(xmm4, xmm7);
    simde__m128i rho_rpi_1_7 = simde_mm_adds_epi16(xmm4, xmm8);
    simde__m128i rho_rmi_1_7 = simde_mm_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm_mulhi_epi16(xmm2, THREE_OVER_SQRT_42);
    simde__m128i rho_rpi_3_1 = simde_mm_adds_epi16(xmm4, xmm5);
    simde__m128i rho_rmi_3_1 = simde_mm_subs_epi16(xmm4, xmm5);
    simde__m128i rho_rpi_3_5 = simde_mm_adds_epi16(xmm4, xmm7);
    simde__m128i rho_rmi_3_5 = simde_mm_subs_epi16(xmm4, xmm7);
    simde__m128i rho_rpi_3_7 = simde_mm_adds_epi16(xmm4, xmm8);
    simde__m128i rho_rmi_3_7 = simde_mm_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm_mulhi_epi16(xmm2, FIVE_OVER_SQRT_42);
    xmm4 = simde_mm_slli_epi16(xmm4, 1);
    simde__m128i rho_rpi_5_1 = simde_mm_adds_epi16(xmm4, xmm5);
    simde__m128i rho_rmi_5_1 = simde_mm_subs_epi16(xmm4, xmm5);
    simde__m128i rho_rpi_5_3 = simde_mm_adds_epi16(xmm4, xmm6);
    simde__m128i rho_rmi_5_3 = simde_mm_subs_epi16(xmm4, xmm6);
    simde__m128i rho_rpi_5_7 = simde_mm_adds_epi16(xmm4, xmm8);
    simde__m128i rho_rmi_5_7 = simde_mm_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm_mulhi_epi16(xmm2, SEVEN_OVER_SQRT_42);
    xmm4 = simde_mm_slli_epi16(xmm4, 2);
    simde__m128i rho_rpi_7_1 = simde_mm_adds_epi16(xmm4, xmm5);
    simde__m128i rho_rmi_7_1 = simde_mm_subs_epi16(xmm4, xmm5);
    simde__m128i rho_rpi_7_3 = simde_mm_adds_epi16(xmm4, xmm6);
    simde__m128i rho_rmi_7_3 = simde_mm_subs_epi16(xmm4, xmm6);
    simde__m128i rho_rpi_7_5 = simde_mm_adds_epi16(xmm4, xmm7);
    simde__m128i rho_rmi_7_5 = simde_mm_subs_epi16(xmm4, xmm7);

    // Rearrange interfering MF output
    xmm0 = stream1_128i_in[i];
    xmm1 = stream1_128i_in[i+1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y1r = simde_mm_unpacklo_epi64(xmm0, xmm1); //[y1r(1),y1r(2),y1r(3),y1r(4)]
    simde__m128i y1i = simde_mm_unpackhi_epi64(xmm0, xmm1); //[y1i(1),y1i(2),y1i(3),y1i(4)]

    // Psi_r calculation from rho_rpi or rho_rmi
    xmm0 = simde_mm_setzero_si128(); // ZERO for abs_pi16
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_7, y1r);
    simde__m128i psi_r_p7_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_5, y1r);
    simde__m128i psi_r_p7_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_3, y1r);
    simde__m128i psi_r_p7_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_1, y1r);
    simde__m128i psi_r_p7_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_1, y1r);
    simde__m128i psi_r_p7_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_3, y1r);
    simde__m128i psi_r_p7_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_5, y1r);
    simde__m128i psi_r_p7_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_7, y1r);
    simde__m128i psi_r_p7_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_7, y1r);
    simde__m128i psi_r_p5_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_5, y1r);
    simde__m128i psi_r_p5_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_3, y1r);
    simde__m128i psi_r_p5_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_1, y1r);
    simde__m128i psi_r_p5_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_1, y1r);
    simde__m128i psi_r_p5_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_3, y1r);
    simde__m128i psi_r_p5_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_5, y1r);
    simde__m128i psi_r_p5_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_7, y1r);
    simde__m128i psi_r_p5_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_7, y1r);
    simde__m128i psi_r_p3_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_5, y1r);
    simde__m128i psi_r_p3_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_3, y1r);
    simde__m128i psi_r_p3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_1, y1r);
    simde__m128i psi_r_p3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_1, y1r);
    simde__m128i psi_r_p3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_3, y1r);
    simde__m128i psi_r_p3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_5, y1r);
    simde__m128i psi_r_p3_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_7, y1r);
    simde__m128i psi_r_p3_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_7, y1r);
    simde__m128i psi_r_p1_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_5, y1r);
    simde__m128i psi_r_p1_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_3, y1r);
    simde__m128i psi_r_p1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_1, y1r);
    simde__m128i psi_r_p1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_1, y1r);
    simde__m128i psi_r_p1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_3, y1r);
    simde__m128i psi_r_p1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_5, y1r);
    simde__m128i psi_r_p1_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_7, y1r);
    simde__m128i psi_r_p1_m7 = simde_mm_abs_epi16(xmm2);

    xmm2 = simde_mm_adds_epi16(rho_rmi_1_7, y1r);
    simde__m128i psi_r_m1_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_5, y1r);
    simde__m128i psi_r_m1_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_3, y1r);
    simde__m128i psi_r_m1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_1, y1r);
    simde__m128i psi_r_m1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_1, y1r);
    simde__m128i psi_r_m1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_3, y1r);
    simde__m128i psi_r_m1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_5, y1r);
    simde__m128i psi_r_m1_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_7, y1r);
    simde__m128i psi_r_m1_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_7, y1r);
    simde__m128i psi_r_m3_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_5, y1r);
    simde__m128i psi_r_m3_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_3, y1r);
    simde__m128i psi_r_m3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_1, y1r);
    simde__m128i psi_r_m3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_1, y1r);
    simde__m128i psi_r_m3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_3, y1r);
    simde__m128i psi_r_m3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_5, y1r);
    simde__m128i psi_r_m3_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_7, y1r);
    simde__m128i psi_r_m3_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_7, y1r);
    simde__m128i psi_r_m5_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_5, y1r);
    simde__m128i psi_r_m5_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_3, y1r);
    simde__m128i psi_r_m5_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_1, y1r);
    simde__m128i psi_r_m5_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_1, y1r);
    simde__m128i psi_r_m5_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_3, y1r);
    simde__m128i psi_r_m5_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_5, y1r);
    simde__m128i psi_r_m5_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_7, y1r);
    simde__m128i psi_r_m5_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_7, y1r);
    simde__m128i psi_r_m7_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_5, y1r);
    simde__m128i psi_r_m7_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_3, y1r);
    simde__m128i psi_r_m7_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_1, y1r);
    simde__m128i psi_r_m7_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_1, y1r);
    simde__m128i psi_r_m7_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_3, y1r);
    simde__m128i psi_r_m7_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_5, y1r);
    simde__m128i psi_r_m7_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_7, y1r);
    simde__m128i psi_r_m7_m7 = simde_mm_abs_epi16(xmm2);

    // Simde__M128i Psi_i calculation from rho_rpi or rho_rmi
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_7, y1i);
    simde__m128i psi_i_p7_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_7, y1i);
    simde__m128i psi_i_p7_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_7, y1i);
    simde__m128i psi_i_p7_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_7, y1i);
    simde__m128i psi_i_p7_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_7, y1i);
    simde__m128i psi_i_p7_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_7, y1i);
    simde__m128i psi_i_p7_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_7, y1i);
    simde__m128i psi_i_p7_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_7, y1i);
    simde__m128i psi_i_p7_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_5, y1i);
    simde__m128i psi_i_p5_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_5, y1i);
    simde__m128i psi_i_p5_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_5, y1i);
    simde__m128i psi_i_p5_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_5, y1i);
    simde__m128i psi_i_p5_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_5, y1i);
    simde__m128i psi_i_p5_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_5, y1i);
    simde__m128i psi_i_p5_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_5, y1i);
    simde__m128i psi_i_p5_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_5, y1i);
    simde__m128i psi_i_p5_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_3, y1i);
    simde__m128i psi_i_p3_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_3, y1i);
    simde__m128i psi_i_p3_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_3, y1i);
    simde__m128i psi_i_p3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_3, y1i);
    simde__m128i psi_i_p3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_3, y1i);
    simde__m128i psi_i_p3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_3, y1i);
    simde__m128i psi_i_p3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_3, y1i);
    simde__m128i psi_i_p3_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_3, y1i);
    simde__m128i psi_i_p3_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_7_1, y1i);
    simde__m128i psi_i_p1_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_5_1, y1i);
    simde__m128i psi_i_p1_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_3_1, y1i);
    simde__m128i psi_i_p1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rmi_1_1, y1i);
    simde__m128i psi_i_p1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_1_1, y1i);
    simde__m128i psi_i_p1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_3_1, y1i);
    simde__m128i psi_i_p1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_5_1, y1i);
    simde__m128i psi_i_p1_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rpi_7_1, y1i);
    simde__m128i psi_i_p1_m7 = simde_mm_abs_epi16(xmm2);

    xmm2 = simde_mm_subs_epi16(rho_rpi_7_1, y1i);
    simde__m128i psi_i_m1_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_1, y1i);
    simde__m128i psi_i_m1_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_1, y1i);
    simde__m128i psi_i_m1_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_1, y1i);
    simde__m128i psi_i_m1_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_1, y1i);
    simde__m128i psi_i_m1_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_1, y1i);
    simde__m128i psi_i_m1_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_1, y1i);
    simde__m128i psi_i_m1_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_1, y1i);
    simde__m128i psi_i_m1_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_3, y1i);
    simde__m128i psi_i_m3_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_3, y1i);
    simde__m128i psi_i_m3_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_3, y1i);
    simde__m128i psi_i_m3_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_3, y1i);
    simde__m128i psi_i_m3_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_3, y1i);
    simde__m128i psi_i_m3_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_3, y1i);
    simde__m128i psi_i_m3_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_3, y1i);
    simde__m128i psi_i_m3_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_3, y1i);
    simde__m128i psi_i_m3_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_5, y1i);
    simde__m128i psi_i_m5_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_5, y1i);
    simde__m128i psi_i_m5_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_5, y1i);
    simde__m128i psi_i_m5_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_5, y1i);
    simde__m128i psi_i_m5_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_5, y1i);
    simde__m128i psi_i_m5_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_5, y1i);
    simde__m128i psi_i_m5_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_5, y1i);
    simde__m128i psi_i_m5_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_5, y1i);
    simde__m128i psi_i_m5_m7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_7_7, y1i);
    simde__m128i psi_i_m7_p7 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_5_7, y1i);
    simde__m128i psi_i_m7_p5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_3_7, y1i);
    simde__m128i psi_i_m7_p3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_subs_epi16(rho_rpi_1_7, y1i);
    simde__m128i psi_i_m7_p1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_1_7, y1i);
    simde__m128i psi_i_m7_m1 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_3_7, y1i);
    simde__m128i psi_i_m7_m3 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_5_7, y1i);
    simde__m128i psi_i_m7_m5 = simde_mm_abs_epi16(xmm2);
    xmm2 = simde_mm_adds_epi16(rho_rmi_7_7, y1i);
    simde__m128i psi_i_m7_m7 = simde_mm_abs_epi16(xmm2);

    // Rearrange desired MF output
    xmm0 = stream0_128i_in[i];
    xmm1 = stream0_128i_in[i+1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y0r = simde_mm_unpacklo_epi64(xmm0, xmm1); // = [y0r(1),y0r(2),y0r(3),y0r(4)]
    simde__m128i y0i = simde_mm_unpackhi_epi64(xmm0, xmm1);

    // Rearrange desired channel magnitudes
    xmm2 = ch_mag_128i[i]; // = [|h|^2(1),|h|^2(1),|h|^2(2),|h|^2(2)]*(2/sqrt(10))
    xmm3 = ch_mag_128i[i+1]; // = [|h|^2(3),|h|^2(3),|h|^2(4),|h|^2(4)]*(2/sqrt(10))
    xmm2 = simde_mm_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    ch_mag_des = simde_mm_unpacklo_epi64(xmm2,xmm3);

    // Rearrange interfering channel magnitudes
    xmm2 = ch_mag_128i_i[i];
    xmm3 = ch_mag_128i_i[i+1];
    xmm2 = simde_mm_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    ch_mag_int  = simde_mm_unpacklo_epi64(xmm2,xmm3);

    y0r_one_over_sqrt_21   = simde_mm_mulhi_epi16(y0r, ONE_OVER_SQRT_42);
    y0r_three_over_sqrt_21 = simde_mm_mulhi_epi16(y0r, THREE_OVER_SQRT_42);
    y0r_five_over_sqrt_21  = simde_mm_mulhi_epi16(y0r, FIVE_OVER_SQRT_42);
    y0r_five_over_sqrt_21  = simde_mm_slli_epi16(y0r_five_over_sqrt_21, 1);
    y0r_seven_over_sqrt_21 = simde_mm_mulhi_epi16(y0r, SEVEN_OVER_SQRT_42);
    y0r_seven_over_sqrt_21 = simde_mm_slli_epi16(y0r_seven_over_sqrt_21, 2); // Q2.14

    y0i_one_over_sqrt_21   = simde_mm_mulhi_epi16(y0i, ONE_OVER_SQRT_42);
    y0i_three_over_sqrt_21 = simde_mm_mulhi_epi16(y0i, THREE_OVER_SQRT_42);
    y0i_five_over_sqrt_21  = simde_mm_mulhi_epi16(y0i, FIVE_OVER_SQRT_42);
    y0i_five_over_sqrt_21  = simde_mm_slli_epi16(y0i_five_over_sqrt_21, 1);
    y0i_seven_over_sqrt_21 = simde_mm_mulhi_epi16(y0i, SEVEN_OVER_SQRT_42);
    y0i_seven_over_sqrt_21 = simde_mm_slli_epi16(y0i_seven_over_sqrt_21, 2); // Q2.14

    simde__m128i y0_p_7_1 = simde_mm_adds_epi16(y0r_seven_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_p_7_3 = simde_mm_adds_epi16(y0r_seven_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_p_7_5 = simde_mm_adds_epi16(y0r_seven_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_p_7_7 = simde_mm_adds_epi16(y0r_seven_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_p_5_1 = simde_mm_adds_epi16(y0r_five_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_p_5_3 = simde_mm_adds_epi16(y0r_five_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_p_5_5 = simde_mm_adds_epi16(y0r_five_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_p_5_7 = simde_mm_adds_epi16(y0r_five_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_p_3_1 = simde_mm_adds_epi16(y0r_three_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_p_3_3 = simde_mm_adds_epi16(y0r_three_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_p_3_5 = simde_mm_adds_epi16(y0r_three_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_p_3_7 = simde_mm_adds_epi16(y0r_three_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_p_1_1 = simde_mm_adds_epi16(y0r_one_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_p_1_3 = simde_mm_adds_epi16(y0r_one_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_p_1_5 = simde_mm_adds_epi16(y0r_one_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_p_1_7 = simde_mm_adds_epi16(y0r_one_over_sqrt_21, y0i_seven_over_sqrt_21);

    simde__m128i y0_m_1_1 = simde_mm_subs_epi16(y0r_one_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_m_1_3 = simde_mm_subs_epi16(y0r_one_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_m_1_5 = simde_mm_subs_epi16(y0r_one_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_m_1_7 = simde_mm_subs_epi16(y0r_one_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_m_3_1 = simde_mm_subs_epi16(y0r_three_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_m_3_3 = simde_mm_subs_epi16(y0r_three_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_m_3_5 = simde_mm_subs_epi16(y0r_three_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_m_3_7 = simde_mm_subs_epi16(y0r_three_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_m_5_1 = simde_mm_subs_epi16(y0r_five_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_m_5_3 = simde_mm_subs_epi16(y0r_five_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_m_5_5 = simde_mm_subs_epi16(y0r_five_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_m_5_7 = simde_mm_subs_epi16(y0r_five_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m128i y0_m_7_1 = simde_mm_subs_epi16(y0r_seven_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m128i y0_m_7_3 = simde_mm_subs_epi16(y0r_seven_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m128i y0_m_7_5 = simde_mm_subs_epi16(y0r_seven_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m128i y0_m_7_7 = simde_mm_subs_epi16(y0r_seven_over_sqrt_21, y0i_seven_over_sqrt_21);

    // Detection of interference term
    ch_mag_int_with_sigma2       = simde_mm_srai_epi16(ch_mag_int, 1); // *2
    two_ch_mag_int_with_sigma2   = ch_mag_int; // *4
    three_ch_mag_int_with_sigma2 = simde_mm_adds_epi16(ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2); // *6
    simde__m128i tmp_result, tmp_result2, tmp_result3, tmp_result4;
    interference_abs_64qam_epi16(psi_r_p7_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p7_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p7_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p7_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p7_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p7_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p7_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p7_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);

    interference_abs_64qam_epi16(psi_i_p7_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p7_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p7_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p7_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p7_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p7_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p7_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p7_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);

    // Calculation of a group of two terms in the bit metric involving product of psi and interference
    prodsum_psi_a_epi16(psi_r_p7_p7, a_r_p7_p7, psi_i_p7_p7, a_i_p7_p7, psi_a_p7_p7);
    prodsum_psi_a_epi16(psi_r_p7_p5, a_r_p7_p5, psi_i_p7_p5, a_i_p7_p5, psi_a_p7_p5);
    prodsum_psi_a_epi16(psi_r_p7_p3, a_r_p7_p3, psi_i_p7_p3, a_i_p7_p3, psi_a_p7_p3);
    prodsum_psi_a_epi16(psi_r_p7_p1, a_r_p7_p1, psi_i_p7_p1, a_i_p7_p1, psi_a_p7_p1);
    prodsum_psi_a_epi16(psi_r_p7_m1, a_r_p7_m1, psi_i_p7_m1, a_i_p7_m1, psi_a_p7_m1);
    prodsum_psi_a_epi16(psi_r_p7_m3, a_r_p7_m3, psi_i_p7_m3, a_i_p7_m3, psi_a_p7_m3);
    prodsum_psi_a_epi16(psi_r_p7_m5, a_r_p7_m5, psi_i_p7_m5, a_i_p7_m5, psi_a_p7_m5);
    prodsum_psi_a_epi16(psi_r_p7_m7, a_r_p7_m7, psi_i_p7_m7, a_i_p7_m7, psi_a_p7_m7);
    prodsum_psi_a_epi16(psi_r_p5_p7, a_r_p5_p7, psi_i_p5_p7, a_i_p5_p7, psi_a_p5_p7);
    prodsum_psi_a_epi16(psi_r_p5_p5, a_r_p5_p5, psi_i_p5_p5, a_i_p5_p5, psi_a_p5_p5);
    prodsum_psi_a_epi16(psi_r_p5_p3, a_r_p5_p3, psi_i_p5_p3, a_i_p5_p3, psi_a_p5_p3);
    prodsum_psi_a_epi16(psi_r_p5_p1, a_r_p5_p1, psi_i_p5_p1, a_i_p5_p1, psi_a_p5_p1);
    prodsum_psi_a_epi16(psi_r_p5_m1, a_r_p5_m1, psi_i_p5_m1, a_i_p5_m1, psi_a_p5_m1);
    prodsum_psi_a_epi16(psi_r_p5_m3, a_r_p5_m3, psi_i_p5_m3, a_i_p5_m3, psi_a_p5_m3);
    prodsum_psi_a_epi16(psi_r_p5_m5, a_r_p5_m5, psi_i_p5_m5, a_i_p5_m5, psi_a_p5_m5);
    prodsum_psi_a_epi16(psi_r_p5_m7, a_r_p5_m7, psi_i_p5_m7, a_i_p5_m7, psi_a_p5_m7);
    prodsum_psi_a_epi16(psi_r_p3_p7, a_r_p3_p7, psi_i_p3_p7, a_i_p3_p7, psi_a_p3_p7);
    prodsum_psi_a_epi16(psi_r_p3_p5, a_r_p3_p5, psi_i_p3_p5, a_i_p3_p5, psi_a_p3_p5);
    prodsum_psi_a_epi16(psi_r_p3_p3, a_r_p3_p3, psi_i_p3_p3, a_i_p3_p3, psi_a_p3_p3);
    prodsum_psi_a_epi16(psi_r_p3_p1, a_r_p3_p1, psi_i_p3_p1, a_i_p3_p1, psi_a_p3_p1);
    prodsum_psi_a_epi16(psi_r_p3_m1, a_r_p3_m1, psi_i_p3_m1, a_i_p3_m1, psi_a_p3_m1);
    prodsum_psi_a_epi16(psi_r_p3_m3, a_r_p3_m3, psi_i_p3_m3, a_i_p3_m3, psi_a_p3_m3);
    prodsum_psi_a_epi16(psi_r_p3_m5, a_r_p3_m5, psi_i_p3_m5, a_i_p3_m5, psi_a_p3_m5);
    prodsum_psi_a_epi16(psi_r_p3_m7, a_r_p3_m7, psi_i_p3_m7, a_i_p3_m7, psi_a_p3_m7);
    prodsum_psi_a_epi16(psi_r_p1_p7, a_r_p1_p7, psi_i_p1_p7, a_i_p1_p7, psi_a_p1_p7);
    prodsum_psi_a_epi16(psi_r_p1_p5, a_r_p1_p5, psi_i_p1_p5, a_i_p1_p5, psi_a_p1_p5);
    prodsum_psi_a_epi16(psi_r_p1_p3, a_r_p1_p3, psi_i_p1_p3, a_i_p1_p3, psi_a_p1_p3);
    prodsum_psi_a_epi16(psi_r_p1_p1, a_r_p1_p1, psi_i_p1_p1, a_i_p1_p1, psi_a_p1_p1);
    prodsum_psi_a_epi16(psi_r_p1_m1, a_r_p1_m1, psi_i_p1_m1, a_i_p1_m1, psi_a_p1_m1);
    prodsum_psi_a_epi16(psi_r_p1_m3, a_r_p1_m3, psi_i_p1_m3, a_i_p1_m3, psi_a_p1_m3);
    prodsum_psi_a_epi16(psi_r_p1_m5, a_r_p1_m5, psi_i_p1_m5, a_i_p1_m5, psi_a_p1_m5);
    prodsum_psi_a_epi16(psi_r_p1_m7, a_r_p1_m7, psi_i_p1_m7, a_i_p1_m7, psi_a_p1_m7);
    prodsum_psi_a_epi16(psi_r_m1_p7, a_r_m1_p7, psi_i_m1_p7, a_i_m1_p7, psi_a_m1_p7);
    prodsum_psi_a_epi16(psi_r_m1_p5, a_r_m1_p5, psi_i_m1_p5, a_i_m1_p5, psi_a_m1_p5);
    prodsum_psi_a_epi16(psi_r_m1_p3, a_r_m1_p3, psi_i_m1_p3, a_i_m1_p3, psi_a_m1_p3);
    prodsum_psi_a_epi16(psi_r_m1_p1, a_r_m1_p1, psi_i_m1_p1, a_i_m1_p1, psi_a_m1_p1);
    prodsum_psi_a_epi16(psi_r_m1_m1, a_r_m1_m1, psi_i_m1_m1, a_i_m1_m1, psi_a_m1_m1);
    prodsum_psi_a_epi16(psi_r_m1_m3, a_r_m1_m3, psi_i_m1_m3, a_i_m1_m3, psi_a_m1_m3);
    prodsum_psi_a_epi16(psi_r_m1_m5, a_r_m1_m5, psi_i_m1_m5, a_i_m1_m5, psi_a_m1_m5);
    prodsum_psi_a_epi16(psi_r_m1_m7, a_r_m1_m7, psi_i_m1_m7, a_i_m1_m7, psi_a_m1_m7);
    prodsum_psi_a_epi16(psi_r_m3_p7, a_r_m3_p7, psi_i_m3_p7, a_i_m3_p7, psi_a_m3_p7);
    prodsum_psi_a_epi16(psi_r_m3_p5, a_r_m3_p5, psi_i_m3_p5, a_i_m3_p5, psi_a_m3_p5);
    prodsum_psi_a_epi16(psi_r_m3_p3, a_r_m3_p3, psi_i_m3_p3, a_i_m3_p3, psi_a_m3_p3);
    prodsum_psi_a_epi16(psi_r_m3_p1, a_r_m3_p1, psi_i_m3_p1, a_i_m3_p1, psi_a_m3_p1);
    prodsum_psi_a_epi16(psi_r_m3_m1, a_r_m3_m1, psi_i_m3_m1, a_i_m3_m1, psi_a_m3_m1);
    prodsum_psi_a_epi16(psi_r_m3_m3, a_r_m3_m3, psi_i_m3_m3, a_i_m3_m3, psi_a_m3_m3);
    prodsum_psi_a_epi16(psi_r_m3_m5, a_r_m3_m5, psi_i_m3_m5, a_i_m3_m5, psi_a_m3_m5);
    prodsum_psi_a_epi16(psi_r_m3_m7, a_r_m3_m7, psi_i_m3_m7, a_i_m3_m7, psi_a_m3_m7);
    prodsum_psi_a_epi16(psi_r_m5_p7, a_r_m5_p7, psi_i_m5_p7, a_i_m5_p7, psi_a_m5_p7);
    prodsum_psi_a_epi16(psi_r_m5_p5, a_r_m5_p5, psi_i_m5_p5, a_i_m5_p5, psi_a_m5_p5);
    prodsum_psi_a_epi16(psi_r_m5_p3, a_r_m5_p3, psi_i_m5_p3, a_i_m5_p3, psi_a_m5_p3);
    prodsum_psi_a_epi16(psi_r_m5_p1, a_r_m5_p1, psi_i_m5_p1, a_i_m5_p1, psi_a_m5_p1);
    prodsum_psi_a_epi16(psi_r_m5_m1, a_r_m5_m1, psi_i_m5_m1, a_i_m5_m1, psi_a_m5_m1);
    prodsum_psi_a_epi16(psi_r_m5_m3, a_r_m5_m3, psi_i_m5_m3, a_i_m5_m3, psi_a_m5_m3);
    prodsum_psi_a_epi16(psi_r_m5_m5, a_r_m5_m5, psi_i_m5_m5, a_i_m5_m5, psi_a_m5_m5);
    prodsum_psi_a_epi16(psi_r_m5_m7, a_r_m5_m7, psi_i_m5_m7, a_i_m5_m7, psi_a_m5_m7);
    prodsum_psi_a_epi16(psi_r_m7_p7, a_r_m7_p7, psi_i_m7_p7, a_i_m7_p7, psi_a_m7_p7);
    prodsum_psi_a_epi16(psi_r_m7_p5, a_r_m7_p5, psi_i_m7_p5, a_i_m7_p5, psi_a_m7_p5);
    prodsum_psi_a_epi16(psi_r_m7_p3, a_r_m7_p3, psi_i_m7_p3, a_i_m7_p3, psi_a_m7_p3);
    prodsum_psi_a_epi16(psi_r_m7_p1, a_r_m7_p1, psi_i_m7_p1, a_i_m7_p1, psi_a_m7_p1);
    prodsum_psi_a_epi16(psi_r_m7_m1, a_r_m7_m1, psi_i_m7_m1, a_i_m7_m1, psi_a_m7_m1);
    prodsum_psi_a_epi16(psi_r_m7_m3, a_r_m7_m3, psi_i_m7_m3, a_i_m7_m3, psi_a_m7_m3);
    prodsum_psi_a_epi16(psi_r_m7_m5, a_r_m7_m5, psi_i_m7_m5, a_i_m7_m5, psi_a_m7_m5);
    prodsum_psi_a_epi16(psi_r_m7_m7, a_r_m7_m7, psi_i_m7_m7, a_i_m7_m7, psi_a_m7_m7);

    // Multiply by sqrt(2)
    psi_a_p7_p7 = simde_mm_mulhi_epi16(psi_a_p7_p7, ONE_OVER_SQRT_2);
    psi_a_p7_p7 = simde_mm_slli_epi16(psi_a_p7_p7, 2);
    psi_a_p7_p5 = simde_mm_mulhi_epi16(psi_a_p7_p5, ONE_OVER_SQRT_2);
    psi_a_p7_p5 = simde_mm_slli_epi16(psi_a_p7_p5, 2);
    psi_a_p7_p3 = simde_mm_mulhi_epi16(psi_a_p7_p3, ONE_OVER_SQRT_2);
    psi_a_p7_p3 = simde_mm_slli_epi16(psi_a_p7_p3, 2);
    psi_a_p7_p1 = simde_mm_mulhi_epi16(psi_a_p7_p1, ONE_OVER_SQRT_2);
    psi_a_p7_p1 = simde_mm_slli_epi16(psi_a_p7_p1, 2);
    psi_a_p7_m1 = simde_mm_mulhi_epi16(psi_a_p7_m1, ONE_OVER_SQRT_2);
    psi_a_p7_m1 = simde_mm_slli_epi16(psi_a_p7_m1, 2);
    psi_a_p7_m3 = simde_mm_mulhi_epi16(psi_a_p7_m3, ONE_OVER_SQRT_2);
    psi_a_p7_m3 = simde_mm_slli_epi16(psi_a_p7_m3, 2);
    psi_a_p7_m5 = simde_mm_mulhi_epi16(psi_a_p7_m5, ONE_OVER_SQRT_2);
    psi_a_p7_m5 = simde_mm_slli_epi16(psi_a_p7_m5, 2);
    psi_a_p7_m7 = simde_mm_mulhi_epi16(psi_a_p7_m7, ONE_OVER_SQRT_2);
    psi_a_p7_m7 = simde_mm_slli_epi16(psi_a_p7_m7, 2);
    psi_a_p5_p7 = simde_mm_mulhi_epi16(psi_a_p5_p7, ONE_OVER_SQRT_2);
    psi_a_p5_p7 = simde_mm_slli_epi16(psi_a_p5_p7, 2);
    psi_a_p5_p5 = simde_mm_mulhi_epi16(psi_a_p5_p5, ONE_OVER_SQRT_2);
    psi_a_p5_p5 = simde_mm_slli_epi16(psi_a_p5_p5, 2);
    psi_a_p5_p3 = simde_mm_mulhi_epi16(psi_a_p5_p3, ONE_OVER_SQRT_2);
    psi_a_p5_p3 = simde_mm_slli_epi16(psi_a_p5_p3, 2);
    psi_a_p5_p1 = simde_mm_mulhi_epi16(psi_a_p5_p1, ONE_OVER_SQRT_2);
    psi_a_p5_p1 = simde_mm_slli_epi16(psi_a_p5_p1, 2);
    psi_a_p5_m1 = simde_mm_mulhi_epi16(psi_a_p5_m1, ONE_OVER_SQRT_2);
    psi_a_p5_m1 = simde_mm_slli_epi16(psi_a_p5_m1, 2);
    psi_a_p5_m3 = simde_mm_mulhi_epi16(psi_a_p5_m3, ONE_OVER_SQRT_2);
    psi_a_p5_m3 = simde_mm_slli_epi16(psi_a_p5_m3, 2);
    psi_a_p5_m5 = simde_mm_mulhi_epi16(psi_a_p5_m5, ONE_OVER_SQRT_2);
    psi_a_p5_m5 = simde_mm_slli_epi16(psi_a_p5_m5, 2);
    psi_a_p5_m7 = simde_mm_mulhi_epi16(psi_a_p5_m7, ONE_OVER_SQRT_2);
    psi_a_p5_m7 = simde_mm_slli_epi16(psi_a_p5_m7, 2);
    psi_a_p3_p7 = simde_mm_mulhi_epi16(psi_a_p3_p7, ONE_OVER_SQRT_2);
    psi_a_p3_p7 = simde_mm_slli_epi16(psi_a_p3_p7, 2);
    psi_a_p3_p5 = simde_mm_mulhi_epi16(psi_a_p3_p5, ONE_OVER_SQRT_2);
    psi_a_p3_p5 = simde_mm_slli_epi16(psi_a_p3_p5, 2);
    psi_a_p3_p3 = simde_mm_mulhi_epi16(psi_a_p3_p3, ONE_OVER_SQRT_2);
    psi_a_p3_p3 = simde_mm_slli_epi16(psi_a_p3_p3, 2);
    psi_a_p3_p1 = simde_mm_mulhi_epi16(psi_a_p3_p1, ONE_OVER_SQRT_2);
    psi_a_p3_p1 = simde_mm_slli_epi16(psi_a_p3_p1, 2);
    psi_a_p3_m1 = simde_mm_mulhi_epi16(psi_a_p3_m1, ONE_OVER_SQRT_2);
    psi_a_p3_m1 = simde_mm_slli_epi16(psi_a_p3_m1, 2);
    psi_a_p3_m3 = simde_mm_mulhi_epi16(psi_a_p3_m3, ONE_OVER_SQRT_2);
    psi_a_p3_m3 = simde_mm_slli_epi16(psi_a_p3_m3, 2);
    psi_a_p3_m5 = simde_mm_mulhi_epi16(psi_a_p3_m5, ONE_OVER_SQRT_2);
    psi_a_p3_m5 = simde_mm_slli_epi16(psi_a_p3_m5, 2);
    psi_a_p3_m7 = simde_mm_mulhi_epi16(psi_a_p3_m7, ONE_OVER_SQRT_2);
    psi_a_p3_m7 = simde_mm_slli_epi16(psi_a_p3_m7, 2);
    psi_a_p1_p7 = simde_mm_mulhi_epi16(psi_a_p1_p7, ONE_OVER_SQRT_2);
    psi_a_p1_p7 = simde_mm_slli_epi16(psi_a_p1_p7, 2);
    psi_a_p1_p5 = simde_mm_mulhi_epi16(psi_a_p1_p5, ONE_OVER_SQRT_2);
    psi_a_p1_p5 = simde_mm_slli_epi16(psi_a_p1_p5, 2);
    psi_a_p1_p3 = simde_mm_mulhi_epi16(psi_a_p1_p3, ONE_OVER_SQRT_2);
    psi_a_p1_p3 = simde_mm_slli_epi16(psi_a_p1_p3, 2);
    psi_a_p1_p1 = simde_mm_mulhi_epi16(psi_a_p1_p1, ONE_OVER_SQRT_2);
    psi_a_p1_p1 = simde_mm_slli_epi16(psi_a_p1_p1, 2);
    psi_a_p1_m1 = simde_mm_mulhi_epi16(psi_a_p1_m1, ONE_OVER_SQRT_2);
    psi_a_p1_m1 = simde_mm_slli_epi16(psi_a_p1_m1, 2);
    psi_a_p1_m3 = simde_mm_mulhi_epi16(psi_a_p1_m3, ONE_OVER_SQRT_2);
    psi_a_p1_m3 = simde_mm_slli_epi16(psi_a_p1_m3, 2);
    psi_a_p1_m5 = simde_mm_mulhi_epi16(psi_a_p1_m5, ONE_OVER_SQRT_2);
    psi_a_p1_m5 = simde_mm_slli_epi16(psi_a_p1_m5, 2);
    psi_a_p1_m7 = simde_mm_mulhi_epi16(psi_a_p1_m7, ONE_OVER_SQRT_2);
    psi_a_p1_m7 = simde_mm_slli_epi16(psi_a_p1_m7, 2);
    psi_a_m1_p7 = simde_mm_mulhi_epi16(psi_a_m1_p7, ONE_OVER_SQRT_2);
    psi_a_m1_p7 = simde_mm_slli_epi16(psi_a_m1_p7, 2);
    psi_a_m1_p5 = simde_mm_mulhi_epi16(psi_a_m1_p5, ONE_OVER_SQRT_2);
    psi_a_m1_p5 = simde_mm_slli_epi16(psi_a_m1_p5, 2);
    psi_a_m1_p3 = simde_mm_mulhi_epi16(psi_a_m1_p3, ONE_OVER_SQRT_2);
    psi_a_m1_p3 = simde_mm_slli_epi16(psi_a_m1_p3, 2);
    psi_a_m1_p1 = simde_mm_mulhi_epi16(psi_a_m1_p1, ONE_OVER_SQRT_2);
    psi_a_m1_p1 = simde_mm_slli_epi16(psi_a_m1_p1, 2);
    psi_a_m1_m1 = simde_mm_mulhi_epi16(psi_a_m1_m1, ONE_OVER_SQRT_2);
    psi_a_m1_m1 = simde_mm_slli_epi16(psi_a_m1_m1, 2);
    psi_a_m1_m3 = simde_mm_mulhi_epi16(psi_a_m1_m3, ONE_OVER_SQRT_2);
    psi_a_m1_m3 = simde_mm_slli_epi16(psi_a_m1_m3, 2);
    psi_a_m1_m5 = simde_mm_mulhi_epi16(psi_a_m1_m5, ONE_OVER_SQRT_2);
    psi_a_m1_m5 = simde_mm_slli_epi16(psi_a_m1_m5, 2);
    psi_a_m1_m7 = simde_mm_mulhi_epi16(psi_a_m1_m7, ONE_OVER_SQRT_2);
    psi_a_m1_m7 = simde_mm_slli_epi16(psi_a_m1_m7, 2);
    psi_a_m3_p7 = simde_mm_mulhi_epi16(psi_a_m3_p7, ONE_OVER_SQRT_2);
    psi_a_m3_p7 = simde_mm_slli_epi16(psi_a_m3_p7, 2);
    psi_a_m3_p5 = simde_mm_mulhi_epi16(psi_a_m3_p5, ONE_OVER_SQRT_2);
    psi_a_m3_p5 = simde_mm_slli_epi16(psi_a_m3_p5, 2);
    psi_a_m3_p3 = simde_mm_mulhi_epi16(psi_a_m3_p3, ONE_OVER_SQRT_2);
    psi_a_m3_p3 = simde_mm_slli_epi16(psi_a_m3_p3, 2);
    psi_a_m3_p1 = simde_mm_mulhi_epi16(psi_a_m3_p1, ONE_OVER_SQRT_2);
    psi_a_m3_p1 = simde_mm_slli_epi16(psi_a_m3_p1, 2);
    psi_a_m3_m1 = simde_mm_mulhi_epi16(psi_a_m3_m1, ONE_OVER_SQRT_2);
    psi_a_m3_m1 = simde_mm_slli_epi16(psi_a_m3_m1, 2);
    psi_a_m3_m3 = simde_mm_mulhi_epi16(psi_a_m3_m3, ONE_OVER_SQRT_2);
    psi_a_m3_m3 = simde_mm_slli_epi16(psi_a_m3_m3, 2);
    psi_a_m3_m5 = simde_mm_mulhi_epi16(psi_a_m3_m5, ONE_OVER_SQRT_2);
    psi_a_m3_m5 = simde_mm_slli_epi16(psi_a_m3_m5, 2);
    psi_a_m3_m7 = simde_mm_mulhi_epi16(psi_a_m3_m7, ONE_OVER_SQRT_2);
    psi_a_m3_m7 = simde_mm_slli_epi16(psi_a_m3_m7, 2);
    psi_a_m5_p7 = simde_mm_mulhi_epi16(psi_a_m5_p7, ONE_OVER_SQRT_2);
    psi_a_m5_p7 = simde_mm_slli_epi16(psi_a_m5_p7, 2);
    psi_a_m5_p5 = simde_mm_mulhi_epi16(psi_a_m5_p5, ONE_OVER_SQRT_2);
    psi_a_m5_p5 = simde_mm_slli_epi16(psi_a_m5_p5, 2);
    psi_a_m5_p3 = simde_mm_mulhi_epi16(psi_a_m5_p3, ONE_OVER_SQRT_2);
    psi_a_m5_p3 = simde_mm_slli_epi16(psi_a_m5_p3, 2);
    psi_a_m5_p1 = simde_mm_mulhi_epi16(psi_a_m5_p1, ONE_OVER_SQRT_2);
    psi_a_m5_p1 = simde_mm_slli_epi16(psi_a_m5_p1, 2);
    psi_a_m5_m1 = simde_mm_mulhi_epi16(psi_a_m5_m1, ONE_OVER_SQRT_2);
    psi_a_m5_m1 = simde_mm_slli_epi16(psi_a_m5_m1, 2);
    psi_a_m5_m3 = simde_mm_mulhi_epi16(psi_a_m5_m3, ONE_OVER_SQRT_2);
    psi_a_m5_m3 = simde_mm_slli_epi16(psi_a_m5_m3, 2);
    psi_a_m5_m5 = simde_mm_mulhi_epi16(psi_a_m5_m5, ONE_OVER_SQRT_2);
    psi_a_m5_m5 = simde_mm_slli_epi16(psi_a_m5_m5, 2);
    psi_a_m5_m7 = simde_mm_mulhi_epi16(psi_a_m5_m7, ONE_OVER_SQRT_2);
    psi_a_m5_m7 = simde_mm_slli_epi16(psi_a_m5_m7, 2);
    psi_a_m7_p7 = simde_mm_mulhi_epi16(psi_a_m7_p7, ONE_OVER_SQRT_2);
    psi_a_m7_p7 = simde_mm_slli_epi16(psi_a_m7_p7, 2);
    psi_a_m7_p5 = simde_mm_mulhi_epi16(psi_a_m7_p5, ONE_OVER_SQRT_2);
    psi_a_m7_p5 = simde_mm_slli_epi16(psi_a_m7_p5, 2);
    psi_a_m7_p3 = simde_mm_mulhi_epi16(psi_a_m7_p3, ONE_OVER_SQRT_2);
    psi_a_m7_p3 = simde_mm_slli_epi16(psi_a_m7_p3, 2);
    psi_a_m7_p1 = simde_mm_mulhi_epi16(psi_a_m7_p1, ONE_OVER_SQRT_2);
    psi_a_m7_p1 = simde_mm_slli_epi16(psi_a_m7_p1, 2);
    psi_a_m7_m1 = simde_mm_mulhi_epi16(psi_a_m7_m1, ONE_OVER_SQRT_2);
    psi_a_m7_m1 = simde_mm_slli_epi16(psi_a_m7_m1, 2);
    psi_a_m7_m3 = simde_mm_mulhi_epi16(psi_a_m7_m3, ONE_OVER_SQRT_2);
    psi_a_m7_m3 = simde_mm_slli_epi16(psi_a_m7_m3, 2);
    psi_a_m7_m5 = simde_mm_mulhi_epi16(psi_a_m7_m5, ONE_OVER_SQRT_2);
    psi_a_m7_m5 = simde_mm_slli_epi16(psi_a_m7_m5, 2);
    psi_a_m7_m7 = simde_mm_mulhi_epi16(psi_a_m7_m7, ONE_OVER_SQRT_2);
    psi_a_m7_m7 = simde_mm_slli_epi16(psi_a_m7_m7, 2);

    // Calculation of a group of two terms in the bit metric involving squares of interference
    square_a_64qam_epi16(a_r_p7_p7, a_i_p7_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_p7);
    square_a_64qam_epi16(a_r_p7_p5, a_i_p7_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_p5);
    square_a_64qam_epi16(a_r_p7_p3, a_i_p7_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_p3);
    square_a_64qam_epi16(a_r_p7_p1, a_i_p7_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_p1);
    square_a_64qam_epi16(a_r_p7_m1, a_i_p7_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_m1);
    square_a_64qam_epi16(a_r_p7_m3, a_i_p7_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_m3);
    square_a_64qam_epi16(a_r_p7_m5, a_i_p7_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_m5);
    square_a_64qam_epi16(a_r_p7_m7, a_i_p7_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_m7);
    square_a_64qam_epi16(a_r_p5_p7, a_i_p5_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_p7);
    square_a_64qam_epi16(a_r_p5_p5, a_i_p5_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_p5);
    square_a_64qam_epi16(a_r_p5_p3, a_i_p5_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_p3);
    square_a_64qam_epi16(a_r_p5_p1, a_i_p5_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_p1);
    square_a_64qam_epi16(a_r_p5_m1, a_i_p5_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_m1);
    square_a_64qam_epi16(a_r_p5_m3, a_i_p5_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_m3);
    square_a_64qam_epi16(a_r_p5_m5, a_i_p5_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_m5);
    square_a_64qam_epi16(a_r_p5_m7, a_i_p5_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_m7);
    square_a_64qam_epi16(a_r_p3_p7, a_i_p3_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_p7);
    square_a_64qam_epi16(a_r_p3_p5, a_i_p3_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_p5);
    square_a_64qam_epi16(a_r_p3_p3, a_i_p3_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_p3);
    square_a_64qam_epi16(a_r_p3_p1, a_i_p3_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_p1);
    square_a_64qam_epi16(a_r_p3_m1, a_i_p3_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_m1);
    square_a_64qam_epi16(a_r_p3_m3, a_i_p3_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_m3);
    square_a_64qam_epi16(a_r_p3_m5, a_i_p3_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_m5);
    square_a_64qam_epi16(a_r_p3_m7, a_i_p3_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_m7);
    square_a_64qam_epi16(a_r_p1_p7, a_i_p1_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_p7);
    square_a_64qam_epi16(a_r_p1_p5, a_i_p1_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_p5);
    square_a_64qam_epi16(a_r_p1_p3, a_i_p1_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_p3);
    square_a_64qam_epi16(a_r_p1_p1, a_i_p1_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_p1);
    square_a_64qam_epi16(a_r_p1_m1, a_i_p1_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_m1);
    square_a_64qam_epi16(a_r_p1_m3, a_i_p1_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_m3);
    square_a_64qam_epi16(a_r_p1_m5, a_i_p1_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_m5);
    square_a_64qam_epi16(a_r_p1_m7, a_i_p1_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_m7);
    square_a_64qam_epi16(a_r_m1_p7, a_i_m1_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_p7);
    square_a_64qam_epi16(a_r_m1_p5, a_i_m1_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_p5);
    square_a_64qam_epi16(a_r_m1_p3, a_i_m1_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_p3);
    square_a_64qam_epi16(a_r_m1_p1, a_i_m1_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_p1);
    square_a_64qam_epi16(a_r_m1_m1, a_i_m1_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_m1);
    square_a_64qam_epi16(a_r_m1_m3, a_i_m1_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_m3);
    square_a_64qam_epi16(a_r_m1_m5, a_i_m1_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_m5);
    square_a_64qam_epi16(a_r_m1_m7, a_i_m1_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_m7);
    square_a_64qam_epi16(a_r_m3_p7, a_i_m3_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_p7);
    square_a_64qam_epi16(a_r_m3_p5, a_i_m3_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_p5);
    square_a_64qam_epi16(a_r_m3_p3, a_i_m3_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_p3);
    square_a_64qam_epi16(a_r_m3_p1, a_i_m3_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_p1);
    square_a_64qam_epi16(a_r_m3_m1, a_i_m3_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_m1);
    square_a_64qam_epi16(a_r_m3_m3, a_i_m3_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_m3);
    square_a_64qam_epi16(a_r_m3_m5, a_i_m3_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_m5);
    square_a_64qam_epi16(a_r_m3_m7, a_i_m3_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_m7);
    square_a_64qam_epi16(a_r_m5_p7, a_i_m5_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_p7);
    square_a_64qam_epi16(a_r_m5_p5, a_i_m5_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_p5);
    square_a_64qam_epi16(a_r_m5_p3, a_i_m5_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_p3);
    square_a_64qam_epi16(a_r_m5_p1, a_i_m5_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_p1);
    square_a_64qam_epi16(a_r_m5_m1, a_i_m5_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_m1);
    square_a_64qam_epi16(a_r_m5_m3, a_i_m5_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_m3);
    square_a_64qam_epi16(a_r_m5_m5, a_i_m5_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_m5);
    square_a_64qam_epi16(a_r_m5_m7, a_i_m5_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_m7);
    square_a_64qam_epi16(a_r_m7_p7, a_i_m7_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_p7);
    square_a_64qam_epi16(a_r_m7_p5, a_i_m7_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_p5);
    square_a_64qam_epi16(a_r_m7_p3, a_i_m7_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_p3);
    square_a_64qam_epi16(a_r_m7_p1, a_i_m7_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_p1);
    square_a_64qam_epi16(a_r_m7_m1, a_i_m7_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_m1);
    square_a_64qam_epi16(a_r_m7_m3, a_i_m7_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_m3);
    square_a_64qam_epi16(a_r_m7_m5, a_i_m7_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_m5);
    square_a_64qam_epi16(a_r_m7_m7, a_i_m7_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_m7);

    // Computing different multiples of ||h0||^2
    // x=1, y=1
    ch_mag_2_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,ONE_OVER_FOUR_SQRT_42);
    ch_mag_2_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_2_over_42_with_sigma2,1);
    // x=1, y=3
    ch_mag_10_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,FIVE_OVER_FOUR_SQRT_42);
    ch_mag_10_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_10_over_42_with_sigma2,1);
    // x=1, x=5
    ch_mag_26_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,THIRTEEN_OVER_FOUR_SQRT_42);
    ch_mag_26_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_26_over_42_with_sigma2,1);
    // x=1, y=7
    ch_mag_50_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,TWENTYFIVE_OVER_FOUR_SQRT_42);
    ch_mag_50_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_50_over_42_with_sigma2,1);
    // x=3, y=3
    ch_mag_18_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,NINE_OVER_FOUR_SQRT_42);
    ch_mag_18_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_18_over_42_with_sigma2,1);
    // x=3, y=5
    ch_mag_34_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,SEVENTEEN_OVER_FOUR_SQRT_42);
    ch_mag_34_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_34_over_42_with_sigma2,1);
    // x=3, y=7
    ch_mag_58_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,TWENTYNINE_OVER_FOUR_SQRT_42);
    ch_mag_58_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_58_over_42_with_sigma2,2);
    // x=5, y=5
    ch_mag_50_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,TWENTYFIVE_OVER_FOUR_SQRT_42);
    ch_mag_50_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_50_over_42_with_sigma2,1);
    // x=5, y=7
    ch_mag_74_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,THIRTYSEVEN_OVER_FOUR_SQRT_42);
    ch_mag_74_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_74_over_42_with_sigma2,2);
    // x=7, y=7
    ch_mag_98_over_42_with_sigma2 = simde_mm_mulhi_epi16(ch_mag_des,FORTYNINE_OVER_FOUR_SQRT_42);
    ch_mag_98_over_42_with_sigma2 = simde_mm_slli_epi16(ch_mag_98_over_42_with_sigma2,2);

    // Computing Metrics
    xmm0 = simde_mm_subs_epi16(psi_a_p7_p7, a_sq_p7_p7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_7_7);
    simde__m128i bit_met_p7_p7 = simde_mm_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p7_p5, a_sq_p7_p5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_7_5);
    simde__m128i bit_met_p7_p5 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p7_p3, a_sq_p7_p3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_7_3);
    simde__m128i bit_met_p7_p3 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p7_p1, a_sq_p7_p1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_7_1);
    simde__m128i bit_met_p7_p1 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p7_m1, a_sq_p7_m1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_7_1);
    simde__m128i bit_met_p7_m1 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p7_m3, a_sq_p7_m3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_7_3);
    simde__m128i bit_met_p7_m3 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p7_m5, a_sq_p7_m5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_7_5);
    simde__m128i bit_met_p7_m5 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p7_m7, a_sq_p7_m7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_7_7);
    simde__m128i bit_met_p7_m7 = simde_mm_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_p7, a_sq_p5_p7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_5_7);
    simde__m128i bit_met_p5_p7 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_p5, a_sq_p5_p5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_5_5);
    simde__m128i bit_met_p5_p5 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_p3, a_sq_p5_p3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_5_3);
    simde__m128i bit_met_p5_p3 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_p1, a_sq_p5_p1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_5_1);
    simde__m128i bit_met_p5_p1 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_m1, a_sq_p5_m1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_5_1);
    simde__m128i bit_met_p5_m1 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_m3, a_sq_p5_m3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_5_3);
    simde__m128i bit_met_p5_m3 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_m5, a_sq_p5_m5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_5_5);
    simde__m128i bit_met_p5_m5 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p5_m7, a_sq_p5_m7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_5_7);
    simde__m128i bit_met_p5_m7 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_p7, a_sq_p3_p7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_3_7);
    simde__m128i bit_met_p3_p7 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_p5, a_sq_p3_p5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_3_5);
    simde__m128i bit_met_p3_p5 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_p3, a_sq_p3_p3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_3_3);
    simde__m128i bit_met_p3_p3 = simde_mm_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_p1, a_sq_p3_p1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_3_1);
    simde__m128i bit_met_p3_p1 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_m1, a_sq_p3_m1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_3_1);
    simde__m128i bit_met_p3_m1 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_m3, a_sq_p3_m3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_3_3);
    simde__m128i bit_met_p3_m3 = simde_mm_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_m5, a_sq_p3_m5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_3_5);
    simde__m128i bit_met_p3_m5 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p3_m7, a_sq_p3_m7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_3_7);
    simde__m128i bit_met_p3_m7 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_p7, a_sq_p1_p7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_1_7);
    simde__m128i bit_met_p1_p7 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_p5, a_sq_p1_p5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_1_5);
    simde__m128i bit_met_p1_p5 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_p3, a_sq_p1_p3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_1_3);
    simde__m128i bit_met_p1_p3 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_p1, a_sq_p1_p1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_p_1_1);
    simde__m128i bit_met_p1_p1 = simde_mm_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_m1, a_sq_p1_m1);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_1_1);
    simde__m128i bit_met_p1_m1 = simde_mm_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_m3, a_sq_p1_m3);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_1_3);
    simde__m128i bit_met_p1_m3 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_m5, a_sq_p1_m5);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_1_5);
    simde__m128i bit_met_p1_m5 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_p1_m7, a_sq_p1_m7);
    xmm1 = simde_mm_adds_epi16(xmm0, y0_m_1_7);
    simde__m128i bit_met_p1_m7 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);

    xmm0 = simde_mm_subs_epi16(psi_a_m1_p7, a_sq_m1_p7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_1_7);
    simde__m128i bit_met_m1_p7 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m1_p5, a_sq_m1_p5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_1_5);
    simde__m128i bit_met_m1_p5 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m1_p3, a_sq_m1_p3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_1_3);
    simde__m128i bit_met_m1_p3 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m1_p1, a_sq_m1_p1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_1_1);
    simde__m128i bit_met_m1_p1 = simde_mm_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m1_m1, a_sq_m1_m1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_1_1);
    simde__m128i bit_met_m1_m1 = simde_mm_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m1_m3, a_sq_m1_m3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_1_3);
    simde__m128i bit_met_m1_m3 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m1_m5, a_sq_m1_m5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_1_5);
    simde__m128i bit_met_m1_m5 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m1_m7, a_sq_m1_m7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_1_7);
    simde__m128i bit_met_m1_m7 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_p7, a_sq_m3_p7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_3_7);
    simde__m128i bit_met_m3_p7 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_p5, a_sq_m3_p5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_3_5);
    simde__m128i bit_met_m3_p5 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_p3, a_sq_m3_p3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_3_3);
    simde__m128i bit_met_m3_p3 = simde_mm_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_p1, a_sq_m3_p1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_3_1);
    simde__m128i bit_met_m3_p1 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_m1, a_sq_m3_m1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_3_1);
    simde__m128i bit_met_m3_m1 = simde_mm_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_m3, a_sq_m3_m3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_3_3);
    simde__m128i bit_met_m3_m3 = simde_mm_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_m5, a_sq_m3_m5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_3_5);
    simde__m128i bit_met_m3_m5 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m3_m7, a_sq_m3_m7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_3_7);
    simde__m128i bit_met_m3_m7 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_p7, a_sq_m5_p7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_5_7);
    simde__m128i bit_met_m5_p7 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_p5, a_sq_m5_p5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_5_5);
    simde__m128i bit_met_m5_p5 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_p3, a_sq_m5_p3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_5_3);
    simde__m128i bit_met_m5_p3 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_p1, a_sq_m5_p1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_5_1);
    simde__m128i bit_met_m5_p1 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_m1, a_sq_m5_m1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_5_1);
    simde__m128i bit_met_m5_m1 = simde_mm_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_m3, a_sq_m5_m3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_5_3);
    simde__m128i bit_met_m5_m3 = simde_mm_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_m5, a_sq_m5_m5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_5_5);
    simde__m128i bit_met_m5_m5 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m5_m7, a_sq_m5_m7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_5_7);
    simde__m128i bit_met_m5_m7 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_p7, a_sq_m7_p7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_7_7);
    simde__m128i bit_met_m7_p7 = simde_mm_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_p5, a_sq_m7_p5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_7_5);
    simde__m128i bit_met_m7_p5 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_p3, a_sq_m7_p3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_7_3);
    simde__m128i bit_met_m7_p3 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_p1, a_sq_m7_p1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_m_7_1);
    simde__m128i bit_met_m7_p1 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_m1, a_sq_m7_m1);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_7_1);
    simde__m128i bit_met_m7_m1 = simde_mm_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_m3, a_sq_m7_m3);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_7_3);
    simde__m128i bit_met_m7_m3 = simde_mm_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_m5, a_sq_m7_m5);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_7_5);
    simde__m128i bit_met_m7_m5 = simde_mm_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm_subs_epi16(psi_a_m7_m7, a_sq_m7_m7);
    xmm1 = simde_mm_subs_epi16(xmm0, y0_p_7_7);
    simde__m128i bit_met_m7_m7 = simde_mm_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);

    // Detection for 1st bit (LTE mapping)
    // bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_m7_p7, bit_met_m7_p5);
    xmm1 = simde_mm_max_epi16(bit_met_m7_p3, bit_met_m7_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m7_m1, bit_met_m7_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m7_m5, bit_met_m7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m5_p7, bit_met_m5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_m5_p3, bit_met_m5_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m5_m1, bit_met_m5_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m5, bit_met_m5_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m3_p7, bit_met_m3_p5);
    xmm1 = simde_mm_max_epi16(bit_met_m3_p3, bit_met_m3_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m3_m1, bit_met_m3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m3_m5, bit_met_m3_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m1_p7, bit_met_m1_p5);
    xmm1 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m1_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_m5, bit_met_m1_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    // bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p7_p7, bit_met_p7_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p7_p3, bit_met_p7_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p7_m1, bit_met_p7_m3);
    xmm3 = simde_mm_max_epi16(bit_met_p7_m5, bit_met_p7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    simde__m128i logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p5_p7, bit_met_p5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p5_p3, bit_met_p5_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p5_m1, bit_met_p5_m3);
    xmm3 = simde_mm_max_epi16(bit_met_p5_m5, bit_met_p5_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p3_p7, bit_met_p3_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p3, bit_met_p3_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_m5, bit_met_p3_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p1_p7, bit_met_p1_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p1_p3, bit_met_p1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_m1, bit_met_p1_m3);
    xmm3 = simde_mm_max_epi16(bit_met_p1_m5, bit_met_p1_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    y0r = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 2nd bit (LTE mapping)
    // bit = 1
    xmm0 = simde_mm_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    // bit = 0
    xmm0 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m7_p1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m7_p5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    y1r = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 3rd bit (LTE mapping)
    xmm0 = simde_mm_max_epi16(bit_met_m7_m7, bit_met_m7_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m7_m3, bit_met_m7_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m7_p1, bit_met_m7_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m7_p5, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m5_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m5_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m5_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p5_m7, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p5_m3, bit_met_p5_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p5_p1, bit_met_p5_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p5_p5, bit_met_p5_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p7_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p7_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p7_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm_max_epi16(bit_met_m3_m7, bit_met_m3_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m3_m3, bit_met_m3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m3_p5, bit_met_m3_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m1_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m1_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p1_m7, bit_met_p1_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p1_m3, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p1_p5, bit_met_p1_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p3_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p3_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    simde__m128i y2r = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 4th bit (LTE mapping)
    xmm0 = simde_mm_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m5_p5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m7_p1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    y0i = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);


    // Detection for 5th bit (LTE mapping)
    xmm0 = simde_mm_max_epi16(bit_met_m7_m7, bit_met_m7_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m7_m3, bit_met_m7_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m7_p1, bit_met_m7_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m7_p5, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m1_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m1_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p1_m7, bit_met_p1_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p1_m3, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p1_p5, bit_met_p1_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p7_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p7_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p7_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m5_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m5_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m5_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_m3_m7, bit_met_m3_m5);
    xmm1 = simde_mm_max_epi16(bit_met_m3_m3, bit_met_m3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m3_p5, bit_met_m3_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p3_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p3_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p3_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p5_m7, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p5_m3, bit_met_p5_m1);
    xmm2 = simde_mm_max_epi16(bit_met_p5_p1, bit_met_p5_p3);
    xmm3 = simde_mm_max_epi16(bit_met_p5_p5, bit_met_p5_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    y1i = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 6th bit (LTE mapping)
    xmm0 = simde_mm_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p1, bit_met_m5_p1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm_max_epi16(bit_met_m5_p5, bit_met_m7_p5);
    xmm4 = simde_mm_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm_max_epi16(logmax_num_re0, xmm5);

    simde__m128i y2i = simde_mm_subs_epi16(logmax_num_re0, logmax_den_re0);

    // map to output stream, difficult to do in SIMD since we have 6 16bit LLRs
    // RE 1
    j = 24*i;
    stream0_out[j + 0] = ((short *)&y0r)[0];
    stream0_out[j + 1] = ((short *)&y1r)[0];
    stream0_out[j + 2] = ((short *)&y2r)[0];
    stream0_out[j + 3] = ((short *)&y0i)[0];
    stream0_out[j + 4] = ((short *)&y1i)[0];
    stream0_out[j + 5] = ((short *)&y2i)[0];
    // RE 2
    stream0_out[j + 6] = ((short *)&y0r)[1];
    stream0_out[j + 7] = ((short *)&y1r)[1];
    stream0_out[j + 8] = ((short *)&y2r)[1];
    stream0_out[j + 9] = ((short *)&y0i)[1];
    stream0_out[j + 10] = ((short *)&y1i)[1];
    stream0_out[j + 11] = ((short *)&y2i)[1];
    // RE 3
    stream0_out[j + 12] = ((short *)&y0r)[2];
    stream0_out[j + 13] = ((short *)&y1r)[2];
    stream0_out[j + 14] = ((short *)&y2r)[2];
    stream0_out[j + 15] = ((short *)&y0i)[2];
    stream0_out[j + 16] = ((short *)&y1i)[2];
    stream0_out[j + 17] = ((short *)&y2i)[2];
    // RE 4
    stream0_out[j + 18] = ((short *)&y0r)[3];
    stream0_out[j + 19] = ((short *)&y1r)[3];
    stream0_out[j + 20] = ((short *)&y2r)[3];
    stream0_out[j + 21] = ((short *)&y0i)[3];
    stream0_out[j + 22] = ((short *)&y1i)[3];
    stream0_out[j + 23] = ((short *)&y2i)[3];
    // RE 5
    stream0_out[j + 24] = ((short *)&y0r)[4];
    stream0_out[j + 25] = ((short *)&y1r)[4];
    stream0_out[j + 26] = ((short *)&y2r)[4];
    stream0_out[j + 27] = ((short *)&y0i)[4];
    stream0_out[j + 28] = ((short *)&y1i)[4];
    stream0_out[j + 29] = ((short *)&y2i)[4];
    // RE 6
    stream0_out[j + 30] = ((short *)&y0r)[5];
    stream0_out[j + 31] = ((short *)&y1r)[5];
    stream0_out[j + 32] = ((short *)&y2r)[5];
    stream0_out[j + 33] = ((short *)&y0i)[5];
    stream0_out[j + 34] = ((short *)&y1i)[5];
    stream0_out[j + 35] = ((short *)&y2i)[5];
    // RE 7
    stream0_out[j + 36] = ((short *)&y0r)[6];
    stream0_out[j + 37] = ((short *)&y1r)[6];
    stream0_out[j + 38] = ((short *)&y2r)[6];
    stream0_out[j + 39] = ((short *)&y0i)[6];
    stream0_out[j + 40] = ((short *)&y1i)[6];
    stream0_out[j + 41] = ((short *)&y2i)[6];
    // RE 8
    stream0_out[j + 42] = ((short *)&y0r)[7];
    stream0_out[j + 43] = ((short *)&y1r)[7];
    stream0_out[j + 44] = ((short *)&y2r)[7];
    stream0_out[j + 45] = ((short *)&y0i)[7];
    stream0_out[j + 46] = ((short *)&y1i)[7];
    stream0_out[j + 47] = ((short *)&y2i)[7];
  }
}


int dlsch_64qam_64qam_llr(LTE_DL_FRAME_PARMS *frame_parms,
                          int32_t **rxdataF_comp,
                          int32_t **rxdataF_comp_i,
                          int32_t **dl_ch_mag,
                          int32_t **dl_ch_mag_i,
                          int32_t **rho_i,
                          int16_t *dlsch_llr,
                          uint8_t symbol,
                          uint8_t first_symbol_flag,
                          uint16_t nb_rb,
                          uint16_t pbch_pss_sss_adjust,
                          //int16_t **llr16p,
                          uint32_t llr_offset)
{

  int16_t *rxF      = (int16_t*)&rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rxF_i    = (int16_t*)&rxdataF_comp_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *ch_mag   = (int16_t*)&dl_ch_mag[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *ch_mag_i = (int16_t*)&dl_ch_mag_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rho      = (int16_t*)&rho_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *llr16;
  int len;
  uint8_t symbol_mod = (symbol >= (7-frame_parms->Ncp))? (symbol-(7-frame_parms->Ncp)) : symbol;

  //first symbol has different structure due to more pilots
  /*if (first_symbol_flag == 1) {
    llr16 = (int16_t*)dlsch_llr;
  } else {
    llr16 = (int16_t*)(*llr16p);
  }*/

  llr16 = (int16_t*)dlsch_llr;

  AssertFatal(llr16!=NULL,"dlsch_16qam_64qam_llr:llr is null, symbol %d\n",symbol);


  if ((symbol_mod==0) || (symbol_mod==(4-frame_parms->Ncp))) {
    // if symbol has pilots
    if (frame_parms->nb_antenna_ports_eNB!=1)
      // in 2 antenna ports we have 8 REs per symbol per RB
      len = (nb_rb*8) - (2*pbch_pss_sss_adjust/3);
    else
      // for 1 antenna port we have 10 REs per symbol per RB
      len = (nb_rb*10) - (5*pbch_pss_sss_adjust/6);
  } else {
    // symbol has no pilots
    len = (nb_rb*12) - pbch_pss_sss_adjust;
  }

  // Round length up to multiple of 16 words
  uint32_t len256i = ((len+16)>>4)*16;
  int32_t *rxF_256i      = (int32_t*) malloc16_clear(len256i*4);
  int32_t *rxF_i_256i    = (int32_t*) malloc16_clear(len256i*4);
  int32_t *ch_mag_256i   = (int32_t*) malloc16_clear(len256i*4);
  int32_t *ch_mag_i_256i = (int32_t*) malloc16_clear(len256i*4);
  int32_t *rho_256i      = (int32_t*) malloc16_clear(len256i*4);

  memcpy(rxF_256i, rxF, len*4);
  memcpy(rxF_i_256i, rxF_i, len*4);
  memcpy(ch_mag_256i, ch_mag, len*4);
  memcpy(ch_mag_i_256i, ch_mag_i, len*4);
  memcpy(rho_256i, rho, len*4);

#if  defined(__x86_64__)
  qam64_qam64_avx2((int32_t *)rxF_256i,
                   (int32_t *)rxF_i_256i,
                   (int32_t *)ch_mag_256i,
                   (int32_t *)ch_mag_i_256i,
                   (int16_t *)llr16,
                   (int32_t *) rho_256i,
                   len);
#else // defined(__arm__) || defined(__aarch64__)
  qam64_qam64((int32_t *)rxF_256i,
                   (int32_t *)rxF_i_256i,
                   (int32_t *)ch_mag_256i,
                   (int32_t *)ch_mag_i_256i,
                   (int16_t *)llr16,
                   (int32_t *) rho_256i,
                   len);
#endif

  free16(rxF_256i, sizeof(rxF_256i));
  free16(rxF_i_256i, sizeof(rxF_i_256i));
  free16(ch_mag_256i, sizeof(ch_mag_256i));
  free16(ch_mag_i_256i, sizeof(ch_mag_i_256i));
  free16(rho_256i, sizeof(rho_256i));

  llr16 += (6*len);
  //*llr16p = (short *)llr16;

  return(0);
}
