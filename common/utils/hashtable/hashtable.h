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

#ifndef _UTILS_COLLECTION_HASH_TABLE_H_
#define _UTILS_COLLECTION_HASH_TABLE_H_
#include<stdlib.h>
#include <stdint.h>
#include <stddef.h>

typedef size_t   hash_size_t;
typedef uint64_t hash_key_t;

#define HASHTABLE_NOT_A_KEY_VALUE ((uint64_t)-1)

typedef enum hashtable_return_code_e {
  HASH_TABLE_OK                      = 0,
  HASH_TABLE_INSERT_OVERWRITTEN_DATA = 1,
  HASH_TABLE_KEY_NOT_EXISTS          = 2,
  HASH_TABLE_KEY_ALREADY_EXISTS      = 3,
  HASH_TABLE_BAD_PARAMETER_HASHTABLE = 4,
  HASH_TABLE_SYSTEM_ERROR            = 5,
  HASH_TABLE_CODE_MAX
} hashtable_rc_t;


typedef struct hash_node_s {
  hash_key_t          key;
  void               *data;
  struct hash_node_s *next;
} hash_node_t;

typedef struct hash_table_s {
  hash_size_t         size;
  struct hash_node_s **nodes;
  hash_size_t       (*hashfunc)(const hash_key_t);
  void              (*freefunc)(void *);
} hash_table_t;

char           *hashtable_rc_code2string(hashtable_rc_t rcP);
void            hash_free_int_func(void *memoryP);
hash_table_t   *hashtable_create (const hash_size_t   size, hash_size_t (*hashfunc)(const hash_key_t ), void (*freefunc)(void *));
hashtable_rc_t  hashtable_destroy(hash_table_t **hashtbl);
hashtable_rc_t  hashtable_is_key_exists (const hash_table_t *const hashtbl, const uint64_t key);
hashtable_rc_t  hashtable_dump_content (const hash_table_t *const hashtblP, char *const buffer_pP, int *const remaining_bytes_in_buffer_pP );
hashtable_rc_t  hashtable_insert (hash_table_t *const hashtbl, const hash_key_t key, void *data);
hashtable_rc_t  hashtable_remove (hash_table_t *const hashtbl, const hash_key_t key);
hashtable_rc_t  hashtable_get    (const hash_table_t *const hashtbl, const hash_key_t key, void **dataP);



#endif

