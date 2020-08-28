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
#include "PHY/NR_REFSIG/ptrs_nr.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_UE_ESTIMATION/filt16a_32.h"

//#define DEBUG_CH
//#define DEBUG_PUSCH

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

  if (pusch_pdu->transform_precoding==1) // if transform precoding is disabled
    nr_pusch_dmrs_rx(gNB, Ns, gNB->nr_gold_pusch_dmrs[pusch_pdu->scid][Ns][symbol], &pilot[0], 1000, 0, nb_rb_pusch, pusch_pdu->rb_start*NR_NB_SC_PER_RB, pusch_pdu->dmrs_config_type);
  else
    nr_pusch_dmrs_rx(gNB, Ns, gNB->nr_gold_pusch_dmrs[pusch_pdu->scid][Ns][symbol], &pilot[0], 1000, 0, nb_rb_pusch, 0, pusch_pdu->dmrs_config_type);

  //------------------------------------------------//

  for (aarx=0; aarx<gNB->frame_parms.nb_antennas_rx; aarx++) {

    pil   = (int16_t *)&pilot[0];
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+nushift)];
    ul_ch = (int16_t *)&ul_ch_estimates[aarx][ch_offset];

    memset(ul_ch,0,4*(gNB->frame_parms.ofdm_symbol_size));

#ifdef DEBUG_PUSCH
    printf("symbol_offset %d, nushift %d\n",symbol_offset,nushift);
    printf("ch est pilot addr %p RB_DL %d\n",&pilot[0], gNB->frame_parms.N_RB_UL);
    printf("bwp_start_subcarrier %d, k %d, first_carrier %d\n",bwp_start_subcarrier,k,gNB->frame_parms.first_carrier_offset);
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
      printf("data 0 : rxF - > (%d,%d)\n",rxF[2],rxF[3]);
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
      printf("data 1 : rxF - > (%d,%d)\n",rxF[2],rxF[3]);
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
      printf("data 2 : rxF - > (%d,%d)\n",rxF[2],rxF[3]);
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
        printf("pilot %u : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
	printf("data %u : rxF - > (%d,%d)\n",pilot_cnt,rxF[2],rxF[3]);
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
	printf("data %u : rxF - > (%d,%d)\n",pilot_cnt+1,rxF[2],rxF[3]);
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
      printf("data %u : rxF - > (%d,%d)\n",pilot_cnt,rxF[2],rxF[3]);
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
      printf("data %u : rxF - > (%d,%d)\n",pilot_cnt+1,rxF[2],rxF[3]);
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
      printf("data %u : rxF - > (%d,%d)\n",pilot_cnt+2,rxF[2],rxF[3]);
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
        }
        else {
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
    }
    else { //pusch_dmrs_type2  |p_r,p_l,d,d,d,d,p_r,p_l,d,d,d,d|

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
                              nfapi_nr_pusch_pdu_t *rel15_ul,
                              uint8_t ulsch_id,
                              uint8_t nr_tti_rx,
                              uint8_t dmrs_symbol_flag,
                              unsigned char symbol,
                              uint32_t nb_re_pusch)
{
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  int16_t *phase_per_symbol;

  uint8_t         L_ptrs          = 0;
  uint8_t         right_side_ref  = 0;
  uint8_t         left_side_ref   = 0;
  uint8_t         nb_dmrs_in_slot = 0;

  //#define DEBUG_UL_PTRS 1
  /* First symbol calculate PTRS symbol index for slot & set the variables */
  if(symbol == rel15_ul->start_symbol_index)
  {
    gNB->pusch_vars[ulsch_id]->ptrs_symbols = 0;
    L_ptrs = 1<<(rel15_ul->pusch_ptrs.ptrs_time_density);
    set_ptrs_symb_idx(&gNB->pusch_vars[ulsch_id]->ptrs_symbols,
                      rel15_ul->nr_of_symbols,
                      rel15_ul->start_symbol_index,
                      L_ptrs,
                      rel15_ul->ul_dmrs_symb_pos);
  }/* First symbol check */

  /* loop over antennas */
  for (int aarx=0; aarx< frame_parms->nb_antennas_rx; aarx++)
  {
    phase_per_symbol = (int16_t*)gNB->pusch_vars[ulsch_id]->ptrs_phase_per_slot[aarx];
    /* if not PTRS symbol set current ptrs symbol index to zero*/
    gNB->pusch_vars[ulsch_id]->ptrs_symbol_index = 0;
    gNB->pusch_vars[ulsch_id]->ptrs_sc_per_ofdm_symbol = 0;
    /* Check if current symbol contains PTRS */
    if(is_ptrs_symbol(symbol, gNB->pusch_vars[ulsch_id]->ptrs_symbols))
    {
      gNB->pusch_vars[ulsch_id]->ptrs_symbol_index = symbol;
      /*------------------------------------------------------------------------------------------------------- */
      /* 1) Estimate phase noise per PTRS symbol                                                                */
      /*------------------------------------------------------------------------------------------------------- */
      nr_pusch_phase_estimation(frame_parms,
                                rel15_ul,
                                (int16_t *)&gNB->pusch_vars[ulsch_id]->ul_ch_ptrs_estimates_ext[aarx][symbol*nb_re_pusch],
                                nr_tti_rx,
                                symbol,
                                (int16_t*)&gNB->pusch_vars[ulsch_id]->rxdataF_comp[aarx][(symbol * nb_re_pusch)],
                                gNB->nr_gold_pusch_dmrs[rel15_ul->scid],
                                &phase_per_symbol[2* symbol],
                                &gNB->pusch_vars[ulsch_id]->ptrs_sc_per_ofdm_symbol);
    }
    /* DMRS Symbol channel estimates extraction */
    else if(dmrs_symbol_flag)
    {
      phase_per_symbol[2* symbol]= (int16_t)((1<<15)-1); // 32767
      phase_per_symbol[2* symbol +1]= 0;// no angle
    }
    /* For last OFDM symbol at each antenna perform interpolation and compensation for the slot*/
    if(symbol == (rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols -1))
    {
      nb_dmrs_in_slot = get_dmrs_symbols_in_slot(rel15_ul->ul_dmrs_symb_pos,(rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols));
      for(uint8_t dmrs_sym = 0; dmrs_sym < nb_dmrs_in_slot;  dmrs_sym ++)
      {
        if(dmrs_sym == 0)
        {
          /* get first DMRS position */
          left_side_ref = get_next_dmrs_symbol_in_slot(rel15_ul->ul_dmrs_symb_pos, rel15_ul->start_symbol_index, (rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols));
          /* get first DMRS position is not at start symbol position then we need to extrapolate left side  */
          if(left_side_ref > rel15_ul->start_symbol_index)
          {
            left_side_ref = rel15_ul->start_symbol_index;
          }
        }
        /* get the next symbol from left_side_ref value */
        right_side_ref = get_next_dmrs_symbol_in_slot(rel15_ul->ul_dmrs_symb_pos, left_side_ref+1, (rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols));
        /* if no symbol found then interpolate till end of slot*/
        if(right_side_ref == 0)
        {
          right_side_ref = (rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols);
        }
        /*------------------------------------------------------------------------------------------------------- */
        /* 2) Interpolate PTRS estimated value in TD */
        /*------------------------------------------------------------------------------------------------------- */
        nr_pusch_phase_interpolation(phase_per_symbol,left_side_ref,right_side_ref);
        /* set left to last dmrs */
        left_side_ref = right_side_ref;
      } /*loop over dmrs positions */

#ifdef DEBUG_UL_PTRS
      LOG_M("ptrsEst.m","est",gNB->pusch_vars[ulsch_id]->ptrs_phase_per_slot[aarx],frame_parms->symbols_per_slot,1,1 );
      LOG_M("rxdataF_bf_ptrs_comp.m","bf_ptrs_cmp",
            &gNB->pusch_vars[0]->rxdataF_comp[aarx][rel15_ul->start_symbol_index * NR_NB_SC_PER_RB * rel15_ul->rb_size],
            rel15_ul->nr_of_symbols * NR_NB_SC_PER_RB * rel15_ul->rb_size,1,1);
#endif

      /*------------------------------------------------------------------------------------------------------- */
      /* 3) Compensated DMRS based estimated signal with PTRS estimation                                        */
      /*--------------------------------------------------------------------------------------------------------*/
      for(uint8_t i =rel15_ul->start_symbol_index; i< (rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols);i++)
      {
#ifdef DEBUG_UL_PTRS
        printf("PTRS: Rotate Symbol %2d with  %d + j* %d\n", i, phase_per_symbol[2* i],phase_per_symbol[(2* i) +1]);
#endif
        rotate_cpx_vector((int16_t*)&gNB->pusch_vars[ulsch_id]->rxdataF_comp[aarx][(i * rel15_ul->rb_size * NR_NB_SC_PER_RB)],
                          &phase_per_symbol[2* i],
                          (int16_t*)&gNB->pusch_vars[ulsch_id]->rxdataF_comp[aarx][(i * rel15_ul->rb_size * NR_NB_SC_PER_RB)],
                          (rel15_ul->rb_size * NR_NB_SC_PER_RB),
                          15);
      }// symbol loop
    }//interpolation and compensation
  }// Antenna loop
}

/*******************************************************************
 *
 * NAME :         nr_pusch_phase_estimation
 *
 * PARAMETERS :   frame_parms  : UL frame parameters
 *                rel15_ul     : UL PDU Structure
 *                Ns           :
 *                Symbol       : OFDM symbol index
 *                rxF          : Channel compensated signal
 *                ptrs_gold_seq: Gold sequence for PTRS regeneration
 *                error_est    : Estimated error output vector [Re Im]
 * RETURN :       nothing
 *
 * DESCRIPTION :
 *  perform phase estimation from regenerated PTRS SC and channel compensated
 *  signal
 *********************************************************************/
void nr_pusch_phase_estimation(NR_DL_FRAME_PARMS *frame_parms,
                               nfapi_nr_pusch_pdu_t *rel15_ul,
                               int16_t *ptrs_ch_p,
                               unsigned char Ns,
                               unsigned char symbol,
                               int16_t *rxF_comp,
                               uint32_t ***ptrs_gold_seq,
                               int16_t *error_est,
                               uint16_t *ptrs_sc)
{
  uint8_t               is_ptrs_re       = 0;
  uint16_t              re_cnt           = 0;
  uint16_t              cnt              = 0;
  unsigned short        nb_re_pusch      = NR_NB_SC_PER_RB * rel15_ul->rb_size;
  uint8_t               K_ptrs           = (rel15_ul->pusch_ptrs.ptrs_freq_density)?4:2;
  uint16_t              sc_per_symbol    = rel15_ul->rb_size/K_ptrs;
  int16_t              *ptrs_p           = (int16_t *)malloc(sizeof(int32_t)*(sc_per_symbol));
  int16_t              *dmrs_comp_p      = (int16_t *)malloc(sizeof(int32_t)*(sc_per_symbol));
  double                abs              = 0.0;
  double                real             = 0.0;
  double                imag             = 0.0;
#ifdef DEBUG_UL_PTRS
  double                alpha            = 0;
#endif
  /* generate PTRS RE for the symbol */
  nr_ptrs_rx_gen(ptrs_gold_seq[Ns][symbol],rel15_ul->rb_size,ptrs_p);

  /* loop over all sub carriers to get compensated RE on ptrs symbols*/
  for (int re = 0; re < nb_re_pusch; re++)
  {
    is_ptrs_re = is_ptrs_subcarrier(re,
                                    rel15_ul->rnti,
                                    0,
                                    rel15_ul->dmrs_config_type,
                                    K_ptrs,
                                    rel15_ul->rb_size,
                                    rel15_ul->pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset,
                                    0,// start_re is 0 here
                                    frame_parms->ofdm_symbol_size);
    if(is_ptrs_re)
    {
      dmrs_comp_p[re_cnt*2]     = rxF_comp[re *2];
      dmrs_comp_p[(re_cnt*2)+1] = rxF_comp[(re *2)+1];
      re_cnt++;
    }
    else
    {
      /* Skip PTRS symbols and keep data in a continuous vector */
      rxF_comp[cnt *2]= rxF_comp[re *2];
      rxF_comp[(cnt *2)+1]= rxF_comp[(re *2)+1];
      cnt++;
    }
  }/* RE loop */
  /* update the total ptrs RE in a symbol */
  *ptrs_sc = re_cnt;

  /*Multiple compensated data with conj of PTRS */
  mult_cpx_vector(dmrs_comp_p, ptrs_p, ptrs_ch_p,(1 + sc_per_symbol/4)*4,15); // 2^15 shifted

  /* loop over all ptrs sub carriers in a symbol */
  /* sum the error vector */
  for(int i = 0;i < sc_per_symbol; i++)
  {
    real+= ptrs_ch_p[(2*i)];
    imag+= ptrs_ch_p[(2*i)+1];
  }
#ifdef DEBUG_UL_PTRS
    alpha = atan(imag/real);
    printf("PTRS: Symbol  %d atan(Im,real):= %f \n",symbol, alpha );
#endif
  /* mean */
  real /= sc_per_symbol;
  imag /= sc_per_symbol;
  /* absolute calculation */
  abs = sqrt(((real * real) + (imag *  imag)));
  /* normalized error estimation */
  error_est[0]= (real / abs)*(1<<15);
  /* compensation in given by conjugate of estimated phase (e^-j*2*pi*fd*t)*/
  error_est[1]= (-1)*(imag / abs)*(1<<15);
#ifdef DEBUG_UL_PTRS
    printf("PTRS: Estimated Symbol  %d -> %d + j* %d \n",symbol, error_est[0], error_est[1] );
#endif
  /* free vectors */
  free(ptrs_p);
  free(dmrs_comp_p);
}


/*******************************************************************
 *
 * NAME :         nr_pusch_phase_interpolation
 *
 * PARAMETERS :   *error_est    : Data Pointer [Re Im Re Im ...]
 *                 start_symbol : Start Symbol
 *                 end_symbol   : End Symbol
 * RETURN :       nothing
 *
 * DESCRIPTION :
 * Perform Interpolation, extrapolation based upon the estimation
 * location between the data Pointer Array.
 *
 *********************************************************************/
void nr_pusch_phase_interpolation(int16_t *error_est,
                                  uint8_t start_symbol,
                                  uint8_t end_symbol
                                  )
{

  int next = 0, prev = 0, candidates= 0, distance=0, leftEdge= 0, rightEdge = 0, getDiff =0 ;
  double weight = 0.0;
  double scale  = 0.125 ; // to avoid saturation due to fixed point multiplication
#ifdef DEBUG_UL_PTRS
  printf("PTRS: INT: Left limit %d, Right limit %d, Loop over %d Symbols \n",
         start_symbol,end_symbol-1, (end_symbol -start_symbol)-1);
#endif
  for(int i =start_symbol; i< end_symbol;i++)
  {
    /* Only update when an estimation is found */
    if( error_est[i*2] != 0 )
    {
      /* if found a symbol then set next symbol also */
      next = nr_ptrs_find_next_estimate(error_est, i, end_symbol);
      /* left extrapolation, if first estimate value is zero */
      if( error_est[i*2] == 0 )
      {
        leftEdge = 1;
      }
      /* right extrapolation, if next is 0 before end symbol */
      if((next == 0) && (end_symbol > i))
      {
        rightEdge = 1;
        /* special case as no right extrapolation possible with DMRS on left */
        /* In this case take mean of most recent 2 estimated points */
        if(prev ==0)
        {
          prev = start_symbol -1;
          next = start_symbol -2;
          getDiff =1;
        }else
        {
          /* for right edge  previous is second last from right side */
          next = prev;
          /* Set the current as recent estimation reference */
          prev = i;
        }
      }
      /* update  current symbol as prev  for next symbol */
      if (rightEdge==0)
        /* Set the current as recent estimation reference */
        prev = i;
    }
    /*extrapolation left side*/
    if(leftEdge)
    {
      distance = next - prev;
      weight = 1.0/distance;
      candidates = i;
      for(int j = 1; j <= candidates; j++)
      {
        error_est[(i-j)*2]    = 8 *(((double)(error_est[prev*2]) * scale * (distance + j) * weight) -
                                    ((double)(error_est[next*2]) * scale * j * weight));
        error_est[((i-j)*2)+1]= 8 *(((double)(error_est[(prev*2)+1]) * scale* (distance + j) * weight) -
                                    ((double)(error_est[((next*2)+1)]) * scale * j * weight));
#ifdef DEBUG_UL_PTRS
        printf("PTRS: INT: Left Edge i= %d weight= %f %d + j*%d, Prev %d Next %d \n",
               (i-j),weight, error_est[(i-j)*2],error_est[((i-j)*2)+1], prev,next);
#endif
      }
      leftEdge = 0;
    }
    /* extrapolation at right side */
    else if (rightEdge )
    {
      if(getDiff)
      {
        error_est[(i+1)*2]    = ((1<<15) +(error_est[prev*2]) - error_est[next*2]);
        error_est[((i+1)*2)+1]= error_est[(prev*2)+1] - error_est[(next*2)+1];
#ifdef DEBUG_UL_PTRS
        printf("PTRS: INT: Right Edge Special Case i= %d weight= %f %d + j*%d, Prev %d Next %d \n",
               (i+1),weight, error_est[(i+1)*2],error_est[((i+1)*2)+1], prev,next);
#endif
        i++;
      }
      else
      {
        distance = prev - next;
        candidates = (end_symbol -1) - i;
        weight = 1.0/distance;
        for(int j = 1; j <= candidates; j++)
        {
          error_est[(i+j)*2]    =  8 *(((double)(error_est[prev*2]) * scale * (distance + j) * weight) -
                                       ((double)(error_est[next*2]) * scale * j * weight));
          error_est[((i+j)*2)+1]=  8 *(((double)(error_est[(prev*2)+1]) * scale * (distance + j) * weight) -
                                       ((double)(error_est[((next*2)+1)]) * scale *j * weight));
#ifdef DEBUG_UL_PTRS
          printf("PTRS: INT: Right Edge i= %d weight= %f %d + j*%d, Prev %d Next %d \n",
                 (i+j),weight, error_est[(i+j)*2],error_est[((i+j)*2)+1], prev,next);
#endif
        }
        if(candidates > 1)
        {
          i+=candidates;
        }
      }
    }
    /* Interpolation between 2 estimated points */
    else if(next != 0 && ( error_est[2*i] == 0 ))
    {
      distance = next - prev;
      weight = 1.0/distance;
      candidates = next - i ;
      for(int j = 0; j < candidates; j++)
      {

        error_est[(i+j)*2]    = 8 *(((double)(error_est[prev*2]) * scale * (distance - (j+1)) * weight) +
                                    ((double)(error_est[next*2]) * scale * (j+1) * weight));
        error_est[((i+j)*2)+1]= 8 *(((double)(error_est[(prev*2)+1]) * scale *(distance - (j+1)) * weight) +
                                    ((double)(error_est[((next*2)+1)]) * scale *(j+1) * weight));
#ifdef DEBUG_UL_PTRS
        printf("PTRS: INT: Interpolation i= %d weight= %f %d + j*%d, Prev %d Next %d\n",
               (i+j),weight, error_est[(i+j)*2],error_est[((i+j)*2)+1],prev,next);
#endif
      }
      if(candidates > 1)
      {
        i+=candidates-1;
      }
    }// interpolation
  }// symbol loop
}

/* Find the next non zero Real value in a complex vector */
int nr_ptrs_find_next_estimate(int16_t *error_est,
                               uint8_t counter,
                               uint8_t end_symbol)
{
  for (int i = counter +1 ; i< end_symbol; i++)
  {
    if( error_est[2*i] != 0)
    {
      return i;
    }
  }
  return 0;
}
