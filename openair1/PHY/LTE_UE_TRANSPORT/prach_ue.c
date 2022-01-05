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

/*! \file PHY/LTE_TRANSPORT/prach_ue.c
 * \brief Top-level routines for decoding the PRACH physical channel V8.6 2009-03
 * \author R. Knopp
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */
#include "PHY/sse_intrin.h"
#include "PHY/defs_UE.h"
#include "targets/RT/USER/lte-softmodem.h"
#include "PHY/phy_extern_ue.h"
//#include "prach.h"
#include "PHY/LTE_TRANSPORT/if4_tools.h"

#include "SCHED_UE/sched_UE.h"
#include "SCHED/sched_common_extern.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "../LTE_TRANSPORT/prach_extern.h"
#include "common/utils/lte/prach_utils.h"

//#define PRACH_DEBUG 1

int32_t generate_prach( PHY_VARS_UE *ue, uint8_t eNB_id, uint8_t subframe, uint16_t Nf ) {
  lte_frame_type_t frame_type         = ue->frame_parms.frame_type;
  //uint8_t tdd_config         = ue->frame_parms.tdd_config;
  uint16_t rootSequenceIndex = ue->frame_parms.prach_config_common.rootSequenceIndex;
  uint8_t prach_ConfigIndex  = ue->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
  uint8_t Ncs_config         = ue->frame_parms.prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig;
  uint8_t restricted_set     = ue->frame_parms.prach_config_common.prach_ConfigInfo.highSpeedFlag;
  //uint8_t n_ra_prboffset     = ue->frame_parms.prach_config_common.prach_ConfigInfo.prach_FreqOffset;
  uint8_t preamble_index     = ue->prach_resources[eNB_id]->ra_PreambleIndex;
  uint8_t tdd_mapindex       = ue->prach_resources[eNB_id]->ra_TDD_map_index;
  int16_t *prachF           = ue->prach_vars[eNB_id]->prachF;
  static int16_t prach_tmp[45600*4] __attribute__((aligned(32)));
  int16_t *prach            = prach_tmp;
  int16_t *prach2;
  int16_t amp               = ue->prach_vars[eNB_id]->amp;
  int16_t Ncp;
  uint8_t n_ra_prb;
  uint16_t NCS;
  uint16_t *prach_root_sequence_map;
  uint16_t preamble_offset,preamble_shift;
  uint16_t preamble_index0,n_shift_ra,n_shift_ra_bar;
  uint16_t d_start=-1,numshift;
  uint8_t prach_fmt = get_prach_fmt(prach_ConfigIndex,frame_type);
  //uint8_t Nsp=2;
  //uint8_t f_ra,t1_ra;
  uint16_t N_ZC = (prach_fmt<4)?839:139;
  uint8_t not_found;
  int k;
  int16_t *Xu;
  uint16_t u;
  int32_t Xu_re,Xu_im;
  uint16_t offset,offset2;
  int prach_start;
  int i, prach_len;
  uint16_t first_nonzero_root_idx=0;

  if ( !(IS_SOFTMODEM_BASICSIM ) ) {
    prach_start =  (ue->rx_offset+subframe*ue->frame_parms.samples_per_tti-ue->hw_timing_advance-ue->N_TA_offset);
#ifdef PRACH_DEBUG
    LOG_I(PHY,"[UE %d] prach_start %d, rx_offset %d, hw_timing_advance %d, N_TA_offset %d\n", ue->Mod_id,
          prach_start,
          ue->rx_offset,
          ue->hw_timing_advance,
          ue->N_TA_offset);
#endif

    if (prach_start<0)
      prach_start+=(ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);

    if (prach_start>=(ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME))
      prach_start-=(ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);
  } else { //normal case (simulation)
    prach_start = subframe*ue->frame_parms.samples_per_tti-ue->N_TA_offset;
    LOG_I(PHY,"[UE %d] prach_start %d, rx_offset %d, hw_timing_advance %d, N_TA_offset %d\n", ue->Mod_id,
          prach_start,
          ue->rx_offset,
          ue->hw_timing_advance,
          ue->N_TA_offset);
  }

  // First compute physical root sequence
  if (restricted_set == 0) {
    AssertFatal(Ncs_config <= 15,
                "[PHY] FATAL, Illegal Ncs_config for unrestricted format %"PRIu8"\n", Ncs_config );
    NCS = NCS_unrestricted[Ncs_config];
  } else {
    AssertFatal(Ncs_config <= 14,
                "[PHY] FATAL, Illegal Ncs_config for restricted format %"PRIu8"\n", Ncs_config );
    NCS = NCS_restricted[Ncs_config];
  }

  n_ra_prb = get_prach_prb_offset(ue->frame_parms.frame_type,
				  ue->frame_parms.tdd_config,
				  ue->frame_parms.N_RB_UL,
                                  ue->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex,
                                  ue->frame_parms.prach_config_common.prach_ConfigInfo.prach_FreqOffset,
                                  tdd_mapindex, Nf);
  prach_root_sequence_map = (prach_fmt<4) ? prach_root_sequence_map0_3 : prach_root_sequence_map4;
  /*
  // this code is not part of get_prach_prb_offset
  if (frame_type == TDD) { // TDD

    if (tdd_preamble_map[prach_ConfigIndex][tdd_config].num_prach==0) {
      LOG_E( PHY, "[PHY][UE %"PRIu8"] Illegal prach_ConfigIndex %"PRIu8" for ", ue->Mod_id, prach_ConfigIndex );
    }

    // adjust n_ra_prboffset for frequency multiplexing (p.36 36.211)
    f_ra = tdd_preamble_map[prach_ConfigIndex][tdd_config].map[tdd_mapindex].f_ra;

    if (prach_fmt < 4) {
      if ((f_ra&1) == 0) {
        n_ra_prb = n_ra_prboffset + 6*(f_ra>>1);
      } else {
        n_ra_prb = ue->frame_parms.N_RB_UL - 6 - n_ra_prboffset + 6*(f_ra>>1);
      }
    } else {
      if ((tdd_config >2) && (tdd_config<6))
        Nsp = 2;

      t1_ra = tdd_preamble_map[prach_ConfigIndex][tdd_config].map[0].t1_ra;

      if ((((Nf&1)*(2-Nsp)+t1_ra)&1) == 0) {
        n_ra_prb = 6*f_ra;
      } else {
        n_ra_prb = ue->frame_parms.N_RB_UL - 6*(f_ra+1);
      }
    }
  }
  */
  // This is the relative offset (for unrestricted case) in the root sequence table (5.7.2-4 from 36.211) for the given preamble index
  preamble_offset = ((NCS==0)? preamble_index : (preamble_index/(N_ZC/NCS)));

  if (restricted_set == 0) {
    // This is the \nu corresponding to the preamble index
    preamble_shift  = (NCS==0)? 0 : (preamble_index % (N_ZC/NCS));
    preamble_shift *= NCS;
  } else { // This is the high-speed case
#ifdef PRACH_DEBUG
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
        DevAssert( index < sizeof(prach_root_sequence_map0_3) / sizeof(prach_root_sequence_map0_3[0]) );
      } else {
        // prach_root_sequence_map points to prach_root_sequence_map4
        DevAssert( index < sizeof(prach_root_sequence_map4) / sizeof(prach_root_sequence_map4[0]) );
      }

      u = prach_root_sequence_map[index];
      uint16_t n_group_ra = 0;

      if ( (du[u]<(N_ZC/3)) && (du[u]>=NCS) ) {
        n_shift_ra     = du[u]/NCS;
        d_start        = (du[u]<<1) + (n_shift_ra * NCS);
        n_group_ra     = N_ZC/d_start;
        n_shift_ra_bar = max(0,(N_ZC-(du[u]<<1)-(n_group_ra*d_start))/N_ZC);
      } else if  ( (du[u]>=(N_ZC/3)) && (du[u]<=((N_ZC - NCS)>>1)) ) {
        n_shift_ra     = (N_ZC - (du[u]<<1))/NCS;
        d_start        = N_ZC - (du[u]<<1) + (n_shift_ra * NCS);
        n_group_ra     = du[u]/d_start;
        n_shift_ra_bar = min(n_shift_ra,max(0,(du[u]- (n_group_ra*d_start))/NCS));
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
#ifdef PRACH_DEBUG

  if (NCS>0)
    LOG_I(PHY,"Generate PRACH for RootSeqIndex %d, Preamble Index %d, NCS %d (NCS_config %d, N_ZC/NCS %d) n_ra_prb %d: Preamble_offset %d, Preamble_shift %d\n",
          rootSequenceIndex,preamble_index,NCS,Ncs_config,N_ZC/NCS,n_ra_prb,
          preamble_offset,preamble_shift);

#endif
  //  nsymb = (frame_parms->Ncp==0) ? 14:12;
  //  subframe_offset = (unsigned int)frame_parms->ofdm_symbol_size*subframe*nsymb;
  k = (12*n_ra_prb) - 6*ue->frame_parms.N_RB_UL;

  if (k<0)
    k+=ue->frame_parms.ofdm_symbol_size;

  k*=12;
  k+=13;
  Xu = (int16_t *)ue->X_u[preamble_offset-first_nonzero_root_idx];
  /*
    k+=(12*ue->frame_parms.first_carrier_offset);
    if (k>(12*ue->frame_parms.ofdm_symbol_size))
    k-=(12*ue->frame_parms.ofdm_symbol_size);
  */
  k*=2;

  switch (ue->frame_parms.N_RB_UL) {
    case 6:
      memset((void *)prachF,0,4*1536);
      break;

    case 15:
      memset((void *)prachF,0,4*3072);
      break;

    case 25:
      memset((void *)prachF,0,4*6144);
      break;

    case 50:
      memset((void *)prachF,0,4*12288);
      break;

    case 75:
      memset((void *)prachF,0,4*18432);
      break;

    case 100:
      if (ue->frame_parms.threequarter_fs == 0)
        memset((void *)prachF,0,4*24576);
      else
        memset((void *)prachF,0,4*18432);

      break;
  }

  for (offset=0,offset2=0; offset<N_ZC; offset++,offset2+=preamble_shift) {
    if (offset2 >= N_ZC)
      offset2 -= N_ZC;

    Xu_re = (((int32_t)Xu[offset<<1]*amp)>>15);
    Xu_im = (((int32_t)Xu[1+(offset<<1)]*amp)>>15);
    prachF[k++]= ((Xu_re*ru[offset2<<1]) - (Xu_im*ru[1+(offset2<<1)]))>>15;
    prachF[k++]= ((Xu_im*ru[offset2<<1]) + (Xu_re*ru[1+(offset2<<1)]))>>15;

    if (k==(12*2*ue->frame_parms.ofdm_symbol_size))
      k=0;
  }

  switch (prach_fmt) {
    case 0:
      Ncp = 3168;
      break;

    case 1:
    case 3:
      Ncp = 21024;
      break;

    case 2:
      Ncp = 6240;
      break;

    case 4:
      Ncp = 448;
      break;

    default:
      Ncp = 3168;
      break;
  }

  switch (ue->frame_parms.N_RB_UL) {
    case 6:
      Ncp>>=4;
      prach+=4; // makes prach2 aligned to 128-bit
      break;

    case 15:
      Ncp>>=3;
      break;

    case 25:
      Ncp>>=2;
      break;

    case 50:
      Ncp>>=1;
      break;

    case 75:
      Ncp=(Ncp*3)>>2;
      break;
  }

  if (ue->frame_parms.threequarter_fs == 1)
    Ncp=(Ncp*3)>>2;

  prach2 = prach+(Ncp<<1);

  // do IDFT
  switch (ue->frame_parms.N_RB_UL) {
    case 6:
      if (prach_fmt == 4) {
        idft(IDFT_256,prachF,prach2,1);
        memmove( prach, prach+512, Ncp<<2 );
        prach_len = 256+Ncp;
      } else {
        idft(IDFT_1536,prachF,prach2,1);
        memmove( prach, prach+3072, Ncp<<2 );
        prach_len = 1536+Ncp;

        if (prach_fmt>1) {
          memmove( prach2+3072, prach2, 6144 );
          prach_len = 2*1536+Ncp;
        }
      }

      break;

    case 15:
      if (prach_fmt == 4) {
        idft(IDFT_512,prachF,prach2,1);
        //TODO: account for repeated format in dft output
        memmove( prach, prach+1024, Ncp<<2 );
        prach_len = 512+Ncp;
      } else {
        idft(IDFT_3072,prachF,prach2,1);
        memmove( prach, prach+6144, Ncp<<2 );
        prach_len = 3072+Ncp;

        if (prach_fmt>1) {
          memmove( prach2+6144, prach2, 12288 );
          prach_len = 2*3072+Ncp;
        }
      }

      break;

    case 25:
    default:
      if (prach_fmt == 4) {
        idft(IDFT_1024,prachF,prach2,1);
        memmove( prach, prach+2048, Ncp<<2 );
        prach_len = 1024+Ncp;
      } else {
        idft(IDFT_6144,prachF,prach2,1);
        /*for (i=0;i<6144*2;i++)
        prach2[i]<<=1;*/
        memmove( prach, prach+12288, Ncp<<2 );
        prach_len = 6144+Ncp;

        if (prach_fmt>1) {
          memmove( prach2+12288, prach2, 24576 );
          prach_len = 2*6144+Ncp;
        }
      }

      break;

    case 50:
      if (prach_fmt == 4) {
        idft(IDFT_2048,prachF,prach2,1);
        memmove( prach, prach+4096, Ncp<<2 );
        prach_len = 2048+Ncp;
      } else {
        idft(IDFT_12288,prachF,prach2,1);
        memmove( prach, prach+24576, Ncp<<2 );
        prach_len = 12288+Ncp;

        if (prach_fmt>1) {
          memmove( prach2+24576, prach2, 49152 );
          prach_len = 2*12288+Ncp;
        }
      }

      break;

    case 75:
      if (prach_fmt == 4) {
        idft(IDFT_3072,prachF,prach2,1);
        //TODO: account for repeated format in dft output
        memmove( prach, prach+6144, Ncp<<2 );
        prach_len = 3072+Ncp;
      } else {
        idft(IDFT_18432,prachF,prach2,1);
        memmove( prach, prach+36864, Ncp<<2 );
        prach_len = 18432+Ncp;

        if (prach_fmt>1) {
          memmove( prach2+36834, prach2, 73728 );
          prach_len = 2*18432+Ncp;
        }
      }

      break;

    case 100:
      if (ue->frame_parms.threequarter_fs == 0) {
        if (prach_fmt == 4) {
          idft(IDFT_4096,prachF,prach2,1);
          memmove( prach, prach+8192, Ncp<<2 );
          prach_len = 4096+Ncp;
        } else {
          idft(IDFT_24576,prachF,prach2,1);
          memmove( prach, prach+49152, Ncp<<2 );
          prach_len = 24576+Ncp;

          if (prach_fmt>1) {
            memmove( prach2+49152, prach2, 98304 );
            prach_len = 2* 24576+Ncp;
          }
        }
      } else {
        if (prach_fmt == 4) {
          idft(IDFT_3072,prachF,prach2,1);
          //TODO: account for repeated format in dft output
          memmove( prach, prach+6144, Ncp<<2 );
          prach_len = 3072+Ncp;
        } else {
          idft(IDFT_18432,prachF,prach2,1);
          memmove( prach, prach+36864, Ncp<<2 );
          prach_len = 18432+Ncp;
          printf("Generated prach for 100 PRB, 3/4 sampling\n");

          if (prach_fmt>1) {
            memmove( prach2+36834, prach2, 73728 );
            prach_len = 2*18432+Ncp;
          }
        }
      }

      break;
  }

  //LOG_I(PHY,"prach_len=%d\n",prach_len);
  AssertFatal(prach_fmt<4,
              "prach_fmt4 not fully implemented" );

  if (!(IS_SOFTMODEM_BASICSIM ) ) {
    int j;
    int overflow = prach_start + prach_len - LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*ue->frame_parms.samples_per_tti;
    LOG_I( PHY, "prach_start=%d, overflow=%d\n", prach_start, overflow );

    for (i=prach_start,j=0; i<min(ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME,prach_start+prach_len); i++,j++) {
      ((int16_t *)ue->common_vars.txdata[0])[2*i] = prach[2*j];
      ((int16_t *)ue->common_vars.txdata[0])[2*i+1] = prach[2*j+1];
    }

    for (i=0; i<overflow; i++,j++) {
      ((int16_t *)ue->common_vars.txdata[0])[2*i] = prach[2*j];
      ((int16_t *)ue->common_vars.txdata[0])[2*i+1] = prach[2*j+1];
    }

#if defined(EXMIMO)

    // handle switch before 1st TX subframe, guarantee that the slot prior to transmission is switch on
    for (k=prach_start - (ue->frame_parms.samples_per_tti>>1) ; k<prach_start ; k++) {
      if (k<0)
        ue->common_vars.txdata[0][k+ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME] &= 0xFFFEFFFE;
      else if (k>(ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME))
        ue->common_vars.txdata[0][k-ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME] &= 0xFFFEFFFE;
      else
        ue->common_vars.txdata[0][k] &= 0xFFFEFFFE;
    }

#endif
  } else { // simulators
    for (i=0; i<prach_len; i++) {
      ((int16_t *)(&ue->common_vars.txdata[0][prach_start]))[2*i] = prach[2*i];
      ((int16_t *)(&ue->common_vars.txdata[0][prach_start]))[2*i+1] = prach[2*i+1];
    }
  }

#if defined(PRACH_WRITE_OUTPUT_DEBUG)
  LOG_M("prach_txF0.m","prachtxF0",prachF,prach_len-Ncp,1,1);
  LOG_M("prach_tx0.m","prachtx0",prach+(Ncp<<1),prach_len-Ncp,1,1);
  LOG_M("txsig.m","txs",(int16_t *)(&ue->common_vars.txdata[0][0]),2*ue->frame_parms.samples_per_tti,1,1);
  exit(-1);
#endif
  return signal_energy( (int *)prach, 256 );
}

