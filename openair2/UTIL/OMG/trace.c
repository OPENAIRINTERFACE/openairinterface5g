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

/*! \file trace.c
* \brief The trace-based mobility model for OMG/OAI (mobility is statically imported from a file)
* \author  S. Gashaw, N. Nikaein, J. Harri
* \date 2014
* \version 0.1
* \company EURECOM
* \email:
* \note
* \warning
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "trace.h"
#define FIXED 1
#define MOVING 0

extern hash_table_t **table;
extern node_info **list_head;
//check if the next input data is the same as the previous



int
start_trace_generator (omg_global_param omg_param_list)
{
  double cur_time = 0.0;
  //int immobile = 0;
  int node_number;
  node_struct *node = NULL;
  mobility_struct *mobility = NULL;
  pair_struct *pair = NULL;
  node_info *temp;

  parse_data (omg_param_list.mobility_file, omg_param_list.nodes_type); //read mobility file

  //LOG_I (OMG, "mobility file %s put in the table %p\n",omg_param_list.mobility_file, table);

  node_number = get_number_of_nodes (omg_param_list.nodes_type);

  if (node_number == 0)
    LOG_E (OMG, "[TRACE] empty input file \n");


  LOG_I (OMG, "TRACE mobility model for %d %d nodes\n", node_number,
         omg_param_list.nodes_type);
  temp = list_head[omg_param_list.nodes_type];

  while (temp != NULL) {
    node_data *this_node =
      get_next_data (table[omg_param_list.nodes_type], temp->g_id, DATA_AND_STATUS_CHANGE);

    if (this_node == NULL) {
      LOG_E (OMG, "[TRACE] Unexpected empty data  entry\n");
      exit (-1);
    }


    node = create_node ();
    mobility = create_mobility ();
    node->id = temp->vid;
    node->gid = temp->g_id;

    /* if(this_node->type == -1)  //if node type is given in the file set the defualt type UE
       node->type = UE;  */
    node->type = omg_param_list.nodes_type;
    node->generator = TRACE;
    node->mob = mobility;

    place_trace_node (node, this_node); //initial positions



    if (this_node->visit == 1) {

      pair = (pair_struct *) malloc (sizeof (struct pair_struct));
      pair->b = node;

      if (this_node->time > cur_time) {
        schedule_trace_node (pair, this_node, cur_time);
      } else {
        if (this_node->x_pos == this_node->next->x_pos &&
            this_node->y_pos == this_node->next->y_pos)

          sleep_trace_node (pair, this_node, cur_time);

        else

          move_trace_node (pair, this_node, cur_time);  //move

      }

      //LOG_I (OMG,
      //"[TRACE] position of node ID: %d %d\n ",node->id, node->type);
    } else {
      temp = temp->next;
      continue;
    }

    job_vector_end[TRACE] = add_job (pair, job_vector_end[TRACE]);

    if (job_vector[TRACE] == NULL)
      job_vector[TRACE] = job_vector_end[TRACE];

    job_vector_len[TRACE]++;

    temp = temp->next;
  }


  if (job_vector[TRACE] == NULL)
    LOG_I (OMG, "[TRACE] Job Vector is NULL\n");

  return 0;
}

void
place_trace_node (node_struct * node, node_data * n)
{
  node_data *n_ptr = n;
  node->x_pos = (double) n_ptr->x_pos;
  node->mob->x_from = node->x_pos;
  node->mob->x_to = node->x_pos;

  node->y_pos = (double) n_ptr->y_pos;
  node->mob->y_from = node->y_pos;
  node->mob->y_to = node->y_pos;

  node->mob->speed = n_ptr->speed;
  node->mob->journey_time = 0.0;
  // node->start_journey=(double) n_ptr->time;      //time of initial position
  // node->mob->target_time=(double) n_ptr->next->time;  //time of next destination
  LOG_I (OMG,
         "[TRACE] Initial position of node ID: %d type: %d (X = %.2f, Y = %.2f) speed = %.2f \n ",
         node->id, node->type, node->x_pos, node->y_pos, node->mob->speed);

  node_vector_end[node->type] =
    (node_list *) add_entry (node, node_vector_end[node->type]);

  if (node_vector[node->type] == NULL)
    node_vector[node->type] = node_vector_end[node->type];

  node_vector_len[node->type]++;

}

void
move_trace_node (pair_struct * pair, node_data * n_data, double cur_time)
{
  node_struct *node;
  node = pair->b;
  double cdistance, journeytime, speed;
  double journeytime_next, distance;
  node->mob->x_from = node->mob->x_to;
  node->mob->y_from = node->mob->y_to;
  node->mobile = 1;
  node->mob->speed = n_data->speed;
  node->mob->x_to = (double) n_data->next->x_pos;
  node->mob->y_to = (double) n_data->next->y_pos;
  node->mob->target_time = (double) n_data->next->time;

  //if speed equals to zero compute the speed(i.e node was on sleep move it)
  if (node->mob->speed == 0) {
    cdistance =
      (double)
      sqrtf
      (pow (node->mob->x_from - node->mob->x_to, 2) +
       pow (node->mob->y_from - node->mob->y_to, 2));
    journeytime = node->mob->target_time - n_data->time;
    speed = (double) cdistance / journeytime;
    node->mob->speed = speed;
  }


  distance =
    (double)
    sqrtf
    (pow (node->mob->x_from - node->mob->x_to, 2) +
     pow (node->mob->y_from - node->mob->y_to, 2));


  journeytime_next = (double) distance / node->mob->speed;  //time to get to destination

  /* LOG_D (OMG,
   "[TRACE] mob->journey_time_next  %.2f target time %.2f next time %.2f speed %.2f\n",
   journeytime_next, node->mob->target_time, n_data->time,
   node->mob->speed);*/

  node->mob->start_journey = cur_time;
  pair->next_event_t = cur_time + journeytime_next; //when to reach the destination

}

void
schedule_trace_node (pair_struct * pair, node_data * n_data, double cur_time)
{
  node_struct *node = pair->b;
  node->mobile = 3;
  node->mob->start_journey = cur_time;
  pair->next_event_t = n_data->time;  //when to move this node
  //LOG_D (OMG, "[TRACE] Node will wake up at time: %.2f\n", pair->next_event_t);
}


void
sleep_trace_node (pair_struct * pair, node_data * n_data, double cur_time)
{
  //double journeytime, distance;
  node_struct *node = pair->b;
  node->mobile = 0;
  node->mob->speed = 0.0;
  node->mob->x_from = node->mob->x_to;
  node->mob->y_from = node->mob->y_to;
  node->x_pos = node->mob->x_to;
  node->y_pos = node->mob->y_to;
  //sleep duration

  /* */
  if (n_data->x_pos == n_data->next->x_pos &&
      n_data->y_pos == n_data->next->y_pos) {
    node->mob->sleep_duration = n_data->next->time - n_data->time;
    LOG_D(OMG,"[TRACE] curretn and next lcoation are the same : sleep for %f\n",node->mob->sleep_duration);
  } else { /* early arrival*/
    //sleep case 2
    //node->mob->sleep_duration = node->mob->target_time - (journeytime + node->mob->start_journey);
    node->mob->sleep_duration = node->mob->target_time - pair->next_event_t;
    LOG_D(OMG,"[TRACE] Early Arrival (target time > journeytime), sleep for %f target time was %f\n",node->mob->sleep_duration,node->mob->target_time);
  }

  node->mob->start_journey = cur_time;
  pair->next_event_t = cur_time + node->mob->sleep_duration;  //when to wake up
  /*LOG_D (OMG, "#[TRACE] node: %d \tsleep duration : %.2f\n", node->id,
   node->mob->sleep_duration);*/

}


void
update_trace_nodes (double cur_time)
{
  job_list *tmp;
  int done = 0;
  node_data *node_n;
  node_struct *my_node;


  tmp = job_vector[TRACE];

  if (tmp == NULL)
    LOG_D (OMG, "[TRACE] last data for all nodes\n");

  while (tmp != NULL && done != 1) {
    //1
    my_node = tmp->pair->b;
    node_n = get_next_data (table[my_node->type], my_node->gid, DATA_ONLY);


    //case1:time to next event equals to current time
    if (tmp->pair != NULL && tmp->pair->next_event_t >= cur_time - omg_eps
        && tmp->pair->next_event_t <= cur_time ) {
      if (node_n->next->next == NULL) {
        tmp->pair->b->x_pos = tmp->pair->b->mob->x_to;
        tmp->pair->b->mob->x_from = tmp->pair->b->x_pos;
        tmp->pair->b->y_pos = tmp->pair->b->mob->y_to;
        tmp->pair->b->mob->y_from = tmp->pair->b->y_pos;
        tmp->pair->b->mobile = 0;
        tmp->pair->b->mob->speed = 0.0;
        // LOG_D (OMG, "[TRACE] last data for all nodes\n");
        tmp = tmp->next;
        continue;
      }

      //LOG_D (OMG, "[TRACE] last data for all nodes\n");
      if (my_node->mobile == 1) {
        if (my_node->mob->target_time > tmp->pair->next_event_t) { //sleep node
          sleep_trace_node (tmp->pair, node_n, cur_time);
        } else {
          node_n =
            get_next_data (table[my_node->type], my_node->gid,
                           DATA_AND_STATUS_CHANGE);


          if (node_n->x_pos == node_n->next->x_pos &&
              node_n->y_pos == node_n->next->y_pos)

            sleep_trace_node (tmp->pair, node_n, cur_time);

          else
            move_trace_node (tmp->pair, node_n, cur_time);
        }

      } else if (my_node->mobile == 0) {
        node_n =
          get_next_data (table[my_node->type], my_node->gid, DATA_AND_STATUS_CHANGE);

        if (node_n->x_pos == node_n->next->x_pos &&
            node_n->y_pos == node_n->next->y_pos)

          sleep_trace_node (tmp->pair, node_n, cur_time);

        else
          move_trace_node (tmp->pair, node_n, cur_time);

      } else {
        node_n = get_next_data (table[my_node->type], my_node->gid, DATA_ONLY);

        if (node_n->x_pos == node_n->next->x_pos &&
            node_n->y_pos == node_n->next->y_pos)

          sleep_trace_node (tmp->pair, node_n, cur_time);

        else
          move_trace_node (tmp->pair, node_n, cur_time);

      }


    }

    //case2: current time is greater than the time to next event
    else if (tmp->pair != NULL && cur_time > tmp->pair->next_event_t) {
      if (node_n->next->next == NULL) {
        tmp->pair->b->x_pos = tmp->pair->b->mob->x_to;
        tmp->pair->b->mob->x_from = tmp->pair->b->x_pos;
        tmp->pair->b->y_pos = tmp->pair->b->mob->y_to;
        tmp->pair->b->mob->y_from = tmp->pair->b->y_pos;
        tmp->pair->b->mobile = 0;
        tmp->pair->b->mob->speed = 0.0;
        // LOG_D (OMG, "[TRACE] last data for all nodes\n");
        tmp = tmp->next;
        continue;
      }

      while (cur_time >= tmp->pair->next_event_t) {


        if (node_n->next->next == NULL) {
          tmp->pair->b->x_pos = tmp->pair->b->mob->x_to;
          tmp->pair->b->mob->x_from = tmp->pair->b->x_pos;
          tmp->pair->b->y_pos = tmp->pair->b->mob->y_to;
          tmp->pair->b->mob->y_from = tmp->pair->b->y_pos;
          break;
        }

        if (my_node->mobile == 1) {
          if (my_node->mob->target_time > tmp->pair->next_event_t) { //sleep node
            sleep_trace_node (tmp->pair, node_n, cur_time);
          } else {
            node_n =
              get_next_data (table[my_node->type], my_node->gid,
                             DATA_AND_STATUS_CHANGE);


            if (node_n->x_pos == node_n->next->x_pos &&
                node_n->y_pos == node_n->next->y_pos)

              sleep_trace_node (tmp->pair, node_n, cur_time);

            else
              move_trace_node (tmp->pair, node_n, cur_time);
          }

        } else if (my_node->mobile == 0) {
          node_n =
            get_next_data (table[my_node->type], my_node->gid,
                           DATA_AND_STATUS_CHANGE);

          if (node_n->x_pos == node_n->next->x_pos &&
              node_n->y_pos == node_n->next->y_pos)

            sleep_trace_node (tmp->pair, node_n, cur_time);

          else
            move_trace_node (tmp->pair, node_n, cur_time);

        } else {
          node_n = get_next_data (table[my_node->type], my_node->gid, DATA_ONLY);

          if (node_n->x_pos == node_n->next->x_pos &&
              node_n->y_pos == node_n->next->y_pos)

            sleep_trace_node (tmp->pair, node_n, cur_time);

          else
            move_trace_node (tmp->pair, node_n, cur_time);

        }

        node_n = get_next_data (table[my_node->type], my_node->gid, DATA_ONLY);

        if (node_n == NULL || node_n->next == NULL) {
          break;
        }

      }     //2
    }
    //case3: current time less than the time to next event
    else {
      done = 1;   //quit the loop
    }


    tmp = tmp->next;

  }       //1

  //sorting the new entries
  //LOG_D (OMG, "--------DISPLAY JOB LIST--------\n");  //LOG_T
  // display_job_list (Job_Vector);
  job_vector[TRACE] = quick_sort (job_vector[TRACE]); ///////////
  //LOG_D (OMG, "--------DISPLAY JOB LIST AFTER SORTING--------\n");
  // display_job_list( job_vector[TRACE]);
}


void
get_trace_positions_updated (double cur_time)
{
  double x_now = 0.0, y_now = 0.0;
  double len, dx, dy;
  job_list *tmp = job_vector[TRACE];

  while (tmp != NULL) {

    if (tmp->pair->b->mobile == 1 && tmp->pair->next_event_t >= cur_time) {

      //printf("hiiiiiiiiiii \n");
      len =
        sqrtf (pow
               (tmp->pair->b->mob->x_from -
                tmp->pair->b->mob->x_to,
                2) + pow (tmp->pair->b->mob->y_from -
                          tmp->pair->b->mob->y_to, 2));

      if (len != 0) {
        dx =
          fabs (tmp->pair->b->mob->x_from -
                tmp->pair->b->mob->x_to) / len;
        dy =
          fabs (tmp->pair->b->mob->y_from -
                tmp->pair->b->mob->y_to) / len;

        //x coordinate
        if (tmp->pair->b->mob->x_from < tmp->pair->b->mob->x_to) {
          x_now =
            tmp->pair->b->mob->x_from +
            (dx *
             (tmp->pair->b->mob->speed *
              (cur_time - tmp->pair->b->mob->start_journey)));
        } else {
          x_now =
            tmp->pair->b->mob->x_from -
            (dx *
             (tmp->pair->b->mob->speed *
              (cur_time - tmp->pair->b->mob->start_journey)));
        }

        //y coordinate
        if (tmp->pair->b->mob->y_from < tmp->pair->b->mob->y_to) {
          y_now =
            tmp->pair->b->mob->y_from +
            (dy *
             (tmp->pair->b->mob->speed *
              (cur_time - tmp->pair->b->mob->start_journey)));
        } else {
          y_now =
            tmp->pair->b->mob->y_from -
            (dy *
             (tmp->pair->b->mob->speed *
              (cur_time - tmp->pair->b->mob->start_journey)));
        }

        tmp->pair->b->x_pos = (double) ((int) (x_now * 100)) / 100;
        tmp->pair->b->y_pos = (double) ((int) (y_now * 100)) / 100;



      } else {
        tmp->pair->b->x_pos = tmp->pair->b->mob->x_to;
        tmp->pair->b->y_pos = tmp->pair->b->mob->y_to;
      }

    }

    tmp = tmp->next;

  }


}


void
clear_list (void)
{


}
