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

/*____________________________OPT/opt.h___________________________
Authors:  Navid NIKAIEN
Company: EURECOM
Emails:
*This file include all defined structures & function headers of this module
This header file must be included */
/**
 * Include bloc
 * */

#ifndef OPT_H_
#define OPT_H_

#ifndef sys_include
#define sys_include
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#endif
#ifndef project_include
#define project_include
#include "UTIL/LOG/log_if.h"
// #include "UTIL/LOG/log_extern.h"
//#include "PHY/defs.h"
//#include "PHY/extern.h"
#include "PHY/impl_defs_lte.h"
#endif

#ifdef OCP_FRAMEWORK
#include <enums.h>
#else
typedef enum trace_mode_e {
  OPT_WIRESHARK,
  OPT_PCAP,
  OPT_TSHARK,
  OPT_NONE
} trace_mode_t;
#endif

typedef enum radio_type_e {
  RADIO_TYPE_FDD = 1,
  RADIO_TYPE_TDD = 2,
  RADIO_TYPE_MAX
} radio_type_t;

extern trace_mode_t opt_type;
extern char in_ip[40];
extern char in_path[100];

/**
 * function def
*/

void trace_pdu(int direction, uint8_t *pdu_buffer, unsigned int pdu_buffer_size,
               int ueid, int rntiType, int rnti, uint16_t sysFrame, uint8_t subframe,
               int oob_event, int oob_event_value);

int init_opt(char *path, char *ip, char *port, radio_type_t radio_type_p);

void terminate_opt(void);

extern int opt_enabled;
//double *timing_analyzer(int index, int direction );

#endif /* OPT_H_ */
