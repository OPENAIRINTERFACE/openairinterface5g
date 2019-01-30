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

/*! \file omg_hashtable.c
* \brief A 'C' implementation of a hashtable
* \author  S. Uppoor
* \date 2011
* \version 0.1
* \company INRIA
* \email: sandesh.uppor@inria.fr
* \note
* \warning
*/


#include "omg_hashtable.h"

#include <stdlib.h>
#include <string.h>


// element operations
/**
 * Function to create a now hash_table element
 * @returns hash_table_element_t object when success
 * @returns NULL when no memory
 */
hash_table_element_t *hash_table_element_new(void) {
  //INFO("creating a new hash table element");
  return calloc(1, hash_table_element_s);
}

/**
 * Function to delete an hash table element
 * @param table table from which element has to be deleted
 * @param element hash table element to be deleted
 */
void hash_table_element_delete(omg_hash_table_t *table, hash_table_element_t *element) {
  //INFO("Deleting an hash table element");
  if (table->mode == MODE_COPY) {
    free(element->value);
    free(element->key);
  } else if (table->mode == MODE_VALUEREF) {
    free(element->key);
  }

  free(element);
}

// hash table operations
/**
 * Fuction to create a new hash table
 * @param mode hash_table_mode which the hash table should follow
 * @returns omg_hash_table_t object which references the hash table
 * @returns NULL when no memory
 */
omg_hash_table_t *hash_table_new(hash_table_mode_t mode) {
  //INFO("Creating a new hash table");
  omg_hash_table_t *table = calloc(1, SIZEOF_HASH_TABLE);

  if (!table) {
    //INFO("No Memory while allocating hash_table");
    return NULL;
  }

  table->mode = mode;
  table->key_num = 128;
  table->key_ratio = 4;
  table->store_house = (hash_table_element_t **) calloc(table->key_num, sizeof(hash_table_element_t *));

  if (!table->store_house) {
    //INFO("No Memory while allocating hash_table store house");
    free(table);
    return NULL;
  }

  return table;
}

/**
 * Function to delete the hash table
 * @param table hash table to be deleted
 */
void hash_table_delete(omg_hash_table_t *table) {
  //INFO("Deleating a hash table");
  size_t i=0;

  for (; i<HASH_LEN; i++) {
    while (table->store_house[i]) {
      hash_table_element_t *temp = table->store_house[i];
      table->store_house[i] = table->store_house[i]->next;
      hash_table_element_delete(table, temp);
    }
  }

  free(table->store_house);
  free(table);
}

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
int hash_table_add(omg_hash_table_t *table, void *key, size_t key_len, void *value, size_t value_len) {
  if ((table->key_count / table->key_num) >= table->key_ratio) {
    //LOG("Ratio(%d) reached the set limit %d\nExpanding hash_table", (table->key_count / table->key_num), table->key_ratio);
    hash_table_resize(table, table->key_num*2);
    //exit(0);
  }

  size_t hash = HASH(key, key_len);
  hash_table_element_t *element = hash_table_element_new();

  if (!element) {
    //INFO("Cannot allocate memory for element");
    return -1; // No Memory
  }

  if (table->mode == MODE_COPY) {
    //LOG("Adding a key-value pair to the hash table with hash -> %d, in COPY MODE", (int)hash);
    element->key = malloc(key_len);
    element->value = malloc(value_len);

    if (element->key && element->value) {
      memcpy(element->key, key, key_len);
      memcpy(element->value, value, value_len);
    } else {
      if (element->key) {
        free(element->key);
        //INFO("Cannot allocate memory for value");
      }

      if (element->value) {
        free(element->value);
        //INFO("Cannot allocate memory for key");
      }

      free(element);
      return -1; //No Memory
    }
  } else if (table->mode == MODE_VALUEREF) {
    //LOG("Adding a key-value pair to the hash table with hash -> %d, in VALUEREF MODE", (int)hash);
    element->key = malloc(key_len);

    if (element->key) {
      memcpy(element->key, key, key_len);
    } else {
      //INFO("Cannot allocate memory for key");
      free(element);
      return -1; //No Memory
    }

    element->value = value;
  } else if (table->mode == MODE_ALLREF) {
    //LOG("Adding a key-value pair to the hash table with hash -> %d, in ALLREF MODE", (int)hash);
    element->key = key;
    element->value = value;
  }

  element->key_len = key_len;
  element->value_len = value_len;
  element->next = NULL;

  // find the key position for chaining
  if (!table->store_house[hash]) {
    //LOG("No Conflicts adding the first element at %d", (int)hash);
    table->store_house[hash] = element;
    table->key_count++;
  } else {
    //LOG("Conflicts adding element at %d", (int)hash);
    hash_table_element_t *temp = table->store_house[hash];

    while(temp->next) {
      while(temp->next && temp->next->key_len!=key_len) {
        temp = temp->next;
      }

      if(temp->next) {
        if (!memcmp(temp->next->key, key, key_len)) {
          //LOG("Found Key at hash -> %d", (int)hash);
          hash_table_element_t *to_delete = temp->next;
          temp->next = element;
          element->next = to_delete->next;
          hash_table_element_delete(table, to_delete);
          // since we are replacing values no need to change key_count
          return 0;
        } else {
          temp = temp->next;
        }
      }
    }

    temp->next = element;
    table->key_count++;
  }

  return 0;
}

/**
 * Function to remove an hash table element (for a given key) from a given hash table
 * @param table hash table from which element has to be removed
 * @param key pointer to the key which has to be removed
 * @param key_len size of the key in bytes
 * @returns 0 on sucess
 * @returns -1 when key is not found
 */
int hash_table_remove(omg_hash_table_t *table, void *key, size_t key_len) {
  //INFO("Deleting a key-value pair from the hash table");
  if ((table->key_num/ table->key_count) >= table->key_ratio) {
    //LOG("Ratio(%d) reached the set limit %d\nContracting hash_table", (table->key_num / table->key_count), table->key_ratio);
    hash_table_resize(table, table->key_num/2);
    //exit(0);
  }

  size_t hash = HASH(key, key_len);

  if (!table->store_house[hash]) {
    //LOG("Key Not Found -> No element at %d", (int)hash);
    return -1; // key not found
  }

  hash_table_element_t *temp = table->store_house[hash];
  hash_table_element_t *prev = temp;

  while(temp) {
    while(temp && temp->key_len!=key_len) {
      prev = temp;
      temp = temp->next;
    }

    if(temp) {
      if (!memcmp(temp->key, key, key_len)) {
        if (prev == table->store_house[hash]) {
          table->store_house[hash] = temp->next;
        } else {
          prev->next = temp->next;
        }

        hash_table_element_delete(table, temp);
        //INFO("Deleted a key-value pair from the hash table");
        table->key_count--;
        return 0;
      }

      prev=temp;
      temp=temp->next;
    }
  }

  //INFO("Key Not Found");
  return -1; // key not found
}

/**
 * Function to lookup a key in a particular table
 * @param table table to look key in
 * @param key pointer to key to be looked for
 * @param key_len size of the key to be searched
 * @returns NULL when key is not found in the hash table
 * @returns void* pointer to the value in the table
 */
void *hash_table_lookup(omg_hash_table_t *table, void *key, size_t key_len) {
  size_t hash = HASH(key, key_len);

  //LOG("Looking up a key-value pair for hash -> %d", (int)hash);
  if (!table->store_house[hash]) {
    //LOG("Key not found at hash %d, no entries", (int)hash);
    return NULL; // key not found
  }

  hash_table_element_t *temp = table->store_house[hash];

  while(temp) {
    while(temp && temp->key_len!=key_len) {
      temp = temp->next;
    }

    if(temp) {
      if (!memcmp(temp->key, key, key_len)) {
        //LOG("Found Key at hash -> %d", (int)hash);
        return temp->value;
      } else {
        temp = temp->next;
      }
    }
  }

  //LOG("Key not found at hash %d", (int)hash);
  return NULL; // key not found
}

/**
 * Function to look if the exists in the hash table
 * @param key pointer to key to be looked for
 * @param key_len size of the key to be searched
 * @returns 0 when key is not found
 * @returns 1 when key is found
 */
int hash_table_has_key(omg_hash_table_t *table, void *key, size_t key_len) {
  size_t hash = HASH(key, key_len);

  //LOG("Searching for key with hash -> %d", (int)hash);
  if (!table->store_house[hash]) {
    //LOG("Key not found with hash -> %d, no entries", (int)hash);
    return 0; // key not found
  }

  hash_table_element_t *temp = table->store_house[hash];

  while(temp) {
    while(temp && temp->key_len!=key_len) {
      temp = temp->next;
    }

    if(temp) {
      if (!memcmp(temp->key, key, key_len)) {
        //LOG("Key Found with hash -> %d", (int)hash);
        return 1; // key found
      }

      temp=temp->next;
    }
  }

  //LOG("Key not found with hash -> %d", (int)hash);
  return 0; // key not found
}

/**
 * Function to return all the keys in a given hash table
 * @param table hash table from which key are to be reterived
 * @param keys a void** pointer where keys are filled in (memory allocated internally and must be freed)
 * @return total number of keys filled in keys
 */
size_t hash_table_get_keys(omg_hash_table_t *table, void **keys) {
  size_t i = 0;
  size_t count = 0;
  keys = calloc(table->key_count, sizeof(void *)*HASH_LEN);

  for(i=0; i<HASH_LEN; i++) {
    if (table->store_house[i]) {
      keys[count++] = table->store_house[i];
      hash_table_element_t *temp = table->store_house[i];

      //#ifdef DEBUG
      //size_t num = 1;
      //#endif
      while(temp->next) {
        keys[count++] = temp->next;
        temp = temp->next;
        //#ifdef DEBUG
        //num++;
        //#endif
      }

      //#ifdef DEBUG
      //LOG("found %d key(s) at hash -> %d", (int)num, (int)i);
      //#endif
    }
  }

  return count;
}

/**
 * Function to get all elements (key - value pairs) from the given hash table
 * @param table hash table from which elements have to be retrieved
 * @param elements a pointer to an array of hash_table_element_t pointer (malloced by function)
 * @returns 1 when no memory
 * @returns count of elements
 */
size_t hash_table_get_elements(omg_hash_table_t *table, hash_table_element_t *** elements) {
  size_t i = 0;
  size_t count = 0;
  (*elements) = (hash_table_element_t **) calloc(table->key_count, sizeof(hash_table_element_t *));

  if (!*elements) {
    //INFO("No Memory to allocate elements array");
    return 1;
  }

  for(i=0; i<HASH_LEN; i++) {
    if (table->store_house[i]) {
      (*elements)[count++] = table->store_house[i];
      hash_table_element_t *temp = table->store_house[i];

      //#ifdef DEBUG
      //size_t num = 1;
      //#endif
      while(temp->next) {
        (*elements)[count++] = temp->next;
        temp = temp->next;
        //#ifdef DEBUG
        //num++;
        //#endif
      }

      //#ifdef DEBUG
      //LOG("found %d key(s) at hash -> %d", (int)num, (int)i);
      //#endif
    }
  }

  return count;
}

/**
 * Function that returns a hash value for a given key and key_len
 * @param key pointer to the key
 * @param key_len length of the key
 * @param max_key max value of the hash to be returned by the function
 * @returns hash value belonging to [0, max_key)
 */
uint16_t hash_table_do_hash(void *key, size_t key_len, uint16_t max_key) {
  uint16_t *ptr = (uint16_t *) key;
  uint16_t hash = 0xbabe; // WHY NOT
  size_t i = 0;

  for(; i<(key_len/2); i++) {
    hash^=(i<<4 ^ *ptr<<8 ^ *ptr);
    ptr++;
  }

  hash = hash % max_key;
  return hash;
}

/**
 * Function to resize the hash table store house
 * @param table hash table to be resized
 * @param len new length of the hash table
 * @returns -1 when no elements in hash table
 * @returns -2 when no emmory for new store house
 * @returns 0 when sucess
 */
int hash_table_resize(omg_hash_table_t *table, size_t len) {
  //LOG("resizing hash table from %d to %d", table->key_num, len);
  hash_table_element_t **elements;
  size_t count;
  // FIXME traversing the elements twice, change it some time soon
  count = hash_table_get_elements(table, &elements);

  if (!count) {
    //INFO("Got No Elements from the hash table");
    return -1;
  }

  // keep the current store house in case we dont get more memory
  hash_table_element_t **temp = table->store_house;
  table->store_house = calloc(len, sizeof(hash_table_element_t *));

  if (!table->store_house) {
    table->store_house = temp;
    //INFO("No Memory for new store house");
    return -2;
  }

  table->key_num = len;
  // fool the new hash table so if refers even previously copied values
  int mode = table->mode;
  table->mode = MODE_ALLREF;

  while(count>0) {
    hash_table_element_t *elem = elements[--count];
    hash_table_add(table, elem->key, elem->key_len, elem->value, elem->value_len);
  }

  table->mode = mode;
  // free old store house
  free(temp);
  return 0;
}



