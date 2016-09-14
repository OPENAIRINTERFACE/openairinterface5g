/**
 * \file omg_vars.h
 * \brief Global variables
 *
 */

#ifndef __OMG_VARS_H__
#define __OMG_VARS_H__

#include "omg.h"

/*!A global variable used to store all the nodes information. It is an array in which every cell stocks the nodes information for a given mobility type. Its length is equal to the maximum number of mobility models that can exist in a single simulation scenario */
node_list* node_vector[MAX_NUM_NODE_TYPES + 10 /*FIXME bad workaround for indexing node_vector with SUMO*/];
node_list* node_vector_end[MAX_NUM_NODE_TYPES + 10 /*FIXME bad workaround for indexing node_vector_end with SUMO*/];

/*! A global variable which represents the length of the Node_Vector */
int node_vector_len[MAX_NUM_NODE_TYPES];
/*!a global veriable used for event average computation*/
int event_sum[100];
int events[100];

/*!A global variable used to store the scheduled jobs, i.e (node, job_time) peers   */
job_list* job_vector[MAX_NUM_MOB_TYPES];
job_list* job_vector_end[MAX_NUM_MOB_TYPES];

/*! A global variable which represents the length of the Job_Vector */
int job_vector_len[MAX_NUM_MOB_TYPES];

/*! A global variable used gather the fondamental parameters needed to launch a simulation scenario*/
omg_global_param omg_param_list[MAX_NUM_NODE_TYPES];

//double m_time;

/*!A global variable used to store selected node position generation way*/
int grid;
double xloc_div;
double yloc_div;
#endif /*  __OMG_VARS_H__ */
