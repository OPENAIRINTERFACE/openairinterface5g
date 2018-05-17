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

#ifndef __NR_TRANSPORT__H__
#define __NR_TRANSPORT__H__

#include "PHY/defs_gNB.h"


/*!
\fn int nr_generate_pss
\brief Generation of the NR PSS
@param
@returns 0 on success
 */
int nr_generate_pss(  int16_t *d_pss,
                      int32_t **txdataF,
                      int16_t amp,
                      uint8_t ssb_start_symbol,
                      nfapi_config_request_t* config,
                      NR_DL_FRAME_PARMS *frame_parms);

/*!
\fn int nr_generate_sss
\brief Generation of the NR SSS
@param
@returns 0 on success
 */
int nr_generate_sss(  int16_t *d_sss,
                      int32_t **txdataF,
                      int16_t amp,
                      uint8_t ssb_start_symbol,
                      nfapi_config_request_t* config,
                      NR_DL_FRAME_PARMS *frame_parms);

/*!
\fn int nr_generate_pbch_dmrs
\brief Generation of the DMRS for the PBCH
@param
@returns 0 on success
 */
int nr_generate_pbch_dmrs(uint32_t *gold_pbch_dmrs,
                          int32_t **txdataF,
                          int16_t amp,
                          uint8_t ssb_start_symbol,
                          uint8_t nu,
                          nfapi_config_request_t* config,
                          NR_DL_FRAME_PARMS *frame_parms);

/*!
\fn int nr_generate_pbch
\brief Generation of the PBCH
@param
@returns 0 on success
 */
int nr_generate_pbch(NR_gNB_PBCH *pbch,
                     uint8_t *pbch_pdu,
                     int32_t **txdataF,
                     int16_t amp,
                     uint8_t ssb_start_symbol,
                     uint8_t nu,
                     nfapi_config_request_t* config,
                     NR_DL_FRAME_PARMS *frame_parms);

#endif /*__NR_TRANSPORT__H__*/
