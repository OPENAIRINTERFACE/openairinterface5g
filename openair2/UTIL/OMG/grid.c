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
/*! \file grid.c
* \brief
* \author S. Gashaw, N. Nikaein, J. Harri
* \date 2014
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "grid.h"
#define   LEFT 0
#define   RIGHT 1
#define   UPPER 2
#define   LOWER 3

int
max_vertices_ongrid (omg_global_param omg_param)
{
  return ((int) (omg_param.max_x / xloc_div + 1.0) *
          (int) (omg_param.max_y / yloc_div + 1.0)) - 1;
}

int
max_connecteddomains_ongrid (omg_global_param omg_param)
{
  return ((int) (omg_param.max_x / xloc_div) *
          (int) (omg_param.max_y / yloc_div)) - 1;
}

double
vertice_xpos (int loc_num, omg_global_param omg_param)
{
  int div, mod;
  double x_pos;

  div = (int) (omg_param.max_x / xloc_div + 1.0);
  mod = (loc_num % div) * xloc_div;
  x_pos = omg_param.min_x + (double) mod;
  //LOG_D(OMG,"mod %d div %d x pos %.2f \n\n",mod,div,x_pos);
  return x_pos;
}

double
vertice_ypos (int loc_num, omg_global_param omg_param)
{
  //LOG_D(OMG,"y pos %.2f \n\n",omg_param.min_y + (double)yloc_div * (int)( loc_num / (int)(omg_param.max_x/xloc_div + 1.0) ));
  return omg_param.min_y +
         (double) yloc_div *(int) (loc_num /
                                   (int) (omg_param.max_x / xloc_div + 1.0));
}

double
area_minx (int block_num, omg_global_param omg_param)
{
  return omg_param.min_x +
         xloc_div * (block_num % (int) (omg_param.max_x / xloc_div));

}

double
area_miny (int block_num, omg_global_param omg_param)
{
  return omg_param.min_y +
         yloc_div * (int) (block_num / (int) (omg_param.max_x / xloc_div));
}

/*for connected domain that only move to the neighbor domains */

unsigned int
next_block (int current_bn, omg_global_param omg_param)
{

  double rnd = randomgen (0, 1);
  unsigned int blk;
  int div = (int) (omg_param.max_x / xloc_div);

  /*left border blocks*/
  if ((current_bn % div) == 0) {
    /*case 1 for left upper and lower corners(only 2 neighbors) */
    if ((int) (current_bn / div) == 0) {
      if (rnd <= 0.5)
        blk = selected_blockn (current_bn, RIGHT, div);
      else
        blk = selected_blockn (current_bn, UPPER, div);
    } else if ((int) (current_bn / div) ==
               (int) (omg_param.max_y / yloc_div) - 1) {
      if (rnd <= 0.5)
        blk = selected_blockn (current_bn, RIGHT, div);
      else
        blk = selected_blockn (current_bn, LOWER, div);
    }
    /*for 3 neighbor blocks */
    else {
      if (rnd <= 0.33)
        blk = selected_blockn (current_bn, RIGHT, div);
      else if (rnd > 0.33 && rnd <= 0.66)
        blk = selected_blockn (current_bn, UPPER, div);
      else
        blk = selected_blockn (current_bn, LOWER, div);
    }

  }
  /*right boredr blocks*/
  else if ((current_bn % (int) (omg_param.max_x / xloc_div)) == div - 1) {
    /*case 1 for right upper and lower corners(only 2 neighbors) */
    if ((int) (current_bn / div) == 0) {
      if (rnd <= 0.5)
        blk = selected_blockn (current_bn, LEFT, div);
      else
        blk = selected_blockn (current_bn, UPPER, div);
    } else if ((int) (current_bn / div) ==
               (int) (omg_param.max_y / yloc_div) - 1) {
      if (rnd <= 0.5)
        blk = selected_blockn (current_bn, LEFT, div);
      else
        blk = selected_blockn (current_bn, LOWER, div);
    }
    /*for 3 neighbor blocks */
    else {
      if (rnd <= 0.33)
        blk = selected_blockn (current_bn, LEFT, div);
      else if (rnd > 0.33 && rnd <= 0.66)
        blk = selected_blockn (current_bn, UPPER, div);
      else
        blk = selected_blockn (current_bn, LOWER, div);
    }


  }
  /*for 3 neighbor uper and lower borders*/
  else if ((int) (current_bn / div) == 0
           || (int) (current_bn / div) ==
           (int) (omg_param.max_y / yloc_div) - 1) {

    if ((int) (current_bn / div) == 0) {
      if (rnd <= 0.33)
        blk = selected_blockn (current_bn, LEFT, div);
      else if (rnd > 0.33 && rnd <= 0.66)
        blk = selected_blockn (current_bn, RIGHT, div);
      else
        blk = selected_blockn (current_bn, UPPER, div);
    }

    else {
      if (rnd <= 0.33)
        blk = selected_blockn (current_bn, LEFT, div);
      else if (rnd > 0.33 && rnd <= 0.66)
        blk = selected_blockn (current_bn, RIGHT, div);
      else
        blk = selected_blockn (current_bn, LOWER, div);
    }

  } else {
    if (rnd <= 0.25)
      blk = selected_blockn (current_bn, LEFT, div);
    else if (rnd > 0.25 && rnd <= 0.50)
      blk = selected_blockn (current_bn, RIGHT, div);
    else if (rnd > 0.50 && rnd <= 0.75)
      blk = selected_blockn (current_bn, UPPER, div);
    else
      blk = selected_blockn (current_bn, LOWER, div);
  }


  return blk;

}

/*retun the block number of neighbor selected for next move */
unsigned int
selected_blockn (int block_n, int type, int div)
{
  unsigned int next_blk = 0;

  switch (type) {
  case LEFT:
    next_blk = block_n - 1;
    break;

  case RIGHT:
    next_blk = block_n + 1;
    break;

  case UPPER:
    next_blk = block_n + div;
    break;

  case LOWER:
    next_blk = block_n - div;
    break;

  default:
    LOG_E (OMG, "wrong type input\n");
  }

  return next_blk;

}
