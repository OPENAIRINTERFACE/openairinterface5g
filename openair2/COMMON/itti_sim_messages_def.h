/*!
\file itti_sim_messages_def.h

\brief itti message for itti simulator
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/



MESSAGE_DEF(GNB_RRC_BCCH_DATA_IND,          MESSAGE_PRIORITY_MED, itti_sim_rrc_ch_t,   GNBBCCHind)
MESSAGE_DEF(GNB_RRC_CCCH_DATA_IND,          MESSAGE_PRIORITY_MED, itti_sim_rrc_ch_t,   GNBCCCHind)
MESSAGE_DEF(GNB_RRC_DCCH_DATA_IND,          MESSAGE_PRIORITY_MED, itti_sim_rrc_ch_t,   GNBDCCHind)
MESSAGE_DEF(UE_RRC_CCCH_DATA_IND,           MESSAGE_PRIORITY_MED, itti_sim_rrc_ch_t,   UECCCHind)
MESSAGE_DEF(UE_RRC_DCCH_DATA_IND,           MESSAGE_PRIORITY_MED, itti_sim_rrc_ch_t,   UEDCCHind)
