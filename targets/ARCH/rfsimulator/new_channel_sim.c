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
* Author and copyright: Laurent Thomas, open-cells.com
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>


#include <common/utils/assertions.h>
#include <common/utils/LOG/log.h>
#include <common/config/config_userapi.h>
#include <openair1/SIMULATION/TOOLS/sim.h>
#include <targets/ARCH/rfsimulator/rfsimulator.h>

// Ziggurat 
static double wn[128],fn[128];
static uint32_t iz,jz,jsr=123456789,kn[128];
static int32_t hz;
#define SHR3 (jz=jsr, jsr^=(jsr<<13),jsr^=(jsr>>17),jsr^=(jsr<<5),jz+jsr)
#define UNI (0.5+(signed) SHR3 * 0.2328306e-9)

double nfix(void) {
  const double r = 3.442620;
  static double x, y;

  for (;;) {
    x=hz *  wn[iz];

    if (iz==0) {
      do {
        x = - 0.2904764 * log (UNI);
        y = - log (UNI);
      } while (y+y < x*x);

      return (hz>0)? r+x : -r-x;
    }

    if (fn[iz]+UNI*(fn[iz-1]-fn[iz])<exp(-0.5*x*x)) {
      return x;
    }

    hz = SHR3;
    iz = hz&127;

    if (abs(hz) < kn[iz]) {
      return ((hz)*wn[iz]);
    }
  }
}

/*!\Procedure to create tables for normal distribution kn,wn and fn. */

void tableNor(unsigned long seed) {
  jsr=seed;
  double dn = 3.442619855899;
  int i;
  const double m1 = 2147483648.0;
  double q;
  double tn = 3.442619855899;
  const double vn = 9.91256303526217E-03;
  q = vn/exp(-0.5*dn*dn);
  kn[0] = ((dn/q)*m1);
  kn[1] = 0;
  wn[0] =  ( q / m1 );
  wn[127] = ( dn / m1 );
  fn[0] = 1.0;
  fn[127] = ( exp ( - 0.5 * dn * dn ) );

  for ( i = 126; 1 <= i; i-- ) {
    dn = sqrt (-2.0 * log ( vn/dn + exp(-0.5*dn*dn)));
    kn[i+1] = ((dn / tn)*m1);
    tn = dn;
    fn[i] = (exp (-0.5*dn*dn));
    wn[i] = (dn / m1);
  }

  return;
}

double gaussZiggurat(double mean, double variance) {
  hz=SHR3;
  iz=hz&127;
  return abs(hz)<kn[iz]? hz*wn[iz] : nfix();
}
