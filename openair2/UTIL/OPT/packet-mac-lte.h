/* packet-mac-lte.h
 *
 * Martin Mathieson
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This header file may also be distributed under
 * the terms of the BSD Licence as follows:
 *
 * Copyright (C) 2009 Martin Mathieson. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

 /* 
 this is wireshark, commit: commit eda834b6e29c36e05a63a6056afa98390ff79357 
 Date:   Wed Aug 22 14:36:20 2018 +0200
 modified to be used in OpenAir to create the LTE MAC/RLC encapsulated in UDP as per Wireshark feature 
 */

#ifndef __UTIL_OPT_PACKET_MAC_LTE__H__
#define __UTIL_OPT_PACKET_MAC_LTE__H__

#include "ws_symbol_export.h"

/** data structure to hold time values with nanosecond resolution*/
typedef struct {
	time_t	secs;
	int	nsecs;
} nstime_t;


/* radioType */
#define FDD_RADIO 1
#define TDD_RADIO 2

/* Direction */
#define DIRECTION_UPLINK   0
#define DIRECTION_DOWNLINK 1

/* rntiType */
#define WS_NO_RNTI     0
#define WS_P_RNTI      1
#define WS_RA_RNTI     2
#define WS_C_RNTI      3
#define WS_SI_RNTI     4
#define WS_SPS_RNTI    5
#define WS_M_RNTI      6
#define WS_SL_BCH_RNTI 7
#define WS_SL_RNTI     8
#define WS_SC_RNTI     9
#define WS_G_RNTI      10

typedef enum mac_lte_oob_event {
    ltemac_send_preamble,
    ltemac_send_sr,
    ltemac_sr_failure
} mac_lte_oob_event;

typedef enum mac_lte_dl_retx {
    dl_retx_no,
    dl_retx_yes,
    dl_retx_unknown
} mac_lte_dl_retx;

typedef enum mac_lte_crc_status {
    crc_fail = 0,
    crc_success = 1,
    crc_high_code_rate = 2,
    crc_pdsch_lost = 3,
    crc_duplicate_nonzero_rv = 4,
    crc_false_dci = 5
} mac_lte_crc_status;

/* N.B. for SCellIndex-r13 extends to 31 */
typedef enum mac_lte_carrier_id {
    carrier_id_primary,
    carrier_id_secondary_1,
    carrier_id_secondary_2,
    carrier_id_secondary_3,
    carrier_id_secondary_4,
    carrier_id_secondary_5,
    carrier_id_secondary_6,
    carrier_id_secondary_7
} mac_lte_carrier_id;

typedef enum mac_lte_ce_mode {
    no_ce_mode = 0,
    ce_mode_a = 1,
    ce_mode_b = 2
} mac_lte_ce_mode;

typedef enum mac_lte_nb_mode {
    no_nb_mode = 0,
    nb_mode = 1
} mac_lte_nb_mode;

/* Context info attached to each LTE MAC frame */
typedef struct mac_lte_info
{
    /* Needed for decode */
    guint8          radioType;
    guint8          direction;
    guint8          rntiType;

    /* Extra info to display */
    guint16         rnti;
    guint16         ueid;

    /* Timing info */
    guint16         sysframeNumber;
    guint16         subframeNumber;
    gboolean        sfnSfInfoPresent;

    /* Optional field. More interesting for TDD (FDD is always -4 subframeNumber) */
    gboolean        subframeNumberOfGrantPresent;
    guint16         subframeNumberOfGrant;

    /* Flag set only if doing PHY-level data test - i.e. there may not be a
       well-formed MAC PDU so just show as raw data */
    gboolean        isPredefinedData;

    /* Length of DL PDU or UL grant size in bytes */
    guint16         length;

    /* 0=newTx, 1=first-retx, etc */
    guint8          reTxCount;
    guint8          isPHICHNACK; /* FALSE=PDCCH retx grant, TRUE=PHICH NACK */

    /* UL only.  Indicates if the R10 extendedBSR-Sizes parameter is set */
    gboolean        isExtendedBSRSizes;

    /* UL only.  Indicates if the R10 simultaneousPUCCH-PUSCH parameter is set for PCell */
    gboolean        isSimultPUCCHPUSCHPCell;

    /* UL only.  Indicates if the R10 extendedBSR-Sizes parameter is set for PSCell */
    gboolean        isSimultPUCCHPUSCHPSCell;

    /* Status of CRC check. For UE it is DL only. For eNodeB it is UL
       only. For an analyzer, it is present for both DL and UL. */
    gboolean        crcStatusValid;
    mac_lte_crc_status crcStatus;

    /* Carrier ID */
    mac_lte_carrier_id   carrierId;

    /* DL only.  Is this known to be a retransmission? */
    mac_lte_dl_retx dl_retx;

    /* DL only. CE mode to be used for RAR decoding */
    mac_lte_ce_mode ceMode;

    /* DL and UL. NB-IoT mode of the UE */
    mac_lte_nb_mode nbMode;

    /* UL only, for now used for CE mode A RAR decoding */
    guint8          nUlRb;

    /* More Physical layer info (see direction above for which side of union to use) */
    union {
        struct mac_lte_ul_phy_info
        {
            guint8 present;  /* Remaining UL fields are present and should be displayed */
            guint8 modulation_type;
            guint8 tbs_index;
            guint8 resource_block_length;
            guint8 resource_block_start;
            guint8 harq_id;
            gboolean ndi;
        } ul_info;
        struct mac_lte_dl_phy_info
        {
            guint8 present; /* Remaining DL fields are present and should be displayed */
            guint8 dci_format;
            guint8 resource_allocation_type;
            guint8 aggregation_level;
            guint8 mcs_index;
            guint8 redundancy_version_index;
            guint8 resource_block_length;
            guint8 harq_id;
            gboolean ndi;
            guint8   transport_block;  /* 0..1 */
        } dl_info;
    } detailed_phy_info;

    /* Relating to out-of-band events */
    /* N.B. dissector will only look to these fields if length is 0... */
    mac_lte_oob_event  oob_event;
    guint8             rapid;
    guint8             rach_attempt_number;
    #define MAX_SRs 20
    guint16            number_of_srs;
    guint16            oob_ueid[MAX_SRs];
    guint16            oob_rnti[MAX_SRs];
} mac_lte_info;

 /* 0 to 10 and 32 to 38 */
#define MAC_LTE_DATA_LCID_COUNT_MAX 18




/* Accessor function to check if a frame was considered to be ReTx */

/**********************************************************************/
/* UDP framing format                                                 */
/* -----------------------                                            */
/* Several people have asked about dissecting MAC by framing          */
/* PDUs over IP.  A suggested format over UDP has been created        */
/* and implemented by this dissector, using the definitions           */
/* below. A link to an example program showing you how to encode      */
/* these headers and send LTE MAC PDUs on a UDP socket is             */
/* provided at https://gitlab.com/wireshark/wireshark/-/wikis/MAC-LTE */
/*                                                                    */
/* A heuristic dissector (enabled by a preference) will               */
/* recognise a signature at the beginning of these frames.            */
/**********************************************************************/


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
