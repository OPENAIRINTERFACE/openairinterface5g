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

/*! \file steadystaterwp.h
* \brief random waypoint mobility generator
* \date 2014
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/
#ifndef STEADYSTATERWP_H_
#define STEADYSTATERWP_H_


int start_steadystaterwp_generator (omg_global_param omg_param_list);

void place_steadystaterwp_node (node_struct* node);

void sleep_steadystaterwp_node (pair_struct* pair, double cur_time);

void move_steadystaterwp_node (pair_struct* pair, double cur_time);

double pause_probability(omg_global_param omg_param);


double initial_pause(omg_global_param omg_param);

double initial_speed(omg_global_param omg_param);

void update_steadystaterwp_nodes (double cur_time);

void get_steadystaterwp_positions_updated (double cur_time);

#endif











