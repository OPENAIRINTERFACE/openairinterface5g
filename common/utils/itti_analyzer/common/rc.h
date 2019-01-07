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

#include <errno.h>
#include <string.h>

#ifndef RC_H_
#define RC_H_

#define RC_OK 0
#define RC_FAIL         -1
#define RC_BAD_PARAM    -2
#define RC_NULL_POINTER -3

static const char * const rc_strings[] =
    {"Ok", "fail", "bad parameter", "null pointer"};

#define CHECK_FCT(fCT)              \
do {                                \
    int rET;                        \
    if ((rET = fCT) != RC_OK) {     \
        fprintf(stderr, #fCT" has failed (%s:%d)\n", __FILE__, __LINE__);   \
        return rET;                 \
    }                               \
} while(0)

#define CHECK_FCT_POSIX(fCT)        \
do {                                \
    if (fCT == -1) {                \
        fprintf(stderr, #fCT" has failed (%d:%s) (%s:%d)\n", errno, \
                strerror(errno), __FILE__, __LINE__);               \
        return RC_FAIL;             \
    }                               \
} while(0)

#define CHECK_FCT_DO(fCT, dO)       \
do {                                \
    int rET;                        \
    if ((rET = fCT) != RC_OK) {     \
        fprintf(stderr, #fCT" has returned %d (%s:%d)\n", rET, __FILE__, __LINE__);   \
        dO;                         \
    }                               \
} while(0)

#define CHECK_BUFFER(bUFFER)        \
do {                                \
    if ((bUFFER) == NULL) {         \
        fprintf(stderr, #bUFFER" is NULL (%s:%d)\n", __FILE__, __LINE__);   \
        return RC_NULL_POINTER;     \
    }                               \
} while(0)

#endif /* RC_H_ */
