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

