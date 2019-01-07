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

/*! \file structures.h
* \brief structures used for the
* \author M. Mosli
* \date 2012
* \version 0.1
* \company Eurecom
* \email: mosli@eurecom.fr
*/

#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "openairinterface5g_limits.h"
#ifndef __PHY_IMPLEMENTATION_DEFS_H__
	#define Maxneighbor NUMBER_OF_UE_MAX
	#ifndef NB_ANTENNAS_RX
		#define NB_ANTENNAS_RX  4
	#endif
#endif
//

// how to add an underlying map as OMV background

typedef struct Geo {
  int x, y,z;
  //int Speedx, Speedy, Speedz; // speeds in each of direction
  int mobility_type; // model of mobility
  int node_type;
  int Neighbors; // number of neighboring nodes (distance between the node and its neighbors < 100)
  int Neighbor[NUMBER_OF_UE_MAX]; // array of its neighbors
  //relavent to UE only
  unsigned short state;
  unsigned short rnti;
  unsigned int connected_eNB;
  int RSSI[NB_ANTENNAS_RX];
  int RSRP;
  int RSRQ;
  int Pathloss;
  /// more info to display
} Geo;

typedef struct Data_Flow_Unit {
  // int total_num_nodes;
  struct Geo geo[NUMBER_OF_eNB_MAX+NUMBER_OF_UE_MAX];
  int end;
} Data_Flow_Unit;


#endif // STRUCTURES_H
