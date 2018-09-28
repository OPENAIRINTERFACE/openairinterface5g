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

#ifndef X2AP_ENB_GENERATE_MESSAGES_H_
#define X2AP_ENB_GENERATE_MESSAGES_H_

#include "x2ap_eNB_defs.h"
#include "x2ap_common.h"

int x2ap_eNB_generate_x2_setup_request(x2ap_eNB_instance_t *instance_p,
				       x2ap_eNB_data_t *x2ap_enb_data_p);

int x2ap_eNB_generate_x2_setup_response(x2ap_eNB_data_t *x2ap_enb_data_p);

int x2ap_eNB_generate_x2_setup_failure(instance_t instance,
                                       uint32_t assoc_id,
                                       X2AP_Cause_PR cause_type,
                                       long cause_value,
                                       long time_to_wait);

int x2ap_eNB_set_cause (X2AP_Cause_t * cause_p,
                        X2AP_Cause_PR cause_type,
                        long cause_value);

#endif /*  X2AP_ENB_GENERATE_MESSAGES_H_ */

