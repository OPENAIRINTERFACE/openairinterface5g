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

/*! \file sumo.c
* \brief The OMG interface to SUMO (Simulation of Urban Mobility)
* \author  S. Uppoor, J. Harri
* \date 2012
* \version 0.1
* \company INRIA, Eurecom
* \email: sandesh.uppor@inria.fr, haerri@eurecom.fr
* \note
* \warning
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>
#include <math.h>
#include <string.h>
#include <signal.h>

#include "sumo.h"

#include "client_traci_OMG.h"
#include "assertions.h"


int
start_sumo_generator (omg_global_param omg_param_list)
{

  char sumo_line[300];
  int n_id;
  static int id = 0;

  // sprintf(sumo_line, "%s -c %s  -b %d -e %d --remote-port %d --step-length %d -v ",omg_param_list.sumo_command, omg_param_list.sumo_config, omg_param_list.sumo_start, omg_param_list.sumo_end, omg_param_list.sumo_port, omg_param_list.sumo_step);

  sprintf (sumo_line, "%s -c %s ", omg_param_list.sumo_command,
           omg_param_list.sumo_config);

  printf ("%s\n", sumo_line);

  if ((pid = fork ()) == 0) {
    // Start SUMO in the child process
    if (system (sumo_line)) ;  /* this if for no gcc warnings */
    //childs addresss space
  }

  //still in the parent process

  // Talk to SUMO

  targetTime = 1;

  departed = NULL;
  arrived = NULL;

  // switch on error to return to OAI
  handshake (omg_param_list.sumo_host, omg_param_list.sumo_port);

  init (omg_param_list.sumo_end - omg_param_list.sumo_start);

  int max_node_SUMO = 100;  //commandGetMaxSUMONodesVariable(); TODO method not implemented in TraCI server..must find another solution

  // printf("received Number of nodes %d\n", max_node_SUMO);

  // create the OAI/SUMO ID manager
  id_manager = create_IDManager ();

  node_struct *node = NULL;
  mobility_struct *mobility = NULL;

  active_nodes = NULL;    // container to return a subset of only ACTIVE OAI nodes in SUMO

  last_update_time = 0.0;

  // just check for faulty values
  if (omg_param_list.nodes <= 0) {
#ifdef STANDALONE
    printf ("Number of nodes has not been set\n");
#else
    LOG_W (OMG, "Number of nodes has not been set\n");
#endif
    return (-1);
  }

#ifdef STANDALONE
  printf ("Number of OAI-equipped nodes in SUMO has been set to %d\n",
          omg_param_list.nodes);
  printf ("Number of SUMO simulated nodes has been set to %d\n",
          max_node_SUMO);
#else
  LOG_I (OMG, "Number of OAI-equipped nodes in SUMO has been set to %d\n",
         omg_param_list.nodes);
  LOG_I (OMG, "Number of SUMO simulated nodes has been set to %d\n",
         max_node_SUMO);
#endif

  // check and match number of nodes in mobility file provided
  if (omg_param_list.nodes > max_node_SUMO) {
#ifdef STANDALONE
    printf ("Not all OAI nodes will be moving according to SUMO.\n");
#else
    LOG_I (OMG, "Not all OAI nodes will be moving according to SUMO.\n");
#endif
  } else {
#ifdef STANDALONE
    printf ("OAI nodes will be mapped to a subset of SUMO nodes\n");
#else
    LOG_I (OMG, "OAI nodes will be mapped to a subset of SUMO nodes\n");
#endif
  }

  if (omg_param_list.nodes_type == eNB) {
#ifdef STANDALONE
    printf ("Node type has been set to eNB\n");
#else
    LOG_I (OMG, "Node type has been set to eNB\n");
#endif
  } else if (omg_param_list.nodes_type == UE) {

#ifdef STANDALONE
    printf ("Node type has been set to UE\n");
#else
    LOG_I (OMG, "Node type has been set to UE\n");
#endif
  }

  for (n_id = id; n_id < omg_param_list.nodes + id; n_id++) {

    node = create_node ();
    mobility = create_mobility ();
    node->mobile = 0;   // 0 means inactive in SUMO; 1 means active in SUMO; as long as a mapping between OAI-SUMO has not been created, nodes are inactive and do not move
    node->id = n_id;    // this is OAI ID, not SUMO
    node->type = omg_param_list.nodes_type; // UE eNB...
    node->generator = SUMO; // SUMO
    node->mob = mobility;

    // FIXME! wrong use of node_vector and node_vector_end
    // FIXME node_vector_end is declared with MAX_NUM_NODE_TYPES elements, but here we are indexing with SUMO from enum mobility_types
    // FIXME node_vector is declared with MAX_NUM_NODE_TYPES elements, but here we are indexing with SUMO from enum mobility_types

    node_vector_end[SUMO] =
      (node_list *) add_entry (node, node_vector_end[SUMO]);

    if (node_vector[SUMO] == NULL)
      node_vector[SUMO] = node_vector_end[SUMO];
  }

  id += omg_param_list.nodes;

  update_IDs ();    // update the mapping between departed and arrived nodes in SUMO.

  return (0);
}

void
update_IDs (void)
{
#ifdef STANDALONE
  printf ("Updating the ID mapping between SUMO and OAI\n");
#else
  LOG_D (OMG, "Updating the ID mapping between SUMO and OAI\n");
#endif
  string_list *tmp_arrived, *tmp_departed;

  if (arrived == NULL) {
    printf ("arrived vehicles is NULL \n");
    return;
  }

  tmp_arrived = arrived;
  tmp_departed = departed;

  if (departed == NULL) {
    printf ("departed vehicles is NULL \n");
    return;
  }

  if (tmp_departed->string != NULL) {
    // char *tmp_string = malloc (sizeof (strlen (tmp_departed->string)));
    char *tmp_string = malloc( strlen(tmp_departed->string) + 1 );
    strcpy (tmp_string, tmp_departed->string);
    //printf("OMG - 2 head is not null and value is: %s\n",tmp_string);
    int OAI_ID = get_oaiID_by_SUMO (tmp_string, id_manager);

    if (OAI_ID == -1) {
      if (!activate_and_map (tmp_string)) {
        // printf("Reached the Maximum of OAI nodes to be mapped to SUMO\n");
        // LOG_I(OMG, "Reached the Maximum of OAI nodes to be mapped to SUMO\n");
        free( tmp_string );
        return;   // stopping mapping as the maximum of OAI nodes has been reached;
      }

    }

    free( tmp_string );
  }

  while (tmp_departed->next != NULL) {
    // printf("OMG - 2 main is not null \n");
    //char tmp_string [strlen(tmp_departed->string)];
    //char *tmp_string = malloc (sizeof (strlen (tmp_departed->string)));
    char *tmp_string = malloc( strlen(tmp_departed->string) + 1 );
    strcpy (tmp_string, tmp_departed->string);
    //char *tmp_string = tmp_departed->string;
    int OAI_ID = get_oaiID_by_SUMO (tmp_string, id_manager);

    if (OAI_ID == -1) {
      if (!activate_and_map (tmp_string)) {
        //printf("Reached the Maximum of OAI nodes to be mapped to SUMO\n");
        //LOG_I(OMG, "Reached the Maximum of OAI nodes to be mapped to SUMO\n");
        free( tmp_string );
        return;   // stopping mapping as the maximum of OAI nodes has been reached;
      }
    }

    free( tmp_string );
    tmp_departed = tmp_departed->next;
  }

  departed = clear_string_list (departed);

  if (tmp_arrived->string != NULL) {
    char *tmp_string = tmp_arrived->string;
    //printf("OMG - 3 head is not null and value is: %s\n",tmp_arrived->string);

    if (!desactivate_and_unmap (tmp_string)) {
      printf ("Could not locate the OAI node ID %s \n", tmp_string);
      //LOG_I(OMG, "Could not locate the OAI node ID %s \n", tmp_string);
    }

  }

  while (tmp_arrived->next != NULL) {
    char *tmp_string = tmp_arrived->string;

    if (!desactivate_and_unmap (tmp_string)) {
      printf ("Could not locate the OAI node\n");
      // LOG_I(OMG, "Could not locate the OAI node\n");
    }

    tmp_arrived = tmp_arrived->next;
  }

  arrived = clear_string_list (arrived);

}

bool
desactivate_and_unmap (char *sumo_id)
{
#ifdef STANDALONE
  printf ("desactivating node %s \n", sumo_id);
#else
  LOG_I (OMG, "desactivating node %s \n", sumo_id);
#endif
  int OAI_ID = get_oaiID_by_SUMO (sumo_id, id_manager);
  remove_oaiID_by_SUMO (sumo_id, id_manager);

  if (OAI_ID != -1) {
    //TODO generalize to UE and eNB (must change the method)

    // FIXME! wrong use of node_vector
    // FIXME node_vector is declared with MAX_NUM_NODE_TYPES elements, but here we are indexing with SUMO from enum mobility_types
    node_struct *node = find_node (node_vector[SUMO], OAI_ID, UE);

    if (node == NULL)
      node = find_node (node_vector[SUMO], OAI_ID, eNB);

    if (node != NULL) {
      node->mobile = 0; // this node is now inactive;
      active_nodes = remove_node_entry (node, active_nodes);
      return true;
    }

  }

#ifdef STANDALONE
  printf
  ("Could not desactive an OAI node, as the SUMO-OAI mapping could not be found for the SUMO node ID %s \n",
   sumo_id);
#else
  LOG_I (OMG,
         "Could not desactive an OAI node, as the SUMO-OAI mapping could not be found for the SUMO node ID %s \n",
         sumo_id);
#endif

  return false;
}


bool
activate_and_map (char *sumo_id)
{
  MapPtr map = create_map ();
#ifdef STANDALONE
  printf ("activating node %s \n", sumo_id);
#else
  LOG_I (OMG, "activating node %s \n", sumo_id);
#endif
  // TODO: So far, only UE can be SUMO mobile, but could change
  AssertFatal( false, "node_vector array has only MAX_NUM_NODE_TYPES (=3) elements, but indexing here with SUMO (=4)");
  node_struct *active_node = get_first_inactive_OAI_node (node_vector[SUMO], UE);

  if (active_node != NULL) {
    // found an inactive OAI node; will be mapped to SUMO
    active_node->mobile = 1;  // now node is active in SUMO

    active_nodes = add_entry (active_node, active_nodes);

    map->oai_id = active_node->id;
    // map->sumo_id = malloc (sizeof ((int) strlen (sumo_id)));
    map->sumo_id = malloc( strlen(sumo_id) + 1 );
    strcpy (map->sumo_id, sumo_id);

#ifdef STANDALONE
    printf ("added a mapping between oai ID:  %d and SUMO ID: %s \n",
            map->oai_id, map->sumo_id);
#else
    LOG_I (OMG, "added a mapping between oai ID:  %d and SUMO ID: %s \n",
           map->oai_id, map->sumo_id);
#endif

    // TODO fusion the two lists...leads to data inconsistency
    // FIXME adding the same memory region to two lists => crash at free() time
    id_manager->map_sumo2oai =
      add_map_entry (map, id_manager->map_sumo2oai);

    id_manager->map_oai2sumo = add_map_entry (map, id_manager->map_oai2sumo); //map_sumo2oai

    return true;

  }

  else {
#ifdef STANDALONE
    printf
    ("All OAI Nodes are already active in SUMO; cannot control this SUMO node in OAI\n");
#else
    LOG_I (OMG,
           "All OAI Nodes are already active in SUMO; cannot control this SUMO node in OAI\n");
#endif
    return false;
  }
}


void
update_sumo_nodes (double cur_time)
{
  if ((cur_time - last_update_time) < MIN_SUMO_STEP) {
    // the min time interval for SUMO must be 100ms or more
    printf ("--------Update Step too small--------\n");
    return;
  }

  last_update_time = cur_time;  // keeps track of the last update time to get the update interval

  // commandSimulationStep(1.0);// Advance the SUMO simulation by cur_time units


  commandSimulationStep (cur_time); // Advance the SUMO simulation by cur_time units

  LOG_I (OMG, "--------Updated SUMO positions by %f [ms]--------\n",
         cur_time);
  update_IDs ();    // both are in the  traCI client


}

void
update_sumo_positions (node_struct * node)
{



#ifdef STANDALONE
  printf ("--------GET SUMO Mobility for a single node--------\n");
#else
  LOG_D (OMG, "--------GET SUMO Mobility for a single node--------\n");
#endif

  Map_list tmp = id_manager->map_oai2sumo;
  char *sumo_id = get_sumo_entry (node->id, tmp);

  if (sumo_id != NULL) {
    //printf(" OAI ID is: %d and SUMO ID is %s \n",node->ID, sumo_id);
    GetPosition (node, sumo_id);
    GetSpeed (node, sumo_id);
  }
}

node_list *
get_sumo_positions_updated (double cur_time)
{


#ifdef STANDALONE
  printf
  ("--------GET SUMO Mobility for a group of ACTIVE OAI nodes--------\n");
#else
  LOG_I (OMG,
         "--------GET SUMO Mobility for a group of ACTIVE OAI nodes--------\n");
#endif

  // FIXME! wrong use of node_vector
  // FIXME node_vector is declared with MAX_NUM_NODE_TYPES elements, but here we are indexing with SUMO from enum mobility_types
  if (node_vector[SUMO] != NULL) {
    node_list *tmp = node_vector[SUMO];

    while (tmp != NULL) {
      if ((tmp->node->generator == SUMO) && (tmp->node->mobile == 1)) {
        // OAI node MUST be active
        LOG_I (OMG, "found an active node with id %d \n",
               tmp->node->id);
        LOG_I (OMG, "Old Positions \n");
        //printf("Old Positions \n");
        display_node_position (tmp->node->id, tmp->node->generator,
                               tmp->node->type, tmp->node->mobile,
                               tmp->node->x_pos, tmp->node->y_pos);

        update_sumo_positions (tmp->node);

        //printf("New Positions \n");
        LOG_I (OMG, "New Positions \n");
        display_node_position (tmp->node->id, tmp->node->generator,
                               tmp->node->type, tmp->node->mobile,
                               tmp->node->x_pos, tmp->node->y_pos);
      }

      tmp = tmp->next;
    }
  }

  return active_nodes;
}

node_struct *
get_first_inactive_OAI_node (node_list * list, int node_type)
{
  node_list *current;

  if (list != NULL) {
    //start search
    current = list;

    while (current->next != NULL) {
      if ((current->node->mobile == 0)
          && (current->node->type == node_type)) {
        return current->node;
      }

      current = current->next;
    }
  }

  return NULL;      // all nodes are active already..reached the maximum number of OAI nodes
}

bool
stop_sumo_generator (void)
{
#ifdef STANDALONE
  printf
  (" --------Destructor for SUMO: closing the TraCI socket and killing SUMO ---- \n");
#else
  LOG_I (OMG,
         " --------Destructor for SUMO: closing the TraCI socket and killing SUMO ---- \n");
#endif
  commandClose ();    // closing the connection with SUMO via TraCI
  close_connection ();    // closing the socket connection
  kill (pid, SIGKILL);    // killing SUMO in case it could not close by itself
  return true;
}
