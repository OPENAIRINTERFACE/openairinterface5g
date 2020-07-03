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

#ifndef __UTIL_OCG_OCG_IF__H__
#define __UTIL_OCG_OCG_IF__H__

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
