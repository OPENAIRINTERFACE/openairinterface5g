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

/*! \file proto_agent_common.c
 * \brief common primitives for all agents
 * \author Navid Nikaein and Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#include<stdio.h>
#include <dlfcn.h>
#include <time.h>

#include "PHY/phy_extern.h"
#include "proto_agent_common.h"
#include "common/utils/LOG/log.h"
#include "common/ran_context.h"

extern RAN_CONTEXT_t RC;

/*
 * message primitives
 */

// Function to fill in the dl_data header (32bits) with the appropriate fields (doing bitwise operations)
void fill_dl_data_header(int pdu_type, int spare, int seq_no, uint32_t *header) {
  uint32_t type = pdu_type;
  uint32_t spare_ = spare;
  uint32_t seq = seq_no;
  type = type << 28;
  spare_ = spare_ << 24;
  *header = (type | spare_);
  *header = (*header | seq);
  return;
}


// Function to retrieve data from the dl_data header (32bits) (doing bitwise operations)
void read_dl_data_header(int *pdu_type, int *spare, int *seqno, uint32_t header) {
  *pdu_type = header;
  *spare = header;
  *seqno = header;
  *pdu_type = *pdu_type >> 28;
  *spare = *spare << 4;
  *spare = *spare >> 28;
  *seqno = *seqno << 8;
  *seqno = *seqno >> 8;
  return;
}

int f1u_serialize_message(Protocol__F1uMessage *msg, void **buf,int *size) {
  *size = protocol__f1u_message__get_packed_size(msg);
  *buf = malloc(*size);

  if (!(*buf))
    goto error;

  protocol__f1u_message__pack(msg, *buf);
  return 0;
error:
  LOG_E(F1U, "an error occured\n");
  return -1;
}

int f1u_deserialize_message(void *data, int size, Protocol__F1uMessage **msg) {
  *msg = protocol__f1u_message__unpack(NULL, size, data);

  if (*msg == NULL)
    goto error;

  return 0;
error:
  LOG_E(F1U, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int f1u_dl_data_create_header(uint32_t pdu_type, uint32_t f1u_sn, Protocol__DlDataHeader **header) {
  *header = malloc(sizeof(Protocol__DlDataHeader));

  if(*header == NULL)
    goto error;

  protocol__dl_data_header__init(*header);
  LOG_D(F1U, "Initialized the DL Data User header\n");
  fill_dl_data_header(pdu_type, 0, f1u_sn, &(*header)->fields);
  return 0;
error:
  LOG_E(F1U, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int f1u_dl_data(const void *params, Protocol__F1uMessage **msg) {
  // Initialize the PDCP params
  dl_data_args *args = (dl_data_args *)params;
  Protocol__DlDataHeader *header;

  if (f1u_dl_data_create_header(args->pdu_type, args->sn, &header) != 0)
    goto error;

  Protocol__DlUserData *dl_data = NULL;
  *msg = malloc(sizeof(Protocol__DlUserData));

  if(*msg == NULL)
    goto error;

  // FIXME: Is the following used? It seems to be overwritten by the function
  // protocol__dl_user_data__init() anyway
  //dl_data = *msg;
  protocol__dl_user_data__init(dl_data);
  // Copy data to the bytes structure
  dl_data->pdu.data = malloc(args->sdu_size);
  dl_data->pdu.len = args->sdu_size;
  memcpy(dl_data->pdu.data, args->sdu_p, args->sdu_size);
  dl_data->frame = args->frame;
  dl_data->subframe = args->subframe;
  dl_data->rnti = args->rnti;
  dl_data->header = header;
  return 0;
error:

  if(header != NULL)
    free(header);

  if(*msg != NULL)
    free(*msg);

  LOG_E(F1U, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int proto_agent_serialize_message(Protocol__FlexsplitMessage *msg, uint8_t **buf, int *size) {
  *size = protocol__flexsplit_message__get_packed_size(msg);
  *buf = malloc(*size);

  if (!(*buf))
    goto error;

  protocol__flexsplit_message__pack(msg, *buf);
  return 0;
error:
  LOG_E(MAC, "an error occured\n");
  return -1;
}

/* We assume that the buffer size is equal to the message size.
   Should be chekced durint Tx/Rx */
int proto_agent_deserialize_message(void *data, int size, Protocol__FlexsplitMessage **msg) {
  *msg = protocol__flexsplit_message__unpack(NULL, size, data);

  if (*msg == NULL)
    goto error;

  return 0;
error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int fsp_create_header(xid_t xid, Protocol__FspType type,  Protocol__FspHeader **header) {
  *header = malloc(sizeof(Protocol__FspHeader));

  if(*header == NULL)
    goto error;

  protocol__fsp_header__init(*header);
  LOG_D(PROTO_AGENT, "Initialized the PROTOBUF message header\n");
  (*header)->version = FLEXSPLIT_VERSION;
  LOG_D(PROTO_AGENT, "Set the vversion to FLEXSPLIT_VERSION\n");
  (*header)->has_version = 1;
  (*header)->type = type;
  (*header)->has_type = 1;
  (*header)->xid = xid;
  (*header)->has_xid = 1;
  return 0;
error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int just_print(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg) {
  return 1;
}

int proto_agent_pdcp_data_req(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg) {
  Protocol__FspCtxt *ctxt = NULL;
  Protocol__FspRlcPdu *pdu = NULL;
  Protocol__FspRlcData *rlc_data = NULL;
  Protocol__FspRlcDataReq *data_req = NULL;
  // Initialize the PDCP params
  data_req_args *args = (data_req_args *)params;
  // Create the protobuf header
  Protocol__FspHeader *header;
  xid_t xid = mod_id;
  LOG_D(PROTO_AGENT, "creating the data_req message\n");

  if (fsp_create_header(xid, PROTOCOL__FSP_TYPE__FSPT_RLC_DATA_REQ, &header) != 0)
    goto error;

  /* Begin constructing the messages. They are defined as follows:
  *  1) fspRlcPdu is storing the bytes of the packet
  *  2) Message fspRlcData is packing the packet + the context of the PDCP (separate message)
  *  3) Messge fspRlcDataReq is packing the header, enb_id and fspRlcData
  */
  ctxt = malloc(sizeof(Protocol__FspCtxt));
  pdu = malloc(sizeof(Protocol__FspRlcPdu));
  rlc_data = malloc(sizeof(Protocol__FspRlcData));
  data_req = malloc(sizeof(Protocol__FspRlcDataReq));
  protocol__fsp_ctxt__init(ctxt);
  protocol__fsp_rlc_pdu__init(pdu);
  protocol__fsp_rlc_data__init(rlc_data);
  protocol__fsp_rlc_data_req__init(data_req);
  // Copy data to the RlcPdu structure
  pdu->fsp_pdu_data.data =  malloc(args->sdu_size);
  pdu->fsp_pdu_data.len = args->sdu_size;
  memcpy(pdu->fsp_pdu_data.data, args->sdu_p->data, args->sdu_size);
  pdu->has_fsp_pdu_data = 1;
  // Copy data to the ctxt structure
  ctxt->fsp_mod_id = args->ctxt->module_id;
  ctxt->fsp_enb_flag = args->ctxt->enb_flag;
  ctxt->fsp_instance = args->ctxt->instance;
  ctxt->fsp_rnti = args->ctxt->rnti;
  ctxt->fsp_frame = args->ctxt->frame;
  ctxt->fsp_subframe = args->ctxt->subframe;
  ctxt->fsp_enb_index = args->ctxt->eNB_index;
  ctxt->has_fsp_mod_id = 1;
  ctxt->has_fsp_enb_flag = 1;
  ctxt->has_fsp_instance = 1;
  ctxt->has_fsp_rnti = 1;
  ctxt->has_fsp_frame = 1;
  ctxt->has_fsp_subframe = 1;
  ctxt->has_fsp_enb_index = 1;
  rlc_data->fsp_ctxt = ctxt;
  rlc_data->fsp_srb_flag = args->srb_flag;
  rlc_data->fsp_mbms_flag = args->MBMS_flag;
  rlc_data->fsp_rb_id = args->rb_id;
  rlc_data->fsp_muip = args->mui;
  rlc_data->fsp_confirm = args->confirm;
  rlc_data->fsp_sdu_buffer_size = args->sdu_size;
  rlc_data->fsp_pdu = pdu;
  rlc_data->has_fsp_srb_flag = 1;
  rlc_data->has_fsp_mbms_flag = 1;
  rlc_data->has_fsp_rb_id = 1;
  rlc_data->has_fsp_muip = 1;
  rlc_data->has_fsp_confirm = 1;
  rlc_data->has_fsp_sdu_buffer_size = 1;
  // Up to here, everything is a signle message that is packed inside another. The final data_req
  // will be created later, after the setting of all variables
  data_req->header = header;
  data_req->enb_id = mod_id;
  data_req->has_enb_id = 1;
  data_req->pdcp_data = rlc_data;
  *msg = malloc(sizeof(Protocol__FlexsplitMessage));

  if(*msg == NULL)
    goto error;

  protocol__flexsplit_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXSPLIT_MESSAGE__MSG_DATA_REQ_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXSPLIT_DIRECTION__INITIATING_MESSAGE; //we will be waiting for the ACK
  (*msg)->has_msg_dir = 1;
  (*msg)->data_req_msg = data_req;
  return 0;
error:

  if(header != NULL)
    free(header);

  if(pdu!=NULL)
    free(pdu);

  if(rlc_data!=NULL)
    free(rlc_data);

  if(data_req!= NULL)
    free(data_req);

  if(*msg != NULL)
    free(*msg);

  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int proto_agent_destroy_pdcp_data_req(Protocol__FlexsplitMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXSPLIT_MESSAGE__MSG_DATA_REQ_MSG)
    goto error;

  free(msg->data_req_msg->header);
  free(msg->data_req_msg->pdcp_data->fsp_pdu->fsp_pdu_data.data);
  free(msg->data_req_msg->pdcp_data->fsp_pdu);
  free(msg->data_req_msg->pdcp_data->fsp_ctxt);
  free(msg->data_req_msg->pdcp_data);
  free(msg->data_req_msg);
  free(msg);
  return 0;
error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int proto_agent_get_ack_result(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg) {
/* code useless in this status: return 0 anyways
  rlc_op_status_t result = 0;
  printf("PROTO_AGENT: handling the data_req_ack message\n");
  Protocol__FlexsplitMessage *input = (Protocol__FlexsplitMessage *)params;
  Protocol__FspRlcDataReqAck *data_ack = input->data_req_ack;
  result = data_ack->result;
  printf("PROTO_AGENT: ACK RESULT IS %u\n", result);
  ack_result = result;
*/
  return 0;
}


int proto_agent_pdcp_data_req_process(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg) {
  rlc_op_status_t result = 0;
  Protocol__FlexsplitMessage *input = (Protocol__FlexsplitMessage *)params;
  Protocol__FspRlcDataReq *data_req = input->data_req_msg;
  Protocol__FspCtxt *ctxt = NULL;
  Protocol__FspRlcData *rlc_data = NULL;
  rlc_data = data_req->pdcp_data;
  ctxt = rlc_data->fsp_ctxt;
  protocol_ctxt_t  ctxt_pP;
  srb_flag_t       srb_flagP = 0;
  rb_id_t          rb_idP = 0;
  mui_t            muiP = 0;
  confirm_t        confirmP = 0;
  MBMS_flag_t      flag_MBMS = 0;
  sdu_size_t       pdcp_pdu_size = 0;
  mem_block_t     *pdcp_pdu_p = NULL;
  // Create a new protocol context for handling the packet
  ctxt_pP.module_id = ctxt->fsp_mod_id;
  ctxt_pP.enb_flag = ctxt->fsp_enb_flag;
  ctxt_pP.instance = ctxt->fsp_instance;
  ctxt_pP.rnti = ctxt->fsp_rnti;
  ctxt_pP.frame = ctxt->fsp_frame;
  ctxt_pP.subframe = ctxt->fsp_subframe;
  ctxt_pP.eNB_index = ctxt->fsp_enb_index;
  srb_flagP = rlc_data->fsp_srb_flag;
  flag_MBMS = rlc_data->fsp_mbms_flag;
  rb_idP = rlc_data->fsp_rb_id;
  muiP = rlc_data->fsp_muip;
  confirmP = rlc_data->fsp_confirm;
  pdcp_pdu_size = rlc_data->fsp_pdu->fsp_pdu_data.len;
  pdcp_pdu_p = get_free_mem_block(pdcp_pdu_size, __func__);

  if (!pdcp_pdu_p) {
    LOG_E(PROTO_AGENT, "%s: an error occured\n", __FUNCTION__);
    return -1;
  }

  memcpy(pdcp_pdu_p->data, rlc_data->fsp_pdu->fsp_pdu_data.data, pdcp_pdu_size);
  if (RC.nrrrc) {
    LOG_D(PROTO_AGENT, "proto_agent received pdcp_data_req \n");
    // for (int i = 0; i < pdcp_pdu_size; i++)
    //   printf(" %2.2x", (unsigned char)pdcp_pdu_p->data[i]);
    // printf("\n");
    du_rlc_data_req(&ctxt_pP, srb_flagP, flag_MBMS, rb_idP, muiP, confirmP, pdcp_pdu_size, pdcp_pdu_p);
    result = 1;
  } else {
    result = rlc_data_req(&ctxt_pP, srb_flagP, flag_MBMS, rb_idP, muiP, confirmP, pdcp_pdu_size, pdcp_pdu_p, NULL, NULL);
  }
  return result;
}

int proto_agent_destroy_pdcp_data_ind(Protocol__FlexsplitMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXSPLIT_MESSAGE__MSG_DATA_IND_MSG)
    goto error;

  free(msg->data_ind_msg->header);
  free(msg->data_ind_msg->rlc_data->fsp_pdu->fsp_pdu_data.data);
  free(msg->data_ind_msg->rlc_data->fsp_pdu);
  free(msg->data_ind_msg->rlc_data->fsp_ctxt);
  free(msg->data_ind_msg->rlc_data);
  free(msg->data_ind_msg);
  free(msg);
  return 0;
error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int proto_agent_pdcp_data_ind(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg) {
  Protocol__FspCtxt *ctxt = NULL;
  Protocol__FspRlcPdu *pdu = NULL;
  Protocol__FspRlcData *rlc_data = NULL;
  Protocol__FspPdcpDataInd *data_ind = NULL;
  // Initialize the PDCP params
  data_req_args *args = (data_req_args *)params;
  // Create the protobuf header
  Protocol__FspHeader *header;
  xid_t xid = mod_id;
  LOG_D(PROTO_AGENT, "creating the data_ind message\n");

  if (fsp_create_header(xid, PROTOCOL__FSP_TYPE__FSPT_PDCP_DATA_IND, &header) != 0)
    goto error;

  /* Begin constructing the messages. They are defined as follows:
  *  1) fspRlcPdu is storing the bytes of the packet
  *  2) Message fspRlcData is packing the packet + the context of the PDCP (separate message)
  *  3) Messge fspRlcDataReq is packing the header, enb_id and fspRlcData
  */
  ctxt = malloc(sizeof(Protocol__FspCtxt));
  pdu = malloc(sizeof(Protocol__FspRlcPdu));
  rlc_data = malloc(sizeof(Protocol__FspRlcData));
  data_ind = malloc(sizeof(Protocol__FspPdcpDataInd));
  protocol__fsp_ctxt__init(ctxt);
  protocol__fsp_rlc_pdu__init(pdu);
  protocol__fsp_rlc_data__init(rlc_data);
  protocol__fsp_pdcp_data_ind__init(data_ind);
  // Copy data to the RlcPdu structure
  pdu->fsp_pdu_data.data =  malloc(args->sdu_size);
  pdu->fsp_pdu_data.len = args->sdu_size;
  memcpy(pdu->fsp_pdu_data.data, args->sdu_p->data, args->sdu_size);
  pdu->has_fsp_pdu_data = 1;
  // Copy data to the ctxt structure
  ctxt->fsp_mod_id = args->ctxt->module_id;
  ctxt->fsp_enb_flag = args->ctxt->enb_flag;
  ctxt->fsp_instance = args->ctxt->instance;
  ctxt->fsp_rnti = args->ctxt->rnti;
  ctxt->fsp_frame = args->ctxt->frame;
  ctxt->fsp_subframe = args->ctxt->subframe;
  ctxt->fsp_enb_index = args->ctxt->eNB_index;
  ctxt->has_fsp_mod_id = 1;
  ctxt->has_fsp_enb_flag = 1;
  ctxt->has_fsp_instance = 1;
  ctxt->has_fsp_rnti = 1;
  ctxt->has_fsp_frame = 1;
  ctxt->has_fsp_subframe = 1;
  ctxt->has_fsp_enb_index = 1;
  rlc_data->fsp_ctxt = ctxt;
  rlc_data->fsp_srb_flag = args->srb_flag;
  rlc_data->fsp_mbms_flag = args->MBMS_flag;
  rlc_data->fsp_rb_id = args->rb_id;
  rlc_data->fsp_sdu_buffer_size = args->sdu_size;
  rlc_data->fsp_pdu = pdu;
  rlc_data->has_fsp_srb_flag = 1;
  rlc_data->has_fsp_mbms_flag = 1;
  rlc_data->has_fsp_rb_id = 1;
  rlc_data->has_fsp_sdu_buffer_size = 1;
  // Up to here, everything is a signle message that is packed inside another. The final data_req
  // will be created later, after the setting of all variables
  data_ind->header = header;
  data_ind->enb_id = mod_id;
  data_ind->has_enb_id = 1;
  data_ind->rlc_data = rlc_data;
  *msg = malloc(sizeof(Protocol__FlexsplitMessage));

  if(*msg == NULL)
    goto error;

  protocol__flexsplit_message__init(*msg);
  LOG_D(PROTO_AGENT,"setting the message case to %d\n", PROTOCOL__FLEXSPLIT_MESSAGE__MSG_DATA_IND_MSG);
  (*msg)->msg_case = PROTOCOL__FLEXSPLIT_MESSAGE__MSG_DATA_IND_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXSPLIT_DIRECTION__INITIATING_MESSAGE; //we will be waiting for the ACK
  (*msg)->has_msg_dir = 1;
  (*msg)->data_ind_msg = data_ind; //data_req;
  return 0;
error:

  if(header != NULL)
    free(header);

  if(pdu!=NULL)
    free(pdu);

  if(rlc_data!=NULL)
    free(rlc_data);

  if(data_ind!= NULL)
    free(data_ind);

  if(*msg != NULL)
    free(*msg);

  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int proto_agent_pdcp_data_ind_process(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg) {
  boolean_t result = 0;
  Protocol__FlexsplitMessage *input = (Protocol__FlexsplitMessage *)params;
  Protocol__FspPdcpDataInd *data_ind = input->data_ind_msg;
  Protocol__FspCtxt *ctxt = NULL;
  Protocol__FspRlcData *rlc_data = NULL;
  rlc_data = data_ind->rlc_data;
  ctxt = rlc_data->fsp_ctxt;
  protocol_ctxt_t  ctxt_pP;
  srb_flag_t       srb_flagP = 0;
  rb_id_t          rb_idP = 0;
  sdu_size_t       pdcp_pdu_size = 0;
  MBMS_flag_t      flag_MBMS = 0;
  mem_block_t     *pdcp_pdu_p = NULL;
  // Create a new protocol context for handling the packet
  ctxt_pP.module_id = ctxt->fsp_mod_id;
  ctxt_pP.enb_flag = ctxt->fsp_enb_flag;
  ctxt_pP.instance = ctxt->fsp_instance;
  ctxt_pP.rnti = ctxt->fsp_rnti;
  ctxt_pP.frame = ctxt->fsp_frame;
  ctxt_pP.subframe = ctxt->fsp_subframe;
  ctxt_pP.brOption = 0;
  ctxt_pP.eNB_index = ctxt->fsp_enb_index;
  srb_flagP = rlc_data->fsp_srb_flag;
  flag_MBMS = rlc_data->fsp_mbms_flag;
  rb_idP = rlc_data->fsp_rb_id;
  pdcp_pdu_size = rlc_data->fsp_pdu->fsp_pdu_data.len;
  pdcp_pdu_p = get_free_mem_block(pdcp_pdu_size, __func__);

  if (!pdcp_pdu_p) goto error;

  memcpy(pdcp_pdu_p->data, rlc_data->fsp_pdu->fsp_pdu_data.data, pdcp_pdu_size);
  //   if (xid == 1)
  //     pdcp_data_ind_wifi((const protocol_ctxt_t*) ctxt_pP, (const srb_flag_t) srb_flagP, (const MBMS_flag_t) flag_MBMS, (const rb_id_t) rb_idP, pdcp_pdu_size, pdcp_pdu_p);
  //   else if (xid == 0)   // FIXME: USE a preprocessed definition
  LOG_D(PROTO_AGENT, "[inst %ld] Received PDCP PDU with size %d for UE RNTI %x RB %ld, Calling pdcp_data_ind\n", ctxt_pP.instance, pdcp_pdu_size,ctxt_pP.rnti,rb_idP);
  result = pdcp_data_ind(&ctxt_pP,
                         srb_flagP,
                         flag_MBMS,
                         rb_idP,
                         pdcp_pdu_size,
                         pdcp_pdu_p, NULL, NULL);
  return result;
error:

  if (pdcp_pdu_p)
    free_mem_block(pdcp_pdu_p, __func__);

  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int proto_agent_hello(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg) {
  Protocol__FspHeader *header;
  Protocol__FspHello *hello_msg = NULL;
  /*TODO: Need to set random xid or xid from received hello message*/
  xid_t xid = mod_id;

  if (fsp_create_header(xid, PROTOCOL__FSP_TYPE__FSPT_HELLO, &header) != 0)
    goto error;

  LOG_D(PROTO_AGENT, "creating the HELLO message\n");
  hello_msg = malloc(sizeof(Protocol__FspHello));

  if(hello_msg == NULL)
    goto error;

  protocol__fsp_hello__init(hello_msg);
  hello_msg->header = header;
  *msg = malloc(sizeof(Protocol__FlexsplitMessage));

  if(*msg == NULL)
    goto error;

  protocol__flexsplit_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXSPLIT_MESSAGE__MSG_HELLO_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXSPLIT_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->has_msg_dir = 1;
  (*msg)->hello_msg = hello_msg;
  return 0;
error:

  if(header != NULL)
    free(header);

  if(hello_msg!=NULL)
    free(hello_msg);

  if(*msg != NULL)
    free(*msg);

  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int proto_agent_destroy_hello(Protocol__FlexsplitMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXSPLIT_MESSAGE__MSG_HELLO_MSG)
    goto error;

  free(msg->hello_msg->header);
  free(msg->hello_msg);
  free(msg);
  return 0;
error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int proto_agent_echo_request(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg) {
  Protocol__FspHeader *header;
  Protocol__FspEchoRequest *echo_request_msg = NULL;
  xid_t xid = mod_id;

  if (fsp_create_header(xid, PROTOCOL__FSP_TYPE__FSPT_ECHO_REQUEST, &header) != 0)
    goto error;

  LOG_D(PROTO_AGENT, "creating the echo request message\n");
  echo_request_msg = malloc(sizeof(Protocol__FspEchoRequest));

  if(echo_request_msg == NULL)
    goto error;

  protocol__fsp_echo_request__init(echo_request_msg);
  echo_request_msg->header = header;
  *msg = malloc(sizeof(Protocol__FlexsplitMessage));

  if(*msg == NULL)
    goto error;

  protocol__flexsplit_message__init(*msg);
  LOG_D(PROTO_AGENT,"setting the message direction to %d\n", PROTOCOL__FLEXSPLIT_MESSAGE__MSG_ECHO_REQUEST_MSG);
  (*msg)->msg_case = PROTOCOL__FLEXSPLIT_MESSAGE__MSG_ECHO_REQUEST_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXSPLIT_DIRECTION__INITIATING_MESSAGE;
  (*msg)->has_msg_dir = 1;
  (*msg)->echo_request_msg = echo_request_msg;
  return 0;
error:

  if(header != NULL)
    free(header);

  if(echo_request_msg != NULL)
    free(echo_request_msg);

  if(*msg != NULL)
    free(*msg);

  return -1;
}

int proto_agent_destroy_echo_request(Protocol__FlexsplitMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXSPLIT_MESSAGE__MSG_ECHO_REQUEST_MSG)
    goto error;

  free(msg->echo_request_msg->header);
  free(msg->echo_request_msg);
  free(msg);
  return 0;
error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int proto_agent_echo_reply(mod_id_t mod_id, const void *params, Protocol__FlexsplitMessage **msg) {
  xid_t xid;
  Protocol__FlexsplitMessage *input = (Protocol__FlexsplitMessage *)params;
  Protocol__FspEchoRequest *echo_req = input->echo_request_msg;
  Protocol__FspEchoReply *echo_reply_msg = NULL;
  xid = (echo_req->header)->xid;
  LOG_D(PROTO_AGENT, "creating the echo reply message\n");
  Protocol__FspHeader *header;

  if (fsp_create_header(xid, PROTOCOL__FSP_TYPE__FSPT_ECHO_REPLY, &header) != 0)
    goto error;

  echo_reply_msg = malloc(sizeof(Protocol__FspEchoReply));

  if(echo_reply_msg == NULL)
    goto error;

  protocol__fsp_echo_reply__init(echo_reply_msg);
  echo_reply_msg->header = header;
  *msg = malloc(sizeof(Protocol__FlexsplitMessage));

  if(*msg == NULL)
    goto error;

  protocol__flexsplit_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXSPLIT_MESSAGE__MSG_ECHO_REPLY_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXSPLIT_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->has_msg_dir = 1;
  (*msg)->echo_reply_msg = echo_reply_msg;
  return 0;
error:

  if(header != NULL)
    free(header);

  if(echo_reply_msg != NULL)
    free(echo_reply_msg);

  if(*msg != NULL)
    free(*msg);

  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int proto_agent_destroy_echo_reply(Protocol__FlexsplitMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXSPLIT_MESSAGE__MSG_ECHO_REPLY_MSG)
    goto error;

  free(msg->echo_reply_msg->header);
  free(msg->echo_reply_msg);
  free(msg);
  return 0;
error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}
