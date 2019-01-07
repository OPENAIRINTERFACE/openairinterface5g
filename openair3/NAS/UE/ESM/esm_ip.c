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

#include "emmData.h"
#include "esmData.h"

char *esm_data_get_ipv4_addr(const OctetString *ip_addr, char *ret)
{
  if (ip_addr->length > 0) {
    sprintf(ret, "%u.%u.%u.%u",
            ip_addr->value[0], ip_addr->value[1],
            ip_addr->value[2], ip_addr->value[3]);
    return ret;
  }

  return (NULL);
}

char *esm_data_get_ipv6_addr(const OctetString *ip_addr, char *ret)
{
  if (ip_addr->length > 0) {
    sprintf(ret, "%x%.2x:%x%.2x:%x%.2x:%x%.2x",
            ip_addr->value[0], ip_addr->value[1],
            ip_addr->value[2], ip_addr->value[3],
            ip_addr->value[4], ip_addr->value[5],
            ip_addr->value[6], ip_addr->value[7]);
    return ret;
  }

  return (NULL);
}

char *esm_data_get_ipv4v6_addr(const OctetString *ip_addr, char *ret)
{
  if (ip_addr->length > 0) {
    sprintf(ret, "%u.%u.%u.%u / %x%.2x:%x%.2x:%x%.2x:%x%.2x",
            ip_addr->value[0], ip_addr->value[1],
            ip_addr->value[2], ip_addr->value[3],
            ip_addr->value[4], ip_addr->value[5],
            ip_addr->value[6], ip_addr->value[7],
            ip_addr->value[8], ip_addr->value[9],
            ip_addr->value[10], ip_addr->value[11]);
    return ret;
  }

  return (NULL);
}
