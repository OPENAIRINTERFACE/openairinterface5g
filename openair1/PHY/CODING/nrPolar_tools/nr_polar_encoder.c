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

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

void polar_encoder(uint32_t *in,
				   uint32_t *out,
				   t_nrPolar_paramsPtr polarParams)
{
	nr_bit2byte_uint32_8_t(in, polarParams->payloadBits, polarParams->nr_polar_a);

	/*
	 * Bytewise operations
	 */
	//Calculate CRC.
	nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_a,
												   polarParams->crc_generator_matrix,
												   polarParams->nr_polar_crc,
												   polarParams->payloadBits,
												   polarParams->crcParityBits);
	for (uint8_t i = 0; i < polarParams->crcParityBits; i++)
		polarParams->nr_polar_crc[i] = (polarParams->nr_polar_crc[i] % 2);

	//Attach CRC to the Transport Block. (a to b)
	for (uint16_t i = 0; i < polarParams->payloadBits; i++) polarParams->nr_polar_b[i] = polarParams->nr_polar_a[i];
	for (uint16_t i = polarParams->payloadBits; i < polarParams->K; i++)
		polarParams->nr_polar_b[i]= polarParams->nr_polar_crc[i-(polarParams->payloadBits)];

	//Interleaving (c to c')
	nr_polar_interleaver(polarParams->nr_polar_b,
						 polarParams->nr_polar_cPrime,
						 polarParams->interleaving_pattern,
						 polarParams->K);

	//Bit insertion (c' to u)
	nr_polar_bit_insertion(polarParams->nr_polar_cPrime,
						   polarParams->nr_polar_u,
						   polarParams->N,
						   polarParams->K,
						   polarParams->Q_I_N,
						   polarParams->Q_PC_N,
						   polarParams->n_pc);

	//Encoding (u to d)
	nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_u,
												   polarParams->G_N,
												   polarParams->nr_polar_d,
												   polarParams->N,
												   polarParams->N);
	for (uint16_t i = 0; i < polarParams->N; i++)
		polarParams->nr_polar_d[i] = (polarParams->nr_polar_d[i] % 2);

	//Rate matching
	//Sub-block interleaving (d to y) and Bit selection (y to e)
	nr_polar_rate_matcher(polarParams->nr_polar_d,
						  polarParams->nr_polar_e,
						  polarParams->rate_matching_pattern,
						  polarParams->encoderLength);

	/*
	 * Return bits.
	 */
	nr_byte2bit_uint8_32_t(polarParams->nr_polar_e, polarParams->encoderLength, out);
}
