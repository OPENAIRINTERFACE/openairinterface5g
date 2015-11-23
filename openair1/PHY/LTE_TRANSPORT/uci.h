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

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
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
