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

/*!\file PHY/CODING/nr_polar_init.h
 * \brief
 * \author Turker Yilmaz
 * \date 2018
 * \version 0.1
 * \company EURECOM
 * \email turker.yilmaz@eurecom.fr
 * \note
 * \warning
*/

#include "nrPolar_tools/nr_polar_defs.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_dci_defs.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_uci_defs.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_pbch_defs.h"
#include "PHY/NR_TRANSPORT/nr_dci.h"

void nr_polar_init(t_nrPolar_paramsPtr *polarParams,
				   int8_t messageType,
				   uint16_t messageLength,
				   uint8_t aggregation_level)
{
	t_nrPolar_paramsPtr currentPtr = *polarParams;

	//Parse the list. If the node is already created, return without initialization.
	while (currentPtr != NULL) {
		if (currentPtr->idx == (messageType * messageLength)) return;
		else currentPtr = currentPtr->nextPtr;
	}

	//Else, initialize and add node to the end of the linked list.
	t_nrPolar_paramsPtr newPolarInitNode = malloc(sizeof(t_nrPolar_params));

	if (newPolarInitNode != NULL) {

		newPolarInitNode->idx = (messageType * messageLength);
		newPolarInitNode->nextPtr = NULL;

		if (messageType == 0) { //PBCH
			newPolarInitNode->n_max = NR_POLAR_PBCH_N_MAX;
			newPolarInitNode->i_il = NR_POLAR_PBCH_I_IL;
			newPolarInitNode->i_seg = NR_POLAR_PBCH_I_SEG;
			newPolarInitNode->n_pc = NR_POLAR_PBCH_N_PC;
			newPolarInitNode->n_pc_wm = NR_POLAR_PBCH_N_PC_WM;
			newPolarInitNode->i_bil = NR_POLAR_PBCH_I_BIL;
			newPolarInitNode->crcParityBits = NR_POLAR_PBCH_CRC_PARITY_BITS;
			newPolarInitNode->payloadBits = NR_POLAR_PBCH_PAYLOAD_BITS;
			newPolarInitNode->encoderLength = NR_POLAR_PBCH_E;
			newPolarInitNode->crcCorrectionBits = NR_POLAR_PBCH_CRC_ERROR_CORRECTION_BITS;
		} else if (messageType == 1) { //DCI
			newPolarInitNode->n_max = NR_POLAR_DCI_N_MAX;
			newPolarInitNode->i_il = NR_POLAR_DCI_I_IL;
			newPolarInitNode->i_seg = NR_POLAR_DCI_I_SEG;
			newPolarInitNode->n_pc = NR_POLAR_DCI_N_PC;
			newPolarInitNode->n_pc_wm = NR_POLAR_DCI_N_PC_WM;
			newPolarInitNode->i_bil = NR_POLAR_DCI_I_BIL;
			newPolarInitNode->crcParityBits = NR_POLAR_DCI_CRC_PARITY_BITS;
			newPolarInitNode->payloadBits = messageLength;
			newPolarInitNode->encoderLength = aggregation_level*108;
			newPolarInitNode->crcCorrectionBits = NR_POLAR_DCI_CRC_ERROR_CORRECTION_BITS;
		} else if (messageType == -1) { //UCI

		} else {
			AssertFatal(1 == 0, "[nr_polar_init] Incorrect Message Type(%d)", messageType);
		}

		newPolarInitNode->K = newPolarInitNode->payloadBits + newPolarInitNode->crcParityBits; // Number of bits to encode.
		newPolarInitNode->N = nr_polar_output_length(newPolarInitNode->K, newPolarInitNode->encoderLength, newPolarInitNode->n_max);
		newPolarInitNode->n = log2(newPolarInitNode->N);

		newPolarInitNode->crc_generator_matrix = crc24c_generator_matrix(newPolarInitNode->payloadBits);
		newPolarInitNode->G_N = nr_polar_kronecker_power_matrices(newPolarInitNode->n);

		//polar_encoder vectors:
		newPolarInitNode->nr_polar_crc = malloc(sizeof(uint8_t) * newPolarInitNode->crcParityBits);
		newPolarInitNode->nr_polar_d = malloc(sizeof(uint8_t) * newPolarInitNode->N);
		newPolarInitNode->nr_polar_e = malloc(sizeof(uint8_t) * newPolarInitNode->encoderLength);

		//Polar Coding vectors
		newPolarInitNode->nr_polar_u = malloc(sizeof(uint8_t) * newPolarInitNode->N); //Decoder: nr_polar_uHat
		newPolarInitNode->nr_polar_cPrime = malloc(sizeof(uint8_t) * newPolarInitNode->K); //Decoder: nr_polar_cHat
		newPolarInitNode->nr_polar_b = malloc(sizeof(uint8_t) * newPolarInitNode->K); //Decoder: nr_polar_bHat
		newPolarInitNode->nr_polar_a = malloc(sizeof(uint8_t) * newPolarInitNode->payloadBits); //Decoder: nr_polar_aHat



		newPolarInitNode->Q_0_Nminus1 = nr_polar_sequence_pattern(newPolarInitNode->n);

		newPolarInitNode->interleaving_pattern = malloc(sizeof(uint16_t) * newPolarInitNode->K);
		nr_polar_interleaving_pattern(newPolarInitNode->K,
									  newPolarInitNode->i_il,
									  newPolarInitNode->interleaving_pattern);

		newPolarInitNode->rate_matching_pattern = malloc(sizeof(uint16_t) * newPolarInitNode->encoderLength);
		uint16_t *J = malloc(sizeof(uint16_t) * newPolarInitNode->N);
		nr_polar_rate_matching_pattern(newPolarInitNode->rate_matching_pattern,
									   J,
									   nr_polar_subblock_interleaver_pattern,
									   newPolarInitNode->K,
									   newPolarInitNode->N,
									   newPolarInitNode->encoderLength);

		newPolarInitNode->information_bit_pattern = malloc(sizeof(uint8_t) * newPolarInitNode->N);
		newPolarInitNode->Q_I_N = malloc(sizeof(int16_t) * (newPolarInitNode->K + newPolarInitNode->n_pc));
		newPolarInitNode->Q_F_N = malloc( sizeof(int16_t) * (newPolarInitNode->N + 1)); // Last element shows the final array index assigned a value.
		newPolarInitNode->Q_PC_N = malloc( sizeof(int16_t) * (newPolarInitNode->n_pc));
		for (int i = 0; i <= newPolarInitNode->N; i++)
			newPolarInitNode->Q_F_N[i] = -1; // Empty array.
		nr_polar_info_bit_pattern(newPolarInitNode->information_bit_pattern,
								  newPolarInitNode->Q_I_N,
								  newPolarInitNode->Q_F_N,
								  J,
								  newPolarInitNode->Q_0_Nminus1,
								  newPolarInitNode->K,
								  newPolarInitNode->N,
								  newPolarInitNode->encoderLength,
								  newPolarInitNode->n_pc);

		newPolarInitNode->channel_interleaver_pattern = malloc(sizeof(uint16_t) * newPolarInitNode->encoderLength);
		nr_polar_channel_interleaver_pattern(newPolarInitNode->channel_interleaver_pattern,
											 newPolarInitNode->i_bil,
											 newPolarInitNode->encoderLength);

		free(J);

	} else {
		AssertFatal(1 == 0, "[nr_polar_init] New t_nrPolar_paramsPtr could not be created");
	}

	currentPtr = *polarParams;
	//If polarParams is empty:
	if (currentPtr == NULL)
	{
		*polarParams = newPolarInitNode;
		return;
	}
	//Else, add node to the end of the linked list.
	while (currentPtr->nextPtr != NULL) {
			currentPtr = currentPtr->nextPtr;
	}
	currentPtr->nextPtr= newPolarInitNode;
	return;
}

void nr_polar_print_polarParams(t_nrPolar_paramsPtr polarParams)
{
	uint8_t i = 0;
	if (polarParams == NULL) {
		printf("polarParams is empty.\n");
	} else {
		while (polarParams != NULL){
			printf("polarParams[%d] = %d\n", i, polarParams->idx);
			polarParams = polarParams->nextPtr;
			i++;
		}
	}
	return;
}

t_nrPolar_paramsPtr nr_polar_params (t_nrPolar_paramsPtr polarParams,
									 int8_t messageType,
									 uint16_t messageLength)
{
	t_nrPolar_paramsPtr currentPtr = NULL;

	while (polarParams != NULL) {
		if (polarParams->idx == (messageType * messageLength)) {
			currentPtr = polarParams;
			break;
		} else {
			polarParams = polarParams->nextPtr;
		}
	}
	return currentPtr;
}
