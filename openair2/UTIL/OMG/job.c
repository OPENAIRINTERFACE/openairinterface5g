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

/*! \file job.c
* \brief handle jobs for future nodes' update
* \author  S. Gashaw, M. Mahersi,  J. Harri, N. Nikaein,
* \date 2014
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "omg.h"

job_list *
add_job (pair_struct * job, job_list * job_vector)
{

  job_list *tmp;
  job_list *entry = (job_list *) malloc (sizeof (struct job_list_struct));
  entry->pair = job;
  entry->next = NULL;

  if (job_vector != NULL) {

    tmp = job_vector;
    tmp->next = entry;
  }

  return entry;

}

//add_job2

job_list *
addjob (pair_struct * job, job_list * job_vector)
{

  job_list *tmp;
  job_list *entry = (job_list *) malloc (sizeof (struct job_list_struct));
  entry->pair = job;
  entry->next = NULL;

  if (job_vector == NULL) {
    return entry;
  } else {
    tmp = job_vector;

    while (tmp->next != NULL) {
      tmp = tmp->next;
    }

    tmp->next = entry;
  }

  return job_vector;


}

// display list of jobs
void
display_job_list (double curr_t, job_list * job_vector)
{

  job_list *tmp = job_vector;

  if (tmp == NULL) {
    LOG_D (OMG, "Empty Job_list\n");
  } else {
    //LOG_D(OMG, "first_Job_time in Job_Vector %f\n", Job_Vector->pair->a);
    while (tmp != NULL) {
      if ((tmp->pair != NULL) /*&& tmp->pair->b->id==0 */ )
        //LOG_D(OMG, "%.2f %.2f \n",tmp->pair->b->x_pos, tmp->pair->b->y_pos);
        LOG_D (OMG, "%.2f  %d %d %.2f %.2f  %.2f\n", curr_t, tmp->pair->b->id,tmp->pair->b->gid,
               tmp->pair->b->x_pos, tmp->pair->b->y_pos,
               tmp->pair->b->mob->speed);


      tmp = tmp->next;

    }
  }
}

//average nodes speed for each mobility type
unsigned int
nodes_avgspeed (job_list * job_vector)
{
  job_list *tmp = job_vector;
  unsigned int avg = 0, cnt = 0;

  if (tmp == NULL) {
    LOG_D (OMG, "Empty Job_list\n");
    return 0;
  } else {
    while (tmp != NULL) {
      if ((tmp->pair != NULL)) {
        avg += tmp->pair->b->mob->speed;
        cnt++;
      }

      tmp = tmp->next;
    }
  }

  return avg / cnt;
}

// quick sort of the linked list
job_list *
job_list_sort (job_list * list, job_list * end)
{

  job_list *pivot, *tmp, *next, *before, *after;

  if (list != end && list->next != end) {

    pivot = list;
    before = pivot;
    after = end;

    for (tmp = list->next; tmp != end; tmp = next) {
      next = tmp->next;

      if (tmp->pair->next_event_t > pivot->pair->next_event_t)
        tmp->next = after, after = tmp;
      else
        tmp->next = before, before = tmp;
    }

    before = job_list_sort (before, pivot);
    after = job_list_sort (after, end);

    pivot->next = after;
    return before;
  }

  return list;
}

job_list *
quick_sort (job_list * list)
{
  return job_list_sort (list, NULL);
}


job_list *
remove_job (job_list * list, int nid, int node_type)
{

  job_list *current, *previous;

  current = list;
  previous = NULL;

  while (current != NULL) {
    if (current->pair->b->id == nid && current->pair->b->type == node_type) {
      if (current == list || previous == NULL)
        list = current->next;
      else
        previous->next = current->next;

      free (current);
      break;

    }

    previous = current;
    current = current->next;
  }

  return list;

}
