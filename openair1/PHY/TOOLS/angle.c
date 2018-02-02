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
