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

#include "oaisim.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "UTIL/FIFO/pad_list.h"

#ifndef OAISIM_FUNCTIONS_H_
#define OAISIM_FUNCTIONS_H_

void get_simulation_options(int argc, char *argv[]);

void check_and_adjust_params(void);

void init_omv(void);

void init_seed(uint8_t set_seed);

void init_openair1(void);

void init_openair2(void);

void init_ocm(void);

void init_otg_pdcp_buffer(void);

void update_omg(frame_t frameP);

void update_omg_ocm(void);

void update_ocm(void);

void update_otg_eNB(module_id_t module_idP, unsigned int ctime);

void update_otg_UE(module_id_t module_idP, unsigned int ctime);

void exit_fun(const char* s);

void init_time(void);

void init_pad(void);

void help(void);

int init_slot_isr(void);

void wait_for_slot_isr(void);

#endif /* OAISIM_FUNCTIONS_H_ */
