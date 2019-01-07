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
