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

/*! \file ngap_gNB_management_procedures.c
 * \brief NGAP gNB task 
 * \author  Yoshio INOUE, Masayuki HARADA 
 * \date 2020
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \version 1.0
 * @ingroup _ngap
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "intertask_interface.h"

#include "assertions.h"
#include "conversions.h"

#include "ngap_common.h"
#include "ngap_gNB_defs.h"
#include "ngap_gNB.h"

ngap_gNB_internal_data_t ngap_gNB_internal_data;

RB_GENERATE(ngap_amf_map, ngap_gNB_amf_data_s, entry, ngap_gNB_compare_assoc_id);

int ngap_gNB_compare_assoc_id(
  struct ngap_gNB_amf_data_s *p1, struct ngap_gNB_amf_data_s *p2)
{
  if (p1->assoc_id == -1) {
    if (p1->cnx_id < p2->cnx_id) {
      return -1;
    }

    if (p1->cnx_id > p2->cnx_id) {
      return 1;
    }
  } else {
    if (p1->assoc_id < p2->assoc_id) {
      return -1;
    }

    if (p1->assoc_id > p2->assoc_id) {
      return 1;
    }
  }

  /* Matching reference */
  return 0;
}

uint16_t ngap_gNB_fetch_add_global_cnx_id(void)
{
  return ++ngap_gNB_internal_data.global_cnx_id;
}

void ngap_gNB_prepare_internal_data(void)
{
  memset(&ngap_gNB_internal_data, 0, sizeof(ngap_gNB_internal_data));
  STAILQ_INIT(&ngap_gNB_internal_data.ngap_gNB_instances_head);
}

void ngap_gNB_insert_new_instance(ngap_gNB_instance_t *new_instance_p)
{
  DevAssert(new_instance_p != NULL);

  STAILQ_INSERT_TAIL(&ngap_gNB_internal_data.ngap_gNB_instances_head,
                     new_instance_p, ngap_gNB_entries);
}

struct ngap_gNB_amf_data_s *ngap_gNB_get_AMF(ngap_gNB_instance_t *instance_p, sctp_assoc_t assoc_id, uint16_t cnx_id)
{
  struct ngap_gNB_amf_data_s  temp;
  struct ngap_gNB_amf_data_s *found;

  memset(&temp, 0, sizeof(struct ngap_gNB_amf_data_s));

  temp.assoc_id = assoc_id;
  temp.cnx_id   = cnx_id;

  if (instance_p == NULL) {
    STAILQ_FOREACH(instance_p, &ngap_gNB_internal_data.ngap_gNB_instances_head,
                   ngap_gNB_entries) {
      found = RB_FIND(ngap_amf_map, &instance_p->ngap_amf_head, &temp);

      if (found != NULL) {
        return found;
      }
    }
  } else {
    return RB_FIND(ngap_amf_map, &instance_p->ngap_amf_head, &temp);
  }

  return NULL;
}

struct ngap_gNB_amf_data_s *ngap_gNB_get_AMF_from_instance(
  ngap_gNB_instance_t *instance_p)
{
 
  struct ngap_gNB_amf_data_s *amf = NULL;
  struct ngap_gNB_amf_data_s *amf_next = NULL;

  for (amf = RB_MIN(ngap_amf_map, &instance_p->ngap_amf_head); amf!=NULL ; amf = amf_next) {
    amf_next = RB_NEXT(ngap_amf_map, &instance_p->ngap_amf_head, amf);
    if (amf->ngap_gNB_instance == instance_p) {
      return amf;
    }
  }

  return NULL;
}

ngap_gNB_instance_t *ngap_gNB_get_instance(instance_t instance)
{
  ngap_gNB_instance_t *temp = NULL;

  STAILQ_FOREACH(temp, &ngap_gNB_internal_data.ngap_gNB_instances_head,
                 ngap_gNB_entries) {
    if (temp->instance == instance) {
      /* Matching occurence */
      return temp;
    }
  }

  return NULL;
}

void ngap_gNB_remove_amf_desc(ngap_gNB_instance_t * instance) 
{

    struct ngap_gNB_amf_data_s *amf = NULL;
    struct ngap_gNB_amf_data_s *amfNext = NULL;
    struct plmn_identity_s* plmnInfo;
    struct served_region_id_s* regionInfo;
    struct served_guami_s* guamInfo;
    struct amf_set_id_s* amfSet;
    struct amf_pointer_s* amfPoint;
    for (amf = RB_MIN(ngap_amf_map, &instance->ngap_amf_head); amf; amf = amfNext) {
      amfNext = RB_NEXT(ngap_amf_map, &instance->ngap_amf_head, amf);
      RB_REMOVE(ngap_amf_map, &instance->ngap_amf_head, amf);
      while (!STAILQ_EMPTY(&amf->served_guami)) {
        guamInfo = STAILQ_FIRST(&amf->served_guami);
        STAILQ_REMOVE_HEAD(&amf->served_guami, next);

        while (!STAILQ_EMPTY(&guamInfo->served_plmns)) {
          plmnInfo = STAILQ_FIRST(&guamInfo->served_plmns);
          STAILQ_REMOVE_HEAD(&guamInfo->served_plmns, next);
          free(plmnInfo);
        }
        while (!STAILQ_EMPTY(&guamInfo->served_region_ids)) {
          regionInfo = STAILQ_FIRST(&guamInfo->served_region_ids);
          STAILQ_REMOVE_HEAD(&guamInfo->served_region_ids, next);
          free(regionInfo);
        }
        while (!STAILQ_EMPTY(&guamInfo->amf_set_ids)) {
          amfSet = STAILQ_FIRST(&guamInfo->amf_set_ids);
          STAILQ_REMOVE_HEAD(&guamInfo->amf_set_ids, next);
          free(amfSet);
        }
        while (!STAILQ_EMPTY(&guamInfo->amf_pointers)) {
          amfPoint = STAILQ_FIRST(&guamInfo->amf_pointers);
          STAILQ_REMOVE_HEAD(&guamInfo->amf_pointers, next);
          free(amfPoint);
        }
        free(guamInfo);
      }
      free(amf);
    }
}
