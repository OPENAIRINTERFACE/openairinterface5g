/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/


#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>

#include <arpa/inet.h>

#include "intertask_interface.h"
#include "mme_config.h"

#include "assertions.h"
#include "conversions.h"
#include "tree.h"
#include "enum_string.h"
#include "common_types.h"

#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "msc.h"

int ue_context_compare_identifiers(struct ue_context_s *p1,
                                   struct ue_context_s *p2);

RB_PROTOTYPE(ue_context_map, ue_context_s, rb_entry,
             ue_context_compare_identifiers);

RB_GENERATE(ue_context_map, ue_context_s, rb_entry,
            ue_context_compare_identifiers);

//------------------------------------------------------------------------------
extern inline int ue_context_compare_identifiers(
  struct ue_context_s *p1, struct ue_context_s *p2)
//------------------------------------------------------------------------------
{
  MME_APP_DEBUG(" ue_context_compare_identifiers IMSI           %"SCNu64"\n", p1->imsi);
  MME_APP_DEBUG(" ue_context_compare_identifiers mme_s11_teid   %08x\n"       , p1->mme_s11_teid);
  MME_APP_DEBUG(" ue_context_compare_identifiers mme_ue_s1ap_id %08x\n"       , p1->mme_ue_s1ap_id);
  MME_APP_DEBUG(" ue_context_compare_identifiers ue_id          %08x\n"       , p1->ue_id);

  if (p1->imsi > 0) {
    MME_APP_DEBUG(" with IMSI          %"SCNu64"\n", p2->imsi);

    /* if IMSI provided */
    if (p1->imsi > p2->imsi) {
      return 1;
    }

    if (p1->imsi < p2->imsi) {
      return -1;
    }
  } else if (p1->mme_s11_teid > 0) {
    MME_APP_DEBUG(" with mme_s11_teid          %08x\n", p2->mme_s11_teid);

    /* if s11 teid provided */
    if (p1->mme_s11_teid > p2->mme_s11_teid) {
      return 1;
    }

    if (p1->mme_s11_teid < p2->mme_s11_teid) {
      return -1;
    }
  } else if (p1->mme_ue_s1ap_id > 0) {
    MME_APP_DEBUG(" with mme_ue_s1ap_id          %08x\n", p2->mme_ue_s1ap_id);

    /* if s1ap ue id provided */
    if (p1->mme_ue_s1ap_id > p2->mme_ue_s1ap_id) {
      return 1;
    }

    if (p1->mme_ue_s1ap_id < p2->mme_ue_s1ap_id) {
      return -1;
    }
  }  else if (p1->ue_id > 0) {
    MME_APP_DEBUG(" with ue_id          %08x\n", p2->ue_id);

    /* if nas ue_id provided */
    if (p1->ue_id > p2->ue_id) {
      return 1;
    }

    if (p1->ue_id < p2->ue_id) {
      return -1;
    }
  } else {
    uint16_t mcc1;
    uint16_t mnc1;
    uint16_t mnc1_len;
    uint16_t mcc2;
    uint16_t mnc2;
    uint16_t mnc2_len;

    PLMN_T_TO_MCC_MNC(p1->guti.gummei.plmn, mcc1, mnc1, mnc1_len);
    PLMN_T_TO_MCC_MNC(p1->guti.gummei.plmn, mcc2, mnc2, mnc2_len);

    /* else compare by GUTI */
    if ((p1->guti.m_tmsi < p2->guti.m_tmsi) &&
        (p1->guti.gummei.MMEcode < p2->guti.gummei.MMEcode) &&
        (p1->guti.gummei.MMEgid < p2->guti.gummei.MMEgid) &&
        (mcc1 < mcc2) &&
        (mnc1 < mnc2) &&
        (mnc1_len < mnc2_len))
      return 1;

    if ((p1->guti.m_tmsi > p2->guti.m_tmsi) &&
        (p1->guti.gummei.MMEcode > p2->guti.gummei.MMEcode) &&
        (p1->guti.gummei.MMEgid > p2->guti.gummei.MMEgid) &&
        (mcc1 > mcc2) &&
        (mnc1 > mnc2) &&
        (mnc1_len > mnc2_len))
      return -1;
  }

  /* Match -> return 0 */
  return 0;
}

//------------------------------------------------------------------------------
ue_context_t *mme_create_new_ue_context(void)
//------------------------------------------------------------------------------
{
  ue_context_t *new_p;

  new_p = malloc(sizeof(ue_context_t));

  if (new_p == NULL) {
    return NULL;
  }

  memset(new_p, 0, sizeof(ue_context_t));
  return new_p;
}


//------------------------------------------------------------------------------
struct ue_context_s *mme_ue_context_exists_imsi(mme_ue_context_t *mme_ue_context,
    mme_app_imsi_t imsi)
//------------------------------------------------------------------------------
{
  struct ue_context_s  reference;

  DevAssert(mme_ue_context != NULL);

  memset(&reference, 0, sizeof(struct ue_context_s));
  reference.imsi = imsi;
  return RB_FIND(ue_context_map, &mme_ue_context->ue_context_tree,
                 &reference);
}


//------------------------------------------------------------------------------
struct ue_context_s *mme_ue_context_exists_s11_teid(mme_ue_context_t *mme_ue_context,
    uint32_t teid)
//------------------------------------------------------------------------------
{
  struct ue_context_s  reference;

  DevAssert(mme_ue_context != NULL);

  memset(&reference, 0, sizeof(struct ue_context_s));
  reference.mme_s11_teid = teid;
  return RB_FIND(ue_context_map, &mme_ue_context->ue_context_tree,
                 &reference);
}



//------------------------------------------------------------------------------
ue_context_t *mme_ue_context_exists_mme_ue_s1ap_id(
  mme_ue_context_t *mme_ue_context,
  uint32_t mme_ue_s1ap_id)
//------------------------------------------------------------------------------
{
  struct ue_context_s  reference;

  DevAssert(mme_ue_context != NULL);

  memset(&reference, 0, sizeof(struct ue_context_s));
  reference.mme_ue_s1ap_id = mme_ue_s1ap_id;
  return RB_FIND(ue_context_map, &mme_ue_context->ue_context_tree,
                 &reference);
}



//------------------------------------------------------------------------------
ue_context_t *mme_ue_context_exists_nas_ue_id(
  mme_ue_context_t *mme_ue_context,
  uint32_t nas_ue_id)
//------------------------------------------------------------------------------
{
  struct ue_context_s  reference;

  DevAssert(mme_ue_context != NULL);

  memset(&reference, 0, sizeof(struct ue_context_s));
  reference.ue_id = nas_ue_id;
  return RB_FIND(ue_context_map, &mme_ue_context->ue_context_tree,
                 &reference);
}




//------------------------------------------------------------------------------
ue_context_t *mme_ue_context_exists_guti(mme_ue_context_t *mme_ue_context,
    GUTI_t guti)
//------------------------------------------------------------------------------
{
  struct ue_context_s  reference;

  DevAssert(mme_ue_context != NULL);

  memset(&reference, 0, sizeof(struct ue_context_s));

  memcpy(&reference.guti, &guti, sizeof(GUTI_t));

  return RB_FIND(ue_context_map, &mme_ue_context->ue_context_tree,
                 &reference);
}

//------------------------------------------------------------------------------
int mme_insert_ue_context(mme_ue_context_t *mme_ue_context, struct ue_context_s *ue_context_p)
//------------------------------------------------------------------------------
{
  struct ue_context_s *collision_p = NULL;

  DevAssert(mme_ue_context != NULL);
  DevAssert(ue_context_p != NULL);

  /* Updating statistics */
  mme_ue_context->nb_ue_managed++;
  mme_ue_context->nb_ue_since_last_stat++;

  collision_p = RB_INSERT(ue_context_map, &mme_ue_context->ue_context_tree,
                          ue_context_p);

  if (collision_p != NULL) {
    fprintf(stderr, "This ue context already exists...\n");
    return -1;
  }

  return 0;
}



//------------------------------------------------------------------------------
void mme_remove_ue_context(mme_ue_context_t *mme_ue_context, struct ue_context_s *ue_context_p)
//------------------------------------------------------------------------------
{
  struct ue_context_s *collision_p = NULL;

  DevAssert(mme_ue_context != NULL);
  DevAssert(ue_context_p != NULL);

  /* Updating statistics */
  mme_ue_context->nb_ue_managed++;
  mme_ue_context->nb_ue_since_last_stat++;

  collision_p = RB_REMOVE(ue_context_map, &mme_ue_context->ue_context_tree,
                          ue_context_p);
#warning "TODO mme_ue_context_free_content"
  //TODO mme_ue_context_free_content(ue_context_p);
  free(ue_context_p);
  return 0;
}

//------------------------------------------------------------------------------
void mme_app_dump_ue_contexts(mme_ue_context_t *mme_ue_context)
//------------------------------------------------------------------------------
{
  struct ue_context_s *context_p;

  MME_APP_DEBUG("-----------------------UE contexts-----------------------\n");
  RB_FOREACH(context_p, ue_context_map, &mme_ue_context->ue_context_tree) {
    uint8_t j;

    MME_APP_DEBUG("    - IMSI ...........: %"SCNu64"\n", context_p->imsi);
    MME_APP_DEBUG("                        |  m_tmsi  | mmec | mmegid | mcc | mnc |\n");
    MME_APP_DEBUG("    - GUTI............: | %08x |  %02x  |  %04x  | %03u | %03u |\n",
                  context_p->guti.m_tmsi, context_p->guti.gummei.MMEcode,
                  context_p->guti.gummei.MMEgid,
                  /* TODO check if two or three digits MNC... */
                  context_p->guti.gummei.plmn.MCCdigit3 * 100 +
                  context_p->guti.gummei.plmn.MCCdigit2 * 10 +
                  context_p->guti.gummei.plmn.MCCdigit1,
                  context_p->guti.gummei.plmn.MNCdigit3 * 100 +
                  context_p->guti.gummei.plmn.MNCdigit2 * 10 +
                  context_p->guti.gummei.plmn.MNCdigit1);
    MME_APP_DEBUG("    - Authenticated ..: %s\n",
                  (context_p->imsi_auth == IMSI_UNAUTHENTICATED) ? "FALSE": "TRUE");
    MME_APP_DEBUG("    - eNB UE s1ap ID .: %08x\n", context_p->eNB_ue_s1ap_id);
    MME_APP_DEBUG("    - MME UE s1ap ID .: %08x\n", context_p->mme_ue_s1ap_id);
    MME_APP_DEBUG("    - MME S11 TEID ...: %08x\n", context_p->mme_s11_teid);
    MME_APP_DEBUG("    - SGW S11 TEID ...: %08x\n", context_p->sgw_s11_teid);
    MME_APP_DEBUG("                        | mcc | mnc | cell id  |\n");
    MME_APP_DEBUG("    - E-UTRAN CGI ....: | %03u | %03u | %08x |\n",
                  context_p->e_utran_cgi.plmn.MCCdigit3 * 100 +
                  context_p->e_utran_cgi.plmn.MCCdigit2 * 10 +
                  context_p->e_utran_cgi.plmn.MCCdigit1,
                  context_p->e_utran_cgi.plmn.MNCdigit3 * 100 +
                  context_p->e_utran_cgi.plmn.MNCdigit2 * 10 +
                  context_p->e_utran_cgi.plmn.MNCdigit1,
                  context_p->e_utran_cgi.cell_identity);
    /* Ctime return a \n in the string */
    MME_APP_DEBUG("    - Last acquired ..: %s", ctime(&context_p->cell_age));

    /* Display UE info only if we know them */
    if (context_p->subscription_known == SUBSCRIPTION_KNOWN) {
      MME_APP_DEBUG("    - Status .........: %s\n",
                    (context_p->sub_status == SS_SERVICE_GRANTED) ? "Granted" : "Barred");
#define DISPLAY_BIT_MASK_PRESENT(mASK)   \
            ((context_p->access_restriction_data & mASK) ? 'X' : 'O')
      MME_APP_DEBUG("    (O = allowed, X = !O) |UTRAN|GERAN|GAN|HSDPA EVO|E_UTRAN|HO TO NO 3GPP|\n");
      MME_APP_DEBUG("    - Access restriction  |  %c  |  %c  | %c |    %c    |   %c   |      %c      |\n",
                    DISPLAY_BIT_MASK_PRESENT(ARD_UTRAN_NOT_ALLOWED),
                    DISPLAY_BIT_MASK_PRESENT(ARD_GERAN_NOT_ALLOWED),
                    DISPLAY_BIT_MASK_PRESENT(ARD_GAN_NOT_ALLOWED),
                    DISPLAY_BIT_MASK_PRESENT(ARD_I_HSDPA_EVO_NOT_ALLOWED),
                    DISPLAY_BIT_MASK_PRESENT(ARD_E_UTRAN_NOT_ALLOWED),
                    DISPLAY_BIT_MASK_PRESENT(ARD_HO_TO_NON_3GPP_NOT_ALLOWED));
      MME_APP_DEBUG("    - Access Mode ....: %s\n",
                    ACCESS_MODE_TO_STRING(context_p->access_mode));
      MME_APP_DEBUG("    - MSISDN .........: %-*s\n",
                    MSISDN_LENGTH, context_p->msisdn);
      MME_APP_DEBUG("    - RAU/TAU timer ..: %u\n", context_p->rau_tau_timer);
      MME_APP_DEBUG("    - IMEISV .........: %*s\n", 15,
                    context_p->me_identity.imeisv);
      MME_APP_DEBUG("    - AMBR (bits/s)     ( Downlink |  Uplink  )\n");
      MME_APP_DEBUG("        Subscribed ...: (%010"PRIu64"|%010"PRIu64")\n",
                    context_p->subscribed_ambr.br_dl,
                    context_p->subscribed_ambr.br_ul);
      MME_APP_DEBUG("        Allocated ....: (%010"PRIu64"|%010"PRIu64")\n",
                    context_p->used_ambr.br_dl, context_p->used_ambr.br_ul);
      MME_APP_DEBUG("    - Known vectors ..: %u\n", context_p->nb_of_vectors);

      for (j = 0; j < context_p->nb_of_vectors; j++) {
        int k;
        char xres_string[3 * XRES_LENGTH_MAX + 1];
        eutran_vector_t *vector_p;

        vector_p = &context_p->vector_list[j];

        MME_APP_DEBUG("        - RAND ..: "RAND_FORMAT"\n",
                      RAND_DISPLAY(vector_p->rand));
        MME_APP_DEBUG("        - AUTN ..: "AUTN_FORMAT"\n",
                      AUTN_DISPLAY(vector_p->autn));
        MME_APP_DEBUG("        - KASME .: "KASME_FORMAT"\n",
                      KASME_DISPLAY_1(vector_p->kasme));
        MME_APP_DEBUG("                   "KASME_FORMAT"\n",
                      KASME_DISPLAY_2(vector_p->kasme));

        for (k = 0; k < vector_p->xres.size; k++) {
          sprintf(&xres_string[k * 3], "%02x,", vector_p->xres.data[k]);
        }

        xres_string[k * 3 - 1] = '\0';
        MME_APP_DEBUG("        - XRES ..: %s\n", xres_string);
      }

      MME_APP_DEBUG("    - PDN List:\n");

      for (j = 0; j < context_p->apn_profile.nb_apns; j++) {
        struct apn_configuration_s *apn_config_p;

        apn_config_p = &context_p->apn_profile.apn_configuration[j];

        /* Default APN ? */
        MME_APP_DEBUG("        - Default APN ...: %s\n",
                      (apn_config_p->context_identifier == context_p->apn_profile.context_identifier)
                      ? "TRUE" : "FALSE");
        MME_APP_DEBUG("        - APN ...........: %s\n", apn_config_p->service_selection);
        MME_APP_DEBUG("        - AMBR (bits/s) ( Downlink |  Uplink  )\n");
        MME_APP_DEBUG("                        (%010"PRIu64"|%010"PRIu64")\n",
                      apn_config_p->ambr.br_dl, apn_config_p->ambr.br_ul);
        MME_APP_DEBUG("        - PDN type ......: %s\n",
                      PDN_TYPE_TO_STRING(apn_config_p->pdn_type));
        MME_APP_DEBUG("        - QOS\n");
        MME_APP_DEBUG("            QCI .........: %u\n",
                      apn_config_p->subscribed_qos.qci);
        MME_APP_DEBUG("            Prio level ..: %u\n",
                      apn_config_p->subscribed_qos.allocation_retention_priority.priority_level);
        MME_APP_DEBUG("            Pre-emp vul .: %s\n",
                      (apn_config_p->subscribed_qos.allocation_retention_priority.pre_emp_vulnerability
                       == PRE_EMPTION_VULNERABILITY_ENABLED) ? "ENABLED" : "DISABLED");
        MME_APP_DEBUG("            Pre-emp cap .: %s\n",
                      (apn_config_p->subscribed_qos.allocation_retention_priority.pre_emp_capability
                       == PRE_EMPTION_CAPABILITY_ENABLED) ? "ENABLED" : "DISABLED");

        if (apn_config_p->nb_ip_address == 0) {
          MME_APP_DEBUG("            IP addr .....: Dynamic allocation\n");
        } else {
          int i;
          MME_APP_DEBUG("            IP addresses :\n");

          for (i = 0; i < apn_config_p->nb_ip_address; i++) {
            if (apn_config_p->ip_address[i].pdn_type == IPv4) {
              MME_APP_DEBUG("                           ["IPV4_ADDR"]\n",
                            IPV4_ADDR_DISPLAY_8(apn_config_p->ip_address[i].address.ipv4_address));
            } else {
              char ipv6[40];
              inet_ntop(AF_INET6, apn_config_p->ip_address[i].address.ipv6_address,ipv6, 40);
              MME_APP_DEBUG("                           [%s]\n",  ipv6);
            }
          }
        }

        MME_APP_DEBUG("\n");
      }

      MME_APP_DEBUG("    - Bearer List:\n");

      for (j = 0; j < BEARERS_PER_UE; j++) {
        bearer_context_t *bearer_context_p;

        bearer_context_p = &context_p->eps_bearers[j];

        if (bearer_context_p->s_gw_teid != 0) {
          MME_APP_DEBUG("        Bearer id .......: %02u\n", j);
          MME_APP_DEBUG("        S-GW TEID (UP)...: %08x\n", bearer_context_p->s_gw_teid);
          MME_APP_DEBUG("        P-GW TEID (UP)...: %08x\n", bearer_context_p->p_gw_teid);
          MME_APP_DEBUG("        QCI .............: %u\n",
                        bearer_context_p->qci);
          MME_APP_DEBUG("        Priority level ..: %u\n",
                        bearer_context_p->prio_level);
          MME_APP_DEBUG("        Pre-emp vul .....: %s\n",
                        (bearer_context_p->pre_emp_vulnerability
                         == PRE_EMPTION_VULNERABILITY_ENABLED) ? "ENABLED" : "DISABLED");
          MME_APP_DEBUG("        Pre-emp cap .....: %s\n",
                        (bearer_context_p->pre_emp_capability
                         == PRE_EMPTION_CAPABILITY_ENABLED) ? "ENABLED" : "DISABLED");
        }
      }
    }
  }
  MME_APP_DEBUG("---------------------------------------------------------\n");
}


//------------------------------------------------------------------------------
void mme_app_handle_s1ap_ue_context_release_req(const s1ap_ue_context_release_req_t const *s1ap_ue_context_release_req)
//------------------------------------------------------------------------------
{
  struct ue_context_s           *ue_context_p = NULL;
  MessageDef                    *message_p    = NULL;

  MME_APP_DEBUG("Received S1AP_UE_CONTEXT_RELEASE_REQ from S1AP\n");
  ue_context_p = mme_ue_context_exists_nas_ue_id(&mme_app_desc.mme_ue_contexts, s1ap_ue_context_release_req->mme_ue_s1ap_id);

  if (ue_context_p == NULL) {
	MSC_LOG_EVENT(
   		MSC_MMEAPP_MME,
   		"0 S1AP_UE_CONTEXT_RELEASE_REQ Unknown mme_ue_s1ap_id 0x%06"PRIX32" ",s1ap_ue_context_release_req->mme_ue_s1ap_id);
    MME_APP_ERROR("UE context doesn't exist for UE 0x%06"PRIX32"/dec%u\n",
    		s1ap_ue_context_release_req->mme_ue_s1ap_id,
    		s1ap_ue_context_release_req->mme_ue_s1ap_id);
    return;
  }
  if ((ue_context_p->mme_s11_teid == 0) &&
	  (ue_context_p->sgw_s11_teid == 0)) {
	  // no session was created, no need for releasing bearers in SGW
	  message_p = itti_alloc_new_message(TASK_MME_APP, S1AP_UE_CONTEXT_RELEASE_COMMAND);

	  AssertFatal(message_p != NULL, "itti_alloc_new_message Failed");
	  memset((void*)&message_p->ittiMsg.s1ap_ue_context_release_command,
	         0,
	         sizeof(s1ap_ue_context_release_command_t));

	  S1AP_UE_CONTEXT_RELEASE_COMMAND(message_p).mme_ue_s1ap_id       = ue_context_p->mme_ue_s1ap_id;
	  MSC_LOG_TX_MESSAGE(
	  		MSC_MMEAPP_MME,
	  		MSC_S1AP_MME,
	  		NULL,0,
	  		"0 S1AP_UE_CONTEXT_RELEASE_COMMAND mme_ue_s1ap_id %06"PRIX32" ",
	  		S1AP_UE_CONTEXT_RELEASE_COMMAND(message_p).mme_ue_s1ap_id);


	  itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, message_p);
  } else {
    mme_app_send_s11_release_access_bearers_req(ue_context_p);
  }
}


//------------------------------------------------------------------------------
void mme_app_handle_s1ap_ue_context_release_complete(const s1ap_ue_context_release_complete_t const *s1ap_ue_context_release_complete)
//------------------------------------------------------------------------------
{
  struct ue_context_s     *ue_context_p  = NULL;
  MessageDef              *message_p     = NULL;

  MME_APP_DEBUG("Received S1AP_UE_CONTEXT_RELEASE_COMPLETE from S1AP\n");

  ue_context_p = mme_ue_context_exists_nas_ue_id(&mme_app_desc.mme_ue_contexts, s1ap_ue_context_release_complete->mme_ue_s1ap_id);

  if (ue_context_p == NULL) {
	MSC_LOG_EVENT(
   		MSC_MMEAPP_MME,
   		"0 S1AP_UE_CONTEXT_RELEASE_COMPLETE Unknown mme_ue_s1ap_id 0x%06"PRIX32" ",s1ap_ue_context_release_complete->mme_ue_s1ap_id);
    MME_APP_ERROR("UE context doesn't exist for mme_ue_s1ap_id 0x%06"PRIX32"/dec%u\n",
    		s1ap_ue_context_release_complete->mme_ue_s1ap_id,
    		s1ap_ue_context_release_complete->mme_ue_s1ap_id);
    return;
  }
  message_p  = itti_alloc_new_message(TASK_MME_APP, S1AP_DEREGISTER_UE_REQ);
  memset((void*)&message_p->ittiMsg.s1ap_deregister_ue_req,
	         0,
	         sizeof(s1ap_deregister_ue_req_t));

  S1AP_DEREGISTER_UE_REQ(message_p).mme_ue_s1ap_id = s1ap_ue_context_release_complete->mme_ue_s1ap_id;


  MSC_LOG_TX_MESSAGE(
  		MSC_MMEAPP_MME,
  		MSC_NAS_MME,
  		NULL,0,
  		"0 S1AP_DEREGISTER_UE_REQ");

  itti_send_msg_to_task(TASK_NAS_MME, INSTANCE_DEFAULT, message_p);

  mme_remove_ue_context(&mme_app_desc.mme_ue_contexts, ue_context_p);
}
