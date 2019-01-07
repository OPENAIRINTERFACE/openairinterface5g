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

/*! \file pad_list.c
* \brief list management primimtives
* \author Mohamed Said MOSLI BOUKSIAA, Lionel GAUTHIER, Navid Nikaein
* \date 2012 - 2014
* \version 0.5
* @ingroup util
*/

#ifndef __list_H__
#define __list_H__

#include "types.h"
//-----------------------------------------------------------------------------

typedef struct Job_List_s {
  Job_elt_t *head;
  Job_elt_t *tail;
  int        nb_elements;
} Job_List_t;

typedef struct Event_List_s {
  Event_elt_t *head;
  Event_elt_t *tail;
  int          nb_elements;
} Event_List_t;

typedef struct Packet_OTG_List_s {
  Packet_otg_elt_t *head;
  Packet_otg_elt_t *tail;
  int               nb_elements;
} Packet_OTG_List_t;

//-----------------------------------------------------------------------------
void         job_list_init (Job_List_t*);
void         job_list_free (Job_List_t* listP);
Job_elt_t* job_list_get_head (Job_List_t*);
Job_elt_t* job_list_remove_head (Job_List_t* );
Job_elt_t* job_list_remove_element (Job_elt_t*, Job_List_t*);
void         job_list_add_head (Job_elt_t* , Job_List_t* );
void         job_list_add_tail_eurecom (Job_elt_t* , Job_List_t* );
void         job_list_add_list (Job_List_t* , Job_List_t* );
void         job_list_display (Job_List_t* );

void         event_list_init (Event_List_t*);
void         event_list_free (Event_List_t* listP);
Event_elt_t* event_list_get_head (Event_List_t*);
Event_elt_t* event_list_remove_head (Event_List_t* );
Event_elt_t* event_list_remove_element (Event_elt_t*, Event_List_t*);
void         event_list_add_head (Event_elt_t* , Event_List_t* );
void         event_list_add_element (Event_elt_t* , Event_elt_t* , Event_List_t* );
void         event_list_add_tail_eurecom (Event_elt_t* , Event_List_t* );
void         event_list_add_list (Event_List_t* , Event_List_t* );
void         event_list_display (Event_List_t* );

void         pkt_list_init (Packet_OTG_List_t*);
void         pkt_list_free (Packet_OTG_List_t* listP);
Packet_otg_elt_t* pkt_list_get_head (Packet_OTG_List_t*);
Packet_otg_elt_t* pkt_list_remove_head (Packet_OTG_List_t* );
Packet_otg_elt_t* pkt_list_remove_element (Packet_otg_elt_t*, Packet_OTG_List_t*);
void         pkt_list_add_head (Packet_otg_elt_t* , Packet_OTG_List_t* );
void         pkt_list_add_element (Packet_otg_elt_t* , Packet_otg_elt_t* , Packet_OTG_List_t* );
void         pkt_list_add_tail_eurecom (Packet_otg_elt_t* , Packet_OTG_List_t* );
void         pkt_list_add_list (Packet_OTG_List_t* , Packet_OTG_List_t* );
void         pkt_list_display (Packet_OTG_List_t* );

#endif
