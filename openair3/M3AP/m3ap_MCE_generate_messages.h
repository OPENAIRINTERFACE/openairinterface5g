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

/*! \file m2ap_eNB_generate_messages.h
 * \brief m2ap procedures for eNB
 * \author Javier Morgade <javier.morgade@ieee.org>
 * \date 2019
 * \version 0.1
 */

#ifndef __M3AP_MCE_GENERATE_MESSAGES__H__
#define __M3AP_MCE_GENERATE_MESSAGES__H__

#include "m2ap_eNB_defs.h"
#include "m2ap_common.h"

int m2ap_eNB_generate_m2_setup_request(m2ap_eNB_instance_t *instance_p,
				       m2ap_eNB_data_t *m2ap_eNB_data_p);

int m2ap_MCE_generate_m2_setup_response(m2ap_eNB_instance_t *instance_p, m2ap_eNB_data_t *m2ap_eNB_data_p);

int m2ap_eNB_set_cause (M2AP_Cause_t * cause_p,
                        M2AP_Cause_PR cause_type,
                        long cause_value);

#endif /*  __M3AP_MCE_GENERATE_MESSAGES__H__ */
