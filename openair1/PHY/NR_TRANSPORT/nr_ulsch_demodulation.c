/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "nfapi_nr_interface_scf.h"
#include "nr_transport_proto.h"
#include "PHY/NR_TRANSPORT/nr_sch_dmrs.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_REFSIG/ptrs_nr.h"
#include "PHY/NR_ESTIMATION/nr_ul_estimation.h"
#include "PHY/defs_nr_common.h"
#include "PHY/nr_phy_common/inc/nr_phy_common.h"
#include "nr_layer_demapping.h"
#include "common/utils/nr/nr_common.h"
#include "platform_types.h"
#include "utils.h"
#include <openair1/PHY/TOOLS/phy_scope_interface.h>
#include "PHY/sse_intrin.h"
#include "T.h"
#include "T_messages_creator.h"
#include <sys/time.h>
#include "openair1/SCHED_NR/sched_nr.h"

#define NR_MAX_PUSCH_SCRAMBLING_STACK_BYTES (2 * 1024 * 1024) // 2MB

#if T_TRACER
static void copy_c16_data_to_slot_memory(c16_t *src, c16_t *dst_slot, int nb_re_pusch, int symbol)
{
  memcpy(&dst_slot[nb_re_pusch * symbol], src, nb_re_pusch * sizeof(c16_t));
}
#endif

#if defined(__aarch64__)

void nr_idft(int32_t *z, uint32_t Msc_PUSCH)
{
  simde__m128i idft_in128[1][3240];
  simde__m128i idft_out128[1][3240];
  simde__m128i norm128;

  int16_t *idft_in0 = (int16_t *)idft_in128[0];
  int16_t *idft_out0 = (int16_t *)idft_out128[0];

  int i;
  int ip;

  LOG_T(PHY, "Doing nr_idft for Msc_PUSCH %d\n", Msc_PUSCH);

  if ((Msc_PUSCH % 1536) > 0) {
    /* Conjugate input. */
    for (i = 0; i < (Msc_PUSCH >> 2); i++) {
      ((simde__m128i *)z)[i] = oai_mm_conj(((simde__m128i *)z)[i]);
    }

    /* Pack input into lane zero of the legacy four-lane layout. */
    for (i = 0, ip = 0; i < Msc_PUSCH; i++, ip += 4) {
      ((uint32_t *)idft_in0)[ip] = z[i];
    }
  }

  const dft_size_idx_t dftsize = get_dft(Msc_PUSCH);

  switch (Msc_PUSCH) {
    case 12:
      dft(dftsize, idft_in0, idft_out0, 0);

      norm128 = simde_mm_set1_epi16(9459);

      for (i = 0; i < 12; i++) {
        ((simde__m128i *)idft_out0)[i] = simde_mm_slli_epi16(simde_mm_mulhi_epi16(((simde__m128i *)idft_out0)[i], norm128), 1);
      }
      break;

    default:
      dft(dftsize, idft_in0, idft_out0, 1);
      break;
  }

  if ((Msc_PUSCH % 1536) > 0) {
    /* Extract lane zero. */
    for (i = 0, ip = 0; i < Msc_PUSCH; i++, ip += 4) {
      z[i] = ((uint32_t *)idft_out0)[ip];
    }

    /* Conjugate output. */
    for (i = 0; i < (Msc_PUSCH >> 2); i++) {
      ((simde__m128i *)z)[i] = oai_mm_conj(((simde__m128i *)z)[i]);
    }
  }
}

#else

void nr_idft(int32_t *z, uint32_t Msc_PUSCH)
{
  const dft_size_idx_t dftsize = get_dft(Msc_PUSCH);

  c16_t idft_input[Msc_PUSCH] __attribute__((aligned(64)));

  c16_t idft_output[Msc_PUSCH] __attribute__((aligned(64)));

  const size_t bytes = (size_t)Msc_PUSCH * sizeof(c16_t);

  memcpy(idft_input, z, bytes);

  idft(dftsize, (int16_t *)idft_input, (int16_t *)idft_output, 1);

  memcpy(z, idft_output, bytes);
}

#endif

static void nr_ulsch_extract_rbs(c16_t *const rxF,
                                 c16_t *const chF,
                                 c16_t *rxFext,
                                 c16_t *chFext,
                                 int choffset,
                                 int is_dmrs_symbol,
                                 const nfapi_nr_pusch_pdu_t *pusch_pdu,
                                 NR_DL_FRAME_PARMS *frame_parms,
                                 uint16_t rnti,
                                 bool is_ptrs)
{
  uint8_t delta = 0;
  if (is_dmrs_symbol) {
    uint8_t max_cdm = (pusch_pdu->dmrs_config_type == pusch_dmrs_type1 ? 2 : 3);
    AssertFatal(pusch_pdu->num_dmrs_cdm_grps_no_data <= max_cdm,
                "cdm group no data %d cannot be greater than %d\n",
                pusch_pdu->num_dmrs_cdm_grps_no_data,
                max_cdm);
    int first_port = get_dmrs_port(0, pusch_pdu->dmrs_ports);
    delta = get_delta(first_port, pusch_pdu->dmrs_config_type);
  }
  int start_re = (pusch_pdu->rb_start + pusch_pdu->bwp_start) * NR_NB_SC_PER_RB;
  int nb_re_pusch = NR_NB_SC_PER_RB * pusch_pdu->rb_size;
  c16_t *rxF_ext = &rxFext[0];
  c16_t *ul_ch0 = &chF[choffset];
  c16_t *ul_ch0_ext = &chFext[0];

  if (is_ptrs) {
    const uint k_ptrs = pusch_pdu->pusch_ptrs.ptrs_freq_density;
    const uint k_rb_ref = get_ptrs_k_RB(pusch_pdu->rb_size, k_ptrs, rnti);
    const uint k_re_ref = pusch_pdu->pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset;
    uint k = start_re;
    uint ch_idx = 0;
    for (uint rb = 0; rb < pusch_pdu->rb_size; rb++) {
      // RB doesn't have PTRS.
      if ((rb - k_rb_ref) % k_ptrs) {
        memcpy(rxF_ext, rxF + k, sizeof(c16_t) * NR_NB_SC_PER_RB);
        rxF_ext += NR_NB_SC_PER_RB;
        memcpy(ul_ch0_ext, ul_ch0 + ch_idx, sizeof(c16_t) * NR_NB_SC_PER_RB);
        ul_ch0_ext += NR_NB_SC_PER_RB;
        // RB has PTRS.
      } else {
        // before PTRS RE
        const uint num_pre_ptrs = k_re_ref;
        const size_t pre_sz = sizeof(c16_t) * num_pre_ptrs;
        memcpy(rxF_ext, rxF + k, pre_sz);
        memcpy(ul_ch0_ext, ul_ch0 + ch_idx, pre_sz);
        // after PTRS RE
        const uint num_post_ptrs = NR_NB_SC_PER_RB - k_re_ref - 1;
        const size_t post_sz = sizeof(c16_t) * num_post_ptrs;
        memcpy(rxF_ext + num_pre_ptrs, rxF + k + k_re_ref + 1, post_sz);
        memcpy(ul_ch0_ext + num_pre_ptrs, ul_ch0 + ch_idx + k_re_ref + 1, post_sz);
        rxF_ext += NR_NB_SC_PER_RB - 1;
        ul_ch0_ext += NR_NB_SC_PER_RB - 1;
      }
      ch_idx += NR_NB_SC_PER_RB;
      k += NR_NB_SC_PER_RB;
    }
  } else if (is_dmrs_symbol == 0) {
    memcpy(rxF_ext, &rxF[start_re], nb_re_pusch * sizeof(c16_t));
    memcpy(ul_ch0_ext, ul_ch0, nb_re_pusch * sizeof(c16_t));
  } else if (pusch_pdu->dmrs_config_type == pusch_dmrs_type1) { // 6 REs / PRB
    AssertFatal(delta == 0 || delta == 1, "Illegal delta %d\n",delta);
    c16_t *rxF32 = &rxF[start_re];
    for (int idx = 1 - delta; idx < nb_re_pusch; idx += 2) {
      *rxF_ext++ = rxF32[idx];
      *ul_ch0_ext++ = ul_ch0[idx];
    }
  } else if (pusch_pdu->dmrs_config_type == pusch_dmrs_type2) { // 8 REs / PRB
    AssertFatal(delta==0||delta==2||delta==4,"Illegal delta %d\n",delta);
    c16_t *rxF32 = &rxF[start_re];
    for (int idx = 0; idx < nb_re_pusch; idx++) {
      if (idx % 6 == 2 * delta || idx % 6 == 2 * delta + 1)
        continue;
      *rxF_ext++ = rxF32[idx];
      *ul_ch0_ext++ = ul_ch0[idx];
    }
  }
}

static int get_nb_re_pusch (NR_DL_FRAME_PARMS *frame_parms, const nfapi_nr_pusch_pdu_t *rel15_ul, int symbol)
{
  uint8_t dmrs_symbol_flag = (rel15_ul->ul_dmrs_symb_pos >> symbol) & 0x01;
  if (dmrs_symbol_flag == 1) {
    if (rel15_ul->dmrs_config_type == 0) {
      // if no data in dmrs cdm group is 1 only even REs have no data
      // if no data in dmrs cdm group is 2 both odd and even REs have no data
      return(rel15_ul->rb_size *(12 - (rel15_ul->num_dmrs_cdm_grps_no_data*6)));
    }
    else return(rel15_ul->rb_size *(12 - (rel15_ul->num_dmrs_cdm_grps_no_data*4)));
  } else
    return (rel15_ul->rb_size * NR_NB_SC_PER_RB);
}

static void inner_rx(PHY_VARS_gNB *gNB,
                     int slot,
                     NR_DL_FRAME_PARMS *frame_parms,
                     NR_gNB_PUSCH *pusch_vars,
                     const nfapi_nr_pusch_pdu_t *rel15_ul,
                     c16_t **rxF,
                     int16_t **llr,
                     int soffset,
                     int symbol,
                     int output_shift,
                     uint32_t nvar,
                     uint16_t ptrs_symb_pos,
                     c16_t cpe,
                     c16_t *rxFext_slot,
                     c16_t *chFext_slot)
{
  int nb_layer = rel15_ul->nrOfLayers;
  int nb_rx_ant = rel15_ul->param_v4.numSpatialStreamIndices;
  int dmrs_symbol_flag = (rel15_ul->ul_dmrs_symb_pos >> symbol) & 0x01;
  int buffer_length = ceil_mod(rel15_ul->rb_size * NR_NB_SC_PER_RB, 16);
  c16_t rxFext[nb_rx_ant][buffer_length] __attribute__((aligned(64)));
  c16_t chFext[nb_layer][nb_rx_ant][buffer_length] __attribute__((aligned(64)));

  memset(rxFext, 0, sizeof(rxFext));
  memset(chFext, 0, sizeof(chFext));
  int dmrs_symbol;
  if (gNB->chest_time == 0)
    dmrs_symbol = dmrs_symbol_flag ? symbol : get_valid_dmrs_idx_for_channel_est(rel15_ul->ul_dmrs_symb_pos, symbol);
  else { // average of channel estimates stored in first symbol
    int end_symbol = rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols;
    dmrs_symbol = get_next_dmrs_symbol_in_slot(rel15_ul->ul_dmrs_symb_pos, rel15_ul->start_symbol_index, end_symbol);
  }

  for (int aarx = 0; aarx < nb_rx_ant; aarx++) {
    for (int aatx = 0; aatx < nb_layer; aatx++) {
      nr_ulsch_extract_rbs(rxF[aarx] + soffset + symbol * frame_parms->ofdm_symbol_size,
                           (c16_t *)pusch_vars->ul_ch_estimates[aatx * nb_rx_ant + aarx],
                           rxFext[aarx],
                           chFext[aatx][aarx],
                           dmrs_symbol * frame_parms->ofdm_symbol_size,
                           dmrs_symbol_flag,
                           rel15_ul,
                           frame_parms,
                           rel15_ul->rnti,
                           IS_BIT_SET(ptrs_symb_pos, symbol));
#if T_TRACER
      // Data Recording application supports only 1 layer and 1 Tx antenna, so only record the first layer and first Tx antenna
      if (aatx == 0 && aarx == 0) {
        int nb_re_pusch = NR_NB_SC_PER_RB * rel15_ul->rb_size;
        // Assume assume Tx and Rx = 1
        if (T_ACTIVE(T_GNB_PHY_UL_FD_PUSCH_IQ)) {
          copy_c16_data_to_slot_memory(rxFext[aarx], rxFext_slot, nb_re_pusch, symbol);
        }
        if (T_ACTIVE(T_GNB_PHY_UL_FD_CHAN_EST_DMRS_INTERPL)) {
          copy_c16_data_to_slot_memory(chFext[aatx][aarx], chFext_slot, nb_re_pusch, symbol);
        }
      }
#endif
    }
  }
  c16_t rho[nb_layer][nb_layer][buffer_length] __attribute__((aligned(64)));
  c16_t rxF_ch_maga[nb_layer][buffer_length] __attribute__((aligned(64)));
  c16_t rxF_ch_magb[nb_layer][buffer_length] __attribute__((aligned(64)));
  c16_t rxF_ch_magc[nb_layer][buffer_length] __attribute__((aligned(64)));

  memset(rho, 0, sizeof(rho));
  for (int i = 0; i < nb_layer; i++)
    memset(&pusch_vars->rxdataF_comp[i][symbol * buffer_length], 0, sizeof(int32_t) * buffer_length);

  nr_channel_compensation(buffer_length,
                          buffer_length,
                          nb_rx_ant,
                          nb_layer,
                          rxFext,
                          chFext,
                          rxF_ch_maga,
                          rxF_ch_magb,
                          rxF_ch_magc,
                          pusch_vars->rxdataF_comp,
                          (nb_layer > 1) ? rho : NULL,
                          cpe,
                          rel15_ul->qam_mod_order,
                          symbol,
                          output_shift);

  if (nb_layer == 1 && rel15_ul->transform_precoding == transformPrecoder_enabled && rel15_ul->qam_mod_order <= 6) {
    if (rel15_ul->qam_mod_order > 2)
      nr_freq_equalization(frame_parms,
                           &pusch_vars->rxdataF_comp[0][symbol * buffer_length],
                           rxF_ch_maga[0],
                           rxF_ch_magb[0],
                           symbol,
                           pusch_vars->ul_valid_re_per_slot[symbol],
                           rel15_ul->qam_mod_order);
    nr_idft((int32_t *)&pusch_vars->rxdataF_comp[0][symbol * buffer_length], pusch_vars->ul_valid_re_per_slot[symbol]);
  }
  if (nb_layer == 2) {
    if (rel15_ul->qam_mod_order <= 6) {
      nr_compute_ML_llr((c16_t *)&pusch_vars->rxdataF_comp[0][symbol * buffer_length],
                        (c16_t *)&pusch_vars->rxdataF_comp[1][symbol * buffer_length],
                        rxF_ch_maga[0],
                        rxF_ch_maga[1],
                        llr[0],
                        llr[1],
                        rho[0][1],
                        rho[1][0],
                        pusch_vars->ul_valid_re_per_slot[symbol],
                        rel15_ul->qam_mod_order);
    }
    else {
      nr_mmse_2layers(pusch_vars->rxdataF_comp,
                      buffer_length,
                      buffer_length,
                      nb_rx_ant,
                      nb_layer,
                      rxF_ch_maga,
                      rxF_ch_magb,
                      rxF_ch_magc,
                      chFext,
                      rel15_ul->rb_size,
                      rel15_ul->qam_mod_order,
                      pusch_vars->log2_maxh,
                      symbol,
                      pusch_vars->ul_valid_re_per_slot[symbol],
                      nvar);
    }
  }
  if (nb_layer != 2 || rel15_ul->qam_mod_order > 6)
    for (int aatx = 0; aatx < nb_layer; aatx++)
           nr_compute_llr(&pusch_vars->rxdataF_comp[aatx][symbol * buffer_length],
                     rxF_ch_maga[aatx],
                     rxF_ch_magb[aatx],
                     rxF_ch_magc[aatx],
                     llr[aatx],
                     pusch_vars->ul_valid_re_per_slot[symbol],
                     symbol,
                     rel15_ul->qam_mod_order);
}

typedef struct puschSymbolProc_s {
  PHY_VARS_gNB *gNB;
  NR_DL_FRAME_PARMS *frame_parms;
  const nfapi_nr_pusch_pdu_t *rel15_ul;
  NR_gNB_PUSCH *pusch_vars;
  int slot;
  int startSymbol;
  int numSymbols;
  int16_t *llr;
  uint32_t nvar;
  uint16_t ptrs_symb_pos;
  c16_t *ptrs_cpe;
  int beam_nb;
  // TODO: Remove assumption of contiguous ports after DAS is properly handled in beamforming
  uint16_t ant_port_start;
  task_ans_t *ans;
  c16_t *pusch_ch_est_dmrs_interpl_slot_mem;
  c16_t *rxFext_slot_mem;
  uint8_t group_size;
  const nfapi_nr_pusch_pdu_t **rel15_ul_group;
  NR_gNB_PUSCH **pusch_vars_group;
  int16_t **scrambling_sequences;
  int *layer_offsets;
  int layers_attenuation;
} puschSymbolProc_t;

static void nr_pusch_symbol_processing(void *arg)
{
  puschSymbolProc_t *rdata=(puschSymbolProc_t*)arg;

  PHY_VARS_gNB *gNB = rdata->gNB;
  NR_DL_FRAME_PARMS *frame_parms = rdata->frame_parms;
  const nfapi_nr_pusch_pdu_t *rel15_ul = rdata->rel15_ul;
  int slot = rdata->slot;
  NR_gNB_PUSCH *pusch_vars = rdata->pusch_vars;
  for (int symbol = rdata->startSymbol; symbol < rdata->startSymbol + rdata->numSymbols; symbol++) {
    if (pusch_vars->ul_valid_re_per_slot[symbol] == 0)
      continue;
    int soffset = (slot % RU_RX_SLOT_DEPTH) * frame_parms->symbols_per_slot * frame_parms->ofdm_symbol_size;
    int buffer_length = ceil_mod(pusch_vars->ul_valid_re_per_slot[symbol] * NR_NB_SC_PER_RB, 16);
    int16_t llrs[rel15_ul->nrOfLayers][ceil_mod(buffer_length * rel15_ul->qam_mod_order, 64)] __attribute__((aligned(32)));
    int16_t *llrss[rel15_ul->nrOfLayers];
    for (int l = 0; l < rel15_ul->nrOfLayers; l++)
      llrss[l] = llrs[l];

    inner_rx(gNB,
             slot,
             frame_parms,
             pusch_vars,
             rel15_ul,
             gNB->common_vars.rxdataF + rdata->ant_port_start,
             llrss,
             soffset,
             symbol,
             pusch_vars->log2_maxh + rdata->layers_attenuation,
             rdata->nvar,
             rdata->ptrs_symb_pos,
             rdata->ptrs_cpe[symbol],
             rdata->rxFext_slot_mem,
             rdata->pusch_ch_est_dmrs_interpl_slot_mem);

    int nb_re_pusch = pusch_vars->ul_valid_re_per_slot[symbol];
    for (int u = 0; u < rdata->group_size; u++) {
      NR_gNB_PUSCH *ue_pusch_vars = rdata->pusch_vars_group[u];
      const nfapi_nr_pusch_pdu_t *ue_pdu = rdata->rel15_ul_group[u];
      int16_t *ue_scrambling_seq = rdata->scrambling_sequences[u];

      const int ue_layers = ue_pdu->nrOfLayers;
      const int qam = ue_pdu->qam_mod_order;
      const int layer_off = rdata->layer_offsets[u];

      ue_pusch_vars->llr_offset[symbol] = pusch_vars->llr_offset[symbol];
      ue_pusch_vars->ul_valid_re_per_slot[symbol] = nb_re_pusch;

      const int sym_bit_offset = ue_pusch_vars->llr_offset[symbol] * ue_layers;
      int16_t *llr_dest = &ue_pusch_vars->llr[sym_bit_offset];
      int16_t *s_seq = &ue_scrambling_seq[sym_bit_offset];

      const int n = nb_re_pusch * ue_layers * qam;
      const int16_t *src;

      // demapping: bring elements into order such that unscrambling is a linear operation
      // e.g., from "RE0-l0, RE1-l0, ..., REn-l0, RE0-l1, ..." to "RE0-l0, Re0-l1, RE1-l0, ..."
      // Each REn-ln = q LLRs (q = QAM order {2,4,6,8}, one LLR/bit).
      if (ue_layers == 1) {
        // no demapping needed
        src = llrss[layer_off];
      } else {
        nr_layer_demapping(ue_layers, qam, nb_re_pusch, &llrss[layer_off], llr_dest);
        src = llr_dest;
      }

      // unscrambling
      int k = 0;
      for (; k + 16 <= n; k += 16) {
        simde__m256i a = simde_mm256_loadu_si256((const simde__m256i *)(src + k));
        simde__m256i b = simde_mm256_loadu_si256((const simde__m256i *)(s_seq + k));
        simde_mm256_storeu_si256((simde__m256i *)(llr_dest + k), simde_mm256_mullo_epi16(a, b));
      }
      for (; k < n; k++)
        llr_dest[k] = src[k] * s_seq[k];
    }
  }

  // Task running in // completed
  completed_task_ans(rdata->ans);
}

static uint32_t average_u32(const uint32_t *x, uint16_t size)
{
  AssertFatal(size > 0 && x != NULL, "x is NULL or size is 0\n");

  uint64_t sum_x = 0;
  simde__m256i vec_sum = simde_mm256_setzero_si256();

  int i = 0;
  for (; i + 8 <= size; i += 8) {
    simde__m256i vec_data = simde_mm256_loadu_si256((simde__m256i *)&x[i]);
    vec_sum = simde_mm256_add_epi32(vec_sum, vec_data);
  }
  uint32_t *vec_sum32 = (uint32_t *)&vec_sum;
  for (int k = 0; k < 8; k++) {
    sum_x += vec_sum32[k];
  }
  for (; i < size; i++) {
    sum_x += x[i];
  }

  return (uint32_t)(sum_x / size);
}

int nr_rx_pusch_group_tp(PHY_VARS_gNB *gNB,
                         NR_gNB_PUSCH **pusch_vars_group,
                         const nfapi_nr_pusch_pdu_t **rel15_ul_group,
                         uint32_t **ret_unav_res_group,
                         uint8_t group_size,
                         uint32_t frame,
                         uint8_t slot)
{
  // This is a reference pdu since all the UEs in the group have same resource related parameters.
  const nfapi_nr_pusch_pdu_t *rel15_ul_ref = rel15_ul_group[0];
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  const nfapi_nr_spatial_stream_index_t *p = &rel15_ul_ref->param_v4;
  uint16_t ant_port_start = get_first_ant_idx(gNB->enable_analog_das,
                                              frame_parms->nb_antennas_tx / gNB->common_vars.num_beams_period,
                                              rel15_ul_ref->beamforming.prgs_list[0].dig_bf_interface_list[0].beam_idx,
                                              p->numSpatialStreamIndices > 0 ? p->spatialStreamIndices[0] : 0);

  uint32_t bwp_start_subcarrier = (rel15_ul_ref->rb_start + rel15_ul_ref->bwp_start) * NR_NB_SC_PER_RB;
  LOG_D(PHY,
        "pusch %d.%d : bwp_start_subcarrier %d, rb_start %d\n",
        frame,
        slot,
        bwp_start_subcarrier,
        rel15_ul_ref->rb_start);
  LOG_D(PHY, "pusch %d.%d : ul_dmrs_symb_pos %x\n", frame, slot, rel15_ul_ref->ul_dmrs_symb_pos);

  // Softscope dumps the whole slot grid; clear unused symbols so they do not keep
  // stale constellation points. scopeData is set only when nrscope is loaded (--doscope).
  if (gNB->scopeData) {
    const int rxdataF_comp_symbol_size = ceil_mod(frame_parms->N_RB_UL * NR_NB_SC_PER_RB, 16);
    const int rxdataF_comp_slot_size = rxdataF_comp_symbol_size * frame_parms->symbols_per_slot;
    for (int ue = 0; ue < group_size; ue++) {
      NR_gNB_PUSCH *pusch_vars = pusch_vars_group[ue];
      const int n_buf = rel15_ul_group[ue]->nrOfLayers;
      for (int i = 0; i < n_buf; i++)
        memset(pusch_vars->rxdataF_comp[i], 0, sizeof(*pusch_vars->rxdataF_comp[i]) * rxdataF_comp_slot_size);
      memset(pusch_vars->ul_valid_re_per_slot, 0, sizeof(*pusch_vars->ul_valid_re_per_slot) * frame_parms->symbols_per_slot);
    }
  }

  // Memories to store data for data recording
  int buffer_length_slot = rel15_ul_ref->rb_size * NR_NB_SC_PER_RB * NR_SYMBOLS_PER_SLOT;
  // data recording application supports only a single layer.
  // nb_rx_ant (= frame_parms->nb_antennas_rx) is limited to 1 for data recording application.
  // int nb_layer (= rel15_ul->nrOfLayers) is limited to 1 for data recording application.

  // Initialize memory for DMRS signals
  c16_t pusch_dmrs_slot_mem[1 * buffer_length_slot] __attribute__((aligned(64)));
  // Initialize memory for channel estimates based on DMRS positions
  c16_t pusch_ch_est_dmrs_pos_slot_mem[buffer_length_slot * 1 * 1] __attribute__((aligned(64)));
  // memory to store slot grid with channel coefficients based on DMRS positions after interpolation
  c16_t pusch_ch_est_dmrs_interpl_slot_mem[buffer_length_slot * 1 * 1] __attribute__((aligned(64)));
  // memory to store extracted data including PUSCH + DMRS
  c16_t rxFext_slot_mem[1 * buffer_length_slot] __attribute__((aligned(64)));

#if T_TRACER
  // Initialize memory for DMRS signals
  if (T_ACTIVE(T_GNB_PHY_UL_FD_DMRS))
    memset(pusch_dmrs_slot_mem, 0, sizeof(c16_t) * 1 * buffer_length_slot);

  // Initialize memory for channel estimates based on DMRS positions
  if (T_ACTIVE(T_GNB_PHY_UL_FD_CHAN_EST_DMRS_POS))
    memset(pusch_ch_est_dmrs_pos_slot_mem, 0, sizeof(c16_t) * buffer_length_slot * 1 * 1);

  // memory to store slot grid with channel coefficients based on DMRS positions after interpolation
  if (T_ACTIVE(T_GNB_PHY_UL_FD_CHAN_EST_DMRS_INTERPL))
    memset(pusch_ch_est_dmrs_interpl_slot_mem, 0, sizeof(c16_t) * buffer_length_slot * 1 * 1);

  // memory to store extracted data including PUSCH + DMRS
  if (T_ACTIVE(T_GNB_PHY_UL_FD_PUSCH_IQ))
    memset(rxFext_slot_mem, 0, sizeof(c16_t) * buffer_length_slot * 1 * 1);
#endif

  // Create a virtual multi layer pdu by accumulating the layers over UEs in the group and storing dmrs ports for joint processing
  uint32_t combined_dmrs_ports = 0;
  int total_layers = 0;
  int layer_offset[group_size];
  for (int u = 0; u < group_size; u++) {
    const nfapi_nr_pusch_pdu_t *p = rel15_ul_group[u];
    combined_dmrs_ports |= p->dmrs_ports;
    layer_offset[u] = total_layers;
    total_layers += rel15_ul_group[u]->nrOfLayers;
  }
  AssertFatal(total_layers <= NR_MAX_NB_LAYERS,
              "MU-MIMO group total_layers=%d > NR_MAX_NB_LAYERS=%d\n",
              total_layers,
              NR_MAX_NB_LAYERS);

  nfapi_nr_pusch_pdu_t joint_pdu = *rel15_ul_ref;
  joint_pdu.nrOfLayers = total_layers;
  joint_pdu.dmrs_ports = combined_dmrs_ports;

  NR_gNB_PUSCH *joint_pv = pusch_vars_group[0];
  LOG_D(PHY,
        "%4u.%u MU-MIMO joint RX: %d UEs, %d total layers, rb_start=%u rb_size=%u qam=%u\n",
        frame,
        slot,
        group_size,
        total_layers,
        rel15_ul_ref->rb_start,
        rel15_ul_ref->rb_size,
        rel15_ul_ref->qam_mod_order);

  //----------------------------------------------------------
  //------------------- Channel estimation -------------------
  //----------------------------------------------------------
  start_meas(&gNB->ulsch_channel_estimation_stats);
  int max_ch = 0;
  uint32_t nvar = 0;
  int end_symbol = rel15_ul_ref->start_symbol_index + rel15_ul_ref->nr_of_symbols;
  uint8_t dmrs_symb_idx = 0;
  for (uint8_t symbol = rel15_ul_ref->start_symbol_index; symbol < end_symbol; symbol++) {
    uint8_t dmrs_symbol_flag = (rel15_ul_ref->ul_dmrs_symb_pos >> symbol) & 0x01;
    LOG_D(PHY, "symbol %d, dmrs_symbol_flag :%d\n", symbol, dmrs_symbol_flag);
    if (dmrs_symbol_flag == 1) {
      for (int u = 0; u < group_size; u++) {
        const nfapi_nr_pusch_pdu_t *p = rel15_ul_group[u];
        for (int nl = 0; nl < p->nrOfLayers; nl++) {
          int global_layer = layer_offset[u] + nl;
          uint32_t nvar_tmp = 0;
          nr_pusch_channel_estimation(gNB,
                                      slot,
                                      global_layer,
                                      get_dmrs_port(nl, p->dmrs_ports),
                                      dmrs_symb_idx,
                                      symbol,
                                      joint_pv,
                                      ant_port_start,
                                      bwp_start_subcarrier,
                                      &joint_pdu,
                                      &max_ch,
                                      &nvar_tmp,
                                      pusch_dmrs_slot_mem,
                                      pusch_ch_est_dmrs_pos_slot_mem);
          nvar += nvar_tmp;
        }
      }
      dmrs_symb_idx++;
    }
  }

  // PTRS processing.
  const bool is_ptrs = rel15_ul_ref->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS;
  c16_t cpe[NR_SYMBOLS_PER_SLOT];
  for (uint s = 0; s < NR_SYMBOLS_PER_SLOT; s++)
    cpe[s] = (c16_t){.r = INT16_MAX}; // zero phase error.
  uint ptrs_re_symbol = 0;
  uint16_t ptrs_symb_pos = 0;
  if (is_ptrs) {
    if (rel15_ul_ref->pusch_ptrs.num_ptrs_ports != 1)
      LOG_W(NR_PHY, "Multi-port PTRS not supported, skipping PTRS processing\n");
    else {
      const NR_DL_FRAME_PARMS *fp = frame_parms;
      ptrs_proc_t p = {.k_ptrs = rel15_ul_ref->pusch_ptrs.ptrs_freq_density,
                       .k_re_ref = rel15_ul_ref->pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset,
                       .symbols_per_slot = fp->symbols_per_slot,
                       .start_rb = rel15_ul_ref->rb_start,
                       .num_rb = rel15_ul_ref->rb_size,
                       .N_RB = fp->N_RB_UL,
                       .start_symb = rel15_ul_ref->start_symbol_index,
                       .num_symb = rel15_ul_ref->nr_of_symbols,
                       .dmrs_symb_pos = rel15_ul_ref->ul_dmrs_symb_pos,
                       .nid = fp->Nid_cell,
                       .nscid = rel15_ul_ref->scid,
                       .first_carrier_offset = 0,
                       .ofdm_symbol_size = fp->ofdm_symbol_size,
                       .slot = slot,
                       .rnti = rel15_ul_ref->rnti};
      const int slot_offset = (p.slot % RU_RX_SLOT_DEPTH) * frame_parms->symbols_per_slot * p.ofdm_symbol_size;
      c16_t *rxdataF = (c16_t *)&gNB->common_vars.rxdataF[ant_port_start][slot_offset];
      ptrs_re_symbol =
          nr_ptrs_run(&p, rel15_ul_ref->pusch_ptrs.ptrs_time_density, rxdataF, (const c16_t *)joint_pv->ul_ch_estimates[0], cpe);
      ptrs_symb_pos = p.ptrs_symb_pos;
    }
  }

  if (dmrs_symb_idx > 0)
    nvar /= (dmrs_symb_idx * total_layers);

  // averaging time domain channel estimates
  // Change to joint processing
  const uint8_t num_sp_streams = rel15_ul_ref->param_v4.numSpatialStreamIndices;
  if (gNB->chest_time == 1) {
    AssertFatal(!is_ptrs, "Time domain averaging of DMRS estimates not allowed with PTRS\n");
    nr_chest_time_domain_avg(frame_parms,
                             joint_pv->ul_ch_estimates,
                             rel15_ul_ref->nr_of_symbols,
                             rel15_ul_ref->start_symbol_index,
                             rel15_ul_ref->ul_dmrs_symb_pos, // change needed ?
                             rel15_ul_ref->rb_size,
                             total_layers,
                             num_sp_streams);
  }

  // ULSCH signal and noise power measurements
  // This is same for all the UEs in the group
  allocCast2D(n0_subband_power,
              unsigned int,
              gNB->measurements.n0_subband_power,
              frame_parms->nb_antennas_rx,
              frame_parms->N_RB_UL,
              false);

  int start_sc = (rel15_ul_ref->bwp_start + rel15_ul_ref->rb_start) * NR_NB_SC_PER_RB;
  for (int aa_pusch = 0; aa_pusch < num_sp_streams; aa_pusch++) {
    const int aarx = ant_port_start + aa_pusch;
    DevAssert(aarx < sizeofArray(joint_pv->ulsch_power));
    joint_pv->ulsch_power[aa_pusch] = 0;
    joint_pv->ulsch_noise_power[aa_pusch] = 0;
    int64_t symb_energy = 0;

    for (uint8_t symbol = rel15_ul_ref->start_symbol_index; symbol < end_symbol; symbol++) {
      int offset0 = ((slot % RU_RX_SLOT_DEPTH) * frame_parms->symbols_per_slot + symbol) * frame_parms->ofdm_symbol_size;
      int offset = offset0 + start_sc;
      c16_t *ul_ch = &gNB->common_vars.rxdataF[aarx][offset];
      symb_energy += signal_energy_nodc(ul_ch, rel15_ul_ref->rb_size * NR_NB_SC_PER_RB);
    }
    joint_pv->ulsch_power[aa_pusch] += (symb_energy / rel15_ul_ref->nr_of_symbols);

    joint_pv->ulsch_noise_power[aa_pusch] +=
        average_u32(&n0_subband_power[aarx][rel15_ul_ref->bwp_start + rel15_ul_ref->rb_start], rel15_ul_ref->rb_size);

    LOG_D(NR_PHY,
          "aa %d, bwp_start%d, rb_start %d, rb_size %d: ulsch_power %d, ulsch_noise_power %d\n",
          aarx,
          rel15_ul_ref->bwp_start,
          rel15_ul_ref->rb_start,
          rel15_ul_ref->rb_size,
          joint_pv->ulsch_power[aa_pusch],
          joint_pv->ulsch_noise_power[aa_pusch]);
  }
  stop_meas(&gNB->ulsch_channel_estimation_stats);

  start_meas(&gNB->rx_pusch_init_stats);

  // Calculate number of unavailable resources due to PTRS
  // This is assumed to be same for all the UEs (same PTRS configuration for all UEs)
  uint32_t unav_res = 0;
  if (rel15_ul_ref->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) {
    int ptrsSymbPerSlot = get_ptrs_symbols_in_slot(ptrs_symb_pos, rel15_ul_ref->start_symbol_index, rel15_ul_ref->nr_of_symbols);
    unav_res = ptrs_re_symbol * ptrsSymbPerSlot;
  }

  // Scrambling initialization
  int number_dmrs_symbols =
      count_bits64_with_mask(rel15_ul_ref->ul_dmrs_symb_pos, rel15_ul_ref->start_symbol_index, rel15_ul_ref->nr_of_symbols);
  int factor = rel15_ul_ref->dmrs_config_type == pusch_dmrs_type1 ? 6 : 4;
  int nb_re_dmrs = factor * rel15_ul_ref->num_dmrs_cdm_grps_no_data;

  int max_G = 0;
  for (int u = 0; u < group_size; u++) {
    const nfapi_nr_pusch_pdu_t *p = rel15_ul_group[u];
    int G_u = nr_get_G(p->rb_size, p->nr_of_symbols, nb_re_dmrs, number_dmrs_symbols, unav_res, p->qam_mod_order, p->nrOfLayers);
    if (G_u > max_G)
      max_G = G_u;
  }

  const uint64_t num_scrambling_bytes = group_size * (max_G + 96) * sizeof(int16_t);
  AssertFatal(num_scrambling_bytes <= NR_MAX_PUSCH_SCRAMBLING_STACK_BYTES,
              "scrambling_sequences stack buffer %" PRIu64 " bytes exceeds %d MB limit : group_size %d, max_G %d\n",
              num_scrambling_bytes,
              NR_MAX_PUSCH_SCRAMBLING_STACK_BYTES >> 20,
              group_size,
              max_G);

  int16_t scrambling_sequences[group_size][max_G + 96] __attribute__((aligned(32)));
  int16_t *scrambling_sequences_arr[group_size];

  for (int u = 0; u < group_size; u++) {
    scrambling_sequences_arr[u] = scrambling_sequences[u];
    const nfapi_nr_pusch_pdu_t *p = rel15_ul_group[u];
    int G_u = nr_get_G(p->rb_size, p->nr_of_symbols, nb_re_dmrs, number_dmrs_symbols, unav_res, p->qam_mod_order, p->nrOfLayers);
    nr_codeword_unscrambling_init(scrambling_sequences_arr[u], G_u, 0, p->data_scrambling_id, p->rnti);
  }

  // Computation of channel levels
  int nb_re_pusch = 0, meas_symbol = -1;
  for (meas_symbol = rel15_ul_ref->start_symbol_index; meas_symbol < end_symbol; meas_symbol++)
    if ((nb_re_pusch = get_nb_re_pusch(frame_parms, &joint_pdu, meas_symbol)) > 0)
      break;

  AssertFatal(nb_re_pusch > 0 && meas_symbol >= 0,
              "nb_re_pusch %d cannot be 0 or meas_symbol %d cannot be negative here\n",
              nb_re_pusch,
              meas_symbol);

  // extract the first dmrs for the channel level computation
  // extract the data in the OFDM frame, to the start of the array
  int soffset = (slot % RU_RX_SLOT_DEPTH) * frame_parms->symbols_per_slot * frame_parms->ofdm_symbol_size;

  nb_re_pusch = ceil_mod(nb_re_pusch, 16);
  int dmrs_symbol;
  if (gNB->chest_time == 0)
    dmrs_symbol = get_valid_dmrs_idx_for_channel_est(rel15_ul_ref->ul_dmrs_symb_pos, meas_symbol);
  else // average of channel estimates stored in first symbol
    dmrs_symbol = get_next_dmrs_symbol_in_slot(rel15_ul_ref->ul_dmrs_symb_pos, rel15_ul_ref->start_symbol_index, end_symbol);
  int size_est = ceil_mod(nb_re_pusch * frame_parms->symbols_per_slot, 16);
  __attribute__((aligned(64))) c16_t ul_ch_estimates_ext[total_layers][num_sp_streams][size_est];
  memset(ul_ch_estimates_ext, 0, sizeof(ul_ch_estimates_ext));
  int buffer_length = rel15_ul_ref->rb_size * NR_NB_SC_PER_RB;
  c16_t temp_rxFext[num_sp_streams][buffer_length] __attribute__((aligned(32)));
  for (int aarx = 0; aarx < num_sp_streams; aarx++)
    for (int nl = 0; nl < total_layers; nl++) {
      start_meas(&gNB->pusch_extraction_stats);
      nr_ulsch_extract_rbs(gNB->common_vars.rxdataF[ant_port_start + aarx] + soffset + meas_symbol * frame_parms->ofdm_symbol_size,
                           (c16_t *)joint_pv->ul_ch_estimates[nl * num_sp_streams + aarx],
                           temp_rxFext[aarx],
                           &ul_ch_estimates_ext[nl][aarx][meas_symbol * nb_re_pusch],
                           dmrs_symbol * frame_parms->ofdm_symbol_size,
                           (rel15_ul_ref->ul_dmrs_symb_pos >> meas_symbol) & 0x01,
                           &joint_pdu,
                           frame_parms,
                           rel15_ul_ref->rnti,
                           IS_BIT_SET(ptrs_symb_pos, meas_symbol));
      stop_meas(&gNB->pusch_extraction_stats);
    }

  //----------------------------------------------------------
  //--------------------- Channel Scaling --------------------
  //----------------------------------------------------------

  int avg[total_layers][num_sp_streams];
  for (int i = 0; i < total_layers; i++)
    nr_channel_level(meas_symbol, size_est, ul_ch_estimates_ext[i], num_sp_streams, avg[i], nb_re_pusch);

  int avgs = 0;
  for (int nl = 0; nl < total_layers; nl++)
    for (int aarx = 0; aarx < num_sp_streams; aarx++)
      avgs = cmax(avgs, avg[nl][aarx]);

  if (total_layers == 2 && rel15_ul_ref->qam_mod_order > 6)
    joint_pv->log2_maxh = (log2_approx(avgs) >> 1) - 3; // for MMSE
  else if (total_layers == 2)
    joint_pv->log2_maxh = (log2_approx(avgs) >> 1) - 2 + log2_approx(num_sp_streams >> 1);
  else
    joint_pv->log2_maxh = (log2_approx(avgs) >> 1) + 1 + log2_approx(num_sp_streams >> 1);

  if (joint_pv->log2_maxh < 1)
    joint_pv->log2_maxh = 1;
  else if (joint_pv->log2_maxh > 14)
    joint_pv->log2_maxh = 14;

  stop_meas(&gNB->rx_pusch_init_stats);

  start_meas(&gNB->rx_pusch_symbol_processing_stats);
  int numSymbols = gNB->num_pusch_symbols_per_thread;
  int total_res = 0;
  int const loop_iter = CEILIDIV(rel15_ul_ref->nr_of_symbols, numSymbols);
  puschSymbolProc_t arr[loop_iter];
  task_ans_t ans;
  init_task_ans(&ans, loop_iter);

  int sz_arr = 0;
  for (uint8_t task_index = 0; task_index < loop_iter; task_index++) {
    int symbol = task_index * numSymbols + rel15_ul_ref->start_symbol_index;
    int res_per_task = 0;
    for (int s = 0; s < numSymbols && s + symbol < end_symbol; s++) {
      int curr_sym = symbol + s;
      joint_pv->ul_valid_re_per_slot[curr_sym] =
          get_nb_re_pusch(frame_parms, &joint_pdu, curr_sym) - (IS_BIT_SET(ptrs_symb_pos, (symbol + s)) ? ptrs_re_symbol : 0);
      if (curr_sym == rel15_ul_ref->start_symbol_index) {
        joint_pv->llr_offset[curr_sym] = 0;
      } else {
        int prev_sym = curr_sym - 1;
        int prev_offset = joint_pv->llr_offset[prev_sym];
        int prev_re = joint_pv->ul_valid_re_per_slot[prev_sym];
        int mod_order = rel15_ul_ref->qam_mod_order;
        joint_pv->llr_offset[curr_sym] = prev_offset + (prev_re * mod_order);
      }
      res_per_task += joint_pv->ul_valid_re_per_slot[curr_sym];
    }
    total_res += res_per_task;
    if (res_per_task > 0) {
      puschSymbolProc_t *rdata = &arr[sz_arr];
      rdata->ans = &ans;
      ++sz_arr;

      rdata->gNB = gNB;
      rdata->frame_parms = frame_parms;
      rdata->rel15_ul = &joint_pdu;
      rdata->slot = slot;
      rdata->startSymbol = symbol;
      // Last task processes remainder symbols
      rdata->numSymbols = task_index == loop_iter - 1 ? rel15_ul_ref->nr_of_symbols - (loop_iter - 1) * numSymbols : numSymbols;
      rdata->pusch_vars = joint_pv;
      rdata->llr = joint_pv->llr;
      rdata->ptrs_symb_pos = ptrs_symb_pos;
      rdata->ptrs_cpe = cpe;
      rdata->nvar = nvar;
      rdata->ant_port_start = ant_port_start;
      rdata->rxFext_slot_mem = rxFext_slot_mem;
      rdata->pusch_ch_est_dmrs_interpl_slot_mem = pusch_ch_est_dmrs_interpl_slot_mem;
      rdata->group_size = group_size;
      rdata->rel15_ul_group = rel15_ul_group;
      rdata->pusch_vars_group = pusch_vars_group;
      rdata->scrambling_sequences = scrambling_sequences_arr;
      rdata->layer_offsets = layer_offset;
      rdata->layers_attenuation = total_layers ? log2_approx(max_ch >> 11) : 0;

      if (rel15_ul_ref->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) {
        nr_pusch_symbol_processing(rdata);
      } else {
        task_t t = {.func = &nr_pusch_symbol_processing, .args = rdata};
        pushTpool(&gNB->threadPool, t);
      }

      LOG_D(PHY, "%d.%d Added symbol %d to process, in pipe\n", frame, slot, symbol);
    } else {
      completed_task_ans(&ans);
    }
  } // symbol loop

#if T_TRACER
  int dmrs_port = get_dmrs_port(0, rel15_ul_ref->dmrs_ports);

  log_ul_fd_dmrs(frame,
                 slot,
                 frame_parms,
                 rel15_ul_ref,
                 number_dmrs_symbols,
                 dmrs_port,
                 (const c16_t *)(&(pusch_dmrs_slot_mem[0])),
                 rel15_ul_ref->rb_size * NR_NB_SC_PER_RB * rel15_ul_ref->nr_of_symbols * 4);

  log_ul_fd_chan_est_dmrs_pos(frame,
                              slot,
                              frame_parms,
                              rel15_ul_ref,
                              number_dmrs_symbols,
                              dmrs_port,
                              (const c16_t *)(&(pusch_ch_est_dmrs_pos_slot_mem[0])),
                              rel15_ul_ref->rb_size * NR_NB_SC_PER_RB * rel15_ul_ref->nr_of_symbols * 4);

  log_ul_fd_pusch_iq(frame,
                     slot,
                     frame_parms,
                     rel15_ul_ref,
                     number_dmrs_symbols,
                     dmrs_port,
                     (const c16_t *)(&(rxFext_slot_mem[0])),
                     rel15_ul_ref->rb_size * NR_NB_SC_PER_RB * rel15_ul_ref->nr_of_symbols * num_sp_streams * 4);

  log_ul_fd_chan_est_dmrs_interpl(
      frame,
      slot,
      frame_parms,
      rel15_ul_ref,
      number_dmrs_symbols,
      dmrs_port,
      (const c16_t *)pusch_ch_est_dmrs_interpl_slot_mem,
      rel15_ul_ref->rb_size * NR_NB_SC_PER_RB * rel15_ul_ref->nr_of_symbols * num_sp_streams * total_layers * 4);
#endif

  join_task_ans(&ans);
  for (int u = 0; u < group_size; u++) {
    NR_gNB_PUSCH *pv = pusch_vars_group[u];
    // Copy unavailable resources per UE
    *ret_unav_res_group[u] = unav_res;
    // Copy power measurements per UE
    pv->ulsch_power_tot = 0;
    pv->ulsch_noise_power_tot = 0;
    for (int aarx = 0; aarx < num_sp_streams; aarx++) {
      pv->ulsch_power[aarx] = joint_pv->ulsch_power[aarx];
      pv->ulsch_noise_power[aarx] = joint_pv->ulsch_noise_power[aarx];
      pv->ulsch_power_tot += pv->ulsch_power[aarx];
      pv->ulsch_noise_power_tot += pv->ulsch_noise_power[aarx];
    }
  }
  stop_meas(&gNB->rx_pusch_symbol_processing_stats);

  // Copy the data to the scope. This cannot be performed in one call to gNBscopeCopy because the data is not contiguous in the
  // buffer due to reference symbol extraction and padding. The gNBscopeCopy call is broken up into steps: trylock, copy, unlock.
  metadata mt = {.slot = slot, .frame = frame};
  if (gNBTryLockScopeData(gNB, gNBPuschRxIq, sizeof(c16_t), 1, total_res, &mt)) {
    int buffer_length = ceil_mod(rel15_ul_ref->rb_size * NR_NB_SC_PER_RB, 16);
    size_t offset = 0;
    for (uint8_t symbol = rel15_ul_ref->start_symbol_index;
         symbol < (rel15_ul_ref->start_symbol_index + rel15_ul_ref->nr_of_symbols);
         symbol++) {
      gNBscopeCopyUnsafe(gNB,
                         gNBPuschRxIq,
                         &pusch_vars_group[0]->rxdataF_comp[0][symbol * buffer_length],
                         sizeof(c16_t) * pusch_vars_group[0]->ul_valid_re_per_slot[symbol],
                         offset,
                         symbol - rel15_ul_ref->start_symbol_index);
      offset += sizeof(c16_t) * pusch_vars_group[0]->ul_valid_re_per_slot[symbol];
    }
    gNBunlockScopeData(gNB, gNBPuschRxIq)
  }
  uint32_t total_llrs = total_res * rel15_ul_ref->qam_mod_order * rel15_ul_ref->nrOfLayers;
  gNBscopeCopyWithMetadata(gNB, gNBPuschLlr, pusch_vars_group[0]->llr, sizeof(c16_t), 1, total_llrs, 0, &mt);
  return 0;
}
