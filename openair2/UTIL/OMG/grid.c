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
