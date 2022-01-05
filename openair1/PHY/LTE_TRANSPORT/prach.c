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

/*! \file PHY/LTE_TRANSPORT/prach.c
 * \brief Top-level routines for generating and decoding the PRACH physical channel V8.6 2009-03
 * \author R. Knopp
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */
#include "PHY/sse_intrin.h"
#include "PHY/defs_eNB.h"
//#include "PHY/phy_extern.h"
//#include "prach.h"
#include "PHY/LTE_TRANSPORT/if4_tools.h"

#include "SCHED/sched_eNB.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "prach_extern.h"
#include <openair1/PHY/LTE_TRANSPORT/transport_proto.h>
#include <executables/split_headers.h>
#include "common/utils/lte/prach_utils.h"

void rx_prach0(PHY_VARS_eNB *eNB,
               RU_t *ru,
	       int frame_prach,
	       int subframe,
               uint16_t *max_preamble,
               uint16_t *max_preamble_energy,
               uint16_t *max_preamble_delay,
               uint16_t *avg_preamble_energy,
               uint16_t Nf,
               uint8_t tdd_mapindex,
               uint8_t br_flag,
               uint8_t ce_level
              ) {
  int i;
  lte_frame_type_t   frame_type;
  uint16_t           rootSequenceIndex;
  uint8_t            prach_ConfigIndex;
  uint8_t            Ncs_config;
  uint8_t            restricted_set;
  uint8_t            n_ra_prb;
  int16_t            *prachF=NULL;
  int16_t            **rxsigF=NULL;
  int16_t *prach2;
  uint8_t preamble_index;
  uint16_t NCS,NCS2;
  uint16_t preamble_offset=0,preamble_offset_old;
  int16_t preamble_shift=0;
  uint32_t preamble_shift2;
  uint16_t preamble_index0=0,n_shift_ra=0,n_shift_ra_bar;
  uint16_t d_start=0;
  uint16_t numshift=0;
  uint16_t *prach_root_sequence_map;
  uint8_t not_found;
  int k=0;
  uint16_t u;
  int16_t *Xu=0;
  uint16_t offset;
  int16_t Ncp;
  uint16_t first_nonzero_root_idx=0;
  uint8_t new_dft=0;
  uint8_t aa;
  int32_t lev;
  int16_t levdB;
  int fft_size=0,log2_ifft_size;
  int16_t prach_ifft_tmp[2048*2] __attribute__((aligned(32)));
  int32_t *prach_ifft=(int32_t *)NULL;
  int32_t **prach_ifftp=(int32_t **)NULL;
  int prach_ifft_cnt=0;
  int exit_flag=0;
  LTE_DL_FRAME_PARMS *fp;
  int nb_rx;
  if(eNB)  {
    fp    = &(eNB->frame_parms);
    nb_rx = fp->nb_antennas_rx;
  } else {
    fp    = (ru->frame_parms);
    nb_rx = ru->nb_rx;
  }
  
  AssertFatal(fp!=NULL,"rx_prach called without valid RU or eNB descriptor\n");
  frame_type          = fp->frame_type;

  if (br_flag == 1) {
    AssertFatal(fp->prach_emtc_config_common.prach_Config_enabled==1,
                "emtc prach_Config is not enabled\n");
    AssertFatal(fp->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[ce_level]==1,
                "ce_level %d is not active\n",ce_level);
    rootSequenceIndex   = fp->prach_emtc_config_common.rootSequenceIndex;
    prach_ConfigIndex   = fp->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[ce_level];
    Ncs_config          = fp->prach_emtc_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig;
    restricted_set      = fp->prach_emtc_config_common.prach_ConfigInfo.highSpeedFlag;
    n_ra_prb            = get_prach_prb_offset(fp->frame_type,fp->tdd_config,fp->N_RB_UL,fp->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[ce_level],
                          fp->prach_emtc_config_common.prach_ConfigInfo.prach_FreqOffset[ce_level],
                          tdd_mapindex,Nf);
    // update pointers to results for ce_level
    max_preamble        += ce_level;
    max_preamble_energy += ce_level;
    max_preamble_delay  += ce_level;
  } else {
    rootSequenceIndex   = fp->prach_config_common.rootSequenceIndex;
    prach_ConfigIndex   = fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
    Ncs_config          = fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig;
    restricted_set      = fp->prach_config_common.prach_ConfigInfo.highSpeedFlag;
    n_ra_prb            = get_prach_prb_offset(fp->frame_type,fp->tdd_config,fp->N_RB_UL,fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex,
                          fp->prach_config_common.prach_ConfigInfo.prach_FreqOffset,
                          tdd_mapindex,Nf);
  }

  int16_t *prach[nb_rx];
  uint8_t prach_fmt = get_prach_fmt(prach_ConfigIndex,frame_type);
  uint16_t N_ZC = (prach_fmt <4)?839:139;

  if (eNB) {
    if (br_flag == 1) {
      prach_ifftp         = eNB->prach_vars_br.prach_ifft[ce_level];
      prachF              = eNB->prach_vars_br.prachF;
      rxsigF              = eNB->prach_vars_br.rxsigF[ce_level];

      if (LOG_DEBUGFLAG(PRACH)) {
        if (((frame_prach)&1023) < 20) LOG_D(PHY,
              "PRACH (eNB) : running rx_prach (br_flag %d, ce_level %d) for frame %d subframe %d, prach_FreqOffset %d, prach_ConfigIndex %d, rootSequenceIndex %d, repetition number %d,numRepetitionsPrePreambleAttempt %d\n",
              br_flag,ce_level,frame_prach,subframe,
              fp->prach_emtc_config_common.prach_ConfigInfo.prach_FreqOffset[ce_level],
              prach_ConfigIndex,rootSequenceIndex,
              eNB->prach_vars_br.repetition_number[ce_level],
              fp->prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[ce_level]);
      }
    } else {
      prach_ifftp       = eNB->prach_vars.prach_ifft[0];
      prachF            = eNB->prach_vars.prachF;
      rxsigF            = eNB->prach_vars.rxsigF[0];

      //if (LOG_DEBUGFLAG(PRACH)) {
        if (((frame_prach)&1023) < 20) LOG_D(PHY,"PRACH (eNB) : running rx_prach for subframe %d, prach_FreqOffset %d, prach_ConfigIndex %d , rootSequenceIndex %d\n", subframe,
              fp->prach_config_common.prach_ConfigInfo.prach_FreqOffset,prach_ConfigIndex,rootSequenceIndex);
      //}
    }
  } else {
    if (br_flag == 1) {
      rxsigF            = ru->prach_rxsigF_br[ce_level];

      if (LOG_DEBUGFLAG(PRACH)) {
        if (((frame_prach)&1023) < 20) LOG_D(PHY,"PRACH (RU) : running rx_prach (br_flag %d, ce_level %d) for frame %d subframe %d, prach_FreqOffset %d, prach_ConfigIndex %d\n",
              br_flag,ce_level,frame_prach,subframe,fp->prach_emtc_config_common.prach_ConfigInfo.prach_FreqOffset[ce_level],prach_ConfigIndex);
      }
    } else {
      rxsigF            = ru->prach_rxsigF[0];

      if (LOG_DEBUGFLAG(PRACH)) {
        if (((frame_prach)&1023) < 20) LOG_D(PHY,"PRACH (RU) : running rx_prach for subframe %d, prach_FreqOffset %d, prach_ConfigIndex %d\n",
              subframe,fp->prach_config_common.prach_ConfigInfo.prach_FreqOffset,prach_ConfigIndex);
      }
    }
  }

  AssertFatal(ru!=NULL,"ru is null\n");
  int8_t dBEn0=0;
  for (aa=0; aa<nb_rx; aa++) {
    if (ru->if_south == LOCAL_RF || ru->function == NGFI_RAU_IF5) { // set the time-domain signal if we have to use it in this node
      // DJP - indexing below in subframe zero takes us off the beginning of the array???
      prach[aa] = (int16_t *)&ru->common.rxdata[aa][(subframe*fp->samples_per_tti)-ru->N_TA_offset];

      if (LOG_DUMPFLAG(PRACH)) {
        int32_t en0=signal_energy((int32_t *)prach[aa],fp->samples_per_tti);
        dBEn0 = dB_fixed(en0);
        int8_t rach_dBm = dBEn0 - ru->rx_total_gain_dB;
        char buffer[80];

        if (dBEn0>30 && prach[0]!= NULL) {
          static int counter=0;
          sprintf(buffer, "%s%d", "/tmp/prach_rx.m",counter);
          LOG_M(buffer,"prach_rx",prach[0],fp->samples_per_tti,1,1);
          exit_flag=1;
        }

        if (dBEn0>30) {
          sprintf(buffer, "rach_dBm:%d",rach_dBm);

          if (prach[0]!= NULL) LOG_M("prach_rx.m","prach_rx",prach[0],fp->samples_per_tti,1,1);

          LOG_I(PHY,"RU %d, br_flag %d ce_level %d frame %d subframe %d per_tti:%d prach:%p (energy %d) TA:%d %s rxdata:%p index:%d\n",
                ru->idx,br_flag,ce_level,frame_prach,subframe,fp->samples_per_tti,
                prach[aa],dBEn0,ru->N_TA_offset,buffer,ru->common.rxdata[aa],
                (subframe*fp->samples_per_tti)-ru->N_TA_offset);
        }
      }
    }
  }

  // First compute physical root sequence
  if (restricted_set == 0) {
    AssertFatal(Ncs_config<=15,
                "Illegal Ncs_config for unrestricted format %d\n",Ncs_config);
    NCS = NCS_unrestricted[Ncs_config];
  } else {
    AssertFatal(Ncs_config<=14,
                "FATAL, Illegal Ncs_config for restricted format %d\n",Ncs_config);
    NCS = NCS_restricted[Ncs_config];
  }

  if (eNB) start_meas(&eNB->rx_prach);

  prach_root_sequence_map = (prach_fmt < 4) ? prach_root_sequence_map0_3 : prach_root_sequence_map4;
  // PDP is oversampled, e.g. 1024 sample instead of 839
  // Adapt the NCS (zero-correlation zones) with oversampling factor e.g. 1024/839
  NCS2 = (N_ZC==839) ? ((NCS<<10)/839) : ((NCS<<8)/139);

  if (NCS2==0)
    NCS2 = N_ZC;

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

  // Adjust CP length based on UL bandwidth
  switch (fp->N_RB_UL) {
    case 6:
      Ncp>>=4;
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

    case 100:
      if (fp->threequarter_fs == 1)
        Ncp=(Ncp*3)>>2;

      break;
  }

  if (((eNB!=NULL) && (ru->function != NGFI_RAU_IF4p5))||
      ((eNB==NULL) && (ru->function == NGFI_RRU_IF4p5))) { // compute the DFTs of the PRACH temporal resources
    // Do forward transform
    if (LOG_DEBUGFLAG(PRACH)) {
      LOG_D(PHY,"rx_prach: Doing FFT for N_RB_UL %d nb_rx:%d Ncp:%d\n",fp->N_RB_UL, nb_rx, Ncp);
    }

    for (aa=0; aa<nb_rx; aa++) {
      AssertFatal(prach[aa]!=NULL,"prach[%d] is null\n",aa);
      prach2 = prach[aa] + (Ncp<<1);

      // do DFT
      switch (fp->N_RB_UL) {
        case 6:
          if (prach_fmt == 4) {
            dft(DFT_256,prach2,rxsigF[aa],1);
            fft_size=256;
          } else {
            dft(DFT_1536,prach2,rxsigF[aa],1);

            if (prach_fmt>1)
              dft(DFT_1536,prach2+3072,rxsigF[aa]+3072,1);
            fft_size=1536;
          }

          break;

        case 15:
          if (prach_fmt == 4) {
            dft(DFT_256,prach2,rxsigF[aa],1);
            fft_size=256;
          } else {
            dft(DFT_3072,prach2,rxsigF[aa],1);

            if (prach_fmt>1)
              dft(DFT_3072,prach2+6144,rxsigF[aa]+6144,1);
          }
          fft_size=3072;
          break;

        case 25:
        default:
          if (prach_fmt == 4) {
            dft(DFT_1024,prach2,rxsigF[aa],1);
            fft_size = 1024;
          } else {
            dft(DFT_6144,prach2,rxsigF[aa],1);

            if (prach_fmt>1)
              dft(DFT_6144,prach2+12288,rxsigF[aa]+12288,1);

            fft_size = 6144;
          }

          break;

        case 50:
          if (prach_fmt == 4) {
            dft(DFT_2048,prach2,rxsigF[aa],1);
            fft_size=2048;
          } else {
            dft(DFT_12288,prach2,rxsigF[aa],1);

            if (prach_fmt>1)
              dft(DFT_12288,prach2+24576,rxsigF[aa]+24576,1);
            fft_size=12288;
          }

          break;

        case 75:
          if (prach_fmt == 4) {
            dft(DFT_3072,prach2,rxsigF[aa],1);
            fft_size=3072;
          } else {
            dft(DFT_18432,prach2,rxsigF[aa],1);

            if (prach_fmt>1)
              dft(DFT_18432,prach2+36864,rxsigF[aa]+36864,1);
            fft_size=18432;
          }

          break;

        case 100:
          if (fp->threequarter_fs==0) {
            if (prach_fmt == 4) {
              dft(DFT_4096,prach2,rxsigF[aa],1);
              fft_size=2048;
            } else {
              dft(DFT_24576,prach2,rxsigF[aa],1);

              if (prach_fmt>1)
                dft(DFT_24576,prach2+49152,rxsigF[aa]+49152,1);
              fft_size=24576;
            }
          } else {
            if (prach_fmt == 4) {
              dft(DFT_3072,prach2,rxsigF[aa],1);
              fft_size=3072;
            } else {
              dft(DFT_18432,prach2,rxsigF[aa],1);

              if (prach_fmt>1)
                dft(DFT_18432,prach2+36864,rxsigF[aa]+36864,1);
              fft_size=18432;
            }
          }

          break;
      }

      k = (12*n_ra_prb) - 6*fp->N_RB_UL;

      if (k<0) {
        k+=(fp->ofdm_symbol_size);
      }

      k*=12;
      k+=13;
      k*=2;
      int dftsize_x2 = fp->ofdm_symbol_size*24;
      //LOG_D(PHY,"Shifting prach_rxF from %d to 0\n",k);

      if ((k+(839*2)) > dftsize_x2) { // PRACH signal is split around DC
        memmove((void *)&rxsigF[aa][dftsize_x2-k],(void *)&rxsigF[aa][0],(k+(839*2)-dftsize_x2)*2);
        memmove((void *)&rxsigF[aa][0],(void *)(&rxsigF[aa][k]),(dftsize_x2-k)*2);
      } else // PRACH signal is not split around DC
        memmove((void *)&rxsigF[aa][0],(void *)(&rxsigF[aa][k]),839*4);
    }
  }

  if ((eNB==NULL)  && ru->function == NGFI_RRU_IF4p5) {
    /// **** send_IF4 of rxsigF to RAU **** ///
    if (br_flag == 1)
      send_IF4p5(ru, frame_prach, subframe, IF4p5_PRACH+1+ce_level);
    else
      send_IF4p5(ru, frame_prach, subframe, IF4p5_PRACH);

    return;
  } else if (eNB!=NULL) {
    if ( LOG_DEBUGFLAG(PRACH)) {
      int en = dB_fixed(signal_energy((int32_t *)&rxsigF[0][0],840));

      if ((en > 10)&&(br_flag==1)) LOG_I(PHY,"PRACH (br_flag %d,ce_level %d, n_ra_prb %d, k %d): Frame %d, Subframe %d => %d dB\n",br_flag,ce_level,n_ra_prb,k,frame_prach,subframe,en);
    }
  }

  // in case of RAU and prach received rx_thread wakes up prach
  // here onwards is for eNodeB_3GPP or NGFI_RAU_IF4p5
  preamble_offset_old = 99;
  uint8_t update_TA  = 4;
  uint8_t update_TA2 = 1;

  switch (eNB->frame_parms.N_RB_DL) {
    case 6:
      update_TA = 16;
      break;

    case 25:
      update_TA = 4;
      break;

    case 50:
      update_TA = 2;
      break;

    case 75:
      update_TA  = 3;
      update_TA2 = 2;
      break;

    case 100:
      update_TA  = 1;
      break;
  }

  *max_preamble_energy=0;
  *avg_preamble_energy=0;
  uint64_t avg_en=0;

  for (preamble_index=0 ; preamble_index<64 ; preamble_index++) {
    if (LOG_DEBUGFLAG(PRACH)) {
    //  int en = dB_fixed(signal_energy((int32_t *)&rxsigF[0][0],840));

      if (dBEn0>30) LOG_I(PHY,"frame %d, subframe %d : Trying preamble %d (br_flag %d)\n",frame_prach,subframe,preamble_index,br_flag);
    }

    if (restricted_set == 0) {
      // This is the relative offset in the root sequence table (5.7.2-4 from 36.211) for the given preamble index
      preamble_offset = ((NCS==0)? preamble_index : (preamble_index/(N_ZC/NCS)));

      if (preamble_offset != preamble_offset_old) {
        preamble_offset_old = preamble_offset;
        new_dft = 1;
        // This is the \nu corresponding to the preamble index
        preamble_shift  = 0;
      } else {
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
      //en = dB_fixed(signal_energy((int32_t *)&rxsigF[0][0],840));

      if (dBEn0>30) LOG_I(PHY,"frame %d, subframe %d : preamble index %d: offset %d, preamble shift %d (br_flag %d, en %d)\n",
                         frame_prach,subframe,preamble_index,preamble_offset,preamble_shift,br_flag,dBEn0);
    }

    log2_ifft_size = 10;

    if (new_dft == 1) {
      new_dft = 0;

      if (br_flag == 1) {
        Xu=(int16_t *)eNB->X_u_br[ce_level][preamble_offset-first_nonzero_root_idx];
        prach_ifft = prach_ifftp[prach_ifft_cnt++];

        if (eNB->prach_vars_br.repetition_number[ce_level]==1) memset(prach_ifft,0,((N_ZC==839)?2048:256)*sizeof(int32_t));
      } else {
        Xu=(int16_t *)eNB->X_u[preamble_offset-first_nonzero_root_idx];
        prach_ifft = prach_ifftp[0];
        memset(prach_ifft,0,((N_ZC==839) ? 2048 : 256)*sizeof(int32_t));
      }

      memset(prachF, 0, sizeof(int16_t)*2*1024 );

    if (LOG_DUMPFLAG(PRACH)) {
        if (prach[0]!= NULL) LOG_M("prach_rx0.m","prach_rx0",prach[0],6144+792,1,1);

        LOG_M("prach_rx1.m","prach_rx1",prach[1],6144+792,1,1);
        LOG_M("prach_rxF0.m","prach_rxF0",rxsigF[0],12288,1,1);
        LOG_M("prach_rxF1.m","prach_rxF1",rxsigF[1],12288,1,1);
    }

      for (aa=0; aa<nb_rx; aa++) {
        // Do componentwise product with Xu* on each antenna
        k=0;

        for (offset=0; offset<(N_ZC<<1); offset+=2) {
          prachF[offset]   = (int16_t)(((int32_t)Xu[offset]*rxsigF[aa][k]   + (int32_t)Xu[offset+1]*rxsigF[aa][k+1])>>15);
          prachF[offset+1] = (int16_t)(((int32_t)Xu[offset]*rxsigF[aa][k+1] - (int32_t)Xu[offset+1]*rxsigF[aa][k])>>15);
          k+=2;

          if (k==(12*2*fp->ofdm_symbol_size))
            k=0;
        }

        // Now do IFFT of size 1024 (N_ZC=839) or 256 (N_ZC=139)
        if (N_ZC == 839) {
          log2_ifft_size = 10;
          idft(IDFT_1024,prachF,prach_ifft_tmp,1);

          // compute energy and accumulate over receive antennas and repetitions for BR
          for (i=0; i<2048; i++)
            prach_ifft[i] += (prach_ifft_tmp[i<<1]*prach_ifft_tmp[i<<1] + prach_ifft_tmp[1+(i<<1)]*prach_ifft_tmp[1+(i<<1)])>>9;
        } else {
          idft(IDFT_256,prachF,prach_ifft_tmp,1);
          log2_ifft_size = 8;

          // compute energy and accumulate over receive antennas and repetitions for BR
          for (i=0; i<256; i++)
            prach_ifft[i] += (prach_ifft_tmp[i<<1]*prach_ifft_tmp[(i<<1)] + prach_ifft_tmp[1+(i<<1)]*prach_ifft_tmp[1+(i<<1)])>>9;
        }

        if (LOG_DUMPFLAG(PRACH)) {
          if (aa==0) LOG_M("prach_rxF_comp0.m","prach_rxF_comp0",prachF,1024,1,1);

          if (aa==1) LOG_M("prach_rxF_comp1.m","prach_rxF_comp1",prachF,1024,1,1);
        }
      }// antennas_rx
    } // new dft

    // check energy in nth time shift, for
    if ((br_flag==0) ||
        (eNB->prach_vars_br.repetition_number[ce_level]==
         eNB->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[ce_level])) {
      if (LOG_DEBUGFLAG(PRACH)) {
        int en = dB_fixed(signal_energy((int32_t *)&rxsigF[0][0],840));

        if (en>60) LOG_I(PHY,"frame %d, subframe %d: Checking for peak in time-domain (br_flag %d, en %d)\n",frame_prach,subframe,br_flag,en);
      }

      preamble_shift2 = ((preamble_shift==0) ? 0 : ((preamble_shift<<log2_ifft_size)/N_ZC));

      for (i=0; i<NCS2; i++) {
        lev = (int32_t)prach_ifft[(preamble_shift2+i)];
        avg_en += lev;
        levdB = dB_fixed_x10(lev);

        if (levdB>*max_preamble_energy) {
          *max_preamble_energy  = levdB;
          *max_preamble_delay   = ((i*fft_size)>>log2_ifft_size)*update_TA/update_TA2;
          *max_preamble         = preamble_index;

          if (LOG_DEBUGFLAG(PRACH)) {
        //    int en = dB_fixed(signal_energy((int32_t *)&rxsigF[0][0],840));

            if (dBEn0>30)
              LOG_D(PHY,"frame %d, subframe %d : max_preamble_energy %d, max_preamble_delay %d, max_preamble %d (br_flag %d,ce_level %d, levdB %d, lev %d)\n",
                    frame_prach,subframe,
                    *max_preamble_energy,*max_preamble_delay,
                    *max_preamble,br_flag,ce_level,levdB,lev);
          }
        }
      } ///ncs2
    }
  }// preamble_index

  *avg_preamble_energy=dB_fixed(avg_en/64);

  if (exit_flag==1) {
    int en = dB_fixed(signal_energy((int32_t *)&rxsigF[0][0],840));

    if (en>30) {
      k = (12*n_ra_prb) - 6*fp->N_RB_UL;

      if (k<0) k+=fp->ofdm_symbol_size;

      k*=12;
      k+=13;
      k*=2;

      if (br_flag == 0) {
        LOG_M("rxsigF.m","prach_rxF",&rxsigF[0][0],12288,1,1);
        LOG_M("prach_rxF_comp0.m","prach_rxF_comp0",prachF,1024,1,1);
        LOG_M("Xu.m","xu",Xu,N_ZC,1,1);
        LOG_M("prach_ifft0.m","prach_t0",prach_ifft,1024,1,1);
        LOG_M("SF2_3.m","sf2_3",&ru->common.rxdata[0][2*fp->samples_per_tti],2*fp->samples_per_tti,1,1);
      } else {
        LOG_E(PHY,"Dumping prach (br_flag %d), k = %d (n_ra_prb %d)\n",br_flag,k,n_ra_prb);
        LOG_M("rxsigF_br.m","prach_rxF_br",&rxsigF[0][0],12288,1,1);
        LOG_M("prach_rxF_comp0_br.m","prach_rxF_comp0_br",prachF,1024,1,1);
        LOG_M("Xu_br.m","xu_br",Xu,N_ZC,1,1);
        LOG_M("prach_ifft0_br.m","prach_t0_br",prach_ifft,1024,1,1);
      }
    }
  } /* LOG_DUMPFLAG(PRACH) */

  if (eNB) stop_meas(&eNB->rx_prach);
  AssertFatal(exit_flag==0,"exiting\n");

}


void rx_prach(PHY_VARS_eNB *eNB,
              RU_t *ru,
              uint16_t *max_preamble,
              uint16_t *max_preamble_energy,
              uint16_t *max_preamble_delay,
              uint16_t *avg_preamble_energy,
              uint16_t Nf,
              uint8_t tdd_mapindex,
              uint8_t br_flag) {
  int i;
  int prach_mask=0;

  int subframe;
  if (eNB)
    subframe= br_flag?eNB->proc.subframe_prach_br:eNB->proc.subframe_prach;
  else 
    subframe= br_flag?ru->proc.subframe_prach_br:ru->proc.subframe_prach;
  
  int frame_prach=eNB?eNB->proc.frame_prach: ru->proc.frame_prach;

  if (br_flag == 0) {
    rx_prach0(eNB,ru,frame_prach,subframe,max_preamble,max_preamble_energy,max_preamble_delay,avg_preamble_energy,Nf,tdd_mapindex,0,0);
  } else { // This is procedure for eMTC, basically handling the repetitions
    prach_mask = is_prach_subframe(&eNB->frame_parms,eNB->proc.frame_prach_br,eNB->proc.subframe_prach_br);

    for (i=0; i<4; i++) {
      if ((eNB->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[i]==1) &&
          ((prach_mask&(1<<(i+1))) > 0)) { // check that prach CE level is active now

        // if first reception in group of repetitions store frame for later (in RA-RNTI for Msg2)
        if (eNB->prach_vars_br.repetition_number[i]==0) eNB->prach_vars_br.first_frame[i]=eNB->proc.frame_prach_br;

        // increment repetition number
        eNB->prach_vars_br.repetition_number[i]++;
        // do basic PRACH reception
        rx_prach0(eNB,ru,frame_prach,subframe,max_preamble,max_preamble_energy,max_preamble_delay,avg_preamble_energy,Nf,tdd_mapindex,1,i);

        // if last repetition, clear counter
        if (eNB->prach_vars_br.repetition_number[i] == eNB->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[i]) {
          eNB->prach_vars_br.repetition_number[i]=0;
        }
      }
    } /* for i ... */
  } /* else br_flag == 0 */
}



