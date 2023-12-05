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
//============================================================================================================================
// encoder interface
#ifndef __NRLDPC_DEFS__H__
#define __NRLDPC_DEFS__H__
#include <openair1/PHY/defs_nr_common.h>
#include "openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_types.h"

/**
   \brief LDPC encoder
   \param 1 input
   \param 2 channel_input
   \param 3 int Zc
   \param 4 int Kb
   \param 5 short block_length
   \param 6 short BG
   \param 7 int n_segment
   \param 8 unsigned int macro_num
   \param 9-12 time_stats_t *tinput,*tprep, *tparity,*toutput
*/
typedef struct {
  unsigned int n_segments;          // optim8seg
  unsigned int macro_num; // optim8segmulti
  unsigned char gen_code; //orig
  time_stats_t *tinput;
  time_stats_t *tprep;
  time_stats_t *tparity;
  time_stats_t *toutput;
  int Kr;
  uint32_t Kb;
  uint32_t Zc;
  void *harq;
  /// Encoder BG
  uint8_t BG;
  /// Interleaver outputs
  unsigned char *output;
  /// Number of bits in "small" code segments
  uint32_t K;
  /// Number of "Filler" bits
  uint32_t F;
  /// Modulation order
  uint8_t Qm;
  uint32_t E;
  unsigned int G;
  // Redundancy version index
  uint8_t rv;
} encoder_implemparams_t;

typedef int32_t(LDPC_initfunc_t)(void);
typedef int32_t(LDPC_shutdownfunc_t)(void);
// decoder interface
/**
   \brief LDPC decoder API type definition
   \param p_decParams LDPC decoder parameters
   \param p_llr Input LLRs
   \param p_llrOut Output vector
   \param p_profiler LDPC profiler statistics
*/

typedef int32_t(LDPC_decoderfunc_t)(t_nrLDPC_dec_params *p_decParams,
                                    uint8_t harq_pid,
                                    uint8_t ulsch_id,
                                    uint8_t C,
                                    int8_t *p_llr,
                                    int8_t *p_out,
                                    t_nrLDPC_time_stats *,
                                    decode_abort_t *ab);
typedef int32_t(LDPC_encoderfunc_t)(uint8_t **, uint8_t **, encoder_implemparams_t *);

#endif
