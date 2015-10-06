/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */

#ifndef S1AP_MESSAGES_TYPES_H_
#define S1AP_MESSAGES_TYPES_H_

#define S1AP_ENB_DEREGISTERED_IND(mSGpTR)   (mSGpTR)->ittiMsg.s1ap_eNB_deregistered_ind
#define S1AP_DEREGISTER_UE_REQ(mSGpTR)      (mSGpTR)->ittiMsg.s1ap_deregister_ue_req
#define S1AP_UE_CONTEXT_RELEASE_REQ(mSGpTR) (mSGpTR)->ittiMsg.s1ap_ue_context_release_req
#define S1AP_UE_CONTEXT_RELEASE_COMMAND(mSGpTR) (mSGpTR)->ittiMsg.s1ap_ue_context_release_command
#define S1AP_UE_CONTEXT_RELEASE_COMPLETE(mSGpTR) (mSGpTR)->ittiMsg.s1ap_ue_context_release_complete

typedef struct s1ap_initial_ue_message_s {
  unsigned     eNB_ue_s1ap_id:24;
  uint32_t     mme_ue_s1ap_id;
  cgi_t        e_utran_cgi;
} s1ap_initial_ue_message_t;

typedef struct s1ap_initial_ctxt_setup_req_s {
  unsigned                eNB_ue_s1ap_id:24;
  uint32_t                mme_ue_s1ap_id;

  /* Key eNB */
  uint8_t                 keNB[32];

  ambr_t                  ambr;
  ambr_t                  apn_ambr;

  /* EPS bearer ID */
  unsigned                ebi:4;

  /* QoS */
  qci_t                   qci;
  priority_level_t        prio_level;
  pre_emp_vulnerability_t pre_emp_vulnerability;
  pre_emp_capability_t    pre_emp_capability;

  /* S-GW TEID for user-plane */
  Teid_t                  teid;
  /* S-GW IP address for User-Plane */
  ip_address_t            s_gw_address;
} s1ap_initial_ctxt_setup_req_t;

typedef struct s1ap_ue_cap_ind_s {
  unsigned  eNB_ue_s1ap_id:24;
  uint32_t  mme_ue_s1ap_id;
  uint8_t   radio_capabilities[100];
  uint32_t  radio_capabilities_length;
} s1ap_ue_cap_ind_t;

#define S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE 20
typedef struct s1ap_eNB_deregistered_ind_s {
  uint8_t  nb_ue_to_deregister;
  uint32_t mme_ue_s1ap_id[S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE];
} s1ap_eNB_deregistered_ind_t;

typedef struct s1ap_deregister_ue_req_s {
  uint32_t mme_ue_s1ap_id;
} s1ap_deregister_ue_req_t;

typedef struct s1ap_ue_context_release_req_s {
  uint32_t mme_ue_s1ap_id;
} s1ap_ue_context_release_req_t;

typedef struct s1ap_ue_context_release_command_s {
  uint32_t mme_ue_s1ap_id;
} s1ap_ue_context_release_command_t;

typedef struct s1ap_ue_context_release_complete_s {
  uint32_t mme_ue_s1ap_id;
} s1ap_ue_context_release_complete_t;

#endif /* S1AP_MESSAGES_TYPES_H_ */
