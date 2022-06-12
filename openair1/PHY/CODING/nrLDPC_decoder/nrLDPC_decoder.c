

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

/*!\file nrLDPC_decoder.c
 * \brief Defines thenrLDPC decoder
*/

#include <stdint.h>
#include <immintrin.h>
#include "nrLDPCdecoder_defs.h"
#include "nrLDPC_types.h"
#include "nrLDPC_init.h"
#include "nrLDPC_mPass.h"
#include "nrLDPC_cnProc.h"
#include "nrLDPC_bnProc.h"
#define UNROLL_CN_PROC 1
#define UNROLL_BN_PROC 1
#define UNROLL_BN_PROC_PC 1
#define UNROLL_BN2CN_PROC 1
/*----------------------------------------------------------------------
|                  cn processing files -->AVX512
/----------------------------------------------------------------------*/

//BG1-------------------------------------------------------------------
#ifdef __AVX512BW__

#include "cnProc_avx512/nrLDPC_cnProc_BG1_R13_AVX512.h"
#include "cnProc_avx512/nrLDPC_cnProc_BG1_R23_AVX512.h"
#include "cnProc_avx512/nrLDPC_cnProc_BG1_R89_AVX512.h"
//BG2-------------------------------------------------------------------
#include "cnProc_avx512/nrLDPC_cnProc_BG2_R15_AVX512.h"
#include "cnProc_avx512/nrLDPC_cnProc_BG2_R13_AVX512.h"
#include "cnProc_avx512/nrLDPC_cnProc_BG2_R23_AVX512.h"

#else

/*----------------------------------------------------------------------
|                  cn Processing files -->AVX2
/----------------------------------------------------------------------*/

//BG1------------------------------------------------------------------
#include "cnProc/nrLDPC_cnProc_BG1_R13_AVX2.h"
#include "cnProc/nrLDPC_cnProc_BG1_R23_AVX2.h"
#include "cnProc/nrLDPC_cnProc_BG1_R89_AVX2.h"
//BG2 --------------------------------------------------------------------
#include "cnProc/nrLDPC_cnProc_BG2_R15_AVX2.h"
#include "cnProc/nrLDPC_cnProc_BG2_R13_AVX2.h"
#include "cnProc/nrLDPC_cnProc_BG2_R23_AVX2.h"

#endif

/*----------------------------------------------------------------------
|                 bn Processing files -->AVX2
/----------------------------------------------------------------------*/

//bnProcPc-------------------------------------------------------------
//BG1------------------------------------------------------------------
#include "bnProcPc/nrLDPC_bnProcPc_BG1_R13_AVX2.h"
#include "bnProcPc/nrLDPC_bnProcPc_BG1_R23_AVX2.h"
#include "bnProcPc/nrLDPC_bnProcPc_BG1_R89_AVX2.h"
//BG2 --------------------------------------------------------------------
#include "bnProcPc/nrLDPC_bnProcPc_BG2_R15_AVX2.h"
#include "bnProcPc/nrLDPC_bnProcPc_BG2_R13_AVX2.h"
#include "bnProcPc/nrLDPC_bnProcPc_BG2_R23_AVX2.h"

//bnProc----------------------------------------------------------------

#ifdef __AVX512BW__
//BG1-------------------------------------------------------------------
#include "bnProc_avx512/nrLDPC_bnProc_BG1_R13_AVX512.h"
#include "bnProc_avx512/nrLDPC_bnProc_BG1_R23_AVX512.h"
#include "bnProc_avx512/nrLDPC_bnProc_BG1_R89_AVX512.h"
//BG2 --------------------------------------------------------------------
#include "bnProc_avx512/nrLDPC_bnProc_BG2_R15_AVX512.h"
#include "bnProc_avx512/nrLDPC_bnProc_BG2_R13_AVX512.h"
#include "bnProc_avx512/nrLDPC_bnProc_BG2_R23_AVX512.h"

#else
#include "bnProc/nrLDPC_bnProc_BG1_R13_AVX2.h"
#include "bnProc/nrLDPC_bnProc_BG1_R23_AVX2.h"
#include "bnProc/nrLDPC_bnProc_BG1_R89_AVX2.h"
//BG2 --------------------------------------------------------------------
#include "bnProc/nrLDPC_bnProc_BG2_R15_AVX2.h"
#include "bnProc/nrLDPC_bnProc_BG2_R13_AVX2.h"
#include "bnProc/nrLDPC_bnProc_BG2_R23_AVX2.h"

#endif






#define NR_LDPC_ENABLE_PARITY_CHECK
//#define NR_LDPC_PROFILER_DETAIL

#ifdef NR_LDPC_DEBUG_MODE
#include "nrLDPC_tools/nrLDPC_debug.h"
#endif

static inline uint32_t nrLDPC_decoder_core(int8_t* p_llr, int8_t* p_out, uint32_t numLLR, t_nrLDPC_lut* p_lut, t_nrLDPC_dec_params* p_decParams, t_nrLDPC_time_stats* p_profiler);
int check_crc(uint8_t* decoded_bytes, uint32_t n, uint32_t F, uint8_t crc_type);
void nrLDPC_initcall(t_nrLDPC_dec_params* p_decParams, int8_t* p_llr, int8_t* p_out) {
}
int32_t nrLDPC_decod(t_nrLDPC_dec_params* p_decParams, int8_t* p_llr, int8_t* p_out, t_nrLDPC_time_stats* p_profiler)
{
    uint32_t numLLR;
    uint32_t numIter = 0;
    t_nrLDPC_lut lut;
    t_nrLDPC_lut* p_lut = &lut;

    // Initialize decoder core(s) with correct LUTs
    numLLR = nrLDPC_init(p_decParams, p_lut);

    // Launch LDPC decoder core for one segment
    numIter = nrLDPC_decoder_core(p_llr, p_out, numLLR, p_lut, p_decParams, p_profiler);

    return numIter;
}

/**
   \brief PerformsnrLDPC decoding of one code block
   \param p_llr Input LLRs
   \param p_out Output vector
   \param numLLR Number of LLRs
   \param p_lut Pointer to decoder LUTs
   \param p_decParamsnrLDPC decoder parameters
   \param p_profilernrLDPC profiler statistics
*/
static inline uint32_t nrLDPC_decoder_core(int8_t* p_llr, int8_t* p_out, uint32_t numLLR, t_nrLDPC_lut* p_lut, t_nrLDPC_dec_params* p_decParams, t_nrLDPC_time_stats* p_profiler)
{
    uint16_t Z          = p_decParams->Z;
    uint8_t  BG         = p_decParams->BG;
    uint8_t  R         = p_decParams->R; //Decoding rate: Format 15,13,... for code rates 1/5, 1/3,... */
    uint8_t  numMaxIter = p_decParams->numMaxIter;
    e_nrLDPC_outMode outMode = p_decParams->outMode;
   // int8_t* cnProcBuf=  cnProcBuf;
   // int8_t* cnProcBufRes= cnProcBufRes;

    int8_t cnProcBuf[NR_LDPC_SIZE_CN_PROC_BUF]    __attribute__ ((aligned(64))) = {0};
    int8_t cnProcBufRes[NR_LDPC_SIZE_CN_PROC_BUF] __attribute__ ((aligned(64))) = {0};
    int8_t bnProcBuf[NR_LDPC_SIZE_BN_PROC_BUF]    __attribute__ ((aligned(64))) = {0};
    int8_t bnProcBufRes[NR_LDPC_SIZE_BN_PROC_BUF] __attribute__ ((aligned(64))) = {0};
    int8_t llrRes[NR_LDPC_MAX_NUM_LLR]            __attribute__ ((aligned(64))) = {0};
    int8_t llrProcBuf[NR_LDPC_MAX_NUM_LLR]        __attribute__ ((aligned(64))) = {0};
    int8_t llrOut[NR_LDPC_MAX_NUM_LLR]            __attribute__ ((aligned(64))) = {0};
    // Minimum number of iterations is 1
    // 0 iterations means hard-decision on input LLRs
    uint32_t i = 1;
    // Initialize with parity check fail != 0
    int32_t pcRes = 1;
    int8_t* p_llrOut;

    if (outMode == nrLDPC_outMode_LLRINT8)
    {
        p_llrOut = p_out;
    }
    else
    {
        // Use LLR processing buffer as temporary output buffer
        p_llrOut = llrOut;
    }


    // Initialization
#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->llr2llrProcBuf);
#endif
    nrLDPC_llr2llrProcBuf(p_lut, p_llr, llrProcBuf, Z, BG);
#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->llr2llrProcBuf);
#endif

#ifdef NR_LDPC_DEBUG_MODE
    nrLDPC_debug_initBuffer2File(nrLDPC_buffers_LLR_PROC);
    nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_LLR_PROC, llrProcBuf);
#endif

#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->llr2CnProcBuf);
#endif
    if (BG == 1) nrLDPC_llr2CnProcBuf_BG1(p_lut, p_llr, cnProcBuf, Z);
    else nrLDPC_llr2CnProcBuf_BG2(p_lut, p_llr, cnProcBuf, Z);
#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->llr2CnProcBuf);
#endif

#ifdef NR_LDPC_DEBUG_MODE
    nrLDPC_debug_initBuffer2File(nrLDPC_buffers_CN_PROC);
    nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_CN_PROC, cnProcBuf);
#endif

    // First iteration

    // CN processing
#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->cnProc);
#endif
    if (BG==1) {
#ifndef UNROLL_CN_PROC      
        nrLDPC_cnProc_BG1(p_lut, cnProcBuf, cnProcBufRes, Z);
#else        
        switch (R)
        {
            case 13:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_R13_AVX512(cnProcBuf, cnProcBufRes, Z);
                #else
                nrLDPC_cnProc_BG1_R13_AVX2(cnProcBuf, cnProcBufRes, Z);
                #endif
                break;
            }

            case 23:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_R23_AVX512(cnProcBuf,cnProcBufRes, Z);
                #else
                nrLDPC_cnProc_BG1_R23_AVX2(cnProcBuf, cnProcBufRes, Z);
                #endif
                break;
            }

            case 89:
            {
                #ifdef __AVX512BW__
                 nrLDPC_cnProc_BG1_R89_AVX512(cnProcBuf, cnProcBufRes, Z);
                #else
                nrLDPC_cnProc_BG1_R89_AVX2(cnProcBuf, cnProcBufRes, Z);
                #endif
                break;
            }

        }
#endif        
    } else {
#ifndef UNROLL_CN_PROC
        nrLDPC_cnProc_BG2(p_lut, cnProcBuf, cnProcBufRes, Z);
#else
        switch (R) {
            case 15:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_R15_AVX512(cnProcBuf, cnProcBufRes, Z);
                #else
                nrLDPC_cnProc_BG2_R15_AVX2(cnProcBuf, cnProcBufRes, Z);
                #endif
                break;
            }
            case 13:
            {
                #ifdef __AVX512BW__
                 nrLDPC_cnProc_BG2_R13_AVX512(cnProcBuf, cnProcBufRes, Z);
                #else
                nrLDPC_cnProc_BG2_R13_AVX2(cnProcBuf, cnProcBufRes, Z);
                #endif
                break;
            }
            case 23:
            {
                #ifdef __AVX512BW__
                 nrLDPC_cnProc_BG2_R23_AVX512(cnProcBuf, cnProcBufRes, Z);
                #else
                nrLDPC_cnProc_BG2_R23_AVX2(cnProcBuf, cnProcBufRes, Z);
                #endif
                break;
            }

        }
#endif        
    }
#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->cnProc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
    nrLDPC_debug_initBuffer2File(nrLDPC_buffers_CN_PROC_RES);
    nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_CN_PROC_RES, cnProcBufRes);
#endif

#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->cn2bnProcBuf);
#endif
    if (BG == 1) nrLDPC_cn2bnProcBuf_BG1(p_lut, cnProcBufRes, bnProcBuf, Z);
    else         nrLDPC_cn2bnProcBuf_BG2(p_lut, cnProcBufRes, bnProcBuf, Z);
#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->cn2bnProcBuf);
#endif

#ifdef NR_LDPC_DEBUG_MODE
    nrLDPC_debug_initBuffer2File(nrLDPC_buffers_BN_PROC);
    nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_BN_PROC, bnProcBuf);
#endif

    // BN processing
#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->bnProcPc);
#endif


#ifndef UNROLL_BN_PROC_PC
    nrLDPC_bnProcPc(p_lut, bnProcBuf, bnProcBufRes, llrProcBuf, llrRes, Z);
#else        
    if (BG==1) {
        switch (R) {
            case 13:
            {
                nrLDPC_bnProcPc_BG1_R13_AVX2(bnProcBuf,bnProcBufRes,llrRes, llrProcBuf, Z);
                break;
            }
            case 23:
            {
                nrLDPC_bnProcPc_BG1_R23_AVX2(bnProcBuf,bnProcBufRes, llrRes, llrProcBuf, Z);
                break;
            }
            case 89:
            {
                nrLDPC_bnProcPc_BG1_R89_AVX2(bnProcBuf,bnProcBufRes, llrRes, llrProcBuf, Z);
                break;
            }
        }
    } else {
        switch (R) {
            case 15:
            {
                nrLDPC_bnProcPc_BG2_R15_AVX2(bnProcBuf,bnProcBufRes, llrRes, llrProcBuf, Z);
                break;
            }
            case 13:
            {
                nrLDPC_bnProcPc_BG2_R13_AVX2(bnProcBuf,bnProcBufRes,llrRes,llrProcBuf, Z);
                break;
            }

            case 23:
            {
                nrLDPC_bnProcPc_BG2_R23_AVX2(bnProcBuf,bnProcBufRes,llrRes, llrProcBuf, Z);
                break;
            }
        }
    }
#endif

#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->bnProcPc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
    nrLDPC_debug_initBuffer2File(nrLDPC_buffers_LLR_RES);
    nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_LLR_RES, llrRes);
#endif

#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->bnProc);
#endif

    if (BG==1) {
#ifndef UNROLL_BN_PROC
        nrLDPC_bnProc(p_lut, bnProcBuf, bnProcBufRes, llrRes, Z);
#else
        switch (R) {
            case 13:
            {
                #ifdef __AVX512BW__
                nrLDPC_bnProc_BG1_R13_AVX512(bnProcBuf, bnProcBufRes,llrRes, Z);
                #else
                nrLDPC_bnProc_BG1_R13_AVX2(bnProcBuf, bnProcBufRes,llrRes, Z);
                #endif
                break;
            }
            case 23:
            {
                #ifdef __AVX512BW__
                nrLDPC_bnProc_BG1_R23_AVX512(bnProcBuf, bnProcBufRes,llrRes, Z);
                #else
                nrLDPC_bnProc_BG1_R23_AVX2(bnProcBuf, bnProcBufRes,llrRes, Z);
                #endif
                break;
            }
            case 89:
            {
                #ifdef __AVX512BW__
                nrLDPC_bnProc_BG1_R89_AVX512(bnProcBuf, bnProcBufRes,llrRes, Z);
                #else
                nrLDPC_bnProc_BG1_R89_AVX2(bnProcBuf, bnProcBufRes,llrRes, Z);
                #endif
                break;
            }
        }
#endif
    } else {
#ifndef UNROLL_BN2CN_PROC
        nrLDPC_bn2cnProcBuf_BG2(p_lut, bnProcBufRes, cnProcBuf, Z);
#else
        switch (R) {
            case 15:
            {
                #ifdef __AVX512BW__
                nrLDPC_bnProc_BG2_R15_AVX512(bnProcBuf, bnProcBufRes,llrRes, Z);
                #else
                nrLDPC_bnProc_BG2_R15_AVX2(bnProcBuf, bnProcBufRes,llrRes, Z);
                #endif
                break;
            }
            case 13:
            {
                #ifdef __AVX512BW__
                nrLDPC_bnProc_BG2_R13_AVX512(bnProcBuf, bnProcBufRes,llrRes, Z);
                #else
                nrLDPC_bnProc_BG2_R13_AVX2(bnProcBuf, bnProcBufRes,llrRes, Z);
                #endif
                break;
            }

            case 23:
            {
                #ifdef __AVX512BW__
                nrLDPC_bnProc_BG2_R23_AVX512(bnProcBuf, bnProcBufRes,llrRes, Z);
                #else
                nrLDPC_bnProc_BG2_R23_AVX2(bnProcBuf, bnProcBufRes,llrRes, Z);
                #endif
                break;
            }
        }
#endif        
   }

#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->bnProc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
    nrLDPC_debug_initBuffer2File(nrLDPC_buffers_BN_PROC_RES);
    nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_BN_PROC_RES, bnProcBufRes);
#endif

    // BN results to CN processing buffer
#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->bn2cnProcBuf);
#endif
    if (BG == 1) nrLDPC_bn2cnProcBuf_BG1(p_lut, bnProcBufRes, cnProcBuf, Z);
    else         nrLDPC_bn2cnProcBuf_BG2(p_lut, bnProcBufRes, cnProcBuf, Z);
#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->bn2cnProcBuf);
#endif

#ifdef NR_LDPC_DEBUG_MODE
    nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_CN_PROC, cnProcBuf);
#endif

    // Parity Check not necessary here since it will fail
    // because first 2 cols/BNs in BG are punctured and cannot be
    // estimated after only one iteration

    // First iteration finished

    while ( (i < numMaxIter) && (pcRes != 0) ) {
        // Increase iteration counter
        i++;

        // CN processing
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->cnProc);
#endif
        if (BG==1) {
#ifndef UNROLL_CN_PROC
           nrLDPC_cnProc_BG1(p_lut, cnProcBuf, cnProcBufRes, Z);
#else        
           switch (R) {
            case 13:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_R13_AVX512(cnProcBuf, cnProcBufRes, Z);
                #else
                nrLDPC_cnProc_BG1_R13_AVX2(cnProcBuf, cnProcBufRes, Z);
                #endif
                break;
            }
            case 23:
            {
                #ifdef __AVX512BW__
                 nrLDPC_cnProc_BG1_R23_AVX512(cnProcBuf, cnProcBufRes, Z);
                #else
                nrLDPC_cnProc_BG1_R23_AVX2(cnProcBuf, cnProcBufRes, Z);
                #endif
                break;
            }
            case 89:
            {
                #ifdef __AVX512BW__
                 nrLDPC_cnProc_BG1_R89_AVX512(cnProcBuf, cnProcBufRes, Z);
                #else
                nrLDPC_cnProc_BG1_R89_AVX2(cnProcBuf, cnProcBufRes, Z);
                #endif
                break;
            }
           }
#endif        
        } else {
#ifndef UNROLL_CN_PROC
           nrLDPC_cnProc_BG2(p_lut, cnProcBuf, cnProcBufRes, Z);
#else
           switch (R) {
            case 15:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_R15_AVX512(cnProcBuf,cnProcBufRes, Z);
                #else
                nrLDPC_cnProc_BG2_R15_AVX2(cnProcBuf, cnProcBufRes, Z);
                #endif
                break;
            }
            case 13:
            {
                #ifdef __AVX512BW__
                 nrLDPC_cnProc_BG2_R13_AVX512(cnProcBuf, cnProcBufRes, Z);
                #else
                nrLDPC_cnProc_BG2_R13_AVX2(cnProcBuf, cnProcBufRes, Z);
                #endif
                break;
            } 
            case 23:
            {
                #ifdef __AVX512BW__
                 nrLDPC_cnProc_BG2_R23_AVX512(cnProcBuf, cnProcBufRes, Z);
                #else
                nrLDPC_cnProc_BG2_R23_AVX2(cnProcBuf, cnProcBufRes, Z);
                #endif
                break;
            }
          }  
#endif
        }
#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->cnProc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_CN_PROC_RES, cnProcBufRes);
#endif

        // Send CN results back to BNs
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->cn2bnProcBuf);
#endif
        if (BG == 1) nrLDPC_cn2bnProcBuf_BG1(p_lut, cnProcBufRes, bnProcBuf, Z);
        else         nrLDPC_cn2bnProcBuf_BG2(p_lut, cnProcBufRes, bnProcBuf, Z);
#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->cn2bnProcBuf);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_BN_PROC, bnProcBuf);
#endif

        // BN Processing
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->bnProcPc);
#endif

#ifndef UNROLL_BN_PROC_PC
        nrLDPC_bnProcPc(p_lut, bnProcBuf, bnProcBufRes, llrProcBuf, llrRes, Z);
#else
        if (BG==1) {
          switch (R) {
            case 13:
            {
                nrLDPC_bnProcPc_BG1_R13_AVX2(bnProcBuf,bnProcBufRes,llrRes, llrProcBuf, Z);
                break;
            }
            case 23:
            {
                nrLDPC_bnProcPc_BG1_R23_AVX2(bnProcBuf,bnProcBufRes, llrRes, llrProcBuf, Z);
                break;
            }
            case 89:
            {
                nrLDPC_bnProcPc_BG1_R89_AVX2(bnProcBuf,bnProcBufRes, llrRes, llrProcBuf, Z);
                break;
            }
          }
        } else {
          switch (R)
          {
            case 15:
            {
                nrLDPC_bnProcPc_BG2_R15_AVX2(bnProcBuf,bnProcBufRes,llrRes, llrProcBuf, Z);
                break;
            }
            case 13:
            {
                nrLDPC_bnProcPc_BG2_R13_AVX2(bnProcBuf,bnProcBufRes,llrRes, llrProcBuf, Z);
                break;
            }
            case 23:
            {
                nrLDPC_bnProcPc_BG2_R23_AVX2(bnProcBuf,bnProcBufRes,llrRes, llrProcBuf, Z);
                break;
            }
          }
        }
#endif
#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->bnProcPc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_LLR_RES, llrRes);
#endif

#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->bnProc);
#endif
#ifndef UNROLL_BN_PROC
        nrLDPC_bnProc(p_lut, bnProcBuf, bnProcBufRes, llrRes, Z);
#else     
        if (BG==1) {
          switch (R) {
            case 13:
            {
                #ifdef __AVX512BW__
                nrLDPC_bnProc_BG1_R13_AVX512(bnProcBuf, bnProcBufRes,llrRes, Z);
                #else
                nrLDPC_bnProc_BG1_R13_AVX2(bnProcBuf, bnProcBufRes,llrRes, Z);
                #endif
                break;
            }
            case 23:
            {
                #ifdef __AVX512BW__
                nrLDPC_bnProc_BG1_R23_AVX512(bnProcBuf, bnProcBufRes,llrRes, Z);
                #else
                nrLDPC_bnProc_BG1_R23_AVX2(bnProcBuf,bnProcBufRes,llrRes, Z);
                #endif
                break;
            }
            case 89:
            {
                #ifdef __AVX512BW__
                nrLDPC_bnProc_BG1_R89_AVX512(bnProcBuf, bnProcBufRes,llrRes, Z);
                #else
                nrLDPC_bnProc_BG1_R89_AVX2(bnProcBuf, bnProcBufRes,llrRes, Z);
                #endif
                break;
            }
          }
        } else {
          switch (R)
          {
            case 15:
            {
                #ifdef __AVX512BW__
                nrLDPC_bnProc_BG2_R15_AVX512(bnProcBuf, bnProcBufRes,llrRes, Z);
                #else
                nrLDPC_bnProc_BG2_R15_AVX2(bnProcBuf, bnProcBufRes,llrRes, Z);
                #endif
                break;
            }
            case 13:
            {
                #ifdef __AVX512BW__
                nrLDPC_bnProc_BG2_R13_AVX512(bnProcBuf, bnProcBufRes,llrRes, Z);
                #else
                nrLDPC_bnProc_BG2_R13_AVX2(bnProcBuf, bnProcBufRes,llrRes, Z);
                #endif
                break;
            }
            case 23:
            {
                #ifdef __AVX512BW__
                nrLDPC_bnProc_BG2_R23_AVX512(bnProcBuf, bnProcBufRes,llrRes, Z);
                #else
                nrLDPC_bnProc_BG2_R23_AVX2(bnProcBuf, bnProcBufRes,llrRes, Z);
                #endif
                break;
            }
          }
        }
#endif



#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->bnProc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_BN_PROC_RES, bnProcBufRes);
#endif

        // BN results to CN processing buffer
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->bn2cnProcBuf);
#endif
        if (BG == 1) nrLDPC_bn2cnProcBuf_BG1(p_lut, bnProcBufRes, cnProcBuf, Z);
        else         nrLDPC_bn2cnProcBuf_BG2(p_lut, bnProcBufRes, cnProcBuf, Z);
#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->bn2cnProcBuf);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_CN_PROC, cnProcBuf);
#endif

   // Parity Check
#ifdef NR_LDPC_ENABLE_PARITY_CHECK
#ifdef NR_LDPC_PROFILER_DETAIL
       start_meas(&p_profiler->cnProcPc);
#endif
       if (BG == 1) pcRes = nrLDPC_cnProcPc_BG1(p_lut, cnProcBuf, cnProcBufRes, Z);
       else         pcRes = nrLDPC_cnProcPc_BG2(p_lut, cnProcBuf, cnProcBufRes, Z);
#ifdef NR_LDPC_PROFILER_DETAIL
       stop_meas(&p_profiler->cnProcPc);
#endif
#endif
   } // end while

   // Last iteration
   if (pcRes != 0) i++;
    // Assign results from processing buffer to output
#ifdef NR_LDPC_PROFILER_DETAIL
   start_meas(&p_profiler->llrRes2llrOut);
#endif
   nrLDPC_llrRes2llrOut(p_lut, p_llrOut, llrRes, Z, BG);
#ifdef NR_LDPC_PROFILER_DETAIL
   stop_meas(&p_profiler->llrRes2llrOut);
#endif

    // Hard-decision
#ifdef NR_LDPC_PROFILER_DETAIL
   start_meas(&p_profiler->llr2bit);
#endif
   if (outMode == nrLDPC_outMode_BIT) nrLDPC_llr2bitPacked(p_out, p_llrOut, numLLR);
   else //if (outMode == nrLDPC_outMode_BITINT8)
     nrLDPC_llr2bit(p_out, p_llrOut, numLLR);
#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->llr2bit);
#endif

    return i;
}




