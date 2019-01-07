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

#ifndef _UTILS_COLLECTION_OBJ_HASH_TABLE_H_
#define _UTILS_COLLECTION_OBJ_HASH_TABLE_H_
#include<stdlib.h>
#include <stdint.h>
#include <stddef.h>

//#include "collection/hashtable/hashtable.h"
#include "hashtable.h"

typedef struct obj_hash_node_s {
    int                 key_size;
    void               *key;
    void               *data;
    struct obj_hash_node_s *next;
} obj_hash_node_t;

typedef struct obj_hash_table_s {
    hash_size_t         size;
    hash_size_t         num_elements;
    struct obj_hash_node_s **nodes;
    hash_size_t       (*hashfunc)(const void*, int);
    void              (*freekeyfunc)(void*);
    void              (*freedatafunc)(void*);
} obj_hash_table_t;

obj_hash_table_t   *obj_hashtable_create  (hash_size_t   size, hash_size_t (*hashfunc)(const void*, int ), void (*freekeyfunc)(void*), void (*freedatafunc)(void*));
hashtable_rc_t      obj_hashtable_destroy (obj_hash_table_t *hashtblP);
hashtable_rc_t      obj_hashtable_is_key_exists (obj_hash_table_t *hashtblP, void* keyP, int key_sizeP);
hashtable_rc_t      obj_hashtable_insert  (obj_hash_table_t *hashtblP,       void* keyP, int key_sizeP, void *dataP);
hashtable_rc_t      obj_hashtable_remove  (obj_hash_table_t *hashtblP, const void* keyP, int key_sizeP);
hashtable_rc_t      obj_hashtable_get     (obj_hash_table_t *hashtblP, const void* keyP, int key_sizeP, void ** dataP);
hashtable_rc_t      obj_hashtable_get_keys(obj_hash_table_t *hashtblP, void ** keysP, unsigned int *sizeP);
hashtable_rc_t      obj_hashtable_resize  (obj_hash_table_t *hashtblP, hash_size_t sizeP);

#endif

