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
#include "openair2/F1AP/f1ap_ids.h"
#include <assert.h>
#include <stdio.h>


typedef struct {
    NR_UE_info_t *ue_list;
    size_t num_ues;
} matched_ues_mac_t;


static
matched_ues_mac_t filter_ues_by_s_nssai_in_du_or_monolithic(test_cond_e const condition, int64_t const value)
{
  matched_ues_mac_t matched_ues = {.num_ues = 0, .ue_list = calloc(MAX_MOBILES_PER_GNB, sizeof(NR_UE_info_t)) };
  assert(matched_ues.ue_list != NULL && "Memory exhausted");

  /* IMPORTANT: in the case of gNB-DU, it is not possible to filter UEs by S-NSSAI as the ngap context is stored in gNB-CU
                instead, we take all connected UEs*/

  // Take MAC info
  UE_iterator(RC.nrmac[0]->UE_info.list, ue)
  {
    if (ue)
    {
      ngap_gNB_ue_context_t *ngap_ue_context_list = NULL;

      // Filter connected UEs by S-NSSAI test condition to get list of matched UEs 
      // note: not possible to filter 
      switch (condition)
      {
      case EQUAL_TEST_COND:
      {
        if (NODE_IS_MONOLITHIC(RC.nrrrc[0]->node_type))
        {
          rrc_gNB_ue_context_t *rrc_ue_context_list = rrc_gNB_get_ue_context_by_rnti(RC.nrrrc[0], ue->rnti);
          ngap_ue_context_list = ngap_get_ue_context(rrc_ue_context_list->ue_context.rrc_ue_id);
          assert(ngap_ue_context_list->gNB_instance[0].s_nssai[0][0].sST == value && "Please, check the condition for S-NSSAI. At the moment, OAI supports eMBB");
        }
          matched_ues.ue_list[matched_ues.num_ues] = *ue;
          matched_ues.num_ues++;
          break;
        }
      
    
      default:
        assert(false && "Condition not yet implemented");
      }
      
    }    
  }

  assert(matched_ues.num_ues >= 1 && "The number of filtered UEs must be at least equal to 1");

  return matched_ues;
}


typedef struct {
  f1_ue_data_t * rrc_ue_id_list;  // list of matched UEs on RRC level containing only rrc_ue_id (gNB CU UE ID)
  size_t num_ues;

} matched_ues_rrc_t;

static
matched_ues_rrc_t filter_ues_by_s_nssai_in_cu(test_cond_e const condition, int64_t const value)
{
  matched_ues_rrc_t matched_ues = {.num_ues = 0, .rrc_ue_id_list = calloc(MAX_MOBILES_PER_GNB, sizeof(f1_ue_data_t)) };
  assert(matched_ues.rrc_ue_id_list != NULL && "Memory exhausted");


  struct rrc_gNB_ue_context_s *ue_context_p1 = NULL;
  RB_FOREACH(ue_context_p1, rrc_nr_ue_tree_s, &RC.nrrrc[0]->rrc_ue_head) {
    
    ngap_gNB_ue_context_t *ngap_ue_context_list = ngap_get_ue_context(ue_context_p1->ue_context.rrc_ue_id);
    
    // Filter connected UEs by S-NSSAI test condition to get list of matched UEs 
    switch (condition)
    {
    case EQUAL_TEST_COND:
      assert(ngap_ue_context_list->gNB_instance[0].s_nssai[0][0].sST == value && "Please, check the condition for S-NSSAI. At the moment, OAI supports eMBB");
      matched_ues.rrc_ue_id_list[matched_ues.num_ues].secondary_ue = ue_context_p1->ue_context.rrc_ue_id;
      matched_ues.num_ues++;
      break;
    
    default:
      assert(false && "Condition not yet implemented");
    }
  }
  
  assert(matched_ues.num_ues >= 1 && "The number of filtered UEs must be at least equal to 1");
  
  return matched_ues;
}

static
nr_rlc_statistics_t get_rlc_stats_per_drb(NR_UE_info_t const * UE)
{
  assert(UE != NULL);

  nr_rlc_statistics_t rlc = {0};
  const int srb_flag = 0;
  const int rb_id = 1;  // at the moment, only 1 DRB is supported

  // Get RLC stats for specific DRB
  const bool rc = nr_rlc_get_statistics(UE->rnti, srb_flag, rb_id, &rlc);
  assert(rc == true && "Cannot get RLC stats\n");

  // Activate average sojourn time at the RLC buffer for specific DRB
  nr_rlc_activate_avg_time_to_tx(UE->rnti, rb_id+3, 1);
  
  return rlc;  
}

static
nr_pdcp_statistics_t get_pdcp_stats_per_drb(const uint32_t rrc_ue_id)
{
  nr_pdcp_statistics_t pdcp = {0};
  const int srb_flag = 0;
  const int rb_id = 1;  // at the moment, only 1 DRB is supported

  // Get PDCP stats for specific DRB
  const bool rc = nr_pdcp_get_statistics(rrc_ue_id, srb_flag, rb_id, &pdcp);
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
  gnb.amf_ue_ngap_id = ue_context_p->ue_context.rrc_ue_id;

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

  // gNB-CU UE F1AP ID List
  // C-ifCUDUseparated 
  if (NODE_IS_CU(RC.nrrrc[0]->node_type))
  {
    gnb.gnb_cu_ue_f1ap_lst_len = 1;
    gnb.gnb_cu_ue_f1ap_lst = calloc(gnb.gnb_cu_ue_f1ap_lst_len, sizeof(uint32_t));
    assert(gnb.gnb_cu_ue_f1ap_lst != NULL);

    for (size_t i = 0; i < gnb.gnb_cu_ue_f1ap_lst_len; i++)
    {
      gnb.gnb_cu_ue_f1ap_lst[i] = ue_context_p->ue_context.rrc_ue_id;
    }
  }


  return gnb;
}

static
gnb_du_e2sm_t fill_gnb_du_data(const f1_ue_data_t * rrc_ue_id)
{
  gnb_du_e2sm_t gnb_du = {0};

  // 6.2.3.21
  // gNB CU UE F1AP
  // Mandatory
  gnb_du.gnb_cu_ue_f1ap = rrc_ue_id->secondary_ue;


  // 6.2.3.25
  // RAN UE ID
  // Optional
  gnb_du.ran_ue_id = NULL;

  return gnb_du;
}

// Bad bad bad. Create a proper exponential moving average filter or at least, only read statistics
// Avoid coupling among sublayers
static uint32_t last_dl_rlc_pdu_total_bytes[MAX_MOBILES_PER_GNB] = {0};
static uint32_t last_ul_rlc_pdu_total_bytes[MAX_MOBILES_PER_GNB] = {0};
static uint32_t last_dl_total_prbs[MAX_MOBILES_PER_GNB] = {0};
static uint32_t last_ul_total_prbs[MAX_MOBILES_PER_GNB] = {0};
static uint32_t last_dl_pdcp_sdu_total_bytes[MAX_MOBILES_PER_GNB] = {0};
static uint32_t last_ul_pdcp_sdu_total_bytes[MAX_MOBILES_PER_GNB] = {0};


static
meas_record_lst_t fill_ue_mac_rlc_data(const NR_UE_info_t* UE, const size_t ue_idx, const char * meas_info_name_str, const uint32_t gran_period_ms)
{
  assert(UE != NULL);

  meas_record_lst_t meas_record = {0};

  // Get RLC stats per DRB
  nr_rlc_statistics_t rlc = get_rlc_stats_per_drb(UE);

  // Measurement Type as requested in Action Definition
  if (strcmp(meas_info_name_str, "DRB.UEThpDl") == 0)  //  3GPP TS 28.522 - section 5.1.1.3.1
  {
    meas_record.value = REAL_MEAS_VALUE;

    // Calculate DL Thp
    meas_record.real_val = (double)(rlc.txpdu_bytes - last_dl_rlc_pdu_total_bytes[ue_idx])*8/gran_period_ms;  // [kbps]
    last_dl_rlc_pdu_total_bytes[ue_idx] = rlc.txpdu_bytes;
    /*  note: per spec, average UE throughput in DL (taken into consideration values from all UEs, and averaged)
        here calculated as: UE specific throughput in DL */
  }
  else if (strcmp(meas_info_name_str, "DRB.UEThpUl") == 0)   //  3GPP TS 28.522 - section 5.1.1.3.3
  {
    meas_record.value = REAL_MEAS_VALUE;

    // Calculate UL Thp
    meas_record.real_val = (double)(rlc.rxpdu_bytes - last_ul_rlc_pdu_total_bytes[ue_idx])*8/gran_period_ms;  // [kbps]
    last_ul_rlc_pdu_total_bytes[ue_idx] = rlc.rxpdu_bytes;
    /*  note: per spec, average UE throughput in UL (taken into consideration values from all UEs, and averaged)
        here calculated as: UE specific throughput in UL */
  }
  else if (strcmp(meas_info_name_str, "DRB.RlcSduDelayDl") == 0)   //  3GPP TS 28.522 - section 5.1.3.3.3
  {
    meas_record.value = REAL_MEAS_VALUE;

    // Get the value of sojourn time at the RLC buffer
    meas_record.real_val = rlc.txsdu_avg_time_to_tx;  // [μs]
    /* note: by default this measurement is calculated for previous 100ms (openair2/LAYER2/nr_rlc/nr_rlc_entity.c:118, 173, 213); please, update according to your needs */
  }
  else if (strcmp(meas_info_name_str, "RRU.PrbTotDl") == 0)   //  3GPP TS 28.522 - section 5.1.1.2.1
  {
    meas_record.value = INTEGER_MEAS_VALUE;

    // Get the number of DL PRBs
    meas_record.int_val = UE->mac_stats.dl.total_rbs - last_dl_total_prbs[ue_idx];   // [PRBs]
    last_dl_total_prbs[ue_idx] = UE->mac_stats.dl.total_rbs;
    /*  note: per spec, DL PRB usage [%] = (total used PRBs for DL traffic / total available PRBs for DL traffic) * 100   
        here calculated as: aggregated DL PRBs (t) - aggregated DL PRBs (t-gran_period) */
  }
  else if (strcmp(meas_info_name_str, "RRU.PrbTotUl") == 0)   //  3GPP TS 28.522 - section 5.1.1.2.2
  {
    meas_record.value = INTEGER_MEAS_VALUE;

    // Get the number of UL PRBs
    meas_record.int_val = UE->mac_stats.ul.total_rbs - last_ul_total_prbs[ue_idx];   // [PRBs]
    last_ul_total_prbs[ue_idx] = UE->mac_stats.ul.total_rbs;
    /*  note: per spec, UL PRB usage [%] = (total used PRBs for UL traffic / total available PRBs for UL traffic) * 100   
        here calculated as: aggregated UL PRBs (t) - aggregated UL PRBs (t-gran_period) */
  }
  else
  {
    assert(false && "Measurement Name not yet implemented");
  }
  

  return meas_record;
}

static
meas_record_lst_t fill_ue_pdcp_data(const uint32_t rrc_ue_id, const size_t ue_idx, const char * meas_info_name_str)
{
  meas_record_lst_t meas_record = {0};

  // Get PDCP stats per DRB
  nr_pdcp_statistics_t pdcp = get_pdcp_stats_per_drb(rrc_ue_id);

  // Measurement Type as requested in Action Definition
  if (strcmp(meas_info_name_str, "DRB.PdcpSduVolumeDL") == 0)   //  3GPP TS 28.522 - section 5.1.2.1.1.1
  {
    meas_record.value = INTEGER_MEAS_VALUE;

    // Get DL data volume delivered to PDCP layer
    meas_record.int_val = (pdcp.rxsdu_bytes - last_dl_pdcp_sdu_total_bytes[ue_idx])*8/1000;   // [kb]
    last_dl_pdcp_sdu_total_bytes[ue_idx] = pdcp.rxsdu_bytes;
    /* note: this measurement is calculated as per spec */
  }
  else if (strcmp(meas_info_name_str, "DRB.PdcpSduVolumeUL") == 0)   //  3GPP TS 28.522 - section 5.1.2.1.2.1
  {
    meas_record.value = INTEGER_MEAS_VALUE;

    // Get UL data volume delivered from PDCP layer
    meas_record.int_val = (pdcp.txsdu_bytes - last_ul_pdcp_sdu_total_bytes[ue_idx])*8/1000;   // [kb]
    last_ul_pdcp_sdu_total_bytes[ue_idx] = pdcp.txsdu_bytes;
    /* note: this measurement is calculated as per spec */
  }
  else
  {
    assert(false && "Measurement Name not yet implemented");
  }
  

  return meas_record;
}

static
meas_info_format_1_lst_t * fill_kpm_meas_info_frm_1(const size_t len, const kpm_act_def_format_1_t * act_def_fr_1)
{
  meas_info_format_1_lst_t * meas_info_lst = calloc(len, sizeof(meas_info_format_1_lst_t));
  assert(meas_info_lst != NULL && "Memory exhausted" );

  // Get measInfo from action definition
  for (size_t i = 0; i < len; i++)
  {
    // Measurement Type
    meas_info_lst[i].meas_type.type = act_def_fr_1->meas_info_lst[i].meas_type.type;
    // Measurement Name
    if (act_def_fr_1->meas_info_lst[i].meas_type.type == NAME_MEAS_TYPE) {
      meas_info_lst[i].meas_type.name.buf = calloc(act_def_fr_1->meas_info_lst[i].meas_type.name.len, sizeof(uint8_t));
      memcpy(meas_info_lst[i].meas_type.name.buf, act_def_fr_1->meas_info_lst[i].meas_type.name.buf, act_def_fr_1->meas_info_lst[i].meas_type.name.len);
      meas_info_lst[i].meas_type.name.len = act_def_fr_1->meas_info_lst[i].meas_type.name.len;
    } else {
      meas_info_lst[i].meas_type.id = act_def_fr_1->meas_info_lst[i].meas_type.id;
    }


    // Label Information
    meas_info_lst[i].label_info_lst_len = 1;
    meas_info_lst[i].label_info_lst = calloc(meas_info_lst[i].label_info_lst_len, sizeof(label_info_lst_t));
    assert(meas_info_lst[i].label_info_lst != NULL && "Memory exhausted" );

    for (size_t j = 0; j < meas_info_lst[i].label_info_lst_len; j++) {
      meas_info_lst[i].label_info_lst[j].noLabel = malloc(sizeof(enum_value_e));
      *meas_info_lst[i].label_info_lst[j].noLabel = TRUE_ENUM_VALUE;
    }
  }

  return meas_info_lst;
}

static
kpm_ind_msg_format_1_t fill_kpm_ind_msg_frm_1_in_du(const NR_UE_info_t* UE, const size_t ue_idx,  const kpm_act_def_format_1_t * act_def_fr_1)
{
  kpm_ind_msg_format_1_t msg_frm_1 = {0};
  
  // Measurement Data contains a set of Meas Records, each collected at each granularity period
  msg_frm_1.meas_data_lst_len = 1;  /*  this value is equal to (kpm_ric_event_trigger_format_1.report_period_ms/act_def_fr_1->gran_period_ms)
                                        please, check their values in xApp */
  
  msg_frm_1.meas_data_lst = calloc(msg_frm_1.meas_data_lst_len, sizeof(*msg_frm_1.meas_data_lst));
  assert(msg_frm_1.meas_data_lst != NULL && "Memory exhausted" );

  for (size_t i = 0; i<msg_frm_1.meas_data_lst_len; i++)
  {
    // Measurement Record
    msg_frm_1.meas_data_lst[i].meas_record_len = act_def_fr_1->meas_info_lst_len;  // record data list length corresponds to info list length from action definition

    msg_frm_1.meas_data_lst[i].meas_record_lst = calloc(msg_frm_1.meas_data_lst[i].meas_record_len, sizeof(meas_record_lst_t));
    assert(msg_frm_1.meas_data_lst[i].meas_record_lst != NULL && "Memory exhausted");

    for (size_t j = 0; j < msg_frm_1.meas_data_lst[i].meas_record_len; j++)  // each meas record corresponds to one meas type
    {
      // Measurement Type as requested in Action Definition
      switch (act_def_fr_1->meas_info_lst[j].meas_type.type)
      {
      case NAME_MEAS_TYPE:
      {
        char meas_info_name_str[act_def_fr_1->meas_info_lst[j].meas_type.name.len + 1];
        memcpy(meas_info_name_str, act_def_fr_1->meas_info_lst[j].meas_type.name.buf, act_def_fr_1->meas_info_lst[j].meas_type.name.len);
        meas_info_name_str[act_def_fr_1->meas_info_lst[j].meas_type.name.len] = '\0';

        msg_frm_1.meas_data_lst[i].meas_record_lst[j] = fill_ue_mac_rlc_data(UE, ue_idx, meas_info_name_str, act_def_fr_1->gran_period_ms);
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
  msg_frm_1.meas_info_lst = fill_kpm_meas_info_frm_1(msg_frm_1.meas_info_lst_len, act_def_fr_1);

  return msg_frm_1;
}

static
kpm_ind_msg_format_3_t fill_kpm_ind_msg_frm_3_in_du(const matched_ues_mac_t matched_ues, const kpm_act_def_format_1_t * act_def_fr_1)
{
  assert(act_def_fr_1 != NULL);


  kpm_ind_msg_format_3_t msg_frm_3 = {0};

  // Fill UE Measurement Reports
  
  msg_frm_3.ue_meas_report_lst_len = matched_ues.num_ues;
  msg_frm_3.meas_report_per_ue = calloc(msg_frm_3.ue_meas_report_lst_len, sizeof(meas_report_per_ue_t));
  assert(msg_frm_3.meas_report_per_ue != NULL && "Memory exhausted");

  for (size_t i = 0; i<msg_frm_3.ue_meas_report_lst_len; i++)
  {
    // Fill UE ID data
    f1_ue_data_t rrc_ue_id = du_get_f1_ue_data(matched_ues.ue_list[i].rnti);  // get gNB CU UE ID as rrc_ue_id
    msg_frm_3.meas_report_per_ue[i].ue_meas_report_lst.type = GNB_DU_UE_ID_E2SM;
    msg_frm_3.meas_report_per_ue[i].ue_meas_report_lst.gnb_du = fill_gnb_du_data(&rrc_ue_id);
      
    // Fill UE related info
    msg_frm_3.meas_report_per_ue[i].ind_msg_format_1 = fill_kpm_ind_msg_frm_1_in_du(&matched_ues.ue_list[i], i, act_def_fr_1);
  }


  return msg_frm_3;
}

static
kpm_ind_msg_format_1_t fill_kpm_ind_msg_frm_1_in_cu(const uint32_t rrc_ue_id, const size_t ue_idx, const kpm_act_def_format_1_t * act_def_fr_1)
{
  kpm_ind_msg_format_1_t msg_frm_1 = {0};
  
  // Measurement Data contains a set of Meas Records, each collected at each granularity period
  msg_frm_1.meas_data_lst_len = 1;  /*  this value is equal to (kpm_ric_event_trigger_format_1.report_period_ms/act_def_fr_1->gran_period_ms)
                                        please, check their values in xApp */
  
  msg_frm_1.meas_data_lst = calloc(msg_frm_1.meas_data_lst_len, sizeof(*msg_frm_1.meas_data_lst));
  assert(msg_frm_1.meas_data_lst != NULL && "Memory exhausted" );

  for (size_t i = 0; i<msg_frm_1.meas_data_lst_len; i++)
  {
    // Measurement Record
    msg_frm_1.meas_data_lst[i].meas_record_len = act_def_fr_1->meas_info_lst_len;  // record data list length corresponds to info list length from action definition

    msg_frm_1.meas_data_lst[i].meas_record_lst = calloc(msg_frm_1.meas_data_lst[i].meas_record_len, sizeof(meas_record_lst_t));
    assert(msg_frm_1.meas_data_lst[i].meas_record_lst != NULL && "Memory exhausted");

    for (size_t j = 0; j < msg_frm_1.meas_data_lst[i].meas_record_len; j++)  // each meas record corresponds to one meas type
    {
      // Measurement Type as requested in Action Definition
      switch (act_def_fr_1->meas_info_lst[j].meas_type.type)
      {
      case NAME_MEAS_TYPE:
      {
        char meas_info_name_str[act_def_fr_1->meas_info_lst[j].meas_type.name.len + 1];
        memcpy(meas_info_name_str, act_def_fr_1->meas_info_lst[j].meas_type.name.buf, act_def_fr_1->meas_info_lst[j].meas_type.name.len);
        meas_info_name_str[act_def_fr_1->meas_info_lst[j].meas_type.name.len] = '\0';

        msg_frm_1.meas_data_lst[i].meas_record_lst[j] = fill_ue_pdcp_data(rrc_ue_id, ue_idx, meas_info_name_str);
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
  msg_frm_1.meas_info_lst = fill_kpm_meas_info_frm_1(msg_frm_1.meas_info_lst_len, act_def_fr_1);

  return msg_frm_1;
}

static
kpm_ind_msg_format_3_t fill_kpm_ind_msg_frm_3_in_cu(const matched_ues_rrc_t matched_ues, const kpm_act_def_format_1_t * act_def_fr_1)
{
  assert(act_def_fr_1 != NULL);


  kpm_ind_msg_format_3_t msg_frm_3 = {0};

  // Fill UE Measurement Reports
  
  msg_frm_3.ue_meas_report_lst_len = matched_ues.num_ues;
  msg_frm_3.meas_report_per_ue = calloc(msg_frm_3.ue_meas_report_lst_len, sizeof(meas_report_per_ue_t));
  assert(msg_frm_3.meas_report_per_ue != NULL && "Memory exhausted");

  for (size_t i = 0; i<msg_frm_3.ue_meas_report_lst_len; i++)
  {
    // Fill UE ID data
    rrc_gNB_ue_context_t *rrc_ue_context_list = rrc_gNB_get_ue_context(RC.nrrrc[0], matched_ues.rrc_ue_id_list[i].secondary_ue);
    msg_frm_3.meas_report_per_ue[i].ue_meas_report_lst.type = GNB_UE_ID_E2SM;
    msg_frm_3.meas_report_per_ue[i].ue_meas_report_lst.gnb = fill_gnb_data(rrc_ue_context_list);

    // Fill UE related info
    msg_frm_3.meas_report_per_ue[i].ind_msg_format_1 = fill_kpm_ind_msg_frm_1_in_cu(matched_ues.rrc_ue_id_list[i].secondary_ue, i, act_def_fr_1);
  }

  return msg_frm_3;
}

static
kpm_ind_msg_format_1_t fill_kpm_ind_msg_frm_1_in_monolithic(const NR_UE_info_t* UE, const size_t ue_idx, const uint32_t rrc_ue_id, const kpm_act_def_format_1_t * act_def_fr_1)
{
  kpm_ind_msg_format_1_t msg_frm_1 = {0};
  
  // Measurement Data contains a set of Meas Records, each collected at each granularity period
  msg_frm_1.meas_data_lst_len = 1;  /*  this value is equal to (kpm_ric_event_trigger_format_1.report_period_ms/act_def_fr_1->gran_period_ms)
                                        please, check their values in xApp */
  
  msg_frm_1.meas_data_lst = calloc(msg_frm_1.meas_data_lst_len, sizeof(*msg_frm_1.meas_data_lst));
  assert(msg_frm_1.meas_data_lst != NULL && "Memory exhausted" );

  for (size_t i = 0; i<msg_frm_1.meas_data_lst_len; i++)
  {
    // Measurement Record
    msg_frm_1.meas_data_lst[i].meas_record_len = act_def_fr_1->meas_info_lst_len;  // record data list length corresponds to info list length from action definition

    msg_frm_1.meas_data_lst[i].meas_record_lst = calloc(msg_frm_1.meas_data_lst[i].meas_record_len, sizeof(meas_record_lst_t));
    assert(msg_frm_1.meas_data_lst[i].meas_record_lst != NULL && "Memory exhausted");

    for (size_t j = 0; j < msg_frm_1.meas_data_lst[i].meas_record_len; j++)  // each meas record corresponds to one meas type
    {
      // Measurement Type as requested in Action Definition
      switch (act_def_fr_1->meas_info_lst[j].meas_type.type)
      {
      case NAME_MEAS_TYPE:
      {
        char meas_info_name_str[act_def_fr_1->meas_info_lst[j].meas_type.name.len + 1];
        memcpy(meas_info_name_str, act_def_fr_1->meas_info_lst[j].meas_type.name.buf, act_def_fr_1->meas_info_lst[j].meas_type.name.len);
        meas_info_name_str[act_def_fr_1->meas_info_lst[j].meas_type.name.len] = '\0';

        if (strcmp(meas_info_name_str, "DRB.PdcpSduVolumeDL") == 0 || strcmp(meas_info_name_str, "DRB.PdcpSduVolumeUL") == 0)
        {
          msg_frm_1.meas_data_lst[i].meas_record_lst[j] = fill_ue_pdcp_data(rrc_ue_id, ue_idx, meas_info_name_str);
        }
        else
        {
          msg_frm_1.meas_data_lst[i].meas_record_lst[j] = fill_ue_mac_rlc_data(UE, ue_idx, meas_info_name_str, act_def_fr_1->gran_period_ms);
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
  msg_frm_1.meas_info_lst = fill_kpm_meas_info_frm_1(msg_frm_1.meas_info_lst_len, act_def_fr_1);


  return msg_frm_1;
}

static
kpm_ind_msg_format_3_t fill_kpm_ind_msg_frm_3_in_monolithic(const matched_ues_mac_t matched_ues, const kpm_act_def_format_1_t * act_def_fr_1)
{
  assert(act_def_fr_1 != NULL);


  kpm_ind_msg_format_3_t msg_frm_3 = {0};

  // Fill UE Measurement Reports
  
  msg_frm_3.ue_meas_report_lst_len = matched_ues.num_ues;
  msg_frm_3.meas_report_per_ue = calloc(msg_frm_3.ue_meas_report_lst_len, sizeof(meas_report_per_ue_t));
  assert(msg_frm_3.meas_report_per_ue != NULL && "Memory exhausted");


  for (size_t i = 0; i<msg_frm_3.ue_meas_report_lst_len; i++)
  {
    // Fill UE ID data
    rrc_gNB_ue_context_t *rrc_ue_context_list = rrc_gNB_get_ue_context_by_rnti(RC.nrrrc[0], matched_ues.ue_list[i].rnti);
    msg_frm_3.meas_report_per_ue[i].ue_meas_report_lst.type = GNB_UE_ID_E2SM;
    msg_frm_3.meas_report_per_ue[i].ue_meas_report_lst.gnb = fill_gnb_data(rrc_ue_context_list);
      
    // Fill UE related info
    msg_frm_3.meas_report_per_ue[i].ind_msg_format_1 = fill_kpm_ind_msg_frm_1_in_monolithic(&matched_ues.ue_list[i], i, rrc_ue_context_list->ue_context.rrc_ue_id, act_def_fr_1);
  }

  return msg_frm_3;
}

static
kpm_ric_ind_hdr_format_1_t kpm_ind_hdr_frm_1(void)
{
  kpm_ric_ind_hdr_format_1_t hdr_frm_1 = {0};

  int64_t const t = time_now_us();
#if defined KPM_V2
  hdr_frm_1.collectStartTime = t/1000000; // seconds
#elif defined KPM_V3 
  hdr_frm_1.collectStartTime = t; // microseconds
#else
  static_assert(0!=0, "Undefined KPM SM Version");
#endif

  hdr_frm_1.fileformat_version = NULL;

  // Check E2 Node NG-RAN Type
  if (NODE_IS_DU(RC.nrrrc[0]->node_type))
  {
    hdr_frm_1.sender_name = calloc(1, sizeof(byte_array_t));
    hdr_frm_1.sender_name->buf = calloc(strlen("My OAI-DU") + 1, sizeof(char));
    memcpy(hdr_frm_1.sender_name->buf, "My OAI-DU", strlen("My OAI-DU"));
    hdr_frm_1.sender_name->len = strlen("My OAI-DU");

    hdr_frm_1.sender_type = calloc(1, sizeof(byte_array_t));
    hdr_frm_1.sender_type->buf = calloc(strlen("DU") + 1, sizeof(char));
    memcpy(hdr_frm_1.sender_type->buf, "DU", strlen("DU"));
    hdr_frm_1.sender_type->len = strlen("DU");
  }
  else if (NODE_IS_CU(RC.nrrrc[0]->node_type))
  {
    hdr_frm_1.sender_name = calloc(1, sizeof(byte_array_t));
    hdr_frm_1.sender_name->buf = calloc(strlen("My OAI-CU") + 1, sizeof(char));
    memcpy(hdr_frm_1.sender_name->buf, "My OAI-CU", strlen("My OAI-CU"));
    hdr_frm_1.sender_name->len = strlen("My OAI-CU");

    hdr_frm_1.sender_type = calloc(1, sizeof(byte_array_t));
    hdr_frm_1.sender_type->buf = calloc(strlen("CU") + 1, sizeof(char));
    memcpy(hdr_frm_1.sender_type->buf, "CU", strlen("CU"));
    hdr_frm_1.sender_type->len = strlen("CU");
  }
  else if (NODE_IS_MONOLITHIC(RC.nrrrc[0]->node_type))
  {
    hdr_frm_1.sender_name = calloc(1, sizeof(byte_array_t));
    hdr_frm_1.sender_name->buf = calloc(strlen("My OAI-MONO") + 1, sizeof(char));
    memcpy(hdr_frm_1.sender_name->buf, "My OAI-MONO", strlen("My OAI-MONO"));
    hdr_frm_1.sender_name->len = strlen("My OAI-MONO");

    hdr_frm_1.sender_type = calloc(1, sizeof(byte_array_t));
    hdr_frm_1.sender_type->buf = calloc(strlen("MONO") + 1, sizeof(char));
    memcpy(hdr_frm_1.sender_type->buf, "MONO", strlen("MONO"));
    hdr_frm_1.sender_type->len = strlen("MONO");
  } else {
    assert(0!=0 && "Unknown node type");
  }

  hdr_frm_1.vendor_name = calloc(1, sizeof(byte_array_t));
  hdr_frm_1.vendor_name->buf = calloc(strlen("OAI") + 1, sizeof(char));
  memcpy(hdr_frm_1.vendor_name->buf, "OAI", strlen("OAI"));
  hdr_frm_1.vendor_name->len = strlen("OAI");

  return hdr_frm_1;
}

kpm_ind_hdr_t kpm_ind_hdr(void)
{
  kpm_ind_hdr_t hdr = {0};

  hdr.type = FORMAT_1_INDICATION_HEADER;
  hdr.kpm_ric_ind_hdr_format_1 = kpm_ind_hdr_frm_1();

  return hdr;
}

void read_kpm_sm(void* data)
{
  assert(data != NULL);
  // assert(data->type == KPM_STATS_V3_0);

  kpm_rd_ind_data_t* const kpm = (kpm_rd_ind_data_t*)data;

  assert(kpm->act_def != NULL && "Cannot be NULL");

  // 7.8 Supported RIC Styles and E2SM IE Formats
  // Action Definition Format 4 corresponds to Indication Message Format 3
  switch (kpm->act_def->type) {
    case FORMAT_4_ACTION_DEFINITION: {
      kpm->ind.hdr = kpm_ind_hdr();

      kpm->ind.msg.type = FORMAT_3_INDICATION_MESSAGE;
      // Filter the UE by the test condition criteria
      kpm_act_def_format_4_t const* frm_4 = &kpm->act_def->frm_4; // 8.2.1.2.4
      for (size_t i = 0; i < frm_4->matching_cond_lst_len; i++) {
        switch (frm_4->matching_cond_lst[i].test_info_lst.test_cond_type) {
          case GBR_TEST_COND_TYPE: {
            assert(0 != 0 && "Not implemented");
            break;
          }
          case AMBR_TEST_COND_TYPE: {
            assert(0 != 0 && "Not implemented");
            break;
          }
          case IsStat_TEST_COND_TYPE: {
            assert(0 != 0 && "Not implemented");
            break;
          }
          case IsCatM_TEST_COND_TYPE: {
            assert(0 != 0 && "Not implemented");
            break;
          }

          case DL_RSRP_TEST_COND_TYPE: {
            assert(0 != 0 && "Not implemented");
            break;
          }

          case DL_RSRQ_TEST_COND_TYPE: {
            assert(0 != 0 && "Not implemented");
            break;
          }

          case UL_RSRP_TEST_COND_TYPE: {
            assert(0 != 0 && "Not implemented");
            break;
          }

          case CQI_TEST_COND_TYPE: {
            // This is a bad idea. Done only to check FlexRIC xAPP
            printf("CQI not implemented!. Randomly filling the data \n");
            goto rnd_data_label;
            break;
          }

          case fiveQI_TEST_COND_TYPE: {
            assert(0 != 0 && "Not implemented");
            break;
          }

          case QCI_TEST_COND_TYPE: {
            ;
            assert(0 != 0 && "Not implemented");
            break;
          }
          case S_NSSAI_TEST_COND_TYPE: {
            assert(frm_4->matching_cond_lst[i].test_info_lst.S_NSSAI == TRUE_TEST_COND_TYPE && "Must be true");
            assert(frm_4->matching_cond_lst[i].test_info_lst.test_cond != NULL && "Even though is optional..");
            assert(frm_4->matching_cond_lst[i].test_info_lst.int_value != NULL && "Even though is optional..");

            test_cond_e const test_cond = *frm_4->matching_cond_lst[i].test_info_lst.test_cond;
            int64_t const value = *frm_4->matching_cond_lst[i].test_info_lst.int_value;
            // Check E2 Node NG-RAN Type
            if (NODE_IS_DU(RC.nrrrc[0]->node_type)) {
              matched_ues_mac_t matched_ues = filter_ues_by_s_nssai_in_du_or_monolithic(test_cond, value);
              kpm->ind.msg.frm_3 = fill_kpm_ind_msg_frm_3_in_du(matched_ues, &frm_4->action_def_format_1);
            } else if (NODE_IS_CU(RC.nrrrc[0]->node_type)) {
              matched_ues_rrc_t matched_ues = filter_ues_by_s_nssai_in_cu(test_cond, value);
              kpm->ind.msg.frm_3 = fill_kpm_ind_msg_frm_3_in_cu(matched_ues, &frm_4->action_def_format_1);
            } else if (NODE_IS_MONOLITHIC(RC.nrrrc[0]->node_type)) {
              matched_ues_mac_t matched_ues = filter_ues_by_s_nssai_in_du_or_monolithic(test_cond, value);
              kpm->ind.msg.frm_3 = fill_kpm_ind_msg_frm_3_in_monolithic(matched_ues, &frm_4->action_def_format_1);
            } else {
              assert(false && "NG-RAN Type not implemented");
            }

            break;
          }

          default:
            assert(false && "Unknown Test condition");
        }
      }

      break;
    }

    default: {
    rnd_data_label:
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

