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

/*! \file flexran_agent_mac_slice_verification.h
 * \brief MAC Agent slice verification helper functions
 * \author Robert Schmidt
 * \date 2018
 * \version 0.1
 */

#include "flexran_agent_common_internal.h"
#include "flexran_agent_mac_internal.h"

int flexran_verify_dl_slice(mid_t mod_id, Protocol__FlexDlSlice *dls);
int flexran_verify_group_dl_slices(mid_t mod_id, Protocol__FlexDlSlice **existing,
    int n_ex, Protocol__FlexDlSlice **update, int n_up);
int flexran_verify_ul_slice(mid_t mod_id, Protocol__FlexUlSlice *uls);
int flexran_verify_group_ul_slices(mid_t mod_id, Protocol__FlexUlSlice **existing,
    int n_ex, Protocol__FlexUlSlice **update, int n_up);
