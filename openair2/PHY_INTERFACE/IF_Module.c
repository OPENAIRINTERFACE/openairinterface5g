#include "openair1/PHY/defs_eNB.h"
#include "openair2/PHY_INTERFACE/IF_Module.h"
#include "openair1/PHY/phy_extern.h"
#include "LAYER2/MAC/mac_extern.h"
#include "LAYER2/MAC/mac_proto.h"
#include "common/ran_context.h"
#include "nfapi/oai_integration/vendor_ext.h"
#define MAX_IF_MODULES 100

IF_Module_t *if_inst[MAX_IF_MODULES];
Sched_Rsp_t Sched_INFO[MAX_IF_MODULES][MAX_NUM_CCs];

extern int oai_nfapi_harq_indication(nfapi_harq_indication_t *harq_ind);
extern int oai_nfapi_crc_indication(nfapi_crc_indication_t *crc_ind);
extern int oai_nfapi_cqi_indication(nfapi_cqi_indication_t *cqi_ind);
extern int oai_nfapi_sr_indication(nfapi_sr_indication_t *ind);
extern int oai_nfapi_rx_ind(nfapi_rx_indication_t *ind);

extern uint16_t sf_ahead;
extern UL_RCC_IND_t  UL_RCC_INFO;

uint16_t frame_cnt=0;
void handle_rach(UL_IND_t *UL_info) {
  int i;

  if(NFAPI_MODE == NFAPI_MODE_VNF) {
    for(uint8_t j = 0; j < NUM_NFPAI_SUBFRAME; j++) {
      if (UL_RCC_INFO.rach_ind[j].rach_indication_body.number_of_preambles>0) {
        AssertFatal(UL_RCC_INFO.rach_ind[j].rach_indication_body.number_of_preambles==1,"More than 1 preamble not supported\n");
        LOG_D(MAC,"UL_info[Frame %d, Subframe %d] Calling initiate_ra_proc RACH:SFN/SF:%d\n",UL_info->frame,UL_info->subframe, NFAPI_SFNSF2DEC(UL_RCC_INFO.rach_ind[j].sfn_sf));
        initiate_ra_proc(UL_info->module_id,
                         UL_info->CC_id,
                         NFAPI_SFNSF2SFN(UL_RCC_INFO.rach_ind[j].sfn_sf),
                         NFAPI_SFNSF2SF(UL_RCC_INFO.rach_ind[j].sfn_sf),
                         UL_RCC_INFO.rach_ind[j].rach_indication_body.preamble_list[0].preamble_rel8.preamble,
                         UL_RCC_INFO.rach_ind[j].rach_indication_body.preamble_list[0].preamble_rel8.timing_advance,
                         UL_RCC_INFO.rach_ind[j].rach_indication_body.preamble_list[0].preamble_rel8.rnti
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                         ,0
#endif
                        );
        free(UL_RCC_INFO.rach_ind[j].rach_indication_body.preamble_list);
        UL_RCC_INFO.rach_ind[j].rach_indication_body.number_of_preambles = 0;
        UL_RCC_INFO.rach_ind[j].header.message_id = 0;
      }
    }
  } else {
    if (UL_info->rach_ind.rach_indication_body.number_of_preambles>0) {
      AssertFatal(UL_info->rach_ind.rach_indication_body.number_of_preambles==1,"More than 1 preamble not supported\n");
      UL_info->rach_ind.rach_indication_body.number_of_preambles=0;
      LOG_D(MAC,"UL_info[Frame %d, Subframe %d] Calling initiate_ra_proc RACH:SFN/SF:%d\n",UL_info->frame,UL_info->subframe, NFAPI_SFNSF2DEC(UL_info->rach_ind.sfn_sf));
      initiate_ra_proc(UL_info->module_id,
                       UL_info->CC_id,
                       NFAPI_SFNSF2SFN(UL_info->rach_ind.sfn_sf),
                       NFAPI_SFNSF2SF(UL_info->rach_ind.sfn_sf),
                       UL_info->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.preamble,
                       UL_info->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.timing_advance,
                       UL_info->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.rnti
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                       ,0
#endif
                      );
    }
  }

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))

  if (UL_info->rach_ind_br.rach_indication_body.number_of_preambles>0) {
    AssertFatal(UL_info->rach_ind_br.rach_indication_body.number_of_preambles<5,"More than 4 preambles not supported\n");

    for (i=0; i<UL_info->rach_ind_br.rach_indication_body.number_of_preambles; i++) {
      AssertFatal(UL_info->rach_ind_br.rach_indication_body.preamble_list[i].preamble_rel13.rach_resource_type>0,
                  "Got regular PRACH preamble, not BL/CE\n");
      LOG_D(MAC,"Frame %d, Subframe %d Calling initiate_ra_proc (CE_level %d)\n",UL_info->frame,UL_info->subframe,
            UL_info->rach_ind_br.rach_indication_body.preamble_list[i].preamble_rel13.rach_resource_type-1);
      UL_info->rach_ind_br.rach_indication_body.number_of_preambles=0;
      initiate_ra_proc(UL_info->module_id,
                       UL_info->CC_id,
                       UL_info->frame,
                       UL_info->subframe,
                       UL_info->rach_ind_br.rach_indication_body.preamble_list[i].preamble_rel8.preamble,
                       UL_info->rach_ind_br.rach_indication_body.preamble_list[i].preamble_rel8.timing_advance,
                       UL_info->rach_ind_br.rach_indication_body.preamble_list[i].preamble_rel8.rnti,
                       UL_info->rach_ind_br.rach_indication_body.preamble_list[i].preamble_rel13.rach_resource_type);
    }

    UL_info->rach_ind_br.rach_indication_body.number_of_preambles=0;
  }

#endif
}

void handle_sr(UL_IND_t *UL_info) {
  int i;

  if (NFAPI_MODE == NFAPI_MODE_PNF) { // PNF
    if (UL_info->sr_ind.sr_indication_body.number_of_srs>0) {
      oai_nfapi_sr_indication(&UL_info->sr_ind);
    }
  } else if(NFAPI_MODE == NFAPI_MODE_VNF) {
    for(uint8_t j = 0; j < NUM_NFPAI_SUBFRAME; j++) {
      if(UL_RCC_INFO.sr_ind[j].sr_indication_body.number_of_srs > 0) {
        for (i=0; i<UL_RCC_INFO.sr_ind[j].sr_indication_body.number_of_srs; i++) {
          SR_indication(UL_info->module_id,
                        UL_info->CC_id,
                        NFAPI_SFNSF2SFN(UL_RCC_INFO.sr_ind[j].sfn_sf),
                        NFAPI_SFNSF2SF(UL_RCC_INFO.sr_ind[j].sfn_sf),
                        UL_RCC_INFO.sr_ind[j].sr_indication_body.sr_pdu_list[i].rx_ue_information.rnti,
                        UL_RCC_INFO.sr_ind[j].sr_indication_body.sr_pdu_list[i].ul_cqi_information.ul_cqi);
        }

        free(UL_RCC_INFO.sr_ind[j].sr_indication_body.sr_pdu_list);
        UL_RCC_INFO.sr_ind[j].sr_indication_body.number_of_srs=0;
        UL_RCC_INFO.sr_ind[j].header.message_id = 0;
      }
    }
  } else {
    for (i=0; i<UL_info->sr_ind.sr_indication_body.number_of_srs; i++)
      SR_indication(UL_info->module_id,
                    UL_info->CC_id,
                    UL_info->frame,
                    UL_info->subframe,
                    UL_info->sr_ind.sr_indication_body.sr_pdu_list[i].rx_ue_information.rnti,
                    UL_info->sr_ind.sr_indication_body.sr_pdu_list[i].ul_cqi_information.ul_cqi);
  }

  UL_info->sr_ind.sr_indication_body.number_of_srs=0;
}

void handle_cqi(UL_IND_t *UL_info) {
  int i;

  if (NFAPI_MODE==NFAPI_MODE_PNF) {
    if (UL_info->cqi_ind.cqi_indication_body.number_of_cqis>0) {
      LOG_D(PHY,"UL_info->cqi_ind.number_of_cqis:%d\n", UL_info->cqi_ind.cqi_indication_body.number_of_cqis);
      UL_info->cqi_ind.header.message_id = NFAPI_RX_CQI_INDICATION;
      UL_info->cqi_ind.sfn_sf = UL_info->frame<<4 | UL_info->subframe;
      oai_nfapi_cqi_indication(&UL_info->cqi_ind);
      UL_info->cqi_ind.cqi_indication_body.number_of_cqis=0;
    }
  } else if (NFAPI_MODE == NFAPI_MODE_VNF) {
    for(uint8_t j = 0; j < NUM_NFPAI_SUBFRAME; j++) {
      if(UL_RCC_INFO.cqi_ind[j].cqi_indication_body.number_of_cqis > 0) {
        for (i=0; i<UL_RCC_INFO.cqi_ind[j].cqi_indication_body.number_of_cqis; i++) {
          cqi_indication(UL_info->module_id,
                         UL_info->CC_id,
                         NFAPI_SFNSF2SFN(UL_RCC_INFO.cqi_ind[j].sfn_sf),
                         NFAPI_SFNSF2SF(UL_RCC_INFO.cqi_ind[j].sfn_sf),
                         UL_RCC_INFO.cqi_ind[j].cqi_indication_body.cqi_pdu_list[i].rx_ue_information.rnti,
                         &UL_RCC_INFO.cqi_ind[j].cqi_indication_body.cqi_pdu_list[i].cqi_indication_rel9,
                         UL_RCC_INFO.cqi_ind[j].cqi_indication_body.cqi_raw_pdu_list[i].pdu,
                         &UL_RCC_INFO.cqi_ind[j].cqi_indication_body.cqi_pdu_list[i].ul_cqi_information);
        }

        free(UL_RCC_INFO.cqi_ind[j].cqi_indication_body.cqi_pdu_list);
        free(UL_RCC_INFO.cqi_ind[j].cqi_indication_body.cqi_raw_pdu_list);
        UL_RCC_INFO.cqi_ind[j].cqi_indication_body.number_of_cqis=0;
        UL_RCC_INFO.cqi_ind[j].header.message_id = 0;
      }
    }
  } else {
    for (i=0; i<UL_info->cqi_ind.cqi_indication_body.number_of_cqis; i++)
      cqi_indication(UL_info->module_id,
                     UL_info->CC_id,
                     NFAPI_SFNSF2SFN(UL_info->cqi_ind.sfn_sf),
                     NFAPI_SFNSF2SF(UL_info->cqi_ind.sfn_sf),
                     UL_info->cqi_ind.cqi_indication_body.cqi_pdu_list[i].rx_ue_information.rnti,
                     &UL_info->cqi_ind.cqi_indication_body.cqi_pdu_list[i].cqi_indication_rel9,
                     UL_info->cqi_ind.cqi_indication_body.cqi_raw_pdu_list[i].pdu,
                     &UL_info->cqi_ind.cqi_indication_body.cqi_pdu_list[i].ul_cqi_information);

    UL_info->cqi_ind.cqi_indication_body.number_of_cqis=0;
  }
}

void handle_harq(UL_IND_t *UL_info) {
  if (NFAPI_MODE == NFAPI_MODE_PNF && UL_info->harq_ind.harq_indication_body.number_of_harqs > 0) { // PNF
    //LOG_D(PHY, "UL_info->harq_ind.harq_indication_body.number_of_harqs:%d Send to VNF\n", UL_info->harq_ind.harq_indication_body.number_of_harqs);
    int retval = oai_nfapi_harq_indication(&UL_info->harq_ind);

    if (retval != 0) {
      LOG_E(PHY, "Failed to encode NFAPI HARQ_IND retval:%d\n", retval);
    }

    UL_info->harq_ind.harq_indication_body.number_of_harqs = 0;
  } else if(NFAPI_MODE == NFAPI_MODE_VNF) {
    for(uint8_t j = 0; j < NUM_NFPAI_SUBFRAME; j++) {
      if(UL_RCC_INFO.harq_ind[j].harq_indication_body.number_of_harqs > 0) {
        for (int i=0; i<UL_RCC_INFO.harq_ind[j].harq_indication_body.number_of_harqs; i++) {
          harq_indication(UL_info->module_id,
                          UL_info->CC_id,
                          NFAPI_SFNSF2SFN(UL_RCC_INFO.harq_ind[j].sfn_sf),
                          NFAPI_SFNSF2SF(UL_RCC_INFO.harq_ind[j].sfn_sf),
                          &UL_RCC_INFO.harq_ind[j].harq_indication_body.harq_pdu_list[i]);
        }

        free(UL_RCC_INFO.harq_ind[j].harq_indication_body.harq_pdu_list);
        UL_RCC_INFO.harq_ind[j].harq_indication_body.number_of_harqs=0;
        UL_RCC_INFO.harq_ind[j].header.message_id = 0;
      }
    }
  } else {
    for (int i=0; i < UL_info->harq_ind.harq_indication_body.number_of_harqs; i++)
      harq_indication(UL_info->module_id,
                      UL_info->CC_id,
                      NFAPI_SFNSF2SFN(UL_info->harq_ind.sfn_sf),
                      NFAPI_SFNSF2SF(UL_info->harq_ind.sfn_sf),
                      &UL_info->harq_ind.harq_indication_body.harq_pdu_list[i]);

    UL_info->harq_ind.harq_indication_body.number_of_harqs = 0;
  }
}

void handle_ulsch(UL_IND_t *UL_info) {
  int i,j;

  if(NFAPI_MODE == NFAPI_MODE_PNF) {
    if (UL_info->crc_ind.crc_indication_body.number_of_crcs>0) {
      //LOG_D(PHY,"UL_info->crc_ind.crc_indication_body.number_of_crcs:%d CRC_IND:SFN/SF:%d\n", UL_info->crc_ind.crc_indication_body.number_of_crcs, NFAPI_SFNSF2DEC(UL_info->crc_ind.sfn_sf));
      oai_nfapi_crc_indication(&UL_info->crc_ind);
      UL_info->crc_ind.crc_indication_body.number_of_crcs = 0;
    }

    if (UL_info->rx_ind.rx_indication_body.number_of_pdus>0) {
      //LOG_D(PHY,"UL_info->rx_ind.number_of_pdus:%d RX_IND:SFN/SF:%d\n", UL_info->rx_ind.rx_indication_body.number_of_pdus, NFAPI_SFNSF2DEC(UL_info->rx_ind.sfn_sf));
      oai_nfapi_rx_ind(&UL_info->rx_ind);
      UL_info->rx_ind.rx_indication_body.number_of_pdus = 0;
    }
  } else if(NFAPI_MODE == NFAPI_MODE_VNF) {
    for(uint8_t k = 0; k < NUM_NFPAI_SUBFRAME; k++) {
      if((UL_RCC_INFO.rx_ind[k].rx_indication_body.number_of_pdus>0) && (UL_RCC_INFO.crc_ind[k].crc_indication_body.number_of_crcs>0)) {
        for (i=0; i<UL_RCC_INFO.rx_ind[k].rx_indication_body.number_of_pdus; i++) {
          for (j=0; j<UL_RCC_INFO.crc_ind[k].crc_indication_body.number_of_crcs; j++) {
            // find crc_indication j corresponding rx_indication i
            LOG_D(PHY,"UL_info->crc_ind.crc_indication_body.crc_pdu_list[%d].rx_ue_information.rnti:%04x UL_info->rx_ind.rx_indication_body.rx_pdu_list[%d].rx_ue_information.rnti:%04x\n",
                  j,UL_RCC_INFO.crc_ind[k].crc_indication_body.crc_pdu_list[j].rx_ue_information.rnti, i,UL_RCC_INFO.rx_ind[k].rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti);

            if (UL_RCC_INFO.crc_ind[k].crc_indication_body.crc_pdu_list[j].rx_ue_information.rnti == UL_RCC_INFO.rx_ind[k].rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti) {
              LOG_D(PHY, "UL_info->crc_ind.crc_indication_body.crc_pdu_list[%d].crc_indication_rel8.crc_flag:%d\n",
                    j, UL_RCC_INFO.crc_ind[k].crc_indication_body.crc_pdu_list[j].crc_indication_rel8.crc_flag);

              if (UL_RCC_INFO.crc_ind[k].crc_indication_body.crc_pdu_list[j].crc_indication_rel8.crc_flag == 1) { // CRC error indication
                LOG_D(MAC,"Frame %d, Subframe %d Calling rx_sdu (CRC error) \n",UL_info->frame,UL_info->subframe);
                rx_sdu(UL_info->module_id,
                       UL_info->CC_id,
                       NFAPI_SFNSF2SFN(UL_RCC_INFO.rx_ind[k].sfn_sf), //UL_info->frame,
                       NFAPI_SFNSF2SF(UL_RCC_INFO.rx_ind[k].sfn_sf), //UL_info->subframe,
                       UL_RCC_INFO.rx_ind[k].rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti,
                       (uint8_t *)NULL,
                       UL_RCC_INFO.rx_ind[k].rx_indication_body.rx_pdu_list[i].rx_indication_rel8.length,
                       UL_RCC_INFO.rx_ind[k].rx_indication_body.rx_pdu_list[i].rx_indication_rel8.timing_advance,
                       UL_RCC_INFO.rx_ind[k].rx_indication_body.rx_pdu_list[i].rx_indication_rel8.ul_cqi);
              } else {
                LOG_D(MAC,"Frame %d, Subframe %d Calling rx_sdu (CRC ok) \n",UL_info->frame,UL_info->subframe);
                rx_sdu(UL_info->module_id,
                       UL_info->CC_id,
                       NFAPI_SFNSF2SFN(UL_RCC_INFO.rx_ind[k].sfn_sf), //UL_info->frame,
                       NFAPI_SFNSF2SF(UL_RCC_INFO.rx_ind[k].sfn_sf), //UL_info->subframe,
                       UL_RCC_INFO.rx_ind[k].rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti,
                       UL_RCC_INFO.rx_ind[k].rx_indication_body.rx_pdu_list[i].data,
                       UL_RCC_INFO.rx_ind[k].rx_indication_body.rx_pdu_list[i].rx_indication_rel8.length,
                       UL_RCC_INFO.rx_ind[k].rx_indication_body.rx_pdu_list[i].rx_indication_rel8.timing_advance,
                       UL_RCC_INFO.rx_ind[k].rx_indication_body.rx_pdu_list[i].rx_indication_rel8.ul_cqi);
              }

              if(UL_RCC_INFO.rx_ind[k].rx_indication_body.rx_pdu_list[i].data != NULL) {
                free(UL_RCC_INFO.rx_ind[k].rx_indication_body.rx_pdu_list[i].data);
              }

              break;
            } //if (UL_info->crc_ind.crc_pdu_list[j].rx_ue_information.rnti == UL_info->rx_ind.rx_pdu_list[i].rx_ue_information.rnti)
          } //    for (j=0;j<UL_info->crc_ind.crc_indication_body.number_of_crcs;j++)
        } //   for (i=0;i<UL_info->rx_ind.number_of_pdus;i++)

        free(UL_RCC_INFO.crc_ind[k].crc_indication_body.crc_pdu_list);
        free(UL_RCC_INFO.rx_ind[k].rx_indication_body.rx_pdu_list);
        UL_RCC_INFO.crc_ind[k].crc_indication_body.number_of_crcs = 0;
        UL_RCC_INFO.crc_ind[k].header.message_id =0;
        UL_RCC_INFO.rx_ind[k].rx_indication_body.number_of_pdus = 0;
        UL_RCC_INFO.rx_ind[k].header.message_id = 0;
      }
    }
  } else {
    if (UL_info->rx_ind.rx_indication_body.number_of_pdus>0 && UL_info->crc_ind.crc_indication_body.number_of_crcs>0) {
      for (i=0; i<UL_info->rx_ind.rx_indication_body.number_of_pdus; i++) {
        for (j=0; j<UL_info->crc_ind.crc_indication_body.number_of_crcs; j++) {
          // find crc_indication j corresponding rx_indication i
          LOG_D(PHY,"UL_info->crc_ind.crc_indication_body.crc_pdu_list[%d].rx_ue_information.rnti:%04x UL_info->rx_ind.rx_indication_body.rx_pdu_list[%d].rx_ue_information.rnti:%04x\n", j,
                UL_info->crc_ind.crc_indication_body.crc_pdu_list[j].rx_ue_information.rnti, i, UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti);

          if (UL_info->crc_ind.crc_indication_body.crc_pdu_list[j].rx_ue_information.rnti ==
              UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti) {
            LOG_D(PHY, "UL_info->crc_ind.crc_indication_body.crc_pdu_list[%d].crc_indication_rel8.crc_flag:%d\n", j, UL_info->crc_ind.crc_indication_body.crc_pdu_list[j].crc_indication_rel8.crc_flag);

            if (UL_info->crc_ind.crc_indication_body.crc_pdu_list[j].crc_indication_rel8.crc_flag == 1) { // CRC error indication
              LOG_D(MAC,"Frame %d, Subframe %d Calling rx_sdu (CRC error) \n",UL_info->frame,UL_info->subframe);
              rx_sdu(UL_info->module_id,
                     UL_info->CC_id,
                     NFAPI_SFNSF2SFN(UL_info->rx_ind.sfn_sf), //UL_info->frame,
                     NFAPI_SFNSF2SF(UL_info->rx_ind.sfn_sf), //UL_info->subframe,
                     UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti,
                     (uint8_t *)NULL,
                     UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.length,
                     UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.timing_advance,
                     UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.ul_cqi);
            } else {
              LOG_D(MAC,"Frame %d, Subframe %d Calling rx_sdu (CRC ok) \n",UL_info->frame,UL_info->subframe);
              rx_sdu(UL_info->module_id,
                     UL_info->CC_id,
                     NFAPI_SFNSF2SFN(UL_info->rx_ind.sfn_sf), //UL_info->frame,
                     NFAPI_SFNSF2SF(UL_info->rx_ind.sfn_sf), //UL_info->subframe,
                     UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti,
                     UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].data,
                     UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.length,
                     UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.timing_advance,
                     UL_info->rx_ind.rx_indication_body.rx_pdu_list[i].rx_indication_rel8.ul_cqi);
            }

            break;
          } //if (UL_info->crc_ind.crc_pdu_list[j].rx_ue_information.rnti ==

          //    UL_info->rx_ind.rx_pdu_list[i].rx_ue_information.rnti)
        } //    for (j=0;j<UL_info->crc_ind.crc_indication_body.number_of_crcs;j++)
      } //   for (i=0;i<UL_info->rx_ind.number_of_pdus;i++)

      UL_info->crc_ind.crc_indication_body.number_of_crcs=0;
      UL_info->rx_ind.rx_indication_body.number_of_pdus = 0;
    } // UL_info->rx_ind.rx_indication_body.number_of_pdus>0 && UL_info->subframe && UL_info->crc_ind.crc_indication_body.number_of_crcs>0
    else if (UL_info->rx_ind.rx_indication_body.number_of_pdus!=0 || UL_info->crc_ind.crc_indication_body.number_of_crcs!=0) {
      LOG_E(PHY,"hoping not to have mis-match between CRC ind and RX ind - hopefully the missing message is coming shortly rx_ind:%d(SFN/SF:%05d) crc_ind:%d(SFN/SF:%05d) UL_info(SFN/SF):%04d%d\n",
            UL_info->rx_ind.rx_indication_body.number_of_pdus, NFAPI_SFNSF2DEC(UL_info->rx_ind.sfn_sf),
            UL_info->crc_ind.crc_indication_body.number_of_crcs, NFAPI_SFNSF2DEC(UL_info->crc_ind.sfn_sf),
            UL_info->frame, UL_info->subframe);
    }
  }
}

/****************************************************************************/
/* debug utility functions begin                                            */
/****************************************************************************/

//#define DUMP_FAPI

#ifdef DUMP_FAPI

#define C do { size = 0; put(0); } while (0)
#define A(...) do { char t[4096]; sprintf(t, __VA_ARGS__); append_string(t); } while (0)

/* eats nothing at startup, but fixed size */
#define SMAX 65536
static char s[SMAX];
static int size;
static int maxsize = SMAX;

static void put(char x) {
  if (size == maxsize) {
    printf("incrase SMAX\n");
    exit(1);
  }

  s[size++] = x;
}

static void append_string(char *t) {
  size--;

  while (*t) put(*t++);

  put(0);
}

static void dump_ul(UL_IND_t *u) {
  int i;
  C;
  A("XXXX UL  mod %d CC %d f.sf %d.%d\n",
    u->module_id, u->CC_id, u->frame, u->subframe);
  A("XXXX     harq_ind %d\n", u->harq_ind.harq_indication_body.number_of_harqs);

  for (i = 0; i < u->harq_ind.harq_indication_body.number_of_harqs; i++) {
    nfapi_harq_indication_pdu_t *v = &u->harq_ind.harq_indication_body.harq_pdu_list[i];
    A("XXXX         harq ind %d\n", i);
    A("XXXX         rnti %d\n", v->rx_ue_information.rnti);
    A("XXXX         tb1 %d tb2 %d\n", v->harq_indication_fdd_rel8.harq_tb1,
      v->harq_indication_fdd_rel8.harq_tb2);
    A("XXXX         number_of_ack_nack %d\n",
      v->harq_indication_fdd_rel9.number_of_ack_nack);
    A("XXXX             harq[0] = %d\n",
      v->harq_indication_fdd_rel9.harq_tb_n[0]);
    A("XXXX harq        ul_cqi %d channel %d\n", v->ul_cqi_information.ul_cqi,
      v->ul_cqi_information.channel);
  }

  A("XXXX     crc_ind  %d\n", u->crc_ind.crc_indication_body.number_of_crcs);
  A("XXXX     sr_ind   %d\n", u->sr_ind.sr_indication_body.number_of_srs);
  A("XXXX     cqi_ind  %d\n", u->cqi_ind.cqi_indication_body.number_of_cqis);

  for (i = 0; i < u->cqi_ind.cqi_indication_body.number_of_cqis; i++) {
    nfapi_cqi_indication_pdu_t *v = &u->cqi_ind.cqi_indication_body.cqi_pdu_list[i];
    A("XXXX         cqi ind %d\n", i);
    A("XXXX cqi         ul_cqi %d channel %d\n", v->ul_cqi_information.ul_cqi,
      v->ul_cqi_information.channel);
  }

  A("XXXX     rach_ind %d\n", u->rach_ind.rach_indication_body.number_of_preambles);
  A("XXXX     rx_ind   %d\n", u->rx_ind.rx_indication_body.number_of_pdus);

  for (i = 0; i < u->rx_ind.rx_indication_body.number_of_pdus; i++) {
    nfapi_rx_indication_pdu_t *v = &u->rx_ind.rx_indication_body.rx_pdu_list[i];
    A("XXXX         rx ind %d\n", i);
    A("XXXX             timing_advance %d\n",
      v->rx_indication_rel8.timing_advance);
    A("XXXX rx          ul_cqi %d\n", v->rx_indication_rel8.ul_cqi);
  }

  LOG_I(PHY, "XXXX UL\nXXXX UL\n%s", s);
}

static char *DL_PDU_TYPE(int x) {
  switch (x) {
    case NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE:
      return "NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE";

    case NFAPI_DL_CONFIG_BCH_PDU_TYPE:
      return "NFAPI_DL_CONFIG_BCH_PDU_TYPE";

    case NFAPI_DL_CONFIG_MCH_PDU_TYPE:
      return "NFAPI_DL_CONFIG_MCH_PDU_TYPE";

    case NFAPI_DL_CONFIG_DLSCH_PDU_TYPE:
      return "NFAPI_DL_CONFIG_DLSCH_PDU_TYPE";

    case NFAPI_DL_CONFIG_PCH_PDU_TYPE:
      return "NFAPI_DL_CONFIG_PCH_PDU_TYPE";

    case NFAPI_DL_CONFIG_PRS_PDU_TYPE:
      return "NFAPI_DL_CONFIG_PRS_PDU_TYPE";

    case NFAPI_DL_CONFIG_CSI_RS_PDU_TYPE:
      return "NFAPI_DL_CONFIG_CSI_RS_PDU_TYPE";

    case NFAPI_DL_CONFIG_EPDCCH_DL_PDU_TYPE:
      return "NFAPI_DL_CONFIG_EPDCCH_DL_PDU_TYPE";

    case NFAPI_DL_CONFIG_MPDCCH_PDU_TYPE:
      return "NFAPI_DL_CONFIG_MPDCCH_PDU_TYPE";

    case NFAPI_DL_CONFIG_NBCH_PDU_TYPE:
      return "NFAPI_DL_CONFIG_NBCH_PDU_TYPE";

    case NFAPI_DL_CONFIG_NPDCCH_PDU_TYPE:
      return "NFAPI_DL_CONFIG_NPDCCH_PDU_TYPE";

    case NFAPI_DL_CONFIG_NDLSCH_PDU_TYPE:
      return "NFAPI_DL_CONFIG_NDLSCH_PDU_TYPE";
  }

  return "UNKNOWN";
}

static char *UL_PDU_TYPE(int x) {
  switch (x) {
    case NFAPI_UL_CONFIG_ULSCH_PDU_TYPE:
      return "NFAPI_UL_CONFIG_ULSCH_PDU_TYPE";

    case NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE:
      return "NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE";

    case NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE:
      return "NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE";

    case NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE:
      return "NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE";

    case NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE:
      return "NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE";

    case NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE:
      return "NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE";

    case NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE:
      return "NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE";

    case NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE:
      return "NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE";

    case NFAPI_UL_CONFIG_UCI_CQI_HARQ_PDU_TYPE:
      return "NFAPI_UL_CONFIG_UCI_CQI_HARQ_PDU_TYPE";

    case NFAPI_UL_CONFIG_UCI_CQI_SR_PDU_TYPE:
      return "NFAPI_UL_CONFIG_UCI_CQI_SR_PDU_TYPE";

    case NFAPI_UL_CONFIG_UCI_CQI_SR_HARQ_PDU_TYPE:
      return "NFAPI_UL_CONFIG_UCI_CQI_SR_HARQ_PDU_TYPE";

    case NFAPI_UL_CONFIG_SRS_PDU_TYPE:
      return "NFAPI_UL_CONFIG_SRS_PDU_TYPE";

    case NFAPI_UL_CONFIG_HARQ_BUFFER_PDU_TYPE:
      return "NFAPI_UL_CONFIG_HARQ_BUFFER_PDU_TYPE";

    case NFAPI_UL_CONFIG_ULSCH_UCI_CSI_PDU_TYPE:
      return "NFAPI_UL_CONFIG_ULSCH_UCI_CSI_PDU_TYPE";

    case NFAPI_UL_CONFIG_ULSCH_UCI_HARQ_PDU_TYPE:
      return "NFAPI_UL_CONFIG_ULSCH_UCI_HARQ_PDU_TYPE";

    case NFAPI_UL_CONFIG_ULSCH_CSI_UCI_HARQ_PDU_TYPE:
      return "NFAPI_UL_CONFIG_ULSCH_CSI_UCI_HARQ_PDU_TYPE";

    case NFAPI_UL_CONFIG_NULSCH_PDU_TYPE:
      return "NFAPI_UL_CONFIG_NULSCH_PDU_TYPE";

    case NFAPI_UL_CONFIG_NRACH_PDU_TYPE:
      return "NFAPI_UL_CONFIG_NRACH_PDU_TYPE";
  }

  return "UNKNOWN";
}

static char *HI_DCI0_PDU_TYPE(int x) {
  switch (x) {
    case NFAPI_HI_DCI0_HI_PDU_TYPE:
      return "NFAPI_HI_DCI0_HI_PDU_TYPE";

    case NFAPI_HI_DCI0_DCI_PDU_TYPE:
      return "NFAPI_HI_DCI0_DCI_PDU_TYPE";

    case NFAPI_HI_DCI0_EPDCCH_DCI_PDU_TYPE:
      return "NFAPI_HI_DCI0_EPDCCH_DCI_PDU_TYPE";

    case NFAPI_HI_DCI0_MPDCCH_DCI_PDU_TYPE:
      return "NFAPI_HI_DCI0_MPDCCH_DCI_PDU_TYPE";

    case NFAPI_HI_DCI0_NPDCCH_DCI_PDU_TYPE:
      return "NFAPI_HI_DCI0_NPDCCH_DCI_PDU_TYPE";
  }

  return "UNKNOWN";
}

static void dump_dl(Sched_Rsp_t *d) {
  int i;
  C;
  A("XXXX DL  mod %d CC %d f.sf %d.%d\n",
    d->module_id, d->CC_id, d->frame, d->subframe);

  if (d->DL_req != NULL) {
    nfapi_dl_config_request_body_t *v=&d->DL_req->dl_config_request_body;
    nfapi_dl_config_request_pdu_t *p = v->dl_config_pdu_list;
    A("XXXX     DL_req sfnsf %d\n", d->DL_req->sfn_sf);
    A("XXXX     PDCCH size   %d\n", v->number_pdcch_ofdm_symbols);
    A("XXXX     DCIs         %d\n", v->number_dci);
    A("XXXX     PDUs         %d\n", v->number_pdu);
    A("XXXX     rntis        %d\n", v->number_pdsch_rnti);
    A("XXXX     pcfich power %d\n", v->transmission_power_pcfich);

    for (i = 0; i < v->number_pdu; i++) {
      A("XXXX         pdu %d\n", i);
      A("XXXX             type %d %s\n", p[i].pdu_type, DL_PDU_TYPE(p[i].pdu_type));

      switch (p[i].pdu_type) {
        case NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE: {
          nfapi_dl_config_dci_dl_pdu_rel8_t *q =
            &p[i].dci_dl_pdu.dci_dl_pdu_rel8;
          A("XXXX                 dci format %d\n", q->dci_format);
          A("XXXX                 cce idx    %d\n", q->cce_idx);
          A("XXXX                 agg lvl    %d\n", q->aggregation_level);
          A("XXXX                 rnti       %d\n", q->rnti);
          A("XXXX                 rb coding  %8.8x\n", q->resource_block_coding);
          A("XXXX                 mcs_1      %d\n", q->mcs_1);
          A("XXXX                 rv_1       %d\n", q->redundancy_version_1);
          A("XXXX                 ndi_1      %d\n", q->new_data_indicator_1);
          A("XXXX                 harq pid   %d\n", q->harq_process);
          A("XXXX                 tpc        %d\n", q->tpc);
          A("XXXX                 tbs idx    %d\n", q->transport_block_size_index);
          A("XXXX                 dl pow off %d\n", q->downlink_power_offset);
          A("XXXX                 rnti type  %d\n", q->rnti_type);
          A("XXXX                 xmit pow   %d\n", q->transmission_power);
          break;
        }

        case NFAPI_DL_CONFIG_DLSCH_PDU_TYPE: {
          nfapi_dl_config_dlsch_pdu_rel8_t *q =
            &p[i].dlsch_pdu.dlsch_pdu_rel8;
          A("XXXX                 pdu_index %d\n", q->pdu_index);
          A("XXXX                 rnti      %d\n", q->rnti);
          A("XXXX                 rv        %d\n", q->redundancy_version);
          A("XXXX                 mcs       %d\n", q->modulation);
          A("XXXX                 pa        %d\n", q->pa);
          break;
        }
      }
    }
  }

  if (d->HI_DCI0_req != NULL) {
    nfapi_hi_dci0_request_body_t *v=&d->HI_DCI0_req->hi_dci0_request_body;
    A("XXXX up  HI_DCI0_req sfnsf %d (%d.%d)\n", d->HI_DCI0_req->sfn_sf,
      d->HI_DCI0_req->sfn_sf/16, d->HI_DCI0_req->sfn_sf%16);
    A("XXXX up     sfnsf %d\n", v->sfnsf);
    A("XXXX up     DCIs  %d\n", v->number_of_dci);
    A("XXXX up     HIs   %d\n", v->number_of_hi);

    for (i = 0; i < v->number_of_dci + v->number_of_hi; i++) {
      nfapi_hi_dci0_request_pdu_t *p = &v->hi_dci0_pdu_list[i];
      A("XXXX up      pdu %d\n", i);
      A("XXXX up          type %d %s\n",p->pdu_type,HI_DCI0_PDU_TYPE(p->pdu_type));

      if (p->pdu_type == NFAPI_HI_DCI0_DCI_PDU_TYPE) {
        nfapi_hi_dci0_dci_pdu_rel8_t *q = &p->dci_pdu.dci_pdu_rel8;
        A("XXXX up          dci_format           %d\n", q->dci_format);
        A("XXXX up          cce_index            %d\n", q->cce_index);
        A("XXXX up          aggregation_level    %d\n", q->aggregation_level);
        A("XXXX up          rnti                 %d\n", q->rnti);
        A("XXXX up          rb start             %d\n", q->resource_block_start);
        A("XXXX up          # rb                 %d\n", q->number_of_resource_block);
        A("XXXX up          mcs_1                %d\n", q->mcs_1);
        A("XXXX up          cshift_2_for_drms    %d\n", q->cyclic_shift_2_for_drms);
        A("XXXX up          freq hop enabled     %d\n", q->frequency_hopping_enabled_flag);
        A("XXXX up          fre hop bits         %d\n", q->frequency_hopping_bits);
        A("XXXX up          NDI_1                %d\n", q->new_data_indication_1);
        A("XXXX up          tx_antenna_seleciton %d\n", q->ue_tx_antenna_seleciton);
        A("XXXX up          tpc                  %d\n", q->tpc);
        A("XXXX up          cqi_csi_request      %d\n", q->cqi_csi_request);
        A("XXXX up          ul_index             %d\n", q->ul_index);
        A("XXXX up          dl_assignment_index  %d\n", q->dl_assignment_index);
        A("XXXX up          tpc_bitmap           %d\n", q->tpc_bitmap);
        A("XXXX up          transmission_power   %d\n", q->transmission_power);
      }

      if (p->pdu_type == NFAPI_HI_DCI0_HI_PDU_TYPE) {
        nfapi_hi_dci0_hi_pdu_rel8_t *q = &p->hi_pdu.hi_pdu_rel8;
        A("XXXX up          rb start    %d\n", q->resource_block_start);
        A("XXXX up          cs2_drms    %d\n", q->cyclic_shift_2_for_drms);
        A("XXXX up          ack         %d\n", q->hi_value);
        A("XXXX up          i_phich     %d\n", q->i_phich);
        A("XXXX up          power       %d\n", q->transmission_power);
      }
    }
  }

  if (d->UL_req != NULL) {
    nfapi_ul_config_request_body_t *v=&d->UL_req->ul_config_request_body;
    A("XXXX     UL_req sfnsf %d (%d.%d)\n", d->UL_req->sfn_sf,
      d->UL_req->sfn_sf/16, d->UL_req->sfn_sf%16);
    A("XXXX     PDUs         %d\n", v->number_of_pdus);
    A("XXXX     ra freq      %d\n", v->rach_prach_frequency_resources);
    A("XXXX     srs?         %d\n", v->srs_present);

    for (i = 0; i < v->number_of_pdus; i++) {
      nfapi_ul_config_request_pdu_t *p = &v->ul_config_pdu_list[i];
      A("XXXX         pdu %d\n", i);
      A("XXXX             type %d %s\n", p->pdu_type, UL_PDU_TYPE(p->pdu_type));

      switch(p->pdu_type) {
        case NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE: {
          nfapi_ul_config_uci_harq_pdu *q = &p->uci_harq_pdu;
          nfapi_ul_config_harq_information_rel9_fdd_t *h =
            &q->harq_information.harq_information_rel9_fdd;
          A("XXXX                 rnti          %d\n",
            q->ue_information.ue_information_rel8.rnti);
          A("XXXX                 harq size     %d\n", h->harq_size);
          A("XXXX                 ack_nack_mode %d\n", h->ack_nack_mode);
          A("XXXX                 # pucch res   %d\n", h->number_of_pucch_resources);
          A("XXXX                 n_pucch_1_0   %d\n", h->n_pucch_1_0);
          A("XXXX                 n_pucch_1_1   %d\n", h->n_pucch_1_1);
          A("XXXX                 n_pucch_1_2   %d\n", h->n_pucch_1_2);
          A("XXXX                 n_pucch_1_3   %d\n", h->n_pucch_1_3);
          break;
        }

        case NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE: {
          nfapi_ul_config_uci_sr_pdu *q = &p->uci_sr_pdu;
          nfapi_ul_config_sr_information_rel8_t *h =
            &q->sr_information.sr_information_rel8;
          A("XXXX                 rnti          %d\n",
            q->ue_information.ue_information_rel8.rnti);
          A("XXXX                 pucch_index   %d\n", h->pucch_index);
        }
      }
    }
  }

  LOG_I(PHY, "XXXX DL\nXXXX DL\n%s", s);
}

#undef C
#undef A

#endif /* DUMP_FAPI */

/****************************************************************************/
/* debug utility functions end                                              */
/****************************************************************************/

void UL_indication(UL_IND_t *UL_info) {
  AssertFatal(UL_info!=NULL,"UL_INFO is null\n");
#ifdef DUMP_FAPI
  dump_ul(UL_info);
#endif
  module_id_t  module_id   = UL_info->module_id;
  int          CC_id       = UL_info->CC_id;
  Sched_Rsp_t  *sched_info = &Sched_INFO[module_id][CC_id];
  IF_Module_t  *ifi        = if_inst[module_id];
  eNB_MAC_INST *mac        = RC.mac[module_id];
  LOG_D(PHY,"SFN/SF:%d%d module_id:%d CC_id:%d UL_info[rx_ind:%d harqs:%d crcs:%d cqis:%d preambles:%d sr_ind:%d]\n",
        UL_info->frame,UL_info->subframe,
        module_id,CC_id,
        UL_info->rx_ind.rx_indication_body.number_of_pdus, UL_info->harq_ind.harq_indication_body.number_of_harqs, UL_info->crc_ind.crc_indication_body.number_of_crcs,
        UL_info->cqi_ind.cqi_indication_body.number_of_cqis, UL_info->rach_ind.rach_indication_body.number_of_preambles, UL_info->sr_ind.sr_indication_body.number_of_srs);

  if(UL_info->frame==1023&&UL_info->subframe==6) { // dl scheduling (0,0)
    frame_cnt= (frame_cnt + 1)%7; // to prevent frame_cnt get too big
    LOG_D(MAC,"current (%d,%d) frame count dl is %d\n",UL_info->frame,UL_info->subframe,frame_cnt);
  }

  if (NFAPI_MODE != NFAPI_MODE_PNF) {
    if (ifi->CC_mask==0) {
      ifi->current_frame    = UL_info->frame;
      ifi->current_subframe = UL_info->subframe;
    } else {
      AssertFatal(UL_info->frame != ifi->current_frame,"CC_mask %x is not full and frame has changed\n",ifi->CC_mask);
      AssertFatal(UL_info->subframe != ifi->current_subframe,"CC_mask %x is not full and subframe has changed\n",ifi->CC_mask);
    }

    ifi->CC_mask |= (1<<CC_id);
  }

  // clear DL/UL info for new scheduling round
  clear_nfapi_information(RC.mac[module_id],CC_id,
                          UL_info->frame,UL_info->subframe);
  handle_rach(UL_info);
  handle_sr(UL_info);
  handle_cqi(UL_info);
  handle_harq(UL_info);
  // clear HI prior to handling ULSCH
  uint8_t sf_ahead_dl = ul_subframe2_k_phich(&mac->common_channels[CC_id], UL_info->subframe);

  if(sf_ahead_dl!=255) {
    mac->HI_DCI0_req[CC_id][(UL_info->subframe+sf_ahead_dl)%10].hi_dci0_request_body.number_of_hi                     = 0;
    LOG_D(MAC,"current (%d,%d) clear HI_DCI0_req[0][%d]\n",UL_info->frame,UL_info->subframe,(UL_info->subframe+sf_ahead_dl)%10);
  }

  handle_ulsch(UL_info);

  if (NFAPI_MODE != NFAPI_MODE_PNF) {
    if (ifi->CC_mask == ((1<<MAX_NUM_CCs)-1)) {
      eNB_dlsch_ulsch_scheduler(module_id,
                                (UL_info->frame+((UL_info->subframe>(9-sf_ahead))?1:0)) % 1024,
                                (UL_info->subframe+sf_ahead)%10);
      ifi->CC_mask            = 0;
      sched_info->module_id   = module_id;
      sched_info->CC_id       = CC_id;
      sched_info->frame       = (UL_info->frame + ((UL_info->subframe>(9-sf_ahead)) ? 1 : 0)) % 1024;
      sched_info->subframe    = (UL_info->subframe+sf_ahead)%10;
      sched_info->DL_req      = &mac->DL_req[CC_id];
      sched_info->HI_DCI0_req = &mac->HI_DCI0_req[CC_id][sched_info->subframe];

      if ((mac->common_channels[CC_id].tdd_Config==NULL) ||
          (is_UL_sf(&mac->common_channels[CC_id],sched_info->subframe)>0))
        sched_info->UL_req      = &mac->UL_req[CC_id];
      else
        sched_info->UL_req      = NULL;

      sched_info->TX_req      = &mac->TX_req[CC_id];
      pthread_mutex_lock(&lock_ue_freelist);
      sched_info->UE_release_req = &mac->UE_release_req;
      pthread_mutex_unlock(&lock_ue_freelist);
#ifdef DUMP_FAPI
      dump_dl(sched_info);
#endif

      if (ifi->schedule_response) {
        AssertFatal(ifi->schedule_response!=NULL,
                    "schedule_response is null (mod %d, cc %d)\n",
                    module_id,
                    CC_id);
        ifi->schedule_response(sched_info);
      }

      LOG_D(PHY,"Schedule_response: SFN_SF:%d%d dl_pdus:%d\n",sched_info->frame,sched_info->subframe,sched_info->DL_req->dl_config_request_body.number_pdu);
    }
  }
}

IF_Module_t *IF_Module_init(int Mod_id) {
  AssertFatal(Mod_id<MAX_MODULES,"Asking for Module %d > %d\n",Mod_id,MAX_IF_MODULES);
  LOG_D(PHY,"Installing callbacks for IF_Module - UL_indication\n");

  if (if_inst[Mod_id]==NULL) {
    if_inst[Mod_id] = (IF_Module_t *)malloc(sizeof(IF_Module_t));
    memset((void *)if_inst[Mod_id],0,sizeof(IF_Module_t));
    if_inst[Mod_id]->CC_mask=0;
    if_inst[Mod_id]->UL_indication = UL_indication;
    AssertFatal(pthread_mutex_init(&if_inst[Mod_id]->if_mutex,NULL)==0,
                "allocation of if_inst[%d]->if_mutex fails\n",Mod_id);
  }

  return if_inst[Mod_id];
}

void IF_Module_kill(int Mod_id) {
  AssertFatal(Mod_id>MAX_MODULES,"Asking for Module %d > %d\n",Mod_id,MAX_IF_MODULES);

  if (if_inst[Mod_id]!=NULL) free(if_inst[Mod_id]);
}
