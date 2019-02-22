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

//#define DEBUG_DCI_POLAR_PARAMS
//#define DEBUG_POLAR_TIMING
//#define DEBUG_CRC

int main(int argc, char *argv[]) {
  //Initiate timing. (Results depend on CPU Frequency. Therefore, might change due to performance variances during simulation.)
  time_stats_t timeEncoder,timeDecoder;
  opp_enabled=1;
  int decoder_int16=0;
  cpu_freq_GHz = get_cpu_freq_GHz();
  reset_meas(&timeEncoder);
  reset_meas(&timeDecoder);
  randominit(0);
  crcTableInit();
  //Default simulation values (Aim for iterations = 1000000.)
  int itr, iterations = 1000, arguments, polarMessageType = 0; //0=PBCH, 1=DCI, -1=UCI
  double SNRstart = -20.0, SNRstop = 0.0, SNRinc= 0.5; //dB
  double SNR, SNR_lin;
  int16_t nBitError = 0; // -1 = Decoding failed (All list entries have failed the CRC checks).
  uint32_t decoderState=0, blockErrorState=0; //0 = Success, -1 = Decoding failed, 1 = Block Error.
  uint16_t testLength = 0, coderLength = 0, blockErrorCumulative=0, bitErrorCumulative=0;
  double timeEncoderCumulative = 0, timeDecoderCumulative = 0;
  uint8_t aggregation_level = 8, decoderListSize = 8, pathMetricAppr = 0;

  while ((arguments = getopt (argc, argv, "s:d:f:m:i:l:a:hqg")) != -1)
    switch (arguments) {
      case 's':
        SNRstart = atof(optarg);
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
        decoder_int16 = 1;
        break;

      case 'g':
        iterations = 1;
        SNRstart = -6.0;
        SNRstop = -6.0;
        decoder_int16 = 1;
        break;

      case 'h':
        printf("./polartest -s SNRstart -d SNRinc -f SNRstop -m [0=PBCH|1=DCI|2=UCI] -i iterations -l decoderListSize -a pathMetricAppr\n");
        exit(-1);

      default:
        perror("[polartest.c] Problem at argument parsing with getopt");
        exit(-1);
    }

  if (polarMessageType == 0) { //PBCH
    testLength = 64;//NR_POLAR_PBCH_PAYLOAD_BITS;
    coderLength = NR_POLAR_PBCH_E;
    aggregation_level = NR_POLAR_PBCH_AGGREGATION_LEVEL;
  } else if (polarMessageType == 1) { //DCI
    //testLength = nr_get_dci_size(params_rel15->dci_format, params_rel15->rnti_type, &fp->initial_bwp_dl, cfg);
    testLength = 41; //20;
    coderLength = 108*8; //to be changed by aggregate level function.
  } else if (polarMessageType == -1) { //UCI
    //testLength = ;
    //coderLength = ;
  }

  //Logging
  time_t currentTime;
  time (&currentTime);
  char fileName[512], currentTimeInfo[25];
  char folderName[] = ".";
  /*
  folderName=getenv("HOME");
  strcat(folderName,"/Desktop/polartestResults");
  */
#ifdef DEBUG_POLAR_TIMING
  sprintf(fileName,"%s/TIMING_ListSize_%d_pmAppr_%d_Payload_%d_Itr_%d",folderName,decoderListSize,pathMetricAppr,testLength,iterations);
#else
  sprintf(fileName,"%s/_ListSize_%d_pmAppr_%d_Payload_%d_Itr_%d",folderName,decoderListSize,pathMetricAppr,testLength,iterations);
#endif
  strftime(currentTimeInfo, 25, "_%Y-%m-%d-%H-%M-%S.csv", localtime(&currentTime));
  strcat(fileName,currentTimeInfo);
  //Create "~/Desktop/polartestResults" folder if it doesn't already exist.
  /*
  struct stat folder = {0};
  if (stat(folderName, &folder) == -1) mkdir(folderName, S_IRWXU | S_IRWXG | S_IRWXO);
  */
  FILE *logFile;
  logFile = fopen(fileName, "w");

  if (logFile==NULL) {
    fprintf(stderr,"[polartest.c] Problem creating file %s with fopen\n",fileName);
    exit(-1);
  }

#ifdef DEBUG_POLAR_TIMING
  fprintf(logFile,
          ",timeEncoderCRCByte[us],timeEncoderCRCBit[us],timeEncoderInterleaver[us],timeEncoderBitInsertion[us],timeEncoder1[us],timeEncoder2[us],timeEncoderRateMatching[us],timeEncoderByte2Bit[us]\n");
#else
  fprintf(logFile,",SNR,nBitError,blockErrorState,t_encoder[us],t_decoder[us]\n");
#endif
  uint8_t testArrayLength = ceil(testLength / 32.0);
  uint8_t coderArrayLength = ceil(coderLength / 32.0);
  uint32_t testInput[testArrayLength]; //generate randomly
  uint32_t encoderOutput[coderArrayLength];
  uint32_t estimatedOutput[testArrayLength]; //decoder output
  memset(testInput,0,sizeof(uint32_t) * testArrayLength);
  memset(encoderOutput,0,sizeof(uint32_t) * coderArrayLength);
  memset(estimatedOutput,0,sizeof(uint32_t) * testArrayLength);
  uint8_t encoderOutputByte[coderLength];
  double modulatedInput[coderLength]; //channel input
  double channelOutput[coderLength];  //add noise
  int16_t channelOutput_int16[coderLength];
  t_nrPolar_params *currentPtr = nr_polar_params(polarMessageType, testLength, aggregation_level);
#ifdef DEBUG_DCI_POLAR_PARAMS
  uint32_t dci_pdu[4];
  memset(dci_pdu,0,sizeof(uint32_t)*4);
  dci_pdu[0]=0x01189400;
  printf("dci_pdu: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n",
         dci_pdu[0], dci_pdu[1], dci_pdu[2], dci_pdu[3]);
  uint32_t encoder_output[54];
  memset(encoder_output,0,sizeof(uint32_t)*54);
  uint16_t size=41;
  uint16_t rnti=3;
  aggregation_level=8;
  t_nrPolar_params *currentPtrDCI=nr_polar_params(1, size, aggregation_level);
  polar_encoder_dci(dci_pdu, encoder_output, currentPtrDCI, rnti);

  for (int i=0; i<54; i++)
    printf("encoder_output: [%2d]->0x%08x \n",i, encoder_output[i]);

  uint8_t *encoder_outputByte = malloc(sizeof(uint8_t) * currentPtrDCI->encoderLength);
  double *channel_output  = malloc (sizeof(double) * currentPtrDCI->encoderLength);
  uint32_t dci_estimation[4];
  memset(dci_estimation,0,sizeof(uint32_t)*4);
  printf("dci_estimation: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n",
         dci_estimation[0], dci_estimation[1], dci_estimation[2], dci_estimation[3]);
  nr_bit2byte_uint32_8_t(encoder_output, currentPtrDCI->encoderLength, encoder_outputByte);
  printf("[polartest] encoder_outputByte: ");

  for (int i = 0; i < currentPtrDCI->encoderLength; i++) printf("%d-", encoder_outputByte[i]);

  printf("\n");

  for(int i=0; i<currentPtrDCI->encoderLength; i++) {
    if (encoder_outputByte[i] == 0) {
      channel_output[i]=1/sqrt(2);
    } else {
      channel_output[i]=(-1)/sqrt(2);
    }
  }

  decoderState = polar_decoder_dci(channel_output,
                                   dci_estimation,
                                   currentPtrDCI,
                                   NR_POLAR_DECODER_LISTSIZE,
                                   NR_POLAR_DECODER_PATH_METRIC_APPROXIMATION,
                                   rnti);
  printf("dci_estimation: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n",
         dci_estimation[0], dci_estimation[1], dci_estimation[2], dci_estimation[3]);
  free(encoder_outputByte);
  free(channel_output);
  return 0;
#endif
#ifdef DEBUG_CRC
  uint32_t crc;
  unsigned int poly24c = 0xb2b11700;
  uint32_t testInputCRC[4];
  testInputCRC[0]=0x00291880;
  //testInputCRC[0]=0x01189400;
  testInputCRC[1]=0x00000000;
  testInputCRC[2]=0x00000000;
  testInputCRC[3]=0x00000000;
  uint32_t testInputcrc=0x01189400;
  uint32_t testInputcrc2=0x00291880;
  uint8_t testInputCRC2[8];
  nr_crc_bit2bit_uint32_8_t(testInputCRC, 32, testInputCRC2);
  printf("testInputCRC2: [0]->%x \t [1]->%x \t [2]->%x \t [3]->%x\n"
         "            [4]->%x \t [5]->%x \t [6]->%x \t [7]->%x\n",
         testInputCRC2[0], testInputCRC2[1], testInputCRC2[2], testInputCRC2[3],
         testInputCRC2[4], testInputCRC2[5], testInputCRC2[6], testInputCRC2[7]);
  unsigned int crc41 = crc24c(testInputCRC, 32);
  unsigned int crc65 = crc24c(testInputCRC, 56);
  printf("crc41: [0]->0x%08x\tcrc65: [0]->0x%08x\n",crc41, crc65);

  for (int i=0; i<32; i++) printf("crc41[%d]=%d\tcrc65[%d]=%d\n",i,(crc41>>i)&1,i,(crc65>>i)&1);

  crc = crc24c(testInputCRC, testLength)>>8;

  for (int i=0; i<24; i++) printf("[i]=%d\n",(crc>>i)&1);

  printf("crc: [0]->0x%08x\n",crc);
  //crcbit(testInputCRC, sizeof(test) - 1, poly24c));
  testInputCRC[testLength>>3] = ((uint8_t *)&crc)[2];
  testInputCRC[1+(testLength>>3)] = ((uint8_t *)&crc)[1];
  testInputCRC[2+(testLength>>3)] = ((uint8_t *)&crc)[0];
  printf("testInputCRC: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n",
         testInputCRC[0], testInputCRC[1], testInputCRC[2], testInputCRC[3]);
  //uint32_t trial32 = 0xffffffff;
  uint32_t trial32 = 0xf10fffff;
  uint8_t a[4];
  //memcpy(a, &trial32, sizeof(trial32));
  *(uint32_t *)a = trial32;
  unsigned char trial[4];
  trial[0]=0xff;
  trial[1]=0xff;
  trial[2]=0x0f;
  trial[3]=0xf1;
  uint32_t trialcrc = crc24c(trial, 32);
  uint32_t trialcrc32 = crc24c((uint8_t *)&trial32, 32);
  //uint32_t trialcrc32 = crc24c(a, 32);
  printf("crcbit(trial = %x\n", crcbit(trial, 4, poly24c));
  printf("trialcrc = %x\n", trialcrc);
  printf("trialcrc32 = %x\n", trialcrc32);

  for (int i=0; i<32; i++) printf("trialcrc[%2d]=%d\ttrialcrc32[%2d]=%d\n",i,(trialcrc>>i)&1,i,(trialcrc32>>i)&1);

  //uint8_t nr_polar_A[32] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
  uint8_t nr_polar_A[32] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,1};
  uint8_t nr_polar_crc[24];
  uint8_t **crc_generator_matrix = crc24c_generator_matrix(32);
  nr_matrix_multiplication_uint8_1D_uint8_2D(nr_polar_A,
      crc_generator_matrix,
      nr_polar_crc,
      32,
      24);

  for (uint8_t i = 0; i < 24; i++) {
    nr_polar_crc[i] = (nr_polar_crc[i] % 2);
    printf("nr_polar_crc[%d]=%d\n",i,nr_polar_crc[i]);
  }

  return 0;
#endif
#ifdef DEBUG_POLAR_TIMING

  for (SNR = SNRstart; SNR <= SNRstop; SNR += SNRinc) {
    SNR_lin = pow(10, SNR / 10);

    for (itr = 1; itr <= iterations; itr++) {
      for (int j=0; j<ceil(testLength / 32.0); j++) {
        for(int i=0; i<32; i++) {
          testInput[j] |= ( ((uint32_t) (rand()%2)) &1);
          testInput[j]<<=1;
        }
      }

      printf("testInput: [0]->0x%08x \n", testInput[0]);
      polar_encoder_timing(testInput, encoderOutput, currentPtr, cpu_freq_GHz, logFile);
    }
  }

  fclose(logFile);
  free(testInput);
  free(encoderOutput);
  free(modulatedInput);
  free(channelOutput);
  free(estimatedOutput);
  return (0);
#endif
  // We assume no a priori knowledge available about the payload.
  double aPrioriArray[currentPtr->payloadBits];

  for (int i=0; i<currentPtr->payloadBits; i++) aPrioriArray[i] = NAN;

  for (SNR = SNRstart; SNR <= SNRstop; SNR += SNRinc) {
    printf("SNR %f\n",SNR);
    SNR_lin = pow(10, SNR/10);

    for (itr = 1; itr <= iterations; itr++) {
      for (int i = 0; i < testArrayLength; i++) {
        for (int j = 0; j < (sizeof(testInput[0])*8)-1; j++) {
          testInput[i] |= ( ((uint32_t) (rand()%2)) &1);
          testInput[i]<<=1;
        }

        testInput[i] |= ( ((uint32_t) (rand()%2)) &1);
      }

      /*printf("testInput: [0]->0x%08x\n", testInput[0]);
      for (int i=0; i<32; i++)
        printf("%d\n",(testInput[0]>>i)&1);*/
      int len_mod64=currentPtr->payloadBits&63;
      ((uint64_t *)testInput)[currentPtr->payloadBits/64]&=((((uint64_t)1)<<len_mod64)-1);
      start_meas(&timeEncoder);

      if (decoder_int16==0)
        polar_encoder(testInput, encoderOutput, currentPtr);
      else
        polar_encoder_fast((uint64_t *)testInput, encoderOutput,0, currentPtr);

      //polar_encoder_fast((uint64_t*)testInput, (uint64_t*)encoderOutput,0, currentPtr);
      stop_meas(&timeEncoder);
      /*printf("encoderOutput: [0]->0x%08x\n", encoderOutput[0]);
      printf("encoderOutput: [1]->0x%08x\n", encoderOutput[1]);*/
      //Bit-to-byte:
      nr_bit2byte_uint32_8_t(encoderOutput, coderLength, encoderOutputByte);

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

      /*decoderState = polar_decoder(channelOutput,
                     estimatedOutput,
                     currentPtr,
                     NR_POLAR_DECODER_LISTSIZE,
                     aPrioriArray,
                     NR_POLAR_DECODER_PATH_METRIC_APPROXIMATION);*/
      if (decoder_int16==0)
        decoderState = polar_decoder_aPriori(channelOutput,
                                             estimatedOutput,
                                             currentPtr,
                                             NR_POLAR_DECODER_LISTSIZE,
                                             NR_POLAR_DECODER_PATH_METRIC_APPROXIMATION,
                                             aPrioriArray);
      else
        decoderState = polar_decoder_int16(channelOutput_int16,
                                           (uint64_t *)estimatedOutput,
                                           currentPtr);

      stop_meas(&timeDecoder);
      /*printf("testInput: [0]->0x%08x\n", testInput[0]);
      printf("estimatedOutput: [0]->0x%08x\n", estimatedOutput[0]);*/

      //calculate errors
      if (decoderState!=0) {
        blockErrorState=-1;
        nBitError=-1;
      } else {
        for (int j = 0; j < currentPtr->payloadBits; j++) {
          if (((estimatedOutput[0]>>j) & 1) != ((testInput[0]>>j) & 1)) nBitError++;

          //          printf("bit %d: %d => %d\n",j,(testInput[0]>>j)&1,(estimatedOutput[0]>>j)&1);
        }

        if (nBitError>0) {
          blockErrorState=1;
          //          printf("Error: Input %x, Output %x\n",testInput[0],estimatedOutput[0]);
        }
      }

      //Iteration times are in microseconds.
      timeEncoderCumulative+=(timeEncoder.diff/(cpu_freq_GHz*1000.0));
      timeDecoderCumulative+=(timeDecoder.diff/(cpu_freq_GHz*1000.0));
      fprintf(logFile,",%f,%d,%d,%f,%f\n", SNR, nBitError, blockErrorState,
              (timeEncoder.diff/(cpu_freq_GHz*1000.0)), (timeDecoder.diff/(cpu_freq_GHz*1000.0)));

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
           (double)timeEncoder.diff/timeEncoder.trials/(cpu_freq_GHz*1000.0),(double)timeDecoder.diff/timeDecoder.trials/(cpu_freq_GHz*1000.0));
    //(timeEncoderCumulative/iterations),timeDecoderCumulative/iterations);

    if (blockErrorCumulative==0 && bitErrorCumulative==0)
      break;

    blockErrorCumulative = 0;
    bitErrorCumulative = 0;
    timeEncoderCumulative = 0;
    timeDecoderCumulative = 0;
  }

  print_meas(&timeEncoder,"polar_encoder",NULL,NULL);
  print_meas(&timeDecoder,"polar_decoder",NULL,NULL);
  fclose(logFile);
  return (0);
}
