#include "ran_func_kpm.h"
#include "openair2/E2AP/flexric/test/rnd/fill_rnd_data_kpm.h"
#include "openair2/E2AP/flexric/src/util/time_now_us.h"
#include "openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "openair3/NGAP/ngap_gNB_ue_context.h"
#include "openair2/LAYER2/nr_pdcp/nr_pdcp_entity.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_entity.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "openair2/LAYER2/nr_pdcp/nr_pdcp_oai_api.h"
#include <assert.h>
#include <stdio.h>

static
size_t get_number_connected_ues(void)
{
  size_t num_ues = 0;

  for (int i = 0; i < RC.nb_nr_macrlc_inst; i++) {
    UE_iterator(RC.nrmac[i]->UE_info.list, ue)
    {
      if (ue) num_ues += 1;
    }    
  }

  return num_ues;
}

static
NR_UE_info_t * connected_ues_list(void)
{
  size_t num_ues = get_number_connected_ues();

  NR_UE_info_t * ue_list = calloc(num_ues, sizeof(NR_UE_info_t));
  assert(ue_list != NULL && "Memory exhausted");

  size_t j = 0;

  for (int i = 0; i < RC.nb_nr_macrlc_inst; i++) {
    UE_iterator(RC.nrmac[i]->UE_info.list, ue)
    {
      if (ue)
      {
        ue_list[j] = *ue;
        j++;
      }
    }    
  }

  return ue_list;
}

static
nr_rlc_statistics_t get_rlc_stats_per_drb(NR_UE_info_t * const UE, int const rb_id)
{
  assert(UE != NULL);

  nr_rlc_statistics_t rlc = {0};
  const int srb_flag = 0;

  // Get RLC stats for specific DRB
  const bool rc = nr_rlc_get_statistics(UE->rnti, srb_flag, rb_id, &rlc);
  assert(rc == true && "Cannot get RLC stats\n");

  // Activate average sojourn time at the RLC buffer for specific DRB
  nr_rlc_activate_avg_time_to_tx(UE->rnti, rb_id+3, 1);
  
  return rlc;  
}

static
nr_pdcp_statistics_t get_pdcp_stats_per_drb(NR_UE_info_t * const UE, int const rb_id)
{
  assert(UE != NULL);

  nr_pdcp_statistics_t pdcp = {0};
  const int srb_flag = 0;

  // Get PDCP stats for specific DRB
  const bool rc = nr_pdcp_get_statistics(UE->rnti, srb_flag, rb_id, &pdcp);
  assert(rc == true && "Cannot get PDCP stats\n");

  return pdcp;
}

static 
gnb_e2sm_t fill_gnb_data(rrc_gNB_ue_context_t * ue_context_p)
{
  gnb_e2sm_t gnb = {0};

  // 6.2.3.16
  // Mandatory
  // AMF UE NGAP ID
  // fill with openair3/NGAP/ngap_gNB_ue_context.h:61
  gnb.amf_ue_ngap_id = ue_context_p->ue_context.gNB_ue_ngap_id;

  // Mandatory
  //GUAMI 6.2.3.17 
  gnb.guami.plmn_id = (e2sm_plmn_t) {
                                    .mcc = ue_context_p->ue_context.ue_guami.mcc,
                                    .mnc = ue_context_p->ue_context.ue_guami.mnc,
                                    .mnc_digit_len = ue_context_p->ue_context.ue_guami.mnc_len
                                    };
  
  gnb.guami.amf_region_id = ue_context_p->ue_context.ue_guami.amf_region_id;
  gnb.guami.amf_set_id = ue_context_p->ue_context.ue_guami.amf_set_id;
  gnb.guami.amf_ptr = ue_context_p->ue_context.ue_guami.amf_pointer;

  return gnb;
}


static 
ue_id_e2sm_t fill_ue_id_data(rrc_gNB_ue_context_t * ue_context_p)
{
  ue_id_e2sm_t ue_id_data = {0};

  ue_id_data.type = GNB_UE_ID_E2SM;
  ue_id_data.gnb = fill_gnb_data(ue_context_p);

  return ue_id_data;
}



uint32_t last_dl_rlc_pdu_total_bytes[MAX_MOBILES_PER_GNB] = {0};
uint32_t last_ul_rlc_pdu_total_bytes[MAX_MOBILES_PER_GNB] = {0};
uint32_t last_dl_total_prbs[MAX_MOBILES_PER_GNB] = {0};
uint32_t last_ul_total_prbs[MAX_MOBILES_PER_GNB] = {0};
uint32_t last_dl_pdcp_sdu_total_bytes[MAX_MOBILES_PER_GNB] = {0};
uint32_t last_ul_pdcp_sdu_total_bytes[MAX_MOBILES_PER_GNB] = {0};

static 
kpm_ind_msg_format_1_t fill_kpm_ind_msg_frm_1(NR_UE_info_t* const UE, size_t const ue_idx, kpm_act_def_format_1_t const * act_def_fr_1)
{
  kpm_ind_msg_format_1_t msg_frm_1 = {0};
  
  // Measurement Data contains a set of Meas Records, each collected at each granularity period
  msg_frm_1.meas_data_lst_len = 1;  /*  this value is equal to (kpm_ric_event_trigger_format_1.report_period_ms/act_def_fr_1->gran_period_ms)
                                        please, check their values in xApp */
  
  msg_frm_1.meas_data_lst = calloc(msg_frm_1.meas_data_lst_len, sizeof(*msg_frm_1.meas_data_lst));
  assert(msg_frm_1.meas_data_lst != NULL && "Memory exhausted" );


  for (size_t i = 0; i<msg_frm_1.meas_data_lst_len; i++)
  {
    meas_data_lst_t* meas_data = &msg_frm_1.meas_data_lst[i];

    // Get RLC stats per DRB
    nr_rlc_statistics_t rlc = get_rlc_stats_per_drb(UE, i+1);

    // Get PDCP stats per DRB
    nr_pdcp_statistics_t pdcp = get_pdcp_stats_per_drb(UE, i+1);
    
    // Measurement Record
    meas_data->meas_record_len = act_def_fr_1->meas_info_lst_len;  // record data list length corresponds to info list length from action definition

    meas_data->meas_record_lst = calloc(meas_data->meas_record_len, sizeof(meas_record_lst_t));
    assert(meas_data->meas_record_lst != NULL && "Memory exhausted");

    for (size_t j = 0; j < meas_data->meas_record_len; j++)  // each meas record corresponds to one meas type
    {
      meas_record_lst_t* meas_record = &meas_data->meas_record_lst[j];

      // Measurement Type as requested in Action Definition
      meas_type_t const meas_info_type = act_def_fr_1->meas_info_lst[j].meas_type;

      switch (meas_info_type.type)
      {
      case NAME_MEAS_TYPE:
      {
        char meas_info_name_str[meas_info_type.name.len + 1];
        memcpy(meas_info_name_str, meas_info_type.name.buf, meas_info_type.name.len);
        meas_info_name_str[meas_info_type.name.len] = '\0';

        if (strcmp(meas_info_name_str, "DRB.UEThpDl") == 0)  //  3GPP TS 28.522 - section 5.1.1.3.1
        {
          meas_record->value = REAL_MEAS_VALUE;

          // Calculate DL Thp
          meas_record->real_val = (double)(rlc.txpdu_bytes - last_dl_rlc_pdu_total_bytes[ue_idx])*8/act_def_fr_1->gran_period_ms;  // [kbps]
          last_dl_rlc_pdu_total_bytes[ue_idx] = rlc.txpdu_bytes;
          /*  note: per spec, average UE throughput in DL (taken into consideration values from all UEs, and averaged)
              here calculated as: UE specific throughput in DL */
        }
        else if (strcmp(meas_info_name_str, "DRB.UEThpUl") == 0)   //  3GPP TS 28.522 - section 5.1.1.3.3
        {
          meas_record->value = REAL_MEAS_VALUE;

          // Calculate UL Thp
          meas_record->real_val = (double)(rlc.rxpdu_bytes - last_ul_rlc_pdu_total_bytes[ue_idx])*8/act_def_fr_1->gran_period_ms;  // [kbps]
          last_ul_rlc_pdu_total_bytes[ue_idx] = rlc.rxpdu_bytes;
          /*  note: per spec, average UE throughput in UL (taken into consideration values from all UEs, and averaged)
              here calculated as: UE specific throughput in UL */
        }
        else if (strcmp(meas_info_name_str, "DRB.RlcSduDelayDl") == 0)   //  3GPP TS 28.522 - section 5.1.3.3.3
        {
          meas_record->value = REAL_MEAS_VALUE;

          // Get the value of sojourn time at the RLC buffer
          meas_record->real_val = rlc.txsdu_avg_time_to_tx;  // [μs]
          /* note: by default this measurement is calculated for previous 100ms (openair2/LAYER2/nr_rlc/nr_rlc_entity.c:118, 173, 213); please, update according to your needs */
        }
        else if (strcmp(meas_info_name_str, "RRU.PrbTotDl") == 0)   //  3GPP TS 28.522 - section 5.1.1.2.1
        {
          meas_record->value = INTEGER_MEAS_VALUE;

          // Get the number of DL PRBs
          meas_record->int_val = UE->mac_stats.dl.total_rbs - last_dl_total_prbs[ue_idx];   // [PRBs]
          last_dl_total_prbs[ue_idx] = UE->mac_stats.dl.total_rbs;
          /*  note: per spec, DL PRB usage [%] = (total used PRBs for DL traffic / total available PRBs for DL traffic) * 100   
              here calculated as: aggregated DL PRBs (t) - aggregated DL PRBs (t-gran_period) */
        }
        else if (strcmp(meas_info_name_str, "RRU.PrbTotUl") == 0)   //  3GPP TS 28.522 - section 5.1.1.2.2
        {
          meas_record->value = INTEGER_MEAS_VALUE;

          // Get the number of UL PRBs
          meas_record->int_val = UE->mac_stats.ul.total_rbs - last_ul_total_prbs[ue_idx];   // [PRBs]
          last_ul_total_prbs[ue_idx] = UE->mac_stats.ul.total_rbs;
          /*  note: per spec, UL PRB usage [%] = (total used PRBs for UL traffic / total available PRBs for UL traffic) * 100   
              here calculated as: aggregated UL PRBs (t) - aggregated UL PRBs (t-gran_period) */
        }
        else if (strcmp(meas_info_name_str, "DRB.PdcpSduVolumeDL") == 0)   //  3GPP TS 28.522 - section 5.1.2.1.1.1
        {
          meas_record->value = INTEGER_MEAS_VALUE;

          // Get DL data volume delivered to PDCP layer
          meas_record->int_val = (pdcp.rxsdu_bytes - last_dl_pdcp_sdu_total_bytes[ue_idx])*8/1000;   // [kb]
          last_dl_pdcp_sdu_total_bytes[ue_idx] = pdcp.rxsdu_bytes;
          /* note: this measurement is calculated as per spec */
        }
        else if (strcmp(meas_info_name_str, "DRB.PdcpSduVolumeUL") == 0)   //  3GPP TS 28.522 - section 5.1.2.1.2.1
        {
          meas_record->value = INTEGER_MEAS_VALUE;

          // Get UL data volume delivered from PDCP layer
          meas_record->int_val = (pdcp.txsdu_bytes - last_ul_pdcp_sdu_total_bytes[ue_idx])*8/1000;   // [kb]
          last_ul_pdcp_sdu_total_bytes[ue_idx] = pdcp.txsdu_bytes;
          /* note: this measurement is calculated as per spec */
        }

        break;
      }
      
      case ID_MEAS_TYPE:
        assert(false && "ID Measurement Type not yet implemented");
        break;

      default:
        assert(false && "Measurement Type not recognized");
        break;
      }

    }
  }

  // Measurement Information - OPTIONAL
  msg_frm_1.meas_info_lst_len = act_def_fr_1->meas_info_lst_len;
  msg_frm_1.meas_info_lst = calloc(msg_frm_1.meas_info_lst_len, sizeof(meas_info_format_1_lst_t));
  assert(msg_frm_1.meas_info_lst != NULL && "Memory exhausted" );

  // Get measInfo from action definition
  for (size_t i = 0; i < msg_frm_1.meas_info_lst_len; i++) {
    // Measurement Type
    msg_frm_1.meas_info_lst[i].meas_type.type = act_def_fr_1->meas_info_lst[i].meas_type.type;
    // Measurement Name
    if (act_def_fr_1->meas_info_lst[i].meas_type.type == NAME_MEAS_TYPE) {
      msg_frm_1.meas_info_lst[i].meas_type.name.buf = calloc(act_def_fr_1->meas_info_lst[i].meas_type.name.len, sizeof(uint8_t));
      memcpy(msg_frm_1.meas_info_lst[i].meas_type.name.buf, act_def_fr_1->meas_info_lst[i].meas_type.name.buf, act_def_fr_1->meas_info_lst[i].meas_type.name.len);
      msg_frm_1.meas_info_lst[i].meas_type.name.len = act_def_fr_1->meas_info_lst[i].meas_type.name.len;
    } else {
      msg_frm_1.meas_info_lst[i].meas_type.id = act_def_fr_1->meas_info_lst[i].meas_type.id;
    }


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


typedef struct {
    NR_UE_info_t *ue_list;
    size_t num_ues;
} matched_ues_t;

static
matched_ues_t filter_ues_by_s_nssai_criteria(test_cond_e const * condition, int64_t const value, NR_UE_info_t * ue_list, size_t const num_connected_ues)
{
  matched_ues_t matched_ues = {.num_ues = 0, .ue_list = calloc(num_connected_ues, sizeof(NR_UE_info_t))};
  assert(matched_ues.ue_list != NULL && "Memory exhausted");
  
  assert(RC.nb_nr_inst == 1 && "Number of RRC instances greater than 1 not yet implemented");
  
  // Check if each UE satisfies the given S-NSSAI criteria
  for (size_t i = 0; i<num_connected_ues; i++)
  {
    rrc_gNB_ue_context_t *rrc_ue_context_list = rrc_gNB_get_ue_context_by_rnti(RC.nrrrc[0], ue_list[i].rnti);
    ngap_gNB_ue_context_t *ngap_ue_context_list = ngap_get_ue_context(rrc_ue_context_list->ue_context.gNB_ue_ngap_id);

    switch (*condition)
    {
    case EQUAL_TEST_COND:
      assert(ngap_ue_context_list->gNB_instance[0].s_nssai[0][0].sST == value && "Please, check the condition for S-NSSAI. At the moment, OAI supports eMBB");
      matched_ues.ue_list[matched_ues.num_ues] = ue_list[i];
      matched_ues.num_ues++;
      break;
    
    default:
      assert(false && "Condition not yet implemented");
    }
  }

  return matched_ues;
}

static
kpm_ind_msg_format_3_t fill_kpm_ind_msg_frm_3(const kpm_act_def_format_4_t * act_def_fr_4)
{
  kpm_ind_msg_format_3_t msg_frm_3 = {0};
    
    // Get the number of connected UEs and its info (RNTI)
    msg_frm_3.ue_meas_report_lst_len = get_number_connected_ues();  // (rand() % 65535) + 1;
    assert(msg_frm_3.ue_meas_report_lst_len != 0 && "Number of UEs to report must be greater than 0");

    NR_UE_info_t * ue_list = connected_ues_list();


    // Filter the UE by the test condition criteria
    matched_ues_t matched_ues = {0};

    for (size_t j = 0; j<act_def_fr_4->matching_cond_lst_len; j++)
    {
      switch (act_def_fr_4->matching_cond_lst[j].test_info_lst.test_cond_type)
      {
      case S_NSSAI_TEST_COND_TYPE:
        assert(act_def_fr_4->matching_cond_lst[j].test_info_lst.S_NSSAI == TRUE_TEST_COND_TYPE && "Must be true");
        
        matched_ues = filter_ues_by_s_nssai_criteria(act_def_fr_4->matching_cond_lst[j].test_info_lst.test_cond, *act_def_fr_4->matching_cond_lst[j].test_info_lst.int_value, ue_list, msg_frm_3.ue_meas_report_lst_len);
        break;
      
      default:
        assert(false && "Test condition type not yet implemented");
      }

    }

    // Fill UE Measurement Reports
    assert(matched_ues.num_ues >= 1 && "The number of filtered UEs must be at least equal to 1");
    msg_frm_3.meas_report_per_ue = calloc(matched_ues.num_ues, sizeof(meas_report_per_ue_t));
    assert(msg_frm_3.meas_report_per_ue != NULL && "Memory exhausted");

    for (size_t i = 0; i<matched_ues.num_ues; i++)
    {
      // Fill UE ID data
      rrc_gNB_ue_context_t *rrc_ue_context_list = rrc_gNB_get_ue_context_by_rnti(RC.nrrrc[0], matched_ues.ue_list[i].rnti);
      msg_frm_3.meas_report_per_ue[i].ue_meas_report_lst = fill_ue_id_data(rrc_ue_context_list);
      
      // Fill UE related info
      msg_frm_3.meas_report_per_ue[i].ind_msg_format_1 = fill_kpm_ind_msg_frm_1(&matched_ues.ue_list[i], i, &act_def_fr_4->action_def_format_1);
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
kpm_ind_hdr_t fill_kpm_ind_hdr(void)
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

  kpm_rd_ind_data_t* const kpm = (kpm_rd_ind_data_t*)data;

  assert(kpm->act_def!= NULL && "Cannot be NULL");

  // 7.8 Supported RIC Styles and E2SM IE Formats
  // Action Definition Format 4 corresponds to Indication Message Format 3
  switch (kpm->act_def->type)
  {
  case FORMAT_4_ACTION_DEFINITION:
  {
    kpm->ind.hdr = fill_kpm_ind_hdr(); 
    
    kpm->ind.msg.type = FORMAT_3_INDICATION_MESSAGE;
    kpm->ind.msg.frm_3 = fill_kpm_ind_msg_frm_3(&kpm->act_def->frm_4);

    break;
  }
  
  default:
  {
    kpm->ind.hdr = fill_rnd_kpm_ind_hdr(); 
    kpm->ind.msg = fill_rnd_kpm_ind_msg();

    break;
  }
  }

}

void read_kpm_setup_sm(void* e2ap)
{
  assert(e2ap != NULL);
//  assert(e2ap->type == KPM_V3_0_AGENT_IF_E2_SETUP_ANS_V0);

  kpm_e2_setup_t* kpm = (kpm_e2_setup_t*)(e2ap);
  kpm->ran_func_def = fill_rnd_kpm_ran_func_def(); 
}

sm_ag_if_ans_t write_ctrl_kpm_sm(void const* src)
{
  assert(0 !=0 && "Not supported");
  (void)src;
  sm_ag_if_ans_t ans = {0};
  return ans;
}

