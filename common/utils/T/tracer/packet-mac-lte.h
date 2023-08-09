/* packet-mac-lte.h
 *
 * Martin Mathieson
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This header file may also be distributed under
 * the terms of the BSD Licence as follows:
 *
 * Copyright (C) 2009 Martin Mathieson. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __COMMON_UTILS_T_TRACER_PACKET_MAC_LTE__H__
#define __COMMON_UTILS_T_TRACER_PACKET_MAC_LTE__H__

/* radioType */
#define FDD_RADIO 1
#define TDD_RADIO 2

/* Direction */
#define DIRECTION_UPLINK   0
#define DIRECTION_DOWNLINK 1

/* rntiType */
#define NO_RNTI     0
#define P_RNTI      1
#define RA_RNTI     2
#define C_RNTI      3
#define SI_RNTI     4
#define SPS_RNTI    5
#define M_RNTI      6
#define SL_BCH_RNTI 7
#define SL_RNTI     8
#define SC_RNTI     9
#define G_RNTI      10

/*****************************************************************/
/* UDP framing format                                            */
/* -----------------------                                       */
/* Several people have asked about dissecting MAC by framing     */
/* PDUs over IP.  A suggested format over UDP has been created   */
/* and implemented by this dissector, using the definitions      */
/* below. A link to an example program showing you how to encode */
/* these headers and send LTE MAC PDUs on a UDP socket is        */
/* provided at https://wiki.wireshark.org/MAC-LTE                */
/*                                                               */
/* A heuristic dissector (enabled by a preference) will          */
/* recognise a signature at the beginning of these frames.       */
/*****************************************************************/


/* Signature.  Rather than try to define a port for this, or make the
   port number a preference, frames will start with this string (with no
   terminating NULL */
#define MAC_LTE_START_STRING "mac-lte"

/* Fixed fields.  This is followed by the following 3 mandatory fields:
   - radioType (1 byte)
   - direction (1 byte)
   - rntiType (1 byte)
   (where the allowed values are defined above */

/* Optional fields. Attaching this info to frames will allow you
   to show you display/filter/plot/add-custom-columns on these fields, so should
   be added if available.
   The format is to have the tag, followed by the value (there is no length field,
   it's implicit from the tag) */

#define MAC_LTE_RNTI_TAG            0x02
/* 2 bytes, network order */

#define MAC_LTE_UEID_TAG            0x03
/* 2 bytes, network order */

#define MAC_LTE_FRAME_SUBFRAME_TAG  0x04
/* 2 bytes, network order, SFN is stored in 12 MSB and SF in 4 LSB */

#define MAC_LTE_PREDEFINED_DATA_TAG 0x05
/* 1 byte */

#define MAC_LTE_RETX_TAG            0x06
/* 1 byte */

#define MAC_LTE_CRC_STATUS_TAG      0x07
/* 1 byte */

#define MAC_LTE_EXT_BSR_SIZES_TAG   0x08
/* 0 byte */

#define MAC_LTE_SEND_PREAMBLE_TAG   0x09
/* 2 bytes, RAPID value (1 byte) followed by RACH attempt number (1 byte) */

#define MAC_LTE_CARRIER_ID_TAG      0x0A
/* 1 byte */

#define MAC_LTE_PHY_TAG             0x0B
/* variable length, length (1 byte) then depending on direction
   in UL: modulation type (1 byte), TBS index (1 byte), RB length (1 byte),
          RB start (1 byte), HARQ id (1 byte), NDI (1 byte)
   in DL: DCI format (1 byte), resource allocation type (1 byte), aggregation level (1 byte),
          MCS index (1 byte), redundancy version (1 byte), resource block length (1 byte),
          HARQ id (1 byte), NDI (1 byte), TB (1 byte), DL reTx (1 byte) */

#define MAC_LTE_SIMULT_PUCCH_PUSCH_PCELL_TAG  0x0C
/* 0 byte */

#define MAC_LTE_SIMULT_PUCCH_PUSCH_PSCELL_TAG 0x0D
/* 0 byte */

#define MAC_LTE_CE_MODE_TAG         0x0E
/* 1 byte containing mac_lte_ce_mode enum value */

#define MAC_LTE_NB_MODE_TAG         0x0F
/* 1 byte containing mac_lte_nb_mode enum value */

#define MAC_LTE_N_UL_RB_TAG         0x10
/* 1 byte containing the number of UL resource blocks: 6, 15, 25, 50, 75 or 100 */

#define MAC_LTE_SR_TAG              0x11
/* 2 bytes for the number of items, followed by that number of ueid, rnti (2 bytes each) */


/* MAC PDU. Following this tag comes the actual MAC PDU (there is no length, the PDU
   continues until the end of the frame) */
#define MAC_LTE_PAYLOAD_TAG 0x01

#endif
