/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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


#include <string.h>

#include "nr_ul_estimation.h"
#include "PHY/sse_intrin.h"
#include "PHY/NR_REFSIG/nr_refsig.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_REFSIG/ptrs_nr.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_UE_ESTIMATION/filt16a_32.h"

#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"
#include "executables/softmodem-common.h"


//#define DEBUG_CH
//#define DEBUG_PUSCH

#define NO_INTERP 1
#define dBc(x,y) (dB_fixed(((int32_t)(x))*(x) + ((int32_t)(y))*(y)))

int nr_pusch_channel_estimation(PHY_VARS_gNB *gNB,
                                unsigned char Ns,
                                unsigned short p,
                                unsigned char symbol,
                                int ul_id,
                                unsigned short bwp_start_subcarrier,
                                nfapi_nr_pusch_pdu_t *pusch_pdu) {

  int pilot[3280] __attribute__((aligned(16)));
  unsigned char aarx;
  unsigned short k;
  unsigned int pilot_cnt,re_cnt;
  int16_t ch[2],ch_r[2],ch_l[2],*pil,*rxF,*ul_ch;
  int16_t *fl,*fm,*fr,*fml,*fmr,*fmm,*fdcl,*fdcr,*fdclh,*fdcrh;
  int ch_offset,symbol_offset ;
  int32_t **ul_ch_estimates_time =  gNB->pusch_vars[ul_id]->ul_ch_estimates_time;
  __m128i *ul_ch_128;

#ifdef DEBUG_CH
  FILE *debug_ch_est;
  debug_ch_est = fopen("debug_ch_est.txt","w");
#endif

  //uint16_t Nid_cell = (eNB_offset == 0) ? gNB->frame_parms.Nid_cell : gNB->measurements.adj_cell_id[eNB_offset-1];

  uint8_t nushift;
  int **ul_ch_estimates  = gNB->pusch_vars[ul_id]->ul_ch_estimates;
  int **rxdataF = gNB->common_vars.rxdataF;
  int soffset = (Ns&3)*gNB->frame_parms.symbols_per_slot*gNB->frame_parms.ofdm_symbol_size;
  nushift = (p>>1)&1;
  gNB->frame_parms.nushift = nushift;

  ch_offset     = gNB->frame_parms.ofdm_symbol_size*symbol;

  symbol_offset = gNB->frame_parms.ofdm_symbol_size*symbol;

  k = bwp_start_subcarrier;
  int re_offset;

  uint16_t nb_rb_pusch = pusch_pdu->rb_size;

  LOG_D(PHY, "In %s: ch_offset %d, soffset %d, symbol_offset %d OFDM size %d, Ns = %d, k = %d symbol %d\n",
        __FUNCTION__,
        ch_offset, soffset,
        symbol_offset,
        gNB->frame_parms.ofdm_symbol_size,
        Ns,
        k,
        symbol);

  switch (nushift) {
   case 0:
         fl = filt8_l0;
         fm = filt8_m0;
         fr = filt8_r0;
         fmm = filt8_mm0;
         fml = filt8_m0;
         fmr = filt8_mr0;
         fdcl = filt8_dcl0;
         fdcr = filt8_dcr0;
         fdclh = filt8_dcl0_h;
         fdcrh = filt8_dcr0_h;
         break;

   case 1:
         fl = filt8_l1;
         fm = filt8_m1;
         fr = filt8_r1;
         fmm = filt8_mm1;
         fml = filt8_ml1;
         fmr = filt8_m1;
         fdcl = filt8_dcl1;
         fdcr = filt8_dcr1;
         fdclh = filt8_dcl1_h;
         fdcrh = filt8_dcr1_h;
         break;

   default:
#ifdef DEBUG_CH
      if (debug_ch_est)
        fclose(debug_ch_est);

#endif
     return(-1);
     break;
   }

  //------------------generate DMRS------------------//

  // transform precoding = 1 means disabled
  if (pusch_pdu->transform_precoding == 1) {
    nr_pusch_dmrs_rx(gNB, Ns, gNB->nr_gold_pusch_dmrs[pusch_pdu->scid][Ns][symbol], &pilot[0], 1000, 0, nb_rb_pusch,
                     (pusch_pdu->bwp_start + pusch_pdu->rb_start)*NR_NB_SC_PER_RB, pusch_pdu->dmrs_config_type);
  }
  else {  // if transform precoding or SC-FDMA is enabled in Uplink

    // NR_SC_FDMA supports type1 DMRS so only 6 DMRS REs per RB possible
    uint16_t index = get_index_for_dmrs_lowpapr_seq(nb_rb_pusch * (NR_NB_SC_PER_RB/2));
    uint8_t u = pusch_pdu->dfts_ofdm.low_papr_group_number; 
    uint8_t v = pusch_pdu->dfts_ofdm.low_papr_sequence_number;
    int16_t *dmrs_seq = gNB_dmrs_lowpaprtype1_sequence[u][v][index];

    AssertFatal(index >= 0, "Num RBs not configured according to 3GPP 38.211 section 6.3.1.4. For PUSCH with transform precoding, num RBs cannot be multiple of any other primenumber other than 2,3,5\n");
    AssertFatal(dmrs_seq != NULL, "DMRS low PAPR seq not found, check if DMRS sequences are generated");

    LOG_D(PHY,"Transform Precoding params. u: %d, v: %d, index for dmrsseq: %d\n", u, v, index);
    
    nr_pusch_lowpaprtype1_dmrs_rx(gNB, Ns, dmrs_seq, &pilot[0], 1000, 0, nb_rb_pusch, 0, pusch_pdu->dmrs_config_type);    

    #ifdef DEBUG_PUSCH
      printf ("NR_UL_CHANNEL_EST: index %d, u %d,v %d\n", index, u, v);
      LOG_M("gNb_DMRS_SEQ.m","gNb_DMRS_SEQ", dmrs_seq,6*nb_rb_pusch,1,1);
    #endif

  }
  //------------------------------------------------//


#ifdef DEBUG_PUSCH
  for (int i = 0; i < (6 * nb_rb_pusch); i++) {
    LOG_I(PHY, "In %s: %d + j*(%d)\n",
      __FUNCTION__,
      ((int16_t*)pilot)[2 * i],
      ((int16_t*)pilot)[1 + (2 * i)]);
  }
#endif

  for (aarx=0; aarx<gNB->frame_parms.nb_antennas_rx; aarx++) {

    re_offset = k;   /* Initializing the Resource element offset for each Rx antenna */

    pil   = (int16_t *)&pilot[0];
    rxF   = (int16_t *)&rxdataF[aarx][(soffset+symbol_offset+k+nushift)];
    ul_ch = (int16_t *)&ul_ch_estimates[p*gNB->frame_parms.nb_antennas_rx+aarx][ch_offset];
    re_offset = k;

    memset(ul_ch,0,4*(gNB->frame_parms.ofdm_symbol_size));

#ifdef DEBUG_PUSCH
    LOG_I(PHY, "In %s symbol_offset %d, nushift %d\n", __FUNCTION__, symbol_offset, nushift);
    LOG_I(PHY, "In %s ch est pilot addr %p, N_RB_UL %d\n", __FUNCTION__, &pilot[0], gNB->frame_parms.N_RB_UL);
    LOG_I(PHY, "In %s bwp_start_subcarrier %d, k %d, first_carrier %d, nb_rb_pusch %d\n", __FUNCTION__, bwp_start_subcarrier, k, gNB->frame_parms.first_carrier_offset, nb_rb_pusch);
    LOG_I(PHY, "In %s rxF addr %p p %d\n", __FUNCTION__, rxF, p);
    LOG_I(PHY, "In %s ul_ch addr %p nushift %d\n", __FUNCTION__, ul_ch, nushift);
#endif
    //if ((gNB->frame_parms.N_RB_UL&1)==0) {

    if (pusch_pdu->dmrs_config_type == pusch_dmrs_type1 && gNB->prb_interpolation == 0){
      LOG_D(PHY,"PUSCH estimation DMRS type 1, Freq-domain interpolation");
      // Treat first 2 pilots specially (left edge)
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PUSCH
      LOG_I(PHY, "In %s ch 0 %d\n", __FUNCTION__, ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1]));
      LOG_I(PHY, "In %s pilot 0 : rxF - > (%d,%d) (%d)  ch -> (%d,%d) (%d), pil -> (%d,%d) \n",
        __FUNCTION__,
        rxF[0],
        rxF[1],
        dBc(rxF[0],rxF[1]),
        ch[0],
        ch[1],
        dBc(ch[0],ch[1]),
        pil[0],
        pil[1]);
      LOG_I(PHY, "In %s data 0 : rxF - > (%d,%d) (%d)\n", __FUNCTION__, rxF[2], rxF[3], dBc(rxF[2],rxF[3]));
#endif

      multadd_real_vector_complex_scalar(fl,
                                         ch,
                                         ul_ch,
                                         8);
      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(soffset+symbol_offset+nushift+re_offset)];
      //for (int i= 0; i<8; i++)
      //printf("ul_ch addr %p %d\n", ul_ch+i, *(ul_ch+i));

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PUSCH
      LOG_I(PHY, "In %s pilot 1 : rxF - > (%d,%d) (%d) ch -> (%d,%d) (%d), pil -> (%d,%d) \n",
        __FUNCTION__,
        rxF[0],
        rxF[1],
        dBc(rxF[0],rxF[1]),
        ch[0],
        ch[1],
        dBc(ch[0],ch[1]),
        pil[0],
        pil[1]);
      LOG_I(PHY, "In %s data 1 : rxF - > (%d,%d) (%d)\n",
        __FUNCTION__,
        rxF[2],
        rxF[3],
        dBc(rxF[2],rxF[3]));
#endif

      multadd_real_vector_complex_scalar(fml,
                                         ch,
                                         ul_ch,
                                         8);
      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(soffset+symbol_offset+nushift+re_offset)];
      //printf("ul_ch addr %p\n",ul_ch);
      
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PUSCH
      LOG_I(PHY, "In %s pilot 2 : rxF - > (%d,%d) (%d) ch -> (%d,%d) (%d), pil -> (%d,%d) \n",
        __FUNCTION__,
        rxF[0],
        rxF[1],
        dBc(rxF[0],rxF[1]),
        ch[0],
        ch[1],
        dBc(ch[0],ch[1]),
        pil[0],
        pil[1]);
      LOG_I(PHY, "In %s data 2 : rxF - > (%d,%d) (%d)\n",
        __FUNCTION__,
        rxF[2],
        rxF[3],
        dBc(rxF[2],rxF[3]));
#endif

      multadd_real_vector_complex_scalar(fmm,
                                         ch,
                                         ul_ch,
                                         8);
                                         
      //for (int i= 0; i<16; i++)
      //printf("ul_ch addr %p %d\n", ul_ch+i, *(ul_ch+i));
      
      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(soffset+symbol_offset+nushift+re_offset)];
      ul_ch+=8;

      for (pilot_cnt=3; pilot_cnt<(6*nb_rb_pusch-3); pilot_cnt += 2) {

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

  #ifdef DEBUG_PUSCH
        printf("pilot %u : rxF - > (%d,%d) (%d) ch -> (%d,%d) (%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],dBc(rxF[0],rxF[1]),ch[0],ch[1],dBc(ch[0],ch[1]),pil[0],pil[1]);
	printf("data %u : rxF - > (%d,%d) (%d)\n",pilot_cnt,rxF[2],rxF[3],dBc(rxF[2],rxF[3]));
  #endif
        multadd_real_vector_complex_scalar(fml,
                                           ch,
                                           ul_ch,
                                           8);
        pil += 2;
        re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(soffset+symbol_offset+nushift+re_offset)];
        //printf("ul_ch addr %p\n",ul_ch);

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

  #ifdef DEBUG_PUSCH
        printf("pilot %u : rxF - > (%d,%d) (%d) ch -> (%d,%d) (%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],dBc(rxF[0],rxF[1]),ch[0],ch[1],dBc(ch[0],ch[1]),pil[0],pil[1]);
	printf("data %u : rxF - > (%d,%d) (%d)\n",pilot_cnt+1,rxF[2],rxF[3],dBc(rxF[2],rxF[3]));
  #endif
        multadd_real_vector_complex_scalar(fmm,
                                           ch,
                                           ul_ch,
                                           8);

        //for (int i= 0; i<16; i++)
        //printf("ul_ch addr %p %d\n", ul_ch+i, *(ul_ch+i));

        pil += 2;
        re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][soffset+(symbol_offset+nushift+re_offset)];
        ul_ch+=8;

      }
      
      // Treat first 2 pilots specially (right edge)
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PUSCH
      printf("pilot %u : rxF - > (%d,%d) (%d) ch -> (%d,%d) (%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],dBc(rxF[0],rxF[1]),ch[0],ch[1],dBc(ch[0],ch[1]),pil[0],pil[1]);
      printf("data %u : rxF - > (%d,%d) (%d)\n",pilot_cnt,rxF[2],rxF[3],dBc(rxF[2],rxF[3]));
#endif
      multadd_real_vector_complex_scalar(fm,
                                         ch,
                                         ul_ch,
                                         8);
                                         
      //for (int i= 0; i<8; i++)
      //printf("ul_ch addr %p %d\n", ul_ch+i, *(ul_ch+i));

      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][soffset+(symbol_offset+nushift+re_offset)];
             
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PUSCH
      printf("ch 0 %d\n",((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1]));
      printf("pilot %u : rxF - > (%d,%d) (%d) ch -> (%d,%d) (%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],dBc(rxF[0],rxF[1]),ch[0],ch[1],dBc(ch[0],ch[1]),pil[0],pil[1]);
      printf("data %u : rxF - > (%d,%d) (%d)\n",pilot_cnt+1,rxF[2],rxF[3],dBc(rxF[2],rxF[3]));
#endif
      multadd_real_vector_complex_scalar(fmr,
                                         ch,
                                         ul_ch,
                                         8);
                                         
      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][soffset+(symbol_offset+nushift+re_offset)];
      ul_ch+=8;
      
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PUSCH
      printf("pilot %u: rxF - > (%d,%d) (%d) ch -> (%d,%d) (%d), pil -> (%d,%d) \n",pilot_cnt+2,rxF[0],rxF[1],dBc(rxF[0],rxF[1]),ch[0],ch[1],dBc(ch[0],ch[1]),pil[0],pil[1]);
      printf("data %u : rxF - > (%d,%d) (%d)\n",pilot_cnt+2,rxF[2],rxF[3],dBc(rxF[2],rxF[3]));
#endif
      multadd_real_vector_complex_scalar(fr,
                                         ch,
                                         ul_ch,
                                         8);


      // check if PRB crosses DC and improve estimates around DC
      if ((bwp_start_subcarrier < gNB->frame_parms.ofdm_symbol_size) && (bwp_start_subcarrier+nb_rb_pusch*12 >= gNB->frame_parms.ofdm_symbol_size)) {
        ul_ch = (int16_t *)&ul_ch_estimates[p*gNB->frame_parms.nb_antennas_rx+aarx][ch_offset];
        uint16_t idxDC = 2*(gNB->frame_parms.ofdm_symbol_size - bwp_start_subcarrier);
        uint16_t idxPil = idxDC/2;
        re_offset = k;
        pil = (int16_t *)&pilot[0];
        pil += (idxPil-2);
        ul_ch += (idxDC-4);
        ul_ch = memset(ul_ch, 0, sizeof(int16_t)*10);
        re_offset = (re_offset+idxDC/2-2) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][soffset+(symbol_offset+nushift+re_offset)];
        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

        // for proper allignment of SIMD vectors
        if((gNB->frame_parms.N_RB_UL&1)==0) {

          multadd_real_vector_complex_scalar(fdcl,
                                             ch,
                                             ul_ch-4,
                                             8);
        
          pil += 4;
          re_offset = (re_offset+4) % gNB->frame_parms.ofdm_symbol_size;
          rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
          ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
          ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
        
          multadd_real_vector_complex_scalar(fdcr,
                                             ch,
                                             ul_ch-4,
                                             8);
        }
        else {
          multadd_real_vector_complex_scalar(fdclh,
                                             ch,
                                             ul_ch,
                                             8);
        
          pil += 4;
          re_offset = (re_offset+4) % gNB->frame_parms.ofdm_symbol_size;
          rxF   = (int16_t *)&rxdataF[aarx][soffset+(symbol_offset+nushift+re_offset)];
          ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
          ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
        
          multadd_real_vector_complex_scalar(fdcrh,
                                             ch,
                                             ul_ch,
                                             8);
        }
      }
#ifdef DEBUG_PUSCH
      ul_ch = (int16_t *)&ul_ch_estimates[p*gNB->frame_parms.nb_antennas_rx+aarx][ch_offset];
      for(uint16_t idxP=0; idxP<ceil((float)nb_rb_pusch*12/8); idxP++) {
        for(uint8_t idxI=0; idxI<16; idxI += 2) {
          printf("%d\t%d\t",ul_ch[idxP*16+idxI],ul_ch[idxP*16+idxI+1]);
        }
        printf("%d\n",idxP);
      }
#endif    
    }
    else if (pusch_pdu->dmrs_config_type == pusch_dmrs_type2 && gNB->prb_interpolation == 0) { //pusch_dmrs_type2  |p_r,p_l,d,d,d,d,p_r,p_l,d,d,d,d|
      LOG_D(PHY,"PUSCH estimation DMRS type 2, Freq-domain interpolation");
      // Treat first DMRS specially (left edge)

        rxF   = (int16_t *)&rxdataF[aarx][soffset+(symbol_offset+nushift+re_offset)];

        ul_ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ul_ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

        pil += 2;
        ul_ch += 2;
        re_offset = (re_offset + 1)%gNB->frame_parms.ofdm_symbol_size;
        ch_offset++;

        for (re_cnt = 1; re_cnt < (nb_rb_pusch*NR_NB_SC_PER_RB) - 5; re_cnt += 6){

          rxF   = (int16_t *)&rxdataF[aarx][soffset+(symbol_offset+nushift+re_offset)];

          ch_l[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
          ch_l[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

          ul_ch[0] = ch_l[0];
          ul_ch[1] = ch_l[1];

          pil += 2;
          ul_ch += 2;
          ch_offset++;

          multadd_real_four_symbols_vector_complex_scalar(filt8_ml2,
                                                          ch_l,
                                                          ul_ch);

          re_offset = (re_offset+5)%gNB->frame_parms.ofdm_symbol_size;

          rxF   = (int16_t *)&rxdataF[aarx][soffset+symbol_offset+nushift+re_offset];

          ch_r[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
          ch_r[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);


          multadd_real_four_symbols_vector_complex_scalar(filt8_mr2,
                                                          ch_r,
                                                          ul_ch);

          //for (int re_idx = 0; re_idx < 8; re_idx += 2)
            //printf("ul_ch = %d + j*%d\n", ul_ch[re_idx], ul_ch[re_idx+1]);

          ul_ch += 8;
          ch_offset += 4;

          ul_ch[0] = ch_r[0];
          ul_ch[1] = ch_r[1];

          pil += 2;
          ul_ch += 2;
          ch_offset++;
          re_offset = (re_offset + 1)%gNB->frame_parms.ofdm_symbol_size;

        }

        // Treat last pilot specially (right edge)

        rxF   = (int16_t *)&rxdataF[aarx][soffset+(symbol_offset+nushift+re_offset)];

        ch_l[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch_l[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

        ul_ch[0] = ch_l[0];
        ul_ch[1] = ch_l[1];

        ul_ch += 2;
        ch_offset++;

        multadd_real_four_symbols_vector_complex_scalar(filt8_rr1,
                                                        ch_l,
                                                        ul_ch);

        multadd_real_four_symbols_vector_complex_scalar(filt8_rr2,
                                                        ch_r,
                                                        ul_ch);

        ul_ch_128 = (__m128i *)&ul_ch_estimates[p*gNB->frame_parms.nb_antennas_rx+aarx][ch_offset];

        ul_ch_128[0] = _mm_slli_epi16 (ul_ch_128[0], 2);
    }

    else if (pusch_pdu->dmrs_config_type == pusch_dmrs_type1) {// this is case without frequency-domain linear interpolation, just take average of LS channel estimates of 6 DMRS REs and use a common value for the whole PRB
      LOG_D(PHY,"PUSCH estimation DMRS type 1, no Freq-domain interpolation");
      int32_t ch_0, ch_1;
      // First PRB
      ch_0 = ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 = ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch[0] = ch_0 / 6;
      ch[1] = ch_1 / 6;




#if NO_INTERP
      for (int i=0;i<12;i++) ((int32_t*)ul_ch)[i] = *(int32_t*)ch;
      ul_ch+=24;
#else
      multadd_real_vector_complex_scalar(filt8_avlip0,
                                         ch,
                                         ul_ch,
                                         8);

      ul_ch += 16;
      multadd_real_vector_complex_scalar(filt8_avlip1,
                                         ch,
                                         ul_ch,
                                         8);

      ul_ch += 16;
      multadd_real_vector_complex_scalar(filt8_avlip2,
                                         ch,
                                         ul_ch,
                                         8);
      ul_ch -= 24;
#endif

      for (pilot_cnt=6; pilot_cnt<6*(nb_rb_pusch-1); pilot_cnt += 6) {

        ch_0 = ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
        ch_1 = ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

        pil += 2;
        re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

        ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
        ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

        pil += 2;
        re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

        ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
        ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

        pil += 2;
        re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

        ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
        ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

        pil += 2;
        re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

        ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
        ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

        pil += 2;
        re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

        ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
        ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

        pil += 2;
        re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

        ch[0] = ch_0 / 6;
        ch[1] = ch_1 / 6;

#if NO_INTERP
      for (int i=0;i<12;i++) ((int32_t*)ul_ch)[i] = *(int32_t*)ch;
      ul_ch+=24;
#else
        ul_ch[6] += (ch[0] * 1365)>>15; // 1/12*16384
        ul_ch[7] += (ch[1] * 1365)>>15; // 1/12*16384

        ul_ch += 8;
        multadd_real_vector_complex_scalar(filt8_avlip3,
                                           ch,
                                           ul_ch,
                                           8);

        ul_ch += 16;
        multadd_real_vector_complex_scalar(filt8_avlip4,
                                           ch,
                                           ul_ch,
                                           8);

        ul_ch += 16;
        multadd_real_vector_complex_scalar(filt8_avlip5,
                                           ch,
                                           ul_ch,
                                           8);
        ul_ch -= 16;
#endif
      }
      // Last PRB
      ch_0 = ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 = ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch[0] = ch_0 / 6;
      ch[1] = ch_1 / 6;

#if NO_INTERP
      for (int i=0;i<12;i++) ((int32_t*)ul_ch)[i] = *(int32_t*)ch;
      ul_ch+=24;
#else
      ul_ch[6] += (ch[0] * 1365)>>15; // 1/12*16384
      ul_ch[7] += (ch[1] * 1365)>>15; // 1/12*16384

      ul_ch += 8;
      multadd_real_vector_complex_scalar(filt8_avlip3,
                                         ch,
                                         ul_ch,
                                         8);

      ul_ch += 16;
      multadd_real_vector_complex_scalar(filt8_avlip6,
                                         ch,
                                         ul_ch,
                                         8);
#endif
    }
    else  { // this is case without frequency-domain linear interpolation, just take average of LS channel estimates of 4 DMRS REs and use a common value for the whole PRB
      LOG_D(PHY,"PUSCH estimation DMRS type 2, no Freq-domain interpolation");
      int32_t ch_0, ch_1;
      //First PRB
      ch_0 = ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 = ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+1) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+5) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+1) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+5) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch[0] = ch_0 / 4;
      ch[1] = ch_1 / 4;

      multadd_real_vector_complex_scalar(filt8_avlip0,
                                         ch,
                                         ul_ch,
                                         8);

      ul_ch += 16;
      multadd_real_vector_complex_scalar(filt8_avlip1,
                                         ch,
                                         ul_ch,
                                         8);

      ul_ch += 16;
      multadd_real_vector_complex_scalar(filt8_avlip2,
                                         ch,
                                         ul_ch,
                                         8);
      ul_ch -= 24;

      for (pilot_cnt=4; pilot_cnt<4*(nb_rb_pusch-1); pilot_cnt += 4) {

        ch_0 = ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
        ch_1 = ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

        pil += 2;
        re_offset = (re_offset+1) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

        ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
        ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

        pil += 2;
        re_offset = (re_offset+5) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

        ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
        ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

        pil += 2;
        re_offset = (re_offset+1) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

        ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
        ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

        pil += 2;
        re_offset = (re_offset+5) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

        ch[0] = ch_0 / 4;
        ch[1] = ch_1 / 4;

        ul_ch[6] += (ch[0] * 1365)>>15; // 1/12*16384
        ul_ch[7] += (ch[1] * 1365)>>15; // 1/12*16384

        ul_ch += 8;
        multadd_real_vector_complex_scalar(filt8_avlip3,
                                           ch,
                                           ul_ch,
                                           8);

        ul_ch += 16;
        multadd_real_vector_complex_scalar(filt8_avlip4,
                                           ch,
                                           ul_ch,
                                           8);

        ul_ch += 16;
        multadd_real_vector_complex_scalar(filt8_avlip5,
                                           ch,
                                           ul_ch,
                                           8);
        ul_ch -= 16;
      }
      // Last PRB
      ch_0 = ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 = ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+1) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+5) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+1) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch_0 += ((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15;
      ch_1 += ((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15;

      pil += 2;
      re_offset = (re_offset+5) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

      ch[0] = ch_0 / 4;
      ch[1] = ch_1 / 4;

      ul_ch[6] += (ch[0] * 1365)>>15; // 1/12*16384
      ul_ch[7] += (ch[1] * 1365)>>15; // 1/12*16384

      ul_ch += 8;
      multadd_real_vector_complex_scalar(filt8_avlip3,
                                         ch,
                                         ul_ch,
                                         8);

      ul_ch += 16;
      multadd_real_vector_complex_scalar(filt8_avlip6,
                                         ch,
                                         ul_ch,
                                         8);
    }
#ifdef DEBUG_PUSCH
    ul_ch = (int16_t *)&ul_ch_estimates[p*gNB->frame_parms.nb_antennas_rx+aarx][ch_offset];
    for(uint16_t idxP=0; idxP<ceil((float)nb_rb_pusch*12/8); idxP++) {
      for(uint8_t idxI=0; idxI<16; idxI += 2) {
        printf("%d\t%d\t",ul_ch[idxP*16+idxI],ul_ch[idxP*16+idxI+1]);
      }
      printf("%d\n",idxP);
    }
#endif

    // Convert to time domain

    switch (gNB->frame_parms.ofdm_symbol_size) {
        case 128:
          idft(IDFT_128,(int16_t*) &ul_ch_estimates[aarx][symbol_offset],
                 (int16_t*) ul_ch_estimates_time[aarx],
                 1);
          break;

        case 256:
          idft(IDFT_256,(int16_t*) &ul_ch_estimates[aarx][symbol_offset],
                 (int16_t*) ul_ch_estimates_time[aarx],
                 1);
          break;

        case 512:
          idft(IDFT_512,(int16_t*) &ul_ch_estimates[aarx][symbol_offset],
                 (int16_t*) ul_ch_estimates_time[aarx],
                 1);
          break;

        case 1024:
          idft(IDFT_1024,(int16_t*) &ul_ch_estimates[aarx][symbol_offset],
                 (int16_t*) ul_ch_estimates_time[aarx],
                 1);
          break;

        case 1536:
          idft(IDFT_1536,(int16_t*) &ul_ch_estimates[aarx][symbol_offset],
                 (int16_t*) ul_ch_estimates_time[aarx],
                 1);
          break;

        case 2048:
          idft(IDFT_2048,(int16_t*) &ul_ch_estimates[aarx][symbol_offset],
                 (int16_t*) ul_ch_estimates_time[aarx],
                 1);
          break;

        case 4096:
          idft(IDFT_4096,(int16_t*) &ul_ch_estimates[aarx][symbol_offset],
                 (int16_t*) ul_ch_estimates_time[aarx],
                 1);
          break;

        case 8192:
          idft(IDFT_8192,(int16_t*) &ul_ch_estimates[aarx][symbol_offset],
                 (int16_t*) ul_ch_estimates_time[aarx],
                 1);
          break;

        default:
          idft(IDFT_512,(int16_t*) &ul_ch_estimates[aarx][symbol_offset],
                 (int16_t*) ul_ch_estimates_time[aarx],
                 1);
          break;
      }

  }

#ifdef DEBUG_CH
  fclose(debug_ch_est);
#endif

  return(0);
}


/*******************************************************************
 *
 * NAME :         nr_pusch_ptrs_processing
 *
 * PARAMETERS :   gNB         : gNB data structure
 *                rel15_ul    : UL parameters
 *                UE_id       : UE ID
 *                nr_tti_rx   : slot rx TTI
 *            dmrs_symbol_flag: DMRS Symbol Flag
 *                symbol      : OFDM Symbol
 *                nb_re_pusch : PUSCH RE's
 *                nb_re_pusch : PUSCH RE's
 *
 * RETURN :       nothing
 *
 * DESCRIPTION :
 *  If ptrs is enabled process the symbol accordingly
 *  1) Estimate phase noise per PTRS symbol
 *  2) Interpolate PTRS estimated value in TD after all PTRS symbols
 *  3) Compensated DMRS based estimated signal with PTRS estimation for slot
 *********************************************************************/
void nr_pusch_ptrs_processing(PHY_VARS_gNB *gNB,
                              NR_DL_FRAME_PARMS *frame_parms,
                              nfapi_nr_pusch_pdu_t *rel15_ul,
                              uint8_t ulsch_id,
                              uint8_t nr_tti_rx,
                              unsigned char symbol,
                              uint32_t nb_re_pusch)
{
  //#define DEBUG_UL_PTRS 1
  int16_t *phase_per_symbol = NULL;
  int32_t *ptrs_re_symbol   = NULL;
  int8_t   ret = 0;

  uint8_t  symbInSlot       = rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols;
  uint8_t *startSymbIndex   = &rel15_ul->start_symbol_index;
  uint8_t *nbSymb           = &rel15_ul->nr_of_symbols;
  uint8_t  *L_ptrs          = &rel15_ul->pusch_ptrs.ptrs_time_density;
  uint8_t  *K_ptrs          = &rel15_ul->pusch_ptrs.ptrs_freq_density;
  uint16_t *dmrsSymbPos     = &rel15_ul->ul_dmrs_symb_pos;
  uint16_t *ptrsSymbPos     = &gNB->pusch_vars[ulsch_id]->ptrs_symbols;
  uint8_t  *ptrsSymbIdx     = &gNB->pusch_vars[ulsch_id]->ptrs_symbol_index;
  uint8_t  *dmrsConfigType  = &rel15_ul->dmrs_config_type;
  uint16_t *nb_rb           = &rel15_ul->rb_size;
  uint8_t  *ptrsReOffset    = &rel15_ul->pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset;
  /* loop over antennas */
  for (int aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    phase_per_symbol = (int16_t*)gNB->pusch_vars[ulsch_id]->ptrs_phase_per_slot[aarx];
    ptrs_re_symbol = &gNB->pusch_vars[ulsch_id]->ptrs_re_per_slot;
    *ptrs_re_symbol = 0;
    phase_per_symbol[(2*symbol)+1] = 0; // Imag
    /* set DMRS estimates to 0 angle with magnitude 1 */
    if(is_dmrs_symbol(symbol,*dmrsSymbPos)) {
      /* set DMRS real estimation to 32767 */
      phase_per_symbol[2*symbol]=(int16_t)((1<<15)-1); // 32767
#ifdef DEBUG_UL_PTRS
      printf("[PHY][PTRS]: DMRS Symbol %d -> %4d + j*%4d\n", symbol, phase_per_symbol[2*symbol],phase_per_symbol[(2*symbol)+1]);
#endif
    }
    else {// real ptrs value is set to 0
      phase_per_symbol[2*symbol] = 0; // Real
    }

    if(symbol == *startSymbIndex) {
      *ptrsSymbPos = 0;
      set_ptrs_symb_idx(ptrsSymbPos,
                        *nbSymb,
                        *startSymbIndex,
                        1<< *L_ptrs,
                        *dmrsSymbPos);
    }
    /* if not PTRS symbol set current ptrs symbol index to zero*/
    *ptrsSymbIdx = 0;
    /* Check if current symbol contains PTRS */
    if(is_ptrs_symbol(symbol, *ptrsSymbPos)) {
      *ptrsSymbIdx = symbol;
      /*------------------------------------------------------------------------------------------------------- */
      /* 1) Estimate common phase error per PTRS symbol                                                                */
      /*------------------------------------------------------------------------------------------------------- */
      nr_ptrs_cpe_estimation(*K_ptrs,*ptrsReOffset,*dmrsConfigType,*nb_rb,
                             rel15_ul->rnti,
                             (int16_t *)&gNB->pusch_vars[ulsch_id]->ul_ch_ptrs_estimates_ext[aarx][symbol*nb_re_pusch],
                             nr_tti_rx,
                             symbol,frame_parms->ofdm_symbol_size,
                             (int16_t*)&gNB->pusch_vars[ulsch_id]->rxdataF_comp[aarx][(symbol * nb_re_pusch)],
                             gNB->nr_gold_pusch_dmrs[rel15_ul->scid][nr_tti_rx][symbol],
                             &phase_per_symbol[2* symbol],
                             ptrs_re_symbol);
    }
    /* For last OFDM symbol at each antenna perform interpolation and compensation for the slot*/
    if(symbol == (symbInSlot -1)) {
      /*------------------------------------------------------------------------------------------------------- */
      /* 2) Interpolate PTRS estimated value in TD */
      /*------------------------------------------------------------------------------------------------------- */
      /* If L-PTRS is > 0 then we need interpolation */
      if(*L_ptrs > 0) {
        ret = nr_ptrs_process_slot(*dmrsSymbPos, *ptrsSymbPos, phase_per_symbol, *startSymbIndex, *nbSymb);
        if(ret != 0) {
          LOG_W(PHY,"[PTRS] Compensation is skipped due to error in PTRS slot processing !!\n");
        }
      }
#ifdef DEBUG_UL_PTRS
      LOG_M("ptrsEstUl.m","est",gNB->pusch_vars[ulsch_id]->ptrs_phase_per_slot[aarx],frame_parms->symbols_per_slot,1,1 );
      LOG_M("rxdataF_bf_ptrs_comp_ul.m","bf_ptrs_cmp",
            &gNB->pusch_vars[0]->rxdataF_comp[aarx][rel15_ul->start_symbol_index * NR_NB_SC_PER_RB * rel15_ul->rb_size],
            rel15_ul->nr_of_symbols * NR_NB_SC_PER_RB * rel15_ul->rb_size,1,1);
#endif
      /*------------------------------------------------------------------------------------------------------- */
      /* 3) Compensated DMRS based estimated signal with PTRS estimation                                        */
      /*--------------------------------------------------------------------------------------------------------*/
      for(uint8_t i = *startSymbIndex; i< symbInSlot ;i++) {
        /* DMRS Symbol has 0 phase so no need to rotate the respective symbol */
        /* Skip rotation if the slot processing is wrong */
        if((!is_dmrs_symbol(i,*dmrsSymbPos)) && (ret == 0)) {
#ifdef DEBUG_UL_PTRS
          printf("[PHY][UL][PTRS]: Rotate Symbol %2d with  %d + j* %d\n", i, phase_per_symbol[2* i],phase_per_symbol[(2* i) +1]);
#endif
          rotate_cpx_vector((int16_t*)&gNB->pusch_vars[ulsch_id]->rxdataF_comp[aarx][(i * rel15_ul->rb_size * NR_NB_SC_PER_RB)],
                            &phase_per_symbol[2* i],
                            (int16_t*)&gNB->pusch_vars[ulsch_id]->rxdataF_comp[aarx][(i * rel15_ul->rb_size * NR_NB_SC_PER_RB)],
                            ((*nb_rb) * NR_NB_SC_PER_RB), 15);
        }// if not DMRS Symbol
      }// symbol loop
    }// last symbol check
  }//Antenna loop
}
