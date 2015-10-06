/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
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

