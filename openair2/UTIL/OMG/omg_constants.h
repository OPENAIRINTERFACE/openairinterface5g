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


