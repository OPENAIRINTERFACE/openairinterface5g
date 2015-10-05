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
