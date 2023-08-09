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

/***************************************************************************
                          mem_block.h  -  description
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr


 ***************************************************************************/
#ifndef __MEM_BLOCK_H__
#    define __MEM_BLOCK_H__

#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "common/platform_constants.h"
//-----------------------------------------------------------------------------

typedef struct mem_block_t {
  struct mem_block_t *next;
  struct mem_block_t *previous;
  size_t size;
  unsigned char pool_id;
  unsigned char *data;
} mem_block_t;

//-----------------------------------------------------------------------------

void        *pool_buffer_init (void);
void        *pool_buffer_clean (void *arg);
void         free_mem_block (mem_block_t * leP, const char* caller);
mem_block_t* get_free_mem_block (uint32_t sizeP, const char* caller);
mem_block_t *get_free_copy_mem_block (void);
mem_block_t *get_free_copy_mem_block_up (void);
mem_block_t *copy_mem_block (mem_block_t * leP, mem_block_t * destP);
void         display_mem_load (void);
#define LIST_NAME_MAX_CHAR 32

typedef struct {
  struct mem_block_t *head;
  struct mem_block_t *tail;
  int                nb_elements;
  char               name[LIST_NAME_MAX_CHAR];
} list2_t;
//-----------------------------------------------------------------------------
typedef struct {
  struct mem_block_t *head;
  struct mem_block_t *tail;
  int                nb_elements;
  char               name[LIST_NAME_MAX_CHAR];
} list_t;

#ifdef __cplusplus
}
#endif
#endif
