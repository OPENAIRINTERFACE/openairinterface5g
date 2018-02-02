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

/*________________________rrc_register.h________________________

 Authors : Hicham Anouar, Raymond Knopp
 Company : EURECOM
 Emails  : anouar@eurecom.fr,  knopp@eurecom.fr
________________________________________________________________*/

#ifndef __MAC_RRC_REGISTER_H__
#    define __MAC_RRC_REGISTER_H__
#include "COMMON/mac_rrc_primitives.h"
/** @defgroup _mac_impl_register RRC Registration interface
 * @ingroup _mac_impl_
 *@{
 */
MAC_RLC_XFACE* mac_rrc_register(RRC_XFACE* RRC_xface);
int mac_rrc_unregister(RRC_XFACE *RRC_xface);
#endif
