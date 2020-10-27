/*!
\file itti_sim_messages_types.h

\brief itti message for itti simulator
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#ifndef ITTI_SIM_MESSAGES_TYPES_H_
#define ITTI_SIM_MESSAGES_TYPES_H_

#include "LTE_asn_constant.h"



#define GNB_RRC_BCCH_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.GNBBCCHind
#define GNB_RRC_CCCH_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.GNBCCCHind
#define GNB_RRC_DCCH_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.GNBDCCHind
#define UE_RRC_CCCH_DATA_IND(mSGpTR)            (mSGpTR)->ittiMsg.UECCCHind
#define UE_RRC_DCCH_DATA_IND(mSGpTR)            (mSGpTR)->ittiMsg.UEDCCHind



typedef struct itti_sim_rrc_ch_s {
    rb_id_t                      rbid;
    uint8_t                     *sdu;
    int                          size;
} itti_sim_rrc_ch_t;

#endif /* ITTI_SIM_MESSAGES_TYPES_H_ */
