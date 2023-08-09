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
#include "PHY/CODING/coding_defs.h"
#include "SIMULATION/TOOLS/sim.h"
//#include "common/utils/LOG/log.h"
#include "coding_unitary_defs.h"
//#define DEBUG_DCI_POLAR_PARAMS
//#define DEBUG_POLAR_TIMING
//#define DEBUG_POLARTEST

configmodule_interface_t *uniqCfg = NULL;
int main(int argc, char *argv[])
{
  //Default simulation values (Aim for iterations = 1000000.) 
  int decoder_int16=0;
  int itr, iterations = 1000, arguments, polarMessageType = 0; //0=PBCH, 1=DCI, 2=UCI
  double SNRstart = -20.0, SNRstop = 0.0, SNRinc= 0.5; //dB
  double SNR, SNR_lin;
  int16_t nBitError = 0; // -1 = Decoding failed (All list entries have failed the CRC checks).
  uint32_t decoderState=0, blockErrorState=0; //0 = Success, -1 = Decoding failed, 1 = Block Error.
  uint16_t testLength = NR_POLAR_PBCH_PAYLOAD_BITS, coderLength = NR_POLAR_PBCH_E;
  uint16_t blockErrorCumulative=0, bitErrorCumulative=0;
  uint8_t aggregation_level = 8, decoderListSize = 8, logFlag = 0;
  uint16_t rnti=0;

  while ((arguments = getopt (argc, argv, "s:d:f:m:i:l:a:p:hqgFL:k:")) != -1) {
    switch (arguments) {
    case 's':
    	SNRstart = atof(optarg);
	SNRstop = SNRstart + 2;
    	break;

    case 'd':
    	SNRinc = atof(optarg);
    	break;

    case 'f':
    	SNRstop = atof(optarg);
    	break;

    case 'm':
    	polarMessageType = atoi(optarg);
    	if (polarMessageType!=0 && polarMessageType!=1 && polarMessageType!=2)
    		printf("Illegal polar message type %d (should be 0,1 or 2)\n", polarMessageType);
    	break;

    case 'i':
    	iterations = atoi(optarg);
    	break;

    case 'l':
      decoderListSize = (uint8_t) atoi(optarg);
      break;

    case 'q':
    	decoder_int16 = 1;
    	break;

    case 'g':
    	iterations = 1;
    	SNRstart = -6.0;
    	SNRstop = -6.0;
    	decoder_int16 = 1;
    	break;

    case 'F':
    	logFlag = 1;
    	break;

    case 'L':
    	aggregation_level=atoi(optarg);
    	if (aggregation_level != 1 && aggregation_level != 2 && aggregation_level != 4 && aggregation_level != 8 && aggregation_level != 16) {
    		printf("Illegal aggregation level %d \n",aggregation_level);
    		exit(-1);
    	}
    	break;

    case 'k':
    	testLength=atoi(optarg);
    	if (testLength < 12 || testLength > 127) {
    		printf("Illegal packet bitlength %d \n",testLength);
    		exit(-1);
    	}
    	break;

    case 'h':
      printf("./polartest\nOptions\n-h Print this help\n-s SNRstart (dB)\n-d SNRinc (dB)\n-f SNRstop (dB)\n-m [0=PBCH|1=DCI|2=UCI]\n"
             "-i Number of iterations\n-l decoderListSize\n-q Flag for optimized coders usage\n-F Flag for test results logging\n"
    		 "-L aggregation level (for DCI)\n-k packet_length (bits) for DCI/UCI\n");
      exit(-1);
      break;

    default:
      perror("[polartest.c] Problem at argument parsing with getopt");
      exit(-1);
      break;
    }
  }
  //Initiate timing. (Results depend on CPU Frequency. Therefore, might change due to performance variances during simulation.)
  time_stats_t timeEncoder,timeDecoder;
  opp_enabled=1;
  reset_meas(&timeEncoder);
  reset_meas(&timeDecoder);
  randominit(0);
  crcTableInit();

  if (polarMessageType == 0) { //PBCH
    aggregation_level = NR_POLAR_PBCH_AGGREGATION_LEVEL;
  } else if (polarMessageType == 1) { //DCI
    coderLength = 108*aggregation_level;
  } else if (polarMessageType == 2) { //UCI
    //pucch2 parameters, 1 symbol, aggregation_level = NPRB
    AssertFatal(aggregation_level>2,"For UCI formats, aggregation (N_RB) should be > 2\n");
    coderLength = 16*aggregation_level; 
  }

  //Logging
  time_t currentTime;
  char fileName[512], currentTimeInfo[25];
  char folderName[] = ".";
  FILE *logFile = NULL;

  if (logFlag){
    time (&currentTime);
#ifdef DEBUG_POLAR_TIMING
    sprintf(fileName,"%s/TIMING_ListSize_%d_Payload_%d_Itr_%d", folderName, decoderListSize, testLength, iterations);
#else
    sprintf(fileName,"%s/_ListSize_%d_Payload_%d_Itr_%d", folderName, decoderListSize, testLength, iterations);
#endif
    strftime(currentTimeInfo, 25, "_%Y-%m-%d-%H-%M-%S.csv", localtime(&currentTime));
    strcat(fileName,currentTimeInfo);
    //Create "~/Desktop/polartestResults" folder if it doesn't already exist.
    /*struct stat folder = {0};
    if (stat(folderName, &folder) == -1) mkdir(folderName, S_IRWXU | S_IRWXG | S_IRWXO);*/
    logFile = fopen(fileName, "w");

    if (logFile==NULL) {
      fprintf(stderr,"[polartest.c] Problem creating file %s with fopen\n",fileName);
      exit(-1);
    }

#ifdef DEBUG_POLAR_TIMING
    fprintf(logFile,",timeEncoderCRCByte[us],timeEncoderCRCBit[us],timeEncoderInterleaver[us],timeEncoderBitInsertion[us],timeEncoder1[us],timeEncoder2[us],timeEncoderRateMatching[us],timeEncoderByte2Bit[us]\n");
#else
    fprintf(logFile,",SNR,nBitError,blockErrorState,t_encoder[us],t_decoder[us]\n");
#endif
  }

  const uint8_t testArrayLength = ceil(testLength / 32.0);
  const uint8_t coderArrayLength = ceil(coderLength / 32.0);
  // in the polar code, often uint64_t arrays are used, but we work with
  // uint32_t arrays below, so realArrayLength is the length that always
  // satisfies uint64_t array length
  const uint8_t realArrayLength = ((testArrayLength + 1) / 2) * 2;
  printf("testArrayLength %d realArrayLength %d\n", testArrayLength, realArrayLength);
  uint32_t testInput[realArrayLength]; //generate randomly
  uint32_t encoderOutput[coderArrayLength];
  uint32_t estimatedOutput[realArrayLength]; //decoder output
  memset(testInput,0,sizeof(uint32_t) * realArrayLength); // does not reset all
  memset(encoderOutput,0,sizeof(uint32_t) * coderArrayLength);
  memset(estimatedOutput,0,sizeof(uint32_t) * realArrayLength);
  uint8_t encoderOutputByte[coderLength];
  double modulatedInput[coderLength]; //channel input
  double channelOutput[coderLength];  //add noise
  int16_t channelOutput_int16[coderLength];

  t_nrPolar_params *currentPtr = nr_polar_params(polarMessageType, testLength, aggregation_level, true);

#ifdef DEBUG_DCI_POLAR_PARAMS
  uint32_t dci_pdu[4];
  memset(dci_pdu,0,sizeof(uint32_t)*4);
  dci_pdu[0]=0x01189400;
  printf("dci_pdu: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n", dci_pdu[0], dci_pdu[1], dci_pdu[2], dci_pdu[3]);
  uint16_t size=41;
  rnti=3;
  aggregation_level=8;
  uint32_t encoder_output[54];
  memset(encoder_output,0,sizeof(uint32_t)*54);
  t_nrPolar_params *currentPtrDCI=nr_polar_params(1, size, aggregation_level);

  polar_encoder_dci(dci_pdu, encoder_output, currentPtrDCI, rnti);
  for (int i=0; i<54; i++) printf("encoder_output: [%2d]->0x%08x \n", i, encoder_output[i]);

  uint8_t *encoder_outputByte = malloc(sizeof(uint8_t) * currentPtrDCI->encoderLength);
  double *modulated_input = malloc (sizeof(double) * currentPtrDCI->encoderLength);
  double *channel_output = malloc (sizeof(double) * currentPtrDCI->encoderLength);
  uint32_t dci_est[4];
  memset(dci_est,0,sizeof(uint32_t)*4);
  printf("dci_est: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n", dci_est[0], dci_est[1], dci_est[2], dci_est[3]);
  nr_bit2byte_uint32_8(encoder_output, currentPtrDCI->encoderLength, encoder_outputByte);
  printf("[polartest] encoder_outputByte: ");
  for (int i = 0; i < currentPtrDCI->encoderLength; i++) printf("%d-", encoder_outputByte[i]); printf("\n");

  SNR_lin = pow(10, 0/10); //SNR = 0 dB
  for(int i=0; i<currentPtrDCI->encoderLength; i++) {
	  if (encoder_outputByte[i] == 0)
		  modulated_input[i]=1/sqrt(2);
	  else
		  modulated_input[i]=(-1)/sqrt(2);
	  channel_output[i] = modulated_input[i] + (gaussdouble(0.0,1.0) * (1/sqrt(2*SNR_lin)));
  }
  decoderState = polar_decoder_dci(channel_output, dci_est, NR_POLAR_DECODER_LISTSIZE, rnti,
                                   1, size, aggregation_level);
  printf("dci_est: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n", dci_est[0], dci_est[1], dci_est[2], dci_est[3]);
  free(encoder_outputByte);
  free(channel_output);
  free(modulated_input);
  if (logFlag)
    fclose(logFile);
  return 0;
#endif

  for (SNR = SNRstart; SNR <= SNRstop; SNR += SNRinc) {
	  printf("SNR %f\n",SNR);
	  SNR_lin = pow(10, SNR/10);

	  for (itr = 1; itr <= iterations; itr++) {
		  //Generate random values for all the bits of "testInput", not just as much as "currentPtr->payloadBits".
		  for (int i = 0; i < testArrayLength; i++) {
			  for (int j = 0; j < (sizeof(testInput[0])*8)-1; j++) {
				  testInput[i] |= ( ((uint32_t) (rand()%2)) &1);
				  testInput[i]<<=1;
			  }
			  testInput[i] |= ( ((uint32_t) (rand()%2)) &1);
		  }

#ifdef DEBUG_POLARTEST
		  //testInput[0] = 0x360f8a5c;
		  printf("testInput: [0]->0x%08x\n", testInput[0]);
#endif
		  int len_mod64=currentPtr->payloadBits&63;
		  ((uint64_t *)testInput)[currentPtr->payloadBits/64]&=((((uint64_t)1)<<len_mod64)-1);

		  start_meas(&timeEncoder);
		  if (decoder_int16==1) {
			  polar_encoder_fast((uint64_t *)testInput, encoderOutput, 0, 0, polarMessageType, testLength, aggregation_level);
			  //polar_encoder_fast((uint64_t*)testInput, (uint64_t*)encoderOutput,0,0,currentPtr);
		  } else { //0 --> PBCH, 1 --> DCI, -1 --> UCI
			  if (polarMessageType == 0)
				  polar_encoder(testInput, encoderOutput, polarMessageType, testLength, aggregation_level);
			  else if (polarMessageType == 1)
                            polar_encoder_dci(testInput, encoderOutput, rnti, polarMessageType, testLength, aggregation_level);
		  }
		  stop_meas(&timeEncoder);

#ifdef DEBUG_POLARTEST
		  printf("encoderOutput: [0]->0x%08x\n", encoderOutput[0]);
		  //for (int i=1;i<coderArrayLength;i++) printf("encoderOutput: [i]->0x%08x\n", i, encoderOutput[1]);
#endif

      //Bit-to-byte:
      nr_bit2byte_uint32_8(encoderOutput, coderLength, encoderOutputByte);

      //BPSK modulation
      for(int i=0; i<coderLength; i++) {
    	  if (encoderOutputByte[i] == 0)
    		  modulatedInput[i]=1/sqrt(2);
    	  else
    		  modulatedInput[i]=(-1)/sqrt(2);

    	  channelOutput[i] = modulatedInput[i] + (gaussdouble(0.0,1.0) * (1/sqrt(2*SNR_lin)));

    	  if (decoder_int16==1) {
    		  if (channelOutput[i] > 15) channelOutput_int16[i] = 127;
    		  else if (channelOutput[i] < -16) channelOutput_int16[i] = -128;
    		  else channelOutput_int16[i] = (int16_t) (8*channelOutput[i]);
    	  }
      }

      start_meas(&timeDecoder);

      if (decoder_int16==1) {
    	  decoderState = polar_decoder_int16(channelOutput_int16, (uint64_t *)estimatedOutput, 0,
                                             polarMessageType, testLength, aggregation_level);
      } else { //0 --> PBCH, 1 --> DCI, -1 --> UCI
    	  if (polarMessageType == 0) {
            decoderState = polar_decoder(channelOutput,
                                         estimatedOutput,
                                         decoderListSize,
                                         polarMessageType, testLength, aggregation_level);
    	  } else if (polarMessageType == 1) {
            decoderState = polar_decoder_dci(channelOutput,
                                             estimatedOutput,
                                             decoderListSize,
                                             rnti,
                                             polarMessageType, testLength, aggregation_level);
    	  }
      }
      stop_meas(&timeDecoder);
      
#ifdef DEBUG_POLARTEST
      printf("estimatedOutput: [0]->0x%08x\n", estimatedOutput[0]);
#endif

      //calculate errors
      if (decoderState!=0) {
    	  blockErrorState=-1;
    	  nBitError=-1;
      } else {
    	  for (int i = 0; i < (testArrayLength-1); i++) {
    	  		for (int j = 0; j < 32; j++) {
    	  			if ( ((estimatedOutput[i]>>j) & 1) != ((testInput[i]>>j) & 1) )
    	  				nBitError++;
    	  		}
    	  	}
    	  for (int j = 0; j < testLength - ((testArrayLength-1) * 32); j++)
    		  if ( ((estimatedOutput[(testArrayLength-1)]>>j) & 1) != ((testInput[(testArrayLength-1)]>>j) & 1) )
    			  nBitError++;
      }
      if (nBitError>0) blockErrorState=1;
#ifdef DEBUG_POLARTEST
          		  for (int i = 0; i < testArrayLength; i++)
          			  printf("[polartest/decoderState=%u] testInput[%d]=0x%08x, estimatedOutput[%d]=0x%08x\n",decoderState, i, testInput[i], i, estimatedOutput[i]);
#endif

      //Iteration times are in microseconds.
      if (logFlag) fprintf(logFile,",%f,%d,%u,%f,%f\n", SNR, nBitError, blockErrorState, (timeEncoder.diff/(get_cpu_freq_GHz()*1000.0)), (timeDecoder.diff/(get_cpu_freq_GHz()*1000.0)));

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
	  memset(testInput,0,sizeof(uint32_t) * realArrayLength);
	  memset(encoderOutput,0,sizeof(uint32_t) * coderArrayLength);
	  memset(estimatedOutput,0,sizeof(uint32_t) * realArrayLength);
    }

    //Calculate error statistics for the SNR.
    printf("[ListSize=%d] SNR=%+8.3f, BLER=%9.6f, BER=%12.9f, t_Encoder=%9.3fus, t_Decoder=%9.3fus\n",
           decoderListSize, SNR, ((double)blockErrorCumulative/iterations),
           ((double)bitErrorCumulative / (iterations*testLength)),
           (double)timeEncoder.diff/timeEncoder.trials/(get_cpu_freq_GHz()*1000.0),(double)timeDecoder.diff/timeDecoder.trials/(get_cpu_freq_GHz()*1000.0));

    if (blockErrorCumulative==0 && bitErrorCumulative==0) break;

    blockErrorCumulative = 0;
    bitErrorCumulative = 0;
  }

  print_meas(&timeEncoder,"polar_encoder",NULL,NULL);
  print_meas(&timeDecoder,"polar_decoder",NULL,NULL);
  if (logFlag) fclose(logFile);
  return (0);
}
