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
 
/*! \file ngap_gNB.h
 * \brief NGAP gNB task
 * \author  Yoshio INOUE, Masayuki HARADA
 * \date 2020
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \version 1.0
 * @ingroup _ngap
 */

#include <stdio.h>
#include <stdint.h>

/** @defgroup _ngap_impl_ NGAP Layer Reference Implementation for gNB
 * @ingroup _ref_implementation_
 * @{
 */

#ifndef NGAP_GNB_H_
#define NGAP_GNB_H_

typedef struct ngap_gNB_config_s {
  // MME related params
  unsigned char amf_enabled;          ///< AMF enabled ?
  unsigned char ngap_enabled;          ///< NGAP enabled ?
} ngap_gNB_config_t;

extern ngap_gNB_config_t ngap_config;

#define AMF_MODE_ENABLED       ngap_config.amf_enabled
#define NGAP_CONF_MODE         ngap_config.ngap_enabled


void *ngap_gNB_process_itti_msg(void*);
void  ngap_gNB_init(void);
void *ngap_gNB_task(void *arg);

uint32_t ngap_generate_gNB_id(void);

#endif /* NGAP_GNB_H_ */

/**
 * @}
 */
