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
/*! \file ngap_gNB_default_values.h
 * \brief ngap default values
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \date 2020
 * \version 0.1
 */

#ifndef NGAP_GNB_DEFAULT_VALUES_H_
#define NGAP_GNB_DEFAULT_VALUES_H_

#define GNB_TAC (1)
#define GNB_MCC (208)
#define GNB_MNC (92)

#define GNB_NAME        "Eurecom GNB"
#define GNB_NAME_FORMAT (GNB_NAME" %u")

#define NGAP_PORT_NUMBER        (38412)
#define NGAP_SCTP_PPID          (60)

#define X2AP_PORT_NUMBER        (36422)
#define X2AP_SCTP_PPID          (27)

#endif /* NGAP_GNB_DEFAULT_VALUES_H_ */
