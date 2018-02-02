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

/*! \file rwp.c
* \brief random waypoint mobility generator
* \author  M. Mahersi,  N. Nikaein,  J. Harri
* \date 2011
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
#include "rwp.h"



int
start_rwp_generator (omg_global_param omg_param_list)
{
  static int n_id = 0;
  int id;
  double cur_time = 0.0;
  node_struct *node = NULL;
  mobility_struct *mobility = NULL;
  pair_struct *pair = NULL;

  srand (omg_param_list.seed + RWP);

  LOG_I (OMG, "# RWP mobility model for %d type %d nodes\n", omg_param_list.nodes,
         omg_param_list.nodes_type);

  for (id = n_id; id < (omg_param_list.nodes + n_id); id++) {

    node = create_node ();
    mobility = create_mobility ();

    node->id = id;
    node->type = omg_param_list.nodes_type;
    node->mob = mobility;
    node->generator = RWP;
    node->event_num = 0;
    place_rwp_node (node);  //initial positions

    pair = (pair_struct *) malloc (sizeof (struct pair_struct));
    pair->b = node;
    sleep_rwp_node (pair, cur_time);  //sleep

    job_vector_end[RWP] = add_job (pair, job_vector_end[RWP]);

    if (job_vector[RWP] == NULL)
      job_vector[RWP] = job_vector_end[RWP];

    job_vector_len[RWP]++;
  }

  n_id += omg_param_list.nodes;



  if (job_vector[RWP] == NULL)
    LOG_E (OMG, "[RWP] Job Vector is NULL\n");

  return (0);
}


void
place_rwp_node (node_struct * node)
{

  int loc_num;
  double block_xmin, block_ymin;


  if (omg_param_list[node->type].rwp_type == RESTIRICTED_RWP) {
    loc_num = (int) randomgen (0, omg_param_list[node->type].max_vertices);
    node->x_pos =
      (double) vertice_xpos (loc_num, omg_param_list[node->type]);
    node->y_pos =
      (double) vertice_ypos (loc_num, omg_param_list[node->type]);
    //LOG_D(OMG,"location number %d x pos %.2f y pos %.2f \n\n",loc_num,node->x_pos,node->y_pos);
  } else if (omg_param_list[node->type].rwp_type == CONNECTED_DOMAIN) {
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
  node->mob->y_to = node->y_pos;
  node->mob->speed = 0.0;
  node->mob->journey_time = 0.0;
  LOG_I (OMG,
         " #[RWP] Initial position of node ID: %d type: %d (X = %.2f, Y = %.2f) speed = 0.0\n ",
         node->id, node->type, node->x_pos, node->y_pos);

  node_vector_end[node->type] =
    (node_list *) add_entry (node, node_vector_end[node->type]);

  if (node_vector[node->type] == NULL)
    node_vector[node->type] = node_vector_end[node->type];

  node_vector_len[node->type]++;
  //Initial_Node_Vector_len[RWP]++;
}


void
sleep_rwp_node (pair_struct * pair, double cur_time)
{
  node_struct *node;
  node = pair->b;
  node->mobile = 0;
  node->mob->speed = 0.0;
  node->mob->x_from = node->mob->x_to;
  node->mob->y_from = node->mob->y_to;
  node->x_pos = node->mob->x_to;
  node->y_pos = node->mob->y_to;

  node->mob->sleep_duration =
    (double) ((int)
              (randomgen
               (omg_param_list[node->type].min_sleep,
                omg_param_list[node->type].max_sleep) * 100)) / 100;
  /*LOG_D (OMG, "#[RWP] node: %d \tsleep duration : %.2f\n", node->id,
     node->mob->sleep_duration); */

  node->mob->start_journey = cur_time;
  pair->next_event_t = cur_time + node->mob->sleep_duration;  //when to wake up
  // LOG_D (OMG, "[RWP] wake up at time: cur_time + sleep_duration : %.2f\n",
  //       pair->a);



}


void
move_rwp_node (pair_struct * pair, double cur_time)
{
  int loc_num;
  double distance, journeytime_next;
  double pr, block_xmin, block_ymin;
  //LOG_D (OMG, "[RWP] move node: %d\n", node->ID);
  node_struct *node;
  node = pair->b;
  node->mob->x_from = node->mob->x_to;
  node->mob->y_from = node->mob->y_to;
  node->x_pos = node->mob->x_to;
  node->y_pos = node->mob->y_to;
  node->mobile = 1;


  if (omg_param_list[node->type].rwp_type == RESTIRICTED_RWP) {
    do {
      loc_num =
        (int) randomgen (0, omg_param_list[node->type].max_vertices);
      node->mob->x_to =
        (double) vertice_xpos (loc_num, omg_param_list[node->type]);
      node->mob->y_to =
        (double) vertice_ypos (loc_num, omg_param_list[node->type]);


    } while (node->mob->x_to == node->mob->x_from
             && node->mob->y_to == node->mob->y_from);

    distance = fabs (node->mob->x_to - node->mob->x_from) +
               fabs (node->mob->y_to - node->mob->y_from);

    /* LOG_D(OMG,"#location number %d x pos to %.2f y pos to  %.2f x pos from %.2f y pos from  %.2f\n\n",loc_num,node->mob->x_to,node->mob->y_to,node->mob->x_from,node->mob->y_from); */

  } else if (omg_param_list[node->type].rwp_type == CONNECTED_DOMAIN) {
    pr = randomgen (0, 1);

    if (pr <= 0.50)
      /*node->block_num =
         (int) randomgen (0, omg_param_list[node->type].max_block_num); */
      node->block_num =
        (int) next_block (node->block_num, omg_param_list[node->type]);

    block_xmin = area_minx (node->block_num, omg_param_list[node->type]);
    block_ymin = area_miny (node->block_num, omg_param_list[node->type]);

    node->mob->x_to =
      (double) ((int) (randomgen (block_xmin, xloc_div + block_xmin) * 100))
      / 100;

    node->mob->y_to =
      (double) ((int) (randomgen (block_ymin, yloc_div + block_ymin) * 100))
      / 100;

    distance =
      (double) ((int) (sqrtf
                       (pow (node->mob->x_from - node->mob->x_to, 2) +
                        pow (node->mob->y_from - node->mob->y_to,
                             2)) * 100)) / 100;

    node->mob->azimuth = atan2 (node->mob->y_to - node->mob->y_from, node->mob->x_to - node->mob->x_from);  //radian
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
      (double) ((int) (sqrtf (pow (node->mob->x_from - node->mob->x_to, 2) +
                              pow (node->mob->y_from - node->mob->y_to,
                                   2)) * 100)) / 100;

    node->mob->azimuth = atan2 (node->mob->y_to - node->mob->y_from, node->mob->x_to - node->mob->x_from);  //radian
  }


  node->mob->speed =
    (double) ((int)
              (randomgen
               (omg_param_list[node->type].min_speed,
                omg_param_list[node->type].max_speed) * 100)) / 100;
  journeytime_next = (double) ((int) (distance / node->mob->speed * 100)) / 100;  //duration to get to dest

  /* LOG_D (OMG, "#[RWP] %d mob->journey_time_next %.2f distance %.2f speed %.2f\n \n",node->id,    journeytime_next,   distance,node->mob->speed);
   */
  node->mob->start_journey = cur_time;
  //LOG_D (OMG, "[RWP] start_journey %.2f\n", node->mob->start_journey);
  pair->next_event_t = cur_time + journeytime_next; //when to reach the destination
  // LOG_D (OMG,
  // "[RWP] reaching the destination at time : start journey + journey_time next =%.2f\n",
  // pair->a);
  /*
    if (node->event_num < 100)
    {
      event_sum[node->event_num] += node->mob->speed;
      events[node->event_num]++;
      node->event_num++;
    }
   */
}


/*update RWP nodes position*/

void
update_rwp_nodes (double cur_time)
{

  int done = 0;
  job_list *tmp = job_vector[RWP];
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
  job_vector[RWP] = quick_sort (job_vector[RWP]); ///////////
  // LOG_D (OMG, "--------DISPLAY JOB LIST AFTER SORTING--------\n");
  //display_job_list (job_vector[RWP]);
}


void
get_rwp_positions_updated (double cur_time)
{

  double x_now = 0.0, y_now = 0.0;
  double len, dx, dy;
  job_list *tmp = job_vector[RWP];

  while (tmp != NULL) {

    if (tmp->pair->b->mobile == 1 && tmp->pair->next_event_t >= cur_time) {

      if (omg_param_list[tmp->pair->b->type].rwp_type == RESTIRICTED_RWP) {
        len =
          tmp->pair->b->mob->speed * (cur_time -
                                      tmp->pair->b->mob->start_journey);
        dx = fabs (tmp->pair->b->mob->x_from - tmp->pair->b->mob->x_to);

        if (dx < len) {
          tmp->pair->b->x_pos = tmp->pair->b->mob->x_to;
          tmp->pair->b->y_pos =
            tmp->pair->b->mob->y_from + (len -
                                         dx) *
            ((tmp->pair->b->mob->y_to -
              tmp->pair->b->mob->y_from) /
             fabs (tmp->pair->b->mob->y_to -
                   tmp->pair->b->mob->y_from));
        } else {
          tmp->pair->b->y_pos = tmp->pair->b->mob->y_from;
          tmp->pair->b->x_pos = tmp->pair->b->mob->x_from + len *
                                ((tmp->pair->b->mob->x_to - tmp->pair->b->mob->x_from)
                                 / fabs (tmp->pair->b->mob->x_to -
                                         tmp->pair->b->mob->x_from));
        }

      }

      else {
        len =
          tmp->pair->b->mob->speed * (cur_time -
                                      tmp->pair->b->mob->start_journey);


        dx = len * cos (tmp->pair->b->mob->azimuth);
        dy = len * sin (tmp->pair->b->mob->azimuth);

        x_now = tmp->pair->b->mob->x_from + dx;
        y_now = tmp->pair->b->mob->y_from + dy;

        tmp->pair->b->x_pos = (double) ((int) (x_now * 100)) / 100;
        tmp->pair->b->y_pos = (double) ((int) (y_now * 100)) / 100;



      }     //grid


    } else {
      tmp->pair->b->x_pos = tmp->pair->b->mob->x_to;
      tmp->pair->b->y_pos = tmp->pair->b->mob->y_to;
    }

    /*if(tmp->pair->b->id==2 && ((int)(cur_time*1000)% 100)==0)
       LOG_D (OMG, "# %d  %.2f %.2f \n\n",tmp->pair->b->id,tmp->pair->b->x_pos,tmp->pair->b->y_pos); */
    tmp = tmp->next;

  }

}
