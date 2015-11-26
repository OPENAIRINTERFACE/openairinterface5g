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

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*****************************************************************************

Source    user_api.h

Version   0.1

Date    2012/02/28

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel

Description Implements the API used by the NAS layer running in the UE
    to send/receive message to/from the user application layer

*****************************************************************************/
#ifndef __USER_API_H__
#define __USER_API_H__

#include "commonDef.h"
#include "networkDef.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int user_api_initialize(const char* host, const char* port, const char* devname, const char* devparams);

int user_api_emm_callback(Stat_t stat, tac_t tac, ci_t ci, AcT_t AcT, const char* data, size_t size);
int user_api_esm_callback(int cid, network_pdn_state_t state);

int user_api_get_fd(void);
const void* user_api_get_data(int index);

int user_api_read_data(int fd);
int user_api_set_data(char *message);
int user_api_send_data(int fd, int length);
void user_api_close(int fd);

int user_api_decode_data(int length);
int user_api_encode_data(const void* data, int add_success_code);

#endif /* __USER_API_H__ */
