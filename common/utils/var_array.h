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


/*! \file var_array.c
* \brief Very primitive variable length array ds. Here for compatibility
* reasons, should be replaced with a more capable data structure.
* @ingroup util
*/

#ifndef VAR_ARRAY_H_
#define VAR_ARRAY_H_

#include <stdlib.h>
#include "common/utils/assertions.h"

typedef struct {
  size_t size;
  size_t mallocedSize;
  size_t atomSize;
  size_t increment;
} varArray_t;

static inline varArray_t *initVarArray(size_t increment, size_t atomSize)
{
  varArray_t *tmp = malloc(sizeof(*tmp) + increment * atomSize);
  tmp->size = 0;
  tmp->atomSize = atomSize;
  tmp->mallocedSize = increment;
  tmp->increment = increment;
  return tmp;
}

static inline void *dataArray(varArray_t *input)
{
  return input + 1;
}

static inline void appendVarArray(varArray_t **inputPtr, void *data)
{
  varArray_t *input = *inputPtr;
  if (input->size >= input->mallocedSize) {
    input->mallocedSize += input->increment;
    *inputPtr = realloc(input, sizeof(varArray_t) + input->mallocedSize * input->atomSize);
    AssertFatal(*inputPtr, "no memory left");
    input = *inputPtr;
  }
  memcpy((uint8_t *)(input + 1) + input->atomSize * input->size++, data, input->atomSize);
}

static inline void freeVarArray(varArray_t *input)
{
  free(input);
}

#endif /* VAR_ARRAY_H_ */
