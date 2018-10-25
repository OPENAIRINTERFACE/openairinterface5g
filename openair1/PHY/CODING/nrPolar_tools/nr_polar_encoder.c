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

//#define DEBUG_POLAR_ENCODER
//#define DEBUG_POLAR_ENCODER_DCI
//#define DEBUG_POLAR_ENCODER_TIMING

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

//input  [a_31 a_30 ... a_0]
//output [f_31 f_30 ... f_0] [f_63 f_62 ... f_32] ...

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
#ifdef DEBUG_POLAR_ENCODER
	for (int i=0; i< polarParams->encoderLength;i++) printf("f[%d]=%d\n", i, polarParams->nr_polar_E[i]);
#endif

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

	/*
	 * Bytewise operations
	 */
	//(a to a')
	nr_bit2byte_uint32_8_t(in, polarParams->payloadBits, polarParams->nr_polar_A);
	for (int i=0; i<polarParams->crcParityBits; i++) polarParams->nr_polar_APrime[i]=1;
	for (int i=0; i<polarParams->payloadBits; i++) polarParams->nr_polar_APrime[i+(polarParams->crcParityBits)]=polarParams->nr_polar_A[i];
#ifdef DEBUG_POLAR_ENCODER_DCI
	printf("[polar_encoder_dci] A: ");
	for (int i=0; i<polarParams->payloadBits; i++) printf("%d-", polarParams->nr_polar_A[i]);
	printf("\n");
	printf("[polar_encoder_dci] APrime: ");
	for (int i=0; i<polarParams->K; i++) printf("%d-", polarParams->nr_polar_APrime[i]);
	printf("\n");
	printf("[polar_encoder_dci] GP: ");
	for (int i=0; i<polarParams->crcParityBits; i++) printf("%d-", polarParams->crc_generator_matrix[0][i]);
	printf("\n");
#endif
	//Calculate CRC.
	nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_APrime,
												   polarParams->crc_generator_matrix,
												   polarParams->nr_polar_crc,
												   polarParams->K,
												   polarParams->crcParityBits);
	for (uint8_t i = 0; i < polarParams->crcParityBits; i++) polarParams->nr_polar_crc[i] = (polarParams->nr_polar_crc[i] % 2);
#ifdef DEBUG_POLAR_ENCODER_DCI
	printf("[polar_encoder_dci] CRC: ");
	for (int i=0; i<polarParams->crcParityBits; i++) printf("%d-", polarParams->nr_polar_crc[i]);
	printf("\n");
#endif

	//Attach CRC to the Transport Block. (a to b)
	for (uint16_t i = 0; i < polarParams->payloadBits; i++)
		polarParams->nr_polar_B[i] = polarParams->nr_polar_A[i];
	for (uint16_t i = polarParams->payloadBits; i < polarParams->K; i++)
		polarParams->nr_polar_B[i]= polarParams->nr_polar_crc[i-(polarParams->payloadBits)];
	//Scrambling (b to c)
	for (int i=0; i<16; i++) {
		polarParams->nr_polar_B[polarParams->payloadBits+8+i] =
				( polarParams->nr_polar_B[polarParams->payloadBits+8+i] + ((n_RNTI>>(15-i))&1) ) % 2;
	}

/*	//(a to a')
	nr_crc_bit2bit_uint32_8_t(in, polarParams->payloadBits, polarParams->nr_polar_aPrime);
	//Parity bits computation (p)
	polarParams->crcBit = crc24c(polarParams->nr_polar_aPrime, (polarParams->payloadBits+polarParams->crcParityBits));
#ifdef DEBUG_POLAR_ENCODER_DCI
	printf("[polar_encoder_dci] crc: 0x%08x\n", polarParams->crcBit);
	for (int i=0; i<32; i++)
	{
		printf("%d\n",((polarParams->crcBit)>>i)&1);
	}
#endif
	//(a to b)
	//
	// Bytewise operations
	//
	uint8_t arrayInd = ceil(polarParams->payloadBits / 8.0);
	for (int i=0; i<arrayInd-1; i++){
		for (int j=0; j<8; j++) {
			polarParams->nr_polar_B[j+(i*8)] = ((polarParams->nr_polar_aPrime[3+i]>>(7-j)) & 1);
		}
	}
	for (int i=0; i<((polarParams->payloadBits)%8); i++) {
			polarParams->nr_polar_B[i+(arrayInd-1)*8] = ((polarParams->nr_polar_aPrime[3+(arrayInd-1)]>>(7-i)) & 1);
	}
	for (int i=0; i<8; i++) {
		polarParams->nr_polar_B[polarParams->payloadBits+i] = ((polarParams->crcBit)>>(31-i))&1;
	}
	//Scrambling (b to c)
	for (int i=0; i<16; i++) {
		polarParams->nr_polar_B[polarParams->payloadBits+8+i] =
				( (((polarParams->crcBit)>>(23-i))&1) + ((n_RNTI>>(15-i))&1) ) % 2;
	}*/
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
	printf("[polar_encoder_dci] E: ");
	for (int i = 0; i < polarParams->encoderLength; i++) printf("%d-", polarParams->nr_polar_E[i]);
	uint8_t outputInd = ceil(polarParams->encoderLength / 32.0);
	printf("\n[polar_encoder_dci] out: ");
	for (int i = 0; i < outputInd; i++) {
		printf("[%d]->0x%08x\t", i, out[i]);
	}
#endif
}

void polar_encoder_timing(uint32_t *in,
						  uint32_t *out,
						  t_nrPolar_paramsPtr polarParams,
						  double cpuFreqGHz,
						  FILE* logFile)
{
	//Initiate timing.
	time_stats_t timeEncoderCRCByte, timeEncoderCRCBit, timeEncoderInterleaver, timeEncoderBitInsertion, timeEncoder1, timeEncoder2, timeEncoderRateMatching, timeEncoderByte2Bit;
	reset_meas(&timeEncoderCRCByte); reset_meas(&timeEncoderCRCBit); reset_meas(&timeEncoderInterleaver); reset_meas(&timeEncoderBitInsertion); reset_meas(&timeEncoder1); reset_meas(&timeEncoder2); reset_meas(&timeEncoderRateMatching); reset_meas(&timeEncoderByte2Bit);
	uint16_t n_RNTI=0x0000;

	start_meas(&timeEncoderCRCByte);
	nr_crc_bit2bit_uint32_8_t(in, polarParams->payloadBits, polarParams->nr_polar_aPrime); //(a to a')
	polarParams->crcBit = crc24c(polarParams->nr_polar_aPrime, (polarParams->payloadBits+polarParams->crcParityBits)); //Parity bits computation (p)
	uint8_t arrayInd = ceil(polarParams->payloadBits / 8.0); //(a to b)
	for (int i=0; i<arrayInd-1; i++)
		for (int j=0; j<8; j++)
			polarParams->nr_polar_B[j+(i*8)] = ((polarParams->nr_polar_aPrime[3+i]>>(7-j)) & 1);
	for (int i=0; i<((polarParams->payloadBits)%8); i++) polarParams->nr_polar_B[i+(arrayInd-1)*8] = ((polarParams->nr_polar_aPrime[3+(arrayInd-1)]>>(7-i)) & 1);
	for (int i=0; i<8; i++) polarParams->nr_polar_B[polarParams->payloadBits+i] = ((polarParams->crcBit)>>(31-i))&1;
	for (int i=0; i<16; i++) polarParams->nr_polar_B[polarParams->payloadBits+8+i] =	( (((polarParams->crcBit)>>(23-i))&1) + ((n_RNTI>>(15-i))&1) ) % 2; //Scrambling (b to c)
	stop_meas(&timeEncoderCRCByte);


	start_meas(&timeEncoderCRCBit);
	nr_bit2byte_uint32_8_t(in, polarParams->payloadBits, polarParams->nr_polar_A);
	nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_A, polarParams->crc_generator_matrix, polarParams->nr_polar_crc, polarParams->payloadBits, polarParams->crcParityBits); //Calculate CRC.
	for (uint8_t i = 0; i < polarParams->crcParityBits; i++) polarParams->nr_polar_crc[i] = (polarParams->nr_polar_crc[i] % 2);
	for (uint16_t i = 0; i < polarParams->payloadBits; i++) polarParams->nr_polar_B[i] = polarParams->nr_polar_A[i]; //Attach CRC to the Transport Block. (a to b)
	for (uint16_t i = polarParams->payloadBits; i < polarParams->K; i++) polarParams->nr_polar_B[i]= polarParams->nr_polar_crc[i-(polarParams->payloadBits)];
	stop_meas(&timeEncoderCRCBit);


	start_meas(&timeEncoderInterleaver); //Interleaving (c to c')
	nr_polar_interleaver(polarParams->nr_polar_B, polarParams->nr_polar_CPrime, polarParams->interleaving_pattern, polarParams->K);
	stop_meas(&timeEncoderInterleaver);


	start_meas(&timeEncoderBitInsertion); //Bit insertion (c' to u)
	nr_polar_bit_insertion(polarParams->nr_polar_CPrime, polarParams->nr_polar_U, polarParams->N, polarParams->K, polarParams->Q_I_N, polarParams->Q_PC_N, polarParams->n_pc);
	stop_meas(&timeEncoderBitInsertion);


	start_meas(&timeEncoder1); //Encoding (u to d)
	nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_U, polarParams->G_N, polarParams->nr_polar_D, polarParams->N, polarParams->N);
	stop_meas(&timeEncoder1);


	start_meas(&timeEncoder2);
	for (uint16_t i = 0; i < polarParams->N; i++) polarParams->nr_polar_D[i] = (polarParams->nr_polar_D[i] % 2);
	stop_meas(&timeEncoder2);


	start_meas(&timeEncoderRateMatching);//Rate matching //Sub-block interleaving (d to y) and Bit selection (y to e)
	nr_polar_interleaver(polarParams->nr_polar_D, polarParams->nr_polar_E, polarParams->rate_matching_pattern, polarParams->encoderLength);
	stop_meas(&timeEncoderRateMatching);


	start_meas(&timeEncoderByte2Bit); //Return bits.
	nr_byte2bit_uint8_32_t(polarParams->nr_polar_E, polarParams->encoderLength, out);
	stop_meas(&timeEncoderByte2Bit);

	fprintf(logFile,",%f,%f,%f,%f,%f,%f,%f,%f\n",
			(timeEncoderCRCByte.diff/(cpuFreqGHz*1000.0)),
			(timeEncoderCRCBit.diff/(cpuFreqGHz*1000.0)),
			(timeEncoderInterleaver.diff/(cpuFreqGHz*1000.0)),
			(timeEncoderBitInsertion.diff/(cpuFreqGHz*1000.0)),
			(timeEncoder1.diff/(cpuFreqGHz*1000.0)),
			(timeEncoder2.diff/(cpuFreqGHz*1000.0)),
			(timeEncoderRateMatching.diff/(cpuFreqGHz*1000.0)),
			(timeEncoderByte2Bit.diff/(cpuFreqGHz*1000.0)));
}
