/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "vnf.h"
#include "nfapi/oai_integration/vendor_ext.h"

void vnf_handle_pnf_param_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s : NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_PARAM.reponse\n");

    nfapi_pnf_param_response_t msg;

    if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->pnf_param_resp) {
        (config->pnf_param_resp)(config, p5_idx, &msg);
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s(): no pnf_params_resp cb installed\n", __func__);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_pnf_config_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_CONFIG_RESPONSE\n");

    nfapi_pnf_config_response_t msg;

    if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->pnf_config_resp) {
        (config->pnf_config_resp)(config, p5_idx, &msg);
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s(): no pnf_config_resp cb installed\n", __func__);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_pnf_start_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_START_RESPONSE\n");

    nfapi_pnf_start_response_t msg;

    if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->pnf_start_resp) {
        (config->pnf_start_resp)(config, p5_idx, &msg);
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s(): no pnf_start_resp cb installed\n", __func__);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_pnf_stop_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PNF_STOP_RESPONSE\n");

    nfapi_pnf_stop_response_t msg;

    if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->pnf_stop_resp) {
        (config->pnf_stop_resp)(config, p5_idx, &msg);
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s(): no pnf_stop_resp cb installed\n", __func__);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_param_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received PARAM_RESPONSE\n");

    nfapi_param_response_t msg;

    if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (msg.error_code == NFAPI_MSG_OK) {
        nfapi_vnf_phy_info_t *phy_info = nfapi_vnf_phy_info_list_find(config, msg.header.phy_id);

        if (msg.nfapi_config.p7_pnf_address_ipv4.tl.tag) {
          struct sockaddr_in sockAddr;
          (void)memcpy(&sockAddr.sin_addr.s_addr, msg.nfapi_config.p7_pnf_address_ipv4.address, NFAPI_IPV4_ADDRESS_LENGTH);
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

      if (config->param_resp) {
        (config->param_resp)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_config_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received CONFIG_RESPONSE\n");

    nfapi_config_response_t msg;

    if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (msg.error_code == NFAPI_MSG_OK) {
        if (config->config_resp) {
          (config->config_resp)(config, p5_idx, &msg);
        }
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_start_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received START_RESPONSE\n");

    nfapi_start_response_t msg;

    if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->start_resp) {
        (config->start_resp)(config, p5_idx, &msg);
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s(): no start_resp cb installed\n", __func__);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_stop_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received STOP.response\n");

    nfapi_stop_response_t msg;

    if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->stop_resp) {
        (config->stop_resp)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_measurement_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received MEASUREMENT.response\n");

    nfapi_measurement_response_t msg;

    if (nfapi_p5_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->measurement_resp) {
        (config->measurement_resp)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_rssi_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received RSSI.response\n");

    nfapi_rssi_response_t msg;

    if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->rssi_resp) {
        (config->rssi_resp)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_rssi_indication(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received RSSI.indication\n");

    nfapi_rssi_indication_t msg;

    if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->rssi_ind) {
        (config->rssi_ind)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_cell_search_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received CELL_SEARCH.response\n");

    nfapi_cell_search_response_t msg;

    if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->cell_search_resp) {
        (config->cell_search_resp)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_cell_search_indication(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "vnf_handle_cell_search_indication: NULL parameters\n");
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received CELL_SEARCH.indication\n");

    nfapi_cell_search_indication_t msg;

    if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->cell_search_ind) {
        (config->cell_search_ind)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "vnf_handle_cell_search_response: Unpack message failed, ignoring\n");
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_broadcast_detect_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received BROADCAST_DETECT.response\n");

    nfapi_broadcast_detect_response_t msg;

    if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->broadcast_detect_resp) {
        (config->broadcast_detect_resp)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_broadcast_detect_indication(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received BROADCAST_DETECT.indication\n");

    nfapi_broadcast_detect_indication_t msg;

    if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->broadcast_detect_ind) {
        (config->broadcast_detect_ind)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
      return;
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_system_information_schedule_response(void *pRecvMsg,
                                                      int recvMsgLen,
                                                      nfapi_vnf_config_t *config,
                                                      int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received SYSTEM_INFORMATION_SCHEDULE.response\n");

    nfapi_system_information_schedule_response_t msg;

    if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->system_information_schedule_resp) {
        (config->system_information_schedule_resp)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_system_information_schedule_indication(void *pRecvMsg,
                                                        int recvMsgLen,
                                                        nfapi_vnf_config_t *config,
                                                        int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received SYSTEM_INFORMATION_SCHEDULE.indication\n");

    nfapi_system_information_schedule_indication_t msg;

    if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->system_information_schedule_ind) {
        (config->system_information_schedule_ind)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_system_information_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received SYSTEM_INFORMATION.response\n");

    nfapi_system_information_response_t msg;

    if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) >= 0) {
      if (config->system_information_resp) {
        (config->system_information_resp)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_system_information_indication(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received SYSTEM_INFORMATION.indication\n");

    nfapi_system_information_indication_t msg;

    if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(msg), &config->codec_config) < 0) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
      return;
    }

    if (config->system_information_ind) {
      (config->system_information_ind)(config, p5_idx, &msg);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

void vnf_handle_nmm_stop_response(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx)
{
  if (pRecvMsg == NULL || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Received NMM_STOP.response\n");

    nfapi_nmm_stop_response_t msg;

    if (nfapi_p4_message_unpack(pRecvMsg, recvMsgLen, &msg, sizeof(nfapi_nmm_stop_response_t), &config->codec_config) >= 0) {
      if (config->nmm_stop_resp) {
        (config->nmm_stop_resp)(config, p5_idx, &msg);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Unpack message failed, ignoring\n", __FUNCTION__);
    }

    if (msg.vendor_extension)
      config->codec_config.deallocate(msg.vendor_extension);
  }
}

static void vnf_handle_p4_p5_message(void *pRecvMsg, int recvMsgLen, int p5_idx, nfapi_vnf_config_t *config)
{
  nfapi_p4_p5_message_header_t messageHeader;

  if (pRecvMsg == NULL || recvMsgLen < NFAPI_HEADER_LENGTH || config == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "vnf_handle_p4_p5_message: invalid input params\n");
    return;
  }

  if (nfapi_p5_message_header_unpack(pRecvMsg,
                                     recvMsgLen,
                                     &messageHeader,
                                     sizeof(nfapi_p4_p5_message_header_t),
                                     &config->codec_config)
      < 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unpack message header failed, ignoring\n");
    return;
  }

  switch (messageHeader.message_id) {
    case NFAPI_PNF_PARAM_RESPONSE:
      vnf_handle_pnf_param_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_PNF_CONFIG_RESPONSE:
      vnf_handle_pnf_config_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_PNF_START_RESPONSE:
      vnf_handle_pnf_start_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_PNF_STOP_RESPONSE:
      vnf_handle_pnf_stop_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_PARAM_RESPONSE:
      vnf_handle_param_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_CONFIG_RESPONSE:
      vnf_handle_config_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_START_RESPONSE:
      vnf_handle_start_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_STOP_RESPONSE:
      vnf_handle_stop_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_MEASUREMENT_RESPONSE:
      vnf_handle_measurement_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_RSSI_RESPONSE:
      vnf_handle_rssi_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_RSSI_INDICATION:
      vnf_handle_rssi_indication(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_CELL_SEARCH_RESPONSE:
      vnf_handle_cell_search_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_CELL_SEARCH_INDICATION:
      vnf_handle_cell_search_indication(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_BROADCAST_DETECT_RESPONSE:
      vnf_handle_broadcast_detect_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_BROADCAST_DETECT_INDICATION:
      vnf_handle_broadcast_detect_indication(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_SYSTEM_INFORMATION_SCHEDULE_RESPONSE:
      vnf_handle_system_information_schedule_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_SYSTEM_INFORMATION_SCHEDULE_INDICATION:
      vnf_handle_system_information_schedule_indication(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_SYSTEM_INFORMATION_RESPONSE:
      vnf_handle_system_information_response(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_SYSTEM_INFORMATION_INDICATION:
      vnf_handle_system_information_indication(pRecvMsg, recvMsgLen, config, p5_idx);
      break;
    case NFAPI_NMM_STOP_RESPONSE:
      vnf_handle_nmm_stop_response(pRecvMsg, recvMsgLen, config, p5_idx);
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

int vnf_read_dispatch_message(nfapi_vnf_config_t *config, nfapi_vnf_pnf_info_t *pnf)
{
  int socket_connected = 1;

  uint32_t header_buffer_size = NFAPI_HEADER_LENGTH;
  uint8_t header_buffer[header_buffer_size];
  memset(header_buffer, 0, header_buffer_size);

  uint32_t stack_buffer_size = 32;
  uint8_t stack_buffer[stack_buffer_size];

  uint8_t *dynamic_buffer = 0;
  uint8_t *read_buffer = &stack_buffer[0];
  uint32_t message_size = 0;

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  socklen_t addr_len = sizeof(addr);

  struct sctp_sndrcvinfo sndrcvinfo;
  (void)memset(&sndrcvinfo, 0, sizeof(struct sctp_sndrcvinfo));

  {
    int flags = MSG_PEEK;
    message_size =
        sctp_recvmsg(pnf->p5_sock, header_buffer, header_buffer_size, (struct sockaddr *)&addr, &addr_len, &sndrcvinfo, &flags);

    if (message_size == -1) {
      NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Failed to peek sctp message size errno:%d\n", errno);
      return 0;
    }

    nfapi_p4_p5_message_header_t header;
    int unpack_result = nfapi_p5_message_header_unpack(header_buffer, header_buffer_size, &header, sizeof(header), 0);
    if (unpack_result < 0) {
      NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Failed to decode message header %d\n", unpack_result);
      return 0;
    }
    message_size = header.message_length;
  }

  if (message_size > stack_buffer_size) {
    dynamic_buffer = (uint8_t *)malloc(message_size);

    if (dynamic_buffer == NULL) {
      NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Failed to allocate dynamic buffer for sctp_recvmsg size:%d\n", message_size);
      return -1;
    }

    read_buffer = dynamic_buffer;
  }

  {
    int flags = 0;
    (void)memset(&sndrcvinfo, 0, sizeof(struct sctp_sndrcvinfo));

    int recvmsg_result =
        sctp_recvmsg(pnf->p5_sock, read_buffer, message_size, (struct sockaddr *)&addr, &addr_len, &sndrcvinfo, &flags);
    if (recvmsg_result == -1) {
      NFAPI_TRACE(NFAPI_TRACE_INFO, "Failed to read sctp message size errno:%d\n", errno);
    } else {
      if (flags & MSG_NOTIFICATION) {
        NFAPI_TRACE(NFAPI_TRACE_INFO, "Notification received from %s:%u\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
      } else {
        if ((flags & 0x80) == 0x80) {
          vnf_handle_p4_p5_message(read_buffer, message_size, pnf->p5_idx, config);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_WARN, "sctp_recvmsg: unhandled mode with flags 0x%x\n", flags);
          NFAPI_TRACE(NFAPI_TRACE_WARN, "Disconnected socket\n");
          socket_connected = 0;
        }
      }
    }
  }

  if (dynamic_buffer) {
    free(dynamic_buffer);
  }

  return socket_connected;
}
