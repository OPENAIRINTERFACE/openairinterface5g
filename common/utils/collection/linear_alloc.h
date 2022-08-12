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

#ifndef LINEAR_ALLOC_H
#define LINEAR_ALLOC_H

#include <limits.h>

typedef unsigned int uid_t;
#define UID_LINEAR_ALLOCATOR_SIZE 1024
#define UID_LINEAR_ALLOCATOR_BITMAP_SIZE (((UID_LINEAR_ALLOCATOR_SIZE/8)/sizeof(unsigned int)) + 1)
typedef struct uid_linear_allocator_s {
  unsigned int bitmap[UID_LINEAR_ALLOCATOR_BITMAP_SIZE];
} uid_allocator_t;

static inline void uid_linear_allocator_init(uid_allocator_t *uia) {
  memset(uia, 0, sizeof(uid_allocator_t));
}

static inline uid_t uid_linear_allocator_new(uid_allocator_t *uia) {
  unsigned int bit_index = 1;
  uid_t        uid = 0;

  for (unsigned int i = 0; i < UID_LINEAR_ALLOCATOR_BITMAP_SIZE; i++) {
    if (uia->bitmap[i] != UINT_MAX) {
      bit_index = 1;
      uid       = 0;

      while ((uia->bitmap[i] & bit_index) == bit_index) {
        bit_index = bit_index << 1;
        uid       += 1;
      }

      uia->bitmap[i] |= bit_index;
      return uid + (i*sizeof(unsigned int)*8);
    }
  }

  return UINT_MAX;
}


static inline void uid_linear_allocator_free(uid_allocator_t *uia, uid_t uid) {
  const unsigned int i = uid/sizeof(unsigned int)/8;
  const unsigned int bit = uid % (sizeof(unsigned int) * 8);
  const unsigned int value = ~(1 << bit);

  if (i < UID_LINEAR_ALLOCATOR_BITMAP_SIZE) {
    uia->bitmap[i] &= value;
  }
}

#endif /* LINEAR_ALLOC_H */
