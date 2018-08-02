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

#ifndef __NR_POLAR_DEFS__H__
#define __NR_POLAR_DEFS__H__

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t nr_polar_subblock_interleaver_pattern[32] = { 0, 1, 2, 4, 3, 5, 6, 7, 8, 16, 9, 17, 10, 18, 11, 19, 12, 20, 13, 21, 14, 22, 15, 23, 24, 25, 26, 28, 27, 29, 30, 31 };

struct nrPolar_params {
	//messageType: 0=PBCH, 1=DCI, -1=UCI
	int8_t idx; //idx = messageType*messageLength;
	struct nrPolar_params *nextPtr;

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

	uint16_t *interleaving_pattern;
	uint16_t *rate_matching_pattern;
	const uint16_t *Q_0_Nminus1;
	int16_t *Q_I_N;
	int16_t *Q_F_N;
	int16_t *Q_PC_N;
	uint8_t *information_bit_pattern;
	uint16_t *channel_interleaver_pattern;

	uint8_t **crc_generator_matrix; //G_P
	uint8_t **G_N;

	//polar_encoder vectors:
	uint8_t *nr_polar_crc;
	uint8_t *nr_polar_b;
	uint8_t *nr_polar_cPrime;
	uint8_t *nr_polar_u;
	uint8_t *nr_polar_d;
} __attribute__ ((__packed__));
typedef struct nrPolar_params t_nrPolar_params;
typedef t_nrPolar_params *t_nrPolar_paramsPtr;

void polar_encoder(uint8_t *input, uint8_t *output, t_nrPolar_paramsPtr polarParams);

int8_t polar_decoder(double *input, uint8_t *output, t_nrPolar_paramsPtr polarParams,
		uint8_t listSize, double *aPrioriPayload, uint8_t pathMetricAppr);

void nr_polar_init(t_nrPolar_paramsPtr *polarParams, int8_t messageType, uint16_t messageLength);

uint8_t** nr_polar_kronecker_power_matrices(uint8_t n);

const uint16_t* nr_polar_sequence_pattern(uint8_t n);

uint32_t nr_polar_output_length(uint16_t K, uint16_t E, uint8_t n_max);

void nr_polar_channel_interleaver_pattern(uint16_t *cip, uint8_t I_BIL,
		uint16_t E);

void nr_polar_rate_matching_pattern(uint16_t *rmp, uint16_t *J, const uint8_t *P_i_,
		uint16_t K, uint16_t N, uint16_t E);

void nr_polar_rate_matching(double *input, double *output, uint16_t *rmp,
		uint16_t K, uint16_t N, uint16_t E);

void nr_polar_interleaving_pattern(uint16_t K, uint8_t I_IL, uint16_t *PI_k_);

void nr_polar_info_bit_pattern(uint8_t *ibp, int16_t *Q_I_N, int16_t *Q_F_N,
		uint16_t *J, const uint16_t *Q_0_Nminus1, uint16_t K, uint16_t N, uint16_t E,
		uint8_t n_PC);

void nr_polar_info_bit_extraction(uint8_t *input, uint8_t *output,
		uint8_t *pattern, uint16_t size);

void nr_byte2bit(uint8_t *array, uint8_t arraySize, uint8_t *bitArray);

void nr_polar_bit_insertion(uint8_t *input, uint8_t *output, uint16_t N,
		uint16_t K, int16_t *Q_I_N, int16_t *Q_PC_N, uint8_t n_PC);

void nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(uint8_t *matrix1, uint8_t **matrix2,
		uint8_t *output, uint16_t row, uint16_t col);

uint8_t ***nr_alloc_uint8_t_3D_array(uint16_t xlen, uint16_t ylen,
		uint16_t zlen);
uint8_t **nr_alloc_uint8_t_2D_array(uint16_t xlen, uint16_t ylen);
double ***nr_alloc_double_3D_array(uint16_t xlen, uint16_t ylen, uint16_t zlen);

void nr_free_uint8_t_3D_array(uint8_t ***input, uint16_t xlen, uint16_t ylen);
void nr_free_uint8_t_2D_array(uint8_t **input, uint16_t xlen);
void nr_free_double_3D_array(double ***input, uint16_t xlen, uint16_t ylen);

void updateLLR(double ***llr, uint8_t **llrU, uint8_t ***bit, uint8_t **bitU,
		uint8_t listSize, uint16_t row, uint16_t col, uint16_t xlen,
		uint8_t ylen, uint8_t approximation);
void updateBit(uint8_t ***bit, uint8_t **bitU, uint8_t listSize, uint16_t row,
		uint16_t col, uint16_t xlen, uint8_t ylen);
void updatePathMetric(double *pathMetric, double ***llr, uint8_t listSize,
		uint8_t bitValue, uint16_t row, uint8_t approximation);
void updatePathMetric2(double *pathMetric, double ***llr, uint8_t listSize,
		uint16_t row, uint8_t approximation);
void computeLLR(double ***llr, uint16_t row, uint16_t col, uint8_t i,
		uint16_t offset, uint8_t approximation);
void updateCrcChecksum(uint8_t **crcChecksum, uint8_t **crcGen,
		uint8_t listSize, uint32_t i2, uint8_t len);
void updateCrcChecksum2(uint8_t **crcChecksum, uint8_t **crcGen,
		uint8_t listSize, uint32_t i2, uint8_t len);
void nr_sort_asc_double_1D_array_ind(double *matrix, uint8_t *ind, uint8_t len);

uint8_t **crc24c_generator_matrix(uint16_t payloadSizeBits);
uint8_t **crc11_generator_matrix(uint16_t payloadSizeBits);
uint8_t **crc6_generator_matrix(uint16_t payloadSizeBits);

static inline void nr_polar_rate_matcher(uint8_t *input, unsigned char *output, uint16_t *pattern, uint16_t size) {
	for (int i=0; i<size; i++) output[i]=input[pattern[i]];
}

static inline void nr_polar_interleaver(uint8_t *input, uint8_t *output, uint16_t *pattern, uint16_t size) {
	for (int i=0; i<size; i++) output[i]=input[pattern[i]];
}

static inline void nr_polar_deinterleaver(uint8_t *input, uint8_t *output, uint16_t *pattern, uint16_t size) {
	for (int i=0; i<size; i++) output[pattern[i]]=input[i];
}

#endif
