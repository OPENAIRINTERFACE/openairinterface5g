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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "obj_hashtable.h"
//-------------------------------------------------------------------------------------------------------------------------------
/*
 * Default hash function
 * def_hashfunc() is the default used by hashtable_create() when the user didn't specify one.
 * This is a simple/naive hash function which adds the key's ASCII char values. It will probably generate lots of collisions on large hash tables.
 */

static hash_size_t def_hashfunc(const void *keyP, int key_sizeP)
{
    hash_size_t hash=0;

    while(key_sizeP) hash^=((unsigned char*)keyP)[key_sizeP --];

    return hash;
}

//-------------------------------------------------------------------------------------------------------------------------------
/*
 * Initialisation
 * hashtable_create() sets up the initial structure of the hash table. The user specified size will be allocated and initialized to NULL.
 * The user can also specify a hash function. If the hashfunc argument is NULL, a default hash function is used.
 * If an error occurred, NULL is returned. All other values in the returned obj_hash_table_t pointer should be released with hashtable_destroy().
 */
obj_hash_table_t *obj_hashtable_create(hash_size_t sizeP, hash_size_t (*hashfuncP)(const void*, int ), void (*freekeyfuncP)(void*), void (*freedatafuncP)(void*))
{
    obj_hash_table_t *hashtbl;

    if(!(hashtbl=malloc(sizeof(obj_hash_table_t)))) return NULL;

    if(!(hashtbl->nodes=calloc(sizeP, sizeof(obj_hash_node_t*)))) {
        free(hashtbl);
        return NULL;
    }

    hashtbl->size=sizeP;

    if(hashfuncP) hashtbl->hashfunc=hashfuncP;
    else hashtbl->hashfunc=def_hashfunc;

    if(freekeyfuncP) hashtbl->freekeyfunc=freekeyfuncP;
    else hashtbl->freekeyfunc=free;

    if(freedatafuncP) hashtbl->freedatafunc=freedatafuncP;
    else hashtbl->freedatafunc=free;

    return hashtbl;
}
//-------------------------------------------------------------------------------------------------------------------------------
/*
 * Cleanup
 * The hashtable_destroy() walks through the linked lists for each possible hash value, and releases the elements. It also releases the nodes array and the obj_hash_table_t.
 */
hashtable_rc_t obj_hashtable_destroy(obj_hash_table_t *hashtblP)
{
    hash_size_t n;
    obj_hash_node_t *node, *oldnode;

    for(n=0; n<hashtblP->size; ++n) {
        node=hashtblP->nodes[n];
        while(node) {
            oldnode=node;
            node=node->next;
            hashtblP->freekeyfunc(oldnode->key);
            hashtblP->freedatafunc(oldnode->data);
            free(oldnode);
        }
    }
    free(hashtblP->nodes);
    free(hashtblP);
    return HASH_TABLE_OK;
}
//-------------------------------------------------------------------------------------------------------------------------------
hashtable_rc_t obj_hashtable_is_key_exists (obj_hash_table_t *hashtblP, void* keyP, int key_sizeP)
//-------------------------------------------------------------------------------------------------------------------------------
{
    obj_hash_node_t *node;
    hash_size_t      hash;

    if (hashtblP == NULL) {
        return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
    }
    hash=hashtblP->hashfunc(keyP, key_sizeP)%hashtblP->size;
    node=hashtblP->nodes[hash];
    while(node) {
        if(node->key == keyP) {
            return HASH_TABLE_OK;
        } else if (node->key_size == key_sizeP) {
            if (memcmp(node->key, keyP, key_sizeP) == 0) {
                return HASH_TABLE_OK;
            }
        }
        node=node->next;
    }
    return HASH_TABLE_KEY_NOT_EXISTS;
}
//-------------------------------------------------------------------------------------------------------------------------------
/*
 * Adding a new element
 * To make sure the hash value is not bigger than size, the result of the user provided hash function is used modulo size.
 */
hashtable_rc_t obj_hashtable_insert(obj_hash_table_t *hashtblP, void* keyP, int key_sizeP, void *dataP)
{
    obj_hash_node_t *node;
    hash_size_t      hash;

    if (hashtblP == NULL) {
        return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
    }
    hash=hashtblP->hashfunc(keyP, key_sizeP)%hashtblP->size;
    node=hashtblP->nodes[hash];
    while(node) {
        if(node->key == keyP) {
            if (node->data) {
                hashtblP->freedatafunc(node->data);
            }
            node->data=dataP;
            // waste of memory here (keyP is lost) we should free it now
            return HASH_TABLE_INSERT_OVERWRITTEN_DATA;
        }
        node=node->next;
    }
    if(!(node=malloc(sizeof(obj_hash_node_t)))) return -1;
    node->key=keyP;
    node->data=dataP;
    if (hashtblP->nodes[hash]) {
        node->next=hashtblP->nodes[hash];
    } else {
        node->next = NULL;
    }
    hashtblP->nodes[hash]=node;
    return HASH_TABLE_OK;
}
//-------------------------------------------------------------------------------------------------------------------------------
/*
 * To remove an element from the hash table, we just search for it in the linked list for that hash value,
 * and remove it if it is found. If it was not found, it is an error and -1 is returned.
 */
hashtable_rc_t obj_hashtable_remove(obj_hash_table_t *hashtblP, const void* keyP, int key_sizeP)
{
    obj_hash_node_t *node, *prevnode=NULL;
    hash_size_t      hash;

    if (hashtblP == NULL) {
        return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
    }

    hash=hashtblP->hashfunc(keyP, key_sizeP)%hashtblP->size;
    node=hashtblP->nodes[hash];
    while(node) {
        if ((node->key == keyP) || ((node->key_size == key_sizeP) && (memcmp(node->key, keyP, key_sizeP) == 0))){
            if(prevnode) {
                prevnode->next=node->next;
            } else {
                hashtblP->nodes[hash]=node->next;
            }
            hashtblP->freekeyfunc(node->key);
            hashtblP->freedatafunc(node->data);
            free(node);
            return HASH_TABLE_OK;
        }
        prevnode=node;
        node=node->next;
    }
    return HASH_TABLE_KEY_NOT_EXISTS;
}
//-------------------------------------------------------------------------------------------------------------------------------
/*
 * Searching for an element is easy. We just search through the linked list for the corresponding hash value.
 * NULL is returned if we didn't find it.
 */
hashtable_rc_t obj_hashtable_get(obj_hash_table_t *hashtblP, const void* keyP, int key_sizeP, void** dataP)
{
    obj_hash_node_t *node;
    hash_size_t      hash;

    if (hashtblP == NULL) {
        *dataP = NULL;
        return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
    }
    hash=hashtblP->hashfunc(keyP, key_sizeP)%hashtblP->size;
    node=hashtblP->nodes[hash];
    while(node) {
        if(node->key == keyP) {
            *dataP = node->data;
            return HASH_TABLE_OK;
        } else if (node->key_size == key_sizeP) {
            if (memcmp(node->key, keyP, key_sizeP) == 0) {
                *dataP = node->data;
                return HASH_TABLE_OK;
            }
        }
        node=node->next;
    }
    *dataP = NULL;
    return HASH_TABLE_KEY_NOT_EXISTS;
}
//-------------------------------------------------------------------------------------------------------------------------------
/*
 * Function to return all keys of an object hash table
 */
hashtable_rc_t obj_hashtable_get_keys(obj_hash_table_t *hashtblP, void ** keysP, unsigned int *sizeP)
{
    size_t                 n     = 0;
    obj_hash_node_t       *node  = NULL;
    obj_hash_node_t       *next  = NULL;

    *sizeP = 0;
    keysP = calloc(hashtblP->num_elements, sizeof(void *));
    if (keysP) {
        for(n=0; n<hashtblP->size; ++n) {
            for(node=hashtblP->nodes[n]; node; node=next) {
                keysP[*sizeP++] = node->key;
                next = node->next;
            }
        }
        return HASH_TABLE_OK;
    }
    return HASH_TABLE_SYSTEM_ERROR;
}
//-------------------------------------------------------------------------------------------------------------------------------
/*
 * Resizing
 * The number of elements in a hash table is not always known when creating the table.
 * If the number of elements grows too large, it will seriously reduce the performance of most hash table operations.
 * If the number of elements are reduced, the hash table will waste memory. That is why we provide a function for resizing the table.
 * Resizing a hash table is not as easy as a realloc(). All hash values must be recalculated and each element must be inserted into its new position.
 * We create a temporary obj_hash_table_t object (newtbl) to be used while building the new hashes.
 * This allows us to reuse hashtable_insert() and hashtable_remove(), when moving the elements to the new table.
 * After that, we can just free the old table and copy the elements from newtbl to hashtbl.
 */
hashtable_rc_t obj_hashtable_resize(obj_hash_table_t *hashtblP, hash_size_t sizeP)
{
    obj_hash_table_t       newtbl;
    hash_size_t        n;
    obj_hash_node_t       *node,*next;

    if (hashtblP == NULL) {
        return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
    }

    newtbl.size     = sizeP;
    newtbl.hashfunc = hashtblP->hashfunc;

    if(!(newtbl.nodes=calloc(sizeP, sizeof(obj_hash_node_t*)))) return HASH_TABLE_SYSTEM_ERROR;

    for(n=0; n<hashtblP->size; ++n) {
        for(node=hashtblP->nodes[n]; node; node=next) {
            next = node->next;
            obj_hashtable_insert(&newtbl, node->key, node->key_size, node->data);
            obj_hashtable_remove(hashtblP, node->key, node->key_size);
        }
    }

    free(hashtblP->nodes);
    hashtblP->size=newtbl.size;
    hashtblP->nodes=newtbl.nodes;

    return HASH_TABLE_OK;
}



