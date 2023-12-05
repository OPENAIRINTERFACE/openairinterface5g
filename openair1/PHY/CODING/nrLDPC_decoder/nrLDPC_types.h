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

/*!\file nrLDPC_types.h
 * \brief Defines all types for the LDPC decoder
 * \author Sebastian Wagner (TCL Communications) Email: <mailto:sebastian.wagner@tcl.com>
 * \date 27-03-2018
 * \version 1.0
 * \note
 * \warning
 */

#ifndef __NR_LDPC_TYPES__H__
#define __NR_LDPC_TYPES__H__
#ifndef CODEGEN
#include "time_meas.h"
#endif
#include "nrLDPCdecoder_defs.h"
typedef struct {
  uint8_t* d;
  int dim1;
  int dim2;
} arr8_t;
typedef struct {
  uint16_t* d;
  int dim1;
  int dim2;
} arr16_t;
typedef struct {
  uint32_t* d;
  int dim1;
  int dim2;
} arr32_t;
// ==============================================================================
// TYPES

/**
   Structure containing the pointers to the LUTs.
 */
typedef struct nrLDPC_lut {
    const uint32_t* startAddrCnGroups; /**< Start addresses for CN groups in CN processing buffer */
    const uint8_t*  numCnInCnGroups; /**< Number of CNs in every CN group */
    const uint8_t*  numBnInBnGroups; /**< Number of CNs in every BN group */
    const uint32_t* startAddrBnGroups; /**< Start addresses for BN groups in BN processing buffer  */
    const uint16_t* startAddrBnGroupsLlr; /**< Start addresses for BN groups in LLR processing buffer  */
    arr16_t circShift[NR_LDPC_NUM_CN_GROUPS_BG1]; /**< LUT for circular shift values for all CN groups and Zs */
    arr32_t startAddrBnProcBuf[NR_LDPC_NUM_CN_GROUPS_BG1]; /**< LUT of start addresses of CN groups in BN proc buffer */
    arr8_t bnPosBnProcBuf[NR_LDPC_NUM_CN_GROUPS_BG1]; /**< LUT of BN positions in BG for CN groups */
    const uint16_t* llr2llrProcBufAddr; /**< LUT for transferring input LLRs to LLR processing buffer */
    const uint8_t*  llr2llrProcBufBnPos; /**< LUT BN position in BG */
    arr8_t posBnInCnProcBuf[NR_LDPC_NUM_CN_GROUPS_BG1]; /**< LUT for llr2cnProcBuf */
} t_nrLDPC_lut;

/**
   Enum with possible LDPC output formats.
 */
typedef enum nrLDPC_outMode {
    nrLDPC_outMode_BIT, /**< 32 bits per uint32_t output */
    nrLDPC_outMode_BITINT8, /**< 1 bit per int8_t output */
    nrLDPC_outMode_LLRINT8 /**< Single LLR value per int8_t output */
} e_nrLDPC_outMode;

/**
   Structure containing LDPC decoder parameters.
 */
typedef struct nrLDPC_dec_params {
    uint8_t BG; /**< Base graph */
    uint16_t Z; /**< Lifting size */
    uint8_t R; /**< Decoding rate: Format 15,13,... for code rates 1/5, 1/3,... */
    uint16_t F; /**< Filler bits */
    uint8_t Qm; /**< Modulation */
    uint8_t rv;
    uint8_t numMaxIter; /**< Maximum number of iterations */
    int E;
    e_nrLDPC_outMode outMode; /**< Output format */
    int crc_type;
    int (*check_crc)(uint8_t* decoded_bytes, uint32_t n, uint8_t crc_type);
    uint8_t setCombIn;
} t_nrLDPC_dec_params;

typedef struct nrLDPCoffload_params {
    uint8_t BG; /**< Base graph */
    uint16_t Z;
    uint16_t Kr;
    uint8_t rv;
    uint32_t E;
    uint16_t n_cb;
    uint16_t F; /**< Filler bits */
    uint8_t Qm; /**< Modulation */
    uint8_t setCombIn;
} t_nrLDPCoffload_params;

/**
   Structure containing LDPC decoder processing time statistics.
 */
#ifndef CODEGEN
typedef struct nrLDPC_time_stats {
    time_stats_t llr2llrProcBuf; /**< Statistics for function llr2llrProcBuf */
    time_stats_t llr2CnProcBuf; /**< Statistics for function llr2CnProcBuf */
    time_stats_t cnProc; /**< Statistics for function cnProc */
    time_stats_t cnProcPc; /**< Statistics for function cnProcPc */
    time_stats_t bnProcPc; /**< Statistics for function bnProcPc */
    time_stats_t bnProc; /**< Statistics for function bnProc */
    time_stats_t cn2bnProcBuf; /**< Statistics for function cn2bnProcBuf */
    time_stats_t bn2cnProcBuf; /**< Statistics for function bn2cnProcBuf */
    time_stats_t llrRes2llrOut; /**< Statistics for function llrRes2llrOut */
    time_stats_t llr2bit; /**< Statistics for function llr2bit */
    time_stats_t total; /**< Statistics for total processing time */
} t_nrLDPC_time_stats;
#endif
/**
   Structure containing the processing buffers
 */
typedef struct nrLDPC_procBuf {
    int8_t* cnProcBuf; /**< CN processing buffer */
    int8_t* cnProcBufRes; /**< Buffer for CN processing results */
    int8_t* bnProcBuf; /**< BN processing buffer */
    int8_t* bnProcBufRes; /**< Buffer for BN processing results */
    int8_t* llrRes; /**< Buffer for LLR results */
    int8_t* llrProcBuf; /**< LLR processing buffer */
} t_nrLDPC_procBuf;



#endif
