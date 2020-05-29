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

/*! \file PHY/LTE_TRANSPORT/prach_common.c
 * \brief Common routines for UE/eNB PRACH physical channel V8.6 2009-03
 * \author R. Knopp
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */
#include "PHY/sse_intrin.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "PHY/impl_defs_nr.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include <openair1/PHY/NR_TRANSPORT/nr_transport_proto_common.h>

#include "T.h"
#include <openair1/PHY/NR_TRANSPORT/nr_transport_proto_common.h>




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








int32_t generate_nr_prach( PHY_VARS_NR_UE *ue, uint8_t eNB_id, uint8_t subframe, uint16_t Nf )
{

  //lte_frame_type_t frame_type         = ue->frame_parms.frame_type;
  //uint8_t tdd_config         = ue->frame_parms.tdd_config;
  NR_DL_FRAME_PARMS *fp=&ue->frame_parms;
  uint16_t rootSequenceIndex = fp->prach_config_common.rootSequenceIndex;
  uint8_t prach_ConfigIndex  = fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
  uint8_t Ncs_config         = fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig;
  uint8_t restricted_set     = fp->prach_config_common.prach_ConfigInfo.highSpeedFlag;
  uint8_t preamble_index     = ue->prach_resources[eNB_id]->ra_PreambleIndex;
  //uint8_t tdd_mapindex       = ue->prach_resources[eNB_id]->ra_TDD_map_index;
  int16_t *prachF           = ue->prach_vars[eNB_id]->prachF;
  int16_t prach_tmp[98304*2*4] __attribute__((aligned(32)));
  int16_t *prach            = prach_tmp;
  int16_t *prach2;
  int16_t amp               = ue->prach_vars[eNB_id]->amp;
  int16_t Ncp;
  uint16_t NCS=0;
  uint16_t *prach_root_sequence_map;
  uint16_t preamble_offset,preamble_shift;
  uint16_t preamble_index0,n_shift_ra,n_shift_ra_bar;
  uint16_t d_start=-1,numshift;

  uint16_t prach_fmt = get_nr_prach_fmt(prach_ConfigIndex,fp->frame_type,fp->freq_range);
  //uint8_t Nsp=2;
  //uint8_t f_ra,t1_ra;
  uint16_t N_ZC = (prach_fmt<4)?839:139;
  uint8_t not_found;
  int16_t *Xu;
  uint16_t u;
  int32_t Xu_re,Xu_im;
  uint16_t offset,offset2;
  int prach_start;
  int i, prach_len=0;
  uint16_t first_nonzero_root_idx=0;

#if defined(OAI_USRP)
  prach_start =  (ue->rx_offset+subframe*fp->samples_per_subframe-ue->hw_timing_advance-ue->N_TA_offset);
#ifdef NR_PRACH_DEBUG
    LOG_I(PHY,"[UE %d] prach_start %d, rx_offset %d, hw_timing_advance %d, N_TA_offset %d\n", ue->Mod_id,
        prach_start,
        ue->rx_offset,
        ue->hw_timing_advance,
        ue->N_TA_offset);
#endif

  if (prach_start<0)
    prach_start+=(fp->samples_per_subframe*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);

  if (prach_start>=(fp->samples_per_subframe*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME))
    prach_start-=(fp->samples_per_subframe*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);

#else //normal case (simulation)
  prach_start = subframe*fp->samples_per_subframe-ue->N_TA_offset;
  LOG_D(PHY,"[UE %d] prach_start %d, rx_offset %d, hw_timing_advance %d, N_TA_offset %d\n", ue->Mod_id,
    prach_start,
    ue->rx_offset,
    ue->hw_timing_advance,
    ue->N_TA_offset);

#endif


  // First compute physical root sequence
  /************************************************************************
  * 4G and NR NCS tables are slightly different and depend on prach format
  * Table 6.3.3.1-5:  for preamble formats with delta_f_RA = 1.25 Khz (formats 0,1,2)
  * Table 6.3.3.1-6:  for preamble formats with delta_f_RA = 5 Khz (formats 3)
  * NOTE: Restricted set type B is not implemented
  *************************************************************************/

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

  prach_root_sequence_map = (prach_fmt<4) ? prach_root_sequence_map_0_3 : prach_root_sequence_map_abc;

  // This is the relative offset (for unrestricted case) in the root sequence table (5.7.2-4 from 36.211) for the given preamble index
  preamble_offset = ((NCS==0)? preamble_index : (preamble_index/(N_ZC/NCS)));

  if (restricted_set == 0) {
    // This is the \nu corresponding to the preamble index
    preamble_shift  = (NCS==0)? 0 : (preamble_index % (N_ZC/NCS));
    preamble_shift *= NCS;
  } else { // This is the high-speed case

#ifdef NR_PRACH_DEBUG
    LOG_I(PHY,"[UE %d] High-speed mode, NCS_config %d\n",ue->Mod_id,Ncs_config);
#endif

    not_found = 1;
    preamble_index0 = preamble_index;
    // set preamble_offset to initial rootSequenceIndex and look if we need more root sequences for this
    // preamble index and find the corresponding cyclic shift
    preamble_offset = 0; // relative rootSequenceIndex;

    while (not_found == 1) {
      // current root depending on rootSequenceIndex and preamble_offset
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

      if (numshift>0 && preamble_index0==preamble_index)
        first_nonzero_root_idx = preamble_offset;

      if (preamble_index0 < numshift) {
        not_found      = 0;
        preamble_shift = (d_start * (preamble_index0/n_shift_ra)) + ((preamble_index0%n_shift_ra)*NCS);

      } else { // skip to next rootSequenceIndex and recompute parameters
        preamble_offset++;
        preamble_index0 -= numshift;
      }
    }
  }

  // now generate PRACH signal
#ifdef NR_PRACH_DEBUG

  if (NCS>0)
    LOG_D(PHY,"Generate PRACH for RootSeqIndex %d, Preamble Index %d, PRACH Format %x, prach_ConfigIndex %d, NCS %d (NCS_config %d, N_ZC/NCS %d): Preamble_offset %d, Preamble_shift %d\n",
          rootSequenceIndex,preamble_index,prach_fmt,prach_ConfigIndex,NCS,Ncs_config,N_ZC/NCS,
          preamble_offset,preamble_shift);

#endif

  //  nsymb = (frame_parms->Ncp==0) ? 14:12;
  //  subframe_offset = (unsigned int)frame_parms->ofdm_symbol_size*subframe*nsymb;
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

  k = (12*n_ra_prb) - 6*fp->N_RB_UL;

  if (k<0) k+=fp->ofdm_symbol_size;

  k*=K;
  k+=kbar;

  LOG_D(PHY,"placing prach in position %d\n",k);
  k*=2;

  Xu = (int16_t*)ue->X_u[preamble_offset-first_nonzero_root_idx];

  /********************************************************
   *
   * In function init_prach_tables:
   * to compute quantized roots of unity ru(n) = 32767 * exp j*[ (2 * PI * n) / N_ZC ]
   *
   * In compute_prach_seq:
   * to calculate Xu = DFT xu = xu (inv_u*k) * Xu[0] (This is a Zadoff-Chou sequence property: DFT ZC sequence is another ZC sequence)
   *
   * In generate_prach:
   * to do the cyclic-shifted DFT by multiplying Xu[k] * ru[k*preamble_shift] as:
   * If X[k] = DFT x(n) -> X_shifted[k] = DFT x(n+preamble_shift) = X[k] * exp -j*[ (2*PI*k*preamble_shift) / N_ZC ]
   *
   *********************************************************/

  AssertFatal(prach_fmt>3,"prach_fmt<=3: Fix this for other formats\n");
  int dftlen=2048;
  if (fp->N_RB_UL >= 137) dftlen=4096;
  if (fp->threequarter_fs==1) dftlen=(3*dftlen)/4;

  for (offset=0,offset2=0; offset<N_ZC; offset++,offset2+=preamble_shift) {

    if (offset2 >= N_ZC)
      offset2 -= N_ZC;

    Xu_re = (((int32_t)Xu[offset<<1]*amp)>>15);
    Xu_im = (((int32_t)Xu[1+(offset<<1)]*amp)>>15);
    prachF[k++]= ((Xu_re*nr_ru[offset2<<1]) - (Xu_im*nr_ru[1+(offset2<<1)]))>>15;
    prachF[k++]= ((Xu_im*nr_ru[offset2<<1]) + (Xu_re*nr_ru[1+(offset2<<1)]))>>15;
    if (k==dftlen) k=0;
  }

  switch (fp->N_RB_UL) {
  case 6:
    memset((void*)prachF,0,4*1536);
    break;

  case 15:
    memset((void*)prachF,0,4*3072);
    break;

  case 25:
    memset((void*)prachF,0,4*6144);
    break;

  case 50:
    memset((void*)prachF,0,4*12288);
    break;

  case 75:
    memset((void*)prachF,0,4*18432);
    break;

  case 100:
    if (fp->threequarter_fs == 0)
      memset((void*)prachF,0,4*24576);
    else
      memset((void*)prachF,0,4*18432);
    break;

  case 106:
    memset((void*)prachF,0,4*24576);
    break;
}

  int mu = 1; // numerology is hardcoded. FIXME!!!
  switch (prach_fmt) {
  case 0:
    Ncp = 3168;
    break;

  case 1:
    Ncp = 2*21024;
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
    Ncp = 3168;
    break;
  }


  if (fp->N_RB_UL <= 100)
    AssertFatal(1==0,"N_RB_UL %d not supported for NR PRACH yet\n",fp->N_RB_UL);
  else if (fp->N_RB_UL < 137) { // 46.08 or 61.44 Ms/s
    if (fp->threequarter_fs==0) { //61.44 Ms/s  
      Ncp<<=1;
      // This is after cyclic prefix (Ncp<<1 samples for 30.72 Ms/s, Ncp<<2 samples for 61.44 Ms/s
      prach2 = prach+(Ncp<<1);
      if (prach_fmt == 0) { //24576 samples @ 30.72 Ms/s, 49152 samples @ 61.44 Ms/s
	idft(IDFT_49152,prachF,prach2,1);
	// here we have |empty | Prach49152|
	memmove(prach,prach+(49152<<1),(Ncp<<3));
	// here we have |Prefix | Prach49152|
	prach_len = 49152+Ncp;
	dftlen=49152;
      }
      else if (prach_fmt == 1) { //24576 samples @ 30.72 Ms/s, 49152 samples @ 61.44 Ms/s
	idft(IDFT_49152,prachF,prach2,1);
	memmove(prach2+(49152<<1),prach2,(49152<<2));
	// here we have |empty | Prach49152 | Prach49152|
	memmove(prach,prach+(49152<<2),(Ncp<<3));
	// here we have |Prefix | Prach49152 | Prach49152|
	prach_len = (49152*2)+Ncp;
	dftlen=49152;
      }
      else if (prach_fmt == 2) { //24576 samples @ 30.72 Ms/s, 49152 samples @ 61.44 Ms/s
	idft(IDFT_49152,prachF,prach2,1);
	memmove(prach2+(49152<<1),prach2,(49152<<2));
	// here we have |empty | Prach49152 | Prach49152| empty49152 | empty49152
	memmove(prach2+(49152<<2),prach2,(49152<<3));
	// here we have |empty | Prach49152 | Prach49152| Prach49152 | Prach49152
	memmove(prach,prach+(49152<<3),(Ncp<<3));
	// here we have |Prefix | Prach49152 | Prach49152| Prach49152 | Prach49152
	prach_len = (49152*4)+Ncp;
	dftlen=49152;
      }
      else if (prach_fmt == 3) { // //6144 samples @ 30.72 Ms/s, 12288 samples @ 61.44 Ms/s
	idft(IDFT_12288,prachF,prach2,1);
	memmove(prach2+(12288<<1),prach2,(12288<<2));
	// here we have |empty | Prach12288 | Prach12288| empty12288 | empty12288
	memmove(prach2+(12288<<2),prach2,(12288<<3));
	// here we have |empty | Prach12288 | Prach12288| Prach12288 | Prach12288
	memmove(prach,prach+(12288<<3),(Ncp<<3));
	// here we have |Prefix | Prach12288 | Prach12288| Prach12288 | Prach12288
	prach_len = (12288*4)+Ncp;
	dftlen=12288;
      }
      else if (prach_fmt == 0xa1 || prach_fmt == 0xb1 || prach_fmt == 0xc0) {
	prach2 = prach+(Ncp<<1);
	idft(IDFT_2048,prachF,prach2,1);
	dftlen=2048;
	// here we have |empty | Prach2048 |
	if (prach_fmt != 0xc0) {
	  memmove(prach2+(2048<<1),prach2,(2048<<2));
	  prach_len = (2048*2)+Ncp;
	}
	else prach_len = (2048*1)+Ncp;
	memmove(prach,prach+(2048<<1),(Ncp<<2));
	// here we have |Prefix | Prach2048 | Prach2048 (if ! 0xc0)  | 
      }
      else if (prach_fmt == 0xa2 || prach_fmt == 0xb2) { // 6x2048
	idft(IDFT_2048,prachF,prach2,1);
	dftlen=2048;
	// here we have |empty | Prach2048 |
	memmove(prach2+(2048<<1),prach2,(2048<<2));
	// here we have |empty | Prach2048 | Prach2048| empty2048 | empty2048 | 
	memmove(prach2+(2048<<2),prach2,(2048<<3));
	// here we have |empty | Prach2048 | Prach2048| Prach2048 | Prach2048 | 
	memmove(prach,prach+(2048<<1),(Ncp<<2));
	// here we have |Prefix | Prach2048 |
	prach_len = (2048*4)+Ncp; 
      }
      else if (prach_fmt == 0xa3 || prach_fmt == 0xb3) { // 6x2048
	prach2 = prach+(Ncp<<1);
	idft(IDFT_2048,prachF,prach2,1);
	dftlen=2048;
	// here we have |empty | Prach2048 |
	memmove(prach2+(2048<<1),prach2,(2048<<2));
	// here we have |empty | Prach2048 | Prach2048| empty2048 | empty2048 | empty2048 | empty2048
	memmove(prach2+(2048<<2),prach2,(2048<<3));
	// here we have |empty | Prach2048 | Prach2048| Prach2048 | Prach2048 | empty2048 | empty2048
	memmove(prach2+((2048<<1)*3),prach2,(2048<<3));
	// here we have |empty | Prach2048 | Prach2048| Prach2048 | Prach2048 | Prach2048 | Prach2048
	memmove(prach,prach+(2048<<1),(Ncp<<2));
	// here we have |Prefix | Prach2048 |
	prach_len = (2048*6)+Ncp; 
      }
      else if (prach_fmt == 0xb4) { // 12x2048
	idft(IDFT_2048,prachF,prach2,1);
	dftlen=2048;
	// here we have |empty | Prach2048 |
	memmove(prach2+(2048<<1),prach2,(2048<<2));
	// here we have |empty | Prach2048 | Prach2048| empty2048 | empty2048 | empty2048 | empty2048
	memmove(prach2+(2048<<2),prach2,(2048<<3));
	// here we have |empty | Prach2048 | Prach2048| Prach2048 | Prach2048 | empty2048 | empty2048
	memmove(prach2+(2048<<3),prach2,(2048<<3));
	// here we have |empty | Prach2048 | Prach2048| Prach2048 | Prach2048 | Prach2048 | Prach2048
	memmove(prach2+(2048<<1)*6,prach2,(2048<<2)*6);
	// here we have |empty | Prach2048 | Prach2048| Prach2048 | Prach2048 | Prach2048 | Prach2048 | Prach2048 | Prach2048| Prach2048 | Prach2048 | Prach2048 | Prach2048|
	memmove(prach,prach+(2048<<1),(Ncp<<2));
	// here we have |Prefix | Prach2048 | Prach2048| Prach2048 | Prach2048 | Prach2048 | Prach2048 | Prach2048 | Prach2048| Prach2048 | Prach2048 | Prach2048 | Prach2048|
	prach_len = (2048*12)+Ncp;
      }
      
    }
    else {     // 46.08 Ms/s
      Ncp = (Ncp*3)/2;
      prach2 = prach+(Ncp<<1);
      if (prach_fmt == 0) {
	idft(IDFT_36864,prachF,prach2,1);
	dftlen=36864;
	// here we have |empty | Prach73728|
	memmove(prach,prach+(36864<<1),(Ncp<<2));
	// here we have |Prefix | Prach73728|
	prach_len = (36864*1)+Ncp;
      }
      else if (prach_fmt == 1) {
	idft(IDFT_36864,prachF,prach2,1);
	dftlen=36864;
	memmove(prach2+(36864<<1),prach2,(36864<<2));
	// here we have |empty | Prach73728 | Prach73728|
	memmove(prach,prach+(36864<<2),(Ncp<<2));
	// here we have |Prefix | Prach73728 | Prach73728|
	prach_len = (36864*2)+Ncp;
      }
      if (prach_fmt == 2) {
	idft(IDFT_36864,prachF,prach2,1);
	dftlen=36864;
	memmove(prach2+(36864<<1),prach2,(36864<<2));
	// here we have |empty | Prach73728 | Prach73728| empty73728 | empty73728
	memmove(prach2+(36864<<2),prach2,(36864<<3));
	// here we have |empty | Prach73728 | Prach73728| Prach73728 | Prach73728
	memmove(prach,prach+(36864<<3),(Ncp<<2));
	// here we have |Prefix | Prach73728 | Prach73728| Prach73728 | Prach73728
	prach_len = (36864*4)+Ncp;
      }
      else if (prach_fmt == 3) {
	idft(IDFT_9216,prachF,prach2,1);
	dftlen=36864;
	memmove(prach2+(9216<<1),prach2,(9216<<2));
	// here we have |empty | Prach9216 | Prach9216| empty9216 | empty9216
	memmove(prach2+(9216<<2),prach2,(9216<<3));
	// here we have |empty | Prach9216 | Prach9216| Prach9216 | Prach9216
	memmove(prach,prach+(9216<<3),(Ncp<<2));
	// here we have |Prefix | Prach9216 | Prach9216| Prach9216 | Prach9216
	prach_len = (9216*4)+Ncp;
      }
      else if (prach_fmt == 0xa1 || prach_fmt == 0xb1 || prach_fmt == 0xc0) {
	idft(IDFT_1536,prachF,prach2,1);
	dftlen=1536;
	// here we have |empty | Prach1536 |
	if (prach_fmt != 0xc0)
	  memmove(prach2+(1536<<1),prach2,(1536<<2));
	memmove(prach,prach+(1536<<1),(Ncp<<2));
	// here we have |Prefix | Prach1536 | Prach1536 (if ! 0xc0)  | 
	prach_len = (1536*2)+Ncp;
      }
      else if (prach_fmt == 0xa2 || prach_fmt == 0xb2) { // 6x1536
	idft(IDFT_1536,prachF,prach2,1);
	dftlen=1536;
	// here we have |empty | Prach1536 |
	memmove(prach2+(1536<<1),prach2,(1536<<2));
	// here we have |empty | Prach1536 | Prach1536| empty1536 | empty1536 |
	memmove(prach2+(1536<<2),prach2,(1536<<3));
	// here we have |empty | Prach1536 | Prach1536| Prach1536 | Prach1536 |
	memmove(prach,prach+(1536<<1),(Ncp<<2));
	// here we have |Prefix | Prach1536 |
	prach_len = (1536*4)+Ncp; 
      }
      else if (prach_fmt == 0xa3 || prach_fmt == 0xb3) { // 6x1536
	idft(IDFT_1536,prachF,prach2,1);
	dftlen=1536;
	// here we have |empty | Prach1536 |
	memmove(prach2+(1536<<1),prach2,(1536<<2));
	// here we have |empty | Prach1536 | Prach1536| empty1536 | empty1536 | empty1536 | empty1536
	memmove(prach2+(1536<<2),prach2,(1536<<3));
	// here we have |empty | Prach1536 | Prach1536| Prach1536 | Prach1536 | empty1536 | empty1536
	memmove(prach2+((1536<<1)*3),prach2,(1536<<3));
	// here we have |empty | Prach1536 | Prach1536| Prach1536 | Prach1536 | Prach1536 | Prach1536
	memmove(prach,prach+(1536<<1),(Ncp<<2));
	// here we have |Prefix | Prach1536 | 
	prach_len = (1536*6)+Ncp; 
      }
      else if (prach_fmt == 0xb4) { // 12x1536
	idft(IDFT_1536,prachF,prach2,1);
	dftlen=1536;
	// here we have |empty | Prach1536 |
	memmove(prach2+(1536<<1),prach2,(1536<<2));
	// here we have |empty | Prach1536 | Prach1536| empty1536 | empty1536 | empty1536 | empty1536
	memmove(prach2+(1536<<2),prach2,(1536<<3));
	// here we have |empty | Prach1536 | Prach1536| Prach1536 | Prach1536 | empty1536 | empty1536
	memmove(prach2+(1536<<3),prach2,(1536<<3));
	// here we have |empty | Prach1536 | Prach1536| Prach1536 | Prach1536 | Prach1536 | Prach1536
	memmove(prach2+(1536<<1)*6,prach2,(1536<<2)*6);
	// here we have |empty | Prach1536 | Prach1536| Prach1536 | Prach1536 | Prach1536 | Prach1536 | Prach1536 | Prach1536| Prach1536 | Prach1536 | Prach1536 | Prach1536|
	memmove(prach,prach+(1536<<1),(Ncp<<2));
	// here we have |Prefix | Prach1536 | Prach1536| Prach1536 | Prach1536 | Prach1536 | Prach1536 | Prach1536 | Prach1536| Prach1536 | Prach1536 | Prach1536 | Prach1536|
	prach_len = (1536*12)+Ncp; 
      }      
    }
  }
  else if (fp->N_RB_UL <= 273) {// 92.16 or 122.88 Ms/s
    if (fp->threequarter_fs==0) { //122.88 Ms/s
      Ncp<<=2;
      prach2 = prach+(Ncp<<1);
      if (prach_fmt == 0) { //24576 samples @ 30.72 Ms/s, 98304 samples @ 122.88 Ms/s
	idft(IDFT_98304,prachF,prach2,1);
	dftlen=98304;
	// here we have |empty | Prach98304|
	memmove(prach,prach+(98304<<1),(Ncp<<2));
	// here we have |Prefix | Prach98304|
	prach_len = (98304*1)+Ncp;
      }
      else if (prach_fmt == 1) {
	idft(IDFT_98304,prachF,prach2,1);
	dftlen=98304;
	memmove(prach2+(98304<<1),prach2,(98304<<2));
	// here we have |empty | Prach98304 | Prach98304|
	memmove(prach,prach+(98304<<2),(Ncp<<2));
	// here we have |Prefix | Prach98304 | Prach98304|
	prach_len = (98304*2)+Ncp;
      }
      else if (prach_fmt == 2) {
	idft(IDFT_98304,prachF,prach2,1);
	dftlen=98304;
	memmove(prach2+(98304<<1),prach2,(98304<<2));
	// here we have |empty | Prach98304 | Prach98304| empty98304 | empty98304
	memmove(prach2+(98304<<2),prach2,(98304<<3));
	// here we have |empty | Prach98304 | Prach98304| Prach98304 | Prach98304
	memmove(prach,prach+(98304<<3),(Ncp<<2));
	// here we have |Prefix | Prach98304 | Prach98304| Prach98304 | Prach98304
	prach_len = (98304*4)+Ncp;
      }
      else if (prach_fmt == 3) { // 4x6144, Ncp 3168
	idft(IDFT_24576,prachF,prach2,1);
	dftlen=24576;
	memmove(prach2+(24576<<1),prach2,(24576<<2));
	// here we have |empty | Prach24576 | Prach24576| empty24576 | empty24576
	memmove(prach2+(24576<<2),prach2,(24576<<3));
	// here we have |empty | Prach24576 | Prach24576| Prach24576 | Prach24576
	memmove(prach,prach+(24576<<3),(Ncp<<2));
	// here we have |Prefix | Prach24576 | Prach24576| Prach24576 | Prach24576
	prach_len = (24576*4)+(Ncp<<1);
      }
      else if (prach_fmt == 0xa1 || prach_fmt == 0xb1 || prach_fmt == 0xc0) {
	idft(IDFT_4096,prachF,prach2,1);
	dftlen=4096;
	// here we have |empty | Prach4096 |
	if (prach_fmt != 0xc0) {
	  memmove(prach2+(4096<<1),prach2,(4096<<2));
	  prach_len = (4096*2)+Ncp; 
	}
	else 	prach_len = (4096*1)+Ncp; 
	memmove(prach,prach+(4096<<1),(Ncp<<2));
	// here we have |Prefix | Prach4096 | Prach4096 (if ! 0xc0)  | 

      }
      else if (prach_fmt == 0xa2 || prach_fmt == 0xb2) { // 4x4096
	idft(IDFT_4096,prachF,prach2,1);
	dftlen=4096;
	// here we have |empty | Prach4096 |
	memmove(prach2+(4096<<1),prach2,(4096<<2));
	// here we have |empty | Prach4096 | Prach4096| empty4096 | empty4096 |
	memmove(prach2+(4096<<2),prach2,(4096<<3));
	// here we have |empty | Prach4096 | Prach4096| Prach4096 | Prach4096 |
	memmove(prach,prach+(4096<<1),(Ncp<<2));
	// here we have |Prefix | Prach4096 |
	prach_len = (4096*4)+Ncp;  
      }
      else if (prach_fmt == 0xa3 || prach_fmt == 0xb3) { // 6x4096
	idft(IDFT_4096,prachF,prach2,1);
	dftlen=4096;
	// here we have |empty | Prach4096 |
	memmove(prach2+(4096<<1),prach2,(4096<<2));
	// here we have |empty | Prach4096 | Prach4096| empty4096 | empty4096 | empty4096 | empty4096
	memmove(prach2+(4096<<2),prach2,(4096<<3));
	// here we have |empty | Prach4096 | Prach4096| Prach4096 | Prach4096 | empty4096 | empty4096
	memmove(prach2+((4096<<1)*3),prach2,(4096<<3));
	// here we have |empty | Prach4096 | Prach4096| Prach4096 | Prach4096 | Prach4096 | Prach4096
	memmove(prach,prach+(4096<<1),(Ncp<<2));
	// here we have |Prefix | Prach4096 | 
	prach_len = (4096*6)+Ncp; 
      }
      else if (prach_fmt == 0xb4) { // 12x4096
	idft(IDFT_4096,prachF,prach2,1);
	dftlen=4096;
	// here we have |empty | Prach4096 |
	memmove(prach2+(4096<<1),prach2,(4096<<2));
	// here we have |empty | Prach4096 | Prach4096| empty4096 | empty4096 | empty4096 | empty4096
	memmove(prach2+(4096<<2),prach2,(4096<<3));
	// here we have |empty | Prach4096 | Prach4096| Prach4096 | Prach4096 | empty4096 | empty4096
	memmove(prach2+(4096<<3),prach2,(4096<<3));
	// here we have |empty | Prach4096 | Prach4096| Prach4096 | Prach4096 | Prach4096 | Prach4096
	memmove(prach2+(4096<<1)*6,prach2,(4096<<2)*6);
	// here we have |empty | Prach4096 | Prach4096| Prach4096 | Prach4096 | Prach4096 | Prach4096 | Prach4096 | Prach4096| Prach4096 | Prach4096 | Prach4096 | Prach4096|
	memmove(prach,prach+(4096<<1),(Ncp<<2));
	// here we have |Prefix | Prach4096 | Prach4096| Prach4096 | Prach4096 | Prach4096 | Prach4096 | Prach4096 | Prach4096| Prach4096 | Prach4096 | Prach4096 | Prach4096|
	prach_len = (4096*12)+Ncp; 
      }
    }
    else {     // 92.16 Ms/s
      Ncp = (Ncp*3);
      prach2 = prach+(Ncp<<1);
      if (prach_fmt == 0) {
	idft(IDFT_73728,prachF,prach2,1);
	dftlen=73728;
	// here we have |empty | Prach73728|
	memmove(prach,prach+(73728<<1),(Ncp<<4));
	// here we have |Prefix | Prach73728|
	prach_len = (73728*1)+Ncp;
      }
      else if (prach_fmt == 1) {
	idft(IDFT_73728,prachF,prach2,1);
	dftlen=73728;
	memmove(prach2+(73728<<1),prach2,(73728<<2));
	// here we have |empty | Prach73728 | Prach73728|
	memmove(prach,prach+(73728<<2),(Ncp<<4));
	// here we have |Prefix | Prach73728 | Prach73728|
	prach_len = (73728*2)+Ncp;
      }
      if (prach_fmt == 2) {
	idft(IDFT_73728,prachF,prach2,1);
	dftlen=73728;
	memmove(prach2+(73728<<1),prach2,(73728<<2));
	// here we have |empty | Prach73728 | Prach73728| empty73728 | empty73728
	memmove(prach2+(73728<<2),prach2,(73728<<3));
	// here we have |empty | Prach73728 | Prach73728| Prach73728 | Prach73728
	memmove(prach,prach+(73728<<3),(Ncp<<4));
	// here we have |Prefix | Prach73728 | Prach73728| Prach73728 | Prach73728
	prach_len = (73728*4)+Ncp;
      }
      else if (prach_fmt == 3) {
	idft(IDFT_18432,prachF,prach2,1);
	dftlen=18432;
	memmove(prach2+(18432<<1),prach2,(18432<<2));
	// here we have |empty | Prach18432 | Prach18432| empty18432 | empty18432
	memmove(prach2+(18432<<2),prach2,(18432<<3));
	// here we have |empty | Prach18432 | Prach18432| Prach18432 | Prach18432
	memmove(prach,prach+(18432<<3),(Ncp<<4));
	// here we have |Prefix | Prach18432 | Prach18432| Prach18432 | Prach18432
	prach_len = (18432*4)+Ncp;
      }
      else if (prach_fmt == 0xa1 || prach_fmt == 0xb1 || prach_fmt == 0xc0) {
	idft(IDFT_3072,prachF,prach2,1);
	dftlen=3072;
	// here we have |empty | Prach3072 |
	if (prach_fmt != 0xc0) {
	  memmove(prach2+(3072<<1),prach2,(3072<<2));
	  prach_len = (3072*2)+Ncp;
	} 
	else 	  prach_len = (3072*1)+Ncp;
	memmove(prach,prach+(3072<<1),(Ncp<<2));
	// here we have |Prefix | Prach3072 | Prach3072 (if ! 0xc0)  | 
      }
      else if (prach_fmt == 0xa3 || prach_fmt == 0xb3) { // 6x3072
	idft(IDFT_3072,prachF,prach2,1);
	dftlen=3072;
	// here we have |empty | Prach3072 |
	memmove(prach2+(3072<<1),prach2,(3072<<2));
	// here we have |empty | Prach3072 | Prach3072| empty3072 | empty3072 | empty3072 | empty3072
	memmove(prach2+(3072<<2),prach2,(3072<<3));
	// here we have |empty | Prach3072 | Prach3072| Prach3072 | Prach3072 | empty3072 | empty3072
	memmove(prach2+((3072<<1)*3),prach2,(3072<<3));
	// here we have |empty | Prach3072 | Prach3072| Prach3072 | Prach3072 | Prach3072 | Prach3072
	memmove(prach,prach+(3072<<1),(Ncp<<2));
	// here we have |Prefix | Prach3072 | 
	prach_len = (3072*6)+Ncp;
      }
      else if (prach_fmt == 0xa2 || prach_fmt == 0xb2) { // 4x3072
	idft(IDFT_3072,prachF,prach2,1);
	dftlen=3072;
	// here we have |empty | Prach3072 |
	memmove(prach2+(3072<<1),prach2,(3072<<2));
	// here we have |empty | Prach3072 | Prach3072| empty3072 | empty3072 |
	memmove(prach2+(3072<<2),prach2,(3072<<3));
	// here we have |empty | Prach3072 | Prach3072| Prach3072 | Prach3072 |
	memmove(prach,prach+(3072<<1),(Ncp<<2));
	// here we have |Prefix | Prach3072 | 
	prach_len = (3072*4)+Ncp;
      }
      else if (prach_fmt == 0xb4) { // 12x3072
	idft(IDFT_3072,prachF,prach2,1);
	dftlen=3072;
	// here we have |empty | Prach3072 |
	memmove(prach2+(3072<<1),prach2,(3072<<2));
	// here we have |empty | Prach3072 | Prach3072| empty3072 | empty3072 | empty3072 | empty3072
	memmove(prach2+(3072<<2),prach2,(3072<<3));
	// here we have |empty | Prach3072 | Prach3072| Prach3072 | Prach3072 | empty3072 | empty3072
	memmove(prach2+(3072<<3),prach2,(3072<<3));
	// here we have |empty | Prach3072 | Prach3072| Prach3072 | Prach3072 | Prach3072 | Prach3072
	memmove(prach2+(3072<<1)*6,prach2,(3072<<2)*6);
	// here we have |empty | Prach3072 | Prach3072| Prach3072 | Prach3072 | Prach3072 | Prach3072 | Prach3072 | Prach3072| Prach3072 | Prach3072 | Prach3072 | Prach3072|
	memmove(prach,prach+(3072<<1),(Ncp<<2));
	// here we have |Prefix | Prach3072 | Prach3072| Prach3072 | Prach3072 | Prach3072 | Prach3072 | Prach3072 | Prach3072| Prach3072 | Prach3072 | Prach3072 | Prach3072|
	prach_len = (3072*12)+Ncp;
      }
    }
  }    




#if defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR)
  int j;
  int overflow = prach_start + prach_len - LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_subframe;
  LOG_D( PHY, "prach_start=%d, overflow=%d\n", prach_start, overflow );

  for (i=prach_start,j=0; i<min(fp->samples_per_subframe*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME,prach_start+prach_len); i++,j++) {
    ((int16_t*)ue->common_vars.txdata[0])[2*i] = prach[2*j];
    ((int16_t*)ue->common_vars.txdata[0])[2*i+1] = prach[2*j+1];
  }

  for (i=0; i<overflow; i++,j++) {
    ((int16_t*)ue->common_vars.txdata[0])[2*i] = prach[2*j];
    ((int16_t*)ue->common_vars.txdata[0])[2*i+1] = prach[2*j+1];
  }
#else

  LOG_D( PHY, "prach_start=%d\n", prach_start);

  for (i=0; i<prach_len; i++) {
    ((int16_t*)(&ue->common_vars.txdata[0][prach_start]))[2*i] = prach[2*i];
    ((int16_t*)(&ue->common_vars.txdata[0][prach_start]))[2*i+1] = prach[2*i+1];
  }
#endif



#if defined(PRACH_WRITE_OUTPUT_DEBUG)
  LOG_M("prach_txF0.m","prachtxF0",prachF,prach_len-Ncp,1,1);
  LOG_M("prach_tx0.m","prachtx0",prach+(Ncp<<1),prach_len-Ncp,1,1);
  LOG_M("txsig.m","txs",(int16_t*)(&ue->common_vars.txdata[0][prach_start]),fp->samples_per_subframe,1,1);
  exit(-1);
#endif

  return signal_energy( (int*)prach, 256 );
}

