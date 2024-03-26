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

#include "ran_func_kpm_subs.h"

#include <search.h>

/* measurements that need to store values from previous reporting period have a limitation
   when it comes to multiple subscriptions to the same UEs; ric_req_id is unique per subscription */
typedef struct uldlcounter {
  uint32_t dl;
  uint32_t ul;
} uldlcounter_t;

static uldlcounter_t last_pdcp_sdu_total_bytes[MAX_MOBILES_PER_GNB] = {0};

static nr_pdcp_statistics_t get_pdcp_stats_per_drb(const uint32_t rrc_ue_id, const int rb_id)
{
  nr_pdcp_statistics_t pdcp = {0};
  const int srb_flag = 0;

  // Get PDCP stats for specific DRB
  const bool rc = nr_pdcp_get_statistics(rrc_ue_id, srb_flag, rb_id, &pdcp);
  assert(rc == true && "Cannot get PDCP stats\n");

  return pdcp;
}

/* 3GPP TS 28.522 - section 5.1.2.1.1.1
  note: this measurement is calculated as per spec */
static meas_record_lst_t fill_DRB_PdcpSduVolumeDL(__attribute__((unused))uint32_t gran_period_ms, cudu_ue_info_pair_t ue_info, const size_t ue_idx)
{
  meas_record_lst_t meas_record = {0};

  // Get PDCP stats per DRB
  const int rb_id = 1;  // at the moment, only 1 DRB is supported
  nr_pdcp_statistics_t pdcp = get_pdcp_stats_per_drb(ue_info.rrc_ue_id, rb_id);

  meas_record.value = INTEGER_MEAS_VALUE;

  // Get DL data volume delivered to PDCP layer
  meas_record.int_val = (pdcp.rxsdu_bytes - last_pdcp_sdu_total_bytes[ue_idx].dl)*8/1000;   // [kb]
  last_pdcp_sdu_total_bytes[ue_idx].dl = pdcp.rxsdu_bytes;

  return meas_record;
}

/* 3GPP TS 28.522 - section 5.1.2.1.2.1
  note: this measurement is calculated as per spec */
static meas_record_lst_t fill_DRB_PdcpSduVolumeUL(__attribute__((unused))uint32_t gran_period_ms, cudu_ue_info_pair_t ue_info, const size_t ue_idx)
{
  meas_record_lst_t meas_record = {0};

  // Get PDCP stats per DRB
  const int rb_id = 1;  // at the moment, only 1 DRB is supported
  nr_pdcp_statistics_t pdcp = get_pdcp_stats_per_drb(ue_info.rrc_ue_id, rb_id);

  meas_record.value = INTEGER_MEAS_VALUE;

  // Get UL data volume delivered from PDCP layer
  meas_record.int_val = (pdcp.txsdu_bytes - last_pdcp_sdu_total_bytes[ue_idx].ul)*8/1000;   // [kb]
  last_pdcp_sdu_total_bytes[ue_idx].ul = pdcp.txsdu_bytes;

  return meas_record;
}

#if defined (NGRAN_GNB_DU)
static uldlcounter_t last_rlc_pdu_total_bytes[MAX_MOBILES_PER_GNB] = {0};
static uldlcounter_t last_total_prbs[MAX_MOBILES_PER_GNB] = {0};

static nr_rlc_statistics_t get_rlc_stats_per_drb(const rnti_t rnti, const int rb_id)
{
  nr_rlc_statistics_t rlc = {0};
  const int srb_flag = 0;

  // Get RLC stats for specific DRB
  const bool rc = nr_rlc_get_statistics(rnti, srb_flag, rb_id, &rlc);
  assert(rc == true && "Cannot get RLC stats\n");

  // Activate average sojourn time at the RLC buffer for specific DRB
  nr_rlc_activate_avg_time_to_tx(rnti, rb_id+3, 1);
  
  return rlc;  
}

/* 3GPP TS 28.522 - section 5.1.3.3.3
  note: by default this measurement is calculated for previous 100ms (openair2/LAYER2/nr_rlc/nr_rlc_entity.c:118, 173, 213); please, update according to your needs */
static meas_record_lst_t fill_DRB_RlcSduDelayDl(__attribute__((unused))uint32_t gran_period_ms, cudu_ue_info_pair_t ue_info, __attribute__((unused))const size_t ue_idx)
{
  meas_record_lst_t meas_record = {0};
  
  // Get RLC stats per DRB
  const int rb_id = 1;  // at the moment, only 1 DRB is supported
  nr_rlc_statistics_t rlc = get_rlc_stats_per_drb(ue_info.ue->rnti, rb_id);

  meas_record.value = REAL_MEAS_VALUE;

  // Get the value of sojourn time at the RLC buffer
  meas_record.real_val = rlc.txsdu_avg_time_to_tx;  // [μs]

  return meas_record;
}

/* 3GPP TS 28.522 - section 5.1.1.3.1
  note: per spec, average UE throughput in DL (taken into consideration values from all UEs, and averaged)
        here calculated as: UE specific throughput in DL */
static meas_record_lst_t fill_DRB_UEThpDl(uint32_t gran_period_ms, cudu_ue_info_pair_t ue_info, const size_t ue_idx)
{
  meas_record_lst_t meas_record = {0};
  
  // Get RLC stats per DRB
  const int rb_id = 1;  // at the moment, only 1 DRB is supported
  nr_rlc_statistics_t rlc = get_rlc_stats_per_drb(ue_info.ue->rnti, rb_id);
  meas_record.value = REAL_MEAS_VALUE;

  // Calculate DL Thp
  meas_record.real_val = (double)(rlc.txpdu_bytes - last_rlc_pdu_total_bytes[ue_idx].dl)*8/gran_period_ms;  // [kbps]
  last_rlc_pdu_total_bytes[ue_idx].dl = rlc.txpdu_bytes;

  return meas_record;
}

/* 3GPP TS 28.522 - section 5.1.1.3.3
  note: per spec, average UE throughput in UL (taken into consideration values from all UEs, and averaged)
        here calculated as: UE specific throughput in UL */
static meas_record_lst_t fill_DRB_UEThpUl(uint32_t gran_period_ms, cudu_ue_info_pair_t ue_info, const size_t ue_idx)
{
  meas_record_lst_t meas_record = {0};
  
  // Get RLC stats per DRB
  const int rb_id = 1;  // at the moment, only 1 DRB is supported
  nr_rlc_statistics_t rlc = get_rlc_stats_per_drb(ue_info.ue->rnti, rb_id);

  meas_record.value = REAL_MEAS_VALUE;

  // Calculate UL Thp
  meas_record.real_val = (double)(rlc.rxpdu_bytes - last_rlc_pdu_total_bytes[ue_idx].ul)*8/gran_period_ms;  // [kbps]
  last_rlc_pdu_total_bytes[ue_idx].ul = rlc.rxpdu_bytes;
  
  return meas_record;
}

/* 3GPP TS 28.522 - section 5.1.1.2.1
  note: per spec, DL PRB usage [%] = (total used PRBs for DL traffic / total available PRBs for DL traffic) * 100   
        here calculated as: aggregated DL PRBs (t) - aggregated DL PRBs (t-gran_period) */
static meas_record_lst_t fill_RRU_PrbTotDl(__attribute__((unused))uint32_t gran_period_ms, cudu_ue_info_pair_t ue_info, const size_t ue_idx)
{
  meas_record_lst_t meas_record = {0};
  
  meas_record.value = INTEGER_MEAS_VALUE;

  // Get the number of DL PRBs
  meas_record.int_val = ue_info.ue->mac_stats.dl.total_rbs - last_total_prbs[ue_idx].dl;   // [PRBs]
  last_total_prbs[ue_idx].dl = ue_info.ue->mac_stats.dl.total_rbs;

  return meas_record;
}

/* 3GPP TS 28.522 - section 5.1.1.2.2
  note: per spec, UL PRB usage [%] = (total used PRBs for UL traffic / total available PRBs for UL traffic) * 100   
        here calculated as: aggregated UL PRBs (t) - aggregated UL PRBs (t-gran_period) */
static meas_record_lst_t fill_RRU_PrbTotUl(__attribute__((unused))uint32_t gran_period_ms, cudu_ue_info_pair_t ue_info, const size_t ue_idx)
{
  meas_record_lst_t meas_record = {0};

  meas_record.value = INTEGER_MEAS_VALUE;

  // Get the number of UL PRBs
  meas_record.int_val = ue_info.ue->mac_stats.ul.total_rbs - last_total_prbs[ue_idx].ul;   // [PRBs]
  last_total_prbs[ue_idx].ul = ue_info.ue->mac_stats.ul.total_rbs;

  return meas_record;
}
#endif

static kv_measure_t lst_measure[] = {
  {.key = "DRB.PdcpSduVolumeDL", .value = fill_DRB_PdcpSduVolumeDL }, 
  {.key = "DRB.PdcpSduVolumeUL", .value = fill_DRB_PdcpSduVolumeUL },
#if defined (NGRAN_GNB_DU)
  {.key = "DRB.RlcSduDelayDl", .value =  fill_DRB_RlcSduDelayDl }, 
  {.key = "DRB.UEThpDl", .value =  fill_DRB_UEThpDl }, 
  {.key = "DRB.UEThpUl", .value =  fill_DRB_UEThpUl }, 
  {.key = "RRU.PrbTotDl", .value =  fill_RRU_PrbTotDl }, 
  {.key = "RRU.PrbTotUl", .value =  fill_RRU_PrbTotUl }, 
#endif
}; 

void init_kpm_subs_data(void)
{
  const size_t ht_len = sizeof(lst_measure) / sizeof(lst_measure[0]);
  hcreate(ht_len);

  ENTRY kv_pair;

  for (size_t i = 0; i < ht_len; i++) {
    kv_pair.key = lst_measure[i].key;
    kv_pair.data = &lst_measure[i];
    hsearch(kv_pair, ENTER);
  }
}

meas_record_lst_t get_kpm_meas_value(char* kpm_meas_name, uint32_t gran_period_ms, cudu_ue_info_pair_t ue_info, const size_t ue_idx)
{
  assert(kpm_meas_name != NULL);

  ENTRY search_entry = {.key = kpm_meas_name};
  ENTRY *found_entry = hsearch(search_entry, FIND);
  assert(found_entry != NULL && "Unsupported KPM measurement name");

  kv_measure_t *kv_found = (kv_measure_t *)found_entry->data;
  meas_record_lst_t meas_record = kv_found->value(gran_period_ms, ue_info, ue_idx);

  return meas_record;
}
