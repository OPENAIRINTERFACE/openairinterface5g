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
 * Author and copyright: Laurent Thomas, open-cells.com
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

rrc_gNB_send_NGAP_NAS_FIRST_REQ() {
   // We are noCore only now
   // create message that should come from 5GC
  
   // send it dow
   rrc_gNB_process_NGAP_DOWNLINK_NAS()
}

rrc_gNB_process_NGAP_DOWNLINK_NAS () {
   do_DLInformationTransfer();
   // send it as DL data
    rrc_data_req (
      &ctxt,
      srb_id,
      (*rrc_eNB_mui)++,
      SDU_CONFIRM_NO,
      length,
      buffer,
      PDCP_TRANSMISSION_MODE_CONTROL);

}


:q

