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

#define NR_POLAR_CRC_ERROR_CORRECTION_BITS 3

#ifndef __NR_POLAR_DEFS__H__
#define __NR_POLAR_DEFS__H__

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "PHY/TOOLS/time_meas.h"

static const uint8_t nr_polar_subblock_interleaver_pattern[32] = { 0, 1, 2, 4, 3, 5, 6, 7, 8, 16, 9, 17, 10, 18, 11, 19, 12, 20, 13, 21, 14, 22, 15, 23, 24, 25, 26, 28, 27, 29, 30, 31 };


#define Nmax 1024
#define nmax 10

typedef struct decoder_list_s {
  
  uint8_t bit[1+nmax][Nmax];
  double llr[1+nmax][Nmax]; 
  uint8_t crcChecksum[24]; 
  double pathMetric; 

} decoder_list_t;

typedef struct decoder_list_int8_s {
  
  uint8_t bit[1+nmax][Nmax] __attribute__((aligned(32)));
  int16_t llr[1+nmax][Nmax]__attribute__((aligned(32))); 
  uint8_t crcChecksum[24]; 
  int32_t pathMetric; 

} decoder_list_int8_t;



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
  uint32_t crc_polynomial;
  
  uint8_t **crc_generator_matrix; //G_P
  uint8_t **G_N;
  uint32_t* crc256Table;
  uint8_t **extended_crc_generator_matrix;

  //polar_encoder vectors:
  uint8_t *nr_polar_crc;
  uint8_t *nr_polar_b;
  uint8_t *nr_polar_cPrime;
  uint8_t *nr_polar_u;
  uint8_t *nr_polar_d;
  
  void (*decoder_kernel)(struct nrPolar_params *,decoder_list_int8_t **);
  decoder_tree_t tree;
} __attribute__ ((__packed__));
typedef struct nrPolar_params t_nrPolar_params;





void polar_encoder(uint8_t *input, uint8_t *output, t_nrPolar_params* polarParams);

void nr_polar_kernal_operation(uint8_t *u, uint8_t *d, uint16_t N);

void generic_polar_decoder(t_nrPolar_params *,decoder_node_t *);

int8_t polar_decoder(double *input, uint8_t *output, t_nrPolar_params *polarParams,
		     uint8_t listSize, double *aPrioriPayload, uint8_t pathMetricAppr,
		     time_stats_t *init,
		     time_stats_t *polar_rate_matching,
		     time_stats_t *decoding,
		     time_stats_t *bit_extraction,
		     time_stats_t *deinterleaving,
		     time_stats_t *sorting,
		     time_stats_t *path_metric,
		     time_stats_t *update_LLR);

int8_t polar_decoder_int8(int16_t *input,
			  uint8_t *output,
			  t_nrPolar_params *polarParams,
			  uint8_t listSize,
			  time_stats_t *init,
			  time_stats_t *polar_rate_matching,
			  time_stats_t *decoding,
			  time_stats_t *bit_extraction,
			  time_stats_t *deinterleaving,
			  time_stats_t *sorting,
			  time_stats_t *path_metric,
			  time_stats_t *update_LLR,
			  int generate_optim_code);

int8_t polar_decoder_int8_new(int16_t *input,
                          uint8_t *output,
                          t_nrPolar_params *polarParams,
                          uint8_t listSize,
                          time_stats_t *init,
                          time_stats_t *polar_rate_matching,
                          time_stats_t *decoding,
                          time_stats_t *bit_extraction,
                          time_stats_t *deinterleaving,
                          time_stats_t *sorting,
                          time_stats_t *path_metric,
                          time_stats_t *update_LLR,
                          int generate_optim_code);


void nr_polar_llrtableinit(void);

void nr_polar_init(t_nrPolar_params* polarParams, int messageType);

uint8_t** nr_polar_kronecker_power_matrices(uint8_t n);

const uint16_t* nr_polar_sequence_pattern(uint8_t n);

uint32_t nr_polar_output_length(uint16_t K, uint16_t E, uint8_t n_max);

void nr_polar_channel_interleaver_pattern(uint16_t *cip, uint8_t I_BIL,
		uint16_t E);

void nr_polar_rate_matching_pattern(uint16_t *rmp, uint16_t *J, const uint8_t *P_i_,
		uint16_t K, uint16_t N, uint16_t E);

void nr_polar_rate_matching(double *input, double *output, uint16_t *rmp,
		uint16_t K, uint16_t N, uint16_t E);

void nr_polar_rate_matching_int8(int16_t *input, int16_t *output, uint16_t *rmp, 
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
double **nr_alloc_double_2D_array(uint16_t xlen, uint16_t ylen);

void nr_free_uint8_t_3D_array(uint8_t ***input, uint16_t xlen, uint16_t ylen);
void nr_free_uint8_t_2D_array(uint8_t **input, uint16_t xlen);
void nr_free_double_2D_array(double **input, uint16_t xlen);

void updateLLR(decoder_list_t **dlist,uint8_t **llrU, uint8_t **bitU,
	       uint8_t listSize, uint16_t row, uint16_t col, uint16_t xlen, uint8_t ylen, uint8_t approximation);

void updateLLR_int8(decoder_list_int8_t **dlist,uint8_t **llrU, uint8_t **bitU,
		    uint8_t listSize, uint16_t row, uint16_t col, uint16_t xlen, uint8_t ylen,int generate_optim_code,FILE *fd);

void updateBit(decoder_list_t **dlist, uint8_t **bitU, uint8_t listSize, uint16_t row,
	       uint16_t col, uint16_t xlen, uint8_t ylen);

void updateBit_int8(decoder_list_int8_t **dlist, uint8_t **bitU, uint8_t listSize, uint16_t row,
		    uint16_t col, uint16_t xlen, uint8_t ylen,int generate_optim_code,FILE *fd);

void updatePathMetric(decoder_list_t **dlist,uint8_t listSize, uint8_t bitValue,
		       uint16_t row, uint8_t approximation);

void updatePathMetric0_int8(decoder_list_int8_t **dlist,uint8_t listSize, uint16_t row,int generate_optim_code,FILE *fd);

void updatePathMetric2(decoder_list_t **dlist, uint8_t listSize, uint16_t row, uint8_t appr);

void updatePathMetric2_int8(decoder_list_int8_t **dlist, uint8_t listSize, uint16_t row,int generate_optim_code,FILE *fd);

void updateCrcChecksum(decoder_list_t **dlist, uint8_t **crcGen,
		       uint8_t listSize, uint32_t i2, uint8_t len);

void updateCrcChecksum_int8(decoder_list_int8_t **dlist, uint8_t **crcGen,
			    uint8_t listSize, uint32_t i2, uint8_t len,int generate_optim_code,FILE *fd);

void updateCrcChecksum2(decoder_list_t **dlist, uint8_t **crcGen,
			uint8_t listSize, uint32_t i2, uint8_t len);

void updateCrcChecksum2_int8(decoder_list_int8_t **dlist, uint8_t **crcGen,
			     uint8_t listSize, uint32_t i2, uint8_t len,int generate_optim_code,FILE *fd);


void nr_sort_asc_double_1D_array_ind(double *matrix, int *ind, int len);

void nr_sort_asc_int16_1D_array_ind(int32_t *matrix, int *ind, int len);

uint8_t **crc24c_generator_matrix(uint16_t payloadSizeBits);
uint8_t **crc11_generator_matrix(uint16_t payloadSizeBits);
uint8_t **crc6_generator_matrix(uint16_t payloadSizeBits);

void crcTable256Init (uint32_t poly, uint32_t* crc256Table);
void nr_crc_computation(uint8_t* input, uint8_t* output, uint16_t payloadBits, uint16_t crcParityBits, uint32_t* crc256Table);
unsigned int crcbit (unsigned char* inputptr, int octetlen, uint32_t poly);
unsigned int crcPayload(unsigned char * inptr, int bitlen, uint32_t* crc256Table);

static inline void nr_polar_rate_matcher(uint8_t *input, unsigned char *output, uint16_t *pattern, uint16_t size) {
	for (int i=0; i<size; i++) output[i]=input[pattern[i]];
}

static inline void nr_polar_interleaver(uint8_t *input, uint8_t *output, uint16_t *pattern, uint16_t size) {
	for (int i=0; i<size; i++) output[i]=input[pattern[i]];
}

static inline void nr_polar_deinterleaver(uint8_t *input, uint8_t *output, uint16_t *pattern, uint16_t size) {
	for (int i=0; i<size; i++) output[pattern[i]]=input[i];
}

void polar_decoder_K56_N512_E864(t_nrPolar_params *polarParams,decoder_list_int8_t **sorted_list);

void build_decoder_tree(t_nrPolar_params *pp);

#define updateLLR_int8_A(dlist,listSize,col,row,offset) {for (int i=0;i<listSize;i++) {if (dlist[(i)]->bit[(col)][(row)-(offset)]==0) dlist[(i)]->llr[(col)][(row)] =  dlist[(i)]->llr[(col)+1][(row)-(offset)] + dlist[(i)]->llr[(col)+1][(row)]; else dlist[(i)]->llr[(col)][(row)] = -dlist[(i)]->llr[(col)+1][(row)-(offset)] + dlist[(i)]->llr[(col)+1][(row)];if(dlist[(i)]->llr[col][row]>127 || dlist[i]->llr[col][row]<-128) printf("dlist[%d]->llr[%d][%d] = %d> 8bit\n",i,col,row,dlist[i]->llr[col][row]);}}

#define updateBit_int8_A(dlist,listSize,col,row) {for (int i=0;i<listSize;i++) {dlist[(i)]->bit[(col)][(row)] = dlist[(i)]->bit[(col)-1][(row)];}}
#define updateBit_int8_B(dlist,listSize,col,row,offset) {for (int i=0;i<listSize;i++) {dlist[(i)]->bit[(col)][(row)] = dlist[(i)]->bit[(col)-1][(row)]^dlist[(i)]->bit[(col)-1][(row)+(offset)];}}

#define updatePathMetric0_int8_A(dlist,listSize,row,mask,absllr) {for (int i=0;i<listSize;i++) { mask=dlist[i]->llr[0][row]>>15;if(mask!=0){absllr=(dlist[i]->llr[0][row]+mask)^mask;dlist[i]->pathMetric+=absllr;}}}

#define updatePathMetric2_int8_A(dlist,listSize,row)    {for (int i=0;i<listSize;i++) {{dlist[i+listSize]->pathMetric = dlist[i]->pathMetric;if (dlist[i]->llr[0][row]<0) dlist[i]->pathMetric-=dlist[i]->llr[0][row];else dlist[i+listSize]->pathMetric += dlist[i]->llr[0][row];}}}

#define updateCrcChecksum_int8_A(dlist,listSize,crcGen,i2,len) {for (int i=0;i<listSize;i++){for (uint8_t j = 0; j < len; j++) dlist[i]->crcChecksum[j] = (dlist[i]->crcChecksum[j]^crcGen[i2][j]);}}

#define updateCrcChecksum2_int8_A(dlist,listSize,crcGen,i2,len) {for (int i=0;i<listSize;i++){for (uint8_t j = 0; j < len; j++) dlist[i+listSize]->crcChecksum[j]=dlist[i]->crcChecksum[j]^crcGen[i2][j];}}

extern int16_t llrtab[256][256];

inline void computeLLR_int8(decoder_list_int8_t **dlist,int listSize, uint16_t row, uint16_t col, 
			    uint16_t offset) __attribute__((always_inline));
inline void computeLLR_int8(decoder_list_int8_t **dlist,int listSize, uint16_t row, uint16_t col, 
			    uint16_t offset) {

  int16_t a;
  int16_t b;
  
  int16_t absA,absB;
  int16_t maska,maskb;
  int16_t minabs;
  //	int16_t **llr=dlist[i]->llr;
  
  
  for (int i=0;i<listSize;i++) {
    a = dlist[i]->llr[col + 1][row];   
    b = dlist[i]->llr[col+1][row + offset];
    
    //    printf("LLR: a %d, b %d\n",a,b);
    maska = a>>15;
    maskb = b>>15;
    absA = (a+maska)^maska;
    absB = (b+maskb)^maskb;
    //	printf("LLR: absA %d, absB %d\n",absA,absB);
    minabs = absA<absB ? absA : absB;
    if ((maska^maskb) == 0)
      dlist[i]->llr[col][row] = minabs;
    else 
      dlist[i]->llr[col][row] = -minabs;
    //	printf("LLR (a %d, b %d): llr[%d][%d] %d\n",a,b,col,row,llr[col][row]);
    
    //dlist[i]->llr[col][row] = llrtab[a+128][b+128];
    //    printf("newLLR [%d,%d]: %d\n",col,row,dlist[i]->llr[col][row]);
  }
}

#define decoder_int8_A(sorted_dlist,currentListSize,polarParams) {for (int i = 0; i < currentListSize; i++) { \
           for (int k = 0; k < (polarParams->n+1); k++) { \
	    memcpy((void*)&sorted_dlist[i+currentListSize]->bit[k][0],\
		   (void*)&sorted_dlist[i]->bit[k][0],\
		   sizeof(uint8_t)*polarParams->N);\
	    memcpy((void*)&sorted_dlist[i+currentListSize]->llr[k][0],\
		   (void*)&sorted_dlist[i]->llr[k][0],\
		   sizeof(int16_t)*polarParams->N);}}}

#define decoder_int8_B(sorted_dlist,currentBit,currentListSize) {for (int i = 0; i < currentListSize; i++) {sorted_dlist[i]->bit[0][currentBit]=0;sorted_dlist[i+currentListSize]->bit[0][currentBit]=1;}}

void inline decoder_int8_C(decoder_list_int8_t *sorted_dlist[],
			   t_nrPolar_params *polarParams,
			   int currentBit,
			   int currentListSize,
			   int listSize) {

  int32_t pathMetric[2*listSize];
  decoder_list_int8_t *temp_dlist[2*listSize];
  
  int listIndex[2*listSize];
  int listIndex2[2*listSize];
 
  for (int i = 0; i < currentListSize; i++) { 
    listIndex[i]=i;
    pathMetric[i] = sorted_dlist[i]->pathMetric;
  }
  nr_sort_asc_int16_1D_array_ind(pathMetric, listIndex, currentListSize);
  for (int i=0;i<currentListSize;i++) {
    listIndex2[listIndex[i]] = i;
  }
  
  // copy the llr/bit arrays that are needed
  for (int i = 0; i < listSize; i++) {
    //	  printf("listIndex[%d] %d\n",i,listIndex[i]);
    if ((listIndex2[i+listSize]<listSize) && (listIndex2[i]<listSize)) { // both '0' and '1' path metrics are to be kept
      // do memcpy of LLR and Bit arrays
      
      for (int k = 0; k < (polarParams->n+1); k++) {
	
	memcpy((void*)&sorted_dlist[i+listSize]->bit[k][0],
	       (void*)&sorted_dlist[i]->bit[k][0],
	       sizeof(uint8_t)*polarParams->N);
	memcpy((void*)&sorted_dlist[i+listSize]->llr[k][0],
	       (void*)&sorted_dlist[i]->llr[k][0],
	       sizeof(int16_t)*polarParams->N);
      }
      sorted_dlist[i]->bit[0][currentBit]=0;
      sorted_dlist[i+listSize]->bit[0][currentBit]=1;
    }
    else if (listIndex2[i+listSize]<listSize) { // only '1' path metric is to be kept
      // just change the current bit from '0' to '1'
      
      for (int k = 0; k < (polarParams->n+1); k++) {
	memcpy((void*)&sorted_dlist[i+listSize]->bit[k][0],
	       (void*)&sorted_dlist[i]->bit[k][0],
	       sizeof(uint8_t)*polarParams->N);
	memcpy((void*)&sorted_dlist[i+listSize]->llr[k][0],
	       (void*)&sorted_dlist[i]->llr[k][0],
	       sizeof(int16_t)*polarParams->N); 
      }
      sorted_dlist[i+listSize]->bit[0][currentBit]=1;
      
      /*
	decoder_list_t *tmp = sorted_dlist[i+listSize];
	sorted_dlist[i+listSize] = sorted_dlist[i];
	sorted_dlist[i+listSize]->pathMetric = tmp->pathMetric;
	sorted_dlist[i+listSize]->bit[0][currentBit]=1;
	memcpy((void*)&sorted_dlist[i+listSize]->crcChecksum[0],
	(void*)&tmp->crcChecksum[0],
	24*sizeof(uint8_t));*/
      
    }
  }
  
 
  
  for (int i = 0; i < 2*listSize; i++) {
    temp_dlist[i] = sorted_dlist[i];
  }
  for (int i = 0; i < 2*listSize; i++) {
    //		  printf("i %d => %d\n",i,listIndex[i]);
    sorted_dlist[i] = temp_dlist[listIndex[i]];
  }
}
#endif
