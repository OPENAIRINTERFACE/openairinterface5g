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

