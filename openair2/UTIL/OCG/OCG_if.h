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

/*! \file OCG_if.h
* \brief Interfaces to the outside of OCG
* \author Lusheng Wang and navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __CONFIGEN_IF_H__
#define __CONFIG_IF_H__

#ifdef __cplusplus
extern "C" {
#endif
/*--- INCLUDES ---------------------------------------------------------------*/
#    include "OCG.h"
/*----------------------------------------------------------------------------*/


#    ifdef COMPONENT_CONFIGEN
#        ifdef COMPONENT_CONFIGEN_IF
#            define private_configen_if(x) x
#            define friend_configen_if(x) x
#            define public_configen_if(x) x
#        else
#            define private_configen_if(x)
#            define friend_configen_if(x) extern x
#            define public_configen_if(x) extern x
#        endif
#    else
#        define private_configen_if(x)
#        define friend_configen_if(x)
#        define public_configen_if(x) extern x
#    endif

#ifdef __cplusplus
}
#endif

#endif
