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

#include "PHY/types.h"



typedef enum {
  ue_selected,
  wideband_cqi_rank1_2A, //wideband_cqi_rank1_2A,
  wideband_cqi_rank2_2A, //wideband_cqi_rank2_2A,
  HLC_subband_cqi_nopmi, //HLC_subband_cqi_nopmi,
  HLC_subband_cqi_rank1_2A, //HLC_subband_cqi_rank1_2A,
  HLC_subband_cqi_rank2_2A, //HLC_subband_cqi_rank2_2A,
  HLC_subband_cqi_modes123, //HLC_subband_cqi_modes123
  HLC_subband_cqi_mcs_CBA, // MCS and RNTI, for contention-based acces
  unknown_cqi//
} UCI_format_t;

// **********************************************1.5 MHz***************************************************************************
typedef struct __attribute__((packed))
{
  uint32_t padding:16;
  uint32_t pmi:12;
  uint32_t cqi1:4;
}
wideband_cqi_rank1_2A_1_5MHz ;
#define sizeof_wideband_cqi_rank1_2A_1_5MHz 16

typedef struct __attribute__((packed))
{
  uint16_t padding:2;
  uint16_t pmi:6;
  uint16_t cqi2:4;
  uint16_t cqi1:4;
}
wideband_cqi_rank2_2A_1_5MHz ;
#define sizeof_wideband_cqi_rank2_2A_1_5MHz 14

typedef struct __attribute__((packed))
{
  uint32_t padding:16;
  uint32_t diffcqi1:12;
  uint32_t cqi1:4;
}
HLC_subband_cqi_nopmi_1_5MHz;
#define sizeof_HLC_subband_cqi_nopmi_1_5MHz 16

typedef struct __attribute__((packed))
{
  uint32_t padding:14;
  uint32_t pmi:2;
  uint32_t diffcqi1:12;
  uint32_t cqi1:4;
}
HLC_subband_cqi_rank1_2A_1_5MHz;
#define sizeof_HLC_subband_cqi_rank1_2A_1_5MHz 18

typedef struct __attribute__((packed))
{
  uint64_t padding:31;
  uint64_t pmi:1;
  uint64_t diffcqi2:12;
  uint64_t cqi2:4;
  uint64_t diffcqi1:12;
  uint64_t cqi1:4;
}
HLC_subband_cqi_rank2_2A_1_5MHz;
#define sizeof_HLC_subband_cqi_rank2_2A_1_5MHz 33

typedef struct __attribute__((packed))
{
  uint32_t padding:16;
  uint32_t diffcqi1:12;
  uint32_t cqi1:4;
}
HLC_subband_cqi_modes123_1_5MHz;
#define sizeof_HLC_subband_cqi_modes123_1_5MHz 16

typedef struct __attribute__((packed))
{
  uint32_t padding:11;
  uint32_t crnti:16;
  uint32_t mcs:5;
}
HLC_subband_cqi_mcs_CBA_1_5MHz;
#define sizeof_HLC_subband_cqi_mcs_CBA_1_5MHz 21


// **********************************************5 MHz***************************************************************************
typedef struct __attribute__((packed))
{
  uint32_t padding:14;
  uint32_t pmi:14;
  uint32_t cqi1:4;
}
wideband_cqi_rank1_2A_5MHz ;
#define sizeof_wideband_cqi_rank1_2A_5MHz 18

typedef struct __attribute__((packed))
{
  uint16_t padding:1;
  uint16_t pmi:7;
  uint16_t cqi2:4;
  uint16_t cqi1:4;
}
wideband_cqi_rank2_2A_5MHz ;
#define sizeof_wideband_cqi_rank2_2A_5MHz 15

typedef struct __attribute__((packed))
{
  uint32_t padding:14;
  uint32_t diffcqi1:14;
  uint32_t cqi1:4;
}
HLC_subband_cqi_nopmi_5MHz;
#define sizeof_HLC_subband_cqi_nopmi_5MHz 18

typedef struct __attribute__((packed))
{
  uint32_t padding:12;
  uint32_t pmi:2;
  uint32_t diffcqi1:14;
  uint32_t cqi1:4;
}
HLC_subband_cqi_rank1_2A_5MHz;
#define sizeof_HLC_subband_cqi_rank1_2A_5MHz 20

typedef struct __attribute__((packed))
{
  uint64_t padding:27;
  uint64_t pmi:1;
  uint64_t diffcqi2:14;
  uint64_t cqi2:4;
  uint64_t diffcqi1:14;
  uint64_t cqi1:4;
}
HLC_subband_cqi_rank2_2A_5MHz;
#define sizeof_HLC_subband_cqi_rank2_2A_5MHz 37

typedef struct __attribute__((packed))
{
  uint32_t padding:14;
  uint32_t diffcqi1:14;
  uint32_t cqi1:4;
}
HLC_subband_cqi_modes123_5MHz;
#define sizeof_HLC_subband_cqi_modes123_5MHz 18

typedef struct __attribute__((packed))
{
  uint32_t padding:11;
  uint32_t crnti:16;
  uint32_t mcs:5;
}
HLC_subband_cqi_mcs_CBA_5MHz;
#define sizeof_HLC_subband_cqi_mcs_CBA_5MHz 21

// **********************************************10 MHz***************************************************************************
typedef struct __attribute__((packed))
{
  uint32_t padding:10;
  uint32_t pmi:18;
  uint32_t cqi1:4;
}
wideband_cqi_rank1_2A_10MHz ;
#define sizeof_wideband_cqi_rank1_2A_10MHz 22

typedef struct __attribute__((packed))
{
  uint32_t padding:15;
  uint32_t pmi:9;
  uint32_t cqi2:4;
  uint32_t cqi1:4;
}
wideband_cqi_rank2_2A_10MHz ;
#define sizeof_wideband_cqi_rank2_2A_10MHz 17

typedef struct __attribute__((packed))
{
  uint32_t padding:10;
  uint32_t diffcqi1:18;
  uint32_t cqi1:4;
}
HLC_subband_cqi_nopmi_10MHz;
#define sizeof_HLC_subband_cqi_nopmi_10MHz 22

typedef struct __attribute__((packed))
{
  uint32_t padding:8;
  uint32_t pmi:2;
  uint32_t diffcqi1:18;
  uint32_t cqi1:4;
}
HLC_subband_cqi_rank1_2A_10MHz;
#define sizeof_HLC_subband_cqi_rank1_2A_10MHz 24

typedef struct __attribute__((packed))
{
  uint64_t padding:19;
  uint64_t pmi:1;
  uint64_t diffcqi2:18;
  uint64_t cqi2:4;
  uint64_t diffcqi1:18;
  uint64_t cqi1:4;
}
HLC_subband_cqi_rank2_2A_10MHz;
#define sizeof_HLC_subband_cqi_rank2_2A_10MHz 45

typedef struct __attribute__((packed))
{
  uint32_t padding:10;
  uint32_t diffcqi1:18;
  uint32_t cqi1:4;
}
HLC_subband_cqi_modes123_10MHz;
#define sizeof_HLC_subband_cqi_modes123_10MHz 22

typedef struct __attribute__((packed))
{
  uint32_t padding:11;
  uint32_t crnti:16;
  uint32_t mcs:5;
}
HLC_subband_cqi_mcs_CBA_10MHz;
#define sizeof_HLC_subband_cqi_mcs_CBA_10MHz 21

// **********************************************20 MHz***************************************************************************
typedef struct __attribute__((packed))
{
  uint32_t padding:2;
  uint32_t pmi:26;
  uint32_t cqi1:4;
}
wideband_cqi_rank1_2A_20MHz ;
#define sizeof_wideband_cqi_rank1_2A_20MHz 20

typedef struct __attribute__((packed))
{
  uint32_t padding:11;
  uint32_t pmi:13;
  uint32_t cqi2:4;
  uint32_t cqi1:4;
}
wideband_cqi_rank2_2A_20MHz ;
#define sizeof_wideband_cqi_rank2_2A_20MHz 21

typedef struct __attribute__((packed))
{
  uint32_t padding:2;
  uint32_t diffcqi1:26;
  uint32_t cqi1:4;
}
HLC_subband_cqi_nopmi_20MHz;
#define sizeof_HLC_subband_cqi_nopmi_20MHz 30

typedef struct __attribute__((packed))
{
  //  uint32_t padding:12;
  uint32_t pmi:2;
  uint32_t diffcqi1:26;
  uint32_t cqi1:4;
}
HLC_subband_cqi_rank1_2A_20MHz;
#define sizeof_HLC_subband_cqi_rank1_2A_20MHz 32

typedef struct __attribute__((packed))
{
  uint64_t padding:3;
  uint64_t pmi:1;
  uint64_t diffcqi2:26;
  uint64_t cqi2:4;
  uint64_t diffcqi1:26;
  uint64_t cqi1:4;
}
HLC_subband_cqi_rank2_2A_20MHz;
#define sizeof_HLC_subband_cqi_rank2_2A_20MHz 61

typedef struct __attribute__((packed))
{
  uint32_t padding:2;
  uint32_t diffcqi1:26;
  uint32_t cqi1:4;
}
HLC_subband_cqi_modes123_20MHz;
#define sizeof_HLC_subband_cqi_modes123_20MHz 30

typedef struct __attribute__((packed))
{
  uint32_t padding:11;
  uint32_t crnti:16;
  uint32_t mcs:5;
}
HLC_subband_cqi_mcs_CBA_20MHz;
#define sizeof_HLC_subband_cqi_mcs_CBA_20MHz 21


#define MAX_CQI_PAYLOAD (sizeof(HLC_subband_cqi_rank2_2A_20MHz)*8*20)
#define MAX_CQI_BITS (sizeof(HLC_subband_cqi_rank2_2A_20MHz)*8)
#define MAX_CQI_BYTES (sizeof(HLC_subband_cqi_rank2_2A_20MHz))
#define MAX_ACK_PAYLOAD 18
#define MAX_RI_PAYLOAD 6
