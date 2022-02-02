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

#include "nr_sdap_gnb.h"
#include <openair2/LAYER2/PDCP_v10.1.0/pdcp.h>

boolean_t sdap_gnb_data_req(protocol_ctxt_t *ctxt_p,
                            const srb_flag_t srb_flag,
                            const rb_id_t rb_id,
                            const mui_t mui,
                            const confirm_t confirm,
                            const sdu_size_t sdu_buffer_size,
                            unsigned char *const sdu_buffer,
                            const pdcp_transmission_mode_t pt_mode,
                            const uint32_t *sourceL2Id,
                            const uint32_t *destinationL2Id
                           ) {
  if(sdu_buffer == NULL) {
    LOG_E(PDCP, "%s:%d:%s: SDAP Layer gNB - NULL sdu_buffer \n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  if(sdu_buffer_size == 0) {
    LOG_E(PDCP, "%s:%d:%s: SDAP Layer gNB - NULL or 0 sdu_buffer_size \n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  uint8_t *sdap_buf = (uint8_t *)malloc(sdu_buffer_size+SDAP_HDR_LENGTH);
  nr_sdap_dl_hdr_t sdap_hdr;
  sdap_hdr.RDI = 0; // SDAP_Hardcoded -
  sdap_hdr.RQI = 0; // SDAP_Hardcoded - Should get this info from DL_PDU_SESSION_INFORMATION
  sdap_hdr.QFI = 1; // SDAP_Hardcoded - Should get this info from DL_PDU_SESSION_INFORMATION
  memcpy(&sdap_buf[0], &sdap_hdr, 1);
  memcpy(&sdap_buf[1], sdu_buffer, sdu_buffer_size);
  rb_id_t sdap_drb_id = rb_id; // SDAP_Hardcoded - Should get this info from QFI to DRB mapping table
  boolean_t ret = pdcp_data_req(ctxt_p,
                                srb_flag,
                                sdap_drb_id,
                                mui,
                                confirm,
                                sdu_buffer_size+1,
                                sdap_buf,
                                pt_mode,
                                sourceL2Id,
                                destinationL2Id);

  if(!ret) {
    LOG_E(PDCP, "%s:%d:%s: SDAP Layer gNB - PDCP DL refused PDU\n", __FILE__, __LINE__, __FUNCTION__);
    free(sdap_buf);
    return 0;
  }

  free(sdap_buf);
  return 1;
}

void sdap_gnb_ul_header_handler(char sdap_gnb_ul_hdr) {
  nr_sdap_ul_hdr_t *sdap_hdr_ul = (nr_sdap_ul_hdr_t *)&sdap_gnb_ul_hdr;

  switch (sdap_hdr_ul->DC) {
    case SDAP_HDR_UL_DATA_PDU:
      LOG_I(PDCP, "%s:%d:%s: SDAP Layer gNB - UL Received SDAP Data PDU\n", __FILE__, __LINE__, __FUNCTION__);
      break;

    case SDAP_HDR_UL_CTRL_PDU:
      LOG_I(PDCP, "%s:%d:%s: SDAP Layer gNB - Received SDAP Control PDU\n", __FILE__, __LINE__, __FUNCTION__);
      break;
  }
}