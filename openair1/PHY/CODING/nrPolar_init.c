#include "nrPolar_tools/nr_polar_defs.h"
#include "nrPolar_tools/nr_polar_pbch_defs.h"

void nr_polar_init(t_nrPolar_params* polarParams, int messageType) {

	if (messageType == 0) { //DCI

	} else if (messageType == 1) { //PBCH
		polarParams->n_max = NR_POLAR_PBCH_N_MAX;
		polarParams->i_il = NR_POLAR_PBCH_I_IL;
		polarParams->n_pc = NR_POLAR_PBCH_N_PC;
		polarParams->n_pc_wm = NR_POLAR_PBCH_N_PC_WM;
		polarParams->i_bil = NR_POLAR_PBCH_I_BIL;
		polarParams->payloadBits = NR_POLAR_PBCH_PAYLOAD_BITS;
		polarParams->encoderLength = NR_POLAR_PBCH_E;
		polarParams->crcParityBits = NR_POLAR_PBCH_CRC_PARITY_BITS;
		polarParams->crcCorrectionBits = NR_POLAR_PBCH_CRC_ERROR_CORRECTION_BITS;

		polarParams->K = polarParams->payloadBits + polarParams->crcParityBits; // Number of bits to encode.
		polarParams->N = nr_polar_output_length(polarParams->K, polarParams->encoderLength, polarParams->n_max);
		polarParams->n = log2(polarParams->N);

		polarParams->crc_generator_matrix=crc24c_generator_matrix(polarParams->payloadBits);
		polarParams->G_N = nr_polar_kronecker_power_matrices(polarParams->n);
	} else if (messageType == 2) { //UCI

	}

	polarParams->Q_0_Nminus1 = nr_polar_sequence_pattern(polarParams->n);

	polarParams->interleaving_pattern = (uint16_t *)malloc(sizeof(uint16_t) * polarParams->K);
	nr_polar_interleaving_pattern(polarParams->K, polarParams->i_il, polarParams->interleaving_pattern);

	polarParams->rate_matching_pattern = (uint16_t *)malloc(sizeof(uint16_t) * polarParams->encoderLength);
	uint16_t *J = malloc(sizeof(uint16_t) * polarParams->N);
	nr_polar_rate_matching_pattern(polarParams->rate_matching_pattern, J, nr_polar_subblock_interleaver_pattern,
			polarParams->K, polarParams->N, polarParams->encoderLength);


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

	free(J);
}
