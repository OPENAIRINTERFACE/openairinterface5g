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
#    ifdef list_C
#        define private_list(x) x
#        define public_list(x) x
#    else
#        define private_list(x)
#        define public_list(x) extern x
#    endif
#    ifdef LIST2_C
#        define private_list2(x) x
#        define public_list2(x) x
#    else
#        define private_list2(x)
#        define public_list2(x) extern x
#    endif
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
public_list( void         job_list_init (Job_List_t*);)
public_list( void         job_list_free (Job_List_t* listP);)
public_list( Job_elt_t* job_list_get_head (Job_List_t*);)
public_list( Job_elt_t* job_list_remove_head (Job_List_t* );)
public_list( Job_elt_t* job_list_remove_element (Job_elt_t*, Job_List_t*);)
public_list( void         job_list_add_head (Job_elt_t* , Job_List_t* );)
public_list( void         job_list_add_tail_eurecom (Job_elt_t* , Job_List_t* );)
public_list( void         job_list_add_list (Job_List_t* , Job_List_t* );)
public_list( void         job_list_display (Job_List_t* );)

public_list( void         event_list_init (Event_List_t*);)
public_list( void         event_list_free (Event_List_t* listP);)
public_list( Event_elt_t* event_list_get_head (Event_List_t*);)
public_list( Event_elt_t* event_list_remove_head (Event_List_t* );)
public_list( Event_elt_t* event_list_remove_element (Event_elt_t*, Event_List_t*);)
public_list( void         event_list_add_head (Event_elt_t* , Event_List_t* );)
public_list( void         event_list_add_element (Event_elt_t* , Event_elt_t* , Event_List_t* );)
public_list( void         event_list_add_tail_eurecom (Event_elt_t* , Event_List_t* );)
public_list( void         event_list_add_list (Event_List_t* , Event_List_t* );)
public_list( void         event_list_display (Event_List_t* );)

public_list( void         pkt_list_init (Packet_OTG_List_t*);)
public_list( void         pkt_list_free (Packet_OTG_List_t* listP);)
public_list( Packet_otg_elt_t* pkt_list_get_head (Packet_OTG_List_t*);)
public_list( Packet_otg_elt_t* pkt_list_remove_head (Packet_OTG_List_t* );)
public_list( Packet_otg_elt_t* pkt_list_remove_element (Packet_otg_elt_t*, Packet_OTG_List_t*);)
public_list( void         pkt_list_add_head (Packet_otg_elt_t* , Packet_OTG_List_t* );)
public_list( void         pkt_list_add_element (Packet_otg_elt_t* , Packet_otg_elt_t* , Packet_OTG_List_t* );)
public_list( void         pkt_list_add_tail_eurecom (Packet_otg_elt_t* , Packet_OTG_List_t* );)
public_list( void         pkt_list_add_list (Packet_OTG_List_t* , Packet_OTG_List_t* );)
public_list( void         pkt_list_display (Packet_OTG_List_t* );)

#endif
