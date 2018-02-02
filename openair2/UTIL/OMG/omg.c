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

/*! \file OMG.c
* \brief Main function containing the OMG API to oaisim or any other simulator/emulator
* \author S. Gashaw, N. Nikaein, J. Harri
* \date 2011 - 2014
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/


#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <math.h>
#include <time.h>
#include "omg.h"
#include "static.h"
#include "rwp.h"
#include "rwalk.h"
#include "trace.h"
#include "grid.h"
#include "steadystaterwp.h"
#ifdef SUMO_IF
#include "sumo.h"
#endif 
#include "../OMV/structures.h"
//#define STANDALONE

float n_frames = 20.0;

int omv_write (int pfd, node_list * ue_node_list, Data_Flow_Unit omv_data);
void omv_end (int pfd, Data_Flow_Unit omv_data);
int omv_enabled;

/*initialze global parameters*/
void
init_omg_global_params (void)
{
  int mob_t, node_t, i;
  xloc_div = 10.0;
  yloc_div = 10.0;
  grid = 0;

  for (mob_t = STATIC; mob_t < MAX_NUM_MOB_TYPES; mob_t++) {
    job_vector_len[mob_t] = 0;
    job_vector[mob_t] = NULL;
    job_vector_end[mob_t] = NULL;
  }

  for (node_t = eNB; node_t < MAX_NUM_NODE_TYPES; node_t++) {
    node_vector[node_t] = NULL;
    node_vector_end[node_t] = NULL;
    node_vector_len[node_t] = 0;
  }

  for (i = 0; i < 100; i++) {
    event_sum[i] = 0;
    events[i] = 0;
  }
}

/*initiate mobility generator*/
void
init_mobility_generator (omg_global_param omg_param_list[])
{
  int node_t, mobility_t;

  for (node_t = eNB; node_t < MAX_NUM_NODE_TYPES; node_t++) {

    mobility_t = omg_param_list[node_t].mobility_type;

    switch (mobility_t) {

    case STATIC:
      start_static_generator (omg_param_list[node_t]);
      break;

    case RWP:
      start_rwp_generator (omg_param_list[node_t]);
      break;

    case RWALK:
      start_rwalk_generator (omg_param_list[node_t]);
      break;

    case TRACE:
      start_trace_generator (omg_param_list[node_t]);
      break;
#ifdef SUMO_IF
    case SUMO:
      start_sumo_generator (omg_param_list[node_t]);
      break;
#endif 
    case STEADY_RWP:
      start_steadystaterwp_generator (omg_param_list[node_t]);
      break;

    default:
      LOG_N (OMG, "Unsupported generator\n");
    }


  }

}

/**************************************************************************************/
/*stop sumo mobiity generator*/
void
stop_mobility_generator (omg_global_param * omg_param_list)
{
  int i;

  for (i = 0; i < MAX_NUM_NODE_TYPES; i++) {
    switch (omg_param_list[i].mobility_type) {

    case STATIC:
      break;

    case RWP:
      break;

    case RWALK:
      break;

    case TRACE:
      clear_list ();
      break;

    case STEADY_RWP:
      break;
#ifdef SUMO_IF
    case SUMO:
      stop_sumo_generator ();
      //LOG_D(OMG," --------OMG will interface with SUMO for mobility generation-------- \n");
      break;
#endif 
    default:
      LOG_N (OMG, "Unsupported generator\n");
    }
  }

}

/*****************************************************************************/

void
update_nodes (double cur_time)
{
  //LOG_D(OMG, "UPDATE NODES" );
  int i = 0;

  for (i = (STATIC + 1); i < MAX_NUM_MOB_TYPES; i++) {
    if (job_vector[i] != NULL) {
      update_node_vector (i, cur_time);
    }
  }

}


void
update_node_vector (int mobility_type, double cur_time)
{
  //set_time(cur_time);
  switch (mobility_type) {
  case RWP:
    update_rwp_nodes (cur_time);
    break;

  case RWALK:
    update_rwalk_nodes (cur_time);
    break;

  case TRACE:
    update_trace_nodes (cur_time);
    break;
#ifdef SUMO_IF
  case SUMO:
    // printf("in SUMO case \n");
    update_sumo_nodes (cur_time);
    break;
#endif 
  case STEADY_RWP:
    update_steadystaterwp_nodes (cur_time);
    break;

  default:
    //printf("STATIC or Unsupported generator %d \n", omg_param_list.mobility_type);
    LOG_N (OMG, "STATIC or Unsupported generator\n");
  }
}

/*********************************************************************************/

/*return updated node position for a given node type*/
node_list *
get_current_positions (int mobility_type, int node_type, double cur_time)
{


  get_nodes_positions (mobility_type, cur_time);
  return node_vector[node_type];

}

/*update current position of nodes for specific mobility type*/
void
get_nodes_positions (int mobility_type, double cur_time)
{
  //printf("%d \n",mobility_type);

  switch (mobility_type) {
  case STATIC:
    break;

  case RWP:
    get_rwp_positions_updated (cur_time);
    break;

  case RWALK:
    get_rwalk_positions_updated (cur_time);
    break;

  case TRACE:
    get_trace_positions_updated (cur_time);
    break;
#ifdef SUMO_IF
  case SUMO:
    LOG_I (OMG, "getting positions from SUMO\n");
    get_sumo_positions_updated (cur_time);
    break;
#endif 
  case STEADY_RWP:
    get_steadystaterwp_positions_updated (cur_time);
    break;

  default:
    LOG_E (OMG, " Unsupported generator \n");
  }

}

/***************************************************************/

// get the position for a specific node

node_struct *
get_node_position (int node_type, int nid)
{
  node_list *tmp;

  tmp = node_vector[node_type];

  while (tmp != NULL) {
    if (tmp->node->id == nid)
      return tmp->node;

    tmp = tmp->next;
  }

  return NULL;
}


/*set new mobility for a node*/
void
set_new_mob_type (int id, int node_t, int mob_t, double cur_time)
{

  int prev_mob;
  node_list *tmp;
  //job_list *tmp2, *prev_job;
  pair_struct *pair;
  double pause_p;

  //find previous mobility type
  tmp = node_vector[node_t];

  while (tmp != NULL) {
    if (tmp->node->id == id && tmp->node->type == node_t) {
      prev_mob = tmp->node->generator;
      break;
    }

    tmp = tmp->next;
  }

  //end

  if (tmp != NULL && prev_mob != mob_t) {

    //initialize node position
    if (mob_t == STATIC || mob_t == RWP || mob_t == RWALK || mob_t == STEADY_RWP) {
      tmp->node->x_pos =
        (double) ((int)
                  (randomgen
                   (omg_param_list[node_t].min_x,
                    omg_param_list[node_t].max_x) * 100)) / 100;
      tmp->node->mob->x_from = tmp->node->x_pos;
      tmp->node->mob->x_to = tmp->node->x_pos;
      tmp->node->y_pos =
        (double) ((int)
                  (randomgen
                   (omg_param_list[node_t].min_y,
                    omg_param_list[node_t].max_y) * 100)) / 100;
      tmp->node->mob->y_from = tmp->node->y_pos;
      tmp->node->mob->y_to = tmp->node->y_pos;
      tmp->node->mob->speed = 0.0;
      tmp->node->generator = mob_t;

      if (prev_mob != STATIC)
        job_vector[prev_mob] =
          remove_job (job_vector[prev_mob], id, node_t);
    }

    //end

    switch (mob_t) {

    case STATIC:

      break;

    case RWP:
      pair = (pair_struct *) malloc (sizeof (struct pair_struct));
      pair->b = tmp->node;
      sleep_rwp_node (pair, cur_time);
      job_vector[RWP] = addjob (pair, job_vector[RWP]);
      break;

    case RWALK:
      pair = (pair_struct *) malloc (sizeof (struct pair_struct));
      pair->b = tmp->node;
      sleep_rwalk_node (pair, cur_time);
      job_vector[RWALK] = addjob (pair, job_vector[RWALK]);
      break;

    case STEADY_RWP:
      tmp->node->event_num = 0;
      pair = (pair_struct *) malloc (sizeof (struct pair_struct));
      pair->b = tmp->node;
      pause_p = pause_probability (omg_param_list[node_t]);

      if (randomgen (0, 1) < pause_p)
        sleep_steadystaterwp_node (pair, cur_time);
      else
        move_steadystaterwp_node (pair, cur_time);

      break;
#ifdef SUMO_IF
    case SUMO:
      LOG_E (OMG, "not possible to change mobility type to sumo \n");
      break;
#endif
    case TRACE:
      LOG_E (OMG, "not possible to change mobility type to trace \n");
      break;

    default:
      LOG_E (OMG, " Unsupported generator \n");
    }

  }


}

#ifdef STANDALONE

/************************** get options ************************************/
void
usage (void)
{
  fprintf (stderr,
           "\n\t-X: assign maximum width of the simulation area for UE nodes(X_max)"
           "\n\t-x: assign minimum width of the simulation area for UE nodes(X_min)"
           "\n\t-C: assign maximum width of the simulation area for eNB nodes(X_max)"
           "\n\t-c: assign minimum width of the simulation area for eNB nodes(X_min)"
           "\n\t-B: assign maximum width of the simulation area for relay nodes(X_max)"
           "\n\t-b: assign minimum width of the simulation area for relay nodes(X_min)"
           "\n\t-Y: assign maximum height of the simulation area for UE nodes(Y_max)"
           "\n\t-y: assign minimum height of the simulation area for UE nodes (Y_min)"
           "\n\t-Z: assign maximum height of the simulation area for eNB nodes(Y_max)"
           "\n\t-z: assign minimum height of the simulation area for eNB nodes(Y_min)"
           "\n\t-W: assign maximum height of the simulation area for relay nodes(Y_max)"
           "\n\t-w: assign minimum height of the simulation area for relay nodes(Y_min)"
           //"\n\t-N: assign number of nodes" "\n\t-n: assign number of frames"
           "\n\t-S: assign maximum duration of sleep/pause time for UE nodes(max_break)"
           "\n\t-s: assign minimum duration of sleep/pause time for UE nodes(min_break)"
           "\n\t-L: assign maximum duration of sleep/pause time for eNB nodes(max_break)"
           "\n\t-l: assign minimum duration of sleep/pause time for eNB nodes(min_break)"
           "\n\t-P: assign maximum duration of sleep/pause time for relay nodes(max_break)"
           "\n\t-p: assign minimum duration of sleep/pause time for relay nodes(min_break)"
           "\n\t-J: assign maximum duration of journey for UE nodes(max_journey_time)"
           "\n\t-j: assign minimum duration of journey for UE nodes(min_journey_time)"
           "\n\t-J: assign maximum duration of journey for eNB nodes(max_journey_time)"
           "\n\t-j: assign minimum duration of journey for eNB nodes(min_journey_time)"
           "\n\t-J: assign maximum duration of journey for relay nodes(max_journey_time)"
           "\n\t-j: assign minimum duration of journey for relay nodes(min_journey_time)"
           "\n\t-D: assign maximum speed for UE nodes"
           "\n\t-d: assign minimum speed for UE nodes"
           "\n\t-I: assign maximum speed for eNB nodes"
           "\n\t-i: assign minimum speed for eNB nodes"
           "\n\t-A: assign maximum speed for relay nodes"
           "\n\t-a: assign minimum speed for relay nodes"
           //"\n\t-g: choose generator(STATIC: 0x00, RWP: 0x01, RWALK: 0x02,TRACE: 0x03 or SUMO:0x04, STEADY_RWP:0x05)\n"
           "\n\t-g: choose to use map for random mobility" "\n\t-h: help"
           "\n\t-U: specifiy number of UE nodes"
           "\n\t-E: specifiy number of eNB nodes"
           "\n\t-R: specifiy number of relay nodes"
           "\n\t-u: choose mobility generator for UE nodes"
           "\n\t-e: choose mobility generator for eNB nodes"
           "\n\t-r: choose mobility generator for relay nodes"
           "\n\t-f: choose seed \n");
  exit (0);
}



int
get_options (int argc, char *argv[])
{
  int node_t;
  char tag;

  while ((tag =
            getopt (argc, argv,
                    "U:u:E:e:R:r:A:a:B:b:C:c:D:d:f:g:hI:i:J:j:k:L:l:N:n:P:p:S:s:T:t:vW:w:X:x:Y:y:Z:z:"))
         != EOF) {


    switch (tag) {

    case 'U':
      if (atoi (optarg) < 0) {
        usage ();
        exit (1);
      }

      omg_param_list[UE].nodes = atoi (optarg);
      LOG_D (OMG, "#Number of UE nodes : %d \n",
             omg_param_list[UE].nodes);
      break;

    case 'E':
      if (atoi (optarg) < 0) {
        usage ();
        exit (1);
      }

      omg_param_list[eNB].nodes = atoi (optarg);
      LOG_D (OMG, "#Number of eNB nodes : %d \n",
             omg_param_list[eNB].nodes);
      break;

    case 'R':
      if (atoi (optarg) < 0) {
        usage ();
        exit (1);
      }

      omg_param_list[RELAY].nodes = atoi (optarg);
      LOG_D (OMG, "#Number of relay nodes : %d \n",
             omg_param_list[RELAY].nodes);
      break;

    case 'k':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#Number of frames can not be negative \n");

      n_frames = abs (atof (optarg));
      LOG_D (OMG, "#Number of frames : %f \n", n_frames);
      break;

    case 'u':
      if (atoi (optarg) < 0 || atoi (optarg) >= MAX_NUM_MOB_TYPES) {
        usage ();
        exit (1);
      }

      omg_param_list[UE].mobility_type = atoi (optarg);
      LOG_D (OMG, "#UE nodes mobility type: %d \n",
             omg_param_list[UE].mobility_type);
      break;

    case 'e':
      if (atoi (optarg) < 0 || atoi (optarg) >= MAX_NUM_MOB_TYPES) {
        usage ();
        exit (1);
      }

      omg_param_list[eNB].mobility_type = atoi (optarg);
      LOG_D (OMG, "#eNB nodes mobility type: %d \n",
             omg_param_list[eNB].mobility_type);
      break;

    case 'r':
      if (atoi (optarg) < 0 || atoi (optarg) >= MAX_NUM_MOB_TYPES) {
        usage ();
        exit (1);
      }

      omg_param_list[RELAY].mobility_type = atoi (optarg);
      LOG_D (OMG, "#relay nodes mobility type: %d \n",
             omg_param_list[RELAY].mobility_type);
      break;

    case 's':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#min sleep can not be negative \n");

      omg_param_list[UE].min_sleep = fabs (atof (optarg));
      LOG_D (OMG, "#UE min sleep is set to: %.2f \n",
             omg_param_list[UE].min_sleep);
      break;

    case 'S':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#max_sleep can not be negative \n");

      omg_param_list[UE].max_sleep = fabs (atof (optarg));
      LOG_D (OMG, "#UE max_sleep is set to : %.2f \n",
             omg_param_list[UE].max_sleep);
      break;

    case 'l':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#min sleep can not be negative or zero \n");

      if (atof (optarg) != 0)
        omg_param_list[eNB].min_sleep = fabs (atof (optarg));

      LOG_D (OMG, "#eNB min sleep is set to : %.2f \n",
             omg_param_list[eNB].min_sleep);
      break;

    case 'L':
      if (atof (optarg) <= 0)
        LOG_E (OMG, "#max_sleep can not be negative or zero \n");

      if (atof (optarg) != 0)
        omg_param_list[eNB].max_sleep = fabs (atof (optarg));

      LOG_D (OMG, "#eNB max_sleep is set to : %.2f \n",
             omg_param_list[eNB].max_sleep);
      break;

    case 'p':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#min sleep can not be negative \n");

      omg_param_list[RELAY].min_sleep = fabs (atof (optarg));
      LOG_D (OMG, "#relay min sleep is set to: %.2f \n",
             omg_param_list[RELAY].min_sleep);
      break;

    case 'P':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#max_sleep can not be negative \n");

      omg_param_list[RELAY].max_sleep = fabs (atof (optarg));
      LOG_D (OMG, "#relay max_sleep is set to : %.2f \n",
             omg_param_list[RELAY].max_sleep);
      break;

    case 'd':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#min_speed  can not be negative \n");

      if (atof (optarg) != 0)
        omg_param_list[UE].min_speed = fabs (atof (optarg));

      LOG_D (OMG, "#UE min_speed is set to: %.2f \n",
             omg_param_list[UE].min_speed);
      break;

    case 'D':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#max_speed  can not be negative \n");

      omg_param_list[UE].max_speed = fabs (atof (optarg));
      LOG_D (OMG, "#UE max_speed is set to: %.2f \n",
             omg_param_list[UE].max_speed);
      break;

    case 'i':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#min_speed  can not be negative \n");

      if (atof (optarg) != 0)
        omg_param_list[eNB].min_speed = fabs (atof (optarg));

      LOG_D (OMG, "#eNB min_speed is set to: %.2f \n",
             omg_param_list[eNB].min_speed);
      break;

    case 'I':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#max_speed  can not be negative \n");

      omg_param_list[eNB].max_speed = fabs (atof (optarg));
      LOG_D (OMG, "#eNB max_speed is set to : %.2f \n",
             omg_param_list[eNB].max_speed);
      break;

    case 'a':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#min_speed  can not be negative \n");

      if (atof (optarg) != 0)
        omg_param_list[RELAY].min_speed = fabs (atof (optarg));

      LOG_D (OMG, "#relay min_speed is set to : %.2f \n",
             omg_param_list[RELAY].min_speed);
      break;

    case 'A':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#max_speed  can not be negative \n");

      omg_param_list[RELAY].max_speed = fabs (atof (optarg));
      LOG_D (OMG, "#relay max_speed is set to: %.2f \n",
             omg_param_list[RELAY].max_speed);
      break;

    case 'v':
      omv_enabled = 1;
      break;

    case 'X':
      omg_param_list[UE].max_x = fabs (atof (optarg));
      LOG_D (OMG, "#UE X_max : %.2f \n", omg_param_list[UE].max_x);
      break;

    case 'x':
      omg_param_list[UE].min_x = fabs (atof (optarg));
      LOG_D (OMG, "#UE X_min : %.2f \n", omg_param_list[UE].min_x);
      break;

    case 'C':
      omg_param_list[eNB].max_x = fabs (atof (optarg));
      LOG_D (OMG, "#eNB X_max : %.2f \n", omg_param_list[eNB].max_x);
      break;

    case 'c':
      omg_param_list[eNB].min_x = fabs (atof (optarg));
      LOG_D (OMG, "#eNB X_min : %.2f \n", omg_param_list[eNB].min_x);
      break;

    case 'B':
      omg_param_list[RELAY].max_x = fabs (atof (optarg));
      LOG_D (OMG, "#relay X_max : %.2f \n", omg_param_list[RELAY].max_x);
      break;

    case 'b':
      omg_param_list[RELAY].min_x = fabs (atof (optarg));
      LOG_D (OMG, "#relay X_min : %.2f \n", omg_param_list[RELAY].min_x);
      break;

    case 'Y':
      omg_param_list[UE].max_y = fabs (atof (optarg));
      LOG_D (OMG, "#UE Y_max : %.2f \n", omg_param_list[UE].max_y);
      break;

    case 'y':
      omg_param_list[UE].min_y = fabs (atof (optarg));
      LOG_D (OMG, "#UE Y_min : %.2f \n", omg_param_list[UE].min_y);
      break;

    case 'Z':
      omg_param_list[eNB].max_y = fabs (atof (optarg));
      LOG_D (OMG, "#eNB Y_max : %.2f \n", omg_param_list[eNB].max_y);
      break;

    case 'z':
      omg_param_list[eNB].min_y = fabs (atof (optarg));
      LOG_D (OMG, "#eNB Y_min : %.2f \n", omg_param_list[eNB].min_y);
      break;

    case 'W':
      omg_param_list[RELAY].max_y = fabs (atof (optarg));
      LOG_D (OMG, "#relay Y_max : %.2f \n", omg_param_list[RELAY].max_y);
      break;

    case 'w':
      omg_param_list[RELAY].min_y = fabs (atof (optarg));
      LOG_D (OMG, "#relay Y_min : %.2f \n", omg_param_list[RELAY].min_y);
      break;

    case 'J':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#Journey_time_max can not be negative \n");

      omg_param_list[UE].max_journey_time = fabs (atof (optarg));
      LOG_D (OMG, "UE Journey_time_max : %.2f \n",
             omg_param_list[UE].max_journey_time);
      break;

    case 'j':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#Journey_time_min can not be negative \n");

      omg_param_list[UE].min_journey_time = fabs (atof (optarg));
      LOG_D (OMG, "#UE Journey_time_min : %.2f \n",
             omg_param_list[UE].min_journey_time);
      break;

    case 'T':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#Journey_time_max can not be negative \n");

      omg_param_list[eNB].max_journey_time = fabs (atof (optarg));
      LOG_D (OMG, "#eNB Journey_time_max : %.2f \n",
             omg_param_list[eNB].max_journey_time);
      break;

    case 't':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#Journey_time_min can not be negative \n");

      omg_param_list[eNB].min_journey_time = fabs (atof (optarg));
      LOG_D (OMG, "#eNB Journey_time_min : %.2f \n",
             omg_param_list[eNB].min_journey_time);
      break;

    case 'N':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#Journey_time_max can not be negative \n");

      omg_param_list[RELAY].max_journey_time = fabs (atof (optarg));
      LOG_D (OMG, "#relay Journey_time_max : %.2f \n",
             omg_param_list[RELAY].max_journey_time);
      break;

    case 'n':
      if (atof (optarg) < 0)
        LOG_E (OMG, "#Journey_time_min can not be negative \n");

      omg_param_list[RELAY].min_journey_time = fabs (atof (optarg));
      LOG_D (OMG, "#relay Journey_time_min : %.2f \n",
             omg_param_list[RELAY].min_journey_time);
      break;

    case 'f':
      for (node_t = eNB; node_t < MAX_NUM_NODE_TYPES; node_t++) {
        omg_param_list[node_t].seed = atoi (optarg);
        LOG_D (OMG, "#Seed is %d \n", omg_param_list[node_t].seed);
      }

      break;

    case 'g':
      if (atoi (optarg) < 1 || atoi (optarg) > 2) {
        LOG_E (OMG, "#value given for graph should be 1 or 2 \n");
        exit (-1);
      }

      grid = abs (atof (optarg));
      LOG_D (OMG, "#graph value is %d \n", grid);
      break;

    case 'h':
      usage ();
      break;

    default:
      usage ();
      exit (1);
    }
  }

  return 1;
}

/**************************** main **********************************/

int
main (int argc, char *argv[])
{
  int i, node_type;
  double cur_time = 0.0;
  double ms = 0.0;
  node_list *current_positions = NULL;
  node_struct *my_node = NULL;
  int my_id = 0, time_s;
  double emu_info_time;
  char full_name[200];
  int pfd[2];     // fd for omv : fixme: this could be a local var
  char fdstr[10];
  char frames[10];
  char num_enb[10];
  char num_ue[10];
  //area_x, area_y and area_z for omv
  char x_area[20];
  char y_area[20];
  char z_area[20];
  char fname[64], vname[64];
  Data_Flow_Unit omv_data;

  //default parameters
  for (node_type = eNB; node_type < MAX_NUM_NODE_TYPES; node_type++) {


    if (node_type == UE)
      omg_param_list[node_type].nodes = 5;
    else
      omg_param_list[node_type].nodes = 0;

    omg_param_list[node_type].min_x = 0;
    omg_param_list[node_type].max_x = 100;
    omg_param_list[node_type].min_y = 0;
    omg_param_list[node_type].max_y = 100;
    omg_param_list[node_type].min_speed = 0.1;
    omg_param_list[node_type].max_speed = 20.0;
    omg_param_list[node_type].min_journey_time = 0.1;
    omg_param_list[node_type].max_journey_time = 20.0;
    omg_param_list[node_type].min_azimuth = 0;
    omg_param_list[node_type].max_azimuth = 360;
    omg_param_list[node_type].min_sleep = 0.1;
    omg_param_list[node_type].max_sleep = 5.0;
    omg_param_list[node_type].mobility_file = "TRACE/mobility_3ues.tr";
    omg_param_list[node_type].mobility_type = STATIC;
    omg_param_list[node_type].nodes_type = node_type;
    //omg_param_list.sumo_config = "SUMO/SCENARIOS/traci.koln.sumo.cfg";
    omg_param_list[node_type].sumo_command = "sumo-gui";
    omg_param_list[node_type].sumo_config =
      "SUMO/SCENARIOS/traci.scen.sumo.cfg";
    omg_param_list[node_type].sumo_start = 0; //25900;
    omg_param_list[node_type].sumo_end = 200; //26200;
    omg_param_list[node_type].sumo_step = 1;
    omg_param_list[node_type].sumo_host = "localhost";
    omg_param_list[node_type].sumo_port = 8890;
  }


  init_omg_global_params ();  //initialize global paramaters

  get_options (argc, argv); // overwrite the default params if any input parameter

  for (node_type = eNB; node_type < MAX_NUM_NODE_TYPES; node_type++) {
    omg_param_list[node_type].max_vertices =
      max_vertices_ongrid (omg_param_list[node_type]);

    omg_param_list[node_type].max_block_num =
      max_connecteddomains_ongrid (omg_param_list[node_type]);

  }



  init_mobility_generator (omg_param_list); //initialize choosen mobility generator

  // ****************init omv********************
  if (omv_enabled == 1) {
    if (pipe (pfd) == -1)
      perror ("pipe error \n");

    sprintf (full_name, "%s/UTIL/OMV/OMV", getenv ("OPENAIR2_DIR"));
    LOG_I (EMU, "Stating the OMV path %s pfd[0] %d pfd[1] %d \n", full_name,
           pfd[0], pfd[1]);





    switch (fork ()) {
    case -1:
      perror ("fork failed \n");
      break;

    case 0:
      if (close (pfd[1]) == -1)
        perror ("close on write\n");

      sprintf (fdstr, "%d", pfd[0]);
      sprintf (num_enb, "%d", 1);
      sprintf (num_ue, "%d", omg_param_list[UE].nodes);
      sprintf (x_area, "%f", omg_param_list[UE].max_x);
      sprintf (y_area, "%f", omg_param_list[UE].max_y);
      sprintf (z_area, "%f", 200.0);
      sprintf (frames, "%d", (int) n_frames);

      execl (full_name, "OMV", fdstr, frames, num_enb, num_ue, x_area,
             y_area, z_area, NULL);
      perror ("error in execl the OMV");
    }

    //parent
    if (close (pfd[0]) == -1)
      perror ("close on read\n");
  }

  //******************************

  for (emu_info_time = 0.0; emu_info_time <= n_frames; emu_info_time += 0.1) {

    update_nodes (emu_info_time);
    time_s = round (emu_info_time * 1000.0);

    //printf("%d \n",time_s);
    for (node_type = eNB; node_type < MAX_NUM_NODE_TYPES; node_type++) {
      current_positions =
        get_current_positions (omg_param_list[node_type].mobility_type,
                               node_type, emu_info_time);

      if (current_positions != NULL)

        display_job_list (emu_info_time,
                          job_vector[omg_param_list
                                     [node_type].mobility_type]);

      /*if(current_positions!= NULL && time_s %1000==0)
         LOG_D(OMG, "%.2f  %d\n",emu_info_time,
         nodes_avgspeed(job_vector[omg_param_list [node_type].mobility_type])); */
    }


    //display current postion of nodes
    if (omv_enabled == 1)
      omv_write (pfd[1], node_vector[SUMO], omv_data);


  }

  /*LOG_I(OMG,"#-----event average-----\n");
     for (i=0;i<100;i++)
     {
     if(events[i]!=0)
     LOG_D(OMG,"%d %d \n",i,event_sum[i]/events[i]);
     } */

  stop_mobility_generator (omg_param_list);

  if (omv_enabled == 1)
    omv_end (pfd[1], omv_data);

  //clear_mem();

  return 0;
}


/**********************************sumo****************************/
int
omv_write (int pfd, node_list * ue_node_list, Data_Flow_Unit omv_data)
{
  int i = 0, j;
  omv_data.end = 0;
  // enb
  omv_data.geo[i].x = 0.0;
  omv_data.geo[i].y = 0.0;
  omv_data.geo[i].z = 1.0;
  omv_data.geo[i].mobility_type = 0;
  omv_data.geo[i].node_type = 0;  //eNB
  omv_data.geo[i].Neighbors = 0;

  for (i = 1; i < omg_param_list[SUMO].nodes + 1; i++) {
    if (ue_node_list != NULL) {
      omv_data.geo[i].x =
        (int) (ue_node_list->node->x_pos <
               0.0) ? 0.0 : ue_node_list->node->x_pos;
      omv_data.geo[i].y =
        (int) (ue_node_list->node->y_pos <
               0.0) ? 0.0 : ue_node_list->node->y_pos;
      omv_data.geo[i].z = 1.0;
      omv_data.geo[i].mobility_type = omg_param_list[SUMO].mobility_type;
      omv_data.geo[i].node_type = 1;  //UE
      ue_node_list = ue_node_list->next;
      omv_data.geo[i].Neighbors = 0;
      printf ("node %d at (%d, %d)\n", i, omv_data.geo[i].x,
              omv_data.geo[i].y);
    }
  }

  if (write (pfd, &omv_data, sizeof (struct Data_Flow_Unit)) == -1)
    perror ("write omv failed");

  return 1;
}

void
omv_end (int pfd, Data_Flow_Unit omv_data)
{
  omv_data.end = 1;

  if (write (pfd, &omv_data, sizeof (struct Data_Flow_Unit)) == -1)
    perror ("write omv failed");
}
#endif
