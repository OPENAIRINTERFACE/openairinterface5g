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
#include "SIMULATION/TOOLS/sim.h"

//#define DEBUG_PBCH 1
//#define DEBUG_PBCH_ENCODING

#ifdef OPENAIR2
//#include "PHY_INTERFACE/defs.h"
#endif

#define PBCH_A 24


uint16_t nr_pbch_extract(int **rxdataF,
                      int **dl_ch_estimates,
                      int **rxdataF_ext,
                      int **dl_ch_estimates_ext,
                      uint32_t symbol,
                      uint32_t high_speed_flag,
                      NR_DL_FRAME_PARMS *frame_parms)
{


  uint16_t rb;
  uint8_t i,j,aarx,aatx;
  int32_t *dl_ch0,*dl_ch0_ext,*rxF,*rxF_ext;

  int rx_offset = frame_parms->ofdm_symbol_size-10*12;
  int nushiftmod4 = frame_parms->nushift;

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
        
    rxF        = &rxdataF[aarx][(rx_offset + (symbol*(frame_parms->ofdm_symbol_size)))];
    rxF_ext    = &rxdataF_ext[aarx][symbol*(20*12)];
#ifdef DEBUG_PBCH
     printf("extract_rbs (nushift %d): rx_offset=%d, symbol %d\n",frame_parms->nushift,
     (rx_offset + (symbol*(frame_parms->ofdm_symbol_size))),symbol);
     int16_t *p = (int16_t *)rxF;
     for (int i =0; i<8;i++){
        printf("rxF [%d]= %d\n",i,rxF[i]);
        printf("pbch extract rxF  %d %d addr %p\n", p[2*i], p[2*i+1], &p[2*i]);
        printf("rxF ext addr %p\n", &rxF_ext[i]);
     }
#endif

    for (rb=0; rb<20; rb++) {
      if (rb==10) {
        rxF       = &rxdataF[aarx][((symbol*(frame_parms->ofdm_symbol_size)))];
      }

      j=0;
      if ((symbol==1) || (symbol==3)) {
        for (i=0; i<12; i++) {
          if ((i!=nushiftmod4) &&
              (i!=(nushiftmod4+4)) &&
              (i!=(nushiftmod4+8))) {
            rxF_ext[j]=rxF[i];
	    //printf("rxF ext[%d] = %d rxF [%d]= %d\n",j,rxF_ext[j],i,rxF[i]);
	    j++;
          }
        }

        rxF+=12;
        rxF_ext+=9;
      } else { //symbol 2
    	if ((rb < 4) || (rb >15)){
    	  for (i=0; i<12; i++) {
        	if ((i!=nushiftmod4) &&
        	    (i!=(nushiftmod4+4)) &&
        	    (i!=(nushiftmod4+8))) {
        	  rxF_ext[j]=rxF[i];
		  //printf("symbol2 rxF ext[%d] = %d at %p\n",j,rxF_ext[j],&rxF[i]);
		  j++;
        	}
    	  }
	}

        rxF+=12;
        rxF_ext+=9;
      }
    }

    for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB;aatx++) {
      if (high_speed_flag == 1)
        dl_ch0     = &dl_ch_estimates[(aatx<<1)+aarx][(symbol*(frame_parms->ofdm_symbol_size))];
      else
        dl_ch0     = &dl_ch_estimates[(aatx<<1)+aarx][0];

      //printf("dl_ch0 addr %p\n",dl_ch0);

      dl_ch0_ext = &dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*(20*12)];

      for (rb=0; rb<20; rb++) {
	j=0;
        if ((symbol==1) || (symbol==3)) {
              	  for (i=0; i<12; i++) {
                    if ((i!=nushiftmod4) &&
                        (i!=(nushiftmod4+4)) &&
                        (i!=(nushiftmod4+8))) {
                           dl_ch0_ext[j]=dl_ch0[i];
			   //if ((rb==0) && (i<2))
			     //printf("dl ch0 ext[%d] = %d dl_ch0 [%d]= %d\n",j,dl_ch0_ext[j],i,dl_ch0[i]);
		           j++;
                    }
          	  }

          dl_ch0+=12;
          dl_ch0_ext+=9;
        }
        else { //symbol 2
              if ((rb < 4) || (rb >15)){
              	  for (i=0; i<12; i++) {
                    if ((i!=nushiftmod4) &&
                        (i!=(nushiftmod4+4)) &&
                        (i!=(nushiftmod4+8))) {
		           dl_ch0_ext[j]=dl_ch0[i];
			   //printf("symbol2 dl ch0 ext[%d] = %d dl_ch0 [%d]= %d\n",j,dl_ch0_ext[j],i,dl_ch0[i]);
			   j++;
                       	}
               	  }
              }

              dl_ch0+=12;
              dl_ch0_ext+=9;

         }
      }
    }  //tx antenna loop

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
  uint8_t aatx,aarx;

#if defined(__x86_64__) || defined(__i386__)
  __m128i avg128;
  __m128i *dl_ch128;
#elif defined(__arm__)
  int32x4_t avg128;
  int16x8_t *dl_ch128;
#endif
  int avg1=0,avg2=0;

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB;aatx++)
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      //clear average level

#if defined(__x86_64__) || defined(__i386__)
      avg128 = _mm_setzero_si128();
      dl_ch128=(__m128i *)&dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*20*12];
#elif defined(__arm__)
      avg128 = vdupq_n_s32(0);
      dl_ch128=(int16x8_t *)&dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*20*12];

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
          }
        */
      }

      avg1 = (((int*)&avg128)[0] +
              ((int*)&avg128)[1] +
              ((int*)&avg128)[2] +
              ((int*)&avg128)[3])/(nb_rb*12);

      if (avg1>avg2)
        avg2 = avg1;

      //msg("Channel level : %d, %d\n",avg1, avg2);
    }
#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
  return(avg2);

}

#if defined(__x86_64__) || defined(__i386__)
__m128i mmtmpP0,mmtmpP1,mmtmpP2,mmtmpP3;
#elif defined(__arm__)
int16x8_t mmtmpP0,mmtmpP1,mmtmpP2,mmtmpP3;
#endif
void nr_pbch_channel_compensation(int **rxdataF_ext,
                               int **dl_ch_estimates_ext,
                               int **rxdataF_comp,
                               NR_DL_FRAME_PARMS *frame_parms,
                               uint8_t symbol,
                               uint8_t output_shift)
{

  uint16_t rb,nb_rb=20;
  uint8_t aatx,aarx;
#if defined(__x86_64__) || defined(__i386__)
  __m128i *dl_ch128,*rxdataF128,*rxdataF_comp128;
#elif defined(__arm__)

#endif

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB;aatx++)
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

#if defined(__x86_64__) || defined(__i386__)
      dl_ch128          = (__m128i *)&dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*20*12];
      rxdataF128        = (__m128i *)&rxdataF_ext[aarx][symbol*20*12];
      rxdataF_comp128   = (__m128i *)&rxdataF_comp[(aatx<<1)+aarx][symbol*20*12];
      //printf("ch compensation dl_ch ext addr %p \n", &dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*20*12]);
      //printf("rxdataf ext addr %p symbol %d\n", &rxdataF_ext[aarx][symbol*20*12], symbol);
      //printf("rxdataf_comp addr %p\n",&rxdataF_comp[(aatx<<1)+aarx][symbol*20*12]); 

#elif defined(__arm__)
// to be filled in
#endif

      for (rb=0; rb<nb_rb; rb++) {
        //printf("rb %d\n",rb);
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
        //  print_shorts("rx:",rxdataF128);
        //  print_shorts("ch:",dl_ch128);
        //  print_shorts("pack:",rxdataF_comp128);

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

  uint8_t aatx, symbol_mod;
  int i, nb_rb=6;
#if defined(__x86_64__) || defined(__i386__)
  __m128i *rxdataF_comp128_0,*rxdataF_comp128_1;
#elif defined(__arm__)
  int16x8_t *rxdataF_comp128_0,*rxdataF_comp128_1;
#endif
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if (frame_parms->nb_antennas_rx>1) {
    for (aatx=0; aatx<4; aatx++) { //frame_parms->nb_antenna_ports_eNB;aatx++) {
#if defined(__x86_64__) || defined(__i386__)
      rxdataF_comp128_0   = (__m128i *)&rxdataF_comp[(aatx<<1)][symbol_mod*6*12];
      rxdataF_comp128_1   = (__m128i *)&rxdataF_comp[(aatx<<1)+1][symbol_mod*6*12];
#elif defined(__arm__)
      rxdataF_comp128_0   = (int16x8_t *)&rxdataF_comp[(aatx<<1)][symbol_mod*6*12];
      rxdataF_comp128_1   = (int16x8_t *)&rxdataF_comp[(aatx<<1)+1][symbol_mod*6*12];

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
  }
#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}

void nr_pbch_unscrambling(NR_DL_FRAME_PARMS *frame_parms,
                       uint8_t* pbch_a,
                       uint32_t length)
{
  int i;
  uint8_t reset;
  uint32_t x1, x2, s=0;

  printf("unscramb nid_cell %d\n",frame_parms->Nid_cell);

  reset = 1;
  // x1 is set in first call to lte_gold_generic
  x2 = frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.6.1
  //  msg("pbch_unscrambling: Nid_cell = %d\n",x2);

  for (i=0; i<length; i++) {
    if (i%32==0) {
      s = lte_gold_generic(&x1, &x2, reset);
            printf("lte_gold[%d]=%x\n",i,s);
      reset = 0;
    }

    printf("s = %d\n",((s>>(i%32))&1) );
    if (((s>>(i%32))&1)==1)
      pbch_a[i] = 1-pbch_a[i];
  }
}

void nr_pbch_alamouti(NR_DL_FRAME_PARMS *frame_parms,
                   int **rxdataF_comp,
                   uint8_t symbol)
{


  int16_t *rxF0,*rxF1;
  //  __m128i *ch_mag0,*ch_mag1,*ch_mag0b,*ch_mag1b;
  uint8_t rb,re,symbol_mod;
  int jj;

  //  printf("Doing alamouti\n");
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;
  jj         = (symbol_mod*6*12);

  rxF0     = (int16_t*)&rxdataF_comp[0][jj];  //tx antenna 0  h0*y
  rxF1     = (int16_t*)&rxdataF_comp[2][jj];  //tx antenna 1  h1*y

  for (rb=0; rb<6; rb++) {

    for (re=0; re<12; re+=2) {

      // Alamouti RX combining

      rxF0[0] = rxF0[0] + rxF1[2];
      rxF0[1] = rxF0[1] - rxF1[3];

      rxF0[2] = rxF0[2] - rxF1[0];
      rxF0[3] = rxF0[3] + rxF1[1];

      rxF0+=4;
      rxF1+=4;
    }

  }

}

void nr_pbch_quantize(int8_t *pbch_llr8,
                   int16_t *pbch_llr,
                   uint16_t len)
{

  uint16_t i;

  for (i=0; i<len; i++) {
    /*if (pbch_llr[i]>7)
      pbch_llr8[i]=7;
    else if (pbch_llr[i]<-8)
      pbch_llr8[i]=-8;
    else*/
      pbch_llr8[i] = (char)(pbch_llr[i]);

  }
}

unsigned short sign(short x) {
  return (unsigned short)x >> 15;
}

uint16_t nr_rx_pbch( PHY_VARS_NR_UE *ue,
		     UE_nr_rxtx_proc_t *proc,
		     NR_UE_PBCH *nr_ue_pbch_vars,
		     NR_DL_FRAME_PARMS *frame_parms,
		     uint8_t eNB_id,
		     MIMO_mode_t mimo_mode,
		     uint32_t high_speed_flag,
		     uint8_t frame_mod4)
{

  NR_UE_COMMON *nr_ue_common_vars = &ue->common_vars;

  uint8_t log2_maxh;
  int max_h=0;

  int symbol,i;
  //uint8_t pbch_a[64];
  uint8_t *pbch_a = malloc(sizeof(uint8_t) * 32); ;

  int8_t *pbch_e_rx;
  uint8_t *decoded_output = nr_ue_pbch_vars->decoded_output;
  uint16_t crc;
  //short nr_demod_table[8] = {0,0,0,1,1,0,1,1};
  double nr_demod_table[8] = {0.707,0.707,0.707,-0.707,-0.707,0.707,-0.707,-0.707};
  double *demod_pbch_e  = malloc (sizeof(double) * 864); 
  unsigned short idx_demod =0;
  int8_t decoderState=0;
  uint8_t decoderListSize = 8, pathMetricAppr = 0;
  double aPrioriArray[frame_parms->pbch_polar_params.payloadBits];  // assume no a priori knowledge available about the payload.
  double *channelOutput  = malloc (sizeof(double) * 864); //add noise
  uint8_t *estimatedOutput = malloc(sizeof(uint8_t) * 32); //decoder output
  memset(&channelOutput[0], 0, sizeof(double) * 864);
  memset(&estimatedOutput[0], 0, sizeof(uint8_t) * 32);

  memset(&pbch_a[0], 0, sizeof(uint8_t) * 32);
  frame_parms->Nid_cell = 100;

  printf("nr_pbch_ue nid_cell %d\n",frame_parms->Nid_cell);

  for (int i=0; i<frame_parms->pbch_polar_params.payloadBits; i++) aPrioriArray[i] = NAN;

  int subframe_rx = proc->subframe_rx;

  pbch_e_rx = &nr_ue_pbch_vars->llr[0];

  // clear LLR buffer
  memset(nr_ue_pbch_vars->llr,0,NR_POLAR_PBCH_E);

  for (symbol=1; symbol<4; symbol++) {

    //printf("address dataf %p",nr_ue_common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe_rx]].rxdataF);
    //write_output("rxdataF0_pbch.m","rxF0pbch",nr_ue_common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe_rx]].rxdataF,frame_parms->ofdm_symbol_size*4,2,1);
  
    nr_pbch_extract(nr_ue_common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe_rx]].rxdataF,
                 nr_ue_common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe_rx]].dl_ch_estimates[eNB_id],
                 nr_ue_pbch_vars->rxdataF_ext,
                 nr_ue_pbch_vars->dl_ch_estimates_ext,
                 symbol,
                 high_speed_flag,
                 frame_parms);
#ifdef DEBUG_PBCH
    msg("[PHY] PBCH Symbol %d ofdm size %d\n",symbol, frame_parms->ofdm_symbol_size );
    msg("[PHY] PBCH starting channel_level\n");
#endif

    max_h = nr_pbch_channel_level(nr_ue_pbch_vars->dl_ch_estimates_ext,
                               frame_parms,
                               symbol);
    log2_maxh = 3+(log2_approx(max_h)/2);

#ifdef DEBUG_PBCH
    msg("[PHY] PBCH log2_maxh = %d (%d)\n",log2_maxh,max_h);
#endif

    nr_pbch_channel_compensation(nr_ue_pbch_vars->rxdataF_ext,
                              nr_ue_pbch_vars->dl_ch_estimates_ext,
                              nr_ue_pbch_vars->rxdataF_comp,
                              frame_parms,
                              symbol,
                              log2_maxh); // log2_maxh+I0_shift

    //write_output("rxdataF_comp.m","rxFcomp",nr_ue_pbch_vars->rxdataF_comp,180,2,1);
  
    /*if (frame_parms->nb_antennas_rx > 1)
      pbch_detection_mrc(frame_parms,
                         nr_ue_pbch_vars->rxdataF_comp,
                         symbol);*/


    if (mimo_mode == ALAMOUTI) {
      nr_pbch_alamouti(frame_parms,nr_ue_pbch_vars->rxdataF_comp,symbol);
    } else if (mimo_mode != SISO) {
      msg("[PBCH][RX] Unsupported MIMO mode\n");
      return(-1);
    }

    if (symbol==2) {
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

  pbch_e_rx = nr_ue_pbch_vars->llr;

  //#ifdef DEBUG_PBCH
  //pbch_e_rx = &nr_ue_pbch_vars->llr[0];

  short *p = (short *)&(nr_ue_pbch_vars->rxdataF_comp[0][1*20*12]);
  for (int cnt = 0; cnt < 8 ; cnt++)
    printf("pbch rx llr %d rxdata_comp %d addr %p\n",*(pbch_e_rx+cnt), p[cnt], &p[0]);
  //#endif

  for (int i=0; i<NR_POLAR_PBCH_E/2; i++){
    idx_demod = (sign(pbch_e_rx[i<<1])&1) ^ ((sign(pbch_e_rx[(i<<1)+1])&1)<<1);
    demod_pbch_e[i<<1] = nr_demod_table[(idx_demod)<<1];
    demod_pbch_e[(i<<1)+1] = nr_demod_table[((idx_demod)<<1)+1];
    if (i<16){
    printf("idx[%d]= %d\n", i , idx_demod);
    printf("sign[%d]= %d sign[%d]= %d\n", i<<1 , sign(pbch_e_rx[i<<1]), (i<<1)+1 , sign(pbch_e_rx[(i<<1)+1]));
    printf("demod_pbch_e2[%d] r = %2.3f i = %2.3f\n", i<<1 , demod_pbch_e[i<<1], demod_pbch_e[(i<<1)+1]);}
  }

		
  //polar decoding de-rate matching
  decoderState = polar_decoder(demod_pbch_e, pbch_a, &frame_parms->pbch_polar_params, decoderListSize, aPrioriArray, pathMetricAppr);

   for (i=0; i<32 /*(NR_POLAR_PBCH_PAYLOAD_BITS>>3)*/; i++)
     printf("pbch_a[%d] = %d \n", i,pbch_a[i]);
  
  //un-scrambling
  nr_pbch_unscrambling(frame_parms,pbch_a,NR_POLAR_PBCH_PAYLOAD_BITS);

   for (i=0; i<32 /*(NR_POLAR_PBCH_PAYLOAD_BITS>>3)*/; i++)
     printf("after unscrambling pbch_a[%d] = %d \n", i,pbch_a[i]);

  // Fix byte endian
  //for (i=0; i<32 /*(NR_POLAR_PBCH_PAYLOAD_BITS>>3)*/; i++)
   // printf("estimated output[%d] = %d \n", i,estimatedOutput[i]);
  //decoded_output[(NR_POLAR_PBCH_PAYLOAD_BITS>>3)-i-1] = pbch_a[i];

  //#ifdef DEBUG_PBCH

    // for (i=0; i<2; i++)
  // printf("[PBCH] pbch_a[%d] = %x\n",i,decoded_output[i]);

  //#endif

#ifdef DEBUG_PBCH
  msg("PBCH CRC %x : %x\n",
      crc16(pbch_a,PBCH_A),
      ((uint16_t)pbch_a[PBCH_A>>3]<<8)+pbch_a[(PBCH_A>>3)+1]);
#endif

  crc = (crc16(pbch_a,PBCH_A)>>16) ^
        (((uint16_t)pbch_a[PBCH_A>>3]<<8)+pbch_a[(PBCH_A>>3)+1]);

  if (crc == 0x0000)
    return(1);
  else if (crc == 0xffff)
    return(2);
  else if (crc == 0x5555)
    return(4);
  else
    return(-1);

}
