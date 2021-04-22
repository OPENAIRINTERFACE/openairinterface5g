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

/*! \file PHY/LTE_TRANSPORT/srs_modulation.c
* \brief Top-level routines for generating sounding reference signal (SRS) V8.6 2009-03
* \author R. Knopp, F. Kaltenberger
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr, florian.kaltenberger@eurecom.fr
* \note
* \warning
*/
#include "PHY/defs_UE.h"
#include "PHY/phy_extern_ue.h"
#include "common/utils/LOG/log.h"

unsigned short msrsb_6_40[8][4] = {{36,12,4,4},
  {32,16,8,4},
  {24,4,4,4},
  {20,4,4,4},
  {16,4,4,4},
  {12,4,4,4},
  {8,4,4,4},
  {4,4,4,4}
};

unsigned short msrsb_41_60[8][4] = {{48,24,12,4},
  {48,16,8,4},
  {40,20,4,4},
  {36,12,4,4},
  {32,16,8,4},
  {24,4,4,4},
  {20,4,4,4},
  {16,4,4,4}
};

unsigned short msrsb_61_80[8][4] = {{72,24,12,4},
  {64,32,16,4},
  {60,20,4,4},
  {48,24,12,4},
  {48,16,8,4},
  {40,20,4,4},
  {36,12,4,4},
  {32,16,8,4}
};

unsigned short msrsb_81_110[8][4] = {{96,48,24,4},
  {96,32,16,4},
  {80,40,20,4},
  {72,24,12,4},
  {64,32,16,4},
  {60,20,4,4},
  {48,24,12,4},
  {48,16,8,4}
};

unsigned short Nb_6_40[8][4] = {{36,12,4,4},
  {32,16,8,4},
  {24,4,4,4},
  {20,4,4,4},
  {16,4,4,4},
  {12,4,4,4},
  {8,4,4,4},
  {4,4,4,4}
};

unsigned short Nb_41_60[8][4] = {{48,24,12,4},
  {48,16,8,4},
  {40,20,4,4},
  {36,12,4,4},
  {32,16,8,4},
  {24,4,4,4},
  {20,4,4,4},
  {16,4,4,4}
};

unsigned short Nb_61_80[8][4] = {{72,24,12,4},
  {64,32,16,4},
  {60,20,4,4},
  {48,24,12,4},
  {48,16,8,4},
  {40,20,4,4},
  {36,12,4,4},
  {32,16,8,4}
};

unsigned short Nb_81_110[8][4] = {{96,48,24,4},
  {96,32,16,4},
  {80,40,20,4},
  {72,24,12,4},
  {64,32,16,4},
  {60,20,4,4},
  {48,24,12,4},
  {48,16,8,4}
};

unsigned short transmission_offset_tdd[16] = {2,6,10,18,14,22,26,30,70,74,194,326,586,210};

int compareints (const void * a, const void * b)
{
  return ( *(unsigned short*)a - *(unsigned short*)b );
}

#define DEBUG_SRS
int32_t generate_srs(LTE_DL_FRAME_PARMS *frame_parms,
		     SOUNDINGRS_UL_CONFIG_DEDICATED *soundingrs_ul_config_dedicated,
		     int32_t *txptr,
		     int16_t amp,
		     uint32_t subframe)
{

  uint16_t msrsb=0,Nb=0,nb,b,msrs0=0,k,Msc_RS,Msc_RS_idx,carrier_pos;
  uint16_t *Msc_idx_ptr;
  int32_t k0;
  //uint32_t subframe_offset;
  uint8_t Bsrs  = soundingrs_ul_config_dedicated->srs_Bandwidth;
  uint8_t Csrs  = frame_parms->soundingrs_ul_config_common.srs_BandwidthConfig;
  uint8_t n_RRC = soundingrs_ul_config_dedicated->freqDomainPosition;
  uint8_t kTC   = soundingrs_ul_config_dedicated->transmissionComb;

  // u is the PUCCH sequence-group number defined in Section 5.5.1.3
  // ν is the base sequence number defined in Section 5.5.1.4
  uint32_t u=frame_parms->pucch_config_common.grouphop[1+(subframe<<1)];;
  uint32_t v=frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.seqhop[1+(subframe<<1)];

  LOG_D(PHY,"SRS root sequence: u %d, v %d\n",u,v);
  LOG_D(PHY,"CommonSrsConfig:    Csrs %d, Ssrs %d, AnSrsSimultan %d \n",Csrs,
        frame_parms->soundingrs_ul_config_common.srs_SubframeConfig,
        frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission);
  LOG_D(PHY,"DedicatedSrsConfig: Bsrs %d, bhop %d, nRRC %d, Isrs %d, kTC %d, n_SRS %d\n",Bsrs,soundingrs_ul_config_dedicated->srs_HoppingBandwidth,n_RRC
                                                                                       ,soundingrs_ul_config_dedicated->srs_ConfigIndex,kTC
                                                                                       ,soundingrs_ul_config_dedicated->cyclicShift);

  if (frame_parms->N_RB_UL < 41) {
    msrs0 = msrsb_6_40[Csrs][0];
    msrsb = msrsb_6_40[Csrs][Bsrs];
    Nb    = Nb_6_40[Csrs][Bsrs];
  } else if (frame_parms->N_RB_UL < 61) {
    msrs0 = msrsb_41_60[Csrs][0];
    msrsb = msrsb_41_60[Csrs][Bsrs];
    Nb    = Nb_41_60[Csrs][Bsrs];
  } else if (frame_parms->N_RB_UL < 81) {
    msrs0 = msrsb_61_80[Csrs][0];
    msrsb = msrsb_61_80[Csrs][Bsrs];
    Nb    = Nb_61_80[Csrs][Bsrs];
  } else if (frame_parms->N_RB_UL <111) {
    msrs0 = msrsb_81_110[Csrs][0];
    msrsb = msrsb_81_110[Csrs][Bsrs];
    Nb    = Nb_81_110[Csrs][Bsrs];
  }

  Msc_RS = msrsb * 6;
  k0 = ( ( (int16_t)(frame_parms->N_RB_UL>>1) - (int16_t)(msrs0>>1) ) * 12 ) + kTC;
  AssertFatal(msrsb,"divide by 0");
  nb  = (4*n_RRC/msrsb)%Nb;

  for (b=0; b<=Bsrs; b++) {
    k0 += 2*nb*Msc_RS;
  }

  if (k0<0) {
    LOG_E(PHY,"generate_srs: invalid parameter set msrs0=%d, msrsb=%d, Nb=%d => nb=%d, k0=%d\n",msrs0,msrsb,Nb,nb,k0);
    return(-1);
  }

  Msc_idx_ptr = (uint16_t*) bsearch((uint16_t*) &Msc_RS, (uint16_t*) dftsizes, 34, sizeof(uint16_t), compareints);

  if (Msc_idx_ptr)
    Msc_RS_idx = Msc_idx_ptr - dftsizes;
  else {
    LOG_E(PHY,"generate_srs: index for Msc_RS=%d not found\n",Msc_RS);
    return(-1);
  }

#ifdef DEBUG_SRS
  LOG_D(PHY,"generate_srs_tx: Msc_RS = %d, Msc_RS_idx = %d, k0 = %d\n",Msc_RS, Msc_RS_idx,k0);
#endif

    carrier_pos = (frame_parms->first_carrier_offset + k0);
    if (carrier_pos>frame_parms->ofdm_symbol_size) {
        carrier_pos -= frame_parms->ofdm_symbol_size;
    }

    for (k=0; k<Msc_RS; k++) {
      int32_t real = ((int32_t) amp * (int32_t) ul_ref_sigs[u][v][Msc_RS_idx][k<<1])     >> 15;
      int32_t imag = ((int32_t) amp * (int32_t) ul_ref_sigs[u][v][Msc_RS_idx][(k<<1)+1]) >> 15;
      txptr[carrier_pos] = (real&0xFFFF) + ((imag<<16)&0xFFFF0000);


      carrier_pos = carrier_pos+2;

      if (carrier_pos >= frame_parms->ofdm_symbol_size)
        carrier_pos=carrier_pos-frame_parms->ofdm_symbol_size;
    }

  return(0);
}

int generate_srs_tx_emul(PHY_VARS_UE *phy_vars_ue,uint8_t subframe)
{

  LOG_D(PHY,"[UE] generate_srs_tx_emul for subframe %d\n",subframe);
  return(0);
}

#ifdef MAIN
main()
{


}
#endif
