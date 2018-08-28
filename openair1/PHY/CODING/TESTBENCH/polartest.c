#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"
#include "PHY/CODING/coding_defs.h"
#include "SIMULATION/TOOLS/sim.h"

//#define DEBUG_POLAR_PARAMS
//#define DEBUG_DCI_POLAR_PARAMS

int main(int argc, char *argv[]) {

	//Initiate timing. (Results depend on CPU Frequency. Therefore, might change due to performance variances during simulation.)
	time_stats_t timeEncoder,timeDecoder;
	opp_enabled=1;
	cpu_freq_GHz = get_cpu_freq_GHz();
	reset_meas(&timeEncoder);
	reset_meas(&timeDecoder);

	randominit(0);
	crcTableInit();
	uint32_t crc;
	//Default simulation values (Aim for iterations = 1000000.)
	int itr, iterations = 1000, arguments, polarMessageType = 0; //0=PBCH, 1=DCI, -1=UCI
	double SNRstart = -20.0, SNRstop = 0.0, SNRinc= 0.5; //dB

	double SNR, SNR_lin;
	int16_t nBitError = 0; // -1 = Decoding failed (All list entries have failed the CRC checks).
	int8_t decoderState=0, blockErrorState=0; //0 = Success, -1 = Decoding failed, 1 = Block Error.
	uint16_t testLength = 0, coderLength = 0, blockErrorCumulative=0, bitErrorCumulative=0;
	double timeEncoderCumulative = 0, timeDecoderCumulative = 0;
	uint8_t aggregation_level, decoderListSize, pathMetricAppr;

	while ((arguments = getopt (argc, argv, "s:d:f:m:i:l:a:")) != -1)
	switch (arguments)
	{
		case 's':
			SNRstart = atof(optarg);
			printf("SNRstart = %f\n", SNRstart);
			break;

		case 'd':
			SNRinc = atof(optarg);
			break;

		case 'f':
			SNRstop = atof(optarg);
			break;

		case 'm':
			polarMessageType = atoi(optarg);
			break;

		case 'i':
			iterations = atoi(optarg);
			break;

		case 'l':
			decoderListSize = (uint8_t) atoi(optarg);
			break;

		case 'a':
			pathMetricAppr = (uint8_t) atoi(optarg);
			break;

		default:
			perror("[polartest.c] Problem at argument parsing with getopt");
			abort ();
	}

	if (polarMessageType == 0) { //PBCH
		testLength = NR_POLAR_PBCH_PAYLOAD_BITS;
		coderLength = NR_POLAR_PBCH_E;
		aggregation_level = NR_POLAR_PBCH_AGGREGATION_LEVEL;
	} else if (polarMessageType == 1) { //DCI
		//testLength = nr_get_dci_size(params_rel15->dci_format, params_rel15->rnti_type, &fp->initial_bwp_dl, cfg);
		testLength = 20;
		coderLength = 108; //to be changed by aggregate level function.
	} else if (polarMessageType == -1) { //UCI
		//testLength = ;
		//coderLength = ;
	}

	//Logging
	time_t currentTime;
	time (&currentTime);
	char *folderName, fileName[512], currentTimeInfo[25];

	folderName=getenv("HOME");
	strcat(folderName,"/Desktop/polartestResults");
	sprintf(fileName,"%s/_ListSize_%d_pmAppr_%d_Payload_%d_Itr_%d",folderName,decoderListSize,pathMetricAppr,testLength,iterations);
	strftime(currentTimeInfo, 25, "_%Y-%m-%d-%H-%M-%S.csv", localtime(&currentTime));
	strcat(fileName,currentTimeInfo);

	//Create "~/Desktop/polartestResults" folder if it doesn't already exist.
	struct stat folder = {0};
	if (stat(folderName, &folder) == -1) mkdir(folderName, S_IRWXU | S_IRWXG | S_IRWXO);

	FILE* logFile;
    logFile = fopen(fileName, "w");
    if (logFile==NULL) {
        fprintf(stderr,"[polartest.c] Problem creating file %s with fopen\n",fileName);
        exit(-1);
      }
    fprintf(logFile,",SNR,nBitError,blockErrorState,t_encoder[us],t_decoder[us]\n");

	//uint8_t *testInput = malloc(sizeof(uint8_t) * testLength); //generate randomly
	//uint8_t *encoderOutput = malloc(sizeof(uint8_t) * coderLength);
	uint32_t testInput[4], encoderOutput[4];
	memset(testInput,0,sizeof(testInput));
	memset(encoderOutput,0,sizeof(encoderOutput));

	double *modulatedInput = malloc (sizeof(double) * coderLength); //channel input

	double *channelOutput  = malloc (sizeof(double) * coderLength); //add noise
	uint32_t *estimatedOutput = malloc(sizeof(uint8_t) * testLength); //decoder output

	t_nrPolar_paramsPtr nrPolar_params = NULL, currentPtr = NULL;
	nr_polar_init(&nrPolar_params, polarMessageType, testLength, aggregation_level);
#ifdef DEBUG_DCI_POLAR_PARAMS
	unsigned int poly24c = 0xb2b11700;
	printf("testInput: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n",
			testInput[0], testInput[1], testInput[2], testInput[3]);
	printf("encOutput: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n",
			  encoderOutput[0], encoderOutput[1], encoderOutput[2], encoderOutput[3]);
	testInput[0]=0x01189400;
	uint8_t testInput2[8];
	nr_crc_bit2bit_uint32_8_t(testInput, 32, testInput2);
	printf("testInput2: [0]->%x \t [1]->%x \t [2]->%x \t [3]->%x\n            [4]->%x \t [5]->%x \t [6]->%x \t [7]->%x \t\n",
				testInput2[0], testInput2[1], testInput2[2], testInput2[3],
				testInput2[4], testInput2[5], testInput2[6], testInput2[7]);
	printf("crc32: [0]->0x%08x\n",crc24c(testInput2, 32));
	printf("crc56: [0]->0x%08x\n",crc24c(testInput2, 56));
	return 0;
	uint8_t testInput8[4];
	/*testInput8[0]=0x00;
	testInput8[1]=0x49;
	testInput8[2]=0x81;
	testInput8[3]=0x10;
	testInput8[4]=0x00;*/
	testInput8[0]=0xff;
	testInput8[1]=0xd0;
	testInput8[2]=0xff;
	testInput8[3]=0x82;
	crc = crc24c(testInput8, 31);
	for (int i=0;i<24;i++) printf("[i]=%d\n",(crc>>i)&1);
	printf("crc: [0]->0x%08x\n",crc);
	printf("crcbit: %x\n",crcbit(testInput8, 3, poly24c));
	return 0;
	unsigned char test[] = "Thebigredfox";

	for (int i=0;i<8;i++) printf("[i]=%d\n",(test[0]>>i)&1);
	printf("test[0]=%x\n",test[0]);
	printf("%s -- sizeof=%d\n",test,sizeof(test));
	printf("%x\n", crcbit(test, sizeof(test) - 1, poly24c));
	printf("%x\n", crc24c(test, (sizeof(test) - 1)*8));
	polarMessageType = 1;
	testLength = 41;
	aggregation_level=1;
	coderLength = 108;
	nr_polar_init(&nrPolar_params, polarMessageType, testLength, aggregation_level);
	nr_polar_print_polarParams(nrPolar_params);

    crc = crc24c(testInput, testLength)>>8;
    for (int i=0;i<24;i++) printf("[i]=%d\n",(crc>>i)&1);
    printf("crc: [0]->0x%08x\n",crc);
    testInput[testLength>>3] = ((uint8_t*)&crc)[2];
    testInput[1+(testLength>>3)] = ((uint8_t*)&crc)[1];
    testInput[2+(testLength>>3)] = ((uint8_t*)&crc)[0];
    printf("testInput: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n",
    			testInput[0], testInput[1], testInput[2], testInput[3]);
	return (0);
	currentPtr = nr_polar_params(nrPolar_params, polarMessageType, testLength, aggregation_level);
	polar_encoder(testInput, encoderOutput, currentPtr);
	printf("AFTER POLAR ENCODING\n");
	printf("testInput: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n",
			testInput[0], testInput[1], testInput[2], testInput[3]);
	printf("encOutput: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n",
			encoderOutput[0], encoderOutput[1], encoderOutput[2], encoderOutput[3]);
	return (0);
#endif

	currentPtr = nr_polar_params(nrPolar_params, polarMessageType, testLength, aggregation_level);

	// We assume no a priori knowledge available about the payload.
	double aPrioriArray[currentPtr->payloadBits];
	for (int i=0; i<currentPtr->payloadBits; i++) aPrioriArray[i] = NAN;

	for (SNR = SNRstart; SNR <= SNRstop; SNR += SNRinc) {
		SNR_lin = pow(10, SNR/10);
		for (itr = 1; itr <= iterations; itr++) {

		for(int i=0; i<testLength; i++) testInput[i]=(uint8_t) (rand() % 2);

		start_meas(&timeEncoder);
		polar_encoder(testInput, encoderOutput, currentPtr);
		stop_meas(&timeEncoder);

		//BPSK modulation
		for(int i=0; i<coderLength; i++) {
			if (encoderOutput[i] == 0)
				modulatedInput[i]=1/sqrt(2);
			else
				modulatedInput[i]=(-1)/sqrt(2);

			channelOutput[i] = modulatedInput[i] + (gaussdouble(0.0,1.0) * (1/sqrt(2*SNR_lin)));
			//printf("%f\n",channelOutput[i]);
		}



		start_meas(&timeDecoder);
		/*decoderState = polar_decoder(channelOutput,
									 estimatedOutput,
									 currentPtr,
									 NR_POLAR_DECODER_LISTSIZE,
									 aPrioriArray,
									 NR_POLAR_DECODER_PATH_METRIC_APPROXIMATION);*/
		decoderState = polar_decoder_aPriori(channelOutput,
											 estimatedOutput,
											 currentPtr,
											 NR_POLAR_DECODER_LISTSIZE,
											 NR_POLAR_DECODER_PATH_METRIC_APPROXIMATION,
											 aPrioriArray);
		stop_meas(&timeDecoder);

		//calculate errors
		if (decoderState==-1) {
			blockErrorState=-1;
			nBitError=-1;
		} else {
			for(int i=0; i<testLength; i++){
				if (estimatedOutput[i]!=testInput[i]) nBitError++;
			}
			if (nBitError>0) blockErrorState=1;
		}

		//Iteration times are in microseconds.
		timeEncoderCumulative+=(timeEncoder.diff_now/(cpu_freq_GHz*1000.0));
		timeDecoderCumulative+=(timeDecoder.diff_now/(cpu_freq_GHz*1000.0));
		fprintf(logFile,",%f,%d,%d,%f,%f\n", SNR, nBitError, blockErrorState,
				(timeEncoder.diff_now/(cpu_freq_GHz*1000.0)), (timeDecoder.diff_now/(cpu_freq_GHz*1000.0)));

		if (nBitError<0) {
			blockErrorCumulative++;
			bitErrorCumulative+=testLength;
		} else {
			blockErrorCumulative+=blockErrorState;
			bitErrorCumulative+=nBitError;
		}

		decoderState=0;
		nBitError=0;
		blockErrorState=0;

		}
		//Calculate error statistics for the SNR.
		printf("[ListSize=%d, Appr=%d] SNR=%+8.3f, BLER=%9.6f, BER=%12.9f, t_Encoder=%9.3fus, t_Decoder=%9.3fus\n",
				decoderListSize, pathMetricAppr, SNR, ((double)blockErrorCumulative/iterations),
				((double)bitErrorCumulative / (iterations*testLength)),
				(timeEncoderCumulative/iterations),timeDecoderCumulative/iterations);

		blockErrorCumulative = 0; bitErrorCumulative = 0;
		timeEncoderCumulative = 0; timeDecoderCumulative = 0;
	}

	print_meas(&timeEncoder,"polar_encoder",NULL,NULL);
	print_meas(&timeDecoder,"polar_decoder",NULL,NULL);

	fclose(logFile);
	//free(testInput);
	//free(encoderOutput);
	free(modulatedInput);
	free(channelOutput);
	free(estimatedOutput);

	return (0);
}
