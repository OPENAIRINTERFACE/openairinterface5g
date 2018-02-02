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

#ifndef OSA_INTERNAL_H_
#define OSA_INTERNAL_H_

#define FC_KENB         (0x11)
#define FC_NH           (0x12)
#define FC_KENB_STAR    (0x13)
/* 33401 #A.7 Algorithm for key derivation function.
 * This FC should be used for:
 * - NAS Encryption algorithm
 * - NAS Integrity algorithm
 * - RRC Encryption algorithm
 * - RRC Integrity algorithm
 * - User Plane Encryption algorithm
 */
#define FC_ALG_KEY_DER  (0x15)
#define FC_KASME_TO_CK  (0x16)

#ifndef hton_int32
# define hton_int32(x)   \
(((x & 0x000000FF) << 24) | ((x & 0x0000FF00) << 8) |  \
((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24))
#endif

// #define SECU_DEBUG

#endif /* OSA_INTERNAL_H_ */
