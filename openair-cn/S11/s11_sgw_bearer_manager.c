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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "assertions.h"
#include "intertask_interface.h"
#include "queue.h"

#include "NwLog.h"
#include "NwGtpv2c.h"
#include "NwGtpv2cIe.h"
#include "NwGtpv2cMsg.h"
#include "NwGtpv2cMsgParser.h"

#include "sgw_lite_ie_defs.h"

#include "s11_common.h"
#include "s11_sgw_bearer_manager.h"
#include "s11_ie_formatter.h"

int s11_sgw_handle_modify_bearer_request(NwGtpv2cStackHandleT *stack_p,
    NwGtpv2cUlpApiT      *pUlpApi)
{
  NwRcT  rc = NW_OK;
  uint8_t  offendingIeType, offendingIeInstance;
  uint16_t offendingIeLength;

  SgwModifyBearerRequest *modify_bearer_request_p;
  MessageDef             *message_p;

  NwGtpv2cMsgParserT *pMsgParser;

  DevAssert(stack_p != NULL);

  message_p = itti_alloc_new_message(TASK_S11, SGW_MODIFY_BEARER_REQUEST);

  modify_bearer_request_p = &message_p->ittiMsg.sgwModifyBearerRequest;

  modify_bearer_request_p->trxn = (void *)pUlpApi->apiInfo.initialReqIndInfo.hTrxn;
  modify_bearer_request_p->teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);

  /* Create a new message parser */
  rc = nwGtpv2cMsgParserNew(*stack_p,
                            NW_GTP_MODIFY_BEARER_REQ,
                            s11_ie_indication_generic,
                            NULL,
                            &pMsgParser);
  DevAssert(NW_OK == rc);

  /* Indication Flags IE */
  rc = nwGtpv2cMsgParserAddIe(pMsgParser,
                              NW_GTPV2C_IE_INDICATION,
                              NW_GTPV2C_IE_INSTANCE_ZERO,
                              NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
                              s11_indication_flags_ie_get,
                              &modify_bearer_request_p->indication_flags);
  DevAssert(NW_OK == rc);

  /* MME-FQ-CSID IE */
  rc = nwGtpv2cMsgParserAddIe(pMsgParser,
                              NW_GTPV2C_IE_FQ_CSID,
                              NW_GTPV2C_IE_INSTANCE_ZERO,
                              NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
                              s11_fqcsid_ie_get,
                              &modify_bearer_request_p->mme_fq_csid);
  DevAssert(NW_OK == rc);

  /* RAT Type IE */
  rc = nwGtpv2cMsgParserAddIe(pMsgParser,
                              NW_GTPV2C_IE_RAT_TYPE,
                              NW_GTPV2C_IE_INSTANCE_ZERO,
                              NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
                              s11_rat_type_ie_get,
                              &modify_bearer_request_p->rat_type);
  DevAssert(NW_OK == rc);

  /* Delay Value IE */
  rc = nwGtpv2cMsgParserAddIe(pMsgParser,
                              NW_GTPV2C_IE_DELAY_VALUE,
                              NW_GTPV2C_IE_INSTANCE_ZERO,
                              NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
                              s11_delay_value_ie_get,
                              &modify_bearer_request_p->delay_dl_packet_notif_req);
  DevAssert(NW_OK == rc);

  /* Bearer Context to be modified IE */
  rc = nwGtpv2cMsgParserAddIe(pMsgParser,
                              NW_GTPV2C_IE_BEARER_CONTEXT,
                              NW_GTPV2C_IE_INSTANCE_ZERO,
                              NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
                              s11_bearer_context_to_modifiy_ie_get,
                              &modify_bearer_request_p->bearer_context_to_modify);
  DevAssert(NW_OK == rc);

  rc = nwGtpv2cMsgParserRun(pMsgParser,
                            pUlpApi->hMsg,
                            &offendingIeType,
                            &offendingIeInstance,
                            &offendingIeLength);

  if (rc != NW_OK) {
    gtp_cause_t         cause;
    NwGtpv2cUlpApiT     ulp_req;

    memset(&ulp_req, 0, sizeof(NwGtpv2cUlpApiT));
    memset(&cause  , 0, sizeof(gtp_cause_t));

    cause.offending_ie_type     = offendingIeType;
    cause.offending_ie_length   = offendingIeLength;
    cause.offending_ie_instance = offendingIeInstance;

    switch (rc) {
    case NW_GTPV2C_MANDATORY_IE_MISSING:
      S11_DEBUG("Mandatory IE type '%u' of instance '%u' missing!\n",
                offendingIeType, offendingIeLength);
      cause.cause_value = NW_GTPV2C_CAUSE_MANDATORY_IE_MISSING;
      break;

    default:
      S11_DEBUG("Unknown message parse error!\n");
      cause.cause_value = 0;
      break;
    }

    /*
     * Send Create session response with failure to Gtpv2c Stack Instance
     */
    ulp_req.apiType = NW_GTPV2C_ULP_API_TRIGGERED_RSP;

    ulp_req.apiInfo.triggeredRspInfo.hTrxn = pUlpApi->apiInfo.initialReqIndInfo.hTrxn;

    rc = nwGtpv2cMsgNew(*stack_p,
                        NW_TRUE,
                        NW_GTP_CREATE_SESSION_RSP,
                        0,
                        nwGtpv2cMsgGetSeqNumber(pUlpApi->hMsg),
                        &(ulp_req.hMsg));

    s11_cause_ie_set(&(ulp_req.hMsg), &cause);

    S11_DEBUG("Received NW_GTP_CREATE_SESSION_REQ, Sending NW_GTP_CREATE_SESSION_RSP!\n");
    rc = nwGtpv2cProcessUlpReq(*stack_p, &ulp_req);
    DevAssert(NW_OK == rc);

    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    message_p = NULL;

    rc = nwGtpv2cMsgParserDelete(*stack_p, pMsgParser);
    DevAssert(NW_OK == rc);

    rc = nwGtpv2cMsgDelete(*stack_p, (pUlpApi->hMsg));
    DevAssert(NW_OK == rc);

    return 0;
  }

  rc = nwGtpv2cMsgParserDelete(*stack_p, pMsgParser);
  DevAssert(NW_OK == rc);

  rc = nwGtpv2cMsgDelete(*stack_p, (pUlpApi->hMsg));
  DevAssert(NW_OK == rc);

  return itti_send_msg_to_task(TASK_SPGW_APP, INSTANCE_DEFAULT, message_p);
}

int s11_sgw_handle_modify_bearer_response(
  NwGtpv2cStackHandleT    *stack_p,
  SgwModifyBearerResponse *modify_bearer_response_p)
{
  gtp_cause_t         cause;
  NwRcT               rc;
  NwGtpv2cUlpApiT     ulp_req;
  NwGtpv2cTrxnHandleT trxn;

  DevAssert(stack_p != NULL);
  DevAssert(modify_bearer_response_p != NULL);

  trxn = (NwGtpv2cTrxnHandleT)modify_bearer_response_p->trxn;

  /* Prepare a modify bearer response to send to MME.
   */
  memset(&ulp_req, 0, sizeof(NwGtpv2cUlpApiT));
  memset(&cause  , 0, sizeof(gtp_cause_t));

  ulp_req.apiType = NW_GTPV2C_ULP_API_TRIGGERED_RSP;

  ulp_req.apiInfo.triggeredRspInfo.hTrxn = trxn;

  rc = nwGtpv2cMsgNew(*stack_p,
                      NW_TRUE,
                      NW_GTP_MODIFY_BEARER_RSP,
                      0,
                      0,
                      &(ulp_req.hMsg));
  DevAssert(NW_OK == rc);

  /* Set the remote TEID */
  rc = nwGtpv2cMsgSetTeid(ulp_req.hMsg, modify_bearer_response_p->teid);
  DevAssert(NW_OK == rc);

  cause.cause_value = (uint8_t)modify_bearer_response_p->cause;

  s11_cause_ie_set(&(ulp_req.hMsg), &cause);

  rc = nwGtpv2cProcessUlpReq(*stack_p, &ulp_req);
  DevAssert(NW_OK == rc);

  return 0;
}
