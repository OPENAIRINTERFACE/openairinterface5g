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

/*! \file rwalk.c
* \brief random walk mobility generator
* \author S. Gashaw, N. Nikaein, J. Harri, M. Mahersi
* \date 2014
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "rwalk.h"
#include "steadystaterwp.h"


int
start_rwalk_generator (omg_global_param omg_param_list)
{

  int id;
  static int n_id = 0;
  double cur_time = 0.0, pause_p, mean_pause, mean_travel;
  node_struct *node = NULL;
  mobility_struct *mobility = NULL;
  pair_struct *pair = NULL;

  srand (omg_param_list.seed + RWALK);

  mean_pause = (omg_param_list.max_sleep - omg_param_list.min_sleep) / 2;
  mean_travel =
    (omg_param_list.max_journey_time - omg_param_list.min_journey_time) / 2;
  pause_p = mean_pause / (mean_travel + mean_pause);

  LOG_I (OMG, "#RWALK mobility model for %d %d nodes\n", omg_param_list.nodes,
         omg_param_list.nodes_type);

  for (id = n_id; id < (omg_param_list.nodes + n_id); id++) {

    node = create_node ();
    mobility = create_mobility ();

    node->id = id;
    node->type = omg_param_list.nodes_type;
    node->mob = mobility;
    node->generator = RWALK;
    node->event_num = 0;
    place_rwalk_node (node);  //initial positions

    pair = (pair_struct *) malloc (sizeof (struct pair_struct));
    pair->b = node;
    // sleep_rwalk_node (pair, cur_time); //sleep
    //pause probability...some of the nodes start at pause & the other on move

    if (randomgen (0, 1) < pause_p)
      sleep_steadystaterwp_node (pair, cur_time); //sleep
    else
      move_steadystaterwp_node (pair, cur_time);


    job_vector_end[RWALK] = add_job (pair, job_vector_end[RWALK]);

    if (job_vector[RWALK] == NULL)
      job_vector[RWALK] = job_vector_end[RWALK];

    job_vector_len[RWALK]++;
  }

  n_id += omg_param_list.nodes;



  if (job_vector[RWALK] == NULL)
    LOG_E (OMG, "[RWP] Job Vector is NULL\n");

  return (0);
}


void
place_rwalk_node (node_struct * node)
{
  node->x_pos =
    (double) ((int)
              (randomgen
               (omg_param_list[node->type].min_x,
                omg_param_list[node->type].max_x) * 100)) / 100;
  node->mob->x_from = node->x_pos;
  node->mob->x_to = node->x_pos;
  node->y_pos =
    (double) ((int)
              (randomgen
               (omg_param_list[node->type].min_y,
                omg_param_list[node->type].max_y) * 100)) / 100;
  node->mob->y_from = node->y_pos;
  node->mob->y_to = node->y_pos;
  node->mob->speed = 0.0;
  node->mob->journey_time = 0.0;

  // LOG_D (OMG, " INITIALIZE RWALK NODE\n ");
  LOG_I (OMG,
         "#Initial position of node ID: %d type: %d (X = %.2f, Y = %.2f) speed = 0.0\n",
         node->id, node->type, node->x_pos, node->y_pos);

  node_vector_end[node->type] =
    (node_list *) add_entry (node, node_vector_end[node->type]);

  if (node_vector[node->type] == NULL)
    node_vector[node->type] = node_vector_end[node->type];

  node_vector_len[node->type]++;

}


void
sleep_rwalk_node (pair_struct * pair, double cur_time)
{
  node_struct *node;
  node = pair->b;
  node->mobile = 0;
  node->mob->speed = 0.0;
  node->mob->x_from = node->mob->x_to;
  node->mob->y_from = node->mob->y_to;
  node->x_pos = node->mob->x_to;
  node->y_pos = node->mob->y_to;

  if (node->event_num == 0)
    node->event_num++;

  node->mob->sleep_duration =
    (double) ((int)
              (randomgen
               (omg_param_list[node->type].min_sleep,
                omg_param_list[node->type].max_sleep) * 100)) / 100;
  // LOG_D (OMG, "node: %d \tsleep duration : %.2f\n", node->ID,
  //       node->mob->sleep_duration);
  node->mob->start_journey = cur_time;
  pair->next_event_t = cur_time + node->mob->sleep_duration;  //when to wake up
  // LOG_D (OMG, "to wake up at time: cur_time + sleep_duration : %.2f\n",
  //       pair->a);

}


void
move_rwalk_node (pair_struct * pair, double cur_time)
{
  double distance, x_next, y_next, journeytime_next;
  int i = 0;
  double dx, dy, x_now, y_now;
  double alpha, distancex, distancey;
  node_struct *node;
  node = pair->b;
  //LOG_D (OMG, "MOVE RWALK NODE ID: %d\n", node->ID);
  node->mob->x_from = node->mob->x_to;
  node->mob->y_from = node->mob->y_to;
  node->x_pos = node->mob->x_to;
  node->y_pos = node->mob->y_to;
  node->mobile = 1;

  node->mob->speed =
    randomgen (omg_param_list[node->type].min_speed,
               omg_param_list[node->type].max_speed);

  node->mob->azimuth =
    randomgen (omg_param_list[node->type].min_azimuth,
               omg_param_list[node->type].max_azimuth);

  if (node->event_num == 0) {
    node->mob->journey_time = residualtime (omg_param_list[node->type]);
    node->event_num++;
  } else {
    node->mob->journey_time =
      randomgen (omg_param_list[node->type].min_journey_time,
                 omg_param_list[node->type].max_journey_time);
  }

  distance = node->mob->speed * node->mob->journey_time;

  dx = distance * cos (node->mob->azimuth * M_PI / 180);

  dy = distance * sin (node->mob->azimuth * M_PI / 180);

  x_next = (double) ((int) ((node->mob->x_from + dx) * 100)) / 100;
  y_next = (double) ((int) ((node->mob->y_from + dy) * 100)) / 100;
  /* LOG_D(OMG,"#node %d X FROM %.2f x next %.2f Y FROM %.2f y next %.2f \n\n",node->id,node->mob->x_from,x_next,  node->mob->y_from,y_next); */

  alpha = (node->mob->azimuth * M_PI / 180);  //in radian
  x_now = node->mob->x_from;
  y_now = node->mob->y_from;

  while (true) {

    if (x_next < omg_param_list[node->type].min_x) {
      distancex =
        (omg_param_list[node->type].min_x - x_now) / cos (alpha);

    } else if (x_next > omg_param_list[node->type].max_x) {
      distancex =
        (omg_param_list[node->type].max_x - x_now) / cos (alpha);

    } else {
      distancex = distance;
    }

    if (y_next < omg_param_list[node->type].min_y) {
      distancey =
        (omg_param_list[node->type].min_y - y_now) / sin (alpha);

    } else if (y_next > omg_param_list[node->type].max_y) {
      distancey =
        (omg_param_list[node->type].max_y - y_now) / sin (alpha);

    } else {
      distancey = distance;
    }


    if ((distancex == distance && distancey == distance))
      break;



    if (distancey < distancex) {

      x_now = distancey * cos (alpha) + x_now;
      y_now = distancey * sin (alpha) + y_now;
      distance = distance - distancey;

      dx = distance * cos (2 * M_PI - alpha);
      dy = distance * sin (2 * M_PI - alpha);

      x_next = x_now + dx;
      y_next = y_now + dy;

      alpha = 2 * M_PI - alpha;

    } else if (distancex < distancey) {

      x_now = distancex * cos (alpha) + x_now;
      y_now = distancex * sin (alpha) + y_now;

      distance = distance - distancex;

      dx = distance * cos (M_PI - alpha);
      dy = distance * sin (M_PI - alpha);

      x_next = x_now + dx;
      y_next = y_now + dy;

      alpha = M_PI - alpha;
    }

    i++;

  }

  node->mob->x_to = x_next;
  node->mob->y_to = y_next;

  /*LOG_D(OMG,"#node %d x to %.2f y to %.2f \n\n",node->id,node->mob->x_to,node->mob->y_to);
     node->mob->start_journey = cur_time; */
  journeytime_next =
    (double) ((int) (distance / node->mob->speed * 100)) / 100;
  pair->next_event_t =
    (double) ((int) ((node->mob->start_journey + journeytime_next) * 100)) /
    100;

  if (node->event_num < 100) {
    event_sum[node->event_num] += node->mob->speed;
    events[node->event_num]++;
    node->event_num++;
  }

}

double
residualtime (omg_global_param omg_param)
{
  double u;
  u = randomgen (0, 1);

  if (u <
      (2 * omg_param.min_sleep /
       (omg_param.max_journey_time + omg_param.min_journey_time)))
    return u * (omg_param.max_journey_time + omg_param.min_journey_time) / 2;
  else
    return omg_param.max_journey_time -
           sqrtf ((1 - u) * (pow (omg_param.max_journey_time, 2) -
                             pow (omg_param.min_sleep, 2)));
}


void
update_rwalk_nodes (double cur_time)
{
  int done = 0;
  job_list *tmp = job_vector[RWALK];
  node_struct *my_node;

  while (tmp != NULL && done == 0) {
    //case1:time to next event equals to current time
    if (tmp->pair != NULL
        && ((double) tmp->pair->next_event_t >= cur_time - omg_eps)
        && ((double) tmp->pair->next_event_t <= cur_time + omg_eps)) {
      my_node = tmp->pair->b;

      if (my_node->mobile == 1)
        sleep_rwalk_node (tmp->pair, cur_time);
      else
        move_rwalk_node (tmp->pair, cur_time);

    }
    //case2: current time is greater than the time to next event

    else if (tmp->pair != NULL
             && (cur_time - omg_eps) > tmp->pair->next_event_t) {

      my_node = tmp->pair->b;

      while (cur_time >= tmp->pair->next_event_t) {
        if (my_node->mobile == 1)
          sleep_rwalk_node (tmp->pair, cur_time);
        else
          move_rwalk_node (tmp->pair, cur_time);

      }
    }
    //case3: current time less than the time to next event
    else {
      done = 1;   //quit the loop
    }


    tmp = tmp->next;
  }

  //sorting the new entries
  // LOG_D (OMG, "--------DISPLAY JOB LIST--------\n");  //LOG_T
  //display_job_list (Job_Vector);
  job_vector[RWALK] = quick_sort (job_vector[RWALK]); ///////////
  // LOG_D (OMG, "--------DISPLAY JOB LIST AFTER SORTING--------\n");
  //display_job_list (job_vector[RWALK]);
}

void
get_rwalk_positions_updated (double cur_time)
{

  double x_now = 0.0, y_now = 0.0;
  double len, dx, dy;
  double distancex, distancey, alpha;
  double x_next, y_next;
  job_list *tmp = job_vector[RWALK];

  while (tmp != NULL) {

    if (tmp->pair->b->mobile == 1 && tmp->pair->next_event_t >= cur_time) {

      len =
        tmp->pair->b->mob->speed * (cur_time -
                                    tmp->pair->b->mob->start_journey);

      dx = len * cos (tmp->pair->b->mob->azimuth * M_PI / 180);
      dy = len * sin (tmp->pair->b->mob->azimuth * M_PI / 180);

      x_next =
        (double) ((int) ((tmp->pair->b->mob->x_from + dx) * 100)) / 100;
      y_next =
        (double) ((int) ((tmp->pair->b->mob->y_from + dy) * 100)) / 100;


      alpha = (tmp->pair->b->mob->azimuth * M_PI / 180);  //in radian
      x_now = tmp->pair->b->mob->x_from;
      y_now = tmp->pair->b->mob->y_from;

      while (true) {

        if (x_next < omg_param_list[tmp->pair->b->type].min_x) {
          distancex =
            (omg_param_list[tmp->pair->b->type].min_x -
             x_now) / cos (alpha);
        } else if (x_next > omg_param_list[tmp->pair->b->type].max_x) {
          distancex =
            (omg_param_list[tmp->pair->b->type].max_x -
             x_now) / cos (alpha);
        } else {
          distancex = len;
        }

        if (y_next < omg_param_list[tmp->pair->b->type].min_y) {
          distancey =
            (omg_param_list[tmp->pair->b->type].min_y -
             y_now) / sin (alpha);
        } else if (y_next > omg_param_list[tmp->pair->b->type].max_y) {
          distancey =
            (omg_param_list[tmp->pair->b->type].max_y -
             y_now) / sin (alpha);
        } else {
          distancey = len;
        }

        if (distancex == len && distancey == len)
          break;


        if (distancey < distancex) {
          x_now = distancey * cos (alpha) + x_now;
          y_now = distancey * sin (alpha) + y_now;
          len = len - distancey;

          dx = len * cos (2 * M_PI - alpha);
          dy = len * sin (2 * M_PI - alpha);

          x_next = x_now + dx;
          y_next = y_now + dy;

          alpha = 2 * M_PI - alpha;

        } else if (distancex < distancey) {
          x_now = distancex * cos (alpha) + x_now;
          y_now = distancex * sin (alpha) + y_now;

          len = len - distancex;

          dx = len * cos (M_PI - alpha);
          dy = len * sin (M_PI - alpha);

          x_next = x_now + dx;
          y_next = y_now + dy;

          alpha = M_PI - alpha;
        }


      }

      tmp->pair->b->x_pos = (double) ((int) (x_next * 100)) / 100;
      tmp->pair->b->y_pos = (double) ((int) (y_next * 100)) / 100;


    } else {
      tmp->pair->b->x_pos = tmp->pair->b->mob->x_to;
      tmp->pair->b->y_pos = tmp->pair->b->mob->y_to;
    }

    //if(tmp->pair->b->id==2 && ((int)(cur_time*1000)% 100)==0)
    //LOG_D (OMG, "%d  %.2f %.2f \n\n",tmp->pair->b->id,tmp->pair->b->x_pos,tmp->pair->b->y_pos);

    tmp = tmp->next;

  }
}
