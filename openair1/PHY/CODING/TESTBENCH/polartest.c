#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_pbch_defs.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_uci_defs.h"
#include "SIMULATION/TOOLS/sim.h"

int main(int argc, char *argv[]) {

	//Initiate timing. (Results depend on CPU Frequency. Therefore, might change due to performance variances during simulation.)
	time_stats_t timeEncoder,timeDecoder;
	time_stats_t polar_decoder_init,polar_rate_matching,decoding,bit_extraction,deinterleaving;
	time_stats_t path_metric,sorting,update_LLR;
	opp_enabled=1;
	int decoder_int8=0;
	cpu_freq_GHz = get_cpu_freq_GHz();
	reset_meas(&timeEncoder);
	reset_meas(&timeDecoder);
	reset_meas(&polar_decoder_init);
	reset_meas(&polar_rate_matching);
	reset_meas(&decoding);
	reset_meas(&bit_extraction);
	reset_meas(&deinterleaving);
	reset_meas(&sorting);
	reset_meas(&path_metric);
	reset_meas(&update_LLR);

	randominit(1234);
	//Default simulation values (Aim for iterations = 1000000.)
	int itr, iterations = 1000, arguments, polarMessageType = 1; //0=DCI, 1=PBCH, 2=UCI
	double SNRstart = -20.0, SNRstop = 0.0, SNRinc= 0.5; //dB

	double SNR, SNR_lin;
	int16_t nBitError = 0; // -1 = Decoding failed (All list entries have failed the CRC checks).
	int8_t decoderState=0, blockErrorState=0; //0 = Success, -1 = Decoding failed, 1 = Block Error.
	uint16_t testLength, coderLength, blockErrorCumulative=0, bitErrorCumulative=0;
	double timeEncoderCumulative = 0, timeDecoderCumulative = 0;

	uint8_t decoderListSize = 8, pathMetricAppr = 0; //0 --> eq. (8a) and (11b), 1 --> eq. (9) and (12)
	int generate_optim_code=0;

	while ((arguments = getopt (argc, argv, "s:d:f:m:i:l:a:h:qg")) != -1)
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

  	        case 'q':
		        decoder_int8=1;
		        break;

    	        case 'g':
		  generate_optim_code=1;
                  iterations=1;
		  SNRstart=-6.0;
		  SNRstop =-6.0;
		  decoder_int8=1;
                  break;
	        case 'h':
		  printf("./polartest -s SNRstart -d SNRinc -f SNRstop -m [0=DCI|1=PBCH|2=UCI] -i iterations -l decoderListSize -a pathMetricAppr\n");
		  exit(-1);

		default:
			perror("[polartest.c] Problem at argument parsing with getopt");
			exit(-1);
	}

	if (polarMessageType == 0) { //DCI
	  //testLength = ;
	  //coderLength = ;
	  printf("polartest for DCI not supported yet\n");
	  exit(-1);
	} else if (polarMessageType == 1) { //PBCH
	  testLength = NR_POLAR_PBCH_PAYLOAD_BITS;
	  coderLength = NR_POLAR_PBCH_E;
	  printf("running polartest for PBCH\n");
	} else if (polarMessageType == 2) { //UCI
	  testLength = NR_POLAR_PUCCH_PAYLOAD_BITS;
	  coderLength = NR_POLAR_PUCCH_E;
	  printf("running polartest for UCI");
	} else {
	  printf("unsupported polarMessageType %d (0=DCI, 1=PBCH, 2=UCI)\n",polarMessageType);
	  exit(-1);
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

	uint8_t *testInput = malloc(sizeof(uint8_t) * testLength); //generate randomly
	uint8_t *encoderOutput = malloc(sizeof(uint8_t) * coderLength);
	double *modulatedInput = malloc (sizeof(double) * coderLength); //channel input

	double *channelOutput  = malloc (sizeof(double) * coderLength); //add noise
	int16_t *channelOutput_int8  = malloc (sizeof(int16_t) * coderLength); //add noise
	uint8_t *estimatedOutput = malloc(sizeof(uint8_t) * testLength); //decoder output

	t_nrPolar_params nrPolar_params;
	nr_polar_init(&nrPolar_params, polarMessageType);
	nr_polar_llrtableinit();

	if (generate_optim_code==1) nrPolar_params.decoder_kernel = NULL;

	// We assume no a priori knowledge available about the payload.
	double aPrioriArray[nrPolar_params.payloadBits];
	for (int i=0; i<=nrPolar_params.payloadBits; i++) aPrioriArray[i] = NAN;

	printf("SNRstart %f, SNRstop %f,, SNRinc %f\n",SNRstart,SNRstop,SNRinc);

	for (SNR = SNRstart; SNR <= SNRstop; SNR += SNRinc) {
	  printf("SNR %f\n",SNR);
		SNR_lin = pow(10, SNR/10);
		for (itr = 1; itr <= iterations; itr++) {

		for(int i=0; i<testLength; i++) 
			testInput[i]=(uint8_t) (rand() % 2);

		start_meas(&timeEncoder);
		polar_encoder(testInput, encoderOutput, &nrPolar_params);
		stop_meas(&timeEncoder);

		//BPSK modulation
		for(int i=0; i<coderLength; i++) {
			if (encoderOutput[i] == 0)
				modulatedInput[i]=1/sqrt(2);
			else
				modulatedInput[i]=(-1)/sqrt(2);

			channelOutput[i] = modulatedInput[i] + (gaussdouble(0.0,1.0) * (1/sqrt(2*SNR_lin)));

			if (decoder_int8==1) {
			  if (channelOutput[i] > 15) channelOutput_int8[i] = 127;
			  else if (channelOutput[i] < -16) channelOutput_int8[i] = -128;
			  else channelOutput_int8[i] = (int16_t) (8*channelOutput[i]);
			}
		}


		start_meas(&timeDecoder);
		if (decoder_int8==0) 
		  decoderState = polar_decoder(channelOutput, estimatedOutput, &nrPolar_params, 
					       decoderListSize, aPrioriArray, pathMetricAppr);
		else
		  decoderState = polar_decoder_int8_new(channelOutput_int8, estimatedOutput, &nrPolar_params); 
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
	free(testInput);
	free(encoderOutput);
	free(modulatedInput);
	free(channelOutput);
	free(estimatedOutput);

	return (0);
}
