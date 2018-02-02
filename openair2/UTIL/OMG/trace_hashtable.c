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

/*! \file hashtable.c
* \brief A 'C' implementation of a hashtable
* \author S. Gashaw,  N. Nikaein, J. Harri
* \date 2014
* \version 0.1
* \company EURECOM
* \email:
* \note
* \warning
*/

#include <stdio.h>
#include "trace_hashtable.h"
#include <stdlib.h>
#include <string.h>
#include "omg.h"

/*Global variables*/
node_info **list_head;
hash_table_t **table;

// hash table operations
/**
 * Fuction to create a new hash table
 * @param mode hash_table_mode which the hash table should follow
 * @returns hash_table_t object which references the hash table
 * @returns NULL when no memory
 */
void
create_new_table (int node_type)
{
  if(table==NULL)
    table = (hash_table_t **) calloc (MAX_NUM_NODE_TYPES, sizeof (hash_table_t*));

  table[node_type]=(hash_table_t *) calloc (MAX_NUM_NODE_TYPES, sizeof (hash_table_t));

  if (table == NULL || table[node_type]==NULL) {
    LOG_E (OMG, "--------table creation failed--------\n");
    exit (-1);
  }

  table[node_type]->key_len = LEN;
  table[node_type]->key_count = 0;
  table[node_type]->ratio = RATIO;

  table[node_type]->data_store =
    (node_container **) calloc (LEN, sizeof (node_container *));

  if (table[node_type]->data_store == NULL) {
    free (table[node_type]);
    LOG_E (OMG, "table creation failed \n");
    exit (-1);
  }

  //for (i = 0; i < LEN; i++)
  // table->data_store[i] = NULL;
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
void
hash_table_add (hash_table_t * t_table, node_data * node,
                node_container * value)
{
  hash_table_t *my_table = t_table;
  node_data *new_node = node;
  int key = new_node->gid;
  int hash_val = hash (&key, my_table->key_len);

  //resize check

  if (value == NULL) {
    //create new container for this node

    my_table->key_count++;

    node_container *new_c =
      (node_container *) calloc (1, sizeof (node_container));

    if (!new_c) {
      LOG_E (OMG, "node block creation failed\n");
      exit (-1);
    }

    new_c->gid = new_node->gid;
    new_c->next = new_node;
    new_c->end = new_c->next;
    new_c->next_c = NULL;

    if (my_table->data_store[hash_val] == NULL)
      my_table->data_store[hash_val] = new_c;
    else {
      node_container *temp1, *save;
      temp1 = my_table->data_store[hash_val];

      while (temp1 != NULL) {
        save = temp1;
        temp1 = temp1->next_c;
      }

      save->next_c = new_c;
    }

  } else {
    node_container *my_c = value;

    //put node data according to time
    if (my_c->gid == new_node->gid) {
      if (my_c->next->time > new_node->time) {
        new_node->next = my_c->next;
        my_c->next = new_node;
        return;
      }

      if (my_c->end->time <= new_node->time) {
        my_c->end->next = new_node;
        my_c->end = new_node;
        return;
      }

      node_data *temp = my_c->next;
      node_data *ptr = temp->next;

      while (ptr != NULL && ptr->time <= new_node->time) {
        temp = ptr;
        ptr = ptr->next;
      }

      temp->next = new_node;
      new_node->next = ptr;
    }
  }


}

/**
 * Function to lookup a key in a particular table
 * @param table table to look key in
 * @param key pointer to key to be looked for
 * @param key_len size of the key to be searched
 * @returns NULL when key is not found in the hash table
 * @returns void* pointer to the value in the table
 */
node_container *
hash_table_lookup (hash_table_t * table, int id)
{
  hash_table_t *my_table = table;
  int key = id;
  int hash_value_of_id = hash (&key, my_table->key_len);

  if (my_table->data_store[hash_value_of_id] == NULL)
    return NULL;

  node_container *temp = my_table->data_store[hash_value_of_id];

  while (temp != NULL) {
    if (temp->gid == id)
      break;

    temp = temp->next_c;
  }

  return temp;
}





/**
 * Function that returns a hash value for a given key and key_len
 * @param key pointer to the key
 * @param key_len length of the key
 * @param max_key max value of the hash to be returned by the function
 * @returns hash value belonging to [0, max_key)
 */
uint16_t
hash (int *key, int len)
{
  uint16_t *ptr = (uint16_t *) key;
  uint16_t hash = 0xbabe; // WHY NOT
  size_t i = 0;

  for (; i < (sizeof (key) / 2); i++) {
    hash ^= (i << 4 ^ *ptr << 8 ^ *ptr);
    ptr++;
  }

  hash = hash % len;
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
int
hash_table_resize ()
{
  return 0; // FIXME
}
