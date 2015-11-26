/* from: http://en.literateprograms.org/Hash_table_%28C%29#chunk%20def:node
 * Original licence Creative Commons CC0 1.0 Waiver.(http://creativecommons.org/publicdomain/zero/1.0/)
 */
/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
#ifndef _UTILS_COLLECTION_HASH_TABLE_H_
#define _UTILS_COLLECTION_HASH_TABLE_H_
#include<stdlib.h>
#include <stdint.h>
#include <stddef.h>

typedef size_t hash_size_t;

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
  uint64_t            key;
  void               *data;
  struct hash_node_s *next;
} hash_node_t;

typedef struct hash_table_s {
  hash_size_t         size;
  hash_size_t         num_elements;
  struct hash_node_s **nodes;
  hash_size_t       (*hashfunc)(const uint64_t);
  void              (*freefunc)(void*);
} hash_table_t;

char*           hashtable_rc_code2string(hashtable_rc_t rcP);
void            hash_free_int_func(void* memoryP);
hash_table_t   *hashtable_create (hash_size_t   size, hash_size_t (*hashfunc)(const uint64_t ), void (*freefunc)(void*));
hashtable_rc_t  hashtable_destroy(hash_table_t *hashtbl);
hashtable_rc_t  hashtable_is_key_exists (hash_table_t *hashtbl, const uint64_t key);
hashtable_rc_t  hashtable_apply_funct_on_elements (hash_table_t *hashtblP, void funct(uint64_t keyP, void* dataP, void* parameterP), void* parameterP);
hashtable_rc_t  hashtable_insert (hash_table_t *hashtbl, const uint64_t key, void *data);
hashtable_rc_t  hashtable_remove (hash_table_t *hashtbl, const uint64_t key);
hashtable_rc_t  hashtable_get    (hash_table_t *hashtbl, const uint64_t key, void **dataP);
hashtable_rc_t  hashtable_resize (hash_table_t *hashtbl, hash_size_t size);



#endif

