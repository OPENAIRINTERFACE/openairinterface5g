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

/*! \file PHY_INTERFACE/defs.h
* \brief mac phy interface primitives
* \author Raymond Knopp and Navid Nikaein
* \date 2011
* \version 0.5
* \mail navid.nikaein@eurecom.fr or openair_tech@eurecom.fr
*/

#ifndef __PHY_INTERFACE_H__
#    define __PHY_INTERFACE_H__

#include "LAYER2/MAC/mac.h"


#define MAX_NUMBER_OF_MAC_INSTANCES 16

#define NULL_PDU 255
#define DCI 0
#define DLSCH 1
#define ULSCH 2

#define mac_exit_wrapper(sTRING)                                                            \
do {                                                                                        \
    char temp[300];                                                                         \
    snprintf(temp, sizeof(temp), "%s in file "__FILE__" at line %d\n", sTRING, __LINE__);   \
    mac_xface->macphy_exit(temp);                                                           \
} while(0)



#endif


/** @} */
