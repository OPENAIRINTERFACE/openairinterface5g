/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/
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
