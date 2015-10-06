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
 * \file grid.h **/

#ifndef GRID_H_
#define GRID_H_

#include "omg.h"

int max_vertices_ongrid(omg_global_param omg_param);

int max_connecteddomains_ongrid(omg_global_param omg_param);


double vertice_xpos(int loc_num, omg_global_param omg_param);


double vertice_ypos(int loc_num, omg_global_param omg_param);


double area_minx(int block_num, omg_global_param omg_param);


double area_miny(int block_num, omg_global_param omg_param);

unsigned int next_block(int current_bn, omg_global_param omg_param);

unsigned int selected_blockn(int block_n,int type,int div);

#endif

