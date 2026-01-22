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

/*! \file f1ap_lib_test.c
 * \brief Test functions for F1AP encoding/decoding library
 * \author Guido Casati, Robert Schmidt
 * \date 2024
 * \version 0.1
 * \note
 * \warning
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "common/utils/assertions.h"
#include "common/utils/utils.h"

#include "f1ap_messages_types.h"
#include "F1AP_F1AP-PDU.h"

#include "lib/f1ap_lib_common.h"
#include "lib/f1ap_rrc_message_transfer.h"
#include "lib/f1ap_interface_management.h"
#include "lib/f1ap_ue_context.h"

void exit_function(const char *file, const char *function, const int line, const char *s, const int assert)
{
  printf("detected error at %s:%d:%s: %s\n", file, line, function, s);
  abort();
}

static F1AP_F1AP_PDU_t *f1ap_encode_decode(const F1AP_F1AP_PDU_t *enc_pdu)
{
  //xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, enc_pdu);

  DevAssert(enc_pdu != NULL);
  char errbuf[1024];
  size_t errlen = sizeof(errbuf);
  int ret = asn_check_constraints(&asn_DEF_F1AP_F1AP_PDU, enc_pdu, errbuf, &errlen);
  AssertFatal(ret == 0, "asn_check_constraints() failed: %s\n", errbuf);

  uint8_t msgbuf[16384];
  asn_enc_rval_t enc = aper_encode_to_buffer(&asn_DEF_F1AP_F1AP_PDU, NULL, enc_pdu, msgbuf, sizeof(msgbuf));
  AssertFatal(enc.encoded > 0, "aper_encode_to_buffer() failed\n");

  F1AP_F1AP_PDU_t *dec_pdu = NULL;
  asn_codec_ctx_t st = {.max_stack_size = 100 * 1000};
  asn_dec_rval_t dec = aper_decode(&st, &asn_DEF_F1AP_F1AP_PDU, (void **)&dec_pdu, msgbuf, enc.encoded, 0, 0);
  AssertFatal(dec.code == RC_OK, "aper_decode() failed\n");

  //xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, dec_pdu);
  return dec_pdu;
}

static void f1ap_msg_free(F1AP_F1AP_PDU_t *pdu)
{
  ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);
}

/**
 * @brief Test Initial UL RRC Message Transfer encoding/decoding
 */
static void test_initial_ul_rrc_message_transfer(void)
{
  plmn_id_t plmn = {.mcc = 208, .mnc = 95, .mnc_digit_length = 2};
  uint8_t rrc[] = "RRC Container";
  uint8_t du2cu[] = "DU2CU Container";
  f1ap_initial_ul_rrc_message_t orig = {
    .gNB_DU_ue_id = 12,
    .plmn = plmn,
    .nr_cellid = 135,
    .crnti = 0x1234,
    .rrc_container = rrc,
    .rrc_container_length = sizeof(rrc),
    .du2cu_rrc_container = du2cu,
    .du2cu_rrc_container_length = sizeof(du2cu),
    .transaction_id = 2,
  };

  F1AP_F1AP_PDU_t *f1enc = encode_initial_ul_rrc_message_transfer(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_initial_ul_rrc_message_t decoded;
  bool ret = decode_initial_ul_rrc_message_transfer(f1dec, &decoded);
  AssertFatal(ret, "decode_initial_ul_rrc_message_transfer(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_initial_ul_rrc_message_transfer(&orig, &decoded);
  AssertFatal(ret, "eq_initial_ul_rrc_message_transfer(): decoded message doesn't match\n");
  free_initial_ul_rrc_message_transfer(&decoded);

  f1ap_initial_ul_rrc_message_t cp = cp_initial_ul_rrc_message_transfer(&orig);
  ret = eq_initial_ul_rrc_message_transfer(&orig, &cp);
  AssertFatal(ret, "eq_initial_ul_rrc_message_transfer(): copied message doesn't match\n");
  free_initial_ul_rrc_message_transfer(&cp);
}

/**
 * @brief Test DL RRC Message Transfer encoding/decoding
 */
static void test_dl_rrc_message_transfer(void)
{
  uint8_t *rrc = calloc_or_fail(strlen("RRC Container") + 1, sizeof(*rrc));
  uint32_t *old_gNB_DU_ue_id = calloc_or_fail(1, sizeof(*old_gNB_DU_ue_id));
  memcpy((void *)rrc, "RRC Container", strlen("RRC Container") + 1);

  f1ap_dl_rrc_message_t orig = {
    .gNB_DU_ue_id = 12,
    .gNB_CU_ue_id = 12,
    .old_gNB_DU_ue_id = old_gNB_DU_ue_id,
    .srb_id = 1,
    .rrc_container = rrc,
    .rrc_container_length = sizeof(rrc),
  };

  F1AP_F1AP_PDU_t *f1enc = encode_dl_rrc_message_transfer(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_dl_rrc_message_t decoded = {0};
  bool ret = decode_dl_rrc_message_transfer(f1dec, &decoded);
  AssertFatal(ret, "decode_initial_ul_rrc_message_transfer(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_dl_rrc_message_transfer(&orig, &decoded);
  AssertFatal(ret, "eq_dl_rrc_message_transfer(): decoded message doesn't match\n");
  free_dl_rrc_message_transfer(&decoded);

  f1ap_dl_rrc_message_t cp = cp_dl_rrc_message_transfer(&orig);
  ret = eq_dl_rrc_message_transfer(&orig, &cp);
  AssertFatal(ret, "eq_dl_rrc_message_transfer(): copied message doesn't match\n");
  free_dl_rrc_message_transfer(&orig);
  free_dl_rrc_message_transfer(&cp);
}

/**
 * @brief Test UL RRC Message Transfer encoding/decoding
 */
static void test_ul_rrc_message_transfer(void)
{
  uint8_t rrc[] = "RRC Container";

  f1ap_ul_rrc_message_t orig = {
    .gNB_DU_ue_id = 12,
    .gNB_CU_ue_id = 12,
    .srb_id = 1,
    .rrc_container = rrc,
    .rrc_container_length = sizeof(rrc),
  };

  F1AP_F1AP_PDU_t *f1enc = encode_ul_rrc_message_transfer(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_ul_rrc_message_t decoded = {0};
  bool ret = decode_ul_rrc_message_transfer(f1dec, &decoded);
  AssertFatal(ret, "decode_initial_ul_rrc_message_transfer(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_ul_rrc_message_transfer(&orig, &decoded);
  AssertFatal(ret, "eq_dl_rrc_message_transfer(): decoded message doesn't match\n");
  free_ul_rrc_message_transfer(&decoded);

  f1ap_ul_rrc_message_t cp = cp_ul_rrc_message_transfer(&orig);
  ret = eq_ul_rrc_message_transfer(&orig, &cp);
  AssertFatal(ret, "eq_dl_rrc_message_transfer(): copied message doesn't match\n");
  free_ul_rrc_message_transfer(&cp);
}

/**
 * @brief Test F1AP Setup Request Encoding/Decoding
 */
static void test_f1ap_setup_request(void)
{
  /* allocate memory */
  /* gNB_DU_name */
  uint8_t *gNB_DU_name = calloc_or_fail(strlen("OAI DU") + 1, sizeof(*gNB_DU_name));
  memcpy(gNB_DU_name, "OAI DU", strlen("OAI DU") + 1);
  /* sys_info */
  uint8_t *mib = calloc_or_fail(3, sizeof(*mib));
  uint8_t *sib1 = calloc_or_fail(3, sizeof(*sib1));
  f1ap_gnb_du_system_info_t sys_info = {
      .mib_length = 3,
      .mib = mib,
      .sib1_length = 3,
      .sib1 = sib1,
  };
  /* measurement_timing_information */
  int measurement_timing_config_len = strlen("0") + 1;
  uint8_t *measurement_timing_information = calloc_or_fail(measurement_timing_config_len, sizeof(*measurement_timing_information));
  memcpy((void *)measurement_timing_information, "0", measurement_timing_config_len);
  /* TAC */
  uint32_t *tac = calloc_or_fail(1, sizeof(*tac));
  /*
   * TDD test
   */
  // Served Cell Info
  f1ap_served_cell_info_t info = {
      .mode = F1AP_MODE_TDD,
      .tdd.freqinfo.arfcn = 640000,
      .tdd.freqinfo.band = 78,
      .tdd.tbw.nrb = 66,
      .tdd.tbw.scs = 1,
      .measurement_timing_config_len = measurement_timing_config_len,
      .measurement_timing_config = measurement_timing_information,
      .nr_cellid = 123456,
      .plmn.mcc = 1,
      .plmn.mnc = 1,
      .plmn.mnc_digit_length = 3,
      .tac = tac,
      .num_plmn = 2,
      .served_plmn_list[0].plmn = {.mcc = 460, .mnc = 11, .mnc_digit_length = 2},
      .served_plmn_list[0].num_nssai = 1,
      .served_plmn_list[0].nssai[0].sst = 1,
      .served_plmn_list[0].nssai[0].sd = 0x010203,
      .served_plmn_list[1].plmn = {.mcc = 208, .mnc = 93, .mnc_digit_length = 2},
      .served_plmn_list[1].num_nssai = 2,
      .served_plmn_list[1].nssai[0].sst = 1,
      .served_plmn_list[1].nssai[0].sd = 0x010203, 
      .served_plmn_list[1].nssai[1].sst = 1,
      .served_plmn_list[1].nssai[1].sd = 0x112233,
  };
  // create message
  f1ap_setup_req_t orig = {
      .gNB_DU_id = 1,
      .gNB_DU_name = (char *)gNB_DU_name,
      .num_cells_available = 1,
      .transaction_id = 2,
      .rrc_ver[0] = 12,
      .rrc_ver[1] = 34,
      .rrc_ver[2] = 56,
      .cell[0].info = info,
  };
  orig.cell[0].sys_info = calloc_or_fail(1, sizeof(*orig.cell[0].sys_info));
  *orig.cell[0].sys_info = sys_info;
  // encode
  F1AP_F1AP_PDU_t *f1enc = encode_f1ap_setup_request(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);
  // decode
  f1ap_setup_req_t decoded = {0};
  bool ret = decode_f1ap_setup_request(f1dec, &decoded);
  AssertFatal(ret, "decode_f1ap_setup_request(): could not decode message\n");
  f1ap_msg_free(f1dec);
  // equality check
  ret = eq_f1ap_setup_request(&orig, &decoded);
  AssertFatal(ret, "eq_f1ap_setup_request(): decoded message doesn't match\n");
  free_f1ap_setup_request(&decoded);
  // deep copy
  f1ap_setup_req_t cp = cp_f1ap_setup_request(&orig);
  ret = eq_f1ap_setup_request(&orig, &cp);
  AssertFatal(ret, "eq_f1ap_setup_request(): copied message doesn't match\n");
  free_f1ap_setup_request(&cp);
  /*
   * FDD test
   */
  info.mode = F1AP_MODE_FDD;
  info.tdd.freqinfo.arfcn = 0;
  info.tdd.freqinfo.band = 0;
  info.tdd.tbw.nrb = 0;
  info.tdd.tbw.scs = 0;
  info.fdd.ul_freqinfo.arfcn = 640000;
  info.fdd.ul_freqinfo.band = 78;
  info.fdd.dl_freqinfo.arfcn = 641000;
  info.fdd.dl_freqinfo.band = 78;
  info.fdd.ul_tbw.nrb = 66;
  info.fdd.ul_tbw.scs = 1;
  info.fdd.dl_tbw.nrb = 66;
  info.fdd.dl_tbw.scs = 1;
  // encode
  f1enc = encode_f1ap_setup_request(&orig);
  f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);
  // decode
  ret = decode_f1ap_setup_request(f1dec, &decoded);
  AssertFatal(ret, "decode_f1ap_setup_request(): could not decode message\n");
  f1ap_msg_free(f1dec);
  // equality check
  ret = eq_f1ap_setup_request(&orig, &decoded);
  AssertFatal(ret, "eq_f1ap_setup_request(): decoded message doesn't match\n");
  free_f1ap_setup_request(&decoded);
  // copy
  cp = cp_f1ap_setup_request(&orig);
  ret = eq_f1ap_setup_request(&orig, &cp);
  AssertFatal(ret, "eq_f1ap_setup_request(): copied message doesn't match\n");
  free_f1ap_setup_request(&cp);
  // free original message
  free_f1ap_setup_request(&orig);
  
  printf("test_f1ap_setup_request() successful\n");
}

/**
 * @brief Test F1AP Setup Response Encoding/Decoding
 */
static void test_f1ap_setup_response(void)
{
  /* allocate memory */
  /* gNB_CU_name */
  char *cu_name = "OAI-CU";
  int len = strlen(cu_name) + 1;
  uint8_t *gNB_CU_name = calloc_or_fail(len, sizeof(*gNB_CU_name));
  memcpy((void *)gNB_CU_name, cu_name, len);
  /* create message */
  f1ap_setup_resp_t orig = {
      .gNB_CU_name = (char *)gNB_CU_name,
      .transaction_id = 2,
      .rrc_ver[0] = 12,
      .rrc_ver[1] = 34,
      .rrc_ver[2] = 56,
      .num_cells_to_activate = 1,
      .cells_to_activate[0].nr_cellid = 123456,
      .cells_to_activate[0].nrpci = 1,
      .cells_to_activate[0].plmn.mcc = 12,
      .cells_to_activate[0].plmn.mnc = 123,
      .cells_to_activate[0].plmn.mnc_digit_length = 3,
  };
  /* Cells to activate */
  if (orig.num_cells_to_activate) {
    /* SI_container */
    char *s = "test";
    int SI_container_length = strlen(s) + 1;
    orig.cells_to_activate[0].num_SI = 1;
    f1ap_sib_msg_t *SI_msg = &orig.cells_to_activate[0].SI_msg[0];
    SI_msg->SI_container = malloc_or_fail(SI_container_length);
    memcpy(SI_msg->SI_container, (uint8_t *)s, SI_container_length);
    SI_msg->SI_container_length = SI_container_length;
    SI_msg->SI_type = 7;
  }
  F1AP_F1AP_PDU_t *f1enc = encode_f1ap_setup_response(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_setup_resp_t decoded = {0};
  bool ret = decode_f1ap_setup_response(f1dec, &decoded);
  AssertFatal(ret, "decode_f1ap_setup_response(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_f1ap_setup_response(&orig, &decoded);
  AssertFatal(ret, "eq_f1ap_setup_response(): decoded message doesn't match\n");
  free_f1ap_setup_response(&decoded);

  f1ap_setup_resp_t cp = cp_f1ap_setup_response(&orig);
  ret = eq_f1ap_setup_response(&orig, &cp);
  AssertFatal(ret, "eq_f1ap_setup_response(): copied message doesn't match\n");
  free_f1ap_setup_response(&cp);
  free_f1ap_setup_response(&orig);
}

/**
 * @brief Test F1AP Setup Failure Encoding/Decoding
 */
static void test_f1ap_setup_failure(void)
{
  /* create message */
  f1ap_setup_failure_t orig = {
      .transaction_id = 2,
      .cause = 4,
  };
  F1AP_F1AP_PDU_t *f1enc = encode_f1ap_setup_failure(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_setup_failure_t decoded = {0};
  bool ret = decode_f1ap_setup_failure(f1dec, &decoded);
  AssertFatal(ret, "decode_f1ap_setup_failure(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_f1ap_setup_failure(&orig, &decoded);
  AssertFatal(ret, "eq_f1ap_setup_failure(): decoded message doesn't match\n");

  f1ap_setup_failure_t cp = cp_f1ap_setup_failure(&orig);
  ret = eq_f1ap_setup_failure(&orig, &cp);
  AssertFatal(ret, "eq_f1ap_setup_failure(): copied message doesn't match\n");
}

static void _test_f1ap_reset_msg(const f1ap_reset_t *orig)
{
  F1AP_F1AP_PDU_t *f1enc = encode_f1ap_reset(orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_reset_t decoded = {0};
  bool ret = decode_f1ap_reset(f1dec, &decoded);
  AssertFatal(ret, "decode_f1ap_reset(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_f1ap_reset(orig, &decoded);
  AssertFatal(ret, "eq_f1ap_reset(): decoded message doesn't match original\n");
  free_f1ap_reset(&decoded);

  f1ap_reset_t cp = cp_f1ap_reset(orig);
  ret = eq_f1ap_reset(orig, &cp);
  AssertFatal(ret, "eq_f1ap_reset(): copied message doesn't match original\n");
  free_f1ap_reset(&cp);

  printf("f1ap_reset successful\n");
}

/**
 * @brief Test F1AP Reset message where the entire F1 interface is reset
 */
static void test_f1ap_reset_all(void)
{
  f1ap_reset_t orig = {
      .transaction_id = 2,
      .cause = F1AP_CAUSE_TRANSPORT,
      .cause_value = 3, /* no type -> whatever */
      .reset_type = F1AP_RESET_ALL,
  };
  _test_f1ap_reset_msg(&orig);
  free_f1ap_reset(&orig);
}

/**
 * @brief Test F1AP Reset message where only some UEs are marked to reset
 */
static void test_f1ap_reset_part(void)
{
  f1ap_reset_t orig = {
      .transaction_id = 3,
      .cause = F1AP_CAUSE_MISC,
      .cause_value = 3, /* no type -> whatever */
      .reset_type = F1AP_RESET_PART_OF_F1_INTERFACE,
      .num_ue_to_reset = 4,
  };
  orig.ue_to_reset = calloc_or_fail(orig.num_ue_to_reset, sizeof(*orig.ue_to_reset));
  orig.ue_to_reset[0].gNB_CU_ue_id = malloc_or_fail(sizeof(*orig.ue_to_reset[0].gNB_CU_ue_id));
  *orig.ue_to_reset[0].gNB_CU_ue_id = 10;
  orig.ue_to_reset[1].gNB_DU_ue_id = malloc_or_fail(sizeof(*orig.ue_to_reset[1].gNB_DU_ue_id));
  *orig.ue_to_reset[1].gNB_DU_ue_id = 11;
  orig.ue_to_reset[2].gNB_CU_ue_id = malloc_or_fail(sizeof(*orig.ue_to_reset[2].gNB_CU_ue_id));
  *orig.ue_to_reset[2].gNB_CU_ue_id = 12;
  orig.ue_to_reset[2].gNB_DU_ue_id = malloc_or_fail(sizeof(*orig.ue_to_reset[2].gNB_DU_ue_id));
  *orig.ue_to_reset[2].gNB_DU_ue_id = 13;
  // orig.ue_to_reset[3] intentionally empty (because it is allowed, both IDs are optional)
  _test_f1ap_reset_msg(&orig);
  free_f1ap_reset(&orig);
}

/**
 * @brief Test F1 reset ack with differently ack'd UEs ("generic ack" is
 * special case with no individual UE acked)
 */
static void test_f1ap_reset_ack(void)
{
  f1ap_reset_ack_t orig = {
    .transaction_id = 4,
    .num_ue_to_reset = 4,
  };
  orig.ue_to_reset = calloc_or_fail(orig.num_ue_to_reset, sizeof(*orig.ue_to_reset));
  orig.ue_to_reset[0].gNB_CU_ue_id = malloc_or_fail(sizeof(*orig.ue_to_reset[0].gNB_CU_ue_id));
  *orig.ue_to_reset[0].gNB_CU_ue_id = 10;
  orig.ue_to_reset[1].gNB_DU_ue_id = malloc_or_fail(sizeof(*orig.ue_to_reset[1].gNB_DU_ue_id));
  *orig.ue_to_reset[1].gNB_DU_ue_id = 11;
  orig.ue_to_reset[2].gNB_CU_ue_id = malloc_or_fail(sizeof(*orig.ue_to_reset[2].gNB_CU_ue_id));
  *orig.ue_to_reset[2].gNB_CU_ue_id = 12;
  orig.ue_to_reset[2].gNB_DU_ue_id = malloc_or_fail(sizeof(*orig.ue_to_reset[2].gNB_DU_ue_id));
  *orig.ue_to_reset[2].gNB_DU_ue_id = 13;
  // orig.ue_to_reset[3] intentionally empty (because it is allowed, both IDs are optional)

  F1AP_F1AP_PDU_t *f1enc = encode_f1ap_reset_ack(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_reset_ack_t decoded = {0};
  bool ret = decode_f1ap_reset_ack(f1dec, &decoded);
  AssertFatal(ret, "decode_f1ap_reset_ack(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_f1ap_reset_ack(&orig, &decoded);
  AssertFatal(ret, "eq_f1ap_reset_ack(): decoded message doesn't match original\n");
  free_f1ap_reset_ack(&decoded);

  f1ap_reset_ack_t cp = cp_f1ap_reset_ack(&orig);
  ret = eq_f1ap_reset_ack(&orig, &cp);
  AssertFatal(ret, "eq_f1ap_reset_ack(): copied message doesn't match original\n");
  free_f1ap_reset_ack(&cp);

  printf("f1ap_reset_ack successful\n");

  free_f1ap_reset_ack(&orig);
}

/**
 * @brief Test F1 gNB-DU Configuration Update
 */
static void test_f1ap_du_configuration_update(void)
{
  /* sys_info */
  uint8_t *mib = calloc_or_fail(3, sizeof(*mib));
  uint8_t *sib1 = calloc_or_fail(3, sizeof(*sib1));
  f1ap_gnb_du_system_info_t sys_info = {
      .mib_length = 3,
      .mib = mib,
      .sib1_length = 3,
      .sib1 = sib1,
  };
  /* measurement_timing_information modify */
  char *s = "1";
  int measurement_timing_config_len = strlen(s) + 1;
  uint8_t *measurement_timing_config_mod = calloc_or_fail(measurement_timing_config_len, sizeof(*measurement_timing_config_mod));
  memcpy((void *)measurement_timing_config_mod, s, measurement_timing_config_len);
  /* TAC modify */
  uint32_t *tac = calloc_or_fail(1, sizeof(*tac));
  *tac = 456;
  /* info modify */
  f1ap_served_cell_info_t info = {
      .mode = F1AP_MODE_TDD,
      .tdd.freqinfo.arfcn = 640000,
      .tdd.freqinfo.band = 78,
      .tdd.tbw.nrb = 66,
      .tdd.tbw.scs = 1,
      .measurement_timing_config_len = measurement_timing_config_len,
      .measurement_timing_config = measurement_timing_config_mod,
      .nr_cellid = 123456,
      .plmn.mcc = 1,
      .plmn.mnc = 1,
      .plmn.mnc_digit_length = 3,
      .tac = tac,
      .num_plmn = 1,
      .served_plmn_list[0].plmn = {.mcc = 1, .mnc = 1, .mnc_digit_length = 3},
      .served_plmn_list[0].num_nssai = 1,
      .served_plmn_list[0].nssai[0].sst = 1,
      .served_plmn_list[0].nssai[0].sd = 0x010203,
  };
  char *mtc2_data = "mtc2";
  uint8_t *mtc2 = (void*)strdup(mtc2_data);
  int mtc2_len = strlen(mtc2_data);
  f1ap_served_cell_info_t info2 = {
      .mode = F1AP_MODE_FDD,
      .fdd.ul_freqinfo.arfcn = 640000,
      .fdd.ul_freqinfo.band = 78,
      .fdd.dl_freqinfo.arfcn = 600000,
      .fdd.dl_freqinfo.band = 78,
      .fdd.ul_tbw.nrb = 66,
      .fdd.ul_tbw.scs = 1,
      .fdd.dl_tbw.nrb = 66,
      .fdd.dl_tbw.scs = 1,
      .measurement_timing_config_len = mtc2_len,
      .measurement_timing_config = mtc2,
      .nr_cellid = 123456,
      .plmn.mcc = 2,
      .plmn.mnc = 2,
      .plmn.mnc_digit_length = 2,
      .num_plmn = 1,
      .served_plmn_list[0].plmn = {.mcc = 2, .mnc = 2, .mnc_digit_length = 2},
      .served_plmn_list[0].num_nssai = 1,
      .served_plmn_list[0].nssai[0].sst = 1,
      .served_plmn_list[0].nssai[0].sd = 0x112233,
  };
  /* create message */
  f1ap_gnb_du_configuration_update_t orig = {
      .transaction_id = 2,
      .num_cells_to_add = 1,
      .cell_to_add[0].info = info2,
      .num_cells_to_modify = 1,
      .cell_to_modify[0].info = info,
      .cell_to_modify[0].old_nr_cellid = 1235UL,
      .cell_to_modify[0].old_plmn.mcc = 208,
      .cell_to_modify[0].old_plmn.mnc = 88,
      .cell_to_modify[0].old_plmn.mnc_digit_length = 2,
      .num_cells_to_delete = 1,
      .cell_to_delete[0].nr_cellid = 1234UL,
      .cell_to_delete[0].plmn.mcc = 1,
      .cell_to_delete[0].plmn.mnc = 1,
      .cell_to_delete[0].plmn.mnc_digit_length = 3,
      .num_status = 2,
      .status[0].nr_cellid = 542UL,
      .status[0].plmn.mcc = 1,
      .status[0].plmn.mnc = 2,
      .status[0].plmn.mnc_digit_length = 3,
      .status[0].service_state = F1AP_STATE_IN_SERVICE,
      .status[1].nr_cellid = 33UL,
      .status[1].plmn.mcc = 5,
      .status[1].plmn.mnc = 13,
      .status[1].plmn.mnc_digit_length = 2,
      .status[1].service_state = F1AP_STATE_OUT_OF_SERVICE,
  };
  orig.cell_to_modify[0].sys_info = calloc_or_fail(1, sizeof(*orig.cell_to_modify[0].sys_info));
  *orig.cell_to_modify[0].sys_info = sys_info;
  orig.gNB_DU_ID = malloc_or_fail(sizeof(*orig.gNB_DU_ID));
  *orig.gNB_DU_ID = 12;

  F1AP_F1AP_PDU_t *f1enc = encode_f1ap_du_configuration_update(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_gnb_du_configuration_update_t decoded = {0};
  bool ret = decode_f1ap_du_configuration_update(f1dec, &decoded);
  AssertFatal(ret, "decode_f1ap_setup_request(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_f1ap_du_configuration_update(&orig, &decoded);
  AssertFatal(ret, "eq_f1ap_setup_request(): decoded message doesn't match\n");
  free_f1ap_du_configuration_update(&decoded);

  f1ap_gnb_du_configuration_update_t cp = cp_f1ap_du_configuration_update(&orig);
  ret = eq_f1ap_du_configuration_update(&orig, &cp);
  AssertFatal(ret, "eq_f1ap_setup_request(): copied message doesn't match\n");
  free_f1ap_du_configuration_update(&cp);
  free_f1ap_du_configuration_update(&orig);
}

/**
 * @brief Test F1 gNB-DU Configuration Update Acknowledge
 */
static void test_f1ap_du_configuration_update_acknowledge(void)
{
  // Create message
  f1ap_gnb_du_configuration_update_acknowledge_t orig = {
      .transaction_id = 5,
      .num_cells_to_activate = 1,
      .cells_to_activate = {{
          .nr_cellid = 987654321,
          .nrpci = 50,
          .plmn = {.mcc = 001, .mnc = 01, .mnc_digit_length = 2},
          .num_SI = 1,
          .SI_msg = {{
              .SI_type = 7,
              .SI_container_length = 10,
              .SI_container = malloc(sizeof(uint8_t) * 10),
          }},
      }},
  };
  for (int i = 0; i < orig.cells_to_activate[0].SI_msg[0].SI_container_length; i++) {
    orig.cells_to_activate[0].SI_msg[0].SI_container[i] = i;
  }
  // ASN.1 enc/dec
  F1AP_F1AP_PDU_t *f1enc = encode_f1ap_du_configuration_update_acknowledge(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);
  // Decoding
  f1ap_gnb_du_configuration_update_acknowledge_t decoded = {0};
  bool ret = decode_f1ap_du_configuration_update_acknowledge(f1dec, &decoded);
  AssertFatal(ret, "decode_f1ap_du_configuration_update_acknowledge(): could not decode message\n");
  f1ap_msg_free(f1dec);
  // Equality check
  ret = eq_f1ap_du_configuration_update_acknowledge(&orig, &decoded);
  AssertFatal(ret, "eq_f1ap_du_configuration_update_acknowledge(): decoded message doesn't match\n");
  free_f1ap_du_configuration_update_acknowledge(&decoded);
  // Deep copy
  f1ap_gnb_du_configuration_update_acknowledge_t cp = cp_f1ap_du_configuration_update_acknowledge(&orig);
  ret = eq_f1ap_du_configuration_update_acknowledge(&orig, &cp);
  AssertFatal(ret, "eq_f1ap_du_configuration_update_acknowledge(): copied message doesn't match\n");
  // Free
  free_f1ap_du_configuration_update_acknowledge(&cp);
  free_f1ap_du_configuration_update_acknowledge(&orig);
}

/**
 * @brief Test F1 gNB-CU Configuration Update
 */
static void test_f1ap_cu_configuration_update(void)
{
  /* create message */
  f1ap_gnb_cu_configuration_update_t orig = {.transaction_id = 2,
                                             .num_cells_to_activate = 1,
                                             .cells_to_activate = {{.nr_cellid = 123456789,
                                                                    .nrpci = 100,
                                                                    .plmn = {.mcc = 001, .mnc = 01, .mnc_digit_length = 2},
                                                                    .num_SI = 1,
                                                                    .SI_msg = {{
                                                                        .SI_type = 7,
                                                                        .SI_container_length = 10,
                                                                        .SI_container = malloc(sizeof(uint8_t) * 10),
                                                                    }}}}};
  F1AP_F1AP_PDU_t *f1enc = encode_f1ap_cu_configuration_update(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_gnb_cu_configuration_update_t decoded = {0};
  bool ret = decode_f1ap_cu_configuration_update(f1dec, &decoded);
  AssertFatal(ret, "decode_f1ap_setup_request(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_f1ap_cu_configuration_update(&orig, &decoded);
  AssertFatal(ret, "eq_f1ap_setup_request(): decoded message doesn't match\n");
  free_f1ap_cu_configuration_update(&decoded);

  f1ap_gnb_cu_configuration_update_t cp = cp_f1ap_cu_configuration_update(&orig);
  ret = eq_f1ap_cu_configuration_update(&orig, &cp);
  AssertFatal(ret, "eq_f1ap_setup_request(): copied message doesn't match\n");
  free_f1ap_cu_configuration_update(&cp);
  free_f1ap_cu_configuration_update(&orig);
}

/**
 * @brief Test F1 gNB-CU Configuration Update Acknowledge
 */
static void test_f1ap_cu_configuration_update_acknowledge(void)
{
  // Create the original message
  f1ap_gnb_cu_configuration_update_acknowledge_t orig = {
      .transaction_id = 2,
      .num_cells_failed_to_be_activated = 1,
      .cells_failed_to_be_activated = {{.nr_cellid = 123456789,
                                        .plmn = {.mcc = 001, .mnc = 01, .mnc_digit_length = 2},
                                        .cause = F1AP_CAUSE_RADIO_NETWORK}}
                                        };
  // F1AP Enc/dec
  F1AP_F1AP_PDU_t *f1enc = encode_f1ap_cu_configuration_update_acknowledge(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);
  // Decoding
  f1ap_gnb_cu_configuration_update_acknowledge_t decoded = {0};
  bool ret = decode_f1ap_cu_configuration_update_acknowledge(f1dec, &decoded);
  AssertFatal(ret, "decode_f1ap_cu_configuration_update_acknowledge(): could not decode message\n");
  f1ap_msg_free(f1dec);
  // Equality check
  ret = eq_f1ap_cu_configuration_update_acknowledge(&orig, &decoded);
  AssertFatal(ret, "eq_f1ap_cu_configuration_update_acknowledge(): decoded message doesn't match\n");
  // No free needed
  // Deep copy
  f1ap_gnb_cu_configuration_update_acknowledge_t cp = cp_f1ap_cu_configuration_update_acknowledge(&orig);
  // Equality check
  ret = eq_f1ap_cu_configuration_update_acknowledge(&orig, &cp);
  AssertFatal(ret, "eq_f1ap_cu_configuration_update_acknowledge(): copied message doesn't match\n");
  // No free needed
}

static byte_array_t get_test_ba(const char *s)
{
  byte_array_t ba = {
      .len = strlen(s) + 1, /* binary data: needs to include null-byte of strdup() */
      .buf = (uint8_t *) strdup(s),
  };
  return ba;
}

static byte_array_t *get_malloced_test_ba(const char *s)
{
  byte_array_t *ba = calloc_or_fail(1, sizeof(*ba));
  *ba = get_test_ba(s);
  return ba;
}

static void test_f1ap_ue_context_setup_request()
{
  plmn_id_t plmn = { .mcc = 001, .mnc = 01, .mnc_digit_length = 2 };
  f1ap_ue_context_setup_req_t orig = {
    .gNB_CU_ue_id = 12,
    //.gNB_DU_ue_id below
    .plmn = plmn,
    .nr_cellid = 132,
    .servCellIndex = 1,
    //.cu_to_du_rrc_info below
    //.srbs below
    //.drbs below
    //gnb_du_ue_agg_mbr_ul below
  };
  _F1_MALLOC(orig.gNB_DU_ue_id, 13);
  orig.cu_to_du_rrc_info.cg_configinfo = get_malloced_test_ba("CG-CONFIGINFO");
  orig.cu_to_du_rrc_info.ue_cap = get_malloced_test_ba("UE CAP");
  orig.cu_to_du_rrc_info.meas_config = get_malloced_test_ba("UE MEASUREMENT CONFIGURATION");
  orig.cu_to_du_rrc_info.meas_timing_config = get_malloced_test_ba("MEASUREMENT TIMING CONFIGURATION");
  orig.cu_to_du_rrc_info.ho_prep_info = get_malloced_test_ba("PREPARATION DATA FOR HANDOVER");
  orig.srbs_len = 2;
  orig.srbs = calloc_or_fail(orig.srbs_len, sizeof(*orig.srbs));
  for (int i = 0; i < orig.srbs_len; ++i)
    orig.srbs[i].id = 1 + i;

  orig.drbs_len = 2;
  orig.drbs = calloc_or_fail(orig.drbs_len, sizeof(*orig.drbs));
  f1ap_drb_to_setup_t *drb1 = &orig.drbs[0];
  drb1->id = 4;
  drb1->qos_choice = F1AP_QOS_CHOICE_NR;
  f1ap_arp_t arp = { 1, MAY_TRIGGER_PREEMPTION, PREEMPTABLE, };
  drb1->nr.drb_qos.qos_type = NON_DYNAMIC;
  drb1->nr.drb_qos.nondyn.fiveQI = 3;
  drb1->nr.drb_qos.arp = arp;
  drb1->nr.nssai = (nssai_t) {.sst = 1, .sd = 123};
  drb1->nr.flows_len = 1;
  f1ap_drb_flows_mapped_t *flow = drb1->nr.flows = calloc_or_fail(drb1->nr.flows_len, sizeof(*flow));
  flow->qfi = 2;
  flow->param.qos_type = NON_DYNAMIC;
  flow->param.nondyn.fiveQI = 3;
  flow->param.arp = arp;
  drb1->up_ul_tnl_len = 1;
  inet_pton(AF_INET, "192.168.40.23", &drb1->up_ul_tnl[0].tl_address);
  drb1->up_ul_tnl[0].teid = 0x11223344;
  drb1->rlc_mode = F1AP_RLC_MODE_UM_BIDIR;
  _F1_MALLOC(drb1->dl_pdcp_sn_len, F1AP_PDCP_SN_18B);
  _F1_MALLOC(drb1->ul_pdcp_sn_len, F1AP_PDCP_SN_18B);

  f1ap_drb_to_setup_t *drb2 = &orig.drbs[1];
  drb2->id = 12;
  drb2->qos_choice = F1AP_QOS_CHOICE_NR;
  drb2->nr.drb_qos.qos_type = DYNAMIC;
  f1ap_dynamic_5qi_t *dyn = &drb2->nr.drb_qos.dyn;
  dyn->prio = 4;
  dyn->pdb = 1000;
  dyn->per.scalar = 4;
  dyn->per.exponent = 8;
  _F1_MALLOC(dyn->delay_critical, true);
  _F1_MALLOC(dyn->avg_win, 3000);
  f1ap_arp_t arp2 = { 13, SHALL_NOT_TRIGGER_PREEMPTION, NOT_PREEMPTABLE, };
  drb2->nr.drb_qos.arp = arp2;
  drb2->nr.nssai = (nssai_t) {.sst = 2, .sd = 0xffffff};
  drb2->nr.flows_len = 2;
  f1ap_drb_flows_mapped_t *flow2 = drb2->nr.flows = calloc_or_fail(drb2->nr.flows_len, sizeof(*flow2));
  flow2[0].qfi = 4;
  flow2[0].param.qos_type = DYNAMIC;
  flow2[0].param.dyn.prio = 4;
  flow2[0].param.dyn.pdb = 999;
  flow2[0].param.dyn.per.scalar = 4;
  flow2[0].param.dyn.per.exponent = 8;
  flow2[0].param.arp = arp2;
  flow2[1].qfi = 5;
  flow2[1].param.qos_type = NON_DYNAMIC;
  flow2[1].param.nondyn.fiveQI = 3;
  flow2[1].param.arp = arp;
  drb2->up_ul_tnl_len = 2;
  inet_pton(AF_INET, "10.0.0.2", &drb2->up_ul_tnl[0].tl_address);
  drb2->up_ul_tnl[0].teid = 0x1234;
  inet_pton(AF_INET, "10.0.0.2", &drb2->up_ul_tnl[1].tl_address);
  drb2->up_ul_tnl[1].teid = 0x1235;
  drb2->rlc_mode = F1AP_RLC_MODE_AM;
  _F1_MALLOC(drb2->dl_pdcp_sn_len, F1AP_PDCP_SN_12B);
  _F1_MALLOC(drb2->ul_pdcp_sn_len, F1AP_PDCP_SN_12B);

  orig.rrc_container = get_malloced_test_ba("RRC container");

  _F1_MALLOC(orig.gnb_du_ue_agg_mbr_ul, 5LL * 1000 * 1000 * 1000);

  F1AP_F1AP_PDU_t *f1enc = encode_ue_context_setup_req(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_ue_context_setup_req_t decoded = {0};
  bool ret = decode_ue_context_setup_req(f1dec, &decoded);
  AssertFatal(ret, "decode_ue_context_setup_req(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_ue_context_setup_req(&orig, &decoded);
  AssertFatal(ret, "eq_ue_context_setup_req(): decoded message doesn't match\n");
  char ip_ret[100];
  inet_ntop(AF_INET, &decoded.drbs[0].up_ul_tnl[0].tl_address, ip_ret, sizeof(ip_ret));
  printf("%s\n", ip_ret);
  inet_ntop(AF_INET, &decoded.drbs[1].up_ul_tnl[0].tl_address, ip_ret, sizeof(ip_ret));
  printf("%s\n", ip_ret);
  free_ue_context_setup_req(&decoded);

  f1ap_ue_context_setup_req_t cp = cp_ue_context_setup_req(&orig);
  ret = eq_ue_context_setup_req(&orig, &cp);
  AssertFatal(ret, "eq_ue_context_setup_req(): copied message doesn't match\n");
  free_ue_context_setup_req(&orig);
  free_ue_context_setup_req(&cp);

  printf("%s() successful\n", __func__);
}

static void test_f1ap_ue_context_setup_request_simple()
{
  plmn_id_t plmn = { .mcc = 001, .mnc = 01, .mnc_digit_length = 2 };
  f1ap_ue_context_setup_req_t orig = {
    .gNB_CU_ue_id = 12,
    .plmn = plmn,
    .nr_cellid = 132,
    .servCellIndex = 1,
    //.cu_to_du_rrc_info: all fields optional, intentionally left empty
    // rest is optional and intentionally left empty
    // .gnb_du_ue_agg_mbr_ul is C-ifDRBSetup => no DRB => no IE
  };
  F1AP_F1AP_PDU_t *f1enc = encode_ue_context_setup_req(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_ue_context_setup_req_t decoded = {0};
  bool ret = decode_ue_context_setup_req(f1dec, &decoded);
  AssertFatal(ret, "decode_ue_context_setup_req(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_ue_context_setup_req(&orig, &decoded);
  AssertFatal(ret, "eq_ue_context_setup_req(): decoded message doesn't match\n");
  free_ue_context_setup_req(&decoded);

  f1ap_ue_context_setup_req_t cp = cp_ue_context_setup_req(&orig);
  ret = eq_ue_context_setup_req(&orig, &cp);
  AssertFatal(ret, "eq_ue_context_setup_req(): copied message doesn't match\n");
  free_ue_context_setup_req(&orig);
  free_ue_context_setup_req(&cp);

  printf("%s() successful\n", __func__);
}

static void test_f1ap_ue_context_setup_response()
{
  f1ap_ue_context_setup_resp_t orig = {
    .gNB_CU_ue_id = 12,
    .gNB_DU_ue_id = 13,
    //.du_to_cu_rrc_info below
    //.crnti below
    //.drbs below
    //.srbs below
  };

  orig.du_to_cu_rrc_info.cell_group_config = get_test_ba("CELL GROUP CONFIG");
  orig.du_to_cu_rrc_info.meas_gap_config = get_malloced_test_ba("MGC");

  _F1_MALLOC(orig.crnti, 0x1234);

  orig.drbs_len = 2;
  orig.drbs = calloc_or_fail(orig.drbs_len, sizeof(*orig.drbs));
  f1ap_drb_setup_t *drb1 = &orig.drbs[0];
  drb1->id = 12;
  _F1_MALLOC(drb1->lcid, 12);
  drb1->up_dl_tnl_len = 2;
  inet_pton(AF_INET, "192.168.40.23", &drb1->up_dl_tnl[0].tl_address);
  drb1->up_dl_tnl[0].teid = 0x11223344;
  inet_pton(AF_INET, "192.168.40.23", &drb1->up_dl_tnl[1].tl_address);
  drb1->up_dl_tnl[1].teid = 0x11223345;
  f1ap_drb_setup_t *drb2 = &orig.drbs[1];
  drb2->id = 13;
  _F1_MALLOC(drb2->lcid, 13);
  drb2->up_dl_tnl_len = 2;
  inet_pton(AF_INET, "192.168.50.23", &drb2->up_dl_tnl[0].tl_address);
  drb2->up_dl_tnl[0].teid = 0x21223354;
  inet_pton(AF_INET, "192.168.50.23", &drb2->up_dl_tnl[1].tl_address);
  drb2->up_dl_tnl[1].teid = 0x21223355;

  orig.srbs_len = 2;
  orig.srbs = calloc_or_fail(orig.srbs_len, sizeof(*orig.srbs));
  for (int i = 0; i < orig.srbs_len; ++i) {
    orig.srbs[i].id = 1 + i;
    orig.srbs[i].lcid = 1+i+10;
  }

  F1AP_F1AP_PDU_t *f1enc = encode_ue_context_setup_resp(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_ue_context_setup_resp_t decoded = {0};
  bool ret = decode_ue_context_setup_resp(f1dec, &decoded);
  AssertFatal(ret, "decode_ue_context_setup_resp(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_ue_context_setup_resp(&orig, &decoded);
  AssertFatal(ret, "eq_ue_context_setup_resp(): decoded message doesn't match\n");
  free_ue_context_setup_resp(&decoded);

  f1ap_ue_context_setup_resp_t cp = cp_ue_context_setup_resp(&orig);
  ret = eq_ue_context_setup_resp(&orig, &cp);
  AssertFatal(ret, "eq_ue_context_setup_resp(): copied message doesn't match\n");
  free_ue_context_setup_resp(&orig);
  free_ue_context_setup_resp(&cp);

  printf("%s() successful\n", __func__);
}

static void test_f1ap_ue_context_setup_response_simple()
{
  f1ap_ue_context_setup_resp_t orig = {
    .gNB_CU_ue_id = 12,
    .gNB_DU_ue_id = 13,
    .du_to_cu_rrc_info.cell_group_config = get_test_ba("small cg"),
    // rest is optional and intentionally left empty
  };

  F1AP_F1AP_PDU_t *f1enc = encode_ue_context_setup_resp(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_ue_context_setup_resp_t decoded = {0};
  bool ret = decode_ue_context_setup_resp(f1dec, &decoded);
  AssertFatal(ret, "decode_ue_context_setup_resp(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_ue_context_setup_resp(&orig, &decoded);
  AssertFatal(ret, "eq_ue_context_setup_resp(): decoded message doesn't match\n");
  free_ue_context_setup_resp(&decoded);

  f1ap_ue_context_setup_resp_t cp = cp_ue_context_setup_resp(&orig);
  ret = eq_ue_context_setup_resp(&orig, &cp);
  AssertFatal(ret, "eq_ue_context_setup_resp(): copied message doesn't match\n");
  free_ue_context_setup_resp(&orig);
  free_ue_context_setup_resp(&cp);

  printf("%s() successful\n", __func__);
}

static void test_f1ap_ue_context_modification_request()
{
  f1ap_ue_context_mod_req_t orig = {
    .gNB_CU_ue_id = 1111,
    .gNB_DU_ue_id = 2222,
    // rest below
  };
  plmn_id_t plmn = { .mcc = 999, .mnc = 101, .mnc_digit_length = 3 };
  _F1_MALLOC(orig.plmn, plmn);
  _F1_MALLOC(orig.nr_cellid, 1532);
  _F1_MALLOC(orig.servCellIndex, 3);

  f1ap_cu_to_du_rrc_info_t cu2du = {
    .cg_configinfo = get_malloced_test_ba("Modified"),
    .ue_cap = get_malloced_test_ba("UE forgot to send earlier"),
    .meas_config = get_malloced_test_ba("with updates from another cell"),
    .meas_timing_config = get_malloced_test_ba("measurement timing configuration"),
    .ho_prep_info = get_malloced_test_ba("prepare to ho soon!"),
  };
  _F1_MALLOC(orig.cu_to_du_rrc_info, cu2du);

  orig.rrc_container = get_malloced_test_ba("important RRC message!!");

  orig.srbs_len = 1;
  orig.srbs = calloc_or_fail(orig.srbs_len, sizeof(*orig.srbs));
  orig.srbs[0].id = 1;

  orig.drbs_len = 1;
  orig.drbs = calloc_or_fail(orig.drbs_len, sizeof(*orig.drbs));
  f1ap_drb_to_setup_t *drb1 = &orig.drbs[0];
  drb1->id = 4;
  drb1->qos_choice = F1AP_QOS_CHOICE_NR;
  f1ap_arp_t arp = { 2, SHALL_NOT_TRIGGER_PREEMPTION, NOT_PREEMPTABLE, };
  drb1->nr.drb_qos.qos_type = NON_DYNAMIC;
  drb1->nr.drb_qos.nondyn.fiveQI = 8;
  drb1->nr.drb_qos.arp = arp;
  drb1->nr.nssai = (nssai_t) {.sst = 2, .sd = 0xffffff};
  drb1->nr.flows_len = 1;
  f1ap_drb_flows_mapped_t *flow = drb1->nr.flows = calloc_or_fail(drb1->nr.flows_len, sizeof(*flow));
  flow->qfi = 2;
  flow->param.qos_type = NON_DYNAMIC;
  flow->param.nondyn.fiveQI = 9;
  flow->param.arp = arp;
  drb1->up_ul_tnl_len = 1;
  inet_pton(AF_INET, "8.8.8.8", &drb1->up_ul_tnl[0].tl_address);
  drb1->up_ul_tnl[0].teid = 0x9876541;
  drb1->rlc_mode = F1AP_RLC_MODE_AM;
  _F1_MALLOC(drb1->dl_pdcp_sn_len, F1AP_PDCP_SN_12B);
  _F1_MALLOC(drb1->ul_pdcp_sn_len, F1AP_PDCP_SN_18B);

  orig.drbs_rel_len = 3;
  orig.drbs_rel = calloc_or_fail(orig.drbs_rel_len, sizeof(*orig.drbs_rel));
  orig.drbs_rel[0].id = 13;
  orig.drbs_rel[1].id = 4;
  orig.drbs_rel[2].id = 19;

  _F1_MALLOC(orig.status, LOWER_LAYERS_RESUME);

  F1AP_F1AP_PDU_t *f1enc = encode_ue_context_mod_req(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_ue_context_mod_req_t decoded = {0};
  bool ret = decode_ue_context_mod_req(f1dec, &decoded);
  AssertFatal(ret, "decode_ue_context_mod_req(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_ue_context_mod_req(&orig, &decoded);
  AssertFatal(ret, "eq_ue_context_mod_req(): decoded message doesn't match\n");
  free_ue_context_mod_req(&decoded);

  f1ap_ue_context_mod_req_t cp = cp_ue_context_mod_req(&orig);
  ret = eq_ue_context_mod_req(&orig, &cp);
  AssertFatal(ret, "eq_ue_context_mod_req(): copied message doesn't match\n");
  free_ue_context_mod_req(&orig);
  free_ue_context_mod_req(&cp);

  printf("%s() successful\n", __func__);
}

static void test_f1ap_ue_context_modification_request_simple()
{
  f1ap_ue_context_mod_req_t orig = {
    .gNB_CU_ue_id = 1111,
    .gNB_DU_ue_id = 2222,
    // rest is optional and intentionally left out
  };

  F1AP_F1AP_PDU_t *f1enc = encode_ue_context_mod_req(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_ue_context_mod_req_t decoded = {0};
  bool ret = decode_ue_context_mod_req(f1dec, &decoded);
  AssertFatal(ret, "decode_ue_context_mod_req(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_ue_context_mod_req(&orig, &decoded);
  AssertFatal(ret, "eq_ue_context_mod_req(): decoded message doesn't match\n");
  free_ue_context_mod_req(&decoded);

  f1ap_ue_context_mod_req_t cp = cp_ue_context_mod_req(&orig);
  ret = eq_ue_context_mod_req(&orig, &cp);
  AssertFatal(ret, "eq_ue_context_mod_req(): copied message doesn't match\n");
  free_ue_context_mod_req(&orig);
  free_ue_context_mod_req(&cp);

  printf("%s() successful\n", __func__);
}

static void test_f1ap_ue_context_modification_response()
{
  f1ap_ue_context_mod_resp_t orig = {
    .gNB_CU_ue_id = 9876,
    .gNB_DU_ue_id = 5432,
    // rest below
  };

  f1ap_du_to_cu_rrc_info_t cu2du = {
    .cell_group_config = get_test_ba("update for the cellgroupconfig, extra long"),
    .meas_gap_config = get_malloced_test_ba("new and shiny MeAs GaP cOnFiG"),
  };
  _F1_MALLOC(orig.du_to_cu_rrc_info, cu2du);

  orig.drbs_len = 1;
  orig.drbs = calloc_or_fail(orig.drbs_len, sizeof(*orig.drbs));
  f1ap_drb_setup_t *drb1 = &orig.drbs[0];
  drb1->id = 4;
  _F1_MALLOC(drb1->lcid, 7);
  drb1->up_dl_tnl_len = 1;
  inet_pton(AF_INET, "127.0.0.1", &drb1->up_dl_tnl[0].tl_address);
  drb1->up_dl_tnl[0].teid = 0xcafe;

  orig.srbs_len = 2;
  orig.srbs = calloc_or_fail(orig.srbs_len, sizeof(*orig.srbs));
  orig.srbs[0].id = 1;
  orig.srbs[0].lcid = 1;
  orig.srbs[1].id = 2;
  orig.srbs[1].lcid = 2;

  F1AP_F1AP_PDU_t *f1enc = encode_ue_context_mod_resp(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_ue_context_mod_resp_t decoded = {0};
  bool ret = decode_ue_context_mod_resp(f1dec, &decoded);
  AssertFatal(ret, "decode_ue_context_mod_resp(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_ue_context_mod_resp(&orig, &decoded);
  AssertFatal(ret, "eq_ue_context_mod_resp(): decoded message doesn't match\n");
  free_ue_context_mod_resp(&decoded);

  f1ap_ue_context_mod_resp_t cp = cp_ue_context_mod_resp(&orig);
  ret = eq_ue_context_mod_resp(&orig, &cp);
  AssertFatal(ret, "eq_ue_context_mod_resp(): copied message doesn't match\n");
  free_ue_context_mod_resp(&orig);
  free_ue_context_mod_resp(&cp);

  printf("%s() successful\n", __func__);
}

static void test_f1ap_ue_context_modification_response_simple()
{
  f1ap_ue_context_mod_resp_t orig = {
    .gNB_CU_ue_id = 9876,
    .gNB_DU_ue_id = 5432,
    // rest is optional and intentionally left out
  };

  F1AP_F1AP_PDU_t *f1enc = encode_ue_context_mod_resp(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_ue_context_mod_resp_t decoded = {0};
  bool ret = decode_ue_context_mod_resp(f1dec, &decoded);
  AssertFatal(ret, "decode_ue_context_mod_resp(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_ue_context_mod_resp(&orig, &decoded);
  AssertFatal(ret, "eq_ue_context_mod_resp(): decoded message doesn't match\n");
  free_ue_context_mod_resp(&decoded);

  f1ap_ue_context_mod_resp_t cp = cp_ue_context_mod_resp(&orig);
  ret = eq_ue_context_mod_resp(&orig, &cp);
  AssertFatal(ret, "eq_ue_context_mod_resp(): copied message doesn't match\n");
  free_ue_context_mod_resp(&orig);
  free_ue_context_mod_resp(&cp);

  printf("%s() successful\n", __func__);
}

static void test_f1ap_ue_context_release_request()
{
  f1ap_ue_context_rel_req_t orig = {
    .gNB_CU_ue_id = 1111,
    .gNB_DU_ue_id = 2222,
    .cause = F1AP_CAUSE_MISC,
    .cause_value = 3,
  };

  F1AP_F1AP_PDU_t *f1enc = encode_ue_context_rel_req(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_ue_context_rel_req_t decoded = {0};
  bool ret = decode_ue_context_rel_req(f1dec, &decoded);
  AssertFatal(ret, "decode_ue_context_rel_req(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_ue_context_rel_req(&orig, &decoded);
  AssertFatal(ret, "eq_ue_context_rel_req(): decoded message doesn't match\n");
  free_ue_context_rel_req(&decoded);

  f1ap_ue_context_rel_req_t cp = cp_ue_context_rel_req(&orig);
  ret = eq_ue_context_rel_req(&orig, &cp);
  AssertFatal(ret, "eq_ue_context_rel_req(): copied message doesn't match\n");
  free_ue_context_rel_req(&orig);
  free_ue_context_rel_req(&cp);

  printf("%s() successful\n", __func__);
}

static void test_f1ap_ue_context_release_command()
{
  f1ap_ue_context_rel_cmd_t orig = {
    .gNB_CU_ue_id = 1111,
    .gNB_DU_ue_id = 2222,
    .cause = F1AP_CAUSE_RADIO_NETWORK,
    .cause_value = 2,
  };

  orig.rrc_container = get_malloced_test_ba("some reject message");
  _F1_MALLOC(orig.srb_id, 1);
  _F1_MALLOC(orig.old_gNB_DU_ue_id, 3333);

  F1AP_F1AP_PDU_t *f1enc = encode_ue_context_rel_cmd(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_ue_context_rel_cmd_t decoded = {0};
  bool ret = decode_ue_context_rel_cmd(f1dec, &decoded);
  AssertFatal(ret, "decode_ue_context_rel_cmd(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_ue_context_rel_cmd(&orig, &decoded);
  AssertFatal(ret, "eq_ue_context_rel_cmd(): decoded message doesn't match\n");
  free_ue_context_rel_cmd(&decoded);

  f1ap_ue_context_rel_cmd_t cp = cp_ue_context_rel_cmd(&orig);
  ret = eq_ue_context_rel_cmd(&orig, &cp);
  AssertFatal(ret, "eq_ue_context_rel_cmd(): copied message doesn't match\n");
  free_ue_context_rel_cmd(&orig);
  free_ue_context_rel_cmd(&cp);

  printf("%s() successful\n", __func__);
}

static void test_f1ap_ue_context_release_complete()
{
  f1ap_ue_context_rel_cplt_t orig = {
    .gNB_CU_ue_id = 1111,
    .gNB_DU_ue_id = 2222,
  };

  F1AP_F1AP_PDU_t *f1enc = encode_ue_context_rel_cplt(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_ue_context_rel_cplt_t decoded = {0};
  bool ret = decode_ue_context_rel_cplt(f1dec, &decoded);
  AssertFatal(ret, "decode_ue_context_rel_cplt(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_ue_context_rel_cplt(&orig, &decoded);
  AssertFatal(ret, "eq_ue_context_rel_cplt(): decoded message doesn't match\n");
  free_ue_context_rel_cplt(&decoded);

  f1ap_ue_context_rel_cplt_t cp = cp_ue_context_rel_cplt(&orig);
  ret = eq_ue_context_rel_cplt(&orig, &cp);
  AssertFatal(ret, "eq_ue_context_rel_cplt(): copied message doesn't match\n");
  free_ue_context_rel_cplt(&orig);
  free_ue_context_rel_cplt(&cp);

  printf("%s() successful\n", __func__);
}

int main()
{
  test_initial_ul_rrc_message_transfer();
  test_dl_rrc_message_transfer();
  test_ul_rrc_message_transfer();
  test_f1ap_setup_request();
  test_f1ap_setup_response();
  test_f1ap_setup_failure();
  test_f1ap_reset_all();
  test_f1ap_reset_part();
  test_f1ap_reset_ack();
  test_f1ap_du_configuration_update();
  test_f1ap_cu_configuration_update();
  test_f1ap_cu_configuration_update_acknowledge();
  test_f1ap_du_configuration_update_acknowledge();
  test_f1ap_ue_context_setup_request();
  test_f1ap_ue_context_setup_request_simple();
  test_f1ap_ue_context_setup_response();
  test_f1ap_ue_context_setup_response_simple();
  test_f1ap_ue_context_modification_request();
  test_f1ap_ue_context_modification_request_simple();
  test_f1ap_ue_context_modification_response();
  test_f1ap_ue_context_modification_response_simple();
  test_f1ap_ue_context_release_request();
  test_f1ap_ue_context_release_command();
  test_f1ap_ue_context_release_complete();
  return 0;
}
