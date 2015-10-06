/* from: http://en.literateprograms.org/Hash_table_%28C%29#chunk%20def:node
 * Original licence Creative Commons CC0 1.0 Waiver.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"


//-------------------------------------------------------------------------------------------------------------------------------
char* hashtable_rc_code2string(hashtable_rc_t rcP)
//-------------------------------------------------------------------------------------------------------------------------------
{
  switch (rcP) {
  case HASH_TABLE_OK:
    return "HASH_TABLE_OK";
    break;

  case HASH_TABLE_INSERT_OVERWRITTEN_DATA:
    return "HASH_TABLE_INSERT_OVERWRITTEN_DATA";
    break;

  case HASH_TABLE_KEY_NOT_EXISTS:
    return "HASH_TABLE_KEY_NOT_EXISTS";
    break;

  case HASH_TABLE_KEY_ALREADY_EXISTS:
    return "HASH_TABLE_KEY_ALREADY_EXISTS";
    break;

  case HASH_TABLE_BAD_PARAMETER_HASHTABLE:
    return "HASH_TABLE_BAD_PARAMETER_HASHTABLE";
    break;

  default:
    return "UNKNOWN hashtable_rc_t";
  }
}
//-------------------------------------------------------------------------------------------------------------------------------
/*
 * free int function
 * hash_free_int_func() is used when this hashtable is used to store int values as data (pointer = value).
 */

void hash_free_int_func(void* memoryP) {}

//-------------------------------------------------------------------------------------------------------------------------------
/*
 * Default hash function
 * def_hashfunc() is the default used by hashtable_create() when the user didn't specify one.
 * This is a simple/naive hash function which adds the key's ASCII char values. It will probably generate lots of collisions on large hash tables.
 */

static hash_size_t def_hashfunc(const uint64_t keyP)
{
  return (hash_size_t)keyP;
}

//-------------------------------------------------------------------------------------------------------------------------------
/*
 * Initialisation
 * hashtable_create() sets up the initial structure of the hash table. The user specified size will be allocated and initialized to NULL.
 * The user can also specify a hash function. If the hashfunc argument is NULL, a default hash function is used.
 * If an error occurred, NULL is returned. All other values in the returned hash_table_t pointer should be released with hashtable_destroy().
 */
hash_table_t *hashtable_create(hash_size_t sizeP, hash_size_t (*hashfuncP)(const uint64_t ), void (*freefuncP)(void*))
{
  hash_table_t *hashtbl;

  if(!(hashtbl=malloc(sizeof(hash_table_t)))) return NULL;

  if(!(hashtbl->nodes=calloc(sizeP, sizeof(hash_node_t*)))) {
    free(hashtbl);
    return NULL;
  }

  hashtbl->size=sizeP;

  if(hashfuncP) hashtbl->hashfunc=hashfuncP;
  else hashtbl->hashfunc=def_hashfunc;

  if(freefuncP) hashtbl->freefunc=freefuncP;
  else hashtbl->freefunc=free;

  return hashtbl;
}
//-------------------------------------------------------------------------------------------------------------------------------
/*
 * Cleanup
 * The hashtable_destroy() walks through the linked lists for each possible hash value, and releases the elements. It also releases the nodes array and the hash_table_t.
 */
hashtable_rc_t hashtable_destroy(hash_table_t *hashtblP)
{
  hash_size_t n;
  hash_node_t *node, *oldnode;

  if (hashtblP == NULL) {
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  for(n=0; n<hashtblP->size; ++n) {
    node=hashtblP->nodes[n];

    while(node) {
      oldnode=node;
      node=node->next;

      if (oldnode->data) {
        hashtblP->freefunc(oldnode->data);
      }

      free(oldnode);
    }
  }

  free(hashtblP->nodes);
  free(hashtblP);
  return HASH_TABLE_OK;
}
//-------------------------------------------------------------------------------------------------------------------------------
hashtable_rc_t hashtable_is_key_exists (hash_table_t *hashtblP, const uint64_t keyP)
//-------------------------------------------------------------------------------------------------------------------------------
{
  hash_node_t *node;
  hash_size_t  hash;

  if (hashtblP == NULL) {
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  hash=hashtblP->hashfunc(keyP)%hashtblP->size;
  node=hashtblP->nodes[hash];

  while(node) {
    if(node->key == keyP) {
      return HASH_TABLE_OK;
    }

    node=node->next;
  }

  return HASH_TABLE_KEY_NOT_EXISTS;
}
//-------------------------------------------------------------------------------------------------------------------------------
hashtable_rc_t hashtable_apply_funct_on_elements (hash_table_t *hashtblP, void functP(uint64_t keyP, void* dataP, void* parameterP), void* parameterP)
//-------------------------------------------------------------------------------------------------------------------------------
{
  hash_node_t  *node         = NULL;
  unsigned int  i            = 0;
  unsigned int  num_elements = 0;

  if (hashtblP == NULL) {
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  while ((num_elements < hashtblP->num_elements) && (i < hashtblP->size)) {
    if (hashtblP->nodes[i] != NULL) {
      node=hashtblP->nodes[i];

      while(node) {
        num_elements += 1;
        functP(node->key, node->data, parameterP);
        node=node->next;
      }
    }

    i += 1;
  }

  return HASH_TABLE_OK;
}
//-------------------------------------------------------------------------------------------------------------------------------
/*
 * Adding a new element
 * To make sure the hash value is not bigger than size, the result of the user provided hash function is used modulo size.
 */
hashtable_rc_t hashtable_insert(hash_table_t *hashtblP, const uint64_t keyP, void *dataP)
{
  hash_node_t *node;
  hash_size_t hash;

  if (hashtblP == NULL) {
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  hash=hashtblP->hashfunc(keyP)%hashtblP->size;

  node=hashtblP->nodes[hash];

  while(node) {
    if(node->key == keyP) {
      if (node->data) {
        hashtblP->freefunc(node->data);
      }

      node->data=dataP;
      return HASH_TABLE_INSERT_OVERWRITTEN_DATA;
    }

    node=node->next;
  }

  if(!(node=malloc(sizeof(hash_node_t)))) return -1;

  node->key=keyP;
  node->data=dataP;

  if (hashtblP->nodes[hash]) {
    node->next=hashtblP->nodes[hash];
  } else {
    node->next = NULL;
  }

  hashtblP->nodes[hash]=node;
  hashtblP->num_elements += 1;
  return HASH_TABLE_OK;
}
//-------------------------------------------------------------------------------------------------------------------------------
/*
 * To remove an element from the hash table, we just search for it in the linked list for that hash value,
 * and remove it if it is found. If it was not found, it is an error and -1 is returned.
 */
hashtable_rc_t hashtable_remove(hash_table_t *hashtblP, const uint64_t keyP)
{
  hash_node_t *node, *prevnode=NULL;
  hash_size_t  hash;

  if (hashtblP == NULL) {
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  hash=hashtblP->hashfunc(keyP)%hashtblP->size;
  node=hashtblP->nodes[hash];

  while(node) {
    if(node->key == keyP) {
      if(prevnode) prevnode->next=node->next;
      else hashtblP->nodes[hash]=node->next;

      if (node->data) {
        hashtblP->freefunc(node->data);
      }

      free(node);
      hashtblP->num_elements -= 1;
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
hashtable_rc_t hashtable_get(hash_table_t *hashtblP, const uint64_t keyP, void** dataP)
{
  hash_node_t *node;
  hash_size_t  hash;

  if (hashtblP == NULL) {
    *dataP = NULL;
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  hash=hashtblP->hashfunc(keyP)%hashtblP->size;
  /*  fprintf(stderr, "hashtable_get() key=%s, hash=%d\n", key, hash);*/

  node=hashtblP->nodes[hash];

  while(node) {
    if(node->key == keyP) {
      *dataP = node->data;
      return HASH_TABLE_OK;
    }

    node=node->next;
  }

  *dataP = NULL;
  return HASH_TABLE_KEY_NOT_EXISTS;
}
//-------------------------------------------------------------------------------------------------------------------------------
/*
 * Resizing
 * The number of elements in a hash table is not always known when creating the table.
 * If the number of elements grows too large, it will seriously reduce the performance of most hash table operations.
 * If the number of elements are reduced, the hash table will waste memory. That is why we provide a function for resizing the table.
 * Resizing a hash table is not as easy as a realloc(). All hash values must be recalculated and each element must be inserted into its new position.
 * We create a temporary hash_table_t object (newtbl) to be used while building the new hashes.
 * This allows us to reuse hashtable_insert() and hashtable_remove(), when moving the elements to the new table.
 * After that, we can just free the old table and copy the elements from newtbl to hashtbl.
 */

hashtable_rc_t hashtable_resize(hash_table_t *hashtblP, hash_size_t sizeP)
{
  hash_table_t       newtbl;
  hash_size_t        n;
  hash_node_t       *node,*next;

  if (hashtblP == NULL) {
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  newtbl.size     = sizeP;
  newtbl.hashfunc = hashtblP->hashfunc;

  if(!(newtbl.nodes=calloc(sizeP, sizeof(hash_node_t*)))) return -1;

  for(n=0; n<hashtblP->size; ++n) {
    for(node=hashtblP->nodes[n]; node; node=next) {
      next = node->next;
      hashtable_insert(&newtbl, node->key, node->data);
      // Lionel GAUTHIER: BAD CODE TO BE REWRITTEN
      hashtable_remove(hashtblP, node->key);

    }
  }

  free(hashtblP->nodes);
  hashtblP->size=newtbl.size;
  hashtblP->nodes=newtbl.nodes;

  return HASH_TABLE_OK;
}



