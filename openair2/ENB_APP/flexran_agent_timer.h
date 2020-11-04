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

/*! \file flexran_agent_timer.h
 * \brief FlexRAN Timer  header
 * \author shahab SHARIAT BAGHERI
 * \date 2017
 * \version 0.1
 */

#ifndef _FLEXRAN_AGENT_TIMER_
#define _FLEXRAN_AGENT_TIMER_

#include "flexran_agent_common.h"
#include "flexran_agent_defs.h"

#define TIMER_NULL                 -1 
#define TIMER_TYPE_INVALIDE        -2
#define	TIMER_SETUP_FAILED         -3
#define	TIMER_REMOVED_FAILED       -4
#define	TIMER_ELEMENT_NOT_FOUND    -5

typedef enum {
  FLEXRAN_AGENT_TIMER_TYPE_ONESHOT,
  FLEXRAN_AGENT_TIMER_TYPE_PERIODIC,
} flexran_agent_timer_type_t;

/* Type of the callback executed when the timer expired */
typedef Protocol__FlexranMessage *(*flexran_agent_timer_callback_t)(
    mid_t mod_id, const Protocol__FlexranMessage *msg);

err_code_t flexran_agent_timer_init(mid_t mod_id);
void       flexran_agent_timer_exit(mid_t mod_id);

/* Signals next subframe for FlexRAN timers */
void flexran_agent_timer_signal(mid_t mod_id);

/* Create a timer for some agent related event with id xid. */
err_code_t flexran_agent_create_timer(mid_t    mod_id,
                                      uint32_t sf,
                                      flexran_agent_timer_type_t timer_type,
                                      xid_t    xid,
                                      flexran_agent_timer_callback_t cb,
                                      Protocol__FlexranMessage *msg);

/* Destroy the timer for task with id xid */
err_code_t flexran_agent_destroy_timer(mid_t mod_id, xid_t xid);

#endif /* _FLEXRAN_AGENT_TIMER_ */
