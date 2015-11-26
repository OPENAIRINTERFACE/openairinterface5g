/*******************************************************************************

  Eurecom OpenAirInterface
  Copyright(c) 1999 - 2013 Eurecom

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information
  Openair Admin: openair_admin@eurecom.fr
  Openair Tech : openair_tech@eurecom.fr
  Forums       : http://forums.eurecom.fr/openairinterface
  Address      : EURECOM, Campus SophiaTech, 450 Route des Chappes
                 06410 Biot FRANCE

*******************************************************************************/

#ifndef S1AP_ENB_TRACE_H_
#define S1AP_ENB_TRACE_H_

// int s1ap_eNB_generate_trace_failure(sctp_data_t        *sctp_data_p,
//                                     int32_t             stream,
//                                     uint32_t            eNB_ue_s1ap_id,
//                                     uint32_t            mme_ue_s1ap_id,
//                                     E_UTRAN_Trace_ID_t *trace_id,
//                                     Cause_t            *cause_p);

// int s1ap_eNB_handle_trace_start(eNB_mme_desc_t *eNB_desc_p,
//                                 sctp_queue_item_t *packet_p,
//                                 struct s1ap_message_s *message_p);
int s1ap_eNB_handle_trace_start(uint32_t               assoc_id,
                                uint32_t               stream,
                                struct s1ap_message_s *message_p);

// int s1ap_eNB_handle_deactivate_trace(eNB_mme_desc_t *eNB_desc_p,
//                                      sctp_queue_item_t *packet_p,
//                                      struct s1ap_message_s *message_p);
int s1ap_eNB_handle_deactivate_trace(uint32_t               assoc_id,
                                     uint32_t               stream,
                                     struct s1ap_message_s *message_p);

#endif /* S1AP_ENB_TRACE_H_ */
