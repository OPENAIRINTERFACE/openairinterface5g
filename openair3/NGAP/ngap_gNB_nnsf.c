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

/*! \file ngap_gNB_nnsf.c
 * \brief ngap NAS node selection functions
 * \author Yoshio INOUE, Masayuki HARADA
 * \date 2020
 * \version 0.1
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 */

#include <stdio.h>
#include <stdlib.h>

#include "intertask_interface.h"

#include "ngap_common.h"

#include "ngap_gNB_defs.h"
#include "ngap_gNB_nnsf.h"

struct ngap_gNB_amf_data_s *
ngap_gNB_nnsf_select_amf(ngap_gNB_instance_t       *instance_p,
                         ngap_rrc_establishment_cause_t  cause)
{
  struct ngap_gNB_amf_data_s *amf_data_p = NULL;
  struct ngap_gNB_amf_data_s *amf_highest_capacity_p = NULL;
  uint8_t current_capacity = 0;

  RB_FOREACH(amf_data_p, ngap_amf_map, &instance_p->ngap_amf_head) {
    if (amf_data_p->state != NGAP_GNB_STATE_CONNECTED) {
      /* The association between AMF and gNB is not ready for the moment,
       * go to the next known AMF.
       */
      if (amf_data_p->state == NGAP_GNB_OVERLOAD) {
        /* AMF is overloaded. We have to check the RRC establishment
         * cause and take decision to the select this AMF depending on
         * the overload state.
         */
        if ((cause == NGAP_RRC_CAUSE_MO_DATA)
            && (amf_data_p->overload_state == NGAP_OVERLOAD_REJECT_MO_DATA)) {
          continue;
        }

        if ((amf_data_p->overload_state == NGAP_OVERLOAD_REJECT_ALL_SIGNALLING)
            && ((cause == NGAP_RRC_CAUSE_MO_SIGNALLING) || (cause == NGAP_RRC_CAUSE_MO_DATA))) {
          continue;
        }

        if ((amf_data_p->overload_state == NGAP_OVERLOAD_ONLY_EMERGENCY_AND_MT)
            && ((cause == NGAP_RRC_CAUSE_MO_SIGNALLING) || (cause == NGAP_RRC_CAUSE_MO_DATA)
                || (cause == NGAP_RRC_CAUSE_HIGH_PRIO_ACCESS))) {
          continue;
        }

        /* At this point, the RRC establishment can be handled by the AMF
         * even if it is in overload state.
         */
      } else {
        /* The AMF is not overloaded, association is simply not ready. */
        continue;
      }
    }

    if (current_capacity < amf_data_p->relative_amf_capacity) {
      /* We find a better AMF, keep a reference to it */
      current_capacity = amf_data_p->relative_amf_capacity;
      amf_highest_capacity_p = amf_data_p;
    }
  }

  return amf_highest_capacity_p;
}

struct ngap_gNB_amf_data_s *
ngap_gNB_nnsf_select_amf_by_plmn_id(ngap_gNB_instance_t       *instance_p,
                                    ngap_rrc_establishment_cause_t  cause,
                                    int                        selected_plmn_identity)
{
  struct ngap_gNB_amf_data_s *amf_data_p = NULL;
  struct ngap_gNB_amf_data_s *amf_highest_capacity_p = NULL;
  uint8_t current_capacity = 0;

  RB_FOREACH(amf_data_p, ngap_amf_map, &instance_p->ngap_amf_head) {
    struct served_guami_s *guami_p = NULL;
    struct plmn_identity_s *served_plmn_p = NULL;
    if (amf_data_p->state != NGAP_GNB_STATE_CONNECTED) {
      /* The association between AMF and gNB is not ready for the moment,
       * go to the next known AMF.
       */
      if (amf_data_p->state == NGAP_GNB_OVERLOAD) {
        /* AMF is overloaded. We have to check the RRC establishment
         * cause and take decision to the select this AMF depending on
         * the overload state.
         */
        if ((cause == NGAP_RRC_CAUSE_MO_DATA)
            && (amf_data_p->overload_state == NGAP_OVERLOAD_REJECT_MO_DATA)) {
          continue;
        }

        if ((amf_data_p->overload_state == NGAP_OVERLOAD_REJECT_ALL_SIGNALLING)
            && ((cause == NGAP_RRC_CAUSE_MO_SIGNALLING) || (cause == NGAP_RRC_CAUSE_MO_DATA))) {
          continue;
        }

        if ((amf_data_p->overload_state == NGAP_OVERLOAD_ONLY_EMERGENCY_AND_MT)
            && ((cause == NGAP_RRC_CAUSE_MO_SIGNALLING) || (cause == NGAP_RRC_CAUSE_MO_DATA)
                || (cause == NGAP_RRC_CAUSE_HIGH_PRIO_ACCESS))) {
          continue;
        }

        /* At this point, the RRC establishment can be handled by the AMF
         * even if it is in overload state.
         */
      } else {
        /* The AMF is not overloaded, association is simply not ready. */
        continue;
      }
    }

    /* Looking for served GUAMI PLMN Identity selected matching the one provided by the UE */
    STAILQ_FOREACH(guami_p, &amf_data_p->served_guami, next) {
      STAILQ_FOREACH(served_plmn_p, &guami_p->served_plmns, next) {
        if ((served_plmn_p->mcc == instance_p->mcc[selected_plmn_identity]) &&
            (served_plmn_p->mnc == instance_p->mnc[selected_plmn_identity])) {
          break;
        }
      }
      /* if found, we can stop the outer loop, too */
      if (served_plmn_p) break;
    }
    /* if we didn't find such a served PLMN, go on with the next AMF */
    if (!served_plmn_p) continue;

    if (current_capacity < amf_data_p->relative_amf_capacity) {
      /* We find a better AMF, keep a reference to it */
      current_capacity = amf_data_p->relative_amf_capacity;
      amf_highest_capacity_p = amf_data_p;
    }
  }

  return amf_highest_capacity_p;
}

struct ngap_gNB_amf_data_s *
ngap_gNB_nnsf_select_amf_by_amf_setid(ngap_gNB_instance_t       *instance_p,
                                     ngap_rrc_establishment_cause_t  cause,
                                     int                        selected_plmn_identity,
                                     uint8_t                    amf_setid)
{
  struct ngap_gNB_amf_data_s *amf_data_p = NULL;

  RB_FOREACH(amf_data_p, ngap_amf_map, &instance_p->ngap_amf_head) {
    struct served_guami_s *guami_p = NULL;

    if (amf_data_p->state != NGAP_GNB_STATE_CONNECTED) {
      /* The association between AMF and gNB is not ready for the moment,
       * go to the next known AMF.
       */
      if (amf_data_p->state == NGAP_GNB_OVERLOAD) {
        /* AMF is overloaded. We have to check the RRC establishment
         * cause and take decision to the select this AMF depending on
         * the overload state.
         */
        if ((cause == NGAP_RRC_CAUSE_MO_DATA)
            && (amf_data_p->overload_state == NGAP_OVERLOAD_REJECT_MO_DATA)) {
          continue;
        }

        if ((amf_data_p->overload_state == NGAP_OVERLOAD_REJECT_ALL_SIGNALLING)
            && ((cause == NGAP_RRC_CAUSE_MO_SIGNALLING) || (cause == NGAP_RRC_CAUSE_MO_DATA))) {
          continue;
        }

        if ((amf_data_p->overload_state == NGAP_OVERLOAD_ONLY_EMERGENCY_AND_MT)
            && ((cause == NGAP_RRC_CAUSE_MO_SIGNALLING) || (cause == NGAP_RRC_CAUSE_MO_DATA)
                || (cause == NGAP_RRC_CAUSE_HIGH_PRIO_ACCESS))) {
          continue;
        }

        /* At this point, the RRC establishment can be handled by the AMF
         * even if it is in overload state.
         */
      } else {
        /* The AMF is not overloaded, association is simply not ready. */
        continue;
      }
    }

    /* Looking for AMF code matching the one provided by NAS */
    STAILQ_FOREACH(guami_p, &amf_data_p->served_guami, next) {
      struct amf_set_id_s      *amf_setid_p = NULL;
      struct plmn_identity_s   *served_plmn_p = NULL;

      STAILQ_FOREACH(served_plmn_p, &guami_p->served_plmns, next) {
        if ((served_plmn_p->mcc == instance_p->mcc[selected_plmn_identity]) &&
            (served_plmn_p->mnc == instance_p->mnc[selected_plmn_identity])) {
          break;
        }
      }
      STAILQ_FOREACH(amf_setid_p, &guami_p->amf_set_ids, next) {
        if (amf_setid_p->amf_set_id == amf_setid) {
          break;
        }
      }

      /* The AMF matches the parameters provided by the NAS layer ->
      * the AMF is knwown and the association is ready.
      * Return the reference to the AMF to use it for this UE.
      */
      if (amf_setid_p && served_plmn_p) {
        return amf_data_p;
      }
    }
  }

  /* At this point no AMF matches the selected PLMN and AMF code. In this case,
   * return NULL. That way the RRC layer should know about it and reject RRC
   * connectivity. */
  return NULL;
}

struct ngap_gNB_amf_data_s *
ngap_gNB_nnsf_select_amf_by_guami(ngap_gNB_instance_t       *instance_p,
                                   ngap_rrc_establishment_cause_t  cause,
                                   ngap_guami_t                   guami)
{
  struct ngap_gNB_amf_data_s *amf_data_p             = NULL;

  RB_FOREACH(amf_data_p, ngap_amf_map, &instance_p->ngap_amf_head) {
    struct served_guami_s *guami_p = NULL;

    if (amf_data_p->state != NGAP_GNB_STATE_CONNECTED) {
      /* The association between AMF and gNB is not ready for the moment,
       * go to the next known AMF.
       */
      if (amf_data_p->state == NGAP_GNB_OVERLOAD) {
        /* AMF is overloaded. We have to check the RRC establishment
         * cause and take decision to the select this AMF depending on
         * the overload state.
         */
        if ((cause == NGAP_RRC_CAUSE_MO_DATA)
            && (amf_data_p->overload_state == NGAP_OVERLOAD_REJECT_MO_DATA)) {
          continue;
        }

        if ((amf_data_p->overload_state == NGAP_OVERLOAD_REJECT_ALL_SIGNALLING)
            && ((cause == NGAP_RRC_CAUSE_MO_SIGNALLING) || (cause == NGAP_RRC_CAUSE_MO_DATA))) {
          continue;
        }

        if ((amf_data_p->overload_state == NGAP_OVERLOAD_ONLY_EMERGENCY_AND_MT)
            && ((cause == NGAP_RRC_CAUSE_MO_SIGNALLING) || (cause == NGAP_RRC_CAUSE_MO_DATA)
                || (cause == NGAP_RRC_CAUSE_HIGH_PRIO_ACCESS))) {
          continue;
        }

        /* At this point, the RRC establishment can be handled by the AMF
         * even if it is in overload state.
         */
      } else {
        /* The AMF is not overloaded, association is simply not ready. */
        continue;
      }
    }

    /* Looking for AMF guami matching the one provided by NAS */
    STAILQ_FOREACH(guami_p, &amf_data_p->served_guami, next) {
      struct served_region_id_s *region_id_p = NULL;
      struct amf_set_id_s       *amf_set_id_p = NULL;
      struct amf_pointer_s      *pointer_p = NULL;
      struct plmn_identity_s   *served_plmn_p = NULL;

      STAILQ_FOREACH(served_plmn_p, &guami_p->served_plmns, next) {
        if ((served_plmn_p->mcc == guami.mcc) &&
            (served_plmn_p->mnc == guami.mnc)) {
          break;
        }
      }
      
      STAILQ_FOREACH(region_id_p, &guami_p->served_region_ids, next) {
        if (region_id_p->amf_region_id == guami.amf_region_id) {
          break;
        }
      }
      STAILQ_FOREACH(amf_set_id_p, &guami_p->amf_set_ids, next) {
        if (amf_set_id_p->amf_set_id == guami.amf_set_id) {
          break;
        }
      }
      STAILQ_FOREACH(pointer_p, &guami_p->amf_pointers, next) {
        if (pointer_p->amf_pointer == guami.amf_pointer) {
          break;
        }
      }

      /* The AMF matches the parameters provided by the NAS layer ->
      * the AMF is knwown and the association is ready.
      * Return the reference to the AMF to use it for this UE.
      */
      if ((region_id_p != NULL) &&
          (amf_set_id_p != NULL) &&
          (pointer_p != NULL) &&
          (served_plmn_p != NULL)) {
        return amf_data_p;
      }
    }
  }

  /* At this point no AMF matches the provided GUAMI. In this case, return
   * NULL. That way the RRC layer should know about it and reject RRC
   * connectivity. */
  return NULL;
}


struct ngap_gNB_amf_data_s *
ngap_gNB_nnsf_select_amf_by_guami_no_cause(ngap_gNB_instance_t       *instance_p,
                                            ngap_guami_t                   guami)
{
  struct ngap_gNB_amf_data_s *amf_data_p             = NULL;
  struct ngap_gNB_amf_data_s *amf_highest_capacity_p = NULL;
  uint8_t                     current_capacity       = 0;


  RB_FOREACH(amf_data_p, ngap_amf_map, &instance_p->ngap_amf_head) {
    struct served_guami_s *guami_p = NULL;

    if (amf_data_p->state != NGAP_GNB_STATE_CONNECTED) {
      /* The association between AMF and gNB is not ready for the moment,
       * go to the next known AMF.
       */
      if (amf_data_p->state == NGAP_GNB_OVERLOAD) {
        /* AMF is overloaded. We have to check the RRC establishment
         * cause and take decision to the select this AMF depending on
         * the overload state.
         */
    } else {
        /* The AMF is not overloaded, association is simply not ready. */
        continue;
      }
    }

    if (current_capacity < amf_data_p->relative_amf_capacity) {
      /* We find a better AMF, keep a reference to it */
      current_capacity = amf_data_p->relative_amf_capacity;
      amf_highest_capacity_p = amf_data_p;
    }

    /* Looking for AMF guami matching the one provided by NAS */
    STAILQ_FOREACH(guami_p, &amf_data_p->served_guami, next) {
      struct served_region_id_s *region_id_p = NULL;
      struct amf_set_id_s       *amf_set_id_p = NULL;
      struct plmn_identity_s    *served_plmn_p = NULL;

      STAILQ_FOREACH(served_plmn_p, &guami_p->served_plmns, next) {
        if ((served_plmn_p->mcc == guami.mcc) &&
            (served_plmn_p->mnc == guami.mnc)) {
          break;
        }
      }
      STAILQ_FOREACH(amf_set_id_p, &guami_p->amf_set_ids, next) {
        if (amf_set_id_p->amf_set_id == guami.amf_set_id) {
          break;
        }
      }
      STAILQ_FOREACH(region_id_p, &guami_p->served_region_ids, next) {
        if (region_id_p->amf_region_id == guami.amf_region_id) {
          break;
        }
      }

      /* The AMF matches the parameters provided by the NAS layer ->
      * the AMF is knwown and the association is ready.
      * Return the reference to the AMF to use it for this UE.
      */
      if ((region_id_p != NULL) &&
          (amf_set_id_p != NULL) &&
          (served_plmn_p != NULL)) {
        return amf_data_p;
      }
    }
  }

  /* At this point no AMF matches the provided GUAMI. Select the one with the
   * highest relative capacity.
   * In case the list of known AMF is empty, simply return NULL, that way the RRC
   * layer should know about it and reject RRC connectivity.
   */

  return amf_highest_capacity_p;
}
