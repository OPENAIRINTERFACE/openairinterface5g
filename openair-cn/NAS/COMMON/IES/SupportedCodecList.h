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
#include <stdint.h>

#include "OctetString.h"

#ifndef SUPPORTED_CODEC_LIST_H_
#define SUPPORTED_CODEC_LIST_H_

#define SUPPORTED_CODEC_LIST_MINIMUM_LENGTH 5
#define SUPPORTED_CODEC_LIST_MAXIMUM_LENGTH 5

typedef struct SupportedCodecList_tag {
  uint8_t  systemidentification;
  uint8_t  lengthofbitmap;
  uint16_t codecbitmap;
} SupportedCodecList;

int encode_supported_codec_list(SupportedCodecList *supportedcodeclist, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_supported_codec_list(SupportedCodecList *supportedcodeclist, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_supported_codec_list_xml(SupportedCodecList *supportedcodeclist, uint8_t iei);

#endif /* SUPPORTED CODEC LIST_H_ */

