/* packet-mac-lte.h
 *
 * Martin Mathieson
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
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

#define WS_CS_RNTI     5

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

/* rlcMode */
#define RLC_TM_MODE 1
#define RLC_UM_MODE 2
#define RLC_AM_MODE 4
#define RLC_PREDEF  8

/* priority ? */

/* channelType */
#define CHANNEL_TYPE_CCCH 1
#define CHANNEL_TYPE_BCCH_BCH 2
#define CHANNEL_TYPE_PCCH 3
#define CHANNEL_TYPE_SRB 4
#define CHANNEL_TYPE_DRB 5
#define CHANNEL_TYPE_BCCH_DL_SCH 6
#define CHANNEL_TYPE_MCCH 7
#define CHANNEL_TYPE_MTCH 8

/* sequenceNumberLength */
#define UM_SN_LENGTH_5_BITS 5
#define UM_SN_LENGTH_10_BITS 10
#define AM_SN_LENGTH_10_BITS 10
#define AM_SN_LENGTH_16_BITS 16


typedef enum rlc_lte_nb_mode {
    rlc_no_nb_mode = 0,
    rlc_nb_mode = 1
} rlc_lte_nb_mode;


/* Info attached to each LTE RLC frame */
typedef struct rlc_lte_info
{
    guint8          rlcMode;
    guint8          direction;
    guint8          priority;
    guint8          sequenceNumberLength;
    guint16         ueid;
    guint16         channelType;
    guint16         channelId; /* for SRB: 1=SRB1, 2=SRB2, 3=SRB1bis; for DRB: DRB ID */
    guint16         pduLength;
    gboolean        extendedLiField;
    rlc_lte_nb_mode nbMode;
} rlc_lte_info;


typedef struct rlc_lte_tap_info {
    /* Info from context */
    guint8          rlcMode;
    guint8          direction;
    guint8          priority;
    guint16         ueid;
    guint16         channelType;
    guint16         channelId;
    guint16         pduLength;
    guint8          sequenceNumberLength;

    nstime_t        rlc_lte_time;
    guint8          loggedInMACFrame;
    guint16         sequenceNumber;
    guint8          isResegmented;
    guint8          isControlPDU;
    guint16         ACKNo;
    #define MAX_NACKs 128
    guint16         noOfNACKs;
    guint16         NACKs[MAX_NACKs];

    guint16         missingSNs;
} rlc_lte_tap_info;


/* Signature.  Rather than try to define a port for this, or make the
   port number a preference, frames will start with this string (with no
   terminating NULL */
#define RLC_LTE_START_STRING "rlc-lte"

/* Fixed field.  This is followed by the following 1 mandatory field:
   - rlcMode (1 byte)
   (where the allowed values are defined above */

/* Conditional field. This field is mandatory in case of RLC Unacknowledged mode.
   In case of RLC Acknowledged mode, the field is optional (assume 10 bits by default).
   The format is to have the tag, followed by the value (there is no length field,
   it's implicit from the tag). The allowed values are defined above. */

#define RLC_LTE_SN_LENGTH_TAG    0x02
/* 1 byte */

/* Optional fields. Attaching this info to frames will allow you
   to show you display/filter/plot/add-custom-columns on these fields, so should
   be added if available.
   The format is to have the tag, followed by the value (there is no length field,
   it's implicit from the tag) */

#define RLC_LTE_DIRECTION_TAG       0x03
/* 1 byte */

#define RLC_LTE_PRIORITY_TAG        0x04
/* 1 byte */

#define RLC_LTE_UEID_TAG            0x05
/* 2 bytes, network order */

#define RLC_LTE_CHANNEL_TYPE_TAG    0x06
/* 2 bytes, network order */

#define RLC_LTE_CHANNEL_ID_TAG      0x07
/* 2 bytes, network order */

#define RLC_LTE_EXT_LI_FIELD_TAG    0x08
/* 0 byte, tag presence indicates that AM DRB PDU is using an extended LI field of 15 bits */

#define RLC_LTE_NB_MODE_TAG         0x09
/* 1 byte containing rlc_lte_nb_mode enum value */

/* RLC PDU. Following this tag comes the actual RLC PDU (there is no length, the PDU
   continues until the end of the frame) */
#define RLC_LTE_PAYLOAD_TAG         0x01

enum pdcp_plane
{
    SIGNALING_PLANE = 1,
    USER_PLANE = 2
};

typedef enum LogicalChannelType
{
    Channel_DCCH=1,
    Channel_BCCH=2,
    Channel_CCCH=3,
    Channel_PCCH=4,
    Channel_DCCH_NB=5,
    Channel_BCCH_NB=6,
    Channel_CCCH_NB=7,
    Channel_PCCH_NB=8
} LogicalChannelType;

typedef enum
{
    BCH_TRANSPORT=1,
    DLSCH_TRANSPORT=2
} BCCHTransportType;

#define PDCP_SN_LENGTH_5_BITS  5
#define PDCP_SN_LENGTH_7_BITS  7
#define PDCP_SN_LENGTH_12_BITS 12
#define PDCP_SN_LENGTH_15_BITS 15
#define PDCP_SN_LENGTH_18_BITS 18

enum lte_security_integrity_algorithm_e { eia0, eia1, eia2, eia3 };
enum lte_security_ciphering_algorithm_e { eea0, eea1, eea2, eea3 };

typedef struct pdcp_lte_security_info_t
{
    guint32                                 configuration_frame;
    gboolean                                seen_next_ul_pdu;  /* i.e. have we seen SecurityModeResponse */
    enum lte_security_integrity_algorithm_e integrity;
    enum lte_security_ciphering_algorithm_e ciphering;

    /* Store previous settings so can revert if get SecurityModeFailure */
    guint32                                 previous_configuration_frame;
    enum lte_security_integrity_algorithm_e previous_integrity;
    enum lte_security_ciphering_algorithm_e previous_ciphering;
} pdcp_lte_security_info_t;


/***********************************************************************/
/* UDP framing format                                                  */
/* -----------------------                                             */
/* Several people have asked about dissecting PDCP by framing          */
/* PDUs over IP.  A suggested format over UDP has been defined         */
/* and implemented by this dissector, using the definitions            */
/* below. A link to an example program showing you how to encode       */
/* these headers and send LTE PDCP PDUs on a UDP socket is             */
/* provided at https://gitlab.com/wireshark/wireshark/-/wikis/PDCP-LTE */
/*                                                                     */
/* A heuristic dissecter (enabled by a preference) will                */
/* recognise a signature at the beginning of these frames.             */
/* Until someone is using this format, suggestions for changes         */
/* are welcome.                                                        */
/***********************************************************************/


/* Signature.  Rather than try to define a port for this, or make the
   port number a preference, frames will start with this string (with no
   terminating NULL */
#define PDCP_LTE_START_STRING "pdcp-lte"

/* Fixed fields.  This is followed by the following 3 mandatory fields:
   - no_header_pdu (1 byte)
   - plane (1 byte)
   - rohc_compression ( byte)
   (where the allowed values are defined above) */

/* Conditional field. This field is mandatory in case of User Plane PDCP PDU.
   The format is to have the tag, followed by the value (there is no length field,
   it's implicit from the tag). The allowed values are defined above. */

#define PDCP_LTE_SEQNUM_LENGTH_TAG          0x02
/* 1 byte */

/* Optional fields. Attaching this info to frames will allow you
   to show you display/filter/plot/add-custom-columns on these fields, so should
   be added if available.
   The format is to have the tag, followed by the value (there is no length field,
   it's implicit from the tag) */

#define PDCP_LTE_DIRECTION_TAG              0x03
/* 1 byte */

#define PDCP_LTE_LOG_CHAN_TYPE_TAG          0x04
/* 1 byte */

#define PDCP_LTE_BCCH_TRANSPORT_TYPE_TAG    0x05
/* 1 byte */

#define PDCP_LTE_ROHC_IP_VERSION_TAG        0x06
/* 2 bytes, network order */

#define PDCP_LTE_ROHC_CID_INC_INFO_TAG      0x07
/* 1 byte */

#define PDCP_LTE_ROHC_LARGE_CID_PRES_TAG    0x08
/* 1 byte */

#define PDCP_LTE_ROHC_MODE_TAG              0x09
/* 1 byte */

#define PDCP_LTE_ROHC_RND_TAG               0x0A
/* 1 byte */

#define PDCP_LTE_ROHC_UDP_CHECKSUM_PRES_TAG 0x0B
/* 1 byte */

#define PDCP_LTE_ROHC_PROFILE_TAG           0x0C
/* 2 bytes, network order */

#define PDCP_LTE_CHANNEL_ID_TAG             0x0D
/* 2 bytes, network order */

#define PDCP_LTE_UEID_TAG                   0x0E
/* 2 bytes, network order */

/* PDCP PDU. Following this tag comes the actual PDCP PDU (there is no length, the PDU
   continues until the end of the frame) */
#define PDCP_LTE_PAYLOAD_TAG                0x01



/* Called by RRC, or other configuration protocols */

/* Function to configure ciphering & integrity algorithms */
void set_pdcp_lte_security_algorithms(guint16 ueid, pdcp_lte_security_info_t *security_info);

/* Function to indicate securityModeCommand did not complete */
void set_pdcp_lte_security_algorithms_failed(guint16 ueid);


/* Called by external dissectors */
void set_pdcp_lte_rrc_ciphering_key(guint16 ueid, const char *key);
void set_pdcp_lte_rrc_integrity_key(guint16 ueid, const char *key);
void set_pdcp_lte_up_ciphering_key(guint16 ueid, const char *key);



/* Context info attached to each NR MAC frame */
typedef struct mac_nr_info
{
    /* Needed for decode */
    guint8          radioType;
    guint8          direction;
    guint8          rntiType;

    /* Extra info to display */
    guint16         rnti;
    guint16         ueid;
    guint8          harqid;

    /* Will these be included in the ME PHR report? */
    guint8          phr_type2_othercell;

    /* Timing info */
    gboolean        sfnSlotInfoPresent;
    guint16         sysframeNumber;
    guint16         slotNumber;

    /* Length of DL PDU or UL grant size in bytes */
    guint16         length;

} mac_nr_info;


/*****************************************************************/
/* UDP framing format                                            */
/* -----------------------                                       */
/* Several people have asked about dissecting MAC by framing     */
/* PDUs over IP.  A suggested format over UDP has been created   */
/* and implemented by this dissector, using the definitions      */
/* below.                                                        */
/*                                                               */
/* A heuristic dissector (enabled by a preference) will          */
/* recognise a signature at the beginning of these frames.       */
/*****************************************************************/


/* Signature.  Rather than try to define a port for this, or make the
   port number a preference, frames will start with this string (with no
   terminating NULL */
#define MAC_NR_START_STRING "mac-nr"

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

#define MAC_NR_RNTI_TAG                0x02
/* 2 bytes, network order */

#define MAC_NR_UEID_TAG                0x03
/* 2 bytes, network order */

#define MAC_NR_FRAME_SUBFRAME_TAG      0x04
/* 2 bytes, deprecated, do not use it */

#define MAC_NR_PHR_TYPE2_OTHERCELL_TAG 0x05
/* 1 byte, TRUE/FALSE */

#define MAC_NR_HARQID                  0x06
/* 1 byte */

#define MAC_NR_FRAME_SLOT_TAG          0x07
/* 4 bytes, network order, SFN is stored in the 2 first bytes and slot number in the 2 last bytes */

/* MAC PDU. Following this tag comes the actual MAC PDU (there is no length, the PDU
   continues until the end of the frame) */
#define MAC_NR_PAYLOAD_TAG             0x01


/* Type to store parameters for configuring LCID->RLC channel settings for DRB */
/* Some are optional, and may not be seen (e.g. on reestablishment) */
typedef struct nr_drb_mac_rlc_mapping_t
{
    gboolean   active;
    guint16    ueid;                /* Mandatory */
    guint8     drbid;               /* Mandatory */

    gboolean   lcid_present;
    guint8     lcid;                /* Part of LogicalChannelConfig - optional */
    gboolean   rlcMode_present;
    guint8     rlcMode;             /* Part of RLC config - optional */

    guint8     tempDirection;       /* So know direction of next SN length... */

    gboolean   rlcUlSnLength_present;
    guint8     rlcUlSnLength;        /* Part of RLC config - optional */
    gboolean   rlcDlSnLength_present;
    guint8     rlcDlSnLength;        /* Part of RLC config - optional */
} nr_drb_mac_rlc_mapping_t;

/* rlcMode */
#define RLC_TM_MODE 1
#define RLC_UM_MODE 2
#define RLC_AM_MODE 4

/* bearerType */
#define BEARER_TYPE_CCCH 1
#define BEARER_TYPE_BCCH_BCH 2
#define BEARER_TYPE_PCCH 3
#define BEARER_TYPE_SRB 4
#define BEARER_TYPE_DRB 5
#define BEARER_TYPE_BCCH_DL_SCH 6

/* sequenceNumberLength */
#define TM_SN_LENGTH_0_BITS  0
#define UM_SN_LENGTH_6_BITS  6
#define UM_SN_LENGTH_12_BITS 12
#define AM_SN_LENGTH_12_BITS 12
#define AM_SN_LENGTH_18_BITS 18

/* Info attached to each NR RLC frame */
typedef struct rlc_nr_info
{
    guint8          rlcMode;
    guint8          direction;
    guint8          sequenceNumberLength;
    guint8          bearerType;
    guint8          bearerId;
    guint16         ueid;
    guint16         pduLength;
} rlc_nr_info;

typedef struct nr_drb_rlc_pdcp_mapping_t
{
    gboolean   active;
    guint16    ueid;                /* Mandatory */
    guint8     drbid;               /* Mandatory */

    gboolean   pdcpUlSnLength_present;
    guint8     pdcpUlSnLength;        /* Part of PDCP config - optional */
    gboolean   pdcpDlSnLength_present;
    guint8     pdcpDlSnLength;        /* Part of PDCP config - optional */
    gboolean   pdcpUlSdap;
    gboolean   pdcpDlSdap;
    gboolean   pdcpIntegrityProtection;
    gboolean   pdcpCipheringDisabled;

} nr_drb_rlc_pdcp_mapping_t;

/* TODO: could probably merge this struct with above */
typedef struct pdcp_ue_parameters {
    guint32   id;
    guint8    pdcp_sn_bits_ul;
    guint8    pdcp_sn_bits_dl;
    gboolean  pdcp_sdap_ul;
    gboolean  pdcp_sdap_dl;
    gboolean  pdcp_integrity;
    gboolean  pdcp_ciphering_disabled;
} pdcp_bearer_parameters;

/*****************************************************************/
/* UDP framing format                                            */
/* -----------------------                                       */
/* Several people have asked about dissecting RLC by framing     */
/* PDUs over IP. A suggested format over UDP has been defined    */
/* and implemented by this dissector, using the definitions      */
/* below.                                                        */
/*                                                               */
/* A heuristic dissector (enabled by a preference) will          */
/* recognise a signature at the beginning of these frames.       */
/*****************************************************************/


/* Signature.  Rather than try to define a port for this, or make the
   port number a preference, frames will start with this string (with no
   terminating NULL */
#define RLC_NR_START_STRING "rlc-nr"

/* Fixed field. This is followed by the following 2 mandatory field:
   - rlcMode (1 byte)
   - sequenceNumberLength (1 byte)
   (where the allowed values are defined above) */

/* Optional fields. Attaching this info to frames will allow you
   to show you display/filter/plot/add-custom-columns on these fields, so should
   be added if available.
   The format is to have the tag, followed by the value (there is no length field,
   it's implicit from the tag) */

#define RLC_NR_DIRECTION_TAG       0x02
/* 1 byte */

#define RLC_NR_UEID_TAG            0x03
/* 2 bytes, network order */

#define RLC_NR_BEARER_TYPE_TAG     0x04
/* 1 byte */

#define RLC_NR_BEARER_ID_TAG       0x05
/* 1 byte */

/* RLC PDU. Following this tag comes the actual RLC PDU (there is no length, the PDU
   continues until the end of the frame) */
#define RLC_NR_PAYLOAD_TAG         0x01

enum pdcp_nr_plane
{
    NR_SIGNALING_PLANE = 1,
    NR_USER_PLANE = 2
};

typedef enum NRBearerType
{
    Bearer_DCCH=1,
    Bearer_BCCH_BCH=2,
    Bearer_BCCH_DL_SCH=3,
    Bearer_CCCH=4,
    Bearer_PCCH=5,
} NRBearerType;


#define PDCP_NR_SN_LENGTH_12_BITS 12
#define PDCP_NR_SN_LENGTH_18_BITS 18

#define PDCP_NR_UL_SDAP_HEADER_PRESENT 0x01
#define PDCP_NR_DL_SDAP_HEADER_PRESENT 0x02

enum nr_security_integrity_algorithm_e { nia0, nia1, nia2, nia3 };
enum nr_security_ciphering_algorithm_e { nea0, nea1, nea2, nea3, nea_disabled=999};

typedef struct pdcp_nr_security_info_t
{
    guint32                                configuration_frame;
    gboolean                               seen_next_ul_pdu;  /* i.e. have we seen SecurityModeResponse */
    enum nr_security_integrity_algorithm_e integrity;
    enum nr_security_ciphering_algorithm_e ciphering;

    /* Store previous settings so can revert if get SecurityModeFailure */
    guint32                                previous_configuration_frame;
    enum nr_security_integrity_algorithm_e previous_integrity;
    enum nr_security_ciphering_algorithm_e previous_ciphering;
} pdcp_nr_security_info_t;


/*****************************************************************/
/* UDP framing format                                            */
/* -----------------------                                       */
/* Several people have asked about dissecting PDCP by framing    */
/* PDUs over IP.  A suggested format over UDP has been defined   */
/* and implemented by this dissector, using the definitions      */
/* below.                                                        */
/*                                                               */
/* A heuristic dissector (enabled by a preference) will          */
/* recognise a signature at the beginning of these frames.       */
/* Until someone is using this format, suggestions for changes   */
/* are welcome.                                                  */
/*****************************************************************/


/* Signature.  Rather than try to define a port for this, or make the
   port number a preference, frames will start with this string (with no
   terminating NULL */
#define PDCP_NR_START_STRING "pdcp-nr"

/* Fixed fields:
   - plane (1 byte) */

/* Conditional field. This field is mandatory in case of User Plane PDCP PDU.
   The format is to have the tag, followed by the value (there is no length field,
   it's implicit from the tag). The allowed values are defined above. */

#define PDCP_NR_SEQNUM_LENGTH_TAG          0x02
/* 1 byte */

/* Optional fields. Attaching this info should be added if available.
   The format is to have the tag, followed by the value (there is no length field,
   it's implicit from the tag) */

#define PDCP_NR_DIRECTION_TAG              0x03
/* 1 byte */

#define PDCP_NR_BEARER_TYPE_TAG            0x04
/* 1 byte */

#define PDCP_NR_BEARER_ID_TAG              0x05
/* 1 byte */

#define PDCP_NR_UEID_TAG                   0x06
/* 2 bytes, network order */

#define PDCP_NR_ROHC_COMPRESSION_TAG       0x07
/* 0 byte */

/* N.B. The following ROHC values only have significance if rohc_compression
   is in use for the current channel */

#define PDCP_NR_ROHC_IP_VERSION_TAG        0x08
/* 1 byte */

#define PDCP_NR_ROHC_CID_INC_INFO_TAG      0x09
/* 0 byte */

#define PDCP_NR_ROHC_LARGE_CID_PRES_TAG    0x0A
/* 0 byte */

#define PDCP_NR_ROHC_MODE_TAG              0x0B
/* 1 byte */

#define PDCP_NR_ROHC_RND_TAG               0x0C
/* 0 byte */

#define PDCP_NR_ROHC_UDP_CHECKSUM_PRES_TAG 0x0D
/* 0 byte */

#define PDCP_NR_ROHC_PROFILE_TAG           0x0E
/* 2 bytes, network order */

#define PDCP_NR_MACI_PRES_TAG              0x0F
/* 0 byte */

#define PDCP_NR_SDAP_HEADER_TAG            0x10
/* 1 byte, bitmask with PDCP_NR_UL_SDAP_HEADER_PRESENT and/or PDCP_NR_DL_SDAP_HEADER_PRESENT */

#define PDCP_NR_CIPHER_DISABLED_TAG        0x11
/* 0 byte */

/* PDCP PDU. Following this tag comes the actual PDCP PDU (there is no length, the PDU
   continues until the end of the frame) */
#define PDCP_NR_PAYLOAD_TAG                0x01


/* Called by RRC, or other configuration protocols */

/* Function to configure ciphering & integrity algorithms */
void set_pdcp_nr_security_algorithms(guint16 ueid, pdcp_nr_security_info_t *security_info);

/* Function to indicate securityModeCommand did not complete */
void set_pdcp_nr_security_algorithms_failed(guint16 ueid);


/* Called by external dissectors */
void set_pdcp_nr_rrc_ciphering_key(guint16 ueid, const char *key);
void set_pdcp_nr_rrc_integrity_key(guint16 ueid, const char *key);
void set_pdcp_nr_up_ciphering_key(guint16 ueid, const char *key);
void set_pdcp_nr_up_integrity_key(guint16 ueid, const char *key);

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
#endif
