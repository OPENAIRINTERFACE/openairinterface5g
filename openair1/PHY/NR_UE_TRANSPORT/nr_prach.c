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
//#include "PHY/defs_common.h"
//#include "PHY/phy_extern.h"
//#include "PHY/phy_extern_ue.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

//#include "PHY/defs.h"
#include "PHY/impl_defs_nr.h"
//#include "PHY/defs_nr_common.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/NR_UE_TRANSPORT/nr_prach.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
//#include "PHY/extern.h"
//#include "LAYER2/MAC/extern.h"
//#include "PHY/NR_UE_TRANSPORT/pucch_nr.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "T.h"



#define NR_PRACH_DEBUG 1




void dump_nr_prach_config(NR_DL_FRAME_PARMS *frame_parms,uint8_t subframe)
{

  FILE *fd;

  fd = fopen("prach_config.txt","w");
  fprintf(fd,"prach_config: subframe          = %d\n",subframe);
  fprintf(fd,"prach_config: N_RB_UL           = %d\n",frame_parms->N_RB_UL);
  fprintf(fd,"prach_config: frame_type        = %s\n",(frame_parms->frame_type==1) ? "TDD":"FDD");

  if(frame_parms->frame_type==1) fprintf(fd,"prach_config: tdd_config        = %d\n",frame_parms->tdd_config);

  fprintf(fd,"prach_config: rootSequenceIndex = %d\n",frame_parms->prach_config_common.rootSequenceIndex);
  fprintf(fd,"prach_config: prach_ConfigIndex = %d\n",frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex);
  fprintf(fd,"prach_config: Ncs_config        = %d\n",frame_parms->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig);
  fprintf(fd,"prach_config: highSpeedFlag     = %d\n",frame_parms->prach_config_common.prach_ConfigInfo.highSpeedFlag);
  fprintf(fd,"prach_config: n_ra_prboffset    = %d\n",frame_parms->prach_config_common.prach_ConfigInfo.prach_FreqOffset);
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

#if 0
uint8_t get_num_prach_tdd(module_id_t Mod_id)
{
  NR_DL_FRAME_PARMS *fp = &PHY_vars_UE_g[Mod_id][0]->frame_parms;
  return(tdd_preamble_map[fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex][fp->tdd_config].num_prach);
}

uint8_t get_fid_prach_tdd(module_id_t Mod_id,uint8_t tdd_map_index)
{
  NR_DL_FRAME_PARMS *fp = &PHY_vars_UE_g[Mod_id][0]->frame_parms;
  return(tdd_preamble_map[fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex][fp->tdd_config].map[tdd_map_index].f_ra);
}
#endif

uint16_t get_nr_prach_fmt(uint8_t prach_ConfigIndex)
{
  //return (table_6_3_3_2_2_prachConfig_Index[prach_ConfigIndex][0]); // if using table 6.3.3.2-2: Random access configurations for FR1 and paired spectrum/supplementary uplink
  return (table_6_3_3_2_3_prachConfig_Index[prach_ConfigIndex][0]); // if using table 6.3.3.2-3: Random access configurations for FR1 and unpaired spectrum
  // For FR2 not implemented. FIXME
}

#if 0
uint8_t get_prach_fmt(uint8_t prach_ConfigIndex,lte_frame_type_t frame_type)
{

  if (frame_type == FDD) // FDD
    return(prach_ConfigIndex>>4);

  else {
    if (prach_ConfigIndex < 20)
      return (0);

    if (prach_ConfigIndex < 30)
      return (1);

    if (prach_ConfigIndex < 40)
      return (2);

    if (prach_ConfigIndex < 48)
      return (3);
    else
      return (4);
  }
}

uint8_t get_prach_prb_offset(NR_DL_FRAME_PARMS *frame_parms,
			     uint8_t prach_ConfigIndex, 
			     uint8_t n_ra_prboffset,
			     uint8_t tdd_mapindex, uint16_t Nf, uint16_t prach_fmt)
{
  lte_frame_type_t frame_type         = frame_parms->frame_type;
  uint8_t tdd_config         = frame_parms->tdd_config;

  uint8_t n_ra_prb;
  uint8_t f_ra,t1_ra;
  uint8_t Nsp=2;

  if (frame_type == TDD) { // TDD

    if (tdd_preamble_map[prach_ConfigIndex][tdd_config].num_prach==0) {
      LOG_E(PHY, "Illegal prach_ConfigIndex %"PRIu8"", prach_ConfigIndex);
      return(-1);
    }

    // adjust n_ra_prboffset for frequency multiplexing (p.36 36.211)
    f_ra = tdd_preamble_map[prach_ConfigIndex][tdd_config].map[tdd_mapindex].f_ra;

    if (prach_fmt < 4) {
      if ((f_ra&1) == 0) {
        n_ra_prb = n_ra_prboffset + 6*(f_ra>>1);
      } else {
        n_ra_prb = frame_parms->N_RB_UL - 6 - n_ra_prboffset + 6*(f_ra>>1);
      }
    } else {
      if ((tdd_config >2) && (tdd_config<6))
        Nsp = 2;

      t1_ra = tdd_preamble_map[prach_ConfigIndex][tdd_config].map[0].t1_ra;

      if ((((Nf&1)*(2-Nsp)+t1_ra)&1) == 0) {
        n_ra_prb = 6*f_ra;
      } else {
        n_ra_prb = frame_parms->N_RB_UL - 6*(f_ra+1);
      }
    }
  }
  else { //FDD
    n_ra_prb = n_ra_prboffset;
  }
  return(n_ra_prb);
}
#endif //0

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

#if 0
int is_prach_subframe0(NR_DL_FRAME_PARMS *frame_parms,uint8_t prach_ConfigIndex,uint32_t frame, uint8_t subframe)
{
  //  uint8_t prach_ConfigIndex  = frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
  uint8_t tdd_config         = frame_parms->tdd_config;
  uint8_t t0_ra;
  uint8_t t1_ra;
  uint8_t t2_ra;

  int prach_mask = 0;

  if (frame_parms->frame_type == FDD) { //FDD
    //implement Table 5.7.1-2 from 36.211 (Rel-10, p.41)
    if ((((frame&1) == 1) && (subframe < 9)) ||
        (((frame&1) == 0) && (subframe == 9)))  // This is an odd frame, ignore even-only PRACH frames
      if (((prach_ConfigIndex&0xf)<3) || // 0,1,2,16,17,18,32,33,34,48,49,50
          ((prach_ConfigIndex&0x1f)==18) || // 18,50
          ((prach_ConfigIndex&0xf)==15))   // 15,47
        return(0);

    switch (prach_ConfigIndex&0x1f) {
    case 0:
    case 3:
      if (subframe==1) prach_mask = 1;
      break;

    case 1:
    case 4:
      if (subframe==4) prach_mask = 1;
      break;

    case 2:
    case 5:
      if (subframe==7) prach_mask = 1;
      break;

    case 6:
      if ((subframe==1) || (subframe==6)) prach_mask=1;
      break;

    case 7:
      if ((subframe==2) || (subframe==7)) prach_mask=1;
      break;

    case 8:
      if ((subframe==3) || (subframe==8)) prach_mask=1;
      break;

    case 9:
      if ((subframe==1) || (subframe==4) || (subframe==7)) prach_mask=1;
      break;

    case 10:
      if ((subframe==2) || (subframe==5) || (subframe==8)) prach_mask=1;
      break;

    case 11:
      if ((subframe==3) || (subframe==6) || (subframe==9)) prach_mask=1;
      break;

    case 12:
      if ((subframe&1)==0) prach_mask=1;
      break;

    case 13:
      if ((subframe&1)==1) prach_mask=1;
      break;

    case 14:
      prach_mask=1;
      break;

    case 15:
      if (subframe==9) prach_mask=1;
      break;
    }
  } else { // TDD

    AssertFatal(prach_ConfigIndex<64,
		"Illegal prach_ConfigIndex %d for ",prach_ConfigIndex);
    AssertFatal(tdd_preamble_map[prach_ConfigIndex][tdd_config].num_prach>0,
		"Illegal prach_ConfigIndex %d for ",prach_ConfigIndex);

    t0_ra = tdd_preamble_map[prach_ConfigIndex][tdd_config].map[0].t0_ra;
    t1_ra = tdd_preamble_map[prach_ConfigIndex][tdd_config].map[0].t1_ra;
    t2_ra = tdd_preamble_map[prach_ConfigIndex][tdd_config].map[0].t2_ra;
#ifdef PRACH_DEBUG
    LOG_I(PHY,"[PRACH] Checking for PRACH format (ConfigIndex %d) in TDD subframe %d (%d,%d,%d)\n",
          prach_ConfigIndex,
          subframe,
          t0_ra,t1_ra,t2_ra);
#endif

    if ((((t0_ra == 1) && ((frame &1)==0))||  // frame is even and PRACH is in even frames
         ((t0_ra == 2) && ((frame &1)==1))||  // frame is odd and PRACH is in odd frames
         (t0_ra == 0)) &&                                // PRACH is in all frames
        (((subframe<5)&&(t1_ra==0)) ||                   // PRACH is in 1st half-frame
         (((subframe>4)&&(t1_ra==1))))) {                // PRACH is in 2nd half-frame
      if ((prach_ConfigIndex<48) &&                          // PRACH only in normal UL subframe
	  (((subframe%5)-2)==t2_ra)) prach_mask=1;
      else if ((prach_ConfigIndex>47) && (((subframe%5)-1)==t2_ra)) prach_mask=1;      // PRACH can be in UpPTS
    }
  }

  return(prach_mask);
}

int is_prach_subframe(NR_DL_FRAME_PARMS *frame_parms,uint32_t frame, uint8_t subframe) {
  
  uint8_t prach_ConfigIndex  = frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
  int prach_mask             = is_prach_subframe0(frame_parms,prach_ConfigIndex,frame,subframe);

#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  int i;

  for (i=0;i<4;i++) {
    if (frame_parms->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[i] == 1) 
      prach_mask|=(is_prach_subframe0(frame_parms,frame_parms->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[i],frame,subframe)<<(i+1));
  }
#endif
  return(prach_mask);
}
#endif //0

void compute_nr_prach_seq(uint16_t rootSequenceIndex,
		       uint8_t prach_ConfigIndex,
		       uint8_t zeroCorrelationZoneConfig,
		       uint8_t highSpeedFlag,
		       lte_frame_type_t frame_type,
		       uint32_t X_u[64][839])
{

  // Compute DFT of x_u => X_u[k] = x_u(inv(u)*k)^* X_u[k] = exp(j\pi u*inv(u)*k*(inv(u)*k+1)/N_ZC)
  unsigned int k,inv_u,i,NCS=0,num_preambles;
  int N_ZC;
  uint8_t prach_fmt = get_prach_fmt(prach_ConfigIndex,frame_type);
  uint16_t *prach_root_sequence_map;
  uint16_t u, preamble_offset;
  uint16_t n_shift_ra,n_shift_ra_bar, d_start,numshift;
  uint8_t not_found;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_UE_COMPUTE_PRACH, VCD_FUNCTION_IN);

#ifdef PRACH_DEBUG
  LOG_I(PHY,"compute_prach_seq: NCS_config %d, prach_fmt %d\n",zeroCorrelationZoneConfig, prach_fmt);
#endif

  AssertFatal(prach_fmt<4,
	      "PRACH sequence is only precomputed for prach_fmt<4 (have %"PRIu8")\n", prach_fmt );
  N_ZC = (prach_fmt < 4) ? 839 : 139;
  //init_prach_tables(N_ZC); //moved to phy_init_lte_ue/eNB, since it takes to long in real-time

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
    //NCS = NCS_restricted[zeroCorrelationZoneConfig];
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


int32_t generate_nr_prach( PHY_VARS_NR_UE *ue, uint8_t eNB_id, uint8_t subframe, uint16_t Nf )
{

  //lte_frame_type_t frame_type         = ue->frame_parms.frame_type;
  //uint8_t tdd_config         = ue->frame_parms.tdd_config;
  uint16_t rootSequenceIndex = ue->frame_parms.prach_config_common.rootSequenceIndex;
  uint8_t prach_ConfigIndex  = ue->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
  uint8_t Ncs_config         = ue->frame_parms.prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig;
  uint8_t restricted_set     = ue->frame_parms.prach_config_common.prach_ConfigInfo.highSpeedFlag;
  //uint8_t n_ra_prboffset     = ue->frame_parms.prach_config_common.prach_ConfigInfo.prach_FreqOffset;
  uint8_t preamble_index     = ue->prach_resources[eNB_id]->ra_PreambleIndex;
  //uint8_t tdd_mapindex       = ue->prach_resources[eNB_id]->ra_TDD_map_index;
  int16_t *prachF           = ue->prach_vars[eNB_id]->prachF;
  static int16_t prach_tmp[45600*4] __attribute__((aligned(32)));
  int16_t *prach            = prach_tmp;
  int16_t *prach2;
  int16_t amp               = ue->prach_vars[eNB_id]->amp;
  int16_t Ncp;
  uint8_t n_ra_prb;
  uint16_t NCS=0;
  uint16_t *prach_root_sequence_map;
  uint16_t preamble_offset,preamble_shift;
  uint16_t preamble_index0,n_shift_ra,n_shift_ra_bar;
  uint16_t d_start,numshift;

  uint16_t prach_fmt = get_nr_prach_fmt(prach_ConfigIndex);
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
  int i, prach_len=0;
  uint16_t first_nonzero_root_idx=0;

#if defined(EXMIMO) || defined(OAI_USRP)
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

#else //normal case (simulation)
  prach_start = subframe*ue->frame_parms.samples_per_tti-ue->N_TA_offset;
  LOG_I(PHY,"[UE %d] prach_start %d, rx_offset %d, hw_timing_advance %d, N_TA_offset %d\n", ue->Mod_id,
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

  n_ra_prb = ue->frame_parms.prach_config_common.prach_ConfigInfo.prach_FreqOffset;
//  n_ra_prb = get_nr_prach_prb_offset(&(ue->frame_parms),
//                                     ue->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex,
//                                     ue->frame_parms.prach_config_common.prach_ConfigInfo.prach_FreqOffset,
//                                     tdd_mapindex,
//                                     Nf,
//                                     prach_fmt);
  prach_root_sequence_map = (prach_fmt<4) ? prach_root_sequence_map_0_3 : prach_root_sequence_map_abc;

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

  Xu = (int16_t*)ue->X_u[preamble_offset-first_nonzero_root_idx];

  /*
    k+=(12*ue->frame_parms.first_carrier_offset);
    if (k>(12*ue->frame_parms.ofdm_symbol_size))
    k-=(12*ue->frame_parms.ofdm_symbol_size);
  */
  k*=2;

  switch (ue->frame_parms.N_RB_UL) {
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
    if (ue->frame_parms.threequarter_fs == 0)
      memset((void*)prachF,0,4*24576);
    else
      memset((void*)prachF,0,4*18432);
    break;

  case 106:
    memset((void*)prachF,0,4*24576);
    break;
}

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

  for (offset=0,offset2=0; offset<N_ZC; offset++,offset2+=preamble_shift) {

    if (offset2 >= N_ZC)
      offset2 -= N_ZC;

    Xu_re = (((int32_t)Xu[offset<<1]*amp)>>15);
    Xu_im = (((int32_t)Xu[1+(offset<<1)]*amp)>>15);
    prachF[k++]= ((Xu_re*nr_ru[offset2<<1]) - (Xu_im*nr_ru[1+(offset2<<1)]))>>15;
    prachF[k++]= ((Xu_im*nr_ru[offset2<<1]) + (Xu_re*nr_ru[1+(offset2<<1)]))>>15;

    if (k==(12*2*ue->frame_parms.ofdm_symbol_size))
      k=0;
  }

  int mu = 1; // numerology is hardcoded. FIXME!!!
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
    Ncp = 3168;
    break;
  }
#if 0 // code for LTE
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
#endif
  if (ue->frame_parms.threequarter_fs == 1)
    Ncp=(Ncp*3)>>2;

  prach2 = prach+(Ncp<<1);

  // do IDFT
  switch (ue->frame_parms.N_RB_UL) {
  case 6:
    if (prach_fmt == 4) {
      idft256(prachF,prach2,1);
      memmove( prach, prach+512, Ncp<<2 );
      prach_len = 256+Ncp;
    } else {
      idft1536(prachF,prach2,1);
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
      idft512(prachF,prach2,1);
      //TODO: account for repeated format in dft output
      memmove( prach, prach+1024, Ncp<<2 );
      prach_len = 512+Ncp;
    } else {
      idft3072(prachF,prach2,1);
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
      idft1024(prachF,prach2,1);
      memmove( prach, prach+2048, Ncp<<2 );
      prach_len = 1024+Ncp;
    } else {
      idft6144(prachF,prach2,1);
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
      idft2048(prachF,prach2,1);
      memmove( prach, prach+4096, Ncp<<2 );
      prach_len = 2048+Ncp;
    } else {
      idft12288(prachF,prach2,1);
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
      idft3072(prachF,prach2,1);
      //TODO: account for repeated format in dft output
      memmove( prach, prach+6144, Ncp<<2 );
      prach_len = 3072+Ncp;
    } else {
      idft18432(prachF,prach2,1);
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
        idft4096(prachF,prach2,1);
        memmove( prach, prach+8192, Ncp<<2 );
        prach_len = 4096+Ncp;
      } else {
        idft24576(prachF,prach2,1);
        memmove( prach, prach+49152, Ncp<<2 );
        prach_len = 24576+Ncp;
        if (prach_fmt>1) {
          memmove( prach2+49152, prach2, 98304 );
          prach_len = 2* 24576+Ncp;
        }
      }
    }
    else {
      if (prach_fmt == 4) {
        idft3072(prachF,prach2,1);
        //TODO: account for repeated format in dft output
        memmove( prach, prach+6144, Ncp<<2 );
        prach_len = 3072+Ncp;
      } else {
        idft18432(prachF,prach2,1);
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

  case 106:
    if (prach_fmt == 0) {
      idft24576(prachF,prach2,1);
      memmove(prach, prach+49152, Ncp<<2);
      prach_len = 24576+Ncp;
    }
    if (prach_fmt == 1) {
      idft24576(prachF,prach2,1);
      memmove(prach2+49152, prach2, 98304);
      memmove(prach, prach+49152, Ncp<<2);
      prach_len = 2 * 24576 + Ncp;
    }
    if (prach_fmt == 2) {
      idft24576(prachF,prach2,1);
      memmove(prach2+49152, prach2, 98304);
      memmove(prach2+98304, prach2, 98304);
      memmove(prach, prach+49152, Ncp<<2);
      prach_len = 4 * 24576 + Ncp;
    }
    if (prach_fmt == 3) {
      idft6144(prachF,prach2,1);
      memmove(prach2+6144, prach2, 12288);
      memmove(prach2+12288, prach2, 12288);
      memmove(prach, prach+12288, Ncp<<2);
      prach_len = 4 * 6144 + Ncp;
    }
// For FR2
    if (prach_fmt == 0xa1) { // we consider numderology mu = 1
      idft1024(prachF,prach2,1);
      memmove(prach2+2048, prach2, 4096);
      memmove(prach, prach+2048, Ncp<<2);
      prach_len = 2 * 1024 + Ncp;
    }
#if 0
    if (prach_fmt == 0xa2) {
    }
    if (prach_fmt == 0xa3) {
    }
    if (prach_fmt == 0xb1) {
    }
    if (prach_fmt == 0xb2) {
    }
    if (prach_fmt == 0xb3) {
    }
    if (prach_fmt == 0xb4) {
    }
    if (prach_fmt == 0xc0) {
    }
    if (prach_fmt == 0xc2) {
    }
#endif
    break;
  }

  //LOG_I(PHY,"prach_len=%d\n",prach_len);

//  AssertFatal(prach_fmt<4,
//	      "prach_fmt4 not fully implemented" );
#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR)
  int j;
  int overflow = prach_start + prach_len - LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*ue->frame_parms.samples_per_tti;
  LOG_I( PHY, "prach_start=%d, overflow=%d\n", prach_start, overflow );

  for (i=prach_start,j=0; i<min(ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME,prach_start+prach_len); i++,j++) {
    ((int16_t*)ue->common_vars.txdata[0])[2*i] = prach[2*j];
    ((int16_t*)ue->common_vars.txdata[0])[2*i+1] = prach[2*j+1];
  }

  for (i=0; i<overflow; i++,j++) {
    ((int16_t*)ue->common_vars.txdata[0])[2*i] = prach[2*j];
    ((int16_t*)ue->common_vars.txdata[0])[2*i+1] = prach[2*j+1];
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
#else

  for (i=0; i<prach_len; i++) {
    ((int16_t*)(&ue->common_vars.txdata[0][prach_start]))[2*i] = prach[2*i];
    ((int16_t*)(&ue->common_vars.txdata[0][prach_start]))[2*i+1] = prach[2*i+1];
  }

#endif



#if defined(PRACH_WRITE_OUTPUT_DEBUG)
  LOG_M("prach_txF0.m","prachtxF0",prachF,prach_len-Ncp,1,1);
  LOG_M("prach_tx0.m","prachtx0",prach+(Ncp<<1),prach_len-Ncp,1,1);
  LOG_M("txsig.m","txs",(int16_t*)(&ue->common_vars.txdata[0][0]),2*ue->frame_parms.samples_per_tti,1,1);
  exit(-1);
#endif

  return signal_energy( (int*)prach, 256 );
}

