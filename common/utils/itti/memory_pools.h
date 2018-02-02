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

#ifndef MEMORY_POOLS_H_
#define MEMORY_POOLS_H_

#include <stdint.h>

typedef void * memory_pools_handle_t;
typedef void * memory_pool_item_handle_t;

memory_pools_handle_t memory_pools_create (uint32_t pools_number);

char *memory_pools_statistics(memory_pools_handle_t memory_pools_handle);

int memory_pools_add_pool (memory_pools_handle_t memory_pools_handle, uint32_t pool_items_number, uint32_t pool_item_size);

memory_pool_item_handle_t memory_pools_allocate (memory_pools_handle_t memory_pools_handle, uint32_t item_size, uint16_t info_0, uint16_t info_1);

int memory_pools_free (memory_pools_handle_t memory_pools_handle, memory_pool_item_handle_t memory_pool_item_handle, uint16_t info_0);

void memory_pools_set_info (memory_pools_handle_t memory_pools_handle, memory_pool_item_handle_t memory_pool_item_handle, int index, uint16_t info);

#endif /* MEMORY_POOLS_H_ */
