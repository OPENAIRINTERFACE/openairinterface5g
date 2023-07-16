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
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "T.h"
c16_t nr_ru[839]; // quantized roots of unity
static uint32_t nr_ZC_inv[839]; // multiplicative inverse for roots u
uint16_t nr_du[838];

/*void dump_nr_prach_config(NR_DL_FRAME_PARMS *frame_parms,uint8_t subframe) {

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

}*/

// This function computes the du
void nr_fill_du(uint16_t N_ZC, const uint16_t *prach_root_sequence_map)
{

  uint16_t iu,u,p;

  for (iu=0; iu<(N_ZC-1); iu++) {

    u=prach_root_sequence_map[iu];
    p=1;

    while (((u*p)%N_ZC)!=1)
      p++;

    nr_du[u] = ((p<(N_ZC>>1)) ? p : (N_ZC-p));
  }

}

void compute_nr_prach_seq(uint8_t short_sequence, uint8_t num_sequences, uint8_t rootSequenceIndex, c16_t X_u[64][839])
{
  // Compute DFT of x_u => X_u[k] = x_u(inv(u)*k)^* X_u[k] = exp(j\pi u*inv(u)*k*(inv(u)*k+1)/N_ZC)
  unsigned int k,inv_u,i;
  int N_ZC;

  const uint16_t *prach_root_sequence_map;
  uint16_t u;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_UE_COMPUTE_PRACH, VCD_FUNCTION_IN);

  LOG_D(PHY,"compute_prach_seq: prach short sequence %x, num_sequences %d, rootSequenceIndex %d\n", short_sequence, num_sequences, rootSequenceIndex);

  N_ZC = (short_sequence) ? 139 : 839;
  
  init_nr_prach_tables(N_ZC);

  if (short_sequence) {
    // FIXME cannot be reached
    prach_root_sequence_map = prach_root_sequence_map_abc;
  } else {
    prach_root_sequence_map = prach_root_sequence_map_0_3;
  }

  LOG_D( PHY, "compute_prach_seq: done init prach_tables\n" );

  for (i=0; i<num_sequences; i++) {
    int index = (rootSequenceIndex+i) % (N_ZC-1);

    if (short_sequence) {
      // prach_root_sequence_map points to prach_root_sequence_map4
      DevAssert( index < sizeof(prach_root_sequence_map_abc) / sizeof(prach_root_sequence_map_abc[0]) );
    } else {
      // prach_root_sequence_map points to prach_root_sequence_map0_3
      DevAssert( index < sizeof(prach_root_sequence_map_0_3) / sizeof(prach_root_sequence_map_0_3[0]) );
    }

    u = prach_root_sequence_map[index];
    LOG_D(PHY,"prach index %d => u=%d\n",index,u);
    inv_u = nr_ZC_inv[u]; // multiplicative inverse of u


    // X_u[0] stores the first ZC sequence where the root u has a non-zero number of shifts
    // for the unrestricted case X_u[0] is the first root indicated by the rootSequenceIndex

    for (k=0; k<N_ZC; k++) {
      // multiply by inverse of 2 (required since ru is exp[j 2\pi n])
      X_u[i][k] = nr_ru[(((k * (1 + (inv_u * k))) % N_ZC) * nr_ZC_inv[2]) % N_ZC];
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
    nr_ru[i].r = (int16_t)(floor(32767.0 * cos(2 * M_PI * (double)i / N_ZC)));
    nr_ru[i].i = (int16_t)(floor(32767.0 * sin(2 * M_PI * (double)i / N_ZC)));
#ifdef PRACH_DEBUG

    if (i<16)
      printf("i %d : runity %d,%d\n", i, nr_ru[i].r, nr_ru[i].i);

#endif
  }
}

