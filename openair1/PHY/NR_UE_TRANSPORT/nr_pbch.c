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

/*! \file PHY/LTE_TRANSPORT/pbch.c
* \brief Top-level routines for generating and decoding  the PBCH/BCH physical/transport channel V8.6 2009-03
* \author R. Knopp, F. Kaltenberger
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger.fr
* \note
* \warning
*/
#include "PHY/defs_nr_UE.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/phy_extern_nr_ue.h"
#include "PHY/sse_intrin.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"

//#define DEBUG_PBCH 1
//#define DEBUG_PBCH_ENCODING

#ifdef OPENAIR2
//#include "PHY_INTERFACE/defs.h"
#endif

#define PBCH_A 24

#define print_shorts(s,x) printf("%s : %d,%d,%d,%d,%d,%d,%d,%d\n",s,((int16_t*)x)[0],((int16_t*)x)[1],((int16_t*)x)[2],((int16_t*)x)[3],((int16_t*)x)[4],((int16_t*)x)[5],((int16_t*)x)[6],((int16_t*)x)[7])

uint16_t nr_pbch_extract(int **rxdataF,
			 int **dl_ch_estimates,
			 int **rxdataF_ext,
			 int **dl_ch_estimates_ext,
			 uint32_t symbol,
			 uint32_t high_speed_flag,
			 int is_synchronized,
			 NR_DL_FRAME_PARMS *frame_parms)
{


  uint16_t rb;
  uint8_t i,j,aarx;
  int32_t *dl_ch0,*dl_ch0_ext,*rxF,*rxF_ext;

  int nushiftmod4 = frame_parms->nushift;

  unsigned int  rx_offset = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier; //and
  if (rx_offset>= frame_parms->ofdm_symbol_size) rx_offset-=frame_parms->ofdm_symbol_size;
  int s_offset=0;
 
  AssertFatal(symbol>=1 && symbol<5, 
	      "symbol %d illegal for PBCH extraction\n",
	      symbol);

  if (is_synchronized==1) s_offset=4;

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
        
    rxF        = &rxdataF[aarx][(symbol+s_offset)*frame_parms->ofdm_symbol_size];
    rxF_ext    = &rxdataF_ext[aarx][(symbol+s_offset)*(20*12)];
#ifdef DEBUG_PBCH
     printf("extract_rbs (nushift %d): rx_offset=%d, symbol %d\n",frame_parms->nushift,
	    (rx_offset + ((symbol+s_offset)*(frame_parms->ofdm_symbol_size))),symbol);
     int16_t *p = (int16_t *)rxF;
     for (int i =0; i<8;i++){
        printf("rxF [%d]= %d\n",i,rxF[i]);
        printf("pbch extract rxF  %d %d addr %p\n", p[2*i], p[2*i+1], &p[2*i]);
        printf("rxF ext addr %p\n", &rxF_ext[i]);
     }
#endif

    for (rb=0; rb<20; rb++) {
      j=0;

      if (symbol==1 || symbol==3) {
        for (i=0; i<12; i++) {
          if ((i!=nushiftmod4) &&
              (i!=(nushiftmod4+4)) &&
              (i!=(nushiftmod4+8))) {
            rxF_ext[j]=rxF[rx_offset];
#ifdef DEBUG_PBCH
	    printf("rxF ext[%d] = (%d,%d) rxF [%d]= (%d,%d)\n",(9*rb) + j,
		   ((int16_t*)&rxF_ext[j])[0],
		   ((int16_t*)&rxF_ext[j])[1],
		   rx_offset,
		   ((int16_t*)&rxF[rx_offset])[0],
		   ((int16_t*)&rxF[rx_offset])[1]);
#endif
	    j++;
          }
	  rx_offset=(rx_offset+1)&(frame_parms->ofdm_symbol_size-1);
        }
        rxF_ext+=9;
      } else { //symbol 2
    	if ((rb < 4) || (rb >15)){
    	  for (i=0; i<12; i++) {
	    if ((i!=nushiftmod4) &&
		(i!=(nushiftmod4+4)) &&
		(i!=(nushiftmod4+8))) {
	      rxF_ext[j]=rxF[rx_offset];
#ifdef DEBUG_PBCH
	      printf("rxF ext[%d] = (%d,%d) rxF [%d]= (%d,%d)\n",(rb<4) ? (9*rb) + j : (9*(rb-12))+j,
		     ((int16_t*)&rxF_ext[j])[0],
		     ((int16_t*)&rxF_ext[j])[1],
		     rx_offset,
		     ((int16_t*)&rxF[rx_offset])[0],
		     ((int16_t*)&rxF[rx_offset])[1]);
#endif    
	      j++;
	    }
	    rx_offset=(rx_offset+1)&(frame_parms->ofdm_symbol_size-1);
    	  }
	  rxF_ext+=9;
	}
	else rx_offset = (rx_offset+12)&(frame_parms->ofdm_symbol_size-1);
        
      }
    }

    if (high_speed_flag == 1)
      dl_ch0     = &dl_ch_estimates[aarx][((symbol+s_offset)*(frame_parms->ofdm_symbol_size))];
    else
      dl_ch0     = &dl_ch_estimates[aarx][0];
    
    //printf("dl_ch0 addr %p\n",dl_ch0);
    
    dl_ch0_ext = &dl_ch_estimates_ext[aarx][(symbol+s_offset)*(20*12)];
    
    for (rb=0; rb<20; rb++) {
      j=0;
      if (symbol==1 || symbol==3) {
	for (i=0; i<12; i++) {
	  if ((i!=nushiftmod4) &&
	      (i!=(nushiftmod4+4)) &&
	      (i!=(nushiftmod4+8))) {
	    dl_ch0_ext[j]=dl_ch0[i];
#ifdef DEBUG_PBCH
	    if ((rb==0) && (i<2))
	      printf("dl ch0 ext[%d] = (%d,%d)  dl_ch0 [%d]= (%d,%d)\n",j,
		     ((int16_t*)&dl_ch0_ext[j])[0],
		     ((int16_t*)&dl_ch0_ext[j])[1],
		     i,
		     ((int16_t*)&dl_ch0[i])[0],
		     ((int16_t*)&dl_ch0[i])[1]);
#endif
	    j++;
	  }
	}
	
	dl_ch0+=12;
	dl_ch0_ext+=9;
      }
      else { 
	if ((rb < 4) || (rb >15)){
	  for (i=0; i<12; i++) {
	    if ((i!=nushiftmod4) &&
		(i!=(nushiftmod4+4)) &&
		(i!=(nushiftmod4+8))) {
	      dl_ch0_ext[j]=dl_ch0[i];
#ifdef DEBUG_PBCH
	      printf("dl ch0 ext[%d] = (%d,%d)  dl_ch0 [%d]= (%d,%d)\n",j,
		     ((int16_t*)&dl_ch0_ext[j])[0],
		     ((int16_t*)&dl_ch0_ext[j])[1],
		     i,
		     ((int16_t*)&dl_ch0[i])[0],
		     ((int16_t*)&dl_ch0[i])[1]);
#endif
	      j++;
	    }
	  }
	  dl_ch0_ext+=9;
	}
	dl_ch0+=12;
      }
    }
  }

  return(0);
}

//__m128i avg128;

//compute average channel_level on each (TX,RX) antenna pair
int nr_pbch_channel_level(int **dl_ch_estimates_ext,
			  NR_DL_FRAME_PARMS *frame_parms,
			  uint32_t symbol)
{

  int16_t rb, nb_rb=20;
  uint8_t aarx;

#if defined(__x86_64__) || defined(__i386__)
  __m128i avg128;
  __m128i *dl_ch128;
#elif defined(__arm__)
  int32x4_t avg128;
  int16x8_t *dl_ch128;
#endif
  int avg1=0,avg2=0;

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    //clear average level
    
#if defined(__x86_64__) || defined(__i386__)
    avg128 = _mm_setzero_si128();
    dl_ch128=(__m128i *)&dl_ch_estimates_ext[aarx][symbol*20*12];
#elif defined(__arm__)
    avg128 = vdupq_n_s32(0);
    dl_ch128=(int16x8_t *)&dl_ch_estimates_ext[aarx][symbol*20*12];
    
#endif
    for (rb=0; rb<nb_rb; rb++) {
#if defined(__x86_64__) || defined(__i386__)
      avg128 = _mm_add_epi32(avg128,_mm_madd_epi16(dl_ch128[0],dl_ch128[0]));
      avg128 = _mm_add_epi32(avg128,_mm_madd_epi16(dl_ch128[1],dl_ch128[1]));
      avg128 = _mm_add_epi32(avg128,_mm_madd_epi16(dl_ch128[2],dl_ch128[2]));
#elif defined(__arm__)
// to be filled in
#endif
      dl_ch128+=3;
      /*
      if (rb==0) {
	print_shorts("dl_ch128",&dl_ch128[0]);
	print_shorts("dl_ch128",&dl_ch128[1]);
	print_shorts("dl_ch128",&dl_ch128[2]);
	}*/
      
    }
    
    avg1 = (((int*)&avg128)[0] +
	    ((int*)&avg128)[1] +
	    ((int*)&avg128)[2] +
	    ((int*)&avg128)[3])/(nb_rb*12);
    
    if (avg1>avg2)
      avg2 = avg1;
    
    //LOG_I(PHY,"Channel level : %d, %d\n",avg1, avg2);
  }
#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
  return(avg2);

}

void nr_pbch_channel_compensation(int **rxdataF_ext,
				  int **dl_ch_estimates_ext,
				  int **rxdataF_comp,
				  NR_DL_FRAME_PARMS *frame_parms,
				  uint8_t symbol,
				  int is_synchronized,
				  uint8_t output_shift)
{

  short conjugate[8]__attribute__((aligned(16))) = {-1,1,-1,1,-1,1,-1,1};
  //short conjugate2[8]__attribute__((aligned(16))) = {1,-1,1,-1,1,-1,1,-1};
#if defined(__x86_64__) || defined(__i386__)
  __m128i mmtmpP0,mmtmpP1,mmtmpP2,mmtmpP3;
#elif defined(__arm__)
  int16x8_t mmtmpP0,mmtmpP1,mmtmpP2,mmtmpP3;
#endif

  uint16_t nb_re=180;
  uint8_t aarx;

#if defined(__x86_64__) || defined(__i386__)
  __m128i *dl_ch128,*rxdataF128,*rxdataF_comp128;
#elif defined(__arm__)

#endif

  AssertFatal((symbol > 0 && symbol < 4 && is_synchronized == 0) || 
	      (symbol > 4 && symbol < 8 && is_synchronized == 1),
	      "symbol %d is illegal for PBCH DM-RS (is_synchronized %d)\n",
	      symbol,is_synchronized);



  if (symbol == 2 || symbol == 6) nb_re = 72;

  //  printf("comp: symbol %d : nb_re %d\n",symbol,nb_re);

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    
#if defined(__x86_64__) || defined(__i386__)
    dl_ch128          = (__m128i *)&dl_ch_estimates_ext[aarx][symbol*20*12];
    rxdataF128        = (__m128i *)&rxdataF_ext[aarx][symbol*20*12];
    rxdataF_comp128   = (__m128i *)&rxdataF_comp[aarx][symbol*20*12];
    /*
    printf("ch compensation dl_ch ext addr %p \n", &dl_ch_estimates_ext[aarx][symbol*20*12]);
    printf("rxdataf ext addr %p symbol %d\n", &rxdataF_ext[aarx][symbol*20*12], symbol);
    printf("rxdataf_comp addr %p\n",&rxdataF_comp[aarx][symbol*20*12]); 
    */
#elif defined(__arm__)
// to be filled in
#endif

    for (int re=0; re<nb_re; re+=12) {
      //            printf("******re %d\n",re);
#if defined(__x86_64__) || defined(__i386__)
      // multiply by conjugated channel
      mmtmpP0 = _mm_madd_epi16(dl_ch128[0],rxdataF128[0]);
      //  print_ints("re",&mmtmpP0);
      // mmtmpP0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpP1 = _mm_shufflelo_epi16(dl_ch128[0],_MM_SHUFFLE(2,3,0,1));
      mmtmpP1 = _mm_shufflehi_epi16(mmtmpP1,_MM_SHUFFLE(2,3,0,1));
      mmtmpP1 = _mm_sign_epi16(mmtmpP1,*(__m128i*)&conjugate[0]);
      //  print_ints("im",&mmtmpP1);
      mmtmpP1 = _mm_madd_epi16(mmtmpP1,rxdataF128[0]);
      // mmtmpP1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpP0 = _mm_srai_epi32(mmtmpP0,output_shift);
      //  print_ints("re(shift)",&mmtmpP0);
      mmtmpP1 = _mm_srai_epi32(mmtmpP1,output_shift);
      //  print_ints("im(shift)",&mmtmpP1);
      mmtmpP2 = _mm_unpacklo_epi32(mmtmpP0,mmtmpP1);
      mmtmpP3 = _mm_unpackhi_epi32(mmtmpP0,mmtmpP1);
      //      print_ints("c0",&mmtmpP2);
      //  print_ints("c1",&mmtmpP3);
      rxdataF_comp128[0] = _mm_packs_epi32(mmtmpP2,mmtmpP3);
      /*
        print_shorts("rx:",rxdataF128);
        print_shorts("ch:",dl_ch128);
        print_shorts("pack:",rxdataF_comp128);
      */
      // multiply by conjugated channel
      mmtmpP0 = _mm_madd_epi16(dl_ch128[1],rxdataF128[1]);
      // mmtmpP0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpP1 = _mm_shufflelo_epi16(dl_ch128[1],_MM_SHUFFLE(2,3,0,1));
      mmtmpP1 = _mm_shufflehi_epi16(mmtmpP1,_MM_SHUFFLE(2,3,0,1));
      mmtmpP1 = _mm_sign_epi16(mmtmpP1,*(__m128i*)&conjugate[0]);
      mmtmpP1 = _mm_madd_epi16(mmtmpP1,rxdataF128[1]);
      // mmtmpP1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpP0 = _mm_srai_epi32(mmtmpP0,output_shift);
      mmtmpP1 = _mm_srai_epi32(mmtmpP1,output_shift);
      mmtmpP2 = _mm_unpacklo_epi32(mmtmpP0,mmtmpP1);
      mmtmpP3 = _mm_unpackhi_epi32(mmtmpP0,mmtmpP1);
      rxdataF_comp128[1] = _mm_packs_epi32(mmtmpP2,mmtmpP3);
      //  print_shorts("rx:",rxdataF128+1);
      //  print_shorts("ch:",dl_ch128+1);
      //  print_shorts("pack:",rxdataF_comp128+1);
      
      // multiply by conjugated channel
      mmtmpP0 = _mm_madd_epi16(dl_ch128[2],rxdataF128[2]);
      // mmtmpP0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpP1 = _mm_shufflelo_epi16(dl_ch128[2],_MM_SHUFFLE(2,3,0,1));
      mmtmpP1 = _mm_shufflehi_epi16(mmtmpP1,_MM_SHUFFLE(2,3,0,1));
      mmtmpP1 = _mm_sign_epi16(mmtmpP1,*(__m128i*)&conjugate[0]);
      mmtmpP1 = _mm_madd_epi16(mmtmpP1,rxdataF128[2]);
      // mmtmpP1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpP0 = _mm_srai_epi32(mmtmpP0,output_shift);
      mmtmpP1 = _mm_srai_epi32(mmtmpP1,output_shift);
      mmtmpP2 = _mm_unpacklo_epi32(mmtmpP0,mmtmpP1);
      mmtmpP3 = _mm_unpackhi_epi32(mmtmpP0,mmtmpP1);
      rxdataF_comp128[2] = _mm_packs_epi32(mmtmpP2,mmtmpP3);
      //  print_shorts("rx:",rxdataF128+2);
      //  print_shorts("ch:",dl_ch128+2);
      //      print_shorts("pack:",rxdataF_comp128+2);
      
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

void nr_pbch_detection_mrc(NR_DL_FRAME_PARMS *frame_parms,
                        int **rxdataF_comp,
                        uint8_t symbol)
{

  uint8_t symbol_mod;
  int i, nb_rb=6;
#if defined(__x86_64__) || defined(__i386__)
  __m128i *rxdataF_comp128_0,*rxdataF_comp128_1;
#elif defined(__arm__)
  int16x8_t *rxdataF_comp128_0,*rxdataF_comp128_1;
#endif
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if (frame_parms->nb_antennas_rx>1) {
#if defined(__x86_64__) || defined(__i386__)
      rxdataF_comp128_0   = (__m128i *)&rxdataF_comp[0][symbol_mod*6*12];
      rxdataF_comp128_1   = (__m128i *)&rxdataF_comp[1][symbol_mod*6*12];
#elif defined(__arm__)
      rxdataF_comp128_0   = (int16x8_t *)&rxdataF_comp[0][symbol_mod*6*12];
      rxdataF_comp128_1   = (int16x8_t *)&rxdataF_comp[1][symbol_mod*6*12];

#endif
      // MRC on each re of rb, both on MF output and magnitude (for 16QAM/64QAM llr computation)
      for (i=0; i<nb_rb*3; i++) {
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

void nr_pbch_unscrambling(NR_UE_PBCH *pbch,
			  uint16_t Nid,
			  uint8_t nushift,
			  uint16_t M,
			  uint16_t length,
			  uint8_t bitwise)
{
  uint8_t reset, offset;
  uint32_t x1, x2, s=0;
  int16_t *demod_pbch_e = pbch->llr;

  uint32_t unscrambling_mask = 0x100006D;


  reset = 1;
  // x1 is set in first call to lte_gold_generic
  x2 = Nid; //this is c_init

  // The Gold sequence is shifted by nushift* M, so we skip (nushift*M /32) double words
  for (int i=0; i<(uint16_t)ceil(((float)nushift*M)/32); i++) {
    s = lte_gold_generic(&x1, &x2, reset);
    reset = 0;
  }
  // Scrambling is now done with offset (nushift*M)%32
  offset = (nushift*M)&0x1f;

  for (int i=0; i<length; i++) {
      if (((i+offset)&0x1f)==0) {
        s = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }
#ifdef DEBUG_PBCH_ENCODING
      if (i<8)
	printf("s: %04x\t", s);
      printf("pbch_a_interleaved 0x%08x\n", pbch->pbch_a_interleaved);
#endif
      if (bitwise) {
	
        (pbch->pbch_a_interleaved) ^= ((unscrambling_mask>>i)&1)? ((pbch->pbch_a_prime>>i)&1)<<i : (((pbch->pbch_a_prime>>i)&1) ^ ((s>>((i+offset)&0x1f))&1))<<i;
      }

      else {
    	  if (((s>>((i+offset)&0x1f))&1)==1)
    		  demod_pbch_e[i] = -demod_pbch_e[i];
#ifdef DEBUG_PBCH_ENCODING
		if (i<8)
	printf("s %d demod_pbch_e[i] %d\n", ((s>>((i+offset)&0x1f))&1), demod_pbch_e[i]);
#endif
      }
  }

}

void nr_pbch_quantize(int16_t *pbch_llr8,
                      int16_t *pbch_llr,
                      uint16_t len)
{

  uint16_t i;

  for (i=0; i<len; i++) {
    if (pbch_llr[i]>31)
      pbch_llr8[i]=32;
    else if (pbch_llr[i]<-31)
      pbch_llr8[i]=-32;
    else
      pbch_llr8[i] = (char)(pbch_llr[i]);

  }
}
/*
unsigned char sign(int8_t x) {
  return (unsigned char)x >> 7;
}
*/

uint8_t pbch_deinterleaving_pattern[32] = {28,0,31,30,1,29,25,27,22,2,24,3,4,5,6,7,18,21,20,8,9,10,11,19,26,12,13,14,15,16,23,17};

int nr_rx_pbch( PHY_VARS_NR_UE *ue,
		UE_nr_rxtx_proc_t *proc,
		NR_UE_PBCH *nr_ue_pbch_vars,
		NR_DL_FRAME_PARMS *frame_parms,
		uint8_t eNB_id,
		MIMO_mode_t mimo_mode,
		uint32_t high_speed_flag)
{

  NR_UE_COMMON *nr_ue_common_vars = &ue->common_vars;

  int max_h=0;

  int symbol,i;
  //uint8_t pbch_a[64];
  uint8_t *pbch_a = malloc(sizeof(uint8_t) * 32);
  uint32_t pbch_a_prime;
  int16_t *pbch_e_rx;
  uint8_t *decoded_output = nr_ue_pbch_vars->decoded_output;
  uint8_t nushift;
  uint16_t M;
  uint8_t Lmax=8; //to update
  uint8_t ssb_index=0;
  //uint16_t crc;
  unsigned short idx_demod =0;
  int8_t decoderState=0;
  uint8_t decoderListSize = 8, pathMetricAppr = 0;

  time_stats_t polar_decoder_init,polar_rate_matching,decoding,bit_extraction,deinterleaving;
  time_stats_t path_metric,sorting,update_LLR;
  memset(&pbch_a[0], 0, sizeof(uint8_t) * NR_POLAR_PBCH_PAYLOAD_BITS);

  //printf("nr_pbch_ue nid_cell %d\n",frame_parms->Nid_cell);


  int subframe_rx = proc->subframe_rx;
  
  pbch_e_rx = &nr_ue_pbch_vars->llr[0];

  // clear LLR buffer
  memset(nr_ue_pbch_vars->llr,0,NR_POLAR_PBCH_E);

  int first_symbol=1;
  if (ue->is_synchronized > 0) first_symbol+=4;

#ifdef DEBUG_PBCH
    //printf("address dataf %p",nr_ue_common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe_rx]].rxdataF);
    write_output("rxdataF0_pbch.m","rxF0pbch",
		 &nr_ue_common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe_rx]].rxdataF[0][first_symbol*frame_parms->ofdm_symbol_size],frame_parms->ofdm_symbol_size*3,1,1);
#endif

  for (symbol=first_symbol; symbol<(first_symbol+3); symbol++) {

    nr_pbch_extract(nr_ue_common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe_rx]].rxdataF,
		    nr_ue_common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe_rx]].dl_ch_estimates[eNB_id],
		    nr_ue_pbch_vars->rxdataF_ext,
		    nr_ue_pbch_vars->dl_ch_estimates_ext,
		    symbol-first_symbol+1,
		    high_speed_flag,
		    ue->is_synchronized,
		    frame_parms);
#ifdef DEBUG_PBCH
    LOG_I(PHY,"[PHY] PBCH Symbol %d ofdm size %d\n",symbol, frame_parms->ofdm_symbol_size );
    LOG_I(PHY,"[PHY] PBCH starting channel_level\n");
#endif

    if (symbol == 1 || symbol == 5) {
      max_h = nr_pbch_channel_level(nr_ue_pbch_vars->dl_ch_estimates_ext,
				    frame_parms,
				    symbol);
      nr_ue_pbch_vars->log2_maxh = 3+(log2_approx(max_h)/2);
    }

#ifdef DEBUG_PBCH
    LOG_I(PHY,"[PHY] PBCH log2_maxh = %d (%d)\n",nr_ue_pbch_vars->log2_maxh,max_h);
#endif

    nr_pbch_channel_compensation(nr_ue_pbch_vars->rxdataF_ext,
				 nr_ue_pbch_vars->dl_ch_estimates_ext,
				 nr_ue_pbch_vars->rxdataF_comp,
				 frame_parms,
				 symbol,
				 ue->is_synchronized,
				 nr_ue_pbch_vars->log2_maxh); // log2_maxh+I0_shift

    /*if (frame_parms->nb_antennas_rx > 1)
      pbch_detection_mrc(frame_parms,
                         nr_ue_pbch_vars->rxdataF_comp,
                         symbol);*/

/*
    if (mimo_mode == ALAMOUTI) {
      nr_pbch_alamouti(frame_parms,nr_ue_pbch_vars->rxdataF_comp,symbol);
    } else if (mimo_mode != SISO) {
      LOG_I(PHY,"[PBCH][RX] Unsupported MIMO mode\n");
      return(-1);
    }
*/
    if (symbol==(first_symbol+1)) {
      nr_pbch_quantize(pbch_e_rx,
                    (short*)&(nr_ue_pbch_vars->rxdataF_comp[0][symbol*240]),
                    144);

      pbch_e_rx+=144;
    } else {
      nr_pbch_quantize(pbch_e_rx,
                    (short*)&(nr_ue_pbch_vars->rxdataF_comp[0][symbol*240]),
                    360);

      pbch_e_rx+=360;
    }


  }

#ifdef DEBUG_PBCH
  write_output("rxdataF_comp.m","rxFcomp",&nr_ue_pbch_vars->rxdataF_comp[0][240*first_symbol],240*3,1,1);
#endif
    

  pbch_e_rx = nr_ue_pbch_vars->llr;
  //demod_pbch_e = nr_ue_pbch_vars->demod_pbch_e;
  pbch_a = nr_ue_pbch_vars->pbch_a;

#ifdef DEBUG_PBCH
  //pbch_e_rx = &nr_ue_pbch_vars->llr[0];

  short *p = (short *)&(nr_ue_pbch_vars->rxdataF_comp[0][first_symbol*20*12]);
  for (int cnt = 0; cnt < 864  ; cnt++)
    printf("pbch rx llr %d\n",*(pbch_e_rx+cnt));

#endif


  //un-scrambling
  M =  NR_POLAR_PBCH_E;
  nushift = (Lmax==4)? ssb_index&3 : ssb_index&7;
  nr_pbch_unscrambling(nr_ue_pbch_vars,frame_parms->Nid_cell,nushift,M,NR_POLAR_PBCH_E,0);



  //polar decoding de-rate matching


  AssertFatal(ue->nrPolar_params != NULL,"ue->nrPolar_params is null\n");

  t_nrPolar_params *currentPtr = nr_polar_params(ue->nrPolar_params, NR_POLAR_PBCH_MESSAGE_TYPE, NR_POLAR_PBCH_PAYLOAD_BITS, NR_POLAR_PBCH_AGGREGATION_LEVEL);

  decoderState = polar_decoder_int16(pbch_e_rx,(uint8_t*)&nr_ue_pbch_vars->pbch_a_prime,currentPtr);


  if(decoderState == -1)
  	return(decoderState);
  	
  	//printf("polar decoder output 0x%08x\n",nr_ue_pbch_vars->pbch_a_prime);
  
  //payload un-scrambling
  memset(&nr_ue_pbch_vars->pbch_a_interleaved, 0, sizeof(uint32_t) );
  M = (Lmax == 64)? (NR_POLAR_PBCH_PAYLOAD_BITS - 6) : (NR_POLAR_PBCH_PAYLOAD_BITS - 3);
  nushift = ((nr_ue_pbch_vars->pbch_a_prime>>6)&1) ^ (((nr_ue_pbch_vars->pbch_a_prime>>24)&1)<<1);
  nr_pbch_unscrambling(nr_ue_pbch_vars,frame_parms->Nid_cell,nushift,M,NR_POLAR_PBCH_PAYLOAD_BITS,1);

  //payload deinterleaving
  //uint32_t in=0;
  uint32_t out=0;

  for (int i=0; i<32; i++) {
    out |= ((nr_ue_pbch_vars->pbch_a_interleaved>>i)&1)<<(pbch_deinterleaving_pattern[i]);
#ifdef DEBUG_PBCH
    printf("i %d in 0x%08x out 0x%08x ilv %d (in>>i)&1) 0x%08x\n", i, nr_ue_pbch_vars->pbch_a_interleaved, out, pbch_deinterleaving_pattern[i], (nr_ue_pbch_vars->pbch_a_interleaved>>i)&1);
#endif
  }

  for (int i=0; i<NR_POLAR_PBCH_PAYLOAD_BITS>>3; i++)
	  decoded_output[i] = (uint8_t)((out>>(i<<3))&0xff);

  // Fix byte endian
  //  for (i=0; i<(NR_POLAR_PBCH_PAYLOAD_BITS>>3); i++)
  //     decoded_output[(NR_POLAR_PBCH_PAYLOAD_BITS>>3)-i-1] = pbch_a[i];
     
#ifdef DEBUG_PBCH
  for (i=0; i<(NR_POLAR_PBCH_PAYLOAD_BITS>>3); i++){
    //  	  printf("unscrambling pbch_a[%d] = %x \n", i,pbch_a[i]);
  	  printf("[PBCH] decoder payload[%d] = %x\n",i,decoded_output[i]);
  }
#endif

    ue->dl_indication.rx_ind = &ue->rx_ind; //  hang on rx_ind instance
    //ue->rx_ind.sfn_slot = 0;  //should be set by higher-1-layer, i.e. clean_and_set_if_instance()
    ue->rx_ind.number_pdus = ue->rx_ind.number_pdus + 1;
    ue->rx_ind.rx_indication_body = (fapi_nr_rx_indication_body_t *)malloc(sizeof(fapi_nr_rx_indication_body_t));
    ue->rx_ind.rx_indication_body->pdu_type = FAPI_NR_RX_PDU_TYPE_MIB;
    ue->rx_ind.rx_indication_body->mib_pdu.pdu = &decoded_output[0];
    ue->rx_ind.rx_indication_body->mib_pdu.additional_bits = decoded_output[3];
    ue->rx_ind.rx_indication_body->mib_pdu.ssb_index = ssb_index;            //  confirm with TCL
    ue->rx_ind.rx_indication_body->mib_pdu.ssb_length = Lmax;                //  confirm with TCL
    ue->rx_ind.rx_indication_body->mib_pdu.cell_id = frame_parms->Nid_cell;  //  confirm with TCL

    if (ue->if_inst && ue->if_inst->dl_indication)
      ue->if_inst->dl_indication(&ue->dl_indication);

    return 0;    
}
