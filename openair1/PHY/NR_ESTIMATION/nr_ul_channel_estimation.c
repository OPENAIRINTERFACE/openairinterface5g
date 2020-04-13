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
#include "PHY/NR_UE_ESTIMATION/filt16a_32.h"

//#define DEBUG_CH


int nr_pusch_channel_estimation(PHY_VARS_gNB *gNB,
                                unsigned char Ns,
                                unsigned short p,
                                unsigned char symbol,
                                unsigned short bwp_start_subcarrier,
                                nfapi_nr_pusch_pdu_t *pusch_pdu) {

  int pilot[3280] __attribute__((aligned(16)));
  unsigned char aarx;
  unsigned short k;
  unsigned int pilot_cnt,re_cnt;
  int16_t ch[2],ch_r[2],ch_l[2],*pil,*rxF,*ul_ch;
  int16_t *fl,*fm,*fr,*fml,*fmr,*fmm,*fdcl,*fdcr,*fdclh,*fdcrh;
  int ch_offset,symbol_offset, UE_id = 0;
  int32_t **ul_ch_estimates_time =  gNB->pusch_vars[UE_id]->ul_ch_estimates_time;
  __m128i *ul_ch_128;

#ifdef DEBUG_CH
  FILE *debug_ch_est;
  debug_ch_est = fopen("debug_ch_est.txt","w");
#endif

  //uint16_t Nid_cell = (eNB_offset == 0) ? gNB->frame_parms.Nid_cell : gNB->measurements.adj_cell_id[eNB_offset-1];

  uint8_t nushift;
  int **ul_ch_estimates  = gNB->pusch_vars[UE_id]->ul_ch_estimates;
  int **rxdataF = gNB->common_vars.rxdataF;

  nushift = (p>>1)&1;
  gNB->frame_parms.nushift = nushift;

  ch_offset     = gNB->frame_parms.ofdm_symbol_size*symbol;

  symbol_offset = gNB->frame_parms.ofdm_symbol_size*symbol;

  k = bwp_start_subcarrier;
  int re_offset = k;

  uint16_t nb_rb_pusch = pusch_pdu->rb_size;

/*
#ifdef DEBUG_CH
  printf("PUSCH Channel Estimation : ch_offset %d, symbol_offset %d OFDM size %d, Ncp=%d, l=%d, Ns=%d, k=%d symbol %d\n", ,ch_offset,symbol_offset,gNB->frame_parms.ofdm_symbol_size,
         gNB->frame_parms.Ncp,l,Ns,k, symbol);
#endif
*/
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

  nr_pusch_dmrs_rx(gNB, Ns, gNB->nr_gold_pusch_dmrs[pusch_pdu->scid][Ns][symbol], &pilot[0], 1000, 0, nb_rb_pusch, pusch_pdu->dmrs_config_type);

  //------------------------------------------------//

  for (aarx=0; aarx<gNB->frame_parms.nb_antennas_rx; aarx++) {

    pil   = (int16_t *)&pilot[0];
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+nushift)];
    ul_ch = (int16_t *)&ul_ch_estimates[aarx][ch_offset];

    memset(ul_ch,0,4*(gNB->frame_parms.ofdm_symbol_size));

#ifdef DEBUG_PUSCH
    printf("ch est pilot addr %p RB_DL %d\n",&pilot[0], gNB->frame_parms.N_RB_UL);
    printf("k %d, first_carrier %d\n",k,gNB->frame_parms.first_carrier_offset);
    printf("rxF addr %p p %d\n", rxF,p);
    printf("ul_ch addr %p nushift %d\n",ul_ch,nushift);
#endif
    //if ((gNB->frame_parms.N_RB_UL&1)==0) {

    if (pusch_pdu->dmrs_config_type == pusch_dmrs_type1){

      // Treat first 2 pilots specially (left edge)
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PUSCH
      printf("ch 0 %d\n",((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1]));
      printf("pilot 0 : rxF - > (%d,%d) addr %p  ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
      printf("data 0 : rxF - > (%d,%d) addr %p  ch -> (%d,%d), pil -> (%d,%d) \n",rxF[2],rxF[3],&rxF[2],ch[0],ch[1],pil[0],pil[1]);
#endif

      multadd_real_vector_complex_scalar(fl,
                                         ch,
                                         ul_ch,
                                         8);
      pil+=2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
      //for (int i= 0; i<8; i++)
      //printf("ul_ch addr %p %d\n", ul_ch+i, *(ul_ch+i));

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PUSCH
      printf("pilot 1 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fml,
                                         ch,
                                         ul_ch,
                                         8);
      pil+=2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
      //printf("ul_ch addr %p\n",ul_ch);
      
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PUSCH
      printf("pilot 2 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fmm,
                                         ch,
                                         ul_ch,
                                         8);
                                         
      //for (int i= 0; i<16; i++)
      //printf("ul_ch addr %p %d\n", ul_ch+i, *(ul_ch+i));
      
      pil+=2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
      ul_ch+=8;

      for (pilot_cnt=3; pilot_cnt<(6*nb_rb_pusch-3); pilot_cnt+=2) {

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

  #ifdef DEBUG_PUSCH
        printf("pilot 1 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
  #endif
        multadd_real_vector_complex_scalar(fml,
                                           ch,
                                           ul_ch,
                                           8);
        pil+=2;
        re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
        //printf("ul_ch addr %p\n",ul_ch);

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

  #ifdef DEBUG_PUSCH
        printf("pilot %u : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
  #endif
        multadd_real_vector_complex_scalar(fmm,
                                           ch,
                                           ul_ch,
                                           8);

        //for (int i= 0; i<16; i++)
        //printf("ul_ch addr %p %d\n", ul_ch+i, *(ul_ch+i));

        pil+=2;
        re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
        ul_ch+=8;

      }
      
      // Treat first 2 pilots specially (right edge)
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PUSCH
      printf("pilot %u : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fm,
                                         ch,
                                         ul_ch,
                                         8);
                                         
      //for (int i= 0; i<8; i++)
      //printf("ul_ch addr %p %d\n", ul_ch+i, *(ul_ch+i));

      pil+=2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
             
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PUSCH
      printf("ch 0 %d\n",((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1]));
      printf("pilot %u: rxF - > (%d,%d) addr %p  ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fmr,
                                         ch,
                                         ul_ch,
                                         8);
                                         
      pil+=2;
      re_offset = (re_offset+2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
      ul_ch+=8;
      
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PUSCH
      printf("pilot %u: rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+2,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fr,
                                         ch,
                                         ul_ch,
                                         8);


    // check if PRB crosses DC and improve estimates around DC
    if ((bwp_start_subcarrier < gNB->frame_parms.ofdm_symbol_size) && (bwp_start_subcarrier+nb_rb_pusch*12 >= gNB->frame_parms.ofdm_symbol_size)) {
      ul_ch = (int16_t *)&ul_ch_estimates[aarx][ch_offset];
      uint16_t idxDC = 2*(gNB->frame_parms.ofdm_symbol_size - bwp_start_subcarrier);
      uint16_t idxPil = idxDC/2;
      re_offset = k;
      pil = (int16_t *)&pilot[0];
      pil += (idxPil-2);
      ul_ch += (idxDC-4);
      ul_ch = memset(ul_ch, 0, sizeof(int16_t)*10);
      re_offset = (re_offset+idxDC/2-2) % gNB->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
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
      } else {
        
        multadd_real_vector_complex_scalar(fdclh,
                                           ch,
                                           ul_ch,
                                           8);
        
        pil += 4;
        re_offset = (re_offset+4) % gNB->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
        
        multadd_real_vector_complex_scalar(fdcrh,
                                           ch,
                                           ul_ch,
                                           8);
      }

    }
#ifdef DEBUG_PDSCH
    ul_ch = (int16_t *)&ul_ch_estimates[aarx][ch_offset];
    for(uint16_t idxP=0; idxP<ceil((float)nb_rb_pusch*12/8); idxP++) {
      for(uint8_t idxI=0; idxI<16; idxI+=2) {
        printf("%d\t%d\t",ul_ch[idxP*16+idxI],ul_ch[idxP*16+idxI+1]);
      }
      printf("%d\n",idxP);
    }
#endif    
    
    } else { //pusch_dmrs_type2  |p_r,p_l,d,d,d,d,p_r,p_l,d,d,d,d|

      // Treat first DMRS specially (left edge)

        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

        ul_ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ul_ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

        pil+=2;
        ul_ch+=2;
        re_offset = (re_offset + 1)%gNB->frame_parms.ofdm_symbol_size;
        ch_offset++;

        for (re_cnt = 1; re_cnt < (nb_rb_pusch*NR_NB_SC_PER_RB) - 5; re_cnt+=6){

          rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

          ch_l[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
          ch_l[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

          ul_ch[0] = ch_l[0];
          ul_ch[1] = ch_l[1];

          pil+=2;
          ul_ch+=2;
          ch_offset++;

          multadd_real_four_symbols_vector_complex_scalar(filt8_ml2,
                                                          ch_l,
                                                          ul_ch);

          re_offset = (re_offset+5)%gNB->frame_parms.ofdm_symbol_size;

          rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

          ch_r[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
          ch_r[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);


          multadd_real_four_symbols_vector_complex_scalar(filt8_mr2,
                                                          ch_r,
                                                          ul_ch);

          //for (int re_idx = 0; re_idx < 8; re_idx+=2)
            //printf("ul_ch = %d + j*%d\n", ul_ch[re_idx], ul_ch[re_idx+1]);

          ul_ch+=8;
          ch_offset+=4;

          ul_ch[0] = ch_r[0];
          ul_ch[1] = ch_r[1];

          pil+=2;
          ul_ch+=2;
          ch_offset++;
          re_offset = (re_offset + 1)%gNB->frame_parms.ofdm_symbol_size;

        }

        // Treat last pilot specially (right edge)

        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];

        ch_l[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch_l[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

        ul_ch[0] = ch_l[0];
        ul_ch[1] = ch_l[1];

        ul_ch+=2;
        ch_offset++;

        multadd_real_four_symbols_vector_complex_scalar(filt8_rr1,
                                                        ch_l,
                                                        ul_ch);

        multadd_real_four_symbols_vector_complex_scalar(filt8_rr2,
                                                        ch_r,
                                                        ul_ch);

        ul_ch_128 = (__m128i *)&ul_ch_estimates[aarx][ch_offset];

        ul_ch_128[0] = _mm_slli_epi16 (ul_ch_128[0], 2);
  }



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
