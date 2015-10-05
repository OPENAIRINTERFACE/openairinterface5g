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
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(MME_CLIENT_TEST)
# include "intertask_interface.h"
#endif

#include "assertions.h"
#include "queue.h"

#include "s1ap_mme.h"
#include "s1ap_mme_decoder.h"
#include "s1ap_mme_handlers.h"
#include "s1ap_ies_defs.h"

#include "s1ap_mme_nas_procedures.h"
#include "s1ap_mme_retransmission.h"
#include "msc.h"

#define S1AP_DEBUG_LIST
#ifdef S1AP_DEBUG_LIST
# define eNB_LIST_OUT(x, args...) S1AP_DEBUG("[eNB]%*s"x"\n", 4*indent, "", ##args)
# define UE_LIST_OUT(x, args...)  S1AP_DEBUG("[UE] %*s"x"\n", 4*indent, "", ##args)
#else
# define eNB_LIST_OUT(x, args...)
# define UE_LIST_OUT(x, args...)
#endif

#if !defined(MME_CLIENT_TEST)
// static pthread_t s1ap_task_thread;

int         hss_associated = 0;
uint32_t    nb_eNB_associated = 0;
STAILQ_HEAD(eNB_list_s, eNB_description_s)
eNB_list_head;

static int  indent = 0;

void *s1ap_mme_thread(void *args);

static int s1ap_send_init_sctp(void)
{
  // Create and alloc new message
  MessageDef *message_p;
  message_p = itti_alloc_new_message(TASK_S1AP, SCTP_INIT_MSG);
  message_p->ittiMsg.sctpInit.port = S1AP_PORT_NUMBER;
  message_p->ittiMsg.sctpInit.ppid = S1AP_SCTP_PPID;
  message_p->ittiMsg.sctpInit.ipv4 = 1;
  message_p->ittiMsg.sctpInit.ipv6 = 0;
  message_p->ittiMsg.sctpInit.nb_ipv4_addr = 1;
  message_p->ittiMsg.sctpInit.ipv4_address[0]
    = mme_config.ipv4.mme_ip_address_for_S1_MME;
  /* SR WARNING: ipv6 multi-homing fails sometimes for localhost.
   * Disable it for now.*/
  message_p->ittiMsg.sctpInit.nb_ipv6_addr = 0;
  message_p->ittiMsg.sctpInit.ipv6_address[0] = "0:0:0:0:0:0:0:1";

  return itti_send_msg_to_task(TASK_SCTP, INSTANCE_DEFAULT, message_p);
}

void *s1ap_mme_thread(void *args)
{
  MessageDef *received_message_p;

  itti_mark_task_ready(TASK_S1AP);
  MSC_START_USE();

  while(1) {
    /* Trying to fetch a message from the message queue.
     * If the queue is empty, this function will block till a
     * message is sent to the task.
     */
    itti_receive_msg(TASK_S1AP, &received_message_p);
    DevAssert(received_message_p != NULL);

    switch (ITTI_MSG_ID(received_message_p)) {
    case ACTIVATE_MESSAGE: {
      hss_associated = 1;
    }
    break;

    case SCTP_DATA_IND: {
      /* New message received from SCTP layer.
       * Decode and handle it.
       */
      s1ap_message message;

      memset((void *)&message, 0, sizeof(s1ap_message));

      /* Invoke S1AP message decoder */
      if (s1ap_mme_decode_pdu(&message,
                              SCTP_DATA_IND(received_message_p).buffer,
                              SCTP_DATA_IND(received_message_p).buf_length) < 0) {
        // TODO: Notify eNB of failure with right cause
        S1AP_ERROR("Failed to decode new buffer\n");
      } else {
        s1ap_mme_handle_message(SCTP_DATA_IND(received_message_p).assoc_id,
                                SCTP_DATA_IND(received_message_p).stream,
                                &message);
      }

      /* Free received PDU array */
      free(SCTP_DATA_IND(received_message_p).buffer);
    }
    break;

    /* SCTP layer notifies S1AP of disconnection of a peer. */
    case SCTP_CLOSE_ASSOCIATION: {
      s1ap_handle_sctp_deconnection(SCTP_CLOSE_ASSOCIATION(received_message_p).assoc_id);
    }
    break;

    case SCTP_NEW_ASSOCIATION: {
      s1ap_handle_new_association(&received_message_p->ittiMsg.sctp_new_peer);
    }
    break;

    case NAS_DOWNLINK_DATA_REQ: {
      /* New message received from NAS task.
       * This corresponds to a S1AP downlink nas transport message.
       */
      s1ap_generate_downlink_nas_transport(NAS_DL_DATA_REQ(received_message_p).UEid,
                                           NAS_DL_DATA_REQ(received_message_p).nasMsg.data,
                                           NAS_DL_DATA_REQ(received_message_p).nasMsg.length);
    }
    break;

    case S1AP_UE_CONTEXT_RELEASE_COMMAND: {
        s1ap_handle_ue_context_release_command(&received_message_p->ittiMsg.s1ap_ue_context_release_command);
    }
    break;

#if defined(DISABLE_USE_NAS)

    case NAS_ATTACH_ACCEPT: {
      s1ap_handle_attach_accepted(&received_message_p->ittiMsg.nas_attach_accept);
    }
    break;
#else
    // handled by MME_APP know
    //case NAS_CONNECTION_ESTABLISHMENT_CNF: {
    //    s1ap_handle_conn_est_cnf(&NAS_CONNECTION_ESTABLISHMENT_CNF(received_message_p));
    //} break;

    case MME_APP_CONNECTION_ESTABLISHMENT_CNF: {
      s1ap_handle_conn_est_cnf(&MME_APP_CONNECTION_ESTABLISHMENT_CNF(received_message_p));
    }
    break;
#endif

    case TIMER_HAS_EXPIRED: {
      s1ap_handle_timer_expiry(&received_message_p->ittiMsg.timer_has_expired);
    }
    break;

    case TERMINATE_MESSAGE: {
      itti_exit_task();
    }
    break;

    default: {
      S1AP_DEBUG("Unkwnon message ID %d:%s\n",
                 ITTI_MSG_ID(received_message_p), ITTI_MSG_NAME(received_message_p));
    }
    break;
    }

    itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), received_message_p);
    received_message_p = NULL;
  }

  return NULL;
}

int s1ap_mme_init(const mme_config_t *mme_config_p)
{
  S1AP_DEBUG("Initializing S1AP interface\n");

  if (get_asn1c_environment_version() < ASN1_MINIMUM_VERSION) {
    S1AP_ERROR("ASN1C version %d fount, expecting at least %d\n",
               get_asn1c_environment_version(), ASN1_MINIMUM_VERSION);
    return -1;
  } else {
    S1AP_DEBUG("ASN1C version %d\n", get_asn1c_environment_version());
  }

#if defined(UPDATE_RELEASE_10)
  S1AP_DEBUG("S1AP Release v10.5\n");
#else
# if defined(UPDATE_RELEASE_9)
  S1AP_DEBUG("S1AP Release v9.8\n");
# else
  S1AP_DEBUG("S1AP Release v8.10\n");
# endif
#endif

  STAILQ_INIT(&eNB_list_head);

  if (itti_create_task(TASK_S1AP, &s1ap_mme_thread, NULL) < 0) {
    S1AP_ERROR("Error while creating S1AP task\n");
    return -1;
  }

  if (s1ap_send_init_sctp() < 0) {
    S1AP_ERROR("Error while sendind SCTP_INIT_MSG to SCTP \n");
    return -1;
  }

  S1AP_DEBUG("Initializing S1AP interface: DONE\n");
  return 0;
}

void s1ap_dump_eNB_list(void)
{
  eNB_description_t *eNB_ref;

  STAILQ_FOREACH(eNB_ref, &eNB_list_head, eNB_entries) {
    s1ap_dump_eNB(eNB_ref);
  }
}

void s1ap_dump_eNB(eNB_description_t *eNB_ref)
{
#ifdef S1AP_DEBUG_LIST
  ue_description_t *ue_ref;
  //Reset indentation
  indent = 0;

  if (eNB_ref == NULL) {
    return;
  }

  eNB_LIST_OUT("");
  eNB_LIST_OUT("eNB name:          %s",   eNB_ref->eNB_name == NULL ? "not present" : eNB_ref->eNB_name);
  eNB_LIST_OUT("eNB ID:            %07x", eNB_ref->eNB_id);
  eNB_LIST_OUT("SCTP assoc id:     %d",   eNB_ref->sctp_assoc_id);
  eNB_LIST_OUT("SCTP instreams:    %d",   eNB_ref->instreams);
  eNB_LIST_OUT("SCTP outstreams:   %d",   eNB_ref->outstreams);
  eNB_LIST_OUT("UE attache to eNB: %d",   eNB_ref->nb_ue_associated);

  indent++;

  STAILQ_FOREACH(ue_ref, &eNB_ref->ue_list_head, ue_entries) {
    s1ap_dump_ue(ue_ref);
  }
  indent--;
  eNB_LIST_OUT("");
#else
  s1ap_dump_ue(NULL);
#endif
}

void s1ap_dump_ue(ue_description_t *ue_ref)
{
#ifdef S1AP_DEBUG_LIST

  if (ue_ref == NULL) return;

  UE_LIST_OUT("eNB UE s1ap id:   0x%06x", ue_ref->eNB_ue_s1ap_id);
  UE_LIST_OUT("MME UE s1ap id:   0x%08x", ue_ref->mme_ue_s1ap_id);
  UE_LIST_OUT("SCTP stream recv: 0x%04x", ue_ref->sctp_stream_recv);
  UE_LIST_OUT("SCTP stream send: 0x%04x", ue_ref->sctp_stream_send);
#else
  ue_ref = ue_ref;
#endif
}

eNB_description_t* s1ap_is_eNB_id_in_list(const uint32_t eNB_id)
{

  eNB_description_t *eNB_ref;

  STAILQ_FOREACH(eNB_ref, &eNB_list_head, eNB_entries) {
    if (eNB_ref->eNB_id == eNB_id) {
      /* We found a matching reference, return it */
      return eNB_ref;
    }
  }
  /* No matching eNB, return NULL */
  return eNB_ref;
}

eNB_description_t* s1ap_is_eNB_assoc_id_in_list(const uint32_t sctp_assoc_id)
{

  eNB_description_t *eNB_ref;

  STAILQ_FOREACH(eNB_ref, &eNB_list_head, eNB_entries) {
    if (eNB_ref->sctp_assoc_id == sctp_assoc_id) {
      /* We found a matching reference, return it */
      return eNB_ref;
    }
  }
  /* No matching eNB or no eNB in list, return NULL */
  return NULL;
}

ue_description_t *s1ap_is_ue_eNB_id_in_list(eNB_description_t *eNB_ref,
    const uint32_t eNB_ue_s1ap_id)
{
  ue_description_t *ue_ref;

  DevAssert(eNB_ref != NULL);

  STAILQ_FOREACH(ue_ref, &eNB_ref->ue_list_head, ue_entries) {
    if (ue_ref->eNB_ue_s1ap_id == eNB_ue_s1ap_id) {
      return ue_ref;
    }
  }

  return NULL;
}

ue_description_t* s1ap_is_ue_mme_id_in_list(const uint32_t mme_ue_s1ap_id)
{

  ue_description_t *ue_ref;
  eNB_description_t *eNB_ref;

  STAILQ_FOREACH(eNB_ref, &eNB_list_head, eNB_entries) {
    STAILQ_FOREACH(ue_ref, &eNB_ref->ue_list_head, ue_entries) {
      // We found a matching reference, return it
      if (ue_ref->mme_ue_s1ap_id == mme_ue_s1ap_id) {
        return ue_ref;
      }
    }
  }

  return NULL;
}

ue_description_t* s1ap_is_teid_in_list(const uint32_t teid)
{

  ue_description_t *ue_ref;
  eNB_description_t *eNB_ref;

  STAILQ_FOREACH(eNB_ref, &eNB_list_head, eNB_entries) {
    STAILQ_FOREACH(ue_ref, &eNB_ref->ue_list_head, ue_entries) {
      // We found a matching reference, return it
      if (ue_ref->teid == teid) {
        return ue_ref;
      }
    }
  }

  return NULL;
}

eNB_description_t* s1ap_new_eNB(void)
{

  eNB_description_t *eNB_ref = NULL;

  eNB_ref = calloc(1, sizeof(eNB_description_t));
  /* Something bad happened during malloc...
   * May be we are running out of memory.
   * TODO: Notify eNB with a cause like Hardware Failure.
   */
  DevAssert(eNB_ref != NULL);

  // Update number of eNB associated
  nb_eNB_associated++;

  STAILQ_INIT(&eNB_ref->ue_list_head);

  eNB_ref->nb_ue_associated = 0;

  STAILQ_INSERT_TAIL(&eNB_list_head, eNB_ref, eNB_entries);

  return eNB_ref;
}

ue_description_t* s1ap_new_ue(const uint32_t sctp_assoc_id)
{

  eNB_description_t *eNB_ref = NULL;
  ue_description_t  *ue_ref  = NULL;

  eNB_ref = s1ap_is_eNB_assoc_id_in_list(sctp_assoc_id);
  DevAssert(eNB_ref != NULL);

  ue_ref = calloc(1, sizeof(ue_description_t));
  /* Something bad happened during malloc...
   * May be we are running out of memory.
   * TODO: Notify eNB with a cause like Hardware Failure.
   */
  DevAssert(ue_ref != NULL);

  ue_ref->eNB = eNB_ref;

  // Increment number of UE
  eNB_ref->nb_ue_associated++;
  STAILQ_INSERT_TAIL(&eNB_ref->ue_list_head, ue_ref, ue_entries);

  return ue_ref;
}

void s1ap_remove_ue(ue_description_t *ue_ref)
{
  eNB_description_t *eNB_ref;

  /* NULL reference... */
  if (ue_ref == NULL) return;

  eNB_ref = ue_ref->eNB;

  /* Updating number of UE */
  eNB_ref->nb_ue_associated--;
  /* Remove any attached timer */
  //     s1ap_timer_remove_ue(ue_ref->mme_ue_s1ap_id);

  STAILQ_REMOVE(&eNB_ref->ue_list_head, ue_ref, ue_description_s, ue_entries);
  eNB_ref->nb_ue_associated--;

  /* Freeing memory */
  free(ue_ref);
  ue_ref = NULL;
}

void s1ap_remove_eNB(eNB_description_t *eNB_ref)
{
  ue_description_t *ue_ref;

  if (eNB_ref == NULL) return;

  while (!STAILQ_EMPTY(&eNB_ref->ue_list_head)) {
    ue_ref = STAILQ_FIRST(&eNB_ref->ue_list_head);
    eNB_ref->nb_ue_associated--;
    STAILQ_REMOVE_HEAD(&eNB_ref->ue_list_head, ue_entries);
    free(ue_ref);
  }

  STAILQ_REMOVE(&eNB_list_head, eNB_ref, eNB_description_s, eNB_entries);

  free(eNB_ref);
  eNB_ref = NULL;
  nb_eNB_associated--;
}

#endif
