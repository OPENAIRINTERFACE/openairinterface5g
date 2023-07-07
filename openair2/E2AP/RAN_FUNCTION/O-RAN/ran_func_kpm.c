#include "ran_func_kpm.h"
#include "../../flexric/test/rnd/fill_rnd_data_kpm.h"
#include "../../flexric/src/util/time_now_us.h"
#include <assert.h>
#include <stdio.h>

static 
gnb_e2sm_t fill_gnb_data(void)
{
  gnb_e2sm_t gnb = {0};

  // 6.2.3.16
  // Mandatory
  // AMF UE NGAP ID
  gnb.amf_ue_ngap_id = 112358132134; // % 2^40;

  // Mandatory
  //GUAMI 6.2.3.17 
  gnb.guami.plmn_id = (e2sm_plmn_t) {.mcc = 505, .mnc = 1, .mnc_digit_len = 2};
  
  gnb.guami.amf_region_id = (rand() % 2^8) + 0;
  gnb.guami.amf_set_id = (rand() % 2^10) + 0;
  gnb.guami.amf_ptr = (rand() % 2^6) + 0;

  return gnb;
}

static 
ue_id_e2sm_t fill_ue_id_data(void)
{
  ue_id_e2sm_t ue_id_data = {0};

  ue_id_data.type = GNB_UE_ID_E2SM;
  ue_id_data.gnb = fill_gnb_data();

  return ue_id_data;
}

static 
kpm_ind_msg_format_1_t fill_kpm_ind_msg_frm_1(void)
{
  kpm_ind_msg_format_1_t msg_frm_1 = {0};

  // Measurement Data
  uint32_t num_drbs = 2;
  msg_frm_1.meas_data_lst_len = num_drbs;  // (rand() % 65535) + 1;
  msg_frm_1.meas_data_lst = calloc(msg_frm_1.meas_data_lst_len, sizeof(*msg_frm_1.meas_data_lst));
  assert(msg_frm_1.meas_data_lst != NULL && "Memory exhausted" );

  for (size_t i = 0; i < msg_frm_1.meas_data_lst_len; i++){
    // Measurement Record
    msg_frm_1.meas_data_lst[i].meas_record_len = 1;  // (rand() % 65535) + 1;
    msg_frm_1.meas_data_lst[i].meas_record_lst = calloc(msg_frm_1.meas_data_lst[i].meas_record_len, sizeof(meas_record_lst_t));
    assert(msg_frm_1.meas_data_lst[i].meas_record_lst != NULL && "Memory exhausted" );

    for (size_t j = 0; j < msg_frm_1.meas_data_lst[i].meas_record_len; j++){
      msg_frm_1.meas_data_lst[i].meas_record_lst[j].value = REAL_MEAS_VALUE; // rand()%END_MEAS_VALUE;
      msg_frm_1.meas_data_lst[i].meas_record_lst[j].real_val = (rand() % 256) + 0.1;
    }
  }

  // Measurement Information - OPTIONAL
  msg_frm_1.meas_info_lst_len = num_drbs;
  msg_frm_1.meas_info_lst = calloc(msg_frm_1.meas_info_lst_len, sizeof(meas_info_format_1_lst_t));
  assert(msg_frm_1.meas_info_lst != NULL && "Memory exhausted" );

  for (size_t i = 0; i < msg_frm_1.meas_info_lst_len; i++) {
    // Measurement Type
    msg_frm_1.meas_info_lst[i].meas_type.type = ID_MEAS_TYPE; 
    // DRB ID
    msg_frm_1.meas_info_lst[i].meas_type.id = i;

    // Label Information
    msg_frm_1.meas_info_lst[i].label_info_lst_len = 1;
    msg_frm_1.meas_info_lst[i].label_info_lst = calloc(msg_frm_1.meas_info_lst[i].label_info_lst_len, sizeof(label_info_lst_t));
    assert(msg_frm_1.meas_info_lst[i].label_info_lst != NULL && "Memory exhausted" );

    for (size_t j = 0; j < msg_frm_1.meas_info_lst[i].label_info_lst_len; j++) {
      msg_frm_1.meas_info_lst[i].label_info_lst[j].noLabel = malloc(sizeof(enum_value_e));
      *msg_frm_1.meas_info_lst[i].label_info_lst[j].noLabel = TRUE_ENUM_VALUE;
    }
  }

  return msg_frm_1;
}

static 
kpm_ind_msg_format_3_t fill_kpm_ind_msg_frm_3_sta(void)
{
  kpm_ind_msg_format_3_t msg_frm_3 = {0};

  uint32_t num_ues = 1;
  msg_frm_3.ue_meas_report_lst_len = num_ues;  // (rand() % 65535) + 1;

  msg_frm_3.meas_report_per_ue = calloc(msg_frm_3.ue_meas_report_lst_len, sizeof(meas_report_per_ue_t));
  assert(msg_frm_3.meas_report_per_ue != NULL && "Memory exhausted");

  for (size_t i = 0; i < msg_frm_3.ue_meas_report_lst_len; i++)
  {
    msg_frm_3.meas_report_per_ue[i].ue_meas_report_lst = fill_ue_id_data();
    msg_frm_3.meas_report_per_ue[i].ind_msg_format_1 = fill_kpm_ind_msg_frm_1();
  }

  return msg_frm_3;
}

static 
kpm_ric_ind_hdr_format_1_t fill_kpm_ind_hdr_frm_1(void)
{
  kpm_ric_ind_hdr_format_1_t hdr_frm_1 = {0};

  hdr_frm_1.collectStartTime = time_now_us();
  
  hdr_frm_1.fileformat_version = NULL;
  
  hdr_frm_1.sender_name = calloc(1, sizeof(byte_array_t));
  hdr_frm_1.sender_name->buf = calloc(strlen("My OAI-MONO") + 1, sizeof(char));
  memcpy(hdr_frm_1.sender_name->buf, "My OAI-MONO", strlen("My OAI-MONO"));
  hdr_frm_1.sender_name->len = strlen("My OAI-MONO");
  
  hdr_frm_1.sender_type = calloc(1, sizeof(byte_array_t));
  hdr_frm_1.sender_type->buf = calloc(strlen("MONO") + 1, sizeof(char));
  memcpy(hdr_frm_1.sender_type->buf, "MONO", strlen("MONO"));
  hdr_frm_1.sender_type->len = strlen("MONO");
  
  hdr_frm_1.vendor_name = calloc(1, sizeof(byte_array_t));
  hdr_frm_1.vendor_name->buf = calloc(strlen("OAI") + 1, sizeof(char));
  memcpy(hdr_frm_1.vendor_name->buf, "OAI", strlen("OAI"));
  hdr_frm_1.vendor_name->len = strlen("OAI");

  return hdr_frm_1;
}

static
kpm_ind_hdr_t fill_kpm_ind_hdr_sta(void)
{
  kpm_ind_hdr_t hdr = {0};

  hdr.type = FORMAT_1_INDICATION_HEADER;
  hdr.kpm_ric_ind_hdr_format_1 = fill_kpm_ind_hdr_frm_1();

  return hdr;
}

void read_kpm_sm(void* data)
{
  assert(data != NULL);
  //assert(data->type == KPM_STATS_V3_0);

  kpm_rd_ind_data_t* kpm = (kpm_rd_ind_data_t*)data;

  assert(kpm->act_def!= NULL && "Cannot be NULL");
  if(kpm->act_def->type == FORMAT_4_ACTION_DEFINITION){

    if(kpm->act_def->frm_4.matching_cond_lst[0].test_info_lst.test_cond_type == CQI_TEST_COND_TYPE
        && *kpm->act_def->frm_4.matching_cond_lst[0].test_info_lst.test_cond == GREATERTHAN_TEST_COND){
      printf("Matching condition: UEs with CQI greater than %ld \n", *kpm->act_def->frm_4.matching_cond_lst[0].test_info_lst.int_value );
    }

    printf("Parameter to report: %s \n", kpm->act_def->frm_4.action_def_format_1.meas_info_lst->meas_type.name.buf); 

    kpm->ind.hdr = fill_kpm_ind_hdr_sta(); 
    // 7.8 Supported RIC Styles and E2SM IE Formats
    // Format 4 corresponds to indication message 3
    kpm->ind.msg.type = FORMAT_3_INDICATION_MESSAGE;
    kpm->ind.msg.frm_3 = fill_kpm_ind_msg_frm_3_sta();
  } else {
     kpm->ind.hdr = fill_kpm_ind_hdr(); 
     kpm->ind.msg = fill_kpm_ind_msg(); 
  }
}

void read_kpm_setup_sm(void* e2ap)
{
  assert(e2ap != NULL);
//  assert(e2ap->type == KPM_V3_0_AGENT_IF_E2_SETUP_ANS_V0);

  kpm_e2_setup_t* kpm = (kpm_e2_setup_t*)(e2ap);
  kpm->ran_func_def = fill_kpm_ran_func_def(); 
}

sm_ag_if_ans_t write_ctrl_kpm_sm(void const* src)
{
  assert(0 !=0 && "Not supported");
  (void)src;
  sm_ag_if_ans_t ans = {0};
  return ans;
}

