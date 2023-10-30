
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

/*! \file rt_profiling.h
 * \brief Definitions for proflling real-time scheduling
 * \author
 * \date 2022
 * \version 0.1
 * \company Eurecom
 * \email:
 * \note
 * \warning
 */
#ifndef TIME_PROFILING_H
#define TIME_PROFILING_H
#ifdef __cplusplus
extern "C"
{
#endif

// depth of trace in slots
#define RT_PROF_DEPTH 100
typedef struct {
   int absslot_rx[RT_PROF_DEPTH];
   struct timespec return_RU_south_in[RT_PROF_DEPTH];
   struct timespec return_RU_feprx[RT_PROF_DEPTH];
   struct timespec return_RU_prachrx[RT_PROF_DEPTH];
   struct timespec return_RU_pushL1[RT_PROF_DEPTH];
   struct timespec start_RU_TX[RT_PROF_DEPTH];
   struct timespec return_RU_TX[RT_PROF_DEPTH];
} rt_ru_profiling_t;

typedef struct {
   int absslot_ux[RT_PROF_DEPTH];
   struct timespec start_L1_RX[RT_PROF_DEPTH];
   struct timespec return_L1_RX[RT_PROF_DEPTH];
   struct timespec start_L1_TX[RT_PROF_DEPTH];
   struct timespec return_L1_TX[RT_PROF_DEPTH];
   struct timespec return_L1_prachrx[RT_PROF_DEPTH];
   struct timespec return_L1_puschL1[RT_PROF_DEPTH];
} rt_L1_profiling_t;
#ifdef __cplusplus
}
#endif
#endif
