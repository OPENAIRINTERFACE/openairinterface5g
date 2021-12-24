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

#ifndef __UTIL_OTG_TRAFFIC_CONFIG__H__
#define __UTIL_OTG_TRAFFIC_CONFIG__H__

//IDT DISTRIBUTION PARAMETERS
#define IDT_DIST GAUSSIAN
#define IDT_MIN 2
#define IDT_MAX 10
#define IDT_STD_DEV 1
#define IDT_LAMBDA 3

//TRANSPORT PROTOCOL
#define TRANS_PROTO TCP
#define IP_V IPV4

//DATA PACKET SIZE DISTRIBUTION PARAMETERS
#define PKTS_SIZE_DIST POISSON
#define PKTS_SIZE_MIN 17
#define PKTS_SIZE_MAX 1500
#define PKTS_SIZE_STD_DEV 30
#define PKTS_SIZE_LAMBDA 500

//SOCKET MODE
#define DST_PORT 1234;
#define DST_IP "127.0.0.1"

#endif
