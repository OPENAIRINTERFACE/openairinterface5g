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
/******************************
 * file: angle.c
 * purpose: compute the angle of a 8 bit complex number
 * author: florian.kaltenberger@eurecom.fr
 * date: 22.9.2009
 *******************************/

#include "costable.h"
#include "defs.h"

unsigned int angle(struct complex16 perror)
{
  int a;

  // a = atan(perror.i/perror.r);

  //since perror is supposed to be on the unit circle, we can also compute a by
  if (perror.i>=0) {
    if (perror.r>=0)
      a = acostable[min(perror.r,255)];
    else
      a = 32768-acostable[min(-perror.r,255)];

    //a = asin(perror.i);
  } else {
    if (perror.r>=0)
      a = 65536-acostable[min(perror.r,255)];
    else
      a = 32768+acostable[min(-perror.r,255)];

    //a = 2*PI-asin(perror.i);
  }

  return a;

}
