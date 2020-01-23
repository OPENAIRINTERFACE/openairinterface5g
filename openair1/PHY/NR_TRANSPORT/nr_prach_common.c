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
 * \brief Common routines for NR UE/gNB PRACH physical channel V15.4 2019-03
 * \author R. Knopp
 * \date 2019
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
#include "PHY/NR_TRANSPORT/nr_prach.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto_common.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "T.h"

void init_nr_prach_tables(int N_ZC);

void dump_nr_prach_config(NR_DL_FRAME_PARMS *frame_parms,uint8_t subframe) {

  FILE *fd;

  fd = fopen("prach_config.txt","w");
  fprintf(fd,"prach_config: subframe          = %d\n",subframe);
  fprintf(fd,"prach_config: N_RB_UL           = %d\n",frame_parms->N_RB_UL);
  fprintf(fd,"prach_config: frame_type        = %s\n",(frame_parms->frame_type==1) ? "TDD":"FDD");

  if (frame_parms->frame_type==TDD) {
    fprintf(fd,"prach_config: p_tdd_UL_DL_Configuration.referenceSCS                  = %d\n",frame_parms->p_tdd_UL_DL_Configuration->referenceSubcarrierSpacing);
    fprintf(fd,"prach_config: p_tdd_UL_DL_Configuration.dl_UL_TransmissionPeriodicity = %d\n",frame_parms->p_tdd_UL_DL_Configuration->dl_UL_TransmissionPeriodicity);
    fprintf(fd,"prach_config: p_tdd_UL_DL_Configuration.nrofDownlinkSlots             = %d\n",frame_parms->p_tdd_UL_DL_Configuration->nrofDownlinkSlots);
    fprintf(fd,"prach_config: p_tdd_UL_DL_Configuration.nrofDownlinkSymbols           = %d\n",frame_parms->p_tdd_UL_DL_Configuration->nrofDownlinkSymbols);
    fprintf(fd,"prach_config: p_tdd_UL_DL_Configuration.nrofUownlinkSlots             = %d\n",frame_parms->p_tdd_UL_DL_Configuration->nrofDownlinkSlots);
    fprintf(fd,"prach_config: p_tdd_UL_DL_Configuration.nrofUownlinkSymbols           = %d\n",frame_parms->p_tdd_UL_DL_Configuration->nrofDownlinkSymbols);
    if (frame_parms->p_tdd_UL_DL_Configuration->p_next) {
      fprintf(fd,"prach_config: p_tdd_UL_DL_Configuration.referenceSCS2                  = %d\n",frame_parms->p_tdd_UL_DL_Configuration->p_next->referenceSubcarrierSpacing);
      fprintf(fd,"prach_config: p_tdd_UL_DL_Configuration.dl_UL_TransmissionPeriodicity2 = %d\n",frame_parms->p_tdd_UL_DL_Configuration->p_next->dl_UL_TransmissionPeriodicity);
      fprintf(fd,"prach_config: p_tdd_UL_DL_Configuration.nrofDownlinkSlots2             = %d\n",frame_parms->p_tdd_UL_DL_Configuration->p_next->nrofDownlinkSlots);
      fprintf(fd,"prach_config: p_tdd_UL_DL_Configuration.nrofDownlinkSymbols2           = %d\n",frame_parms->p_tdd_UL_DL_Configuration->p_next->nrofDownlinkSymbols);
      fprintf(fd,"prach_config: p_tdd_UL_DL_Configuration.nrofUownlinkSlots2             = %d\n",frame_parms->p_tdd_UL_DL_Configuration->p_next->nrofDownlinkSlots);
      fprintf(fd,"prach_config: p_tdd_UL_DL_Configuration.nrofUownlinkSymbols2           = %d\n",frame_parms->p_tdd_UL_DL_Configuration->p_next->nrofDownlinkSymbols);

    }
  }

  fprintf(fd,"prach_config: rootSequenceIndex = %d\n",frame_parms->prach_config_common.rootSequenceIndex);
  fprintf(fd,"prach_config: prach_ConfigIndex = %d\n",frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex);
  fprintf(fd,"prach_config: Ncs_config        = %d\n",frame_parms->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig);
  fprintf(fd,"prach_config: highSpeedFlag     = %d\n",frame_parms->prach_config_common.prach_ConfigInfo.highSpeedFlag);
  fprintf(fd,"prach_config: n_ra_prboffset    = %d\n",frame_parms->prach_config_common.prach_ConfigInfo.msg1_frequencystart);
  fclose(fd);

}

// This function computes the du
void nr_fill_du(uint8_t prach_fmt)
{

  uint16_t iu,u,p;
  uint16_t N_ZC;
  uint16_t *prach_root_sequence_map;

  if (prach_fmt<4) {
    N_ZC = 839;
    prach_root_sequence_map = prach_root_sequence_map_0_3;
  } else {
    N_ZC = 139;
    prach_root_sequence_map = prach_root_sequence_map_abc;
  }

  for (iu=0; iu<(N_ZC-1); iu++) {

    u=prach_root_sequence_map[iu];
    p=1;

    while (((u*p)%N_ZC)!=1)
      p++;

    nr_du[u] = ((p<(N_ZC>>1)) ? p : (N_ZC-p));
  }

}

int is_nr_prach_subframe(NR_DL_FRAME_PARMS *frame_parms,uint32_t frame, uint8_t subframe) {

  uint8_t prach_ConfigIndex  = frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
  /*
  // For FR1 paired
  if (((frame%table_6_3_3_2_2_prachConfig_Index[prach_ConfigIndex][2]) == table_6_3_3_2_2_prachConfig_Index[prach_ConfigIndex][3]) &&
  ((table_6_3_3_2_2_prachConfig_Index[prach_ConfigIndex][4]&(1<<subframe)) == 1)) {
  // using table 6.3.3.2-2: Random access configurations for FR1 and paired spectrum/supplementary uplink
  return(1);
  } else {
  return(0);
  }
  */
  // For FR1 unpaired
  if (((frame%table_6_3_3_2_3_prachConfig_Index[prach_ConfigIndex][2]) == table_6_3_3_2_3_prachConfig_Index[prach_ConfigIndex][3]) &&
      ((table_6_3_3_2_3_prachConfig_Index[prach_ConfigIndex][4]&(1<<subframe)) == 1)) {
    // using table 6.3.3.2-2: Random access configurations for FR1 and unpaired
    return(1);
  } else {
    return(0);
  }
  /*
  // For FR2: FIXME
  if ((((frame%table_6_3_3_2_4_prachConfig_Index[prach_ConfigIndex][2]) == table_6_3_3_2_4_prachConfig_Index[prach_ConfigIndex][3]) ||
  ((frame%table_6_3_3_2_4_prachConfig_Index[prach_ConfigIndex][2]) == table_6_3_3_2_4_prachConfig_Index[prach_ConfigIndex][4]))
  &&
  ((table_6_3_3_2_4_prachConfig_Index[prach_ConfigIndex][5]&(1<<subframe)) == 1)) {
  // using table 6.3.3.2-2: Random access configurations for FR1 and unpaired
  return(1);
  } else {
  return(0);
  }
  */
}


int do_prach_rx(NR_DL_FRAME_PARMS *fp,int frame,int slot) {

  int subframe = slot / fp->slots_per_subframe;
  // when were in the last slot of the subframe and this is a PRACH subframe ,return 1
  if (((slot%fp->slots_per_subframe) == fp->slots_per_subframe-1)&&
      (is_nr_prach_subframe(fp,frame,subframe))) return (1);
  else return(0);
}

uint16_t get_nr_prach_fmt(int prach_ConfigIndex,lte_frame_type_t frame_type, nr_frequency_range_e fr)
{

  if (frame_type==FDD) return (table_6_3_3_2_2_prachConfig_Index[prach_ConfigIndex][0]); // if using table 6.3.3.2-2: Random access configurations for FR1 and paired spectrum/supplementary uplink
  else if (fr==nr_FR1) return (table_6_3_3_2_3_prachConfig_Index[prach_ConfigIndex][0]); // if using table 6.3.3.2-3: Random access configurations for FR1 and unpaired spectrum
  else AssertFatal(1==0,"FR2 prach configuration not supported yet\n");
  // For FR2 not implemented. FIXME
}

void compute_nr_prach_seq(uint16_t rootSequenceIndex,
			  uint8_t prach_ConfigIndex,
			  uint8_t zeroCorrelationZoneConfig,
			  uint8_t highSpeedFlag,
			  lte_frame_type_t frame_type,
			  nr_frequency_range_e fr,
			  uint32_t X_u[64][839])
{

  // Compute DFT of x_u => X_u[k] = x_u(inv(u)*k)^* X_u[k] = exp(j\pi u*inv(u)*k*(inv(u)*k+1)/N_ZC)
  unsigned int k,inv_u,i,NCS=0,num_preambles;
  int N_ZC;
  uint8_t prach_fmt = get_nr_prach_fmt(prach_ConfigIndex,frame_type,fr);
  uint16_t *prach_root_sequence_map;
  uint16_t u, preamble_offset;
  uint16_t n_shift_ra,n_shift_ra_bar, d_start,numshift;
  uint8_t not_found;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_UE_COMPUTE_PRACH, VCD_FUNCTION_IN);

#ifdef NR_PRACH_DEBUG
  LOG_I(PHY,"compute_prach_seq: NCS_config %d, prach_fmt %x\n",zeroCorrelationZoneConfig, prach_fmt);
#endif


  N_ZC = (prach_fmt < 4) ? 839 : 139;
  //init_prach_tables(N_ZC); //moved to phy_init_lte_ue/eNB, since it takes to long in real-time
  
  init_nr_prach_tables(N_ZC);

  if (prach_fmt < 4) {
    prach_root_sequence_map = prach_root_sequence_map_0_3;
  } else {
    // FIXME cannot be reached
    prach_root_sequence_map = prach_root_sequence_map_abc;
  }


#ifdef PRACH_DEBUG
  LOG_I( PHY, "compute_prach_seq: done init prach_tables\n" );
#endif




  int restricted_Type = 0; //this is hardcoded ('0' for restricted_TypeA; and '1' for restricted_TypeB). FIXME

  if (highSpeedFlag== 0) {
    
#ifdef PRACH_DEBUG
    LOG_I(PHY,"Low speed prach : NCS_config %d\n",zeroCorrelationZoneConfig);
#endif
    
    AssertFatal(zeroCorrelationZoneConfig<=15,
		"FATAL, Illegal Ncs_config for unrestricted format %"PRIu8"\n", zeroCorrelationZoneConfig );
    if (prach_fmt<3)  NCS = NCS_unrestricted_delta_f_RA_125[zeroCorrelationZoneConfig];
    if (prach_fmt==3) NCS = NCS_unrestricted_delta_f_RA_5[zeroCorrelationZoneConfig];
    if (prach_fmt>3)  NCS = NCS_unrestricted_delta_f_RA_15[zeroCorrelationZoneConfig];
    
    num_preambles = (NCS==0) ? 64 : ((64*NCS)/N_ZC);

    if (NCS>0) num_preambles++;

    preamble_offset = 0;
  } else {

#ifdef PRACH_DEBUG
    LOG_I( PHY, "high speed prach : NCS_config %"PRIu8"\n", zeroCorrelationZoneConfig );
#endif

    AssertFatal(zeroCorrelationZoneConfig<=14,
		"FATAL, Illegal Ncs_config for restricted format %"PRIu8"\n", zeroCorrelationZoneConfig );
    if (prach_fmt<3){
      if (restricted_Type == 0) NCS = NCS_restricted_TypeA_delta_f_RA_125[zeroCorrelationZoneConfig]; // for TypeA, this is hardcoded. FIXME
      if (restricted_Type == 1) NCS = NCS_restricted_TypeB_delta_f_RA_125[zeroCorrelationZoneConfig]; // for TypeB, this is hardcoded. FIXME
    }
    if (prach_fmt==3){
      if (restricted_Type == 0) NCS = NCS_restricted_TypeA_delta_f_RA_5[zeroCorrelationZoneConfig]; // for TypeA, this is hardcoded. FIXME
      if (restricted_Type == 1) NCS = NCS_restricted_TypeB_delta_f_RA_5[zeroCorrelationZoneConfig]; // for TypeB, this is hardcoded. FIXME
    }
    if (prach_fmt>3){

    }

    nr_fill_du(prach_fmt);

    num_preambles = 64; // compute ZC sequence for 64 possible roots
    // find first non-zero shift root (stored in preamble_offset)
    not_found = 1;
    preamble_offset = 0;

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
      if (numshift>0)
        not_found = 0;
      else
        preamble_offset++;
    }
  }

#ifdef PRACH_DEBUG

  if (NCS>0)
    LOG_I( PHY, "Initializing %u preambles for PRACH (NCS_config %"PRIu8", NCS %u, N_ZC/NCS %u)\n",
           num_preambles, zeroCorrelationZoneConfig, NCS, N_ZC/NCS );

#endif

  for (i=0; i<num_preambles; i++) {
    int index = (rootSequenceIndex+i+preamble_offset) % N_ZC;

    if (prach_fmt<4) {
      // prach_root_sequence_map points to prach_root_sequence_map0_3
      DevAssert( index < sizeof(prach_root_sequence_map_0_3) / sizeof(prach_root_sequence_map_0_3[0]) );
    } else {
      // prach_root_sequence_map points to prach_root_sequence_map4
      DevAssert( index < sizeof(prach_root_sequence_map_abc) / sizeof(prach_root_sequence_map_abc[0]) );
    }

    u = prach_root_sequence_map[index];

    inv_u = nr_ZC_inv[u]; // multiplicative inverse of u


    // X_u[0] stores the first ZC sequence where the root u has a non-zero number of shifts
    // for the unrestricted case X_u[0] is the first root indicated by the rootSequenceIndex

    for (k=0; k<N_ZC; k++) {
      // 420 is the multiplicative inverse of 2 (required since ru is exp[j 2\pi n])
      X_u[i][k] = ((uint32_t*)nr_ru)[(((k*(1+(inv_u*k)))%N_ZC)*420)%N_ZC];
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_UE_COMPUTE_PRACH, VCD_FUNCTION_OUT);

}

void init_nr_prach_tables(int N_ZC)
{

  int i,m;

  // Compute the modular multiplicative inverse 'iu' of u s.t. iu*u = 1 mod N_ZC
  nr_ZC_inv[0] = 0;
  nr_ZC_inv[1] = 1;

  for (i=2; i<N_ZC; i++) {
    for (m=2; m<N_ZC; m++)
      if (((i*m)%N_ZC) == 1) {
        nr_ZC_inv[i] = m;
        break;
      }

#ifdef PRACH_DEBUG

    if (i<16)
      printf("i %d : inv %d\n",i,nr_ZC_inv[i]);

#endif
  }

  // Compute quantized roots of unity
  for (i=0; i<N_ZC; i++) {
    nr_ru[i<<1]     = (int16_t)(floor(32767.0*cos(2*M_PI*(double)i/N_ZC)));
    nr_ru[1+(i<<1)] = (int16_t)(floor(32767.0*sin(2*M_PI*(double)i/N_ZC)));
#ifdef PRACH_DEBUG

    if (i<16)
      printf("i %d : runity %d,%d\n",i,nr_ru[i<<1],nr_ru[1+(i<<1)]);

#endif
  }
}

