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
#include "PHY/NR_TRANSPORT/nr_transport.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto_common.h"

extern uint16_t NCS_unrestricted_delta_f_RA_125[16];
extern uint16_t NCS_restricted_TypeA_delta_f_RA_125[15];
extern uint16_t NCS_restricted_TypeB_delta_f_RA_125[13];
extern uint16_t NCS_unrestricted_delta_f_RA_5[16];
extern uint16_t NCS_restricted_TypeA_delta_f_RA_5[16];
extern uint16_t NCS_restricted_TypeB_delta_f_RA_5[14];
extern uint16_t NCS_unrestricted_delta_f_RA_15[16];
extern uint16_t prach_root_sequence_map_0_3[838];
extern uint16_t prach_root_sequence_map_abc[138];
extern int64_t table_6_3_3_2_2_prachConfig_Index [256][9];
extern int64_t table_6_3_3_2_3_prachConfig_Index [256][9];
extern int64_t table_6_3_3_2_4_prachConfig_Index [256][10];
extern uint16_t nr_du[838];
extern int16_t nr_ru[2*839];

void rx_nr_prach_ru(RU_t *ru,
		    int frame,
		    int subframe) {

  AssertFatal(ru!=NULL,"ru is null\n");

  int16_t            **rxsigF=NULL;
  NR_DL_FRAME_PARMS *fp=ru->nr_frame_parms;
  int16_t prach_ConfigIndex   = fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
  int16_t *prach[ru->nb_rx];
  uint8_t prach_fmt = get_nr_prach_fmt(prach_ConfigIndex,fp->frame_type,fp->freq_range);

  rxsigF            = ru->prach_rxsigF;

  AssertFatal(ru->if_south == LOCAL_RF,"we shouldn't call this if if_south != LOCAL_RF\n");
  for (int aa=0; aa<ru->nb_rx; aa++) 
    prach[aa] = (int16_t*)&ru->common.rxdata[aa][(subframe*fp->samples_per_subframe)-ru->N_TA_offset];
  


  int mu = fp->numerology_index;
  int Ncp;
  int16_t *prach2;


  switch (prach_fmt) {
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

  case 0xa1:
    Ncp = 288/(1<<mu);
    break;

  case 0xa2:
    Ncp = 576/(1<<mu);
    break;

  case 0xa3:
    Ncp = 864/(1<<mu);
    break;

  case 0xb1:
    Ncp = 216/(1<<mu);
    break;

  case 0xb2:
    Ncp = 360/(1<<mu);
    break;

  case 0xb3:
    Ncp = 504/(1<<mu);
    break;

  case 0xb4:
    Ncp = 936/(1<<mu);
    break;

  case 0xc0:
    Ncp = 1240/(1<<mu);
    break;

  case 0xc2:
    Ncp = 2048/(1<<mu);
    break;

  default:
    AssertFatal(1==0,"unknown prach format %x\n",prach_fmt);
    break;
  }

  // Do forward transform
  if (LOG_DEBUGFLAG(PRACH)) {
    LOG_D(PHY,"rx_prach: Doing FFT for N_RB_UL %d nb_rx:%d Ncp:%d\n",fp->N_RB_UL, ru->nb_rx, Ncp);
  }
  AssertFatal(mu==1,"only 30 kHz SCS handled for now\n");

  // Note: Assumes PUSCH SCS @ 30 kHz, take values for formats 0-2 and adjust for others below
  int kbar = 1;
  int K    = 24;
  if (prach_fmt == 3) { 
    K=4;
    kbar=10;
  }
  else if (prach_fmt > 3) {
    // Note: Assumes that PRACH SCS is same as PUSCH SCS
    K=1;
    kbar=2;
  }
  int n_ra_prb            = fp->prach_config_common.prach_ConfigInfo.msg1_frequencystart;
  int k                   = (12*n_ra_prb) - 6*fp->N_RB_UL;

  int N_ZC = (prach_fmt<4)?839:139;

  if (k<0) k+=(fp->ofdm_symbol_size);
  
  k*=K;
  k+=kbar; 
  int reps=1;
  int dftlen=0;

  for (int aa=0; aa<ru->nb_rx; aa++) {
    AssertFatal(prach[aa]!=NULL,"prach[%d] is null\n",aa);

  
    // do DFT
    if (fp->N_RB_UL <= 100)
      AssertFatal(1==0,"N_RB_UL %d not support for NR PRACH yet\n",fp->N_RB_UL);
    else if (fp->N_RB_UL < 137) {
      if (fp->threequarter_fs==0) { 
	//40 MHz @ 61.44 Ms/s 
	//50 MHz @ 61.44 Ms/s
	prach2 = prach[aa] + (Ncp<<2);
	if (prach_fmt == 0 || prach_fmt == 1 || prach_fmt == 2)
	  dft(DFT_49152,prach2,rxsigF[aa],1);
	if (prach_fmt == 1 || prach_fmt == 2) {
	  dft(DFT_49152,prach2+98304,rxsigF[aa]+98304,1);
	  reps++;
	}
	if (prach_fmt == 2) {
	  dft(DFT_49152,prach2+(98304*2),rxsigF[aa]+(98304*2),1);
	  dft(DFT_49152,prach2+(98304*3),rxsigF[aa]+(98304*3),1);
	  reps+=2;
	}
	if (prach_fmt == 3) { 
	  for (int i=0;i<4;i++) dft(DFT_12288,prach2+(i*12288*2),rxsigF[aa]+(i*12288*2),1);
	  reps=4;
	}
	if (prach_fmt >3) {
	  dft(DFT_2048,prach2,rxsigF[aa],1);
	  if (prach_fmt != 0xc0) {
	    dft(DFT_2048,prach2+4096,rxsigF[aa]+4096,1);
	    reps++;
	  }
	} 
	if (prach_fmt == 0xa2 || prach_fmt == 0xa3 || prach_fmt == 0xb2 || prach_fmt == 0xb3 || prach_fmt == 0xb4 || prach_fmt == 0xc2) {     
	  dft(DFT_2048,prach2+4096*2,rxsigF[aa]+4096*2,1);
	  dft(DFT_2048,prach2+4096*3,rxsigF[aa]+4096*3,1);
	  reps+=2;
	} 
	if (prach_fmt == 0xa3 || prach_fmt == 0xb3 || prach_fmt == 0xc2) {     
	  dft(DFT_2048,prach2+4096*4,rxsigF[aa]+4096*4,1);
	  dft(DFT_2048,prach2+4096*5,rxsigF[aa]+4096*5,1);
	  reps+=2;
	} 
	if (prach_fmt == 0xc2) {
	  for (int i=6;i<11;i++) dft(DFT_2048,prach2+(3072*i),rxsigF[aa]+(3072*i),1);
	  reps+=6;
	}
      } else {
	//	40 MHz @ 46.08 Ms/s
	prach2 = prach[aa] + (3*Ncp);
	AssertFatal(fp->N_RB_UL <= 107,"cannot do 108..136 PRBs with 3/4 sampling\n");
	if (prach_fmt == 0 || prach_fmt == 1 || prach_fmt == 2) {
	  dft(DFT_36864,prach2,rxsigF[aa],1);
	  reps++;
	}
	if (prach_fmt == 1 || prach_fmt == 2) {
	  dft(DFT_36864,prach2+73728,rxsigF[aa]+73728,1);
	  reps++;
	}
	if (prach_fmt == 2) {
	  dft(DFT_36864,prach2+(98304*2),rxsigF[aa]+(98304*2),1);
	  dft(DFT_36864,prach2+(98304*3),rxsigF[aa]+(98304*3),1);
	  reps+=2;
	}
	if (prach_fmt == 3) {
	  for (int i=0;i<4;i++) dft(DFT_9216,prach2+(i*9216*2),rxsigF[aa]+(i*9216*2),1);
	  reps=4;
	}
	if (prach_fmt >3) {
	  dft(DFT_1536,prach2,rxsigF[aa],1);
	  if (prach_fmt != 0xc0) {
	    dft(DFT_1536,prach2+3072,rxsigF[aa]+3072,1);
	    reps++;
	  }
	}  
	if (prach_fmt == 0xa2 || prach_fmt == 0xa3 || prach_fmt == 0xb2 || prach_fmt == 0xb3 || prach_fmt == 0xb4 || prach_fmt == 0xc2) {     
	  dft(DFT_1536,prach2+3072*2,rxsigF[aa]+3072*2,1);
	  dft(DFT_1536,prach2+3072*3,rxsigF[aa]+3072*3,1);
	  reps+=2;
	} 
	if (prach_fmt == 0xa3 || prach_fmt == 0xb3 || prach_fmt == 0xc2) {     
	  dft(DFT_1536,prach2+3072*4,rxsigF[aa]+3072*4,1);
	  dft(DFT_1536,prach2+3072*5,rxsigF[aa]+3072*5,1);
	  reps+=2;
	} 
	if (prach_fmt == 0xc2) {
	  for (int i=6;i<11;i++) dft(DFT_1536,prach2+(3072*i),rxsigF[aa]+(3072*i),1);
	  reps+=6;
	}
      }
    }
    else if (fp->N_RB_UL <= 273) {
      if (fp->threequarter_fs==0) {
	prach2 = prach[aa] + (Ncp<<3); 
	dftlen=98304;
	//80,90,100 MHz @ 61.44 Ms/s 
	if (prach_fmt == 0 || prach_fmt == 1 || prach_fmt == 2)
	  dft(DFT_98304,prach2,rxsigF[aa],1);
	if (prach_fmt == 1 || prach_fmt == 2) {
	  dft(DFT_98304,prach2+196608,rxsigF[aa]+196608,1);
	  reps++;
	}
	if (prach_fmt == 1 || prach_fmt == 2) {
	  dft(DFT_98304,prach2+196608,rxsigF[aa]+196608,1);
	  dft(DFT_98304,prach2+(196608*2),rxsigF[aa]+(196608*2),1);
	  reps+=2;
	}
	if (prach_fmt == 3) {
	  dft(DFT_24576,prach2+(2*49152),rxsigF[aa]+(2*49152),1);
	  reps=4;
	  dftlen=24576;
	}
	if (prach_fmt >3) {
	  dftlen=4096;
	  dft(DFT_4096,prach2,rxsigF[aa],1);
	  if (prach_fmt != 0xc0) { 
	    dft(DFT_4096,prach2+8192,rxsigF[aa]+8192,1);
	    reps++;
	  }
	} 
	if (prach_fmt == 0xa2 || prach_fmt == 0xa3 || prach_fmt == 0xb2 || prach_fmt == 0xb3 || prach_fmt == 0xb4 || prach_fmt == 0xc2) {     
	  dft(DFT_4096,prach2+8192*2,rxsigF[aa]+8192*2,1);
	  dft(DFT_4096,prach2+8192*3,rxsigF[aa]+8192*3,1);
	  reps+=2;
	} 
	if (prach_fmt == 0xa3 || prach_fmt == 0xb3 || prach_fmt == 0xc2) {     
	  dft(DFT_4096,prach2+8192*4,rxsigF[aa]+8192*4,1);
	  dft(DFT_4096,prach2+8192*5,rxsigF[aa]+8192*5,1);
	  reps+=2;
	} 
	if (prach_fmt == 0xc2) {
	  for (int i=6;i<11;i++) dft(DFT_4096,prach2+(8192*i),rxsigF[aa]+(8192*i),1);
	  reps+=6;
	}
      } else {
	AssertFatal(fp->N_RB_UL <= 217,"cannot do more than 217 PRBs with 3/4 sampling\n");
	prach2 = prach[aa] + (6*Ncp);
	//	80 MHz @ 46.08 Ms/s
	dftlen=73728;
	if (prach_fmt == 0 || prach_fmt == 1 || prach_fmt == 2) {
	  dft(DFT_73728,prach2,rxsigF[aa],1);
	  reps++;
	}
	if (prach_fmt == 1 || prach_fmt == 2) {
	  dft(DFT_73728,prach2+(2*73728),rxsigF[aa]+(2*73728),1);
	  reps++;
	}
	if (prach_fmt == 3) {
	  dft(DFT_73728,prach2+(4*73728),rxsigF[aa]+(4*73728),1);
	  reps=4;
	  dftlen=18432;
	}

	if (prach_fmt >3) {
	  dftlen=3072;
	  dft(DFT_3072,prach2,rxsigF[aa],1);
	  if (prach_fmt != 0xc0) {
	    dft(DFT_3072,prach2+6144,rxsigF[aa]+6144,1);
	    reps++;
	  }
	} 
	if (prach_fmt == 0xa2 || prach_fmt == 0xa3 || prach_fmt == 0xb2 || prach_fmt == 0xb3 || prach_fmt == 0xb4 || prach_fmt == 0xc2) {     
	  dft(DFT_3072,prach2+6144*2,rxsigF[aa]+6144*2,1);
	  dft(DFT_3072,prach2+6144*3,rxsigF[aa]+6144*3,1);
	  reps+=2;
	} 
	if (prach_fmt == 0xa3 || prach_fmt == 0xb3 || prach_fmt == 0xc2) {     
	  dft(DFT_3072,prach2+6144*4,rxsigF[aa]+6144*4,1);
	  dft(DFT_3072,prach2+6144*5,rxsigF[aa]+6144*5,1);
	  reps+=2;
	} 
	if (prach_fmt == 0xc2) {
	  for (int i=6;i<11;i++) dft(DFT_3072,prach2+(6144*i),rxsigF[aa]+(6144*i),1);
	  reps+=6;
	}
      }
    }

    //Coherent combining of PRACH repetitions (assumes channel does not change, to be revisted for "long" PRACH)
    int16_t rxsigF_tmp[N_ZC<<1];
    //    if (k+N_ZC > dftlen) { // PRACH signal is split around DC 
    int16_t *rxsigF2=rxsigF[aa];
    int k2=k<<1;

    for (int j=0;j<N_ZC<<1;j++,k2++) {
      if (k2==(dftlen<<1)) k2=0;
      rxsigF_tmp[j] = rxsigF2[k2];
      for (int i=1;i<reps;i++) rxsigF_tmp[j] += rxsigF2[k2+(i*N_ZC<<1)];
    }
    memcpy((void*)rxsigF2,(void *)rxsigF_tmp,N_ZC<<2);

  }

}

void rx_nr_prach(PHY_VARS_gNB *gNB,
		 int frame,
		 int subframe,
		 uint16_t *max_preamble,
		 uint16_t *max_preamble_energy,
		 uint16_t *max_preamble_delay
		 )
{
  AssertFatal(gNB!=NULL,"Can only be called from gNB\n");

  int i;

  NR_DL_FRAME_PARMS *fp;
  lte_frame_type_t   frame_type;
  uint16_t           rootSequenceIndex;  
  uint8_t            prach_ConfigIndex;   
  uint8_t            Ncs_config;          
  uint8_t            restricted_set;      
  uint8_t            n_ra_prb;
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
  
  nr_frequency_range_e freq_range;

  fp    = &gNB->frame_parms;
  nb_rx = fp->nb_antennas_rx;
  
  frame_type          = fp->frame_type;
  freq_range          = fp->freq_range;
  rootSequenceIndex   = fp->prach_config_common.rootSequenceIndex;
  prach_ConfigIndex   = fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
  Ncs_config          = fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig;
  restricted_set      = fp->prach_config_common.prach_ConfigInfo.highSpeedFlag;
   

  uint8_t prach_fmt = get_nr_prach_fmt(prach_ConfigIndex,frame_type,freq_range);
  uint16_t N_ZC = (prach_fmt <4)?839:139;

  prach_ifft        = gNB->prach_vars.prach_ifft;
  prachF            = gNB->prach_vars.prachF;
  if (LOG_DEBUGFLAG(PRACH)){
    if ((frame&1023) < 20) LOG_D(PHY,"PRACH (gNB) : running rx_prach for subframe %d, msg1_frequencystart %d, prach_ConfigIndex %d , rootSequenceIndex %d\n", subframe,fp->prach_config_common.prach_ConfigInfo.msg1_frequencystart,prach_ConfigIndex,rootSequenceIndex);
  }






  int restricted_Type = 0; //this is hardcoded ('0' for restricted_TypeA; and '1' for restricted_TypeB). FIXME
  if (prach_fmt<3){
    if (restricted_set == 0) {
      NCS = NCS_unrestricted_delta_f_RA_125[Ncs_config];
    } else {
      if (restricted_Type == 0) NCS = NCS_restricted_TypeA_delta_f_RA_125[Ncs_config]; // for TypeA, this is hardcoded. FIXME
      if (restricted_Type == 1) NCS = NCS_restricted_TypeB_delta_f_RA_125[Ncs_config]; // for TypeB, this is hardcoded. FIXME
    }
  }
  if (prach_fmt==3){
    if (restricted_set == 0) {
      NCS = NCS_unrestricted_delta_f_RA_5[Ncs_config];
    } else {
      if (restricted_Type == 0) NCS = NCS_restricted_TypeA_delta_f_RA_5[Ncs_config]; // for TypeA, this is hardcoded. FIXME
      if (restricted_Type == 1) NCS = NCS_restricted_TypeB_delta_f_RA_5[Ncs_config]; // for TypeB, this is hardcoded. FIXME
    }
  }
  if (prach_fmt>3){
    NCS = NCS_unrestricted_delta_f_RA_15[Ncs_config];
  }

  AssertFatal(NCS!=99,"NCS has not been set\n");

  start_meas(&gNB->rx_prach);

  prach_root_sequence_map = (prach_fmt<4) ? prach_root_sequence_map_0_3 : prach_root_sequence_map_abc;

  // PDP is oversampled, e.g. 1024 sample instead of 839
  // Adapt the NCS (zero-correlation zones) with oversampling factor e.g. 1024/839
  NCS2 = (N_ZC==839) ? ((NCS<<10)/839) : ((NCS<<8)/139);

  if (NCS2==0) NCS2 = N_ZC;


  preamble_offset_old = 99;

  
  *max_preamble_energy=0;
  for (preamble_index=0 ; preamble_index<64 ; preamble_index++) {

    if (LOG_DEBUGFLAG(PRACH)){
      int en = dB_fixed(signal_energy((int32_t*)&rxsigF[0][0],(N_ZC==839) ? 840: 140));
      if (en>60) LOG_I(PHY,"frame %d, subframe %d : Trying preamble %d \n",frame,subframe,preamble_index);
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

      // set preamble_offset to initial rootSequenceIndex and look if we need more root sequences for this
      // preamble index and find the corresponding cyclic shift
      // Check if all shifts for that root have been processed
      if (preamble_index0 == numshift) {
        not_found = 1;
        new_dft   = 1;
        preamble_index0 -= numshift;
        (preamble_offset==0 && numshift==0) ? (preamble_offset) : (preamble_offset++);

        while (not_found == 1) {
          // current root depending on rootSequenceIndex
          int index = (rootSequenceIndex + preamble_offset) % N_ZC;

	  if (prach_fmt<4) {
	    // prach_root_sequence_map points to prach_root_sequence_map0_3
	    DevAssert( index < sizeof(prach_root_sequence_map_0_3) / sizeof(prach_root_sequence_map_0_3[0]) );
	  } else {
	    // prach_root_sequence_map points to prach_root_sequence_map4
	    DevAssert( index < sizeof(prach_root_sequence_map_abc) / sizeof(prach_root_sequence_map_abc[0]) );
	  }
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
      if (en>60) LOG_I(PHY,"frame %d, subframe %d : preamble index %d, NCS %d, NCS_config %d, N_ZC/NCS %d: offset %d, preamble shift %d , en %d)\n",
		       frame,subframe,preamble_index,NCS,Ncs_config,N_ZC/NCS,preamble_offset,preamble_shift,en);
    }

    if (new_dft == 1) {
      new_dft = 0;

      Xu=(int16_t*)gNB->X_u[preamble_offset-first_nonzero_root_idx];
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
	    prach_ifft[i] += (prach_ifft_tmp[i<<1]*prach_ifft_tmp[i<<1] + prach_ifft_tmp[1+(i<<1)]*prach_ifft_tmp[1+(i<<1)])>>10;
	} else {
	  idft(IDFT_256,prachF,prach_ifft_tmp,1);
	  log2_ifft_size = 8;
	  // compute energy and accumulate over receive antennas and repetitions for BR
	  for (i=0;i<256;i++)
	    prach_ifft[i] += (prach_ifft_tmp[i<<1]*prach_ifft_tmp[(i<<1)] + prach_ifft_tmp[1+(i<<1)]*prach_ifft_tmp[1+(i<<1)])>>10;
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
	*max_preamble_energy  = levdB;
	*max_preamble_delay   = i; // Note: This has to be normalized to the 30.72 Ms/s sampling rate 
	*max_preamble         = preamble_index;
      }
    }
  }// preamble_index

  if (LOG_DUMPFLAG(PRACH)) {
    int en = dB_fixed(signal_energy((int32_t*)&rxsigF[0][0],840));  
    if (en>60) {
      int k = (12*n_ra_prb) - 6*fp->N_RB_UL;
      
      if (k<0) k+=fp->ofdm_symbol_size;
      
      k*=12;
      k+=13;
      k*=2;
      

      LOG_M("rxsigF.m","prach_rxF",&rxsigF[0][0],12288,1,1);
      LOG_M("prach_rxF_comp0.m","prach_rxF_comp0",prachF,1024,1,1);
      LOG_M("Xu.m","xu",Xu,N_ZC,1,1);
      LOG_M("prach_ifft0.m","prach_t0",prach_ifft,1024,1,1);
    }
  } /* LOG_DUMPFLAG(PRACH) */
  if (gNB) stop_meas(&gNB->rx_prach);

}

