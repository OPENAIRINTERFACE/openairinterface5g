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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "OctetString.h"

#define DUMP_OUTPUT_SIZE 1024
static  char _dump_output[DUMP_OUTPUT_SIZE];

OctetString* dup_octet_string(OctetString *octetstring)
{
  OctetString *os_p = NULL;

  if (octetstring) {
    os_p = calloc(1,sizeof(OctetString));
    os_p->length = octetstring->length;
    os_p->value = malloc(octetstring->length+1);
    memcpy(os_p->value, octetstring->value, octetstring->length);
    os_p->value[octetstring->length] = '\0';
  }
  return os_p;
}


void free_octet_string(OctetString *octetstring)
{
  if (octetstring) {
    if (octetstring->value) free(octetstring->value);
    octetstring->value  = NULL;
    octetstring->length = 0;
    free(octetstring);
  }
}


int encode_octet_string(OctetString *octetstring, uint8_t *buffer, uint32_t buflen)
{
  if (octetstring != NULL) {
	if ((octetstring->value != NULL) && (octetstring->length > 0)) {
      CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, octetstring->length, buflen);
      memcpy((void*)buffer, (void*)octetstring->value, octetstring->length);
      return octetstring->length;
	} else {
	  return 0;
	}
  } else {
    return 0;
  }
}

int decode_octet_string(OctetString *octetstring, uint16_t pdulen, uint8_t *buffer, uint32_t buflen)
{
  if (buflen < pdulen)
    return -1;

  if ((octetstring != NULL) && (buffer!= NULL)) {
    octetstring->length = pdulen;
    octetstring->value = malloc(sizeof(uint8_t) * (pdulen+1));
    memcpy((void*)octetstring->value, (void*)buffer, pdulen);
    octetstring->value[pdulen] = '\0';
    return octetstring->length;
  } else {
    return -1;
  }
}

char* dump_octet_string_xml( const OctetString * const octetstring)
{
  int i;
  int remaining_size = DUMP_OUTPUT_SIZE;
  int size           = 0;
  int size_print     = 0;

  size_print = snprintf(_dump_output, remaining_size, "<Length>%u</Length>\n\t<values>", octetstring->length);
  size += size_print;
  remaining_size -= size_print;

  for (i = 0; i < octetstring->length; i++) {
	size_print = snprintf(&_dump_output[size], remaining_size, "0x%x ", octetstring->value[i]);
	size += size_print;
    remaining_size -= size_print;
  }

  size_print = snprintf(&_dump_output[size], remaining_size, "</values>\n");
  return _dump_output;
}

char* dump_octet_string( const OctetString * const octetstring)
{
  int i;
  int remaining_size = DUMP_OUTPUT_SIZE;
  int size           = 0;
  int size_print     = 0;

  for (i = 0; i < octetstring->length; i++) {
	size_print = snprintf(&_dump_output[size], remaining_size, "0x%x ", octetstring->value[i]);
	size += size_print;
    remaining_size -= size_print;
  }

  return _dump_output;
}
