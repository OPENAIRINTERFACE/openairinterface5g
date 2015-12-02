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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/*
                                play_scenario_s1ap.c
                                -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <crypt.h>
#include <sys/time.h>
#include "tree.h"
#include "queue.h"


#include "intertask_interface.h"
#include "timer.h"
#include "platform_types.h"
#include "assertions.h"
#include "conversions.h"
#include "s1ap_common.h"
#include "play_scenario_s1ap_eNB_defs.h"
#include "play_scenario.h"
#include "msc.h"
//------------------------------------------------------------------------------
s1ap_eNB_internal_data_t s1ap_eNB_internal_data;
RB_GENERATE(s1ap_mme_map, s1ap_eNB_mme_data_s, entry, et_s1ap_eNB_compare_assoc_id);
//------------------------------------------------------------------------------
extern et_scenario_t  *g_scenario;
extern uint32_t        g_constraints;
//------------------------------------------------------------------------------
int et_s1ap_eNB_compare_assoc_id(
  struct s1ap_eNB_mme_data_s *p1, struct s1ap_eNB_mme_data_s *p2)
{
  if (p1->assoc_id == -1) {
    if (p1->cnx_id < p2->cnx_id) {
      return -1;
    }

    if (p1->cnx_id > p2->cnx_id) {
      return 1;
    }
  } else {
    if (p1->assoc_id < p2->assoc_id) {
      return -1;
    }

    if (p1->assoc_id > p2->assoc_id) {
      return 1;
    }
  }

  /* Matching reference */
  return 0;
}
//------------------------------------------------------------------------------
uint32_t et_s1ap_generate_eNB_id(void)
{
  char    *out;
  char     hostname[50];
  int      ret;
  uint32_t eNB_id;

  /* Retrieve the host name */
  ret = gethostname(hostname, sizeof(hostname));
  DevAssert(ret == 0);

  out = crypt(hostname, "eurecom");
  DevAssert(out != NULL);

  eNB_id = ((out[0] << 24) | (out[1] << 16) | (out[2] << 8) | out[3]);

  return eNB_id;
}
//------------------------------------------------------------------------------
uint16_t et_s1ap_eNB_fetch_add_global_cnx_id(void)
{
  return ++s1ap_eNB_internal_data.global_cnx_id;
}

//------------------------------------------------------------------------------
void et_s1ap_eNB_prepare_internal_data(void)
{
  memset(&s1ap_eNB_internal_data, 0, sizeof(s1ap_eNB_internal_data));
  STAILQ_INIT(&s1ap_eNB_internal_data.s1ap_eNB_instances_head);
}

//------------------------------------------------------------------------------
void et_s1ap_eNB_insert_new_instance(s1ap_eNB_instance_t *new_instance_p)
{
  DevAssert(new_instance_p != NULL);

  STAILQ_INSERT_TAIL(&s1ap_eNB_internal_data.s1ap_eNB_instances_head,
                     new_instance_p, s1ap_eNB_entries);
}

//------------------------------------------------------------------------------
struct s1ap_eNB_mme_data_s *et_s1ap_eNB_get_MME(
  s1ap_eNB_instance_t *instance_p,
  int32_t assoc_id, uint16_t cnx_id)
{
  struct s1ap_eNB_mme_data_s  temp;
  struct s1ap_eNB_mme_data_s *found;

  memset(&temp, 0, sizeof(struct s1ap_eNB_mme_data_s));

  temp.assoc_id = assoc_id;
  temp.cnx_id   = cnx_id;

  if (instance_p == NULL) {
    STAILQ_FOREACH(instance_p, &s1ap_eNB_internal_data.s1ap_eNB_instances_head,
                   s1ap_eNB_entries) {
      found = RB_FIND(s1ap_mme_map, &instance_p->s1ap_mme_head, &temp);

      if (found != NULL) {
        return found;
      }
    }
  } else {
    return RB_FIND(s1ap_mme_map, &instance_p->s1ap_mme_head, &temp);
  }

  return NULL;
}

//------------------------------------------------------------------------------
s1ap_eNB_instance_t *et_s1ap_eNB_get_instance(instance_t instance)
{
  s1ap_eNB_instance_t *temp = NULL;

  STAILQ_FOREACH(temp, &s1ap_eNB_internal_data.s1ap_eNB_instances_head,
                 s1ap_eNB_entries) {
    if (temp->instance == instance) {
      /* Matching occurence */
      return temp;
    }
  }

  return NULL;
}
//------------------------------------------------------------------------------
void et_s1ap_eNB_itti_send_sctp_data_req(instance_t instance, int32_t assoc_id, uint8_t *buffer,
                                      uint32_t buffer_length, uint16_t stream)
{
  MessageDef      *message_p;
  sctp_data_req_t *sctp_data_req;

  message_p = itti_alloc_new_message(TASK_S1AP, SCTP_DATA_REQ);

  sctp_data_req = &message_p->ittiMsg.sctp_data_req;

  sctp_data_req->assoc_id      = assoc_id;
  sctp_data_req->buffer        = buffer;
  sctp_data_req->buffer_length = buffer_length;
  sctp_data_req->stream        = stream;

  itti_send_msg_to_task(TASK_SCTP, instance, message_p);
}
//------------------------------------------------------------------------------
int et_s1ap_is_matching(et_s1ap_t * const s1ap1, et_s1ap_t * const s1ap2, const uint32_t constraints)
{
  if (s1ap1->pdu.present != s1ap2->pdu.present)     return -ET_ERROR_MATCH_PACKET_S1AP_PRESENT;
  switch (s1ap1->pdu.present) {
    case  S1AP_PDU_PR_NOTHING:
      break;
    case  S1AP_PDU_PR_initiatingMessage:
      if (s1ap1->pdu.choice.initiatingMessage.procedureCode != s1ap2->pdu.choice.initiatingMessage.procedureCode) return -ET_ERROR_MATCH_PACKET_S1AP_PROCEDURE_CODE;
      if (s1ap1->pdu.choice.initiatingMessage.criticality != s1ap2->pdu.choice.initiatingMessage.criticality) return -ET_ERROR_MATCH_PACKET_S1AP_CRITICALITY;
      break;
    case  S1AP_PDU_PR_successfulOutcome:
      if (s1ap1->pdu.choice.successfulOutcome.procedureCode != s1ap2->pdu.choice.successfulOutcome.procedureCode) return -ET_ERROR_MATCH_PACKET_S1AP_PROCEDURE_CODE;
      if (s1ap1->pdu.choice.successfulOutcome.criticality != s1ap2->pdu.choice.successfulOutcome.criticality) return -ET_ERROR_MATCH_PACKET_S1AP_CRITICALITY;
      break;
    case  S1AP_PDU_PR_unsuccessfulOutcome:
      if (s1ap1->pdu.choice.unsuccessfulOutcome.procedureCode != s1ap2->pdu.choice.unsuccessfulOutcome.procedureCode) return -ET_ERROR_MATCH_PACKET_S1AP_PROCEDURE_CODE;
      if (s1ap1->pdu.choice.unsuccessfulOutcome.criticality != s1ap2->pdu.choice.unsuccessfulOutcome.criticality) return -ET_ERROR_MATCH_PACKET_S1AP_CRITICALITY;
      break;
    default:
      AssertFatal(0, "Unknown pdu.present %d", s1ap1->pdu.present);
  }

  if (s1ap1->binary_stream_allocated_size == s1ap2->binary_stream_allocated_size) {
    if (memcmp((void*)s1ap1->binary_stream, (void*)s1ap2->binary_stream, s1ap1->binary_stream_allocated_size) ==  0) return 0;
  }
  // if no matching, may be the scenario need minor corrections (same enb_ue_s1ap_id but need to update mme_ue_s1ap_id)
  return et_s1ap_ies_is_matching(s1ap1->pdu.present, &s1ap1->message, &s1ap2->message, constraints);
}

//------------------------------------------------------------------------------
et_packet_t* et_build_packet_from_s1ap_data_ind(et_event_s1ap_data_ind_t * const s1ap_data_ind)
{
  et_packet_t     * packet    = NULL;
  AssertFatal (NULL != s1ap_data_ind, "Bad parameter sctp_data_ind\n");
  packet = calloc(1, sizeof(*packet));
  packet->action                                        = ET_PACKET_ACTION_S1C_NULL;
  //packet->time_relative_to_first_packet.tv_sec          = 0;
  //packet->time_relative_to_first_packet.tv_usec         = 0;
  //packet->time_relative_to_last_sent_packet.tv_sec      = 0;
  //packet->time_relative_to_last_sent_packet.tv_usec     = 0;
  //packet->time_relative_to_last_received_packet.tv_sec  = 0;
  //packet->time_relative_to_last_received_packet.tv_usec = 0;
  //packet->original_frame_number                         = 0;
  //packet->packet_number                                 = 0;
  packet->enb_instance = 0; //TODO
  //packet->ip_hdr;
  // keep in mind: allocated buffer: sctp_datahdr.payload.binary_stream
  packet->sctp_hdr.chunk_type = SCTP_CID_DATA;
  memcpy((void*)&packet->sctp_hdr.u.data_hdr, (void*)&s1ap_data_ind->sctp_datahdr, sizeof(packet->sctp_hdr));
  //packet->next = NULL;
  packet->status = ET_PACKET_STATUS_RECEIVED;
  //packet->timer_id = 0;
  AssertFatal(0 == gettimeofday(&packet->timestamp_packet, NULL), "gettimeofday() Failed");
  return packet;
}


//------------------------------------------------------------------------------
void et_scenario_set_packet_received(et_packet_t * const packet)
{
  int rc = 0;
  packet->status = ET_PACKET_STATUS_RECEIVED;
  S1AP_DEBUG("Packet num %d received\n", packet->packet_number);
  if (packet->timer_id != 0) {
    rc = timer_remove(packet->timer_id);
    AssertFatal(rc == 0, "Timer on Rx packet num %d unknown", packet->packet_number);
  }
}

//------------------------------------------------------------------------------
void et_s1ap_process_rx_packet(et_event_s1ap_data_ind_t * const s1ap_data_ind)
{
  et_packet_t     * packet    = NULL;
  et_packet_t     * rx_packet = NULL;
  unsigned long int not_found = 1;
  long              rv        = 0;

  AssertFatal (NULL != s1ap_data_ind, "Bad parameter sctp_data_ind\n");
  rx_packet = et_build_packet_from_s1ap_data_ind(s1ap_data_ind);

  if (NULL == g_scenario->last_rx_packet) {
    packet = g_scenario->list_packet;
    while (NULL != packet) {
      if (packet->action == ET_PACKET_ACTION_S1C_RECEIVE) {
        if (packet->status == ET_PACKET_STATUS_RECEIVED) {
          g_scenario->last_rx_packet = packet;
        } else {
          break;
        }
      }
      packet = packet->next;
    }
    packet = g_scenario->list_packet;
  } else {
    packet = g_scenario->last_rx_packet;
  }
  // not_found threshold may sure depend on number of mme, may be not sure on number of UE
  while ((NULL != packet) && (not_found < 5)) {
    if (packet->action == ET_PACKET_ACTION_S1C_RECEIVE) {
      rv = et_sctp_is_matching(&packet->sctp_hdr, &rx_packet->sctp_hdr, g_constraints);
      if (0 == rv) {
        S1AP_DEBUG("Compare RX packet with packet num %d succeeded\n", packet->packet_number);
        et_scenario_set_packet_received(packet);
      } else {
        S1AP_DEBUG("Compare RX packet with packet num %d failed %s\n", packet->packet_number, et_error_match2str(rv));
      }
    }
    not_found += 1;
    packet = packet->next;
  }
  S1AP_DEBUG("Rx packet not found in scenario:\n");
  et_display_packet_sctp(&rx_packet->sctp_hdr);
}

//------------------------------------------------------------------------------
void et_s1ap_eNB_handle_sctp_data_ind(sctp_data_ind_t * const sctp_data_ind)
{
  int            result = 0;
  et_event_t     event;

  DevAssert(sctp_data_ind != NULL);

  memset((void*)&event, 0, sizeof(event));

  event.code = ET_EVENT_RX_S1AP;
  event.u.s1ap_data_ind.sctp_datahdr.tsn                       = 0;
  event.u.s1ap_data_ind.sctp_datahdr.stream                    = sctp_data_ind->stream;
  event.u.s1ap_data_ind.sctp_datahdr.ssn                       = 0;
  event.u.s1ap_data_ind.sctp_datahdr.ppid                      = S1AP_SCTP_PPID;
  event.u.s1ap_data_ind.sctp_datahdr.assoc_id                  = sctp_data_ind->assoc_id;

  event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream_pos = 0;
  event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream_allocated_size = sctp_data_ind->buffer_length;
  event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream = NULL;
  if ((sctp_data_ind->buffer_length > 0) && (NULL != sctp_data_ind->buffer)) {
    event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream   = calloc(1, sctp_data_ind->buffer_length);
    memcpy(event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream,
           sctp_data_ind->buffer,
           sctp_data_ind->buffer_length);

    if (et_s1ap_decode_pdu(
           &event.u.s1ap_data_ind.sctp_datahdr.payload.pdu,
           &event.u.s1ap_data_ind.sctp_datahdr.payload.message,
           event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream,
           event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream_allocated_size) < 0) {
      AssertFatal (0, "ERROR Cannot decode RX S1AP message!\n");
    }

  }

  result = itti_free(TASK_UNKNOWN, sctp_data_ind->buffer);
  AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);

  et_scenario_fsm_notify_event(event);

}
//------------------------------------------------------------------------------
void et_s1ap_eNB_register_mme(s1ap_eNB_instance_t *instance_p,
                                  net_ip_address_t    *mme_ip_address,
                                  net_ip_address_t    *local_ip_addr,
                                  uint16_t             in_streams,
                                  uint16_t             out_streams)
{
  MessageDef                 *message_p                   = NULL;
  sctp_new_association_req_t *sctp_new_association_req_p  = NULL;
  s1ap_eNB_mme_data_t        *s1ap_mme_data_p             = NULL;

  DevAssert(instance_p != NULL);
  DevAssert(mme_ip_address != NULL);

  message_p = itti_alloc_new_message(TASK_S1AP, SCTP_NEW_ASSOCIATION_REQ);

  sctp_new_association_req_p = &message_p->ittiMsg.sctp_new_association_req;

  sctp_new_association_req_p->port = S1AP_PORT_NUMBER;
  sctp_new_association_req_p->ppid = S1AP_SCTP_PPID;

  sctp_new_association_req_p->in_streams  = in_streams;
  sctp_new_association_req_p->out_streams = out_streams;

  memcpy(&sctp_new_association_req_p->remote_address,
         mme_ip_address,
         sizeof(*mme_ip_address));

  memcpy(&sctp_new_association_req_p->local_address,
         local_ip_addr,
         sizeof(*local_ip_addr));

  /* Create new MME descriptor */
  s1ap_mme_data_p = calloc(1, sizeof(*s1ap_mme_data_p));
  DevAssert(s1ap_mme_data_p != NULL);

  s1ap_mme_data_p->cnx_id                = et_s1ap_eNB_fetch_add_global_cnx_id();
  sctp_new_association_req_p->ulp_cnx_id = s1ap_mme_data_p->cnx_id;

  s1ap_mme_data_p->assoc_id          = -1;
  s1ap_mme_data_p->s1ap_eNB_instance = instance_p;

  memcpy((void*)&s1ap_mme_data_p->mme_net_ip_address, mme_ip_address, sizeof(*mme_ip_address));


  STAILQ_INIT(&s1ap_mme_data_p->served_gummei);

  /* Insert the new descriptor in list of known MME
   * but not yet associated.
   */
  RB_INSERT(s1ap_mme_map, &instance_p->s1ap_mme_head, s1ap_mme_data_p);
  s1ap_mme_data_p->state = S1AP_ENB_STATE_WAITING;
  instance_p->s1ap_mme_nb ++;
  instance_p->s1ap_mme_pending_nb ++;

  itti_send_msg_to_task(TASK_SCTP, instance_p->instance, message_p);
}
//------------------------------------------------------------------------------
void et_s1ap_update_assoc_id_of_packets(const int32_t assoc_id,
                                        struct s1ap_eNB_instance_s * const s1ap_eNB_instance,
                                        s1ap_eNB_mme_data_t        * const mme_desc_p)
{
  et_packet_t     *packet = NULL;
  int              ret;
  unsigned int     old_enb_port = 0;
  unsigned int     old_mme_port = 0;

  S1AP_DEBUG("%s for SCTP association (%u)\n",__FUNCTION__,assoc_id);

  packet = g_scenario->list_packet;
  while (NULL != packet) {
    switch (packet->sctp_hdr.chunk_type) {

      case SCTP_CID_DATA :
        S1AP_DEBUG("%s for SCTP association (%u) SCTP_CID_DATA\n",__FUNCTION__,assoc_id);
        if ((ET_PACKET_STATUS_NONE == packet->status) || (ET_PACKET_STATUS_SCHEDULED_FOR_RECEIVING == packet->status)) {
          if (0 < old_mme_port) {
            if (packet->action == ET_PACKET_ACTION_S1C_SEND) {
              ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.dst, &mme_desc_p->mme_net_ip_address);
              if (0 == ret) {
                ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.src, &s1ap_eNB_instance->s1c_net_ip_address);
                if (0 == ret) {
                  // same IP src, same IP dst
                  if ((packet->sctp_hdr.dst_port == old_mme_port) && (packet->sctp_hdr.src_port == old_enb_port)) {
                    packet->sctp_hdr.u.data_hdr.assoc_id = assoc_id;
                    S1AP_DEBUG("tPacket:\tnum %u  | original frame number %u \n", packet->packet_number, packet->original_frame_number);
                    S1AP_DEBUG("\tUpdated assoc id: %u\n", assoc_id);
                  }
                }
              }
            } else if (packet->action == ET_PACKET_ACTION_S1C_RECEIVE) {
              ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.src, &mme_desc_p->mme_net_ip_address);
              if (0 == ret) {
                ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.dst, &s1ap_eNB_instance->s1c_net_ip_address);
                if (0 == ret) {
                  // same IP src, same IP dst
                  if ((packet->sctp_hdr.src_port == old_mme_port) && (packet->sctp_hdr.dst_port == old_enb_port)) {
                    packet->sctp_hdr.u.data_hdr.assoc_id = assoc_id;
                    S1AP_DEBUG("tPacket:\tnum %u  | original frame number %u \n", packet->packet_number, packet->original_frame_number);
                    S1AP_DEBUG("\tUpdated assoc id: %u\n", assoc_id);
                  }
                }
              }
            }
          }
        }
        break;

        // Strong assumption
        // in replayed scenario, the order of SCTP INIT packets is supposed to be the same as in the catched scenario
      case SCTP_CID_INIT:
        S1AP_DEBUG("%s for SCTP association (%u) SCTP_CID_INIT\n",__FUNCTION__,assoc_id);
        ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.dst, &mme_desc_p->mme_net_ip_address);
        if (0 == ret) {
          ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.src, &s1ap_eNB_instance->s1c_net_ip_address);
          if (0 == ret) {
            if (0 == old_enb_port) {
              if (ET_PACKET_STATUS_NONE == packet->status) {
                packet->status = ET_PACKET_STATUS_SENT;
                old_enb_port = packet->sctp_hdr.src_port;
                S1AP_DEBUG("%s for SCTP association (%u) SCTP_CID_INIT SUCCESS\n",__FUNCTION__,assoc_id);
              }
            }
          }
        }
        break;

      case SCTP_CID_INIT_ACK:
        S1AP_DEBUG("%s for SCTP association (%u) SCTP_CID_INIT_ACK\n",__FUNCTION__,assoc_id);
        ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.src, &mme_desc_p->mme_net_ip_address);
        if (0 == ret) {
          ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.dst, &s1ap_eNB_instance->s1c_net_ip_address);
          if (0 == ret) {
            if (old_enb_port == packet->sctp_hdr.dst_port) {
              if (ET_PACKET_STATUS_NONE == packet->status) {
                packet->status = ET_PACKET_STATUS_RECEIVED;
                old_mme_port = packet->sctp_hdr.dst_port;
                S1AP_DEBUG("%s for SCTP association (%u) SCTP_CID_INIT_ACK SUCCESS\n",__FUNCTION__,assoc_id);
              }
            }
          }
        }
        break;

      case SCTP_CID_HEARTBEAT:
      case SCTP_CID_HEARTBEAT_ACK:
      case SCTP_CID_COOKIE_ECHO:
      case SCTP_CID_COOKIE_ACK:
      case SCTP_CID_ECN_ECNE:
      case SCTP_CID_ECN_CWR:
        break;

      case SCTP_CID_ABORT:
      case SCTP_CID_SHUTDOWN:
      case SCTP_CID_SHUTDOWN_ACK:
      case SCTP_CID_ERROR:
      case SCTP_CID_SHUTDOWN_COMPLETE:
        //TODO
        break;

      default:
        AssertFatal(0, "Unknown chunk_type %d packet num %d", packet->sctp_hdr.chunk_type, packet->packet_number);
        ;
    }
    packet = packet->next;
  }
}
//------------------------------------------------------------------------------
void et_s1ap_handle_s1_setup_message(s1ap_eNB_mme_data_t *mme_desc_p, int sctp_shutdown)
{
  if (sctp_shutdown) {
    /* A previously connected MME has been shutdown */

    /* TODO check if it was used by some eNB and send a message to inform these eNB if there is no more associated MME */
    if (mme_desc_p->state == S1AP_ENB_STATE_CONNECTED) {
      mme_desc_p->state = S1AP_ENB_STATE_DISCONNECTED;

      if (mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb > 0) {
        /* Decrease associated MME number */
        mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb --;
      }

      /* If there are no more associated MME, inform eNB app */
      if (mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb == 0) {
        MessageDef                 *message_p;

        message_p = itti_alloc_new_message(TASK_S1AP, S1AP_DEREGISTERED_ENB_IND);
        S1AP_DEREGISTERED_ENB_IND(message_p).nb_mme = 0;
        itti_send_msg_to_task(TASK_ENB_APP, mme_desc_p->s1ap_eNB_instance->instance, message_p);
      }
    }
  } else {
    /* Check that at least one setup message is pending */
    DevCheck(mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb > 0, mme_desc_p->s1ap_eNB_instance->instance,
             mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb, 0);

    if (mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb > 0) {
      /* Decrease pending messages number */
      mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb --;
      mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb++;
    }

    et_s1ap_update_assoc_id_of_packets(mme_desc_p->assoc_id,
        mme_desc_p->s1ap_eNB_instance,
        mme_desc_p);


    /* If there are no more pending messages, inform eNB app */
    if (mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb == 0) {
      MessageDef                 *message_p;

      message_p = itti_alloc_new_message(TASK_S1AP, S1AP_REGISTER_ENB_CNF);
      S1AP_REGISTER_ENB_CNF(message_p).nb_mme = mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb;
      itti_send_msg_to_task(TASK_ENB_APP, mme_desc_p->s1ap_eNB_instance->instance, message_p);
    }
  }
}
//------------------------------------------------------------------------------
void et_s1ap_eNB_handle_register_eNB(instance_t instance, s1ap_register_enb_req_t *s1ap_register_eNB)
{
  s1ap_eNB_instance_t *new_instance;
  uint8_t index;

  DevAssert(s1ap_register_eNB != NULL);

  /* Look if the provided instance already exists */
  new_instance = et_s1ap_eNB_get_instance(instance);

  if (new_instance != NULL) {
    /* Checks if it is a retry on the same eNB */
    DevCheck(new_instance->eNB_id == s1ap_register_eNB->eNB_id, new_instance->eNB_id, s1ap_register_eNB->eNB_id, 0);
    DevCheck(new_instance->cell_type == s1ap_register_eNB->cell_type, new_instance->cell_type, s1ap_register_eNB->cell_type, 0);
    DevCheck(new_instance->tac == s1ap_register_eNB->tac, new_instance->tac, s1ap_register_eNB->tac, 0);
    DevCheck(new_instance->mcc == s1ap_register_eNB->mcc, new_instance->mcc, s1ap_register_eNB->mcc, 0);
    DevCheck(new_instance->mnc == s1ap_register_eNB->mnc, new_instance->mnc, s1ap_register_eNB->mnc, 0);
    DevCheck(new_instance->mnc_digit_length == s1ap_register_eNB->mnc_digit_length, new_instance->mnc_digit_length, s1ap_register_eNB->mnc_digit_length, 0);
    DevCheck(memcmp((void*)&new_instance->s1c_net_ip_address, (void*)&s1ap_register_eNB->enb_ip_address, sizeof(new_instance->s1c_net_ip_address)) == 0, 0,0,0);
  } else {
    new_instance = calloc(1, sizeof(s1ap_eNB_instance_t));
    DevAssert(new_instance != NULL);

    RB_INIT(&new_instance->s1ap_ue_head);
    RB_INIT(&new_instance->s1ap_mme_head);

    /* Copy usefull parameters */
    new_instance->instance         = instance;
    new_instance->eNB_name         = s1ap_register_eNB->eNB_name;
    new_instance->eNB_id           = s1ap_register_eNB->eNB_id;
    new_instance->cell_type        = s1ap_register_eNB->cell_type;
    new_instance->tac              = s1ap_register_eNB->tac;
    new_instance->mcc              = s1ap_register_eNB->mcc;
    new_instance->mnc              = s1ap_register_eNB->mnc;
    new_instance->mnc_digit_length = s1ap_register_eNB->mnc_digit_length;
    memcpy((void*)&new_instance->s1c_net_ip_address, (void*)&s1ap_register_eNB->enb_ip_address, sizeof(new_instance->s1c_net_ip_address));

    /* Add the new instance to the list of eNB (meaningfull in virtual mode) */
    et_s1ap_eNB_insert_new_instance(new_instance);

    S1AP_DEBUG("Registered new eNB[%d] and %s eNB id %u\n",
               instance,
               s1ap_register_eNB->cell_type == CELL_MACRO_ENB ? "macro" : "home",
               s1ap_register_eNB->eNB_id);
  }

  DevCheck(s1ap_register_eNB->nb_mme <= S1AP_MAX_NB_MME_IP_ADDRESS,
           S1AP_MAX_NB_MME_IP_ADDRESS, s1ap_register_eNB->nb_mme, 0);

  /* Trying to connect to provided list of MME ip address */
  for (index = 0; index < s1ap_register_eNB->nb_mme; index++) {
    et_s1ap_eNB_register_mme(new_instance,
                      &s1ap_register_eNB->mme_ip_address[index],
                          &s1ap_register_eNB->enb_ip_address,
                          s1ap_register_eNB->sctp_in_streams,
                          s1ap_register_eNB->sctp_out_streams);
  }
}

//------------------------------------------------------------------------------
void et_s1ap_eNB_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp)
{
  s1ap_eNB_instance_t *instance_p      = NULL;
  s1ap_eNB_mme_data_t *s1ap_mme_data_p = NULL;

  DevAssert(sctp_new_association_resp != NULL);

  instance_p = et_s1ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  s1ap_mme_data_p = et_s1ap_eNB_get_MME(instance_p, -1,
                                     sctp_new_association_resp->ulp_cnx_id);
  DevAssert(s1ap_mme_data_p != NULL);

  if (sctp_new_association_resp->sctp_state != SCTP_STATE_ESTABLISHED) {
    S1AP_WARN("Received unsuccessful result for SCTP association (%u), instance %d, cnx_id %u\n",
              sctp_new_association_resp->sctp_state,
              instance,
              sctp_new_association_resp->ulp_cnx_id);

    et_s1ap_handle_s1_setup_message(s1ap_mme_data_p, sctp_new_association_resp->sctp_state == SCTP_STATE_SHUTDOWN);

    return;
  }

  S1AP_DEBUG("Received successful result for SCTP association (%u), instance %d, cnx_id %u\n",
             sctp_new_association_resp->sctp_state,
             instance,
             sctp_new_association_resp->ulp_cnx_id);
  /* Update parameters */
  s1ap_mme_data_p->assoc_id    = sctp_new_association_resp->assoc_id;
  s1ap_mme_data_p->in_streams  = sctp_new_association_resp->in_streams;
  s1ap_mme_data_p->out_streams = sctp_new_association_resp->out_streams;

  et_s1ap_handle_s1_setup_message(s1ap_mme_data_p, sctp_new_association_resp->sctp_state == SCTP_STATE_SHUTDOWN);
}

//------------------------------------------------------------------------------
void *et_s1ap_eNB_task(void *arg)
{
  MessageDef *received_msg = NULL;
  int         result;

  S1AP_DEBUG("Starting S1AP layer\n");

  et_s1ap_eNB_prepare_internal_data();

  itti_mark_task_ready(TASK_S1AP);
  MSC_START_USE();

  while (1) {
    itti_receive_msg(TASK_S1AP, &received_msg);

    switch (ITTI_MSG_ID(received_msg)) {
    case TERMINATE_MESSAGE:
      itti_exit_task();
      break;

    case S1AP_REGISTER_ENB_REQ: {
      /* Register a new eNB.
       * in Virtual mode eNBs will be distinguished using the mod_id/
       * Each eNB has to send an S1AP_REGISTER_ENB message with its
       * own parameters.
       */
      et_s1ap_eNB_handle_register_eNB(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                   &S1AP_REGISTER_ENB_REQ(received_msg));
    }
    break;

    case SCTP_NEW_ASSOCIATION_RESP: {
      et_s1ap_eNB_handle_sctp_association_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                            &received_msg->ittiMsg.sctp_new_association_resp);
    }
    break;

    case SCTP_DATA_IND: {
      et_s1ap_eNB_handle_sctp_data_ind(&received_msg->ittiMsg.sctp_data_ind);
    }
    break;

    case TIMER_HAS_EXPIRED:
      LOG_I(S1AP, " Received TIMER_HAS_EXPIRED: timer_id %d\n", TIMER_HAS_EXPIRED(received_msg).timer_id);
      {
        et_packet_t * packet = (et_packet_t*)TIMER_HAS_EXPIRED (received_msg).arg;
        et_event_t    event;
        if (packet->status == ET_PACKET_STATUS_SCHEDULED_FOR_RECEIVING) {
          memset((void*)&event, 0, sizeof(event));
          event.code = ET_EVENT_RX_PACKET_TIME_OUT;
          event.u.rx_packet_time_out = packet;
          et_scenario_fsm_notify_event(event);
        } else if (packet->status == ET_PACKET_STATUS_SCHEDULED_FOR_SENDING) {
          memset((void*)&event, 0, sizeof(event));
          event.code = ET_EVENT_TX_TIMED_PACKET;
          event.u.tx_timed_packet = packet;
          et_scenario_fsm_notify_event(event);
        } else if ((packet->status != ET_PACKET_STATUS_SENT) && ((packet->status != ET_PACKET_STATUS_RECEIVED))) {
          AssertFatal (0, "Bad status %d of packet timed out!\n", packet->status);
        }
      }
      if (TIMER_HAS_EXPIRED (received_msg).timer_id == g_scenario->enb_register_retry_timer_id) {
        /* Restart the registration process */
        g_scenario->registered_enb = 0;
        et_eNB_app_register (g_scenario->enb_properties);
      }
      break;

    default:
      S1AP_ERROR("Received unhandled message: %d:%s\n",
                 ITTI_MSG_ID(received_msg), ITTI_MSG_NAME(received_msg));
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);

    received_msg = NULL;
  }

  return NULL;
}


