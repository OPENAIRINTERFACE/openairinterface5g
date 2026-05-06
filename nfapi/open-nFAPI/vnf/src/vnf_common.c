/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <stdlib.h>
#include <errno.h>

#include "vnf.h"
#include "nfapi/oai_integration/vendor_ext.h"

void *vnf_malloc(nfapi_vnf_config_t *config, size_t size)
{
  if (config->malloc) {
    return (config->malloc)(size);
  } else {
    return calloc(1, size);
  }
}

void vnf_free(nfapi_vnf_config_t *config, void *ptr)
{
  if (config->free) {
    return (config->free)(ptr);
  } else {
    return free(ptr);
  }
}

void nfapi_vnf_phy_info_list_add(nfapi_vnf_config_t *config, nfapi_vnf_phy_info_t *info)
{
  NFAPI_TRACE(NFAPI_TRACE_INFO, "Adding phy p5_idx:%d phy_id:%d\n", info->p5_idx, info->phy_id);
  info->next = config->phy_list;
  config->phy_list = info;
}

nfapi_vnf_phy_info_t *nfapi_vnf_phy_info_list_find(nfapi_vnf_config_t *config, uint16_t phy_id)
{
  nfapi_vnf_phy_info_t *curr = config->phy_list;
  while (curr != 0) {
    if (curr->phy_id == phy_id)
      return curr;
    curr = curr->next;
  }
  return 0;
}

void nfapi_vnf_pnf_list_add(nfapi_vnf_config_t *config, nfapi_vnf_pnf_info_t *node)
{
  node->next = config->pnf_list;
  config->pnf_list = node;
}

nfapi_vnf_pnf_info_t *nfapi_vnf_pnf_list_find(nfapi_vnf_config_t *config, int p5_idx)
{
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "config->pnf_list:%p\n", config->pnf_list);

  nfapi_vnf_pnf_info_t *curr = config->pnf_list;
  while (curr != 0) {
    if (curr->p5_idx == p5_idx)
      return curr;
    curr = curr->next;
  }
  NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s(): could not find P5 connection for p5_idx %d\n", __func__, p5_idx);

  return 0;
}

void vnf_handle_vendor_extension(void *pRecvMsg,
                                  int recvMsgLen,
                                  nfapi_vnf_config_t *config,
                                  int p5_idx,
                                  uint16_t message_id)
{
  NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);

  if (config->allocate_p4_p5_vendor_ext && config->deallocate_p4_p5_vendor_ext) {
    uint16_t msg_size;

    nfapi_p4_p5_message_header_t *msg = config->allocate_p4_p5_vendor_ext(message_id, &msg_size);

    if (msg) {
      if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, msg, msg_size, &config->codec_config) >= 0) {
        if (config->vendor_ext)
          config->vendor_ext(config, p5_idx, msg);
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
      }

      config->deallocate_p4_p5_vendor_ext(msg);
    } else {
      NFAPI_TRACE(NFAPI_TRACE_INFO, "failed to allocate vendor extention structure\n");
    }
  }
}

static int vnf_send_p5_msg(vnf_t *vnf, nfapi_vnf_pnf_info_t *pnf, const void *msg, int len, uint8_t stream)
{
  ssize_t result = -1;
  if (vnf->sctp) {
    result = sctp_sendmsg(pnf->p5_sock,
                          msg,
                          len,
                          (struct sockaddr *)&pnf->p5_pnf_sockaddr,
                          sizeof(pnf->p5_pnf_sockaddr),
                          1,
                          0,
                          stream,
                          0,
                          4);
  } else {
    result = send(pnf->p5_sock, msg, len, 0);
  }

  if (result != len) {
    if (result < 0) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "sctp sendto failed errno: %d\n", errno);
    }
  }

  return 0;
}

int vnf_pack_and_send_p5_message(vnf_t *vnf, uint16_t p5_idx, nfapi_p4_p5_message_header_t *msg, uint16_t msg_len)
{
  nfapi_vnf_pnf_info_t *pnf = nfapi_vnf_pnf_list_find(&(vnf->_public), p5_idx);

  if (pnf) {
    int packedMessageLength =
        nfapi_p5_message_pack(msg, msg_len, vnf->tx_message_buffer, sizeof(vnf->tx_message_buffer), &vnf->_public.codec_config);

    if (packedMessageLength < 0) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p5_message_pack failed with return %d\n", packedMessageLength);
      return -1;
    }

    return vnf_send_p5_msg(vnf, pnf, vnf->tx_message_buffer, packedMessageLength, 0);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() cannot find pnf info for p5_idx:%d\n", __FUNCTION__, p5_idx);
    return -1;
  }
}

int vnf_pack_and_send_p4_message(vnf_t *vnf, uint16_t p5_idx, nfapi_p4_p5_message_header_t *msg, uint16_t msg_len)
{
  nfapi_vnf_pnf_info_t *pnf = nfapi_vnf_pnf_list_find(&(vnf->_public), p5_idx);

  if (pnf) {
    int packedMessageLength =
        nfapi_p4_message_pack(msg, msg_len, vnf->tx_message_buffer, sizeof(vnf->tx_message_buffer), &vnf->_public.codec_config);

    if (packedMessageLength < 0) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p4_message_pack failed with return %d\n", packedMessageLength);
      return -1;
    }

    return vnf_send_p5_msg(vnf, pnf, vnf->tx_message_buffer, packedMessageLength, 0);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() cannot find pnf info for p5_idx:%d\n", __FUNCTION__, p5_idx);
    return -1;
  }
}
