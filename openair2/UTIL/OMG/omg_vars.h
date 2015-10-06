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
