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

/*!\file common/utils/T/T_defs.h
 * \brief
 * \author
 * \date
 * \version
 * \company EURECOM
 * \email
 * \note
 * \warning
*/

#ifndef _T_defs_H_
#define _T_defs_H_

/* comment (and recompile everything) to not send time in events */
#define T_SEND_TIME

/* maximum number of arguments for the T macro */
#define T_MAX_ARGS 16

/* maximum size of a message - increase if needed */
#if BASIC_SIMULATOR
   /* let's have 100 RBs functional for the basic simulator */
#  define T_BUFFER_MAX (1024*64*4)
#else
#  define T_BUFFER_MAX (1024*64*4)
#endif

/* size of the local cache for messages (must be pow(2,something)) */
#if BASIC_SIMULATOR
   /* we don't need much space for the basic simulator */
#  define T_CACHE_SIZE 1024
#else
#  define T_CACHE_SIZE (8192)
#endif

/* maximum number of bytes a message can contain */
#ifdef T_SEND_TIME
#  define T_PAYLOAD_MAXSIZE (T_BUFFER_MAX-sizeof(int)-sizeof(struct timespec))
#else
#  define T_PAYLOAD_MAXSIZE (T_BUFFER_MAX-sizeof(int))
#endif

typedef struct {
  /* 'busy' is a bit field
   * bit 0: 1 means that slot is acquired by writer
   * bit 1: 1 means that slot is ready for consumption
   */
  volatile int busy;
  char buffer[T_BUFFER_MAX];
  int length;
} T_cache_t;

/* number of VCD functions (to be kept up to date! see in T_messages.txt) */
#define VCD_NUM_FUNCTIONS (273)

/* number of VCD variables (to be kept up to date! see in T_messages.txt) */
#define VCD_NUM_VARIABLES (187)

/* first VCD function (to be kept up to date! see in T_messages.txt) */
#define VCD_FIRST_FUNCTION    ((uintptr_t)T_VCD_FUNCTION_RT_SLEEP)

/* first VCD variable (to be kept up to date! see in T_messages.txt) */
#define VCD_FIRST_VARIABLE    ((uintptr_t)T_VCD_VARIABLE_FRAME_NUMBER_TX0_ENB)

#endif /* _T_defs_H_ */
