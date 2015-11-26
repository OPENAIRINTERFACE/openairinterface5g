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

/*! \file rrc_eNB_ral.h
 * \brief rrc procedures for handling RAL messages
 * \author Lionel GAUTHIER
 * \date 2013
 * \version 1.0
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr
 */
#ifndef __RRC_ENB_RAL_H__
#    define __RRC_ENB_RAL_H__
//-----------------------------------------------------------------------------
#        ifdef RRC_ENB_RAL_C
#            define private_rrc_enb_ral(x)    x
#            define protected_rrc_enb_ral(x)  x
#            define public_rrc_enb_ral(x)     x
#        else
#            ifdef RRC_ENB
#                define private_rrc_enb_ral(x)
#                define protected_rrc_enb_ral(x)  extern x
#                define public_rrc_enb_ral(x)     extern x
#            else
#                define private_rrc_enb_ral(x)
#                define protected_rrc_enb_ral(x)
#                define public_rrc_enb_ral(x)     extern x
#            endif
#        endif
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
//-----------------------------------------------------------------------------
#include "intertask_interface.h"
#include "ral_messages_types.h"
#include "defs.h"


private_rrc_enb_ral(  int rrc_enb_ral_delete_all_thresholds_type        (unsigned int mod_idP, ral_link_param_type_t *param_type_pP);)
private_rrc_enb_ral(  int rrc_enb_ral_delete_threshold                  (unsigned int mod_idP, ral_link_param_type_t* param_type_pP,
                        ral_threshold_t* threshold_pP);)
protected_rrc_enb_ral(int rrc_enb_ral_handle_configure_threshold_request(unsigned int mod_idP, MessageDef *msg_pP);)

#endif
