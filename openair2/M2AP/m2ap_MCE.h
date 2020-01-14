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

/*! \file m2ap_MCE.h
 * \brief m2ap tasks for MCE
 * \author Javier Morgade <javier.morgade@ieee.org>
 * \date 2019
 * \version 0.1
 */

#include <stdio.h>
#include <stdint.h>

/** @defgroup _m2ap_impl_ M2AP Layer Reference Implementation
 * @ingroup _ref_implementation_
 * @{
 */

#ifndef M2AP_MCE_H_
#define M2AP_MCE_H_

#include "m2ap_MCE_defs.h"


int m2ap_MCE_init_sctp (m2ap_MCE_instance_t *instance_p,
                        net_ip_address_t    *local_ip_addr,
                        uint32_t enb_port_for_M2C);

void *m2ap_MCE_task(void *arg);

int is_m2ap_MCE_enabled(void);

#endif /* M2AP_MCE_H_ */

/**
 * @}
 */
