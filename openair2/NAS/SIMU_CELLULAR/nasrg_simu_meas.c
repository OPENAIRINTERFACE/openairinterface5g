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


#include "openair_types.h"
#include "rrc_constant.h"

#include "rrc_msg_constant.h"
#include "rrc_rrm_primitives.h"
#include "rrc_msg_ies.h"

#include "nas_simu_proto.h"

extern int  rrc_rg_rrm_in_fifo;

int sim_counter=0;

/* Mobile Station */
//-------------------------------------------------------------------
void nasrg_meas_if_setup (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  // measurement identity  Intra-Frequency
  rrm_control->type = IX_ifM;
  // measurementCommand         SETUP
  rrm_control->command = MC_setup;
  rrm_control->BaseStation = 0;
  rrm_control->UE_Id = UE_Id;
  rrm_control->amount = amount16;
  rrm_control->period = int2000;
}

//-------------------------------------------------------------------
void nasrg_meas_if_release (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  // measurement identity  Intra-Frequency
  rrm_control->type = IX_ifM;
  // measurementCommand    RELEASE
  rrm_control->command = MC_release;
  rrm_control->BaseStation = 0;
  rrm_control->UE_Id = UE_Id;
}

//-------------------------------------------------------------------
void nasrg_meas_tv_setup (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  // measurement identity  Traffic Volume
  rrm_control->type = IX_tvM;
  // measurementCommand   SETUP
  rrm_control->command = MC_setup;
  rrm_control->BaseStation = 0;
  rrm_control->UE_Id = UE_Id;
  rrm_control->amount = amount_inf;
  rrm_control->period = int2000;
}

//-------------------------------------------------------------------
void nasrg_meas_tv_release (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  // measurement identity  Traffic Volume
  rrm_control->type = IX_tvM;
  // measurementCommand    RELEASE
  rrm_control->command = MC_release;
  rrm_control->BaseStation = 0;
  rrm_control->UE_Id = UE_Id;
}

//-------------------------------------------------------------------
void nasrg_meas_q_setup (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  // measurement identity  Quality
  rrm_control->type = IX_qM;
  // measurementCommand   SETUP
  rrm_control->command = MC_setup;
  rrm_control->BaseStation = 0;
  rrm_control->UE_Id = UE_Id;
  rrm_control->amount = amount2; //for test
  //rrm_control->amount = amount_inf; // for execution
  rrm_control->period = int3000;
}

//-------------------------------------------------------------------
void nasrg_meas_q_release (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  // measurement identity  Quality
  rrm_control->type = IX_qM;
  // measurementCommand    RELEASE
  rrm_control->command = MC_release;
  rrm_control->BaseStation = 0;
  rrm_control->UE_Id = UE_Id;
}

//-------------------------------------------------------------------
void nasrg_meas_int_ue_setup (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  // measurement identity  Internal UE
  rrm_control->type = IX_iueM;
  // measurementCommand   SETUP
  rrm_control->command = MC_setup;
  rrm_control->BaseStation = 0;
  rrm_control->UE_Id = UE_Id;
  rrm_control->amount = amount8;
  rrm_control->period = int1000;
}

//-------------------------------------------------------------------
void nasrg_meas_int_ue_release (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  // measurement identity  Internal UE
  rrm_control->type = IX_iueM;
  // measurementCommand    RELEASE
  rrm_control->command = MC_release;
  rrm_control->BaseStation = 0;
  rrm_control->UE_Id = UE_Id;
}


/* Base Station */
//-------------------------------------------------------------------
void nasrg_meas_bs_tv_setup (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  // measurement identity  Traffic Volume
  rrm_control->type = IX_tvbM;
  // measurementCommand   SETUP
  rrm_control->command = MC_setup;
  rrm_control->BaseStation = 1;
  rrm_control->UE_Id = UE_Id;
  rrm_control->amount = amount_inf;
  rrm_control->period = int4000;
}

//-------------------------------------------------------------------
void nasrg_meas_bs_tv_release (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  // measurement identity  Traffic Volume
  rrm_control->type = IX_tvbM;
  // measurementCommand    RELEASE
  rrm_control->command = MC_release;
  rrm_control->BaseStation = 1;
  rrm_control->UE_Id = UE_Id;
}

//-------------------------------------------------------------------
void nasrg_meas_bs_q_setup (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  // measurement identity  Quality
  rrm_control->type = IX_qbM;
  // measurementCommand   SETUP
  rrm_control->command = MC_setup;
  rrm_control->BaseStation = 1;
  rrm_control->UE_Id = UE_Id;
  rrm_control->amount = amount2;
  rrm_control->period = int3000;
}

//-------------------------------------------------------------------
void nasrg_meas_bs_q_release (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  // measurement identity  Quality
  rrm_control->type = IX_qbM;
  // measurementCommand    RELEASE
  rrm_control->command = MC_release;
  rrm_control->BaseStation = 1;
  rrm_control->UE_Id = UE_Id;
}

//-------------------------------------------------------------------
void nasrg_meas_int_bs_setup (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  // measurement identity  Internal BS
  rrm_control->type = IX_ibsM;
  // measurementCommand   SETUP
  rrm_control->command = MC_setup;
  rrm_control->BaseStation = 1;
  rrm_control->UE_Id = maxUsers;        //should not be set
  rrm_control->amount = amount16;
  rrm_control->period = int4000;
}

//-------------------------------------------------------------------
void nasrg_meas_int_bs_release (int UE_Id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  // measurement identity  Internal BS
  rrm_control->type = IX_ibsM;
  // measurementCommand    RELEASE
  rrm_control->command = MC_release;
  rrm_control->BaseStation = 1;
  rrm_control->UE_Id = maxUsers;        //should not be set
}

//-------------------------------------------------------------------
void nasrg_send_meas_request_to_rrc (int ue_id, struct rrc_rrm_measure_ctl *rrm_control)
{
  //-------------------------------------------------------------------
  rpc_message     rpc_mess;
  measurement_request meas_request;
  int count =0;

  printf ("[NASRG_MEAS_TEST] RPC_MEASUREMENT_REQUEST --> RRC\n ");
  memset ((char*)& rpc_mess, 0, sizeof (rpc_message));
  memset ((char*)& meas_request, 0, sizeof (measurement_request));
  //build header
  rpc_mess.type = RPC_MEASUREMENT_REQUEST;
  rpc_mess.length = sizeof (measurement_request);
  // build content
  meas_request.measurement_type = rrm_control->type;
  meas_request.measurement_command = rrm_control->command;
  // different conding between RRM and RRC
  // Mobile terminal RRC = 0, RRM = 1
  // Base Station    RRC = 1, RRM = 2
  meas_request.equipment_type = rrm_control->BaseStation + 1;
  meas_request.equipment_id = rrm_control->UE_Id;
  meas_request.amount = rrm_control->amount;
  meas_request.period = rrm_control->period;

  // send to RRC
  count = write(rrc_rg_rrm_in_fifo, (uint8_t *) & rpc_mess, sizeof (rpc_message));
  count += write(rrc_rg_rrm_in_fifo, (uint8_t *) & meas_request, sizeof (measurement_request));

  if (count > 0) {
    printf ("RRM message sent successfully on RRM FIFO, length: %d\n", count);
  } else {
    printf ("RRM FIFO transmit failed");
  }

#ifdef DEBUG_NAS_MEAS_SIMU
  nas_rg_print_buffer ((char *) & rpc_mess, sizeof (rpc_message));
  nas_rg_print_buffer ((char *) & meas_request, sizeof (meas_request));
#endif
}

//-------------------------------------------------------------------
//  in case not  rrc_rg_meas_loop (time,UE_Id);
int nasrg_rrm_meas_stop (int time, int UE_Id)
{
  //-------------------------------------------------------------------
  struct rrc_rrm_measure_ctl control;

  // Measurement Control  - Stop Quality previously sent by RRM
  nasrg_meas_q_release (UE_Id, &control);
  //rrc_rrm_measure_request (control);
  nasrg_send_meas_request_to_rrc (UE_Id, &control);
  return 0;
}
//-------------------------------------------------------------------
//        rrc_rg_meas_loop (time,UE_Id);
int nasrg_meas_loop (int time, int UE_Id)
{
  //-------------------------------------------------------------------
  struct rrc_rrm_measure_ctl control;

  sim_counter++;
  //  if (time % 5 == 0) {
#ifdef DEBUG_NAS_MEAS_SIMU
  printf ("\n[NASRG_MEAS_TEST] Simu Measurement Time : %d, counter : %d\n", time, sim_counter);
#endif

  //  }
  if (sim_counter == 8) {
    // Measurement Control  - Stop Quality previously sent by RRM
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] NAS sends Measurement Control (Stop Quality) message at time : %d\n\n", sim_counter);
#endif
    // setup if measurement for MT
    nasrg_meas_q_release (UE_Id, &control);
    //rrc_rrm_measure_request (control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  // Intra-Frequency
  //  if (sim_counter == 140) {
  if (sim_counter == 16) {
    // Measurement Control  - Start Intra-frequency
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] RG sends Measurement Control (Start Intra-freq) message at time : %d\n\n", sim_counter);
#endif
    // setup if measurement for MT
    nasrg_meas_if_setup (UE_Id, &control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  //  if (sim_counter == 700) {
  if (sim_counter == 70) {
    // Measurement Control  - Stop Intra-frequency
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] RG sends Measurement Control (Stop Intra-freq) message at time : %d\n\n", sim_counter);
#endif
    // setup if measurement for MT
    nasrg_meas_if_release (UE_Id, &control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  // Traffic Volume
  //  if (sim_counter == 100) {
  if (sim_counter == 12) {
    // Measurement Control  - Start Traffic Volume
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] RG sends Measurement Control (Start Traffic Volume) message at time : %d\n\n", sim_counter);
#endif
    // setup if measurement for MT
    nasrg_meas_tv_setup (UE_Id, &control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  //  if (sim_counter == 990) {
  if (sim_counter == 92) {
    // Measurement Control  - Stop Traffic Volume
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] RG sends Measurement Control (Stop Traffic Volume) message at time : %d\n\n", sim_counter);
#endif
    // setup if measurement for MT
    nasrg_meas_tv_release (UE_Id, &control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  // Quality
  //  if (sim_counter == 503) {
  if (sim_counter == 50) {
    // Measurement Control  - Start Quality
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] RG sends Measurement Control (Start Quality) message at time : %d\n\n", sim_counter);
#endif
    // setup if measurement for MT
    nasrg_meas_q_setup (UE_Id, &control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  //  if (sim_counter == 995) {
  if (sim_counter == 96) {
    // Measurement Control  - Stop Quality
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] RG sends Measurement Control (Stop Quality) message at time : %d\n\n", sim_counter);
#endif
    // setup if measurement for MT
    nasrg_meas_q_release (UE_Id, &control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  // Internal UE
  //  if (sim_counter == 210) {
  if (sim_counter == 20) {
    // Measurement Control  - Start Internal UE
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] RG sends Measurement Control (Start Int UE) message at time : %d\n\n", sim_counter);
#endif
    // setup int_ue measurement for MT
    nasrg_meas_int_ue_setup (UE_Id, &control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  //  if (sim_counter == 999) {
  if (sim_counter == 98) {
    // Measurement Control  - Stop Internal UE
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] RG sends Measurement Control (Stop Int UE) message at time : %d\n\n", sim_counter);
#endif
    nasrg_meas_int_ue_release (UE_Id, &control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  // Internal BS
  //  if (sim_counter == 105) {
  if (sim_counter == 14) {
    // Measurement Control  - Start Internal BS
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] RG starts internal Measurement (Int BS) at time : %d\n\n", sim_counter);
#endif
    nasrg_meas_int_bs_setup (UE_Id, &control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  //  if (sim_counter == 980) {
  if (sim_counter == 86) {
    // Measurement Control  - Stop Internal BS
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] RG stops internal Measurement (Int BS) at time : %d\n\n", sim_counter);
#endif
    nasrg_meas_int_bs_release (UE_Id, &control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  // Traffic Volume BS
  //  if (sim_counter == 220) {
  if (sim_counter == 22) {
    // Measurement Control  - Start Traffic Volume BS
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] RG starts internal Measurement (Traffic Volume BS) at time : %d\n\n", sim_counter);
#endif
    nasrg_meas_bs_tv_setup (UE_Id, &control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  //  if (sim_counter == 900) {
  if (sim_counter == 80) {
    // Measurement Control  - Stop Traffic Volume BS
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] RG stops internal Measurement (Traffic Volume BS) at time : %d\n\n", sim_counter);
#endif
    nasrg_meas_bs_tv_release (UE_Id, &control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  // Quality BS
  //  if (sim_counter == 625) {
  if (sim_counter == 62) {
    // Measurement Control  - Start Quality BS
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] RG starts internal Measurement (Quality BS) at time : %d\n\n", sim_counter);
#endif
    nasrg_meas_bs_q_setup (UE_Id, &control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  //  if (sim_counter == 989) {
  if (sim_counter == 90) {
    // Measurement Control  - Stop Quality BS
#ifdef DEBUG_NAS_MEAS_SIMU
    printf ("\n\n[NASRG_MEAS_TEST] RG stops internal Measurement (Quality BS) at time : %d\n\n", sim_counter);
#endif
    nasrg_meas_bs_q_release (UE_Id, &control);
    nasrg_send_meas_request_to_rrc (UE_Id, &control);
  }

  return sim_counter;
}

//-----------------------------------------------------------------------------
//void rrc_rg_print_meas_report (struct rrc_rg_mt_meas_rep *p){
void nasrg_print_meas_report (char *rrc_rrm_meas_payload, uint16_t type)
{
  //-----------------------------------------------------------------------------
  int i, j;
  char *payload[20] =
  { "pl0", "pl4", "pl8", "pl16", "pl32", "pl64", "pl128", "pl256", "pl512", "pl1024", "pl2k", "pl4k", "pl8k", "pl16k", "pl32k", "pl64k", "pl128k", "pl256k", "pl512k", "pl1024k" };
  char *average[20] = {
    "pla0", "pla4", "pla8", "pla16", "pla32", "pla64", "pla128", "pla256", "pla512", "pla1024", "pla2k",
    "pla4k", "pla8k", "pla16k", "pla32k", "pla64k", "pla128k", "pla256k", "pla512k","pla1024k"
  };
  char *variance[14] = { "plv0", "plv4", "plv8", "plv16", "plv32", "plv64", "plv128", "plv256", "plv512", "plv1024", "plv2k", "plv4k", "plv8k", "plv16k" };

  printf ("\n[NASRG-MEAS] **************  MT  Measurement Report Message   *****************\n");

  switch (type) {
  case RPC_L1_MEASUREMENT_MT_INTRA_FREQUENCY_REPORT: {
    struct rrc_rrm_meas_report_mt_if *p;
    p = (struct rrc_rrm_meas_report_mt_if *)rrc_rrm_meas_payload;
    printf ("Intra Frequency Measurement\t Number of cells: %d\n", p->if_num_cells);

    for (i = 0; i < p->if_num_cells; i++) {
      printf ("[NASRG-MEAS] Cell number: %d\tCell Identity: %d\tCell Parameters Id: %d\tP-CCPCH RSCP: %d\tPathloss: %d\n", i, p->if_cell_id[i], p->if_cell_parms_id[i], p->if_BCH_RSCP[i], p->if_pathloss[i]);
      printf ("\tTimeslot ISCP for each slot:\t");

      for (j = 0; j < JRRM_SLOTS_PER_FRAME; j++)
        printf ("%d: %d, ", j, p->if_slot_iscp[i][j]);

      printf ("\n");
    }
  }
  break;

  case RPC_L1_MEASUREMENT_MT_TRAFFIC_VOLUME_REPORT: {
    struct rrc_rrm_meas_report_mt_tv *p;
    p = (struct rrc_rrm_meas_report_mt_tv *)rrc_rrm_meas_payload;
    printf ("Traffic Volume Measurement -\t Number of RBs: %d\n", p->tv_num_rbs);

    for (i = 0; i < p->tv_num_rbs; i++)
      printf ("[NASRG-MEAS] RB_Id : %d\t, RLC_BufferPayload : %s\t, Average : %s\t, Variance : %s\n",
              p->tv_rbid[i], payload[p->tv_rb_payload[i]], average[p->tv_rb_average[i]], variance[p->tv_rb_variance[i]]);
  }
  break;

  case RPC_L1_MEASUREMENT_MT_QUALITY_REPORT: {
    struct rrc_rrm_meas_report_mt_q *p;
    p = (struct rrc_rrm_meas_report_mt_q *)rrc_rrm_meas_payload;
    printf ("Quality Measurement -\t Number of Transport Channels : %d, Number of TFCS : %d\n", p->q_num_TrCH, p->q_num_tfcs);
    printf ("[NASRG-MEAS] BLER for each Transport Channel:\t");

    for (i = 0; i < p->q_num_TrCH; i++)
      printf ("%d: %d, ", p->q_dl_TrCH_id[i], p->q_dl_TrCH_BLER[i]);

    printf ("\n");
    printf ("[NASRG-MEAS] SIR for each TFCS for each slot:\n");

    for (i = 0; i < p->q_num_tfcs; i++) {
      printf ("%d: ", p->q_tfcs_id[i]);

      for (j = 0; j < JRRM_SLOTS_PER_FRAME; j++)
        printf ("%d, ", p->q_sir[i][j]);

      printf ("\n");
    }
  }
  break;

  case RPC_L1_MEASUREMENT_MT_INTERNAL_REPORT: {
    struct rrc_rrm_meas_report_mt_int *p;
    p = (struct rrc_rrm_meas_report_mt_int *)rrc_rrm_meas_payload;
    printf ("UE internal Measurement\t Timing Advance: %d\n", p->int_timing_advance);
    printf ("[NASRG-MEAS] Transmitted Power for each slot:\t");

    for (j = 0; j < JRRM_SLOTS_PER_FRAME; j++)
      printf ("%d: %d, ", j, p->int_xmit_power[j]);

    printf ("\n");
  }
  break;
  }

  printf ("[NASRG-MEAS] *****END*********    Measurement Report Message   *******\n\n");
}

//-----------------------------------------------------------------------------
void nasrg_print_bs_meas_report (char *rrc_rrm_meas_payload, uint16_t type)
{
  //-----------------------------------------------------------------------------
  int i, j;
  char *payload[20] =
  { "pl0", "pl4", "pl8", "pl16", "pl32", "pl64", "pl128", "pl256", "pl512", "pl1024", "pl2k", "pl4k", "pl8k", "pl16k", "pl32k", "pl64k", "pl128k", "pl256k", "pl512k", "pl1024k" };
  char *average[20] = {
    "pla0", "pla4", "pla8", "pla16", "pla32", "pla64", "pla128", "pla256", "pla512", "pla1024", "pla2k",
    "pla4k", "pla8k", "pla16k", "pla32k", "pla64k", "pla128k", "pla256k", "pla512k", "pla1024k"
  };
  char *variance[14] = { "plv0", "plv4", "plv8", "plv16", "plv32", "plv64", "plv128", "plv256", "plv512", "plv1024", "plv2k", "plv4k", "plv8k", "plv16k" };

  printf ("\n[NASRG-MEAS] **************  BS  Measurement Report Message   *****************\n");

  switch (type) {
  case RPC_L1_MEASUREMENT_RG_TRAFFIC_VOLUME_REPORT: {
    struct rrc_rrm_meas_report_bs_tv *p;
    p = (struct rrc_rrm_meas_report_bs_tv *)rrc_rrm_meas_payload;
    printf ("Traffic Volume Measurement -\t Number of RBs: %d\n", p->tv_num_rbs);

    for (i = 0; i < p->tv_num_rbs; i++)
      printf ("[NASRG-MEAS] RB_Id : %d\t, RLC_BufferPayload : %s\t, Average : %s\t, Variance : %s\n",
              p->tv_rbid[i], payload[p->tv_rb_payload[i]], average[p->tv_rb_average[i]], variance[p->tv_rb_variance[i]]);
  }
  break;

  case RPC_L1_MEASUREMENT_RG_QUALITY_REPORT: {
    struct rrc_rrm_meas_report_bs_q *p;
    p = (struct rrc_rrm_meas_report_bs_q *)rrc_rrm_meas_payload;
    printf ("Quality Measurement -\t Number of Transport Channels : %d, Number of TFCS : %d\n", p->q_num_TrCH, p->q_num_tfcs);
    printf ("[NASRG-MEAS] BLER for each Transport Channel:\t");

    for (i = 0; i < p->q_num_TrCH; i++)
      printf ("%d: %d, ", p->q_dl_TrCH_id[i], p->q_dl_TrCH_BLER[i]);

    printf ("\n");
    printf ("[NASRG-MEAS] SIR for each TFCS for each slot:\n");

    for (i = 0; i < p->q_num_tfcs; i++) {
      printf ("%d: ", p->q_tfcs_id[i]);

      for (j = 0; j < JRRM_SLOTS_PER_FRAME; j++)
        printf ("%d, ", p->q_sir[i][j]);

      printf ("\n");
    }
  }
  break;

  case RPC_L1_MEASUREMENT_RG_INTERNAL_REPORT: {
    struct rrc_rrm_meas_report_bs_int *p;
    p = (struct rrc_rrm_meas_report_bs_int *)rrc_rrm_meas_payload;
    printf ("BS internal Measurement\n");

    for (i = 0; i < numANTENNAS; i++) {
      printf ("[NASRG-MEAS] Transmitted Power for each slot on antenna %d :  ", i);

      for (j = 0; j < JRRM_SLOTS_PER_FRAME; j++)
        printf ("%d: %d, ", j, p->int_xmit_power[i][j]);

      printf ("\n");
    }

    for (i = 0; i < JRRM_SLOTS_PER_FRAME; i++) {
      printf ("[NASRG-MEAS] RSCP for each channel on slot %d :  ", i);

      for (j = 0; j < MAXCH; j++)
        printf ("%d: %d, ", j, p->int_rscp[i][j]);

      printf ("\n");
    }

    for (i = 0; i < numANTENNAS; i++) {
      printf ("[NASRG-MEAS] RSSI RF for each slot on antenna %d :  ", i);

      for (j = 0; j < JRRM_SLOTS_PER_FRAME; j++)
        printf ("%d: %d, ", j, p->int_rssi_rf[i][j]);

      printf ("\n");
    }

    for (i = 0; i < NUMSPARE; i++) {
      printf ("[NASRG-MEAS] Spare value : %d: %d\n", i, p->int_spare[i]);
    }
  }
  break;
  }

  printf ("[NASRG-MEAS] *****END*********    Measurement Report Message   *******\n\n");
}

