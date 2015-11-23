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

#ifndef __FIFO_TYPES_H__
#define __FIFO_TYPES_H__
#include "platform_types.h"

/* Types regrouping both user-defined and regular events */
typedef enum  {
  MIN_ET=0,
  OAI_ET=MIN_ET, // config events
  SYS_ET,
  TOPO_ET,
  APP_ET,
  EMU_ET,
  DL_ET, // frame events
  UL_ET,
  S_ET,
  PHY_ET, // protocol events
  MAC_ET,
  RLC_ET,
  PDCP_ET,
  RRC_ET,
  MAX_ET
} Event_Type_t;

/* decomposition of node functions into jobs for a given event */
typedef enum Job_type_e { JT_OTG, JT_PDCP, JT_PHY_MAC, JT_INIT_SYNC, JT_DL, JT_UL, RN_DL, RN_UL, JT_END} Job_Type_t;

typedef enum Operation_Type_e { READ, WRITE, RESET} Operation_Type_t;

typedef struct Job_s {
  enum Job_type_e type;
  int             exe_time; /* execution time at the worker*/
  int             nid; /* node id*/
  eNB_flag_t      eNB_flag;
  frame_t         frame;
  int             last_slot;
  int             next_slot;
  int             ctime;
} Job_t;

typedef struct Signal_buffers_s { // (s = transmit, r,r0 = receive)
  double **s_re;
  double **s_im;
  double **r_re;
  double **r_im;
  double **r_re0;
  double **r_im0;
} Signal_buffers_t;

/*!\brief  sybframe type : DL, UL, SF, */

typedef struct Packet_otg_s {
  unsigned int              sdu_buffer_size;
  unsigned char            *sdu_buffer;
  module_id_t               module_id;
  rb_id_t                   rb_id;
  module_id_t               dst_id;
  boolean_t                 is_ue;
  pdcp_transmission_mode_t  mode;
} Packet_otg_t;

typedef struct {
  Event_Type_t type;
  enum Operation_Type_e optype; //op
  char             *key;
  void             *value;
  frame_t           frame;
  int ue;
  int lcid;
} Event_t;

/*typedef struct Global_Time {
  uint32_t frame;
  int32_t slot;
  int32_t last_slot;
  int32_t next_slot;
  double time_s;
  double time_ms;
};*/



typedef struct Packet_otg_elt_s {
  struct Packet_otg_elt_s *next;
  struct Packet_otg_elt_s *previous;
  Packet_otg_t             otg_pkt;
} Packet_otg_elt_t;

typedef struct Job_element_s {
  struct Job_element_s *next;
  Job_t                 job;
} Job_elt_t;

typedef struct Event_element_s {
  struct Event_element_s *next;
  struct Event_element_s *previous;
  Event_t               event;
} Event_elt_t;
#endif
