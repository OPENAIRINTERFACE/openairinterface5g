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

/** @defgroup _cli_if Interfaces of CLI
 * @{*/


void cli_init (void); 
int cli_server_init(cli_handler_t handler); 
void cli_server_cleanup(void);
void cli_server_recv(const void * data, socklen_t len);
/* @}*/

#ifdef __cplusplus
}
#endif

#endif

