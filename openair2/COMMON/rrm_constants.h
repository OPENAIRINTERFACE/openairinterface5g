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

/*
                               rrm_constants.h
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr



 ***************************************************************************/
#    ifndef __RRM_CONSTANTS_H__
#        define __RRM_CONSTANTS_H__
#        include "platform_constants.h"

/*#################################################
  CONSTANTS FOR L1/L2/L3 LAYERS
#################################################*/
#        define RRM_CONFIG_VALID    0xFF
#        define RRM_CONFIG_INVALID  0x00
#        define RRM_NO_REFERENCE    -1
#        define TRANSACTION_NULL 0

// RRM action
/*
#        define ACTION_NULL    0
#        define ACTION_ADD     1
#        define ACTION_REMOVE  2
#        define ACTION_MODIFY  3
#        define ACTION_SET_SECURITY_MODE 4
#        define ACTION_MBMS_ADD 10
#        define ACTION_MBMS_MODIFY 11
*/

#        define MOBILE    7

// equipment type
#        define MOBILE_TERMINAL 1
#        define RADIO_GATEWAY   2


#        define UNUSED_PARAMETER -1
//---------------------------
// COMMON TRCH
//---------------------------
#        define PREFERED_RLC_BLOCK_SIZE 336
//(43 bytes)


#        define JRRM_MAX_TTI                       2
#        define JRRM_MAX_COMMANDS_PER_TRANSACTION  50

#    endif
