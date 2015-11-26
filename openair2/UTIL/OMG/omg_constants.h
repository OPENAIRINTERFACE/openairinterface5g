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
 * \file omg_constants.h
 * \brief Constants and Enumarations
 *
 */

// for the function set: change first_job_time accordingly for the two cases empty/non empty job_vector

#ifndef __OMG_CONSTANTS_H_
#define __OMG_CONSTANTS_H_


/*! The available mobility models */
typedef enum {
  STATIC=0,  /*!< STATIC mobility model */
  RWP,  /*!< Random Way Point mobility model */
  RWALK, /*!< Random Walk mobility model */
  TRACE,  /*!< Trace-based Mobility description file */
  SUMO,  /*!< SUMO-based mobility model  */
  STEADY_RWP, /*!steady state RWP*/
  MAX_NUM_MOB_TYPES /*!< The maximum number of mobility models. Used to adjust the length of the #Node_Vector */
} mobility_types;

/*
 * this is a sub type of standard RWP (not steady_RWP)
 */
typedef enum {
  MIN_RWP_TYPES=0,  /*!< STATIC mobility model */
  RESTIRICTED_RWP,  /*!< Random Way Point mobility model */
  CONNECTED_DOMAIN, /*!< Random Walk mobility model */
  MAX_RWP_TYPES /*!< The maximum number of mobility models. Used to adjust the length of the #Node_Vector */
} omg_rwp_types;

//#define RESTIRICTED_RWP 1
//#define CONNECTED_DOMAIN 2

/*! The available nodes types */
typedef enum {
  eNB=0, /*!< enhanced Node B  */
  UE, /*!< User Equipement  */
  RELAY,
  MAX_NUM_NODE_TYPES /*!< All the types. Used to perform the same operations to all the types of nodes */
} node_types;


//#define eps 0.00000095367431649629
/*! A constant used to compare two doubles */
#define omg_eps 0.10 //10.99

#define SLEEP 1
#define GET_DATA 0
#define GET_DATA_UPDATE 1


#endif /* __OMG_H_ */


