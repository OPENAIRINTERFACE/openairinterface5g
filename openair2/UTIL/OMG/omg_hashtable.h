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

/*! \file omg_hashtable.h
* \brief A 'C' implementation of a hashtable
* \author  S. Uppoor
* \date 2011
* \version 0.1
* \company INRIA
* \email: sandesh.uppor@inria.fr
* \note
* \warning
*/

#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include<sys/types.h>
#include<stdint.h>

#define HASH_LEN table->key_num
#define HASH(x,y) hash_table_do_hash(x,y,HASH_LEN)

// forward declaration
typedef struct hash_table_element hash_table_element_t;

/**
 * @struct hash_table_element "hashtable.h"
 * @brief stores an hash table element for use in the hash table
 */
struct hash_table_element {
  /**
   * store the length in bytes of the key
   */
  size_t key_len;
  /**
   * stores the length in bytes of the key (only for copy mode)
   */
  size_t value_len;
  /**
   * pointer to the key
   */
  void * key;
  /**
   * pointer to the value
   */
  void * value;
  /**
   * next chained key for this hash
   */
  hash_table_element_t * next;
};
#define hash_table_element_s sizeof(hash_table_element_t)

/**
 * @enum hash_table_mode defines the mode of operation of hash table
 */
typedef enum hash_table_mode {
  /** copy mode here values as well as key is copied */
  MODE_COPY,
  /** value reference mode, here ONLY key is copies and value is always referred */
  MODE_VALUEREF,
  /** in this mode all keys and values are referred */
  MODE_ALLREF
} hash_table_mode_t;

/**
 * @struct hash_table "hashtable.h"
 * @brief identifies the hashtable for which operations are to be performed
 */
typedef struct omg_hash_table_s {
  /**
   * the hash table array where all values are stored
   */
  hash_table_element_t  ** store_house;

  /**
   * mode of the hash table
   */
  hash_table_mode_t mode;

  /**
   * number of keys in the hash table
   */
  size_t key_count;

  /**
   * number of keys allocated in the hash table
   */
  uint16_t key_num;

  /**
   * the ratio of key_count / key_num at which the hash table should be expanded
   */
  size_t key_ratio;

} omg_hash_table_t;
#define SIZEOF_HASH_TABLE sizeof(omg_hash_table_t)


// element operations
/**
 * Function to create a now hash_table element
 * @returns hash_table_element_t object when success
 * @returns NULL when no memory
 */
hash_table_element_t * hash_table_element_new(void);

/**
 * Function to delete an hash table element
 * @param table table from which element has to be deleted
 * @param element hash table element to be deleted
 */
void hash_table_element_delete(omg_hash_table_t *, hash_table_element_t *);

/**
 * Function that returns a hash value for a given key and key_len
 * @param key pointer to the key
 * @param key_len length of the key
 * @param max_key max value of the hash to be returned by the function
 * @returns hash value belonging to [0, max_key)
 */
uint16_t hash_table_do_hash(void * key, size_t key_len, uint16_t max_key);

// hash table operations
/**
 * Fuction to create a new hash table
 * @param mode hash_table_mode which the hash table should follow
 * @returns omg_hash_table_t object which references the hash table
 * @returns NULL when no memory
 */
omg_hash_table_t * hash_table_new(hash_table_mode_t);

/**
 * Function to delete the hash table
 * @param table hash table to be deleted
 */
void hash_table_delete(omg_hash_table_t *);

/**
 * macro to add a key - value pair to the hash table
 * @note use this macro when size of key and/or value can be given by sizeof
 * @param table hash table to add element to
 * @param key pointer to the key for the hash table
 * @param value pointer to the value to be added against the key
 * @returns 0 on sucess
 * @returns -1 when no memory
 */
#define HT_ADD(table, key, value) hash_table_add(table, (void *) key, sizeof(*key), (void *) value, sizeof(*value))

/**
 * Function to add a key - value pair to the hash table, use HT_ADD macro
 * @param table hash table to add element to
 * @param key pointer to the key for the hash table
 * @param key_len length of the key in bytes
 * @param value pointer to the value to be added against the key
 * @param value_len length of the value in bytes
 * @returns 0 on sucess
 * @returns -1 when no memory
 */
int hash_table_add(omg_hash_table_t *, void *, size_t, void *, size_t);

/**
 * macro to remove an hash table element (for a given key) from a given hash table
 * @note use this macro when size of key and/or value can be given by sizeof
 * @param table hash table from which element has to be removed
 * @param key pointer to the key which has to be removed
 * @returns 0 on sucess
 * @returns -1 when key is not found
 */
#define HT_REMOVE(table, key) hash_table_remove(table, key, sizeof(*key))

/**
 * Function to remove an hash table element (for a given key) from a given hash table
 * @param table hash table from which element has to be removed
 * @param key pointer to the key which has to be removed
 * @param key_len size of the key in bytes
 * @returns 0 on sucess
 * @returns -1 when key is not found
 */
int hash_table_remove(omg_hash_table_t *, void *, size_t);

/**
 * macro to lookup a key in a particular table
 * @param table table to look key in
 * @param key pointer to key to be looked for
 * @returns NULL when key is not found in the hash table
 * @returns void* pointer to the value in the table
 */
#define HT_LOOKUP(table, key) hash_table_lookup(table, key, sizeof(*key))

/**
 * Function to lookup a key in a particular table
 * @note use this macro when size of key and/or value can be given by sizeof
 * @param table table to look key in
 * @param key pointer to key to be looked for
 * @param key_len size of the key to be searched
 * @returns NULL when key is not found in the hash table
 * @returns void* pointer to the value in the table
 */
void * hash_table_lookup(omg_hash_table_t *, void *, size_t);

/**
 * macro to look if the exists in the hash table
 * @note use this macro when size of key and/or value can be given by sizeof
 * @param key pointer to key to be looked for
 * @returns 0 when key is not found
 * @returns 1 when key is found
 */
#define HT_HAS_KEY(table, key) hash_table_has_key(table, key, sizeof(*key))

/**
 * Function to look if the exists in the hash table
 * @param key pointer to key to be looked for
 * @param key_len size of the key to be searched
 * @returns 0 when key is not found
 * @returns 1 when key is found
 */
int hash_table_has_key(omg_hash_table_t *, void *, size_t);

/**
 * Function to return all the keys in a given hash table
 * @param table hash table from which key are to be reterived
 * @param keys a void** pointer where keys are filled in (memory allocated internally and must be freed)
 * @return total number of keys filled in keys
 */
size_t hash_table_get_keys(omg_hash_table_t *, void **);

/**
 * Function to get all elements (key - value pairs) from the given hash table
 * @param table hash table from which elements have to be retrieved
 * @param elements a pointer to an array of hash_table_element_t pointer (malloced by function)
 * @returns 1 when no memory
 * @returns count of elements
 */
size_t hash_table_get_elements(omg_hash_table_t *, hash_table_element_t *** );

/**
 * Function to resize the hash table store house
 * @param table hash table to be resized
 * @param len new length of the hash table
 * @returns -1 when no elements in hash table
 * @returns -2 when no emmory for new store house
 * @returns 0 when sucess
 */
int hash_table_resize(omg_hash_table_t *, size_t);
#endif
