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

/*! \file m3ap_handler.h
 * \brief m3ap handler procedures for MME
 * \author Javier Morgade <javier.morgade@ieee.org>
 * \date 2019
 * \version 0.1
 */

#ifndef M3AP_MME_HANDLERS_H_
#define M3AP_MME_HANDLERS_H_

#include "m3ap_MME_defs.h"

//void m3ap_handle_m2_setup_message(m2ap_eNB_instance_t *instance_p, m2ap_eNB_data_t *eNB_desc_p, int sctp_shutdown);

//int m3ap_eNB_handle_message(instance_t instance, uint32_t assoc_id, int32_t stream,
                            //const uint8_t * const data, const uint32_t data_length);

int m3ap_MME_handle_message(instance_t instance, uint32_t assoc_id, int32_t stream,
                            const uint8_t * const data, const uint32_t data_length);


#endif /* M3AP_MME_HANDLERS_H_ */
