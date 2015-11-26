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

typedef enum trace_mode_e {
  OPT_WIRESHARK,
  OPT_PCAP,
  OPT_TSHARK,
  OPT_NONE
} trace_mode_t;

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
               int ueid, int rntiType, int rnti, uint8_t subframe,
               int oob_event, int oob_event_value);

int init_opt(char *path, char *ip, char *port, radio_type_t radio_type_p);

void terminate_opt(void);

extern int opt_enabled;
//double *timing_analyzer(int index, int direction );

#endif /* OPT_H_ */
