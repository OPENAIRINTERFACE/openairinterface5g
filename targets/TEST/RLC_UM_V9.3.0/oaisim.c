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

#include <string.h>
#include <math.h>
#include <unistd.h>
#include "SIMULATION/TOOLS/defs.h"
#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/vars.h"
#include "MAC_INTERFACE/vars.h"

#include "rlc.h"
#include "rlc_um.h"
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/vars.h"
#include "UTIL/LOG/log_if.h"
#include "UTIL/LOG/log_extern.h"
#include "RRC/LITE/vars.h"
#include "PHY_INTERFACE/vars.h"
#include "UTIL/OCG/OCG.h"



int main(int argc, char **argv)
{
  logInit();
  mac_xface = malloc(sizeof(MAC_xface));
  rlc_um_v9_3_0_test();
  return(0);
}


