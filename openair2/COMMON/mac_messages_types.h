/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*
 * mac_messages_types.h
 */

#ifndef MAC_MESSAGES_TYPES_H_
#define MAC_MESSAGES_TYPES_H_

#include <LTE_DRX-Config.h>
#include "OCTET_STRING.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_config.h"

//-------------------------------------------------------------------------------------------//
// Defines to access message fields.
#define RRC_MAC_IN_SYNC_IND(mSGpTR)             (mSGpTR)->ittiMsg.rrc_mac_in_sync_ind
#define RRC_MAC_OUT_OF_SYNC_IND(mSGpTR)         (mSGpTR)->ittiMsg.rrc_mac_out_of_sync_ind
#define NR_RRC_MAC_SYNC_IND(mSGpTR)             (mSGpTR)->ittiMsg.nr_rrc_mac_sync_ind

#define RRC_MAC_BCCH_DATA_REQ(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_bcch_data_req
#define RRC_MAC_BCCH_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_bcch_data_ind
#define NR_RRC_MAC_BCCH_DATA_IND(mSGpTR)        (mSGpTR)->ittiMsg.nr_rrc_mac_bcch_data_ind
#define NR_RRC_MAC_SBCCH_DATA_IND(mSGpTR)       (mSGpTR)->ittiMsg.nr_rrc_mac_sbcch_data_ind

#define NR_RRC_MAC_MEAS_DATA_IND(mSGpTR)        (mSGpTR)->ittiMsg.nr_rrc_mac_meas_data_ind

#define RRC_MAC_BCCH_MBMS_DATA_REQ(mSGpTR)      (mSGpTR)->ittiMsg.rrc_mac_bcch_mbms_data_req
#define RRC_MAC_BCCH_MBMS_DATA_IND(mSGpTR)      (mSGpTR)->ittiMsg.rrc_mac_bcch_mbms_data_ind

#define RRC_MAC_CCCH_DATA_REQ(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_ccch_data_req
#define RRC_MAC_CCCH_DATA_CNF(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_ccch_data_cnf
#define RRC_MAC_CCCH_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_ccch_data_ind
#define NR_RRC_MAC_CCCH_DATA_IND(mSGpTR)        (mSGpTR)->ittiMsg.nr_rrc_mac_ccch_data_ind

#define RRC_MAC_MCCH_DATA_REQ(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_mcch_data_req
#define RRC_MAC_MCCH_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_mcch_data_ind
#define RRC_MAC_PCCH_DATA_REQ(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_pcch_data_req
#define NR_RRC_MAC_PCCH_DATA_IND(mSGpTR) (mSGpTR)->ittiMsg.nr_rrc_mac_pcch_data_ind

#define NR_RRC_MAC_RA_IND(mSGpTR)               (mSGpTR)->ittiMsg.nr_rrc_mac_ra_ind
#define NR_RRC_MAC_MSG3_IND(mSGpTR)             (mSGpTR)->ittiMsg.nr_rrc_mac_msg3_ind
#define NR_RRC_MAC_INAC_IND(mSGpTR)             (mSGpTR)->ittiMsg.nr_rrc_mac_inac_ind
#define NR_RRC_MAC_VERIFY(mSGpTR)               (mSGpTR)->ittiMsg.nr_rrc_mac_verify

#define RRC_MAC_DRX_CONFIG_REQ(mSGpTR)          (mSGpTR)->ittiMsg.rrc_mac_drx_config_req

#define GNB_SAT_POSITION_UPDATE(mSGpTR)         (mSGpTR)->ittiMsg.gnb_sat_position_update

// Some constants from "LAYER2/MAC/defs.h"
#define BCCH_SDU_SIZE                           (512)
#define BCCH_SDU_MBMS_SIZE                      (512)
#define CCCH_SDU_SIZE                           (1024)
#define MCCH_SDU_SIZE                           (512)
#define PCCH_SDU_SIZE                           (512)

//-------------------------------------------------------------------------------------------//
// Messages between RRC and MAC layers

typedef struct NRRrcMacRaInd_s {
  bool RA_succeeded;
  int gnb_index;
} NRRrcMacRaInd;

typedef struct NRRrcMacMsg3Ind_s {
  uint16_t rnti;
  int gnb_id;
  bool prepare_payload;
} NRRrcMacMsg3Ind;

typedef struct NRRrcMacVerify_s {
  bool L2_RRC_verification_failed; // not to leave the struct empty
} NRRrcMacVerify;

typedef struct NRRrcMacInacInd_s {
  bool inactivity_timer_expired; // not to leave the struct empty
} NRRrcMacInacInd;

typedef struct RrcMacInSyncInd_s {
  uint32_t  frame;
  uint8_t   sub_frame;
  uint16_t  enb_index;
} RrcMacInSyncInd;

typedef RrcMacInSyncInd RrcMacOutOfSyncInd;

typedef struct NRRrcMacSyncInd_s {
  uint32_t frame;
  bool in_sync;
} NRRrcMacSyncInd;

typedef struct RrcMacBcchDataReq_s {
  uint32_t  frame;
  uint32_t  sdu_size;
  uint8_t   sdu[BCCH_SDU_SIZE];
  uint8_t   enb_index;
} RrcMacBcchDataReq;

typedef struct RrcMacBcchDataInd_s {
  uint32_t  frame;
  uint8_t   sub_frame;
  uint32_t  sdu_size;
  uint8_t   sdu[BCCH_SDU_SIZE];
  uint8_t   enb_index;
  uint8_t   rsrq;
  uint8_t   rsrp;
} RrcMacBcchDataInd;

typedef struct NRRrcMacBcchDataInd_s {
  uint32_t  hfn;
  uint32_t  frame;
  uint8_t   slot;
  uint32_t  sdu_size;
  uint8_t   sdu[BCCH_SDU_SIZE];
  uint8_t   gnb_index;
  bool      is_bch;
  uint8_t   rsrq;
  uint8_t   rsrp;
  uint32_t  phycellid;
  long      ssb_arfcn;
} NRRrcMacBcchDataInd;

typedef struct NRRrcMacSBcchDataInd_s {
  uint32_t  hfn;
  uint32_t  frame;
  uint8_t   slot;
  uint32_t  sdu_size;
  uint8_t   sdu[BCCH_SDU_SIZE];
  uint8_t   gnb_index;
  uint16_t  rx_slss_id;
  uint8_t   rsrq;
  uint8_t   rsrp;
} NRRrcMacSBcchDataInd;

typedef struct NRRrcMacMeasDataInd_s {
  uint8_t gnb_index;
  int rsrp_dBm;
  uint16_t Nid_cell;
  bool is_csi_meas;
  bool is_neighboring_cell;
} NRRrcMacMeasDataInd;

typedef struct RrcMacBcchMbmsDataReq_s {
  uint32_t  frame;
  uint32_t  sdu_size;
  uint8_t   sdu[BCCH_SDU_MBMS_SIZE];
  uint8_t   enb_index;
} RrcMacBcchMbmsDataReq;

typedef struct RrcMacBcchMbmsDataInd_s {
  uint32_t  frame;
  uint8_t   sub_frame;
  uint32_t  sdu_size;
  uint8_t   sdu[BCCH_SDU_MBMS_SIZE];
  uint8_t   enb_index;
  uint8_t   rsrq;
  uint8_t   rsrp;
} RrcMacBcchMbmsDataInd;


typedef struct RrcMacCcchDataReq_s {
  uint32_t  frame;
  uint32_t  sdu_size;
  uint8_t   sdu[CCCH_SDU_SIZE];
  uint8_t   enb_index;
} RrcMacCcchDataReq;

typedef struct RrcMacCcchDataCnf_s {
  uint8_t   enb_index;
} RrcMacCcchDataCnf;

typedef struct RrcMacCcchDataInd_s {
  uint32_t  frame;
  uint8_t   sub_frame;
  uint16_t  rnti;
  uint32_t  sdu_size;
  uint8_t   sdu[CCCH_SDU_SIZE];
  uint8_t   enb_index;
  int       CC_id;
} RrcMacCcchDataInd;

typedef struct NRRrcMacCcchDataInd_s {
  uint32_t  sdu_size;
  uint8_t   sdu[CCCH_SDU_SIZE];
} NRRrcMacCcchDataInd;

typedef struct NRRrcMacPcchDataInd_s {
  uint32_t sdu_size;
  uint8_t sdu[PCCH_SDU_SIZE];
} NRRrcMacPcchDataInd;

typedef struct RrcMacMcchDataReq_s {
  uint32_t  frame;
  uint32_t  sdu_size;
  uint8_t   sdu[MCCH_SDU_SIZE];
  uint8_t   enb_index;
  uint8_t   mbsfn_sync_area;
} RrcMacMcchDataReq;

typedef struct RrcMacMcchDataInd_s {
  uint32_t  frame;
  uint8_t   sub_frame;
  uint32_t  sdu_size;
  uint8_t   sdu[MCCH_SDU_SIZE];
  uint8_t   enb_index;
  uint8_t   mbsfn_sync_area;
} RrcMacMcchDataInd;

typedef struct RrcMacPcchDataReq_s {
  uint32_t  frame;
  uint32_t  sdu_size;
  uint8_t   sdu[PCCH_SDU_SIZE];
  uint8_t   enb_index;
} RrcMacPcchDataReq;

/* RRC configures DRX context (MAC timers) of a UE */
typedef struct rrc_mac_drx_config_req_s {
  /* UE RNTI to configure */
  rnti_t rnti;

  /* DRX configuration from MacMainConfig to configure UE's local timers */
  LTE_DRX_Config_t * drx_Configuration;
} rrc_mac_drx_config_req_t;

#endif /* MAC_MESSAGES_TYPES_H_ */
