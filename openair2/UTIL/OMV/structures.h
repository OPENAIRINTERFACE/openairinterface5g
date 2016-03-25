/*******************************************************************************

  Eurecom OpenAirInterface 2
  Copyright(c) 1999 - 2010 Eurecom

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information
  Openair Admin: openair_admin@eurecom.fr
  Openair Tech : openair_tech@eurecom.fr
  Forums       : http://forums.eurecom.fsr/openairinterface
  Address      : Eurecom, 2229, route des crêtes, 06560 Valbonne Sophia Antipolis, France

*******************************************************************************/


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

#ifndef __PHY_IMPLEMENTATION_DEFS_H__
#define Maxneighbor 64
#define NUMBER_OF_UE_MAX 64
#define NUMBER_OF_eNB_MAX 3
#ifndef NB_ANTENNAS_RX
#  define NB_ANTENNAS_RX  4
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
