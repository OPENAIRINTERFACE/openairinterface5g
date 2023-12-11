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

/*!\file PHY/CODING/nrPolar_tools/nr_polar_defs.h
 * \brief
 * \author Raymond Knopp, Turker Yilmaz
 * \date 2018
 * \version 0.1
 * \company EURECOM
 * \email raymond.knopp@eurecom.fr, turker.yilmaz@eurecom.fr
 * \note
 * \warning
*/

#ifndef __NR_POLAR_DEFS__H__
#define __NR_POLAR_DEFS__H__

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "PHY/CODING/nrPolar_tools/nr_polar_dci_defs.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_uci_defs.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_pbch_defs.h"
#include "PHY/CODING/coding_defs.h"
//#include "SIMULATION/TOOLS/sim.h"

#define NR_POLAR_DECODER_LISTSIZE 8 //uint8_t

#define NR_POLAR_AGGREGATION_LEVEL_1_PRIME 149 //uint16_t
#define NR_POLAR_AGGREGATION_LEVEL_2_PRIME 151 //uint16_t
#define NR_POLAR_AGGREGATION_LEVEL_4_PRIME 157 //uint16_t
#define NR_POLAR_AGGREGATION_LEVEL_8_PRIME 163 //uint16_t
#define NR_POLAR_AGGREGATION_LEVEL_16_PRIME 167 //uint16_t

static const uint8_t nr_polar_subblock_interleaver_pattern[32] = {0,1,2,4,3,5,6,7,8,16,9,17,10,18,11,19,12,20,13,21,14,22,15,23,24,25,26,28,27,29,30,31};

#define Nmax 1024
#define nmax 10

#define uint128_t __uint128_t

typedef struct decoder_node_t_s {
  struct decoder_node_t_s *left;
  struct decoder_node_t_s *right;
  int level;
  int leaf;
  int Nv;
  int first_leaf_index;
  int all_frozen;
  int16_t *alpha;
  int16_t *beta;
} decoder_node_t;

typedef struct decoder_tree_t_s {
  decoder_node_t *root;
  int num_nodes;
} decoder_tree_t;

struct nrPolar_params {
  //messageType: 0=PBCH, 1=DCI, -1=UCI

  struct nrPolar_params *nextPtr __attribute__((aligned(16)));
  bool busy;
  uint32_t idx;
  uint8_t n_max;
  uint8_t i_il;
  uint8_t i_seg;
  uint8_t n_pc;
  uint8_t n_pc_wm;
  uint8_t i_bil;
  uint16_t payloadBits;
  uint16_t encoderLength;
  uint8_t crcParityBits;
  uint8_t crcCorrectionBits;
  uint16_t K;
  uint16_t N;
  uint8_t n;
  uint32_t crcBit;

  uint16_t *interleaving_pattern;
  uint16_t *deinterleaving_pattern;
  uint16_t *rate_matching_pattern;
  const uint16_t *Q_0_Nminus1;
  int16_t *Q_I_N;
  int16_t *Q_F_N;
  int16_t *Q_PC_N;
  uint8_t *information_bit_pattern;
  uint8_t *parity_check_bit_pattern;
  uint16_t *channel_interleaver_pattern;
  //uint32_t crc_polynomial;

  const uint8_t **crc_generator_matrix; // G_P
  const uint8_t **G_N;
  fourDimArray_t *G_N_tab;
  int groupsize;
  int *rm_tab;
  uint64_t cprime_tab0[32][256];
  uint64_t cprime_tab1[32][256];
  uint64_t B_tab0[32][256];
  uint64_t B_tab1[32][256];
  uint8_t **extended_crc_generator_matrix;
  //lowercase: bits, Uppercase: Bits stored in bytes
  //polar_encoder vectors
  uint8_t *nr_polar_crc;
  uint8_t *nr_polar_aPrime;
  uint8_t *nr_polar_APrime;
  uint8_t *nr_polar_D;
  uint8_t *nr_polar_E;

  //Polar Coding vectors
  uint8_t *nr_polar_A;
  uint8_t *nr_polar_CPrime;
  uint8_t *nr_polar_B;
  uint8_t *nr_polar_U;

  decoder_tree_t tree;
} __attribute__ ((__packed__));
typedef struct nrPolar_params t_nrPolar_params;

void polar_encoder(uint32_t *input,
                   uint32_t *output,
                   int8_t messageType,
                   uint16_t messageLength,
                   uint8_t aggregation_level);

void polar_encoder_dci(uint32_t *in,
                       uint32_t *out,
                       uint16_t n_RNTI,
                       int8_t messageType,
                       uint16_t messageLength,
                       uint8_t aggregation_level);

void polar_encoder_fast(uint64_t *A,
                        void *out,
                        int32_t crcmask,
                        uint8_t ones_flag,
                        int8_t messageType,
                        uint16_t messageLength,
                        uint8_t aggregation_level);

int8_t polar_decoder(double *input,
                     uint32_t *output,
                     uint8_t listSize,
                     int8_t messageType,
                     uint16_t messageLength,
                     uint8_t aggregation_level);

uint32_t polar_decoder_int16(int16_t *input,
                             uint64_t *out,
                             uint8_t ones_flag,
                             int8_t messageType,
                             uint16_t messageLength,
                             uint8_t aggregation_level);

int8_t polar_decoder_dci(double *input,
                         uint32_t *out,
                         uint8_t listSize,
                         uint16_t n_RNTI,
                         int8_t messageType,
                         uint16_t messageLength,
                         uint8_t aggregation_level);

void generic_polar_decoder(const t_nrPolar_params *pp,
                           decoder_node_t *node);

void applyFtoleft(const t_nrPolar_params *pp,
                  decoder_node_t *node);

void applyGtoright(const t_nrPolar_params *pp,
                   decoder_node_t *node);

void computeBeta(const t_nrPolar_params *pp,
                 decoder_node_t *node);

void build_decoder_tree(t_nrPolar_params *pp);
void build_polar_tables(t_nrPolar_params *polarParams);
void init_polar_deinterleaver_table(t_nrPolar_params *polarParams);

void nr_polar_print_polarParams(void);

t_nrPolar_params *nr_polar_params (int8_t messageType,
                                   uint16_t messageLength,
                                   uint8_t aggregation_level,
				   int decoder_flag);

uint16_t nr_polar_aggregation_prime (uint8_t aggregation_level);

const uint8_t **nr_polar_kronecker_power_matrices(uint8_t n);

const uint16_t *nr_polar_sequence_pattern(uint8_t n);

/*!@fn uint32_t nr_polar_output_length(uint16_t K, uint16_t E, uint8_t n_max)
 * @brief Computes...
 * @param K Number of bits to encode (=payloadBits+crcParityBits)
 * @param E
 * @param n_max */
uint32_t nr_polar_output_length(uint16_t K,
                                uint16_t E,
                                uint8_t n_max);

void nr_polar_channel_interleaver_pattern(uint16_t *cip, const uint8_t I_BIL, const uint16_t E);

void nr_polar_rate_matching_pattern(uint16_t *rmp,
                                    uint16_t *J,
                                    const uint8_t *P_i_,
                                    uint16_t K,
                                    uint16_t N,
                                    uint16_t E);

void nr_polar_rate_matching(double *input,
                            double *output,
                            uint16_t *rmp,
                            uint16_t K,
                            uint16_t N,
                            uint16_t E);

void nr_polar_rate_matching_int16(int16_t *input,
                                  int16_t *output,
                                  const uint16_t *rmp,
                                  const uint16_t K,
                                  const uint16_t N,
                                  const uint16_t E,
                                  const uint8_t i_bil);

void nr_polar_interleaving_pattern(uint16_t K,
                                   uint8_t I_IL,
                                   uint16_t *PI_k_);

void nr_polar_info_bit_pattern(uint8_t *ibp,
                               uint8_t *pcbp,
                               int16_t *Q_I_N,
                               int16_t *Q_F_N,
                               int16_t *Q_PC_N,
                               const uint16_t *J,
                               const uint16_t *Q_0_Nminus1,
                               uint16_t K,
                               uint16_t N,
                               const uint16_t E,
                               uint8_t n_PC,
                               uint8_t n_pc_wm);

void nr_polar_info_bit_extraction(uint8_t *input,
                                  uint8_t *output,
                                  uint8_t *pattern,
                                  uint16_t size);

void nr_bit2byte_uint32_8(uint32_t *in,
                          uint16_t arraySize,
                          uint8_t *out);

void nr_byte2bit_uint8_32(uint8_t *in,
                          uint16_t arraySize,
                          uint32_t *out);

const uint8_t **crc24c_generator_matrix(uint16_t payloadSizeBits);

void nr_polar_generate_u(uint64_t *u,
                         const uint64_t *Cprime,
                         const uint8_t *information_bit_pattern,
                         const uint8_t *parity_check_bit_pattern,
                         uint16_t N,
                         uint8_t n_pc);

void nr_polar_uxG(uint64_t *D, const uint64_t *u, const fourDimArray_t *G_N_tab, uint16_t N);

void nr_polar_info_extraction_from_u(uint64_t *Cprime,
                                     const uint8_t *u,
                                     const uint8_t *information_bit_pattern,
                                     const uint8_t *parity_check_bit_pattern,
                                     uint16_t N,
                                     uint8_t n_pc);

void nr_polar_bit_insertion(uint8_t *input,
                            uint8_t *output,
                            uint16_t N,
                            uint16_t K,
                            int16_t *Q_I_N,
                            int16_t *Q_PC_N,
                            uint8_t n_PC);

void nr_matrix_multiplication_uint8_1D_uint8_2D(const uint8_t *matrix1,
                                                const uint8_t **matrix2,
                                                uint8_t *output,
                                                uint16_t row,
                                                uint16_t col);

void nr_sort_asc_double_1D_array_ind(double *matrix,
                                     uint8_t *ind,
                                     uint8_t len);

void nr_sort_asc_int16_1D_array_ind(int32_t *matrix,
                                    int *ind,
                                    int len);

void nr_free_double_2D_array(double **input, uint16_t xlen);

#ifndef __cplusplus
void updateLLR(uint8_t listSize,
	       uint16_t row,
	       uint16_t col,
	        uint16_t xlen,
	       uint8_t ylen,
	       int zlen,
	       double  llr[xlen][ylen][zlen],
	       uint8_t llrU[xlen][ylen],
	       uint8_t bit[xlen][ylen][zlen],
	       uint8_t bitU[xlen][ylen]
	       );
void updatePathMetric(double *pathMetric,
		      uint8_t listSize,
		      uint8_t bitValue,
		      uint16_t row,
		      int xlen,
		      int ylen,
		      int zlen,
		      double llr[xlen][ylen][zlen]
		      );
void updatePathMetric2(double *pathMetric,
		       uint8_t listSize,
		       uint16_t row,
		       int xlen,
		       int ylen,
		       int zlen,
		       double llr[xlen][ylen][zlen]);
#endif
//Also nr_polar_rate_matcher
static inline void nr_polar_interleaver(uint8_t *input,
                                        uint8_t *output,
                                        uint16_t *pattern,
                                        uint16_t size)
{
  for (int i=0; i<size; i++) output[i]=input[pattern[i]];
}

static inline void nr_polar_deinterleaver(uint8_t *input,
										  uint8_t *output,
										  uint16_t *pattern,
										  uint16_t size)
{
	for (int i=0; i<size; i++) output[pattern[i]]=input[i];
}
void delete_decoder_tree(t_nrPolar_params *);

extern pthread_mutex_t PolarListMutex;
#define polarReturn                          \
pthread_mutex_lock(&PolarListMutex);            \
polarParams->busy=false;                         \
pthread_mutex_unlock(&PolarListMutex);          \
return

#endif
