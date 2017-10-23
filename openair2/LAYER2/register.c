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

/*________________________rrc_register.c________________________

 Authors : Hicham Anouar, Raymond Knopp
 Company : EURECOM
 Emails  : anouar@eurecom.fr,  knopp@eurecom.fr
________________________________________________________________*/


#ifndef USER_MODE
#define __NO_VERSION__
#else
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#endif

#include "COMMON/openair_types.h"
#include "MAC/extern.h"

#include <linux/module.h>

#ifndef USER_MODE

//-----------------------------------------------------------------------------------------------------------//
MAC_RLC_XFACE* mac_rrc_register(RRC_XFACE* RRC_xface)
{
  //-----------------------------------------------------------------------------------------------------------//
  if(Is_rrc_registered) {
    msg("[OPENAIR][MAC][RRC_REGISTER] RRC interface already registered, aborting ...\n");
    return NULL;
  } else {
    msg("[OPENAIR][MAC][RRC_REGISTER] Registering RRC Interface, Mac_rlc_xface=%p\n",Mac_rlc_xface);
    Rrc_xface=RRC_xface;
    Is_rrc_registered=1;
    return Mac_rlc_xface;
  }
}

//-----------------------------------------------------------------------------------------------------------//
int mac_rrc_unregister(RRC_XFACE *RRC_xface)
{
  //-----------------------------------------------------------------------------------------------------------//
  if (Rrc_xface == RRC_xface) {
    msg("[OPENAIR][MAC XFACE][RRC_UNREGISTER] Unregistering RRC interface\n");
    Rrc_xface=NULL;
    Is_rrc_registered=0;
    return(0);
  } else {
    msg("[OPENAIR][MAC XFACE][RRC_UNREGISTER] Not the right interface descriptor pointer!!!, aborting ...\n");
    return (-1);
  }

}

EXPORT_SYMBOL(mac_rrc_register);
EXPORT_SYMBOL(mac_rrc_unregister);
#endif //USER_MODE


