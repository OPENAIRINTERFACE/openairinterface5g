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

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "assertions.h"
#include "conversions.h"

#include "sgw_lite_messages_types.h"

#include "NwGtpv2c.h"
#include "NwGtpv2cIe.h"
#include "NwGtpv2cMsg.h"
#include "NwGtpv2cMsgParser.h"

#include "s11_common.h"
#include "s11_ie_formatter.h"

NwRcT s11_imsi_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  Imsi_t *imsi;
  uint8_t i;
  uint8_t mask = 0x0F;
  uint8_t imsi_length = 2 * ieLength;

  DevAssert(arg != NULL);
  imsi = (Imsi_t *)arg;

  for (i = 0; i < ieLength * 2; i++) {
    if (mask == 0x0F) {
      imsi->digit[i] = (ieValue[i / 2] & (mask));
    } else {
      imsi->digit[i] = (ieValue[i / 2] & (mask)) >> 4;
    }

    imsi->digit[i] += '0';
    mask = ~mask;
  }

  if (imsi->digit[imsi_length - 1] == (0x0f + '0')) {
    imsi->digit[imsi_length - 1] = 0;
    imsi_length--;
  }

  imsi->length = imsi_length;

  S11_DEBUG("\t- IMSI length %d\n", imsi->length);
  S11_DEBUG("\t-      value  %*s\n", imsi->length, imsi->digit);

  return NW_OK;
}

int s11_imsi_ie_set(NwGtpv2cMsgHandleT *msg, const Imsi_t *imsi)
{
  uint8_t *temp = NULL;
  uint8_t imsi_length, i;
  NwRcT   rc;

  DevAssert(msg != NULL);
  DevAssert(imsi != NULL);

  /* In case of odd/even imsi */
  imsi_length = imsi->length % 2 == 0 ? imsi->length / 2 : imsi->length / 2 + 1;

  temp = calloc(imsi_length, sizeof(uint8_t));

  DevAssert(temp != NULL);

  for (i = 0; i < imsi->length; i++) {
    temp[i / 2] |= ((imsi->digit[i] - '0') & 0x0F) << (i % 2 ? 4 : 0);
  }

  rc = nwGtpv2cMsgAddIe(*msg, NW_GTPV2C_IE_IMSI, imsi_length, 0, temp);
  DevAssert(NW_OK == rc);

  free(temp);

  return 0;
}

NwRcT s11_msisdn_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  Msisdn_t *msisdn;
  uint8_t   i;
  uint8_t   mask = 0x0F;
  uint8_t   msisdn_length = 2 * ieLength;

  DevAssert(arg != NULL);
  msisdn = (Msisdn_t *)arg;

  for (i = 0; i < ieLength * 2; i++) {
    if (mask == 0x0F) {
      msisdn->digit[i] = (ieValue[i / 2] & (mask));
    } else {
      msisdn->digit[i] = (ieValue[i / 2] & (mask)) >> 4;
    }

    msisdn->digit[i] += '0';
    mask = ~mask;
  }

  if (msisdn->digit[msisdn_length - 1] == (0x0f + '0')) {
    msisdn->digit[msisdn_length - 1] = 0;
    msisdn_length--;
  }

  msisdn->length = msisdn_length;

  S11_DEBUG("\t- MSISDN length %d\n", msisdn->length);
  S11_DEBUG("\t-        value  %*s\n", msisdn->length, (char *)msisdn->digit);

  return NW_OK;
}

NwRcT s11_mei_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  Mei_t *mei = (Mei_t *)arg;

  DevAssert(mei != NULL);



  return NW_OK;
}

NwRcT s11_pdn_type_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  pdn_type_t *pdn_type = (pdn_type_t *)arg;

  DevAssert(pdn_type != NULL);

  if (*ieValue == 1) {
    /* Only IPv4 */
    *pdn_type = IPv4;
  } else if (*ieValue == 2) {
    /* Only IPv6 */
    *pdn_type = IPv6;
  } else if (*ieValue == 3) {
    /* IPv4 and/or IPv6 */
    *pdn_type = IPv4_AND_v6;
  } else {
    S11_ERROR("Received unknown value for PDN Type: %u\n", *ieValue);
    return NW_GTPV2C_IE_INCORRECT;
  }

  S11_DEBUG("\t- PDN type %u\n", *pdn_type);

  return NW_OK;
}

int s11_pdn_type_ie_set(NwGtpv2cMsgHandleT *msg, const pdn_type_t *pdn_type)
{
  NwRcT   rc;
  uint8_t value;

  DevAssert(pdn_type != NULL);
  DevAssert(msg != NULL);

  switch (*pdn_type) {
  case IPv4:
    value = 1;
    break;

  case IPv6:
    value = 2;
    break;

  case IPv4_AND_v6:
  case IPv4_OR_v6:
    value = 3;
    break;

  default:
    S11_ERROR("Invalid PDN type received: %d\n", *pdn_type);
    return -1;
  }

  rc = nwGtpv2cMsgAddIe(*msg, NW_GTPV2C_IE_PDN_TYPE, 1, 0, (uint8_t*)&value);
  DevAssert(NW_OK == rc);

  return 0;
}

NwRcT s11_rat_type_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  rat_type_t *rat_type = (rat_type_t *)arg;

  DevAssert(rat_type != NULL);

  switch (*ieValue) {
  case 1:
    *rat_type = RAT_UTRAN;
    break;

  case 2:
    *rat_type = RAT_GERAN;
    break;

  case 3:
    *rat_type = RAT_WLAN;
    break;

  case 4:
    *rat_type = RAT_GAN;
    break;

  case 5:
    *rat_type = RAT_HSPA_EVOLUTION;
    break;

  case 6:
    *rat_type = RAT_EUTRAN;
    break;

  default:
    S11_ERROR("Can't map GTP RAT type %u to EPC definition\n"
              "\tCheck TS.29.274 #8.17 for possible values\n", *ieValue);
    return NW_GTPV2C_IE_INCORRECT;
  }

  S11_DEBUG("\t- RAT type (%d): %d\n", *ieValue, *rat_type);

  return NW_OK;
}

int s11_rat_type_ie_set(NwGtpv2cMsgHandleT *msg, const rat_type_t *rat_type)
{
  NwRcT   rc;
  uint8_t value;

  DevAssert(rat_type != NULL);
  DevAssert(msg != NULL);

  switch (*rat_type) {
  case RAT_UTRAN:
    value = 1;
    break;

  case RAT_GERAN:
    value = 2;
    break;

  case RAT_WLAN:
    value = 3;
    break;

  case RAT_GAN:
    value = 4;
    break;

  case RAT_HSPA_EVOLUTION:
    value = 5;
    break;

  case RAT_EUTRAN:
    value = 6;
    break;

  default:
    S11_ERROR("Can't map RAT type %d to GTP RAT type\n"
              "\tCheck TS.29.274 #8.17 for possible values\n", *rat_type);
    return -1;
  }

  rc = nwGtpv2cMsgAddIe(*msg, NW_GTPV2C_IE_RAT_TYPE, 1, 0, (uint8_t*)&value);
  DevAssert(NW_OK == rc);

  return 0;
}

int s11_ebi_ie_set(NwGtpv2cMsgHandleT *msg, const unsigned ebi)
{
  NwRcT   rc;
  uint8_t value = 0;

  value = ebi & 0x0F;

  rc = nwGtpv2cMsgAddIe(*msg, NW_GTPV2C_IE_EBI, 1, 0,
                        &value);
  DevAssert(NW_OK == rc);

  return 0;
}

NwRcT s11_ebi_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  uint8_t *ebi = (uint8_t *)arg;

  DevAssert(ebi != NULL);

  *ebi = ieValue[0] & 0x0F;

  S11_DEBUG("\t- EBI %u\n", *ebi);

  return NW_OK;
}

NwRcT s11_cause_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  SGWCause_t *cause = (SGWCause_t *)arg;

  DevAssert(cause != NULL);

  *cause = ieValue[0];

  S11_DEBUG("\t- Cause %u\n", *cause);

  return NW_OK;
}

int s11_cause_ie_set(NwGtpv2cMsgHandleT *msg,
                     const gtp_cause_t  *cause)
{
  NwRcT rc;
  uint8_t value[6];

  DevAssert(msg != NULL);
  DevAssert(cause != NULL);

  value[0] = cause->cause_value;
  value[1] = ((cause->pce & 0x1) << 2) | ((cause->bce & 0x1) << 1) | (cause->cs & 0x1);

  if (cause->offending_ie_type != 0) {
    value[2] = cause->offending_ie_type;
    value[3] = (cause->offending_ie_length & 0xFF00) >> 8;
    value[4] = cause->offending_ie_length & 0x00FF;
    value[5] = cause->offending_ie_instance & 0x0F;
    rc = nwGtpv2cMsgAddIe(*msg, NW_GTPV2C_IE_CAUSE, 6, 0, value);
  } else {
    rc = nwGtpv2cMsgAddIe(*msg, NW_GTPV2C_IE_CAUSE, 2, 0, value);
  }

  DevAssert(NW_OK == rc);

  return rc == NW_OK ? 0 : -1;
}

NwRcT s11_bearer_context_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  bearer_to_create_t *bearer_to_create = (bearer_to_create_t *)arg;
  uint8_t read = 0;
  NwRcT   rc;

  DevAssert(bearer_to_create != NULL);

  while (ieLength > read) {
    NwGtpv2cIeTlvT *ie_p;

    ie_p = (NwGtpv2cIeTlvT*)&ieValue[read];

    switch (ie_p->t) {
    case NW_GTPV2C_IE_EBI:
      rc = s11_ebi_ie_get(ie_p->t, ie_p->l, ie_p->i,
                          &ieValue[read + sizeof(NwGtpv2cIeTlvT)],
                          &bearer_to_create->eps_bearer_id);
      DevAssert(NW_OK == rc);
      break;

    case NW_GTPV2C_IE_BEARER_LEVEL_QOS:
      break;

    default:
      S11_ERROR("Received unexpected IE %u\n", ie_p->t);
      return NW_GTPV2C_IE_INCORRECT;
    }

    read += (ntohs(ie_p->l) + sizeof(NwGtpv2cIeTlvT));
  }

  return NW_OK;
}

int s11_bearer_context_to_create_ie_set(
  NwGtpv2cMsgHandleT *msg, const bearer_to_create_t *bearer_to_create)
{
  NwRcT rc;

  DevAssert(msg != NULL);
  DevAssert(bearer_to_create != NULL);

  /* Start section for grouped IE: bearer context to create */
  rc = nwGtpv2cMsgGroupedIeStart(*msg, NW_GTPV2C_IE_BEARER_CONTEXT,
                                 NW_GTPV2C_IE_INSTANCE_ZERO);
  DevAssert(NW_OK == rc);

  s11_ebi_ie_set(msg, bearer_to_create->eps_bearer_id);

  /* End section for grouped IE: bearer context to create */
  rc = nwGtpv2cMsgGroupedIeEnd(*msg);
  DevAssert(NW_OK == rc);
  return 0;
}

NwRcT s11_bearer_context_to_modifiy_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  bearer_context_to_modify_t *bearer_to_modify = (bearer_context_to_modify_t *)arg;
  uint8_t read = 0;
  NwRcT   rc;

  DevAssert(bearer_to_modify != NULL);

  while (ieLength > read) {
    NwGtpv2cIeTlvT *ie_p;

    ie_p = (NwGtpv2cIeTlvT*)&ieValue[read];

    switch (ie_p->t) {
    case NW_GTPV2C_IE_EBI:
      rc = s11_ebi_ie_get(ie_p->t, ie_p->l, ie_p->i,
                          &ieValue[read + sizeof(NwGtpv2cIeTlvT)],
                          &bearer_to_modify->eps_bearer_id);
      DevAssert(NW_OK == rc);
      break;

    case NW_GTPV2C_IE_FTEID:
      rc = s11_fteid_ie_get(ie_p->t, ie_p->l, ie_p->i,
                            &ieValue[read + sizeof(NwGtpv2cIeTlvT)],
                            &bearer_to_modify->s1_eNB_fteid);
      break;

    default:
      S11_ERROR("Received unexpected IE %u\n", ie_p->t);
      return NW_GTPV2C_IE_INCORRECT;
    }

    read += (ntohs(ie_p->l) + sizeof(NwGtpv2cIeTlvT));
  }

  return NW_OK;
}

NwRcT s11_bearer_context_created_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  bearer_context_created_t *bearer_created = (bearer_context_created_t *)arg;
  uint8_t read = 0;
  NwRcT   rc;

  DevAssert(bearer_created != NULL);

  while (ieLength > read) {
    NwGtpv2cIeTlvT *ie_p;

    ie_p = (NwGtpv2cIeTlvT*)&ieValue[read];

    switch (ie_p->t) {
    case NW_GTPV2C_IE_EBI:
      rc = s11_ebi_ie_get(ie_p->t, ie_p->l, ie_p->i,
                          &ieValue[read + sizeof(NwGtpv2cIeTlvT)],
                          &bearer_created->eps_bearer_id);
      DevAssert(NW_OK == rc);
      break;

    case NW_GTPV2C_IE_FTEID:
      rc = s11_fteid_ie_get(ie_p->t, ie_p->l, ie_p->i,
                            &ieValue[read + sizeof(NwGtpv2cIeTlvT)],
                            &bearer_created->s1u_sgw_fteid);
      break;

    case NW_GTPV2C_IE_CAUSE:
      rc = s11_cause_ie_get(ie_p->t, ie_p->l, ie_p->i,
                            &ieValue[read + sizeof(NwGtpv2cIeTlvT)],
                            &bearer_created->cause);
      break;

    default:
      S11_ERROR("Received unexpected IE %u\n", ie_p->t);
      return NW_GTPV2C_IE_INCORRECT;
    }

    read += (ntohs(ie_p->l) + sizeof(NwGtpv2cIeTlvT));
  }

  return NW_OK;
}

int s11_bearer_context_created_ie_set(
  NwGtpv2cMsgHandleT *msg, const bearer_context_created_t *bearer)
{
  NwRcT rc;

  DevAssert(msg != NULL);
  DevAssert(bearer != NULL);

  /* Start section for grouped IE: bearer context created */
  rc = nwGtpv2cMsgGroupedIeStart(*msg, NW_GTPV2C_IE_BEARER_CONTEXT,
                                 NW_GTPV2C_IE_INSTANCE_ZERO);
  DevAssert(NW_OK == rc);

  s11_ebi_ie_set(msg, bearer->eps_bearer_id);

  rc = nwGtpv2cMsgAddIeCause(*msg, NW_GTPV2C_IE_INSTANCE_ZERO, bearer->cause,
                             0, 0, 0);
  DevAssert(NW_OK == rc);

  rc = nwGtpv2cMsgAddIeFteid(*msg, NW_GTPV2C_IE_INSTANCE_ZERO,
                             bearer->s1u_sgw_fteid.interface_type,
                             bearer->s1u_sgw_fteid.teid,
                             bearer->s1u_sgw_fteid.ipv4 ? htonl(bearer->s1u_sgw_fteid.ipv4_address) : 0,
                             bearer->s1u_sgw_fteid.ipv6 ? (uint8_t*)bearer->s1u_sgw_fteid.ipv6_address : NULL);
  DevAssert(NW_OK == rc);

  /* End section for grouped IE: bearer context created */
  rc = nwGtpv2cMsgGroupedIeEnd(*msg);
  DevAssert(NW_OK == rc);

  return 0;
}

/* This IE shall be included in the E-UTRAN initial attach,
 * PDP Context Activation and UE Requested PDN connectivity procedures.
 * This IE denotes the most stringent restriction as required
 * by any already active bearer context. If there are no already active bearer
 * contexts, this value is set to the least restrictive type.
 */
int s11_apn_restriction_ie_set(
  NwGtpv2cMsgHandleT *msg, const uint8_t apn_restriction)
{
  NwRcT rc;

  DevAssert(msg != NULL);

  rc = nwGtpv2cMsgAddIe(*msg, NW_GTPV2C_IE_APN_RESTRICTION, 1, 0,
                        (uint8_t *)&apn_restriction);
  DevAssert(NW_OK == rc);

  return 0;
}

NwRcT s11_serving_network_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  ServingNetwork_t *serving_net = (ServingNetwork_t *)arg;

  DevAssert(serving_net != NULL);

  serving_net->mcc[1] = (ieValue[0] & 0xF0) >> 4;
  serving_net->mcc[0] = (ieValue[0] & 0x0F);
  serving_net->mcc[2] = (ieValue[1] & 0x0F);

  if ((ieValue[1] & 0xF0) == 0xF0) {
    /* Two digits MNC */
    serving_net->mnc[0] = 0;
    serving_net->mnc[2] = (ieValue[2] & 0xF0) >> 4;
    serving_net->mnc[1] = (ieValue[2] & 0x0F);
  } else {
    serving_net->mnc[0] = (ieValue[1] & 0xF0) >> 4;
    serving_net->mnc[2] = (ieValue[2] & 0xF0) >> 4;
    serving_net->mnc[1] = (ieValue[2] & 0x0F);
  }

  S11_DEBUG("\t- Serving network %d.%d\n",
            serving_net->mcc[0] * 100 + serving_net->mcc[1] * 10
            + serving_net->mcc[2],
            serving_net->mnc[0] * 100 + serving_net->mnc[1] * 10
            + serving_net->mnc[2]);

  return NW_OK;
}

int s11_serving_network_ie_set(
  NwGtpv2cMsgHandleT     *msg,
  const ServingNetwork_t *serving_network)
{
  NwRcT   rc;
  uint8_t value[3];

  DevAssert(msg != NULL);
  DevAssert(serving_network != NULL);

  /* MCC Decimal | MCC Hundreds */
  value[0] = ((serving_network->mcc[1] & 0x0F) << 4) |
             (serving_network->mcc[2] & 0x0F);
  value[1] = serving_network->mcc[0] & 0x0F;

  if ((serving_network->mnc[0] & 0xF) == 0xF) {
    /* Only two digits */
    value[1] |= 0xF0;
    value[2] = ((serving_network->mnc[2] & 0x0F) << 4) |
               (serving_network->mnc[1] & 0x0F);
  } else {
    value[1] |= (serving_network->mnc[2] & 0x0F) << 4;
    value[2] = ((serving_network->mnc[1] & 0x0F) << 4) |
               (serving_network->mnc[0] & 0x0F);
  }

  rc = nwGtpv2cMsgAddIe(*msg, NW_GTPV2C_IE_SERVING_NETWORK, 3, 0, value);
  DevAssert(NW_OK == rc);

  return 0;
}

NwRcT s11_fteid_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  uint8_t offset = 0;
  FTeid_t *fteid = (FTeid_t *)arg;

  DevAssert(fteid != NULL);

  fteid->ipv4 = (ieValue[0] & 0x80) >> 7;
  fteid->ipv6 = (ieValue[0] & 0x40) >> 6;

  fteid->interface_type = ieValue[0] & 0x1F;
  S11_DEBUG("\t- F-TEID type %d\n", fteid->interface_type);

  /* Copy the TEID or GRE key */
  fteid->teid = ntoh_int32_buf(&ieValue[1]);

  S11_DEBUG("\t- TEID/GRE    %08x\n", fteid->teid);

  if (fteid->ipv4 == 1) {
    /* IPv4 present: copy the 4 bytes */
    memcpy(&fteid->ipv4_address, &ieValue[5], 4);
    offset = 4;

    S11_DEBUG("\t- IPv4 addr   "IPV4_ADDR"\n",
              IPV4_ADDR_FORMAT(fteid->ipv4_address));
  }

  if (fteid->ipv6 == 1) {
    char ipv6_ascii[40];
    /* IPv6 present: copy the 16 bytes
     * WARNING: if Ipv4 is present, 4 bytes of offset should be applied
     */
    memcpy(fteid->ipv6_address, &ieValue[5 + offset], 16);

    inet_ntop(AF_INET6, fteid->ipv6_address, ipv6_ascii, 40);
    S11_DEBUG("\t- IPv6 addr   %s\n", ipv6_ascii);
  }

  return NW_OK;
}

NwRcT s11_paa_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  uint8_t offset = 0;
  PAA_t *paa = (PAA_t *)arg;

  DevAssert(paa != NULL);

  paa->pdn_type = ieValue[0] & 0x07;
  S11_DEBUG("\t- PAA type  %d\n", paa->pdn_type);

  if (paa->pdn_type & 0x2) {
    char ipv6_ascii[40];
    /* IPv6 present: copy the 16 bytes
     * WARNING: if both ipv4 and ipv6 are present,
     *          17 bytes of offset should be applied for ipv4
     * NOTE: an ipv6 prefix length is prepend
     * NOTE: in Rel.8 the prefix length has a default value of /64
     */
    paa->ipv6_prefix_length = ieValue[1];
    memcpy(paa->ipv6_address, &ieValue[2], 16);

    inet_ntop(AF_INET6, paa->ipv6_address, ipv6_ascii, 40);
    S11_DEBUG("\t- IPv6 addr %s/%u\n", ipv6_ascii, paa->ipv6_prefix_length);
  }

  if (paa->pdn_type == 3) {
    offset = 17;
  }

  if (paa->pdn_type & 0x1) {
    memcpy(paa->ipv4_address, &ieValue[1 + offset], 4);

    S11_DEBUG("\t- IPv4 addr "IPV4_ADDR"\n",
              paa->ipv4_address[0], paa->ipv4_address[1],
              paa->ipv4_address[2], paa->ipv4_address[3]);
  }

  paa->pdn_type -= 1;
  return NW_OK;
}

int s11_paa_ie_set(NwGtpv2cMsgHandleT *msg, const PAA_t *paa)
{
  /* ipv4 address = 4 + ipv6 address = 16 + ipv6 prefix length = 1
   * + pdn_type = 1
   * = maximum of 22 bytes
   */
  uint8_t temp[22];
  uint8_t pdn_type;
  uint8_t offset = 0;
  NwRcT   rc;

  DevAssert(paa != NULL);

  pdn_type = paa->pdn_type + 1;

  temp[offset] = pdn_type;
  offset++;

  if (pdn_type & 0x2) {
    /* If ipv6 or ipv4v6 present */
    temp[1] = paa->ipv6_prefix_length;
    memcpy(&temp[2], paa->ipv6_address, 16);
    offset += 17;
  }

  if (pdn_type & 0x1) {
    memcpy(&temp[offset], paa->ipv4_address, 4);
    offset += 4;
  }

  rc = nwGtpv2cMsgAddIe(*msg, NW_GTPV2C_IE_PAA, offset, 0,
                        temp);
  DevAssert(NW_OK == rc);

  return 0;
}

/* The encoding of the APN shall follow the Name Syntax defined in RFC 2181,
 * RFC 1035 and RFC 1123. The APN consists of one or more labels. Each label
 * is coded as a one octet length field followed by that number of octets
 * coded as 8 bit ASCII characters. Following RFC 1035 the labels shall consist
 * only of the alphabetic characters (A-Z and a-z), digits (0-9)
 * and the hyphen (-).
 */
NwRcT s11_apn_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  uint8_t read = 1;
  uint8_t word_length;
  char *apn = (char *)arg;

  DevAssert(apn != NULL);

  DevCheck(ieLength <= APN_MAX_LENGTH, ieLength, APN_MAX_LENGTH, 0);

  word_length = ieValue[0];

  while (read < ieLength) {
    if (word_length > 0) {
      apn[read - 1] = ieValue[read];
      word_length--;
    } else {
      /* This is not an alphanumeric character */
      word_length = ieValue[read];
      /* Replace the length attribute by '.' */
      apn[read - 1] = '.';
    }

    read ++;
  }

  apn[read - 1] = '\0';

  S11_DEBUG("\t- APN %s\n", apn);

  return NW_OK;
}

int s11_apn_ie_set(NwGtpv2cMsgHandleT *msg, const char *apn)
{
  NwRcT    rc;
  uint8_t *value;
  uint8_t  apn_length;
  uint8_t  offset = 0;
  uint8_t *last_size;
  uint8_t  word_length = 0;

  DevAssert(apn != NULL);
  DevAssert(msg != NULL);

  apn_length = strlen(apn);

  value = calloc(apn_length + 1, sizeof(uint8_t));
  last_size = &value[0];

  while (apn[offset]) {
    /* We replace the . by the length of the word */
    if (apn[offset] == '.') {
      *last_size = word_length;
      word_length = 0;
      last_size = &value[offset + 1];
    } else {
      word_length++;
      value[offset + 1] = apn[offset];
    }

    offset++;
  }

  *last_size = word_length;

  rc = nwGtpv2cMsgAddIe(*msg, NW_GTPV2C_IE_APN, apn_length + 1, 0, value);
  DevAssert(NW_OK == rc);

  free(value);

  return 0;
}

NwRcT s11_ambr_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  ambr_t *ambr = (ambr_t *)arg;

  DevAssert(ambr != NULL);

  ambr->br_ul = ntoh_int32_buf(&ieValue[0]);
  ambr->br_dl = ntoh_int32_buf(&ieValue[4]);

  S11_DEBUG("\t- AMBR UL %"PRIu64"\n", ambr->br_ul);
  S11_DEBUG("\t- AMBR DL %"PRIu64"\n", ambr->br_dl);

  return NW_OK;
}

NwRcT s11_uli_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  Uli_t *uli = (Uli_t *)arg;

  DevAssert(uli != NULL);

  uli->present = ieValue[0];

  if (uli->present & ULI_CGI) {

  }

  return NW_OK;
}

int s11_bearer_qos_ie_set(NwGtpv2cMsgHandleT *msg, const BearerQOS_t *bearer_qos)
{
  NwRcT   rc;
  uint8_t value[18];

  DevAssert(msg != NULL);
  DevAssert(bearer_qos != NULL);

  value[0] = (bearer_qos->pci << 6) | (bearer_qos->pl << 2) | (bearer_qos->pvi);
  value[1] =  bearer_qos->qci;

  /* TODO: check endianness */
  memcpy(&value[2],  &bearer_qos->mbr.br_ul, 4);
  memcpy(&value[6],  &bearer_qos->mbr.br_dl, 4);
  memcpy(&value[10], &bearer_qos->gbr.br_ul, 4);
  memcpy(&value[14], &bearer_qos->gbr.br_dl, 4);

  rc = nwGtpv2cMsgAddIe(*msg, NW_GTPV2C_IE_BEARER_LEVEL_QOS, 18, 0, value);
  DevAssert(NW_OK == rc);

  return 0;
}

NwRcT s11_ip_address_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  gtp_ip_address_t *ip_address = (gtp_ip_address_t *)arg;

  DevAssert(ip_address != NULL);

  if (ieLength == 4) {
    /* This is an IPv4 Address */
    ip_address->present = GTP_IP_ADDR_v4;
    memcpy(ip_address->address.v4, ieValue, 4);
  } else if (ieLength == 16) {
    /* This is an IPv6 Address */
    ip_address->present = GTP_IP_ADDR_v6;
    memcpy(ip_address->address.v6, ieValue, 16);
  } else {
    /* Length doesn't lie in possible values */
    return NW_GTPV2C_IE_INCORRECT;
  }

  return NW_OK;
}

int s11_ip_address_ie_set(NwGtpv2cMsgHandleT     *msg,
                          const gtp_ip_address_t *ip_address)
{
  return 0;
}

NwRcT s11_delay_value_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  DelayValue_t *delay_value = (DelayValue_t *)arg;

  DevAssert(arg != NULL);

  if (ieLength != 1) {
    return NW_GTPV2C_IE_INCORRECT;
  }

  *delay_value = ieValue[0];

  S11_DEBUG("\t - Delay Value %u\n", *delay_value);

  return NW_OK;
}

int s11_delay_value_ie_set(NwGtpv2cMsgHandleT *msg,
                           const DelayValue_t *delay_value)
{
  uint8_t value;
  NwRcT   rc;

  DevAssert(msg != NULL);
  DevAssert(delay_value != NULL);

  value = *delay_value;

  rc = nwGtpv2cMsgAddIe(*msg, NW_GTPV2C_IE_DELAY_VALUE, 1, 0, (uint8_t*)&value);
  DevAssert(NW_OK == rc);

  return 0;
}

NwRcT s11_ue_time_zone_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  UETimeZone_t *ue_time_zone = (UETimeZone_t *)arg;

  DevAssert(ue_time_zone != NULL);

  if (ieLength != 2) {
    return NW_GTPV2C_IE_INCORRECT;
  }

  ue_time_zone->time_zone = ieValue[0];
  ue_time_zone->daylight_saving_time = ieValue[1] & 0x03;

  S11_DEBUG("\t - Time Zone    %u\n", ue_time_zone->time_zone);
  S11_DEBUG("\t - Daylight SVT %u\n", ue_time_zone->daylight_saving_time);

  return NW_OK;
}

int s11_ue_time_zone_ie_set(NwGtpv2cMsgHandleT *msg,
                            const UETimeZone_t *ue_time_zone)
{
  uint8_t value[2];
  NwRcT   rc;

  DevAssert(msg != NULL);
  DevAssert(ue_time_zone != NULL);

  value[0] = ue_time_zone->time_zone;
  value[1] = ue_time_zone->daylight_saving_time;

  rc = nwGtpv2cMsgAddIe(*msg, NW_GTPV2C_IE_UE_TIME_ZONE, 2, 0, value);
  DevAssert(NW_OK == rc);

  return 0;
}

NwRcT s11_target_identification_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  target_identification_t *target_identification = (target_identification_t *)arg;

  DevAssert(target_identification != NULL);

  target_identification->target_type = ieValue[0];

  switch (target_identification->target_type) {
  case TARGET_ID_RNC_ID: {
    target_identification->target_id.rnc_id.lac = (ieValue[4] << 8) |
        ieValue[5];
    target_identification->target_id.rnc_id.rac = ieValue[6];

    if (ieLength == 11) {
      /* Extended RNC id */
      target_identification->target_id.rnc_id.rnc_id = (ieValue[7] << 24) |
          (ieValue[8] << 16) | (ieValue[9] << 8) | (ieValue[10]);
    } else if (ieLength == 9) {
      /* Normal RNC id */
      target_identification->target_id.rnc_id.rnc_id = (ieValue[7] << 8) |
          ieValue[8];
    } else {
      /* This case is not possible */
      return NW_GTPV2C_IE_INCORRECT;
    }

    S11_DEBUG("\t\t- LAC 0x%04x\n", target_identification->target_id.rnc_id.lac);
    S11_DEBUG("\t\t- RAC 0x%02x\n", target_identification->target_id.rnc_id.rac);
    S11_DEBUG("\t\t- RNC 0x%08x\n", target_identification->target_id.rnc_id.rnc_id);
  }
  break;

  case TARGET_ID_MACRO_ENB_ID: {
    if (ieLength != 9) {
      return NW_GTPV2C_IE_INCORRECT;
    }

    target_identification->target_id.macro_enb_id.enb_id = ((ieValue[4] & 0x0F) << 16) |
        (ieValue[5] << 8) | ieValue[6];
    target_identification->target_id.macro_enb_id.tac = (ieValue[7] << 8) | ieValue[8];
    S11_DEBUG("\t\t- ENB Id 0x%06x\n", target_identification->target_id.macro_enb_id.enb_id);
    S11_DEBUG("\t\t- TAC    0x%04x\n", target_identification->target_id.macro_enb_id.tac);
  }
  break;

  case TARGET_ID_HOME_ENB_ID: {
    if (ieLength != 10) {
      return NW_GTPV2C_IE_INCORRECT;
    }

    target_identification->target_id.home_enb_id.enb_id = ((ieValue[4] & 0x0F) << 14) |
        (ieValue[5] << 16) | (ieValue[6] << 8) | ieValue[7];
    target_identification->target_id.home_enb_id.tac = (ieValue[8] << 8) | ieValue[9];
    S11_DEBUG("\t\t- ENB Id 0x%07x\n", target_identification->target_id.home_enb_id.enb_id);
    S11_DEBUG("\t\t- TAC    0x%04x\n", target_identification->target_id.home_enb_id.tac);
  }
  break;

  default:
    return NW_GTPV2C_IE_INCORRECT;
  }

  return NW_OK;
}

NwRcT s11_bearer_flags_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  bearer_flags_t *bearer_flags = (bearer_flags_t *)arg;

  DevAssert(arg != NULL);

  if (ieLength != 1) {
    return NW_GTPV2C_IE_INCORRECT;
  }

  bearer_flags->ppc = ieValue[0] & 0x01;
  bearer_flags->vb  = ieValue[0] & 0x02;

  return NW_OK;
}

int s11_bearer_flags_ie_set(NwGtpv2cMsgHandleT   *msg,
                            const bearer_flags_t *bearer_flags)
{
  NwRcT   rc;
  uint8_t value;

  DevAssert(msg != NULL);
  DevAssert(bearer_flags != NULL);

  value = (bearer_flags->vb << 1) | bearer_flags->ppc;

  rc = nwGtpv2cMsgAddIe(*msg, NW_GTPV2C_IE_BEARER_FLAGS, 1, 0, (uint8_t*)&value);
  DevAssert(NW_OK == rc);

  return 0;
}

NwRcT s11_indication_flags_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  indication_flags_t *indication_flags = (indication_flags_t*)arg;

  DevAssert(indication_flags != NULL);

  if (ieLength < 3) {
    return NW_GTPV2C_IE_INCORRECT;
  }

  *indication_flags = ieValue[0] | (ieValue[1] << 8) | (ieValue[2] << 16);

  S11_DEBUG("\t- Indication Flags %06x\n", *indication_flags);

  return NW_OK;
}

NwRcT s11_fqcsid_ie_get(
  uint8_t ieType, uint8_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg)
{
  FQ_CSID_t *fq_csid = (FQ_CSID_t *)arg;

  DevAssert(fq_csid != NULL);

  fq_csid->node_id_type = (ieValue[0] & 0xF0) >> 4;
  S11_DEBUG("\t- FQ-CSID type %u\n", fq_csid->node_id_type);

  /* NOTE: Values of Number of CSID other than 1 are only employed in the
   * Delete PDN Connection Set Request and Response.
   */
  if ((ieValue[0] & 0x0F) != 1) {
    return NW_GTPV2C_IE_INCORRECT;
  }

  switch (fq_csid->node_id_type) {
  case GLOBAL_UNICAST_IPv4: {
    if (ieLength != 7) {
      return NW_GTPV2C_IE_INCORRECT;
    }

    fq_csid->node_id.unicast_ipv4 = (ieValue[1] << 24) |
                                    (ieValue[2] << 16) | (ieValue[3] << 8) | (ieValue[4]);
    fq_csid->csid = (ieValue[5] << 8) | ieValue[6];

    S11_DEBUG("\t- v4 address ["IPV4_ADDR"]\n",
              IPV4_ADDR_FORMAT(fq_csid->node_id.unicast_ipv4));
  }
  break;

  case GLOBAL_UNICAST_IPv6: {
    char ipv6[40];

    if (ieLength != 19) {
      return NW_GTPV2C_IE_INCORRECT;
    }

    memcpy(fq_csid->node_id.unicast_ipv6, &ieValue[1], 16);
    fq_csid->csid = (ieValue[17] << 8) | ieValue[18];

    /* Convert the ipv6 to printable string */
    inet_ntop(AF_INET6, fq_csid->node_id.unicast_ipv6, ipv6, 40);

    S11_DEBUG("\t- v6 address [%s]\n", fq_csid->node_id.unicast_ipv6);
  }
  break;

  default:
    return NW_GTPV2C_IE_INCORRECT;
  }

  S11_DEBUG("\t- CSID 0x%04x\n", fq_csid->csid);
  return NW_OK;
}
