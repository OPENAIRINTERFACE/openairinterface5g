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

/*! \file ue_power.c
* \brief Routines to compute UE TX power according to 36.213
* \author R. Knopp, F. Kaltenberger
* \date 2020
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
* \note
* \warning
*/

#include <stdint.h>
#include <stdio.h>
#include "PHY/defs_eNB.h"
#include "PHY/TOOLS/dB_routines.h"

extern int16_t hundred_times_delta_TF[100];
extern uint16_t hundred_times_log10_NPRB[100];

int16_t estimate_ue_tx_power(int norm,uint32_t tbs, uint32_t nb_rb, uint8_t control_only, int ncp, uint8_t use_srs)
{

  /// The payload + CRC size in bits, "B"
  uint32_t B;
  /// Number of code segments
  uint32_t C;
  /// Number of "small" code segments
  uint32_t Cminus;
  /// Number of "large" code segments
  uint32_t Cplus;
  /// Number of bits in "small" code segments (<6144)
  uint32_t Kminus;
  /// Number of bits in "large" code segments (<6144)
  uint32_t Kplus;
  /// Total number of bits across all segments
  uint32_t sumKr;
  /// Number of "Filler" bits
  uint32_t F;
  // num resource elements
  uint32_t num_re=0.0;
  // num symbols
  uint32_t num_symb=0.0;
  /// effective spectral efficiency of the PUSCH
  uint32_t MPR_x100=0;
  /// beta_offset
  uint16_t beta_offset_pusch_x8=8;
  /// delta mcs
  float delta_mcs=0.0;
  /// bandwidth factor
  float bw_factor=0.0;

  B= tbs+24;
  lte_segmentation(NULL,
                   NULL,
                   B,
                   &C,
                   &Cplus,
                   &Cminus,
                   &Kplus,
                   &Kminus,
                   &F);


  sumKr = Cminus*Kminus + Cplus*Kplus;
  num_symb = 12-(ncp<<1)-(use_srs==0?0:1);
  num_re = num_symb * nb_rb * 12;

  if (num_re == 0)
    return(0);

  MPR_x100 = 100*sumKr/num_re;

  if (control_only == 1 )
    beta_offset_pusch_x8=8; // fixme

  //(beta_offset_pusch_x8=ue->ulsch[eNB_id]->harq_processes[harq_pid]->control_only == 1) ? ue->ulsch[eNB_id]->beta_offset_cqi_times8:8;

  // if deltamcs_enabledm
  delta_mcs = ((hundred_times_delta_TF[MPR_x100/6]+10*dB_fixed_times10((beta_offset_pusch_x8)>>3))/100.0);
                                                                                                                 
  bw_factor = (norm!=0) ? 0 : (hundred_times_log10_NPRB[nb_rb-1]/100.0);
  LOG_D(PHY,"estimated ue tx power %d (num_re %d, sumKr %d, mpr_x100 %d, delta_mcs %f, bw_factor %f)\n",
         (int16_t)ceil(delta_mcs + bw_factor), num_re, sumKr, MPR_x100, delta_mcs, bw_factor);
  return (int16_t)ceil(delta_mcs + bw_factor);

}
    
