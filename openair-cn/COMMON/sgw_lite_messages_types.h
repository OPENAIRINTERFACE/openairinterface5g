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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/


/*! \file sgw_lite_messages_types.h
 * \brief S11 definitions for interaction between MME and S11
 * 3GPP TS 29.274.
 * Messages are the same as for GTPv2-C but here we abstract the UDP layer
 * \author Sebastien ROUX <sebastien.roux@eurecom.fr>
 * \date 2013
 * \version 0.1
 */

#include "../SGW-LITE/sgw_lite_ie_defs.h"

#ifndef SGW_LITE_MESSAGES_TYPES_H_
#define SGW_LITE_MESSAGES_TYPES_H_

#define SGW_CREATE_SESSION_REQUEST(mSGpTR)         (mSGpTR)->ittiMsg.sgwCreateSessionRequest
#define SGW_CREATE_SESSION_RESPONSE(mSGpTR)        (mSGpTR)->ittiMsg.sgwCreateSessionResponse
#define SGW_MODIFY_BEARER_REQUEST(mSGpTR)          (mSGpTR)->ittiMsg.sgwModifyBearerRequest
#define SGW_MODIFY_BEARER_RESPONSE(mSGpTR)         (mSGpTR)->ittiMsg.sgwModifyBearerResponse
#define SGW_DELETE_SESSION_REQUEST(mSGpTR)         (mSGpTR)->ittiMsg.sgwDeleteSessionRequest
#define SGW_DELETE_SESSION_RESPONSE(mSGpTR)        (mSGpTR)->ittiMsg.sgwDeleteSessionResponse
#define SGW_RELEASE_ACCESS_BEARERS_REQUEST(mSGpTR) (mSGpTR)->ittiMsg.sgwReleaseAccessBearersRequest
#define SGW_RELEASE_ACCESS_BEARERS_RESPONSE(mSGpTR) (mSGpTR)->ittiMsg.sgwReleaseAccessBearersResponse


/** @struct SgwCreateSessionRequest
 *  @brief Create Session Request
 *
 * Spec 3GPP TS 29.274, Universal Mobile Telecommunications System (UMTS);
 *                      LTE; 3GPP Evolved Packet System (EPS);
 *                      Evolved General Packet Radio Service (GPRS);
 *                      Tunnelling Protocol for Control plane (GTPv2-C); Stage 3
 * The Create Session Request will be sent on S11 interface as
 * part of these procedures:
 * - E-UTRAN Initial Attach
 * - UE requested PDN connectivity
 * - Tracking Area Update procedure with Serving GW change
 * - S1/X2-based handover with SGW change
 */
typedef struct SgwCreateSessionRequest_s {
  Teid_t             teid;                ///< S11- S-GW Tunnel Endpoint Identifier

  Imsi_t             imsi;                ///< The IMSI shall be included in the message on the S4/S11
  ///< interface, and on S5/S8 interface if provided by the
  ///< MME/SGSN, except for the case:
  ///<     - If the UE is emergency attached and the UE is UICCless.
  ///< The IMSI shall be included in the message on the S4/S11
  ///< interface, and on S5/S8 interface if provided by the
  ///< MME/SGSN, but not used as an identifier
  ///<     - if UE is emergency attached but IMSI is not authenticated.
  ///< The IMSI shall be included in the message on the S2b interface.

  Msisdn_t           msisdn;              ///< For an E-UTRAN Initial Attach the IE shall be included
  ///< when used on the S11 interface, if provided in the
  ///< subscription data from the HSS.
  ///< For a PDP Context Activation procedure the IE shall be
  ///< included when used on the S4 interface, if provided in the
  ///< subscription data from the HSS.
  ///< The IE shall be included for the case of a UE Requested
  ///< PDN Connectivity, if the MME has it stored for that UE.
  ///< It shall be included when used on the S5/S8 interfaces if
  ///< provided by the MME/SGSN.
  ///< The ePDG shall include this IE on the S2b interface during
  ///< an Attach with GTP on S2b and a UE initiated Connectivity
  ///< to Additional PDN with GTP on S2b, if provided by the
  ///< HSS/AAA.

  Mei_t              mei;                 ///< The MME/SGSN shall include the ME Identity (MEI) IE on
  ///< the S11/S4 interface:
  ///<     - If the UE is emergency attached and the UE is UICCless
  ///<     - If the UE is emergency attached and the IMSI is not authenticated
  ///< For all other cases the MME/SGSN shall include the ME
  ///< Identity (MEI) IE on the S11/S4 interface if it is available.
  ///< If the SGW receives this IE, it shall forward it to the PGW
  ///< on the S5/S8 interface.

  Uli_t              uli;                 ///< This IE shall be included on the S11 interface for E-
  ///< UTRAN Initial Attach and UE-requested PDN Connectivity
  ///< procedures. It shall include ECGI&TAI. The MME/SGSN
  ///< shall also include it on the S11/S4 interface for
  ///< TAU/RAU/X2-Handover/Enhanced SRNS Relocation
  ///< procedure if the PGW has requested location information
  ///< change reporting and MME/SGSN support location
  ///< information change reporting. The SGW shall include this
  ///< IE on S5/S8 if it receives the ULI from MME/SGSN.

  ServingNetwork_t   serving_network;     ///< This IE shall be included on the S4/S11, S5/S8 and S2b
  ///< interfaces for an E-UTRAN initial attach, a PDP Context
  ///< Activation, a UE requested PDN connectivity, an Attach
  ///< with GTP on S2b, a UE initiated Connectivity to Additional
  ///< PDN with GTP on S2b and a Handover to Untrusted Non-
  ///< 3GPP IP Access with GTP on S2b.

  rat_type_t         rat_type;            ///< This IE shall be set to the 3GPP access type or to the
  ///< value matching the characteristics of the non-3GPP access
  ///< the UE is using to attach to the EPS.
  ///< The ePDG may use the access technology type of the
  ///< untrusted non-3GPP access network if it is able to acquire
  ///< it; otherwise it shall indicate Virtual as the RAT Type.
  ///< See NOTE 3, NOTE 4.

  indication_flags_t indication_flags;    ///< This IE shall be included if any one of the applicable flags
  ///< is set to 1.
  ///< Applicable flags are:
  ///<     - S5/S8 Protocol Type: This flag shall be used on
  ///<       the S11/S4 interfaces and set according to the
  ///<       protocol chosen to be used on the S5/S8
  ///<       interfaces.
  ///<
  ///<     - Dual Address Bearer Flag: This flag shall be used
  ///<       on the S2b, S11/S4 and S5/S8 interfaces and shall
  ///<       be set to 1 when the PDN Type, determined based
  ///<       on UE request and subscription record, is set to
  ///<       IPv4v6 and all SGSNs which the UE may be
  ///<       handed over to support dual addressing. This shall
  ///<       be determined based on node pre-configuration by
  ///<       the operator.
  ///<
  ///<     - Handover Indication: This flag shall be set to 1 on
  ///<       the S11/S4 and S5/S8 interface during an E-
  ///<       UTRAN Initial Attach or a UE Requested PDN
  ///<       Connectivity or aPDP Context Activation procedure
  ///<       if the PDN connection/PDP Context is handed-over
  ///<       from non-3GPP access.
  ///<       This flag shall be set to 1 on the S2b interface
  ///<       during a Handover to Untrusted Non-3GPP IP
  ///<       Access with GTP on S2b and IP address
  ///<       preservation is requested by the UE.
  ///<
  ///<       ....
  ///<     - Unauthenticated IMSI: This flag shall be set to 1
  ///<       on the S4/S11 and S5/S8 interfaces if the IMSI
  ///<       present in the message is not authenticated and is
  ///<       for an emergency attached UE.

  FTeid_t            sender_fteid_for_cp; ///< Sender F-TEID for control plane (MME)

  FTeid_t            pgw_address_for_cp;  ///< PGW S5/S8 address for control plane or PMIP
  ///< This IE shall be sent on the S11 / S4 interfaces. The TEID
  ///< or GRE Key is set to "0" in the E-UTRAN initial attach, the
  ///< PDP Context Activation and the UE requested PDN
  ///< connectivity procedures.

  char               apn[APN_MAX_LENGTH + 1]; ///< Access Point Name

  SelectionMode_t    selection_mode;      ///< Selection Mode
  ///< This IE shall be included on the S4/S11 and S5/S8
  ///< interfaces for an E-UTRAN initial attach, a PDP Context
  ///< Activation and a UE requested PDN connectivity.
  ///< This IE shall be included on the S2b interface for an Initial
  ///< Attach with GTP on S2b and a UE initiated Connectivity to
  ///< Additional PDN with GTP on S2b.
  ///< It shall indicate whether a subscribed APN or a non
  ///< subscribed APN chosen by the MME/SGSN/ePDG was
  ///< selected.
  ///< CO: When available, this IE shall be sent by the MME/SGSN on
  ///< the S11/S4 interface during TAU/RAU/HO with SGW
  ///< relocation.

  pdn_type_t         pdn_type;            ///< PDN Type
  ///< This IE shall be included on the S4/S11 and S5/S8
  ///< interfaces for an E-UTRAN initial attach, a PDP Context
  ///< Activation and a UE requested PDN connectivity.
  ///< This IE shall be set to IPv4, IPv6 or IPv4v6. This is based
  ///< on the UE request and the subscription record retrieved
  ///< from the HSS (for MME see 3GPP TS 23.401 [3], clause
  ///< 5.3.1.1, and for SGSN see 3GPP TS 23.060 [35], clause
  ///< 9.2.1). See NOTE 1.

  PAA_t              paa;                 ///< PDN Address Allocation
  ///< This IE shall be included the S4/S11, S5/S8 and S2b
  ///< interfaces for an E-UTRAN initial attach, a PDP Context
  ///< Activation, a UE requested PDN connectivity, an Attach
  ///< with GTP on S2b, a UE initiated Connectivity to Additional
  ///< PDN with GTP on S2b and a Handover to Untrusted Non-
  ///< 3GPP IP Access with GTP on S2b. For PMIP-based
  ///< S5/S8, this IE shall also be included on the S4/S11
  ///< interfaces for TAU/RAU/Handover cases involving SGW
  ///< relocation.
  ///< The PDN type field in the PAA shall be set to IPv4, or IPv6
  ///< or IPv4v6 by MME, based on the UE request and the
  ///< subscription record retrieved from the HSS.
  ///< For static IP address assignment (for MME see 3GPP TS
  ///< 23.401 [3], clause 5.3.1.1, for SGSN see 3GPP TS 23.060
  ///< [35], clause 9.2.1, and for ePDG see 3GPP TS 23.402 [45]
  ///< subclause 4.7.3), the MME/SGSN/ePDG shall set the IPv4
  ///< address and/or IPv6 prefix length and IPv6 prefix and
  ///< Interface Identifier based on the subscribed values
  ///< received from HSS, if available. The value of PDN Type
  ///< field shall be consistent with the value of the PDN Type IE,
  ///< if present in this message.
  ///< For a Handover to Untrusted Non-3GPP IP Access with
  ///< GTP on S2b, the ePDG shall set the IPv4 address and/or
  ///< IPv6 prefix length and IPv6 prefix and Interface Identifier
  ///< based on the IP address(es) received from the UE.
  ///< If static IP address assignment is not used, and for
  ///< scenarios other than a Handover to Untrusted Non-3GPP
  ///< IP Access with GTP on S2b, the IPv4 address shall be set
  ///< to 0.0.0.0, and/or the IPv6 Prefix Length and IPv6 prefix
  ///< and Interface Identifier shall all be set to zero.
  ///<
  ///< CO: This IE shall be sent by the MME/SGSN on S11/S4
  ///< interface during TAU/RAU/HO with SGW relocation.

  // APN Restriction Maximum_APN_Restriction ///< This IE shall be included on the S4/S11 and S5/S8
  ///< interfaces in the E-UTRAN initial attach, PDP Context
  ///< Activation and UE Requested PDN connectivity
  ///< procedures.
  ///< This IE denotes the most stringent restriction as required
  ///< by any already active bearer context. If there are no
  ///< already active bearer contexts, this value is set to the least
  ///< restrictive type.

  ambr_t             ambr;                ///< Aggregate Maximum Bit Rate (APN-AMBR)
  ///< This IE represents the APN-AMBR. It shall be included on
  ///< the S4/S11, S5/S8 and S2b interfaces for an E-UTRAN
  ///< initial attach, UE requested PDN connectivity, the PDP
  ///< Context Activation procedure using S4, the PS mobility
  ///< from the Gn/Gp SGSN to the S4 SGSN/MME procedures,
  ///< Attach with GTP on S2b and a UE initiated Connectivity to
  ///< Additional PDN with GTP on S2b.

  // EBI Linked EPS Bearer ID             ///< This IE shall be included on S4/S11 in RAU/TAU/HO
  ///< except in the Gn/Gp SGSN to MME/S4-SGSN
  ///< RAU/TAU/HO procedures with SGW change to identify the
  ///< default bearer of the PDN Connection

  pco_flat_t         pco;                 /// PCO protocol_configuration_options
  ///< This IE is not applicable to TAU/RAU/Handover. If
  ///< MME/SGSN receives PCO from UE (during the attach
  ///< procedures), the MME/SGSN shall forward the PCO IE to
  ///< SGW. The SGW shall also forward it to PGW.

  bearer_to_create_t bearer_to_create;    ///< Bearer Contexts to be created
  ///< Several IEs with the same type and instance value shall be
  ///< included on the S4/S11 and S5/S8 interfaces as necessary
  ///< to represent a list of Bearers. One single IE shall be
  ///< included on the S2b interface.
  ///< One bearer shall be included for an E-UTRAN Initial
  ///< Attach, a PDP Context Activation, a UE requested PDN
  ///< Connectivity, an Attach with GTP on S2b, a UE initiated
  ///< Connectivity to Additional PDN with GTP on S2b and a
  ///< Handover to Untrusted Non-3GPP IP Access with GTP on
  ///< S2b.
  ///< One or more bearers shall be included for a
  ///< Handover/TAU/RAU with an SGW change.

  ///bearer_to_remove_t bearer_to_remove;    ///< This IE shall be included on the S4/S11 interfaces for the
  ///< TAU/RAU/Handover cases where any of the bearers
  ///< existing before the TAU/RAU/Handover procedure will be
  ///< deactivated as consequence of the TAU/RAU/Handover
  ///< procedure.
  ///< For each of those bearers, an IE with the same type and
  ///< instance value shall be included.

  // Trace Information trace_information  ///< This IE shall be included on the S4/S11 interface if an
  ///< SGW trace is activated, and/or on the S5/S8 and S2b
  ///< interfaces if a PGW trace is activated. See 3GPP TS
  ///< 32.422 [18].

  // Recovery Recovery                    ///< This IE shall be included on the S4/S11, S5/S8 and S2b
  ///< interfaces if contacting the peer for the first time

  FQ_CSID_t          mme_fq_csid;         ///< This IE shall be included by the MME on the S11 interface
  ///< and shall be forwarded by an SGW on the S5/S8 interfaces
  ///< according to the requirements in 3GPP TS 23.007 [17].

  FQ_CSID_t          sgw_fq_csid;         ///< This IE shall included by the SGW on the S5/S8 interfaces
  ///< according to the requirements in 3GPP TS 23.007 [17].

  //FQ_CSID_t          epdg_fq_csid;      ///< This IE shall be included by the ePDG on the S2b interface
  ///< according to the requirements in 3GPP TS 23.007 [17].

  UETimeZone_t       ue_time_zone;        ///< This IE shall be included by the MME over S11 during
  ///< Initial Attach, UE Requested PDN Connectivity procedure.
  ///< This IE shall be included by the SGSN over S4 during PDP
  ///< Context Activation procedure.
  ///< This IE shall be included by the MME/SGSN over S11/S4
  ///< TAU/RAU/Handover with SGW relocation.
  ///< C: If SGW receives this IE, SGW shall forward it to PGW
  ///< across S5/S8 interface.

  UCI_t              uci;                 ///< User CSG Information
  ///< CO This IE shall be included on the S4/S11 interface for E-
  ///< UTRAN Initial Attach, UE-requested PDN Connectivity and
  ///< PDP Context Activation using S4 procedures if the UE is
  ///< accessed via CSG cell or hybrid cell. The MME/SGSN
  ///< shall also include it for TAU/RAU/Handover procedures if
  ///< the PGW has requested CSG info reporting and
  ///< MME/SGSN support CSG info reporting. The SGW shall
  ///< include this IE on S5/S8 if it receives the User CSG
  ///< information from MME/SGSN.

  // Charging Characteristics
  // MME/S4-SGSN LDN
  // SGW LDN
  // ePDG LDN
  // Signalling Priority Indication
  // MMBR Max MBR/APN-AMBR
  // Private Extension

  /* S11 stack specific parameter. Not used in standalone epc mode */
  void              *trxn;                ///< Transaction identifier
  uint32_t           peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  uint16_t           peer_port;           ///< MME port for S-GW or S-GW port for MME
} SgwCreateSessionRequest;

/** @struct SgwCreateSessionResponse
 *  @brief Create Session Response
 *
 * The Create Session Response will be sent on S11 interface as
 * part of these procedures:
 * - E-UTRAN Initial Attach
 * - UE requested PDN connectivity
 * - Tracking Area Update procedure with SGW change
 * - S1/X2-based handover with SGW change
 */
typedef struct SgwCreateSessionResponse_s {
  Teid_t                   teid;                ///< Tunnel Endpoint Identifier

  // here fields listed in 3GPP TS 29.274
  SGWCause_t               cause;               ///< If the SGW cannot accept any of the "Bearer Context Created" IEs within Create Session Request
  ///< message, the SGW shall send the Create Session Response with appropriate reject Cause value.

  // change_reporting_action                    ///< This IE shall be included on the S5/S8 and S4/S11
  ///< interfaces with the appropriate Action field if the location
  ///< Change Reporting mechanism is to be started or stopped
  ///< for this subscriber in the SGSN/MME.

  // csg_Information_reporting_action           ///< This IE shall be included on the S5/S8 and S4/S11
  ///< interfaces with the appropriate Action field if the CSG Info
  ///< reporting mechanism is to be started or stopped for this
  ///< subscriber in the SGSN/MME.

  FTeid_t                  s11_sgw_teid;        ///< Sender F-TEID for control plane
  ///< This IE shall be sent on the S11/S4 interfaces. For the
  ///< S5/S8/S2b interfaces it is not needed because its content
  ///< would be identical to the IE PGW S5/S8/S2b F-TEID for
  ///< PMIP based interface or for GTP based Control Plane
  ///< interface.

  FTeid_t                  s5_s8_pgw_teid;      ///< PGW S5/S8/S2b F-TEID for PMIP based interface or for GTP based Control Planeinterface
  ///< PGW shall include this IE on the S5/S8 interfaces during
  ///< the Initial Attach, UE requested PDN connectivity and PDP
  ///< Context Activation procedures.
  ///< If SGW receives this IE it shall forward the IE to MME/S4-
  ///< SGSN on S11/S4 interface.
  ///< This IE shall include the TEID in the GTP based S5/S8
  ///< case and the GRE key in the PMIP based S5/S8 case.
  ///< In PMIP based S5/S8 case, same IP address is used for
  ///< both control plane and the user plane communication.
  ///<
  ///< PGW shall include this IE on the S2b interface during the
  ///< Attach with GTP on S2b, UE initiated Connectivity to
  ///< Additional PDN with GTP on S2b and Handover to
  ///< Untrusted Non-3GPP IP Access with GTP on S2b
  ///< procedures.


  PAA_t                    paa;                 ///< PDN Address Allocation
  ///< This IE shall be included on the S5/S8, S4/S11 and S2b
  ///< interfaces for the E-UTRAN initial attach, PDP Context
  ///< Activation, UE requested PDN connectivity, Attach with
  ///< GTP on S2b, UE initiated Connectivity to Additional PDN
  ///< with GTP on S2b and Handover to Untrusted Non-3GPP IP
  ///< Access with GTP on S2b procedures.
  ///< The PDN type field in the PAA shall be set to IPv4, or IPv6
  ///< or IPv4v6 by the PGW.
  ///< For the interfaces other than S2b, if the DHCPv4 is used
  ///< for IPv4 address allocation, the IPv4 address field shall be
  ///< set to 0.0.0.0.

  APNRestriction_t         apn_restriction;     ///< This IE shall be included on the S5/S8 and S4/S11
  ///< interfaces in the E-UTRAN initial attach, PDP Context
  ///< Activation and UE Requested PDN connectivity
  ///< procedures.
  ///< This IE shall also be included on S4/S11 during the Gn/Gp
  ///< SGSN to S4 SGSN/MME RAU/TAU procedures.
  ///< This IE denotes the restriction on the combination of types
  ///< of APN for the APN associated with this EPS bearer
  ///< Context.

  ambr_t             ambr;                      ///< Aggregate Maximum Bit Rate (APN-AMBR)
  ///< This IE represents the APN-AMBR. It shall be included on
  ///< the S5/S8, S4/S11 and S2b interfaces if the received APN-
  ///< AMBR has been modified by the PCRF.

  // EBI Linked EPS Bearer ID                   ///< This IE shall be sent on the S4/S11 interfaces during
  ///< Gn/Gp SGSN to S4-SGSN/MME RAU/TAU procedure to
  ///< identify the default bearer the PGW selects for the PDN
  ///< Connection.

  pco_flat_t         pco;// PCO protocol_configuration_options
  ///< This IE is not applicable for TAU/RAU/Handover. If PGW
  ///< decides to return PCO to the UE, PGW shall send PCO to
  ///< SGW. If SGW receives the PCO IE, SGW shall forward it
  ///< MME/SGSN.

  bearer_context_created_t bearer_context_created;///< EPS bearers corresponding to Bearer Contexts sent in
  ///< request message. Several IEs with the same type and
  ///< instance value may be included on the S5/S8 and S4/S11
  ///< as necessary to represent a list of Bearers. One single IE
  ///< shall be included on the S2b interface.
  ///< One bearer shall be included for E-UTRAN Initial Attach,
  ///< PDP Context Activation or UE Requested PDN
  ///< Connectivity , Attach with GTP on S2b, UE initiated
  ///< Connectivity to Additional PDN with GTP on S2b, and
  ///< Handover to Untrusted Non-3GPP IP Access with GTP on
  ///< S2b.
  ///< One or more created bearers shall be included for a
  ///< Handover/TAU/RAU with an SGW change. See NOTE 2.

  // Bearer_Context bearer_contexts_marked_for_removal ///< EPS bearers corresponding to Bearer Contexts to be
  ///< removed that were sent in the Create Session Request
  ///< message.
  ///< For each of those bearers an IE with the same type and
  ///< instance value shall be included on the S4/S11 interfaces.

  // Recovery Recovery                          ///< This IE shall be included on the S4/S11, S5/S8 and S2b
  ///< interfaces if contacting the peer for the first time

  // FQDN charging_Gateway_name                 ///< When Charging Gateway Function (CGF) Address is
  ///< configured, the PGW shall include this IE on the S5
  ///< interface.
  ///< NOTE 1: Both Charging Gateway Name and Charging Gateway Address shall not be included at the same
  ///< time. When both are available, the operator configures a preferred value.

  // IP Address charging_Gateway_address        ///< When Charging Gateway Function (CGF) Address is
  ///< configured, the PGW shall include this IE on the S5
  ///< interface. See NOTE 1.


  FQ_CSID_t                pgw_fq_csid;         ///< This IE shall be included by the PGW on the S5/S8 and
  ///< S2b interfaces and, when received from S5/S8 be
  ///< forwarded by the SGW on the S11 interface according to
  ///< the requirements in 3GPP TS 23.007 [17].

  FQ_CSID_t                sgw_fq_csid;         ///< This IE shall be included by the SGW on the S11 interface
  ///< according to the requirements in 3GPP TS 23.007 [17].

  // Local Distinguished Name (LDN) SGW LDN     ///< This IE is optionally sent by the SGW to the MME/SGSN
  ///< on the S11/S4 interfaces (see 3GPP TS 32.423 [44]),
  ///< when contacting the peer node for the first time.
  ///< Also:
  ///< This IE is optionally sent by the SGW to the MME/SGSN
  ///< on the S11/S4 interfaces (see 3GPP TS 32.423 [44]),
  ///< when communicating the LDN to the peer node for the first
  ///< time.

  // Local Distinguished Name (LDN) PGW LDN     ///< This IE is optionally included by the PGW on the S5/S8
  ///< and S2b interfaces (see 3GPP TS 32.423 [44]), when
  ///< contacting the peer node for the first time.
  ///< Also:
  ///< This IE is optionally included by the PGW on the S5/S8
  ///< interfaces (see 3GPP TS 32.423 [44]), when
  ///< communicating the LDN to the peer node for the first time.

  // EPC_Timer pgw_back_off_time                ///< This IE may be included on the S5/S8 and S4/S11
  ///< interfaces when the PDN GW rejects the Create Session
  ///< Request with the cause "APN congestion". It indicates the
  ///< time during which the MME or S4-SGSN should refrain
  ///< from sending subsequent PDN connection establishment
  ///< requests to the PGW for the congested APN for services
  ///< other than Service Users/emergency services.
  ///< See NOTE 3:
  ///< The last received value of the PGW Back-Off Time IE shall supersede any previous values received
  ///< from that PGW and for this APN in the MME/SGSN.

  // Private Extension                          ///< This IE may be sent on the S5/S8, S4/S11 and S2b
  ///< interfaces.

  /* S11 stack specific parameter. Not used in standalone epc mode */
  void                    *trxn;               ///< Transaction identifier
  uint32_t                 peer_ip;            ///< MME ipv4 address
} SgwCreateSessionResponse;

/** @struct SgwModifyBearerRequest
 *  @brief Modify Bearer Request
 *
 * The Modify Bearer Request will be sent on S11 interface as
 * part of these procedures:
 * - E-UTRAN Tracking Area Update without SGW Change
 * - UE triggered Service Request
 * - S1-based Handover
 * - E-UTRAN Initial Attach
 * - UE requested PDN connectivity
 * - X2-based handover without SGWrelocation
 */
typedef struct SgwModifyBearerRequest_s {
  Teid_t                     teid;             ///< S11 SGW Tunnel Endpoint Identifier

  // MEI                    ME Identity (MEI)  ///< C:This IE shall be sent on the S5/S8 interfaces for the Gn/Gp
  ///< SGSN to MME TAU.

  Uli_t                      uli;              ///< C: The MME/SGSN shall include this IE for
  ///< TAU/RAU/Handover procedures if the PGW has requested
  ///< location information change reporting and MME/SGSN
  ///< support location information change reporting.
  ///< An MME/SGSN which supports location information
  ///< change shall include this IE for UE-initiated Service
  ///< Request procedure if the PGW has requested location
  ///< information change reporting and the UE’s location info
  ///< has changed.
  ///< The SGW shall include this IE on S5/S8 if it receives the
  ///< ULI from MME/SGSN.
  ///< CO:This IE shall also be included on the S4/S11 interface for a
  ///< TAU/RAU/Handover with MME/SGSN change without
  ///< SGW change procedure, if the level of support (User
  ///< Location Change Reporting and/or CSG Information
  ///< Change Reporting) changes the MME shall include the
  ///< ECGI/TAI in the ULI, the SGSN shall include the CGI/SAI
  ///< in the ULI.
  ///< The SGW shall include this IE on S5/S8 if it receives the
  ///< ULI from MME/SGSN.

  ServingNetwork_t           serving_network;  ///< CO:This IE shall be included on S11/S4 interface during the
  ///< following procedures:
  ///< - TAU/RAU/handover if Serving Network is changed.
  ///< - TAU/RAU when the UE was ISR activated which is
  ///<   indicated by ISRAU flag.
  ///< - UE triggered Service Request when UE is ISR
  ///<   activated.
  ///< - UE initiated Service Request if ISR is not active, but
  ///<   the Serving Network has changed during previous
  ///<   mobility procedures, i.e. intra MME/S4-SGSN
  ///<   TAU/RAU and the change has not been reported to
  ///<   the PGW yet.
  ///< - TAU/RAU procedure as part of the optional network
  ///<   triggered service restoration procedure with ISR, as
  ///<   specified by 3GPP TS 23.007 [17].
  ///<
  ///< CO:This IE shall be included on S5/S8 if the SGW receives this
  ///< IE from MME/SGSN and if ISR is not active.
  ///< This IE shall be included on S5/S8 if the SGW receives this
  ///< IE from MME/SGSN and ISR is active and the Modify
  ///< Bearer Request message needs to be sent to the PGW as
  ///< specified in the 3GPP TS 23.401 [3].

  rat_type_t                 rat_type;         ///< C: This IE shall be sent on the S11 interface for a TAU with
  ///< an SGSN interaction, UE triggered Service Request or an I-
  ///< RAT Handover.
  ///< This IE shall be sent on the S4 interface for a RAU with
  ///< MME interaction, a RAU with an SGSN change, a UE
  ///< Initiated Service Request or an I-RAT Handover.
  ///< This IE shall be sent on the S5/S8 interface if the RAT type
  ///< changes.
  ///< CO: CO If SGW receives this IE from MME/SGSN during a
  ///< TAU/RAU/Handover with SGW change procedure, the
  ///< SGW shall forward it across S5/S8 interface to PGW.
  ///< CO: The IE shall be sent on the S11/S4 interface during the
  ///< following procedures:
  ///< - an inter MM TAU or inter SGSN RAU when UE was
  ///<   ISR activated which is indicated by ISRAU flag.
  ///< - TAU/RAU procedure as part of optional network
  ///<   triggered service restoration procedure with ISR, as
  ///<   specified by 3GPP TS 23.007 [17].
  ///< If ISR is active, this IE shall also be included on the S11
  ///< interface in the S1-U GTP-U tunnel setup procedure during
  ///< an intra-MME intra-SGW TAU procedure.

  indication_flags_t         indication_flags; ///< C:This IE shall be included if any one of the applicable flags
  ///< is set to 1.
  ///< Applicable flags are:
  ///< -ISRAI: This flag shall be used on S4/S11 interface
  ///<   and set to 1 if the ISR is established between the
  ///<   MME and the S4 SGSN.
  ///< - Handover Indication: This flag shall be set to 1 on
  ///<   the S4/S11 and S5/S8 interfaces during an E-
  ///<   UTRAN Initial Attach or for a UE Requested PDN
  ///<   Connectivity or a PDP Context Activation
  ///<   procedure, if the PDN connection/PDP context is
  ///<   handed-over from non-3GPP access.
  ///< - Direct Tunnel Flag: This flag shall be used on the
  ///<   S4 interface and set to 1 if Direct Tunnel is used.
  ///< - Change Reporting support Indication: shall be
  ///<   used on S4/S11, S5/S8 and set if the SGSN/MME
  ///<   supports location Info Change Reporting. This flag
  ///<   should be ignored by SGW if no message is sent
  ///<   on S5/S8. See NOTE 4.
  ///< - CSG Change Reporting Support Indication: shall
  ///<   be used on S4/S11, S5/S8 and set if the
  ///<   SGSN/MME supports CSG Information Change
  ///<   Reporting. This flag shall be ignored by SGW if no
  ///<   message is sent on S5/S8. See NOTE 4.
  ///< - Change F-TEID support Indication: This flag shall
  ///<   be used on S4/S11 for an IDLE state UE initiated
  ///<   TAU/RAU procedure and set to 1 to allow the
  ///<   SGW changing the GTP-U F-TEID.

  FTeid_t                  sender_fteid_for_cp; ///< C: Sender F-TEID for control plane
  ///< This IE shall be sent on the S11 and S4 interfaces for a
  ///< TAU/RAU/ Handover with MME/SGSN change and without
  ///< any SGW change.
  ///< This IE shall be sent on the S5 and S8 interfaces for a
  ///< TAU/RAU/Handover with a SGW change.

  ambr_t                   apn_ambr;            ///< C: Aggregate Maximum Bit Rate (APN-AMBR)
  ///< The APN-AMBR shall be sent for the PS mobility from the
  ///< Gn/Gp SGSN to the S4 SGSN/MME procedures..

  /* Delay Value in integer multiples of 50 millisecs, or zero */
  DelayValue_t               delay_dl_packet_notif_req; ///<C:This IE shall be sent on the S11 interface for a UE
  ///< triggered Service Request.
  ///< CO: This IE shall be sent on the S4 interface for a UE triggered
  ///< Service Request.

  bearer_context_to_modify_t bearer_context_to_modify;///< C: This IE shall be sent on the S4/S11 interface and S5/S8
  ///< interface except on the S5/S8 interface for a UE triggered
  ///< Service Request.
  ///< When Handover Indication flag is set to 1 (i.e., for
  ///< EUTRAN Initial Attach or UE Requested PDN Connectivity
  ///< when the UE comes from non-3GPP access), the PGW
  ///< shall ignore this IE. See NOTE 1.
  ///< Several IEs with the same type and instance value may be
  ///< included as necessary to represent a list of Bearers to be
  ///< modified.
  ///< During a TAU/RAU/Handover procedure with an SGW
  ///< change, the SGW includes all bearers it received from the
  ///< MME/SGSN (Bearer Contexts to be created, or Bearer
  ///< Contexts to be modified and also Bearer Contexts to be
  ///< removed) into the list of 'Bearer Contexts to be modified'
  ///< IEs, which are then sent on the S5/S8 interface to the
  ///< PGW (see NOTE 2).

  // Bearer Context   Bearer Contexts to be removed; ///< C: This IE shall be included on the S4 and S11 interfaces for
  ///< the TAU/RAU/Handover and Service Request procedures
  ///< where any of the bearers existing before the
  ///< TAU/RAU/Handover procedure and Service Request
  ///< procedures will be deactivated as consequence of the
  ///< TAU/RAU/Handover procedure and Service Request
  ///< procedures. (NOTE 3)
  ///< For each of those bearers, an IE with the same type and
  ///< instance value, shall be included.

  // recovery_t(restart counter) recovery;      ///< C: This IE shall be included if contacting the peer for the first
  ///< time.

  UETimeZone_t               ue_time_zone;      ///< CO: This IE shall be included by the MME/SGSN on the S11/S4
  ///< interfaces if the UE Time Zone has changed in the case of
  ///< TAU/RAU/Handover.
  ///< C: If SGW receives this IE, SGW shall forward it to PGW
  ///< across S5/S8 interface.

  FQ_CSID_t                  mme_fq_csid;       ///< C: This IE shall be included by MME on S11 and shall be
  ///< forwarded by SGW on S5/S8 according to the
  ///< requirements in 3GPP TS 23.007 [17].

  FQ_CSID_t                  sgw_fq_csid;       ///< C: This IE shall be included by SGW on S5/S8 according to
  ///< the requirements in 3GPP TS 23.007 [17].

  UCI_t                      uci;               ///< CO: The MME/SGSN shall include this IE for
  ///< TAU/RAU/Handover procedures and UE-initiated Service
  ///< Request procedure if the PGW has requested CSG Info
  ///< reporting and the MME/SGSN support the CSG
  ///< information reporting. The SGW shall include this IE on
  ///< S5/S8 if it receives the User CSG Information from
  ///< MME/SGSN.

  // Local Distinguished Name (LDN) MME/S4-SGSN LDN ///< O: This IE is optionally sent by the MME to the SGW on the
  ///< S11 interface and by the SGSN to the SGW on the S4
  ///< interface (see 3GPP TS 32.423 [44]), when communicating
  ///< the LDN to the peer node for the first time.

  // Local Distinguished Name (LDN) SGW LDN     ///< O: This IE is optionally sent by the SGW to the PGW on the
  ///< S5/S8 interfaces (see 3GPP TS 32.423 [44]), for inter-
  ///< SGW mobity, when communicating the LDN to the peer
  ///< node for the first time.

  // MMBR           Max MBR/APN-AMBR            ///< CO: If the S4-SGSN supports Max MBR/APN-AMBR, this IE
  ///< shall be included by the S4-SGSN over S4 interface in the
  ///< following cases:
  ///< - during inter SGSN RAU/SRNS relocation without
  ///<   SGW relocation and inter SGSN SRNS relocation
  ///<   with SGW relocation if Higher bitrates than
  ///<   16 Mbps flag is not included in the MM Context IE
  ///<   in the Context Response message or in the MM
  ///<   Context IE in the Forward Relocation Request
  ///<   message from the old S4-SGSN, while it is
  ///<   received from target RNC or a local Max
  ///<   MBR/APN-AMBR is configured based on
  ///<   operator's policy.
  ///<   - during Service Request procedure if Higher
  ///<   bitrates than 16 Mbps flag is received but the S4-
  ///<   SGSN has not received it before from an old RNC
  ///<   or the S4-SGSN has not updated the Max
  ///<   MBR/APN-AMBR to the PGW yet.
  ///< If SGW receives this IE, SGW shall forward it to PGW
  ///< across S5/S8 interface.

  // Private Extension   Private Extension

  /* S11 stack specific parameter. Not used in standalone epc mode */
  void                      *trxn;                        ///< Transaction identifier
} SgwModifyBearerRequest;

/** @struct SgwModifyBearerResponse
 *  @brief Modify Bearer Response
 *
 * The Modify Bearer Response will be sent on S11 interface as
 * part of these procedures:
 * - E-UTRAN Tracking Area Update without SGW Change
 * - UE triggered Service Request
 * - S1-based Handover
 * - E-UTRAN Initial Attach
 * - UE requested PDN connectivity
 * - X2-based handover without SGWrelocation
 */
typedef struct SgwModifyBearerResponse_s {
  Teid_t                   teid;                ///< S11 MME Tunnel Endpoint Identifier

  // here fields listed in 3GPP TS 29.274
  SGWCause_t               cause;               ///<

  ebi_t                    linked_eps_bearer_id;///< This IE shall be sent on S5/S8 when the UE moves from a
  ///< Gn/Gp SGSN to the S4 SGSN or MME to identify the
  ///< default bearer the PGW selects for the PDN Connection.
  ///< This IE shall also be sent by SGW on S11, S4 during
  ///< Gn/Gp SGSN to S4-SGSN/MME HO procedures to identify
  ///< the default bearer the PGW selects for the PDN
  ///< Connection.

  ambr_t                   apn_ambr;            ///< Aggregate Maximum Bit Rate (APN-AMBR)
  ///< This IE shall be included in the PS mobility from Gn/Gp
  ///< SGSN to the S4 SGSN/MME procedures if the received
  ///< APN-AMBR has been modified by the PCRF.

  APNRestriction_t         apn_restriction;     ///< This IE denotes the restriction on the combination of types
  ///< of APN for the APN associated with this EPS bearer
  ///< Context. This IE shall be included over S5/S8 interfaces,
  ///< and shall be forwarded over S11/S4 interfaces during
  ///< Gn/Gp SGSN to MME/S4-SGSN handover procedures.
  ///< This IE shall also be included on S5/S8 interfaces during
  ///< the Gn/Gp SGSN to S4 SGSN/MME RAU/TAU
  ///< procedures.
  ///< The target MME or SGSN determines the Maximum APN
  ///< Restriction using the APN Restriction.
  // PCO protocol_configuration_options         ///< If SGW receives this IE from PGW on GTP or PMIP based
  ///< S5/S8, the SGW shall forward PCO to MME/S4-SGSN
  ///< during Inter RAT handover from the UTRAN or from the
  ///< GERAN to the E-UTRAN. See NOTE 2:
  ///< If MME receives the IE, but no NAS message is sent, MME discards the IE.

#define MODIFY_BEARER_RESPONSE_MOD  0x0
#define MODIFY_BEARER_RESPONSE_REM  0x1
  unsigned                 bearer_present:1;    ///< Choice present in union choice
  union {
    bearer_context_modified_t bearer_contexts_modified;///< EPS bearers corresponding to Bearer Contexts to be
    ///< modified that were sent in Modify Bearer Request
    ///< message. Several IEs with the same type and instance
    ///< value may be included as necessary to represent a list of
    ///< the Bearers which are modified.
    bearer_for_removal_t      bearer_for_removal;///< EPS bearers corresponding to Bearer Contexts to be
    ///< removed sent in the Modify Bearer Request message.
    ///< Shall be included if request message contained Bearer
    ///< Contexts to be removed.
    ///< For each of those bearers an IE with the same type and
    ///< instance value shall be included.
  } bearer_choice;

  // change_reporting_action                    ///< This IE shall be included with the appropriate Action field If
  ///< the location Change Reporting mechanism is to be started
  ///< or stopped for this subscriber in the SGSN/MME.

  // csg_Information_reporting_action           ///< This IE shall be included with the appropriate Action field if
  ///< the location CSG Info change reporting mechanism is to be
  ///< started or stopped for this subscriber in the SGSN/MME.

  // FQDN Charging Gateway Name                 ///< When Charging Gateway Function (CGF) Address is
  ///< configured, the PGW shall include this IE on the S5
  ///< interface during SGW relocation and when the UE moves
  ///< from Gn/Gp SGSN to S4-SGSN/MME. See NOTE 1:
  ///< Both Charging Gateway Name and Charging Gateway Address shall not be included at the same
  ///< time. When both are available, the operator configures a preferred value.

  // IP Address Charging Gateway Address        ///< When Charging Gateway Function (CGF) Address is
  ///< configured, the PGW shall include this IE on the S5
  ///< interface during SGW relocation and when the UE moves
  ///< from Gn/Gp SGSN to S4-SGSN/MME. See NOTE 1:
  ///< Both Charging Gateway Name and Charging Gateway Address shall not be included at the same
  ///< time. When both are available, the operator configures a preferred value.

  FQ_CSID_t                pgw_fq_csid;         ///< This IE shall be included by PGW on S5/S8and shall be
  ///< forwarded by SGW on S11 according to the requirements
  ///< in 3GPP TS 23.007 [17].

  FQ_CSID_t                sgw_fq_csid;         ///< This IE shall be included by SGW on S11 according to the
  ///< requirements in 3GPP TS 23.007 [17].

  // recovery_t(restart counter) recovery;      ///< This IE shall be included if contacting the peer for the first
  ///< time.

  // Local Distinguished Name (LDN) SGW LDN     ///< This IE is optionally sent by the SGW to the MME/SGSN
  ///< on the S11/S4 interfaces (see 3GPP TS 32.423 [44]),
  ///< when contacting the peer node for the first time.

  // Local Distinguished Name (LDN) PGW LDN     ///< This IE is optionally included by the PGW on the S5/S8
  ///< and S2b interfaces (see 3GPP TS 32.423 [44]), when
  ///< contacting the peer node for the first time.

  // Private Extension Private Extension        ///< optional

  /* S11 stack specific parameter. Not used in standalone epc mode */
  void                         *trxn;                      ///< Transaction identifier
} SgwModifyBearerResponse;

typedef struct SgwDeleteSessionRequest_s {
  Teid_t      teid;                   ///< Tunnel Endpoint Identifier
  EBI_t       lbi;                    ///< Linked EPS Bearer ID
  FTeid_t     sender_fteid_for_cp;    ///< Sender F-TEID for control plane

  /* Operation Indication: This flag shall be set over S4/S11 interface
   * if the SGW needs to forward the Delete Session Request message to
   * the PGW. This flag shall not be set if the ISR associated GTP
   * entity sends this message to the SGW in the Detach procedure.
   * This flag shall also not be set to 1 in the SRNS Relocation Cancel
   * Using S4 (6.9.2.2.4a in 3GPP TS 23.060 [4]), Inter RAT handover
   * Cancel procedure with SGW change TAU with Serving GW change,
   * Gn/Gb based RAU (see 5.5.2.5, 5.3.3.1, D.3.5 in 3GPP TS 23.401 [3],
   * respectively), S1 Based handover Cancel procedure with SGW change.
   */
  indication_flags_t indication_flags;

  /* GTPv2-C specific parameters */
  void       *trxn;
  uint32_t    peer_ip;
} SgwDeleteSessionRequest;

/** @struct SgwDeleteSessionResponse
 *  @brief Delete Session Response
 *
 * The Delete Session Response will be sent on S11 interface as
 * part of these procedures:
 * - EUTRAN Initial Attach
 * - UE, HSS or MME Initiated Detach
 * - UE or MME Requested PDN Disconnection
 * - Tracking Area Update with SGW Change
 * - S1 Based Handover with SGW Change
 * - X2 Based Handover with SGW Relocation
 * - S1 Based handover cancel with SGW change
 */
typedef struct SgwDeleteSessionResponse_s {
  Teid_t      teid;                   ///< Remote Tunnel Endpoint Identifier
  SGWCause_t  cause;

  /* GTPv2-C specific parameters */
  void       *trxn;
  uint32_t    peer_ip;
} SgwDeleteSessionResponse;

/** @struct SgwReleaseAccessBearersRequest
 *  @brief Release AccessBearers Request
 *
 * The Release Access Bearers Request message shall sent on the S11 interface by
 * the MME to the SGW as part of the S1 release procedure.
 * The message shall also be sent on the S4 interface by the SGSN to the SGW as
 * part of the procedures:
 * -    RAB release using S4
 * -    Iu Release using S4
 * -    READY to STANDBY transition within the network
 */
typedef struct SgwReleaseAccessBearersRequest_s {
	Teid_t     teid;                     ///< Tunnel Endpoint Identifier
	uint32_t   num_rabs;
	EBI_t      list_of_rabs[8]  ;        ///< Shall be present on S4 interface when this message is
                                         ///< used to release a subset of all active RABs according to
                                         ///< the RAB release procedure.
                                         ///< Several IEs with this type and instance values shall be
                                         ///< included as necessary to represent a list of RABs to be
                                         ///< released.

	node_type_t originating_node;        ///< This IE shall be sent on S11 interface, if ISR is active in the MME.
                                         ///< This IE shall be sent on S4 interface, if ISR is active in the SGSN
	// Private Extension Private Extension ///< optional

} SgwReleaseAccessBearersRequest;

/** @struct SgwReleaseAccessBearersResponse
 *  @brief Release AccessBearers Response
 *
 * The Release Access Bearers Response message is sent on the S11 interface by the SGW to the MME as part of the S1
 * release procedure.
 * The message shall also be sent on the S4 interface by the SGW to the SGSN as part of the procedures:
 * -  RAB release using S4
 * -  Iu Release using S4
 * -  READY to STANDBY transition within the network
 * Possible Cause values are specified in Table 8.4-1. Message specific cause values are:
 * - "Request accepted".
 * - "Request accepted partially".
 * - "Context not found
 */
typedef struct SgwReleaseAccessBearersResponse_s {
	Teid_t      teid;                   ///< Tunnel Endpoint Identifier
	SGWCause_t  cause;
	// Recovery           ///< optional This IE shall be included if contacting the peer for the first time
	// Private Extension  ///< optional

} SgwReleaseAccessBearersResponse;

#endif
