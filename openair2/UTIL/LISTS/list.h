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


/*! \file pad_list.c
* \brief list management primimtives
* \author Mohamed Said MOSLI BOUKSIAA, Lionel GAUTHIER, Navid Nikaein
* \date 2012 - 2014
* \version 0.5
* @ingroup util
*/

/*
                                 list.h
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr

 ***************************************************************************/
#ifndef __LIST_H__
#    define __LIST_H__

#    include "UTIL/MEM/mem_block.h"
//-----------------------------------------------------------------------------
#    ifdef LIST_C
#        define private_list(x) x
#        define public_list(x) x
#    else
#        define private_list(x)
#        define public_list(x) extern x
#    endif
#    ifdef LIST2_C
#        define private_list2(x) x
#        define public_list2(x) x
#    else
#        define private_list2(x)
#        define public_list2(x) extern x
#    endif
//-----------------------------------------------------------------------------
#include "platform_constants.h"
#include<linux/types.h>
#include<stdlib.h>
#include<sys/queue.h>

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
//-----------------------------------------------------------------------------
public_list( void         list_init (list_t* , char *);)
public_list( void         list_free (list_t* listP);)
public_list( mem_block_t* list_get_head (list_t*);)
public_list( mem_block_t* list_remove_head (list_t* );)
public_list( mem_block_t* list_remove_element (mem_block_t*, list_t*);)
public_list( void         list_add_head (mem_block_t* , list_t* );)
public_list( void         list_add_tail_eurecom (mem_block_t* , list_t* );)
public_list( void         list_add_list (list_t* , list_t* );)
public_list( void         list_display (list_t* );)
//-----------------------------------------------------------------------------
public_list2( void         list2_init           (list2_t*, char*);)
public_list2( void         list2_free           (list2_t* );)
public_list2( mem_block_t* list2_get_head       (list2_t*);)
public_list2( mem_block_t* list2_get_tail       (list2_t*);)
public_list2( mem_block_t* list2_remove_element (mem_block_t* , list2_t* );)
public_list2( mem_block_t* list2_remove_head    (list2_t* );)
public_list2( mem_block_t* list2_remove_tail    (list2_t* );)
public_list2( void         list2_add_head       (mem_block_t* , list2_t* );)
public_list2( void         list2_add_tail       (mem_block_t* , list2_t* );)
public_list2( void         list2_add_list       (list2_t* , list2_t* );)
public_list2( void         list2_display        (list2_t* );)
//-----------------------------------------------------------------------------
/* The following lists are used for sorting numbers */
#ifndef LINUX_LIST
/*! \brief the node structure */
struct node {
  struct node* next; /*!< \brief is a node pointer */
  double val; /*!< \brief is a the value of a node pointer*/
};
//-----------------------------------------------------------------------------
/*! \brief the list structure */
struct list {
  struct node* head; /*!< \brief is a node pointer */
  ssize_t size; /*!< \brief is the list size*/
};
#else
//-----------------------------------------------------------------------------
struct entry {
  double val;
  LIST_ENTRY(entry) entries;
};
//-----------------------------------------------------------------------------
struct list {
  LIST_HEAD(listhead, entry) head;
  ssize_t size;
};
#endif
//-----------------------------------------------------------------------------
public_list2( void   push_front  (struct list*, double); )
public_list2( void   initialize  (struct list*);         )
public_list2( void   del         (struct list*);         )
public_list2( void   totable     (double*, struct list*);)
public_list2( int compare (const void * a, const void * b);)
public_list2( int32_t calculate_median(struct list *loc_list);)

#endif
