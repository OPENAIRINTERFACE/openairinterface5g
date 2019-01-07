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

/*! \file list.c
* \brief list management primimtives. It includes three implementations: (1) with mem_blocks, (2) standard list implementation (mem usage 314MB) (3) Linux Queue-based implementation (mem usage 394 MB)
* \author  Navid Nikaein, Mohamed Said MOSLI BOUKSIAA, Lionel GAUTHIER
* \date 2012 - 2014
* \version 0.5
* @ingroup util
*/

#define LIST_C
#define NULL 0

#include "list.h"
#include <string.h>

//-----------------------------------------------------------------------------
/*
 * initialize list
 */
//-----------------------------------------------------------------------------
void
list_init (list_t * listP, char *nameP)
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
list_free (list_t * listP)
{
  //-----------------------------------------------------------------------------
  mem_block_t      *le;

  while ((le = list_remove_head (listP))) {
    free_mem_block (le, __func__);
  }
}
//-----------------------------------------------------------------------------
mem_block_t *
list_get_head (list_t * listP)
{
  //-----------------------------------------------------------------------------
  return listP->head;
}
//-----------------------------------------------------------------------------
/*
 *  remove an element from head of a list
 *  @param  pointer on targeted list
 *  @return pointer on removed mem_block_t
 */
mem_block_t *
list_remove_head (list_t * listP)
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
      head->next = NULL;
    }
  } else {
    //msg("[MEM_MGT][WARNING] remove_head_from_list(%s) no elements\n",listP->name);
  }

  return head;
}

//-----------------------------------------------------------------------------
mem_block_t *
list_remove_element (mem_block_t * elementP, list_t * listP)
{
  //-----------------------------------------------------------------------------

  // access optimisation;
  mem_block_t      *head;

  if (elementP != NULL) {
    // test head
    head = listP->head;

    if (listP->head == elementP) {
      // almost one element
      if (head != NULL) {
        listP->head = head->next;
        listP->nb_elements = listP->nb_elements - 1;

        // if only one element, update tail
        if (listP->head == NULL) {
          listP->tail = NULL;
        } else {
          head->next = NULL;
        }
      }
    } else {
      while (head) {
        if (head->next == elementP) {
          head->next = elementP->next;
          listP->nb_elements = listP->nb_elements - 1;

          if (listP->tail == elementP) {
            listP->tail = head;
          }

          return elementP;
        } else {
          head = head->next;
        }
      }
    }
  }

  return elementP;
}

//-----------------------------------------------------------------------------
/*
 *  add an element to the beginning of a list
 *  @param  pointer on targeted list
 *  @return pointer on removed mem_block_t
 */
void
list_add_head (mem_block_t * elementP, list_t * listP)
{
  //-----------------------------------------------------------------------------

  // access optimisation;
  mem_block_t      *head;

  if (elementP != NULL) {
    head = listP->head;
    listP->nb_elements = listP->nb_elements + 1;

    // almost one element
    if (head == NULL) {
      listP->head = elementP;
      listP->tail = elementP;
    } else {
      elementP->next = head;
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
list_add_tail_eurecom (mem_block_t * elementP, list_t * listP)
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
      listP->head = elementP;
    } else {
      tail->next = elementP;
    }

    listP->tail = elementP;
  } else {
    //msg("[CNT_LIST][ERROR] add_cnt_tail() element NULL\n");
  }
}

//-----------------------------------------------------------------------------
void
list_add_list (list_t * sublistP, list_t * listP)
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
      }

      listP->tail = sublistP->tail;
      // clear sublist
      sublistP->head = NULL;
      sublistP->tail = NULL;
      listP->nb_elements = listP->nb_elements + sublistP->nb_elements;
      sublistP->nb_elements = 0;
    }
  }
}

//-----------------------------------------------------------------------------
void
list_display (list_t * listP)
{
  //-----------------------------------------------------------------------------

  mem_block_t      *cursor;
  //  unsigned short             nb_elements = 0;

  // test lists
  if (listP) {
    cursor = listP->head;

    if (cursor) {
      // almost one element
      //msg ("Display list %s %p", listP->name, listP);
      // while (cursor != NULL) {
      // msg ("%d:", cursor->pool_id);
      // msg ("\n");
      // cursor = cursor->next;
      // nb_elements++;
      //}
      //      msg (" found nb_elements %d nb_elements %d\n", nb_elements, listP->nb_elements);
    }
  } else {
    //msg ("[SDU_MNGT][WARNING] display_cnt_list() : list is NULL\n");
  }
}
#ifndef LINUX_LIST
/*! \fn void push_front(struct list* z, double val)
* \brief this function pushes front new values in a predefined list.
* \param z is the predefined list
*       val is the new value to be pushed inside the list
* \return
*/
void
push_front(struct list* z, double val)
{

  struct node* p = (struct node*) malloc(sizeof(struct node));

  p->next = z->head;
  p->val = val;
  z->head = p;

  z->size++;

  return;

}

/*! \fn void initialize(struct list* z)
* \brief initialize a list structure.
* \param z is the list
* \return
*/
void
initialize(struct list* z)
{
  z->head = NULL;
  z->size = 0;

  return;
}

/*! \fn void del(struct list* z)
* \brief delete a list structure.
* \param z is the list
* \return
*/
void
del(struct list* z)
{
  struct node* cur;
  struct node* x = z->head;

  while((x != NULL) && (z->size > 0)) {
    cur = x;
    x = x->next;
    free(cur);
    z->size--;
  }

  z->head = NULL;
  z->size = 0;

  return;
}
/*! \fn void totable(double* table, struct list* v)
* \brief convert a list structure to a table.
* \param table is a pointer to double table
*        v is the list to be converted
* \return
*/
void
totable(double* table, struct list* v)
{
  int i = 0;

  struct node* x = v->head;

  while(x) {
    table[i] = x->val;
    i++;
    x = x->next;
  }

  return;
}
/*! \fn int compare (const void * a, const void * b)
* \brief compare the value of two parameters
* \param a is a double, b is another double
* \return 1 if a>b, -0 if a<b
*/
int compare (const void * a, const void * b)
{
  double x = *(double*)a;
  double y = *(double*)b;

  return ( x > y );
}
#else
// push an element to the head of the list
void push_front(struct list* z, double val)
{
  struct entry* p = (struct entry*) malloc(sizeof(struct entry));
  p->val = val;

  LIST_INSERT_HEAD(&z->head, p, entries);
  z->size++;

  return;

}


// initialization
void initialize(struct list* z)
{
  LIST_INIT(&z->head);
  z->size = 0;

  return;
}

// delete
void del(struct list* z)
{
  while (z->head.lh_first != NULL) {
    free(z->head.lh_first);
    LIST_REMOVE(z->head.lh_first, entries);
  }

  z->size = 0;

  return;
}

// convert the list to a table
void totable(double* table, struct list* v)
{
  int i = 0;

  struct entry* x;

  for (x = v->head.lh_first; x != NULL; x = x->entries.le_next) {
    table[i] = x->val;
    i++;
  }

  return;
}

int compare (const void * a, const void * b)
{
  double x = *(double*)a;
  double y = *(double*)b;

  return ( x > y );
}
#endif

/*! \fn int32_t calculate_median(struct list *loc_list)
* \brief calculate the median of a list
* \param loc_list is the list
* \return the median value
*/
int32_t calculate_median(struct list *loc_list)
{
  int32_t median = 0;

  if (loc_list->size > 0) {
    double* table = (double*) malloc(loc_list->size * sizeof(double));
    totable(table, loc_list);
    /// sort the table in ascending way
    qsort (table, loc_list->size, sizeof(double), &compare);
    /// median is the value at middle the sorted table
    /// Q1 is the value at 1/4 the sorted table
    /// Q3 is the value at 3/4 the sorted table
    median = table[loc_list->size/2];
    free(table);
  }

  return median;
}
