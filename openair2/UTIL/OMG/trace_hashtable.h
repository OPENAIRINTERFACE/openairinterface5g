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

/*! \file trace_hashtable.h
* \brief A 'C' implementation of a hashtable
* \author  S. Uppoor
* \date 2011
* \version 0.1
* \company INRIA
* \email: sandesh.uppor@inria.fr
* \note
* \warning
*/

#ifndef _TRACE_HASHTABLE_H
#define _TRACE_HASHTABLE_H
#define LEN 1
#define RATIO 4
#include<sys/types.h>
#include<stdint.h>
#define DATA_ONLY 0
#define DATA_AND_STATUS_CHANGE 1

/**
 * @struct Simple struct to hold only few information
 * @brief The entry in each node is a line from the external mobility file
 */




typedef struct parsed_data {
  int vid;
  int gid;
  int type;
  double time;
  double x_pos;
  double y_pos;
  double speed;
  int visit;
  struct parsed_data* next;
} node_data;



/**
 * @struct hash_table_element "hashtable.h"
 * @brief stores an hash table element for use in the hash table
 */


typedef struct container {
  int flag;
  int gid;
  node_data* next;
  node_data* end;
  struct container* next_c;
} node_container;


/**
 * @struct hash_table "hashtable.h"
 * @brief identifies the hashtable for which operations are to be performed
 */

typedef struct hash_table {
  int key_len;
  int key_count;
  int ratio;
  node_container **data_store;
} hash_table_t;



/**
 * @struct struct acts as gateway to hashtable
 * @brief holds vehicle id and initial pointer to the head of the linked list in hastable
 */


typedef struct nodeinfo {
  int vid;
  int g_id;
  struct nodeinfo* next;
} node_info;




/**
 * Function that returns a hash value for a given key and key_len
 * @param key pointer to the key
 * @param key_len length of the key
 *f @param max_key max value of the hash to be returned by the function
 * @returns hash value belonging to [0, max_key)
 */
uint16_t hash (int *key, int len);

// hash table operations
/**
 * Fuction to create a new hash table
 * @param mode hash_table_mode which the hash table should follow
 * @returns hash_table_t object which references the hash table
 * @returns NULL when no memory
 */
void create_new_table (int node_type);

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
void hash_table_add (hash_table_t* t_table, node_data* node, node_container* value);


/**
 * Function to lookup a key in a particular table
 * @note use this macro when size of key and/or value can be given by sizeof
 * @param table table to look key in
 * @param key pointer to key to be looked for
 * @param key_len size of the key to be searched
 * @returns NULL when key is not found in the hash table
 * @returns void* pointer to the value in the table
 */
node_container* hash_table_lookup (hash_table_t* table, int id);



/**
 * Function to resize the hash table store house
 * @param table hash table to be resized
 * @param len new length of the hash table
 * @returns -1 when no elements in hash table
 * @returns -2 when no emmory for new store house
 * @returns 0 when sucess
 */
int hash_table_resize (void);
#endif
