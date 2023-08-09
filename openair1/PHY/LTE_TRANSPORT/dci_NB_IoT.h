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
#ifndef __DCI_NB_IOT_H__
#define __DCI_NB_IOT_H__

#include <stdint.h>


typedef enum 
{
  DCIFormatN0 = 0,
  DCIFormatN1,
  DCIFormatN1_RA,//is for initial RA procedure (semi-static information) so maybe is not needed
  DCIFormatN1_RAR,
  DCIFormatN2,
  DCIFormatN2_Ind,
  DCIFormatN2_Pag,
}DCI_format_NB_IoT_t;

///  DCI Format Type 0 (180 kHz, 23 bits)
struct DCIFormatN0{
  /// type = 0 => DCI Format N0, type = 1 => DCI Format N1, 1 bits
  uint8_t type;
  /// Subcarrier indication, 6 bits
  uint8_t scind;
  /// Resourse Assignment (RU Assignment), 3 bits
  uint8_t ResAssign;
  /// Modulation and Coding Scheme, 4 bits
  uint8_t mcs;
  /// New Data Indicator, 1 bits
  uint8_t ndi;
  /// Scheduling Delay, 2 bits
  uint8_t Scheddly;
  /// Repetition Number, 3 bits
  uint8_t RepNum;
  /// Redundancy version for HARQ (only use 0 and 2), 1 bits
  uint8_t rv;
  /// DCI subframe repetition Number, 2 bits
  uint8_t DCIRep;
};

typedef struct DCIFormatN0 DCIFormatN0_t;

///  DCI Format Type N1 for User data
struct DCIFormatN1{
  /// type = 0 => DCI Format N0, type = 1 => DCI Format N1,1bits
  uint8_t type;
  //NPDCCH order indicator (set to 0), 1 bits
  uint8_t orderIndicator;
  // Scheduling Delay,3 bits
  uint8_t Scheddly;
  // Resourse Assignment (RU Assignment),3 bits
  uint8_t ResAssign;
  // Modulation and Coding Scheme,4 bits
  uint8_t mcs;
  // Repetition Number,4 bits
  uint8_t RepNum;
  // New Data Indicator,1 bits
  uint8_t ndi;
  // HARQ-ACK resource,4 bits
  uint8_t HARQackRes;
  // DCI subframe repetition Number,2 bits
  uint8_t DCIRep;
};


typedef struct DCIFormatN1 DCIFormatN1_t;

///  DCI Format Type N1 for initial RA
struct DCIFormatN1_RA{
  /// type = 0 => DCI Format N0, type = 1 => DCI Format N1, 1 bits
  uint8_t type;
  //NPDCCH order indicator (set to 0),1 bits
  uint8_t orderIndicator;
  // Start number of NPRACH repetiiton, 2 bits
  uint8_t Scheddly;
  // Subcarrier indication of NPRACH, 6 bits
  uint8_t scind;
  // All the remainging bits, 13 bits
  uint8_t remaingingBits;
};

typedef struct DCIFormatN1_RA DCIFormatN1_RA_t;

///  DCI Format Type N1 for User data
struct DCIFormatN1_RAR{
  /// type = 0 => DCI Format N0, type = 1 => DCI Format N1,1bits
  uint8_t type;
  //NPDCCH order indicator (set to 0), 1 bits
  uint8_t orderIndicator;
  // Scheduling Delay,3 bits
  uint8_t Scheddly;
  // Resourse Assignment (RU Assignment),3 bits
  uint8_t ResAssign;
  // Modulation and Coding Scheme,4 bits
  uint8_t mcs;
  // Repetition Number,4 bits
  uint8_t RepNum;
  // New Data Indicator,1 bits,reserved in the RAR
  uint8_t ndi;
  // HARQ-ACK resource,4 bits,reserved in the RAR
  uint8_t HARQackRes;
  // DCI subframe repetition Number,2 bits
  uint8_t DCIRep;
};

typedef struct DCIFormatN1_RAR DCIFormatN1_RAR_t;

//  DCI Format Type N2 for direct indication, 15 bits
struct DCIFormatN2_Ind{
  //Flag for paging(1)/direct indication(0), set to 0,1 bits
  uint8_t type;
  //Direct indication information, 8 bits
  uint8_t directIndInf;
  // Reserved information bits, 6 bits
  uint8_t resInfoBits;
};

typedef struct DCIFormatN2_Ind DCIFormatN2_Ind_t;

//  DCI Format Type N2 for Paging, 15 bits
struct DCIFormatN2_Pag{
  //Flag for paging(1)/direct indication(0), set to 1,1 bits
  uint8_t type;
  // Resourse Assignment (RU Assignment), 3 bits
  uint8_t ResAssign;
  // Modulation and Coding Scheme, 4 bits
  uint8_t mcs;
  // Repetition Number, 4 bits
  uint8_t RepNum;
  // Reserved 3 bits
  uint8_t DCIRep;
};

typedef struct DCIFormatN2_Pag DCIFormatN2_Pag_t;

typedef union DCI_CONTENT {
 // 
 DCIFormatN0_t DCIN0;
 //
 DCIFormatN1_t DCIN1;
 //
 DCIFormatN1_RA_t DCIN1_RA;
 //
 DCIFormatN1_RAR_t DCIN1_RAR;
 //
 DCIFormatN2_Ind_t DCIN2_Ind;
 //
 DCIFormatN2_Pag_t DCIN2_Pag;

 }DCI_CONTENT;

 /*Structure for packing*/

 struct DCIN0{
  /// DCI subframe repetition Number, 2 bits
  uint8_t DCIRep:2;
  /// New Data Indicator, 1 bits
  uint8_t ndi:1;
  /// Repetition Number, 3 bits
  uint8_t RepNum:3;
  /// Redundancy version for HARQ (only use 0 and 2), 1 bits
  uint8_t rv:1;
  /// Modulation and Coding Scheme, 4 bits
  uint8_t mcs:4;
  /// Scheduling Delay, 2 bits
  uint8_t Scheddly:2;
  /// Resourse Assignment (RU Assignment), 3 bits
  uint8_t ResAssign:3;
  /// Subcarrier indication, 6 bits
  uint8_t scind:6;
  /// type = 0 => DCI Format N0, type = 1 => DCI Format N1, 1 bits
  uint8_t type:1;
 } __attribute__ ((__packed__));

 typedef struct DCIN0 DCIN0_t;
#define sizeof_DCIN0_t 23

struct DCIN1_RAR{
  // DCI subframe repetition Number, 2 bits
  uint8_t DCIRep:2;
  // HARQ-ACK resource,4 bits
  uint8_t HARQackRes:4; 
  // New Data Indicator,1 bits
  uint8_t ndi:1;
  // Repetition Number, 4 bits
  uint8_t RepNum:4;
  // Modulation and Coding Scheme, 4 bits
  uint8_t mcs:4;
  // Resourse Assignment (RU Assignment), 3 bits
  uint8_t ResAssign:3;
  // Scheduling Delay, 3 bits
  uint8_t Scheddly:3;
  //NPDCCH order indicator (set to 0),1 bits
  uint8_t orderIndicator:1;
  /// type = 0 => DCI Format N0, type = 1 => DCI Format N1, 1 bits
  uint8_t type:1;
 } __attribute__ ((__packed__));

 typedef struct DCIN1_RAR DCIN1_RAR_t;
#define sizeof_DCIN1_RAR_t 23

struct DCIN1{
  // DCI subframe repetition Number, 2 bits
  uint8_t DCIRep:2;
  // HARQ-ACK resource,4 bits
  uint8_t HARQackRes:4; 
  // New Data Indicator,1 bits
  uint8_t ndi:1;
  // Repetition Number, 4 bits
  uint8_t RepNum:4;
  // Modulation and Coding Scheme, 4 bits
  uint8_t mcs:4;
  // Resourse Assignment (RU Assignment), 3 bits
  uint8_t ResAssign:3;
  // Scheduling Delay, 3 bits
  uint8_t Scheddly:3;
  //NPDCCH order indicator (set to 0),1 bits
  uint8_t orderIndicator:1;
  /// type = 0 => DCI Format N0, type = 1 => DCI Format N1, 1 bits
  uint8_t type:1;
 } __attribute__ ((__packed__));

 typedef struct DCIN1 DCIN1_t;
#define sizeof_DCIN1_t 23

//  DCI Format Type N2 for direct indication, 15 bits
struct DCIN2_Ind{
  // Reserved information bits, 6 bits
  uint8_t resInfoBits:6;
  //Direct indication information, 8 bits
  uint8_t directIndInf:8;
  //Flag for paging(1)/direct indication(0), set to 0,1 bits
  uint8_t type:1;
} __attribute__ ((__packed__));;

typedef struct DCIN2_Ind DCIN2_Ind_t;
#define sizeof_DCIN2_Ind_t 15

//  DCI Format Type N2 for Paging, 15 bits
struct DCIN2_Pag{
  // Reserved 3 bits
  uint8_t DCIRep:3;
  // Repetition Number, 4 bits
  uint8_t RepNum:4;
  // Modulation and Coding Scheme, 4 bits
  uint8_t mcs:4;
  // Resourse Assignment (RU Assignment), 3 bits
  uint8_t ResAssign:3;
  //Flag for paging(1)/direct indication(0), set to 1,1 bits
  uint8_t type:1;
} __attribute__ ((__packed__));;

typedef struct DCIN2_Pag DCIN2_Pag_t;

#define sizeof_DCIN2_Pag_t 15

#define MAX_DCI_SIZE_BITS_NB_IoT 23

#endif
