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
