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

/*! \file PHY/LTE_TRANSPORT/dci.h
* \brief typedefs for LTE DCI structures from 36-212, V8.6 2009-03.  Limited to 5 MHz formats for the moment.Current LTE compliance V8.6 2009-03.
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/

#include <stdint.h>

///  DCI Format Type 0 (5 MHz,TDD0, 27 bits)
struct DCI0_5MHz_TDD0 {
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
  /// Hopping flag
  uint32_t hopping:1;
  /// RB Assignment (ceil(log2(N_RB_UL*(N_RB_UL+1)/2)) bits)
  uint32_t rballoc:9;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Power Control
  uint32_t TPC:2;
  /// Cyclic shift
  uint32_t cshift:3;
  /// DAI (TDD)
  uint32_t ulindex:2;
  /// CQI Request
  uint32_t cqi_req:1;
  /// Padding to get to size of DCI1A
  uint32_t padding:2;
} __attribute__ ((__packed__));

typedef struct DCI0_5MHz_TDD0 DCI0_5MHz_TDD0_t;
#define sizeof_DCI0_5MHz_TDD_0_t 27

///  DCI Format Type 0 (1.5 MHz,TDD1-6, 23 bits)
struct DCI0_1_5MHz_TDD_1_6 {
  /// Padding
  uint32_t padding:11;
  /// CQI Request
  uint32_t cqi_req:1;
  /// DAI
  uint32_t dai:2;
  /// Cyclic shift
  uint32_t cshift:3;
  /// Power Control
  uint32_t TPC:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_UL*(N_RB_UL+1)/2)) bits)
  uint32_t rballoc:5;
  /// Hopping flag
  uint32_t hopping:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI0_1_5MHz_TDD_1_6 DCI0_1_5MHz_TDD_1_6_t;
#define sizeof_DCI0_1_5MHz_TDD_1_6_t 24

/// DCI Format Type 1A (1.5 MHz, TDD, frame 1-6, 24 bits)
struct DCI1A_1_5MHz_TDD_1_6 {
  /// padding
  uint32_t padding:9;
  /// Downlink Assignment Index
  uint32_t dai:2;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:4;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL*(N_RB_DL-1)/2)) bits)
  uint32_t rballoc:5;
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI1A_1_5MHz_TDD_1_6 DCI1A_1_5MHz_TDD_1_6_t;
#define sizeof_DCI1A_1_5MHz_TDD_1_6_t 24


///  DCI Format Type 0 (5 MHz,TDD1-6, 27 bits)
struct DCI0_5MHz_TDD_1_6 {
  /// Padding
  uint32_t padding:7;
  /// CQI Request
  uint32_t cqi_req:1;
  /// DAI
  uint32_t dai:2;
  /// Cyclic shift
  uint32_t cshift:3;
  /// Power Control
  uint32_t TPC:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_UL*(N_RB_UL+1)/2)) bits)
  uint32_t rballoc:9;
  /// Hopping flag
  uint32_t hopping:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI0_5MHz_TDD_1_6 DCI0_5MHz_TDD_1_6_t;
#define sizeof_DCI0_5MHz_TDD_1_6_t 27

/// DCI Format Type 1A (5 MHz, TDD, frame 1-6, 27 bits)
struct DCI1A_5MHz_TDD_1_6 {
  /// padding
  uint32_t padding:5;
  /// Downlink Assignment Index
  uint32_t dai:2;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:4;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL*(N_RB_DL-1)/2)) bits)
  uint32_t rballoc:9;
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI1A_5MHz_TDD_1_6 DCI1A_5MHz_TDD_1_6_t;
#define sizeof_DCI1A_5MHz_TDD_1_6_t 27


///  DCI Format Type 0 (10 MHz,TDD1-6, 29 bits)
struct DCI0_10MHz_TDD_1_6 {
  /// Padding
  uint32_t padding:5;
  /// CQI Request
  uint32_t cqi_req:1;
  /// DAI
  uint32_t dai:2;
  /// Cyclic shift
  uint32_t cshift:3;
  /// Power Control
  uint32_t TPC:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_UL*(N_RB_UL+1)/2)) bits)
  uint32_t rballoc:11;
  /// Hopping flag
  uint32_t hopping:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI0_10MHz_TDD_1_6 DCI0_10MHz_TDD_1_6_t;
#define sizeof_DCI0_10MHz_TDD_1_6_t 29

/// DCI Format Type 1A (10 MHz, TDD, frame 1-6, 30 bits)
struct DCI1A_10MHz_TDD_1_6 {
  /// padding
  uint32_t padding:3;
  /// Downlink Assignment Index
  uint32_t dai:2;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:4;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL*(N_RB_DL-1)/2)) bits)
  uint32_t rballoc:11;
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI1A_10MHz_TDD_1_6 DCI1A_10MHz_TDD_1_6_t;
#define sizeof_DCI1A_10MHz_TDD_1_6_t 29


///  DCI Format Type 0 (20 MHz,TDD1-6, 27 bits)
struct DCI0_20MHz_TDD_1_6 {
  /// Padding
  uint32_t padding:3;
  /// CQI request
  uint32_t cqi_req:1;
  /// DAI
  uint32_t dai:2;
  /// Cyclic shift
  uint32_t cshift:3;
  /// Power Control
  uint32_t TPC:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_UL*(N_RB_UL+1)/2)) bits)
  uint32_t rballoc:13;
  /// Hopping flag
  uint32_t hopping:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI0_20MHz_TDD_1_6 DCI0_20MHz_TDD_1_6_t;
#define sizeof_DCI0_20MHz_TDD_1_6_t 31

/// DCI Format Type 1A (20 MHz, TDD, frame 1-6, 27 bits)
struct DCI1A_20MHz_TDD_1_6 {
  uint32_t padding:1;
  /// Downlink Assignment Index
  uint32_t dai:2;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:4;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL*(N_RB_DL-1)/2)) bits)
  uint32_t rballoc:13;
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI1A_20MHz_TDD_1_6 DCI1A_20MHz_TDD_1_6_t;
#define sizeof_DCI1A_20MHz_TDD_1_6_t 31

///  DCI Format Type 0 (1.5 MHz,FDD, 25 bits)
struct DCI0_1_5MHz_FDD {
  /// Padding
  uint32_t padding:13;
  /// CQI Request
  uint32_t cqi_req:1;
  /// DRS Cyclic Shift
  uint32_t cshift:3;
  /// Power Control
  uint32_t TPC:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_UL*(N_RB_UL+1)/2)) bits)
  uint32_t rballoc:5;
  /// Hopping flag
  uint32_t hopping:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;

} __attribute__ ((__packed__));

typedef struct DCI0_1_5MHz_FDD DCI0_1_5MHz_FDD_t;
#define sizeof_DCI0_1_5MHz_FDD_t 21

struct DCI1A_1_5MHz_FDD {
  /// padding
  uint32_t padding:12;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL(N_RB_DL+1)/2)) bits)
  uint32_t rballoc:5;
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI1A_1_5MHz_FDD DCI1A_1_5MHz_FDD_t;
#define sizeof_DCI1A_1_5MHz_FDD_t 21


///  DCI Format Type 0 (5 MHz,FDD, 23 bits)
struct DCI0_5MHz_FDD {
  /// Padding
  uint32_t padding:9;
  /// CQI Request
  uint32_t cqi_req:1;
  /// DRS Cyclic Shift
  uint32_t cshift:3;
  /// Power Control
  uint32_t TPC:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_UL*(N_RB_UL+1)/2)) bits)
  uint32_t rballoc:9;
  /// Hopping flag
  uint32_t hopping:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;

} __attribute__ ((__packed__));

typedef struct DCI0_5MHz_FDD DCI0_5MHz_FDD_t;
#define sizeof_DCI0_5MHz_FDD_t 25

struct DCI1A_5MHz_FDD {
  /// padding
  uint32_t padding:8;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL(N_RB_DL+1)/2)) bits)
  uint32_t rballoc:9;
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI1A_5MHz_FDD DCI1A_5MHz_FDD_t;
#define sizeof_DCI1A_5MHz_FDD_t 25



///  DCI Format Type 0 (10 MHz,FDD, 25 bits)
struct DCI0_10MHz_FDD {
  /// Padding
  uint32_t padding:7;
  /// CQI Request
  uint32_t cqi_req:1;
  /// DRS Cyclic Shift
  uint32_t cshift:3;
  /// Power Control
  uint32_t TPC:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_UL*(N_RB_UL+1)/2)) bits)
  uint32_t rballoc:11;
  /// Hopping flag
  uint32_t hopping:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;

} __attribute__ ((__packed__));

typedef struct DCI0_10MHz_FDD DCI0_10MHz_FDD_t;
#define sizeof_DCI0_10MHz_FDD_t 27

struct DCI1A_10MHz_FDD {
  /// padding
  uint32_t padding:6;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL(N_RB_DL+1)/2)) bits)
  uint32_t rballoc:11;
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI1A_10MHz_FDD DCI1A_10MHz_FDD_t;
#define sizeof_DCI1A_10MHz_FDD_t 27

///  DCI Format Type 0 (20 MHz,FDD, 25 bits)
struct DCI0_20MHz_FDD {
  /// Padding
  uint32_t padding:5;
  /// CQI Request
  uint32_t cqi_req:1;
  /// DRS Cyclic Shift
  uint32_t cshift:3;
  /// Power Control
  uint32_t TPC:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_UL*(N_RB_UL+1)/2)) bits)
  uint32_t rballoc:13;
  /// Hopping flag
  uint32_t hopping:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;

} __attribute__ ((__packed__));

typedef struct DCI0_20MHz_FDD DCI0_20MHz_FDD_t;
#define sizeof_DCI0_20MHz_FDD_t 28

struct DCI1A_20MHz_FDD {
  /// padding
  uint32_t padding:4;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL*(N_RB_DL+1)/2)) bits)
  uint32_t rballoc:13;
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI1A_20MHz_FDD DCI1A_20MHz_FDD_t;
#define sizeof_DCI1A_20MHz_FDD_t 28

/// DCI Format Type 1 (1.5 MHz, TDD, 23 bits)
struct DCI1_1_5MHz_TDD {
  /// padding bits to align to 32-bits
  uint32_t padding:9;
  /// DAI (TDD)
  uint32_t dai:2;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:4;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:6;
  /// Resource Allocation Header
  uint32_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI1_1_5MHz_TDD DCI1_1_5MHz_TDD_t;
#define sizeof_DCI1_1_5MHz_TDD_t 23

/// DCI Format Type 1 (5 MHz, TDD, 30 bits)
struct DCI1_5MHz_TDD {
  /// padding bits to align to 32-bits
  uint32_t padding:2;
  /// DAI (TDD)
  uint32_t dai:2;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:4;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:13;
  /// Resource Allocation Header
  uint32_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI1_5MHz_TDD DCI1_5MHz_TDD_t;
#define sizeof_DCI1_5MHz_TDD_t 30

/// DCI Format Type 1 (10 MHz, TDD, 34 bits)
struct DCI1_10MHz_TDD {
  /// padding bits to align to 64-bits
  uint64_t padding:30;
  /// DAI (TDD)
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// Redundancy version
  uint64_t rv:2;
  /// New Data Indicator
  uint64_t ndi:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Modulation and Coding Scheme and Redundancy Version
  uint64_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI1_10MHz_TDD DCI1_10MHz_TDD_t;
#define sizeof_DCI1_10MHz_TDD_t 34

/// DCI Format Type 1 (20 MHz, TDD, 42 bits)
struct DCI1_20MHz_TDD {
  /// padding bits to align to 64-bits
  uint64_t padding:22;
  /// DAI (TDD)
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// Redundancy version
  uint64_t rv:2;
  /// New Data Indicator
  uint64_t ndi:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Modulation and Coding Scheme and Redundancy Version
  uint64_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI1_20MHz_TDD DCI1_20MHz_TDD_t;
#define sizeof_DCI1_20MHz_TDD_t 42

/// DCI Format Type 1 (1.5 MHz, FDD, 21 bits)
struct DCI1_1_5MHz_FDD {
  /// padding bits to align to 32-bits
  uint32_t padding:11;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:4;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(N_RB_DL/P)) bits)
  uint32_t rballoc:6;
  /// Resource Allocation Header
  uint32_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI1_1_5MHz_FDD DCI1_1_5MHz_FDD_t;
#define sizeof_DCI1_1_5MHz_FDD_t 23

/// DCI Format Type 1 (5 MHz, FDD, 27 bits)
struct DCI1_5MHz_FDD {
  /// padding its (not transmitted)
  uint32_t padding:5;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits
  uint32_t rballoc:13;
  /// Resource Allocation Header
  uint32_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI1_5MHz_FDD DCI1_5MHz_FDD_t;
#define sizeof_DCI1_5MHz_FDD_t 27

/// DCI Format Type 1 (10 MHz, FDD, 31 bits)
struct DCI1_10MHz_FDD {
  /// padding bits (not transmitted)
  uint32_t padding:1;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits
  uint32_t rballoc:17;
  /// Resource Allocation Header
  uint32_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI1_10MHz_FDD DCI1_10MHz_FDD_t;
#define sizeof_DCI1_10MHz_FDD_t 31

/// DCI Format Type 1 (20 MHz, FDD, 39 bits)
struct DCI1_20MHz_FDD {
  /// padding bits (not transmitted)
  uint64_t padding:25;
  /// Power Control
  uint64_t TPC:2;
  /// Redundancy version
  uint64_t rv:2;
  /// New Data Indicator
  uint64_t ndi:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Modulation and Coding Scheme and Redundancy Version
  uint64_t mcs:5;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI1_20MHz_FDD DCI1_20MHz_FDD_t;
#define sizeof_DCI1_20MHz_FDD_t 39

/// DCI Format Type 1A (5 MHz, TDD, frame 1-6, 27 bits)
struct DCI1A_RA_5MHz_TDD_1_6 {
  /// padding bits to align to 32-bits
  uint32_t padding:11;
  /// PRACH mask index
  uint32_t prach_mask_index:4;
  /// Preamble Index
  uint32_t preamble_index:6;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:9;
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI1A_RA_5MHz_TDD_1_6 DCI1A_RA_5MHz_TDD_1_6_t;
#define sizeof_DCI1A_RA_5MHz_TDD_1_6_t 27

/// DCI Format Type 1A (5 MHz, TDD, frame 0, 27 bits)
/*
struct DCI1A_5MHz_TDD_0 {
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:9;
  union {
    RA_PDSCH_TDD_t ra_pdsch;
    PDSCH_TDD_t pdsch;
  } pdu;
} __attribute__ ((__packed__));

typedef struct DCI1A_5MHz_TDD_0 DCI1A_5MHz_TDD_0_t;
#define sizeof_DCI1A_5MHz_TDD_0_t 27
*/

/// DCI Format Type 1B (5 MHz, FDD, 2 Antenna Ports, 27 bits)
struct DCI1B_5MHz_2A_FDD {
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:9;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Redundancy version
  uint32_t rv:2;
  /// Power Control
  uint32_t TPC:2;
  /// TPMI information for precoding
  uint32_t tpmi:2;
  /// TMI confirmation for precoding
  uint32_t pmi:1;
  /// Padding to remove size ambiguity (26 bits -> 27 bits)
  uint32_t padding:1;
} __attribute__ ((__packed__));

typedef struct DCI1B_5MHz_2A_FDD DCI1B_5MHz_2A_FDD_t;
#define sizeof_DCI1B_5MHz_FDD_t 27

/// DCI Format Type 1B (5 MHz, TDD, 2 Antenna Ports, 29 bits)
struct DCI1B_5MHz_2A_TDD {
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:9;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// HARQ Process
  uint32_t harq_pid:4;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Redundancy version
  uint32_t rv:2;
  /// Power Control
  uint32_t TPC:2;
  /// Downlink Assignment Index
  uint32_t dai:2;
  /// TPMI information for precoding
  uint32_t tpmi:2;
  /// TMI confirmation for precoding
  uint32_t pmi:1;
} __attribute__ ((__packed__));

typedef struct DCI1B_5MHz_2A_TDD DCI1B_5MHz_2A_TDD_t;
#define sizeof_DCI1B_5MHz_2A_TDD_t 29

/// DCI Format Type 1B (5 MHz, FDD, 4 Antenna Ports, 28 bits)
struct DCI1B_5MHz_4A_FDD {
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:9;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Redundancy version
  uint32_t rv:2;
  /// Power Control
  uint32_t TPC:2;
  /// TPMI information for precoding
  uint32_t tpmi:4;
  /// TMI confirmation for precoding
  uint32_t pmi:1;
} __attribute__ ((__packed__));

typedef struct DCI1B_5MHz_4A_FDD DCI1B_5MHz_4A_FDD_t;
#define sizeof_DCI1B_5MHz_4A_FDD_t 28

/// DCI Format Type 1B (5 MHz, TDD, 4 Antenna Ports, 31 bits)
struct DCI1B_5MHz_4A_TDD {
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:9;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// HARQ Process
  uint32_t harq_pid:4;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Redundancy version
  uint32_t rv:2;
  /// Power Control
  uint32_t TPC:2;
  /// Downlink Assignment Index
  uint32_t dai:2;
  /// TPMI information for precoding
  uint32_t tpmi:4;
  /// TMI confirmation for precoding
  uint32_t pmi:1;
} __attribute__ ((__packed__));

typedef struct DCI1B_5MHz_4A_TDD DCI1B_5MHz_4A_TDD_t;
#define sizeof_DCI1B_5MHz_4A_TDD_t 31

/// DCI Format Type 1C (1.4 MHz, 8 bits)
struct DCI1C_1_5MHz
{
  /// padding to 32bits
  uint32_t padding:24;
  uint32_t mcs:5;
  uint32_t rballoc:3;  // N_RB_step = 2, Ngap=Ngap1=3, NDLVRBGap = 6, ceil(log2((3*4)/2)) = 3
} __attribute__ ((__packed__));

typedef struct DCI1C_1_5MHz DCI1C_1_5MHz_t;
#define sizeof_DCI1C_1_5MHz_t 8

/*********************************************************
**********************************************************/
/// DCI Format Type 1C (5 MHz, 12 bits)
struct DCI1C_5MHz
{
  /// padding to 32bits
  uint32_t padding:20;
  uint32_t mcs:5;
  uint32_t rballoc:7;   // N_RB_step = 2, Ngap1=Ngap2=12, NDLVRBGap = 24, ceil(log2((12*13)/2)) = 7
} __attribute__ ((__packed__));

typedef struct DCI1C_5MHz DCI1C_5MHz_t;
#define sizeof_DCI1C_5MHz_t 12 

/// DCI Format Type 1C (10 MHz, 13 bits)
struct DCI1C_10MHz
{
  /// padding to 32bits
  uint32_t padding:19;
  uint32_t mcs:5;
  uint32_t rballoc:7;  // N_RB_step = 4, Ngap1=27, NDLVRBGap = 46, ceil(log2(((11*12)/2)) = 7
  uint32_t Ngap:1;
} __attribute__ ((__packed__));

typedef struct DCI1C_10MHz DCI1C_10MHz_t;
#define sizeof_DCI1C_10MHz_t 13

/// DCI Format Type 1C (15 MHz, 14 bits)
struct DCI1C_15MHz
{
  /// padding to 32bits
  uint32_t padding:18;
  uint32_t mcs:5;
  uint32_t rballoc:8; // N_RB_step = 4, Ngap1=64, ceil(log2((16*17)/2)) = 8
  uint32_t Ngap:1;
} __attribute__ ((__packed__));

typedef struct DCI1C_15MHz DCI1C_15MHz_t;
#define sizeof_DCI1C_15MHz_t X

/// DCI Format Type 1C (20 MHz, 15 bits)
struct DCI1C_20MHz
{
  /// padding to 32bits
  uint32_t padding:17;
  uint32_t mcs:5;
  uint32_t rballoc:9; // N_RB_step = 4, Ngap1=48, ceil(log2((24*25)/2)) = 9
  uint32_t Ngap:1;
} __attribute__ ((__packed__));

typedef struct DCI1C_20MHz DCI1C_20MHz_t;
#define sizeof_DCI1C_20MHz_t 15

/*********************************************************
**********************************************************/

/// DCI Format Type 1D (5 MHz, FDD, 2 Antenna Ports, 27 bits)
struct DCI1D_5MHz_2A_FDD {
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:9;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Redundancy version
  uint32_t rv:2;
  /// Power Control
  uint32_t TPC:2;
  /// TPMI information for precoding
  uint32_t tpmi:2;
  /// TMI confirmation for precoding
  uint32_t pmi:1;
  /// Downlink Power Offset
  uint32_t dl_power_off:1;
} __attribute__ ((__packed__));

typedef struct DCI1D_5MHz_2A_FDD DCI1D_5MHz_2A_FDD_t;
#define sizeof_DCI1D_5MHz_2A_FDD_t 27

/// DCI Format Type 1D (5 MHz, TDD, 2 Antenna Ports, 30 bits)
struct DCI1D_5MHz_2A_TDD {
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:9;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// HARQ Process
  uint32_t harq_pid:4;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Redundancy version
  uint32_t rv:2;
  /// Power Control
  uint32_t TPC:2;
  /// Downlink Assignment Index
  uint32_t dai:2;
  /// TPMI information for precoding
  uint32_t tpmi:2;
  /// TMI confirmation for precoding
  uint32_t pmi:1;
  /// Downlink Power Offset
  uint32_t dl_power_off:1;
} __attribute__ ((__packed__));

typedef struct DCI1D_5MHz_2A_TDD DCI1D_5MHz_2A_TDD_t;
#define sizeof_DCI1D_5MHz_2A_TDD_t 30

/// DCI Format Type 1D (5 MHz, FDD, 4 Antenna Ports, 29 bits)
struct DCI1D_5MHz_4A_FDD {
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:9;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Redundancy version
  uint32_t rv:2;
  /// Power Control
  uint32_t TPC:2;
  /// TPMI information for precoding
  uint32_t tpmi:4;
  /// TMI confirmation for precoding
  uint32_t pmi:1;
  /// Downlink Power Offset
  uint32_t dl_power_off:1;
}  __attribute__ ((__packed__));

typedef struct DCI1D_5MHz_4A_FDD DCI1D_5MHz_4A_FDD_t;
#define sizeof_DCI1D_5MHz_4A_FDD_t 29

/// DCI Format Type 1D (5 MHz, TDD, 4 Antenna Ports, 33 bits)
struct DCI1D_5MHz_4A_TDD {
  /// Localized/Distributed VRB
  uint32_t vrb_type:1;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:9;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:5;
  /// HARQ Process
  uint32_t harq_pid:4;
  /// New Data Indicator
  uint32_t ndi:1;
  /// Redundancy version
  uint32_t rv:2;
  /// Power Control
  uint32_t TPC:2;
  /// Downlink Assignment Index
  uint32_t dai:2;
  /// TPMI information for precoding
  uint32_t tpmi:4;
  /// TMI confirmation for precoding
  uint32_t pmi:1;
  /// Downlink Power Offset
  uint32_t dl_power_off:1;
  /// Padding to remove size ambiguity (32 bits -> 33 bits)
  uint32_t padding:1;
} __attribute__ ((__packed__));

typedef struct DCI1D_5MHz_4A_TDD DCI1D_5MHz_4A_TDD_t;
#define sizeof_DCI1D_5MHz_4A_TDD_t 33


///******************NEW DCI Format for MU-MIMO****************///////////

/// DCI Format Type 1E (5 MHz, TDD, 2 Antenna Ports, more than 10 PRBs, 34 bits)
struct DCI1E_5MHz_2A_M10PRB_TDD {
  /// padding to 64bits
  uint64_t padding:30;
  /// Redundancy version 2
  ///uint64_t rv2:2;
  /// New Data Indicator 2
  ///uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  ///uint64_t mcs2:5;
  /// TPMI information for precoding
  uint64_t tpmi:3;
  /// Redundancy version 1
  uint64_t rv:2;
  /// New Data Indicator 1
  uint64_t ndi:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs:5;
  /// TB swap
  ///uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
  /// Downlink Power offset for MU-MIMO
  uint64_t dl_power_off:1;
} __attribute__ ((__packed__));
typedef struct DCI1E_5MHz_2A_M10PRB_TDD DCI1E_5MHz_2A_M10PRB_TDD_t;
#define sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t 34

// **********************************************************
// ********************FORMAT 2 DCIs ************************
// **********************************************************
/// DCI Format Type 2 (1.5 MHz, TDD, 2 Antenna Ports, 34 bits)
struct DCI2_1_5MHz_2A_TDD {
  /// padding to 64bits
  uint64_t padding:30;
  /// TPMI information for precoding
  uint64_t tpmi:3;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:6;
} __attribute__ ((__packed__));

typedef struct DCI2_1_5MHz_2A_TDD DCI2_1_5MHz_2A_TDD_t;
#define sizeof_DCI2_1_5MHz_2A_TDD_t 34

/// DCI Format Type 2 (1.5 MHz, TDD, 4 Antenna Ports, 37 bits)
struct DCI2_1_5MHz_4A_TDD {
  /// padding for 64-bit
  uint64_t padding:27;
  /// TPMI information for precoding
  uint64_t tpmi:6;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:6;
}  __attribute__ ((__packed__));

typedef struct DCI2_1_5MHz_4A_TDD DCI2_1_5MHz_4A_TDD_t;
#define sizeof_DCI2_1_5MHz_4A_TDD_t 37

/// DCI Format Type 2 (5 MHz, TDD, 2 Antenna Ports, 42 bits)
struct DCI2_5MHz_2A_TDD {
  /// padding to 64bits
  uint64_t padding:22;
  /// TPMI information for precoding
  uint64_t tpmi:3;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2_5MHz_2A_TDD DCI2_5MHz_2A_TDD_t;
#define sizeof_DCI2_5MHz_2A_TDD_t 42

/// DCI Format Type 2 (5 MHz, TDD, 4 Antenna Ports, 45 bits)
struct DCI2_5MHz_4A_TDD {
  /// padding for 64-bit
  uint64_t padding:19;
  /// TPMI information for precoding
  uint64_t tpmi:6;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2_5MHz_4A_TDD DCI2_5MHz_4A_TDD_t;
#define sizeof_DCI2_5MHz_4A_TDD_t 45

/// DCI Format Type 2 (10 MHz, TDD, 2 Antenna Ports, 46 bits)
struct DCI2_10MHz_2A_TDD {
  /// padding to 64bits
  uint64_t padding:18;
  /// TPMI information for precoding
  uint64_t tpmi:3;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2_10MHz_2A_TDD DCI2_10MHz_2A_TDD_t;
#define sizeof_DCI2_10MHz_2A_TDD_t 46

/// DCI Format Type 2 (10 MHz, TDD, 4 Antenna Ports, 49 bits)
struct DCI2_10MHz_4A_TDD {
  /// padding for 64-bit
  uint64_t padding:15;
  /// TPMI information for precoding
  uint64_t tpmi:6;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2_10MHz_4A_TDD DCI2_10MHz_4A_TDD_t;
#define sizeof_DCI2_10MHz_4A_TDD_t 49

/// DCI Format Type 2 (20 MHz, TDD, 2 Antenna Ports, 54 bits)
struct DCI2_20MHz_2A_TDD {
  /// padding to 64bits
  uint64_t padding:10;
  /// TPMI information for precoding
  uint64_t tpmi:3;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2_20MHz_2A_TDD DCI2_20MHz_2A_TDD_t;
#define sizeof_DCI2_20MHz_2A_TDD_t 54

/// DCI Format Type 2 (20 MHz, TDD, 4 Antenna Ports, 57 bits)
struct DCI2_20MHz_4A_TDD {
  /// padding for 64-bit
  uint64_t padding:7;
  /// TPMI information for precoding
  uint64_t tpmi:6;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2_20MHz_4A_TDD DCI2_20MHz_4A_TDD_t;
#define sizeof_DCI2_20MHz_4A_TDD_t 58

/// DCI Format Type 2 (1.5 MHz, FDD, 2 Antenna Ports, 31 bits)
struct DCI2_1_5MHz_2A_FDD {
  //padding for 32 bits
  uint32_t padding:1;
  /// precoding bits
  uint32_t tpmi:3;
  /// Redundancy version 2
  uint32_t rv2:2;
  /// New Data Indicator 2
  uint32_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint32_t mcs2:5;
  /// Redundancy version 1
  uint32_t rv1:2;
  /// New Data Indicator 1
  uint32_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint32_t mcs1:5;
  /// TB swap
  uint32_t tb_swap:1;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// Power Control
  uint32_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:6;
} __attribute__ ((__packed__));

typedef struct DCI2_1_5MHz_2A_FDD DCI2_1_5MHz_2A_FDD_t;
#define sizeof_DCI2_1_5MHz_2A_FDD_t 31

/// DCI Format Type 2 (1.5 MHz, FDD, 4 Antenna Ports, 34 bits)
struct DCI2_1_5MHz_4A_FDD {
  /// padding for 32 bits
  uint64_t padding:30;
  /// precoding bits
  uint64_t tpmi:6;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:6;
} __attribute__ ((__packed__));

typedef struct DCI2_1_5MHz_4A_FDD DCI2_1_5MHz_4A_FDD_t;
#define sizeof_DCI2_1_5MHz_4A_FDD_t 34

/// DCI Format Type 2 (5 MHz, FDD, 2 Antenna Ports, 39 bits)
struct DCI2_5MHz_2A_FDD {
  /// padding for 64-bit
  uint64_t padding:25;
  /// TPMI information for precoding
  uint64_t tpmi:3;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

#define sizeof_DCI2_5MHz_2A_FDD_t 39
typedef struct DCI2_5MHz_2A_FDD DCI2_5MHz_2A_FDD_t;

/// DCI Format Type 2 (5 MHz, TDD, 4 Antenna Ports, 42 bits)
struct DCI2_5MHz_4A_FDD {
  /// padding for 64-bit
  uint64_t padding:21;
  /// TPMI information for precoding
  uint64_t tpmi:6;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2_5MHz_4A_FDD_t 42
typedef struct DCI2_5MHz_4A_FDD DCI2_5MHz_4A_FDD_t;

/// DCI Format Type 2 (10 MHz, FDD, 2 Antenna Ports, 43 bits)
struct DCI2_10MHz_2A_FDD {
  /// padding for 64-bit
  uint64_t padding:21;
  /// TPMI information for precoding
  uint64_t tpmi:3;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

#define sizeof_DCI2_10MHz_2A_FDD_t 43
typedef struct DCI2_10MHz_2A_FDD DCI2_10MHz_2A_FDD_t;

/// DCI Format Type 2 (5 MHz, TDD, 4 Antenna Ports, 46 bits)
struct DCI2_10MHz_4A_FDD {
  /// padding for 64-bit
  uint64_t padding:18;
  /// TPMI information for precoding
  uint64_t tpmi:6;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2_10MHz_4A_FDD_t 46
typedef struct DCI2_10MHz_4A_FDD DCI2_10MHz_4A_FDD_t;

/// DCI Format Type 2 (20 MHz, FDD, 2 Antenna Ports, 51 bits)
struct DCI2_20MHz_2A_FDD {
  /// padding for 64-bit
  uint64_t padding:13;
  /// TPMI information for precoding
  uint64_t tpmi:3;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

#define sizeof_DCI2_20MHz_2A_FDD_t 51
typedef struct DCI2_20MHz_2A_FDD DCI2_20MHz_2A_FDD_t;

/// DCI Format Type 2 (20 MHz, FDD, 4 Antenna Ports, 54 bits)
struct DCI2_20MHz_4A_FDD {
  /// padding for 64-bit
  uint64_t padding:10;
  /// TPMI information for precoding
  uint64_t tpmi:6;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
}  __attribute__ ((__packed__));
#define sizeof_DCI2_20MHz_4A_FDD_t 54
typedef struct DCI2_20MHz_4A_FDD DCI2_20MHz_4A_FDD_t;




// *******************************************************************
// ********************FORMAT 2A DCIs*********************************
// *******************************************************************

/// DCI Format Type 2A (1.5 MHz, TDD, 2 Antenna Ports, 32 bits)
struct DCI2A_1_5MHz_2A_TDD {
  /// Redundancy version 2
  uint32_t rv2:2;
  /// New Data Indicator 2
  uint32_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint32_t mcs2:5;
  /// Redundancy version 1
  uint32_t rv1:2;
  /// New Data Indicator 1
  uint32_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint32_t mcs1:5;
  /// TB swap
  uint32_t tb_swap:1;
  /// HARQ Process
  uint32_t harq_pid:4;
  /// Downlink Assignment Index
  uint32_t dai:2;
  /// Power Control
  uint32_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:6;
  /// Resource Allocation Header
  uint32_t rah:1;
} __attribute__ ((__packed__));

#define sizeof_DCI2A_1_5MHz_2A_TDD_t 32
typedef struct DCI2A_1_5MHz_2A_TDD DCI2A_1_5MHz_2A_TDD_t;

/// DCI Format Type 2A (1.5 MHz, TDD, 4 Antenna Ports, 34 bits)
struct DCI2A_1_5MHz_4A_TDD {
  uint64_t padding:30;
  /// TPMI information for precoding
  uint64_t tpmi:2;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:6;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_1_5MHz_4A_TDD_t 34
typedef struct DCI2A_1_5MHz_4A_TDD DCI2A_1_5MHz_4A_TDD_t;

/// DCI Format Type 2A (1.5 MHz, FDD, 2 Antenna Ports, 29 bits)
struct DCI2A_1_5MHz_2A_FDD {
  uint32_t padding:18;
  /// Redundancy version 2
  uint32_t rv2:2;
  /// New Data Indicator 2
  uint32_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint32_t mcs2:5;
  /// Redundancy version 1
  uint32_t rv1:2;
  /// New Data Indicator 1
  uint32_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint32_t mcs1:5;
  /// TB swap
  uint32_t tb_swap:1;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// Power Control
  uint32_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:6;
  /// Resource Allocation Header
  uint32_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_1_5MHz_2A_FDD_t 29
typedef struct DCI2A_1_5MHz_2A_FDD DCI2A_1_5MHz_2A_FDD_t;

/// DCI Format Type 2A (1.5 MHz, FDD, 4 Antenna Ports, 31 bits)
struct DCI2A_1_5MHz_4A_FDD {
  uint32_t padding:1;
  /// TPMI information for precoding
  uint32_t tpmi:2;
  /// Redundancy version 2
  uint32_t rv2:2;
  /// New Data Indicator 2
  uint32_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint32_t mcs2:5;
  /// Redundancy version 1
  uint32_t rv1:2;
  /// New Data Indicator 1
  uint32_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint32_t mcs1:5;
  /// TB swap
  uint32_t tb_swap:1;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// Power Control
  uint32_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:6;
  /// Resource Allocation Header
  uint32_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_1_5MHz_4A_FDD_t 38
typedef struct DCI2A_1_5MHz_4A_FDD DCI2A_1_5MHz_4A_FDD_t;

/// DCI Format Type 2A (5 MHz, TDD, 2 Antenna Ports, 39 bits)
struct DCI2A_5MHz_2A_TDD {
  uint64_t padding:25;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_5MHz_2A_TDD_t 39
typedef struct DCI2A_5MHz_2A_TDD DCI2A_5MHz_2A_TDD_t;

/// DCI Format Type 2A (5 MHz, TDD, 4 Antenna Ports, 41 bits)
struct DCI2A_5MHz_4A_TDD {
  uint64_t padding:23;
  /// TPMI information for precoding
  uint64_t tpmi:2;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_5MHz_4A_TDD_t 41
typedef struct DCI2A_5MHz_4A_TDD DCI2A_5MHz_4A_TDD_t;

/// DCI Format Type 2A (5 MHz, FDD, 2 Antenna Ports, 36 bits)
struct DCI2A_5MHz_2A_FDD {
  uint64_t padding:28;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_5MHz_2A_FDD_t 36
typedef struct DCI2A_5MHz_2A_FDD DCI2A_5MHz_2A_FDD_t;

/// DCI Format Type 2A (5 MHz, FDD, 4 Antenna Ports, 38 bits)
struct DCI2A_5MHz_4A_FDD {
  uint64_t padding:26;
  /// TPMI information for precoding
  uint64_t tpmi:2;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_5MHz_4A_FDD_t 38
typedef struct DCI2A_5MHz_4A_FDD DCI2A_5MHz_4A_FDD_t;

/// DCI Format Type 2A (10 MHz, TDD, 2 Antenna Ports, 39 bits)
struct DCI2A_10MHz_2A_TDD {
  uint64_t padding:25;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_10MHz_2A_TDD_t 39
typedef struct DCI2A_10MHz_2A_TDD DCI2A_10MHz_2A_TDD_t;

/// DCI Format Type 2A (10 MHz, TDD, 4 Antenna Ports, 41 bits)
struct DCI2A_10MHz_4A_TDD {
  uint64_t padding:23;
  /// TPMI information for precoding
  uint64_t tpmi:2;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_10MHz_4A_TDD_t 41
typedef struct DCI2A_10MHz_4A_TDD DCI2A_10MHz_4A_TDD_t;

/// DCI Format Type 2A (10 MHz, FDD, 2 Antenna Ports, 40 bits)
struct DCI2A_10MHz_2A_FDD {
  uint64_t padding:24;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_10MHz_2A_FDD_t 41
typedef struct DCI2A_10MHz_2A_FDD DCI2A_10MHz_2A_FDD_t;

/// DCI Format Type 2A (10 MHz, FDD, 4 Antenna Ports, 38 bits)
struct DCI2A_10MHz_4A_FDD {
  uint64_t padding:26;
  /// TPMI information for precoding
  uint64_t tpmi:2;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_10MHz_4A_FDD_t 38
typedef struct DCI2A_10MHz_4A_FDD DCI2A_10MHz_4A_FDD_t;

/// DCI Format Type 2A (20 MHz, TDD, 2 Antenna Ports, 51 bits)
struct DCI2A_20MHz_2A_TDD {
  uint64_t padding:13;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_20MHz_2A_TDD_t 51
typedef struct DCI2A_20MHz_2A_TDD DCI2A_20MHz_2A_TDD_t;

/// DCI Format Type 2A (20 MHz, TDD, 4 Antenna Ports, 53 bits)
struct DCI2A_20MHz_4A_TDD {
  uint64_t padding:11;
  /// TPMI information for precoding
  uint64_t tpmi:2;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_20MHz_4A_TDD_t 53
typedef struct DCI2A_20MHz_4A_TDD DCI2A_20MHz_4A_TDD_t;

/// DCI Format Type 2A (20 MHz, FDD, 2 Antenna Ports, 48 bits)
struct DCI2A_20MHz_2A_FDD {
  uint64_t padding:16;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_20MHz_2A_FDD_t 48
typedef struct DCI2A_20MHz_2A_FDD DCI2A_20MHz_2A_FDD_t;

/// DCI Format Type 2A (20 MHz, FDD, 4 Antenna Ports, 50 bits)
struct DCI2A_20MHz_4A_FDD {
  uint64_t padding:14;
  /// TPMI information for precoding
  uint64_t tpmi:2;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// TB swap
  uint64_t tb_swap:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));
#define sizeof_DCI2A_20MHz_4A_FDD_t 50
typedef struct DCI2A_20MHz_4A_FDD DCI2A_20MHz_4A_FDD_t;

// *******************************************************************
// ********************FORMAT 2B DCIs*********************************
// *******************************************************************
/// DCI Format Type 2B (1.5 MHz, TDD,  33 bits)
struct DCI2B_1_5MHz_TDD {
  uint64_t padding:31;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// SRS Request
  uint64_t srs_req:1;
  /// Scrambling ID
  uint64_t scrambling_id:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:6;
  /// Padding for ambiguity
  uint64_t padding0:1;
} __attribute__ ((__packed__));

typedef struct DCI2B_1_5MHz_TDD DCI2B_1_5MHz_TDD_t;
#define sizeof_DCI2B_1_5MHz_TDD_t 33

/// DCI Format Type 2B (5 MHz, TDD,  39 bits)
struct DCI2B_5MHz_TDD {
  /// padding to 64bits
  uint64_t padding:25;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// SRS Request
  uint64_t srs_req:1;
  /// Scrambling ID
  uint64_t scrambling_id:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2B_5MHz_TDD DCI2B_5MHz_TDD_t;
#define sizeof_DCI2B_5MHz_TDD_t 39

/// DCI Format Type 2B (10 MHz, TDD,  43 bits)
struct DCI2B_10MHz_TDD {
  /// padding to 64bits
  uint64_t padding:21;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// SRS Request
  uint64_t srs_req:1;
  /// Scrambling ID
  uint64_t scrambling_id:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2B_10MHz_TDD DCI2B_10MHz_TDD_t;
#define sizeof_DCI2B_10MHz_TDD_t 43

/// DCI Format Type 2B (20 MHz, TDD,  51 bits)
struct DCI2B_20MHz_TDD {
  /// padding to 64bits
  uint64_t padding:13;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// SRS Request
  uint64_t srs_req:1;
  /// Scrambling ID
  uint64_t scrambling_id:1;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2B_20MHz_TDD DCI2B_20MHz_TDD_t;
#define sizeof_DCI2B_20MHz_TDD_t 51

/// DCI Format Type 2B (1.5 MHz, FDD,  28 bits)
struct DCI2B_1_5MHz_FDD {
  //padding for 32 bits
  uint32_t padding:4;
  /// Redundancy version 2
  uint32_t rv2:2;
  /// New Data Indicator 2
  uint32_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint32_t mcs2:5;
  /// Redundancy version 1
  uint32_t rv1:2;
  /// New Data Indicator 1
  uint32_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint32_t mcs1:5;
  /// Scrambling ID
  uint32_t scrambling_id:1;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// Power Control
  uint32_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:6;
} __attribute__ ((__packed__));

typedef struct DCI2B_1_5MHz_FDD DCI2B_1_5MHz_FDD_t;
#define sizeof_DCI2B_1_5MHz_FDD_t 28

/// DCI Format Type 2B (5 MHz, FDD,  36 bits)
struct DCI2B_5MHz_FDD {
  /// padding for 64-bit
  uint64_t padding:28;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// Scrambling ID
  uint64_t scrambling_id:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

#define sizeof_DCI2B_5MHz_FDD_t 36
typedef struct DCI2B_5MHz_FDD DCI2B_5MHz_FDD_t;

/// DCI Format Type 2B (10 MHz, FDD,  41 bits)
struct DCI2B_10MHz_FDD {
  /// padding for 64-bit
  uint64_t padding:23;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// Scrambling ID
  uint64_t scrambling_id:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
  /// Padding for ambiguity
  uint64_t padding0:1;
} __attribute__ ((__packed__));

#define sizeof_DCI2B_10MHz_FDD_t 41
typedef struct DCI2B_10MHz_FDD DCI2B_10MHz_FDD_t;

/// DCI Format Type 2B (20 MHz, FDD,  48 bits)
struct DCI2B_20MHz_FDD {
  /// padding for 64-bit
  uint64_t padding:16;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// Scrambling ID
  uint64_t scrambling_id:1;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

#define sizeof_DCI2B_20MHz_FDD_t 48
typedef struct DCI2B_20MHz_FDD DCI2B_20MHz_FDD_t;

// *******************************************************************
// ********************FORMAT 2C DCIs*********************************
// *******************************************************************

/// DCI Format Type 2C (1.5 MHz, TDD,  34 bits)
struct DCI2C_1_5MHz_TDD {
  uint64_t padding:30;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// SRS Request
  uint64_t srs_req:1;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:6;
} __attribute__ ((__packed__));

typedef struct DCI2C_1_5MHz_TDD DCI2C_1_5MHz_TDD_t;
#define sizeof_DCI2C_1_5MHz_TDD_t 34

/// DCI Format Type 2C (5 MHz, TDD,  41 bits)
struct DCI2C_5MHz_TDD {
  /// padding to 64bits
  uint64_t padding:23;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// SRS Request
  uint64_t srs_req:1;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2C_5MHz_TDD DCI2C_5MHz_TDD_t;
#define sizeof_DCI2C_5MHz_TDD_t 41

/// DCI Format Type 2C (10 MHz, TDD,  45 bits)
struct DCI2C_10MHz_TDD {
  /// padding to 64bits
  uint64_t padding:19;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// SRS Request
  uint64_t srs_req:1;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2C_10MHz_TDD DCI2C_10MHz_TDD_t;
#define sizeof_DCI2C_10MHz_TDD_t 45

/// DCI Format Type 2C (20 MHz, TDD,  53 bits)
struct DCI2C_20MHz_TDD {
  /// padding to 64bits
  uint64_t padding:11;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// SRS Request
  uint64_t srs_req:1;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2C_20MHz_TDD DCI2C_20MHz_TDD_t;
#define sizeof_DCI2C_20MHz_TDD_t 53

/// DCI Format Type 2C (1.5 MHz, FDD,  30 bits)
struct DCI2C_1_5MHz_FDD {
  //padding for 32 bits
  uint32_t padding:2;
  /// Redundancy version 2
  uint32_t rv2:2;
  /// New Data Indicator 2
  uint32_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint32_t mcs2:5;
  /// Redundancy version 1
  uint32_t rv1:2;
  /// New Data Indicator 1
  uint32_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint32_t mcs1:5;
  /// Scrambling ID
  uint32_t ap_si_nl_id:3;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// Power Control
  uint32_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint32_t rballoc:6;
} __attribute__ ((__packed__));

typedef struct DCI2C_1_5MHz_FDD DCI2C_1_5MHz_FDD_t;
#define sizeof_DCI2C_1_5MHz_FDD_t 30

/// DCI Format Type 2C (5 MHz, FDD,  38 bits)
struct DCI2C_5MHz_FDD {
  /// padding for 64-bit
  uint64_t padding:26;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

#define sizeof_DCI2C_5MHz_FDD_t 38
typedef struct DCI2C_5MHz_FDD DCI2C_5MHz_FDD_t;

/// DCI Format Type 2C (10 MHz, FDD,  42 bits)
struct DCI2C_10MHz_FDD {
  /// padding for 64-bit
  uint64_t padding:22;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

#define sizeof_DCI2C_10MHz_FDD_t 43
typedef struct DCI2C_10MHz_FDD DCI2C_10MHz_FDD_t;

/// DCI Format Type 2C (20 MHz, FDD,  50 bits)
struct DCI2C_20MHz_FDD {
  /// padding for 64-bit
  uint64_t padding:14;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

#define sizeof_DCI2C_20MHz_FDD_t 50
typedef struct DCI2C_20MHz_FDD DCI2C_20MHz_FDD_t;

// *******************************************************************
// ********************FORMAT 2D DCIs*********************************
// *******************************************************************

/// DCI Format Type 2D (1.5 MHz, TDD,  36 bits)
struct DCI2D_1_5MHz_TDD {
  uint64_t padding:28;
  /// PDSCH REsource Mapping and Quasi-Co-Location Indicator
  uint64_t REMQCL:2;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// SRS Request
  uint64_t srs_req:1;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:6;
} __attribute__ ((__packed__));

typedef struct DCI2D_1_5MHz_TDD DCI2D_1_5MHz_TDD_t;
#define sizeof_DCI2D_1_5MHz_TDD_t 36

/// DCI Format Type 2D (5 MHz, TDD,  43 bits)
struct DCI2D_5MHz_TDD {
  /// padding to 64bits
  uint64_t padding:21;
  /// PDSCH REsource Mapping and Quasi-Co-Location Indicator
  uint64_t REMQCL:2;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// SRS Request
  uint64_t srs_req:1;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2D_5MHz_TDD DCI2D_5MHz_TDD_t;
#define sizeof_DCI2D_5MHz_TDD_t 43

/// DCI Format Type 2D (10 MHz, TDD,  47 bits)
struct DCI2D_10MHz_TDD {
  /// padding to 64bits
  uint64_t padding:17;
  /// PDSCH REsource Mapping and Quasi-Co-Location Indicator
  uint64_t REMQCL:2;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// SRS Request
  uint64_t srs_req:1;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2D_10MHz_TDD DCI2D_10MHz_TDD_t;
#define sizeof_DCI2D_10MHz_TDD_t 47

/// DCI Format Type 2D (20 MHz, TDD,  55 bits)
struct DCI2D_20MHz_TDD {
  /// padding to 64bits
  uint64_t padding:9;
  /// PDSCH REsource Mapping and Quasi-Co-Location Indicator
  uint64_t REMQCL:2;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// SRS Request
  uint64_t srs_req:1;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:4;
  /// Downlink Assignment Index
  uint64_t dai:2;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

typedef struct DCI2D_20MHz_TDD DCI2D_20MHz_TDD_t;
#define sizeof_DCI2D_20MHz_TDD_t 55

/// DCI Format Type 2D (1.5 MHz, FDD,  33 bits)
struct DCI2D_1_5MHz_FDD {
  //padding for 33 bits
  uint64_t padding:31;
  /// PDSCH REsource Mapping and Quasi-Co-Location Indicator
  uint64_t REMQCL:2;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:6;
  /// padding for ambiguity
  uint64_t padding0;
} __attribute__ ((__packed__));

typedef struct DCI2D_1_5MHz_FDD DCI2D_1_5MHz_FDD_t;
#define sizeof_DCI2D_1_5MHz_FDD_t 33

/// DCI Format Type 2D (5 MHz, FDD,  41 bits)
struct DCI2D_5MHz_FDD {
  /// padding for 64-bit
  uint64_t padding:23;
  /// PDSCH REsource Mapping and Quasi-Co-Location Indicator
  uint64_t REMQCL:2;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:13;
  /// Resource Allocation Header
  uint64_t rah:1;
  /// padding for ambiguity
  uint64_t padding0:1;
} __attribute__ ((__packed__));

#define sizeof_DCI2D_5MHz_FDD_t 41
typedef struct DCI2D_5MHz_FDD DCI2D_5MHz_FDD_t;

/// DCI Format Type 2D (10 MHz, FDD,  45 bits)
struct DCI2D_10MHz_FDD {
  /// padding for 64-bit
  uint64_t padding:19;
  /// PDSCH REsource Mapping and Quasi-Co-Location Indicator
  uint64_t REMQCL:2;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:17;
  /// Resource Allocation Header
  uint64_t rah:1;
  /// padding for ambiguity
  uint64_t padding0:1;
} __attribute__ ((__packed__));

#define sizeof_DCI2D_10MHz_FDD_t 45
typedef struct DCI2D_10MHz_FDD DCI2D_10MHz_FDD_t;

/// DCI Format Type 2D (20 MHz, FDD,  52 bits)
struct DCI2D_20MHz_FDD {
  /// padding for 64-bit
  uint64_t padding:12;
  /// Redundancy version 2
  uint64_t rv2:2;
  /// New Data Indicator 2
  uint64_t ndi2:1;
  /// Modulation and Coding Scheme and Redundancy Version 2
  uint64_t mcs2:5;
  /// Redundancy version 1
  uint64_t rv1:2;
  /// New Data Indicator 1
  uint64_t ndi1:1;
  /// Modulation and Coding Scheme and Redundancy Version 1
  uint64_t mcs1:5;
  /// Scrambling ID
  uint64_t ap_si_nl_id:3;
  /// HARQ Process
  uint64_t harq_pid:3;
  /// Power Control
  uint64_t TPC:2;
  /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
  uint64_t rballoc:25;
  /// Resource Allocation Header
  uint64_t rah:1;
} __attribute__ ((__packed__));

#define sizeof_DCI2D_20MHz_FDD_t 52
typedef struct DCI2D_20MHz_FDD DCI2D_20MHz_FDD_t;


typedef struct __attribute__ ((__packed__))
{
  uint32_t TPC:28;
}
DCI3_5MHz_TDD_0_t;
#define sizeof_DCI3_5MHz_TDD_0_t 27

typedef struct __attribute__ ((__packed__))
{
  uint32_t TPC:28;
}
DCI3_5MHz_TDD_1_6_t;
#define sizeof_DCI3_5MHz_TDD_1_6_t 27


typedef struct __attribute__ ((__packed__))
{
  uint32_t TPC:26;
}
DCI3_5MHz_FDD_t;
#define sizeof_DCI3_5MHz_FDD_t 25

///  DCI Format Type 0 (1.5 MHz,9 bits)
struct DCI0A_1_5MHz {
  /// Padding
  uint32_t padding:19;
  /// Cyclic shift
  uint32_t cshift:3;
  /// RB Assignment (ceil(log2(N_RB_UL*(N_RB_UL+1)/2)) bits)
  uint32_t rballoc:5;
  /// Hopping flag
  uint32_t hopping:1;
} __attribute__ ((__packed__));
#define sizeof_DCI0A_1_5MHz 9

///  DCI Format Type 0 (5 MHz,13 bits)
struct DCI0A_5MHz {
  /// Padding
  uint32_t padding:19;
  /// Cyclic shift
  uint32_t cshift:3;
  /// RB Assignment (ceil(log2(N_RB_UL*(N_RB_UL+1)/2)) bits)
  uint32_t rballoc:9;
  /// Hopping flag
  uint32_t hopping:1;
} __attribute__ ((__packed__));
#define sizeof_DCI0A_5MHz 13

///  DCI Format Type 0 (10 MHz,15 bits)
struct DCI0A_10_MHz {
  /// Padding
  uint32_t padding:17;
  /// Cyclic shift
  uint32_t cshift:3;
  /// RB Assignment (ceil(log2(N_RB_UL*(N_RB_UL+1)/2)) bits)
  uint32_t rballoc:11;
  /// Hopping flag
  uint32_t hopping:1;
} __attribute__ ((__packed__));
#define sizeof_DCI0A_10MHz 15

///  DCI Format Type 0 (20 MHz,17 bits)
struct DCI0A_20_MHz {
  /// Padding
  uint32_t padding:15;
  /// Cyclic shift
  uint32_t cshift:3;
  /// RB Assignment (ceil(log2(N_RB_UL*(N_RB_UL+1)/2)) bits)
  uint32_t rballoc:13;
  /// Hopping flag
  uint32_t hopping:1;
} __attribute__ ((__packed__));
#define sizeof_DCI0A_20MHz 17

#define MAX_DCI_SIZE_BITS 45

struct DCI_INFO_EXTRACTED {
    /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
    uint8_t type;
    /// Resource Allocation Header
    uint8_t rah;
    /// HARQ Process
    uint8_t harq_pid;
    /// CQI Request
    uint8_t cqi_req;
    /// SRS Request
    uint8_t srs_req;
    /// Power Control
    uint8_t TPC;
    /// Localized/Distributed VRB
    uint8_t vrb_type;
    /// RB Assignment (ceil(log2(N_RB_DL/P)) bits)
    uint32_t rballoc;
    // Applicable only when vrb_type = 1
    uint8_t Ngap;
    /// Cyclic shift
    uint8_t cshift;
    /// Hopping flag
    uint8_t hopping;
    /// Downlink Assignment Index
    uint8_t dai;
    /// DAI (TDD)
    uint8_t ulindex;

    /// TB swap
    uint8_t tb_swap;
    /// TPMI information for precoding
    uint8_t tpmi;
    /// Redundancy version 2
    uint8_t rv2;
    /// New Data Indicator 2
    uint8_t ndi2;
    /// Modulation and Coding Scheme and Redundancy Version 2
    uint8_t mcs2;
    /// Redundancy version 1
    uint8_t rv1;
    /// New Data Indicator 1
    uint8_t ndi1;
    /// Modulation and Coding Scheme and Redundancy Version 1
    uint8_t mcs1;

    /// Scrambling ID
    uint64_t ap_si_nl_id:3;
};
typedef struct DCI_INFO_EXTRACTED DCI_INFO_EXTRACTED_t;
