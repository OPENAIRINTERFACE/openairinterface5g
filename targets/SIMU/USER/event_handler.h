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

/*! \file event_handler.h
* \brief primitives to handle event acting on oai
* \author Navid Nikaein and Mohamed Said MOSLI BOUKSIAA,
* \date 2014
* \version 0.5
* @ingroup _oai
*/

#include "oaisim.h"
#include "UTIL/FIFO/pad_list.h"



void add_event(Event_t event);

void schedule(Operation_Type_t op, Event_Type_t type, int frame, char * key, void* value, int ue, int lcid);

void schedule_delayed(Operation_Type_t op, Event_Type_t type, char * key, void* value, char * time, int ue, int lcid);

void schedule_events(void);

void execute_events(frame_t frame);


void update_oai_model(char * key, void * value);

void update_sys_model(Event_t event);

void update_topo_model(Event_t event);

void update_app_model(Event_t event);

void update_emu_model(Event_t event);

void update_mac(Event_t event);

int validate_mac(Event_t event);


/*
void schedule_end_of_simulation(End_Of_Sim_Event_Type type, int value);

int end_of_simulation();
*/
