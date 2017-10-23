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

#ifndef __NAS_CONTROL_H__
#define __NAS_CONTROL_H__

#ifdef NODE_MT
//-----------------------------------------------------------------------------
void nas_ue_control_init (void);
//int nas_ue_DC_attach(void);
//int nas_ue_DC_attach_complete(void);
int  nas_ue_GC_Rcve_FIFO (void);
int  nas_ue_NT_Rcve_FIFO (void);
int  nas_ue_DC_Rcve_FIFO (void);
#endif

#ifdef NODE_RG
//-----------------------------------------------------------------------------
void nas_rg_control_init (void);
//int nas_rg_DC_ConnectMs(void) ;
int  nas_rg_DC_Rcve_FIFO (int time);

int nasrg_rrm_socket_init(void);
void nasrg_rrm_fifos_init (void);
void nasrg_rrm_to_rrc_write (void);
void nasrg_rrm_from_rrc_read (void);

int nasrg_meas_loop (int time, int UE_Id);
void nas_rg_print_buffer (char *buffer, int length);
/*void nasrg_print_meas_report (char *rrc_rrm_meas_payload, uint16_t type);
void nasrg_print_bs_meas_report (char *rrc_rrm_meas_payload, uint16_t type);*/
void nasrg_print_meas_report (char *rrc_rrm_meas_payload, unsigned short type);
void nasrg_print_bs_meas_report (char *rrc_rrm_meas_payload, unsigned short type);
#endif

#endif
