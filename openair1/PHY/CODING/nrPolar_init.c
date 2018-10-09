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

#include "nrPolar_tools/nr_polar_defs.h"
#include "nrPolar_tools/nr_polar_pbch_defs.h"
#include "nrPolar_tools/nr_polar_uci_defs.h"

void nr_polar_init(t_nrPolar_params* polarParams, int messageType) {

uint32_t poly6 = 0x84000000; // 1000100000... -> D^6+D^5+1
uint32_t poly11 = 0x63200000; //11000100001000... -> D^11+D^10+D^9+D^5+1
uint32_t poly16 = 0x81080000; //100000010000100... - > D^16+D^12+D^5+1
uint32_t poly24a = 0x864cfb00; //100001100100110011111011 -> D^24+D^23+D^18+D^17+D^14+D^11+D^10+D^7+D^6+D^5+D^4+D^3+D+1
uint32_t poly24b = 0x80006300; //100000000000000001100011 -> D^24+D^23+D^6+D^5+D+1
uint32_t poly24c = 0xB2B11700; //101100101011000100010111 -> D^24...

	if (messageType == 0) { //DCI

	} else if (messageType == 1) { //PBCH
		polarParams->n_max = NR_POLAR_PBCH_N_MAX;
		polarParams->i_il = NR_POLAR_PBCH_I_IL;
		polarParams->i_seg = NR_POLAR_PBCH_I_SEG;
		polarParams->n_pc = NR_POLAR_PBCH_N_PC;
		polarParams->n_pc_wm = NR_POLAR_PBCH_N_PC_WM;
		polarParams->i_bil = NR_POLAR_PBCH_I_BIL;
		polarParams->payloadBits = NR_POLAR_PBCH_PAYLOAD_BITS;
		polarParams->encoderLength = NR_POLAR_PBCH_E;
		polarParams->crcParityBits = NR_POLAR_PBCH_CRC_PARITY_BITS;

		polarParams->K = polarParams->payloadBits + polarParams->crcParityBits; // Number of bits to encode.
		polarParams->N = nr_polar_output_length(polarParams->K, polarParams->encoderLength, polarParams->n_max);
		polarParams->n = log2(polarParams->N);

		polarParams->crc_generator_matrix=crc24c_generator_matrix(polarParams->payloadBits);
		polarParams->crc_polynomial = poly24c;
		polarParams->G_N = nr_polar_kronecker_power_matrices(polarParams->n);

		//polar_encoder vectors:
		polarParams->nr_polar_crc = malloc(sizeof(uint8_t) * polarParams->crcParityBits);
		polarParams->nr_polar_cPrime = malloc(sizeof(uint8_t) * polarParams->K);
		polarParams->nr_polar_d = malloc(sizeof(uint8_t) * polarParams->N);

		//Polar Coding vectors
		polarParams->nr_polar_u = malloc(sizeof(uint8_t) * polarParams->N); //Decoder: nr_polar_uHat
		polarParams->nr_polar_cPrime = malloc(sizeof(uint8_t) * polarParams->K); //Decoder: nr_polar_cHat
		polarParams->nr_polar_b = malloc(sizeof(uint8_t) * polarParams->K); //Decoder: nr_polar_bHat

		polarParams->decoder_kernel = NULL;//polar_decoder_K56_N512_E864;



	} else if (messageType == 2) { //UCI
		polarParams->payloadBits = NR_POLAR_PUCCH_PAYLOAD_BITS;	//A depends on what they carry...
		polarParams->encoderLength = NR_POLAR_PUCCH_E ; //E depends on other standards 6.3.1.4
	
	        if (polarParams->payloadBits <= 11) //Ref. 38-212, Section 6.3.1.2.2
			polarParams->crcParityBits = 0; //K=A
       		else //Ref. 38-212, Section 6.3.1.2.1
		{
	 	        if (polarParams->payloadBits < 20)
                		polarParams->crcParityBits = NR_POLAR_PUCCH_CRC_PARITY_BITS_SHORT;
            		else
                		polarParams->crcParityBits =  NR_POLAR_PUCCH_CRC_PARITY_BITS_LONG;

            		if (polarParams->payloadBits >= 360 && polarParams->encoderLength >= 1088)
		                polarParams->i_seg = NR_POLAR_PUCCH_I_SEG_LONG; // -> C=2
		        else
		                polarParams->i_seg = NR_POLAR_PUCCH_I_SEG_SHORT; // -> C=1
	        }

	        polarParams->K = polarParams->payloadBits + polarParams->crcParityBits; // Number of bits to encode.

	        //K_r = K/C ; C = I_seg+1
	        if((polarParams->K)/(polarParams->i_seg+1)>=18 && (polarParams->K)/(polarParams->i_seg+1)<=25) //Ref. 38-212, Section 6.3.1.3.1
	        {
		        polarParams->n_max = NR_POLAR_PUCCH_N_MAX;
		        polarParams->i_il =NR_POLAR_PUCCH_I_IL;
		        polarParams->n_pc = NR_POLAR_PUCCH_N_PC_SHORT;

            		if( (polarParams->encoderLength - polarParams->K)/(polarParams->i_seg + 1) + 3 > 192 )
		                polarParams->n_pc_wm = NR_POLAR_PUCCH_N_PC_WM_LONG;
            		else
		                polarParams->n_pc_wm = NR_POLAR_PUCCH_N_PC_WM_SHORT;
		}

	        if( (polarParams->K)/(polarParams->i_seg + 1) > 30 ) //Ref. 38-212, Section 6.3.1.3.1
	        {
		        polarParams->n_max = NR_POLAR_PUCCH_N_MAX;
                        polarParams->i_il =NR_POLAR_PUCCH_I_IL;
		        polarParams->n_pc = NR_POLAR_PUCCH_N_PC_LONG;
		        polarParams->n_pc_wm = NR_POLAR_PUCCH_N_PC_WM_LONG;
        	}

	        polarParams->i_bil = NR_POLAR_PUCCH_I_BIL; //Ref. 38-212, Section 6.3.1.4.1

		polarParams->N = nr_polar_output_length(polarParams->K, polarParams->encoderLength, polarParams->n_max);
		polarParams->n = log2(polarParams->N);

		if((polarParams->payloadBits) <= 19)
		{
			polarParams->crc_generator_matrix=crc6_generator_matrix(polarParams->payloadBits);
			polarParams->crc_polynomial = poly6;

		}
		else
		{
			polarParams->crc_generator_matrix=crc11_generator_matrix(polarParams->payloadBits); 
			polarParams->crc_polynomial = poly11;
		}
		polarParams->G_N = nr_polar_kronecker_power_matrices(polarParams->n);

                //polar_encoder vectors:
                polarParams->nr_polar_crc = malloc(sizeof(uint8_t) * polarParams->crcParityBits);
                polarParams->nr_polar_cPrime = malloc(sizeof(uint8_t) * polarParams->K);
                polarParams->nr_polar_d = malloc(sizeof(uint8_t) * polarParams->N);

                //Polar Coding vectors
                polarParams->nr_polar_u = malloc(sizeof(uint8_t) * polarParams->N); //Decoder: nr_polar_uHat
                polarParams->nr_polar_cPrime = malloc(sizeof(uint8_t) * polarParams->K); //Decoder: nr_polar_cHat
                polarParams->nr_polar_b = malloc(sizeof(uint8_t) * polarParams->K); //Decoder: nr_polar_bHat

	}

	polarParams->crcCorrectionBits = NR_POLAR_CRC_ERROR_CORRECTION_BITS;

	polarParams->crc256Table = malloc(sizeof(uint32_t)*256);	
	crcTable256Init(polarParams->crc_polynomial, polarParams->crc256Table);
	polarParams->Q_0_Nminus1 = nr_polar_sequence_pattern(polarParams->n);

	polarParams->interleaving_pattern = malloc(sizeof(uint16_t) * polarParams->K);
	nr_polar_interleaving_pattern(polarParams->K, polarParams->i_il, polarParams->interleaving_pattern);

	polarParams->rate_matching_pattern = malloc(sizeof(uint16_t) * polarParams->encoderLength);
	uint16_t *J = malloc(sizeof(uint16_t) * polarParams->N);
	nr_polar_rate_matching_pattern(polarParams->rate_matching_pattern, J,
		nr_polar_subblock_interleaver_pattern, polarParams->K, polarParams->N, polarParams->encoderLength);


	polarParams->information_bit_pattern = malloc(sizeof(uint8_t) * polarParams->N);
	polarParams->Q_I_N = malloc(sizeof(int16_t) * (polarParams->K + polarParams->n_pc));
	polarParams->Q_F_N = malloc(sizeof(int16_t) * (polarParams->N+1)); // Last element shows the final array index assigned a value.
	polarParams->Q_PC_N = malloc(sizeof(int16_t) * (polarParams->n_pc));
	for (int i=0; i<=polarParams->N; i++) polarParams->Q_F_N[i] = -1; // Empty array.
	nr_polar_info_bit_pattern(polarParams->information_bit_pattern,
			polarParams->Q_I_N, polarParams->Q_F_N, J, polarParams->Q_0_Nminus1,
			polarParams->K, polarParams->N, polarParams->encoderLength, polarParams->n_pc);

	polarParams->channel_interleaver_pattern = malloc(sizeof(uint16_t) * polarParams->encoderLength);
	nr_polar_channel_interleaver_pattern(polarParams->channel_interleaver_pattern,
			polarParams->i_bil, polarParams->encoderLength);

	polarParams->extended_crc_generator_matrix = malloc(polarParams->K * sizeof(uint8_t *)); //G_P3
	uint8_t tempECGM[polarParams->K][polarParams->crcParityBits];
	for (int i = 0; i < polarParams->K; i++){
	  polarParams->extended_crc_generator_matrix[i] = malloc(polarParams->crcParityBits * sizeof(uint8_t));
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
	    polarParams->extended_crc_generator_matrix[i][j]=tempECGM[polarParams->interleaving_pattern[i]][j];
	  }
	}


	build_decoder_tree(polarParams);
	printf("decoder tree nodes %d\n",polarParams->tree.num_nodes);
	

	
	free(J);
}
