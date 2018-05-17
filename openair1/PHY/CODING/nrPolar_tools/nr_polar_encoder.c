#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_pbch_defs.h"

void polar_encoder(uint8_t *input, uint8_t *channel_input, t_nrPolar_params *polarParams){
	/*//Create the Transport Block.
	unsigned int payload=0xb3f02c82;
	uint8_t pbchTransportBlockSize = ( polarParams->K / (8*sizeof(uint8_t)) );
	uint8_t *pbchTransportBlock = malloc(pbchTransportBlockSize);
	memcpy(pbchTransportBlock,&payload,sizeof(payload));

	//Attach CRC to the Transport Block. (a to b)
	uint32_t crc = crc24c(&payload, NR_POLAR_PBCH_PAYLOAD_BITS)>>8;
	pbchTransportBlock[NR_POLAR_PBCH_PAYLOAD_BITS>>3] = ((uint8_t*)&crc)[2];
	pbchTransportBlock[1+(NR_POLAR_PBCH_PAYLOAD_BITS>>3)] = ((uint8_t*)&crc)[1];
	pbchTransportBlock[2+(NR_POLAR_PBCH_PAYLOAD_BITS>>3)] = ((uint8_t*)&crc)[0];*/

	/*
	 * Bytewise operations
	 */

	//Calculate CRC.
	uint8_t *nr_polar_crc = malloc(sizeof(uint8_t) * polarParams->crcParityBits);
	nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(input, polarParams->crc_generator_matrix,
			nr_polar_crc, polarParams->payloadBits, polarParams->crcParityBits);
	for (uint8_t i = 0; i < polarParams->crcParityBits; i++) nr_polar_crc[i] = (nr_polar_crc[i] % 2);

	//Attach CRC to the Transport Block. (a to b)
	uint8_t *nr_polar_b = malloc(sizeof(uint8_t) * polarParams->K);
	for (uint16_t i = 0; i < polarParams->payloadBits; i++) nr_polar_b[i] = input[i];
	for (uint16_t i = polarParams->payloadBits; i < polarParams->K; i++) nr_polar_b[i]= nr_polar_crc[i-(polarParams->payloadBits)];

	//Interleaving (c to c')
	uint8_t *nr_polar_cPrime = malloc(sizeof(uint8_t) * polarParams->K);
	nr_polar_interleaver(nr_polar_b, nr_polar_cPrime, polarParams->interleaving_pattern, polarParams->K);

	//Bit insertion (c' to u)
	uint8_t *nr_polar_u = malloc(sizeof(uint8_t) * polarParams->N);
	nr_polar_bit_insertion(nr_polar_cPrime, nr_polar_u, polarParams->N, polarParams->K,
			polarParams->Q_I_N, polarParams->Q_PC_N, polarParams->n_pc);

	//Encoding (u to d)
	uint8_t *pbch_polar_encoder_output = malloc(sizeof(uint8_t) * polarParams->N);
	nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(nr_polar_u, polarParams->G_N, pbch_polar_encoder_output, polarParams->N, polarParams->N);
	for (uint16_t i = 0; i < polarParams->N; i++) pbch_polar_encoder_output[i] = (pbch_polar_encoder_output[i] % 2);

	//Rate matching
	//Sub-block interleaving (d to y) and Bit selection (y to e)
	nr_polar_rate_matcher(pbch_polar_encoder_output, channel_input, polarParams->rate_matching_pattern, polarParams->encoderLength);

	//free(pbchTransportBlock);
	free(nr_polar_crc);
	free(nr_polar_b);
	free(nr_polar_cPrime);
	free(nr_polar_u);
	free(pbch_polar_encoder_output);
}
