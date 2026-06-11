/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#include "vnf_nr.h"
#include "nfapi_nr_interface.h"
#include "nfapi_nr_interface_scf.h"
#include "nfapi/oai_integration/vendor_ext.h"

void vnf_nr_handle_pnf_param_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s : NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_PARAM.reponse\n");

    nfapi_nr_pnf_param_response_t msg;

    const bool result = config->unpack_func(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config);
    if (result) {
      if (config->pnf_nr_param_resp) {
        (config->pnf_nr_param_resp)(config, p5_idx, &msg);
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s(): no pnf_nr_param_resp cb installed\n", __func__);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_nr_handle_pnf_config_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_CONFIG_RESPONSE\n");

    nfapi_nr_pnf_config_response_t msg;

    const bool result = config->unpack_func(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config);
    if (result) {
      if (config->pnf_nr_config_resp) {
        (config->pnf_nr_config_resp)(config, p5_idx, &msg);
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s(): no pnf_nr_config_resp cb installed\n", __func__);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_nr_handle_pnf_start_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_START_RESPONSE\n");

    nfapi_nr_pnf_start_response_t msg;

    const bool result = config->unpack_func(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config);
    if (result) {
      if (config->pnf_nr_start_resp) {
        (config->pnf_nr_start_resp)(config, p5_idx, &msg);
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s(): no pnf_nr_start_resp cb installed\n", __func__);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_nr_handle_pnf_stop_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_STOP_RESPONSE\n");

    nfapi_nr_pnf_stop_response_t msg;

    const bool result = config->unpack_func(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config);
    if (result) {
      if (config->pnf_stop_resp) {
        (config->pnf_stop_resp)(config, p5_idx, (nfapi_pnf_stop_response_t *)&msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_nr_handle_param_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PARAM_RESPONSE\n");

    nfapi_nr_param_response_scf_t msg;

    const bool result = config->unpack_func(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config);
    if (result) {
      if (msg.error_code == NFAPI_NR_PARAM_MSG_OK) {
        nfapi_vnf_phy_info_t *phy_info = nfapi_vnf_phy_info_list_find(config, msg.header.phy_id);
        if (phy_info) {
          if (msg.nfapi_config.p7_pnf_address_ipv4.tl.tag) {
            struct sockaddr_in sockAddr;
            (void)memcpy(&sockAddr.sin_addr.s_addr,
                         msg.nfapi_config.p7_pnf_address_ipv4.address,
                         NFAPI_IPV4_ADDRESS_LENGTH);
            NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF P7 IPv4 address: %s\n", inet_ntoa(sockAddr.sin_addr));
            phy_info->p7_pnf_address.sin_addr = sockAddr.sin_addr;
          }

          if (msg.nfapi_config.p7_pnf_address_ipv6.tl.tag) {
            struct sockaddr_in6 sockAddr6;
            char addr6[64];
            (void)memcpy(&sockAddr6.sin6_addr, msg.nfapi_config.p7_pnf_address_ipv6.address, NFAPI_IPV6_ADDRESS_LENGTH);
            NFAPI_TRACE(NFAPI_TRACE_INFO,
                        "PNF P7 IPv6 address: %s\n",
                        inet_ntop(AF_INET6, &sockAddr6.sin6_addr, addr6, sizeof(addr6)));
          }

          if (msg.nfapi_config.p7_pnf_port.tl.tag) {
            NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF P7 Port: %d\n", msg.nfapi_config.p7_pnf_port.value);
            phy_info->p7_pnf_address.sin_port = htons(msg.nfapi_config.p7_pnf_port.value);
          }
        }
      }

      if (config->nr_param_resp) {
        (config->nr_param_resp)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_nr_handle_config_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received CONFIG_RESPONSE\n");

    nfapi_nr_config_response_scf_t msg;

    const bool result = config->unpack_func(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config);
    if (result) {
      if (msg.error_code == NFAPI_NR_CONFIG_MSG_OK) {
        if (config->nr_config_resp) {
          (config->nr_config_resp)(config, p5_idx, &msg);
        }
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_nr_handle_start_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received START_RESPONSE\n");

    nfapi_nr_start_response_scf_t msg;

    const bool result = config->unpack_func(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config);
    if (result) {
      if (msg.error_code == NFAPI_NR_START_MSG_OK) {
        if (config->nr_start_resp) {
          (config->nr_start_resp)(config, p5_idx, &msg);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s(): no nr_start_resp cb installed\n", __func__);
        }
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_nr_handle_error_indication(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received ERROR.indication\n");

    nfapi_nr_error_indication_scf_t msg;

    if (config->unpack_func(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config)) {
      if (config->nr_error_ind) {
        (config->nr_error_ind)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_nr_handle_stop_indication(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received STOP.indication\n");

    nfapi_nr_stop_indication_scf_t msg;

    if (config->unpack_func(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config)) {
      if (config->nr_stop_ind) {
        (config->nr_stop_ind)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_nr_handle_p4_p5_message(void *pRecvMsg, int recvMsgLen, int p5_idx, nfapi_vnf_config_t *config)
{
  nfapi_nr_p4_p5_message_header_t messageHeader;

  if (pRecvMsg == NULL || recvMsgLen < NFAPI_HEADER_LENGTH || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "vnf_handle_p4_p5_message: invalid input params\n");
    return;
  }

  const bool result = config->hdr_unpack_func(pRecvMsg,
                                               recvMsgLen,
                                               &messageHeader,
                                               sizeof(nfapi_nr_p4_p5_message_header_t),
                                               &config->codec_config);
  if (!result) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unpack message header failed, ignoring\n");
    return;
  }

  switch (messageHeader.message_id) {
    case NFAPI_NR_PHY_MSG_TYPE_PNF_PARAM_RESPONSE:
      vnf_nr_handle_pnf_param_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_NR_PHY_MSG_TYPE_PNF_CONFIG_RESPONSE:
      vnf_nr_handle_pnf_config_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_NR_PHY_MSG_TYPE_PNF_START_RESPONSE:
      vnf_nr_handle_pnf_start_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_PNF_STOP_RESPONSE:
      vnf_nr_handle_pnf_stop_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE:
      vnf_nr_handle_param_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_NR_PHY_MSG_TYPE_CONFIG_RESPONSE:
      vnf_nr_handle_config_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_NR_PHY_MSG_TYPE_ERROR_INDICATION:
      vnf_nr_handle_error_indication(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_NR_PHY_MSG_TYPE_START_RESPONSE:
      vnf_nr_handle_start_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_NR_PHY_MSG_TYPE_STOP_INDICATION:
      vnf_nr_handle_stop_indication(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    default:
      if (messageHeader.message_id >= NFAPI_VENDOR_EXT_MSG_MIN && messageHeader.message_id <= NFAPI_VENDOR_EXT_MSG_MAX) {
        vnf_handle_vendor_extension(pRecvMsg, recvMsgLen, config, p5_idx, messageHeader.message_id);
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s P5 Unknown message ID %d\n", __FUNCTION__, messageHeader.message_id);
      }
      break;
  }
}
