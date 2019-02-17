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
//#define LIST_C
//#define NULL 0

#include <stdio.h>
#include <stdlib.h>
#include "pad_list.h"
//#include <mpi.h>


void job_list_init (Job_List_t *listP) {
  listP->tail = NULL;
  listP->head = NULL;
  listP->nb_elements = 0;
}

void event_list_init (Event_List_t *listP) {
  listP->tail = NULL;
  listP->head = NULL;
  listP->nb_elements = 0;
}

void pkt_list_init (Packet_OTG_List_t *listP) {
  listP->tail = NULL;
  listP->head = NULL;
  listP->nb_elements = 0;
}

//-----------------------------------------------------------------------------

void job_list_free (Job_List_t *listP) {
  Job_elt_t *le;

  while ((le = job_list_remove_head (listP))) {
    free(le);
  }
}

void event_list_free (Event_List_t *listP) {
  Event_elt_t *le;

  while ((le = event_list_remove_head (listP))) {
    free(le);
  }
}

void pkt_list_free (Packet_OTG_List_t *listP) {
  Packet_otg_elt_t *le;

  while ((le = pkt_list_remove_head (listP))) {
    free(le);
  }
}

//-----------------------------------------------------------------------------

Job_elt_t *job_list_get_head (Job_List_t *listP) {
  return listP->head;
}

Event_elt_t *event_list_get_head (Event_List_t *listP) {
  return listP->head;
}

Packet_otg_elt_t *pkt_list_get_head (Packet_OTG_List_t *listP) {
  return listP->head;
}

//-----------------------------------------------------------------------------

/*
 *  remove an element from head of a list
 *  @param  pointer on targeted list
 *  @return pointer on removed Job_elt_t
 */
Job_elt_t *job_list_remove_head (Job_List_t *listP) {
  // access optimisation
  Job_elt_t *head;
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

Event_elt_t *event_list_remove_head (Event_List_t *listP) {
  // access optimisation
  Event_elt_t *head;
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

Packet_otg_elt_t *pkt_list_remove_head (Packet_OTG_List_t *listP) {
  // access optimisation
  Packet_otg_elt_t *head;
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

Job_elt_t *job_list_remove_element (Job_elt_t *elementP, Job_List_t *listP) {
  // access optimisation;
  Job_elt_t *head;

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

Event_elt_t *event_list_remove_element (Event_elt_t *elementP, Event_List_t *listP) {
  // access optimisation;
  Event_elt_t *head;

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

Packet_otg_elt_t *pkt_list_remove_element (Packet_otg_elt_t *elementP, Packet_OTG_List_t *listP) {
  // access optimisation;
  Packet_otg_elt_t *head;

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
 *  @return pointer on removed Job_elt_t
 */

void job_list_add_head (Job_elt_t *elementP, Job_List_t *listP) {
  // access optimisation;
  Job_elt_t *head;

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

void event_list_add_head (Event_elt_t *elementP, Event_List_t *listP) {
  // access optimisation;
  Event_elt_t *head;

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

void pkt_list_add_head (Packet_otg_elt_t *elementP, Packet_OTG_List_t *listP) {
  // access optimisation;
  Packet_otg_elt_t *head;

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

void event_list_add_element (Event_elt_t *elementP, Event_elt_t *previous, Event_List_t *listP) {
  // access optimisation;
  Event_elt_t *next;

  if (elementP != NULL && previous != NULL) {
    elementP->next = NULL;
    next = previous->next;
    listP->nb_elements = listP->nb_elements + 1;

    // almost one element
    if (next == NULL) {
      previous->next = elementP;
      listP->tail = elementP;
    } else {
      elementP->next = previous->next;
      previous->next = elementP;
    }
  }
}


//-----------------------------------------------------------------------------
/*
 *  add an element to the end of a list
 *  @param  pointer on targeted list
 *  @return pointer on removed Job_elt_t
 */
void job_list_add_tail_eurecom (Job_elt_t *elementP, Job_List_t *listP) {
  Job_elt_t *tail;

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

void event_list_add_tail_eurecom (Event_elt_t *elementP, Event_List_t *listP) {
  Event_elt_t *tail;

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

void pkt_list_add_tail_eurecom (Packet_otg_elt_t *elementP, Packet_OTG_List_t *listP) {
  Packet_otg_elt_t *tail;

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
void job_list_add_list (Job_List_t *sublistP, Job_List_t *listP) {
  if (sublistP) {
    if (sublistP->head) {
      // access optimisation
      Job_elt_t *tail;
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

void event_list_add_list (Event_List_t *sublistP, Event_List_t *listP) {
  if (sublistP) {
    if (sublistP->head) {
      // access optimisation
      Event_elt_t *tail;
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

void pkt_list_add_list (Packet_OTG_List_t *sublistP, Packet_OTG_List_t *listP) {
  if (sublistP) {
    if (sublistP->head) {
      // access optimisation
      Packet_otg_elt_t *tail;
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
void job_list_display (Job_List_t *listP) {
  //Correct the output once the content of struct Job is fixed
  /*Job_elt_t *cursor;
  unsigned short nb_elements = 0;

  if (listP) {
    cursor = listP->head;
    if (cursor) {
      while (cursor != NULL) {
          if (cursor->job.type == eNB_DL)
              printf ("DL eNB From (%d,%d)\n", (cursor->job).nid, (cursor->job).mid);
          else if (cursor->job.type == UE_DL)
              printf ("DL UE From (%d,%d)\n", (cursor->job).nid, (cursor->job).mid);
          else if (cursor->job.type == UE_UL)
              printf ("UL UE From (%d,%d)\n", (cursor->job).nid, (cursor->job).mid);
          else if (cursor->job.type == eNB_UL)
              printf ("UL eNB From (%d,%d)\n", (cursor->job).nid, (cursor->job).mid);
        //msg ("From (%d,%d) To (%d,%d)\n", (cursor->job).node1, (cursor->job).master1, (cursor->job).node2, (cursor->job).master2);
        cursor = cursor->next;
        nb_elements++;
      }
      printf ("Found nb_elements %d nb_elements %d\n", nb_elements, listP->nb_elements);
      //msg ("Found nb_elements %d nb_elements %d\n", nb_elements, listP->nb_elements);
    }
  } else {
    printf ("[SDU_MNGT][WARNING] display_cnt_dbl_lk_list_up() : list is NULL\n");
    //msg ("[SDU_MNGT][WARNING] display_cnt_dbl_lk_list_up() : list is NULL\n");
  }*/
}

void event_list_display (Event_List_t *listP) {
  Event_elt_t *cursor;
  unsigned short nb_elements = 0;

  if (listP) {
    cursor = listP->head;

    if (cursor) {
      while (cursor != NULL) {
        printf ("Ev (%d,%d)\n", (cursor->event).type, (cursor->event).frame);
        //msg ("From (%d,%d) To (%d,%d)\n", (cursor->job).node1, (cursor->job).master1, (cursor->job).node2, (cursor->job).master2);
        cursor = cursor->next;
        nb_elements++;
      }

      printf ("Found nb_elements %d nb_elements %d\n", nb_elements, listP->nb_elements);
      //msg ("Found nb_elements %d nb_elements %d\n", nb_elements, listP->nb_elements);
    }
  } else {
    printf ("[SDU_MNGT][WARNING] display_cnt_dbl_lk_list_up() : list is NULL\n");
    //msg ("[SDU_MNGT][WARNING] display_cnt_dbl_lk_list_up() : list is NULL\n");
  }
}

void pkt_list_display (Packet_OTG_List_t *listP) {
  Packet_otg_elt_t *cursor;
  unsigned short nb_elements = 0;

  if (listP) {
    cursor = listP->head;

    if (cursor) {
      while (cursor != NULL) {
        printf ("Pkt (DST %d, RB %d)\n", (cursor->otg_pkt).dst_id, (cursor->otg_pkt).rb_id);
        //msg ("From (%d,%d) To (%d,%d)\n", (cursor->job).node1, (cursor->job).master1, (cursor->job).node2, (cursor->job).master2);
        cursor = cursor->next;
        nb_elements++;
      }

      printf ("Found nb_elements %d nb_elements %d\n", nb_elements, listP->nb_elements);
      //msg ("Found nb_elements %d nb_elements %d\n", nb_elements, listP->nb_elements);
    }
  } else {
    printf ("[SDU_MNGT][WARNING] display_cnt_dbl_lk_list_up() : list is NULL\n");
    //msg ("[SDU_MNGT][WARNING] display_cnt_dbl_lk_list_up() : list is NULL\n");
  }
}
