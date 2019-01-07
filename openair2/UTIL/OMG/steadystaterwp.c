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

/*! \file steadystaterwp.c
* \brief random waypoint mobility generator
* \author  S. Gashaw,  J. Harri, N. Nikaein,
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
#include "omg.h"
#include "steadystaterwp.h"
#include "rwp.h"
#define RESTIRICTED_RWP 1
#define CONNECTED_DOMAIN 2

int
start_steadystaterwp_generator (omg_global_param omg_param_list)
{
  static int n_id = 0;
  int id;
  double cur_time = 0.0, pause_p;
  node_struct *node = NULL;
  mobility_struct *mobility = NULL;
  pair_struct *pair = NULL;

  pause_p = pause_probability (omg_param_list);

  srand (omg_param_list.seed + RWP);

  LOG_I (OMG, "STEADY_RWP mobility model for %d %d nodes\n",
         omg_param_list.nodes, omg_param_list.nodes_type);

  for (id = n_id; id < (omg_param_list.nodes + n_id); id++) {

    node = create_node ();
    mobility = create_mobility ();

    node->id = id;
    node->type = omg_param_list.nodes_type;
    node->mob = mobility;
    node->generator = STEADY_RWP;
    node->event_num = 0;
    place_rwp_node (node);  //initial positions

    pair = (pair_struct *) malloc (sizeof (struct pair_struct));
    pair->b = node;
    //pause probability...some of the nodes start at pause & the other on move

    if (randomgen (0, 1) < pause_p)
      sleep_steadystaterwp_node (pair, cur_time); //sleep
    else
      move_steadystaterwp_node (pair, cur_time);

    job_vector_end[STEADY_RWP] = add_job (pair, job_vector_end[STEADY_RWP]);

    if (job_vector[STEADY_RWP] == NULL)
      job_vector[STEADY_RWP] = job_vector_end[STEADY_RWP];

    job_vector_len[STEADY_RWP]++;
  }

  n_id += omg_param_list.nodes;



  if (job_vector[STEADY_RWP] == NULL)
    LOG_E (OMG, "[STEADY_RWP] Job Vector is NULL\n");

  return (0);
}

/*********************************xxxxxx************************************/

void
place_steadystaterwp_node (node_struct * node)
{
  int loc_num;
  double block_xmin, block_ymin;


  if (grid == RESTIRICTED_RWP) {
    loc_num = (int) randomgen (0, omg_param_list[node->type].max_vertices);
    node->x_pos =
      (double) vertice_xpos (loc_num, omg_param_list[node->type]);
    node->y_pos =
      (double) vertice_ypos (loc_num, omg_param_list[node->type]);
    //LOG_D(OMG,"location number %d x pos %.2f y pos %.2f \n\n",loc_num,node->x_pos,node->y_pos);
  } else if (grid == CONNECTED_DOMAIN) {
    node->block_num =
      (int) randomgen (0, omg_param_list[node->type].max_block_num);
    block_xmin = area_minx (node->block_num, omg_param_list[node->type]);
    block_ymin = area_miny (node->block_num, omg_param_list[node->type]);

    node->x_pos =
      (double) ((int) (randomgen (block_xmin, xloc_div + block_xmin) * 100))
      / 100;

    node->y_pos =
      (double) ((int) (randomgen (block_ymin, yloc_div + block_ymin) * 100))
      / 100;

  } else {
    node->x_pos =
      (double) ((int)
                (randomgen
                 (omg_param_list[node->type].min_x,
                  omg_param_list[node->type].max_x) * 100)) / 100;

    node->y_pos =
      (double) ((int)
                (randomgen
                 (omg_param_list[node->type].min_y,
                  omg_param_list[node->type].max_y) * 100)) / 100;
  }

  node->mob->x_from = node->x_pos;
  node->mob->x_to = node->x_pos;
  node->mob->y_from = node->y_pos;
  node->mob->speed = 0.0;
  node->mob->journey_time = 0.0;
  LOG_I (OMG,
         "#[STEADY_RWP] Initial position of node ID: %d type: %d X = %.2f, Y = %.2f\n ",
         node->id, node->type, node->x_pos, node->y_pos);

  node_vector_end[node->type] =
    (node_list *) add_entry (node, node_vector_end[node->type]);

  if (node_vector[node->type] == NULL)
    node_vector[node->type] = node_vector_end[node->type];

  node_vector_len[node->type]++;
  //Initial_Node_Vector_len[RWP]++;
}


/*********************************xxxxxx************************************/

void
sleep_steadystaterwp_node (pair_struct * pair, double cur_time)
{
  //static int initial = 1;
  node_struct *node;
  node = pair->b;
  node->mobile = 0;
  node->mob->speed = 0.0;
  node->mob->x_from = node->mob->x_to;
  node->mob->y_from = node->mob->y_to;
  node->x_pos = node->mob->x_to;
  node->y_pos = node->mob->y_to;

  if (node->event_num == 0) {
    node->mob->sleep_duration = initial_pause (omg_param_list[node->type]);
    node->event_num++;
  } else {
    node->mob->sleep_duration =
      (double) ((int)
                (randomgen
                 (omg_param_list[node->type].min_sleep,
                  omg_param_list[node->type].max_sleep) * 100)) / 100;
  }

  // LOG_D (OMG, "[STEADY_RWP] node: %d \tsleep duration : %.2f\n", node->ID,
  //       node->mob->sleep_duration);

  node->mob->start_journey = cur_time;
  pair->next_event_t = cur_time + node->mob->sleep_duration;  //when to wake up
  // LOG_D (OMG, "[STEADY_RWP] wake up at time: cur_time + sleep_duration : %.2f\n",
  //       pair->a);


}


void
move_steadystaterwp_node (pair_struct * pair, double cur_time)
{
  //static int initial = 1;
  double distance, journeytime_next, max_distance;
  double temp_x, temp_y, u1, u2;
  //int loc_num;
  double pr, block_xmin, block_ymin;

  //LOG_D (OMG, "[STEADY_RWP] move node: %d\n", node->ID);
  node_struct *node;
  node = pair->b;
  node->mob->x_from = node->mob->x_to;
  node->mob->y_from = node->mob->y_to;
  node->x_pos = node->mob->x_to;
  node->y_pos = node->mob->y_to;
  node->mobile = 1;

  //initial move
  if (node->event_num == 0) {


    if (grid == CONNECTED_DOMAIN) {
      max_distance = 2 * yloc_div;

      /* sqrtf (pow
         (omg_param_list[node->type].max_x -
         omg_param_list[node->type].min_x,
         2) + pow (omg_param_list[node->type].max_y -
         omg_param_list[node->type].min_y, 2)); */
      while (true) {
        pr = randomgen (0, 1);

        if (pr <= 0.50)
          /*node->block_num =
             (int) randomgen (0,
             omg_param_list[node->type].max_block_num); */
          node->block_num =
            (int) next_block (node->block_num,
                              omg_param_list[node->type]);

        block_xmin =
          area_minx (node->block_num, omg_param_list[node->type]);
        block_ymin =
          area_miny (node->block_num, omg_param_list[node->type]);

        node->mob->x_to =
          (double) ((int)
                    (randomgen (block_xmin, xloc_div + block_xmin) *
                     100)) / 100;

        node->mob->y_to =
          (double) ((int)
                    (randomgen (block_ymin, yloc_div + block_ymin) *
                     100)) / 100;
        pr = randomgen (0, 1);

        if (pr <= 0.50)
          /*node->block_num =
             (int) randomgen (0,
             omg_param_list[node->type].max_block_num); */
          node->block_num =
            (int) next_block (node->block_num,
                              omg_param_list[node->type]);

        block_xmin =
          area_minx (node->block_num, omg_param_list[node->type]);
        block_ymin =
          area_miny (node->block_num, omg_param_list[node->type]);

        temp_x =
          (double) ((int)
                    (randomgen (block_xmin, xloc_div + block_xmin) *
                     100)) / 100;

        temp_y =
          (double) ((int)
                    (randomgen (block_ymin, yloc_div + block_ymin) *
                     100)) / 100;


        u1 = randomgen (0, 1);

        if (u1 <
            (sqrtf
             (pow (node->mob->x_to - temp_x, 2) +
              pow (node->mob->y_to - temp_y, 2)) / max_distance)) {
          u2 = randomgen (0, 1);
          distance =
            (double) ((int)
                      (sqrtf (pow (temp_x - node->mob->x_to, 2) +
                              pow (temp_y - node->mob->y_to,
                                   2)) * 100)) / 100;
          node->mob->azimuth = atan2 (node->mob->y_to - node->mob->y_from, node->mob->x_to - node->mob->x_from);  //radian
          node->mob->x_from =
            temp_x + u2 * distance * cos (node->mob->azimuth);
          node->mob->y_from =
            temp_y + u2 * distance * sin (node->mob->azimuth);
          break;
        }
      }
    } else {
      max_distance =
        sqrtf (pow
               (omg_param_list[node->type].max_x -
                omg_param_list[node->type].min_x,
                2) + pow (omg_param_list[node->type].max_y -
                          omg_param_list[node->type].min_y, 2));

      while (true) {
        node->mob->x_to =
          (double) ((int)
                    (randomgen
                     (omg_param_list[node->type].min_x,
                      omg_param_list[node->type].max_x) * 100)) / 100;

        node->mob->y_to =
          (double) ((int)
                    (randomgen
                     (omg_param_list[node->type].min_y,
                      omg_param_list[node->type].max_y) * 100)) / 100;

        temp_x = (double) ((int)
                           (randomgen
                            (omg_param_list[node->type].min_x,
                             omg_param_list[node->type].max_x) * 100)) /
                 100;

        temp_y = (double) ((int)
                           (randomgen
                            (omg_param_list[node->type].min_y,
                             omg_param_list[node->type].max_y) * 100)) /
                 100;

        u1 = randomgen (0, 1);

        if (u1 <
            (sqrtf
             (pow (node->mob->x_to - temp_x, 2) +
              pow (node->mob->y_to - temp_y, 2)) / max_distance)) {
          u2 = randomgen (0, 1);
          node->mob->x_from =
            u2 * temp_x + (1 - u2) * node->mob->x_to;
          node->mob->y_from =
            u2 * temp_y + (1 - u2) * node->mob->y_to;
          break;
        }
      }
    }


    node->mob->speed = initial_speed (omg_param_list[node->type]);
    node->event_num++;
    distance =
      (double) ((int)
                (sqrtf (pow (node->mob->x_from - node->mob->x_to, 2) +
                        pow (node->mob->y_from - node->mob->y_to,
                             2)) * 100)) / 100;

  }
  //for the next move
  else {

    if (grid == CONNECTED_DOMAIN) {
      pr = randomgen (0, 1);

      if (pr <= 0.50)
        /* node->block_num =
           (int) randomgen (0, omg_param_list[node->type].max_block_num); */
        node->block_num =
          (int) next_block (node->block_num, omg_param_list[node->type]);

      block_xmin =
        area_minx (node->block_num, omg_param_list[node->type]);
      block_ymin =
        area_miny (node->block_num, omg_param_list[node->type]);

      node->mob->x_to =
        (double) ((int)
                  (randomgen (block_xmin, xloc_div + block_xmin) * 100)) /
        100;

      node->mob->y_to =
        (double) ((int)
                  (randomgen (block_ymin, yloc_div + block_ymin) * 100)) /
        100;

      distance =
        (double) ((int) (sqrtf
                         (pow (node->mob->x_from - node->mob->x_to, 2) +
                          pow (node->mob->y_from - node->mob->y_to,
                               2)) * 100)) / 100;

      node->mob->azimuth =
        atan2 (node->mob->y_to - node->mob->y_from,
               node->mob->x_to - node->mob->x_from);
    } else {
      node->mob->x_to =
        (double) ((int)
                  (randomgen
                   (omg_param_list[node->type].min_x,
                    omg_param_list[node->type].max_x) * 100)) / 100;

      node->mob->y_to =
        (double) ((int)
                  (randomgen
                   (omg_param_list[node->type].min_y,
                    omg_param_list[node->type].max_y) * 100)) / 100;

      distance =
        (double) ((int)
                  (sqrtf (pow (node->mob->x_from - node->mob->x_to, 2) +
                          pow (node->mob->y_from - node->mob->y_to,
                               2)) * 100)) / 100;


    }

    node->mob->speed =
      (double) ((int)
                (randomgen
                 (omg_param_list[node->type].min_speed,
                  omg_param_list[node->type].max_speed) * 100)) / 100;
    node->event_num++;

  }

  journeytime_next = (double) ((int) (distance / node->mob->speed * 100)) / 100;  //duration to get to dest

  //LOG_D (OMG, "[STEADY_RWP] mob->journey_time_next %.2f\n", journeyTime_next);

  node->mob->start_journey = cur_time;
  //LOG_D (OMG, "[STEADY_RWP] start_journey %.2f\n", node->mob->start_journey);
  pair->next_event_t = cur_time + journeytime_next; //when to reach the destination
  // LOG_D (OMG,
  // "[STEADY_RWP] reaching the destination at time : start journey + journey_time next =%.2f\n",
  // pair->a);

  if (node->event_num < 100) {
    event_sum[node->event_num] += node->mob->speed;
    events[node->event_num]++;
    node->event_num++;
  }

}

double
pause_probability (omg_global_param omg_param)
{

  double alpha, delta;
  alpha =
    (omg_param.max_sleep + omg_param.min_sleep) * (omg_param.max_speed -
        omg_param.min_speed) / (2 *
                                log
                                (omg_param.
                                 max_speed
                                 /
                                 omg_param.
                                 min_speed));

  delta =
    sqrtf (pow (omg_param.max_x - omg_param.min_x, 2) +
           pow (omg_param.max_y - omg_param.min_y, 2));

  return alpha / (alpha + delta);

}

double
initial_pause (omg_global_param omg_param)
{
  double u;
  u = randomgen (0, 1);

  if (u <
      (2 * omg_param.min_sleep / (omg_param.max_sleep + omg_param.min_sleep)))
    return u * (omg_param.max_sleep + omg_param.min_sleep) / 2;

  else
    return omg_param.max_sleep -
           sqrtf ((1 - u) * (pow (omg_param.max_sleep, 2) -
                             pow (omg_param.min_sleep, 2)));
}

double
initial_speed (omg_global_param omg_param)
{
  double u;
  u = randomgen (0, 1);

  return pow (omg_param.max_speed, u) / pow (omg_param.min_speed, u - 1);

}

/*update RWP nodes position*/

void
update_steadystaterwp_nodes (double cur_time)
{

  int done = 0;
  job_list *tmp = job_vector[STEADY_RWP];
  node_struct *my_node;

  while (tmp != NULL && done == 0) {
    //case1:time to next event equals to current time
    if (tmp->pair != NULL
        && ((double) tmp->pair->next_event_t >= cur_time - omg_eps)
        && ((double) tmp->pair->next_event_t <= cur_time + omg_eps)) {
      my_node = tmp->pair->b;

      if (my_node->mobile == 1)
        sleep_rwp_node (tmp->pair, cur_time);
      else
        move_rwp_node (tmp->pair, cur_time);

    }
    //case2: current time is greater than the time to next event

    else if (tmp->pair != NULL
             && (cur_time - omg_eps) > tmp->pair->next_event_t) {

      my_node = tmp->pair->b;

      while (cur_time >= tmp->pair->next_event_t) {
        if (my_node->mobile == 1)
          sleep_rwp_node (tmp->pair, cur_time);
        else
          move_rwp_node (tmp->pair, cur_time);

      }
    }
    //case3: current time less than the time to next event
    else {
      done = 1;   //quit the loop
    }


    tmp = tmp->next;
  }

  //sorting the new entries
  //LOG_D (OMG, "--------DISPLAY JOB LIST--------\n");   //LOG_T
  // display_job_list (job_vector);
  job_vector[STEADY_RWP] = quick_sort (job_vector[STEADY_RWP]); ///////////
  // LOG_D (OMG, "--------DISPLAY JOB LIST AFTER SORTING--------\n");
  // display_job_list (job_vector[STEADY_RWP]);
}


void
get_steadystaterwp_positions_updated (double cur_time)
{

  double x_now = 0.0, y_now = 0.0;
  double len, dx, dy;
  job_list *tmp = job_vector[STEADY_RWP];

  while (tmp != NULL) {

    if (tmp->pair->b->mobile == 1 && tmp->pair->next_event_t >= cur_time) {


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
        //len

      }


    } else {
      tmp->pair->b->x_pos = tmp->pair->b->mob->x_to;
      tmp->pair->b->y_pos = tmp->pair->b->mob->y_to;
    }


    tmp = tmp->next;

  }

}
