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


extern nfapi_ue_release_request_body_t release_rntis;
int oai_nfapi_rach_ind(nfapi_rach_indication_t *rach_ind);
void configure_nfapi_pnf(char *vnf_ip_addr, int vnf_p5_port, char *pnf_ip_addr, int pnf_p7_port, int vnf_p7_port);
void configure_nr_nfapi_pnf(char *vnf_ip_addr, int vnf_p5_port, char *pnf_ip_addr, int pnf_p7_port, int vnf_p7_port);

void oai_subframe_ind(uint16_t sfn, uint16_t sf);
void handle_nr_slot_ind(uint16_t sfn, uint16_t slot);
uint32_t sfnslot_add_slot(uint16_t sfn, uint16_t slot, int offset);

int oai_nfapi_nr_slot_indication(nfapi_nr_slot_indication_scf_t *ind);
int oai_nfapi_nr_rx_data_indication(nfapi_nr_rx_data_indication_t *ind);
int oai_nfapi_nr_crc_indication(nfapi_nr_crc_indication_t *ind);
int oai_nfapi_nr_srs_indication(nfapi_nr_srs_indication_t *ind);
int oai_nfapi_nr_uci_indication(nfapi_nr_uci_indication_t *ind);
int oai_nfapi_nr_rach_indication(nfapi_nr_rach_indication_t *ind);

