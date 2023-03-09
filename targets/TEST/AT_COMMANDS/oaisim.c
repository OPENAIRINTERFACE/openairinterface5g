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

#include "LAYER2/RLC/AM_v9.3.0/rlc_am.h"
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/vars.h"
#include "UTIL/LOG/log_if.h"
#include "UTIL/LOG/log_extern.h"
#include "RRC/LTE/vars.h"
#include "PHY_INTERFACE/vars.h"
#include "UTIL/AT_COMMANDS/parser.h"

mapping level_names[] = {
  {"emerg", LOG_EMERG},
  {"alert", LOG_ALERT},
  {"crit", LOG_CRIT},
  {"err", LOG_ERR},
  {"warn", LOG_WARNING},
  {"notice", LOG_NOTICE},
  {"info", LOG_INFO},
  {"debug", LOG_DEBUG},
  {"trace", LOG_TRACE},
  {NULL, -1}
};

int main(int argc, char **argv)
{
  char * g_log_level="trace"; // by default global log level is set to trace

  //initialize the log generator
  logInit(map_str_to_int(level_names, g_log_level));
  LOG_T(LOG,"global log level is set to %s \n",g_log_level );

  at_parse_init(at_null_callback_handler, mt_null_attach_state_request_handler, mt_null_attach_request_handler, mt_null_detach_request_handler);
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  at_parse_string("AT+CGATT=?\r");
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  at_parse_string("AT+CGATT?\r");
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  at_parse_string("AT+CGATT?\r");
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  at_parse_string("AT+CGATT?\r");
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  at_parse_string("AT+CGATT?\r");
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  at_parse_string("AT+CGATT=1\r");
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  at_parse_string("AT+CGATT=0\r");
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  return(0);
}


