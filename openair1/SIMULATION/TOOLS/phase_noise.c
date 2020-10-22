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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include  "sim.h"


/* linear phase noise model */
void phase_noise(double ts, int16_t * InRe, int16_t * InIm)
{
  double fd = 300;//0.01*30000
  static double i=0;
  double real, imag,x ,y;
  i++;
  real = cos(fd * 2 * M_PI * i * ts);
  imag = sin (fd * 2 * M_PI * i * ts);
  x = ((real * (double)InRe[0]) - (imag * (double)InIm[0])) ;
  y= ((real * (double)InIm[0]) + (imag * (double)InRe[0])) ;
  InRe[0]= (int16_t)(x);
  InIm[0]= (int16_t)(y);
}
