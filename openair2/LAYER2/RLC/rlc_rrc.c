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

/*
                                rlc_rrc.c
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier at eurecom dot fr
*/

#define RLC_RRC_C
#include "rlc.h"
#include "rlc_am.h"
#include "rlc_um.h"
#include "rlc_tm.h"
#include "common/utils/LOG/log.h"
#include "LTE_RLC-Config.h"
#include "LTE_DRB-ToAddMod.h"
#include "LTE_DRB-ToAddModList.h"
#include "LTE_SRB-ToAddMod.h"
#include "LTE_SRB-ToAddModList.h"
#include "LTE_DL-UM-RLC.h"
#if (LTE_RRC_VERSION >= MAKE_VERSION(9, 0, 0))
#include "LTE_PMCH-InfoList-r9.h"
#endif

#include "LAYER2/MAC/mac_extern.h"
#include "assertions.h"
//-----------------------------------------------------------------------------
rlc_op_status_t rrc_rlc_config_asn1_req (const protocol_ctxt_t   * const ctxt_pP,
    const LTE_SRB_ToAddModList_t   * const srb2add_listP,
    const LTE_DRB_ToAddModList_t   * const drb2add_listP,
    const LTE_DRB_ToReleaseList_t  * const drb2release_listP
#if (LTE_RRC_VERSION >= MAKE_VERSION(9, 0, 0))
    ,const LTE_PMCH_InfoList_r9_t * const pmch_InfoList_r9_pP
    ,const uint32_t sourceL2Id
    ,const uint32_t destinationL2Id
#endif
                                        )
{
  //-----------------------------------------------------------------------------
  rb_id_t                rb_id           = 0;
  logical_chan_id_t      lc_id           = 0;
  LTE_DRB_Identity_t     drb_id          = 0;
  LTE_DRB_Identity_t*    pdrb_id         = NULL;
  long int               cnt             = 0;
  const LTE_SRB_ToAddMod_t  *srb_toaddmod_p  = NULL;
  const LTE_DRB_ToAddMod_t  *drb_toaddmod_p  = NULL;
  rlc_union_t           *rlc_union_p     = NULL;
  hash_key_t             key             = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t         h_rc;
#if (LTE_RRC_VERSION >= MAKE_VERSION(9, 0, 0))
  int                        i, j;
  LTE_MBMS_SessionInfoList_r9_t *mbms_SessionInfoList_r9_p = NULL;
  LTE_MBMS_SessionInfo_r9_t     *MBMS_SessionInfo_p        = NULL;
  mbms_session_id_t          mbms_session_id;
  mbms_service_id_t          mbms_service_id;
  LTE_DL_UM_RLC_t                dl_um_rlc;


#endif

  /* for no gcc warnings */
  (void)rlc_union_p;
  (void)key;
  (void)h_rc;

  LOG_D(RLC, PROTOCOL_CTXT_FMT" CONFIG REQ ASN1 \n",
        PROTOCOL_CTXT_ARGS(ctxt_pP));

  if (srb2add_listP != NULL) {
    for (cnt=0; cnt<srb2add_listP->list.count; cnt++) {
      rb_id = srb2add_listP->list.array[cnt]->srb_Identity;
      lc_id = rb_id;

      LOG_D(RLC, "Adding SRB %ld, rb_id %d\n",srb2add_listP->list.array[cnt]->srb_Identity,rb_id);
      srb_toaddmod_p = srb2add_listP->list.array[cnt];

      if (srb_toaddmod_p->rlc_Config) {
        switch (srb_toaddmod_p->rlc_Config->present) {
        case LTE_SRB_ToAddMod__rlc_Config_PR_NOTHING:
          break;

        case LTE_SRB_ToAddMod__rlc_Config_PR_explicitValue:
          switch (srb_toaddmod_p->rlc_Config->choice.explicitValue.present) {
          case LTE_RLC_Config_PR_NOTHING:
            break;

          case LTE_RLC_Config_PR_am:
            if (rrc_rlc_add_rlc (ctxt_pP, SRB_FLAG_YES, MBMS_FLAG_NO, rb_id, lc_id, RLC_MODE_AM
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                                ,0,
                                0
#endif
                  ) != NULL) {
              config_req_rlc_am_asn1 (
                ctxt_pP,
                SRB_FLAG_YES,
                &srb_toaddmod_p->rlc_Config->choice.explicitValue.choice.am,
                rb_id, lc_id);
            } else {
              LOG_E(RLC, PROTOCOL_CTXT_FMT" ERROR IN ALLOCATING SRB %d \n",
                    PROTOCOL_CTXT_ARGS(ctxt_pP),
                    rb_id);
            }

            break;

          case LTE_RLC_Config_PR_um_Bi_Directional:
            if (rrc_rlc_add_rlc (ctxt_pP, SRB_FLAG_YES, MBMS_FLAG_NO, rb_id, lc_id, RLC_MODE_UM
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                                ,0,
                                0
#endif
            ) != NULL) {
              config_req_rlc_um_asn1(
                ctxt_pP,
                SRB_FLAG_YES,
                MBMS_FLAG_NO,
                UNUSED_PARAM_MBMS_SESSION_ID,
                UNUSED_PARAM_MBMS_SERVICE_ID,
                &srb_toaddmod_p->rlc_Config->choice.explicitValue.choice.um_Bi_Directional.ul_UM_RLC,
                &srb_toaddmod_p->rlc_Config->choice.explicitValue.choice.um_Bi_Directional.dl_UM_RLC,
                rb_id, lc_id
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
               ,0, 0
#endif
               );
            } else {
              LOG_E(RLC, PROTOCOL_CTXT_FMT" ERROR IN ALLOCATING SRB %d \n",
                    PROTOCOL_CTXT_ARGS(ctxt_pP),
                    rb_id);
            }

            break;

          case LTE_RLC_Config_PR_um_Uni_Directional_UL:
            if (rrc_rlc_add_rlc (ctxt_pP, SRB_FLAG_YES, MBMS_FLAG_NO, rb_id, lc_id, RLC_MODE_UM
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                                 ,0,
                                 0
#endif
                  ) != NULL) {
              config_req_rlc_um_asn1(
                ctxt_pP,
                SRB_FLAG_YES,
                MBMS_FLAG_NO,
                UNUSED_PARAM_MBMS_SESSION_ID,
                UNUSED_PARAM_MBMS_SERVICE_ID,
                &srb_toaddmod_p->rlc_Config->choice.explicitValue.choice.um_Uni_Directional_UL.ul_UM_RLC,
                NULL,
                rb_id, lc_id
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
               ,0, 0
#endif
               );
            } else {
              LOG_E(RLC, PROTOCOL_CTXT_FMT" ERROR IN ALLOCATING SRB %d \n",
                    PROTOCOL_CTXT_ARGS(ctxt_pP),
                    rb_id);
            }

            break;

          case LTE_RLC_Config_PR_um_Uni_Directional_DL:
            if (rrc_rlc_add_rlc (ctxt_pP, SRB_FLAG_YES, MBMS_FLAG_NO, rb_id, lc_id, RLC_MODE_UM
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                                 ,0,
                                 0
#endif
                                 ) != NULL) {
              config_req_rlc_um_asn1(
                ctxt_pP,
                SRB_FLAG_YES,
                MBMS_FLAG_NO,
                UNUSED_PARAM_MBMS_SESSION_ID,
                UNUSED_PARAM_MBMS_SERVICE_ID,
                NULL,
                &srb_toaddmod_p->rlc_Config->choice.explicitValue.choice.um_Uni_Directional_DL.dl_UM_RLC,
                rb_id, lc_id
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
               ,0, 0
#endif
               );
            } else {
              LOG_E(RLC, PROTOCOL_CTXT_FMT" ERROR IN ALLOCATING SRB %d \n",
                    PROTOCOL_CTXT_ARGS(ctxt_pP),
                    rb_id);
            }

            break;

          default:
            LOG_E(RLC, PROTOCOL_CTXT_FMT" UNKNOWN RLC CONFIG %d \n",
                  PROTOCOL_CTXT_ARGS(ctxt_pP),
                  srb_toaddmod_p->rlc_Config->choice.explicitValue.present);
            break;
          }

          break;

        case LTE_SRB_ToAddMod__rlc_Config_PR_defaultValue:
//#warning TO DO SRB_ToAddMod__rlc_Config_PR_defaultValue
          LOG_I(RRC, "RLC SRB1 is default value !!\n");
          struct LTE_RLC_Config__am  *  config_am_pP = &srb_toaddmod_p->rlc_Config->choice.explicitValue.choice.am;
          config_am_pP->dl_AM_RLC.t_Reordering     = LTE_T_Reordering_ms35;
          config_am_pP->dl_AM_RLC.t_StatusProhibit = LTE_T_StatusProhibit_ms0;
          config_am_pP->ul_AM_RLC.t_PollRetransmit = LTE_T_PollRetransmit_ms45;
          config_am_pP->ul_AM_RLC.pollPDU          = LTE_PollPDU_pInfinity;
          config_am_pP->ul_AM_RLC.pollByte         = LTE_PollByte_kBinfinity;
          config_am_pP->ul_AM_RLC.maxRetxThreshold = LTE_UL_AM_RLC__maxRetxThreshold_t4;

          if (rrc_rlc_add_rlc (ctxt_pP, SRB_FLAG_YES, MBMS_FLAG_NO, rb_id, lc_id, RLC_MODE_AM
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                               ,0,
                               0
#endif
                               ) != NULL) {
            config_req_rlc_am_asn1 (
              ctxt_pP,
              SRB_FLAG_YES,
              &srb_toaddmod_p->rlc_Config->choice.explicitValue.choice.am,
              rb_id,lc_id);
          } else {
            LOG_E(RLC, PROTOCOL_CTXT_FMT" ERROR IN ALLOCATING SRB %d \n",
                  PROTOCOL_CTXT_ARGS(ctxt_pP),
                  rb_id);
          }
/*
          if (rrc_rlc_add_rlc   (ctxt_pP, SRB_FLAG_YES, MBMS_FLAG_NO, rb_id, lc_id, RLC_MODE_UM) != NULL) {
            config_req_rlc_um_asn1(
              ctxt_pP,
              SRB_FLAG_YES,
              MBMS_FLAG_NO,
              UNUSED_PARAM_MBMS_SESSION_ID,
              UNUSED_PARAM_MBMS_SERVICE_ID,
              NULL, // TO DO DEFAULT CONFIG
              NULL, // TO DO DEFAULT CONFIG
              rb_id, lc_id);
          } else {
            LOG_D(RLC, PROTOCOL_CTXT_FMT" ERROR IN ALLOCATING SRB %d \n",
                  PROTOCOL_CTXT_ARGS(ctxt_pP),
                  rb_id);
          }
          */

          break;

        default:
          ;
        }
      }
    }
  }

  if (drb2add_listP != NULL) {
    for (cnt=0; cnt<drb2add_listP->list.count; cnt++) {
      drb_toaddmod_p = drb2add_listP->list.array[cnt];

      drb_id = drb_toaddmod_p->drb_Identity;
      if (drb_toaddmod_p->logicalChannelIdentity) {
        lc_id = *drb_toaddmod_p->logicalChannelIdentity;
      } else {
        LOG_E(RLC, PROTOCOL_CTXT_FMT" logicalChannelIdentity is missing from drb-ToAddMod information element!\n", PROTOCOL_CTXT_ARGS(ctxt_pP));
        continue;
      }

      if (lc_id == 1 || lc_id == 2) {
        LOG_E(RLC, PROTOCOL_CTXT_FMT" logicalChannelIdentity = %d is invalid in RRC message when adding DRB!\n", PROTOCOL_CTXT_ARGS(ctxt_pP), lc_id);
        continue;
      }

      LOG_D(RLC, "Adding DRB %ld, lc_id %d\n",drb_id,lc_id);

      if (drb_toaddmod_p->rlc_Config) {

        switch (drb_toaddmod_p->rlc_Config->present) {
        case LTE_RLC_Config_PR_NOTHING:
          break;

        case LTE_RLC_Config_PR_am:
          if (rrc_rlc_add_rlc (ctxt_pP, SRB_FLAG_NO, MBMS_FLAG_NO, drb_id, lc_id, RLC_MODE_AM
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                               ,0,
                               0
#endif
            ) != NULL) {
            config_req_rlc_am_asn1 (
              ctxt_pP,
              SRB_FLAG_NO,
              &drb_toaddmod_p->rlc_Config->choice.am,
              drb_id, lc_id);
          }

          break;

        case LTE_RLC_Config_PR_um_Bi_Directional:
          if (rrc_rlc_add_rlc (ctxt_pP, SRB_FLAG_NO, MBMS_FLAG_NO, drb_id, lc_id, RLC_MODE_UM
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                               ,sourceL2Id,
                               destinationL2Id
#endif
              ) != NULL) {
            config_req_rlc_um_asn1(
              ctxt_pP,
              SRB_FLAG_NO,
              MBMS_FLAG_NO,
              UNUSED_PARAM_MBMS_SESSION_ID,
              UNUSED_PARAM_MBMS_SERVICE_ID,
              &drb_toaddmod_p->rlc_Config->choice.um_Bi_Directional.ul_UM_RLC,
              &drb_toaddmod_p->rlc_Config->choice.um_Bi_Directional.dl_UM_RLC,
              drb_id, lc_id
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
              ,sourceL2Id,
              destinationL2Id
#endif
              );
          }

          break;

        case LTE_RLC_Config_PR_um_Uni_Directional_UL:
          if (rrc_rlc_add_rlc (ctxt_pP, SRB_FLAG_NO, MBMS_FLAG_NO, drb_id, lc_id, RLC_MODE_UM
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                              ,0,
                               0
#endif
             ) != NULL) {
            config_req_rlc_um_asn1(
              ctxt_pP,
              SRB_FLAG_NO,
              MBMS_FLAG_NO,
              UNUSED_PARAM_MBMS_SESSION_ID,
              UNUSED_PARAM_MBMS_SERVICE_ID,
              &drb_toaddmod_p->rlc_Config->choice.um_Uni_Directional_UL.ul_UM_RLC,
              NULL,
              drb_id, lc_id
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
               ,0, 0
#endif
               );
          }

          break;

        case LTE_RLC_Config_PR_um_Uni_Directional_DL:
          if (rrc_rlc_add_rlc (ctxt_pP, SRB_FLAG_NO, MBMS_FLAG_NO, drb_id, lc_id, RLC_MODE_UM
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                              ,0,
                               0
#endif
                               ) != NULL) {
            config_req_rlc_um_asn1(
              ctxt_pP,
              SRB_FLAG_NO,
              MBMS_FLAG_NO,
              UNUSED_PARAM_MBMS_SESSION_ID,
              UNUSED_PARAM_MBMS_SERVICE_ID,
              NULL,
              &drb_toaddmod_p->rlc_Config->choice.um_Uni_Directional_DL.dl_UM_RLC,
              drb_id, lc_id
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
               ,0, 0
#endif
               );
          }

          break;

        default:
          LOG_W(RLC, PROTOCOL_CTXT_FMT"[RB %ld] unknown drb_toaddmod_p->rlc_Config->present \n",
                PROTOCOL_CTXT_ARGS(ctxt_pP),
                drb_id);
        }
      }
    }
  }

  if (drb2release_listP != NULL) {
    for (cnt=0; cnt<drb2release_listP->list.count; cnt++) {
      pdrb_id = drb2release_listP->list.array[cnt];
      rrc_rlc_remove_rlc(
        ctxt_pP,
        SRB_FLAG_NO,
        MBMS_FLAG_NO,
        *pdrb_id);
    }
  }

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))

  if (pmch_InfoList_r9_pP != NULL) {
    for (i=0; i<pmch_InfoList_r9_pP->list.count; i++) {
      mbms_SessionInfoList_r9_p = &(pmch_InfoList_r9_pP->list.array[i]->mbms_SessionInfoList_r9);

      for (j=0; j<mbms_SessionInfoList_r9_p->list.count; j++) {
        MBMS_SessionInfo_p = mbms_SessionInfoList_r9_p->list.array[j];
        if (0/*MBMS_SessionInfo_p->sessionId_r9*/)
          mbms_session_id  = MBMS_SessionInfo_p->sessionId_r9->buf[0];
        else
          mbms_session_id  = MBMS_SessionInfo_p->logicalChannelIdentity_r9;
        lc_id              = mbms_session_id;
        mbms_service_id    = MBMS_SessionInfo_p->tmgi_r9.serviceId_r9.buf[2]; //serviceId is 3-octet string
//        mbms_service_id    = j;

        // can set the mch_id = i
        if (ctxt_pP->enb_flag) {
          rb_id =  (mbms_service_id * LTE_maxSessionPerPMCH ) + mbms_session_id;//+ (LTE_maxDRB + 3) * MAX_MOBILES_PER_ENB; // 1
          rlc_mbms_lcid2service_session_id_eNB[ctxt_pP->module_id][lc_id].service_id                     = mbms_service_id;
          rlc_mbms_lcid2service_session_id_eNB[ctxt_pP->module_id][lc_id].session_id                     = mbms_session_id;
          rlc_mbms_enb_set_lcid_by_rb_id(ctxt_pP->module_id,rb_id,lc_id);

        } else {
          rb_id =  (mbms_service_id * LTE_maxSessionPerPMCH ) + mbms_session_id; // + (LTE_maxDRB + 3); // 15
          rlc_mbms_lcid2service_session_id_ue[ctxt_pP->module_id][lc_id].service_id                    = mbms_service_id;
          rlc_mbms_lcid2service_session_id_ue[ctxt_pP->module_id][lc_id].session_id                    = mbms_session_id;
          rlc_mbms_ue_set_lcid_by_rb_id(ctxt_pP->module_id,rb_id,lc_id);
        }

        key = RLC_COLL_KEY_MBMS_VALUE(ctxt_pP->module_id, ctxt_pP->module_id, ctxt_pP->enb_flag, mbms_service_id, mbms_session_id);

        h_rc = hashtable_get(rlc_coll_p, key, (void**)&rlc_union_p);

        if (h_rc == HASH_TABLE_KEY_NOT_EXISTS) {
          rlc_union_p = rrc_rlc_add_rlc   (
                          ctxt_pP,
                          SRB_FLAG_NO,
                          MBMS_FLAG_YES,
                          rb_id,
                          lc_id,
                          RLC_MODE_UM, 0, 0);
          //AssertFatal(rlc_union_p != NULL, "ADD MBMS RLC UM FAILED");
          if(rlc_union_p == NULL){
            LOG_E(RLC, "ADD MBMS RLC UM FAILED\n");
          }
        }

        LOG_I(RLC, PROTOCOL_CTXT_FMT" CONFIG REQ MBMS ASN1 LC ID %u RB ID %u SESSION ID %u SERVICE ID %u\n",
              PROTOCOL_CTXT_ARGS(ctxt_pP),
              lc_id,
              rb_id,
              mbms_session_id,
              mbms_service_id
             );
        dl_um_rlc.sn_FieldLength = LTE_SN_FieldLength_size5;
        dl_um_rlc.t_Reordering   = LTE_T_Reordering_ms0;

        config_req_rlc_um_asn1 (
          ctxt_pP,
          SRB_FLAG_NO,
          MBMS_FLAG_YES,
          mbms_session_id,
          mbms_service_id,
          NULL,
          &dl_um_rlc,
          rb_id, lc_id
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
          ,0, 0
#endif
               );
      }
    }
  }

#endif

  LOG_D(RLC, PROTOCOL_CTXT_FMT" CONFIG REQ ASN1 END \n",
        PROTOCOL_CTXT_ARGS(ctxt_pP));
  return RLC_OP_STATUS_OK;
}
//-----------------------------------------------------------------------------
void
rb_free_rlc_union (
  void *rlcu_pP)
{
  //-----------------------------------------------------------------------------
  rlc_union_t * rlcu_p;

  if (rlcu_pP) {
    rlcu_p = (rlc_union_t *)(rlcu_pP);
    LOG_D(RLC,"%s %p \n",__FUNCTION__,rlcu_pP);

    switch (rlcu_p->mode) {
    case RLC_MODE_AM:
      rlc_am_cleanup(&rlcu_p->rlc.am);
      break;

    case RLC_MODE_UM:
      rlc_um_cleanup(&rlcu_p->rlc.um);
      break;

    case RLC_MODE_TM:
      rlc_tm_cleanup(&rlcu_p->rlc.tm);
      break;

    default:
      LOG_W(RLC,
            "%s %p unknown RLC type\n",
            __FUNCTION__,
            rlcu_pP);
      break;
    }
  }
}

//-----------------------------------------------------------------------------
rlc_op_status_t rrc_rlc_remove_ue (
  const protocol_ctxt_t* const ctxt_pP)
{
  //-----------------------------------------------------------------------------
  rb_id_t                rb_id;

  for (rb_id = 1; rb_id <= 2; rb_id++) {
    rrc_rlc_remove_rlc(ctxt_pP,
                       SRB_FLAG_YES,
                       MBMS_FLAG_NO,
                       rb_id);
  }

  for (rb_id = 1; rb_id <= LTE_maxDRB; rb_id++) {
    rrc_rlc_remove_rlc(ctxt_pP,
                       SRB_FLAG_NO,
                       MBMS_FLAG_NO,
                       rb_id);
  }

  return RLC_OP_STATUS_OK;
}

//-----------------------------------------------------------------------------
rlc_op_status_t rrc_rlc_remove_rlc   (
  const protocol_ctxt_t* const ctxt_pP,
  const srb_flag_t  srb_flagP,
  const MBMS_flag_t MBMS_flagP,
  const rb_id_t     rb_idP)
{
  //-----------------------------------------------------------------------------
  logical_chan_id_t      lcid            = 0;
  hash_key_t             key             = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t         h_rc;
  hash_key_t             key_lcid        = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t         h_lcid_rc;
  rlc_union_t           *rlc_union_p = NULL;
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
  rlc_mbms_id_t         *mbms_id_p  = NULL;
#endif

  /* for no gcc warnings */
  (void)lcid;

#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))

  if (MBMS_flagP == TRUE) {
    if (ctxt_pP->enb_flag) {
      lcid = rlc_mbms_enb_get_lcid_by_rb_id(ctxt_pP->module_id,rb_idP);
      mbms_id_p = &rlc_mbms_lcid2service_session_id_eNB[ctxt_pP->module_id][lcid];
      rlc_mbms_lcid2service_session_id_eNB[ctxt_pP->module_id][lcid].service_id = 0;
      rlc_mbms_lcid2service_session_id_eNB[ctxt_pP->module_id][lcid].session_id = 0;
      rlc_mbms_rbid2lcid_ue[ctxt_pP->module_id][rb_idP] = RLC_LC_UNALLOCATED;

    } else {
      lcid = rlc_mbms_ue_get_lcid_by_rb_id(ctxt_pP->module_id,rb_idP);
      mbms_id_p = &rlc_mbms_lcid2service_session_id_ue[ctxt_pP->module_id][lcid];
      rlc_mbms_lcid2service_session_id_eNB[ctxt_pP->module_id][lcid].service_id = 0;
      rlc_mbms_lcid2service_session_id_eNB[ctxt_pP->module_id][lcid].session_id = 0;
      rlc_mbms_rbid2lcid_ue[ctxt_pP->module_id][rb_idP] = RLC_LC_UNALLOCATED;
    }

    key = RLC_COLL_KEY_MBMS_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, mbms_id_p->service_id, mbms_id_p->session_id);
  } else
#endif
  {
    key = RLC_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, srb_flagP);
  }


  //AssertFatal (rb_idP < NB_RB_MAX, "RB id is too high (%u/%d)!\n", rb_idP, NB_RB_MAX);
	if(rb_idP >= NB_RB_MAX){
		LOG_E(RLC, "RB id is too high (%u/%d)!\n", rb_idP, NB_RB_MAX);
		return RLC_OP_STATUS_BAD_PARAMETER;
	}

  h_rc = hashtable_get(rlc_coll_p, key, (void**)&rlc_union_p);

  if (h_rc == HASH_TABLE_OK) {
    // also remove the hash-key created by LC-id
    switch (rlc_union_p->mode) {
    case RLC_MODE_AM:
      lcid = rlc_union_p->rlc.am.channel_id;
      break;
    case RLC_MODE_UM:
      lcid = rlc_union_p->rlc.um.channel_id;
      break;
    case RLC_MODE_TM:
      lcid = rlc_union_p->rlc.tm.channel_id;
      break;
    default:
      LOG_E(RLC, PROTOCOL_CTXT_FMT"[%s %u] RLC mode is unknown!\n",
            PROTOCOL_CTXT_ARGS(ctxt_pP),
            (srb_flagP) ? "SRB" : "DRB",
            rb_idP);
    }
    key_lcid = RLC_COLL_KEY_LCID_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, lcid, srb_flagP);
    h_lcid_rc = hashtable_get(rlc_coll_p, key_lcid, (void**)&rlc_union_p);
  } else {
    h_lcid_rc = HASH_TABLE_KEY_NOT_EXISTS;
  }

  if ((h_rc == HASH_TABLE_OK) && (h_lcid_rc == HASH_TABLE_OK)) {
    h_lcid_rc = hashtable_remove(rlc_coll_p, key_lcid);
    h_rc = hashtable_remove(rlc_coll_p, key);
    LOG_D(RLC, PROTOCOL_CTXT_FMT"[%s %u LCID %d] RELEASED %s\n",
          PROTOCOL_CTXT_ARGS(ctxt_pP),
          (srb_flagP) ? "SRB" : "DRB",
          rb_idP, lcid,
          (srb_flagP) ? "SRB" : "DRB");
  } else if ((h_rc == HASH_TABLE_KEY_NOT_EXISTS) || (h_lcid_rc == HASH_TABLE_KEY_NOT_EXISTS)) {
    LOG_D(RLC, PROTOCOL_CTXT_FMT"[%s %u LCID %d] RELEASE : RLC NOT FOUND %s, by RB-ID=%d, by LC-ID=%d\n",
          PROTOCOL_CTXT_ARGS(ctxt_pP),
          (srb_flagP) ? "SRB" : "DRB",
          rb_idP, lcid,
          (srb_flagP) ? "SRB" : "DRB",
          h_rc, h_lcid_rc);
  } else {
    LOG_E(RLC, PROTOCOL_CTXT_FMT"[%s %u LCID %d] RELEASE : INTERNAL ERROR %s\n",
          PROTOCOL_CTXT_ARGS(ctxt_pP),
          (srb_flagP) ? "SRB" : "DRB",
          rb_idP, lcid,
          (srb_flagP) ? "SRB" : "DRB");
  }

  return RLC_OP_STATUS_OK;
}
//-----------------------------------------------------------------------------
rlc_union_t* rrc_rlc_add_rlc   (
  const protocol_ctxt_t* const ctxt_pP,
  const srb_flag_t        srb_flagP,
  const MBMS_flag_t       MBMS_flagP,
  const rb_id_t           rb_idP,
  const logical_chan_id_t chan_idP,
  const rlc_mode_t        rlc_modeP
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  ,const uint32_t sourceL2Id,
  const uint32_t  destinationL2Id
#endif
)
{

  //-----------------------------------------------------------------------------
  hash_key_t             key         = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t         h_rc;
  hash_key_t             key_lcid    = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t         h_lcid_rc;
  rlc_union_t           *rlc_union_p = NULL;
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
  rlc_mbms_id_t         *mbms_id_p  = NULL;
  logical_chan_id_t      lcid            = 0;
#endif


  if (MBMS_flagP == FALSE) {
    //AssertFatal (rb_idP < NB_RB_MAX, "RB id is too high (%u/%d)!\n", rb_idP, NB_RB_MAX);
    //AssertFatal (chan_idP < RLC_MAX_LC, "LC id is too high (%u/%d)!\n", chan_idP, RLC_MAX_LC);
  	if(rb_idP >= NB_RB_MAX){
  		LOG_E(RLC, "RB id is too high (%u/%d)!\n", rb_idP, NB_RB_MAX);
  		return NULL;
  	}
  	if(chan_idP >= RLC_MAX_LC){
  		LOG_E(RLC, "LC id is too high (%u/%d)!\n", chan_idP, RLC_MAX_LC);
  		return NULL;
  	}

  }

#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))

  if (MBMS_flagP == TRUE) {
    if (ctxt_pP->enb_flag) {
      lcid = rlc_mbms_enb_get_lcid_by_rb_id(ctxt_pP->module_id,rb_idP);
      mbms_id_p = &rlc_mbms_lcid2service_session_id_eNB[ctxt_pP->module_id][lcid];
      //LG 2014-04-15rlc_mbms_lcid2service_session_id_eNB[ctxt_pP->module_id][lcid].service_id = 0;
      //LG 2014-04-15rlc_mbms_lcid2service_session_id_eNB[ctxt_pP->module_id][lcid].session_id = 0;
      //LG 2014-04-15rlc_mbms_rbid2lcid_eNB[ctxt_pP->module_id][rb_idP] = RLC_LC_UNALLOCATED;

    } else {
      lcid = rlc_mbms_ue_get_lcid_by_rb_id(ctxt_pP->module_id,rb_idP);
      mbms_id_p = &rlc_mbms_lcid2service_session_id_ue[ctxt_pP->module_id][lcid];
      //LG 2014-04-15rlc_mbms_lcid2service_session_id_eNB[ctxt_pP->module_id][lcid].service_id = 0;
      //LG 2014-04-15rlc_mbms_lcid2service_session_id_eNB[ctxt_pP->module_id][lcid].session_id = 0;
      //LG 2014-04-15rlc_mbms_rbid2lcid_ue[ctxt_pP->module_id][rb_idP] = RLC_LC_UNALLOCATED;
    }

    key = RLC_COLL_KEY_MBMS_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, mbms_id_p->service_id, mbms_id_p->session_id);
  }else
  if ((sourceL2Id > 0) && (destinationL2Id > 0) ){
     key = RLC_COLL_KEY_SOURCE_DEST_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, sourceL2Id, destinationL2Id, srb_flagP);
     key_lcid = RLC_COLL_KEY_LCID_SOURCE_DEST_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, chan_idP, sourceL2Id, destinationL2Id, srb_flagP);
  } else
#endif
  {
    key = RLC_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, srb_flagP);
    key_lcid = RLC_COLL_KEY_LCID_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, chan_idP, srb_flagP);
  }

  h_rc = hashtable_get(rlc_coll_p, key, (void**)&rlc_union_p);

  if (h_rc == HASH_TABLE_OK) {
    LOG_W(RLC, PROTOCOL_CTXT_FMT"[%s %u] rrc_rlc_add_rlc , already exist %s\n",
          PROTOCOL_CTXT_ARGS(ctxt_pP),
          (srb_flagP) ? "SRB" : "DRB",
          rb_idP,
          (srb_flagP) ? "SRB" : "DRB");
    //AssertFatal(rlc_union_p->mode == rlc_modeP, "Error rrc_rlc_add_rlc , already exist but RLC mode differ");
  	if(rlc_union_p->mode != rlc_modeP){
  		LOG_E(RLC, "Error rrc_rlc_add_rlc , already exist but RLC mode differ\n");
  		return NULL;
  	}
  	return rlc_union_p;
  } else if (h_rc == HASH_TABLE_KEY_NOT_EXISTS) {
    rlc_union_p = calloc(1, sizeof(rlc_union_t));
    h_rc = hashtable_insert(rlc_coll_p, key, rlc_union_p);
    h_lcid_rc = hashtable_insert(rlc_coll_p, key_lcid, rlc_union_p);

    if ((h_rc == HASH_TABLE_OK) && (h_lcid_rc == HASH_TABLE_OK)) {
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))

      if (MBMS_flagP == TRUE) {
        LOG_I(RLC, PROTOCOL_CTXT_FMT" RLC service id %u session id %u rrc_rlc_add_rlc\n",
              PROTOCOL_CTXT_ARGS(ctxt_pP),
              mbms_id_p->service_id,
              mbms_id_p->session_id);
      } else
#endif
      {
        LOG_I(RLC, PROTOCOL_CTXT_FMT" [%s %u] rrc_rlc_add_rlc  %s\n",
              PROTOCOL_CTXT_ARGS(ctxt_pP),
              (srb_flagP) ? "SRB" : "DRB",
              rb_idP,
              (srb_flagP) ? "SRB" : "DRB");
      }

      rlc_union_p->mode = rlc_modeP;
      return rlc_union_p;
    } else {
      LOG_E(RLC, PROTOCOL_CTXT_FMT"[%s %u] rrc_rlc_add_rlc FAILED %s (add by RB_id=%d; add by LC_id=%d)\n",
            PROTOCOL_CTXT_ARGS(ctxt_pP),
            (srb_flagP) ? "SRB" : "DRB",
            rb_idP,
            (srb_flagP) ? "SRB" : "DRB",
            h_rc, h_lcid_rc);
      free(rlc_union_p);
      rlc_union_p = NULL;
      return NULL;
    }
  } else {
    LOG_E(RLC, PROTOCOL_CTXT_FMT"[%s %u] rrc_rlc_add_rlc , INTERNAL ERROR %s\n",
          PROTOCOL_CTXT_ARGS(ctxt_pP),
          (srb_flagP) ? "SRB" : "DRB",
          rb_idP,
          (srb_flagP) ? "SRB" : "DRB");
  }
  return NULL;
}
//-----------------------------------------------------------------------------
rlc_op_status_t rrc_rlc_config_req   (
  const protocol_ctxt_t* const ctxt_pP,
  const srb_flag_t      srb_flagP,
  const MBMS_flag_t     mbms_flagP,
  const config_action_t actionP,
  const rb_id_t         rb_idP,
  const rlc_info_t      rlc_infoP)
{
  //-----------------------------------------------------------------------------
  //rlc_op_status_t status;

  LOG_D(RLC, PROTOCOL_CTXT_FMT" CONFIG_REQ for RAB %u\n",
        PROTOCOL_CTXT_ARGS(ctxt_pP),
        rb_idP);

  //AssertFatal (rb_idP < NB_RB_MAX, "RB id is too high (%u/%d)!\n", rb_idP, NB_RB_MAX);
	if(rb_idP >= NB_RB_MAX){
		LOG_E(RLC, "RB id is too high (%u/%d)!\n", rb_idP, NB_RB_MAX);
		return RLC_OP_STATUS_BAD_PARAMETER;
	}

  switch (actionP) {

  case CONFIG_ACTION_ADD:
    if (rrc_rlc_add_rlc(ctxt_pP, srb_flagP, MBMS_FLAG_NO, rb_idP, rb_idP, rlc_infoP.rlc_mode
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                        ,0,
                        0
#endif
    ) != NULL) {
      return RLC_OP_STATUS_INTERNAL_ERROR;
    }

    // no break, fall to next case
  case CONFIG_ACTION_MODIFY:
    switch (rlc_infoP.rlc_mode) {
    case RLC_MODE_AM:
      LOG_I(RLC, PROTOCOL_CTXT_FMT"[RB %u] MODIFY RB AM\n",
            PROTOCOL_CTXT_ARGS(ctxt_pP),
            rb_idP);

      config_req_rlc_am(
        ctxt_pP,
        srb_flagP,
        &rlc_infoP.rlc.rlc_am_info,
        rb_idP, rb_idP);
      break;

    case RLC_MODE_UM:
      LOG_I(RLC, PROTOCOL_CTXT_FMT"[RB %u] MODIFY RB UM\n",
            PROTOCOL_CTXT_ARGS(ctxt_pP),
            rb_idP);
      config_req_rlc_um(
        ctxt_pP,
        srb_flagP,
        &rlc_infoP.rlc.rlc_um_info,
        rb_idP, rb_idP);
      break;

    case RLC_MODE_TM:
      LOG_I(RLC, PROTOCOL_CTXT_FMT"[RB %u] MODIFY RB TM\n",
            PROTOCOL_CTXT_ARGS(ctxt_pP),
            rb_idP);
      config_req_rlc_tm(
        ctxt_pP,
        srb_flagP,
        &rlc_infoP.rlc.rlc_tm_info,
        rb_idP, rb_idP);
      break;

    default:
      return RLC_OP_STATUS_BAD_PARAMETER;
    }

    break;

  case CONFIG_ACTION_REMOVE:
    return rrc_rlc_remove_rlc(ctxt_pP, srb_flagP, mbms_flagP, rb_idP);
    break;

  default:
    return RLC_OP_STATUS_BAD_PARAMETER;
  }

  return RLC_OP_STATUS_OK;
}
//-----------------------------------------------------------------------------
rlc_op_status_t rrc_rlc_data_req     (
  const protocol_ctxt_t* const ctxt_pP,
  const MBMS_flag_t MBMS_flagP,
  const rb_id_t     rb_idP,
  const mui_t       muiP,
  const confirm_t   confirmP,
  const sdu_size_t  sdu_sizeP,
  char* sduP)
{
  //-----------------------------------------------------------------------------
  mem_block_t*   sdu;

  sdu = get_free_mem_block(sdu_sizeP, __func__);

  if (sdu != NULL) {
    memcpy (sdu->data, sduP, sdu_sizeP);
    return rlc_data_req(ctxt_pP, SRB_FLAG_YES, MBMS_flagP, rb_idP, muiP, confirmP, sdu_sizeP, sdu
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                        ,NULL, NULL
#endif
                        );
  } else {
    return RLC_OP_STATUS_INTERNAL_ERROR;
  }
}

//-----------------------------------------------------------------------------
void rrc_rlc_register_rrc (rrc_data_ind_cb_t rrc_data_indP, rrc_data_conf_cb_t rrc_data_confP)
{
  //-----------------------------------------------------------------------------
  rlc_rrc_data_ind  = rrc_data_indP;
  rlc_rrc_data_conf = rrc_data_confP;
}

