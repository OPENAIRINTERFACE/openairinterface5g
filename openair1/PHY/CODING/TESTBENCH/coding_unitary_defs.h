/*
 *  Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The OpenAirInterface Software Alliance licenses this file to You under
 *  the OAI Public License, Version 1.1  (the "License"); you may not use this file
 *  except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *  http://www.openairinterface.org/?page_id=698
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *  -------------------------------------------------------------------------------
 *  For more information about the OpenAirInterface (OAI) Software Alliance:
 *        contact@openairinterface.org
 */

#ifndef __CODING_UNITARY_DEFS__h__
#define __CODING_UNITARY_DEFS__h__
int oai_exit=0;
unsigned int NB_UE_INST = 1;
#include "openair1/PHY/defs_UE.h"
PHY_VARS_UE ***PHY_vars_UE_g;
#include "common/ran_context.h"
RAN_CONTEXT_t RC;

void exit_function(const char* file, const char* function, const int line, const char *s) {
  const char * msg= s==NULL ? "no comment": s;
  printf("Exiting at: %s:%d %s(), %s\n", file, line, function, msg);
  exit(-1);
}

signed char quantize(double D, double x, unsigned char B) {
  double qxd;
  short maxlev;
  qxd = floor(x / D);
  maxlev = 1 << (B - 1); //(char)(pow(2,B-1));

  if (qxd <= -maxlev)
    qxd = -maxlev;
  else if (qxd >= maxlev)
    qxd = maxlev - 1;

  return ((char) qxd);
}


#endif

