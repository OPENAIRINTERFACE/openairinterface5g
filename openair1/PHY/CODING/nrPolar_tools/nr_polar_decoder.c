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
 * For more information about the OpenAirInterface (OAI) Software Alliance
 *      contact@openairinterface.org
 */

/*!\file PHY/CODING/nrPolar_tools/nr_polar_decoder.c
 * \brief
 * \author Turker Yilmaz
 * \date 2018
 * \version 0.1
 * \company EURECOM
 * \email turker.yilmaz@eurecom.fr
 * \note
 * \warning
*/

/*
 * Return values:
 *  0 --> Success
 * -1 --> All list entries have failed the CRC checks
 */

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"


int8_t polar_decoder(
		double *input,
		uint8_t *out,
		t_nrPolar_paramsPtr polarParams,
		uint8_t listSize,
		uint8_t pathMetricAppr)
{
	//Assumes no a priori knowledge.

	uint8_t ***bit = nr_alloc_uint8_t_3D_array(polarParams->N, (polarParams->n+1), 2*listSize);
	uint8_t **bitUpdated = nr_alloc_uint8_t_2D_array(polarParams->N, (polarParams->n+1)); //0=False, 1=True
	uint8_t **llrUpdated = nr_alloc_uint8_t_2D_array(polarParams->N, (polarParams->n+1)); //0=False, 1=True
	double ***llr = nr_alloc_double_3D_array(polarParams->N, (polarParams->n+1), 2*listSize);
	uint8_t **crcChecksum = nr_alloc_uint8_t_2D_array(polarParams->crcParityBits, 2*listSize);
	double *pathMetric = malloc(sizeof(double)*(2*listSize));
	uint8_t *crcState = malloc(sizeof(uint8_t)*(2*listSize)); //0=False, 1=True

	for (int i=0; i<(2*listSize); i++) {
		pathMetric[i] = 0;
		crcState[i]=1;
	}
	for (int i=0; i<polarParams->N; i++) {
		llrUpdated[i][polarParams->n]=1;
		bitUpdated[i][0]=((polarParams->information_bit_pattern[i]+1) % 2);
	}

	uint8_t **extended_crc_generator_matrix = malloc(polarParams->K * sizeof(uint8_t *)); //G_P3
	uint8_t **tempECGM = malloc(polarParams->K * sizeof(uint8_t *)); //G_P2
	for (int i = 0; i < polarParams->K; i++){
		extended_crc_generator_matrix[i] = malloc(polarParams->crcParityBits * sizeof(uint8_t));
		tempECGM[i] = malloc(polarParams->crcParityBits * sizeof(uint8_t));
	}

	for (int i=0; i<polarParams->payloadBits; i++) {
		for (int j=0; j<polarParams->crcParityBits; j++) {
			tempECGM[i][j]=polarParams->crc_generator_matrix[i][j];
		}
	}
	for (int i=polarParams->payloadBits; i<polarParams->K; i++) {
			for (int j=0; j<polarParams->crcParityBits; j++) {
				if( (i-polarParams->payloadBits) == j ){
					tempECGM[i][j]=1;
				} else {
					tempECGM[i][j]=0;
				}
			}
	}

	for (int i=0; i<polarParams->K; i++) {
		for (int j=0; j<polarParams->crcParityBits; j++) {
			extended_crc_generator_matrix[i][j]=tempECGM[polarParams->interleaving_pattern[i]][j];
		}
	}

	//The index of the last 1-valued bit that appears in each column.
	uint16_t last1ind[polarParams->crcParityBits];
	for (int j=0; j<polarParams->crcParityBits; j++) {
			for (int i=0; i<polarParams->K; i++) {
				if (extended_crc_generator_matrix[i][j]==1) last1ind[j]=i;
			}
	}

	double *d_tilde = malloc(sizeof(double) * polarParams->N);
	nr_polar_rate_matching(input, d_tilde, polarParams->rate_matching_pattern, polarParams->K, polarParams->N, polarParams->encoderLength);
	for (int j = 0; j < polarParams->N; j++) llr[j][polarParams->n][0]=d_tilde[j];


	/*
	 * SCL polar decoder.
	 */

	uint32_t nonFrozenBit=0;
	uint8_t currentListSize=1;
	uint8_t decoderIterationCheck=0;
	int16_t checkCrcBits=-1;
	uint8_t listIndex[2*listSize], copyIndex;

	for (uint16_t currentBit=0; currentBit<polarParams->N; currentBit++){
		updateLLR(llr, llrUpdated, bit, bitUpdated, currentListSize, currentBit, 0, polarParams->N, (polarParams->n+1), pathMetricAppr);
		if (polarParams->information_bit_pattern[currentBit]==0) { //Frozen bit.
			updatePathMetric(pathMetric, llr, currentListSize, 0, currentBit, pathMetricAppr); //approximation=0 --> 11b, approximation=1 --> 12
		} else { //Information or CRC bit.
			updatePathMetric2(pathMetric, llr, currentListSize, currentBit, pathMetricAppr);

			for (int i = 0; i < currentListSize; i++) {
				for (int j = 0; j < polarParams->N; j++) {
					for (int k = 0; k < (polarParams->n+1); k++) {
						bit[j][k][i+currentListSize]=bit[j][k][i];
						llr[j][k][i+currentListSize]=llr[j][k][i];}}}
			for (int i = 0; i < currentListSize; i++) {
				bit[currentBit][0][i]=0;
				crcState[i+currentListSize]=crcState[i];
			}
			for (int i = currentListSize; i < 2*currentListSize; i++) bit[currentBit][0][i]=1;
			bitUpdated[currentBit][0]=1;
			updateCrcChecksum2(crcChecksum, extended_crc_generator_matrix, currentListSize, nonFrozenBit, polarParams->crcParityBits);
			currentListSize*=2;

			//Keep only the best "listSize" number of entries.
			if (currentListSize > listSize) {
				for (uint8_t i = 0; i < 2*listSize; i++) listIndex[i]=i;
				nr_sort_asc_double_1D_array_ind(pathMetric, listIndex, currentListSize);

				//sort listIndex[listSize, ..., 2*listSize-1] in descending order.
				uint8_t swaps, tempInd;
				for (uint8_t i = 0; i < listSize; i++) {
					swaps = 0;
					for (uint8_t j = listSize; j < (2*listSize - i) - 1; j++) {
						if (listIndex[j+1] > listIndex[j]) {
							tempInd = listIndex[j];
							listIndex[j] = listIndex[j + 1];
							listIndex[j + 1] = tempInd;
							swaps++;
						}
					}
					if (swaps == 0)
						break;
				}

				//First, backup the best "listSize" number of entries.
				for (int k=(listSize-1); k>0; k--) {
					for (int i=0; i<polarParams->N; i++) {
						for (int j=0; j<(polarParams->n+1); j++) {
							bit[i][j][listIndex[(2*listSize-1)-k]]=bit[i][j][listIndex[k]];
							llr[i][j][listIndex[(2*listSize-1)-k]]=llr[i][j][listIndex[k]];
						}
					}
				}
				for (int k=(listSize-1); k>0; k--) {
					for (int i = 0; i < polarParams->crcParityBits; i++) {
						crcChecksum[i][listIndex[(2*listSize-1)-k]] = crcChecksum[i][listIndex[k]];
					}
				}
				for (int k=(listSize-1); k>0; k--) crcState[listIndex[(2*listSize-1)-k]]=crcState[listIndex[k]];

				//Copy the best "listSize" number of entries to the first indices.
				for (int k = 0; k < listSize; k++) {
					if (k > listIndex[k]) {
						copyIndex = listIndex[(2*listSize-1)-k];
					} else { //Use the backup.
						copyIndex = listIndex[k];
					}
					for (int i = 0; i < polarParams->N; i++) {
						for (int j = 0; j < (polarParams->n + 1); j++) {
							bit[i][j][k] = bit[i][j][copyIndex];
							llr[i][j][k] = llr[i][j][copyIndex];
						}
					}
				}
				for (int k = 0; k < listSize; k++) {
					if (k > listIndex[k]) {
						copyIndex = listIndex[(2*listSize-1)-k];
					} else { //Use the backup.
						copyIndex = listIndex[k];
					}
					for (int i = 0; i < polarParams->crcParityBits; i++) {
						crcChecksum[i][k]=crcChecksum[i][copyIndex];
					}
				}
				for (int k = 0; k < listSize; k++) {
					if (k > listIndex[k]) {
						copyIndex = listIndex[(2*listSize-1)-k];
					} else { //Use the backup.
						copyIndex = listIndex[k];
					}
					crcState[k]=crcState[copyIndex];
				}
				currentListSize = listSize;
				}


			for (int i=0; i<polarParams->crcParityBits; i++) {
				if (last1ind[i]==nonFrozenBit) {
					checkCrcBits=i;
					break;
				}
			}

			if ( checkCrcBits > (-1) ) {
				for (uint8_t i = 0; i < currentListSize; i++) {
					if (crcChecksum[checkCrcBits][i]==1) {
						crcState[i]=0; //0=False, 1=True
					}
				}
			}

			for (uint8_t i = 0; i < currentListSize; i++) decoderIterationCheck+=crcState[i];
			if (decoderIterationCheck==0) {
				//perror("[SCL polar decoder] All list entries have failed the CRC checks.");
				free(d_tilde);
				free(pathMetric);
				free(crcState);
				nr_free_uint8_t_3D_array(bit, polarParams->N, (polarParams->n+1));
				nr_free_double_3D_array(llr, polarParams->N, (polarParams->n+1));
				nr_free_uint8_t_2D_array(crcChecksum, polarParams->crcParityBits);
				return(-1);
			}

			nonFrozenBit++;
			decoderIterationCheck=0;
			checkCrcBits=-1;
		}
	}

	for (uint8_t i = 0; i < 2*listSize; i++) listIndex[i]=i;
	nr_sort_asc_double_1D_array_ind(pathMetric, listIndex, currentListSize);

	for (uint8_t i = 0; i < fmin(listSize, (pow(2,polarParams->crcCorrectionBits)) ); i++) {
		if ( crcState[listIndex[i]] == 1 ) {
			for (int j = 0; j < polarParams->N; j++) polarParams->nr_polar_U[j]=bit[j][0][listIndex[i]];

			//Extract the information bits (û to ĉ)
			nr_polar_info_bit_extraction(polarParams->nr_polar_U, polarParams->nr_polar_CPrime, polarParams->information_bit_pattern, polarParams->N);

			//Deinterleaving (ĉ to b)
			nr_polar_deinterleaver(polarParams->nr_polar_CPrime, polarParams->nr_polar_B, polarParams->interleaving_pattern, polarParams->K);

			//Remove the CRC (â)
			for (int j = 0; j < polarParams->payloadBits; j++) polarParams->nr_polar_A[j]=polarParams->nr_polar_B[j];

			break;
		}
	}

	free(d_tilde);
	free(pathMetric);
	free(crcState);
	nr_free_uint8_t_3D_array(bit, polarParams->N, (polarParams->n+1));
	nr_free_double_3D_array(llr, polarParams->N, (polarParams->n+1));
	nr_free_uint8_t_2D_array(crcChecksum, polarParams->crcParityBits);
	nr_free_uint8_t_2D_array(extended_crc_generator_matrix, polarParams->K);
	nr_free_uint8_t_2D_array(tempECGM, polarParams->K);

	/*
	 * Return bits.
	 */
	//nr_byte2bit_uint8_32_t(polarParams->nr_polar_A, polarParams->payloadBits, out);
	return(0);
}

int8_t polar_decoder_aPriori(double *input,
							 uint32_t *out,
							 t_nrPolar_paramsPtr polarParams,
							 uint8_t listSize,
							 uint8_t pathMetricAppr,
							 double *aPrioriPayload)
{
	uint8_t ***bit = nr_alloc_uint8_t_3D_array(polarParams->N, (polarParams->n+1), 2*listSize);
	uint8_t **bitUpdated = nr_alloc_uint8_t_2D_array(polarParams->N, (polarParams->n+1)); //0=False, 1=True
	uint8_t **llrUpdated = nr_alloc_uint8_t_2D_array(polarParams->N, (polarParams->n+1)); //0=False, 1=True
	double ***llr = nr_alloc_double_3D_array(polarParams->N, (polarParams->n+1), 2*listSize);
	uint8_t **crcChecksum = nr_alloc_uint8_t_2D_array(polarParams->crcParityBits, 2*listSize);
	double *pathMetric = malloc(sizeof(double)*(2*listSize));
	uint8_t *crcState = malloc(sizeof(uint8_t)*(2*listSize)); //0=False, 1=True

	for (int i=0; i<(2*listSize); i++) {
		pathMetric[i] = 0;
		crcState[i]=1;
	}
	for (int i=0; i<polarParams->N; i++) {
		llrUpdated[i][polarParams->n]=1;
		bitUpdated[i][0]=((polarParams->information_bit_pattern[i]+1) % 2);
	}

	uint8_t **extended_crc_generator_matrix = malloc(polarParams->K * sizeof(uint8_t *)); //G_P3
	uint8_t **tempECGM = malloc(polarParams->K * sizeof(uint8_t *)); //G_P2
	for (int i = 0; i < polarParams->K; i++){
		extended_crc_generator_matrix[i] = malloc(polarParams->crcParityBits * sizeof(uint8_t));
		tempECGM[i] = malloc(polarParams->crcParityBits * sizeof(uint8_t));
	}

	for (int i=0; i<polarParams->payloadBits; i++) {
		for (int j=0; j<polarParams->crcParityBits; j++) {
			tempECGM[i][j]=polarParams->crc_generator_matrix[i][j];
		}
	}
	for (int i=polarParams->payloadBits; i<polarParams->K; i++) {
			for (int j=0; j<polarParams->crcParityBits; j++) {
				if( (i-polarParams->payloadBits) == j ){
					tempECGM[i][j]=1;
				} else {
					tempECGM[i][j]=0;
				}
			}
	}

	for (int i=0; i<polarParams->K; i++) {
		for (int j=0; j<polarParams->crcParityBits; j++) {
			extended_crc_generator_matrix[i][j]=tempECGM[polarParams->interleaving_pattern[i]][j];
		}
	}

	//The index of the last 1-valued bit that appears in each column.
	uint16_t last1ind[polarParams->crcParityBits];
	for (int j=0; j<polarParams->crcParityBits; j++) {
			for (int i=0; i<polarParams->K; i++) {
				if (extended_crc_generator_matrix[i][j]==1) last1ind[j]=i;
			}
	}

	double *d_tilde = malloc(sizeof(double) * polarParams->N);
	nr_polar_rate_matching(input, d_tilde, polarParams->rate_matching_pattern, polarParams->K, polarParams->N, polarParams->encoderLength);
	for (int j = 0; j < polarParams->N; j++) llr[j][polarParams->n][0]=d_tilde[j];


	/*
	 * SCL polar decoder.
	 */

	uint32_t nonFrozenBit=0;
	uint8_t currentListSize=1;
	uint8_t decoderIterationCheck=0;
	int16_t checkCrcBits=-1;
	uint8_t listIndex[2*listSize], copyIndex;

	for (uint16_t currentBit=0; currentBit<polarParams->N; currentBit++){

		updateLLR(llr, llrUpdated, bit, bitUpdated, currentListSize, currentBit, 0, polarParams->N, (polarParams->n+1), pathMetricAppr);
		if (polarParams->information_bit_pattern[currentBit]==0) { //Frozen bit.
			updatePathMetric(pathMetric, llr, currentListSize, 0, currentBit, pathMetricAppr); //approximation=0 --> 11b, approximation=1 --> 12
		} else { //Information or CRC bit.
			if ( (polarParams->interleaving_pattern[nonFrozenBit] <= polarParams->payloadBits) &&
					(aPrioriPayload[polarParams->interleaving_pattern[nonFrozenBit]] == 0) ) {
				//Information bit with known value of "0".
				updatePathMetric(pathMetric, llr, currentListSize, 0, currentBit, pathMetricAppr);
				bitUpdated[currentBit][0]=1; //0=False, 1=True
			} else if ( (polarParams->interleaving_pattern[nonFrozenBit] <= polarParams->payloadBits) &&
					(aPrioriPayload[polarParams->interleaving_pattern[nonFrozenBit]] == 1) ) {
				//Information bit with known value of "1".
				updatePathMetric(pathMetric, llr, currentListSize, 1, currentBit, pathMetricAppr);
				for (uint8_t i=0; i<currentListSize; i++) bit[currentBit][0][i]=1;
				bitUpdated[currentBit][0]=1;
				updateCrcChecksum(crcChecksum, extended_crc_generator_matrix, currentListSize, nonFrozenBit, polarParams->crcParityBits);
			} else {
				updatePathMetric2(pathMetric, llr, currentListSize, currentBit, pathMetricAppr);

				for (int i = 0; i < currentListSize; i++) {
					for (int j = 0; j < polarParams->N; j++) {
						for (int k = 0; k < (polarParams->n+1); k++) {
							bit[j][k][i+currentListSize]=bit[j][k][i];
							llr[j][k][i+currentListSize]=llr[j][k][i];}}}
				for (int i = 0; i < currentListSize; i++) {
					bit[currentBit][0][i]=0;
					crcState[i+currentListSize]=crcState[i];
				}
				for (int i = currentListSize; i < 2*currentListSize; i++) bit[currentBit][0][i]=1;
				bitUpdated[currentBit][0]=1;
				updateCrcChecksum2(crcChecksum, extended_crc_generator_matrix, currentListSize, nonFrozenBit, polarParams->crcParityBits);
				currentListSize*=2;

				//Keep only the best "listSize" number of entries.
				if (currentListSize > listSize) {
					for (uint8_t i = 0; i < 2*listSize; i++) listIndex[i]=i;
					nr_sort_asc_double_1D_array_ind(pathMetric, listIndex, currentListSize);

					//sort listIndex[listSize, ..., 2*listSize-1] in descending order.
					uint8_t swaps, tempInd;
					for (uint8_t i = 0; i < listSize; i++) {
						swaps = 0;
						for (uint8_t j = listSize; j < (2*listSize - i) - 1; j++) {
							if (listIndex[j+1] > listIndex[j]) {
								tempInd = listIndex[j];
								listIndex[j] = listIndex[j + 1];
								listIndex[j + 1] = tempInd;
								swaps++;
							}
						}
						if (swaps == 0)
							break;
					}

					//First, backup the best "listSize" number of entries.
					for (int k=(listSize-1); k>0; k--) {
						for (int i=0; i<polarParams->N; i++) {
							for (int j=0; j<(polarParams->n+1); j++) {
								bit[i][j][listIndex[(2*listSize-1)-k]]=bit[i][j][listIndex[k]];
								llr[i][j][listIndex[(2*listSize-1)-k]]=llr[i][j][listIndex[k]];
							}
						}
					}
					for (int k=(listSize-1); k>0; k--) {
						for (int i = 0; i < polarParams->crcParityBits; i++) {
							crcChecksum[i][listIndex[(2*listSize-1)-k]] = crcChecksum[i][listIndex[k]];
						}
					}
					for (int k=(listSize-1); k>0; k--) crcState[listIndex[(2*listSize-1)-k]]=crcState[listIndex[k]];

					//Copy the best "listSize" number of entries to the first indices.
					for (int k = 0; k < listSize; k++) {
						if (k > listIndex[k]) {
							copyIndex = listIndex[(2*listSize-1)-k];
						} else { //Use the backup.
							copyIndex = listIndex[k];
						}
						for (int i = 0; i < polarParams->N; i++) {
							for (int j = 0; j < (polarParams->n + 1); j++) {
								bit[i][j][k] = bit[i][j][copyIndex];
								llr[i][j][k] = llr[i][j][copyIndex];
							}
						}
					}
					for (int k = 0; k < listSize; k++) {
						if (k > listIndex[k]) {
							copyIndex = listIndex[(2*listSize-1)-k];
						} else { //Use the backup.
							copyIndex = listIndex[k];
						}
						for (int i = 0; i < polarParams->crcParityBits; i++) {
							crcChecksum[i][k]=crcChecksum[i][copyIndex];
						}
					}
					for (int k = 0; k < listSize; k++) {
						if (k > listIndex[k]) {
							copyIndex = listIndex[(2*listSize-1)-k];
						} else { //Use the backup.
							copyIndex = listIndex[k];
						}
						crcState[k]=crcState[copyIndex];
					}
					currentListSize = listSize;
				}
			}

			for (int i=0; i<polarParams->crcParityBits; i++) {
				if (last1ind[i]==nonFrozenBit) {
					checkCrcBits=i;
					break;
				}
			}

			if ( checkCrcBits > (-1) ) {
				for (uint8_t i = 0; i < currentListSize; i++) {
					if (crcChecksum[checkCrcBits][i]==1) {
						crcState[i]=0; //0=False, 1=True
					}
				}
			}

			for (uint8_t i = 0; i < currentListSize; i++) decoderIterationCheck+=crcState[i];
			if (decoderIterationCheck==0) {
				//perror("[SCL polar decoder] All list entries have failed the CRC checks.");
				free(d_tilde);
				free(pathMetric);
				free(crcState);
				nr_free_uint8_t_3D_array(bit, polarParams->N, (polarParams->n+1));
				nr_free_double_3D_array(llr, polarParams->N, (polarParams->n+1));
				nr_free_uint8_t_2D_array(crcChecksum, polarParams->crcParityBits);
				return(-1);
			}

			nonFrozenBit++;
			decoderIterationCheck=0;
			checkCrcBits=-1;
		}
	}

	for (uint8_t i = 0; i < 2*listSize; i++) listIndex[i]=i;
	nr_sort_asc_double_1D_array_ind(pathMetric, listIndex, currentListSize);

	for (uint8_t i = 0; i < fmin(listSize, (pow(2,polarParams->crcCorrectionBits)) ); i++) {
		if ( crcState[listIndex[i]] == 1 ) {
			for (int j = 0; j < polarParams->N; j++) polarParams->nr_polar_U[j]=bit[j][0][listIndex[i]];

			//Extract the information bits (û to ĉ)
			nr_polar_info_bit_extraction(polarParams->nr_polar_U, polarParams->nr_polar_CPrime, polarParams->information_bit_pattern, polarParams->N);

			//Deinterleaving (ĉ to b)
			nr_polar_deinterleaver(polarParams->nr_polar_CPrime, polarParams->nr_polar_B, polarParams->interleaving_pattern, polarParams->K);

			//Remove the CRC (â)
			for (int j = 0; j < polarParams->payloadBits; j++) polarParams->nr_polar_A[j]=polarParams->nr_polar_B[j];

			break;
		}
	}

	free(d_tilde);
	free(pathMetric);
	free(crcState);
	nr_free_uint8_t_3D_array(bit, polarParams->N, (polarParams->n+1));
	nr_free_double_3D_array(llr, polarParams->N, (polarParams->n+1));
	nr_free_uint8_t_2D_array(crcChecksum, polarParams->crcParityBits);
	nr_free_uint8_t_2D_array(extended_crc_generator_matrix, polarParams->K);
	nr_free_uint8_t_2D_array(tempECGM, polarParams->K);

	/*
	 * Return bits.
	 */
	nr_byte2bit_uint8_32_t(polarParams->nr_polar_A, polarParams->payloadBits, out);
	return(0);

}



int8_t polar_decoder_aPriori_timing(double *input,
									uint32_t *out,
									t_nrPolar_paramsPtr polarParams,
									uint8_t listSize,
									uint8_t pathMetricAppr,
									double *aPrioriPayload,
									double cpuFreqGHz,
									FILE* logFile)
{

	uint8_t ***bit = nr_alloc_uint8_t_3D_array(polarParams->N, (polarParams->n+1), 2*listSize);
	uint8_t **bitUpdated = nr_alloc_uint8_t_2D_array(polarParams->N, (polarParams->n+1)); //0=False, 1=True
	uint8_t **llrUpdated = nr_alloc_uint8_t_2D_array(polarParams->N, (polarParams->n+1)); //0=False, 1=True
	double ***llr = nr_alloc_double_3D_array(polarParams->N, (polarParams->n+1), 2*listSize);
	uint8_t **crcChecksum = nr_alloc_uint8_t_2D_array(polarParams->crcParityBits, 2*listSize);
	double *pathMetric = malloc(sizeof(double)*(2*listSize));
	uint8_t *crcState = malloc(sizeof(uint8_t)*(2*listSize)); //0=False, 1=True

	for (int i=0; i<(2*listSize); i++) {
		pathMetric[i] = 0;
		crcState[i]=1;
	}
	for (int i=0; i<polarParams->N; i++) {
		llrUpdated[i][polarParams->n]=1;
		bitUpdated[i][0]=((polarParams->information_bit_pattern[i]+1) % 2);
	}

	uint8_t **extended_crc_generator_matrix = malloc(polarParams->K * sizeof(uint8_t *)); //G_P3
	uint8_t **tempECGM = malloc(polarParams->K * sizeof(uint8_t *)); //G_P2
	for (int i = 0; i < polarParams->K; i++){
		extended_crc_generator_matrix[i] = malloc(polarParams->crcParityBits * sizeof(uint8_t));
		tempECGM[i] = malloc(polarParams->crcParityBits * sizeof(uint8_t));
	}

	for (int i=0; i<polarParams->payloadBits; i++) {
		for (int j=0; j<polarParams->crcParityBits; j++) {
			tempECGM[i][j]=polarParams->crc_generator_matrix[i][j];
		}
	}
	for (int i=polarParams->payloadBits; i<polarParams->K; i++) {
			for (int j=0; j<polarParams->crcParityBits; j++) {
				if( (i-polarParams->payloadBits) == j ){
					tempECGM[i][j]=1;
				} else {
					tempECGM[i][j]=0;
				}
			}
	}

	for (int i=0; i<polarParams->K; i++) {
		for (int j=0; j<polarParams->crcParityBits; j++) {
			extended_crc_generator_matrix[i][j]=tempECGM[polarParams->interleaving_pattern[i]][j];
		}
	}

	//The index of the last 1-valued bit that appears in each column.
	uint16_t last1ind[polarParams->crcParityBits];
	for (int j=0; j<polarParams->crcParityBits; j++) {
			for (int i=0; i<polarParams->K; i++) {
				if (extended_crc_generator_matrix[i][j]==1) last1ind[j]=i;
			}
	}

	double *d_tilde = malloc(sizeof(double) * polarParams->N);
	nr_polar_rate_matching(input, d_tilde, polarParams->rate_matching_pattern, polarParams->K, polarParams->N, polarParams->encoderLength);
	for (int j = 0; j < polarParams->N; j++) llr[j][polarParams->n][0]=d_tilde[j];


	/*
	 * SCL polar decoder.
	 */

	uint32_t nonFrozenBit=0;
	uint8_t currentListSize=1;
	uint8_t decoderIterationCheck=0;
	int16_t checkCrcBits=-1;
	uint8_t listIndex[2*listSize], copyIndex;

	for (uint16_t currentBit=0; currentBit<polarParams->N; currentBit++){
		updateLLR(llr, llrUpdated, bit, bitUpdated, currentListSize, currentBit, 0, polarParams->N, (polarParams->n+1), pathMetricAppr);
		if (polarParams->information_bit_pattern[currentBit]==0) { //Frozen bit.
			updatePathMetric(pathMetric, llr, currentListSize, 0, currentBit, pathMetricAppr); //approximation=0 --> 11b, approximation=1 --> 12
		} else { //Information or CRC bit.
			if ( (polarParams->interleaving_pattern[nonFrozenBit] <= polarParams->payloadBits) &&
					(aPrioriPayload[polarParams->interleaving_pattern[nonFrozenBit]] == 0) ) {
				//Information bit with known value of "0".
				updatePathMetric(pathMetric, llr, currentListSize, 0, currentBit, pathMetricAppr);
				bitUpdated[currentBit][0]=1; //0=False, 1=True
			} else if ( (polarParams->interleaving_pattern[nonFrozenBit] <= polarParams->payloadBits) &&
					(aPrioriPayload[polarParams->interleaving_pattern[nonFrozenBit]] == 1) ) {
				//Information bit with known value of "1".
				updatePathMetric(pathMetric, llr, currentListSize, 1, currentBit, pathMetricAppr);
				for (uint8_t i=0; i<currentListSize; i++) bit[currentBit][0][i]=1;
				bitUpdated[currentBit][0]=1;
				updateCrcChecksum(crcChecksum, extended_crc_generator_matrix, currentListSize, nonFrozenBit, polarParams->crcParityBits);
			} else {
				updatePathMetric2(pathMetric, llr, currentListSize, currentBit, pathMetricAppr);

				for (int i = 0; i < currentListSize; i++) {
					for (int j = 0; j < polarParams->N; j++) {
						for (int k = 0; k < (polarParams->n+1); k++) {
							bit[j][k][i+currentListSize]=bit[j][k][i];
							llr[j][k][i+currentListSize]=llr[j][k][i];}}}
				for (int i = 0; i < currentListSize; i++) {
					bit[currentBit][0][i]=0;
					crcState[i+currentListSize]=crcState[i];
				}
				for (int i = currentListSize; i < 2*currentListSize; i++) bit[currentBit][0][i]=1;
				bitUpdated[currentBit][0]=1;
				updateCrcChecksum2(crcChecksum, extended_crc_generator_matrix, currentListSize, nonFrozenBit, polarParams->crcParityBits);
				currentListSize*=2;

				//Keep only the best "listSize" number of entries.
				if (currentListSize > listSize) {
					for (uint8_t i = 0; i < 2*listSize; i++) listIndex[i]=i;
					nr_sort_asc_double_1D_array_ind(pathMetric, listIndex, currentListSize);

					//sort listIndex[listSize, ..., 2*listSize-1] in descending order.
					uint8_t swaps, tempInd;
					for (uint8_t i = 0; i < listSize; i++) {
						swaps = 0;
						for (uint8_t j = listSize; j < (2*listSize - i) - 1; j++) {
							if (listIndex[j+1] > listIndex[j]) {
								tempInd = listIndex[j];
								listIndex[j] = listIndex[j + 1];
								listIndex[j + 1] = tempInd;
								swaps++;
							}
						}
						if (swaps == 0)
							break;
					}

					//First, backup the best "listSize" number of entries.
					for (int k=(listSize-1); k>0; k--) {
						for (int i=0; i<polarParams->N; i++) {
							for (int j=0; j<(polarParams->n+1); j++) {
								bit[i][j][listIndex[(2*listSize-1)-k]]=bit[i][j][listIndex[k]];
								llr[i][j][listIndex[(2*listSize-1)-k]]=llr[i][j][listIndex[k]];
							}
						}
					}
					for (int k=(listSize-1); k>0; k--) {
						for (int i = 0; i < polarParams->crcParityBits; i++) {
							crcChecksum[i][listIndex[(2*listSize-1)-k]] = crcChecksum[i][listIndex[k]];
						}
					}
					for (int k=(listSize-1); k>0; k--) crcState[listIndex[(2*listSize-1)-k]]=crcState[listIndex[k]];

					//Copy the best "listSize" number of entries to the first indices.
					for (int k = 0; k < listSize; k++) {
						if (k > listIndex[k]) {
							copyIndex = listIndex[(2*listSize-1)-k];
						} else { //Use the backup.
							copyIndex = listIndex[k];
						}
						for (int i = 0; i < polarParams->N; i++) {
							for (int j = 0; j < (polarParams->n + 1); j++) {
								bit[i][j][k] = bit[i][j][copyIndex];
								llr[i][j][k] = llr[i][j][copyIndex];
							}
						}
					}
					for (int k = 0; k < listSize; k++) {
						if (k > listIndex[k]) {
							copyIndex = listIndex[(2*listSize-1)-k];
						} else { //Use the backup.
							copyIndex = listIndex[k];
						}
						for (int i = 0; i < polarParams->crcParityBits; i++) {
							crcChecksum[i][k]=crcChecksum[i][copyIndex];
						}
					}
					for (int k = 0; k < listSize; k++) {
						if (k > listIndex[k]) {
							copyIndex = listIndex[(2*listSize-1)-k];
						} else { //Use the backup.
							copyIndex = listIndex[k];
						}
						crcState[k]=crcState[copyIndex];
					}
					currentListSize = listSize;
				}
			}

			for (int i=0; i<polarParams->crcParityBits; i++) {
				if (last1ind[i]==nonFrozenBit) {
					checkCrcBits=i;
					break;
				}
			}

			if ( checkCrcBits > (-1) ) {
				for (uint8_t i = 0; i < currentListSize; i++) {
					if (crcChecksum[checkCrcBits][i]==1) {
						crcState[i]=0; //0=False, 1=True
					}
				}
			}

			for (uint8_t i = 0; i < currentListSize; i++) decoderIterationCheck+=crcState[i];
			if (decoderIterationCheck==0) {
				//perror("[SCL polar decoder] All list entries have failed the CRC checks.");
				free(d_tilde);
				free(pathMetric);
				free(crcState);
				nr_free_uint8_t_3D_array(bit, polarParams->N, (polarParams->n+1));
				nr_free_double_3D_array(llr, polarParams->N, (polarParams->n+1));
				nr_free_uint8_t_2D_array(crcChecksum, polarParams->crcParityBits);
				return(-1);
			}

			nonFrozenBit++;
			decoderIterationCheck=0;
			checkCrcBits=-1;
		}
	}

	for (uint8_t i = 0; i < 2*listSize; i++) listIndex[i]=i;
	nr_sort_asc_double_1D_array_ind(pathMetric, listIndex, currentListSize);

	for (uint8_t i = 0; i < fmin(listSize, (pow(2,polarParams->crcCorrectionBits)) ); i++) {
		if ( crcState[listIndex[i]] == 1 ) {
			for (int j = 0; j < polarParams->N; j++) polarParams->nr_polar_U[j]=bit[j][0][listIndex[i]];

			//Extract the information bits (û to ĉ)
			nr_polar_info_bit_extraction(polarParams->nr_polar_U, polarParams->nr_polar_CPrime, polarParams->information_bit_pattern, polarParams->N);

			//Deinterleaving (ĉ to b)
			nr_polar_deinterleaver(polarParams->nr_polar_CPrime, polarParams->nr_polar_B, polarParams->interleaving_pattern, polarParams->K);

			//Remove the CRC (â)
			for (int j = 0; j < polarParams->payloadBits; j++) polarParams->nr_polar_A[j]=polarParams->nr_polar_B[j];

			break;
		}
	}

	free(d_tilde);
	free(pathMetric);
	free(crcState);
	nr_free_uint8_t_3D_array(bit, polarParams->N, (polarParams->n+1));
	nr_free_double_3D_array(llr, polarParams->N, (polarParams->n+1));
	nr_free_uint8_t_2D_array(crcChecksum, polarParams->crcParityBits);
	nr_free_uint8_t_2D_array(extended_crc_generator_matrix, polarParams->K);
	nr_free_uint8_t_2D_array(tempECGM, polarParams->K);

	/*
	 * Return bits.
	 */
	nr_byte2bit_uint8_32_t(polarParams->nr_polar_A, polarParams->payloadBits, out);
	return(0);
}


int8_t polar_decoder_dci(double *input,
						 uint32_t *out,
						 t_nrPolar_paramsPtr polarParams,
						 uint8_t listSize,
						 uint8_t pathMetricAppr,
						 uint16_t n_RNTI)
{

	uint8_t ***bit = nr_alloc_uint8_t_3D_array(polarParams->N, (polarParams->n+1), 2*listSize);
	uint8_t **bitUpdated = nr_alloc_uint8_t_2D_array(polarParams->N, (polarParams->n+1)); //0=False, 1=True
	uint8_t **llrUpdated = nr_alloc_uint8_t_2D_array(polarParams->N, (polarParams->n+1)); //0=False, 1=True
	double ***llr = nr_alloc_double_3D_array(polarParams->N, (polarParams->n+1), 2*listSize);
	uint8_t **crcChecksum = nr_alloc_uint8_t_2D_array(polarParams->crcParityBits, 2*listSize);
	double *pathMetric = malloc(sizeof(double)*(2*listSize));
	uint8_t *crcState = malloc(sizeof(uint8_t)*(2*listSize)); //0=False, 1=True
	uint8_t extended_crc_scrambling_pattern[polarParams->crcParityBits];

	for (int i=0; i<(2*listSize); i++) {
		pathMetric[i] = 0;
		crcState[i]=1;
	}
	for (int i=0; i<polarParams->N; i++) {
		llrUpdated[i][polarParams->n]=1;
		bitUpdated[i][0]=((polarParams->information_bit_pattern[i]+1) % 2);
	}

	uint8_t **extended_crc_generator_matrix = malloc(polarParams->K * sizeof(uint8_t *)); //G_P3: K-by-P
	uint8_t **tempECGM = malloc(polarParams->K * sizeof(uint8_t *)); //G_P2: K-by-P
	for (int i = 0; i < polarParams->K; i++){
		extended_crc_generator_matrix[i] = malloc(polarParams->crcParityBits * sizeof(uint8_t));
		tempECGM[i] = malloc(polarParams->crcParityBits * sizeof(uint8_t));
	}

	for (int i=0; i<polarParams->payloadBits; i++) {
		for (int j=0; j<polarParams->crcParityBits; j++) {
			tempECGM[i][j]=polarParams->crc_generator_matrix[i+polarParams->crcParityBits][j];
		}
	}
	for (int i=polarParams->payloadBits; i<polarParams->K; i++) {
			for (int j=0; j<polarParams->crcParityBits; j++) {
				if( (i-polarParams->payloadBits) == j ){
					tempECGM[i][j]=1;
				} else {
					tempECGM[i][j]=0;
				}
			}
	}

	for (int i=0; i<polarParams->K; i++) {
		for (int j=0; j<polarParams->crcParityBits; j++) {
			extended_crc_generator_matrix[i][j]=tempECGM[polarParams->interleaving_pattern[i]][j];
		}
	}

	//The index of the last 1-valued bit that appears in each column.
	uint16_t last1ind[polarParams->crcParityBits];
	for (int j=0; j<polarParams->crcParityBits; j++) {
			for (int i=0; i<polarParams->K; i++) {
				if (extended_crc_generator_matrix[i][j]==1) last1ind[j]=i;
			}
	}

	for (int i=0;i<8;i++) extended_crc_scrambling_pattern[i]=0;
	for (int i=8; i<polarParams->crcParityBits; i++) {
		extended_crc_scrambling_pattern[i]=(n_RNTI>>(23-i))&1;
	}

	double *d_tilde = malloc(sizeof(double) * polarParams->N);
	nr_polar_rate_matching(input, d_tilde, polarParams->rate_matching_pattern, polarParams->K, polarParams->N, polarParams->encoderLength);
	for (int j = 0; j < polarParams->N; j++) llr[j][polarParams->n][0]=d_tilde[j];

	/*
	 * SCL polar decoder.
	 */

	for (int i=0; i<polarParams->crcParityBits; i++) {
		for (int j=0; j<polarParams->crcParityBits; j++) crcChecksum[i][0]=crcChecksum[i][0]+polarParams->crc_generator_matrix[j][i];
		crcChecksum[i][0]=(crcChecksum[i][0]%2);
	}

	uint32_t nonFrozenBit=0;
	uint8_t currentListSize=1;
	uint8_t decoderIterationCheck=0;
	int16_t checkCrcBits=-1;
	uint8_t listIndex[2*listSize], copyIndex;

	for (uint16_t currentBit=0; currentBit<polarParams->N; currentBit++){
		updateLLR(llr, llrUpdated, bit, bitUpdated, currentListSize, currentBit, 0, polarParams->N, (polarParams->n+1), pathMetricAppr);
		if (polarParams->information_bit_pattern[currentBit]==0) { //Frozen bit.
			updatePathMetric(pathMetric, llr, currentListSize, 0, currentBit, pathMetricAppr); //approximation=0 --> 11b, approximation=1 --> 12
		} else { //Information or CRC bit.
				updatePathMetric2(pathMetric, llr, currentListSize, currentBit, pathMetricAppr);

				for (int i = 0; i < currentListSize; i++) {
					for (int j = 0; j < polarParams->N; j++) {
						for (int k = 0; k < (polarParams->n+1); k++) {
							bit[j][k][i+currentListSize]=bit[j][k][i];
							llr[j][k][i+currentListSize]=llr[j][k][i];}}}
				for (int i = 0; i < currentListSize; i++) {
					bit[currentBit][0][i]=0;
					crcState[i+currentListSize]=crcState[i];
				}
				for (int i = currentListSize; i < 2*currentListSize; i++) bit[currentBit][0][i]=1;
				bitUpdated[currentBit][0]=1;
				updateCrcChecksum2(crcChecksum, extended_crc_generator_matrix, currentListSize, nonFrozenBit, polarParams->crcParityBits);
				currentListSize*=2;

				//Keep only the best "listSize" number of entries.
				if (currentListSize > listSize) {
					for (uint8_t i = 0; i < 2*listSize; i++) listIndex[i]=i;
					nr_sort_asc_double_1D_array_ind(pathMetric, listIndex, currentListSize);

					//sort listIndex[listSize, ..., 2*listSize-1] in descending order.
					uint8_t swaps, tempInd;
					for (uint8_t i = 0; i < listSize; i++) {
						swaps = 0;
						for (uint8_t j = listSize; j < (2*listSize - i) - 1; j++) {
							if (listIndex[j+1] > listIndex[j]) {
								tempInd = listIndex[j];
								listIndex[j] = listIndex[j + 1];
								listIndex[j + 1] = tempInd;
								swaps++;
							}
						}
						if (swaps == 0)
							break;
					}

					//First, backup the best "listSize" number of entries.
					for (int k=(listSize-1); k>0; k--) {
						for (int i=0; i<polarParams->N; i++) {
							for (int j=0; j<(polarParams->n+1); j++) {
								bit[i][j][listIndex[(2*listSize-1)-k]]=bit[i][j][listIndex[k]];
								llr[i][j][listIndex[(2*listSize-1)-k]]=llr[i][j][listIndex[k]];
							}
						}
					}
					for (int k=(listSize-1); k>0; k--) {
						for (int i = 0; i < polarParams->crcParityBits; i++) {
							crcChecksum[i][listIndex[(2*listSize-1)-k]] = crcChecksum[i][listIndex[k]];
						}
					}
					for (int k=(listSize-1); k>0; k--) crcState[listIndex[(2*listSize-1)-k]]=crcState[listIndex[k]];

					//Copy the best "listSize" number of entries to the first indices.
					for (int k = 0; k < listSize; k++) {
						if (k > listIndex[k]) {
							copyIndex = listIndex[(2*listSize-1)-k];
						} else { //Use the backup.
							copyIndex = listIndex[k];
						}
						for (int i = 0; i < polarParams->N; i++) {
							for (int j = 0; j < (polarParams->n + 1); j++) {
								bit[i][j][k] = bit[i][j][copyIndex];
								llr[i][j][k] = llr[i][j][copyIndex];
							}
						}
					}
					for (int k = 0; k < listSize; k++) {
						if (k > listIndex[k]) {
							copyIndex = listIndex[(2*listSize-1)-k];
						} else { //Use the backup.
							copyIndex = listIndex[k];
						}
						for (int i = 0; i < polarParams->crcParityBits; i++) {
							crcChecksum[i][k]=crcChecksum[i][copyIndex];
						}
					}
					for (int k = 0; k < listSize; k++) {
						if (k > listIndex[k]) {
							copyIndex = listIndex[(2*listSize-1)-k];
						} else { //Use the backup.
							copyIndex = listIndex[k];
						}
						crcState[k]=crcState[copyIndex];
					}
					currentListSize = listSize;
				}

			for (int i=0; i<polarParams->crcParityBits; i++) {
				if (last1ind[i]==nonFrozenBit) {
					checkCrcBits=i;
					break;
				}
			}

			if ( checkCrcBits > (-1) ) {
				for (uint8_t i = 0; i < currentListSize; i++) {
					if (crcChecksum[checkCrcBits][i]!=extended_crc_scrambling_pattern[checkCrcBits]) {
						crcState[i]=0; //0=False, 1=True
					}
				}
			}

			for (uint8_t i = 0; i < currentListSize; i++) decoderIterationCheck+=crcState[i];
			if (decoderIterationCheck==0) {
				//perror("[SCL polar decoder] All list entries have failed the CRC checks.");
				free(d_tilde);
				free(pathMetric);
				free(crcState);
				nr_free_uint8_t_3D_array(bit, polarParams->N, (polarParams->n+1));
				nr_free_double_3D_array(llr, polarParams->N, (polarParams->n+1));
				nr_free_uint8_t_2D_array(crcChecksum, polarParams->crcParityBits);
				return(-1);
			}

			nonFrozenBit++;
			decoderIterationCheck=0;
			checkCrcBits=-1;
		}
	}

	for (uint8_t i = 0; i < 2*listSize; i++) listIndex[i]=i;
	nr_sort_asc_double_1D_array_ind(pathMetric, listIndex, currentListSize);

	for (uint8_t i = 0; i < fmin(listSize, (pow(2,polarParams->crcCorrectionBits)) ); i++) {
		if ( crcState[listIndex[i]] == 1 ) {
			for (int j = 0; j < polarParams->N; j++) polarParams->nr_polar_U[j]=bit[j][0][listIndex[i]];

			//Extract the information bits (û to ĉ)
			nr_polar_info_bit_extraction(polarParams->nr_polar_U, polarParams->nr_polar_CPrime, polarParams->information_bit_pattern, polarParams->N);

			//Deinterleaving (ĉ to b)
			nr_polar_deinterleaver(polarParams->nr_polar_CPrime, polarParams->nr_polar_B, polarParams->interleaving_pattern, polarParams->K);

			//Remove the CRC (â)
			for (int j = 0; j < polarParams->payloadBits; j++) polarParams->nr_polar_A[j]=polarParams->nr_polar_B[j];

			break;
		}
	}

	free(d_tilde);
	free(pathMetric);
	free(crcState);
	nr_free_uint8_t_3D_array(bit, polarParams->N, (polarParams->n+1));
	nr_free_double_3D_array(llr, polarParams->N, (polarParams->n+1));
	nr_free_uint8_t_2D_array(crcChecksum, polarParams->crcParityBits);
	nr_free_uint8_t_2D_array(extended_crc_generator_matrix, polarParams->K);
	nr_free_uint8_t_2D_array(tempECGM, polarParams->K);

	/*
	 * Return bits.
	 */
	nr_byte2bit_uint8_32_t(polarParams->nr_polar_A, polarParams->payloadBits, out);
	return(0);
}


int8_t polar_decoder_int16(int16_t *input,
			   uint8_t *out,
			   t_nrPolar_params *polarParams)
{
  

  
  int16_t d_tilde[polarParams->N];// = malloc(sizeof(double) * polarParams->N);
  nr_polar_rate_matching_int16(input, d_tilde, polarParams->rate_matching_pattern, polarParams->K, polarParams->N, polarParams->encoderLength);
  for (int i=0;i<polarParams->N;i++) {
    if (d_tilde[i]<-128) d_tilde[i]=-128;
    else if (d_tilde[i]>127) d_tilde[i]=128;
  }
  memcpy((void*)&polarParams->tree.root->alpha[0],(void*)&d_tilde[0],sizeof(int16_t)*polarParams->N);
  
  /*
   * SCL polar decoder.
   */


  generic_polar_decoder(polarParams,polarParams->tree.root);


  //Extract the information bits (û to ĉ)
  nr_polar_info_bit_extraction(polarParams->nr_polar_U, polarParams->nr_polar_CPrime, polarParams->information_bit_pattern, polarParams->N);

  //Deinterleaving (ĉ to b)

  nr_polar_deinterleaver(polarParams->nr_polar_CPrime, polarParams->nr_polar_B, polarParams->interleaving_pattern, polarParams->K);

  //Remove the CRC (â)
  //for (int j = 0; j < polarParams->payloadBits; j++) polarParams->nr_polar_A[j]=polarParams->nr_polar_B[j];  

  // Check the CRC
  for (int j=0;j<polarParams->crcParityBits;j++) {
    int crcbit=0;
    for (int i=0;i<polarParams->payloadBits;i++)
      crcbit = crcbit ^ (polarParams->crc_generator_matrix[i][j] & polarParams->nr_polar_B[i]);
    if (crcbit != polarParams->nr_polar_B[polarParams->payloadBits+j]) return(-1); 
  }
  // pack into ceil(payloadBits/32) 32 bit words, lowest index in MSB
  //  nr_byte2bit_uint8_32_t(polarParams->nr_polar_A, polarParams->payloadBits, out);
  nr_byte2bit_uint8_32_t(polarParams->nr_polar_B, polarParams->payloadBits, out);
  return(0);
}
