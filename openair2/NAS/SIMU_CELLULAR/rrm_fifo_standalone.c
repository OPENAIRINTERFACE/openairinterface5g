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

#include <errno.h>
#include <sys/time.h>
#include <ctype.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
//
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
//-----------------------------------------------------------------------------
#include "openair_types.h"
#include "rrc_constant.h"
#include "nas_simu_proto.h"

#include "rrc_rrm_sap.h"
#include "rrc_msg_constant.h"
#include "rrc_rrm_primitives.h"

void nasrg_meas_q_setup (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control);
void nasrg_meas_q_release (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control);
void nasrg_send_meas_request_to_rrc (int ue_id, struct rrc_rrm_measure_ctl *rrm_control);
//-----------------------------------------------------------------------------
int  rrc_rg_rrm_in_fifo;
int  rrc_rg_rrm_out_fifo;
int  tXmit_id;
#define RRM_MSG_MAX_LENGTH 1500
struct rrc_rrm_measure_ctl control;

//-----------------------------------------------------------------------------
// Open FIFOs for RG RRC SAPs
void nasrg_rrm_fifos_init (void)
{
  //-----------------------------------------------------------------------------
  int write_flag = O_WRONLY | O_NONBLOCK | O_NDELAY;
  int read_flag = O_RDONLY | O_NONBLOCK | O_NDELAY;

  // Open FIFOs

  while ((rrc_rg_rrm_in_fifo = open (RRM_INPUT_SAPI, write_flag)) < 0) {
    printf ("%s returned value %d\n", RRM_INPUT_SAPI, rrc_rg_rrm_in_fifo);
    fflush(stdout);
    sleep (1);
  }

  while ((rrc_rg_rrm_out_fifo = open (RRM_OUTPUT_SAPI, read_flag)) < 0) {
    printf ("%s returned value %d\n", RRM_OUTPUT_SAPI, rrc_rg_rrm_out_fifo);
    fflush(stdout);
    sleep (1);
  }

  printf ("FIFO RRM_INPUT_SAPI, %d\n", rrc_rg_rrm_in_fifo);
  printf ("FIFO RRM_OUTPUT_SAPI, %d\n", rrc_rg_rrm_out_fifo);
}

//-------------------------------------------------------------------
void rrm_connection_response (void)
{
  //-------------------------------------------------------------------
  rpc_message     rpc_mess;
  connection_response response;
  int count=0;
  char *sim_data = "Void configuration\0";

  printf ("[CELL-RRM] RPC_CONNECTION_RESPONSE --> RRC\n ");
  rpc_mess.type = RPC_CONNECTION_RESPONSE;
  rpc_mess.length = sizeof (connection_response)+ strlen (sim_data);
  response.equipment_id = 0;
  response.status = STATUS_CONNECTION_ACCEPTED;

  //count = rtf_put (protocol_bs->rrc.rc_rrm.output_fifo, (uint8_t *) & rpc_mess, sizeof (rpc_message));
  //count = rtf_put (protocol_bs->rrc.rc_rrm.output_fifo, (uint8_t *) & confirm, sizeof (add_user_confirm));
  count = write(rrc_rg_rrm_in_fifo, (uint8_t *) & rpc_mess, sizeof (rpc_message));
  count += write(rrc_rg_rrm_in_fifo, (uint8_t *) & response, sizeof (connection_response));
  count += write(rrc_rg_rrm_in_fifo, (uint8_t *) sim_data, strlen (sim_data));

  if (count > 0) {
    printf ("RRM message sent successfully on RRM FIFO, length: %d\n", count);
  } else {
    printf ("RRM FIFO transmit failed");
  }
}

//-------------------------------------------------------------------
void rrm_add_user_response (char *rcve_buffer)
{
  //-------------------------------------------------------------------
  rpc_message     rpc_mess;
  add_user_response response;
  add_user_request *request;
  int count=0;
  char *sim_data = "Void configuration\0";

  request = (add_user_request *)rcve_buffer;
  printf ("[CELL-RRM] RPC_ADD_USER_RESPONSE --> RRC\n ");
  rpc_mess.type = RPC_ADD_USER_RESPONSE;
  rpc_mess.length = sizeof (add_user_response)+ strlen (sim_data);
  response.equipment_id = 0;
  response.user_id = request->user_id;
  response.tx_id = request->tx_id;
  response.status = ADD_USER_SUCCESSFUL;

  //count = rtf_put (protocol_bs->rrc.rc_rrm.output_fifo, (uint8_t *) & rpc_mess, sizeof (rpc_message));
  //count = rtf_put (protocol_bs->rrc.rc_rrm.output_fifo, (uint8_t *) & confirm, sizeof (add_user_confirm));
  count = write(rrc_rg_rrm_in_fifo, (uint8_t *) & rpc_mess, sizeof (rpc_message));
  count += write(rrc_rg_rrm_in_fifo, (uint8_t *) & response, sizeof (add_user_response));
  count += write(rrc_rg_rrm_in_fifo, (uint8_t *) sim_data, strlen (sim_data));

  if (count > 0) {
    printf ("RRM message sent successfully on RRM FIFO, length: %d\n", count);
  } else {
    printf ("RRM FIFO transmit failed");
  }
}

//-------------------------------------------------------------------
void rrm_remove_user_response (char *rcve_buffer)
{
  //-------------------------------------------------------------------
  rpc_message     rpc_mess;
  remove_user_response response;
  remove_user_request *request;
  int count=0;
  char *sim_data = "Void configuration\0";

  request = (remove_user_request *)rcve_buffer;
  printf ("[CELL-RRM] RPC_REMOVE_USER_RESPONSE --> RRC\n ");
  rpc_mess.type = RPC_REMOVE_USER_RESPONSE;
  rpc_mess.length = sizeof (remove_user_response)+ strlen (sim_data);
  response.equipment_id = 0;
  response.user_id = request->user_id;
  response.tx_id = request->tx_id;
  response.status = REMOVE_USER_SUCCESSFUL;

  //count = rtf_put (protocol_bs->rrc.rc_rrm.output_fifo, (uint8_t *) & rpc_mess, sizeof (rpc_message));
  //count = rtf_put (protocol_bs->rrc.rc_rrm.output_fifo, (uint8_t *) & confirm, sizeof (add_user_confirm));
  count = write(rrc_rg_rrm_in_fifo, (uint8_t *) & rpc_mess, sizeof (rpc_message));
  count += write(rrc_rg_rrm_in_fifo, (uint8_t *) & response, sizeof (remove_user_response));
  count += write(rrc_rg_rrm_in_fifo, (uint8_t *) sim_data, strlen (sim_data));

  if (count > 0) {
    printf ("RRM message sent successfully on RRM FIFO, length: %d\n", count);
  } else {
    printf ("RRM FIFO transmit failed");
  }

  /* Not sure it is really needed here
  usleep (5000); //wait 5ms
  // stop q measurement for MT
  nasrg_meas_q_release (UE_Id, &control);
  nasrg_send_meas_request_to_rrc (UE_Id, &control);
  */
}

//-------------------------------------------------------------------
void rrm_add_radio_access_bearer_response (char *rcve_buffer)
{
  //-------------------------------------------------------------------
  rpc_message     rpc_mess;
  add_radio_access_bearer_response response;
  add_radio_access_bearer_request *request;
  int count=0;
  char *sim_data = "Void configuration\0";

  request = (add_radio_access_bearer_request *)rcve_buffer;
  printf ("[CELL-RRM] RPC_ADD_RADIO_ACCESS_BEARER_RESPONSE --> RRC\n ");
  rpc_mess.type = RPC_ADD_RADIO_ACCESS_BEARER_RESPONSE;
  rpc_mess.length = sizeof (add_radio_access_bearer_response)+ strlen (sim_data);
  response.user_id = request->user_id;
  response.rab_id = request->rab_id;
  response.tx_id = request->tx_id;
  response.status = ADD_RADIO_ACCESS_BEARER_SUCCESSFUL;

  //count = rtf_put (protocol_bs->rrc.rc_rrm.output_fifo, (uint8_t *) & rpc_mess, sizeof (rpc_message));
  //count = rtf_put (protocol_bs->rrc.rc_rrm.output_fifo, (uint8_t *) & confirm, sizeof (add_user_confirm));
  count = write(rrc_rg_rrm_in_fifo, (uint8_t *) & rpc_mess, sizeof (rpc_message));
  count += write(rrc_rg_rrm_in_fifo, (uint8_t *) & response, sizeof (add_radio_access_bearer_response));
  count += write(rrc_rg_rrm_in_fifo, (uint8_t *) sim_data, strlen (sim_data));

  if (count > 0) {
    printf ("RRM message sent successfully on RRM FIFO, length: %d\n", count);
  } else {
    printf ("RRM FIFO transmit failed");
  }
}

//-------------------------------------------------------------------
void rrm_add_measurement_request (char *rcve_buffer)
{
  //-------------------------------------------------------------------
  add_radio_access_bearer_confirm *confirm;

  confirm = (add_radio_access_bearer_confirm *)rcve_buffer;
  printf ("[CELL-RRM] RPC_L1_MEASUREMENT_MT_QUALITY_REPORT --> RRC\n ");

  // TEMP removed the Meas request (causes bothe messages to be received together by RRC RG
  //   usleep (20000); //wait 5ms
  // setup q measurement for MT
  nasrg_meas_q_setup (confirm->user_id, &control);
  nasrg_send_meas_request_to_rrc (confirm->user_id, &control);

}


//-------------------------------------------------------------------
void rrm_remove_radio_access_bearer_response (char *rcve_buffer)
{
  //-------------------------------------------------------------------
  rpc_message     rpc_mess;
  remove_radio_access_bearer_response response;
  remove_radio_access_bearer_request *request;
  int count=0;
  char *sim_data = "Void configuration\0";

  request = (remove_radio_access_bearer_request *)rcve_buffer;
  printf ("[CELL-RRM] RPC_REMOVE_RADIO_ACCESS_BEARER_RESPONSE --> RRC\n ");
  rpc_mess.type = RPC_REMOVE_RADIO_ACCESS_BEARER_RESPONSE;
  rpc_mess.length = sizeof (remove_radio_access_bearer_response)+ strlen (sim_data);
  response.user_id = request->user_id;
  response.rab_id = request->rab_id;
  response.tx_id = request->tx_id;
  response.status = REMOVE_RADIO_ACCESS_BEARER_SUCCESSFUL;

  //count = rtf_put (protocol_bs->rrc.rc_rrm.output_fifo, (uint8_t *) & rpc_mess, sizeof (rpc_message));
  //count = rtf_put (protocol_bs->rrc.rc_rrm.output_fifo, (uint8_t *) & confirm, sizeof (add_user_confirm));
  count = write(rrc_rg_rrm_in_fifo, (uint8_t *) & rpc_mess, sizeof (rpc_message));
  count += write(rrc_rg_rrm_in_fifo, (uint8_t *) & response, sizeof (remove_radio_access_bearer_response));
  count += write(rrc_rg_rrm_in_fifo, (uint8_t *) sim_data, strlen (sim_data));

  if (count > 0) {
    printf ("RRM message sent successfully on RRM FIFO, length: %d\n", count);
  } else {
    printf ("RRM FIFO transmit failed");
  }

  usleep (20000); //wait 5ms
  // stop q measurement for MT
  nasrg_meas_q_release (request->user_id, &control);
  nasrg_send_meas_request_to_rrc (request->user_id, &control);
}


//-----------------------------------------------------------------------------
// Check if anything in RRC FIFO and send it to RRM
void nasrg_rrm_from_rrc_read (void)
{
  //-----------------------------------------------------------------------------
  char rcve_buffer[RRM_MSG_MAX_LENGTH];
  int  rpc_header_size=0;
  int  count = 0;
  rpc_message *rrc_rrm_mess;
  char *rrc_rrm_meas_payload;

  memset (rcve_buffer, 0, RRM_MSG_MAX_LENGTH - 1);
  rpc_header_size = sizeof (rpc_message);
  rrc_rrm_mess = (rpc_message *) rcve_buffer;

  if ((count = read (rrc_rg_rrm_out_fifo, rcve_buffer, rpc_header_size)) > 0) {
    //printf("\n RRM FIFO %d, bytes count %d", rrc_rg_rrm_out_fifo, count);
#ifdef DEBUG_RRC_RRM_INTF
    printf ("\n [DEBUG]Message Received in RRM FIFO %d , length %d, type %d \n", rrc_rg_rrm_out_fifo, rrc_rrm_mess->length, rrc_rrm_mess->type);
#endif
    //get the rest of the primitive
    count += read (rrc_rg_rrm_out_fifo, &(rcve_buffer[rpc_header_size]), rrc_rrm_mess->length );

    switch (rrc_rrm_mess->type) {
    case RPC_CONNECTION_REQUEST:
      printf ("\n[CELL-RRM]Received RPC_CONNECTION_REQUEST, length %d\n", rrc_rrm_mess->length);
      rrm_connection_response();
      break;

    case RPC_ADD_USER_REQUEST:
      printf ("\n[CELL-RRM]Received RPC_ADD_USER_REQUEST, length %d\n", rrc_rrm_mess->length);
      rrm_add_user_response(&(rcve_buffer[rpc_header_size]));
      break;

    case RPC_REMOVE_USER_REQUEST:
      printf ("\n[CELL-RRM]Received RPC_REMOVE_USER_REQUEST, length %d\n", rrc_rrm_mess->length);
      rrm_remove_user_response(&(rcve_buffer[rpc_header_size]));
      break;

    case RPC_ADD_RADIO_ACCESS_BEARER_REQUEST:
      printf ("\n[CELL-RRM]Received RPC_ADD_RADIO_ACCESS_BEARER_REQUEST, length %d\n", rrc_rrm_mess->length);
      rrm_add_radio_access_bearer_response(&(rcve_buffer[rpc_header_size]));
      break;

    case RPC_REMOVE_RADIO_ACCESS_BEARER_REQUEST:
      printf ("\n[CELL-RRM]Received RPC_REMOVE_RADIO_ACCESS_BEARER_REQUEST, length %d\n", rrc_rrm_mess->length);
      rrm_remove_radio_access_bearer_response(&(rcve_buffer[rpc_header_size]));
      break;

    case RPC_ADD_USER_CONFIRM:
      printf ("\n[CELL-RRM]Received RPC RPC_ADD_USER_CONFIRM , length %d\n", rrc_rrm_mess->length);
      break;

    case RPC_ADD_RADIO_ACCESS_BEARER_CONFIRM:
      printf ("\n[CELL-RRM]Received RPC RPC_ADD_RADIO_ACCESS_BEARER_CONFIRM , length %d\n", rrc_rrm_mess->length);
      //rrm_add_measurement_request(&(rcve_buffer[rpc_header_size]));
      break;

    case RPC_L1_MEASUREMENT_RG_INTERNAL_REPORT:
    case RPC_L1_MEASUREMENT_RG_QUALITY_REPORT:
    case RPC_L1_MEASUREMENT_RG_TRAFFIC_VOLUME_REPORT:
      rrc_rrm_meas_payload = (char *)rcve_buffer +12; //12 = rpc_header + equipment_id
      //nas_rg_print_buffer (rrc_rrm_meas_payload, rrc_rrm_mess->length);
      nasrg_print_bs_meas_report (rrc_rrm_meas_payload,rrc_rrm_mess->type);
      break;

    case RPC_L1_MEASUREMENT_MT_INTERNAL_REPORT:
    case RPC_L1_MEASUREMENT_MT_QUALITY_REPORT:
    case RPC_L1_MEASUREMENT_MT_TRAFFIC_VOLUME_REPORT:
    case RPC_L1_MEASUREMENT_MT_INTRA_FREQUENCY_REPORT:
      rrc_rrm_meas_payload = (char *)rcve_buffer +12; //12 = rpc_header + equipment_id
      //nas_rg_print_buffer (rrc_rrm_meas_payload, rrc_rrm_mess->length);
      nasrg_print_meas_report (rrc_rrm_meas_payload,rrc_rrm_mess->type);
      break;

    default:
      break;
    }

    fflush(stdout);
  }
}


//-----------------------------------------------------------------------------
int main (int argc, char **argv)
{
  //-----------------------------------------------------------------------------
  int time = 0;
  //     int rc, sd_rrm;
  //     fd_set readfds;
  //     struct timeval tv;
  tXmit_id =0;
  nasrg_rrm_fifos_init ();
  printf ("[RRM] RRM FIFOs ready\n");
  fflush(stdout);

  while (1) {
    usleep (100000);
    time ++;
    //printf ("\n[RRM_TEST] Simu Measurement Time at Main: %d\n", time);

    // check RRM FIFO
    nasrg_rrm_from_rrc_read();
  }
}
