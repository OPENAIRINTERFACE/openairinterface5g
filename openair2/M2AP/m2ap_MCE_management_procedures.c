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

/*! \file m2ap_MCE_management_procedures.c
 * \brief m2ap tasks for MCE
 * \author Javier Morgade  <javier.morade@ieee.org>
 * \date 2018
 * \version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "intertask_interface.h"

#include "assertions.h"
#include "conversions.h"

#include "m2ap_common.h"
#include "m2ap_MCE_defs.h"
#include "m2ap_MCE.h"


#define M2AP_DEBUG_LIST
#ifdef M2AP_DEBUG_LIST
#  define M2AP_MCE_LIST_OUT(x, args...) M2AP_DEBUG("[MCE]%*s"x"\n", 4*indent, "", ##args)
#else
#  define M2AP_MCE_LIST_OUT(x, args...)
#endif

static int                  indent = 0;


m2ap_MCE_internal_data_t m2ap_MCE_internal_data;

RB_GENERATE(m2ap_mce_map, m2ap_MCE_data_s, entry, m2ap_MCE_compare_assoc_id);

int m2ap_MCE_compare_assoc_id(
  struct m2ap_MCE_data_s *p1, struct m2ap_MCE_data_s *p2)
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

uint16_t m2ap_MCE_fetch_add_global_cnx_id(void)
{
  return ++m2ap_MCE_internal_data.global_cnx_id;
}

void m2ap_MCE_prepare_internal_data(void)
{
  memset(&m2ap_MCE_internal_data, 0, sizeof(m2ap_MCE_internal_data));
  STAILQ_INIT(&m2ap_MCE_internal_data.m2ap_MCE_instances_head);
}

void m2ap_MCE_insert_new_instance(m2ap_MCE_instance_t *new_instance_p)
{
  DevAssert(new_instance_p != NULL);

  STAILQ_INSERT_TAIL(&m2ap_MCE_internal_data.m2ap_MCE_instances_head,
                     new_instance_p, m2ap_MCE_entries);
}

void dump_mce_tree_m2(m2ap_MCE_data_t *t)
{
  if (t == NULL) return;
  printf("-----------------------\n");
  printf("MCE id %d %s\n", t->MCE_id, t->MCE_name);
  printf("state %d\n", t->state);
  printf("nextstream %d\n", t->nextstream);
  printf("in_streams %d out_streams %d\n", t->in_streams, t->out_streams);
  printf("cnx_id %d assoc_id %d\n", t->cnx_id, t->assoc_id);
  dump_mce_tree_m2(t->entry.rbe_left);
  dump_mce_tree_m2(t->entry.rbe_right);
}

void dump_mce_trees_m2(void)
{
m2ap_MCE_instance_t *zz;
STAILQ_FOREACH(zz, &m2ap_MCE_internal_data.m2ap_MCE_instances_head,
               m2ap_MCE_entries) {
//printf("here comes the tree (instance %d):\n---------------------------------------------\n", zz->instance);
dump_mce_tree_m2(zz->m2ap_mce_head.rbh_root);
//printf("---------------------------------------------\n");
}
}

struct m2ap_MCE_data_s *m2ap_get_MCE(m2ap_MCE_instance_t *instance_p, sctp_assoc_t assoc_id, uint16_t cnx_id)
{
  struct m2ap_MCE_data_s  temp;
  struct m2ap_MCE_data_s *found;

//printf("m2ap_get_MCE at 1 (looking for assoc_id %d cnx_id %d)\n", assoc_id, cnx_id);
//dump_mce_trees_m2();

  memset(&temp, 0, sizeof(struct m2ap_MCE_data_s));

  temp.assoc_id = assoc_id;
  temp.cnx_id   = cnx_id;

  if (instance_p == NULL) {
    STAILQ_FOREACH(instance_p, &m2ap_MCE_internal_data.m2ap_MCE_instances_head,
                   m2ap_MCE_entries) {
      found = RB_FIND(m2ap_mce_map, &instance_p->m2ap_mce_head, &temp);

      if (found != NULL) {
        return found;
      }
    }
  } else {
    return RB_FIND(m2ap_mce_map, &instance_p->m2ap_mce_head, &temp);
  }

  return NULL;
}


m2ap_MCE_instance_t *m2ap_MCE_get_instance(instance_t instance)
{
  m2ap_MCE_instance_t *temp = NULL;

  STAILQ_FOREACH(temp, &m2ap_MCE_internal_data.m2ap_MCE_instances_head,
                 m2ap_MCE_entries) {
    if (temp->instance == instance) {
      /* Matching occurence */
      return temp;
    }
  }

  return NULL;
}

/// utility functions

void m2ap_dump_MCE (m2ap_MCE_data_t  * MCE_ref);

void
m2ap_dump_MCE_list (void) {
   m2ap_MCE_instance_t *inst = NULL;
   struct m2ap_MCE_data_s *found = NULL;
   struct m2ap_MCE_data_s temp;

   memset(&temp, 0, sizeof(struct m2ap_MCE_data_s));

  STAILQ_FOREACH (inst, &m2ap_MCE_internal_data.m2ap_MCE_instances_head,  m2ap_MCE_entries) {
    found = RB_FIND(m2ap_mce_map, &inst->m2ap_mce_head, &temp);
    m2ap_dump_MCE (found);
  }
}

void m2ap_dump_MCE (m2ap_MCE_data_t  * MCE_ref) {

  if (MCE_ref == NULL) {
    return;
  }

  M2AP_MCE_LIST_OUT ("");
  M2AP_MCE_LIST_OUT ("MCE name:          %s", MCE_ref->MCE_name == NULL ? "not present" : MCE_ref->MCE_name);
  M2AP_MCE_LIST_OUT ("MCE STATE:         %07x", MCE_ref->state);
  M2AP_MCE_LIST_OUT ("MCE ID:            %07x", MCE_ref->MCE_id);
  indent++;
  M2AP_MCE_LIST_OUT ("SCTP cnx id:     %d", MCE_ref->cnx_id);
  M2AP_MCE_LIST_OUT ("SCTP assoc id:     %d", MCE_ref->assoc_id);
  M2AP_MCE_LIST_OUT ("SCTP instreams:    %d", MCE_ref->in_streams);
  M2AP_MCE_LIST_OUT ("SCTP outstreams:   %d", MCE_ref->out_streams);
  indent--;
}

m2ap_MCE_data_t  * m2ap_is_MCE_pci_in_list (const uint32_t pci)
{
  m2ap_MCE_instance_t    *inst;
  struct m2ap_MCE_data_s *elm;

  STAILQ_FOREACH(inst, &m2ap_MCE_internal_data.m2ap_MCE_instances_head, m2ap_MCE_entries) {
    RB_FOREACH(elm, m2ap_mce_map, &inst->m2ap_mce_head) {
      for (int i = 0; i<elm->num_cc; i++) {
        if (elm->Nid_cell[i] == pci) {
          return elm;
        }
      }
    }
  }
  return NULL;
}

m2ap_MCE_data_t  * m2ap_is_MCE_id_in_list (const uint32_t MCE_id)
{
  m2ap_MCE_instance_t    *inst;
  struct m2ap_MCE_data_s *elm;

  STAILQ_FOREACH(inst, &m2ap_MCE_internal_data.m2ap_MCE_instances_head, m2ap_MCE_entries) {
    RB_FOREACH(elm, m2ap_mce_map, &inst->m2ap_mce_head) {
      if (elm->MCE_id == MCE_id)
        return elm;
    }
  }
  return NULL;
}

m2ap_MCE_data_t  * m2ap_is_MCE_assoc_id_in_list (const uint32_t sctp_assoc_id)
{
  m2ap_MCE_instance_t    *inst;
  struct m2ap_MCE_data_s *found;
  struct m2ap_MCE_data_s temp;

  temp.assoc_id = sctp_assoc_id;
  temp.cnx_id = -1;

  STAILQ_FOREACH(inst, &m2ap_MCE_internal_data.m2ap_MCE_instances_head, m2ap_MCE_entries) {
    found = RB_FIND(m2ap_mce_map, &inst->m2ap_mce_head, &temp);
    if (found != NULL){
      if (found->assoc_id == sctp_assoc_id) {
	return found;
      }
    }
  }
  return NULL;
}
