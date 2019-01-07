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

/**
 * \file defs.h
 * \brief Typedefs & Prototypes of OMG functions
 */

#ifndef __DEFS_H__
#define  __DEFS_H__

#include <stdlib.h>

// TYPEDEFS

#define true 1
#define false 0

//typedef char bool;
#include <stdbool.h>

#ifdef STANDALONE
typedef struct {
  char *name; /*!< \brief string name of item */
  int value;  /*!< \brief integer value of mapping */
} mapping;
int  map_str_to_int(mapping *map, const char *str);
char *map_int_to_str(mapping *map, int val);
#endif

/****************************************************************************
!A sructure that includes all the characteristic mobility elements of a node
*****************************************************************************/
struct mobility_struct {
  double x_from; /*!< The X coordinate of the previous location of the node */
  double y_from; /*!< The Y coordinate of the previous location of the node */
  double x_to; /*!< The X coordinate of the destination location of the node */
  double y_to; /*!< The Y coordinate of the destination location of the node */
  double speed; /*!< The speed of the node */
  double sleep_duration; /*!< The sleep duration of the node (Used with the RWP model) */
  double azimuth; /*!< The direction in which the node moves (Used with RWALK model) */
  double journey_time; /*!< The duration of the node trip */
  double start_journey; /*!< The instant on which the node trip starts*/
  double target_time;/*!<The time instant before the node should complete>*/
  double target_speed;/*!<The time instant before the node should complete>*/
};

typedef struct mobility_struct mobility_struct; /*!< The typedef that reflects a #mobility_struct*/

/******************************************************************************
!A sructure that defines a node and its associated informaion
*******************************************************************************/
struct node_struct {
  int id; /*!< The identifier of the node in question */
  int gid;  /*!< given id of node used for trace mobility */
  node_types type; /*!< The node's type, it is one of types enumarated in #node_types */
  int mobile;  /*!< The node status: static or mobile  */
  double x_pos; /*!< The X coordinate of the current location of the node */
  double y_pos; /*!< The Y coordinate of the current location of the node */
  struct mobility_struct *mob; /*!< An instantiation of the structure #mobility_struct that includes the mobility elements corresponding to the node*/
  int generator; /*!< The mobility generator according to which the node is moving or stagnating. It is one of the types enumarated in #mobility_types*/
  int block_num; /*!<block identification for connected domain rwp*/
  int event_num;
};

typedef struct node_struct node_struct; /*!< The typedef that reflects a #node_struct*/

/********************************************************************************
!A sructure that gathers all the existing nodes
*********************************************************************************/
struct node_list_struct {
  struct node_struct *node;  /*!< Avariable of type #NodePtr. It represents a node */
  struct node_list_struct *next; /*!< A pointer to the next element */
};

typedef struct node_list_struct node_list; /*!< The typedef that reflects a #node_list_struct*/

/********************************************************************************
!A sructure that represents a peer (Node, Time),
i.e the node in question and its job (move or sleep) execution time.
It is the atomic component of the #job_list_struct structure
*********************************************************************************/

struct pair_struct {
  node_struct *b; /*!< A variable of type #NodePtr. It represents a node */
  double next_event_t;  /*!< The corresponding time of job execution */
};

typedef struct pair_struct pair_struct;  /*!< The typedef that reflects a #pair_struct*/


/*****************************************************************************
!A sructure that gathers the jobs to be executed by all the non static nodes.
In the context of OMG, a job is either a move or a sleep
*****************************************************************************/
struct job_list_struct {
  pair_struct* pair; /*!< A variable of type #Pair. It represents a (Node, job-execution Time) peer */
  struct job_list_struct *next; /*!< A pointer to the next element */
};
typedef struct job_list_struct job_list;  /*!< The typedef that reflects a #job_list_struct*/

/****************************************************************************************
!A sructure that gathers the ultimate parameters needed to launch a simulation scenario
***************************************************************************************/
typedef struct {
  int nodes; /*!< The total number of nodes */
  //int number_of_nodes[MAX_NUM_NODE_TYPES];
  double min_x; /*!< The minimum value that the X coordinate might take. In other words, the minimum boundary of the simulation area according to the X axis */
  double max_x; /*!< The maximum value that the X coordinate might take, i.e the maximum boundary of the simulation area according to the X axis*/
  double min_y; /*!< The minimum value that the Y coordinate might take, i.e the minimum boundary of the simulation area according to the Y axis */
  double max_y; /*!< The minimum value that the Y coordinate might take, i.e the maximum boundary of the simulation area according to the Y axis */
  bool user_fixed; /*!< Sets if the coordinates are user defined*/
  double fixed_x; /*!< The user defined x value*/
  double fixed_y; /*!< The user defined y value*/
  double min_speed; /*!< The minimum speed. It should be different than 0.0 in order to avoid instability*/
  double max_speed; /*!< The maximum allowed speed */
  double min_journey_time; /*!< The minimum allowed trip duration. It should be different than 0.0 in order to avoid instability and properly reflect the mobility model behavior */
  double max_journey_time; /*!< The maximum allowed trip duration */
  double min_azimuth; /*!< The minimum allowed value of the RWALK movement angle*/
  double max_azimuth; /*!< The maximum allowed value of the RWALK movement angle */
  double min_sleep; /*!< The minimum allowed sleep duration. It should be different than 0.0 in order to avoid instability */
  double max_sleep; /*!< The minimum allowed sleep duration*/
  int mobility_type; /*!< The chosen mobility model for the nodes in question. It should be one of the types inumarated as #mobility_types*/
  int rwp_type; /*!< The chosen RWP  mobility model for the nodes in question either RESTRICTED or CONNECTED_DOMAIN. */
  int nodes_type; /*!< The type of the nodes in question. It should be one of the types inumarated as #node_types */
  //int nodes_mob_type[MAX_NUM_NODE_TYPES];
  int max_vertices; /*!<maximum number of verices in the grid>*/
  int max_block_num; /*!<maximum number of blocks in the grid>*/

  int seed; /*!< The seed used to generate the random positions*/
  char* mobility_file; /*!< The mobility file name containing the mobility traces used by the TRACE model; DEFAULT: TRACE/example_trace.tr */
  char* sumo_command; /*!< The command to start SUMO; Either 'sumo' or 'sumo-gui' in case a GUI would be required; see SUMO for further details; DEFAULT: sumo */
  char* sumo_config; /*!< The configuration file for SUMO; it must be a '.cfg' file located in ./SUMO/Scenarios folder DEFAULT: SUMO/Scenarios/scen.sumo.cfg */
  int sumo_start; /*!< The time at which SUMO should start; Default: 0s */
  int sumo_end; /*!< The time at which SUMO should stop; It shold be greater or equal to OAI Default: END OF EMULATION */
  int sumo_step; /*!< The simulation step of SUMO, in ms. it is 1000ms by default (i.e. SUMO updates its mobility every second); DEFAULT: 1000ms (1s) */
  char* sumo_host; /*!< The IP host address where SUMO will be run. DEFAULT: localhost */
  int sumo_port; /*!< The port number attached to SUMO on the host DEFAULT: TBC */
} omg_global_param;

/******************************************************************************
!A string List structure
*******************************************************************************/
struct string_list_struct {
  char* string;  /*! a string */
  struct string_list_struct *next; /*!< A pointer to the next string in the list */
};

typedef struct string_list_struct string_list; /*!< The typedef that reflects a #string_list_struct*/

/* PROTOTYPES */


/**
 * \fn void update_node_vector(int mobility_type, double cur_time)
 * \brief According to the mobility type, call the corresponding update function
 * \param mobility_type an integer that represents the mobility model name
 * \param cur_time a variable of type double that represents the current time
 */
void update_node_vector(int mobility_type, double cur_time);

/**
 * \fn NodePtr get_node_position(int node_type, int nID)
 * \brief According to the node type and ID, look for the corresponding updated position and display it
 * \param node_type an integer that represents the node type (UE, eNB, etc.)
 * \param nID a variable of type int that represents the specific node's ID
 * \return a NodePtr structure that stores the updated information about the specific node ((X,Y) coordinates), speed, etc.)
 */
node_struct* get_node_position(int node_type, int nid);


/**
 * \fn Node_list add_entry(NodePtr node, Node_list Node_Vector)
 * \brief Add the Node sructure to the specified Node_Vector
 * \param node a pointer to the node that should be added to the Node_Vector
 * \param Node_Vector a pointer of type Node_list that represents the Node_Vector storing all the nodes of the specified mobility type
 * \return the Node_list to which a new entry is added
 */
node_list* add_entry(node_struct* node, node_list* node_vector);

/**
 * \fn Node_list remove_node_entry(NodePtr node, Node_list Node_Vector)
 * \brief Remove the Node sructure to the specified Node_Vector.
 *     Note: the entry will be removed, but the Node will not be deleted.
 * \param node a pointer to the node that should be deleted to the Node_Vector
 * \param Node_Vector a pointer of type Node_list that represents the Node_Vector storing all the nodes of the specified mobility type
 * \return the Node_list to which a new entry is removed
 */
node_list* remove_node_entry(node_struct* node, node_list* node_vector);

/**
 * \fn void display_node_list(Node_list Node_Vector)
 * \brief Display the useful informaion about the specified Node_list (for instance the nodes IDs, their types (UE, eNB, etc.), their corresponding mobility models, their status (moving, sleeping) and  their current positions)
 * \param Node_Vector a pointer of type Node_list that represents the Node_list to be dispalyed
 */
void display_node_list(node_list* node_vector);


/**
 * \fn Node_list remove_node(Node_list list, int nID, int node_type)
 * \brief Remove a given node, identified by its ID and type, from the specified Node_list
 * \param list a pointer of type Node_list that represents the Node_Vector from which the element is to be removed
 * \param nID an int that represents the ID of the node to be removed
 * \param node_type an int that represents the type of the node to be removed from the Node_list
 * \return a pointer to the Node_list from which a new entry is removed
 */
node_list* remove_node(node_list* list, int nid, int node_type);

/**
 * \fn NodePtr find_node(Node_list list, int nID, int node_type)
 * \brief Returns the OAI node of ID nID and type node_type;
 * \param list a pointer of type Node_list that represents the Node_Vector from which the element is to be located
 * \param nID an int that represents the ID of the node to be located
 * \param node_type an int that represents the type of the node to be located in the Node_list
 * \return a pointer to the Node
 */
node_struct* find_node(node_list* list, int nid, int node_type);

/**
 * \fn void delete_entry(Node_list Node_Vector)
 * \brief Delete a Node_list entry node; calls delete_node to subsequently delete the node; free the memory
 * \param list a pointer of type Node_list that represents the entry to be deleted
 */
void delete_entry(node_list* node_vector);

/**
 * \fn void delete_entry(NodePtr node)
 * \brief Delete a node, and all subsequent data in its structure; free the memory
 * \param node a pointer to the node that should be deleted
 */
void delete_node(node_struct* node);

/**
 * \fn void clear node_List(Node_list list)
 * \brief Totally clears a Node_list and its substructure; free the memory
 * \param list a pointer of type Node_list that represents the list to be cleared
 * \return reference to the HEAD of the cleared Node_list
 */
node_list* clear_node_List(node_list* list);

/**
 * \fn void init_node_list(Node_list list)
 * \brief reset a node list, put the reference to each node to NULL, but does not delete the node
 * \param list a pointer of type Node_list that represents the list to be reset
 * \return reference to the HEAD of the reset Node_list
 */
node_list* reset_node_list(node_list* list);

/**
 * \fn void display_node_position(int ID, int generator, int type, int mobile, double X, double Y)
 * \brief Display information about the node identified by the parameters passed as arguments
 * \param ID the node's ID
 * \param generator the node's mobility type
 * \param type the node's type
 * \param mobile the node's status (moving or sleeping)
 * \param X the node's current location according to the X axis
 * \param Y the node's current location according to the Y axis
 */
void display_node_position(int id, int generator, int type, int mobile, double x, double y);


/**
 * \fn Node_list filter(Node_list Vector, int node_type)
 * \brief Filter the content of the specified Node_list in order to return only the nodes whose type is the one passed as argument
 * \param Vector a pointer of type Node_list that represents the Node_Vector to be filtered
 * \param node_type the desired type
 * \return a pointer to a new Node_list which is the input Node_list after filtering
 */
node_list* filter(node_list* vector, int node_type);


/**
 * \fn double randomGen(double a, double b)
 * \brief Generate a random number in the range [a, b] and of type double
 * \param a lower limit
 * \param b upper limit
 * \return double: the generated random number
 */
double randomgen(double a, double b);


/**
 * \fn NodePtr create_node(void)
 * \brief Creates a new #NodePtr by allocating the needed memory space for it
 * \return the created node structure
 */
node_struct* create_node(void);


/**
 * \fn MobilityPtr create_mobility(void)
 * \brief Creates a new #MobilityPtr by allocating the needed memory space for it
 * \return the created mobility structure
 */
mobility_struct* create_mobility(void);


/**
 * \fn Job_list add_job(Pair job, Job_list Job_Vector)
 * \brief adds a new job to the Job_list
 * \param job a variable of type #Pair that represents the new job
 * \param Job_Vector a variable #Job_list that represents the Job_list that stores all the scheduled jobs
 * \return the created mobility structure
 */
job_list* add_job(pair_struct* job, job_list* job_vector);
job_list* addjob(pair_struct* job, job_list* job_vector);


/**
 * \fn void display_job_list(Job_list Job_Vector)
 * \brief Traverse the Job_Vector to diplay its contents
 * \param Job_Vector the structure that stoks all the jobs
 */
void display_job_list(double curr_t, job_list* job_vector);
/**
*time average of nodes speed
*/
unsigned int nodes_avgspeed(job_list* job_vector);
/**
 * \fn Job_list job_list_sort (Job_list list, Job_list end)
 * \brief Called by the function Job_list quick_sort (Job_list list), it executes the quick sort
 * \param list the Job_list to be sorted
 * \param end Job_list needed for intermediate computation
 * \return a Job_list that is used by Job_list quick_sort (Job_list list) to perform the sort operation
 */
job_list* job_list_sort (job_list* list, job_list* end);


/**
 * \fn Job_list quick_sort (Job_list list)
 * \brief Apply a quicksort to sort the list contents
 * \param list the structure that stoks all the jobs
 * \return the sorted list
 */
job_list* quick_sort (job_list* list);


/**
 * \fn Job_list remove_job(Job_list list, int nID, int node_type)
 * \brief Look for the node idenitied by the ID and type passed as arguments and the remove its corresponding job from the #Job_list list
 * \param list the Job_list from which the job is to be removed
 * \param nID the ID of the node whose job should be removed
 * \param node_type the type of the node whose job should be removed
 * \return the updated Job_list after removing the job in question
 */
job_list* remove_job(job_list* list, int nid, int node_type);

/**
 * \fn int length(char* s)
 * \brief Helper function to return the length of a string
 * \param s the string, of which we will return the length
 * \return the length of the string c
 */
int length(char* s);

/**
 * \fn void reset_String_list(String_list)
 * \brief clears the String_list and free memory
 * \param list the String_list to be cleared;
 * \return reference to the HEAD of the cleared String_list
 */
string_list* clear_string_list(string_list* list);

/**
 * \fn void usage()
 * \brief Define the arguments that can be used in the STAND_ALONE method to launch the OMG generator (mainly for test and debug purpose)
  */
void usage(void);


#endif /*  __DEFS_H__ */
