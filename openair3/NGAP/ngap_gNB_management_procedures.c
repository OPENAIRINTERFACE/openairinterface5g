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
 * \author  S. Roux and Navid Nikaein 
 * \date 2010 - 2016
 * \email: navid.nikaein@eurecom.fr
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

RB_GENERATE(ngap_mme_map, ngap_gNB_mme_data_s, entry, ngap_gNB_compare_assoc_id);

int ngap_gNB_compare_assoc_id(
  struct ngap_gNB_mme_data_s *p1, struct ngap_gNB_mme_data_s *p2)
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

struct ngap_gNB_mme_data_s *ngap_gNB_get_MME(
  ngap_gNB_instance_t *instance_p,
  int32_t assoc_id, uint16_t cnx_id)
{
  struct ngap_gNB_mme_data_s  temp;
  struct ngap_gNB_mme_data_s *found;

  memset(&temp, 0, sizeof(struct ngap_gNB_mme_data_s));

  temp.assoc_id = assoc_id;
  temp.cnx_id   = cnx_id;

  if (instance_p == NULL) {
    STAILQ_FOREACH(instance_p, &ngap_gNB_internal_data.ngap_gNB_instances_head,
                   ngap_gNB_entries) {
      found = RB_FIND(ngap_mme_map, &instance_p->ngap_mme_head, &temp);

      if (found != NULL) {
        return found;
      }
    }
  } else {
    return RB_FIND(ngap_mme_map, &instance_p->ngap_mme_head, &temp);
  }

  return NULL;
}

struct ngap_gNB_mme_data_s *ngap_gNB_get_MME_from_instance(
  ngap_gNB_instance_t *instance_p)
{
 
  struct ngap_gNB_mme_data_s *mme = NULL;
  struct ngap_gNB_mme_data_s *mme_next = NULL;

  for (mme = RB_MIN(ngap_mme_map, &instance_p->ngap_mme_head); mme!=NULL ; mme = mme_next) {
    mme_next = RB_NEXT(ngap_mme_map, &instance_p->ngap_mme_head, mme);
    if (mme->ngap_gNB_instance == instance_p) {
      return mme;
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

void ngap_gNB_remove_mme_desc(ngap_gNB_instance_t * instance) 
{

    struct ngap_gNB_mme_data_s *mme = NULL;
    struct ngap_gNB_mme_data_s *mmeNext = NULL;
    struct plmn_identity_s* plmnInfo;
    struct served_group_id_s* groupInfo;
    struct served_gummei_s* gummeiInfo;
    struct mme_code_s* mmeCode;

    for (mme = RB_MIN(ngap_mme_map, &instance->ngap_mme_head); mme; mme = mmeNext) {
      mmeNext = RB_NEXT(ngap_mme_map, &instance->ngap_mme_head, mme);
      RB_REMOVE(ngap_mme_map, &instance->ngap_mme_head, mme);
      while (!STAILQ_EMPTY(&mme->served_gummei)) {
        gummeiInfo = STAILQ_FIRST(&mme->served_gummei);
        STAILQ_REMOVE_HEAD(&mme->served_gummei, next);
	
        while (!STAILQ_EMPTY(&gummeiInfo->served_plmns)) {
	  plmnInfo = STAILQ_FIRST(&gummeiInfo->served_plmns);
	  STAILQ_REMOVE_HEAD(&gummeiInfo->served_plmns, next);
	  free(plmnInfo);
        }
        while (!STAILQ_EMPTY(&gummeiInfo->served_group_ids)) {
	  groupInfo = STAILQ_FIRST(&gummeiInfo->served_group_ids);
	  STAILQ_REMOVE_HEAD(&gummeiInfo->served_group_ids, next);
	  free(groupInfo);
        }
        while (!STAILQ_EMPTY(&gummeiInfo->mme_codes)) {
	  mmeCode = STAILQ_FIRST(&gummeiInfo->mme_codes);
	  STAILQ_REMOVE_HEAD(&gummeiInfo->mme_codes, next);
	  free(mmeCode);
        }
        free(gummeiInfo);
      }
      free(mme);
    }
}
