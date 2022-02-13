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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include<linux/types.h>
#include<stdlib.h>
#include<sys/queue.h>
#include <string.h>


#include "UTIL/MEM/mem_block.h"
#include <common/utils/assertions.h>


//-----------------------------------------------------------------------------
void         list_init (list_t* , char *);
void         list_free (list_t* listP);
mem_block_t* list_get_head (list_t*);
mem_block_t* list_remove_head (list_t* );
mem_block_t* list_remove_element (mem_block_t*, list_t*);
void         list_add_head (mem_block_t* , list_t* );
void         list_add_tail_eurecom (mem_block_t* , list_t* );
void         list_add_list (list_t* , list_t* );
void         list_display (list_t* );
//-----------------------------------------------------------------------------
void         list2_init           (list2_t*, char*);
void         list2_free           (list2_t* );
mem_block_t* list2_get_head       (list2_t*);
mem_block_t* list2_get_tail       (list2_t*);
mem_block_t* list2_remove_element (mem_block_t* , list2_t* );
mem_block_t* list2_remove_head    (list2_t* );
mem_block_t* list2_remove_tail    (list2_t* );
void         list2_add_head       (mem_block_t* , list2_t* );
void         list2_add_tail       (mem_block_t* , list2_t* );
void         list2_add_list       (list2_t* , list2_t* );
void         list2_display        (list2_t* );
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
void   push_front  (struct list*, double); 
void   initialize  (struct list*);         
void   del         (struct list*);         
void   totable     (double*, struct list*);
int compare (const void * a, const void * b);
int32_t calculate_median(struct list *loc_list);


typedef struct {
  size_t size;
  size_t mallocedSize;
  size_t atomSize;
  size_t increment;
} varArray_t;

static inline varArray_t * initVarArray(size_t increment, size_t atomSize) {
    varArray_t * tmp=(varArray_t *)malloc(sizeof(varArray_t)+increment*atomSize);
    tmp->size=0;
    tmp->atomSize=atomSize;
    tmp->mallocedSize=increment;
    tmp->increment=increment;
    return(tmp);
}

static inline void * dataArray(varArray_t * input) {
  return input+1;
}

static inline void appendVarArray(varArray_t ** inputPtr, void* data) {
  varArray_t *input=*inputPtr;
  if (input->size>=input->mallocedSize) {
     input->mallocedSize+=input->increment;
     *inputPtr=(varArray_t *)realloc(input,sizeof(varArray_t)+input->mallocedSize*input->atomSize);
     AssertFatal(*inputPtr, "no memory left");
     input=*inputPtr;
  }
  memcpy((uint8_t*)(input+1)+input->atomSize*input->size++, data, input->atomSize);
}

static inline void freeVarArray(varArray_t * input) {
   free(input);
}

#endif
