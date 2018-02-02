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

/*! \file rlc_um_fsm.h
* \brief This file defines the prototypes of the functions dealing with the finite state machine of the RLC UM protocol instance.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_um_fsm_impl_ RLC UM FSM Implementation
* @ingroup _rlc_um_impl_
* @{
*/
#    ifndef __RLC_UM_FSM_PROTO_EXTERN_H__
#        define __RLC_UM_FSM_PROTO_EXTERN_H__
//-----------------------------------------------------------------------------
#        ifdef RLC_UM_FSM_C
#            define private_rlc_um_fsm(x)    x
#            define protected_rlc_um_fsm(x)  x
#            define public_rlc_um_fsm(x)     x
#        else
#            ifdef RLC_UM_MODULE
#                define private_rlc_um_fsm(x)
#                define protected_rlc_um_fsm(x)  extern x
#                define public_rlc_um_fsm(x)     extern x
#            else
#                define private_rlc_um_fsm(x)
#                define protected_rlc_um_fsm(x)
#                define public_rlc_um_fsm(x)     extern x
#            endif
#        endif
#        include "platform_types.h"
#        include "rlc_um_entity.h"
//-----------------------------------------------------------------------------
/*! \fn int rlc_um_fsm_notify_event (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP, uint8_t eventP)
* \brief    Send an event to the RLC UM finite state machine.
* \param[in]  ctxtP       Running context.
* \param[in]  rlcP        RLC UM protocol instance pointer.
* \param[in]  eventP      Event (#RLC_UM_RECEIVE_CRLC_CONFIG_REQ_ENTER_NULL_STATE_EVENT,
*                         #RLC_UM_RECEIVE_CRLC_CONFIG_REQ_ENTER_DATA_TRANSFER_READY_STATE_EVENT,
*                         #RLC_UM_RECEIVE_CRLC_SUSPEND_REQ_EVENT,
*                         #RLC_UM_TRANSMIT_CRLC_SUSPEND_CNF_EVENT,
*                         #RLC_UM_RECEIVE_CRLC_RESUME_REQ_EVENT).
* \return     1 if no error was encountered, 0 if the event was not processed.
* \Note       This FSM is not LTE 9.3.0 compliant, it has to be modified or removed.
*/
protected_rlc_um_fsm(int      rlc_um_fsm_notify_event (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP, uint8_t eventP));
/** @} */
#    endif
