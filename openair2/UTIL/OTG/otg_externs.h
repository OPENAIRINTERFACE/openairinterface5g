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

/*! \file otg_externs.h
* \brief extern parameters
* \author n. nikaein A. Hafsaoui
* \date 2012
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __OTG_EXTERNS_H__
#    define __OTG_EXTERNS_H__


/*!< \brief main log variables */
extern otg_t *g_otg; /*!< \brief global params */
extern otg_multicast_t *g_otg_multicast; /*!< \brief global params */
extern otg_info_t *otg_info; /*!< \brief info otg */
extern otg_multicast_info_t *otg_multicast_info; /*!< \brief  info otg: measurements about the simulation  */
extern otg_forms_info_t *otg_forms_info;

extern mapping otg_multicast_app_type_names[] ;

extern mapping otg_app_type_names[];

extern mapping otg_transport_protocol_names[];

extern mapping otg_ip_version_names[];

extern mapping otg_multicast_app_type_names[];

extern mapping otg_distribution_names[];

extern mapping frame_type_names[];

extern mapping switch_names[] ;

extern mapping packet_gen_names[];

#endif

