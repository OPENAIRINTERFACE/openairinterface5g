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

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_pbch_defs.h"

void polar_encoder(
		uint8_t *input,
		uint8_t *output,
		t_nrPolar_params *polarParams)
{

	/*
	 * Bytewise operations
	 */

	//Calculate CRC.
	// --- OLD ---
	//nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(input, polarParams->crc_generator_matrix,
	//		polarParams->nr_polar_crc, polarParams->payloadBits, polarParams->crcParityBits);
	//for (uint8_t i = 0; i < polarParams->crcParityBits; i++) polarParams->nr_polar_crc[i] = (polarParams->nr_polar_crc[i] % 2);
	// --- NEW ---
	nr_crc_computation(input, polarParams->nr_polar_crc, polarParams->payloadBits, polarParams->crcParityBits, polarParams->crc256Table);

	//Attach CRC to the Transport Block. (a to b)
	for (uint16_t i = 0; i < polarParams->payloadBits; i++) polarParams->nr_polar_b[i] = input[i];
	for (uint16_t i = polarParams->payloadBits; i < polarParams->K; i++)
		polarParams->nr_polar_b[i]= polarParams->nr_polar_crc[i-(polarParams->payloadBits)];

	//Interleaving (c to c')
	nr_polar_interleaver(polarParams->nr_polar_b, polarParams->nr_polar_cPrime, polarParams->interleaving_pattern, polarParams->K);

	//Bit insertion (c' to u)
	nr_polar_bit_insertion(polarParams->nr_polar_cPrime, polarParams->nr_polar_u, polarParams->N, polarParams->K,
			polarParams->Q_I_N, polarParams->Q_PC_N, polarParams->n_pc);

	//Encoding (u to d)
	nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_u, polarParams->G_N, polarParams->nr_polar_d, polarParams->N, polarParams->N);
	for (uint16_t i = 0; i < polarParams->N; i++) polarParams->nr_polar_d[i] = (polarParams->nr_polar_d[i] % 2);

	//Rate matching
	//Sub-block interleaving (d to y) and Bit selection (y to e)
	nr_polar_rate_matcher(polarParams->nr_polar_d, output, polarParams->rate_matching_pattern, polarParams->encoderLength);
}
