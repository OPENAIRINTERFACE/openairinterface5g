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
#include <stdint.h>
#include <assert.h>

#ifndef OCTET_STRING_H_
#define OCTET_STRING_H_

typedef struct OctetString_tag {
  uint32_t  length;
  uint8_t  *value;
} OctetString;
#define FREE_OCTET_STRING(oCTETsTRING)                     \
    do {                                                   \
        if ((oCTETsTRING).value != NULL) {                 \
            free((oCTETsTRING).value);                     \
            (oCTETsTRING).value = NULL;                    \
        }                                                  \
        (oCTETsTRING).length = 0;                          \
    } while (0);


#define DUP_OCTET_STRING(oCTETsTRINGoRIG,oCTETsTRINGcOPY)                   \
    do {                                                                    \
        if ((oCTETsTRINGoRIG).value == NULL) {                              \
            (oCTETsTRINGcOPY).length = 0;                                   \
            (oCTETsTRINGcOPY).value = NULL;                                 \
            break;                                                          \
        }                                                                   \
        (oCTETsTRINGcOPY).length = (oCTETsTRINGoRIG).length;                 \
        (oCTETsTRINGcOPY).value  = malloc((oCTETsTRINGoRIG).length+1);      \
        (oCTETsTRINGcOPY).value[(oCTETsTRINGoRIG).length] = '\0';           \
        memcpy((oCTETsTRINGcOPY).value,                                     \
            (oCTETsTRINGoRIG).value,                                        \
            (oCTETsTRINGoRIG).length);                                      \
    } while (0);

OctetString* dup_octet_string(OctetString*octetstring);

void free_octet_string(OctetString *octetstring);

int encode_octet_string(OctetString *octetstring, uint8_t *buffer, uint32_t len);

int decode_octet_string(OctetString *octetstring, uint16_t pdulen, uint8_t *buffer, uint32_t buflen);

char* dump_octet_string_xml(const OctetString * const octetstring);

char* dump_octet_string(const OctetString * const octetstring);

#endif /* OCTET_STRING_H_ */

