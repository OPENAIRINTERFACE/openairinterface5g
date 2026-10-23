/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "rrc_gNB_nrdc.h"

#include "rrc_gNB_UE_context.h"
#include "rrc_gNB_du.h"
#include "rrc_cell_management.h"
#include "rrc_gNB_measurements.h"
#include "rrc_gNB_radio_bearers.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac.h"
#include "NR_UE-CapabilityRequestFilterNR.h"
#include "NR_UECapabilityEnquiry-v1560-IEs.h"
#include "common/utils/time_manager/time_timer.h"
#include "openair2/F1AP/lib/f1ap_ue_context.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "openair2/RRC/NR/rrc_gNB_NGAP.h"

typedef enum {
  NRDC_NONE,
  ACTIVATE_NRDC_WAIT_FOR_CAPABILITIES,
  ACTIVATE_NRDC_WAIT_FOR_A4_RECONFIGURATION_COMPLETE,
  ACTIVATE_NRDC_WAIT_FOR_SCG_MEASUREMENT,
  ACTIVATE_NRDC_WAIT_FOR_F1_CONTEXT_SETUP_RESPONSE,
  ACTIVATE_NRDC_WAIT_FOR_ACTIVATION_RECONFIGURATION_COMPLETE,
  NRDC_ACTIVE,
  DEACTIVATE_NRDC_WAIT_FOR_F1_CONTEXT_MODIFICATION_RESPONSE,
  DEACTIVATE_NRDC_WAIT_FOR_PDU_SESSION_RELEASE_RECONFIGURATION_COMPLETE,
  NRDC_DU_CONTEXT_TO_RELEASE,
  NRDC_DEACTIVATED,
  SCG_FAILURE_WAIT_FOR_F1_CONTEXT_MODIFICATION_RESPONSE,
  SCG_FAILURE_WAIT_FOR_RECONFIGURATION_COMPLETE
} nrdc_state_t;

typedef struct {
  nrdc_state_t state;
  int mcg_band;
  int scg_band;
  uint64_t mcg_cell_id;
  uint64_t scg_cell_id;
  int xid;
  void *timer;
  OCTET_STRING_t uecap;
  int meas_object_id;
  int report_config_id;
  int measurement_id;
  int du_assoc_id;
  uint32_t secondary_du_ue_id;
  int drb_id;
  int pdu_session_to_release;
} nrdc_ue_state_t;

static int generate_ue_capability_enquiry(uint8_t *out, int outsize, int xid, int mcg_band, int scg_band)
{
  /* capability request filter */
  NR_UE_CapabilityRequestFilterNR_t filter = {
    .frequencyBandListFilter = &(struct NR_FreqBandList) {
      .list = {
        .array = (struct NR_FreqBandInformation *[]) {
          &(struct NR_FreqBandInformation) {
            .present = NR_FreqBandInformation_PR_bandInformationNR,
            .choice = {
              .bandInformationNR = &(struct NR_FreqBandInformationNR) {
                .bandNR = (NR_FreqBandIndicatorNR_t) mcg_band
              }
            }
          },
          &(struct NR_FreqBandInformation) {
            .present = NR_FreqBandInformation_PR_bandInformationNR,
            .choice = {
              .bandInformationNR = &(struct NR_FreqBandInformationNR) {
                .bandNR = (NR_FreqBandIndicatorNR_t) scg_band
              }
            }
          }
        },
        .count = 2
      }
    }
  };

  uint8_t filter_buf[512];
  int filter_buf_size = sizeof(filter_buf);
  asn_enc_rval_t size = uper_encode_to_buffer(&asn_DEF_NR_UE_CapabilityRequestFilterNR, NULL, &filter, filter_buf, filter_buf_size);
  AssertFatal(size.encoded > 0, "failed to encode FreqBandInformationNR\n");
  filter_buf_size = (size.encoded + 7) / 8;

  /* capability enquiry extension */
  NR_UECapabilityEnquiry_v1560_IEs_t ext = {
    .capabilityRequestFilterCommon = &(struct NR_UE_CapabilityRequestFilterCommon) {
      .mrdc_Request = &(struct NR_UE_CapabilityRequestFilterCommon__mrdc_Request) {
        .includeNR_DC = &(long) { NR_UE_CapabilityRequestFilterCommon__mrdc_Request__includeNR_DC_true }
      },
      .ext2 = &(struct NR_UE_CapabilityRequestFilterCommon__ext2) {
        .requestedCellGrouping_r16 = &(struct NR_UE_CapabilityRequestFilterCommon__ext2__requestedCellGrouping_r16) {
          .list = {
            .array = (struct NR_CellGrouping_r16 *[]) {
              &(struct NR_CellGrouping_r16) {
                .mcg_r16 = {
                  .list = {
                    .array = (NR_FreqBandIndicatorNR_t *[]) {
                      &(NR_FreqBandIndicatorNR_t) { mcg_band }
                    },
                    .count = 1
                  }
                },
                .scg_r16 = {
                  .list = {
                    .array = (NR_FreqBandIndicatorNR_t *[]) {
                      &(NR_FreqBandIndicatorNR_t) { scg_band }
                    },
                    .count = 1
                  }
                },
                /* not sure of this sync/async mode, see 38.133 chapter 7
                 * and maybe 38.321, 38.214, 38.401 (chapter 9, synchronization)
                 * for TAG where timing matters
                 */
                .mode_r16 = NR_CellGrouping_r16__mode_r16_async
              }
            },
            .count = 1
          }
        }
      }
    }
  };

  uint8_t ext_buf[512];
  int ext_buf_size = sizeof(ext_buf);
  size = uper_encode_to_buffer(&asn_DEF_NR_UECapabilityEnquiry_v1560_IEs, NULL, &ext, ext_buf, ext_buf_size);
  AssertFatal(size.encoded > 0, "failed to encode FreqBandInformationNR\n");
  ext_buf_size = (size.encoded + 7) / 8;

  /* capability enquiry */
  NR_DL_DCCH_Message_t m = {
    .message = {
      .present = NR_DL_DCCH_MessageType_PR_c1,
      .choice = {
        .c1 = &(struct NR_DL_DCCH_MessageType__c1) {
          .present = NR_DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry,
          .choice = {
            .ueCapabilityEnquiry = &(struct NR_UECapabilityEnquiry) {
              .rrc_TransactionIdentifier = xid,
              .criticalExtensions = {
                .present = NR_UECapabilityEnquiry__criticalExtensions_PR_ueCapabilityEnquiry,
                .choice = {
                  .ueCapabilityEnquiry = &(struct NR_UECapabilityEnquiry_IEs) {
                    .ue_CapabilityRAT_RequestList = {
                      .list = {
                        .array = (struct NR_UE_CapabilityRAT_Request *[]) {
                          &(struct NR_UE_CapabilityRAT_Request) {
                            .rat_Type = NR_RAT_Type_nr,
                            .capabilityRequestFilter = &(OCTET_STRING_t) {
                              .buf = filter_buf,
                              .size = filter_buf_size
                            }
                          }
                        },
                        .count = 1
                      }
                    },
                    .ue_CapabilityEnquiryExt = &(OCTET_STRING_t) {
                      .buf = ext_buf,
                      .size = ext_buf_size
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  };

  size = uper_encode_to_buffer(&asn_DEF_NR_DL_DCCH_Message, NULL, &m, out, outsize);
  AssertFatal(size.encoded > 0, "failed to encode UECapabilityEnquiry\n");

  return (size.encoded + 7) / 8;
}

static int generate_a4_measurement(gNB_RRC_UE_t *ue,
                                   uint8_t *out,
                                   int outsize,
                                   int xid,
                                   int scg_band,
                                   int scg_ssb_arfcn,
                                   int scg_ssb_subcarrier_spacing,
                                   NR_SSB_MTC_t *du_ssb_mtc)
{
  nrdc_ue_state_t *nrdc = ue->nrdc;
  nrdc->meas_object_id = allocate_measurement_object_id(ue);
  nrdc->report_config_id = allocate_report_config_id(ue);
  nrdc->measurement_id = allocate_measurement_id(ue);

  NR_DL_DCCH_Message_t m = {
    .message = {
      .present = NR_DL_DCCH_MessageType_PR_c1,
      .choice = {
        .c1 = &(struct NR_DL_DCCH_MessageType__c1) {
          .present = NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration,
          .choice = {
            .rrcReconfiguration = &(struct NR_RRCReconfiguration) {
              .rrc_TransactionIdentifier = xid,
              .criticalExtensions = {
                .present = NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration,
                .choice = {
                  .rrcReconfiguration = &(struct NR_RRCReconfiguration_IEs){
                    .measConfig = &(struct NR_MeasConfig) {
                      .measObjectToAddModList = &(struct NR_MeasObjectToAddModList) {
                        .list = {
                          .array = (struct NR_MeasObjectToAddMod *[]) {
                            &(struct NR_MeasObjectToAddMod) {
                              .measObjectId = nrdc->meas_object_id,
                              .measObject = {
                                .present = NR_MeasObjectToAddMod__measObject_PR_measObjectNR,
                                .choice = {
                                  .measObjectNR = &(struct NR_MeasObjectNR) {
                                    .ssbFrequency = &(NR_ARFCN_ValueNR_t) { scg_ssb_arfcn },
                                    .ssbSubcarrierSpacing = &(NR_SubcarrierSpacing_t) { scg_ssb_subcarrier_spacing },
                                    .smtc1 = du_ssb_mtc,
                                    .referenceSignalConfig = {
                                      .ssb_ConfigMobility = &(struct NR_SSB_ConfigMobility) {
                                        .deriveSSB_IndexFromCell = false                   /* hardcoded */
                                      }
                                    },
                                    .absThreshSS_BlocksConsolidation = &(struct NR_ThresholdNR) {
                                      .thresholdRSRP = &(NR_RSRP_Range_t) { 36 }           /* hardcoded */
                                    },
                                    .nrofSS_BlocksToAverage = &(long) { 4 },               /* hardcoded */
                                    .quantityConfigIndex = 1,                              /* hardcoded, todo: check this value */
                                    .ext1 = &(struct NR_MeasObjectNR__ext1) {
                                      .freqBandIndicatorNR = &(NR_FreqBandIndicatorNR_t) { scg_band }
                                    }
                                  }
                                }
                              }
                            }
                          },
                          .count = 1
                        }
                      },
                      .reportConfigToAddModList = &(struct NR_ReportConfigToAddModList) {
                        .list = {
                          .array = (struct NR_ReportConfigToAddMod *[]) {
                            &(struct NR_ReportConfigToAddMod) {
                              .reportConfigId = nrdc->report_config_id,
                              .reportConfig = {
                                .present = NR_ReportConfigToAddMod__reportConfig_PR_reportConfigNR,
                                .choice = {
                                  .reportConfigNR = &(struct NR_ReportConfigNR) {
                                    .reportType = {
                                      .present = NR_ReportConfigNR__reportType_PR_eventTriggered,
                                      .choice = {
                                        .eventTriggered = &(struct NR_EventTriggerConfig) {
                                          .eventId = {
                                            .present = NR_EventTriggerConfig__eventId_PR_eventA4,
                                            .choice = {
                                              .eventA4 = &(struct NR_EventTriggerConfig__eventId__eventA4) {
                                                .a4_Threshold = {
                                                  .present = NR_MeasTriggerQuantity_PR_rsrp,
                                                  .choice = {
                                                    .rsrp = 20                             /* hardcoded */
                                                  }
                                                },
                                                .reportOnLeave = false,                    /* hardcoded */
                                                .hysteresis = 0,                           /* hardcoded */
                                                .timeToTrigger = NR_TimeToTrigger_ms40,    /* hardcoded */
                                                .useAllowedCellList = false                /* todo: check this */
                                              }
                                            }
                                          },
                                          .rsType = NR_NR_RS_Type_ssb,                     /* hardcoded */
                                          .reportInterval = NR_ReportInterval_ms120,       /* hardcoded */
                                          //.reportAmount = NR_EventTriggerConfig__reportAmount_r2,      /* todo: choose a proper value, maybe 1 would be enough? */
                                          .reportAmount = NR_EventTriggerConfig__reportAmount_infinity,
                                          .reportQuantityCell = {
                                            .rsrp = true,                                  /* hardcoded */
                                            .rsrq = true,                                  /* hardcoded */
                                            .sinr = true                                   /* hardcoded */
                                          },
                                          .maxReportCells = 4,                             /* hardcoded */
                                          .reportQuantityRS_Indexes = &(NR_MeasReportQuantity_t){
                                            .rsrp = true,                                  /* hardcoded */
                                            .rsrq = true,                                  /* hardcoded */
                                            .sinr = true                                   /* hardcoded */
                                          },
                                          .maxNrofRS_IndexesToReport = &(long) { 32 },     /* hardcoded */
                                          .includeBeamMeasurements = true                  /* hardcoded */
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          },
                          .count = 1
                        }
                      },
                      .measIdToAddModList = &(struct NR_MeasIdToAddModList) {
                        .list = {
                          .array = (struct NR_MeasIdToAddMod *[]) {
                            &(struct NR_MeasIdToAddMod) {
                              .measId = nrdc->measurement_id,
                              .measObjectId = nrdc->meas_object_id,
                              .reportConfigId = nrdc->report_config_id
                            }
                          },
                          .count = 1
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  };
  asn_enc_rval_t size = uper_encode_to_buffer(&asn_DEF_NR_DL_DCCH_Message, NULL, &m, out, outsize);
  AssertFatal(size.encoded > 0, "failed to encode A4 measurement\n");

  return (size.encoded + 7) / 8;
}

/* called from timer thread, so simply send a "nrdc timeout" ITTI message to
 * RRC that will do the real work
 */
static void timeout(void **p, uint64_t *v)
{
  gNB_RRC_INST *rrc = p[0];
  uint64_t ue_id = v[0];
  int xid = v[1];
  nrdc_state_t state = v[2];

  MessageDef *message_p = itti_alloc_new_message(TASK_RRC_GNB, rrc->module_id, NR_RRC_NRDC_TIMEOUT);
  nr_rrc_nrdc_timeout_t *m = &NR_RRC_NRDC_TIMEOUT(message_p);
  memset(m, 0, sizeof(*m));
  m->ue_id = ue_id;
  m->xid = xid;
  m->state = state;

  itti_send_msg_to_task(TASK_RRC_GNB, rrc->module_id, message_p);
}

/* This function checks that the CU has DUs connected that correspond
 * to a configured NR-DC combination.
 * For the first one that is found, the UE capabilities for NR-DC are
 * requested.
 */
void rrc_gnb_nrdc_start(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue)
{
  /* if already in NR-DC, do nothing */
  if (ue->nrdc)
    return;

  /* look for an NR-DC combination, configured and available */
  nr_rrc_cell_container_t *ue_pcell = rrc_get_pcell_for_ue(rrc, ue);
  if (ue_pcell == NULL) {
    LOG_W(NR_RRC, "NR-DC: UE %d has no PCell, don't try to activate NR-DC\n", ue->rrc_ue_id);
    return;
  }

  bool combination_found = false;
  int mcg_band = 0;
  int scg_band = 0;
  uint64_t mcg_cell_id = 0;
  uint64_t scg_cell_id = 0;

  for (int i = 0; i < rrc->nrdc_config.combination_count; i++) {
    nr_rrc_cell_container_t *mcg_cell = get_cell_by_cell_id(&rrc->cells, rrc->nrdc_config.combinations[i].mcg_cell_id);
    nr_rrc_cell_container_t *scg_cell = get_cell_by_cell_id(&rrc->cells, rrc->nrdc_config.combinations[i].scg_cell_id);
    /* MCG cell must be the one used by the UE */
    if (mcg_cell == ue_pcell && scg_cell != NULL) {
      mcg_band = mcg_cell->info.mode == NR_MODE_TDD ? mcg_cell->info.tdd.dlul.band : mcg_cell->info.fdd.dl.band;
      scg_band = scg_cell->info.mode == NR_MODE_TDD ? scg_cell->info.tdd.dlul.band : scg_cell->info.fdd.dl.band;
      mcg_cell_id = rrc->nrdc_config.combinations[i].mcg_cell_id;
      scg_cell_id = rrc->nrdc_config.combinations[i].scg_cell_id;
      combination_found = true;
      break;
    }
  }

  if (!combination_found) {
    LOG_W(NR_RRC, "NR-DC: UE %d: no NR-DC combination found, NR-DC will not be activated\n", ue->rrc_ue_id);
    return;
  }

  /* a combination is found, start NR-DC, ask for UE capabilities */
  LOG_I(NR_RRC, "NR-DC: UE %d: trying band combination MCG band %d and SCG band %d\n", ue->rrc_ue_id, mcg_band, scg_band);

  /* allocate the NR-DC state data in the UE */
  nrdc_ue_state_t *nrdc = calloc_or_fail(1, sizeof(*nrdc));
  ue->nrdc = nrdc;

  nrdc->state = ACTIVATE_NRDC_WAIT_FOR_CAPABILITIES;

  nrdc->mcg_band = mcg_band;
  nrdc->scg_band = scg_band;
  nrdc->mcg_cell_id = mcg_cell_id;
  nrdc->scg_cell_id = scg_cell_id;

  int xid = rrc_gNB_get_next_transaction_identifier(rrc->module_id);
  nrdc->xid = xid;
  ue->xids[xid] = RRC_F1_NRDC_IN_PROGRESS;

  uint8_t ue_cap[1024];
  int size = generate_ue_capability_enquiry(ue_cap, sizeof(ue_cap), xid, mcg_band, scg_band);

  /* send it to the UE */
  uint32_t msg_id = NR_DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry;
  nr_rrc_transfer_protected_rrc_message(rrc, ue, DL_SCH_LCID_DCCH, msg_id, ue_cap, size);

  /* start a timer to react after some time (2s, arbitrary value) if the UE does not reply */
  nrdc->timer = tick_timeout_start(2000, timeout,
                                  (void *[]){ rrc }, 1,
                                  (uint64_t []){ ue->rrc_ue_id, xid, nrdc->state }, 3);
}

void rrc_gnb_free_nrdc(gNB_RRC_UE_t *ue)
{
  if (!ue->nrdc)
    return;
  nrdc_ue_state_t *nrdc = ue->nrdc;
  if (nrdc->timer) {
    tick_timeout_stop(nrdc->timer);
    nrdc->timer = NULL;
  }
  free(nrdc->uecap.buf);
  nrdc->uecap.buf = NULL;
  nrdc->uecap.size = 0;
  free(ue->nrdc);
  ue->nrdc = NULL;
}

static OCTET_STRING_t *extract_nr_capabilities(const NR_UECapabilityInformation_t *ue_cap)
{
  if (ue_cap == NULL
      || ue_cap->criticalExtensions.present != NR_UECapabilityInformation__criticalExtensions_PR_ueCapabilityInformation)
    return NULL;
  struct NR_UECapabilityInformation_IEs *cap = ue_cap->criticalExtensions.choice.ueCapabilityInformation;
  if (cap == NULL)
    return NULL;
  struct NR_UE_CapabilityRAT_ContainerList *clist = cap->ue_CapabilityRAT_ContainerList;
  if (clist == NULL)
    return NULL;

  for (int i = 0; i < clist->list.count; i++) {
    struct NR_UE_CapabilityRAT_Container *ct = clist->list.array[i];
    if (ct != NULL && ct->rat_Type == NR_RAT_Type_nr)
      return &ct->ue_CapabilityRAT_Container;
  }

  return NULL;
}

static bool band_combination_supported_by_ue(OCTET_STRING_t *nr_uecap, int mcg_band, int scg_band)
{
  /* decode the NR UE capabilities container */
  NR_UE_NR_Capability_t *cap = NULL;
  asn_dec_rval_t rval = uper_decode(NULL, &asn_DEF_NR_UE_NR_Capability, (void **)&cap, nr_uecap->buf, nr_uecap->size, 0, 0);

  if (rval.code != RC_OK) {
    LOG_W(NR_RRC, "NR-DC: failed to decode UE capabilities, ignoring\n");
    return false;
  }

  if (cap == NULL)
    return false;

  struct NR_BandCombinationList *comblist = cap->rf_Parameters.supportedBandCombinationList;
  if (comblist == NULL) {
    ASN_STRUCT_FREE(asn_DEF_NR_UE_NR_Capability, cap);
    return false;
  }

  /* check all supported combinations */
  for (int i = 0; i < comblist->list.count; i++) {
    bool mcg_found = false;
    bool scg_found = false;

    /* check all bands of the combination.
     * We accept that there are more bands in the combination.
     * As long as both MCG and SCG are present with DL and UL
     * bandwidth classes both not NULL, then we're happy.
     */
    /* todo: validate bandwidth classes */
    struct NR_BandCombination *comb = comblist->list.array[i];
    if (comb == NULL)
      continue;
    for (int i = 0; i < comb->bandList.list.count; i++) {
      struct NR_BandParameters *p = comb->bandList.list.array[i];
      if (p == NULL)
        continue;
      /* check MCG */
      if (mcg_found == false) {
        if (p->present == NR_BandParameters_PR_nr
            && p->choice.nr != NULL
            && p->choice.nr->bandNR == mcg_band
            && p->choice.nr->ca_BandwidthClassDL_NR != NULL
            && p->choice.nr->ca_BandwidthClassUL_NR != NULL) {
          mcg_found = true;
          /* in case mcg band == scg band, skip scg check */
          continue;
        }
      }
      /* check SCG */
      if (scg_found == false) {
        if (p->present == NR_BandParameters_PR_nr
            && p->choice.nr != NULL
            && p->choice.nr->bandNR == scg_band
            && p->choice.nr->ca_BandwidthClassDL_NR != NULL
            && p->choice.nr->ca_BandwidthClassUL_NR != NULL)
          scg_found = true;
      }
    }

    if (mcg_found && scg_found) {
      ASN_STRUCT_FREE(asn_DEF_NR_UE_NR_Capability, cap);
      return true;
    }
  }

  ASN_STRUCT_FREE(asn_DEF_NR_UE_NR_Capability, cap);

  return false;
}

void rrc_gnb_nrdc_ue_capabilities_received(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue,  const NR_UECapabilityInformation_t *ue_cap)
{
  LOG_D(NR_RRC, "rrc_gnb_nrdc_ue_capabilities_received called!!\n");

  nrdc_ue_state_t *nrdc = ue->nrdc;
  if (!nrdc) {
    LOG_W(NR_RRC, "no NR-DC context found for ue %d\n", ue->rrc_ue_id);
    return;
  }

  if (nrdc->state != ACTIVATE_NRDC_WAIT_FOR_CAPABILITIES) {
    LOG_W(NR_RRC, "ignore unexpected NR-DC capabilities received for ue %d\n", ue->rrc_ue_id);
    return;
  }

  /* stop the timer */
  if (nrdc->timer)
    tick_timeout_stop(nrdc->timer);
  nrdc->timer = NULL;

  ue->xids[nrdc->xid] = RRC_ACTION_NONE;
  nrdc->xid = -1;

  /* save the capabilities to send to SCG DU later */
  OCTET_STRING_t *nr_uecap = extract_nr_capabilities(ue_cap);
  if (nr_uecap == NULL) {
    /* capabilities have to be there */
    LOG_E(NR_RRC, "NR-DC: no NR capabilities found, cancelling NR-DC activation procedure for UE %d\n", ue->rrc_ue_id);
    rrc_gnb_free_nrdc(ue);
    return;
  }
  DevAssert(nrdc->uecap.buf == NULL);
  nrdc->uecap.buf = malloc_or_fail(nr_uecap->size);
  memcpy(nrdc->uecap.buf, nr_uecap->buf, nr_uecap->size);
  nrdc->uecap.size = nr_uecap->size;

  /* check capabilities */
  if (!band_combination_supported_by_ue(nr_uecap, nrdc->mcg_band, nrdc->scg_band)) {
    LOG_W(NR_RRC, "NR-DC: UE %d does not support MCG band %d and SCG band %d, do not activate NR-DC\n",
          ue->rrc_ue_id, nrdc->mcg_band, nrdc->scg_band);
    rrc_gnb_free_nrdc(ue);
    return;
  }

  /* capabilities accept the configured NR-DC combination */
  nrdc->state = ACTIVATE_NRDC_WAIT_FOR_A4_RECONFIGURATION_COMPLETE;

  nr_rrc_cell_container_t *cell = get_cell_by_cell_id(&rrc->cells, nrdc->scg_cell_id);
  if (!cell) {
    /* no cell found, cancel the process */
    LOG_E(NR_RRC, "no cell found for SCG band %d, cancelling NR-DC activation procedure for UE %d\n", nrdc->scg_band, ue->rrc_ue_id);
    rrc_gnb_free_nrdc(ue);
    return;
  }
  NR_MeasTimingList_t *mtlist = cell->mtc->criticalExtensions.choice.c1->choice.measTimingConf->measTiming;
  NR_SSB_MTC_t *ssb_mtc = &mtlist->list.array[0]->frequencyAndTiming->ssb_MeasurementTimingConfiguration;

  /* configure A4 for the SCG SpCell */
  int xid = rrc_gNB_get_next_transaction_identifier(rrc->module_id);
  nrdc->xid = xid;
  ue->xids[xid] = RRC_F1_NRDC_IN_PROGRESS;

  uint8_t a4_buf[1024];
  int scs = cell->info.mode == NR_MODE_TDD ? cell->info.tdd.dlul.scs : cell->info.fdd.dl.scs;
  int size = generate_a4_measurement(ue,
                                     a4_buf,
                                     sizeof(a4_buf),
                                     xid,
                                     nrdc->scg_band,
                                     get_ssb_arfcn(cell),
                                     scs,
                                     ssb_mtc);

  /* send it to the UE */
  uint32_t msg_id = NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration;
  nr_rrc_transfer_protected_rrc_message(rrc, ue, DL_SCH_LCID_DCCH, msg_id, a4_buf, size);

  /* start a timer to react after some time (2s, arbitrary value) if the UE does not reply */
  nrdc->timer = tick_timeout_start(2000, timeout,
                                   (void *[]){ rrc }, 1,
                                   (uint64_t []){ ue->rrc_ue_id, xid, nrdc->state }, 3);
}

/* this function is called for ack of A4 measurement setting
 * secondary cell group addition, PDU session release
 */
void rrc_gnb_nrdc_rrc_reconfiguration_complete_received(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue, int xid)
{
  LOG_D(NR_RRC, "rrc_gnb_nrdc_rrc_reconfiguration_complete_received called!!\n");

  nrdc_ue_state_t *nrdc = ue->nrdc;
  if (!nrdc) {
    LOG_W(NR_RRC, "no NR-DC context found for ue %d\n", ue->rrc_ue_id);
    return;
  }

  if (nrdc->state != ACTIVATE_NRDC_WAIT_FOR_A4_RECONFIGURATION_COMPLETE
      && nrdc->state != ACTIVATE_NRDC_WAIT_FOR_ACTIVATION_RECONFIGURATION_COMPLETE
      && nrdc->state != DEACTIVATE_NRDC_WAIT_FOR_PDU_SESSION_RELEASE_RECONFIGURATION_COMPLETE
      && nrdc->state != SCG_FAILURE_WAIT_FOR_RECONFIGURATION_COMPLETE) {
    LOG_W(NR_RRC, "ignore unexpected NR-DC RRC Reconfiguration received for ue %d\n", ue->rrc_ue_id);
    return;
  }

  if (xid != nrdc->xid) {
    LOG_W(NR_RRC, "ignore wrong NR-DC procedure for ue %d (received %d expected %d)\n", ue->rrc_ue_id, xid, nrdc->xid);
    return;
  }

  /* stop the timer if any */
  if (nrdc->timer)
    tick_timeout_stop(nrdc->timer);
  nrdc->timer = NULL;

  ue->xids[xid] = RRC_ACTION_NONE;
  nrdc->xid = -1;

  if (nrdc->state == ACTIVATE_NRDC_WAIT_FOR_A4_RECONFIGURATION_COMPLETE) {
    /* measurements correctly configured for NR-DC combination
     * do nothing until we receive a measurement
     * (or until the UE is removed from the system)
     */
    nrdc->state = ACTIVATE_NRDC_WAIT_FOR_SCG_MEASUREMENT;
    return;
  }

  if (nrdc->state == DEACTIVATE_NRDC_WAIT_FOR_PDU_SESSION_RELEASE_RECONFIGURATION_COMPLETE) {
    /* set the xid of the pdu session */
    /* todo: remove this 'xid' for pdu session structure, it makes no sense at all */
    FOR_EACH_SEQ_ARR(rrc_pdu_session_param_t *, session, &ue->pduSessions) {
      if (session->param.pdusession_id == nrdc->pdu_session_to_release) {
        session->xid = xid;
      }
    }
    rrc_gNB_send_NGAP_PDUSESSION_RELEASE_RESPONSE(rrc, ue, xid);
    LOG_I(NR_RRC, "ue %d: NR-DC PDU session is removed\n", ue->rrc_ue_id);
    nrdc->state = NRDC_DU_CONTEXT_TO_RELEASE;
    return;
  }

  if (nrdc->state == SCG_FAILURE_WAIT_FOR_RECONFIGURATION_COMPLETE) {
    LOG_I(NR_RRC, "NR-DC: SCG released for UE %d (UE is not in NR-DC anymore)\n", ue->rrc_ue_id);
    rrc_gnb_free_nrdc(ue);
    return;
  }

  /* Reconfiguration Complete for activation is received, NR-DC is now active */
  nrdc->state = NRDC_ACTIVE;
  LOG_I(NR_RRC, "ue %d: NR-DC is now active\n", ue->rrc_ue_id);
}

void rrc_gnb_nrdc_measurement_received(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue, NR_MeasurementReport_t *meas)
{
  LOG_D(NR_RRC, "rrc_gnb_nrdc_measurement_received called!!\n");

  nrdc_ue_state_t *nrdc = ue->nrdc;
  if (!nrdc) {
    LOG_W(NR_RRC, "no NR-DC context found for ue %d\n", ue->rrc_ue_id);
    return;
  }

  if (nrdc->state != ACTIVATE_NRDC_WAIT_FOR_SCG_MEASUREMENT) {
    LOG_W(NR_RRC, "ignore NR-DC measurement for ue %d\n", ue->rrc_ue_id);
    return;
  }

  /* retrieve the UE primary DU ID */
  f1_ue_data_t ue_data = cu_get_f1_ue_data(ue->rrc_ue_id);
  RETURN_IF_INVALID_ASSOC_ID(ue_data.du_assoc_id);
  uint32_t ue_du_id = ue_data.secondary_ue;

  /* measurement received, send context setup request to the secondary DU
   * including CG-ConfigInfo to inform the secondary DU that the CU requests
   * an SCG establishment
   */

  nr_rrc_cell_container_t *cell = get_cell_by_cell_id(&rrc->cells, nrdc->scg_cell_id);
  if (!cell) {
    /* no cell found, cancel the process */
    LOG_E(NR_RRC, "no cell found for SCG band %d, cancelling NR-DC activation procedure for UE %d\n", nrdc->scg_band, ue->rrc_ue_id);
    rrc_gnb_free_nrdc(ue);
    return;
  }

  uint64_t *ue_agg_mbr = malloc_or_fail(sizeof(*ue_agg_mbr));
  *ue_agg_mbr = 2000000000 /*bps*/;       /* todo: hardcoded, use correct value */

  /* tranfser the first DRB (should be the 'default' one) */
  /* note: this code is copy/paste from fill_drb_to_be_setup(), think about how to factorize */
  int nb_drb = 1;
  f1ap_drb_to_setup_t *drbs = calloc_or_fail(1, sizeof(*drbs));
  f1ap_drb_to_setup_t *drb = &drbs[0];
  drb_t *rrc_drb = seq_arr_front(&ue->drbs);
  DevAssert(rrc_drb);
  /* fetch an existing PDU session for this DRB */
  rrc_pdu_session_param_t *pdu = find_pduSession_from_drbId(ue, rrc_drb->drb_id);
  AssertFatal(pdu != NULL, "no PDU session for DRB ID %d\n", rrc_drb->drb_id);

  drb->id = rrc_drb->drb_id;
  nrdc->drb_id = rrc_drb->drb_id;

  drb->qos_choice = F1AP_QOS_CHOICE_NR;
  drb->nr.nssai = pdu->param.nssai;
  drb->nr.flows_len = 1;
  drb->nr.flows = calloc_or_fail(1, sizeof(*drb->nr.flows));

  // Find the QoS flow associated with this DRB
  // Since we don't have QFI mapping in the new structure, we'll use the first QoS flow
  AssertFatal(seq_arr_size(&pdu->param.qos) == 1, "only 1 Qos flow supported\n");
  nr_rrc_qos_t *qos_param = (nr_rrc_qos_t *)seq_arr_at(&pdu->param.qos, 0);
  DevAssert(qos_param->qos.qfi > 0);
  drb->nr.flows[0].qfi = qos_param->qos.qfi;
  drb->nr.flows[0].param = nr_rrc_get_f1_qos_flow_param(&qos_param->qos);
  /* the DRB QoS parameters: we just reuse the ones from the first flow */
  drb->nr.drb_qos = drb->nr.flows[0].param;

  memcpy(&drb->up_ul_tnl[0].tl_address, &rrc_drb->cuup_tunnel_config.addr.buffer, sizeof(uint8_t) * 4);
  drb->up_ul_tnl[0].teid = rrc_drb->cuup_tunnel_config.teid;
  drb->up_ul_tnl_len = 1;

  drb->rlc_mode = rrc->configuration.um_on_default_drb ? F1AP_RLC_MODE_UM_BIDIR : F1AP_RLC_MODE_AM;
  nr_pdcp_configuration_t *pdcp = &rrc_drb->pdcp_config;
  DevAssert(pdcp->drb.sn_size == 18 || pdcp->drb.sn_size == 12);
  drb->dl_pdcp_sn_len = malloc_or_fail(sizeof(*drb->dl_pdcp_sn_len));
  *drb->dl_pdcp_sn_len = pdcp->drb.sn_size == 18 ? F1AP_PDCP_SN_18B : F1AP_PDCP_SN_12B;
  drb->ul_pdcp_sn_len = malloc_or_fail(sizeof(*drb->ul_pdcp_sn_len));
  *drb->ul_pdcp_sn_len = pdcp->drb.sn_size == 18 ? F1AP_PDCP_SN_18B : F1AP_PDCP_SN_12B;

  /* create CG-ConfigInfo to inform the DU that the CU requests
   * an SCG establishment (38.331 11.2.2 CG-ConfigInfo)
   */
  /* include NR-DC UE NR capabilities into the CG Config Info, inside a container */
  NR_UE_CapabilityRAT_ContainerList_t uecap_container = {
    .list = {
      .array = (struct NR_UE_CapabilityRAT_Container *[]) {
        &(struct NR_UE_CapabilityRAT_Container) {
          .rat_Type = NR_RAT_Type_nr,
          .ue_CapabilityRAT_Container = nrdc->uecap
        }
      },
      .count = 1
    }
  };
  /* encode the container */
  OCTET_STRING_t uecap_container_buf = { 0 };
  ssize_t encoded = uper_encode_to_new_buffer(&asn_DEF_NR_UE_CapabilityRAT_ContainerList, NULL, &uecap_container, (void **)&uecap_container_buf.buf);
  DevAssert(encoded > 0);
  uecap_container_buf.size = encoded;

  /* create the CG Config Info containing the UE capabilities */
  NR_CG_ConfigInfo_t cg = {
    .criticalExtensions = {
      .present = NR_CG_ConfigInfo__criticalExtensions_PR_c1,
      .choice = {
        .c1 = &(struct NR_CG_ConfigInfo__criticalExtensions__c1) {
          .present = NR_CG_ConfigInfo__criticalExtensions__c1_PR_cg_ConfigInfo,
          .choice = {
            .cg_ConfigInfo = &(struct NR_CG_ConfigInfo_IEs) {
              .ue_CapabilityInfo = &uecap_container_buf
            }
          }
        }
      }
    }
  };
  /* encode the CG Config Info */
  OCTET_STRING_t cgbuf = { 0 };
  encoded = uper_encode_to_new_buffer(&asn_DEF_NR_CG_ConfigInfo, NULL, &cg, (void **)&cgbuf.buf);
  DevAssert(encoded > 0);
  cgbuf.size = encoded;
  free(uecap_container_buf.buf);

  /* create the F1 Context Setup Request message */
  byte_array_t *cg_configinfo = calloc_or_fail(1, sizeof(*cg_configinfo));
  *cg_configinfo = create_byte_array(cgbuf.size, cgbuf.buf);
  free(cgbuf.buf);

  nrdc->du_assoc_id = cell->assoc_id;

  f1ap_ue_context_setup_req_t ue_context_setup_req = {
    .gNB_CU_ue_id = ue->rrc_ue_id,
    .plmn.mcc = cell->info.plmn.mcc,
    .plmn.mnc = cell->info.plmn.mnc,
    .plmn.mnc_digit_length = cell->info.plmn.mnc_digit_length,
    .nr_cellid = cell->info.cell_id,
    .servCellIndex = 0, // TODO: correct value?
    .drbs_len = nb_drb,
    .drbs = drbs,
    .cu_to_du_rrc_info.cg_configinfo = cg_configinfo,
    .gnb_du_ue_agg_mbr_ul = ue_agg_mbr,
  };
  rrc->mac_rrc.ue_context_setup_request(cell->assoc_id, &ue_context_setup_req);
  free_ue_context_setup_req(&ue_context_setup_req);

  /* measurement received, send context modification request to the primary DU
   * to remove the transfered RLC bearer
   */

  /* note: we ignore the modification response, to be changed if needed,
   * modifications to be done in rrc_CU_process_ue_context_modification_response()
   * and then add something in this file somewhere
   */
  f1ap_drb_to_release_t *drbs_release = calloc_or_fail(1, sizeof(*drbs_release));
  f1ap_drb_to_release_t *drb_release = &drbs_release[0];
  drb_release->id = rrc_drb->drb_id;
  f1ap_ue_context_mod_req_t ue_context_mod_req = {
    .gNB_CU_ue_id = ue->rrc_ue_id,
    .gNB_DU_ue_id = ue_du_id,
    .drbs_rel_len = 1,
    .drbs_rel = drbs_release
  };
  rrc->mac_rrc.ue_context_modification_request(ue_data.du_assoc_id, &ue_context_mod_req);
  free_ue_context_mod_req(&ue_context_mod_req);

  nrdc->state = ACTIVATE_NRDC_WAIT_FOR_F1_CONTEXT_SETUP_RESPONSE;

  LOG_D(NR_RRC, "NR-DC activation: rrc->mac_rrc.ue_context_setup_request() has been called\n");
}

void rrc_gnb_nrdc_timeout(gNB_RRC_INST *rrc, nr_rrc_nrdc_timeout_t *timeout)
{
  rrc_gNB_ue_context_t *ue_context = rrc_gNB_get_ue_context(rrc, timeout->ue_id);
  if (!ue_context) {
    LOG_W(NR_RRC, "UE %"PRIu64" not found, discard NR-DC timeout\n", timeout->ue_id);
    return;
  }
  gNB_RRC_UE_t *ue = &ue_context->ue_context;
  int state = timeout->state;
  int xid = timeout->xid;

  nrdc_ue_state_t *nrdc = ue->nrdc;
  if (!nrdc) {
    LOG_W(NR_RRC, "no NR-DC context found for ue %d, discard timeout\n", ue->rrc_ue_id);
    return;
  }
  if (nrdc->state != state) {
    LOG_W(NR_RRC, "bad state %d for NR-DC context of ue %d, discard timeout\n", state, ue->rrc_ue_id);
    return;
  }
  if (nrdc->xid != xid) {
    LOG_W(NR_RRC, "bad xid %d for NR-DC context of ue %d, discard timeout\n", xid, ue->rrc_ue_id);
    return;
  }

  /* stop the timer (if any) */
  if (nrdc->timer)
    tick_timeout_stop(nrdc->timer);
  nrdc->timer = NULL;

  /* clear the current action for this xid (if any) */
  if (xid != -1) {
    ue->xids[xid] = RRC_ACTION_NONE;
  }

  /* cancel the NR-DC activation procedure */
  LOG_E(NR_RRC, "timeout, cancelling NR-DC activation procedure for UE %d\n", ue->rrc_ue_id);
  rrc_gnb_free_nrdc(ue);
}

int get_scg_measurement_id(gNB_RRC_UE_t *ue)
{
  if (!ue->nrdc)
    return -1;
  nrdc_ue_state_t *nrdc = ue->nrdc;
  return nrdc->measurement_id;
}

bool rrc_gnb_nrdc_wait_for_f1_context_setup_response(gNB_RRC_UE_t *ue)
{
  if (!ue->nrdc)
    return false;
  nrdc_ue_state_t *nrdc = ue->nrdc;
  return nrdc->state == ACTIVATE_NRDC_WAIT_FOR_F1_CONTEXT_SETUP_RESPONSE;
}

void nrdc_rrc_CU_process_ue_context_setup_response(gNB_RRC_UE_t *ue, gNB_RRC_INST *rrc, f1ap_ue_context_setup_resp_t *resp)
{
  LOG_D(NR_RRC, "nrdc_rrc_CU_process_ue_context_setup_response called\n");

  nrdc_ue_state_t *nrdc = ue->nrdc;
  if (!nrdc) {
    LOG_E(NR_RRC, "no NR-DC found for ue %d, ignore ContextSetupResponse\n", ue->rrc_ue_id);
    return;
  }

  /* store secondary DU UE ID */
  nrdc->secondary_du_ue_id = resp->gNB_DU_ue_id;

  /* modify GTP bearer endpoint */
  AssertFatal(resp->drbs_len == 1, "bad setup response\n");
  store_du_f1u_tunnel(resp->drbs, resp->drbs_len, ue);
  e1_send_bearer_updates(rrc, ue, resp->drbs_len, resp->drbs);

  /* reconfigure the UE to trigger NR-DC activation */
  OCTET_STRING_t cgbuf = {
    .buf = resp->du_to_cu_rrc_info.cell_group_config.buf,
    .size = resp->du_to_cu_rrc_info.cell_group_config.len
  };

  /* build an RRCReconfiguration that contains only the sCG */
  NR_RRCReconfiguration_t scg = {
    .rrc_TransactionIdentifier = 0 /* xid */,  /* not sure what to put here */
    .criticalExtensions = {
      .present = NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration,
      .choice = {
        .rrcReconfiguration = &(struct NR_RRCReconfiguration_IEs) {
          .secondaryCellGroup = &cgbuf
        }
      }
    }
  };

  /* encode it */
  OCTET_STRING_t nr_scg = { 0 };

  ssize_t encoded = uper_encode_to_new_buffer(&asn_DEF_NR_RRCReconfiguration, NULL, &scg, (void **)&nr_scg.buf);
  if (encoded <= 0) {
    AssertFatal(encoded > 0, "Failed to encode RRCReconfiguration\n");
    LOG_E(NR_RRC, "go_fr2: Failed to encode RRCReconfiguration\n");
    return;
  }
  nr_scg.size = encoded;

  /* we have to release rlc of drb1 in the master cell group */
  NR_CellGroupConfig_t mcg = {
    .cellGroupId = 0,
    .rlc_BearerToReleaseList = &(struct NR_CellGroupConfig__rlc_BearerToReleaseList) {
      .list = {
        .array = (NR_LogicalChannelIdentity_t *[]) {
          &(NR_LogicalChannelIdentity_t) {
            4
          }
        },
        .count = 1
      }
    }
  };

  /* encode the mcg */
  OCTET_STRING_t nr_mcg = { 0 };
  encoded = uper_encode_to_new_buffer(&asn_DEF_NR_CellGroupConfig, NULL, &mcg, (void **)&nr_mcg.buf);
  if (encoded <= 0) {
    AssertFatal(encoded > 0, "Failed to encode CellGroupConfig of MCG\n");
    LOG_E(NR_RRC, "go_fr2: Failed to encode CellGroupConfig of MCG\n");
    return;
  }
  nr_mcg.size = encoded;

  uint8_t xid = rrc_gNB_get_next_transaction_identifier(rrc->module_id);
  ue->xids[xid] = RRC_F1_NRDC_IN_PROGRESS;
  ue->ongoing_reconfiguration = true;

  /* build main RRCReconfiguration containing the sCG one
   * (we have to embed an RRCReconfiguration inside an RRCReconfiguration
   * as explained in 38.331 6.2.2 description of RRCReconfiguration,
   * especially the explaination about secondaryCellGroup)
   */
  NR_DL_DCCH_Message_t dl_dcch_msg = {
    .message = {
      .present = NR_DL_DCCH_MessageType_PR_c1,
      .choice = {
        .c1 = &(struct NR_DL_DCCH_MessageType__c1) {
          .present = NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration,
          .choice = {
            .rrcReconfiguration = &(struct NR_RRCReconfiguration) {
              .rrc_TransactionIdentifier = xid,
              .criticalExtensions = {
                .present = NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration,
                .choice = {
                  .rrcReconfiguration = &(struct NR_RRCReconfiguration_IEs) {
                    .measConfig = &(struct NR_MeasConfig) {
                      .measObjectToRemoveList = &(struct NR_MeasObjectToRemoveList) {
                        .list = {
                          .array = (NR_MeasObjectId_t *[]) {
                            &(NR_MeasObjectId_t) { nrdc->meas_object_id }
                          },
                          .count = 1
                        }
                      },
                      .reportConfigToRemoveList = &(struct NR_ReportConfigToRemoveList) {
                        .list = {
                          .array = (NR_ReportConfigId_t *[]) {
                            &(NR_ReportConfigId_t) { nrdc->report_config_id }
                          },
                          .count = 1
                        }
                      },
                      .measIdToRemoveList = &(struct NR_MeasIdToRemoveList) {
                        .list = {
                          .array = (NR_MeasId_t *[]) {
                            &(NR_MeasId_t) { nrdc->measurement_id }
                          },
                          .count = 1
                        }
                      }
                    },
                    .radioBearerConfig = &(struct NR_RadioBearerConfig) {
                      .drb_ToAddModList = &(struct NR_DRB_ToAddModList) {
                        .list = {
                          .array = (struct NR_DRB_ToAddMod *[]) {
                            &(struct NR_DRB_ToAddMod) {
                              .drb_Identity = (NR_DRB_Identity_t) nrdc->drb_id,
                              .recoverPDCP = &(long) {
                                NR_DRB_ToAddMod__recoverPDCP_true
                              }
                            }
                          },
                          .count = 1
                        }
                      },
                    },
                    .nonCriticalExtension = &(struct NR_RRCReconfiguration_v1530_IEs) {
                      .masterCellGroup = &nr_mcg,
                      .nonCriticalExtension = &(struct NR_RRCReconfiguration_v1540_IEs) {
                        .nonCriticalExtension = &(struct NR_RRCReconfiguration_v1560_IEs) {
                          .mrdc_SecondaryCellGroupConfig = &(struct NR_SetupRelease_MRDC_SecondaryCellGroupConfig) {
                            .present = NR_SetupRelease_MRDC_SecondaryCellGroupConfig_PR_setup,
                            .choice = {
                              .setup = &(struct NR_MRDC_SecondaryCellGroupConfig) {
                                .mrdc_SecondaryCellGroup = {
                                  .present = NR_MRDC_SecondaryCellGroupConfig__mrdc_SecondaryCellGroup_PR_nr_SCG,
                                  .choice = {
                                    .nr_SCG = nr_scg
                                  }
                                }
                              }
                            }
                          },
                          .sk_Counter = &(NR_SK_Counter_t) {
                            (NR_SK_Counter_t) 0
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  };

  /* free the measurement IDs */
  free_measurement_object_id(ue, nrdc->meas_object_id);
  free_report_config_id(ue, nrdc->report_config_id);
  free_measurement_id(ue, nrdc->measurement_id);
  nrdc->meas_object_id = -1;
  nrdc->report_config_id = -1;
  nrdc->measurement_id = -1;

  /* encode and send */
  unsigned char *buf = NULL;

  int len = uper_encode_to_new_buffer(&asn_DEF_NR_DL_DCCH_Message, NULL, &dl_dcch_msg, (void **)&buf);
  free(nr_mcg.buf);
  if (len <= 0) {
    AssertFatal(len > 0, "Failed to encode DL-DCCH message\n");
    LOG_E(NR_RRC, "go_fr2: Failed to encode DL-DCCH message\n");
    return;
  }

  const uint32_t msg_id = NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration;
  nr_rrc_transfer_protected_rrc_message(rrc, ue, DL_SCH_LCID_DCCH, msg_id, buf, len);

  free(buf);

  /* no need for a timer */
  nrdc->state = ACTIVATE_NRDC_WAIT_FOR_ACTIVATION_RECONFIGURATION_COMPLETE;
  nrdc->xid = xid;
}

bool is_nrdc_bearer(gNB_RRC_UE_t *ue, int rb_id)
{
  if (ue->nrdc == NULL)
    return false;

  nrdc_ue_state_t *nrdc = ue->nrdc;

  return nrdc->drb_id == rb_id;
}

void nrdc_release_bearer(const gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue, int bearer_id)
{
  if (ue->nrdc == NULL)
    return;
  nrdc_ue_state_t *nrdc = ue->nrdc;

  /* we must be in state NRDC_ACTIVE to release the bearer */
  DevAssert(nrdc->state == NRDC_ACTIVE);

  /* check that it's the right bearer, do nothing if not */
  if (nrdc->drb_id != bearer_id) {
    LOG_E(NR_RRC, "wrong NR-DC bearer released (expected %d, requested %d), do nothing\n", nrdc->drb_id, bearer_id);
    return;
  }

  /* find the PDU session associated to this bearer and remember it to
   * release it properly later (after RRC Reconfiguration)
   */
  drb_t *drb = get_drb(&ue->drbs, nrdc->drb_id);
  int pdu_session_id = drb->pdusession_id;
  nrdc->pdu_session_to_release = pdu_session_id;

  f1ap_ue_context_mod_req_t req = {
      .gNB_CU_ue_id = ue->rrc_ue_id,
      .gNB_DU_ue_id = nrdc->secondary_du_ue_id,
      .servCellIndex = RRC_PCELL_INDEX,
  };
  req.plmn = malloc_or_fail(sizeof(*req.plmn));
  *req.plmn = rrc->configuration.plmn[0];
  /* hack: gcc warning because of const, remove warning by explicit cast */
  gNB_RRC_INST *rrc_no_const = (gNB_RRC_INST *)rrc;
  nr_rrc_cell_container_t *cell = get_cell_by_cell_id(&rrc_no_const->cells, nrdc->scg_cell_id);
  DevAssert(cell != NULL);
  req.nr_cellid = malloc_or_fail(sizeof(*req.nr_cellid));
  *req.nr_cellid = cell->info.cell_id;
  req.drbs_rel = malloc_or_fail(sizeof(*req.drbs_rel));
  req.drbs_rel_len = 1;
  req.drbs_rel[0].id = nrdc->drb_id;

  /* request CellGroupConfig from SCG DU in the response */
  req.gNB_DU_Configuration_Query = calloc_or_fail(1, sizeof(*req.gNB_DU_Configuration_Query));
  *req.gNB_DU_Configuration_Query = true;

  /* send UE Context Modification to SCG DU */
  rrc->mac_rrc.ue_context_modification_request(cell->assoc_id, &req);
  LOG_I(NR_RRC, "NR-DC: UE %d: F1 Release DRB (%d)\n", ue->rrc_ue_id, nrdc->drb_id);
  free_ue_context_mod_req(&req);

  nrdc->state = DEACTIVATE_NRDC_WAIT_FOR_F1_CONTEXT_MODIFICATION_RESPONSE;
}

bool rrc_gnb_nrdc_wait_for_f1_context_modification_response(gNB_RRC_UE_t *ue)
{
  if (!ue->nrdc)
    return false;
  nrdc_ue_state_t *nrdc = ue->nrdc;
  return nrdc->state == DEACTIVATE_NRDC_WAIT_FOR_F1_CONTEXT_MODIFICATION_RESPONSE;
}

void nrdc_rrc_CU_process_ue_context_modification_response(gNB_RRC_UE_t *ue, gNB_RRC_INST *rrc, f1ap_ue_context_mod_resp_t *resp)
{
  LOG_D(NR_RRC, "nrdc_rrc_CU_process_ue_context_modification_response called\n");

  nrdc_ue_state_t *nrdc = ue->nrdc;
  if (!nrdc) {
    LOG_E(NR_RRC, "no NR-DC found for ue %d, ignore ContextModificationResponse\n", ue->rrc_ue_id);
    return;
  }

  if (nrdc->state == SCG_FAILURE_WAIT_FOR_F1_CONTEXT_MODIFICATION_RESPONSE)
    return nrdc_scg_failure_context_modification_response(ue, rrc, resp);

  DevAssert(nrdc->state == DEACTIVATE_NRDC_WAIT_FOR_F1_CONTEXT_MODIFICATION_RESPONSE);
  DevAssert(ue->nas_pdu.buf && ue->nas_pdu.len);

  /* only works for deactivation, where we need du_to_cu_rrc_info, but no drbs and no srbs to setup */
  if (resp->drbs_len || resp->srbs_len || resp->du_to_cu_rrc_info == NULL
      || resp->du_to_cu_rrc_info->cell_group_config.buf == NULL
      || resp->du_to_cu_rrc_info->cell_group_config.len == 0) {
    LOG_E(NR_RRC, "NR-DC ignore bad UE Context Modification Response\n");
    return;
  }

  int xid = rrc_gNB_get_next_transaction_identifier(rrc->module_id);
  nrdc->xid = xid;
  ue->xids[xid] = RRC_F1_NRDC_IN_PROGRESS;

  NR_DL_DCCH_Message_t dl_dcch_msg = {
    .message = {
      .present = NR_DL_DCCH_MessageType_PR_c1,
      .choice = { .c1 = &(struct NR_DL_DCCH_MessageType__c1) {
          .present = NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration,
          .choice = {
            .rrcReconfiguration = &(struct NR_RRCReconfiguration) {
              .rrc_TransactionIdentifier = xid,
              .criticalExtensions = {
                .present = NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration,
                .choice = {
                  .rrcReconfiguration = &(struct NR_RRCReconfiguration_IEs) {
                    .nonCriticalExtension = &(struct NR_RRCReconfiguration_v1530_IEs) {
                      .dedicatedNAS_MessageList = &(struct NR_RRCReconfiguration_v1530_IEs__dedicatedNAS_MessageList) {
                        .list = {
                          .array = (NR_DedicatedNAS_Message_t *[]) {
                            &(NR_DedicatedNAS_Message_t) {
                              .buf = ue->nas_pdu.buf,
                              .size = ue->nas_pdu.len
                            }
                          },
                          .count = 1
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  };

  /* encode and send */
  unsigned char *buf = NULL;

  int len = uper_encode_to_new_buffer(&asn_DEF_NR_DL_DCCH_Message, NULL, &dl_dcch_msg, (void **)&buf);
  if (len <= 0) {
    AssertFatal(len > 0, "Failed to encode DL-DCCH message\n");
    LOG_E(NR_RRC, "NR-DC Failed to encode DL-DCCH message\n");
    return;
  }

  /* free the nas_pdu */
  free(ue->nas_pdu.buf);
  ue->nas_pdu.buf = NULL;
  ue->nas_pdu.len = 0;

  const uint32_t msg_id = NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration;
  nr_rrc_transfer_protected_rrc_message(rrc, ue, DL_SCH_LCID_DCCH, msg_id, buf, len);

  free(buf);

  nrdc->state = DEACTIVATE_NRDC_WAIT_FOR_PDU_SESSION_RELEASE_RECONFIGURATION_COMPLETE;
  nrdc->drb_id = -1;
}

/* return true if the message is handled here */
bool nrdc_handle_f1_context_release_complete(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue, f1ap_ue_context_rel_cplt_t *complete)
{
  if (ue->nrdc == NULL)
    return false;
  nrdc_ue_state_t *nrdc = ue->nrdc;

  if (complete->gNB_DU_ue_id != nrdc->secondary_du_ue_id)
    return false;

  /* nothing to do for the moment, to be changed if needed */

  LOG_I(NR_RRC, "NR-DC: UE %d received F1 UE Context Release Complete\n", ue->rrc_ue_id);

  return true;
}

static bool is_ue_releasable(gNB_RRC_UE_t *ue)
{
  if (ue->nrdc == NULL)
    return false;

  nrdc_ue_state_t *nrdc = ue->nrdc;

  if (nrdc->state == NRDC_NONE
      || nrdc->state == ACTIVATE_NRDC_WAIT_FOR_CAPABILITIES
      || nrdc->state == ACTIVATE_NRDC_WAIT_FOR_A4_RECONFIGURATION_COMPLETE
      || nrdc->state == ACTIVATE_NRDC_WAIT_FOR_SCG_MEASUREMENT
      || nrdc->state == NRDC_DEACTIVATED)
    return false;

  return true;
}

void nrdc_scg_ue_release(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue)
{
  if (ue->nrdc == NULL)
    return;
  nrdc_ue_state_t *nrdc = ue->nrdc;

  /* if the UE is not fully configured yet, do nothing */
  if (!is_ue_releasable(ue))
    return;

  LOG_I(NR_RRC, "NR-DC: release/deactivate SCG for UE %d\n", ue->rrc_ue_id);

  f1ap_ue_context_rel_cmd_t cmd = {
    .gNB_CU_ue_id = ue->rrc_ue_id,
    .gNB_DU_ue_id = nrdc->secondary_du_ue_id,
    .cause = F1AP_CAUSE_RADIO_NETWORK,
    .cause_value = 10, // 10 = F1AP_CauseRadioNetwork_normal_release
  };
  rrc->mac_rrc.ue_context_release_command(nrdc->du_assoc_id, &cmd);

  /* NR-DC is put to deactivated mode, release will happen when the UE is removed from the CU */
  nrdc->state = NRDC_DEACTIVATED;
}

void nrdc_handle_scg_failure_information(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue, NR_SCGFailureInformation_t *msg)
{
  if (ue->nrdc == NULL)
    return;
  nrdc_ue_state_t *nrdc = ue->nrdc;

  /* SCG Failure: put back the NR-DC DRB from SCG to MCG */

  uint64_t *ue_agg_mbr = malloc_or_fail(sizeof(*ue_agg_mbr));
  *ue_agg_mbr = 2000000000 /*bps*/;       /* todo: hardcoded, use correct value */

  /* tranfser back the first DRB (should be the 'default' one) */
  /* note: this code is copy/paste from fill_drb_to_be_setup(), think about how to factorize */
  f1ap_drb_to_setup_t *drbs = calloc_or_fail(1, sizeof(*drbs));
  f1ap_drb_to_setup_t *drb = &drbs[0];
  /* bad, don't use seq_arr_front(), take the real one (nrdc->ndr_id) */
  drb_t *rrc_drb = seq_arr_front(&ue->drbs);
  DevAssert(rrc_drb);
  /* fetch an existing PDU session for this DRB */
  rrc_pdu_session_param_t *pdu = find_pduSession_from_drbId(ue, rrc_drb->drb_id);
  AssertFatal(pdu != NULL, "no PDU session for DRB ID %d\n", rrc_drb->drb_id);

  drb->id = nrdc->drb_id;

  drb->qos_choice = F1AP_QOS_CHOICE_NR;
  drb->nr.nssai = pdu->param.nssai;
  drb->nr.flows_len = 1;
  drb->nr.flows = calloc_or_fail(1, sizeof(*drb->nr.flows));

  // Find the QoS flow associated with this DRB
  // Since we don't have QFI mapping in the new structure, we'll use the first QoS flow
  AssertFatal(seq_arr_size(&pdu->param.qos) == 1, "only 1 Qos flow supported\n");
  nr_rrc_qos_t *qos_param = (nr_rrc_qos_t *)seq_arr_at(&pdu->param.qos, 0);
  DevAssert(qos_param->qos.qfi > 0);
  drb->nr.flows[0].qfi = qos_param->qos.qfi;
  drb->nr.flows[0].param = nr_rrc_get_f1_qos_flow_param(&qos_param->qos);
  /* the DRB QoS parameters: we just reuse the ones from the first flow */
  drb->nr.drb_qos = drb->nr.flows[0].param;

  memcpy(&drb->up_ul_tnl[0].tl_address, &rrc_drb->cuup_tunnel_config.addr.buffer, sizeof(uint8_t) * 4);
  drb->up_ul_tnl[0].teid = rrc_drb->cuup_tunnel_config.teid;
  drb->up_ul_tnl_len = 1;

  drb->rlc_mode = rrc->configuration.um_on_default_drb ? F1AP_RLC_MODE_UM_BIDIR : F1AP_RLC_MODE_AM;
  nr_pdcp_configuration_t *pdcp = &rrc_drb->pdcp_config;
  DevAssert(pdcp->drb.sn_size == 18 || pdcp->drb.sn_size == 12);
  drb->dl_pdcp_sn_len = malloc_or_fail(sizeof(*drb->dl_pdcp_sn_len));
  *drb->dl_pdcp_sn_len = pdcp->drb.sn_size == 18 ? F1AP_PDCP_SN_18B : F1AP_PDCP_SN_12B;
  drb->ul_pdcp_sn_len = malloc_or_fail(sizeof(*drb->ul_pdcp_sn_len));
  *drb->ul_pdcp_sn_len = pdcp->drb.sn_size == 18 ? F1AP_PDCP_SN_18B : F1AP_PDCP_SN_12B;

  /* create the F1 Context Modification Request message */
  f1_ue_data_t ue_data = cu_get_f1_ue_data(ue->rrc_ue_id);
  uint32_t ue_du_id = ue_data.secondary_ue;

  bool *gNB_DU_Configuration_Query = calloc_or_fail(1, sizeof(*gNB_DU_Configuration_Query));
  *gNB_DU_Configuration_Query = true;

  f1ap_ue_context_mod_req_t ue_context_mod_req = {
    .gNB_CU_ue_id = ue->rrc_ue_id,
    .gNB_DU_ue_id = ue_du_id,
    .drbs_len = 1,
    .drbs = drbs,
    .gNB_DU_Configuration_Query = gNB_DU_Configuration_Query
  };
  rrc->mac_rrc.ue_context_modification_request(ue_data.du_assoc_id, &ue_context_mod_req);
  free_ue_context_mod_req(&ue_context_mod_req);

  /* remove the UE from SCG DU (we ignore the replied UEContextReleaseComplete) */
  nrdc_scg_ue_release(rrc, ue);

  nrdc->state = SCG_FAILURE_WAIT_FOR_F1_CONTEXT_MODIFICATION_RESPONSE;
}

bool rrc_gnb_nrdc_wait_for_scg_failure_context_modification_response(gNB_RRC_UE_t *ue)
{
  if (!ue->nrdc)
    return false;
  nrdc_ue_state_t *nrdc = ue->nrdc;
  return nrdc->state == SCG_FAILURE_WAIT_FOR_F1_CONTEXT_MODIFICATION_RESPONSE;
}

void nrdc_scg_failure_context_modification_response(gNB_RRC_UE_t *ue, gNB_RRC_INST *rrc, f1ap_ue_context_mod_resp_t *resp)
{
  LOG_E(NR_RRC, "nrdc_scg_failure_context_modification_response called\n");

  if (ue->nrdc == NULL)
    return;
  nrdc_ue_state_t *nrdc = ue->nrdc;

  /* modify GTP bearer endpoint */
  AssertFatal(resp->drbs_len == 1, "bad scg failure context modification response\n");
  store_du_f1u_tunnel(resp->drbs, resp->drbs_len, ue);
  e1_send_bearer_updates(rrc, ue, resp->drbs_len, resp->drbs);

  /* reconfigure the UE to release the SCG and put back the bearer to MCG */
  OCTET_STRING_t cgbuf = { .buf = resp->du_to_cu_rrc_info->cell_group_config.buf,
                           .size = resp->du_to_cu_rrc_info->cell_group_config.len };

  int xid = rrc_gNB_get_next_transaction_identifier(rrc->module_id);
  nrdc->xid = xid;
  ue->xids[xid] = RRC_F1_NRDC_IN_PROGRESS;

  NR_DL_DCCH_Message_t dl_dcch_msg = {
    .message = {
      .present = NR_DL_DCCH_MessageType_PR_c1,
      .choice = { .c1 = &(struct NR_DL_DCCH_MessageType__c1) {
          .present = NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration,
          .choice = {
            .rrcReconfiguration = &(struct NR_RRCReconfiguration) {
              .rrc_TransactionIdentifier = xid,
              .criticalExtensions = {
                .present = NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration,
                .choice = {
                  .rrcReconfiguration = &(struct NR_RRCReconfiguration_IEs) {
                    .radioBearerConfig = &(struct NR_RadioBearerConfig) {
                      .drb_ToAddModList = &(struct NR_DRB_ToAddModList) {
                        .list = {
                          .array = (struct NR_DRB_ToAddMod *[]) {
                            &(struct NR_DRB_ToAddMod) {
                              .drb_Identity = (NR_DRB_Identity_t) nrdc->drb_id,
                              .recoverPDCP = &(long) {
                                NR_DRB_ToAddMod__recoverPDCP_true
                              }
                            }
                          },
                          .count = 1
                        }
                      },
                    },
                    .nonCriticalExtension = &(struct NR_RRCReconfiguration_v1530_IEs) {
                      .masterCellGroup = &cgbuf,
                      .nonCriticalExtension = &(struct NR_RRCReconfiguration_v1540_IEs) {
                        .nonCriticalExtension = &(struct NR_RRCReconfiguration_v1560_IEs) {
                          .mrdc_SecondaryCellGroupConfig = &(struct NR_SetupRelease_MRDC_SecondaryCellGroupConfig) {
                            .present = NR_SetupRelease_MRDC_SecondaryCellGroupConfig_PR_release,
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  };

  /* encode and send */
  unsigned char *buf = NULL;

  int len = uper_encode_to_new_buffer(&asn_DEF_NR_DL_DCCH_Message, NULL, &dl_dcch_msg, (void **)&buf);
  if (len <= 0) {
    AssertFatal(len > 0, "Failed to encode DL-DCCH message\n");
    LOG_E(NR_RRC, "NR-DC Failed to encode DL-DCCH message\n");
    return;
  }

  const uint32_t msg_id = NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration;
  nr_rrc_transfer_protected_rrc_message(rrc, ue, DL_SCH_LCID_DCCH, msg_id, buf, len);
  free(buf);

  nrdc->state = SCG_FAILURE_WAIT_FOR_RECONFIGURATION_COMPLETE;
}
