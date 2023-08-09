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

/*! \file common/utils/telnetsrv/telnetsrv_ltemeasur_def.h
 * \brief: definitions of macro used to initialize the telnet_ltemeasurdef_t
 * \        strucures arrays which are then used by the display functions
 * \        in telnetsrv_measurements.c. 
 * \author Francois TABURET
 * \date 2019
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */

#ifndef __TELNETSRV_LTEMEASUR_DEF__H__
#define __TELNETSRV_LTEMEASUR_DEF__H__

#define LTEMAC_MEASURE \
{ \
  {"total_num_bcch_pdu",   &(macstatptr->total_num_bcch_pdu),TELNET_VARTYPE_INT32,0},\
  {"bcch_buffer",	   &(macstatptr->bcch_buffer),TELNET_VARTYPE_INT32,0},\
  {"total_bcch_buffer",	   &(macstatptr->total_bcch_buffer),TELNET_VARTYPE_INT32,0},\
  {"bcch_mcs",	           &(macstatptr->bcch_mcs),TELNET_VARTYPE_INT32,0},\
  {"total_num_ccch_pdu",   &(macstatptr->total_num_ccch_pdu),TELNET_VARTYPE_INT32,0},\
  {"ccch_buffer",	   &(macstatptr->ccch_buffer),TELNET_VARTYPE_INT32,0},\
  {"total_ccch_buffer",	   &(macstatptr->total_ccch_buffer),TELNET_VARTYPE_INT32,0},\
  {"ccch_mcs",	           &(macstatptr->ccch_mcs),TELNET_VARTYPE_INT32,0},\
  {"total_num_pcch_pdu",   &(macstatptr->total_num_pcch_pdu),TELNET_VARTYPE_INT32,0},\
  {"pcch_buffer",	   &(macstatptr->pcch_buffer),TELNET_VARTYPE_INT32,0},\
  {"total_pcch_buffer",	   &(macstatptr->total_pcch_buffer),TELNET_VARTYPE_INT32,0},\
  {"pcch_mcs",	           &(macstatptr->pcch_mcs),TELNET_VARTYPE_INT32,0},\
  {"num_dlactive_UEs",	   &(macstatptr->num_dlactive_UEs),TELNET_VARTYPE_INT16,0},\
  {"available_prbs",	   &(macstatptr->available_prbs),TELNET_VARTYPE_INT16,0},\
  {"total_available_prbs", &(macstatptr->total_available_prbs),TELNET_VARTYPE_INT32,0},\
  {"available_ncces",	   &(macstatptr->available_ncces),TELNET_VARTYPE_INT16,0},\
  {"dlsch_bitrate",	   &(macstatptr->dlsch_bitrate),TELNET_VARTYPE_INT32,0},\
  {"dlsch_bytes_tx",	   &(macstatptr->dlsch_bytes_tx),TELNET_VARTYPE_INT32,0},\
  {"dlsch_pdus_tx",	   &(macstatptr->dlsch_pdus_tx),TELNET_VARTYPE_INT32,0},\
  {"total_dlsch_bitrate",  &(macstatptr->total_dlsch_bitrate),TELNET_VARTYPE_INT32,0},\
  {"total_dlsch_bytes_tx", &(macstatptr->total_dlsch_bytes_tx),TELNET_VARTYPE_INT32,0},\
  {"total_dlsch_pdus_tx",  &(macstatptr->total_dlsch_pdus_tx),TELNET_VARTYPE_INT32,0},\
  {"ulsch_bitrate",	   &(macstatptr->ulsch_bitrate),TELNET_VARTYPE_INT32,0},\
  {"ulsch_bytes_rx",	   &(macstatptr->ulsch_bytes_rx),TELNET_VARTYPE_INT32,0},\
  {"ulsch_pdus_rx",	   &(macstatptr->ulsch_pdus_rx),TELNET_VARTYPE_INT32,0},\
  {"total_ulsch_bitrate",  &(macstatptr->total_ulsch_bitrate),TELNET_VARTYPE_INT32,0},\
  {"total_ulsch_bytes_rx", &(macstatptr->total_ulsch_bytes_rx),TELNET_VARTYPE_INT32,0},\
  {"total_ulsch_pdus_rx",  &(macstatptr->total_ulsch_pdus_rx),TELNET_VARTYPE_INT32,0},\
  {"sched_decisions",	   &(macstatptr->sched_decisions),TELNET_VARTYPE_INT32,0},\
  {"missed_deadlines",	   &(macstatptr->missed_deadlines),TELNET_VARTYPE_INT32,0},\
}

#define LTEMAC_UEMEASURE \
{ \
  {"dlsch_mcs1",	   &(macuestatptr->dlsch_mcs1),TELNET_VARTYPE_INT8,0},\
  {"dlsch_mcs2",	   &(macuestatptr->dlsch_mcs2),TELNET_VARTYPE_INT8,0},\
  {"rbs_used",	           &(macuestatptr->rbs_used),TELNET_VARTYPE_INT32,0},\
  {"rbs_used_retx",	   &(macuestatptr->rbs_used_retx),TELNET_VARTYPE_INT16,0},\
  {"total_rbs_used",	   &(macuestatptr->total_rbs_used),TELNET_VARTYPE_INT16,0},\
  {"ncce_used",	           &(macuestatptr->ncce_used),TELNET_VARTYPE_INT16,0},\
  {"ncce_used_retx",	   &(macuestatptr->ncce_used_retx),TELNET_VARTYPE_INT16,0},\
  {"TBS",	           &(macuestatptr->TBS),TELNET_VARTYPE_INT32,0},\
  {"total_pdu_bytes",	   &(macuestatptr->total_pdu_bytes),TELNET_VARTYPE_INT64,0},\
  {"total_num_pdus",	   &(macuestatptr->total_num_pdus),TELNET_VARTYPE_INT32,0},\
  {"overhead_bytes",	   &(macuestatptr->overhead_bytes),TELNET_VARTYPE_INT64,0},\
  {"crnti",	           &(macuestatptr->crnti),TELNET_VARTYPE_INT16,0},\
  {"snr",                  &(macuestatptr->snr),TELNET_VARTYPE_INT32,0},\
  {"target_snr ",          &(macuestatptr->target_snr),TELNET_VARTYPE_INT32,0},\
  {"ulsch_mcs1",	   &(macuestatptr->ulsch_mcs1),TELNET_VARTYPE_INT8,0},\
  {"ulsch_mcs2",	   &(macuestatptr->ulsch_mcs2),TELNET_VARTYPE_INT8,0},\
  {"rbs_used_rx",	   &(macuestatptr->rbs_used_rx),TELNET_VARTYPE_INT32,0},\
  {"rbs_used_retx_rx",	   &(macuestatptr->rbs_used_retx_rx),TELNET_VARTYPE_INT32,0},\
  {"total_rbs_used_rx",	   &(macuestatptr->total_rbs_used_rx),TELNET_VARTYPE_INT32,0},\
  {"ulsch_TBS",	           &(macuestatptr->ulsch_TBS),TELNET_VARTYPE_INT32,0},\
  {"total_pdu_bytes_rx",   &(macuestatptr->total_pdu_bytes_rx),TELNET_VARTYPE_INT64,0},\
  {"total_num_pdus_rx",	   &(macuestatptr->total_num_pdus_rx),TELNET_VARTYPE_INT32,0},\
  {"num_errors_rx",	   &(macuestatptr->num_errors_rx),TELNET_VARTYPE_INT32,0},\
}

#define LTE_RLCMEASURE \
{ \
  {"rlc_mode",                        NULL, TELNET_VARTYPE_UINT, 0},\
  {"tx_pdcp_sdu",                     NULL, TELNET_VARTYPE_UINT, 0},\
  {"tx_pdcp_bytes",                   NULL, TELNET_VARTYPE_UINT, 0},\
  {"tx_pdcp_sdu_discarded",           NULL, TELNET_VARTYPE_UINT, 0},\
  {"tx_pdcp_bytes_discarded",         NULL, TELNET_VARTYPE_UINT, 0},\
  {"tx_data_pdu",                     NULL, TELNET_VARTYPE_UINT, 0},\
  {"tx_data_bytes",                   NULL, TELNET_VARTYPE_UINT, 0},\
  {"tx_retransmit_pdu_by_status",     NULL, TELNET_VARTYPE_UINT, 0},\
  {"tx_retransmit_bytes_by_status",   NULL, TELNET_VARTYPE_UINT, 0},\
  {"tx_retransmit_pdu",               NULL, TELNET_VARTYPE_UINT, 0},\
  {"tx_retransmit_bytes",             NULL, TELNET_VARTYPE_UINT, 0},\
  {"tx_control_pdu",                  NULL, TELNET_VARTYPE_UINT, 0},\
  {"tx_control_bytes",                NULL, TELNET_VARTYPE_UINT, 0},\
  {"rx_pdcp_sdu",                     NULL, TELNET_VARTYPE_UINT, 0},\
  {"rx_pdcp_bytes",                   NULL, TELNET_VARTYPE_UINT, 0},\
  {"rx_data_pdus_duplicate",          NULL, TELNET_VARTYPE_UINT, 0},\
  {"rx_data_bytes_duplicate",         NULL, TELNET_VARTYPE_UINT, 0},\
  {"rx_data_pdu",                     NULL, TELNET_VARTYPE_UINT, 0},\
  {"rx_data_bytes",                   NULL, TELNET_VARTYPE_UINT, 0},\
  {"rx_data_pdu_dropped",             NULL, TELNET_VARTYPE_UINT, 0},\
  {"rx_data_bytes_dropped",           NULL, TELNET_VARTYPE_UINT, 0},\
  {"rx_data_pdu_out_of_window",       NULL, TELNET_VARTYPE_UINT, 0},\
  {"rx_data_bytes_out_of_window",     NULL, TELNET_VARTYPE_UINT, 0},\
  {"rx_control_pdu",                  NULL, TELNET_VARTYPE_UINT, 0},\
  {"rx_control_bytes",                NULL, TELNET_VARTYPE_UINT, 0},\
  {"timer_reorder_tout",              NULL, TELNET_VARTYPE_UINT, 0},\
  {"timer_poll_retrans_tout",         NULL, TELNET_VARTYPE_UINT, 0},\
  {"timer_status_prohibit_tout",      NULL, TELNET_VARTYPE_UINT, 0},\
}

#endif
