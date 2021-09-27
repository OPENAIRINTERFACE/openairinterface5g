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

void get_num_re_dmrs(nfapi_nr_ue_pusch_pdu_t *pusch_pdu,
                     uint8_t *nb_dmrs_re_per_rb,
                     uint16_t *number_dmrs_symbols);

/** \brief Function for UE/PHY to compute PUSCH transmit power in power-control procedure.
    @param Mod_id Module id of UE
    @returns Po_NOMINAL_PUSCH (PREAMBLE_RECEIVED_TARGET_POWER+DELTA_PREAMBLE
*/
int nr_get_Po_NOMINAL_PUSCH(NR_PRACH_RESOURCES_t *prach_resources, module_id_t module_idP, uint8_t CC_id);

/** \brief Function to compute DELTA_PREAMBLE from 38.321 subclause 7.3
   (for RA power ramping procedure and Msg3 PUSCH power control policy)
    @param Mod_id Module id of UE
    @returns DELTA_PREAMBLE
*/
int8_t nr_get_DELTA_PREAMBLE(module_id_t mod_id, int CC_id, uint16_t prach_format);

/** \brief Function to compute configured maximum output power according to clause 6.2.4 of 3GPP TS 38.101-1 version 16.5.0 Release 16
    @param Mod_id Module id of UE
*/
long nr_get_Pcmax(module_id_t mod_id);
/** @}*/
