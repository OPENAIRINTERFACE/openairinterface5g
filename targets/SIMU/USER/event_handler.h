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
