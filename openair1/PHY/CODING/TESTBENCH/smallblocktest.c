#include <getopt.h>
#include "SIMULATION/TOOLS/sim.h"
#include "PHY/CODING/nrSmallBlock/nr_small_block_defs.h"
#include "coding_unitary_defs.h"

//#define DEBUG_SMALLBLOCKTEST


int main(int argc, char *argv[])
{
	time_stats_t timeEncoder,timeDecoder;
	opp_enabled=1;
	reset_meas(&timeEncoder);
	reset_meas(&timeDecoder);
	randominit(0);

	int arguments, iterations = 1000, messageLength = 11;
	//int matlabDebug = 0;
	uint32_t testInput, encoderOutput, codingDifference, nBitError=0, blockErrorState = 0, blockErrorCumulative=0, bitErrorCumulative=0;
	uint16_t estimatedOutput;
	double SNRstart = -20.0, SNRstop = 5.0, SNRinc= 0.5; //dB
	double SNR, SNR_lin, sigma;
	double modulatedInput[NR_SMALL_BLOCK_CODED_BITS], channelOutput[NR_SMALL_BLOCK_CODED_BITS];
	//int16_t channelOutput_int16[NR_SMALL_BLOCK_CODED_BITS];
	int8_t channelOutput_int8[NR_SMALL_BLOCK_CODED_BITS];
	unsigned char qbits=8;

	while ((arguments = getopt (argc, argv, "s:d:f:l:i:mhg")) != -1)
	switch (arguments)
	{
		case 's':
			SNRstart = atof(optarg);
			break;

		case 'd':
			SNRinc = atof(optarg);
			break;

		case 'f':
			SNRstop = atof(optarg);
			break;

		case 'l':
			messageLength = atoi(optarg);
			break;

		case 'i':
			iterations = atoi(optarg);
			break;

		/*case 'm':
			matlabDebug = 1;
			//#define DEBUG_POLAR_MATLAB
			break;*/

		case 'g':
			iterations = 1;
			SNRstart = -6.0;
			SNRstop = -6.0;
			messageLength = 11;
			break;

		case 'h':
			//printf("./smallblocktest -s SNRstart -d SNRinc -f SNRstop -l messageLength -i iterations -m Matlab Debug\n");
			printf("./smallblocktest -s SNRstart -d SNRinc -f SNRstop -l messageLength -i iterations\n");
			exit(-1);

		default:
			perror("[smallblocktest.c] Problem at argument parsing with getopt");
			exit(-1);
	}


	uint16_t mask = 0x07ff >> (11-messageLength);
	for (SNR = SNRstart; SNR <= SNRstop; SNR += SNRinc) {
		printf("SNR %f\n",SNR);
		SNR_lin = pow(10, SNR/10.0);
		sigma = 1.0/sqrt(SNR_lin);

		for (int itr = 1; itr <= iterations; itr++) {

			//Generate random test input of length "messageLength"
			testInput = 0;
			for (int i = 1; i < messageLength; i++) {
				testInput |= ( ((uint32_t) (rand()%2)) &1);
				testInput<<=1;
				}
			testInput |= ( ((uint32_t) (rand()%2)) &1);
			//Encoding
			start_meas(&timeEncoder);
			encoderOutput = encodeSmallBlock((uint16_t*)&testInput, (uint8_t)messageLength);
			stop_meas(&timeEncoder);

			for (int i=0; i<NR_SMALL_BLOCK_CODED_BITS; i++) {
				//BPSK modulation
				if ((encoderOutput>>i) & 1 ) {
					modulatedInput[i]=-1;
				} else {
					modulatedInput[i]=1;
				}

				//AWGN
				channelOutput[i] = modulatedInput[i] + ( gaussdouble(0.0,1.0) * ( 1/sqrt(SNR_lin) ) );

				//Quantization
				channelOutput_int8[i] = quantize(sigma/16.0, channelOutput[i], qbits);
			}

			//Decoding
			start_meas(&timeDecoder);
			estimatedOutput = decodeSmallBlock(channelOutput_int8, (uint8_t)messageLength);
			stop_meas(&timeDecoder);

#ifdef DEBUG_SMALLBLOCKTEST
			printf("[smallblocktest] Input = 0x%x, Output = 0x%x, DecoderOutput = 0x%x\n", testInput, encoderOutput, estimatedOutput);
			for (int i=0;i<32;i++)
				printf("[smallblocktest] Input[%d] = %d, Output[%d] = %d, codingDifference[%d]=%d, Mask[%d] = %d\n", i, (testInput>>i)&1, i, (estimatedOutput>>i)&1, i, (codingDifference>>i)&1, i, (mask>>i)&1);
#endif

			//Error Calculation
			estimatedOutput &= mask;
			codingDifference = ((uint32_t)estimatedOutput) ^ testInput; // Count the # of 1's in codingDifference by Brian Kernighan’s algorithm.

			for (nBitError = 0; codingDifference; nBitError++)
				codingDifference &= codingDifference - 1;

			blockErrorState = (nBitError > 0) ? 1 : 0;

			blockErrorCumulative+=blockErrorState;
			bitErrorCumulative+=nBitError;

			nBitError = 0; blockErrorState = 0;
		}

		//Error statistics for the SNR; iteration times are in nanoseconds and microseconds, respectively.
		printf("[smallblocktest] SNR=%+7.3f, BER=%9.6f, BLER=%9.6f, t_Encoder=%9.3fns, t_Decoder=%7.3fus\n",
				SNR,
				((double)bitErrorCumulative / (iterations*messageLength)),
				((double)blockErrorCumulative/iterations),
				((double)timeEncoder.diff/timeEncoder.trials)/(get_cpu_freq_GHz()),
				((double)timeDecoder.diff/timeDecoder.trials)/(get_cpu_freq_GHz()*1000.0));

		blockErrorCumulative=0;
		bitErrorCumulative=0;
	}

	print_meas(&timeEncoder, "smallblock_encoder", NULL, NULL);
	print_meas(&timeDecoder, "smallblock_decoder", NULL, NULL);

	return (0);
}
