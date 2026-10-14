/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <gtest/gtest.h>
#include <cstring>
#ifdef __cplusplus
extern "C" {
#endif
#include "openair2/RRC/NR/MESSAGES/asn1_msg.h"
#include "common/ran_context.h"
#include <stdbool.h>
#include "common/utils/assertions.h"
#include "common/utils/LOG/log.h"
#include "NR_DRB-ToAddMod.h"
#include "NR_DRB-ToAddModList.h"
#include "NR_SRB-ToAddModList.h"
#include "NR_InterFreqCarrierFreqInfo.h"
#include "ds/byte_array.h"
#include "common/utils/nr/nr_common.h"
#include "NR_SIB2.h"
#include "NR_SIB3.h"
#include "NR_SIB4.h"
#include "NR_IntraFreqNeighCellInfo.h"
#include "NR_IntraFreqNeighCellList.h"
#include "NR_InterFreqCarrierFreqInfo.h"
#include "NR_InterFreqNeighCellInfo.h"
#include "NR_InterFreqNeighCellList.h"
#include "asn_application.h"
#include "NR_PCCH-Message.h"
#include "NR_PagingRecord.h"
#include "openair3/UTILS/conversions.h"
RAN_CONTEXT_t RC;
#ifdef __cplusplus
}
#endif

static NR_SIB2_t *encode_and_decode_sib2(const NR_SIB2_t *sib2, byte_array_t *ba_out)
{
  *ba_out = do_SIB2_NR(sib2);
  EXPECT_GT(ba_out->len, 0);
  EXPECT_NE(ba_out->buf, nullptr);

  NR_SIB2_t *decoded = nullptr;
  asn_dec_rval_t dec = uper_decode(nullptr, &asn_DEF_NR_SIB2, (void **)&decoded, ba_out->buf, ba_out->len, 0, 0);
  EXPECT_EQ(dec.code, RC_OK);
  EXPECT_NE(decoded, nullptr);
  return decoded;
}

static NR_SIB3_t *encode_and_decode_sib3(const NR_SIB3_t *sib3, byte_array_t *ba_out)
{
  *ba_out = do_SIB3_NR(sib3);
  EXPECT_GT(ba_out->len, 0);
  EXPECT_NE(ba_out->buf, nullptr);

  NR_SIB3_t *decoded = nullptr;
  asn_dec_rval_t dec = uper_decode(nullptr, &asn_DEF_NR_SIB3, (void **)&decoded, ba_out->buf, ba_out->len, 0, 0);
  EXPECT_EQ(dec.code, RC_OK);
  EXPECT_NE(decoded, nullptr);
  return decoded;
}

static NR_SIB4_t *encode_and_decode_sib4(NR_SIB4_t *sib4, byte_array_t *ba_out)
{
  *ba_out = do_SIB4_NR(sib4);
  EXPECT_GT(ba_out->len, 0);
  EXPECT_NE(ba_out->buf, nullptr);

  NR_SIB4_t *decoded = nullptr;
  asn_dec_rval_t dec = uper_decode(nullptr, &asn_DEF_NR_SIB4, (void **)&decoded, ba_out->buf, ba_out->len, 0, 0);
  EXPECT_EQ(dec.code, RC_OK);
  EXPECT_NE(decoded, nullptr);
  return decoded;
}

TEST(nr_asn1, rrc_reject)
{
  unsigned char buf[1000];
  EXPECT_GT(do_RRCReject(buf), 0);
}

TEST(nr_asn1, sa_capability_enquiry)
{
  unsigned char buf[1000];
  EXPECT_GT(do_NR_SA_UECapabilityEnquiry(buf, 0, 78), 0);
}

TEST(nr_asn1, rrc_reconfiguration_complete_for_nsa)
{
  unsigned char buf[1000];
  EXPECT_GT(do_NR_RRCReconfigurationComplete_for_nsa(buf, 1000, 0), 0);
}

TEST(nr_asn1, ul_information_transfer)
{
  unsigned char *buf = NULL;
  unsigned char pdu[20] = {0};
  EXPECT_GT(do_NR_ULInformationTransfer(&buf, 20, pdu), 0);
  EXPECT_NE(buf, nullptr);
  free(buf);
}

TEST(nr_asn1, rrc_reestablishment_request)
{
  unsigned char buf[1000];
  const uint16_t c_rnti = 1;
  const uint32_t cell_id = 177;
  EXPECT_GT(do_RRCReestablishmentRequest(buf, NR_ReestablishmentCause_reconfigurationFailure, cell_id, c_rnti, 0), 0);
}

TEST(nr_asn1, rrc_reestablishment)
{
  unsigned char buf[1000];
  const uint8_t nh_ncc = 0;
  EXPECT_GT(do_RRCReestablishment(nh_ncc, buf, 1000, 0), 0);
}

static void encode_decode_paging(nr_paging_params_t params, nr_paging_params_t *decoded_params)
{
  byte_array_t msg = do_NR_Paging(1, &params);
  ASSERT_NE(msg.buf, nullptr);
  ASSERT_GT(msg.len, 0u);

  int count = 0;
  int dec_ret = nr_pcch_decode(msg, decoded_params, &count);
  ASSERT_EQ(dec_ret, 0);
  ASSERT_GE(count, 1);

  free_byte_array(msg);
}

/* Encode NR Paging (PCCH) with do_NR_Paging, validate with nr_pcch_decode (same decoder as UE). */
TEST(nr_asn1, paging)
{
  nr_paging_params_t decoded_params[NR_PCCH_MAX_PAGING_RECORDS];

  /* Full 48-bit identity (AMF Set ID + Pointer + M-TMSI) */
  {
    const uint64_t fiveg_s_tmsi = nr_construct_5g_s_tmsi(1, 0, 0xc00006f8U);
    EXPECT_EQ(fiveg_s_tmsi, 0x0040c00006f8ULL);
    nr_paging_params_t params = {.ue_identity_type = NR_PagingUE_Identity_PR_ng_5G_S_TMSI,
                                 .ue_identity = {.fiveg_s_tmsi = fiveg_s_tmsi},
                                 .access_type = false,
                                 .paging_cause = NULL};
    encode_decode_paging(params, decoded_params);
    EXPECT_EQ(decoded_params[0].ue_identity.fiveg_s_tmsi, fiveg_s_tmsi);
  }

  /* accessType enc/dec: encode with access_type true */
  {
    nr_paging_params_t params = {.ue_identity_type = NR_PagingUE_Identity_PR_ng_5G_S_TMSI,
                                 .ue_identity = {.fiveg_s_tmsi = nr_construct_5g_s_tmsi(0, 0, 0xDEADBEEFU)},
                                 .access_type = true,
                                 .paging_cause = NULL};
    encode_decode_paging(params, decoded_params);
    EXPECT_TRUE(decoded_params[0].access_type);
  }

  /* 3 PagingRecords in one PCCH */
  {
    const uint64_t tmsi[] = {nr_construct_5g_s_tmsi(1, 0, 0xc00006f8U),
                             nr_construct_5g_s_tmsi(2, 1, 0xdeadbeefU),
                             nr_construct_5g_s_tmsi(0, 0, 0x12345678U)};
    nr_paging_params_t params[3] = {
        {.ue_identity_type = NR_PagingUE_Identity_PR_ng_5G_S_TMSI, .ue_identity = {.fiveg_s_tmsi = tmsi[0]}},
        {.ue_identity_type = NR_PagingUE_Identity_PR_ng_5G_S_TMSI,
         .ue_identity = {.fiveg_s_tmsi = tmsi[1]},
         .access_type = true},
        {.ue_identity_type = NR_PagingUE_Identity_PR_ng_5G_S_TMSI, .ue_identity = {.fiveg_s_tmsi = tmsi[2]}},
    };

    byte_array_t msg = do_NR_Paging(3, params);
    ASSERT_NE(msg.buf, nullptr);
    ASSERT_GT(msg.len, 0u);

    int count = 0;
    ASSERT_EQ(nr_pcch_decode(msg, decoded_params, &count), 0);
    ASSERT_EQ(count, 3);
    for (int i = 0; i < 3; i++) {
      EXPECT_EQ(decoded_params[i].ue_identity.fiveg_s_tmsi, tmsi[i]);
      EXPECT_EQ(decoded_params[i].access_type, params[i].access_type);
    }

    free_byte_array(msg);
  }

  /* fullI-RNTI (40 bits): encode/decode roundtrip */
  const uint8_t test_full_i_rntis[][NR_PAGING_FULL_I_RNTI_SIZE] = {
      {0x00, 0x00, 0x00, 0x00, 0x00},
      {0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
      {0x12, 0x34, 0x56, 0x78, 0x9A},
  };

  for (size_t i = 0; i < sizeofArray(test_full_i_rntis); i++) {
    nr_paging_params_t params = {.ue_identity_type = NR_PagingUE_Identity_PR_fullI_RNTI,
                                 .ue_identity = {.full_i_rnti = {0}},
                                 .access_type = false,
                                 .paging_cause = NULL};
    memcpy(params.ue_identity.full_i_rnti, test_full_i_rntis[i], NR_PAGING_FULL_I_RNTI_SIZE);

    encode_decode_paging(params, decoded_params);
    ASSERT_EQ(decoded_params[0].ue_identity_type, NR_PagingUE_Identity_PR_fullI_RNTI);
    ASSERT_EQ(memcmp(decoded_params[0].ue_identity.full_i_rnti, test_full_i_rntis[i], NR_PAGING_FULL_I_RNTI_SIZE), 0)
        << "Decoded fullI-RNTI should match original";
  }
}

void free_RRCReconfiguration_params(nr_rrc_reconfig_param_t params)
{
  ASN_STRUCT_FREE(asn_DEF_NR_MeasConfig, params.meas_config);
  ASN_STRUCT_FREE(asn_DEF_NR_DRB_ToAddModList, params.drb_config_list);
  ASN_STRUCT_FREE(asn_DEF_NR_SRB_ToAddModList, params.srb_config_list);
  ASN_STRUCT_FREE(asn_DEF_NR_SecurityConfig, params.security_config);
  for (int i = 0; i < params.num_nas_msg; i++)
    FREE_AND_ZERO_BYTE_ARRAY(params.dedicated_NAS_msg_list[i]);
}

TEST(nr_asn1, rrc_reconfiguration)
{
  // SRB Configuration
  NR_SRB_ToAddModList_t *srb_config_list = (NR_SRB_ToAddModList_t *)calloc_or_fail(1, sizeof(*srb_config_list));
  for (int i = 0; i < 4; i++) {
    if (i == 1 || i == 2) {
      NR_SRB_ToAddMod_t *srb = (NR_SRB_ToAddMod_t *)calloc_or_fail(1, sizeof(*srb));
      ASN_SEQUENCE_ADD(&srb_config_list->list, srb);
      srb->srb_Identity = i;
      if (i == 1 || i == 2) {
        srb->reestablishPDCP = (long *)calloc_or_fail(1, sizeof(*srb->reestablishPDCP));
        *srb->reestablishPDCP = 0;
      }
    }
  }

  // DRB Configuration
  NR_DRB_ToAddModList_t *drb_config_list = (NR_DRB_ToAddModList_t *)calloc_or_fail(1, sizeof(*drb_config_list));
  for (int i = 0; i < 32; i++) {
    if (i == 1 || i == 2) {
      NR_DRB_ToAddMod_t *drb = (NR_DRB_ToAddMod_t *)calloc_or_fail(1, sizeof(*drb));
      ASN_SEQUENCE_ADD(&drb_config_list->list, drb);
      drb->drb_Identity = i;
      drb->reestablishPDCP = (long *)calloc_or_fail(1, sizeof(*drb->reestablishPDCP));
      *drb->reestablishPDCP = 0;
    }
  }

  // nr_rrc_reconfig_param_t setup
  nr_rrc_reconfig_param_t params = {};
  params.srb_config_list = srb_config_list;
  params.drb_config_list = drb_config_list;
  params.num_nas_msg = 2;
  params.masterKeyUpdate = false;
  params.nextHopChainingCount = 1;

  byte_array_t nas_pdu_1;
  nas_pdu_1.buf = (uint8_t *)malloc_or_fail(4);
  memcpy(nas_pdu_1.buf, "NAS1", 4);
  nas_pdu_1.len = 4;

  byte_array_t nas_pdu_2;
  nas_pdu_2.buf = (uint8_t *)malloc_or_fail(4);
  memcpy(nas_pdu_2.buf, "NAS2", 4);
  nas_pdu_2.len = 4;

  params.dedicated_NAS_msg_list[0] = nas_pdu_1;
  params.dedicated_NAS_msg_list[1] = nas_pdu_2;

  byte_array_t msg = do_RRCReconfiguration(&params);

  EXPECT_GT(msg.len, 0);
  EXPECT_NE(msg.buf, nullptr);

  LOG_D(NR_RRC, "RRCReconfiguration: Encoded (%ld bytes)\n", msg.len);

  free_byte_array(msg);
  free_RRCReconfiguration_params(params);
}

TEST(nr_asn1, sib2_basic_encode_decode)
{
  NR_SIB2_t *sib2 = (NR_SIB2_t *)calloc_or_fail(1, sizeof(*sib2));
  sib2->cellReselectionInfoCommon.q_Hyst = NR_SIB2__cellReselectionInfoCommon__q_Hyst_dB0;
  sib2->cellReselectionServingFreqInfo.cellReselectionPriority = 3;
  sib2->cellReselectionServingFreqInfo.threshServingLowP = 10;
  sib2->intraFreqCellReselectionInfo.q_RxLevMin = -56;
  sib2->intraFreqCellReselectionInfo.s_IntraSearchP = 22;
  sib2->intraFreqCellReselectionInfo.t_ReselectionNR = 1;
  sib2->intraFreqCellReselectionInfo.deriveSSB_IndexFromCell = true;

  byte_array_t ba = {};
  NR_SIB2_t *decoded = encode_and_decode_sib2(sib2, &ba);

  EXPECT_EQ(decoded->cellReselectionServingFreqInfo.cellReselectionPriority,
            sib2->cellReselectionServingFreqInfo.cellReselectionPriority);
  EXPECT_EQ(decoded->cellReselectionServingFreqInfo.threshServingLowP, sib2->cellReselectionServingFreqInfo.threshServingLowP);

  ASN_STRUCT_FREE(asn_DEF_NR_SIB2, sib2);
  ASN_STRUCT_FREE(asn_DEF_NR_SIB2, decoded);
  free_byte_array(ba);
}

TEST(nr_asn1, sib3_basic_encode_decode)
{
  NR_SIB3_t *sib3 = (NR_SIB3_t *)calloc_or_fail(1, sizeof(*sib3));
  sib3->intraFreqNeighCellList = (struct NR_IntraFreqNeighCellList *)calloc_or_fail(1, sizeof(*sib3->intraFreqNeighCellList));

  NR_IntraFreqNeighCellInfo_t *cell = (NR_IntraFreqNeighCellInfo_t *)calloc_or_fail(1, sizeof(*cell));
  cell->physCellId = 100;
  cell->q_OffsetCell = NR_Q_OffsetRange_dB0;
  ASN_SEQUENCE_ADD(&sib3->intraFreqNeighCellList->list, cell);

  byte_array_t ba = {};
  NR_SIB3_t *decoded = encode_and_decode_sib3(sib3, &ba);

  ASSERT_NE(decoded->intraFreqNeighCellList, nullptr);
  EXPECT_EQ(decoded->intraFreqNeighCellList->list.count, 1);
  EXPECT_EQ(decoded->intraFreqNeighCellList->list.array[0]->physCellId, cell->physCellId);

  ASN_STRUCT_FREE(asn_DEF_NR_SIB3, sib3);
  ASN_STRUCT_FREE(asn_DEF_NR_SIB3, decoded);
  free_byte_array(ba);
}

TEST(nr_asn1, sib4_basic_encode_decode)
{
  NR_SIB4_t *sib4 = (NR_SIB4_t *)calloc_or_fail(1, sizeof(*sib4));

  NR_InterFreqCarrierFreqInfo_t *carrier = (NR_InterFreqCarrierFreqInfo_t *)calloc_or_fail(1, sizeof(*carrier));
  carrier->dl_CarrierFreq = 2000;
  carrier->ssbSubcarrierSpacing = NR_SubcarrierSpacing_kHz15;
  carrier->deriveSSB_IndexFromCell = true;
  carrier->q_RxLevMin = -56;
  carrier->t_ReselectionNR = 1;
  carrier->threshX_HighP = 5;
  carrier->threshX_LowP = 3;
  carrier->interFreqNeighCellList = (struct NR_InterFreqNeighCellList *)calloc_or_fail(1, sizeof(*carrier->interFreqNeighCellList));

  NR_InterFreqNeighCellInfo_t *ncell = (NR_InterFreqNeighCellInfo_t *)calloc_or_fail(1, sizeof(*ncell));
  ncell->physCellId = 200;
  ncell->q_OffsetCell = NR_Q_OffsetRange_dB0;
  ASN_SEQUENCE_ADD(&carrier->interFreqNeighCellList->list, ncell);
  ASN_SEQUENCE_ADD(&sib4->interFreqCarrierFreqList.list, carrier);

  byte_array_t ba = {};
  NR_SIB4_t *decoded = encode_and_decode_sib4(sib4, &ba);

  ASSERT_GT(decoded->interFreqCarrierFreqList.list.count, 0);
  EXPECT_EQ(decoded->interFreqCarrierFreqList.list.array[0]->dl_CarrierFreq, carrier->dl_CarrierFreq);

  ASN_STRUCT_FREE(asn_DEF_NR_SIB4, sib4);
  ASN_STRUCT_FREE(asn_DEF_NR_SIB4, decoded);
  free_byte_array(ba);
}

TEST(nr_asn1, inter_freq_carrier_freq_info_ranges)
{
  NR_SIB4_t *sib4 = (NR_SIB4_t *)calloc_or_fail(1, sizeof(*sib4));
  ASN_SEQUENCE_ADD(&sib4->interFreqCarrierFreqList.list,
                   (NR_InterFreqCarrierFreqInfo_t *)calloc_or_fail(1, sizeof(NR_InterFreqCarrierFreqInfo_t)));

  NR_InterFreqCarrierFreqInfo_t *carrier = sib4->interFreqCarrierFreqList.list.array[0];
  carrier->dl_CarrierFreq = 640000;
  carrier->ssbSubcarrierSpacing = NR_SubcarrierSpacing_kHz30;
  carrier->deriveSSB_IndexFromCell = 1;
  carrier->threshX_HighP = 0;
  carrier->threshX_LowP = 0;

  NR_Q_OffsetRange_t *qoff = (NR_Q_OffsetRange_t *)calloc_or_fail(1, sizeof(*qoff));
  *qoff = 15; // arbitrary valid Q-OffsetRange enum
  carrier->q_OffsetFreq = qoff;

  // Optional threshX_Q struct
  carrier->threshX_Q = (decltype(carrier->threshX_Q))calloc_or_fail(1, sizeof(*carrier->threshX_Q));
  carrier->threshX_Q->threshX_HighQ = 0;
  carrier->threshX_Q->threshX_LowQ = 0;

  auto try_encode = [](NR_SIB4_t *s, bool expect_success) {
    byte_array_t ba = do_SIB4_NR(s);
    int len = (int)ba.len;
    if (expect_success) {
      EXPECT_GT(len, 0);
      EXPECT_NE(ba.buf, nullptr);
    } else {
      EXPECT_LE(len, 0);
    }
    free_byte_array(ba);
  };

  // Valid lower bounds for all constrained scalars
  carrier->q_RxLevMin = -70;
  carrier->t_ReselectionNR = 0;
  carrier->threshX_HighP = 0;
  carrier->threshX_LowP = 0;
  carrier->threshX_Q->threshX_HighQ = 0;
  carrier->threshX_Q->threshX_LowQ = 0;
  *qoff = 15;
  try_encode(sib4, true);

  // Invalid q_RxLevMin (below ASN.1 range)
  carrier->q_RxLevMin = -80;
  carrier->t_ReselectionNR = 0;
  try_encode(sib4, false);

  // Invalid t_ReselectionNR (above ASN.1 range)
  carrier->q_RxLevMin = -70;
  carrier->t_ReselectionNR = 8;
  try_encode(sib4, false);

  // Restore valid q_RxLevMin / t_ReselectionNR
  carrier->q_RxLevMin = -70;
  carrier->t_ReselectionNR = 0;

  // Invalid threshX_HighP (below 0)
  carrier->threshX_HighP = -1;
  try_encode(sib4, false);
  carrier->threshX_HighP = 0;

  // Invalid threshX_LowP (below 0)
  carrier->threshX_LowP = -1;
  try_encode(sib4, false);
  carrier->threshX_LowP = 0;

  // Invalid threshX_HighQ (below 0)
  carrier->threshX_Q->threshX_HighQ = -1;
  try_encode(sib4, false);
  carrier->threshX_Q->threshX_HighQ = 0;

  // Invalid threshX_LowQ (below 0)
  carrier->threshX_Q->threshX_LowQ = -1;
  try_encode(sib4, false);
  carrier->threshX_Q->threshX_LowQ = 0;

  // Invalid q_OffsetFreq (outside [-24,24] enum range)
  *qoff = 99;
  try_encode(sib4, false);

  ASN_STRUCT_FREE(asn_DEF_NR_SIB4, sib4);
}

int main(int argc, char **argv)
{
  logInit();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
