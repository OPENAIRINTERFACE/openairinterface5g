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

/*! \file cli_if.h
* \brief cli interface
* \author Navid Nikaein
* \date 2011 - 2014
* \version 0.1
* \warning This component can be runned only in the user-space
* @ingroup util
*/
#ifndef __CLI_IF_H__
#    define __CLI_IF_H__


/*--- INCLUDES ---------------------------------------------------------------*/
#    include "cli.h"
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

#    ifdef COMPONENT_CLI
#        ifdef COMPONENT_CLI_IF
#            define private_cli_if(x) x
#            define friend_cli_if(x) x
#            define public_cli_if(x) x
#        else
#            define private_cli_if(x)
#            define friend_cli_if(x) extern x
#            define public_cli_if(x) extern x
#        endif
#    else
#        define private_cli_if(x)
#        define friend_cli_if(x)
#        define public_cli_if(x) extern x
#    endif

/** @defgroup _cli_if Interfaces of CLI
 * @{*/


public_cli_if( void cli_init (void); )
public_cli_if( int cli_server_init(cli_handler_t handler); )
public_cli_if(void cli_server_cleanup(void);)
public_cli_if(void cli_server_recv(const void * data, socklen_t len);)
/* @}*/

#ifdef __cplusplus
}
#endif

#endif

