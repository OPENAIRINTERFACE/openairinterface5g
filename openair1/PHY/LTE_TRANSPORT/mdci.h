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

#ifndef __M_DCI__H__
#define __M_DCI__H__

#include <stdint.h>


///  basic DCI Format Type 6-0A (5 MHz, FDD, rep2, no mod order override - XX bits)
struct DCI6_0A_5MHz {
  /// padding to fill 32 bit word
  uint32_t padding:5;
  /// DCI subframe repetition
  uint32_t dci_rep:2;
  /// SRS Request
  uint32_t srs_req:1;
  /// CSI Request
  uint32_t csi_req:1;
  /// Power Control
  uint32_t TPC:2;
  /// redundancy version
  uint32_t rv_idx:2;
  /// new data indicator
  uint32_t ndi:1;
  /// harq id
  uint32_t harq_pid:3;
  /// Repetition number
  uint32_t rep:2;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:4;
  /// RB Assignment (ceil(log2(floor(N_RB_UL/6))) + 5 bits)
  uint32_t rballoc:5;
  /// narrowband index log2(floor(N_RB_DL/6))) bits
  uint32_t narrowband:2;
  /// Hopping flag
  uint32_t hopping:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_0A_5MHz DCI6_0A_5MHz_t; 
#define sizeof_DCI6_0A_5MHz_t 28 // extended by one bit to match DCI6_1A

/// basic DCI Format Type 6-1A (5 MHz, FDD primary carrier,  24 bits, 5 bit format, TM!=9,TM!=6, no scheduling enhancement)
struct DCI6_1A_5MHz {
  /// padding to fill 32-bit word
  uint32_t padding:4;
  /// DCI subframe repetition number 
  uint32_t dci_rep:2;
  /// HARQ-ACK resource offset
  uint32_t harq_ack_off:2;
  /// SRS request 
  uint32_t srs_req:1;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// Repetition number
  uint32_t rep:2;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:4;
  /// Resource block assignment (assignment flag = 0 for 5 MHz, ceil(log2(floor(N_RB_DL/6)))+5)
  uint32_t rballoc:5;
  /// narrowband index log2(floor(N_RB_DL/6))) bits
  uint32_t narrowband:2;
  /// Frequency hopping flag
  uint32_t hopping:1;
  /// 0/1A differentiator
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_1A_5MHz DCI6_1A_5MHz_t;
#define sizeof_DCI6_1A_5MHz_t 28

///  basic DCI Format Type 6-0A (10 MHz, FDD, rep2, no mod order override - XX bits)
struct DCI6_0A_10MHz {
  /// padding to fill 32 bit word
  uint32_t padding:4;
  /// DCI subframe repetition
  uint32_t dci_rep:2;
  /// SRS Request
  uint32_t srs_req:1;
  /// CSI Request
  uint32_t csi_req:1;
  /// Power Control
  uint32_t TPC:2;
  /// redundancy version
  uint32_t rv_idx:2;
  /// new data indicator
  uint32_t ndi:1;
  /// harq id
  uint32_t harq_pid:3;
  /// Repetition number
  uint32_t rep:2;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:4;
  /// RB Assignment (ceil(log2(floor(N_RB_UL/6))) + 5 bits)
  uint32_t rballoc:5;
  /// narrowband index log2(floor(N_RB_DL/6))) bits
  uint32_t narrowband:3;
  /// Hopping flag
  uint32_t hopping:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_0A_10MHz DCI6_0A_10MHz_t; 
#define sizeof_DCI6_0A_10MHz_t 29 // extended by one bit to match DCI6_1A

/// basic DCI Format Type 6-1A (10 MHz, FDD primary carrier,  24 bits, 5 bit format, TM!=9,TM!=6, no scheduling enhancement)
struct DCI6_1A_10MHz {
  /// padding to fill 32-bit word
  uint32_t padding:3;
  /// DCI subframe repetition number 
  uint32_t dci_rep:2;
  /// HARQ-ACK resource offset
  uint32_t harq_ack_off:2;
  /// SRS request 
  uint32_t srs_req:1;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// Repetition number
  uint32_t rep:2;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:4;
  /// Resource block assignment
  uint32_t rballoc:5;
  /// narrowband index log2(floor(N_RB_DL/6))) bits
  uint32_t narrowband:3;
  /// Frequency hopping flag
  uint32_t hopping:1;
  /// 0/1A differentiator
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_1A_10MHz DCI6_1A_10MHz_t;
#define sizeof_DCI6_1A_10MHz_t 29

///  basic DCI Format Type 6-0A (20 MHz, FDD, rep2, no mod order override - XX bits)
struct DCI6_0A_20MHz {
  /// padding to fill 32 bit word
  uint32_t padding:4;
  /// DCI subframe repetition
  uint32_t dci_rep:2;
  /// SRS Request
  uint32_t srs_req:1;
  /// CSI Request
  uint32_t csi_req:1;
  /// Power Control
  uint32_t TPC:2;
  /// redundancy version
  uint32_t rv_idx:2;
  /// new data indicator
  uint32_t ndi:1;
  /// harq id
  uint32_t harq_pid:3;
  /// Repetition number
  uint32_t rep:2;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:4;
  /// RB Assignment (ceil(log2(floor(N_RB_UL/6))) + 5 bits)
  uint32_t rballoc:5;
  /// narrowband index log2(floor(N_RB_DL/6))) bits
  uint32_t narrowband:4;
  /// Hopping flag
  uint32_t hopping:1;
  /// type = 0 => DCI Format 0, type = 1 => DCI Format 1A
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_0A_20MHz DCI6_0A_20MHz_t; 
#define sizeof_DCI6_0A_20MHz_t 30 // extended by one bit to match DCI6_1A

/// basic DCI Format Type 6-1A (20 MHz, FDD primary carrier,  24 bits, 5 bit format, TM!=9,TM!=6, no scheduling enhancement)
struct DCI6_1A_20MHz {
  /// padding to fill 32-bit word
  uint32_t padding:3;
  /// DCI subframe repetition number 
  uint32_t dci_rep:2;
  /// HARQ-ACK resource offset
  uint32_t harq_ack_off:2;
  /// SRS request 
  uint32_t srs_req:1;
  /// Power Control
  uint32_t TPC:2;
  /// Redundancy version
  uint32_t rv:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:3;
  /// Repetition number
  uint32_t rep:2;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:4;
  /// Resource block assignment (assignment flag = 0 for 20 MHz, ceil(log2(floor(N_RB_DL/6)))+5)
  uint32_t rballoc:5;
  /// narrowband index log2(floor(N_RB_DL/6))) bits
  uint32_t narrowband:4;
  /// Frequency hopping flag
  uint32_t hopping:1;
  /// 0/1A differentiator
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_1A_20MHz DCI6_1A_20MHz_t;
#define sizeof_DCI6_1A_20MHz_t 30

///  basic DCI Format Type 6-0B (5 MHz)
struct DCI6_0B_5MHz {
  /// padding to fill 32-bit word
  uint32_t padding:15;
  /// DCI subframe repetition
  uint32_t dci_rep:2;
  /// new data indicator
  uint32_t ndi:1;
  /// harq id
  uint32_t harq_pid:1;
  /// Repetition number
  uint32_t rep:3;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:4;
  /// RB Assignment (ceil(log2(floor(N_RB_UL/6))) + 3 bits)
  uint32_t rballoc:5;
  /// type = 0 => DCI Format 0B, type = 1 => DCI Format 1B
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_0B_5MHz DCI6_0B_5MHz_t; 
#define sizeof_DCI6_0B_5MHz_t 17

/// basic DCI Format Type 6-1B (5 MHz)
struct DCI6_1B_5MHz {
  /// padding to fill 32-bit word
  uint32_t padding:15;
  /// DCI subframe repetition number 
  uint32_t dci_rep:2;
  /// HARQ-ACK resource offset
  uint32_t harq_ack_off:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:1;
  /// Repetition number
  uint32_t rep:3;
  /// Resource block assignment (assignment flag = 0 for 5 MHz, ceil(log2(floor(N_RB_DL/6)))+1)
  uint32_t rballoc:3;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:4;
  /// 0B/1B differentiator
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_1B_5MHz DCI6_1B_5MHz_t;
#define sizeof_DCI6_1B_5MHz_t 17 

///  basic DCI Format Type 6-0B (10 MHz)
struct DCI6_0B_10MHz {
  /// padding to fill 32-bit word
  uint32_t padding:14;
  /// DCI subframe repetition
  uint32_t dci_rep:2;
  /// new data indicator
  uint32_t ndi:1;
  /// harq id
  uint32_t harq_pid:1;
  /// Repetition number
  uint32_t rep:3;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:4;
  /// RB Assignment (ceil(log2(floor(N_RB_UL/6))) + 3 bits)
  uint32_t rballoc:6;
  /// type = 0 => DCI Format 0B, type = 1 => DCI Format 1B
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_0B_10MHz DCI6_0B_10MHz_t; 
#define sizeof_DCI6_0B_10MHz_t 18

/// basic DCI Format Type 6-1B (10 MHz)
struct DCI6_1B_10MHz {
  /// padding to fill 32-bit word
  uint32_t padding:15;
  /// DCI subframe repetition number 
  uint32_t dci_rep:2;
  /// HARQ-ACK resource offset
  uint32_t harq_ack_off:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:1;
  /// Repetition number
  uint32_t rep:2;
  /// Resource block assignment (assignment flag = 0 for 5 MHz, ceil(log2(floor(N_RB_DL/6)))+1)
  uint32_t rballoc:4;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:4;
  /// 0B/1B differentiator
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_1B_10MHz DCI6_1B_10MHz_t;
#define sizeof_DCI6_1B_10MHz_t 17 

///  basic DCI Format Type 6-0B (20 MHz)
struct DCI6_0B_20MHz {
  /// padding to fill 32-bit word
  uint32_t padding:14;
  /// DCI subframe repetition
  uint32_t dci_rep:2;
  /// new data indicator
  uint32_t ndi:1;
  /// harq id
  uint32_t harq_pid:1;
  /// Repetition number
  uint32_t rep:2;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:4;
  /// RB Assignment (ceil(log2(floor(N_RB_UL/6))) + 3 bits)
  uint32_t rballoc:7;
  /// type = 0 => DCI Format 0B, type = 1 => DCI Format 1B
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_0B_20MHz DCI6_0B_20MHz_t; 
#define sizeof_DCI6_0B_20MHz_t 18

/// basic DCI Format Type 6-1B (10 MHz)
struct DCI6_1B_20MHz {
  /// padding to fill 32-bit word
  uint32_t padding:14;
  /// DCI subframe repetition number 
  uint32_t dci_rep:2;
  /// HARQ-ACK resource offset
  uint32_t harq_ack_off:2;
  /// New Data Indicator
  uint32_t ndi:1;
  /// HARQ Process
  uint32_t harq_pid:1;
  /// Repetition number
  uint32_t rep:2;
  /// Resource block assignment (assignment flag = 0 for 5 MHz, ceil(log2(floor(N_RB_DL/6)))+1)
  uint32_t rballoc:5;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:4;
  /// 0B/1B differentiator
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_1B_20MHz DCI6_1B_20MHz_t;
#define sizeof_DCI6_1B_20MHz_t 18 

/// basic DCI Format Type 6-2 for paging (5 MHz)
struct DCI6_2_paging_5MHz {
  /// padding to reach 32-bits
  uint32_t padding:21;
  /// DCI subframe repetition number 
  uint32_t dci_rep:2;
  /// Repetition number
  uint32_t rep:3;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:3;
  /// Resource block assignment (ceil(log2(floor(N_RB_DL/6))) bits)
  uint32_t rballoc:2;
  /// 0B/1B paging/directindication differentiator
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_2_paging_5MHz DCI6_2_paging_5MHz_t;

/// basic DCI Format Type 6-2 for direct indication (5 MHz)
struct DCI6_2_di_5MHz {
  /// padding to reach 32-bits
  uint32_t padding:23;
  /// Direct indication information
  uint32_t di_info:8;
  /// 0B/1B paging/directindication differentiator
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_2_di_5MHz DCI6_2_di_5MHz_t;
#define sizeof_DCI6_2_5MHz_t 11

/// basic DCI Format Type 6-2 for paging (10 MHz)
struct DCI6_2_paging_10MHz {
  /// padding to reach 32-bits
  uint32_t padding:20;
  /// DCI subframe repetition number 
  uint32_t dci_rep:2;
  /// Repetition number
  uint32_t rep:3;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:3;
  /// Resource block assignment (ceil(log2(floor(N_RB_DL/6))))
  uint32_t rballoc:3;
  /// 0B/1B paging/directindication differentiator
  uint32_t type:1;
} __attribute__ ((__packed__));
typedef struct DCI6_2_paging_10MHz DCI6_2_paging_10MHz_t;

/// basic DCI Format Type 6-2 for direct indication (10 MHz)
struct DCI6_2_di_10MHz {
  /// padding to reach 32-bits
  uint32_t padding:23;
  /// Direct indication information
  uint32_t di_info:8;
  /// 0B/1B paging/directindication differentiator
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_2_di_10MHz DCI6_2_di_10MHz_t;
#define sizeof_DCI6_2_10MHz_t 12

/// basic DCI Format Type 6-2 for paging (20 MHz)
struct DCI6_2_paging_20MHz {
  /// padding to reach 32-bits
  uint32_t padding:19;
  /// DCI subframe repetition number 
  uint32_t dci_rep:2;
  /// Repetition number
  uint32_t rep:3;
  /// Modulation and Coding Scheme and Redundancy Version
  uint32_t mcs:3;
  /// Resource block assignment (ceil(log2(floor(N_RB_DL/6))))
  uint32_t rballoc:4;
  /// 0B/1B paging/directindication differentiator
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_2_paging_20MHz DCI6_2_paging_20MHz_t;

/// basic DCI Format Type 6-2 for direct indication (20 MHz)
struct DCI6_2_di_20MHz {
  /// padding to reach 32-bits
  uint32_t padding:23;
  /// Direct indication information
  uint32_t di_info:8;
  /// 0B/1B paging/directindication differentiator
  uint32_t type:1;
} __attribute__ ((__packed__));

typedef struct DCI6_2_di_20MHz DCI6_2_di_20MHz_t;
#define sizeof_DCI6_2_20MHz_t 13

#endif
