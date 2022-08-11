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

#ifndef UTILS_CONFIG_H_ASN1
#define UTILS_CONFIG_H_ASN1

// This is hard coded file name "config.h" and HAVE_CONFIG_H_ in asn1c skeletons

/*
 * This file "config.h" will be used by asn1c if HAVE_CONFIG_H_ is defined and
 * included. This logs asn1c encoder and decoder traces at execution time using
 * the regular OAI logging system, i.e., LOG_I(ASN...);
 *
 * As it is very verbose, note that you can change the log level per module in
 * source or in gdb, e.g., to only activate it for a short time.
 *
 * In code:
 * ```
 * set_log(ASN, OAI_INFO); // enable logging
 * // do your encoding here
 * set_log(ASN, OAI_ERR);  // disable logging
 * ```
 *
 * in gdb:
 * ```
 * gdb> p set_log(ASN, 1) // disable log, 1 == OAI_ERR
 * gdb> p set_log(ASN, 5) // enable log,  5 == OAI_INFO
 * ```
 */

#include "common/utils/LOG/log.h"
#if DEBUG_ASN1C
#define ASN_DEBUG(x...) do{ LOG_I(ASN,x);LOG_I(ASN,"\n"); } while(false)
#else
#define ASN_DEBUG(x...)
#endif

#endif /* UTILS_CONFIG_H_ASN1 */
