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

/*! \file PHY/LTE_TRANSPORT/dci_nr.c
 * \brief Implements PDCCH physical channel TX/RX procedures (36.211) and DCI encoding/decoding (36.212/36.213). Current LTE compliance V8.6 2009-03.
 * \author R. Knopp, A. Mico Pereperez
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "executables/softmodem-common.h"
#include "nr_transport_proto_ue.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_dci_defs.h"
#include "PHY/phy_extern_nr_ue.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/sse_intrin.h"
#include "common/utils/nr/nr_common.h"

#include "assertions.h"
#include "T.h"

char nr_dci_format_string[8][30] = {
  "NR_DL_DCI_FORMAT_1_0",
  "NR_DL_DCI_FORMAT_1_1",
  "NR_DL_DCI_FORMAT_2_0",
  "NR_DL_DCI_FORMAT_2_1",
  "NR_DL_DCI_FORMAT_2_2",
  "NR_DL_DCI_FORMAT_2_3",
  "NR_UL_DCI_FORMAT_0_0",
  "NR_UL_DCI_FORMAT_0_1"};

//#define DEBUG_DCI_DECODING 1

//#define NR_LTE_PDCCH_DCI_SWITCH
//#define NR_PDCCH_DCI_DEBUG            // activates NR_PDCCH_DCI_DEBUG logs
#ifdef NR_PDCCH_DCI_DEBUG
#define LOG_DNL(a, ...) printf("\n\t\t<-NR_PDCCH_DCI_DEBUG (%s)-> " a, __func__, ##__VA_ARGS__ )
#define LOG_DD(a, ...) printf("\t<-NR_PDCCH_DCI_DEBUG (%s)-> " a, __func__, ##__VA_ARGS__ )
#define LOG_DDD(a, ...) printf("\t\t<-NR_PDCCH_DCI_DEBUG (%s)-> " a, __func__, ##__VA_ARGS__ )
#else
#define LOG_DNL(a...)
#define LOG_DD(a...)
#define LOG_DDD(a...)
#endif
#define NR_NBR_CORESET_ACT_BWP 3      // The number of CoreSets per BWP is limited to 3 (including initial CORESET: ControlResourceId 0)
#define NR_NBR_SEARCHSPACE_ACT_BWP 10 // The number of SearSpaces per BWP is limited to 10 (including initial SEARCHSPACE: SearchSpaceId 0)


#ifdef LOG_I
  #undef LOG_I
  #define LOG_I(A,B...) printf(B)
#endif



//static const int16_t conjugate[8]__attribute__((aligned(32))) = {-1,1,-1,1,-1,1,-1,1};


void nr_pdcch_demapping_deinterleaving(uint32_t *llr,
                                       uint32_t *z,
                                       uint8_t coreset_time_dur,
                                       uint8_t start_symbol,
                                       uint32_t coreset_nbr_rb,
                                       uint8_t reg_bundle_size_L,
                                       uint8_t coreset_interleaver_size_R,
                                       uint8_t n_shift,
                                       uint8_t number_of_candidates,
                                       uint16_t *CCE,
                                       uint8_t *L) {
  /*
   * This function will do demapping and deinterleaving from llr containing demodulated symbols
   * Demapping will regroup in REG and bundles
   * Deinterleaving will order the bundles
   *
   * In the following example we can see the process. The llr contains the demodulated IQs, but they are not ordered from REG 0,1,2,..
   * In e_rx (z) we will order the REG ids and group them into bundles.
   * Then we will put the bundles in the correct order as indicated in subclause 7.3.2.2
   *
   llr --------------------------> e_rx (z) ----> e_rx (z)
   |   ...
   |   ...
   |   REG 26
   symbol 2    |   ...
   |   ...
   |   REG 5
   |   REG 2

   |   ...
   |   ...
   |   REG 25
   symbol 1    |   ...
   |   ...
   |   REG 4
   |   REG 1

   |   ...
   |   ...                           ...              ...
   |   REG 24 (bundle 7)             ...              ...
   symbol 0    |   ...                           bundle 3         bundle 6
   |   ...                           bundle 2         bundle 1
   |   REG 3                         bundle 1         bundle 7
   |   REG 0  (bundle 0)             bundle 0         bundle 0

  */
  int c = 0, r = 0;
  uint16_t bundle_j = 0, f_bundle_j = 0, f_reg = 0;
  uint32_t coreset_C = 0;
  uint16_t index_z, index_llr;
  int coreset_interleaved = 0;

  if (reg_bundle_size_L != 0) { // interleaving will be done only if reg_bundle_size_L != 0
    coreset_interleaved = 1;
    coreset_C = (uint32_t) (coreset_nbr_rb / (coreset_interleaver_size_R * reg_bundle_size_L));
  } else {
    reg_bundle_size_L = 6;
  }


  int f_bundle_j_list[(2*NR_MAX_PDCCH_AGG_LEVEL) - 1] = {};

  for (int reg = 0; reg < coreset_nbr_rb; reg++) {
    if ((reg % reg_bundle_size_L) == 0) {
      if (r == coreset_interleaver_size_R) {
        r = 0;
        c++;
      }

      bundle_j = (c * coreset_interleaver_size_R) + r;
      f_bundle_j = ((r * coreset_C) + c + n_shift) % (coreset_nbr_rb / reg_bundle_size_L);

      if (coreset_interleaved == 0) f_bundle_j = bundle_j;

      f_bundle_j_list[reg / 6] = f_bundle_j;

    }
    if ((reg % reg_bundle_size_L) == 0) r++;
  }

  // Get cce_list indices by reg_idx in ascending order
  int f_bundle_j_list_id = 0;
  int f_bundle_j_list_ord[(2*NR_MAX_PDCCH_AGG_LEVEL)-1] = {};
  for (int c_id = 0; c_id < number_of_candidates; c_id++ ) {
    f_bundle_j_list_id = CCE[c_id];
    for (int p = 0; p < NR_MAX_PDCCH_AGG_LEVEL; p++) {
      for (int p2 = CCE[c_id]; p2 < CCE[c_id] + L[c_id]; p2++) {
        AssertFatal(p2<2*NR_MAX_PDCCH_AGG_LEVEL,"number_of_candidates %d : p2 %d,  CCE[%d] %d, L[%d] %d\n",number_of_candidates,p2,c_id,CCE[c_id],c_id,L[c_id]);
        if (f_bundle_j_list[p2] == p) {
          AssertFatal(f_bundle_j_list_id < 2*NR_MAX_PDCCH_AGG_LEVEL,"f_bundle_j_list_id %d\n",f_bundle_j_list_id);
          f_bundle_j_list_ord[f_bundle_j_list_id] = p;
          f_bundle_j_list_id++;
          break;
        }
      }
    }
  }

  int rb = 0;
  for (int c_id = 0; c_id < number_of_candidates; c_id++ ) {
    for (int symbol_idx = start_symbol; symbol_idx < start_symbol+coreset_time_dur; symbol_idx++) {
      for (int cce_count = CCE[c_id/coreset_time_dur]+c_id%coreset_time_dur; cce_count < CCE[c_id/coreset_time_dur]+c_id%coreset_time_dur+L[c_id]; cce_count += coreset_time_dur) {
        for (int reg_in_cce_idx = 0; reg_in_cce_idx < NR_NB_REG_PER_CCE; reg_in_cce_idx++) {

          f_reg = (f_bundle_j_list_ord[cce_count] * reg_bundle_size_L) + reg_in_cce_idx;
          index_z = 9 * rb;
          index_llr = (uint16_t) (f_reg + symbol_idx * coreset_nbr_rb) * 9;

          for (int i = 0; i < 9; i++) {
            z[index_z + i] = llr[index_llr + i];
#ifdef NR_PDCCH_DCI_DEBUG
            LOG_I(PHY,"[cce_count=%d,reg_in_cce_idx=%d,bundle_j=%d,symbol_idx=%d,candidate=%d] z[%d]=(%d,%d) <-> \t[f_reg=%d,fbundle_j=%d] llr[%d]=(%d,%d) \n",
                  cce_count,reg_in_cce_idx,bundle_j,symbol_idx,c_id,(index_z + i),*(int16_t *) &z[index_z + i],*(1 + (int16_t *) &z[index_z + i]),
                   f_reg,f_bundle_j,(index_llr + i),*(int16_t *) &llr[index_llr + i], *(1 + (int16_t *) &llr[index_llr + i]));
#endif
          }
          rb++;
        }
      }
    }
  }
}

int32_t nr_pdcch_llr(NR_DL_FRAME_PARMS *frame_parms, int32_t **rxdataF_comp,
                     int16_t *pdcch_llr, uint8_t symbol,uint32_t coreset_nbr_rb) {
  int16_t *rxF = (int16_t *) &rxdataF_comp[0][(symbol * coreset_nbr_rb * 12)];
  int32_t i;
  int16_t *pdcch_llrp;
  pdcch_llrp = &pdcch_llr[2 * symbol * coreset_nbr_rb * 9];

  if (!pdcch_llrp) {
    LOG_E(PHY,"pdcch_qpsk_llr: llr is null, symbol %d\n", symbol);
    return (-1);
  }

  LOG_DDD("llr logs: pdcch qpsk llr for symbol %d (pos %d), llr offset %ld\n",symbol,(symbol*frame_parms->N_RB_DL*12),pdcch_llrp-pdcch_llr);

  //for (i = 0; i < (frame_parms->N_RB_DL * ((symbol == 0) ? 16 : 24)); i++) {
  for (i = 0; i < (coreset_nbr_rb * ((symbol == 0) ? 18 : 18)); i++) {
    if (*rxF > 31)
      *pdcch_llrp = 31;
    else if (*rxF < -32)
      *pdcch_llrp = -32;
    else
      *pdcch_llrp = (*rxF);

    LOG_DDD("llr logs: rb=%d i=%d *rxF:%d => *pdcch_llrp:%d\n",i/18,i,*rxF,*pdcch_llrp);
    rxF++;
    pdcch_llrp++;
  }

  return (0);
}


#if 0
int32_t pdcch_llr(NR_DL_FRAME_PARMS *frame_parms,
                  int32_t **rxdataF_comp,
                  char *pdcch_llr,
                  uint8_t symbol) {
  int16_t *rxF= (int16_t *) &rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  int32_t i;
  char *pdcch_llr8;
  pdcch_llr8 = &pdcch_llr[2*symbol*frame_parms->N_RB_DL*12];

  if (!pdcch_llr8) {
    LOG_E(PHY,"pdcch_qpsk_llr: llr is null, symbol %d\n",symbol);
    return(-1);
  }

  //    printf("pdcch qpsk llr for symbol %d (pos %d), llr offset %d\n",symbol,(symbol*frame_parms->N_RB_DL*12),pdcch_llr8-pdcch_llr);

  for (i=0; i<(frame_parms->N_RB_DL*((symbol==0) ? 16 : 24)); i++) {
    if (*rxF>31)
      *pdcch_llr8=31;
    else if (*rxF<-32)
      *pdcch_llr8=-32;
    else
      *pdcch_llr8 = (char)(*rxF);

    //    printf("%d %d => %d\n",i,*rxF,*pdcch_llr8);
    rxF++;
    pdcch_llr8++;
  }

  return(0);
}
#endif

//__m128i avg128P;

//compute average channel_level on each (TX,RX) antenna pair
void nr_pdcch_channel_level(int32_t **dl_ch_estimates_ext,
                            NR_DL_FRAME_PARMS *frame_parms,
                            int32_t *avg,
                            int symbol,
                            uint8_t nb_rb) {
  int16_t rb;
  uint8_t aarx;
#if defined(__x86_64__) || defined(__i386__)
  __m128i *dl_ch128;
  __m128i avg128P;
#elif defined(__arm__)
  int16x8_t *dl_ch128;
  int32x4_t *avg128P;
#endif

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    //clear average level
#if defined(__x86_64__) || defined(__i386__)
    avg128P = _mm_setzero_si128();
    dl_ch128=(__m128i *)&dl_ch_estimates_ext[aarx][symbol*nb_rb*12];
#elif defined(__arm__)
    dl_ch128=(int16x8_t *)&dl_ch_estimates_ext[aarx][symbol*nb_rb*12];
#endif

    for (rb=0; rb<(nb_rb*3)>>2; rb++) {
#if defined(__x86_64__) || defined(__i386__)
      avg128P = _mm_add_epi32(avg128P,_mm_madd_epi16(dl_ch128[0],dl_ch128[0]));
      avg128P = _mm_add_epi32(avg128P,_mm_madd_epi16(dl_ch128[1],dl_ch128[1]));
      avg128P = _mm_add_epi32(avg128P,_mm_madd_epi16(dl_ch128[2],dl_ch128[2]));
#elif defined(__arm__)
#endif
      //      for (int i=0;i<24;i+=2) printf("pdcch channel re %d (%d,%d)\n",(rb*12)+(i>>1),((int16_t*)dl_ch128)[i],((int16_t*)dl_ch128)[i+1]);
      dl_ch128+=3;
      /*
      if (rb==0) {
      print_shorts("dl_ch128",&dl_ch128[0]);
      print_shorts("dl_ch128",&dl_ch128[1]);
      print_shorts("dl_ch128",&dl_ch128[2]);
      }
      */
    }

    DevAssert( nb_rb );
    avg[aarx] = (((int32_t *)&avg128P)[0] +
                 ((int32_t *)&avg128P)[1] +
                 ((int32_t *)&avg128P)[2] +
                 ((int32_t *)&avg128P)[3])/(nb_rb*9);
    LOG_DDD("Channel level : %d\n",avg[aarx]);
  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}

#if defined(__x86_64) || defined(__i386__)
  __m128i mmtmpPD0,mmtmpPD1,mmtmpPD2,mmtmpPD3;
#elif defined(__arm__)

#endif




// This function will extract the mapped DM-RS PDCCH REs as per 38.211 Section 7.4.1.3.2 (Mapping to physical resources)
void nr_pdcch_extract_rbs_single(int32_t **rxdataF,
                                 int32_t **dl_ch_estimates,
                                 int32_t **rxdataF_ext,
                                 int32_t **dl_ch_estimates_ext,
                                 uint8_t symbol,
                                 NR_DL_FRAME_PARMS *frame_parms,
                                 uint8_t *coreset_freq_dom,
                                 uint32_t coreset_nbr_rb,
                                 uint32_t n_BWP_start) {
  /*
   * This function is demapping DM-RS PDCCH RE
   * Implementing 38.211 Section 7.4.1.3.2 Mapping to physical resources
   * PDCCH DM-RS signals are mapped on RE a_k_l where:
   * k = 12*n + 4*kprime + 1
   * n=0,1,..
   * kprime=0,1,2
   * According to this equations, DM-RS PDCCH are mapped on k where k%12==1 || k%12==5 || k%12==9
   *
   */
  // the bitmap coreset_frq_domain contains 45 bits
#define CORESET_FREQ_DOMAIN_BITMAP_SIZE   45
  // each bit is associated to 6 RBs
#define BIT_TO_NBR_RB_CORESET_FREQ_DOMAIN  6
#define NBR_RE_PER_RB_WITH_DMRS           12
  // after removing the 3 DMRS RE, the RB contains 9 RE with PDCCH
#define NBR_RE_PER_RB_WITHOUT_DMRS         9
  uint16_t c_rb, nb_rb = 0;
  //uint8_t rb_count_bit;
  uint8_t i, j, aarx;
  int32_t *dl_ch0, *dl_ch0_ext, *rxF, *rxF_ext;

#ifdef DEBUG_DCI_DECODING
  uint8_t symbol_mod = (symbol >= (7 - frame_parms->Ncp)) ? symbol - (7 - frame_parms->Ncp) : symbol;
  LOG_I(PHY, "extract_rbs_single: symbol_mod %d\n",symbol_mod);
#endif

  for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
    dl_ch0 = &dl_ch_estimates[aarx][frame_parms->ofdm_symbol_size*symbol];
    LOG_DDD("dl_ch0 = &dl_ch_estimates[aarx = (%d)][0]\n",aarx);

    dl_ch0_ext = &dl_ch_estimates_ext[aarx][symbol * (coreset_nbr_rb * NBR_RE_PER_RB_WITH_DMRS)];
    LOG_DDD("dl_ch0_ext = &dl_ch_estimates_ext[aarx = (%d)][symbol * (frame_parms->N_RB_DL * 9) = (%d)]\n",
           aarx,symbol * (coreset_nbr_rb * NBR_RE_PER_RB_WITH_DMRS));
    rxF_ext = &rxdataF_ext[aarx][symbol * (coreset_nbr_rb * NBR_RE_PER_RB_WITH_DMRS)];
    LOG_DDD("rxF_ext = &rxdataF_ext[aarx = (%d)][symbol * (frame_parms->N_RB_DL * 9) = (%d)]\n",
           aarx,symbol * (coreset_nbr_rb * NBR_RE_PER_RB_WITH_DMRS));

    /*
     * The following for loop handles treatment of PDCCH contained in table rxdataF (in frequency domain)
     * In NR the PDCCH IQ symbols are contained within RBs in the CORESET defined by higher layers which is located within the BWP
     * Lets consider that the first RB to be considered as part of the CORESET and part of the PDCCH is n_BWP_start
     * Several cases have to be handled differently as IQ symbols are situated in different parts of rxdataF:
     * 1. Number of RBs in the system bandwidth is even
     *    1.1 The RB is <  than the N_RB_DL/2 -> IQ symbols are in the second half of the rxdataF (from first_carrier_offset)
     *    1.2 The RB is >= than the N_RB_DL/2 -> IQ symbols are in the first half of the rxdataF (from element 0)
     * 2. Number of RBs in the system bandwidth is odd
     * (particular case when the RB with DC as it is treated differently: it is situated in symbol borders of rxdataF)
     *    2.1 The RB is <= than the N_RB_DL/2   -> IQ symbols are in the second half of the rxdataF (from first_carrier_offset)
     *    2.2 The RB is >  than the N_RB_DL/2+1 -> IQ symbols are in the first half of the rxdataF (from element 0 + 2nd half RB containing DC)
     *    2.3 The RB is == N_RB_DL/2+1          -> IQ symbols are in the lower border of the rxdataF for first 6 IQ element and the upper border of the rxdataF for the last 6 IQ elements
     * If the first RB containing PDCCH within the UE BWP and within the CORESET is higher than half of the system bandwidth (N_RB_DL),
     * then the IQ symbol is going to be found at the position 0+c_rb-N_RB_DL/2 in rxdataF and
     * we have to point the pointer at (1+c_rb-N_RB_DL/2) in rxdataF
     */

    int c_rb_by6;
    c_rb = 0;
    for (int rb=0;rb<coreset_nbr_rb;rb++,c_rb++) {
      c_rb_by6 = c_rb/6;

      // skip zeros in frequency domain bitmap
      while ((coreset_freq_dom[c_rb_by6>>3] & (1<<(7-(c_rb_by6&7)))) == 0) {
        c_rb+=6;
        c_rb_by6 = c_rb/6;
      }

      rxF=NULL;

      // first we set initial conditions for pointer to rxdataF depending on the situation of the first RB within the CORESET (c_rb = n_BWP_start)
      if (((c_rb + n_BWP_start) < (frame_parms->N_RB_DL >> 1)) && ((frame_parms->N_RB_DL & 1) == 0)) {
        //if RB to be treated is lower than middle system bandwidth then rxdataF pointed at (offset + c_br + symbol * ofdm_symbol_size): even case
        rxF = &rxdataF[aarx][(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size)))+n_BWP_start*12];
        LOG_DDD("in even case c_rb (%d) is lower than half N_RB_DL -> rxF = &rxdataF[aarx = (%d)][(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size))) = (%d)]\n",
               c_rb,aarx,(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size))));
      }

      if (((c_rb + n_BWP_start) >= (frame_parms->N_RB_DL >> 1)) && ((frame_parms->N_RB_DL & 1) == 0)) {
        // number of RBs is even  and c_rb is higher than half system bandwidth (we don't skip DC)
        // if these conditions are true the pointer has to be situated at the 1st part of the rxdataF
        rxF = &rxdataF[aarx][(12*(c_rb - (frame_parms->N_RB_DL>>1)) + (symbol * (frame_parms->ofdm_symbol_size)))+n_BWP_start*12]; // we point at the 1st part of the rxdataF in symbol
        LOG_DDD("in even case c_rb (%d) is higher than half N_RB_DL (not DC) -> rxF = &rxdataF[aarx = (%d)][(12*(c_rb - (frame_parms->N_RB_DL>>1)) + (symbol * (frame_parms->ofdm_symbol_size))) = (%d)]\n",
               c_rb,aarx,(12*(c_rb - (frame_parms->N_RB_DL>>1)) + (symbol * (frame_parms->ofdm_symbol_size))));
        //rxF = &rxdataF[aarx][(1 + 12*(c_rb - (frame_parms->N_RB_DL>>1)) + (symbol * (frame_parms->ofdm_symbol_size)))]; // we point at the 1st part of the rxdataF in symbol
        //#ifdef NR_PDCCH_DCI_DEBUG
        //  LOG_DDD("in even case c_rb (%d) is higher than half N_RB_DL (not DC) -> rxF = &rxdataF[aarx = (%d)][(1 + 12*(c_rb - (frame_parms->N_RB_DL>>1)) + (symbol * (frame_parms->ofdm_symbol_size))) = (%d)]\n",
        //         c_rb,aarx,(1 + 12*(c_rb - (frame_parms->N_RB_DL>>1)) + (symbol * (frame_parms->ofdm_symbol_size))));
        //#endif
      }

      if (((c_rb + n_BWP_start) < (frame_parms->N_RB_DL >> 1)) && ((frame_parms->N_RB_DL & 1) != 0)) {
        //if RB to be treated is lower than middle system bandwidth then rxdataF pointed at (offset + c_br + symbol * ofdm_symbol_size): odd case
        rxF = &rxdataF[aarx][(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size)))+n_BWP_start*12];
#ifdef NR_PDCCH_DCI_DEBUG
        LOG_D(PHY,"in odd case c_rb (%d) is lower or equal than half N_RB_DL -> rxF = &rxdataF[aarx = (%d)][(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size))) = (%d)]\n",
               c_rb,aarx,(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size))));
#endif
      }

      if (((c_rb + n_BWP_start) > (frame_parms->N_RB_DL >> 1)) && ((frame_parms->N_RB_DL & 1) != 0)) {
        // number of RBs is odd  and   c_rb is higher than half system bandwidth + 1
        // if these conditions are true the pointer has to be situated at the 1st part of the rxdataF just after the first IQ symbols of the RB containing DC
        rxF = &rxdataF[aarx][(12*(c_rb - (frame_parms->N_RB_DL>>1)) - 6 + (symbol * (frame_parms->ofdm_symbol_size)))+n_BWP_start*12]; // we point at the 1st part of the rxdataF in symbol
#ifdef NR_PDCCH_DCI_DEBUG
        LOG_D(PHY,"in odd case c_rb (%d) is higher than half N_RB_DL (not DC) -> rxF = &rxdataF[aarx = (%d)][(12*(c_rb - frame_parms->N_RB_DL) - 5 + (symbol * (frame_parms->ofdm_symbol_size))) = (%d)]\n",
               c_rb,aarx,(12*(c_rb - (frame_parms->N_RB_DL>>1)) - 6 + (symbol * (frame_parms->ofdm_symbol_size))));
#endif
      }

      if (((c_rb + n_BWP_start) == (frame_parms->N_RB_DL >> 1)) && ((frame_parms->N_RB_DL & 1) != 0)) { // treatment of RB containing the DC
        // if odd number RBs in system bandwidth and first RB to be treated is higher than middle system bandwidth (around DC)
        // we have to treat the RB in two parts: first part from i=0 to 5, the data is at the end of rxdataF (pointing at the end of the table)
        rxF = &rxdataF[aarx][(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size)))+n_BWP_start*12];
#ifdef NR_PDCCH_DCI_DEBUG
        LOG_D(PHY,"in odd case c_rb (%d) is half N_RB_DL + 1 we treat DC case -> rxF = &rxdataF[aarx = (%d)][(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size))) = (%d)]\n",
               c_rb,aarx,(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size))));
#endif
        j = 0;

        for (i = 0; i < 6; i++) { //treating first part of the RB note that i=5 would correspond to DC. We treat it in NR
          if ((i != 1) && (i != 5)) {
            dl_ch0_ext[j] = dl_ch0[i];
            rxF_ext[j++] = rxF[i];
            //              printf("**extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j-1],*(1+(short*)&rxF_ext[j-1]));
          }
        }

        // then we point at the begining of the symbol part of rxdataF do process second part of RB
        rxF = &rxdataF[aarx][((symbol * (frame_parms->ofdm_symbol_size)))]; // we point at the 1st part of the rxdataF in symbol
#ifdef NR_PDCCH_DCI_DEBUG
        LOG_D(PHY,"in odd case c_rb (%d) is half N_RB_DL +1 we treat DC case -> rxF = &rxdataF[aarx = (%d)][(symbol * (frame_parms->ofdm_symbol_size)) = (%d)]\n",
               c_rb,aarx,(symbol * (frame_parms->ofdm_symbol_size)));
#endif
        for (; i < 12; i++) {
          if ((i != 9)) {
            dl_ch0_ext[j] = dl_ch0[i];
            rxF_ext[j++] = rxF[(1 + i - 6)];
            //              printf("**extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j-1],*(1+(short*)&rxF_ext[j-1]));
          }
        }

        nb_rb++;
        dl_ch0_ext += NBR_RE_PER_RB_WITHOUT_DMRS;
        rxF_ext += NBR_RE_PER_RB_WITHOUT_DMRS;
        dl_ch0 += 12;
        //rxF += 7;
        //c_rb++;
        //n_BWP_start++; // We have to increment this variable here to be consequent in the for loop afterwards
        //}
      } else { // treatment of any RB that does not contain the DC
        j = 0;

        for (i = 0; i < 12; i++) {
          if ((i != 1) && (i != 5) && (i != 9)) {
            rxF_ext[j] = rxF[i];
#ifdef NR_PDCCH_DCI_DEBUG
            LOG_D(PHY,"RB[c_rb %d] \t RE[re %d] => rxF_ext[%d]=(%d,%d)\t rxF[%d]=(%d,%d)\n",
                   c_rb, i, j, *(short *) &rxF_ext[j],*(1 + (short *) &rxF_ext[j]), i,
                   *(short *) &rxF[i], *(1 + (short *) &rxF[i]));
#endif
            dl_ch0_ext[j] = dl_ch0[i];

            //LOG_DDD("ch %d => dl_ch0(%d,%d)\n", i, *(short *) &dl_ch0[i], *(1 + (short*) &dl_ch0[i]));
            //printf("\t-> dl_ch0[%d] => dl_ch0_ext[%d](%d,%d)\n", i,j, *(short *) &dl_ch0[i], *(1 + (short*) &dl_ch0[i]));
            j++;
          } else {
#ifdef NR_PDCCH_DCI_DEBUG
            LOG_D(PHY,"RB[c_rb %d] \t RE[re %d] => rxF_ext[%d]=(%d,%d)\t rxF[%d]=(%d,%d) \t\t <==> DM-RS PDCCH, this is a pilot symbol\n",
                   c_rb, i, j, *(short *) &rxF_ext[j], *(1 + (short *) &rxF_ext[j]), i,
                   *(short *) &rxF[i], *(1 + (short *) &rxF[i]));
#endif
          }
        }

        nb_rb++;
        dl_ch0_ext += NBR_RE_PER_RB_WITHOUT_DMRS;
        rxF_ext += NBR_RE_PER_RB_WITHOUT_DMRS;
        dl_ch0 += 12;
        //rxF += 12;
        //}
      }
    }
  }
}

#define print_shorts(s,x) printf("%s %d,%d,%d,%d,%d,%d,%d,%d\n",s,(x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5],(x)[6],(x)[7])

void nr_pdcch_channel_compensation(int32_t **rxdataF_ext,
                                   int32_t **dl_ch_estimates_ext,
                                   int32_t **rxdataF_comp,
                                   int32_t **rho,
                                   NR_DL_FRAME_PARMS *frame_parms,
                                   uint8_t symbol,
                                   uint8_t output_shift,
                                   uint32_t coreset_nbr_rb) {
  uint16_t rb; //,nb_rb=20;
  uint8_t aarx;
#if defined(__x86_64__) || defined(__i386__)
  __m128i mmtmpP0,mmtmpP1,mmtmpP2,mmtmpP3;
#elif defined(__arm__)
  int16x8_t mmtmpP0,mmtmpP1,mmtmpP2,mmtmpP3;
#endif
#if defined(__x86_64__) || defined(__i386__)
  __m128i *dl_ch128,*rxdataF128,*rxdataF_comp128;
#elif defined(__arm__)
#endif

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
#if defined(__x86_64__) || defined(__i386__)
    dl_ch128          = (__m128i *)&dl_ch_estimates_ext[aarx][symbol*coreset_nbr_rb*12];
    rxdataF128        = (__m128i *)&rxdataF_ext[aarx][symbol*coreset_nbr_rb*12];
    rxdataF_comp128   = (__m128i *)&rxdataF_comp[aarx][symbol*coreset_nbr_rb*12];
    //printf("ch compensation dl_ch ext addr %p \n", &dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*20*12]);
    //printf("rxdataf ext addr %p symbol %d\n", &rxdataF_ext[aarx][symbol*20*12], symbol);
    //printf("rxdataf_comp addr %p\n",&rxdataF_comp[(aatx<<1)+aarx][symbol*20*12]);
#elif defined(__arm__)
    // to be filled in
#endif

    for (rb=0; rb<(coreset_nbr_rb*3)>>2; rb++) {
#if defined(__x86_64__) || defined(__i386__)
      // multiply by conjugated channel
      mmtmpP0 = _mm_madd_epi16(dl_ch128[0],rxdataF128[0]);
      //print_ints("re",&mmtmpP0);
      // mmtmpP0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpP1 = _mm_shufflelo_epi16(dl_ch128[0],_MM_SHUFFLE(2,3,0,1));
      mmtmpP1 = _mm_shufflehi_epi16(mmtmpP1,_MM_SHUFFLE(2,3,0,1));
      mmtmpP1 = _mm_sign_epi16(mmtmpP1,*(__m128i *)&conjugate[0]);
      //print_ints("im",&mmtmpP1);
      mmtmpP1 = _mm_madd_epi16(mmtmpP1,rxdataF128[0]);
      // mmtmpP1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpP0 = _mm_srai_epi32(mmtmpP0,output_shift);
      //  print_ints("re(shift)",&mmtmpP0);
      mmtmpP1 = _mm_srai_epi32(mmtmpP1,output_shift);
      //  print_ints("im(shift)",&mmtmpP1);
      mmtmpP2 = _mm_unpacklo_epi32(mmtmpP0,mmtmpP1);
      mmtmpP3 = _mm_unpackhi_epi32(mmtmpP0,mmtmpP1);
      //print_ints("c0",&mmtmpP2);
      //print_ints("c1",&mmtmpP3);
      rxdataF_comp128[0] = _mm_packs_epi32(mmtmpP2,mmtmpP3);
//      print_shorts("rx:",(int16_t*)rxdataF128);
//      print_shorts("ch:",(int16_t*)dl_ch128);
//      print_shorts("pack:",(int16_t*)rxdataF_comp128);
      // multiply by conjugated channel
      mmtmpP0 = _mm_madd_epi16(dl_ch128[1],rxdataF128[1]);
      // mmtmpP0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpP1 = _mm_shufflelo_epi16(dl_ch128[1],_MM_SHUFFLE(2,3,0,1));
      mmtmpP1 = _mm_shufflehi_epi16(mmtmpP1,_MM_SHUFFLE(2,3,0,1));
      mmtmpP1 = _mm_sign_epi16(mmtmpP1,*(__m128i *)&conjugate[0]);
      mmtmpP1 = _mm_madd_epi16(mmtmpP1,rxdataF128[1]);
      // mmtmpP1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpP0 = _mm_srai_epi32(mmtmpP0,output_shift);
      mmtmpP1 = _mm_srai_epi32(mmtmpP1,output_shift);
      mmtmpP2 = _mm_unpacklo_epi32(mmtmpP0,mmtmpP1);
      mmtmpP3 = _mm_unpackhi_epi32(mmtmpP0,mmtmpP1);
      rxdataF_comp128[1] = _mm_packs_epi32(mmtmpP2,mmtmpP3);
      //print_shorts("rx:",rxdataF128+1);
      //print_shorts("ch:",dl_ch128+1);
      //print_shorts("pack:",rxdataF_comp128+1);
      // multiply by conjugated channel
      mmtmpP0 = _mm_madd_epi16(dl_ch128[2],rxdataF128[2]);
      // mmtmpP0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpP1 = _mm_shufflelo_epi16(dl_ch128[2],_MM_SHUFFLE(2,3,0,1));
      mmtmpP1 = _mm_shufflehi_epi16(mmtmpP1,_MM_SHUFFLE(2,3,0,1));
      mmtmpP1 = _mm_sign_epi16(mmtmpP1,*(__m128i *)&conjugate[0]);
      mmtmpP1 = _mm_madd_epi16(mmtmpP1,rxdataF128[2]);
      // mmtmpP1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpP0 = _mm_srai_epi32(mmtmpP0,output_shift);
      mmtmpP1 = _mm_srai_epi32(mmtmpP1,output_shift);
      mmtmpP2 = _mm_unpacklo_epi32(mmtmpP0,mmtmpP1);
      mmtmpP3 = _mm_unpackhi_epi32(mmtmpP0,mmtmpP1);
      rxdataF_comp128[2] = _mm_packs_epi32(mmtmpP2,mmtmpP3);
      ///////////////////////////////////////////////////////////////////////////////////////////////
      //print_shorts("rx:",rxdataF128+2);
      //print_shorts("ch:",dl_ch128+2);
      //print_shorts("pack:",rxdataF_comp128+2);

      for (int i=0; i<12 ; i++)
        LOG_DDD("rxdataF128[%d]=(%d,%d) X dlch[%d]=(%d,%d) rxdataF_comp128[%d]=(%d,%d)\n",
                (rb*12)+i, ((short *)rxdataF128)[i<<1],((short *)rxdataF128)[1+(i<<1)],
                (rb*12)+i, ((short *)dl_ch128)[i<<1],((short *)dl_ch128)[1+(i<<1)],
                (rb*12)+i, ((short *)rxdataF_comp128)[i<<1],((short *)rxdataF_comp128)[1+(i<<1)]);

      dl_ch128+=3;
      rxdataF128+=3;
      rxdataF_comp128+=3;
#elif defined(__arm__)
      // to be filled in
#endif
    }
  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}


void nr_pdcch_detection_mrc(NR_DL_FRAME_PARMS *frame_parms,
                         int32_t **rxdataF_comp,
                         uint8_t symbol) {
#if defined(__x86_64__) || defined(__i386__)
  __m128i *rxdataF_comp128_0,*rxdataF_comp128_1;
#elif defined(__arm__)
  int16x8_t *rxdataF_comp128_0,*rxdataF_comp128_1;
#endif
  int32_t i;

  if (frame_parms->nb_antennas_rx>1) {
#if defined(__x86_64__) || defined(__i386__)
    rxdataF_comp128_0   = (__m128i *)&rxdataF_comp[0][symbol*frame_parms->N_RB_DL*12];
    rxdataF_comp128_1   = (__m128i *)&rxdataF_comp[1][symbol*frame_parms->N_RB_DL*12];
#elif defined(__arm__)
    rxdataF_comp128_0   = (int16x8_t *)&rxdataF_comp[0][symbol*frame_parms->N_RB_DL*12];
    rxdataF_comp128_1   = (int16x8_t *)&rxdataF_comp[1][symbol*frame_parms->N_RB_DL*12];
#endif

    // MRC on each re of rb
    for (i=0; i<frame_parms->N_RB_DL*3; i++) {
#if defined(__x86_64__) || defined(__i386__)
      rxdataF_comp128_0[i] = _mm_adds_epi16(_mm_srai_epi16(rxdataF_comp128_0[i],1),_mm_srai_epi16(rxdataF_comp128_1[i],1));
#elif defined(__arm__)
      rxdataF_comp128_0[i] = vhaddq_s16(rxdataF_comp128_0[i],rxdataF_comp128_1[i]);
#endif
    }
  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}

int32_t nr_rx_pdcch(PHY_VARS_NR_UE *ue,
                    UE_nr_rxtx_proc_t *proc,
                    fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15) {

  uint32_t frame = proc->frame_rx;
  uint32_t slot  = proc->nr_slot_rx;
  NR_UE_COMMON *common_vars      = &ue->common_vars;
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  NR_UE_PDCCH *pdcch_vars        = ue->pdcch_vars[proc->thread_id][0];

  uint8_t log2_maxh, aarx;
  int32_t avgs;
  int32_t avgP[4];
  int n_rb,rb_offset;
  get_coreset_rballoc(rel15->coreset.frequency_domain_resource,&n_rb,&rb_offset);
  LOG_D(PHY,"pdcch coreset: freq %x, n_rb %d, rb_offset %d\n",
        rel15->coreset.frequency_domain_resource[0],n_rb,rb_offset);
  for (int s=rel15->coreset.StartSymbolIndex; s<(rel15->coreset.StartSymbolIndex+rel15->coreset.duration); s++) {
    LOG_D(PHY,"in nr_pdcch_extract_rbs_single(rxdataF -> rxdataF_ext || dl_ch_estimates -> dl_ch_estimates_ext)\n");

    nr_pdcch_extract_rbs_single(common_vars->common_vars_rx_data_per_thread[proc->thread_id].rxdataF,
                                pdcch_vars->dl_ch_estimates,
                                pdcch_vars->rxdataF_ext,
                                pdcch_vars->dl_ch_estimates_ext,
                                s,
                                frame_parms,
                                rel15->coreset.frequency_domain_resource,
                                n_rb,
                                rel15->BWPStart);

    LOG_D(PHY,"we enter nr_pdcch_channel_level(avgP=%d) => compute channel level based on ofdm symbol 0, pdcch_vars[eNB_id]->dl_ch_estimates_ext\n",*avgP);
    LOG_D(PHY,"in nr_pdcch_channel_level(dl_ch_estimates_ext -> dl_ch_estimates_ext)\n");
    // compute channel level based on ofdm symbol 0
    nr_pdcch_channel_level(pdcch_vars->dl_ch_estimates_ext,
                           frame_parms,
                           avgP,
                           s,
                           n_rb);
    avgs = 0;

    for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++)
      avgs = cmax(avgs, avgP[aarx]);

    log2_maxh = (log2_approx(avgs) / 2) + 5;  //+frame_parms->nb_antennas_rx;

#ifdef UE_DEBUG_TRACE
    LOG_D(PHY,"slot %d: pdcch log2_maxh = %d (%d,%d)\n",slot,log2_maxh,avgP[0],avgs);
#endif
#if T_TRACER
    T(T_UE_PHY_PDCCH_ENERGY, T_INT(0), T_INT(0), T_INT(frame%1024), T_INT(slot),
      T_INT(avgP[0]), T_INT(avgP[1]), T_INT(avgP[2]), T_INT(avgP[3]));
#endif
    LOG_D(PHY,"we enter nr_pdcch_channel_compensation(log2_maxh=%d)\n",log2_maxh);
    LOG_D(PHY,"in nr_pdcch_channel_compensation(rxdataF_ext x dl_ch_estimates_ext -> rxdataF_comp)\n");
    // compute LLRs for ofdm symbol 0 only
    nr_pdcch_channel_compensation(pdcch_vars->rxdataF_ext,
                                  pdcch_vars->dl_ch_estimates_ext,
                                  pdcch_vars->rxdataF_comp,
                                  NULL,
                                  frame_parms,
                                  s,
                                  log2_maxh,
                                  n_rb); // log2_maxh+I0_shift
    if (frame_parms->nb_antennas_rx > 1) {
      LOG_D(PHY,"we enter nr_pdcch_detection_mrc(frame_parms->nb_antennas_rx=%d)\n", frame_parms->nb_antennas_rx);
      nr_pdcch_detection_mrc(frame_parms, pdcch_vars->rxdataF_comp,s);
    }

    LOG_D(PHY,"we enter nr_pdcch_llr(for symbol %d), pdcch_vars[eNB_id]->rxdataF_comp ---> pdcch_vars[eNB_id]->llr \n",s);
    LOG_D(PHY,"in nr_pdcch_llr(rxdataF_comp -> llr)\n");
    nr_pdcch_llr(frame_parms,
                 pdcch_vars->rxdataF_comp,
                 pdcch_vars->llr,
                 s,
                 n_rb);
#if T_TRACER
    
    //  T(T_UE_PHY_PDCCH_IQ, T_INT(frame_parms->N_RB_DL), T_INT(frame_parms->N_RB_DL),
    //  T_INT(n_pdcch_symbols),
    //  T_BUFFER(pdcch_vars[eNB_id]->rxdataF_comp, frame_parms->N_RB_DL*12*n_pdcch_symbols* 4));
    
#endif
#ifdef DEBUG_DCI_DECODING
    printf("demapping: slot %d, mi %d\n",slot,get_mi(frame_parms,slot));
#endif
  }

  LOG_D(PHY,"we enter nr_pdcch_demapping_deinterleaving(), number of candidates %d\n",rel15->number_of_candidates);
  nr_pdcch_demapping_deinterleaving((uint32_t *) pdcch_vars->llr,
                                    (uint32_t *) pdcch_vars->e_rx,
                                    rel15->coreset.duration,
                                    rel15->coreset.StartSymbolIndex,
                                    n_rb,
                                    rel15->coreset.RegBundleSize,
                                    rel15->coreset.InterleaverSize,
                                    rel15->coreset.ShiftIndex,
                                    rel15->number_of_candidates,
                                    rel15->CCE,
                                    rel15->L);
    /*
    nr_pdcch_unscrambling(rel15->rnti,
                          frame_parms,
                          slot,
                          pdcch_vars->e_rx,
                          rel15->coreset.duration*n_rb*9*2,
                          // get_nCCE(n_pdcch_symbols, frame_parms, mi) * 72,
                          rel15->coreset.pdcch_dmrs_scrambling_id);
    */
  LOG_D(PHY,"we end nr_pdcch_unscrambling()\n");
  LOG_D(PHY,"Ending nr_rx_pdcch() function\n");

  return (0);
}



void nr_pdcch_unscrambling(int16_t *z,
                           uint16_t scrambling_RNTI,
                           uint32_t length,
                           uint16_t pdcch_DMRS_scrambling_id,
                           int16_t *z2) {
  int i;
  uint8_t reset;
  uint32_t x1, x2, s = 0;
  uint16_t n_id; //{0,1,...,65535}
  uint32_t rnti = (uint32_t) scrambling_RNTI;
  reset = 1;
  // x1 is set in first call to lte_gold_generic
  n_id = pdcch_DMRS_scrambling_id;
  x2 = ((rnti<<16) + n_id); //mod 2^31 is implicit //this is c_init in 38.211 v15.1.0 Section 7.3.2.3

  LOG_D(PHY,"PDCCH Unscrambling x2 %x : scrambling_RNTI %x\n", x2, rnti);

  for (i = 0; i < length; i++) {
    if ((i & 0x1f) == 0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
    }

    if (((s >> (i % 32)) & 1) == 1) z2[i] = -z[i];
    else z2[i]=z[i];
  }
}


/* This function compares the received DCI bits with
 * re-encoded DCI bits and returns the number of mismatched bits
 */
uint16_t nr_dci_false_detection(uint64_t *dci,
                            int16_t *soft_in,
                            const t_nrPolar_params *polar_param,
                            int encoded_length,
                            int rnti) {

  uint32_t encoder_output[NR_MAX_DCI_SIZE_DWORD];
  polar_encoder_fast(dci, (void*)encoder_output, rnti, 1, (t_nrPolar_params *)polar_param);
  uint8_t *enout_p = (uint8_t*)encoder_output;
  uint16_t x = 0;

  for (int i=0; i<encoded_length/8; i++) {
    x += ( enout_p[i] & 1 ) ^ ( ( soft_in[i*8] >> 15 ) & 1);
    x += ( ( enout_p[i] >> 1 ) & 1 ) ^ ( ( soft_in[i*8+1] >> 15 ) & 1 );
    x += ( ( enout_p[i] >> 2 ) & 1 ) ^ ( ( soft_in[i*8+2] >> 15 ) & 1 );
    x += ( ( enout_p[i] >> 3 ) & 1 ) ^ ( ( soft_in[i*8+3] >> 15 ) & 1 );
    x += ( ( enout_p[i] >> 4 ) & 1 ) ^ ( ( soft_in[i*8+4] >> 15 ) & 1 );
    x += ( ( enout_p[i] >> 5 ) & 1 ) ^ ( ( soft_in[i*8+5] >> 15 ) & 1 );
    x += ( ( enout_p[i] >> 6 ) & 1 ) ^ ( ( soft_in[i*8+6] >> 15 ) & 1 );
    x += ( ( enout_p[i] >> 7 ) & 1 ) ^ ( ( soft_in[i*8+7] >> 15 ) & 1 );
  }
  return x;
}

uint8_t nr_dci_decoding_procedure(PHY_VARS_NR_UE *ue,
                                  UE_nr_rxtx_proc_t *proc,
                                  fapi_nr_dci_indication_t *dci_ind,
                                  fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15) {

  NR_UE_PDCCH *pdcch_vars = ue->pdcch_vars[proc->thread_id][0];

  //int gNB_id = 0;
  int16_t tmp_e[16*108];
  rnti_t n_rnti;

  for (int j=0;j<rel15->number_of_candidates;j++) {
    int CCEind = rel15->CCE[j];
    int L = rel15->L[j];

    // Loop over possible DCI lengths
    for (int k = 0; k < rel15->num_dci_options; k++) {
      // skip this candidate if we've already found one with the
      // same rnti and format at a different aggregation level
      int dci_found=0;
      for (int ind=0;ind < dci_ind->number_of_dcis ; ind++) {
        if (rel15->rnti== dci_ind->dci_list[ind].rnti &&
            rel15->dci_format_options[k]==dci_ind->dci_list[ind].dci_format) {
           dci_found=1;
           break;
        }
      }
      if (dci_found==1) continue;
      int dci_length = rel15->dci_length_options[k];
      uint64_t dci_estimation[2]= {0};
      const t_nrPolar_params *currentPtrDCI = nr_polar_params(NR_POLAR_DCI_MESSAGE_TYPE, dci_length, L, 1, &ue->polarList);

      LOG_D(PHY, "Trying DCI candidate %d of %d number of candidates, CCE %d (%d), L %d, length %d, format %s\n",
            j, rel15->number_of_candidates, CCEind, CCEind*9*6*2, L, dci_length,nr_dci_format_string[rel15->dci_format_options[k]]);

      nr_pdcch_unscrambling(&pdcch_vars->e_rx[CCEind*108], rel15->coreset.scrambling_rnti, L*108, rel15->coreset.pdcch_dmrs_scrambling_id, tmp_e);

#ifdef DEBUG_DCI_DECODING
      uint32_t * z = (uint32_t *) &pdcch_vars->e_rx[CCEind*108];
      for (int index_z = 0; index_z < 96; index_z++){
        for (int i=0; i<9; i++) {
          LOG_D(PHY,"z[%d]=(%d,%d) \n", (9*index_z + i), *(int16_t *) &z[index_z + i],*(1 + (int16_t *) &z[index_z + i]));
        }
      }
#endif

      uint16_t crc = polar_decoder_int16(tmp_e,
                                         dci_estimation,
                                         1,
                                         currentPtrDCI);

      n_rnti = rel15->rnti;
      LOG_D(PHY, "(%i.%i) dci indication (rnti %x,dci format %s,n_CCE %d,payloadSize %d)\n",
            proc->frame_rx, proc->nr_slot_rx,n_rnti,nr_dci_format_string[rel15->dci_format_options[k]],CCEind,dci_length);
      if (crc == n_rnti) {
        LOG_D(PHY, "(%i.%i) Received dci indication (rnti %x,dci format %s,n_CCE %d,payloadSize %d,payload %llx)\n",
              proc->frame_rx, proc->nr_slot_rx,n_rnti,nr_dci_format_string[rel15->dci_format_options[k]],CCEind,dci_length,*(unsigned long long*)dci_estimation);
        uint16_t mb = nr_dci_false_detection(dci_estimation,tmp_e,currentPtrDCI,L*108,n_rnti);
        ue->dci_thres = (ue->dci_thres + mb) / 2;
        if (mb > (ue->dci_thres+20)) {
          LOG_W(PHY,"DCI false positive. Dropping DCI index %d. Mismatched bits: %d/%d. Current DCI threshold: %d\n",j,mb,L*108,ue->dci_thres);
          continue;
        }
        else {
          dci_ind->SFN = proc->frame_rx;
          dci_ind->slot = proc->nr_slot_rx;
          dci_ind->dci_list[dci_ind->number_of_dcis].rnti        = n_rnti;
          dci_ind->dci_list[dci_ind->number_of_dcis].n_CCE       = CCEind;
          dci_ind->dci_list[dci_ind->number_of_dcis].N_CCE       = L;
          dci_ind->dci_list[dci_ind->number_of_dcis].dci_format  = rel15->dci_format_options[k];
          dci_ind->dci_list[dci_ind->number_of_dcis].payloadSize = dci_length;
          memcpy((void*)dci_ind->dci_list[dci_ind->number_of_dcis].payloadBits,(void*)dci_estimation,8);
          dci_ind->number_of_dcis++;
          break;    // If DCI is found, no need to check for remaining DCI lengths
        }
      } else {
        LOG_D(PHY,"(%i.%i) Decoded crc %x does not match rnti %x for DCI format %d\n", proc->frame_rx, proc->nr_slot_rx, crc, n_rnti, rel15->dci_format_options[k]);
      }
    }
  }
  pdcch_vars->nb_search_space = 0;
  return(dci_ind->number_of_dcis);
}


