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

/*!\file PHY/CODING/nrPolar_tools/nr_polar_encoder.c
 * \brief
 * \author Turker Yilmaz
 * \date 2018
 * \version 0.1
 * \company EURECOM
 * \email turker.yilmaz@eurecom.fr
 * \note
 * \warning
*/

#define DEBUG_POLAR_ENCODER_DCI

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

void polar_encoder(uint32_t *in,
				   uint32_t *out,
				   t_nrPolar_paramsPtr polarParams)
{
	if (polarParams->idx == 0){//PBCH
		nr_bit2byte_uint32_8_t(in, polarParams->payloadBits, polarParams->nr_polar_A);
		/*
		 * Bytewise operations
		 */
		//Calculate CRC.
		nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_A,
													   polarParams->crc_generator_matrix,
													   polarParams->nr_polar_crc,
													   polarParams->payloadBits,
													   polarParams->crcParityBits);
		for (uint8_t i = 0; i < polarParams->crcParityBits; i++)
			polarParams->nr_polar_crc[i] = (polarParams->nr_polar_crc[i] % 2);

		//Attach CRC to the Transport Block. (a to b)
		for (uint16_t i = 0; i < polarParams->payloadBits; i++)
			polarParams->nr_polar_B[i] = polarParams->nr_polar_A[i];
		for (uint16_t i = polarParams->payloadBits; i < polarParams->K; i++)
			polarParams->nr_polar_B[i]= polarParams->nr_polar_crc[i-(polarParams->payloadBits)];
	} else { //UCI

	}

	//Interleaving (c to c')
	nr_polar_interleaver(polarParams->nr_polar_B,
						 polarParams->nr_polar_CPrime,
						 polarParams->interleaving_pattern,
						 polarParams->K);

	//Bit insertion (c' to u)
	nr_polar_bit_insertion(polarParams->nr_polar_CPrime,
						   polarParams->nr_polar_U,
						   polarParams->N,
						   polarParams->K,
						   polarParams->Q_I_N,
						   polarParams->Q_PC_N,
						   polarParams->n_pc);

	//Encoding (u to d)
	nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_U,
												   polarParams->G_N,
												   polarParams->nr_polar_D,
												   polarParams->N,
												   polarParams->N);
	for (uint16_t i = 0; i < polarParams->N; i++)
		polarParams->nr_polar_D[i] = (polarParams->nr_polar_D[i] % 2);

	//Rate matching
	//Sub-block interleaving (d to y) and Bit selection (y to e)
	nr_polar_interleaver(polarParams->nr_polar_D,
						 polarParams->nr_polar_E,
						 polarParams->rate_matching_pattern,
						 polarParams->encoderLength);

	/*
	 * Return bits.
	 */
	nr_byte2bit_uint8_32_t(polarParams->nr_polar_E, polarParams->encoderLength, out);
}

void polar_encoder_dci(uint32_t *in,
					   uint32_t *out,
					   t_nrPolar_paramsPtr polarParams,
					   uint16_t n_RNTI)
{
#ifdef DEBUG_POLAR_ENCODER_DCI
	printf("[polar_encoder_dci] in: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n", in[0], in[1], in[2], in[3]);
#endif
	//(a to a')
	nr_crc_bit2bit_uint32_8_t(in, polarParams->payloadBits, polarParams->nr_polar_aPrime);
	//Parity bits computation (p)
	polarParams->crcBit = crc24c(polarParams->nr_polar_aPrime,
								 (polarParams->payloadBits+polarParams->crcParityBits));
#ifdef DEBUG_POLAR_ENCODER_DCI
	printf("[polar_encoder_dci] crc: 0x%08x\n", polarParams->crcBit);
#endif
	//(a to b)
	//Using "nr_polar_aPrime" to hold "nr_polar_b".
	uint8_t arrayInd = ceil(polarParams->payloadBits / 32.0);
	for (int j=0; j<arrayInd-1; j++) {
		for (int i=0; i<32; i++) {
			polarParams->nr_polar_B[i+j*32] = (in[j]>>i)&1;
		}
	}
	for (int i=0; i<((polarParams->payloadBits)%32); i++) {
		polarParams->nr_polar_B[i+(arrayInd-1)*32] = (in[(arrayInd-1)]>>i)&1;
	}
	for (int i=0; i<8; i++) {
		polarParams->nr_polar_B[polarParams->payloadBits+i] = ((polarParams->crcBit)>>(31-i))&1;
	}
	//Scrambling
	for (int i=0; i<16; i++) {
		polarParams->nr_polar_B[polarParams->payloadBits+8+i] =
				( (((polarParams->crcBit)>>(23-i))&1) + ((n_RNTI>>(15-i))&1) ) % 2;
	}
#ifdef DEBUG_POLAR_ENCODER_DCI
	printf("[polar_encoder_dci] B: ");
	for (int i = 0; i < polarParams->K; i++) printf("%d-", polarParams->nr_polar_B[i]);
	printf("\n");
#endif

	//Interleaving (c to c')
	nr_polar_interleaver(polarParams->nr_polar_B,
						 polarParams->nr_polar_CPrime,
						 polarParams->interleaving_pattern,
						 polarParams->K);

	//Bit insertion (c' to u)
	nr_polar_bit_insertion(polarParams->nr_polar_CPrime,
						   polarParams->nr_polar_U,
						   polarParams->N,
						   polarParams->K,
						   polarParams->Q_I_N,
						   polarParams->Q_PC_N,
						   polarParams->n_pc);

	//Encoding (u to d)
	nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_U,
												   polarParams->G_N,
												   polarParams->nr_polar_D,
												   polarParams->N,
												   polarParams->N);
	for (uint16_t i = 0; i < polarParams->N; i++)
		polarParams->nr_polar_D[i] = (polarParams->nr_polar_D[i] % 2);

	//Rate matching
	//Sub-block interleaving (d to y) and Bit selection (y to e)
	nr_polar_interleaver(polarParams->nr_polar_D,
						 polarParams->nr_polar_E,
						 polarParams->rate_matching_pattern,
						 polarParams->encoderLength);

	/*
	 * Return bits.
	 */
	nr_byte2bit_uint8_32_t(polarParams->nr_polar_E, polarParams->encoderLength, out);
#ifdef DEBUG_POLAR_ENCODER_DCI
	uint8_t outputInd = ceil(polarParams->encoderLength / 32.0);
	printf("[polar_encoder_dci] out: ");
	for (int i = 0; i < outputInd; i++) {
		printf("[%d]->0x%08x\t", i, out[i]);
	}
#endif
}
