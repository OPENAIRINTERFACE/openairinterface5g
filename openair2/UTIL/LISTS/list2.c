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

/***************************************************************************
                          list2.c  -  description
                             -------------------
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 ***************************************************************************/
#define LIST2_C
#include "list.h"
#include "assertions.h"

#include <string.h>


//-----------------------------------------------------------------------------
/*
 * initialize list
 */
//-----------------------------------------------------------------------------
void
list2_init (list2_t * listP, char *nameP)
{
  //-----------------------------------------------------------------------------
  if (nameP) {
    strncpy( listP->name, nameP, LIST_NAME_MAX_CHAR );
    listP->name[LIST_NAME_MAX_CHAR-1] = 0; // terminate string
  }

  listP->tail = NULL;
  listP->head = NULL;
  listP->nb_elements = 0;
}

//-----------------------------------------------------------------------------
void
list2_free (list2_t * listP)
{
  //-----------------------------------------------------------------------------

  mem_block_t      *le;

  while ((le = list2_remove_head (listP))) {
    free_mem_block (le, __func__);
  }
}

//-----------------------------------------------------------------------------
/*
 *  remove an element from list
 *  @param  pointer on targeted list
 *  @param  mem_block_t to remove
 *  @return pointer on removed mem_block_t
 */
mem_block_t *
list2_remove_element (mem_block_t * elementP, list2_t * listP)
{
  //-----------------------------------------------------------------------------

  if (elementP != NULL) {
    // head of list
    if (elementP == listP->head) {
      listP->head = elementP->next;

      if (listP->head == NULL) {
        listP->tail = NULL;
      } else {
        elementP->next->previous = NULL;
        elementP->next = NULL;
      }

      // tail of the list
      // note : case of 1 remaining element in the list has been treated above
    } else if (elementP == listP->tail) {
      // so several elements in the list
      listP->tail = elementP->previous;
      listP->tail->next = NULL;
      elementP->previous = NULL;
      // in the middle of the list, after a head element and before the tail element
    } else {
      // link element n-1 with element n+1
      elementP->previous->next = elementP->next;
      elementP->next->previous = elementP->previous;
      elementP->next = NULL;
      elementP->previous = NULL;
    }

    listP->nb_elements = listP->nb_elements - 1;
  }

  return elementP;
}
//-----------------------------------------------------------------------------
mem_block_t *
list2_get_head (list2_t * listP)
{
  //-----------------------------------------------------------------------------
  return listP->head;
}

//-----------------------------------------------------------------------------
mem_block_t *
list2_get_tail (list2_t * listP)
{
  //-----------------------------------------------------------------------------
  return listP->tail;
}

//-----------------------------------------------------------------------------
/*
 *  remove an element from head of a list
 *  @param  pointer on targeted list
 *  @return pointer on removed mem_block_t
 */
mem_block_t *
list2_remove_head (list2_t * listP)
{
  //-----------------------------------------------------------------------------

  // access optimisation
  mem_block_t      *head;

  head = listP->head;

  // almost one element
  if (head != NULL) {
    listP->head = head->next;
    listP->nb_elements = listP->nb_elements - 1;

    // if only one element, update tail
    if (listP->head == NULL) {
      listP->tail = NULL;
    } else {
      listP->head->previous = NULL;
      head->next = NULL;
    }
  } else {
    //msg("[MEM_MGT][WARNING] remove_head_from_list(%s) no elements\n",listP->name);
  }

  return head;
}

//-----------------------------------------------------------------------------
/*
 *  remove an element from tail of a list
 *  @param  pointer on targeted list
 *  @return pointer on removed mem_block_t
 */
mem_block_t *
list2_remove_tail (list2_t * listP)
{
  //-----------------------------------------------------------------------------

  // access optimisation;
  mem_block_t      *tail;


  tail = listP->tail;

  // almost one element;
  if (tail != NULL) {
    listP->nb_elements = listP->nb_elements - 1;

    // if only one element, update head, tail;
    if (listP->head == tail) {
      listP->head = NULL;
      listP->tail = NULL;
    } else {
      listP->tail = tail->previous;
      tail->previous->next = NULL;
    }

    tail->previous = NULL;
  } else {
    //msg("[MEM_MGT][WARNING] remove_head_from_list(%s) no elements\n",listP->name);
  }

  return tail;
}

//-----------------------------------------------------------------------------
/*
 *  add an element to the beginning of a list
 *  @param  pointer on targeted list
 *  @return pointer on removed mem_block_t
 */
void
list2_add_head (mem_block_t * elementP, list2_t * listP)
{
  //-----------------------------------------------------------------------------

  // access optimisation;
  mem_block_t      *head;

  if (elementP != NULL) {
    head = listP->head;
    listP->nb_elements = listP->nb_elements + 1;

    // almost one element
    if (head == NULL) {
      elementP->previous = NULL;
      elementP->next = NULL;
      listP->head = elementP;
      listP->tail = elementP;
    } else {
      elementP->next = head;
      head->previous = elementP;
      elementP->previous = NULL;
      listP->head = elementP;
    }
  }
}

//-----------------------------------------------------------------------------
/*
 *  add an element to the end of a list
 *  @param  pointer on targeted list
 *  @return pointer on removed mem_block_t
 */
void
list2_add_tail (mem_block_t * elementP, list2_t * listP)
{
  mem_block_t      *tail;
  //-----------------------------------------------------------------------------

  if (elementP != NULL) {
    // access optimisation
    listP->nb_elements = listP->nb_elements + 1;
    elementP->next = NULL;
    tail = listP->tail;

    // almost one element
    if (tail == NULL) {
      elementP->previous = NULL;
      listP->head = elementP;
    } else {
      tail->next = elementP;
      elementP->previous = tail;
    }

    listP->tail = elementP;
  }
}

//-----------------------------------------------------------------------------
void
list2_add_list (list2_t * sublistP, list2_t * listP)
{
  //-----------------------------------------------------------------------------

  if (sublistP) {
    if (sublistP->head) {
      // access optimisation
      mem_block_t      *tail;

      tail = listP->tail;

      // almost one element
      if (tail == NULL) {
        listP->head = sublistP->head;
      } else {
        tail->next = sublistP->head;
        sublistP->head->previous = tail;
      }

      listP->tail = sublistP->tail;
      // clear sublist
      sublistP->head = NULL;
      sublistP->tail = NULL;
    }

    listP->nb_elements = listP->nb_elements + sublistP->nb_elements;
  }
}

//-----------------------------------------------------------------------------
void
list2_display (list2_t * listP)
{
  //-----------------------------------------------------------------------------
  /*
    mem_block_t      *cursor;
    unsigned short             nb_elements = 0;
    //uint32_t nb_bytes;
    // uint32_t index;

    // test lists
    if (listP) {
      cursor = listP->head;
      if (cursor) {
        // almost one element
        msg ("Display list %s %p", listP->name, listP);
        while (cursor != NULL) {
          msg ("%d:", cursor->pool_id);
          //nb_bytes = (( sdu_management*)(cursor->misc))->size;
          //   for (index=0; index < nb_bytes; index++) {
          //   msg("%02X.",cursor->data[index]);
          //   }
          msg ("\n");
          cursor = cursor->next;
          nb_elements++;
        }
        msg (" found nb_elements %d nb_elements %d\n", nb_elements, listP->nb_elements);
        AssertFatal(nb_elements == listP->nb_elements, "Bad count of elements %d != %d", nb_elements, listP->nb_elements);
      }
    }*/
}
