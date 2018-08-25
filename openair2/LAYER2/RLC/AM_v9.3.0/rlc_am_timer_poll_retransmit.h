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

/*! \file rlc_am_timer_poll_retransmit.h
* \brief This file defines the prototypes of the functions manipulating the t-PollRetransmit timer.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_am_timers_impl_ RLC AM Timers Reference Implementation
* @ingroup _rlc_am_internal_impl_
* @{
*/

#ifndef __RLC_AM_TIMER_POLL_RETRANSMIT_H__
#    define __RLC_AM_TIMER_POLL_RETRANSMIT_H__

/*! \fn void rlc_am_check_timer_poll_retransmit(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlc_pP)
* \brief      Check if timer Poll-retransmit has timed-out, if so it is stopped and has the status "timed-out".
* \param[in]  ctxt_pP           Running context.
* \param[in]  rlc_pP            RLC AM protocol instance pointer.

*/
void rlc_am_check_timer_poll_retransmit(
    const protocol_ctxt_t* const ctxt_pP,
    rlc_am_entity_t* const       rlc_pP);

/*! \fn int  rlc_am_is_timer_poll_retransmit_timed_out(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlc_pP)
* \brief      Boolean function, check if timer Poll-retransmit has timed-out.
* \param[in]  ctxt_pP           Running context.
* \param[in]  rlc_pP            RLC AM protocol instance pointer.
* \return     1 if timer Poll-retransmit has timed-out, else 0.
*/
int  rlc_am_is_timer_poll_retransmit_timed_out(
    const protocol_ctxt_t* const ctxt_pP,
    rlc_am_entity_t* const       rlc_pP);

/*! \fn void rlc_am_stop_and_reset_timer_poll_retransmit(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlc_pP)
* \brief      Stop and reset the timer Poll-retransmit.
* \param[in]  ctxt_pP           Running context.
* \param[in]  rlc_pP            RLC AM protocol instance pointer.

*/
void rlc_am_stop_and_reset_timer_poll_retransmit(
    const protocol_ctxt_t* const ctxt_pP,
    rlc_am_entity_t* const       rlc_pP);

/*! \fn void rlc_am_start_timer_poll_retransmit(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlc_pP)
* \brief      Re-arm (based on RLC AM config parameter) and start timer Poll-retransmit.
* \param[in]  ctxt_pP           Running context.
* \param[in]  rlc_pP            RLC AM protocol instance pointer.
* \param[in]  frame             Frame index
*/
void rlc_am_start_timer_poll_retransmit(
    const protocol_ctxt_t* const ctxt_pP,
    rlc_am_entity_t* const       rlc_pP);

/*! \fn void rlc_am_init_timer_poll_retransmit(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlc_pP, const uint32_t time_outP)
* \brief      Initialize the timer Poll-retransmit with RLC AM time-out config parameter.
* \param[in]  ctxt_pP           Running context.
* \param[in]  rlc_pP            RLC AM protocol instance pointer.
* \param[in]  time_outP         Time-out in frame units.
*/
void rlc_am_init_timer_poll_retransmit(
    const protocol_ctxt_t* const ctxt_pP,
    rlc_am_entity_t* const       rlc_pP,
    const uint32_t               time_outP);
/** @} */
#endif
