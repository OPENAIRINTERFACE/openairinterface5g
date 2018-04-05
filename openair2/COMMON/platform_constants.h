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

/*
                                 platform_constants.h
                             -------------------
  AUTHOR  : Lionel GAUTHIER and Navid Nikaein
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr

 ***************************************************************************/

#ifdef CMAKER
#include "asn1_constants.h"
#else
#include "RRC/LITE/MESSAGES/asn1_constants.h"
#endif

#ifndef __PLATFORM_CONSTANTS_H__
#    define __PLATFORM_CONSTANTS_H__

#ifdef JUMBO_FRAME
#define NL_MAX_PAYLOAD 18000  /* this should cover the max mtu size*/
#else
#define NL_MAX_PAYLOAD 9000  /* this should cover the max mtu size*/
#endif

#ifdef LARGE_SCALE
#    define NB_MODULES_MAX 128
#    define NB_NODE_MAX    128
#else
#    define NB_MODULES_MAX 32
#    define NB_NODE_MAX    32
#endif

#ifdef JUMBO_FRAME
#    define MAX_IP_PACKET_SIZE         10000 // 9000
#else
# if defined(OAI_NW_DRIVER_TYPE_ETHERNET)
/* SR: When using ethernet network driver the packet size is 1512 :
 * 1500 bytes IP packet + 14 bytes ethernet header
 */
#   define MAX_IP_PACKET_SIZE          1514
# else
#   define MAX_IP_PACKET_SIZE          1500 // 3000
# endif
#endif
// overwrite the previous deinitions

#    define MAX_MODULES                NB_MODULES_MAX

#ifdef LARGE_SCALE
#    define MAX_MOBILES_PER_ENB         128
//#    define MAX_RG                      2
#else
#    define MAX_MOBILES_PER_ENB         16
//#    define MAX_RG                      2
#endif

#define MAX_MANAGED_ENB_PER_MOBILE  2

///NB-IOT
#define NB_RB_MAX_NB_IOT  (maxDRB_NB_r13 + 3) //MP: NB_IoT --> 2(DRB)+3(SRBs - 2 is not used) = 5


#define DEFAULT_RAB_ID 1

#define NB_RB_MAX      (maxDRB + 3) /* was 11, now 14, maxDRB comes from asn1_constants.h, + 3 because of 3 SRB, one invisible id 0, then id 1 and 2 */
#if defined(Rel10) || defined(Rel14)
#define NB_RB_MBMS_MAX (maxSessionPerPMCH*maxServiceCount)
#else
// Do not allocate unused memory
#define NB_RB_MBMS_MAX 1
#endif
#define NB_RAB_MAX     maxDRB       /* was 8, now 11 */
#define RAB_SHIFT1     9
#define RAB_SHIFT2     3
#define RAB_OFFSET     0x0007
#define RAB_OFFSET1    0x7E00
#define RAB_OFFSET2    0x01F8
#define DIR_OFFSET     0x8000
#define DIR_SHIFT      15
#define CH_OFFSET      0x0004
#define CH_SHIFT       2

// RLC_AM_SEND_MRW
#        define SEND_MRW_OFF    15
#        define SEND_MRW_ON     240

// RLC_SEGMENTATION_INDICATION
#        define SEGMENTATION_NOT_ALLOWED  0
#        define SEGMENTATION_ALLOWED      1

// RLC_DELIVERY_OF_ERRONEOUS_SDU
#        define DELIVERY_OF_ERRONEOUS_SDU_NO           0
#        define DELIVERY_OF_ERRONEOUS_SDU_YES          1
#        define DELIVERY_OF_ERRONEOUS_SDU_NO_DETECT    2


// CBA constant
#define NUM_MAX_CBA_GROUP 4

#    ifndef __cplusplus
#        ifndef NULL
#            define NULL 0
#        endif
#        ifndef null
#            define null 0
#        endif
#    endif

#define  UNUSED_PARAM_MBMS_SESSION_ID  0
#define  UNUSED_PARAM_MBMS_SERVICE_ID  0

#define printk printf

#define UNUSED_VARIABLE(vARIABLE)   (void)(vARIABLE)

#endif /* __PLATFORM_CONSTANTS_H__ */
