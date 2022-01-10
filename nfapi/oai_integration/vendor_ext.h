/*
 * Copyright 2017 Cisco Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _VENDOR_EXT_H_
#define _VENDOR_EXT_H_

#include "nfapi_interface.h"

typedef enum {
  P5_VENDOR_EXT_REQ = NFAPI_VENDOR_EXT_MSG_MIN,
  P5_VENDOR_EXT_RSP,

  P7_VENDOR_EXT_REQ,
  P7_VENDOR_EXT_IND

} vendor_ext_message_id_e;

typedef struct {
  nfapi_p4_p5_message_header_t header;
  uint16_t dummy1;
  uint16_t dummy2;
} vendor_ext_p5_req;

typedef struct {
  nfapi_p4_p5_message_header_t header;
  uint16_t error_code;
} vendor_ext_p5_rsp;

typedef struct {
  nfapi_p7_message_header_t header;
  uint16_t dummy1;
  uint16_t dummy2;
} vendor_ext_p7_req;

typedef struct {
  nfapi_p7_message_header_t header;
  uint16_t error_code;
} vendor_ext_p7_ind;

typedef struct {
  nfapi_tl_t tl;
  uint32_t dummy;
} vendor_ext_tlv_1;

#define VENDOR_EXT_TLV_1_TAG 0xF001

typedef struct {
  nfapi_tl_t tl;
  uint32_t dummy;
} vendor_ext_tlv_2;

#define VENDOR_EXT_TLV_2_TAG 0xF002

typedef enum {
  NFAPI_MONOLITHIC=0,
  NFAPI_MODE_PNF,
  NFAPI_MODE_VNF,
  NFAPI_UE_STUB_PNF,
  NFAPI_UE_STUB_OFFNET,
  NFAPI_MODE_STANDALONE_PNF,
  NFAPI_MODE_UNKNOWN
} nfapi_mode_t;

const char *nfapi_get_strmode(void);
void nfapi_logmode(void);
nfapi_mode_t nfapi_getmode(void);
void nfapi_setmode(nfapi_mode_t nfapi_mode);
#define NFAPI_MODE (nfapi_getmode())
#endif // _VENDOR_EXT_
