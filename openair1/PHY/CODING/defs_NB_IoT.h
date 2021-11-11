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

/* file: PHY/CODING/defs_NB_IoT.h
   purpose: Top-level definitions, data types and function prototypes for openairinterface coding blocks for NB-IoT
   author: matthieu.kanj@b-com.com, raymond.knopp@eurecom.fr, michele.paffetti@studio.unibo.it
   date: 29.06.2017
*/

#ifndef OPENAIR1_PHY_CODING_DEFS_NB_IOT_H_
#define OPENAIR1_PHY_CODING_DEFS_NB_IOT_H_

#include <stdint.h>  // for uint8/16/32_t

#define CRC24_A_NB_IoT 0
#define CRC24_B_NB_IoT 1
#define CRC16_NB_IoT 2
#define CRC8_NB_IoT 3

//#define MAX_TURBO_ITERATIONS_MBSFN 8  // no MBSFN
#define MAX_TURBO_ITERATIONS_NB_IoT 4

#define LTE_NULL_NB_IoT 2  // defined also in PHY/LTE_TRANSPORT/defs_NB_IoT.h

/** \fn uint32_t sub_block_interleaving_cc(uint32_t D, uint8_t *d,uint8_t *w)
\brief This is the subblock interleaving algorithm for convolutionally coded blocks from 36-212 (Release 13.4, 2017).
This function takes the d-sequence and generates the w-sequence.  The nu-sequence from 36-212 is implicit.
\param D Number of input bits
\param d Pointer to input (d-sequence, convolutional code output)
\param w Pointer to output (w-sequence, interleaver output)
\returns Interleaving matrix cardinality (\f$K_{\pi}\f$  from 36-212)
*/
uint32_t sub_block_interleaving_cc_NB_IoT(uint32_t D, uint8_t *d,uint8_t *w);

/**
\brief This is the NB-IoT rate matching algorithm for Convolutionally-coded channels (e.g. BCH,DCI,UCI).  It is taken directly from 36-212 (Rel 8 8.6, 2009-03), pages 16-18 )
\param RCC R^CC_subblock from subblock interleaver (number of rows in interleaving matrix) for up to 8 segments
\param E Number of coded channel bits
\param w This is a pointer to the w-sequence (second interleaver output)
\param e This is a pointer to the e-sequence (rate matching output, channel input/output bits)
\returns \f$E\f$, the number of coded bits per segment */

uint32_t lte_rate_matching_cc_NB_IoT(uint32_t RCC,      // RRC = 2
				     				 uint16_t E,        // E = 1600
				     				 uint8_t *w,	// length
				    				 uint8_t *e);	// length 1600

/** \fn void ccodelte_encode(int32_t numbits,uint8_t add_crc, uint8_t *inPtr,uint8_t *outPtr,uint16_t rnti)
\brief This function implements the LTE convolutional code of rate 1/3
  with a constraint length of 7 bits. The inputs are bit packed in octets
(from MSB to LSB). Trellis tail-biting is included here.
@param numbits Number of bits to encode
@param add_crc crc to be appended (8 bits) if add_crc = 1
@param inPtr Pointer to input buffer
@param outPtr Pointer to output buffer
@param rnti RNTI for CRC scrambling
*/

void ccode_encode_NB_IoT (int32_t numbits,
						  uint8_t add_crc,
						  uint8_t *inPtr,
						  uint8_t *outPtr,
						  uint16_t rnti);

/*!\fn void ccodelte_init(void)
\brief This function initializes the generator polynomials for an LTE convolutional code.*/
void ccodelte_init_NB_IoT(void);

/*!\fn void crcTableInit(void)
\brief This function initializes the different crc tables.*/
void crcTableInit_NB_IoT (void);


/*!\fn uint32_t crc24a(uint8_t *inPtr, int32_t bitlen)
\brief This computes a 24-bit crc ('a' variant for overall transport block)
based on 3GPP UMTS/LTE specifications.
@param inPtr Pointer to input byte stream
@param bitlen length of inputs in bits
*/
uint32_t crc24a_NB_IoT (uint8_t *inPtr, int32_t bitlen);

/*!\fn uint32_t crc24b(uint8_t *inPtr, int32_t bitlen)
\brief This computes a 24-bit crc ('b' variant for transport-block segments)
based on 3GPP UMTS/LTE specifications.
@param inPtr Pointer to input byte stream
@param bitlen length of inputs in bits
*/
uint32_t crc24b_NB_IoT (uint8_t *inPtr, int32_t bitlen);

/*!\fn uint32_t crc16(uint8_t *inPtr, int32_t bitlen)
\brief This computes a 16-bit crc based on 3GPP UMTS specifications.
@param inPtr Pointer to input byte stream
@param bitlen length of inputs in bits*/
uint32_t crc16_NB_IoT (uint8_t *inPtr, int32_t bitlen);

/*!\fn uint32_t crc8(uint8_t *inPtr, int32_t bitlen)
\brief This computes a 8-bit crc based on 3GPP UMTS specifications.
@param inPtr Pointer to input byte stream
@param bitlen length of inputs in bits*/
uint32_t crc8_NB_IoT  (uint8_t *inPtr, int32_t bitlen);






uint32_t crcbit_NB_IoT (uint8_t * ,
                 		int32_t,
                 		uint32_t);


/*!\fn void phy_viterbi_lte_sse2(int8_t *y, uint8_t *decoded_bytes, uint16_t n)
\brief This routine performs a SIMD optmized Viterbi decoder for the LTE 64-state tail-biting convolutional code.
@param y Pointer to soft input (coded on 8-bits but should be limited to 4-bit precision to avoid overflow)
@param decoded_bytes Pointer to decoded output
@param n Length of input/trellis depth in bits*/
//void phy_viterbi_lte_sse2(int8_t *y,uint8_t *decoded_bytes,uint16_t n);
void phy_viterbi_lte_sse2_NB_IoT(int8_t *y,uint8_t *decoded_bytes,uint16_t n);

/** \fn void sub_block_deinterleaving_cc(uint32_t D, int8_t *d,int8_t *w)
\brief This is the subblock deinterleaving algorithm for convolutionally-coded data from 36-212 (Release 8, 8.6 2009-03), pages 15-16.
This function takes the w-sequence and generates the d-sequence.  The nu-sequence from 36-212 is implicit.
\param D Number of input bits
\param d Pointer to output (d-sequence, turbo code output)
\param w Pointer to input (w-sequence, interleaver output)
*/
void sub_block_deinterleaving_cc_NB_IoT(uint32_t D,int8_t *d,int8_t *w);


/*
\brief This is the LTE rate matching algorithm for Convolutionally-coded channels (e.g. BCH,DCI,UCI).  It is taken directly from 36-212 (Rel 8 8.6, 2009-03), pages 16-18 )
\param RCC R^CC_subblock from subblock interleaver (number of rows in interleaving matrix)
\param E This the number of coded bits allocated for channel
\param w This is a pointer to the soft w-sequence (second interleaver output) with soft-combined outputs from successive HARQ rounds
\param dummy_w This is the first row of the interleaver matrix for identifying/discarding the "LTE-NULL" positions
\param soft_input This is a pointer to the soft channel output
\returns \f$E\f$, the number of coded bits per segment
*/
void lte_rate_matching_cc_rx_NB_IoT(uint32_t RCC,
                             		uint16_t E,
                             		int8_t *w,
                             		uint8_t *dummy_w,
                             		int8_t *soft_input);

/** \fn generate_dummy_w_cc(uint32_t D, uint8_t *w)
\brief This function generates a dummy interleaved sequence (first row) for receiver (convolutionally-coded data), in order to identify the NULL positions used to make the matrix complete.
\param D Number of systematic bits plus 4 (plus 4 for termination)
\param w This is the dummy sequence (first row), it will contain zeros and at most 31 "LTE_NULL" values
\returns Interleaving matrix cardinality (\f$K_{\pi}\f$ from 36-212)
*/
uint32_t generate_dummy_w_cc_NB_IoT(uint32_t D, uint8_t *w);

/** \fn lte_segmentation(uint8_t *input_buffer,
              uint8_t **output_buffers,
            uint32_t B,
            uint32_t *C,
            uint32_t *Cplus,
            uint32_t *Cminus,
            uint32_t *Kplus,
            uint32_t *Kminus,
            uint32_t *F)
\brief This function implements the LTE transport block segmentation algorithm from 36-212, V8.6 2009-03.
@param input_buffer
@param output_buffers
@param B
@param C
@param Cplus
@param Cminus
@param Kplus
@param Kminus
@param F
*/
int32_t lte_segmentation_NB_IoT(uint8_t *input_buffer,
                         		uint8_t **output_buffers,
                         		uint32_t B,
                         		uint32_t *C,
                         		uint32_t *Cplus,
                         		uint32_t *Cminus,
                         		uint32_t *Kplus,
                         		uint32_t *Kminus,
                         		uint32_t *F);

/** \fn void sub_block_deinterleaving_turbo(uint32_t D, int16_t *d,int16_t *w)
\brief This is the subblock deinterleaving algorithm from 36-212 (Release 8, 8.6 2009-03), pages 15-16.
This function takes the w-sequence and generates the d-sequence.  The nu-sequence from 36-212 is implicit.
\param D Number of systematic bits plus 4 (plus 4 for termination)
\param d Pointer to output (d-sequence, turbo code output)
\param w Pointer to input (w-sequence, interleaver output)
*/
//*****************void sub_block_deinterleaving_turbo(uint32_t D, int16_t *d,int16_t *w);

/**
\brief This is the LTE rate matching algorithm for Turbo-coded channels (e.g. DLSCH,ULSCH).  It is taken directly from 36-212 (Rel 8 8.6, 2009-03), pages 16-18 )
\param RTC R^TC_subblock from subblock interleaver (number of rows in interleaving matrix)
\param G This the number of coded transport bits allocated in sub-frame
\param w This is a pointer to the soft w-sequence (second interleaver output) with soft-combined outputs from successive HARQ rounds
\param dummy_w This is the first row of the interleaver matrix for identifying/discarding the "LTE-NULL" positions
\param soft_input This is a pointer to the soft channel output
\param C Number of segments (codewords) in the sub-frame
\param Nsoft Total number of soft bits (from UE capabilities in 36-306)
\param Mdlharq Number of HARQ rounds
\param Kmimo MIMO capability for this DLSCH (0 = no MIMO)
\param rvidx round index (0-3)
\param clear 1 means clear soft buffer (start of HARQ round)
\param Qm modulation order (2,4,6)
\param Nl number of layers (1,2)
\param r segment number
\param E_out the number of coded bits per segment
\returns 0 on success, -1 on failure
*/

// int lte_rate_matching_turbo_rx(uint32_t RTC,
//                                uint32_t G,
//                                int16_t *w,
//                                uint8_t *dummy_w,
//                                int16_t *soft_input,
//                                uint8_t C,
//                                uint32_t Nsoft,
//                                uint8_t Mdlharq,
//                                uint8_t Kmimo,
//                                uint8_t rvidx,
//                                uint8_t clear,
//                                uint8_t Qm,
//                                uint8_t Nl,
//                                uint8_t r,
//                                uint32_t *E_out);

// uint32_t lte_rate_matching_turbo_rx_abs(uint32_t RTC,
//                                         uint32_t G,
//                                         double *w,
//                                         uint8_t *dummy_w,
//                                         double *soft_input,
//                                         uint8_t C,
//                                         uint32_t Nsoft,
//                                         uint8_t Mdlharq,
//                                         uint8_t Kmimo,
//                                         uint8_t rvidx,
//                                         uint8_t clear,
//                                         uint8_t Qm,
//                                         uint8_t Nl,
//                                         uint8_t r,
//                                         uint32_t *E_out);

void ccode_encode_npdsch_NB_IoT (int32_t   numbits,
								 uint8_t   *inPtr,
								 uint8_t   *outPtr,
								 uint32_t  crc);

#endif /* OPENAIR1_PHY_CODING_DEFS_NB_IOT_H_ */
