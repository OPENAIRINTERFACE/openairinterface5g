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

/*! \file vcd_signal_dumper.c
 * \brief Dump functions calls and variables to VCD file. Use GTKWave to display this file.
 * \author S. Roux
 * \maintainer: navid nikaein
 * \date 2012 - 2104
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr
 * \note
 * \warning
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <error.h>
#include <time.h>
#include <unistd.h>

#include "assertions.h"

#include "vcd_signal_dumper.h"

#define VCDSIGNALDUMPER_VERSION_MAJOR 0
#define VCDSIGNALDUMPER_VERSION_MINOR 1

// Global variable. If the VCD option is set at execution time, output VCD trace. Otherwise this module has no effect.
int ouput_vcd = 0;

struct vcd_module_s {
  const char     *name;
  int             number_of_signals;
  const char    **signals_names;
  vcd_signal_type signal_type;
  int             signal_size;
} vcd_module_s;

const char* eurecomVariablesNames[] = {
  "frame_number_TX0_eNB",
  "mask_ru",
  "mask_tx_ru",
  "frame_number_TX1_eNB",
  "frame_number_RX0_eNB",
  "frame_number_RX1_eNB",
  "subframe_number_TX0_eNB",
  "subframe_number_TX1_eNB",
  "subframe_number_RX0_eNB",
  "subframe_number_RX1_eNB",
  "frame_number_TX0_RU",
  "frame_number_TX1_RU",
  "frame_number_RX0_RU",
  "frame_number_RX1_RU",
  "tti_number_TX0_RU",
  "tti_number_TX1_RU",
  "tti_number_RX0_RU",
  "tti_number_RX1_RU",
  "subframe_number_if4p5_north_out",
  "frame_number_if4p5_north_out",
  "subframe_number_if4p5_north_asynch_in",
  "frame_number_if4p5_north_asynch_in",
  "subframe_number_if4p5_south_out_ru",
  "subframe_number_if4p5_south_out_ru1",
  "subframe_number_if4p5_south_out_ru2",
  "frame_number_if4p5_south_out_ru",
  "frame_number_if4p5_south_out_ru1",
  "frame_number_if4p5_south_out_ru2",
  "subframe_number_if4p5_south_in_ru",
  "subframe_number_if4p5_south_in_ru1",
  "subframe_number_if4p5_south_in_ru2",
  "frame_number_if4p5_south_in_ru",
  "frame_number_if4p5_south_in_ru1",
  "frame_number_if4p5_south_in_ru2",
  "subframe_number_wakeup_l1s_ru",
  "subframe_number_wakeup_l1s_ru1",
  "subframe_number_wakeup_l1s_ru2",
  "frame_number_wakeup_l1s_ru",
  "frame_number_wakeup_l1s_ru1",
  "frame_number_wakeup_l1s_ru2",
  "subframe_number_wakeup_rxtx_rx_ru",
  "subframe_number_wakeup_rxtx_rx_ru1",
  "subframe_number_wakeup_rxtx_rx_ru2",
  "frame_number_wakeup_rxtx_rx_ru",
  "frame_number_wakeup_rxtx_rx_ru1",
  "frame_number_wakeup_rxtx_rx_ru2",
  "subframe_number_wakeup_rxtx_tx_ru",
  "subframe_number_wakeup_rxtx_tx_ru1",
  "subframe_number_wakeup_rxtx_tx_ru2",
  "frame_number_wakeup_rxtx_tx_ru",
  "frame_number_wakeup_rxtx_tx_ru1",
  "frame_number_wakeup_rxtx_tx_ru2",
  "ic_enb",
  "ic_enb1",
  "ic_enb2",
  "l1_proc_ic",
  "l1_proc_tx_ic",
  "runtime_TX_eNB",
  "runtime_RX_eNB",
  "frame_number_TX0_UE",
  "frame_number_TX1_UE",
  "frame_number_RX0_UE",
  "frame_number_RX1_UE",
  "subframe_TX0_UE",
  "subframe_TX1_UE",
  "subframe_RX0_UE",
  "subframe_RX1_UE",
  "ue_rx_offset",
  "diff2",
  "hw_subframe",
  "hw_frame",
  "hw_subframe_rx",
  "hw_frame_rx",
  "txcnt",
  "rxcnt",
  "trx_ts",
  "trx_tst",
  "trx_ts_ue",
  "trx_tst_ue",
  "trx_write_flags",
  "tx_ts",
  "rx_ts",
  "hw_cnt_rx",
  "lhw_cnt_rx",
  "hw_cnt_tx",
  "lhw_cnt_tx",
  "pck_rx",
  "pck_tx",
  "rx_seq_num",
  "rx_seq_num_prv",
  "tx_seq_num",
  "cnt",
  "dummy_dump",
  "itti_send_msg",
  "itti_poll_msg",
  "itti_recv_msg",
  "itti_alloc_msg",
  "mp_alloc",
  "mp_free",
  "ue_inst_cnt_rx",
  "ue_inst_cnt_tx",
  "dci_info",
  "ue0_BSR",
  "ue0_BO",
  "ue0_scheduled",
  "ue0_timing_advance",
  "ue0_SR_ENERGY",
  "ue0_SR_THRES",
  "ue0_rssi0",
  "ue0_rssi1",
  "ue0_rssi2",
  "ue0_rssi3",
  "ue0_rssi4",
  "ue0_rssi5",
  "ue0_rssi6",
  "ue0_rssi7",
  "ue0_res0",
  "ue0_res1",
  "ue0_res2",
  "ue0_res3",
  "ue0_res4",
  "ue0_res5",
  "ue0_res6",
  "ue0_res7",
  "ue0_MCS0",
  "ue0_MCS1",
  "ue0_MCS2",
  "ue0_MCS3",
  "ue0_MCS4",
  "ue0_MCS5",
  "ue0_MCS6",
  "ue0_MCS7",
  "ue0_RB0",
  "ue0_RB1",
  "ue0_RB2",
  "ue0_RB3",
  "ue0_RB4",
  "ue0_RB5",
  "ue0_RB6",
  "ue0_RB7",
  "ue0_ROUND0",
  "ue0_ROUND1",
  "ue0_ROUND2",
  "ue0_ROUND3",
  "ue0_ROUND4",
  "ue0_ROUND5",
  "ue0_ROUND6",
  "ue0_ROUND7",
  "ue0_SFN0",
  "ue0_SFN1",
  "ue0_SFN2",
  "ue0_SFN3",
  "ue0_SFN4",
  "ue0_SFN5",
  "ue0_SFN6",
  "ue0_SFN7",
  "send_if4_symbol",
  "recv_if4_symbol",
  "send_if5_pkt_id",
  "recv_if5_pkt_id",
  "ue_pdcp_flush_size",
  "ue_pdcp_flush_err",
  "ue0_trx_read_ns",
  "ue0_trx_write_ns",
  "ue0_trx_read_ns_missing",
  "ue0_trx_write_ns_missing",
  "enb_thread_rxtx_CPUID",
  "ru_thread_CPUID",
  "ru_thread_tx_CPUID",
  "ue0_on_duration_timer",
  "ue0_drx_inactivity",
  "ue0_drx_short_cycle",
  "ue0_short_drx_cycle_number",
  "ue0_drx_long_cycle",
  "ue0_drx_retransmission_harq0",
  "ue0_drx_active_time",
  "ue0_drx_active_time_condition",
  /*signal for NR*/
  "frame_number_TX0_gNB",
  "frame_number_TX1_gNB",
  "frame_number_RX0_gNB",
  "frame_number_RX1_gNB",
  "slot_number_TX0_gNB",
  "slot_number_TX1_gNB",
  "slot_number_RX0_gNB",
  "slot_number_RX1_gNB",
  "ru_tx_ofdm_mask",
  "usrp_send_return"
};

const char* eurecomFunctionsNames[] = {
  /*  softmodem signals   */
  "rt_sleep",
  "trx_read",
  "trx_write",
  "trx_read_ue",
  "trx_write_ue",
  "trx_read_if0",
  "trx_read_if1",
  "trx_write_if0",
  "trx_write_if1",
  "eNB_thread_rxtx0",
  "eNB_thread_rxtx1",
  "ue_thread_synch",
  "ue_thread_rxtx0",
  "ue_thread_rxtx1",
  "trx_read_sf9",
  "trx_write_sf9",
  "ue_signal_cond_rxtx0",
  "ue_signal_cond_rxtx1",
  "ue_wait_cond_rxtx0",
  "ue_wait_cond_rxtx1",
  "ue_lock_mutex_rxtx_for_cond_wait0",
  "ue_lock_mutex_rxtx_for_cond_wait1",
  "ue_lock_mutex_rxtx_for_cnt_decrement0",
  "ue_lock_mutex_rxtx_for_cnt_decrement1",
  "ue_lock_mutex_rxtx_for_cnt_increment0",
  "ue_lock_mutex_rxtx_for_cnt_increment1",
  "lock_mutex_ru",
  "lock_mutex_ru1",
  "lock_mutex_ru2",
  /* uhd signals */
  "trx_write_thread",
  /* simulation signals */
  "do_DL_sig",
  "do_UL_sig",
  "UE_trx_read",
  /* RRH signals  */
  "eNB_tx",
  "eNB_rx",
  "eNB_trx",
  "eNB_tm",
  "eNB_rx_sleep",
  "eNB_tx_sleep",
  "eNB_proc_sleep",
  "trx_read_rf",
  "trx_write_rf",
  /* PHY signals  */
  "ue_synch",
  "ue_slot_fep",
  "ue_slot_fep_pdcch",
  "ue_slot_fep_pbch",
  "ue_slot_fep_pdsch",
  "ue_slot_fep_mbsfn",
  "ue_slot_fep_mbsfn_khz_1dot25",
  "ue_rrc_measurements",
  "ue_gain_control",
  "ue_adjust_synch",
  "lte_ue_measurement_procedures",
  "lte_ue_pdcch_procedures",
  "lte_ue_pbch_procedures",
  "phy_procedures_eNb_tx0",
  "phy_procedures_eNb_tx1",
  "phy_procedures_ru_feprx0",
  "phy_procedures_ru_feprx1",
  "phy_procedures_ru_feprx2",
  "phy_procedures_ru_feprx3",
  "phy_procedures_ru_feprx4",
  "phy_procedures_ru_feprx5",
  "phy_procedures_ru_feprx6",
  "phy_procedures_ru_feprx7",
  "phy_procedures_ru_feprx8",
  "phy_procedures_ru_feprx9",
  "phy_procedures_ru_feptx_ofdm0",
  "phy_procedures_ru_feptx_ofdm1",
  "phy_procedures_ru_feptx_ofdm2",
  "phy_procedures_ru_feptx_ofdm3",
  "phy_procedures_ru_feptx_ofdm4",
  "phy_procedures_ru_feptx_ofdm5",
  "phy_procedures_ru_feptx_ofdm6",
  "phy_procedures_ru_feptx_ofdm7",
  "phy_procedures_ru_feptx_ofdm8",
  "phy_procedures_ru_feptx_ofdm9",
  "phy_procedures_ru_feptx_ofdm10",
  "phy_procedures_ru_feptx_ofdm11",
  "phy_procedures_ru_feptx_ofdm12",
  "phy_procedures_ru_feptx_ofdm13",
  "phy_procedures_ru_feptx_ofdm14",
  "phy_procedures_ru_feptx_ofdm15",
  "phy_procedures_ru_feptx_ofdm16",
  "phy_procedures_ru_feptx_prec0",
  "phy_procedures_ru_feptx_prec1",
  "phy_procedures_ru_feptx_prec2",
  "phy_procedures_ru_feptx_prec3",
  "phy_procedures_ru_feptx_prec4",
  "phy_procedures_ru_feptx_prec5",
  "phy_procedures_ru_feptx_prec6",
  "phy_procedures_ru_feptx_prec7",
  "phy_procedures_ru_feptx_prec8",
  "phy_procedures_ru_feptx_prec9",
  "phy_procedures_eNb_rx_uespec0",
  "phy_procedures_eNb_rx_uespec1",
  "phy_procedures_ue_tx",
  "phy_procedures_ue_rx",
  "phy_procedures_ue_tx_ulsch_uespec",
  "phy_procedures_nr_ue_tx_ulsch_uespec",
  "phy_procedures_ue_tx_pucch",
  "phy_procedures_ue_tx_ulsch_common",
  "phy_procedures_ue_tx_prach",
  "phy_procedures_ue_tx_ulsch_rar",
  "phy_procedures_eNB_lte",
  "phy_procedures_UE_lte",
  "pdsch_thread",
  "dlsch_thread0",
  "dlsch_thread1",
  "dlsch_thread2",
  "dlsch_thread3",
  "dlsch_thread4",
  "dlsch_thread5",
  "dlsch_thread6",
  "dlsch_thread7",
  "dlsch_decoding0",
  "dlsch_decoding1",
  "dlsch_decoding2",
  "dlsch_decoding3",
  "dlsch_decoding4",
  "dlsch_decoding5",
  "dlsch_decoding6",
  "dlsch_decoding7",
  "dlsch_segmentation",
  "dlsch_deinterleaving",
  "dlsch_rate_matching",
  "dlsch_ldpc",
  "dlsch_compine_seg",
  "dlsch_pmch_decoding",
  "rx_pdcch",
  "dci_decoding",
  "rx_phich",
  "rx_pmch",
  "rx_pmch_khz_1dot25",
  "pdsch_procedures",
  "pdsch_procedures_crnti",
  //"dlsch_procedures_crnti",
  "pdsch_procedures_si",
  "pdsch_procedures_p",
  "pdsch_procedures_ra",
  "phy_ue_config_sib2",
  "macxface_phy_config_sib1_eNB",
  "macxface_phy_config_sib2_eNB",
  "macxface_phy_config_dedicated_eNB",
  "phy_ue_compute_prach",
  "phy_enb_ulsch_msg3",
  "phy_enb_ulsch_decoding0",
  "phy_enb_ulsch_decoding1",
  "phy_enb_ulsch_decoding2",
  "phy_enb_ulsch_decoding3",
  "phy_enb_ulsch_decoding4",
  "phy_enb_ulsch_decoding5",
  "phy_enb_ulsch_decoding6",
  "phy_enb_ulsch_decoding7",
  "phy_enb_sfgen",
  "phy_enb_prach_rx",
  "phy_ru_prach_rx",
  "phy_enb_pdcch_tx",
  "phy_enb_common_tx",
  "phy_enb_rs_tx",
  "phy_ue_generate_prach",
  "phy_ue_ulsch_modulation",
  "phy_ue_ulsch_encoding",
#if 1 // add for debugging losing PDSCH immediately before and after reporting CQI
  "phy_ue_ulsch_encoding_fill_cqi",
#endif
  "phy_ue_ulsch_scrambling",
  "phy_eNB_dlsch_modulation",
  "phy_eNB_dlsch_encoding",
  "phy_eNB_dlsch_encoding_w",
  "phy_eNB_dlsch_scrambling",
  "phy_eNB_beam_precoding",
  "phy_eNB_ofdm_mod_l",
  /* MAC  signals  */
  "macxface_macphy_init",
  "macxface_macphy_exit",
  "macxface_eNB_dlsch_ulsch_scheduler",
  "macxface_fill_rar",
  "macxface_terminate_ra_proc",
  "macxface_initiate_ra_proc",
  "macxface_cancel_ra_proc",
  "macxface_get_dci_sdu",
  "macxface_get_dlsch_sdu",
  "macxface_rx_sdu",
  "macxface_mrbch_phy_sync_failure",
  "macxface_SR_indication",
  "mac_dlsch_preprocessor",
  "mac_schedule_dlsch",
  "mac_fill_dlsch_dci",
  "macxface_out_of_sync_ind",
  "macxface_ue_decode_si",
  "macxface_ue_decode_pcch",
  "macxface_ue_decode_ccch",
  "macxface_ue_decode_bcch",
  "macxface_ue_send_sdu",
  "macxface_ue_get_sdu",
  "macxface_ue_get_rach",
  "macxface_ue_process_rar",
  "macxface_ue_scheduler",
  "macxface_ue_get_sr",
  "ue_send_mch_sdu",
  /*RLC signals   */
  "rlc_data_req",
  // "rlc_data_ind", // this calls "pdcp_data_ind",
  "mac_rlc_status_ind",
  "mac_rlc_data_req",
  "mac_rlc_data_ind",
  "rlc_um_try_reassembly",
  "rlc_um_check_timer_dar_time_out",
  "rlc_um_receive_process_dar",
  /* PDCP signals   */
  "pdcp_run",
  "pdcp_data_req",
  "pdcp_data_ind",
  "pdcp_apply_security",
  "pdcp_validate_security",
  "pdcp_fifo_read",
  "pdcp_fifo_read_buffer",
  "pdcp_fifo_flush",
  "pdcp_fifo_flush_buffer",
  "pdcp_mbms_fifo_read",
  "pdcp_mbms_fifo_read_buffer",
  /* RRC signals  */
  "rrc_rx_tx",
  "rrc_mac_config_req",
  "rrc_ue_decode_sib1",
  "rrc_ue_decode_si",
  /* GTPV1U signals */
  "gtpv1u_enb_task",
  "gtpv1u_process_udp_req",
  "gtpv1u_process_tunnel_data_req",
  /* UDP signals */
  "udp_enb_task",
  /* MISC signals  */
  "emu_transport",
  "log_record",
  "itti_enqueue_message",
  "itti_dump_enqueue_message",
  "itti_dump_enqueue_message_malloc",
  "itti_relay_thread",
  "test",
  /* IF4/IF5 signals */
  "send_if4_ru",
  "send_if4_ru1",
  "send_if4_ru2",
  "recv_if4_ru",
  "recv_if4_ru1",
  "recv_if4_ru2",
  "send_if5",
  "recv_if5",
  "compress_if",
  "decompress_if",
  "nfapi_subframe",
  "generate_pcfich",
  "generate_dci0",
  "generate_dlsch",
  "generate_phich",
  "pdcch_scrambling",
  "pdcch_modulation",
  "pdcch_interleaving",
  "pdcch_tx",
  /*NR softmodem signal*/
  "wakeup_txfh",
  "gNB_thread_rxtx0",
  "gNB_thread_rxtx1",
  "ru_thread_tx_wait",
  "gNB_ulsch_decoding",
  "gNB_pdsch_codeword_scrambling",
  "gNB_dlsch_encoding",
  "gNB_pdsch_modulation",
  "gNB_pdcch_tx",
  "phy_procedures_gNB_tx",
  "phy_procedures_gNB_common_tx",
  "phy_procedures_gNB_uespec_rx",
  "nr_rx_pusch",
  "nr_ulsch_procedures_rx",
  "macxface_gNB_dlsch_ulsch_scheduler",

  /*NR ue-softmodem signal*/
  "nr_ue_ulsch_encoding",
  "nr_segmentation",
  "ldpc_encoder_optim",
  "nr_rate_matching_ldpc",
  "nr_interleaving_ldpc",
  "pss_synchro_nr",
  "pss_search_time_nr",
  "nr_initial_ue_sync",
  "beam_switching_gpio"
};

struct vcd_module_s vcd_modules[] = {
  { "variables", VCD_SIGNAL_DUMPER_VARIABLES_END, eurecomVariablesNames, VCD_WIRE, 64 },
  { "functions", VCD_SIGNAL_DUMPER_FUNCTIONS_END, eurecomFunctionsNames, VCD_WIRE, 1 },
  //    { "ue_procedures_functions", VCD_SIGNAL_DUMPER_UE_PROCEDURES_FUNCTIONS_END, eurecomUEFunctionsNames, VCD_WIRE, 1 },
};

FILE *vcd_fd = NULL;
#if defined(ENABLE_VCD_FIFO)
static inline unsigned long long int vcd_get_time(void);
#endif

#if defined(ENABLE_USE_CPU_EXECUTION_TIME)
struct timespec     g_time_start;
#endif


#if defined(ENABLE_VCD_FIFO)

# define VCD_POLL_DELAY         (500)           // Poll delay in micro-seconds
# define VCD_MAX_WAIT_DELAY     (200 * 1000)    // Maximum data ready wait delay in micro-seconds
# define VCD_FIFO_NB_ELEMENTS   (1 << 24)       // Must be a power of 2
# define VCD_FIFO_MASK          (VCD_FIFO_NB_ELEMENTS - 1)

typedef struct vcd_queue_user_data_s {
  uint32_t log_id;
  vcd_signal_dumper_modules module;
  union data_u {
    struct function_s {
      vcd_signal_dump_functions function_name;
      vcd_signal_dump_in_out    in_out;
    } function;
    struct variable_s {
      vcd_signal_dump_variables variable_name;
      unsigned long value;
    } variable;
  } data;

  long long unsigned int time;
} vcd_queue_user_data_t;

typedef struct vcd_fifo_s {
  vcd_queue_user_data_t user_data[VCD_FIFO_NB_ELEMENTS];

  volatile uint32_t write_index;
  volatile uint32_t read_index;
} vcd_fifo_t;

vcd_fifo_t vcd_fifo;

pthread_t vcd_dumper_thread;
#endif

#define BYTE_SIZE   8
#define NIBBLE_SIZE 4
static void uint64_to_binary(uint64_t value, char *binary)
{
  static const char * const nibbles_start[] = {
    "",    "1",   "10",   "11",
    "100",  "101",  "110",  "111",
    "1000", "1001", "1010", "1011",
    "1100", "1101", "1110", "1111",
  };
  static const char * const nibbles[] = {
    "0000", "0001", "0010", "0011",
    "0100", "0101", "0110", "0111",
    "1000", "1001", "1010", "1011",
    "1100", "1101", "1110", "1111",
  };
  int nibble;
  int nibble_value;
  int nibble_size;
  int zero = 1;

  for (nibble = 0; nibble < (sizeof (uint64_t) * (BYTE_SIZE / NIBBLE_SIZE)); nibble++) {
    nibble_value = value >> ((sizeof (uint64_t) * BYTE_SIZE) - NIBBLE_SIZE);

    if (zero) {
      if (nibble_value > 0) {
        zero = 0;
        nibble_size = strlen(nibbles_start[nibble_value]);
        memcpy (binary, nibbles_start[nibble_value], nibble_size);
        binary += nibble_size;
      }
    } else {
      memcpy (binary, nibbles[nibble_value], NIBBLE_SIZE);
      binary += NIBBLE_SIZE;
    }

    value <<= NIBBLE_SIZE;
  }

  /* Add a '0' if the value was null */
  if (zero) {
    binary[0] = '0';
    binary ++;
  }

  /* Add a null value at the end of the string */
  binary[0] = '\0';
}

#if defined(ENABLE_VCD_FIFO)
inline static uint32_t vcd_get_write_index(void)
{
  uint32_t write_index;
  uint32_t read_index;

  /* Get current write index and increment it (atomic operation) */
  write_index = __sync_fetch_and_add(&vcd_fifo.write_index, 1);
  /* Wrap index */
  write_index &= VCD_FIFO_MASK;

  /* Check FIFO overflow (increase VCD_FIFO_NB_ELEMENTS if this assert is triggered) */
  DevCheck((read_index = vcd_fifo.read_index, ((write_index + 1) & VCD_FIFO_MASK) != read_index), write_index, read_index, VCD_FIFO_NB_ELEMENTS);

  return write_index;
}


void *vcd_dumper_thread_rt(void *args)
{
  vcd_queue_user_data_t *data;
  char binary_string[(sizeof (uint64_t) * BYTE_SIZE) + 1];
  struct sched_param sched_param;
  uint32_t data_ready_wait;

  return 0; //signal_mask(); //function defined at common/utils/ocp_itti/intertask_interface.cpp

  sched_param.sched_priority = sched_get_priority_min(SCHED_FIFO) + 1;
  sched_setscheduler(0, SCHED_FIFO, &sched_param);

  while(1) {
    if (vcd_fifo.read_index == (vcd_fifo.write_index & VCD_FIFO_MASK)) {
      /* No element -> sleep a while */
      usleep(VCD_POLL_DELAY);
    } else {
      data = &vcd_fifo.user_data[vcd_fifo.read_index];
      data_ready_wait = 0;

      while (data->module == VCD_SIGNAL_DUMPER_MODULE_FREE) {
        /* Check wait delay (increase VCD_MAX_WAIT_DELAY if this assert is triggered and that no thread is locked) */
        DevCheck(data_ready_wait < VCD_MAX_WAIT_DELAY, data_ready_wait, VCD_MAX_WAIT_DELAY, 0);

        /* data is not yet ready, wait for it to be completed */
        data_ready_wait += VCD_POLL_DELAY;
        usleep(VCD_POLL_DELAY);
      }

      switch (data->module) {
      case VCD_SIGNAL_DUMPER_MODULE_VARIABLES:
        if (vcd_fd != NULL) {
          int variable_name;
          variable_name = (int)data->data.variable.variable_name;
          fprintf(vcd_fd, "#%llu\n", data->time);
          /* Set variable to value */
          uint64_to_binary(data->data.variable.value, binary_string);
          fprintf(vcd_fd, "b%s %s_w\n", binary_string,
                  eurecomVariablesNames[variable_name]);
        }

        break;

      case VCD_SIGNAL_DUMPER_MODULE_FUNCTIONS:
        if (vcd_fd != NULL) {
          int function_name;

          function_name = (int)data->data.function.function_name;
          fprintf(vcd_fd, "#%llu\n", data->time);

          /* Check if we are entering or leaving the function ( 0 = leaving, 1 = entering) */
          if (data->data.function.in_out == VCD_FUNCTION_IN)
            /* Set event to 1 */
            fprintf(vcd_fd, "1%s_w\n", eurecomFunctionsNames[function_name]);
          else
            fprintf(vcd_fd, "0%s_w\n", eurecomFunctionsNames[function_name]);

          fflush(vcd_fd);
        }

        break;

      default:
        DevParam(data->module, 0, 0);
        break;
      }

      data->module = VCD_SIGNAL_DUMPER_MODULE_FREE;
      vcd_fifo.read_index = (vcd_fifo.read_index + 1) & VCD_FIFO_MASK;
    }
  }

  return NULL;
}
#endif

void vcd_signal_dumper_init(char *filename)
{
  if (ouput_vcd) {
    //        char filename[] = "/tmp/openair_vcd_dump.vcd";

    if ((vcd_fd = fopen(filename, "w+")) == NULL) {
      perror("vcd_signal_dumper_init: cannot open file");
      return;
    }

#if defined(ENABLE_USE_CPU_EXECUTION_TIME)
    clock_gettime(CLOCK_MONOTONIC, &g_time_start);
#endif

    vcd_signal_dumper_create_header();

#if defined(ENABLE_VCD_FIFO)
    vcd_fifo.write_index = 0;
    vcd_fifo.read_index = 0;

    fprintf(stderr, "[VCD] Creating dumper thread\n");

    if (pthread_create(&vcd_dumper_thread, NULL, vcd_dumper_thread_rt, NULL) < 0) {
      fprintf(stderr, "vcd_signal_dumper_init: Failed to create thread: %s\n",
              strerror(errno));
      ouput_vcd = 0;
      return;
    }

#endif
  }
}

void vcd_signal_dumper_close(void)
{
  if (ouput_vcd) {
#if defined(ENABLE_VCD_FIFO)

#else

    if (vcd_fd != NULL) {
      fclose(vcd_fd);
      vcd_fd = NULL;
    }

#endif
  }
}

static inline void vcd_signal_dumper_print_time_since_start(void)
{
  if (vcd_fd != NULL) {
#if defined(ENABLE_USE_CPU_EXECUTION_TIME)
    struct timespec time;
    long long unsigned int nanosecondsSinceStart;
    long long unsigned int secondsSinceStart;

    clock_gettime(CLOCK_MONOTONIC, &time);

    /* Get current execution time in nanoseconds */
    nanosecondsSinceStart = (long long unsigned int)((time.tv_nsec - g_time_start.tv_nsec));
    secondsSinceStart     = (long long unsigned int)time.tv_sec - (long long unsigned int)g_time_start.tv_sec;
    /* Write time in nanoseconds */
    fprintf(vcd_fd, "#%llu\n", nanosecondsSinceStart + (secondsSinceStart * 1000000000UL));
#endif
  }
}

#if defined(ENABLE_VCD_FIFO)
static inline unsigned long long int vcd_get_time(void)
{
#if defined(ENABLE_USE_CPU_EXECUTION_TIME)
  struct timespec time;

  clock_gettime(CLOCK_MONOTONIC, &time);

  return (long long unsigned int)((time.tv_nsec - g_time_start.tv_nsec)) +
         ((long long unsigned int)time.tv_sec - (long long unsigned int)g_time_start.tv_sec) * 1000000000UL;
#endif
  return 0;
}
#endif

void vcd_signal_dumper_create_header(void)
{
  if (ouput_vcd) {
    struct tm *pDate;
    time_t intps;

    intps = time(NULL);
    pDate = localtime(&intps);

    if (vcd_fd != NULL) {
      int i, j;
      fprintf(vcd_fd, "$date\n\t%s$end\n", asctime(pDate));
      // Display version
      fprintf(vcd_fd, "$version\n\tVCD plugin ver%d.%d\n$end\n", VCDSIGNALDUMPER_VERSION_MAJOR, VCDSIGNALDUMPER_VERSION_MINOR);
      // Init timescale, here = 1ns
      fprintf(vcd_fd, "$timescale 1 ns $end\n");

      /* Initialize each module definition */
      for(i = 0; i < sizeof(vcd_modules) / sizeof(struct vcd_module_s); i++) {
        struct vcd_module_s *module;
        module = &vcd_modules[i];
        fprintf(vcd_fd, "$scope module %s $end\n", module->name);

        /* Declare each signal as defined in array */
        for (j = 0; j < module->number_of_signals; j++) {
          const char *signal_name;
          signal_name = module->signals_names[j];

          if (VCD_WIRE == module->signal_type) {
            fprintf(vcd_fd, "$var wire %d %s_w %s $end\n", module->signal_size, signal_name, signal_name);
          } else  if (VCD_REAL == module->signal_type) {
            fprintf(vcd_fd, "$var real %d %s_r %s $end\n", module->signal_size, signal_name, signal_name);
          } else {
            // Handle error here
          }
        }

        fprintf(vcd_fd, "$upscope $end\n");
      }

      /* Init variables and functions to 0 */
      fprintf(vcd_fd, "$dumpvars\n");

      for(i = 0; i < sizeof(vcd_modules) / sizeof(struct vcd_module_s); i++) {
        struct vcd_module_s *module;
        module = &vcd_modules[i];

        /* Declare each signal as defined in array */
        for (j = 0; j < module->number_of_signals; j++) {
          const char *signal_name;
          signal_name = module->signals_names[j];

          if (VCD_WIRE == module->signal_type) {
            if (module->signal_size > 1) {
              fprintf(vcd_fd, "b0 %s_w $end\n", signal_name);
            } else {
              fprintf(vcd_fd, "0%s_w $end\n", signal_name);
            }
          } else  if (VCD_REAL == module->signal_type) {
            fprintf(vcd_fd, "r0 %s_r $end\n", signal_name);
          } else {
            // Handle error here
          }
        }
      }

      fprintf(vcd_fd, "$end\n");
      fprintf(vcd_fd, "$enddefinitions $end\n\n");
      //fflush(vcd_fd);
    }
  }
}

void vcd_signal_dumper_dump_variable_by_name(vcd_signal_dump_variables variable_name,
    unsigned long             value)
{
  DevCheck((0 <= variable_name) && (variable_name < VCD_SIGNAL_DUMPER_VARIABLES_END),
           variable_name, VCD_SIGNAL_DUMPER_VARIABLES_END, 0);

  if (ouput_vcd) {
#if defined(ENABLE_VCD_FIFO)
    uint32_t write_index = vcd_get_write_index();

    vcd_fifo.user_data[write_index].time = vcd_get_time();
    vcd_fifo.user_data[write_index].data.variable.variable_name = variable_name;
    vcd_fifo.user_data[write_index].data.variable.value = value;
    vcd_fifo.user_data[write_index].module = VCD_SIGNAL_DUMPER_MODULE_VARIABLES; // Set when all other fields are set to validate the user_data
#else
    char binary_string[(sizeof (uint64_t) * BYTE_SIZE) + 1];

    if (vcd_fd != NULL) {
      vcd_signal_dumper_print_time_since_start();

      /* Set variable to value */
      uint64_to_binary(value, binary_string);
      fprintf(vcd_fd, "b%s %s_w\n", binary_string, eurecomVariablesNames[variable_name]);
      //fflush(vcd_fd);
    }

#endif
  }
}

void vcd_signal_dumper_dump_function_by_name(vcd_signal_dump_functions  function_name,
    vcd_signal_dump_in_out     in_out)
{
  DevCheck((0 <= function_name) && (function_name < VCD_SIGNAL_DUMPER_FUNCTIONS_END),
           function_name, VCD_SIGNAL_DUMPER_FUNCTIONS_END, 0);

  if (ouput_vcd) {
#if defined(ENABLE_VCD_FIFO)
    uint32_t write_index = vcd_get_write_index();

    vcd_fifo.user_data[write_index].time = vcd_get_time();
    vcd_fifo.user_data[write_index].data.function.function_name = function_name;
    vcd_fifo.user_data[write_index].data.function.in_out = in_out;
    vcd_fifo.user_data[write_index].module = VCD_SIGNAL_DUMPER_MODULE_FUNCTIONS; // Set when all other fields are set to validate the user_data
#else

    if (vcd_fd != NULL) {
      vcd_signal_dumper_print_time_since_start();

      /* Check if we are entering or leaving the function ( 0 = leaving, 1 = entering) */
      if (in_out == VCD_FUNCTION_IN)
        /* Set event to 1 */
        fprintf(vcd_fd, "1%s_w\n", eurecomFunctionsNames[function_name]);
      else
        fprintf(vcd_fd, "0%s_w\n", eurecomFunctionsNames[function_name]);

      //fflush(vcd_fd);
    }

#endif
  }
}

