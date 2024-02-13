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

#include "openair2/LAYER2/NR_MAC_UE/mac_defs.h"
#include "NR_SidelinkPreconfigNR-r16.h"
#include "mac_proto.h"

void sl_ue_mac_free(uint8_t module_id)
{

  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

  sl_nr_phy_config_request_t *sl_config =
                    &mac->SL_MAC_PARAMS->sl_phy_config.sl_config_req;

  uint8_t syncsource = sl_config->sl_sync_source.sync_source;

  //Allocated by MAC only in case of SYNC_REF_UE
  //else it is freed as part of RRC pre-config structure
  if (syncsource == SL_SYNC_SOURCE_SYNC_REF_UE) {
    ASN_STRUCT_FREE (asn_DEF_NR_TDD_UL_DL_Pattern, mac->SL_MAC_PARAMS->sl_TDD_config);
  }

  fapi_nr_max_tdd_periodicity_t *tdd_list =
                            sl_config->tdd_table.max_tdd_periodicity_list;

  // @todo: maybe this should be done by phy
  if (tdd_list) {
    int mu = sl_config->sl_bwp_config.sl_scs;
    int nb_slots_to_set = TDD_CONFIG_NB_FRAMES*(1<<mu)*NR_NUMBER_OF_SUBFRAMES_PER_FRAME;
    for (int i=0; i<nb_slots_to_set; i++) {
      free_and_zero(tdd_list[i].max_num_of_symbol_per_slot_list);
    }
    free_and_zero(sl_config->tdd_table.max_tdd_periodicity_list);
  }

  for (int i=0;i<SL_NR_MAC_NUM_RX_RESOURCE_POOLS;i++) {
    free_and_zero(mac->SL_MAC_PARAMS->sl_RxPool[i]);
  }
  for (int i=0;i<SL_NR_MAC_NUM_TX_RESOURCE_POOLS;i++) {
    free_and_zero(mac->SL_MAC_PARAMS->sl_TxPool[i]);
  }

  free_and_zero(mac->SL_MAC_PARAMS);
}

void sl_set_tdd_config_nr_ue(fapi_nr_tdd_table_t *tdd_table,
                             int mu,
                             NR_TDD_UL_DL_Pattern_t *pattern)
{
  const int nrofUplinkSlots = pattern->nrofUplinkSlots;
  const int nrofUplinkSymbols = pattern->nrofUplinkSymbols;
  const int nb_periods_per_frame = get_nb_periods_per_frame(pattern->dl_UL_TransmissionPeriodicity);
  const int nb_slots_per_period = ((1 << mu) * NR_NUMBER_OF_SUBFRAMES_PER_FRAME) / nb_periods_per_frame;
  tdd_table->tdd_period_in_slots = nb_slots_per_period;

  LOG_I(PHY,"UL slots:%d, symbols:%d, slots_per_period:%d\n",
                          nrofUplinkSlots, nrofUplinkSymbols, nb_slots_per_period);

  tdd_table->max_tdd_periodicity_list = (fapi_nr_max_tdd_periodicity_t *) malloc(nb_slots_per_period * sizeof(fapi_nr_max_tdd_periodicity_t));

  for(int memory_alloc = 0 ; memory_alloc < nb_slots_per_period; memory_alloc++)
    tdd_table->max_tdd_periodicity_list[memory_alloc].max_num_of_symbol_per_slot_list =
      (fapi_nr_max_num_of_symbol_per_slot_t *) malloc(NR_NUMBER_OF_SYMBOLS_PER_SLOT*sizeof(fapi_nr_max_num_of_symbol_per_slot_t));

  int slot_number = (nb_slots_per_period - nrofUplinkSlots) - (nrofUplinkSymbols ? 1 : 0);
  if (nrofUplinkSymbols != 0) {
    for(int number_of_symbol = NR_NUMBER_OF_SYMBOLS_PER_SLOT - nrofUplinkSymbols; number_of_symbol < NR_NUMBER_OF_SYMBOLS_PER_SLOT; number_of_symbol++) {
      tdd_table->max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol].slot_config = 1;
    }
    slot_number++;
  }
  while(slot_number < nb_slots_per_period) {
    for (int number_of_symbol = 0; number_of_symbol < nrofUplinkSlots * NR_NUMBER_OF_SYMBOLS_PER_SLOT; number_of_symbol++) {
      tdd_table->max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol%NR_NUMBER_OF_SYMBOLS_PER_SLOT].slot_config = 1;
      if((number_of_symbol + 1) % NR_NUMBER_OF_SYMBOLS_PER_SLOT == 0)
        slot_number++;
    }
  }
}

//Prepares the PHY config to be sent to PHY. Prepares from the Valus from MAC context.
static void  sl_prepare_phy_config(int module_id,
                                   sl_nr_phy_config_request_t *phycfg,
                                   NR_SL_FreqConfigCommon_r16_t *freqcfg,
                                   uint8_t sync_source,
                                   uint32_t sl_OffsetDFN,
                                   NR_TDD_UL_DL_ConfigCommon_t *sl_TDD_config)
{


  phycfg->sl_sync_source.sync_source = sync_source;
  LOG_I(NR_MAC, "Sidelink CFG: sync source:%d\n", phycfg->sl_sync_source.sync_source);

  uint32_t pointA_ARFCN = freqcfg->sl_AbsoluteFrequencyPointA_r16;
  AssertFatal(pointA_ARFCN, "sl_AbsoluteFrequencyPointA_r16 cannot be 0\n");

  int sl_band = 0;
  //REL 16 3GPP spec 38.101 section 5.2E.1 specifies 2 bands for operation on PC5 interface.
  //Band 47, Band 38
  if (pointA_ARFCN >= 790334 && pointA_ARFCN <= 795000)
    sl_band = 47;
  else if (pointA_ARFCN >= 514000 && pointA_ARFCN <= 524000)
    sl_band = 38;

  AssertFatal(sl_band, "not valid band for Sidelink operation\n");

  uint32_t SSB_ARFCN = (freqcfg->sl_AbsoluteFrequencySSB_r16)
                            ? *freqcfg->sl_AbsoluteFrequencySSB_r16 : 0;

  AssertFatal(SSB_ARFCN, "sl_AbsoluteFrequencySSB cannot be 0\n");

  LOG_I(NR_MAC, "SIDELINK CONFIGs: AbsFreqSSB:%d, AbsFreqPointA:%d, SL band:%d\n",
                                                        SSB_ARFCN,pointA_ARFCN, sl_band);

  //FREQSHIFT_7P5KHZ is DISABLED
  phycfg->sl_carrier_config.sl_frequency_shift_7p5khz = 0;
  phycfg->sl_carrier_config.sl_value_N = freqcfg->valueN_r16;

  NR_SCS_SpecificCarrier_t *carriercfg =
            freqcfg->sl_SCS_SpecificCarrierList_r16.list.array[0];

  AssertFatal(carriercfg, "SCS_SpecificCarrier cannot be NULL");

  int bw_index = get_supported_band_index(carriercfg->subcarrierSpacing,
                                          sl_band,
                                          carriercfg->carrierBandwidth);
  phycfg->sl_carrier_config.sl_bandwidth = get_supported_bw_mhz(FR1, bw_index);

  phycfg->sl_carrier_config.sl_frequency =
              from_nrarfcn(sl_band,carriercfg->subcarrierSpacing,pointA_ARFCN); // freq in kHz

  phycfg->sl_carrier_config.sl_grid_size = carriercfg->carrierBandwidth;
  //For sidelink offset to carrier is 0. hence not used
  //phycfg->sl_carrier_config.sl_k0 = carriercfg->offsetToCarrier;

  NR_SL_BWP_Generic_r16_t *bwp_generic = NULL;
  if (freqcfg->sl_BWP_List_r16 &&
      freqcfg->sl_BWP_List_r16->list.array[0] &&
      freqcfg->sl_BWP_List_r16->list.array[0]->sl_BWP_Generic_r16)
    bwp_generic = freqcfg->sl_BWP_List_r16->list.array[0]->sl_BWP_Generic_r16;

  AssertFatal(bwp_generic, "SL-BWP Generic cannot be NULL");

  NR_BWP_t *sl_bwp = bwp_generic->sl_BWP_r16;
  AssertFatal(sl_bwp, "SL-BWP cannot be NULL");

  int  locbw = bwp_generic->sl_BWP_r16->locationAndBandwidth;
  phycfg->sl_bwp_config.sl_bwp_size = NRRIV2BW(locbw, MAX_BWP_SIZE);
  phycfg->sl_bwp_config.sl_bwp_start = NRRIV2PRBOFFSET(locbw, MAX_BWP_SIZE);
  phycfg->sl_bwp_config.sl_scs = sl_bwp->subcarrierSpacing;


  int scs_scaling = 1<<(phycfg->sl_bwp_config.sl_scs);

  if (pointA_ARFCN < 600000)
    scs_scaling = scs_scaling*3;
  if (pointA_ARFCN > 2016666)
    scs_scaling = scs_scaling>>2;
  //SSB arfcn points to middle RE of PSBCH 11 RBs
  uint32_t diff = (SSB_ARFCN - 66*scs_scaling) - pointA_ARFCN;
  //the RE offset from pointA where SSB starts
  phycfg->sl_bwp_config.sl_ssb_offset_point_a = diff/scs_scaling;

#ifdef SL_DEBUG
  printf("diff:%d, scaling:%d, pointa:%d, ssb:%d\n", diff, scs_scaling, pointA_ARFCN, SSB_ARFCN);
#endif

  phycfg->sl_bwp_config.sl_dc_location = (bwp_generic->sl_TxDirectCurrentLocation_r16) ?
                                            *bwp_generic->sl_TxDirectCurrentLocation_r16 : 0;

  const uint8_t values[] = {7,8,9,10,11,12,13,14};
  phycfg->sl_bwp_config.sl_num_symbols = (bwp_generic->sl_LengthSymbols_r16) ?
                                            values[*bwp_generic->sl_LengthSymbols_r16] : 0;

  phycfg->sl_bwp_config.sl_start_symbol = (bwp_generic->sl_StartSymbol_r16) ?
                                                  *bwp_generic->sl_StartSymbol_r16 : 0;

  //0-EXTENDED, 1-NORMAL CP
  phycfg->sl_bwp_config.sl_cyclic_prefix = (sl_bwp->cyclicPrefix) ? EXTENDED : NORMAL;

  AssertFatal(phycfg->sl_bwp_config.sl_cyclic_prefix == NORMAL, "Only NORMAL-CP Supported. Ext CP not yet supported\n");


  AssertFatal(phycfg->sl_bwp_config.sl_start_symbol >= 0 && phycfg->sl_bwp_config.sl_start_symbol <=7,
                                                                "Sidelink Start symbol should be in range 0-7\n");

  AssertFatal(phycfg->sl_bwp_config.sl_num_symbols >= 7 && phycfg->sl_bwp_config.sl_num_symbols <=14,
                                                                "Num Sidelink symbols should be in range 7-14\n");

  AssertFatal((phycfg->sl_bwp_config.sl_start_symbol + phycfg->sl_bwp_config.sl_num_symbols) <= 14,
                                                          "Incorrect configuration of Start and num SL symbols\n");

  //Configure PHY with TDD config only if the sync source is known.
  if (sync_source == SL_SYNC_SOURCE_LOCAL_TIMING ||
      sync_source == SL_SYNC_SOURCE_GNSS) {

    phycfg->config_mask = 0xF;//Total config is sent
    phycfg->sl_sync_source.gnss_dfn_offset = sl_OffsetDFN;

    // TDD Table Configuration
    sl_set_tdd_config_nr_ue(&phycfg->tdd_table,
                            phycfg->sl_bwp_config.sl_scs,
                            &sl_TDD_config->pattern1);

    LOG_I(NR_MAC, "SIDELINK CONFIGs: tdd config period:%ld, mu:%ld, DLslots:%ld,ULslots:%ld Mixedslotsym DL:UL %ld:%ld\n",
                          sl_TDD_config->pattern1.dl_UL_TransmissionPeriodicity, sl_TDD_config->referenceSubcarrierSpacing,
                          sl_TDD_config->pattern1.nrofDownlinkSlots, sl_TDD_config->pattern1.nrofUplinkSlots,
                          sl_TDD_config->pattern1.nrofDownlinkSymbols,sl_TDD_config->pattern1.nrofUplinkSymbols);

  } else if (sync_source == SL_SYNC_SOURCE_NONE) {
    //Only Carrier config, BWP config sent
    phycfg->config_mask = 0x9;//partial config is sent
  }

//#ifdef SL_DEBUG
  char str[5][20] = {"NONE","GNBENB","GNSS","SYNC_REF_UE","LOCAL_TIMING"};
  LOG_I(NR_MAC, "UE[%d] Function %s - Phy config preparation:\n",module_id, __FUNCTION__);
  LOG_I(NR_MAC, "UE[%d] Sync source params: sync_source :%d-%s, gnss_dfn_offset:%d, rx_slss_id:%d\n",
                                                        module_id,phycfg->sl_sync_source.sync_source,
                                                        str[phycfg->sl_sync_source.sync_source],
                                                        phycfg->sl_sync_source.gnss_dfn_offset,
                                                        phycfg->sl_sync_source.rx_slss_id);
  LOG_I(NR_MAC, "UE[%d] Carrier CFG Params: freq:%ld, bw:%d, gridsize:%d, valueN:%d\n",
                                                        module_id,phycfg->sl_carrier_config.sl_frequency,
                                                        phycfg->sl_carrier_config.sl_bandwidth,
                                                        phycfg->sl_carrier_config.sl_grid_size,
                                                        phycfg->sl_carrier_config.sl_value_N);
  LOG_I(NR_MAC, "UE[%d] SL-BWP Params: start:%d, size:%d, scs:%d, Ncp:%d, startsym:%d, numsym:%d,ssb_offset:%d,dcloc:%d\n",
                                                        module_id,phycfg->sl_bwp_config.sl_bwp_start,
                                                        phycfg->sl_bwp_config.sl_bwp_size,
                                                        phycfg->sl_bwp_config.sl_scs,
                                                        phycfg->sl_bwp_config.sl_cyclic_prefix,
                                                        phycfg->sl_bwp_config.sl_start_symbol,
                                                        phycfg->sl_bwp_config.sl_num_symbols,
                                                        phycfg->sl_bwp_config.sl_ssb_offset_point_a,
                                                        phycfg->sl_bwp_config.sl_dc_location);

//#endif

  return;
}

// RRC calls this API when RRC is configured with Sidelink PRE-configuration I.E
int nr_rrc_mac_config_req_sl_preconfig(module_id_t module_id,
                                       NR_SL_PreconfigurationNR_r16_t *sl_preconfiguration,
                                       uint8_t sync_source)
{

  LOG_I(NR_MAC,"[UE%d] SL RRC->MAC CONFIG RECEIVED. Syncsource:%d\n",
                                                module_id, sync_source);

  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  AssertFatal(sl_preconfiguration !=NULL,"SL-Preconfig Cannot be NULL");
  AssertFatal(mac, "mac should have an instance");

  if (!mac->SL_MAC_PARAMS)
    mac->SL_MAC_PARAMS = CALLOC(1, sizeof(sl_nr_ue_mac_params_t));

  sl_nr_ue_mac_params_t *sl_mac = mac->SL_MAC_PARAMS;

  NR_SidelinkPreconfigNR_r16_t *sl_preconfig = &sl_preconfiguration->sidelinkPreconfigNR_r16;

  //Only one entry supported in rel16.
  //Carrier freq config used for Sidelink

  NR_SL_FreqConfigCommon_r16_t *freqcfg = (sl_preconfig->sl_PreconfigFreqInfoList_r16)
                                              ? sl_preconfig->sl_PreconfigFreqInfoList_r16->list.array[0]
                                              : NULL;

  AssertFatal(freqcfg !=NULL,"SL fcfg Cannot be NULL");

  //MAx num of consecutive HARQ DTX before triggering RLF.
  const uint8_t MaxNumConsecutiveDTX[] = {1,2,3,4,6,8,16,32};
  sl_mac->sl_MaxNumConsecutiveDTX = (sl_preconfig->sl_MaxNumConsecutiveDTX_r16)
                                        ? MaxNumConsecutiveDTX[*sl_preconfig->sl_MaxNumConsecutiveDTX_r16]
                                        : 0;

  //priority of SL-SSB tx and rx
  sl_mac->sl_SSB_PriorityNR = (sl_preconfig->sl_SSB_PriorityNR_r16)
                                      ? *sl_preconfig->sl_SSB_PriorityNR_r16 : 0;

  //Indicates if CSI Reporting is enabled in UNICAST. is 0-ENABLED, 1-DISABLED
  sl_mac->sl_CSI_Acquisition = (sl_preconfig->sl_CSI_Acquisition_r16) ? 0 : 1;

  //Used for DFN calculation in case Sync source = GNSS.
  uint32_t sl_OffsetDFN = (sl_preconfig->sl_OffsetDFN_r16)
                                ? *sl_preconfig->sl_OffsetDFN_r16 : 0;

  NR_SL_BWP_ConfigCommon_r16_t *bwp = NULL;
  if (freqcfg->sl_BWP_List_r16 &&
      freqcfg->sl_BWP_List_r16->list.array[0])
    bwp = freqcfg->sl_BWP_List_r16->list.array[0];

  AssertFatal(bwp!=NULL, "BWP config common cannot be NULL\n");
  if (bwp->sl_BWP_PoolConfigCommon_r16) {
    if (bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16) {

      for (int i=0;i<bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16->list.count;i++) {
        NR_SL_ResourcePool_r16_t *rxpool = bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16->list.array[i];
        if (rxpool) {
          if (sl_mac->sl_RxPool[i] == NULL)
            sl_mac->sl_RxPool[i] = malloc16_clear(sizeof(SL_ResourcePool_params_t));
          sl_mac->sl_RxPool[i]->respool = rxpool;
          uint16_t sci_1a_len = 0, num_subch = 0;
          sci_1a_len = sl_determine_sci_1a_len(&num_subch,
                                               sl_mac->sl_RxPool[i]->respool,
                                               &sl_mac->sl_RxPool[i]->sci_1a);
          sl_mac->sl_RxPool[i]->num_subch = num_subch;
          sl_mac->sl_RxPool[i]->sci_1a_len = sci_1a_len;

          LOG_I(NR_MAC,"Rxpool[%d] - num subchannels:%d, sci_1a_len:%d\n",i,
                                                  sl_mac->sl_RxPool[i]->num_subch,
                                                  sl_mac->sl_RxPool[i]->sci_1a_len);

        }
      }
    }

    if (bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16) {

      for (int i=0;i<bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16->list.count;i++) {

        NR_SL_ResourcePool_r16_t *txpool =
                bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16->list.array[0]->sl_ResourcePool_r16;

        if (txpool) {
          if (sl_mac->sl_TxPool[i] == NULL)
            sl_mac->sl_TxPool[i] = malloc16_clear(sizeof(SL_ResourcePool_params_t));
          sl_mac->sl_TxPool[i]->respool = txpool;

          uint16_t sci_1a_len = 0, num_subch = 0;
          sci_1a_len = sl_determine_sci_1a_len(&num_subch,
                                               sl_mac->sl_TxPool[i]->respool,
                                               &sl_mac->sl_TxPool[i]->sci_1a);

          sl_mac->sl_TxPool[i]->num_subch = num_subch;
          sl_mac->sl_TxPool[i]->sci_1a_len = sci_1a_len;

          LOG_I(NR_MAC,"Txpool[%d] - num subchannels:%d, sci_1a_len:%d\n",i,
                                                      sl_mac->sl_TxPool[i]->num_subch,
                                                      sl_mac->sl_TxPool[i]->sci_1a_len);
        }
      }
    }
  }

  if (sync_source == SL_SYNC_SOURCE_GNSS ||
      sync_source == SL_SYNC_SOURCE_LOCAL_TIMING) {

    NR_TDD_UL_DL_ConfigCommon_t *tdd_uldl_config = NULL;
    if (sl_preconfig->sl_PreconfigGeneral_r16 &&
        sl_preconfig->sl_PreconfigGeneral_r16->sl_TDD_Configuration_r16)
      tdd_uldl_config = sl_preconfig->sl_PreconfigGeneral_r16->sl_TDD_Configuration_r16;

    AssertFatal((tdd_uldl_config!=NULL), "Sidelink MAC CFG: TDD Config cannot be NULL");
    AssertFatal((tdd_uldl_config->pattern2 == NULL), "Sidelink MAC CFG: pattern2 not yet supported");

    sl_mac->sl_TDD_config = sl_preconfig->sl_PreconfigGeneral_r16->sl_TDD_Configuration_r16;
  }

  //Do not copy TDD config yet as SYNC source is not yet found
  if (sync_source == SL_SYNC_SOURCE_NONE) {
    if (sl_mac->sl_TDD_config)
      ASN_STRUCT_FREE(asn_DEF_NR_TDD_UL_DL_ConfigCommon, sl_mac->sl_TDD_config);
    sl_mac->sl_TDD_config = NULL;
  }

  nr_sl_phy_config_t *sl_phy_cfg = &sl_mac->sl_phy_config;
  sl_phy_cfg->Mod_id = module_id;
  sl_phy_cfg->CC_id = 0;

  sl_prepare_phy_config(module_id, &sl_phy_cfg->sl_config_req,
                        freqcfg, sync_source, sl_OffsetDFN, sl_mac->sl_TDD_config);

  return 0;
}


//Copies the values of SSB time allocation from ASN format to MAC context
static void sl_mac_config_ssb_time_alloc(uint8_t module_id,
                                         NR_SL_SSB_TimeAllocation_r16_t *sl_SSB_TimeAllocation_r16,
                                         sl_ssb_timealloc_t *ssb_time_alloc)
{

  const uint8_t values[] = {1,2,4,8,16,32,64};
  ssb_time_alloc->sl_NumSSB_WithinPeriod =
                                      (sl_SSB_TimeAllocation_r16->sl_NumSSB_WithinPeriod_r16 != NULL)
                                             ? values[*sl_SSB_TimeAllocation_r16->sl_NumSSB_WithinPeriod_r16] : 0;
  ssb_time_alloc->sl_TimeOffsetSSB = (sl_SSB_TimeAllocation_r16->sl_TimeOffsetSSB_r16 != NULL)
                                             ? *sl_SSB_TimeAllocation_r16->sl_TimeOffsetSSB_r16 : 0;
  ssb_time_alloc->sl_TimeInterval =  (sl_SSB_TimeAllocation_r16->sl_TimeInterval_r16 != NULL)
                                             ? *sl_SSB_TimeAllocation_r16->sl_TimeInterval_r16 : 0;

}



//This API is called by RRC after it determines that UE needs to transmit SL-SSB
// SLSS id and SL-MIB is given to MAC by RRC
void nr_rrc_mac_transmit_slss_req(module_id_t module_id,
                                  uint8_t *sl_mib_payload,
                                  uint16_t tx_slss_id,
                                  NR_SL_SSB_TimeAllocation_r16_t *ssb_ta)
{

  sl_nr_ue_mac_params_t *sl_mac = get_mac_inst(module_id)->SL_MAC_PARAMS;
  AssertFatal(sl_mac,"sidelink MAC cannot be NULL");
  AssertFatal(tx_slss_id < 672,"SLSS id cannot be >= 672. id:%d", tx_slss_id);
  AssertFatal(ssb_ta,"ssb_ta cannot be NULL");

  sl_mac->tx_sl_bch.slss_id = tx_slss_id;
  sl_mac->tx_sl_bch.status = 1;
  memcpy(sl_mac->tx_sl_bch.sl_mib,sl_mib_payload, 4);

  sl_mac->tx_sl_bch.num_ssb = 0;
  sl_mac->tx_sl_bch.ssb_slot = 0;

  sl_mac_config_ssb_time_alloc(module_id,
                               ssb_ta,
                               &sl_mac->tx_sl_bch.ssb_time_alloc);

  LOG_I(NR_MAC,"[UE%d]SL RRC->MAC: TX SLSS REQ SLSS-id:%d, SL-MIB:%x, numssb:%d, offset:%d, interval:%d\n",
                                                  module_id, sl_mac->tx_sl_bch.slss_id,
                                                  *((uint32_t *)sl_mib_payload),
                                                  sl_mac->tx_sl_bch.ssb_time_alloc.sl_NumSSB_WithinPeriod,
                                                  sl_mac->tx_sl_bch.ssb_time_alloc.sl_TimeOffsetSSB,
                                                  sl_mac->tx_sl_bch.ssb_time_alloc.sl_TimeInterval);

  uint8_t byte0 = 0;
  uint8_t byte1 = 0;
  sl_nr_bwp_config_t *cfg = &sl_mac->sl_phy_config.sl_config_req.sl_bwp_config;
  sl_prepare_psbch_payload(sl_mac->sl_TDD_config, &byte0, &byte1,
                            cfg->sl_scs,cfg->sl_num_symbols,cfg->sl_start_symbol);

  sl_mac->tx_sl_bch.sl_mib[0] = byte0;
  sl_mac->tx_sl_bch.sl_mib[1] = byte1 | sl_mac->tx_sl_bch.sl_mib[1];

  LOG_I(NR_MAC, "[UE%d]SL RRC->MAC: TX SLSS REQ - TDD CONFIG STUFFED INSIDE - SL-MIB :%x\n",
                                                    module_id, *((uint32_t *)sl_mac->tx_sl_bch.sl_mib));
}

//This API is called by RRC after it determines that UE needs to keep
// receiving SL-SSB from the sync ref UE
void nr_rrc_mac_config_req_sl_mib(module_id_t module_id,
                                  NR_SL_SSB_TimeAllocation_r16_t *ssb_ta,
                                  uint16_t rx_slss_id,
                                  uint8_t *sl_mib)
{

  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  sl_nr_ue_mac_params_t *sl_mac = mac->SL_MAC_PARAMS;
  AssertFatal(sl_mac,"Sidelink MAC instance cannot be NULL");

  AssertFatal(ssb_ta,"ssb_ta cannot be NULL");
  AssertFatal(rx_slss_id < 672,"SLSS id cannot be >= 672. id:%d", rx_slss_id);
  AssertFatal(sl_mib,"sl_mib cannot be NULL");

  sl_nr_phy_config_request_t *sl_config = &sl_mac->sl_phy_config.sl_config_req;

  //Update configs if Sync source is not set else nothing to be done
  if (  sl_config->sl_sync_source.sync_source == SL_SYNC_SOURCE_NONE) {
    //Set SYNC source as SYNC REF UE and send the remaining config to PHY
    sl_config->config_mask = 0xF;//all configs done.
    sl_config->sl_sync_source.sync_source = SL_SYNC_SOURCE_SYNC_REF_UE;
    sl_config->sl_sync_source.rx_slss_id = rx_slss_id;


    sl_mac->rx_sl_bch.status = 1;
    sl_mac->rx_sl_bch.slss_id = rx_slss_id;

    sl_mac->rx_sl_bch.num_ssb = 0;
    sl_mac->rx_sl_bch.ssb_slot = 0;

    sl_mac_config_ssb_time_alloc(module_id,
                                ssb_ta,
                                &sl_mac->rx_sl_bch.ssb_time_alloc);

    LOG_I(NR_MAC,"[UE%d]SL RRC->MAC: RX SLSS REQ SLSS-id:%d, SL-MIB:%x, numssb:%d, offset:%d, interval:%d\n",
                                                      module_id, sl_mac->rx_sl_bch.slss_id,
                                                      *((uint32_t *)sl_mib),
                                                    sl_mac->rx_sl_bch.ssb_time_alloc.sl_NumSSB_WithinPeriod,
                                                    sl_mac->rx_sl_bch.ssb_time_alloc.sl_TimeOffsetSSB,
                                                    sl_mac->rx_sl_bch.ssb_time_alloc.sl_TimeInterval);

    if (sl_mac->sl_TDD_config == NULL)
      sl_mac->sl_TDD_config = CALLOC(sizeof(NR_TDD_UL_DL_ConfigCommon_t), 1);

    sl_nr_phy_config_request_t *cfg = &sl_mac->sl_phy_config.sl_config_req;
    int ret = 1;
    ret = sl_decode_sl_TDD_Config(sl_mac->sl_TDD_config,
                                  sl_mib[0], sl_mib[1]&0xF0,
                                  cfg->sl_bwp_config.sl_scs,
                                  cfg->sl_bwp_config.sl_num_symbols,
                                  cfg->sl_bwp_config.sl_start_symbol);

    if (ret == 0) {
      //sl_tdd_config bytes are all 1's - no TDD config present use all slots for sidelink.
      //Spec not clear -- TBD...
      sl_mac->sl_TDD_config->pattern1.nrofUplinkSlots =
                        NR_NUMBER_OF_SUBFRAMES_PER_FRAME*(1<<cfg->sl_bwp_config.sl_scs);
    }

    sl_set_tdd_config_nr_ue(&cfg->tdd_table,
                            cfg->sl_bwp_config.sl_scs,
                            &sl_mac->sl_TDD_config->pattern1);
    LOG_I(MAC, "SIDELINK CONFIGs: tdd config period:%ld, mu:%ld, DLslots:%ld,ULslots:%ld Mixedslotsym DL:UL %ld:%ld\n",
                            sl_mac->sl_TDD_config->pattern1.dl_UL_TransmissionPeriodicity,sl_mac->sl_TDD_config->referenceSubcarrierSpacing,
                            sl_mac->sl_TDD_config->pattern1.nrofDownlinkSlots, sl_mac->sl_TDD_config->pattern1.nrofUplinkSlots,
                            sl_mac->sl_TDD_config->pattern1.nrofDownlinkSymbols,sl_mac->sl_TDD_config->pattern1.nrofUplinkSymbols);

  }

}

