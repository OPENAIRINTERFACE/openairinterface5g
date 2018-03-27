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

#include "../defs_NR.h"

int phy_init_nr_gNB(nfapi_param_t* nfapi_params)
{

  nfapi_params->pnf_phy_rel15.number_of_phys = 1;
  nfapi_params->pnf_phy_rel15.phy[0].mu =1;
  nfapi_params->subframe_config.duplex_mode.value = 1; //FDD
  nfapi_params->subframe_config.dl_cyclic_prefix_type.value = 0; //NORMAL
  nfapi_params->rf_config.dl_channel_bandwidth.value = 106;
  nfapi_params->rf_config.ul_channel_bandwidth.value = 106;

  return 0;
}
