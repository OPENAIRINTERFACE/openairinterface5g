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

/*! \file PHY/NR_TRANSPORT/nr_prach.c
 * \brief Top-level routines for generating and decoding the PRACH physical channel V15.4 2018-12
 * \author R. Knopp
 * \date 2019
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */

#include "PHY/defs_gNB.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"

extern uint16_t prach_root_sequence_map_0_3[838];
extern uint16_t prach_root_sequence_map_abc[138];
extern uint16_t nr_du[838];
extern const char *prachfmt[];

void init_prach_list(PHY_VARS_gNB *gNB) {

  AssertFatal(gNB!=NULL,"gNB is null\n");
  for (int i=0; i<NUMBER_OF_NR_PRACH_MAX; i++){
		gNB->prach_vars.list[i].frame = -1;
		gNB->prach_vars.list[i].slot  = -1;
	}
}

void free_nr_prach_entry(PHY_VARS_gNB *gNB, int prach_id) {

  gNB->prach_vars.list[prach_id].frame = -1;
	gNB->prach_vars.list[prach_id].slot  = -1;

}

int16_t find_nr_prach(PHY_VARS_gNB *gNB,int frame, int slot, find_type_t type) {

  AssertFatal(gNB!=NULL,"gNB is null\n");
  for (uint16_t i=0; i<NUMBER_OF_NR_PRACH_MAX; i++) {
    LOG_D(PHY,"searching for PRACH in %d.%d prach_index %d=> %d.%d\n", frame,slot,i,
	  gNB->prach_vars.list[i].frame,gNB->prach_vars.list[i].slot);
		if((type == SEARCH_EXIST_OR_FREE) &&
		  (gNB->prach_vars.list[i].frame == -1) &&
		  (gNB->prach_vars.list[i].slot == -1)) {
		  return i;
		}
    else if ((type == SEARCH_EXIST) &&
		  (gNB->prach_vars.list[i].frame == frame) &&
      (gNB->prach_vars.list[i].slot  == slot)) {
		  return i;
		}
  }
  return -1;
}

void nr_fill_prach(PHY_VARS_gNB *gNB,
		   int SFN,
		   int Slot,
		   nfapi_nr_prach_pdu_t *prach_pdu) {

  int prach_id = find_nr_prach(gNB,SFN,Slot,SEARCH_EXIST_OR_FREE);
  AssertFatal( ((prach_id>=0) && (prach_id<NUMBER_OF_NR_PRACH_MAX)) || (prach_id < 0),
              "illegal or no prach_id found!!! prach_id %d\n",prach_id);

  gNB->prach_vars.list[prach_id].frame=SFN;
  gNB->prach_vars.list[prach_id].slot=Slot;
  memcpy((void*)&gNB->prach_vars.list[prach_id].pdu,(void*)prach_pdu,sizeof(*prach_pdu));

}

void init_prach_ru_list(RU_t *ru) {

  AssertFatal(ru!=NULL,"ruis null\n");
  for (int i=0; i<NUMBER_OF_NR_RU_PRACH_MAX; i++) {
			ru->prach_list[i].frame = -1;
			ru->prach_list[i].slot  = -1;
	}		
  pthread_mutex_init(&ru->prach_list_mutex,NULL);
}

int16_t find_nr_prach_ru(RU_t *ru,int frame,int slot, find_type_t type) {

  AssertFatal(ru!=NULL,"ru is null\n");
  pthread_mutex_lock(&ru->prach_list_mutex);
  for (uint16_t i=0; i<NUMBER_OF_NR_RU_PRACH_MAX; i++) {
    LOG_D(PHY,"searching for PRACH in %d.%d : prach_index %d=> %d.%d\n", frame,slot,i,
	  ru->prach_list[i].frame,ru->prach_list[i].slot);
		if((type == SEARCH_EXIST_OR_FREE) &&
		  (ru->prach_list[i].frame == -1) &&
		  (ru->prach_list[i].slot == -1)) {
      pthread_mutex_unlock(&ru->prach_list_mutex);
		  return i;
		}	
    else if ((type == SEARCH_EXIST) &&
		  (ru->prach_list[i].frame == frame) &&
      (ru->prach_list[i].slot  == slot)) {
      pthread_mutex_unlock(&ru->prach_list_mutex);
      return i;
    }
  }
  pthread_mutex_unlock(&ru->prach_list_mutex);
  return -1;
}

void nr_fill_prach_ru(RU_t *ru,
		      int SFN,
		      int Slot,
		      nfapi_nr_prach_pdu_t *prach_pdu) {

  int prach_id = find_nr_prach_ru(ru,SFN,Slot,SEARCH_EXIST_OR_FREE);
  AssertFatal( ((prach_id>=0) && (prach_id<NUMBER_OF_NR_PRACH_MAX)) || (prach_id < 0) ,
              "illegal or no prach_id found!!! prach_id %d\n",prach_id);

  pthread_mutex_lock(&ru->prach_list_mutex);
  ru->prach_list[prach_id].frame              = SFN;
  ru->prach_list[prach_id].slot               = Slot;
  ru->prach_list[prach_id].fmt                = prach_pdu->prach_format;
  ru->prach_list[prach_id].numRA              = prach_pdu->num_ra;
  ru->prach_list[prach_id].prachStartSymbol   = prach_pdu->prach_start_symbol;
  ru->prach_list[prach_id].num_prach_ocas     = prach_pdu->num_prach_ocas;
  pthread_mutex_unlock(&ru->prach_list_mutex);  

}

void free_nr_ru_prach_entry(RU_t *ru,
			    int prach_id) {

  pthread_mutex_lock(&ru->prach_list_mutex);
  ru->prach_list[prach_id].frame = -1;
	ru->prach_list[prach_id].slot  = -1;
  pthread_mutex_unlock(&ru->prach_list_mutex);

}


void rx_nr_prach_ru(RU_t *ru,
		    int prachFormat,
		    int numRA,
		    int prachStartSymbol,
		    int prachOccasion,
		    int frame,
		    int slot) {

  AssertFatal(ru!=NULL,"ru is null\n");

  int16_t            **rxsigF=NULL;
  NR_DL_FRAME_PARMS *fp=ru->nr_frame_parms;
  int slot2=slot;

  int16_t *prach[ru->nb_rx];
  int prach_sequence_length = ru->config.prach_config.prach_sequence_length.value;

  int msg1_frequencystart   = ru->config.prach_config.num_prach_fd_occasions_list[numRA].k1.value;

  int sample_offset_slot = (prachStartSymbol==0?0:fp->ofdm_symbol_size*prachStartSymbol+fp->nb_prefix_samples0+fp->nb_prefix_samples*(prachStartSymbol-1));
  //to be checked for mu=0;

  LOG_D(PHY,"frame %d, slot %d: doing rx_nr_prach_ru for format %d, numRA %d, prachStartSymbol %d, prachOccasion %d\n",frame,slot,prachFormat,numRA,prachStartSymbol,prachOccasion);

  rxsigF            = ru->prach_rxsigF[prachOccasion];

  AssertFatal(ru->if_south == LOCAL_RF,"we shouldn't call this if if_south != LOCAL_RF\n");

  for (int aa=0; aa<ru->nb_rx; aa++){ 
    if (prach_sequence_length == 0) slot2=(slot/fp->slots_per_subframe)*fp->slots_per_subframe; 
    prach[aa] = (int16_t*)&ru->common.rxdata[aa][fp->get_samples_slot_timestamp(slot2,fp,0)+sample_offset_slot-ru->N_TA_offset];
  } 

  idft_size_idx_t dftsize;
  int dftlen=0;
  int mu = fp->numerology_index;
  int Ncp = 0;
  int16_t *prach2;

  if (prach_sequence_length == 0) {
    LOG_D(PHY,"PRACH (ru %d) in %d.%d, format %d, msg1_frequencyStart %d\n",
	  ru->idx,frame,slot2,prachFormat,msg1_frequencystart);
    AssertFatal(prachFormat<4,"Illegal prach format %d for length 839\n",prachFormat);
    switch (prachFormat) {
    case 0:
      Ncp = 3168;
      break;
      
    case 1:
      Ncp = 21024;
      break;
      
    case 2:
      Ncp = 4688;
      break;
      
    case 3:
      Ncp = 3168;
      break;
      
    }
  }
  else {
    LOG_D(PHY,"PRACH (ru %d) in %d.%d, format %s, msg1_frequencyStart %d,startSymbol %d\n",
	  ru->idx,frame,slot,prachfmt[prachFormat],msg1_frequencystart,prachStartSymbol);

    switch (prachFormat) {
    case 4: //A1
      Ncp = 288/(1<<mu);
      break;
      
    case 5: //A2
      Ncp = 576/(1<<mu);
      break;
      
    case 6: //A3
      Ncp = 864/(1<<mu);
      break;
      
    case 7: //B1
      Ncp = 216/(1<<mu);
    break;
    
    /*
    // B2 and B3 do not exist in FAPI
    case 4: //B2
      Ncp = 360/(1<<mu);
      break;
      
    case 5: //B3
      Ncp = 504/(1<<mu);
      break;
    */
    case 8: //B4
      Ncp = 936/(1<<mu);
      break;
      
    case 9: //C0
      Ncp = 1240/(1<<mu);
      break;
      
    case 10: //C2
      Ncp = 2048/(1<<mu);
      break;
      
    default:
      AssertFatal(1==0,"unknown prach format %x\n",prachFormat);
      break;
    }
  }
  // Do forward transform
  if (LOG_DEBUGFLAG(PRACH)) {
    LOG_D(PHY,"rx_prach: Doing PRACH FFT for nb_rx:%d Ncp:%d\n",ru->nb_rx, Ncp);
  }

  

  // Note: Assumes PUSCH SCS @ 30 kHz, take values for formats 0-2 and adjust for others below
  int kbar = 1;
  int K    = 24;
  if (prach_sequence_length == 0 && prachFormat == 3) { 
    K=4;
    kbar=10;
  }
  else if (prach_sequence_length == 1) {
    // Note: Assumes that PRACH SCS is same as PUSCH SCS
    K=1;
    kbar=2;
  }
  int n_ra_prb            = msg1_frequencystart;
  int k                   = (12*n_ra_prb) - 6*fp->N_RB_UL;

  int N_ZC = (prach_sequence_length==0)?839:139;

  if (k<0) k+=(fp->ofdm_symbol_size);
  
  k*=K;
  k+=kbar;

  int reps=1;

  for (int aa=0; aa<ru->nb_rx; aa++) {
    AssertFatal(prach[aa]!=NULL,"prach[%d] is null\n",aa);

    // do DFT
    if (mu==1) {
      switch(fp->samples_per_subframe) {
        case 15360:
          // 10, 15 MHz @ 15.36 Ms/s
          prach2 = prach[aa] + (1*Ncp); // Ncp is for 30.72 Ms/s, so divide by 2 to bring to 15.36 Ms/s and multiply by 2 for I/Q
          if (prach_sequence_length == 0) {
            if (prachFormat == 0 || prachFormat == 1 || prachFormat == 2) {
              dftlen=12288;
              dft(DFT_12288,prach2,rxsigF[aa],1);
            }
            if (prachFormat == 1 || prachFormat == 2) {
              dft(DFT_12288,prach2+24576,rxsigF[aa]+24576,1);
              reps++;
            }
            if (prachFormat == 2) {
              dft(DFT_12288,prach2+(24576*2),rxsigF[aa]+(24576*2),1);
              dft(DFT_12288,prach2+(24576*3),rxsigF[aa]+(24576*3),1);
              reps+=2;
            }
            if (prachFormat == 3) {
              dftlen=3072;
              for (int i=0;i<4;i++) dft(DFT_3072,prach2+(i*3072*2),rxsigF[aa]+(i*3072*2),1);
              reps=4;
            }
          } else { // 839 sequence
            if (prachStartSymbol == 0) prach2+=16; // 8 samples @ 15.36 Ms/s in first symbol of each half subframe (15/30 kHz only)

            dftlen=512;
            dft(DFT_512,prach2,rxsigF[aa],1);
            if (prachFormat != 9/*C0*/) {
              dft(DFT_512,prach2+1024,rxsigF[aa]+1024,1);
              reps++;
            }
            if (prachFormat == 5/*A2*/ || prachFormat == 6/*A3*/|| prachFormat == 8/*B4*/ || prachFormat == 10/*C2*/) {
              dft(DFT_512,prach2+1024*2,rxsigF[aa]+1024*2,1);
              dft(DFT_512,prach2+1024*3,rxsigF[aa]+1024*3,1);
              reps+=2;
            }
            if (prachFormat == 6/*A3*/ || prachFormat == 8/*B4*/) {
              dft(DFT_512,prach2+1024*4,rxsigF[aa]+1024*4,1);
              dft(DFT_512,prach2+1024*5,rxsigF[aa]+1024*5,1);
              reps+=2;
            }
            if (prachFormat == 8/*B4*/) {
              for (int i=6;i<12;i++) dft(DFT_512,prach2+(1024*i),rxsigF[aa]+(1024*i),1);
              reps+=6;
            }
          }
          break;

        case 30720:
          // 20, 25, 30 MHz @ 30.72 Ms/s
          prach2 = prach[aa] + (2*Ncp); // Ncp is for 30.72 Ms/s, so just multiply by 2 for I/Q
          if (prach_sequence_length == 0) {
            if (prachFormat == 0 || prachFormat == 1 || prachFormat == 2) {
              dftlen=24576;
              dft(DFT_24576,prach2,rxsigF[aa],1);
            }
            if (prachFormat == 1 || prachFormat == 2) {
              dft(DFT_24576,prach2+49152,rxsigF[aa]+49152,1);
              reps++;
            }
            if (prachFormat == 2) {
              dft(DFT_24576,prach2+(49152*2),rxsigF[aa]+(49152*2),1);
              dft(DFT_24576,prach2+(49152*3),rxsigF[aa]+(49152*3),1);
              reps+=2;
            }
            if (prachFormat == 3) {
              dftlen=6144;
              for (int i=0;i<4;i++) dft(DFT_6144,prach2+(i*6144*2),rxsigF[aa]+(i*6144*2),1);
              reps=4;
            }
          } else { // 839 sequence
            if (prachStartSymbol == 0) prach2+=32; // 16 samples @ 30.72 Ms/s in first symbol of each half subframe (15/30 kHz only)

            dftlen=1024;
            dft(DFT_1024,prach2,rxsigF[aa],1);
            if (prachFormat != 9/*C0*/) {
              dft(DFT_1024,prach2+2048,rxsigF[aa]+2048,1);
              reps++;
            }
            if (prachFormat == 5/*A2*/ || prachFormat == 6/*A3*/|| prachFormat == 8/*B4*/ || prachFormat == 10/*C2*/) {
              dft(DFT_1024,prach2+2048*2,rxsigF[aa]+2048*2,1);
              dft(DFT_1024,prach2+2048*3,rxsigF[aa]+2048*3,1);
              reps+=2;
            }
            if (prachFormat == 6/*A3*/ || prachFormat == 8/*B4*/) {
              dft(DFT_1024,prach2+2048*4,rxsigF[aa]+2048*4,1);
              dft(DFT_1024,prach2+2048*5,rxsigF[aa]+2048*5,1);
              reps+=2;
            }
            if (prachFormat == 8/*B4*/) {
              for (int i=6;i<12;i++) dft(DFT_1024,prach2+(2048*i),rxsigF[aa]+(2048*i),1);
              reps+=6;
            }
          }
          break;

        case 61440:
          // 40, 50, 60 MHz @ 61.44 Ms/s
          prach2 = prach[aa] + (4*Ncp); // Ncp is for 30.72 Ms/s, so multiply by 2 for I/Q, and 2 to bring to 61.44 Ms/s
          if (prach_sequence_length == 0) {
            if (prachFormat == 0 || prachFormat == 1 || prachFormat == 2) {
              dftlen=49152;
              dft(DFT_49152,prach2,rxsigF[aa],1);
            }
            if (prachFormat == 1 || prachFormat == 2) {
              dft(DFT_49152,prach2+98304,rxsigF[aa]+98304,1);
              reps++;
            }
            if (prachFormat == 2) {
              dft(DFT_49152,prach2+(98304*2),rxsigF[aa]+(98304*2),1);
              dft(DFT_49152,prach2+(98304*3),rxsigF[aa]+(98304*3),1);
              reps+=2;
            }
            if (prachFormat == 3) {
              dftlen=12288;
              for (int i=0;i<4;i++) dft(DFT_12288,prach2+(i*12288*2),rxsigF[aa]+(i*12288*2),1);
              reps=4;
            }
          } else { // 839 sequence
            if (prachStartSymbol == 0) prach2+=64; // 32 samples @ 61.44 Ms/s in first symbol of each half subframe (15/30 kHz only)

            dftlen=2048;
            dft(DFT_2048,prach2,rxsigF[aa],1);
            if (prachFormat != 9/*C0*/) {
              dft(DFT_2048,prach2+4096,rxsigF[aa]+4096,1);
              reps++;
            }
            if (prachFormat == 5/*A2*/ || prachFormat == 6/*A3*/|| prachFormat == 8/*B4*/ || prachFormat == 10/*C2*/) {
              dft(DFT_2048,prach2+4096*2,rxsigF[aa]+4096*2,1);
              dft(DFT_2048,prach2+4096*3,rxsigF[aa]+4096*3,1);
              reps+=2;
            }
            if (prachFormat == 6/*A3*/ || prachFormat == 8/*B4*/) {
              dft(DFT_2048,prach2+4096*4,rxsigF[aa]+4096*4,1);
              dft(DFT_2048,prach2+4096*5,rxsigF[aa]+4096*5,1);
              reps+=2;
            }
            if (prachFormat == 8/*B4*/) {
              for (int i=6;i<12;i++) dft(DFT_2048,prach2+(4096*i),rxsigF[aa]+(4096*i),1);
              reps+=6;
            }
          }
          break;

        case 46080:
          // 40 MHz @ 46.08 Ms/s
          prach2 = prach[aa] + (3*Ncp); // 46.08 is 1.5 * 30.72, times 2 for I/Q
          if (prach_sequence_length == 0) {
            if (prachFormat == 0 || prachFormat == 1 || prachFormat == 2) {
              dftlen=36864;
              dft(DFT_36864,prach2,rxsigF[aa],1);
            }
            if (prachFormat == 1 || prachFormat == 2) {
              dft(DFT_36864,prach2+73728,rxsigF[aa]+73728,1);
              reps++;
            }
            if (prachFormat == 2) {
              dft(DFT_36864,prach2+(73728*2),rxsigF[aa]+(73728*2),1);
              dft(DFT_36864,prach2+(73728*3),rxsigF[aa]+(73728*3),1);
              reps+=2;
            }
            if (prachFormat == 3) {
              dftlen=9216;
              for (int i=0;i<4;i++) dft(DFT_9216,prach2+(i*9216*2),rxsigF[aa]+(i*9216*2),1);
              reps=4;
            }
          } else { // 839 sequence
            if (prachStartSymbol == 0) prach2+=48; // 24 samples @ 46.08 Ms/s in first symbol of each half subframe (15/30 kHz only)

            dftlen=1536;
            dft(DFT_1536,prach2,rxsigF[aa],1);
            if (prachFormat != 9/*C0*/) {
              dft(DFT_1536,prach2+3072,rxsigF[aa]+3072,1);
              reps++;
            }
            if (prachFormat == 5/*A2*/ || prachFormat == 6/*A3*/|| prachFormat == 8/*B4*/ || prachFormat == 10/*C2*/) {
              dft(DFT_1536,prach2+3072*2,rxsigF[aa]+3072*2,1);
              dft(DFT_1536,prach2+3072*3,rxsigF[aa]+3072*3,1);
              reps+=2;
            }
            if (prachFormat == 6/*A3*/ || prachFormat == 8/*B4*/) {
              dft(DFT_1536,prach2+3072*4,rxsigF[aa]+3072*4,1);
              dft(DFT_1536,prach2+3072*5,rxsigF[aa]+3072*5,1);
              reps+=2;
            }
            if (prachFormat == 8/*B4*/) {
              for (int i=6;i<12;i++) dft(DFT_1536,prach2+(3072*i),rxsigF[aa]+(3072*i),1);
              reps+=6;
            }
          }
          break;

        case 122880:
          // 70, 80, 90, 100 MHz @ 122.88 Ms/s
          prach2 = prach[aa] + (8*Ncp);
          if (prach_sequence_length == 0) {
            if (prachFormat == 0 || prachFormat == 1 || prachFormat == 2) {
              dftlen=98304;
              dft(DFT_98304,prach2,rxsigF[aa],1);
            }
            if (prachFormat == 1 || prachFormat == 2) {
              dft(DFT_98304,prach2+196608,rxsigF[aa]+196608,1);
              reps++;
            }
            if (prachFormat == 2) {
              dft(DFT_98304,prach2+(196608*2),rxsigF[aa]+(196608*2),1);
              dft(DFT_98304,prach2+(196608*3),rxsigF[aa]+(196608*3),1);
              reps+=2;
            }
            if (prachFormat == 3) {
              dftlen=24576;
              for (int i=0;i<4;i++) dft(DFT_24576,prach2+(i*2*24576),rxsigF[aa]+(i*2*24576),1);
              reps=4;
            }
          } else { // 839 sequence
            if (prachStartSymbol == 0) prach2+=128; // 64 samples @ 122.88 Ms/s in first symbol of each half subframe (15/30 kHz only)

            dftlen=4096;
            dft(DFT_4096,prach2,rxsigF[aa],1);
            if (prachFormat != 9/*C0*/) {
              dft(DFT_4096,prach2+8192,rxsigF[aa]+8192,1);
              reps++;
            }

            if (prachFormat == 5/*A2*/ || prachFormat == 6/*A3*/|| prachFormat == 8/*B4*/ || prachFormat == 10/*C2*/) {
              dft(DFT_4096,prach2+8192*2,rxsigF[aa]+8192*2,1);
              dft(DFT_4096,prach2+8192*3,rxsigF[aa]+8192*3,1);
              reps+=2;
            }
            if (prachFormat == 6/*A3*/ || prachFormat == 8/*B4*/) {
              dft(DFT_4096,prach2+8192*4,rxsigF[aa]+8192*4,1);
              dft(DFT_4096,prach2+8192*5,rxsigF[aa]+8192*5,1);
              reps+=2;
            }
            if (prachFormat == 8/*B4*/) {
              for (int i=6;i<12;i++) dft(DFT_4096,prach2+(8192*i),rxsigF[aa]+(8192*i),1);
              reps+=6;
            }
          }
          break;

        case 92160:
          // 80, 90 MHz @ 92.16 Ms/s
          prach2 = prach[aa] + (6*Ncp);
          if (prach_sequence_length == 0) {
            if (prachFormat == 0 || prachFormat == 1 || prachFormat == 2) {
              dftlen=73728;
              dft(DFT_73728,prach2,rxsigF[aa],1);
            }
            if (prachFormat == 1 || prachFormat == 2) {
              dft(DFT_73728,prach2+147456,rxsigF[aa]+147456,1);
              reps++;
            }
            if (prachFormat == 2) {
              dft(DFT_73728,prach2+(147456*2),rxsigF[aa]+(147456*2),1);
              dft(DFT_73728,prach2+(147456*3),rxsigF[aa]+(147456*3),1);
              reps+=2;
            }
            if (prachFormat == 3) {
              dftlen=18432;
              for (int i=0;i<4;i++) dft(DFT_18432,prach2+(i*2*18432),rxsigF[aa]+(i*2*18432),1);
              reps=4;
            }
          } else {
            if (prachStartSymbol == 0) prach2+=96; // 64 samples @ 122.88 Ms/s in first symbol of each half subframe (15/30 kHz only)

            dftlen=3072;
            dft(DFT_3072,prach2,rxsigF[aa],1);
            if (prachFormat != 9/*C0*/) {
              dft(DFT_3072,prach2+6144,rxsigF[aa]+6144,1);
              reps++;
            }

            if (prachFormat == 5/*A2*/ || prachFormat == 6/*A3*/|| prachFormat == 8/*B4*/ || prachFormat == 10/*C2*/) {
              dft(DFT_3072,prach2+6144*2,rxsigF[aa]+6144*2,1);
              dft(DFT_3072,prach2+6144*3,rxsigF[aa]+6144*3,1);
              reps+=2;
            }
            if (prachFormat == 6/*A3*/ || prachFormat == 8/*B4*/) {
              dft(DFT_3072,prach2+6144*4,rxsigF[aa]+6144*4,1);
              dft(DFT_3072,prach2+6144*5,rxsigF[aa]+6144*5,1);
              reps+=2;
            }
            if (prachFormat == 8/*B4*/) {
              for (int i=6;i<12;i++) dft(DFT_3072,prach2+(6144*i),rxsigF[aa]+(6144*i),1);
              reps+=6;
            }
          }
          break;
        default:
          AssertFatal(1==0,"sample_rate %f MHz not support for NR PRACH yet\n", fp->samples_per_subframe / 1000.0);
      }
    }
    else if (mu==3) {
      if (fp->threequarter_fs) {
	AssertFatal(1==0,"3/4 sampling not supported for numerology %d\n",mu);
      }
      if (prach_sequence_length == 0) {
	AssertFatal(1==0,"long prach not supported for numerology %d\n",mu);
      }
      if (fp->N_RB_UL == 32) {
	prach2 = prach[aa] + (Ncp<<2); // Ncp is for 30.72 Ms/s, so multiply by 2 for I/Q, and 2 for 61.44Msps
	if (slot%(fp->slots_per_subframe/2)==0 && prachStartSymbol == 0)
	  prach2+=64; // 32 samples @ 61.44 Ms/s in first symbol of each half subframe
	dftlen=512;
	dftsize = DFT_512;
      }
      else if (fp->N_RB_UL == 66) {
	prach2 = prach[aa] + (Ncp<<3); // Ncp is for 30.72 Ms/s, so multiply by 4 for I/Q, and 2 for 122.88Msps
	if (slot%(fp->slots_per_subframe/2)==0 && prachStartSymbol == 0)
	  prach2+=128; // 64 samples @ 122.88 Ms/s in first symbol of each half subframe 
	dftlen=1024;
	dftsize = DFT_1024;
      }
      else {
	AssertFatal(1==0,"N_RB_UL %d not support for numerology %d\n",fp->N_RB_UL,mu);
      }
      
      dft(dftsize,prach2,rxsigF[aa],1);
      if (prachFormat != 9/*C0*/) {
	dft(dftsize,prach2+dftlen*2,rxsigF[aa]+dftlen*2,1);
	reps++;
      }
	  
      if (prachFormat == 5/*A2*/ || prachFormat == 6/*A3*/|| prachFormat == 8/*B4*/ || prachFormat == 10/*C2*/) {     
	dft(dftsize,prach2+dftlen*4,rxsigF[aa]+dftlen*4,1);
	dft(dftsize,prach2+dftlen*6,rxsigF[aa]+dftlen*6,1);
	reps+=2;
      } 
      if (prachFormat == 6/*A3*/ || prachFormat == 8/*B4*/) {     
	dft(dftsize,prach2+dftlen*8,rxsigF[aa]+dftlen*8,1);
	dft(dftsize,prach2+dftlen*10,rxsigF[aa]+dftlen*10,1);
	reps+=2;
      } 
      if (prachFormat == 8/*B4*/) {
	for (int i=6;i<12;i++)
	  dft(dftsize,prach2+(dftlen*2*i),rxsigF[aa]+(dftlen*2*i),1);
	reps+=6;
      }
    }
    else {
      AssertFatal(1==0,"Numerology not supported\n");
    }

    //LOG_M("ru_rxsigF_tmp.m","rxsFtmp", rxsigF[aa], dftlen*2*reps, 1, 1);

    //Coherent combining of PRACH repetitions (assumes channel does not change, to be revisted for "long" PRACH)
    LOG_D(PHY,"Doing PRACH combining of %d reptitions N_ZC %d\n",reps,N_ZC);
    int16_t rxsigF_tmp[N_ZC<<1];
    //    if (k+N_ZC > dftlen) { // PRACH signal is split around DC 
    int16_t *rxsigF2=rxsigF[aa];
    int k2=k<<1;

    for (int j=0;j<N_ZC<<1;j++,k2++) {
      if (k2==(dftlen<<1)) k2=0;
      rxsigF_tmp[j] = rxsigF2[k2];
      for (int i=1;i<reps;i++) rxsigF_tmp[j] += rxsigF2[k2+(i*dftlen<<1)];
    }
    memcpy((void*)rxsigF2,(void *)rxsigF_tmp,N_ZC<<2);
  }

}

void rx_nr_prach(PHY_VARS_gNB *gNB,
		 nfapi_nr_prach_pdu_t *prach_pdu,
		 int prachOccasion,
		 int frame,
		 int subframe,
		 uint16_t *max_preamble,
		 uint16_t *max_preamble_energy,
		 uint16_t *max_preamble_delay
		 )
{
  AssertFatal(gNB!=NULL,"Can only be called from gNB\n");

  int i;

  nfapi_nr_prach_config_t *cfg=&gNB->gNB_config.prach_config;
  NR_DL_FRAME_PARMS *fp;

  uint16_t           rootSequenceIndex;  
  int                numrootSequenceIndex;
  uint8_t            restricted_set;      
  uint8_t            n_ra_prb=0xFF;
  int16_t            *prachF=NULL;
  int                nb_rx;

  int16_t **rxsigF            = gNB->prach_vars.rxsigF;

  uint8_t preamble_index;
  uint16_t NCS=99,NCS2;
  uint16_t preamble_offset=0,preamble_offset_old;
  int16_t preamble_shift=0;
  uint32_t preamble_shift2;
  uint16_t preamble_index0=0,n_shift_ra=0,n_shift_ra_bar;
  uint16_t d_start=0;
  uint16_t numshift=0;
  uint16_t *prach_root_sequence_map;
  uint8_t not_found;
  uint16_t u;
  int16_t *Xu=0;
  uint16_t offset;
  uint16_t first_nonzero_root_idx=0;
  uint8_t new_dft=0;
  uint8_t aa;
  int32_t lev;
  int16_t levdB;
  int log2_ifft_size=10;
  int16_t prach_ifft_tmp[2048*2] __attribute__((aligned(32)));
  int32_t *prach_ifft=(int32_t*)NULL;
  
  fp = &gNB->frame_parms;

  nb_rx = gNB->gNB_config.carrier_config.num_rx_ant.value;
  rootSequenceIndex   = cfg->num_prach_fd_occasions_list[prach_pdu->num_ra].prach_root_sequence_index.value;
  numrootSequenceIndex   = cfg->num_prach_fd_occasions_list[prach_pdu->num_ra].num_root_sequences.value;
  NCS          = prach_pdu->num_cs;//cfg->num_prach_fd_occasions_list[0].prach_zero_corr_conf.value;
  int prach_sequence_length = cfg->prach_sequence_length.value;
  int msg1_frequencystart   = cfg->num_prach_fd_occasions_list[prach_pdu->num_ra].k1.value;
  //  int num_unused_root_sequences = cfg->num_prach_fd_occasions_list[0].num_unused_root_sequences.value;
  // cfg->num_prach_fd_occasions_list[0].unused_root_sequences_list

  restricted_set      = cfg->restricted_set_config.value;

  uint8_t prach_fmt = prach_pdu->prach_format;
  uint16_t N_ZC = (prach_sequence_length==0)?839:139;

  LOG_D(PHY,"L1 PRACH RX: rooSequenceIndex %d, numRootSeqeuences %d, NCS %d, N_ZC %d, format %d \n",rootSequenceIndex,numrootSequenceIndex,NCS,N_ZC,prach_fmt);

  prach_ifft        = gNB->prach_vars.prach_ifft;
  prachF            = gNB->prach_vars.prachF;
  if (LOG_DEBUGFLAG(PRACH)){
    if ((frame&1023) < 20) LOG_D(PHY,"PRACH (gNB) : running rx_prach for subframe %d, msg1_frequencystart %d, rootSequenceIndex %d\n", subframe,msg1_frequencystart,rootSequenceIndex);
  }

  start_meas(&gNB->rx_prach);

  prach_root_sequence_map = (cfg->prach_sequence_length.value==0) ? prach_root_sequence_map_0_3 : prach_root_sequence_map_abc;

  // PDP is oversampled, e.g. 1024 sample instead of 839
  // Adapt the NCS (zero-correlation zones) with oversampling factor e.g. 1024/839
  NCS2 = (N_ZC==839) ? ((NCS<<10)/839) : ((NCS<<8)/139);

  if (NCS2==0) NCS2 = N_ZC;


  preamble_offset_old = 99;

  
  *max_preamble_energy=0;
  *max_preamble_delay=0;
  *max_preamble=0;

  for (preamble_index=0 ; preamble_index<64 ; preamble_index++) {

    if (LOG_DEBUGFLAG(PRACH)){
      int en = dB_fixed(signal_energy((int32_t*)&rxsigF[0][0],(N_ZC==839) ? 840: 140));
      if (en>60) LOG_D(PHY,"frame %d, subframe %d : Trying preamble %d \n",frame,subframe,preamble_index);
    }
    if (restricted_set == 0) {
      // This is the relative offset in the root sequence table (5.7.2-4 from 36.211) for the given preamble index
      preamble_offset = ((NCS==0)? preamble_index : (preamble_index/(N_ZC/NCS)));
   
      if (preamble_offset != preamble_offset_old) {
        preamble_offset_old = preamble_offset;
        new_dft = 1;
        // This is the \nu corresponding to the preamble index
        preamble_shift  = 0;
      }
      
      else {
        preamble_shift  -= NCS;

        if (preamble_shift < 0)
          preamble_shift+=N_ZC;
      }
    } else { // This is the high-speed case
      new_dft = 0;
      nr_fill_du(N_ZC,prach_root_sequence_map);
      // set preamble_offset to initial rootSequenceIndex and look if we need more root sequences for this
      // preamble index and find the corresponding cyclic shift
      // Check if all shifts for that root have been processed
      if (preamble_index0 == numshift) {
        not_found = 1;
        new_dft   = 1;
        preamble_index0 -= numshift;
        //(preamble_offset==0 && numshift==0) ? (preamble_offset) : (preamble_offset++);

        while (not_found == 1) {
          // current root depending on rootSequenceIndex
          int index = (rootSequenceIndex + preamble_offset) % N_ZC;

	  u = prach_root_sequence_map[index];
	  
	  uint16_t n_group_ra = 0;
	  
	  if ( (nr_du[u]<(N_ZC/3)) && (nr_du[u]>=NCS) ) {
	    n_shift_ra     = nr_du[u]/NCS;
	    d_start        = (nr_du[u]<<1) + (n_shift_ra * NCS);
	    n_group_ra     = N_ZC/d_start;
	    n_shift_ra_bar = max(0,(N_ZC-(nr_du[u]<<1)-(n_group_ra*d_start))/N_ZC);
	  } else if  ( (nr_du[u]>=(N_ZC/3)) && (nr_du[u]<=((N_ZC - NCS)>>1)) ) {
	    n_shift_ra     = (N_ZC - (nr_du[u]<<1))/NCS;
	    d_start        = N_ZC - (nr_du[u]<<1) + (n_shift_ra * NCS);
	    n_group_ra     = nr_du[u]/d_start;
	    n_shift_ra_bar = min(n_shift_ra,max(0,(nr_du[u]- (n_group_ra*d_start))/NCS));
	  } else {
	    n_shift_ra     = 0;
	    n_shift_ra_bar = 0;
	  }
	  
	  // This is the number of cyclic shifts for the current root u
	  numshift = (n_shift_ra*n_group_ra) + n_shift_ra_bar;
	  // skip to next root and recompute parameters if numshift==0
	  (numshift>0) ? (not_found = 0) : (preamble_offset++);
	}
      }        
      
      
      if (n_shift_ra>0)
        preamble_shift = -((d_start * (preamble_index0/n_shift_ra)) + ((preamble_index0%n_shift_ra)*NCS)); // minus because the channel is h(t -\tau + Cv)
      else
        preamble_shift = 0;

      if (preamble_shift < 0)
        preamble_shift+=N_ZC;

      preamble_index0++;

      if (preamble_index == 0)
        first_nonzero_root_idx = preamble_offset;
    }

    // Compute DFT of RX signal (conjugate input, results in conjugate output) for each new rootSequenceIndex
    if (LOG_DEBUGFLAG(PRACH)) {
      int en = dB_fixed(signal_energy((int32_t*)&rxsigF[0][0],840));
      if (en>60) LOG_I(PHY,"frame %d, subframe %d : preamble index %d, NCS %d, N_ZC/NCS %d: offset %d, preamble shift %d , en %d)\n",
		       frame,subframe,preamble_index,NCS,N_ZC/NCS,preamble_offset,preamble_shift,en);
    }

    LOG_D(PHY,"PRACH RX preamble_index %d, preamble_offset %d\n",preamble_index,preamble_offset);


    if (new_dft == 1) {
      new_dft = 0;

      Xu=(int16_t*)gNB->X_u[preamble_offset-first_nonzero_root_idx];

      LOG_D(PHY,"PRACH RX new dft preamble_offset-first_nonzero_root_idx %d\n",preamble_offset-first_nonzero_root_idx);


      memset(prach_ifft,0,((N_ZC==839) ? 2048 : 256)*sizeof(int32_t));
    

      memset(prachF, 0, sizeof(int16_t)*2*1024 );
      if (LOG_DUMPFLAG(PRACH)) {      
	       LOG_M("prach_rxF0.m","prach_rxF0",rxsigF[0],N_ZC,1,1);
	       LOG_M("prach_rxF1.m","prach_rxF1",rxsigF[1],6144,1,1);
      }
   
      for (aa=0;aa<nb_rx; aa++) {
	// Do componentwise product with Xu* on each antenna 

	       for (offset=0; offset<(N_ZC<<1); offset+=2) {
	          prachF[offset]   = (int16_t)(((int32_t)Xu[offset]*rxsigF[aa][offset]   + (int32_t)Xu[offset+1]*rxsigF[aa][offset+1])>>15);
	          prachF[offset+1] = (int16_t)(((int32_t)Xu[offset]*rxsigF[aa][offset+1] - (int32_t)Xu[offset+1]*rxsigF[aa][offset])>>15);
	       }
	
	       // Now do IFFT of size 1024 (N_ZC=839) or 256 (N_ZC=139)
	       if (N_ZC == 839) {
	         log2_ifft_size = 10;
	         idft(IDFT_1024,prachF,prach_ifft_tmp,1);
	         // compute energy and accumulate over receive antennas
	         for (i=0;i<2048;i++)
	           prach_ifft[i] += ((int32_t)prach_ifft_tmp[i<<1]*(int32_t)prach_ifft_tmp[i<<1] + (int32_t)prach_ifft_tmp[1+(i<<1)]*(int32_t)prach_ifft_tmp[1+(i<<1)])/nb_rx;
	       } else {
	         idft(IDFT_256,prachF,prach_ifft_tmp,1);
	         log2_ifft_size = 8;
	  // compute energy and accumulate over receive antennas and repetitions for BR
	  for (i=0;i<256;i++)
	    prach_ifft[i] += ((int32_t)prach_ifft_tmp[i<<1]*(int32_t)prach_ifft_tmp[(i<<1)] + (int32_t)prach_ifft_tmp[1+(i<<1)]*(int32_t)prach_ifft_tmp[1+(i<<1)])/nb_rx;
	}

	if (LOG_DUMPFLAG(PRACH)) {	
	  if (aa==0) LOG_M("prach_rxF_comp0.m","prach_rxF_comp0",prachF,1024,1,1);
    if (aa==1) LOG_M("prach_rxF_comp1.m","prach_rxF_comp1",prachF,1024,1,1);
	}
      }// antennas_rx
    } // new dft
    
    // check energy in nth time shift, for 

    preamble_shift2 = ((preamble_shift==0) ? 0 : ((preamble_shift<<log2_ifft_size)/N_ZC));

    for (i=0; i<NCS2; i++) {
      lev = (int32_t)prach_ifft[(preamble_shift2+i)];
      levdB = dB_fixed_times10(lev);
      if (levdB>*max_preamble_energy) {
	      LOG_D(PHY,"preamble_index %d, delay %d en %d dB > %d dB\n",preamble_index,i,levdB,*max_preamble_energy);
	      *max_preamble_energy  = levdB;
	      *max_preamble_delay   = i; // Note: This has to be normalized to the 30.72 Ms/s sampling rate 
	      *max_preamble         = preamble_index;
      }
    }
  }// preamble_index


  // The conversion from *max_preamble_delay from TA value is done here.
  // It is normalized to the 30.72 Ms/s, considering the numerology, N_RB and the sampling rate
  // See table 6.3.3.1 -1 and -2 in 38211.

  // Format 0, 1, 2: 24576 samples @ 30.72 Ms/s, 98304 samples @ 122.88 Ms/s
  // By solving:
  // max_preamble_delay * ( (24576*(fs/30.72M)) / 1024 ) / fs = TA * 16 * 64 / 2^mu * Tc

  // Format 3: 6144 samples @ 30.72 Ms/s, 24576 samples @ 122.88 Ms/s
  // By solving:
  // max_preamble_delay * ( (6144*(fs/30.72M)) / 1024 ) / fs = TA * 16 * 64 / 2^mu * Tc

  // Format >3: 2048/2^mu samples @ 30.72 Ms/s, 2048/2^mu * 4 samples @ 122.88 Ms/s
  // By solving:
  // max_preamble_delay * ( (2048/2^mu*(fs/30.72M)) / 256 ) / fs = TA * 16 * 64 / 2^mu * Tc
  uint16_t *TA = max_preamble_delay;
  int mu = fp->numerology_index;
  if (cfg->prach_sequence_length.value==0) {
    if (prach_fmt == 0 || prach_fmt == 1 || prach_fmt == 2) *TA = *TA*3*(1<<mu)/2;
    else if (prach_fmt == 3)                                *TA = *TA*3*(1<<mu)/8;
  }
  else *TA = *TA/2;


  if (LOG_DUMPFLAG(PRACH)) {
    //int en = dB_fixed(signal_energy((int32_t*)&rxsigF[0][0],840));
    //    if (en>60) {
      int k = (12*n_ra_prb) - 6*fp->N_RB_UL;
      
      if (k<0) k+=fp->ofdm_symbol_size;
      
      k*=12;
      k+=13;
      k*=2;
      

      LOG_M("rxsigF.m","prach_rxF",&rxsigF[0][0],12288,1,1);
      LOG_M("prach_rxF_comp0.m","prach_rxF_comp0",prachF,1024,1,1);
      LOG_M("Xu.m","xu",Xu,N_ZC,1,1);
      LOG_M("prach_ifft0.m","prach_t0",prach_ifft,1024,1,1);
      //    }
  } /* LOG_DUMPFLAG(PRACH) */
  stop_meas(&gNB->rx_prach);

}

