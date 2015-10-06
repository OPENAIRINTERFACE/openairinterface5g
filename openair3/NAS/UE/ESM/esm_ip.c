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

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
#include "emmData.h"
#include "esmData.h"

char ip_addr_str[100];

inline char *esm_data_get_ipv4_addr(const OctetString *ip_addr)
{
  if (ip_addr->length > 0) {
    sprintf(ip_addr_str, "%u.%u.%u.%u",
            ip_addr->value[0], ip_addr->value[1],
            ip_addr->value[2], ip_addr->value[3]);
    return ip_addr_str;
  }

  return (NULL);
}

inline char *esm_data_get_ipv6_addr(const OctetString *ip_addr)
{
  if (ip_addr->length > 0) {
    sprintf(ip_addr_str, "%x%.2x:%x%.2x:%x%.2x:%x%.2x",
            ip_addr->value[0], ip_addr->value[1],
            ip_addr->value[2], ip_addr->value[3],
            ip_addr->value[4], ip_addr->value[5],
            ip_addr->value[6], ip_addr->value[7]);
    return ip_addr_str;
  }

  return (NULL);
}

inline char *esm_data_get_ipv4v6_addr(const OctetString *ip_addr)
{
  if (ip_addr->length > 0) {
    sprintf(ip_addr_str, "%u.%u.%u.%u / %x%.2x:%x%.2x:%x%.2x:%x%.2x",
            ip_addr->value[0], ip_addr->value[1],
            ip_addr->value[2], ip_addr->value[3],
            ip_addr->value[4], ip_addr->value[5],
            ip_addr->value[6], ip_addr->value[7],
            ip_addr->value[8], ip_addr->value[9],
            ip_addr->value[10], ip_addr->value[11]);
    return ip_addr_str;
  }

  return (NULL);
}
