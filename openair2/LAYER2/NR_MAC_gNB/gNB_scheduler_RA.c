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

/*! \file gNB_scheduler_RA.c
 * \brief primitives used for random access
 * \author
 * \date
 * \email:
 * \version
 */

#include nr_mac_gNB.h

void
schedule_RA(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP)
{

  eNB_MAC_INST *mac = RC.mac[module_idP];
  COMMON_channels_t *cc = mac->common_channels;
  RA_t *ra;
  uint8_t i;

  start_meas(&mac->schedule_ra);


  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

    for (i = 0; i < NB_RA_PROC_MAX; i++) {

	    ra = (RA_t *) & cc[CC_id].ra[i];
      //LOG_D(MAC,"RA[state:%d]\n",ra->state);

	    if (ra->state == MSG2)
        generate_Msg2(module_idP, CC_id, frameP, subframeP, ra);
	    else if (ra->state == MSG4)
        generate_Msg4(module_idP, CC_id, frameP, subframeP, ra);
	    else if (ra->state == WAITMSG4ACK)
        check_Msg4_retransmission(module_idP, CC_id, frameP, subframeP, ra);

	}			// for i=0 .. N_RA_PROC-1
    }				// CC_id

    stop_meas(&mac->schedule_ra);
}
