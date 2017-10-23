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
