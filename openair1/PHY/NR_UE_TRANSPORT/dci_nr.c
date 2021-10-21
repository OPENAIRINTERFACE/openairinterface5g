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

#ifdef USER_MODE
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
#endif

#include <LAYER2/NR_MAC_UE/mac_defs.h>
#include <LAYER2/NR_MAC_UE/mac_proto.h>
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
#define NR_PDCCH_DCI_RUN              // activates new nr functions
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

#ifdef NR_PDCCH_DCI_RUN


//static const int16_t conjugate[8]__attribute__((aligned(32))) = {-1,1,-1,1,-1,1,-1,1};


void nr_pdcch_demapping_deinterleaving(uint32_t *llr,
                                       uint32_t *z,
                                       uint8_t coreset_time_dur,
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
    for (int symbol_idx = 0; symbol_idx < coreset_time_dur; symbol_idx++) {
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

#endif

#ifdef NR_PDCCH_DCI_RUN
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
#endif


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
    dl_ch128=(__m128i *)&dl_ch_estimates_ext[aarx][0];
#elif defined(__arm__)
    dl_ch128=(int16x8_t *)&dl_ch_estimates_ext[aarx][0];
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
    //            printf("Channel level : %d\n",avg[(aatx<<1)+aarx]);
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




#ifdef NR_PDCCH_DCI_RUN
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
    dl_ch0 = &dl_ch_estimates[aarx][0];
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

#endif


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

#if 0
void pdcch_siso(NR_DL_FRAME_PARMS *frame_parms,
                int32_t **rxdataF_comp,
                uint8_t l) {
  uint8_t rb,re,jj,ii;
  jj=0;
  ii=0;

  for (rb=0; rb<frame_parms->N_RB_DL; rb++) {
    for (re=0; re<12; re++) {
      rxdataF_comp[0][jj++] = rxdataF_comp[0][ii];
      ii++;
    }
  }
}
#endif

#ifdef NR_PDCCH_DCI_RUN
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

#endif

#if 0
void nr_pdcch_scrambling(NR_DL_FRAME_PARMS *frame_parms,
                      uint8_t nr_slot_rx,
                      uint8_t *e,
                      uint32_t length) {
  int i;
  uint8_t reset;
  uint32_t x1, x2, s=0;
  reset = 1;
  // x1 is set in lte_gold_generic
  x2 = (nr_slot_rx<<9) + frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.8.2

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
#endif


#ifdef NR_PDCCH_DCI_RUN

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

#endif


#ifdef NR_PDCCH_DCI_RUN
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

      LOG_D(PHY, "Trying DCI candidate %d of %d number of candidates, CCE %d (%d), L %d, length %d, format %s\n", j, rel15->number_of_candidates, CCEind, CCEind*9*6*2, L, dci_length,nr_dci_format_string[rel15->dci_format_options[k]]);

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

/*
void nr_dci_decoding_procedure0(int s,
                                int p,
                                int coreset_time_dur,
                                uint16_t coreset_nbr_rb,
                                NR_UE_PDCCH **pdcch_vars,
                                int do_common,
                                uint8_t nr_slot_rx,
                                NR_DCI_ALLOC_t *dci_alloc,
                                int16_t eNB_id,
                                uint8_t current_thread_id,
                                NR_DL_FRAME_PARMS *frame_parms,
                                //uint8_t mi,
                                uint16_t crc_scrambled_values[TOTAL_NBR_SCRAMBLED_VALUES],
                                uint8_t L,
                                NR_UE_SEARCHSPACE_CSS_DCI_FORMAT_t format_css,
                                NR_UE_SEARCHSPACE_USS_DCI_FORMAT_t format_uss,
                                uint8_t sizeof_bits,
                                uint8_t sizeof_bytes,
                                uint8_t *dci_cnt,
                                crc_scrambled_t *crc_scrambled,
                                format_found_t *format_found,
                                uint16_t pdcch_DMRS_scrambling_id,
                                uint32_t *CCEmap0,
                                uint32_t *CCEmap1,
                                uint32_t *CCEmap2) {
  uint32_t crc, CCEind, nCCE[3];
  uint32_t *CCEmap = NULL, CCEmap_mask = 0;
  uint8_t L2 = (1 << L);
  unsigned int Yk, nb_candidates = 0, i, m;
  unsigned int CCEmap_cand;
  uint32_t decoderState=0;
  // A[p], p is the current active CORESET
  uint16_t A[3]= {39827,39829,39839};
  //Table 10.1-2: Maximum number of PDCCH candidates    per slot and per serving cell as a function of the subcarrier spacing value 2^mu*15 KHz, mu {0,1,2,3}
  uint8_t m_max_slot_pdcch_Table10_1_2 [4] = {44,36,22,20};
  //Table 10.1-3: Maximum number of non-overlapped CCEs per slot and per serving cell as a function of the subcarrier spacing value 2^mu*15 KHz, mu {0,1,2,3}
  //uint8_t cce_max_slot_pdcch_Table10_1_3 [4] = {56,56,48,32};
  int coreset_nbr_cce_per_symbol=0;
  LOG_DDD("format_found is %d \n", *format_found);
  //if (mode == NO_DCI) {
  //  #ifdef NR_PDCCH_DCI_DEBUG
  //    LOG_DDD("skip DCI decoding: expect no DCIs at nr_slot_rx %d in current searchSpace\n", nr_slot_rx);
  //  #endif
  //  return;
  //}
  LOG_DDD("frequencyDomainResources=%lx, duration=%d\n",
         pdcch_vars[eNB_id]->coreset[p].frequencyDomainResources, pdcch_vars[eNB_id]->coreset[p].duration);

  // nCCE = get_nCCE(pdcch_vars[eNB_id]->num_pdcch_symbols, frame_parms, mi);
  for (int i = 0; i < 45; i++) {
    // this loop counts each bit of the bit map coreset_freq_dom, and increments nbr_RB_coreset for each bit set to '1'
    if (((pdcch_vars[eNB_id]->coreset[p].frequencyDomainResources & 0x1FFFFFFFFFFF) >> i) & 0x1) coreset_nbr_cce_per_symbol++;
  }

  nCCE[p] = pdcch_vars[eNB_id]->coreset[p].duration*coreset_nbr_cce_per_symbol; // 1 CCE = 6 RB
  // p is the current CORESET we are currently monitoring (among the 3 possible CORESETs in a BWP)
  // the number of CCE in the current CORESET is:
  //   the number of symbols in the CORESET (pdcch_vars[eNB_id]->coreset[p].duration)
  //   multiplied by the number of bits set to '1' in the frequencyDomainResources bitmap
  //   (1 bit set to '1' corresponds to 6 RB and 1 CCE = 6 RB)
  LOG_DDD("nCCE[%d]=%d\n",p,nCCE[p]);

  //  if (nCCE > get_nCCE(3, frame_parms, 1)) {
  //LOG_D(PHY,
  //"skip DCI decoding: nCCE=%d > get_nCCE(3,frame_parms,1)=%d\n",
  //nCCE, get_nCCE(3, frame_parms, 1));
  //return;
 // }

//  if (nCCE < L2) {
//  LOG_D(PHY, "skip DCI decoding: nCCE=%d < L2=%d\n", nCCE, L2);
//  return;
//  }

//  if (mode == NO_DCI) {
//  LOG_D(PHY, "skip DCI decoding: expect no DCIs at nr_slot_rx %d\n",
//  nr_slot_rx);
//  return;
//  }

  if (do_common == 1) {
    Yk = 0;

    if (pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.common_dci_formats == cformat2_0) {
      // for dci_format_2_0, the nb_candidates is obtained from a different variable
      switch (L2) {
        case 1:
          nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.sfi_nrofCandidates_aggrlevel1;
          break;

        case 2:
          nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.sfi_nrofCandidates_aggrlevel2;
          break;

        case 4:
          nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.sfi_nrofCandidates_aggrlevel4;
          break;

        case 8:
          nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.sfi_nrofCandidates_aggrlevel8;
          break;

        case 16:
          nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.sfi_nrofCandidates_aggrlevel16;
          break;

        default:
          break;
      }
    } else if (pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.common_dci_formats == cformat2_3) {
      // for dci_format_2_3, the nb_candidates is obtained from a different variable
      nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.srs_nrofCandidates;
    } else {
      nb_candidates = (L2 == 4) ? 4 : ((L2 == 8)? 2 : 1); // according to Table 10.1-1 (38.213 section 10.1)
      LOG_DDD("we are in common searchSpace and nb_candidates=%u for L2=%d\n", nb_candidates, L2);
    }
  } else {
    switch (L2) {
      case 1:
        nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].nrofCandidates_aggrlevel1;
        break;

      case 2:
        nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].nrofCandidates_aggrlevel2;
        break;

      case 4:
        nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].nrofCandidates_aggrlevel4;
        break;

      case 8:
        nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].nrofCandidates_aggrlevel8;
        break;

      case 16:
        nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].nrofCandidates_aggrlevel16;
        break;

      default:
        break;
    }

    // Find first available in ue specific search space
    // according to procedure in Section 10.1 of 38.213
    // compute Yk
    Yk = (unsigned int) pdcch_vars[eNB_id]->crnti;

    for (i = 0; i <= nr_slot_rx; i++)
      Yk = (Yk * A[p%3]) % 65537;
  }

  LOG_DDD("L2(%d) | nCCE[%d](%d) | Yk(%u) | nb_candidates(%u)\n", L2, p, nCCE[p], Yk, nb_candidates);
  //  for (CCEind=0;
  //    CCEind<nCCE2;
  //    CCEind+=(1<<L)) {
  //  if (nb_candidates * L2 > nCCE[p])
  //    nb_candidates = nCCE[p] / L2;
  // In the next code line there is maybe a bug. The spec is not comparing Table 10.1-2 with nb_candidates, but with total number of candidates for all s and all p
  int m_p_s_L_max = (m_max_slot_pdcch_Table10_1_2[1]<=nb_candidates ? m_max_slot_pdcch_Table10_1_2[1] : nb_candidates);

  if (L==4) m_p_s_L_max=1; // Table 10.1-2 is not defined for L=4

  if(0 <= L && L < 4) LOG_DDD("m_max_slot_pdcch_Table10_1_2(%d)=%d\n",L,m_max_slot_pdcch_Table10_1_2[L]);

  for (m = 0; m < nb_candidates; m++) {
    int n_ci = 0;

    if (nCCE[p] < L2) return;

  LOG_DDD("debug1(%d)=nCCE[p]/L2 | nCCE[%d](%d) | L2(%d)\n",nCCE[p] / L2,p,nCCE[p],L2);
  LOG_DDD("debug2(%d)=L2*m_p_s_L_max | L2(%d) | m_p_s_L_max(%d)\n",L2*m_p_s_L_max,L2,m_p_s_L_max);
  CCEind = (((Yk + (uint16_t)(floor((m*nCCE[p])/(L2*m_p_s_L_max))) + n_ci) % (uint16_t)(floor(nCCE[p] / L2))) * L2);
  LOG_DDD("CCEind(%u) = (((Yk(%u) + ((m(%u)*nCCE[p](%u))/(L2(%d)*m_p_s_L_max(%d)))) %% (nCCE[p] / L2)) * L2)\n",
            CCEind,Yk,m,nCCE[p],L2,m_p_s_L_max);
  LOG_DDD("n_candidate(m)=%u | CCEind=%u |",m,CCEind);

    if (CCEind < 32)
      CCEmap = CCEmap0;
    else if (CCEind < 64)
      CCEmap = CCEmap1;
    else if (CCEind < 96)
      CCEmap = CCEmap2;
    else AssertFatal(1==0,"Illegal CCEind %u (Yk %u, m %u, nCCE %u, L2 %u\n", CCEind, Yk, m, nCCE[p], L2);

    switch (L2) {
      case 1:
        CCEmap_mask = (1 << (CCEind & 0x1f));
        break;

      case 2:
        CCEmap_mask = (3 << (CCEind & 0x1f));
        break;

      case 4:
        CCEmap_mask = (0xf << (CCEind & 0x1f));
        break;

      case 8:
        CCEmap_mask = (0xff << (CCEind & 0x1f));
        break;

      case 16:
        CCEmap_mask = (0xfff << (CCEind & 0x1f));
        break;

      default:
        LOG_E(PHY, "Illegal L2 value %d\n", L2);
        //mac_xface->macphy_exit("Illegal L2\n");
        return; // not reached
    }

    CCEmap_cand = (*CCEmap) & CCEmap_mask;
    // CCE is not allocated yet
    LOG_DDD("CCEmap_cand=%u \n",CCEmap_cand);

    if (CCEmap_cand == 0) {
#ifdef DEBUG_DCI_DECODING

      if (do_common == 1)
        LOG_I(PHY,"[DCI search nPdcch %d - common] Attempting candidate %d Aggregation Level %d DCI length %d at CCE %d/%d (CCEmap %x,CCEmap_cand %x)\n",
              pdcch_vars[eNB_id]->num_pdcch_symbols,m,L2,sizeof_bits,CCEind,nCCE,*CCEmap,CCEmap_mask);
      else
        LOG_I(PHY,"[DCI search nPdcch %d - ue spec] Attempting candidate %d Aggregation Level %d DCI length %d at CCE %d/%d (CCEmap %x,CCEmap_cand %x) format %d\n",
              pdcch_vars[eNB_id]->num_pdcch_symbols,m,L2,sizeof_bits,CCEind,nCCE,*CCEmap,CCEmap_mask,format_uss);

#endif
      LOG_DDD("... we enter function dci_decoding(sizeof_bits=%d L=%d) -----\n",sizeof_bits,L);
      LOG_DDD("... we have to replace this part of the code by polar decoding\n");
      //      for (int m=0; m < (nCCE[p]*6*9*2); m++)
      LOG_DDD("(polar decoding)-> polar intput (with coreset_time_dur=%d, coreset_nbr_rb=%d, p=%d, CCEind=%u): \n",
             coreset_time_dur,coreset_nbr_rb,p,CCEind);
      
      //int reg_p=0,reg_e=0;
      //for (int m=0; m < (L2*6); m++){
      //reg_p = (((int)floor(m/coreset_time_dur))+((m%coreset_time_dur)*(L2*6/coreset_time_dur)))*9*2;
      //reg_e = m*9*2;
      //for (int i=0; i<9*2; i++){
      //polar_input[reg_p+i] = (pdcch_vars[eNB_id]->e_rx[((CCEind*9*6*2) + reg_e + i)]>0) ? (1.0):(-1.0);
      //polar_input[reg_e+i] = (pdcch_vars[eNB_id]->e_rx[((CCEind*9*6*2) + reg_e + i)]>0) ? (1/sqrt(2)):((-1)/sqrt(2));
      //printf("\t m=%d \tpolar_input[%d]=%lf <-> e_rx[%d]=%d\n",m,reg_e+i,polar_input[reg_e+i],
      //        ((CCEind*9*6*2) + reg_e + i),pdcch_vars[eNB_id]->e_rx[((CCEind*9*6*2) + reg_e + i)]);
      //printf("\t m=%d \tpolar_input[%d]=%lf <-> e_rx[%d]=%d\n",m,reg_p+i,polar_input[reg_p+i],
      //        ((CCEind*9*6*2) + reg_e + i),pdcch_vars[eNB_id]->e_rx[((CCEind*9*6*2) + reg_e + i)]);
      //}
      //}

      //#ifdef NR_PDCCH_DCI_DEBUG
      //printf("\n");
      //int j=0;
      //uint32_t polar_hex[27] = {0};
      //for (int i=0; i<L2*9*6*2; i++){2
      //if ((i%32 == 0) && (i!=0)) j++;
      //polar_hex[j] = (polar_hex[j]<<1) + ((polar_input[i]==-1)? 1:0);
      //polar_hex[j] = polar_hex[j] + (((polar_input[i]==((-1)/sqrt(2)))?1:0)<<(i%32));
      //}
      //for (j=0;j<27;j++) LOG_DDD("polar_hex[%d]=%x\n",j,polar_hex[j]);
      //#endif
      
      uint64_t dci_estimation[2]= {0};
      const t_nrPolar_params *currentPtrDCI=nr_polar_params(1, sizeof_bits, L2,1);
      decoderState = polar_decoder_int16(&pdcch_vars[eNB_id]->e_rx[CCEind*9*6*2],
                                         dci_estimation,
                                         1,
                                         currentPtrDCI);
      crc = decoderState;
      //crc = (crc16(&dci_decoded_output[current_thread_id][0], sizeof_bits) >> 16) ^ extract_crc(&dci_decoded_output[current_thread_id][0], sizeof_bits);
      LOG_DDD("... we end function dci_decoding() with crc=%x\n",crc);
      LOG_DDD("... we have to replace this part of the code by polar decoding\n");
#ifdef DEBUG_DCI_DECODING
      LOG_DDD("(nr_dci_decoding_procedure0: crc =>%d\n",crc);
#endif //uint16_t tc_rnti, uint16_t int_rnti, uint16_t sfi_rnti, uint16_t tpc_pusch_rnti, uint16_t tpc_pucch_rnti, uint16_t tpc_srs__rnti
      LOG_DDD("format_found=%d\n",*format_found);
      LOG_DDD("crc_scrambled=%d\n",*crc_scrambled);

      if (crc == crc_scrambled_values[_C_RNTI_])  {
        *crc_scrambled =_c_rnti;
        *format_found=1;
      }

      if (crc == crc_scrambled_values[_CS_RNTI_])  {
        *crc_scrambled =_cs_rnti;
        *format_found=1;
      }

      if (crc == crc_scrambled_values[_NEW_RNTI_])  {
        *crc_scrambled =_new_rnti;
        *format_found=1;
      }

      if (crc == crc_scrambled_values[_TC_RNTI_])  {
        *crc_scrambled =_tc_rnti;
        *format_found=_format_1_0_found;
      }

      if (crc == crc_scrambled_values[_P_RNTI_])  {
        *crc_scrambled =_p_rnti;
        *format_found=_format_1_0_found;
      }

      if (crc == crc_scrambled_values[_SI_RNTI_])  {
        *crc_scrambled =_si_rnti;
        *format_found=_format_1_0_found;
      }

      if (crc == crc_scrambled_values[_RA_RNTI_])  {
        *crc_scrambled =_ra_rnti;
        *format_found=_format_1_0_found;
      }

      if (crc == crc_scrambled_values[_SP_CSI_RNTI_])  {
        *crc_scrambled =_sp_csi_rnti;
        *format_found=_format_0_1_found;
      }

      if (crc == crc_scrambled_values[_SFI_RNTI_])  {
        *crc_scrambled =_sfi_rnti;
        *format_found=_format_2_0_found;
      }

      if (crc == crc_scrambled_values[_INT_RNTI_])  {
        *crc_scrambled =_int_rnti;
        *format_found=_format_2_1_found;
      }

      if (crc == crc_scrambled_values[_TPC_PUSCH_RNTI_]) {
        *crc_scrambled =_tpc_pusch_rnti;
        *format_found=_format_2_2_found;
      }

      if (crc == crc_scrambled_values[_TPC_PUCCH_RNTI_]) {
        *crc_scrambled =_tpc_pucch_rnti;
        *format_found=_format_2_2_found;
      }

      if (crc == crc_scrambled_values[_TPC_SRS_RNTI_]) {
        *crc_scrambled =_tpc_srs_rnti;
        *format_found=_format_2_3_found;
      }


      LOG_DDD("format_found=%d\n",*format_found);
      LOG_DDD("crc_scrambled=%d\n",*crc_scrambled);

      if (*format_found!=255) {
        dci_alloc[*dci_cnt].dci_length = sizeof_bits;
        dci_alloc[*dci_cnt].rnti = crc;
        dci_alloc[*dci_cnt].L = L;
        dci_alloc[*dci_cnt].firstCCE = CCEind;
        memcpy(&dci_alloc[*dci_cnt].dci_pdu[0],dci_estimation,8);

        LOG_DDD("rnti matches -> DCI FOUND !!! crc =>0x%x, sizeof_bits %d, sizeof_bytes %d \n",
                dci_alloc[*dci_cnt].rnti, dci_alloc[*dci_cnt].dci_length, sizeof_bytes);
        LOG_DDD("dci_cnt %d (format_css %d crc_scrambled %d) L %d, firstCCE %d pdu[0] 0x%lx pdu[1] 0x%lx \n",
                *dci_cnt, format_css,*crc_scrambled,dci_alloc[*dci_cnt].L, dci_alloc[*dci_cnt].firstCCE,dci_alloc[*dci_cnt].dci_pdu[0],dci_alloc[*dci_cnt].dci_pdu[1]);
        if ((format_css == cformat0_0_and_1_0) || (format_uss == uformat0_0_and_1_0)) {
          if ((*crc_scrambled == _p_rnti) || (*crc_scrambled == _si_rnti) || (*crc_scrambled == _ra_rnti)) {
            dci_alloc[*dci_cnt].format = format1_0;
            *dci_cnt = *dci_cnt + 1;
            *format_found=_format_1_0_found;
            //      LOG_DDD("a format1_0=%d and dci_cnt=%d\n",*format_found,*dci_cnt);
          } else {
            if ((dci_estimation[0]&1) == 0) {
              dci_alloc[*dci_cnt].format = format0_0;
              *dci_cnt = *dci_cnt + 1;
              *format_found=_format_0_0_found;
              //        LOG_DDD("b format0_0=%d and dci_cnt=%d\n",*format_found,*dci_cnt);
            }

            if ((dci_estimation[0]&1) == 1) {
              dci_alloc[*dci_cnt].format = format1_0;
              *dci_cnt = *dci_cnt + 1;
              *format_found=_format_1_0_found;
              //        LOG_DDD("c format1_0=%d and dci_cnt=%d\n",*format_found,*dci_cnt);
            }
          }
        }

        if (format_css == cformat2_0) {
          dci_alloc[*dci_cnt].format = format2_0;
          *dci_cnt = *dci_cnt + 1;
          *format_found=_format_2_0_found;
        }

        if (format_css == cformat2_1) {
          dci_alloc[*dci_cnt].format = format2_1;
          *dci_cnt = *dci_cnt + 1;
          *format_found=_format_2_1_found;
        }

        if (format_css == cformat2_2) {
          dci_alloc[*dci_cnt].format = format2_2;
          *dci_cnt = *dci_cnt + 1;
          *format_found=_format_2_2_found;
        }

        if (format_css == cformat2_3) {
          dci_alloc[*dci_cnt].format = format2_3;
          *dci_cnt = *dci_cnt + 1;
          *format_found=_format_2_3_found;
        }

        if (format_uss == uformat0_1_and_1_1) {
          if ((dci_estimation[0]&1) == 0) {
            dci_alloc[*dci_cnt].format = format0_1;
            *dci_cnt = *dci_cnt + 1;
            *format_found=_format_0_1_found;
          }

          if ((dci_estimation[0]&1) == 1) {
            dci_alloc[*dci_cnt].format = format1_1;
            *dci_cnt = *dci_cnt + 1;
            *format_found=_format_1_1_found;
          }
        }

        // store first nCCE of group for PUCCH transmission of ACK/NAK
        pdcch_vars[eNB_id]->nCCE[nr_slot_rx] = CCEind;

        //        if (crc == si_rnti) {
        //        dci_alloc[*dci_cnt].format = format_si;
        //        *dci_cnt = *dci_cnt + 1;
        //        } else if (crc == p_rnti) {
        //        dci_alloc[*dci_cnt].format = format_p;
        //        *dci_cnt = *dci_cnt + 1;
        //        } else if (crc == ra_rnti) {
        //        dci_alloc[*dci_cnt].format = format_ra;
        //        // store first nCCE of group for PUCCH transmission of ACK/NAK
        //        pdcch_vars[eNB_id]->nCCE[nr_slot_rx] = CCEind;
        //        *dci_cnt = *dci_cnt + 1;
        //        } else if (crc == pdcch_vars[eNB_id]->crnti) {

        //        if ((mode & UL_DCI) && (format_c == format0)
        //        && ((dci_decoded_output[current_thread_id][0] & 0x80)
        //        == 0)) { // check if pdu is format 0 or 1A
        //        if (*format0_found == 0) {
        //        dci_alloc[*dci_cnt].format = format0;
        //        *format0_found = 1;
        //        *dci_cnt = *dci_cnt + 1;
        //        pdcch_vars[eNB_id]->nCCE[nr_slot_rx] = CCEind;
        //        }
        //        } else if (format_c == format0) { // this is a format 1A DCI
        //        dci_alloc[*dci_cnt].format = format1A;
        //        *dci_cnt = *dci_cnt + 1;
        //        pdcch_vars[eNB_id]->nCCE[nr_slot_rx] = CCEind;
        //        } else {
        //        // store first nCCE of group for PUCCH transmission of ACK/NAK
        //        if (*format_c_found == 0) {
        //        dci_alloc[*dci_cnt].format = format_c;
        //        *dci_cnt = *dci_cnt + 1;
        //        *format_c_found = 1;
        //        pdcch_vars[eNB_id]->nCCE[nr_slot_rx] = CCEind;
        //        }
        //        }
        //        }
        //LOG_I(PHY,"DCI decoding CRNTI  [format: %d, nCCE[nr_slot_rx: %d]: %d ], AggregationLevel %d \n",format_c, nr_slot_rx, pdcch_vars[eNB_id]->nCCE[nr_slot_rx],L2);
        //  memcpy(&dci_alloc[*dci_cnt].dci_pdu[0],dci_decoded_output,sizeof_bytes);
        switch (1 << L) {
          case 1:
            *CCEmap |= (1 << (CCEind & 0x1f));
            break;

          case 2:
            *CCEmap |= (1 << (CCEind & 0x1f));
            break;

          case 4:
            *CCEmap |= (1 << (CCEind & 0x1f));
            break;

          case 8:
            *CCEmap |= (1 << (CCEind & 0x1f));
            break;

          case 16:
            *CCEmap |= (1 << (CCEind & 0x1f));
            break;
        }

#ifdef DEBUG_DCI_DECODING
        LOG_I(PHY,"[DCI search] Found DCI %d rnti %x Aggregation %d length %d format %d in CCE %d (CCEmap %x) candidate %d / %d \n",
              *dci_cnt,crc,1<<L,sizeof_bits,dci_alloc[*dci_cnt-1].format,CCEind,*CCEmap,m,nb_candidates );
        //  nr_extract_dci_into(
        //  dump_dci(frame_parms,&dci_alloc[*dci_cnt-1]);
#endif
        return;
      } // rnti match
    } else { // CCEmap_cand == 0
      printf("\n");
    }

    
    //  if ( agregationLevel != 0xFF &&
    //  (format_c == format0 && m==0 && si_rnti != SI_RNTI))
    //  {
    //  //Only valid for OAI : Save some processing time when looking for DCI format0. From the log we see the DCI only on candidate 0.
    //  return;
    //  }
    
  } // candidate loop

  LOG_DDD("end candidate loop\n");
}
*/
#endif








#if 0


uint8_t nr_dci_decoding_procedure(int s,
                                  int p,
                                  PHY_VARS_NR_UE *ue,
                                  NR_DCI_ALLOC_t *dci_alloc,
                                  NR_SEARCHSPACE_TYPE_t searchSpacetype,
                                  int16_t eNB_id,
                                  uint8_t nr_slot_rx,
                                  uint8_t dci_fields_sizes_cnt[MAX_NR_DCI_DECODED_SLOT][NBR_NR_DCI_FIELDS][NBR_NR_FORMATS],
                                  uint16_t n_RB_ULBWP,
                                  uint16_t n_RB_DLBWP,
                                  crc_scrambled_t *crc_scrambled,
                                  format_found_t *format_found,
                                  uint16_t crc_scrambled_values[TOTAL_NBR_SCRAMBLED_VALUES]) {
  //                                  uint8_t dci_fields_sizes[NBR_NR_DCI_FIELDS][NBR_NR_FORMATS],
  LOG_DD("(nr_dci_decoding_procedure) nr_slot_rx=%d n_RB_ULBWP=%d n_RB_DLBWP=%d format_found=%d\n",
         nr_slot_rx,n_RB_ULBWP,n_RB_DLBWP,*format_found);
  int do_common = (int)searchSpacetype;
  uint8_t dci_fields_sizes[NBR_NR_DCI_FIELDS][NBR_NR_FORMATS];
  crc_scrambled_t crc_scrambled_ = *crc_scrambled;
  format_found_t format_found_   = *format_found;
  uint8_t dci_cnt = 0, old_dci_cnt = 0;
  uint32_t CCEmap0 = 0, CCEmap1 = 0, CCEmap2 = 0;
  NR_UE_PDCCH **pdcch_vars = ue->pdcch_vars[ue->current_thread_id[nr_slot_rx]];
  NR_UE_PDCCH *pdcch_vars2 = ue->pdcch_vars[ue->current_thread_id[nr_slot_rx]][eNB_id];
  uint16_t pdcch_DMRS_scrambling_id = pdcch_vars2->coreset[p].pdcchDMRSScramblingID;
  uint64_t coreset_freq_dom = pdcch_vars2->coreset[p].frequencyDomainResources;
  int coreset_time_dur = pdcch_vars2->coreset[p].duration;
  uint16_t coreset_nbr_rb=0;

  for (int i = 0; i < 45; i++) {
    // this loop counts each bit of the bit map coreset_freq_dom, and increments nbr_RB_coreset for each bit set to '1'
    if (((coreset_freq_dom & 0x1FFFFFFFFFFF) >> i) & 0x1) coreset_nbr_rb++;
  }

  coreset_nbr_rb = 6 * coreset_nbr_rb;
  // coreset_time_dur,coreset_nbr_rb,
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  //uint8_t mi;// = get_mi(&ue->frame_parms, nr_slot_rx);
  //uint8_t tmode = ue->transmission_mode[eNB_id];
  //uint8_t frame_type = frame_parms->frame_type;
  uint8_t format_0_0_1_0_size_bits = 0, format_0_0_1_0_size_bytes = 0; //FIXME
  uint8_t format_0_1_1_1_size_bits = 0, format_0_1_1_1_size_bytes = 0; //FIXME
  uint8_t format_2_0_size_bits = 0, format_2_0_size_bytes = 0; //FIXME
  uint8_t format_2_1_size_bits = 0, format_2_1_size_bytes = 0; //FIXME
  uint8_t format_2_2_size_bits = 0, format_2_2_size_bytes = 0; //FIXME
  uint8_t format_2_3_size_bits = 0, format_2_3_size_bytes = 0; //FIXME
  /*
   *
   * The implementation of this function will depend on the information given by the searchSpace IE
   *
   * In LTE the UE has no knowledge about:
   * - the type of search (common or ue-specific)
   * - the DCI format it is going to be decoded when performing the PDCCH monitoring
   * So the blind decoding has to be done for common and ue-specific searchSpaces for each aggregation level and for each dci format
   *
   * In NR the UE has a knowledge about the search Space type and the DCI format it is going to be decoded,
   * so in the blind decoding we can call the function nr_dci_decoding_procedure0 with the searchSpace type and the dci format parameter
   * We will call this function as many times as aggregation levels indicated in searchSpace
   * Implementation according to 38.213 v15.1.0 Section 10.
   *
   */
  NR_UE_SEARCHSPACE_CSS_DCI_FORMAT_t css_dci_format = pdcch_vars2->searchSpace[s].searchSpaceType.common_dci_formats;       //FIXME!!!
  NR_UE_SEARCHSPACE_USS_DCI_FORMAT_t uss_dci_format = pdcch_vars2->searchSpace[s].searchSpaceType.ue_specific_dci_formats;  //FIXME!!!
  // The following initialization is only for test purposes. To be removed
  // NR_UE_SEARCHSPACE_CSS_DCI_FORMAT_t
  css_dci_format = cformat0_0_and_1_0;
  //NR_UE_SEARCHSPACE_USS_DCI_FORMAT_t
  uss_dci_format = uformat0_0_and_1_0;
  /*
   * Possible overlap between RE for SS/PBCH blocks (described in section 10, 38.213) has not been implemented yet
   * This can be implemented by setting variable 'mode = NO_DCI' when overlap occurs
   */
  //dci_detect_mode_t mode = 3; //dci_detect_mode_select(&ue->frame_parms, nr_slot_rx);
  LOG_DD("searSpaceType=%d\n",do_common);
  LOG_DD("%s_dci_format=%d\n",do_common?"uss":"css",css_dci_format);


  // A set of PDCCH candidates for a UE to monitor is defined in terms of PDCCH search spaces
  if (do_common==0) { // COMMON SearchSpaceType assigned to current SearchSpace/CORESET
    // Type0-PDCCH  common search space for a DCI format with CRC scrambled by a SI-RNTI
    // number of consecutive resource blocks and a number of consecutive symbols for
    // the control resource set of the Type0-PDCCH common search space from
    // the four most significant bits of RMSI-PDCCH-Config as described in Tables 13-1 through 13-10
    // and determines PDCCH monitoring occasions
    // from the four least significant bits of RMSI-PDCCH-Config,
    // included in MasterInformationBlock, as described in Tables 13-11 through 13-15
    // Type0A-PDCCH common search space for a DCI format with CRC scrambled by a SI-RNTI
    // Type1-PDCCH  common search space for a DCI format with CRC scrambled by a RA-RNTI, or a TC-RNTI, or a C-RNTI
    // Type2-PDCCH  common search space for a DCI format with CRC scrambled by a P-RNTI
    if (css_dci_format == cformat0_0_and_1_0) {
      // 38.213 v15.1.0 Table 10.1-1: CCE aggregation levels and maximum number of PDCCH candidates per CCE
      // aggregation level for Type0/Type0A/Type2-PDCCH common search space
      //   CCE Aggregation Level    Number of Candidates
      //           4                       4
      //           8                       2
      //           16                      1
      // FIXME
      // We shall consider Table 10.1-1 to calculate the blind decoding only for Type0/Type0A/Type2-PDCCH
      // Shall we consider the nrofCandidates in SearSpace IE that considers Aggregation Levels 1,2,4,8,16? Our implementation considers Table 10.1-1
      // blind decoding (Type0-PDCCH,Type0A-PDCCH,Type1-PDCCH,Type2-PDCCH)
      // for format0_0 => we are NOT implementing format0_0 for common search spaces. FIXME!
      // for format0_0 and format1_0, first we calculate dci pdu size
      format_0_0_1_0_size_bits = nr_dci_format_size(ue,eNB_id,nr_slot_rx,p,_c_rnti,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes,0);
      format_0_0_1_0_size_bytes = (format_0_0_1_0_size_bits%8 == 0) ? (uint8_t)floor(format_0_0_1_0_size_bits/8) : (uint8_t)(floor(format_0_0_1_0_size_bits/8) + 1);
      LOG_DD("calculating dci format size for common searchSpaces with format css_dci_format=%d, format_0_0_1_0_size_bits=%d, format_0_0_1_0_size_bytes=%d\n",
             css_dci_format,format_0_0_1_0_size_bits,format_0_0_1_0_size_bytes);

      for (int aggregationLevel = 0; aggregationLevel<5 ; aggregationLevel++) { // We fix aggregationLevel to 3 for testing=> nbr of CCE=8
        //for (int aggregationLevel = 2; aggregationLevel<5 ; aggregationLevel++) {
        // for aggregation level aggregationLevel. The number of candidates (for L2= 2^aggregationLevel) will be calculated in function nr_dci_decoding_procedure0
        LOG_DD("common searchSpaces with format css_dci_format=%d and aggregation_level=%d\n",
               css_dci_format,(1<<aggregationLevel));
        old_dci_cnt = dci_cnt;
        nr_dci_decoding_procedure0(s,p,coreset_time_dur,coreset_nbr_rb,pdcch_vars, 1, nr_slot_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_slot_rx], frame_parms,
                                   crc_scrambled_values, aggregationLevel,
                                   cformat0_0_and_1_0, uformat0_0_and_1_0,
                                   format_0_0_1_0_size_bits, format_0_0_1_0_size_bytes, &dci_cnt,
                                   &crc_scrambled_, &format_found_, pdcch_DMRS_scrambling_id,&CCEmap0, &CCEmap1, &CCEmap2);

        if (dci_cnt != old_dci_cnt) {
          // we will exit the loop as we have found the DCI
          aggregationLevel = 5;
          format_0_0_1_0_size_bits = nr_dci_format_size(ue,eNB_id,nr_slot_rx,p,crc_scrambled_,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes,
                                     0); // after decoding dci successfully we recalculate dci pdu size with correct crc scrambled to get the right field sizes
          old_dci_cnt = dci_cnt;

          for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
            for (int j=0; j<NBR_NR_FORMATS; j++)
              dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
        }
      }
    }

    // Type3-PDCCH  common search space for a DCI format with CRC scrambled by INT-RNTI, or SFI-RNTI,
    //    or TPC-PUSCH-RNTI, or TPC-PUCCH-RNTI, or TPC-SRS-RNTI, or C-RNTI, or CS-RNTI(s), or SP-CSI-RNTI
    if (css_dci_format == cformat2_0) {
      // for format2_0, first we calculate dci pdu size
      format_2_0_size_bits = nr_dci_format_size(ue,eNB_id,nr_slot_rx,p,_sfi_rnti,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes,4);
      format_2_0_size_bytes = (format_2_0_size_bits%8 == 0) ? (uint8_t)floor(format_2_0_size_bits/8) : (uint8_t)(floor(format_2_0_size_bits/8) + 1);
      LOG_DD("calculating dci format size for common searchSpaces with format css_dci_format=%d, format2_0_size_bits=%d, format2_0_size_bytes=%d\n",
             css_dci_format,format_2_0_size_bits,format_2_0_size_bytes);

      for (int aggregationLevelSFI = 0; aggregationLevelSFI<5 ; aggregationLevelSFI++) {
        LOG_DD("common searchSpaces with format css_dci_format=%d and aggregation_level=%d\n",
               css_dci_format,(1<<aggregationLevelSFI));
        // for aggregation level 'aggregationLevelSFI'. The number of candidates (nrofCandidates-SFI) will be calculated in function nr_dci_decoding_procedure0
        old_dci_cnt = dci_cnt;
        nr_dci_decoding_procedure0(s,p,coreset_time_dur,coreset_nbr_rb,pdcch_vars, 1, nr_slot_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_slot_rx], frame_parms,
                                   crc_scrambled_values, aggregationLevelSFI,
                                   cformat2_0, uformat0_0_and_1_0,
                                   format_2_0_size_bits, format_2_0_size_bytes, &dci_cnt,
                                   &crc_scrambled_, &format_found_,pdcch_DMRS_scrambling_id, &CCEmap0, &CCEmap1, &CCEmap2);

        if (dci_cnt != old_dci_cnt) {
          // we will exit the loop as we have found the DCI
          aggregationLevelSFI = 5;
          old_dci_cnt = dci_cnt;

          for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
            for (int j=0; j<NBR_NR_FORMATS; j++)
              dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
        }
      }
    }

    if (css_dci_format == cformat2_1) {
      // for format2_1, first we calculate dci pdu size
      format_2_1_size_bits = nr_dci_format_size(ue,eNB_id,nr_slot_rx,p,_int_rnti,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes,5);
      format_2_1_size_bytes = (format_2_1_size_bits%8 == 0) ? (uint8_t)floor(format_2_1_size_bits/8) : (uint8_t)(floor(format_2_1_size_bits/8) + 1);
      LOG_DD("calculating dci format size for common searchSpaces with format css_dci_format=%d, format2_1_size_bits=%d, format2_1_size_bytes=%d\n",
             css_dci_format,format_2_1_size_bits,format_2_1_size_bytes);

      for (int aggregationLevel = 0; aggregationLevel<5 ; aggregationLevel++) {
        LOG_DD("common searchSpaces with format css_dci_format=%d and aggregation_level=%d\n",
               css_dci_format,(1<<aggregationLevel));
        // for aggregation level 'aggregationLevelSFI'. The number of candidates (nrofCandidates-SFI) will be calculated in function nr_dci_decoding_procedure0
        old_dci_cnt = dci_cnt;
        nr_dci_decoding_procedure0(s,p,coreset_time_dur,coreset_nbr_rb,pdcch_vars, 1, nr_slot_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_slot_rx], frame_parms,
                                   crc_scrambled_values, aggregationLevel,
                                   cformat2_1, uformat0_0_and_1_0,
                                   format_2_1_size_bits, format_2_1_size_bytes, &dci_cnt,
                                   &crc_scrambled_, &format_found_,pdcch_DMRS_scrambling_id, &CCEmap0, &CCEmap1, &CCEmap2);

        if (dci_cnt != old_dci_cnt) {
          // we will exit the loop as we have found the DCI
          aggregationLevel = 5;
          old_dci_cnt = dci_cnt;

          for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
            for (int j=0; j<NBR_NR_FORMATS; j++)
              dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
        }
      }
    }

    if (css_dci_format == cformat2_2) {
      // for format2_2, first we calculate dci pdu size
      format_2_2_size_bits = nr_dci_format_size(ue,eNB_id,nr_slot_rx,p,_tpc_pucch_rnti,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes,6);
      format_2_2_size_bytes = (format_2_2_size_bits%8 == 0) ? (uint8_t)floor(format_2_2_size_bits/8) : (uint8_t)(floor(format_2_2_size_bits/8) + 1);
      LOG_DD("calculating dci format size for common searchSpaces with format css_dci_format=%d, format2_2_size_bits=%d, format2_2_size_bytes=%d\n",
             css_dci_format,format_2_2_size_bits,format_2_2_size_bytes);

      for (int aggregationLevel = 0; aggregationLevel<5 ; aggregationLevel++) {
        LOG_DD("common searchSpaces with format css_dci_format=%d and aggregation_level=%d\n",
               css_dci_format,(1<<aggregationLevel));
        // for aggregation level 'aggregationLevelSFI'. The number of candidates (nrofCandidates-SFI) will be calculated in function nr_dci_decoding_procedure0
        old_dci_cnt = dci_cnt;
        nr_dci_decoding_procedure0(s,p,coreset_time_dur,coreset_nbr_rb,pdcch_vars, 1, nr_slot_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_slot_rx], frame_parms,
                                   crc_scrambled_values, aggregationLevel,
                                   cformat2_2, uformat0_0_and_1_0,
                                   format_2_2_size_bits, format_2_2_size_bytes, &dci_cnt,
                                   &crc_scrambled_, &format_found_,pdcch_DMRS_scrambling_id, &CCEmap0, &CCEmap1, &CCEmap2);

        if (dci_cnt != old_dci_cnt) {
          // we will exit the loop as we have found the DCI
          aggregationLevel = 5;
          old_dci_cnt = dci_cnt;

          for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
            for (int j=0; j<NBR_NR_FORMATS; j++)
              dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
        }
      }
    }

    if (css_dci_format == cformat2_3) {
      // for format2_1, first we calculate dci pdu size
      format_2_3_size_bits = nr_dci_format_size(ue,eNB_id,nr_slot_rx,p,_tpc_srs_rnti,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes,7);
      format_2_3_size_bytes = (format_2_3_size_bits%8 == 0) ? (uint8_t)floor(format_2_3_size_bits/8) : (uint8_t)(floor(format_2_3_size_bits/8) + 1);
      LOG_DD("calculating dci format size for common searchSpaces with format css_dci_format=%d, format2_3_size_bits=%d, format2_3_size_bytes=%d\n",
             css_dci_format,format_2_3_size_bits,format_2_3_size_bytes);

      for (int aggregationLevel = 0; aggregationLevel<5 ; aggregationLevel++) {
        LOG_DD("common searchSpaces with format css_dci_format=%d and aggregation_level=%d\n",
               css_dci_format,(1<<aggregationLevel));
        // for aggregation level 'aggregationLevelSFI'. The number of candidates (nrofCandidates-SFI) will be calculated in function nr_dci_decoding_procedure0
        old_dci_cnt = dci_cnt;
        nr_dci_decoding_procedure0(s,p,coreset_time_dur,coreset_nbr_rb,pdcch_vars, 1, nr_slot_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_slot_rx], frame_parms,
                                   crc_scrambled_values, aggregationLevel,
                                   cformat2_3, uformat0_0_and_1_0,
                                   format_2_3_size_bits, format_2_3_size_bytes, &dci_cnt,
                                   &crc_scrambled_, &format_found_,pdcch_DMRS_scrambling_id, &CCEmap0, &CCEmap1, &CCEmap2);

        if (dci_cnt != old_dci_cnt) {
          // we will exit the loop as we have found the DCI
          aggregationLevel = 5;
          old_dci_cnt = dci_cnt;

          for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
            for (int j=0; j<NBR_NR_FORMATS; j++)
              dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
        }
      }
    }
  } else { // UE-SPECIFIC SearchSpaceType assigned to current SearchSpace/CORESET
    // UE-specific search space for a DCI format with CRC scrambled by C-RNTI, or CS-RNTI(s), or SP-CSI-RNTI
    if (uss_dci_format == uformat0_0_and_1_0) {
      // for format0_0 and format1_0, first we calculate dci pdu size
      format_0_0_1_0_size_bits = nr_dci_format_size(ue,eNB_id,nr_slot_rx,p,_c_rnti,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes,0);
      format_0_0_1_0_size_bytes = (format_0_0_1_0_size_bits%8 == 0) ? (uint8_t)floor(format_0_0_1_0_size_bits/8) : (uint8_t)(floor(format_0_0_1_0_size_bits/8) + 1);
      LOG_DD("calculating dci format size for UE-specific searchSpaces with format uss_dci_format=%d, format_0_0_1_0_size_bits=%d, format_0_0_1_0_size_bytes=%d\n",
             css_dci_format,format_0_0_1_0_size_bits,format_0_0_1_0_size_bytes);

      for (int aggregationLevel = 0; aggregationLevel<5 ; aggregationLevel++) { // We fix aggregationLevel to 3 for testing=> nbr of CCE=8
        //for (int aggregationLevel = 2; aggregationLevel<5 ; aggregationLevel++) {
        // for aggregation level aggregationLevel. The number of candidates (for L2= 2^aggregationLevel) will be calculated in function nr_dci_decoding_procedure0
        LOG_DD("common searchSpaces with format css_dci_format=%d and aggregation_level=%d\n",
               css_dci_format,(1<<aggregationLevel));
        old_dci_cnt = dci_cnt;
        nr_dci_decoding_procedure0(s,p,coreset_time_dur,coreset_nbr_rb,pdcch_vars, 0, nr_slot_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_slot_rx], frame_parms,
                                   crc_scrambled_values, aggregationLevel,
                                   cformat0_0_and_1_0, uformat0_0_and_1_0,
                                   format_0_0_1_0_size_bits, format_0_0_1_0_size_bytes, &dci_cnt,
                                   &crc_scrambled_, &format_found_,pdcch_DMRS_scrambling_id, &CCEmap0, &CCEmap1, &CCEmap2);

        if (dci_cnt != old_dci_cnt) {
          // we will exit the loop as we have found the DCI
          aggregationLevel = 5;
          old_dci_cnt = dci_cnt;

          for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
            for (int j=0; j<NBR_NR_FORMATS; j++)
              dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
        }
      }
    }

    if (uss_dci_format == uformat0_1_and_1_1) {
      // for format0_0 and format1_0, first we calculate dci pdu size
      format_0_1_1_1_size_bits = nr_dci_format_size(ue,eNB_id,nr_slot_rx,p,_c_rnti,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes,1);
      format_0_1_1_1_size_bytes = (format_0_1_1_1_size_bits%8 == 0) ? (uint8_t)floor(format_0_1_1_1_size_bits/8) : (uint8_t)(floor(format_0_1_1_1_size_bits/8) + 1);
      LOG_DD("calculating dci format size for UE-specific searchSpaces with format uss_dci_format=%d, format_0_1_1_1_size_bits=%d, format_0_1_1_1_size_bytes=%d\n",
             css_dci_format,format_0_1_1_1_size_bits,format_0_1_1_1_size_bytes);

      for (int aggregationLevel = 0; aggregationLevel<5 ; aggregationLevel++) { // We fix aggregationLevel to 3 for testing=> nbr of CCE=8
        //for (int aggregationLevel = 2; aggregationLevel<5 ; aggregationLevel++) {
        // for aggregation level aggregationLevel. The number of candidates (for L2= 2^aggregationLevel) will be calculated in function nr_dci_decoding_procedure0
        LOG_DD("common searchSpaces with format css_dci_format=%d and aggregation_level=%d\n",
               css_dci_format,(1<<aggregationLevel));
        old_dci_cnt = dci_cnt;
        nr_dci_decoding_procedure0(s,p,coreset_time_dur,coreset_nbr_rb,pdcch_vars, 0, nr_slot_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_slot_rx], frame_parms,
                                   crc_scrambled_values, aggregationLevel,
                                   cformat0_0_and_1_0, uformat0_1_and_1_1,
                                   format_0_1_1_1_size_bits, format_0_1_1_1_size_bytes, &dci_cnt,
                                   &crc_scrambled_, &format_found_,pdcch_DMRS_scrambling_id, &CCEmap0, &CCEmap1, &CCEmap2);

        if (dci_cnt != old_dci_cnt) {
          // we will exit the loop as we have found the DCI
          aggregationLevel = 5;
          old_dci_cnt = dci_cnt;

          for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
            for (int j=0; j<NBR_NR_FORMATS; j++)
              dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
        }
      }
    }
  }

  *crc_scrambled = crc_scrambled_;
  *format_found  = format_found_;
  LOG_DD("at the end crc_scrambled=%d and format_found=%d\n",*crc_scrambled,*format_found);
  LOG_DD("at the end dci_cnt=%d \n",dci_cnt);
  return(dci_cnt);
}

#endif
