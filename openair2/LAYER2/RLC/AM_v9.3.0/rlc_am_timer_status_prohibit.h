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
/*! \file rlc_am_timer_status_prohibit.h
* \brief This file defines the prototypes of the functions manipulating the t-StatusProhibit timer.
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
#ifndef __RLC_AM_TIMER_STATUS_PROHIBIT_H__
#    define __RLC_AM_TIMER_STATUS_PROHIBIT_H__
#    ifdef RLC_AM_TIMER_STATUS_PROHIBIT_C
#        define private_rlc_am_timer_status_prohibit(x)    x
#        define protected_rlc_am_timer_status_prohibit(x)  x
#        define public_rlc_am_timer_status_prohibit(x)     x
#    else
#        ifdef RLC_AM_MODULE
#            define private_rlc_am_timer_status_prohibit(x)
#            define protected_rlc_am_timer_status_prohibit(x)  extern x
#            define public_rlc_am_timer_status_prohibit(x)     extern x
#        else
#            define private_rlc_am_timer_status_prohibit(x)
#            define protected_rlc_am_timer_status_prohibit(x)
#            define public_rlc_am_timer_status_prohibit(x)     extern x
#        endif
#    endif

/*! \fn void rlc_am_check_timer_status_prohibit(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlc_pP)
* \brief      Check if timer status-prohibit has timed-out, if so it is stopped and has the status "timed-out".
* \param[in]  ctxt_pP           Running context.
* \param[in]  rlc_pP            RLC AM protocol instance pointer.
*/
protected_rlc_am_timer_status_prohibit(void rlc_am_check_timer_status_prohibit(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlc_pP));

/*! \fn void rlc_am_stop_and_reset_timer_status_prohibit(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlc_pP)
* \brief      Stop and reset the timer status-prohibit.
* \param[in]  ctxt_pP           Running context.
* \param[in]  rlc_pP            RLC AM protocol instance pointer.
*/
protected_rlc_am_timer_status_prohibit(void rlc_am_stop_and_reset_timer_status_prohibit(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlc_pP);)

/*! \fn void rlc_am_start_timer_status_prohibit(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlc_pP)
* \brief      Re-arm (based on RLC AM config parameter) and start timer status-prohibit.
* \param[in]  ctxt_pP           Running context.
* \param[in]  rlc_pP            RLC AM protocol instance pointer.
*/
protected_rlc_am_timer_status_prohibit(void rlc_am_start_timer_status_prohibit(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlc_pP);)

/*! \fn void rlc_am_init_timer_status_prohibit(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlc_pP, const uint32_t time_outP)
* \brief      Initialize the timer status-prohibit with RLC AM time-out config parameter.
* \param[in]  ctxt_pP           Running context.
* \param[in]  rlc_pP            RLC AM protocol instance pointer.
* \param[in]  time_outP         Time-out in frameP units.
*/
protected_rlc_am_timer_status_prohibit(void rlc_am_init_timer_status_prohibit(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlc_pP, const uint32_t time_outP);)
/** @} */
#endif
