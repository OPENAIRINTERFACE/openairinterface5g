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

/* \file config_ue.c
 * \brief common utility functions for NR (gNB and UE)
 * \author R. Knopp,
 * \date 2019
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */

#ifndef __COMMON_UTILS_NR_NR_COMMON__H__
#define __COMMON_UTILS_NR_NR_COMMON__H__

#include <stdint.h>
#include "assertions.h"
#include "PHY/defs_common.h"

typedef struct nr_bandentry_s {
  int16_t band;
  uint64_t ul_min;
  uint64_t ul_max;
  uint64_t dl_min;
  uint64_t dl_max;
  uint64_t step_size;
  uint64_t N_OFFs_DL;
  uint8_t deltaf_raster;
} nr_bandentry_t;

extern const nr_bandentry_t nr_bandtable[];
static inline int get_num_dmrs(uint16_t dmrs_mask ) {

  int num_dmrs=0;

  for (int i=0;i<16;i++) num_dmrs+=((dmrs_mask>>i)&1);
  return(num_dmrs);
}


int get_SLIV(uint8_t S, uint8_t L);
void get_coreset_rballoc(uint8_t *FreqDomainResource,int *n_rb,int *rb_offset);
uint16_t config_bandwidth(int mu, int nb_rb, int nr_band);
int get_nr_table_idx(int nr_bandP, uint8_t scs_index);
int32_t get_delta_duplex(int nr_bandP, uint8_t scs_index);
frame_type_t get_frame_type(uint16_t nr_bandP, uint8_t scs_index);
uint16_t get_band(uint64_t downlink_frequency, int32_t delta_duplex);
int NRRIV2BW(int locationAndBandwidth,int N_RB);
int NRRIV2PRBOFFSET(int locationAndBandwidth,int N_RB);
int PRBalloc_to_locationandbandwidth0(int NPRB,int RBstart,int BWPsize);
int PRBalloc_to_locationandbandwidth(int NPRB,int RBstart);
int get_subband_size(int NPRB,int size);
void SLIV2SL(int SLIV,int *S,int *L);
int get_dmrs_port(int nl, uint16_t dmrs_ports);
uint16_t SL_to_bitmap(int startSymbolIndex, int nrOfSymbols);
int get_nb_periods_per_frame(uint8_t tdd_period);
int get_supported_band_index(int scs, int band, int n_rbs);
long rrc_get_max_nr_csrs(uint8_t max_rbs, long b_SRS);

#define CEILIDIV(a,b) ((a+b-1)/b)
#define ROUNDIDIV(a,b) (((a<<1)+b)/(b<<1))

#define cmax(a,b)  ((a>b) ? (a) : (b))
#define cmax3(a,b,c) ((cmax(a,b)>c) ? (cmax(a,b)) : (c))
#define cmin(a,b)  ((a<b) ? (a) : (b))

#ifdef __cplusplus
#ifdef min
#undef min
#undef max
#endif
#else
#define max(a,b) cmax(a,b)
#define min(a,b) cmin(a,b)
#endif

#endif
